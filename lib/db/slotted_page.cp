// Origin: Pure-CP Stdlib (Database Storage Engine Slotted-Page Binary Layout, In-Place Compaction & CRC32 Integrity) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation PostgreSQL/SQLite-style slotted page engine:
//   - Binary little-endian layout operating directly on caller-owned `page_buf: *[]u8` (4KB/8KB/any 64..65535B)
//   - Fixed 32-byte header (magic "CPS1", page_id, lsn, flags, slot_count, free_start, free_end, fragmented_bytes, crc32)
//   - Slot directory growing upward from offset 32 (6 bytes/slot: offset, len, flags)
//   - Record payload area growing downward from page end with zero-copy `getRecord`
//   - O(1)-extra-memory in-place compaction (`compactPage`), tombstone slot reuse & automatic compaction on insert/update
//   - Full page CRC32 checksum (`sealChecksum`) & deep structural verification (`verifyPageIntegrity`)

// ============================================================================
// Constants
// ============================================================================

/// 4-byte ASCII magic `"CPS1"` stored in little-endian (`'C','P','S','1'`).
pub const SLOTTED_PAGE_MAGIC: u32 = 0x31535043;

/// Fixed slotted page header size in bytes.
pub const SLOTTED_PAGE_HEADER_SIZE: usize = 32;

/// Fixed size of each slot directory entry in bytes (`offset: u16, len: u16, flags: u16`).
pub const SLOTTED_SLOT_SIZE: usize = 6;

/// Minimum valid page size in bytes.
pub const SLOTTED_PAGE_MIN_SIZE: usize = 64;

/// Maximum valid page size in bytes (bounded by 16-bit intra-page offsets).
pub const SLOTTED_PAGE_MAX_SIZE: usize = 65535;

/// Slot flag: record is live and readable.
pub const SLOT_FLAG_ALIVE: u16 = 0x0001;

/// Slot flag: record has been deleted (tombstone eligible for slot reuse).
pub const SLOT_FLAG_DELETED: u16 = 0x0002;

const HDR_OFF_MAGIC: usize = 0;
const HDR_OFF_PAGE_ID: usize = 4;
const HDR_OFF_LSN: usize = 8;
const HDR_OFF_FLAGS: usize = 16;
const HDR_OFF_SLOT_COUNT: usize = 18;
const HDR_OFF_FREE_START: usize = 20;
const HDR_OFF_FREE_END: usize = 22;
const HDR_OFF_FRAG_BYTES: usize = 24;
const HDR_OFF_RESERVED: usize = 26;
const HDR_OFF_CRC32: usize = 28;

// ============================================================================
// Internal Little-Endian & CRC32 Helpers
// ============================================================================

fn readU16Le(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn writeU16Le(buf: *[]u8, off: usize, v: u16) {
    buf[off] = (v & 0xff) as u8;
    buf[off + 1] = ((v >> 8) & 0xff) as u8;
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn writeU32Le(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xff) as u8;
    buf[off + 1] = ((v >> 8) & 0xff) as u8;
    buf[off + 2] = ((v >> 16) & 0xff) as u8;
    buf[off + 3] = ((v >> 24) & 0xff) as u8;
}

fn readU64Le(buf: []const u8, off: usize): u64 {
    const lo: u64 = readU32Le(buf, off) as u64;
    const hi: u64 = readU32Le(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn writeU64Le(buf: *[]u8, off: usize, v: u64) {
    writeU32Le(buf, off, (v & 0xffffffff) as u32);
    writeU32Le(buf, off + 4, ((v >> 32) & 0xffffffff) as u32);
}

fn crc32UpdateByte(crc_in: u32, byte: u8): u32 {
    let crc: u32 = crc_in ^ (byte as u32);
    let b: usize = 0;
    while (b < 8) : (b += 1) {
        if ((crc & 1) != 0) {
            crc = (crc >> 1) ^ 0xEDB88320;
        } else {
            crc = crc >> 1;
        }
    }
    return crc;
}

/// Computes IEEE CRC32 over `page_buf`, skipping the 4-byte checksum field at `[28..32]`.
pub fn slottedPageCrc32(page_buf: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < page_buf.len) : (i += 1) {
        if (i >= HDR_OFF_CRC32 && i < HDR_OFF_CRC32 + 4) {
            continue;
        }
        crc = crc32UpdateByte(crc, page_buf[i]);
    }
    return crc ^ 0xFFFFFFFF;
}

fn isValidPageSize(len: usize): bool {
    return len >= SLOTTED_PAGE_MIN_SIZE && len <= SLOTTED_PAGE_MAX_SIZE;
}

fn slotDirOffset(slot_id: usize): usize {
    return SLOTTED_PAGE_HEADER_SIZE + slot_id * SLOTTED_SLOT_SIZE;
}

fn hasValidHeaderBasics(page_buf: []const u8): bool {
    if (!isValidPageSize(page_buf.len)) return false;
    if (readU32Le(page_buf, HDR_OFF_MAGIC) != SLOTTED_PAGE_MAGIC) return false;
    const slot_count: usize = readU16Le(page_buf, HDR_OFF_SLOT_COUNT) as usize;
    const free_start: usize = readU16Le(page_buf, HDR_OFF_FREE_START) as usize;
    const free_end: usize = readU16Le(page_buf, HDR_OFF_FREE_END) as usize;
    const expected_free_start: usize = SLOTTED_PAGE_HEADER_SIZE + slot_count * SLOTTED_SLOT_SIZE;
    if (free_start != expected_free_start) return false;
    if (free_start > free_end || free_end > page_buf.len) return false;
    return true;
}

// ============================================================================
// 1. Enums & Metadata Structs
// ============================================================================

/// Result of `verifyPageIntegrity(page_buf)`.
pub enum SlottedPageStatus: u8 {
    ok = 0,
    invalid_page_size = 1,
    invalid_magic = 2,
    checksum_mismatch = 3,
    corrupt_header = 4,
    corrupt_slot_bounds = 5,
    overlapping_records = 6,
}

/// Decoded 32-byte slotted page header.
pub struct SlottedPageHeader {
    page_id: u32 = 0;
    lsn: u64 = 0;
    flags: u16 = 0;
    slot_count: u16 = 0;
    free_start: u16 = 0;
    free_end: u16 = 0;
    fragmented_bytes: u16 = 0;
    crc32: u32 = 0;
}

/// Decoded 6-byte slot directory entry.
pub struct SlottedSlotMeta {
    offset: u16 = 0;
    len: u16 = 0;
    flags: u16 = 0;

    fn isAlive(): bool {
        return (_.flags & SLOT_FLAG_ALIVE) != 0 && (_.flags & SLOT_FLAG_DELETED) == 0;
    }

    fn isDeleted(): bool {
        return (_.flags & SLOT_FLAG_DELETED) != 0;
    }
}

/// Free-space accounting for a slotted page.
pub struct SlottedFreeSpace {
    /// Immediately usable contiguous gap `free_end - free_start`.
    contiguous_bytes: usize = 0;
    /// Dead payload bytes reclaimable via `compactPage`.
    fragmented_bytes: usize = 0;
    /// Total free payload bytes after compaction (`contiguous + fragmented`).
    total_reclaimable_bytes: usize = 0;
    /// Number of deleted tombstone slots available for zero-overhead slot reuse.
    reusable_slots: usize = 0;
}

// ============================================================================
// 2. Page Initialization, Inspection & Checksum Operations
// ============================================================================

/// Formats `page_buf` as a fresh empty slotted page with `page_id`, `lsn`, `flags`, and initial CRC32.
/// Returns `false` if `page_buf.len < 64` or `page_buf.len > 65535`.
pub fn initPage(page_buf: *[]u8, page_id: u32, lsn: u64, flags: u16): bool {
    if (!isValidPageSize(page_buf.len)) return false;

    let i: usize = 0;
    while (i < page_buf.len) : (i += 1) {
        page_buf[i] = 0;
    }

    writeU32Le(page_buf, HDR_OFF_MAGIC, SLOTTED_PAGE_MAGIC);
    writeU32Le(page_buf, HDR_OFF_PAGE_ID, page_id);
    writeU64Le(page_buf, HDR_OFF_LSN, lsn);
    writeU16Le(page_buf, HDR_OFF_FLAGS, flags);
    writeU16Le(page_buf, HDR_OFF_SLOT_COUNT, 0);
    writeU16Le(page_buf, HDR_OFF_FREE_START, SLOTTED_PAGE_HEADER_SIZE as u16);
    writeU16Le(page_buf, HDR_OFF_FREE_END, page_buf.len as u16);
    writeU16Le(page_buf, HDR_OFF_FRAG_BYTES, 0);
    writeU16Le(page_buf, HDR_OFF_RESERVED, 0);

    const crc: u32 = slottedPageCrc32(page_buf[0..]);
    writeU32Le(page_buf, HDR_OFF_CRC32, crc);
    return true;
}

/// Decodes the 32-byte header of `page_buf`. Returns `null` if size or magic is invalid.
pub fn readPageHeader(page_buf: []const u8): SlottedPageHeader? {
    if (!isValidPageSize(page_buf.len)) return null;
    if (readU32Le(page_buf, HDR_OFF_MAGIC) != SLOTTED_PAGE_MAGIC) return null;
    return SlottedPageHeader{
        page_id: readU32Le(page_buf, HDR_OFF_PAGE_ID),
        lsn: readU64Le(page_buf, HDR_OFF_LSN),
        flags: readU16Le(page_buf, HDR_OFF_FLAGS),
        slot_count: readU16Le(page_buf, HDR_OFF_SLOT_COUNT),
        free_start: readU16Le(page_buf, HDR_OFF_FREE_START),
        free_end: readU16Le(page_buf, HDR_OFF_FREE_END),
        fragmented_bytes: readU16Le(page_buf, HDR_OFF_FRAG_BYTES),
        crc32: readU32Le(page_buf, HDR_OFF_CRC32),
    };
}

/// Reads metadata for `slot_id`. Returns `null` if header is invalid or `slot_id >= slot_count`.
pub fn readSlotMeta(page_buf: []const u8, slot_id: u16): SlottedSlotMeta? {
    if (!hasValidHeaderBasics(page_buf)) return null;
    const slot_count: u16 = readU16Le(page_buf, HDR_OFF_SLOT_COUNT);
    if (slot_id >= slot_count) return null;
    const off: usize = slotDirOffset(slot_id as usize);
    return SlottedSlotMeta{
        offset: readU16Le(page_buf, off),
        len: readU16Le(page_buf, off + 2),
        flags: readU16Le(page_buf, off + 4),
    };
}

/// Updates the page header `lsn` field.
pub fn setPageLsn(page_buf: *[]u8, lsn: u64): bool {
    if (!hasValidHeaderBasics(page_buf[0..])) return false;
    writeU64Le(page_buf, HDR_OFF_LSN, lsn);
    return true;
}

/// Computes and writes the IEEE CRC32 checksum into header bytes `[28..32]`.
pub fn sealChecksum(page_buf: *[]u8): u32? {
    if (!hasValidHeaderBasics(page_buf[0..])) return null;
    const crc: u32 = slottedPageCrc32(page_buf[0..]);
    writeU32Le(page_buf, HDR_OFF_CRC32, crc);
    return crc;
}

/// Performs deep integrity verification on `page_buf`:
///   1. Page size in `[64, 65535]`
///   2. Magic == `"CPS1"`
///   3. Stored CRC32 matches recomputed CRC32
///   4. Header pointer invariants (`free_start == 32 + slot_count * 6 <= free_end <= page_len`)
///   5. Every live slot lies strictly within `[free_end, page_len]` with `len > 0`
///   6. No two live records overlap in the payload region
pub fn verifyPageIntegrity(page_buf: []const u8): SlottedPageStatus {
    if (!isValidPageSize(page_buf.len)) return SlottedPageStatus.invalid_page_size;
    if (readU32Le(page_buf, HDR_OFF_MAGIC) != SLOTTED_PAGE_MAGIC) return SlottedPageStatus.invalid_magic;

    const stored_crc: u32 = readU32Le(page_buf, HDR_OFF_CRC32);
    const calc_crc: u32 = slottedPageCrc32(page_buf);
    if (stored_crc != calc_crc) return SlottedPageStatus.checksum_mismatch;

    const slot_count: usize = readU16Le(page_buf, HDR_OFF_SLOT_COUNT) as usize;
    const free_start: usize = readU16Le(page_buf, HDR_OFF_FREE_START) as usize;
    const free_end: usize = readU16Le(page_buf, HDR_OFF_FREE_END) as usize;
    const expected_free_start: usize = SLOTTED_PAGE_HEADER_SIZE + slot_count * SLOTTED_SLOT_SIZE;
    if (free_start != expected_free_start || free_start > free_end || free_end > page_buf.len) {
        return SlottedPageStatus.corrupt_header;
    }

    // Validate bounds and pairwise non-overlap of all live slots.
    let i: usize = 0;
    while (i < slot_count) : (i += 1) {
        const si_off: usize = slotDirOffset(i);
        const off_i: usize = readU16Le(page_buf, si_off) as usize;
        const len_i: usize = readU16Le(page_buf, si_off + 2) as usize;
        const flg_i: u16 = readU16Le(page_buf, si_off + 4);
        const alive_i: bool = (flg_i & SLOT_FLAG_ALIVE) != 0 && (flg_i & SLOT_FLAG_DELETED) == 0;
        if (!alive_i) continue;

        if (len_i == 0 || off_i < free_end || off_i + len_i > page_buf.len) {
            return SlottedPageStatus.corrupt_slot_bounds;
        }

        let j: usize = i + 1;
        while (j < slot_count) : (j += 1) {
            const sj_off: usize = slotDirOffset(j);
            const flg_j: u16 = readU16Le(page_buf, sj_off + 4);
            const alive_j: bool = (flg_j & SLOT_FLAG_ALIVE) != 0 && (flg_j & SLOT_FLAG_DELETED) == 0;
            if (!alive_j) continue;

            const off_j: usize = readU16Le(page_buf, sj_off) as usize;
            const len_j: usize = readU16Le(page_buf, sj_off + 2) as usize;
            if (len_j == 0 || off_j < free_end || off_j + len_j > page_buf.len) {
                return SlottedPageStatus.corrupt_slot_bounds;
            }

            // Intervals [off_i, off_i + len_i) and [off_j, off_j + len_j) overlap iff:
            if (off_i < off_j + len_j && off_j < off_i + len_i) {
                return SlottedPageStatus.overlapping_records;
            }
        }
    }

    return SlottedPageStatus.ok;
}

/// Computes contiguous free space, reclaimable fragmented bytes, and reusable tombstone slot count.
pub fn freeSpaceBytes(page_buf: []const u8): SlottedFreeSpace? {
    if (!hasValidHeaderBasics(page_buf)) return null;
    const slot_count: usize = readU16Le(page_buf, HDR_OFF_SLOT_COUNT) as usize;
    const free_start: usize = readU16Le(page_buf, HDR_OFF_FREE_START) as usize;
    const free_end: usize = readU16Le(page_buf, HDR_OFF_FREE_END) as usize;
    const frag: usize = readU16Le(page_buf, HDR_OFF_FRAG_BYTES) as usize;

    let reusable: usize = 0;
    let i: usize = 0;
    while (i < slot_count) : (i += 1) {
        const flg: u16 = readU16Le(page_buf, slotDirOffset(i) + 4);
        if ((flg & SLOT_FLAG_DELETED) != 0) {
            reusable += 1;
        }
    }

    const contig: usize = free_end - free_start;
    return SlottedFreeSpace{
        contiguous_bytes: contig,
        fragmented_bytes: frag,
        total_reclaimable_bytes: contig + frag,
        reusable_slots: reusable,
    };
}

// ============================================================================
// 3. O(1)-Extra-Memory In-Place Compaction & CRUD Operations
// ============================================================================

/// Compacts all live records tightly against the end of `page_buf` in-place with O(1) extra memory.
/// Preserves all `slot_id`s, reclaims all `fragmented_bytes`, and updates `free_end`.
pub fn compactPage(page_buf: *[]u8): bool {
    if (!hasValidHeaderBasics(page_buf[0..])) return false;

    const slot_count: usize = readU16Le(page_buf[0..], HDR_OFF_SLOT_COUNT) as usize;
    let write_end: usize = page_buf.len;
    let upper_bound: usize = page_buf.len + 1;

    // Repeatedly pick the live slot with the highest `offset < upper_bound` and shift it right
    // against `write_end`. Because all live intervals are disjoint, `new_off = write_end - len >= old_off`,
    // so copying bytes from high to low is strictly overlap-safe with zero heap or stack temp buffers.
    while (true) {
        let best_slot: usize = slot_count;
        let best_off: usize = 0;
        let best_len: usize = 0;

        let i: usize = 0;
        while (i < slot_count) : (i += 1) {
            const s_off: usize = slotDirOffset(i);
            const flg: u16 = readU16Le(page_buf[0..], s_off + 4);
            const alive: bool = (flg & SLOT_FLAG_ALIVE) != 0 && (flg & SLOT_FLAG_DELETED) == 0;
            if (!alive) continue;

            const r_off: usize = readU16Le(page_buf[0..], s_off) as usize;
            const r_len: usize = readU16Le(page_buf[0..], s_off + 2) as usize;
            if (r_off < upper_bound) {
                if (best_slot == slot_count || r_off > best_off) {
                    best_slot = i;
                    best_off = r_off;
                    best_len = r_len;
                }
            }
        }

        if (best_slot == slot_count) break;
        if (best_len > write_end || best_off + best_len > page_buf.len) return false;

        const new_off: usize = write_end - best_len;
        if (new_off != best_off) {
            if (new_off > best_off) {
                // Reverse copy prevents overlap clobbering when shifting toward higher addresses.
                let k: usize = best_len;
                while (k > 0) : (k -= 1) {
                    page_buf[new_off + k - 1] = page_buf[best_off + k - 1];
                }
            } else {
                let k: usize = 0;
                while (k < best_len) : (k += 1) {
                    page_buf[new_off + k] = page_buf[best_off + k];
                }
            }
            writeU16Le(page_buf, slotDirOffset(best_slot), new_off as u16);
        }

        write_end = new_off;
        upper_bound = best_off;
    }

    writeU16Le(page_buf, HDR_OFF_FREE_END, write_end as u16);
    writeU16Le(page_buf, HDR_OFF_FRAG_BYTES, 0);
    return true;
}

/// Inserts `record` into `page_buf`, reusing a deleted tombstone slot when available and
/// automatically triggering `compactPage` if contiguous space is insufficient but fragmented space suffices.
/// Returns the assigned `slot_id`, or `null` if `record.len == 0`, page is invalid, or page is full.
pub fn insertRecord(page_buf: *[]u8, record: []const u8): u16? {
    if (record.len == 0) return null;
    if (!hasValidHeaderBasics(page_buf[0..])) return null;

    const slot_count: usize = readU16Le(page_buf[0..], HDR_OFF_SLOT_COUNT) as usize;
    let free_start: usize = readU16Le(page_buf[0..], HDR_OFF_FREE_START) as usize;
    let free_end: usize = readU16Le(page_buf[0..], HDR_OFF_FREE_END) as usize;
    const frag: usize = readU16Le(page_buf[0..], HDR_OFF_FRAG_BYTES) as usize;

    // Search for a reusable deleted tombstone slot first.
    let reuse_slot: usize = slot_count;
    let i: usize = 0;
    while (i < slot_count) : (i += 1) {
        const flg: u16 = readU16Le(page_buf[0..], slotDirOffset(i) + 4);
        if ((flg & SLOT_FLAG_DELETED) != 0) {
            reuse_slot = i;
            break;
        }
    }

    const slot_overhead: usize = if (reuse_slot < slot_count) 0 else SLOTTED_SLOT_SIZE;
    const needed: usize = record.len + slot_overhead;
    const contig: usize = free_end - free_start;

    if (contig < needed) {
        if (contig + frag < needed) return null;
        if (!compactPage(page_buf)) return null;
        free_start = readU16Le(page_buf[0..], HDR_OFF_FREE_START) as usize;
        free_end = readU16Le(page_buf[0..], HDR_OFF_FREE_END) as usize;
        if (free_end - free_start < needed) return null;
    }

    const new_off: usize = free_end - record.len;
    let k: usize = 0;
    while (k < record.len) : (k += 1) {
        page_buf[new_off + k] = record[k];
    }

    const target_slot: usize = reuse_slot;
    const s_off: usize = slotDirOffset(target_slot);
    writeU16Le(page_buf, s_off, new_off as u16);
    writeU16Le(page_buf, s_off + 2, record.len as u16);
    writeU16Le(page_buf, s_off + 4, SLOT_FLAG_ALIVE);

    if (reuse_slot == slot_count) {
        writeU16Le(page_buf, HDR_OFF_SLOT_COUNT, (slot_count + 1) as u16);
        writeU16Le(page_buf, HDR_OFF_FREE_START, (free_start + SLOTTED_SLOT_SIZE) as u16);
    }
    writeU16Le(page_buf, HDR_OFF_FREE_END, new_off as u16);
    return target_slot as u16;
}

/// Zero-copy lookup of the live record stored at `slot_id`.
/// Returns `null` if page is invalid, `slot_id` is out of bounds, or the slot is deleted.
pub fn getRecord(page_buf: []const u8, slot_id: u16): ([]const u8)? {
    if (!hasValidHeaderBasics(page_buf)) return null;
    const slot_count: u16 = readU16Le(page_buf, HDR_OFF_SLOT_COUNT);
    if (slot_id >= slot_count) return null;

    const s_off: usize = slotDirOffset(slot_id as usize);
    const r_off: usize = readU16Le(page_buf, s_off) as usize;
    const r_len: usize = readU16Le(page_buf, s_off + 2) as usize;
    const flg: u16 = readU16Le(page_buf, s_off + 4);
    if ((flg & SLOT_FLAG_ALIVE) == 0 || (flg & SLOT_FLAG_DELETED) != 0) return null;

    const free_end: usize = readU16Le(page_buf, HDR_OFF_FREE_END) as usize;
    if (r_len == 0 || r_off < free_end || r_off + r_len > page_buf.len) return null;
    return page_buf[r_off .. r_off + r_len];
}

/// Updates an existing live record at `slot_id` with `new_record`.
///
/// - When `new_record.len <= old_len`: overwrites in-place and adds any shrunk bytes to `fragmented_bytes`.
/// - When `new_record.len > old_len`: atomically checks space (compacting if reclaimable space suffices),
///   relocates the record within the page, and preserves `slot_id`. If space is insufficient, leaves
///   the original record intact and returns `false`.
pub fn updateRecordInPlace(page_buf: *[]u8, slot_id: u16, new_record: []const u8): bool {
    if (new_record.len == 0) return false;
    if (!hasValidHeaderBasics(page_buf[0..])) return false;

    const slot_count: u16 = readU16Le(page_buf[0..], HDR_OFF_SLOT_COUNT);
    if (slot_id >= slot_count) return false;

    const s_off: usize = slotDirOffset(slot_id as usize);
    const old_off: usize = readU16Le(page_buf[0..], s_off) as usize;
    const old_len: usize = readU16Le(page_buf[0..], s_off + 2) as usize;
    const flg: u16 = readU16Le(page_buf[0..], s_off + 4);
    if ((flg & SLOT_FLAG_ALIVE) == 0 || (flg & SLOT_FLAG_DELETED) != 0) return false;

    // Fast path: same-size or shrinking update overwrites in-place.
    if (new_record.len <= old_len) {
        let k: usize = 0;
        while (k < new_record.len) : (k += 1) {
            page_buf[old_off + k] = new_record[k];
        }
        writeU16Le(page_buf, s_off + 2, new_record.len as u16);
        if (new_record.len < old_len) {
            const frag: usize = readU16Le(page_buf[0..], HDR_OFF_FRAG_BYTES) as usize;
            writeU16Le(page_buf, HDR_OFF_FRAG_BYTES, (frag + (old_len - new_record.len)) as u16);
        }
        return true;
    }

    // Growing update: verify total available capacity (including old_len once freed) before mutating.
    let free_start: usize = readU16Le(page_buf[0..], HDR_OFF_FREE_START) as usize;
    let free_end: usize = readU16Le(page_buf[0..], HDR_OFF_FREE_END) as usize;
    const frag: usize = readU16Le(page_buf[0..], HDR_OFF_FRAG_BYTES) as usize;
    const contig: usize = free_end - free_start;

    if (contig + frag + old_len < new_record.len) {
        return false;
    }

    // Temporarily release the old payload by marking slot deleted so compaction reclaims `old_len` too.
    if (contig < new_record.len) {
        writeU16Le(page_buf, s_off + 4, SLOT_FLAG_DELETED);
        writeU16Le(page_buf, HDR_OFF_FRAG_BYTES, (frag + old_len) as u16);
        if (!compactPage(page_buf)) {
            // Should never happen after capacity pre-check, restore flags defensively.
            writeU16Le(page_buf, s_off + 4, SLOT_FLAG_ALIVE);
            return false;
        }
        free_start = readU16Le(page_buf[0..], HDR_OFF_FREE_START) as usize;
        free_end = readU16Le(page_buf[0..], HDR_OFF_FREE_END) as usize;
    } else {
        // Enough contiguous space without compaction; old payload becomes fragmented.
        writeU16Le(page_buf, HDR_OFF_FRAG_BYTES, (frag + old_len) as u16);
    }

    const new_off: usize = free_end - new_record.len;
    let k: usize = 0;
    while (k < new_record.len) : (k += 1) {
        page_buf[new_off + k] = new_record[k];
    }

    writeU16Le(page_buf, s_off, new_off as u16);
    writeU16Le(page_buf, s_off + 2, new_record.len as u16);
    writeU16Le(page_buf, s_off + 4, SLOT_FLAG_ALIVE);
    writeU16Le(page_buf, HDR_OFF_FREE_END, new_off as u16);
    return true;
}

/// Marks `slot_id` as deleted (tombstone), adding its payload length to `fragmented_bytes`.
/// Returns `false` if page is invalid, `slot_id` is out of bounds, or slot is already deleted.
pub fn deleteRecord(page_buf: *[]u8, slot_id: u16): bool {
    if (!hasValidHeaderBasics(page_buf[0..])) return false;
    const slot_count: u16 = readU16Le(page_buf[0..], HDR_OFF_SLOT_COUNT);
    if (slot_id >= slot_count) return false;

    const s_off: usize = slotDirOffset(slot_id as usize);
    const old_len: usize = readU16Le(page_buf[0..], s_off + 2) as usize;
    const flg: u16 = readU16Le(page_buf[0..], s_off + 4);
    if ((flg & SLOT_FLAG_ALIVE) == 0 || (flg & SLOT_FLAG_DELETED) != 0) return false;

    writeU16Le(page_buf, s_off, 0);
    writeU16Le(page_buf, s_off + 2, 0);
    writeU16Le(page_buf, s_off + 4, SLOT_FLAG_DELETED);

    const frag: usize = readU16Le(page_buf[0..], HDR_OFF_FRAG_BYTES) as usize;
    writeU16Le(page_buf, HDR_OFF_FRAG_BYTES, (frag + old_len) as u16);
    return true;
}
