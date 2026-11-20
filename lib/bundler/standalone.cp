// Source: /data/dev/bun/src/standalone_graph/StandaloneModuleGraph.zig
// ============================================================================
// lib/bundler/standalone.cp — Single-File Executable Virtual Module Graph (VFS)
// ============================================================================

const std = @import("std");

pub const STANDALONE_HEADER_MAGIC: []const u8 = "CPSG0001";
pub const STANDALONE_TRAILER_MAGIC: []const u8 = "\n---- CP! -----\n";
pub const STANDALONE_HEADER_SIZE: usize = 32;
pub const STANDALONE_DESCRIPTOR_SIZE: usize = 32;
pub const STANDALONE_FOOTER_SIZE: usize = 40;

pub enum StandaloneModuleFormat: u8 {
    None = 0,
    Esm = 1,
    Cjs = 2,
}

pub enum StandaloneFileSide: u8 {
    Server = 0,
    Client = 1,
}

pub enum StandaloneEncoding: u8 {
    Binary = 0,
    Latin1 = 1,
    Utf8 = 2,
}

pub err StandaloneGraphError {
    BufferTooSmall,
    InvalidHeaderMagic,
    UnsupportedVersion,
    InvalidTrailerMagic,
    InvalidByteCount,
    OutOfBounds,
    InvalidEntryPoint,
    OutOfMemory,
}

pub struct StandaloneModuleEntry {
    path: []const u8 = "",
    contents: []const u8 = "",
    sourcemap: []const u8 = "",
    format: StandaloneModuleFormat = .Esm,
    side: StandaloneFileSide = .Server,
    encoding: StandaloneEncoding = .Utf8,
    is_entry: bool = false,
}

fn writeU32Le(dst: []u8, offset: usize, val: u32) {
    dst[offset] = (val & 0xFF) as u8;
    dst[offset + 1] = ((val >> 8) & 0xFF) as u8;
    dst[offset + 2] = ((val >> 16) & 0xFF) as u8;
    dst[offset + 3] = ((val >> 24) & 0xFF) as u8;
}

fn writeU64Le(dst: []u8, offset: usize, val: u64) {
    let v = val;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        dst[offset + i] = (v & 0xFF) as u8;
        v >>= 8;
    }
}

fn readU32Le(src: []const u8, offset: usize): u32 {
    return (src[offset] as u32) |
        ((src[offset + 1] as u32) << 8) |
        ((src[offset + 2] as u32) << 16) |
        ((src[offset + 3] as u32) << 24);
}

fn readU64Le(src: []const u8, offset: usize): u64 {
    let v: u64 = 0;
    let i: usize = 8;
    while (i > 0) {
        i -= 1;
        v = (v << 8) | (src[offset + i] as u64);
    }
    return v;
}

/// Embedded single-file executable module graph (VFS) supporting binary serialization
/// (Header Magic + Entry Table + Payload + Footer Trailer) and zero-copy deserialization.
pub struct StandaloneModuleGraph {
    entries: std.ArrayList(StandaloneModuleEntry) = .empty,
    entry_point_id: u32 = 0,
    flags: u32 = 0,
    allocator: std.mem.Allocator = std.heap.page_allocator,

    pub static fn init(allocator: std.mem.Allocator): StandaloneModuleGraph {
        return StandaloneModuleGraph{
            entries: .empty,
            entry_point_id: 0,
            flags: 0,
            allocator: allocator,
        };
    }

    pub fn deinit(*_) {
        _.entries.deinit(_.allocator);
    }

    pub fn count(): usize {
        return _.entries.items.len;
    }

    pub fn addModule(*_, entry: StandaloneModuleEntry): u32! {
        const idx = _.entries.items.len as u32;
        if (entry.is_entry) {
            _.entry_point_id = idx;
        }
        _.entries.append(_.allocator, entry)?;
        return idx;
    }

    pub fn entryPoint(): StandaloneModuleEntry? {
        if (_.entries.items.len == 0) return null;
        if ((_.entry_point_id as usize) >= _.entries.items.len) return null;
        return _.entries.items[_.entry_point_id as usize];
    }

    pub fn find(path: []const u8): StandaloneModuleEntry? {
        for (entry of _.entries.items) {
            if (std.mem.eql(u8, entry.path, path)) {
                return entry;
            }
        }
        return null;
    }

    pub fn findContent(path: []const u8): ([]const u8)? {
        if (entry := _.find(path)) {
            return entry.contents;
        }
        return null;
    }

    /// Serializes the module graph into a self-contained binary blob:
    /// `[32B Header] [N * 32B Entry Descriptors] [Contiguous Payload] [40B Footer Trailer]`
    pub fn serialize(allocator: std.mem.Allocator): ([]u8)! {
        const n = _.entries.items.len;
        const table_offset: usize = STANDALONE_HEADER_SIZE;
        const table_bytes: usize = n * STANDALONE_DESCRIPTOR_SIZE;
        const payload_start: usize = table_offset + table_bytes;

        let payload_len: usize = 0;
        for (e of _.entries.items) {
            payload_len += e.path.len + e.contents.len + e.sourcemap.len;
        }

        const total_size = payload_start + payload_len + STANDALONE_FOOTER_SIZE;
        let buf = allocator.alloc(u8, total_size)?;
        errdefer allocator.free(buf);

        // 1. Write 32-byte Header
        @memcpy(buf[0..8], STANDALONE_HEADER_MAGIC);
        writeU32Le(buf, 8, 1); // format version = 1
        writeU32Le(buf, 12, _.flags);
        writeU32Le(buf, 16, _.entry_point_id);
        writeU32Le(buf, 20, n as u32);
        writeU64Le(buf, 24, table_offset as u64);

        // 2. Write Descriptor Table + Payload bytes
        let cursor = payload_start;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const e = _.entries.items[i];
            const desc_off = table_offset + i * STANDALONE_DESCRIPTOR_SIZE;

            const path_off = cursor;
            if (e.path.len > 0) {
                @memcpy(buf[cursor .. cursor + e.path.len], e.path);
                cursor += e.path.len;
            }

            const contents_off = cursor;
            if (e.contents.len > 0) {
                @memcpy(buf[cursor .. cursor + e.contents.len], e.contents);
                cursor += e.contents.len;
            }

            const sm_off = cursor;
            if (e.sourcemap.len > 0) {
                @memcpy(buf[cursor .. cursor + e.sourcemap.len], e.sourcemap);
                cursor += e.sourcemap.len;
            }

            writeU32Le(buf, desc_off + 0, path_off as u32);
            writeU32Le(buf, desc_off + 4, e.path.len as u32);
            writeU32Le(buf, desc_off + 8, contents_off as u32);
            writeU32Le(buf, desc_off + 12, e.contents.len as u32);
            writeU32Le(buf, desc_off + 16, sm_off as u32);
            writeU32Le(buf, desc_off + 20, e.sourcemap.len as u32);
            buf[desc_off + 24] = e.format as u8;
            buf[desc_off + 25] = e.side as u8;
            buf[desc_off + 26] = e.encoding as u8;
            buf[desc_off + 27] = if (e.is_entry) 1 else 0;
            writeU32Le(buf, desc_off + 28, 0);
        }

        // 3. Write 40-byte Footer Trailer
        const footer_off = total_size - STANDALONE_FOOTER_SIZE;
        writeU64Le(buf, footer_off + 0, total_size as u64);
        writeU64Le(buf, footer_off + 8, table_offset as u64);
        writeU32Le(buf, footer_off + 16, n as u32);
        writeU32Le(buf, footer_off + 20, _.entry_point_id);
        @memcpy(buf[footer_off + 24 .. footer_off + 40], STANDALONE_TRAILER_MAGIC);

        return buf;
    }

    /// Zero-copy deserialization from `bytes` with strict header magic, footer trailer magic,
    /// and bounds verification on all descriptor offsets and lengths.
    /// Slices in `StandaloneModuleEntry` borrow directly from `bytes` without copying payloads.
    pub static fn deserialize(allocator: std.mem.Allocator, bytes: []const u8): StandaloneModuleGraph! {
        if (bytes.len < STANDALONE_HEADER_SIZE + STANDALONE_FOOTER_SIZE) {
            return error.BufferTooSmall;
        }

        // Validate Header Magic & Version
        if (!std.mem.eql(u8, bytes[0..8], STANDALONE_HEADER_MAGIC)) {
            return error.InvalidHeaderMagic;
        }
        const version = readU32Le(bytes, 8);
        if (version != 1) {
            return error.UnsupportedVersion;
        }
        const flags = readU32Le(bytes, 12);
        const entry_point_id = readU32Le(bytes, 16);
        const entry_count = readU32Le(bytes, 20);
        const table_offset = readU64Le(bytes, 24);

        // Validate Footer Trailer Magic & Consistency
        const footer_off = bytes.len - STANDALONE_FOOTER_SIZE;
        if (!std.mem.eql(u8, bytes[footer_off + 24 .. footer_off + 40], STANDALONE_TRAILER_MAGIC)) {
            return error.InvalidTrailerMagic;
        }
        const footer_total = readU64Le(bytes, footer_off + 0);
        const footer_table_off = readU64Le(bytes, footer_off + 8);
        const footer_count = readU32Le(bytes, footer_off + 16);
        const footer_entry_id = readU32Le(bytes, footer_off + 20);

        if (footer_total != (bytes.len as u64) ||
            footer_table_off != table_offset ||
            footer_count != entry_count ||
            footer_entry_id != entry_point_id)
        {
            return error.InvalidByteCount;
        }

        if (table_offset < (STANDALONE_HEADER_SIZE as u64) || table_offset > (footer_off as u64)) {
            return error.OutOfBounds;
        }

        const n = entry_count as usize;
        const table_start = table_offset as usize;
        // Overflow-safe check for table_bytes = n * STANDALONE_DESCRIPTOR_SIZE
        if (n > (footer_off - table_start) / STANDALONE_DESCRIPTOR_SIZE) {
            return error.OutOfBounds;
        }
        const payload_start = table_start + n * STANDALONE_DESCRIPTOR_SIZE;
        const payload_end = footer_off;

        if (n > 0 && (entry_point_id as usize) >= n) {
            return error.InvalidEntryPoint;
        }

        let graph = StandaloneModuleGraph.init(allocator);
        graph.flags = flags;
        graph.entry_point_id = entry_point_id;
        errdefer graph.deinit();

        let i: usize = 0;
        while (i < n) : (i += 1) {
            const desc_off = table_start + i * STANDALONE_DESCRIPTOR_SIZE;
            const path_off = readU32Le(bytes, desc_off + 0) as usize;
            const path_len = readU32Le(bytes, desc_off + 4) as usize;
            const contents_off = readU32Le(bytes, desc_off + 8) as usize;
            const contents_len = readU32Le(bytes, desc_off + 12) as usize;
            const sm_off = readU32Le(bytes, desc_off + 16) as usize;
            const sm_len = readU32Le(bytes, desc_off + 20) as usize;

            if (path_off < payload_start || path_off > payload_end || path_len > payload_end - path_off) {
                return error.OutOfBounds;
            }
            if (contents_off < payload_start || contents_off > payload_end || contents_len > payload_end - contents_off) {
                return error.OutOfBounds;
            }
            if (sm_off < payload_start || sm_off > payload_end || sm_len > payload_end - sm_off) {
                return error.OutOfBounds;
            }

            const fmt_raw = bytes[desc_off + 24];
            const side_raw = bytes[desc_off + 25];
            const enc_raw = bytes[desc_off + 26];
            const is_entry = bytes[desc_off + 27] != 0;

            const format: StandaloneModuleFormat = if (fmt_raw == 1) .Esm else if (fmt_raw == 2) .Cjs else .None;
            const side: StandaloneFileSide = if (side_raw == 1) .Client else .Server;
            const encoding: StandaloneEncoding = if (enc_raw == 1) .Latin1 else if (enc_raw == 2) .Utf8 else .Binary;

            graph.entries.append(allocator, StandaloneModuleEntry{
                path: bytes[path_off .. path_off + path_len],
                contents: bytes[contents_off .. contents_off + contents_len],
                sourcemap: bytes[sm_off .. sm_off + sm_len],
                format: format,
                side: side,
                encoding: encoding,
                is_entry: is_entry,
            })?;
        }

        return graph;
    }
}
