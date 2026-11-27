// Reference Only: Functional overlap with lib/pack/zip.cp | Kept for comparison/analysis
// Source: cp/lib/zip/zip.cp (Pure CP PKZIP Archive Builder & Safe Reader via cap:deflate)
// Zero external C dependency implementation using pure CP, std, and cap:deflate.

const std = @import("std");
import * as deflate from "cap:deflate";

const LOCAL_FILE_HEADER_SIG: u32 = 0x04034b50;
const CENTRAL_DIR_HEADER_SIG: u32 = 0x02014b50;
const EOCD_SIG: u32 = 0x06054b50;

const DEFAULT_MAX_ENTRIES: usize = 65535;
const DEFAULT_MAX_ENTRY_BYTES: usize = 256 * 1024 * 1024;

/// Error set for PKZIP archive construction and safe reading.
pub err ZipError {
    InvalidArchive,
    UnsupportedCompression,
    ChecksumMismatch,
    TruncatedArchive,
    UnsafePathTraversal,
    TooManyEntries,
    EntryTooLarge,
    OutOfMemory,
}

/// PKZIP compression methods supported by `cap:zip`.
pub enum CompressionMethod: u16 {
    Stored = 0,
    Deflated = 8,
}

/// A decoded entry inside a `ZipArchive`.
pub class ZipEntry {
    pub name: []const u8,
    pub compression: CompressionMethod,
    pub crc32: u32,
    pub compressed_size: u32,
    pub uncompressed_size: u32,
    pub data: []const u8,
}

/// An in-memory decoded PKZIP archive owning all entry names and payloads.
pub class ZipArchive {
    pub entries: []ZipEntry,

    /// Finds an entry by exact path name, or returns `null` if not present.
    pub fn find(name: []const u8): (*const ZipEntry)? {
        const entries = _.entries;
        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            if (std.mem.eql(u8, entries[i].name, name)) {
                return &entries[i];
            }
        }
        return null;
    }

    /// Releases all memory owned by the archive and its entries.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.entries.len) : (i += 1) {
            allocator.free(_.entries[i].name);
            allocator.free(_.entries[i].data);
        }
        allocator.free(_.entries);
        _.entries = _.entries[0..0];
    }
}

class BuilderItem {
    name: []u8,
    compression: CompressionMethod,
    crc32: u32,
    uncompressed_size: u32,
    compressed_data: []u8,
}

/// Builder for constructing standard PKZIP archives with Stored (0) and Deflated (8) entries.
pub class ZipBuilder {
    allocator: std.mem.Allocator,
    items: std.ArrayList(BuilderItem),

    /// Initializes an empty `ZipBuilder` backed by `allocator`.
    pub static fn init(allocator: std.mem.Allocator): ZipBuilder {
        return ZipBuilder{
            allocator: allocator,
            items: .empty,
        };
    }

    /// Releases all buffered entries and internal storage.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            _.allocator.free(_.items.items[i].name);
            _.allocator.free(_.items.items[i].compressed_data);
        }
        _.items.deinit(_.allocator);
    }

    /// Adds an uncompressed (`Stored = 0`) entry to the archive.
    pub fn addStored(*_, name: []const u8, data: []const u8): void ! ZipError {
        if (name.len > 0xffff) {
            return ZipError.InvalidArchive;
        }
        if (data.len > std.math.maxInt(u32)) {
            return ZipError.EntryTooLarge;
        }
        if (_.items.items.len >= 0xffff) {
            return ZipError.TooManyEntries;
        }

        const name_copy = _.allocator.dupe(u8, name) catch return ZipError.OutOfMemory;
        errdefer _.allocator.free(name_copy);

        const data_copy = _.allocator.dupe(u8, data) catch return ZipError.OutOfMemory;
        errdefer _.allocator.free(data_copy);

        const item = BuilderItem{
            name: name_copy,
            compression: CompressionMethod.Stored,
            crc32: deflate.crc32(data),
            uncompressed_size: data.len as u32,
            compressed_data: data_copy,
        };
        _.items.append(_.allocator, item) catch return ZipError.OutOfMemory;
    }

    /// Adds a DEFLATE-compressed (`Deflated = 8`) entry to the archive via `cap:deflate`.
    pub fn addDeflated(*_, name: []const u8, data: []const u8): void ! ZipError {
        if (name.len > 0xffff) {
            return ZipError.InvalidArchive;
        }
        if (data.len > std.math.maxInt(u32)) {
            return ZipError.EntryTooLarge;
        }
        if (_.items.items.len >= 0xffff) {
            return ZipError.TooManyEntries;
        }

        const name_copy = _.allocator.dupe(u8, name) catch return ZipError.OutOfMemory;
        errdefer _.allocator.free(name_copy);

        const compressed = deflate.compressDeflate(_.allocator, data) catch |e| {
            if (e == deflate.DeflateError.OutOfMemory) {
                return ZipError.OutOfMemory;
            }
            return ZipError.InvalidArchive;
        };
        errdefer _.allocator.free(compressed);

        if (compressed.len > std.math.maxInt(u32)) {
            return ZipError.EntryTooLarge;
        }

        const item = BuilderItem{
            name: name_copy,
            compression: CompressionMethod.Deflated,
            crc32: deflate.crc32(data),
            uncompressed_size: data.len as u32,
            compressed_data: compressed,
        };
        _.items.append(_.allocator, item) catch return ZipError.OutOfMemory;
    }

    /// Serializes all added entries into a complete PKZIP byte buffer (Local File Headers + Central Directory + EOCD).
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn finish(allocator: std.mem.Allocator): ([]u8) ! ZipError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);

        const count = _.items.items.len;
        if (count > 0xffff) {
            return ZipError.TooManyEntries;
        }

        let offsets = allocator.alloc(u32, count) catch return ZipError.OutOfMemory;
        defer allocator.free(offsets);

        // 1. Write Local File Headers and compressed payloads
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (out.items.len > std.math.maxInt(u32)) {
                return ZipError.EntryTooLarge;
            }
            offsets[i] = out.items.len as u32;

            const item = _.items.items[i];
            const method_val: u16 = @intFromEnum(item.compression);
            const comp_size: u32 = item.compressed_data.len as u32;
            const name_len: u16 = item.name.len as u16;

            appendU32LE(allocator, &out, LOCAL_FILE_HEADER_SIG)?;
            appendU16LE(allocator, &out, 20)?; // version needed to extract (2.0)
            appendU16LE(allocator, &out, 0x0800)?; // UTF-8 filename flag
            appendU16LE(allocator, &out, method_val)?;
            appendU16LE(allocator, &out, 0)?; // last mod file time
            appendU16LE(allocator, &out, 0x0021)?; // last mod file date (1980-01-01)
            appendU32LE(allocator, &out, item.crc32)?;
            appendU32LE(allocator, &out, comp_size)?;
            appendU32LE(allocator, &out, item.uncompressed_size)?;
            appendU16LE(allocator, &out, name_len)?;
            appendU16LE(allocator, &out, 0)?; // extra field length
            out.appendSlice(allocator, item.name) catch return ZipError.OutOfMemory;
            out.appendSlice(allocator, item.compressed_data) catch return ZipError.OutOfMemory;
        }

        if (out.items.len > std.math.maxInt(u32)) {
            return ZipError.EntryTooLarge;
        }
        const cd_offset: u32 = out.items.len as u32;

        // 2. Write Central Directory Headers
        i = 0;
        while (i < count) : (i += 1) {
            const item = _.items.items[i];
            const method_val: u16 = @intFromEnum(item.compression);
            const comp_size: u32 = item.compressed_data.len as u32;
            const name_len: u16 = item.name.len as u16;

            appendU32LE(allocator, &out, CENTRAL_DIR_HEADER_SIG)?;
            appendU16LE(allocator, &out, 20)?; // version made by
            appendU16LE(allocator, &out, 20)?; // version needed to extract
            appendU16LE(allocator, &out, 0x0800)?; // general purpose bit flag
            appendU16LE(allocator, &out, method_val)?;
            appendU16LE(allocator, &out, 0)?; // last mod file time
            appendU16LE(allocator, &out, 0x0021)?; // last mod file date
            appendU32LE(allocator, &out, item.crc32)?;
            appendU32LE(allocator, &out, comp_size)?;
            appendU32LE(allocator, &out, item.uncompressed_size)?;
            appendU16LE(allocator, &out, name_len)?;
            appendU16LE(allocator, &out, 0)?; // extra field length
            appendU16LE(allocator, &out, 0)?; // file comment length
            appendU16LE(allocator, &out, 0)?; // disk number start
            appendU16LE(allocator, &out, 0)?; // internal file attributes
            appendU32LE(allocator, &out, 0)?; // external file attributes
            appendU32LE(allocator, &out, offsets[i])?; // relative offset of local header
            out.appendSlice(allocator, item.name) catch return ZipError.OutOfMemory;
        }

        const cd_size_usize = out.items.len - (cd_offset as usize);
        if (cd_size_usize > std.math.maxInt(u32)) {
            return ZipError.EntryTooLarge;
        }
        const cd_size: u32 = cd_size_usize as u32;

        // 3. Write End of Central Directory Record (EOCD)
        const count_u16: u16 = count as u16;
        appendU32LE(allocator, &out, EOCD_SIG)?;
        appendU16LE(allocator, &out, 0)?; // number of this disk
        appendU16LE(allocator, &out, 0)?; // disk where central directory starts
        appendU16LE(allocator, &out, count_u16)?; // entries on this disk
        appendU16LE(allocator, &out, count_u16)?; // total entries
        appendU32LE(allocator, &out, cd_size)?;
        appendU32LE(allocator, &out, cd_offset)?;
        appendU16LE(allocator, &out, 0)?; // comment length

        return out.toOwnedSlice(allocator) catch return ZipError.OutOfMemory;
    }
}

fn readU16LE(buf: []const u8, pos: usize): u16 {
    return (buf[pos] as u16) | ((buf[pos + 1] as u16) << 8);
}

fn readU32LE(buf: []const u8, pos: usize): u32 {
    return (buf[pos] as u32) |
        ((buf[pos + 1] as u32) << 8) |
        ((buf[pos + 2] as u32) << 16) |
        ((buf[pos + 3] as u32) << 24);
}

fn appendU16LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u16): void ! ZipError {
    out.append(alloc, (v & 0xff) as u8) catch return ZipError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return ZipError.OutOfMemory;
}

fn appendU32LE(alloc: std.mem.Allocator, out: *std.ArrayList(u8), v: u32): void ! ZipError {
    out.append(alloc, (v & 0xff) as u8) catch return ZipError.OutOfMemory;
    out.append(alloc, ((v >> 8) & 0xff) as u8) catch return ZipError.OutOfMemory;
    out.append(alloc, ((v >> 16) & 0xff) as u8) catch return ZipError.OutOfMemory;
    out.append(alloc, ((v >> 24) & 0xff) as u8) catch return ZipError.OutOfMemory;
}

/// Validates entry path against directory traversal (`..`), leading `/`, backslashes `\`, and NUL bytes.
fn validateEntryName(name: []const u8): void ! ZipError {
    if (name.len == 0) {
        return ZipError.UnsafePathTraversal;
    }
    if (name[0] == '/') {
        return ZipError.UnsafePathTraversal;
    }

    let seg_start: usize = 0;
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        const ch = name[i];
        if (ch == '\\' || ch == 0) {
            return ZipError.UnsafePathTraversal;
        }
        if (ch == '/') {
            const seg = name[seg_start..i];
            if (std.mem.eql(u8, seg, "..")) {
                return ZipError.UnsafePathTraversal;
            }
            seg_start = i + 1;
        }
    }
    const last_seg = name[seg_start..name.len];
    if (std.mem.eql(u8, last_seg, "..")) {
        return ZipError.UnsafePathTraversal;
    }
}

fn findEocdOffset(zip_bytes: []const u8): usize ! ZipError {
    if (zip_bytes.len < 22) {
        if (zip_bytes.len >= 4) {
            const sig = readU32LE(zip_bytes, 0);
            if (sig == LOCAL_FILE_HEADER_SIG || sig == CENTRAL_DIR_HEADER_SIG || sig == EOCD_SIG) {
                return ZipError.TruncatedArchive;
            }
        }
        return ZipError.InvalidArchive;
    }

    const max_back: usize = 65535 + 22;
    const min_pos: usize = if (zip_bytes.len > max_back) (zip_bytes.len - max_back) else 0;
    let pos: usize = zip_bytes.len - 22;

    while (true) {
        if (readU32LE(zip_bytes, pos) == EOCD_SIG) {
            const comment_len = readU16LE(zip_bytes, pos + 20) as usize;
            if (pos + 22 + comment_len == zip_bytes.len) {
                return pos;
            }
        }
        if (pos == min_pos) {
            break;
        }
        pos -= 1;
    }

    const head_sig = readU32LE(zip_bytes, 0);
    if (head_sig == LOCAL_FILE_HEADER_SIG || head_sig == CENTRAL_DIR_HEADER_SIG) {
        return ZipError.TruncatedArchive;
    }
    return ZipError.InvalidArchive;
}

/// Safely reads and validates a PKZIP archive with default limits (max 65535 entries, 256 MiB per entry).
pub fn readArchive(allocator: std.mem.Allocator, zip_bytes: []const u8): ZipArchive ! ZipError {
    return readArchiveWithLimits(allocator, zip_bytes, DEFAULT_MAX_ENTRIES, DEFAULT_MAX_ENTRY_BYTES);
}

/// Safely reads and validates a PKZIP archive with explicit entry count and per-entry byte limits.
/// Rejects path traversal (`..`, leading `/`, `\`), CRC-32 mismatches, truncated archives, and oversized entries.
pub fn readArchiveWithLimits(
    allocator: std.mem.Allocator,
    zip_bytes: []const u8,
    max_entries: usize,
    max_entry_bytes: usize,
): ZipArchive ! ZipError {
    const eocd_pos = findEocdOffset(zip_bytes)?;

    const disk_num = readU16LE(zip_bytes, eocd_pos + 4);
    const cd_disk = readU16LE(zip_bytes, eocd_pos + 6);
    const disk_entries = readU16LE(zip_bytes, eocd_pos + 8);
    const total_entries = readU16LE(zip_bytes, eocd_pos + 10);
    const cd_size = readU32LE(zip_bytes, eocd_pos + 12) as usize;
    const cd_offset = readU32LE(zip_bytes, eocd_pos + 16) as usize;

    if (disk_num != 0 || cd_disk != 0 || disk_entries != total_entries) {
        return ZipError.InvalidArchive;
    }

    const entry_count = total_entries as usize;
    if (entry_count > max_entries) {
        return ZipError.TooManyEntries;
    }

    if (cd_offset > eocd_pos || cd_size > eocd_pos - cd_offset) {
        return ZipError.TruncatedArchive;
    }
    const cd_end = cd_offset + cd_size;

    let entries_list: std.ArrayList(ZipEntry) = .empty;
    errdefer {
        let k: usize = 0;
        while (k < entries_list.items.len) : (k += 1) {
            allocator.free(entries_list.items[k].name);
            allocator.free(entries_list.items[k].data);
        }
        entries_list.deinit(allocator);
    }

    let pos: usize = cd_offset;
    let idx: usize = 0;
    while (idx < entry_count) : (idx += 1) {
        if (pos + 46 > cd_end) {
            return ZipError.TruncatedArchive;
        }
        const cd_sig = readU32LE(zip_bytes, pos);
        if (cd_sig != CENTRAL_DIR_HEADER_SIG) {
            return ZipError.InvalidArchive;
        }

        const flags = readU16LE(zip_bytes, pos + 8);
        const method_u16 = readU16LE(zip_bytes, pos + 10);
        const expected_crc = readU32LE(zip_bytes, pos + 16);
        const comp_size = readU32LE(zip_bytes, pos + 20);
        const uncomp_size = readU32LE(zip_bytes, pos + 24);
        const name_len = readU16LE(zip_bytes, pos + 28) as usize;
        const extra_len = readU16LE(zip_bytes, pos + 30) as usize;
        const comment_len = readU16LE(zip_bytes, pos + 32) as usize;
        const local_offset = readU32LE(zip_bytes, pos + 42) as usize;

        const header_total = 46 + name_len + extra_len + comment_len;
        if (pos + header_total > cd_end) {
            return ZipError.TruncatedArchive;
        }

        const name_slice = zip_bytes[(pos + 46)..(pos + 46 + name_len)];
        validateEntryName(name_slice)?;

        if ((flags & 1) != 0) {
            return ZipError.UnsupportedCompression;
        }

        let method: CompressionMethod = CompressionMethod.Stored;
        if (method_u16 == 0) {
            method = CompressionMethod.Stored;
        } else if (method_u16 == 8) {
            method = CompressionMethod.Deflated;
        } else {
            return ZipError.UnsupportedCompression;
        }

        if ((uncomp_size as usize) > max_entry_bytes) {
            return ZipError.EntryTooLarge;
        }
        if (method == CompressionMethod.Stored && comp_size != uncomp_size) {
            return ZipError.InvalidArchive;
        }
        if (method == CompressionMethod.Stored && (comp_size as usize) > max_entry_bytes) {
            return ZipError.EntryTooLarge;
        }

        if (local_offset + 30 > cd_offset) {
            return ZipError.TruncatedArchive;
        }
        const lfh_sig = readU32LE(zip_bytes, local_offset);
        if (lfh_sig != LOCAL_FILE_HEADER_SIG) {
            return ZipError.InvalidArchive;
        }

        const lfh_name_len = readU16LE(zip_bytes, local_offset + 26) as usize;
        const lfh_extra_len = readU16LE(zip_bytes, local_offset + 28) as usize;
        const data_start = local_offset + 30 + lfh_name_len + lfh_extra_len;
        if (data_start > cd_offset || (comp_size as usize) > cd_offset - data_start) {
            return ZipError.TruncatedArchive;
        }
        const data_end = data_start + (comp_size as usize);
        const compressed_slice = zip_bytes[data_start..data_end];

        let decompressed: []u8 = undefined;
        if (method == CompressionMethod.Stored) {
            decompressed = allocator.dupe(u8, compressed_slice) catch return ZipError.OutOfMemory;
        } else {
            decompressed = deflate.decompressDeflateWithMaxSize(allocator, compressed_slice, max_entry_bytes) catch |e| {
                if (e == deflate.DeflateError.OutputTooLarge) {
                    return ZipError.EntryTooLarge;
                }
                if (e == deflate.DeflateError.OutOfMemory) {
                    return ZipError.OutOfMemory;
                }
                if (e == deflate.DeflateError.UnexpectedEof) {
                    return ZipError.TruncatedArchive;
                }
                return ZipError.InvalidArchive;
            };
        }
        errdefer allocator.free(decompressed);

        if (decompressed.len != (uncomp_size as usize)) {
            return ZipError.InvalidArchive;
        }
        if (deflate.crc32(decompressed) != expected_crc) {
            return ZipError.ChecksumMismatch;
        }

        const name_copy = allocator.dupe(u8, name_slice) catch return ZipError.OutOfMemory;
        errdefer allocator.free(name_copy);

        const entry = ZipEntry{
            name: name_copy,
            compression: method,
            crc32: expected_crc,
            compressed_size: comp_size,
            uncompressed_size: uncomp_size,
            data: decompressed,
        };
        entries_list.append(allocator, entry) catch return ZipError.OutOfMemory;

        pos += header_total;
    }

    const owned_entries = entries_list.toOwnedSlice(allocator) catch return ZipError.OutOfMemory;
    return ZipArchive{ entries: owned_entries };
}
