// Origin: Pure-CP Stdlib (Zero-Alloc Microsoft VHDX 1.0 Virtual Hard Disk Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// VHDX 1.0 Protocol Constants (MS-VHDX Specification)
// ---------------------------------------------------------------------------

pub const VHDX_FILE_ID_SIG: []const u8 = "vhdxfile";
pub const VHDX_HEADER_SIG: []const u8 = "head";
pub const VHDX_REGION_SIG: []const u8 = "regi";
pub const VHDX_METADATA_SIG: []const u8 = "metadata";

pub const VHDX_HEADER_SIZE: usize = 4096;
pub const VHDX_REGION_SIZE: usize = 65536;

pub const VHDX_HEADER1_OFFSET: usize = 65536;
pub const VHDX_HEADER2_OFFSET: usize = 131072;
pub const VHDX_REGION1_OFFSET: usize = 196608;
pub const VHDX_REGION2_OFFSET: usize = 262144;

pub const VHDX_FILE_ID_CREATOR_OFFSET: usize = 8;
pub const VHDX_FILE_ID_CREATOR_LEN: usize = 512;
pub const VHDX_MAX_REGION_ENTRIES: u32 = 2047;
pub const VHDX_REGION_ENTRY_SIZE: usize = 32;

/// Well-known BAT Region GUID: `2DC27766-F623-4200-9D64-115E9BFD4A08` (mixed-endian).
pub const REGION_GUID_BAT: [16]u8 = [
    0x66, 0x77, 0xC2, 0x2D,
    0x23, 0xF6,
    0x00, 0x42,
    0x9D, 0x64,
    0x11, 0x5E, 0x9B, 0xFD, 0x4A, 0x08,
];

/// Well-known Metadata Region GUID: `8B7CA206-4790-4B9A-B8FE-575F050F886E` (mixed-endian).
pub const REGION_GUID_METADATA: [16]u8 = [
    0x06, 0xA2, 0x7C, 0x8B,
    0x90, 0x47,
    0x9A, 0x4B,
    0xB8, 0xFE,
    0x57, 0x5F, 0x05, 0x0F, 0x88, 0x6E,
];

// Payload BAT Entry State (low 3 bits of 64-bit BAT entry)
pub const BAT_NOT_PRESENT: u8 = 0;
pub const BAT_UNDEFINED: u8 = 1;
pub const BAT_ZERO: u8 = 2;
pub const BAT_UNMAPPED: u8 = 3;
pub const BAT_FULLY_PRESENT: u8 = 6;
pub const BAT_PARTIALLY_PRESENT: u8 = 7;

// ---------------------------------------------------------------------------
// Internal Little-Endian & Byte Comparison Helpers
// ---------------------------------------------------------------------------

fn readU16Le(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return b0 | (b1 << 8);
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn readU64Le(buf: []const u8, off: usize): u64 {
    const lo = readU32Le(buf, off) as u64;
    const hi = readU32Le(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn readGuid16(buf: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        out[i] = buf[off + i];
        i += 1;
    }
    return out;
}

fn guidEq(a: [16]u8, b: [16]u8): bool {
    let i: usize = 0;
    while (i < 16) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn matchBytes(buf: []const u8, off: usize, sig: []const u8): bool {
    if (buf.len < off + sig.len) {
        return false;
    }
    let i: usize = 0;
    while (i < sig.len) {
        if (buf[off + i] != sig[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// CRC32C (Castagnoli, reflected polynomial 0x82F63B78)
// ---------------------------------------------------------------------------

const CRC32C_POLY: u32 = 0x82F63B78;

fn crc32cUpdateByte(crc_in: u32, b: u8): u32 {
    let crc: u32 = crc_in ^ (b as u32);
    let bit: usize = 0;
    while (bit < 8) {
        if ((crc & 1) != 0) {
            crc = (crc >> 1) ^ CRC32C_POLY;
        } else {
            crc = crc >> 1;
        }
        bit += 1;
    }
    return crc;
}

/// Computes the standard CRC32C (Castagnoli) checksum of `data`.
pub fn vhdxCrc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc32cUpdateByte(crc, data[i]);
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

/// Computes the CRC32C checksum of `data` while treating the 4-byte checksum field
/// at `[zero_offset .. zero_offset + 4]` as zero bytes, without mutating `data`.
pub fn vhdxCrc32cWithZeroField(data: []const u8, zero_offset: usize): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        const b: u8 = if (i >= zero_offset && i < zero_offset + 4) 0 else data[i];
        crc = crc32cUpdateByte(crc, b);
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// VHDX File Type Identifier (offset 0, signature `"vhdxfile"` + UTF-16LE creator).
pub struct VhdxFileId {
    creator_utf16le: []const u8,

    /// Decodes the null-terminated UTF-16LE creator string into ASCII bytes in `out`.
    /// Non-ASCII code units are replaced with `'?'`. Returns the number of bytes written.
    fn decodeCreatorAscii(out: []u8): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i + 1 < _.creator_utf16le.len && written < out.len) {
            const unit = readU16Le(_.creator_utf16le, i);
            if (unit == 0) {
                break;
            }
            out[written] = if (unit < 0x80) (unit as u8) else 0x3F;
            written += 1;
            i += 2;
        }
        return written;
    }
}

/// VHDX 4KB Header (offset 64KB / 128KB, signature `"head"`).
pub struct VhdxHeader {
    checksum: u32,
    sequence_number: u64,
    file_write_guid: [16]u8,
    data_write_guid: [16]u8,
    log_guid: [16]u8,
    log_version: u16,
    version: u16,
    log_length: u32,
    log_offset: u64,
}

/// A single 32-byte entry inside the VHDX Region Table.
pub struct VhdxRegionEntry {
    guid: [16]u8,
    file_offset: u64,
    length: u32,
    required: bool,

    fn isBat(): bool {
        return guidEq(_.guid, REGION_GUID_BAT);
    }

    fn isMetadata(): bool {
        return guidEq(_.guid, REGION_GUID_METADATA);
    }
}

/// Zero-copy iterator over `VhdxRegionEntry` records in a validated Region Table.
pub struct VhdxRegionIter {
    entries_bytes: []const u8,
    pos: usize,

    fn next(*_): VhdxRegionEntry? {
        if (_.pos + VHDX_REGION_ENTRY_SIZE > _.entries_bytes.len) {
            return null;
        }
        const off = _.pos;
        _.pos += VHDX_REGION_ENTRY_SIZE;
        return VhdxRegionEntry {
            guid: readGuid16(_.entries_bytes, off),
            file_offset: readU64Le(_.entries_bytes, off + 16),
            length: readU32Le(_.entries_bytes, off + 24),
            required: (readU32Le(_.entries_bytes, off + 28) & 1) != 0,
        };
    }
}

/// VHDX 64KB Region Table (offset 192KB / 256KB, signature `"regi"`).
pub struct VhdxRegionTable {
    checksum: u32,
    entry_count: u32,
    entries_bytes: []const u8,

    fn entries(): VhdxRegionIter {
        return VhdxRegionIter {
            entries_bytes: _.entries_bytes,
            pos: 0,
        };
    }

    fn findByGuid(target: [16]u8): VhdxRegionEntry? {
        let iter = _.entries();
        while (entry := iter.next()) {
            if (guidEq(entry.guid, target)) {
                return entry;
            }
        }
        return null;
    }

    fn findBat(): VhdxRegionEntry? {
        return _.findByGuid(REGION_GUID_BAT);
    }

    fn findMetadata(): VhdxRegionEntry? {
        return _.findByGuid(REGION_GUID_METADATA);
    }
}

/// Decoded 64-bit VHDX Block Allocation Table (BAT) entry.
pub struct VhdxBatEntry {
    state: u8,
    file_offset_mb: u64,

    fn fileOffsetBytes(): u64 {
        return _.file_offset_mb * 1048576;
    }

    fn isPresent(): bool {
        return _.state == BAT_FULLY_PRESENT || _.state == BAT_PARTIALLY_PRESENT;
    }

    fn isZero(): bool {
        return _.state == BAT_ZERO;
    }
}

// ---------------------------------------------------------------------------
// Parsing & Selection Functions
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with the 8-byte `"vhdxfile"` signature.
pub fn isVhdx(data: []const u8): bool {
    return matchBytes(data, 0, VHDX_FILE_ID_SIG);
}

/// Parses the VHDX File Type Identifier at offset 0.
pub fn parseVhdxFileId(data: []const u8): VhdxFileId? {
    if (!isVhdx(data)) {
        return null;
    }
    const max_end: usize = VHDX_FILE_ID_CREATOR_OFFSET + VHDX_FILE_ID_CREATOR_LEN;
    const creator_end: usize = if (data.len >= max_end) max_end else data.len;
    return VhdxFileId {
        creator_utf16le: data[VHDX_FILE_ID_CREATOR_OFFSET..creator_end],
    };
}

/// Parses and validates a 4KB VHDX Header (`"head"`, `version == 1`, and 4KB CRC32C).
pub fn parseVhdxHeader(block4k: []const u8): VhdxHeader? {
    if (block4k.len < VHDX_HEADER_SIZE) {
        return null;
    }
    if (!matchBytes(block4k, 0, VHDX_HEADER_SIG)) {
        return null;
    }
    const version = readU16Le(block4k, 66);
    if (version != 1) {
        return null;
    }
    const stored_checksum = readU32Le(block4k, 4);
    const computed_checksum = vhdxCrc32cWithZeroField(block4k[0..VHDX_HEADER_SIZE], 4);
    if (stored_checksum != computed_checksum) {
        return null;
    }
    return VhdxHeader {
        checksum: stored_checksum,
        sequence_number: readU64Le(block4k, 8),
        file_write_guid: readGuid16(block4k, 16),
        data_write_guid: readGuid16(block4k, 32),
        log_guid: readGuid16(block4k, 48),
        log_version: readU16Le(block4k, 64),
        version: version,
        log_length: readU32Le(block4k, 68),
        log_offset: readU64Le(block4k, 72),
    };
}

/// Selects the active VHDX Header from the two redundant 4KB header slots.
/// Returns the valid header with the higher `sequence_number` (or `null` if neither is valid).
pub fn selectActiveHeader(h1: VhdxHeader?, h2: VhdxHeader?): VhdxHeader? {
    if (a := h1) {
        if (b := h2) {
            if (b.sequence_number > a.sequence_number) {
                return b;
            }
            return a;
        }
        return a;
    }
    return h2;
}

/// Parses and validates a 64KB VHDX Region Table (`"regi"`, `entry_count <= 2047`, and 64KB CRC32C).
pub fn parseVhdxRegionTable(block64k: []const u8): VhdxRegionTable? {
    if (block64k.len < VHDX_REGION_SIZE) {
        return null;
    }
    if (!matchBytes(block64k, 0, VHDX_REGION_SIG)) {
        return null;
    }
    const entry_count = readU32Le(block64k, 8);
    if (entry_count > VHDX_MAX_REGION_ENTRIES) {
        return null;
    }
    const stored_checksum = readU32Le(block64k, 4);
    const computed_checksum = vhdxCrc32cWithZeroField(block64k[0..VHDX_REGION_SIZE], 4);
    if (stored_checksum != computed_checksum) {
        return null;
    }
    const entries_end: usize = (16 as usize) + (entry_count as usize) * VHDX_REGION_ENTRY_SIZE;
    return VhdxRegionTable {
        checksum: stored_checksum,
        entry_count: entry_count,
        entries_bytes: block64k[16..entries_end],
    };
}

/// Decodes a 64-bit little-endian VHDX BAT entry into its 3-bit state and 44-bit MB file offset.
pub fn decodeVhdxBatEntry(raw: u64): VhdxBatEntry {
    return VhdxBatEntry {
        state: (raw & 0x7) as u8,
        file_offset_mb: raw >> 20,
    };
}
