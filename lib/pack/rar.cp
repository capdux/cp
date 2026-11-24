// Origin: Pure-CP Stdlib (Zero-Alloc RAR4 & RAR5 Archive Vint, CRC32 & Block/File Parser) | Created: 2026-11-22

// RAR Signatures & Versions
pub const RAR4_SIGNATURE: [7]u8 = [0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00];
pub const RAR5_SIGNATURE: [8]u8 = [0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x01, 0x00];
pub const RAR_VER_4: u8 = 4;
pub const RAR_VER_5: u8 = 5;

// RAR5 Header Types
pub const RAR5_HEAD_MAIN: u64 = 1;
pub const RAR5_HEAD_FILE: u64 = 2;
pub const RAR5_HEAD_SERVICE: u64 = 3;
pub const RAR5_HEAD_CRYPT: u64 = 4;
pub const RAR5_HEAD_ENDARC: u64 = 5;

// RAR5 Common Header Flags
pub const HFL_EXTRA: u64 = 0x0001;
pub const HFL_DATA: u64 = 0x0002;
pub const HFL_SKIP_IF_UNKNOWN: u64 = 0x0004;
pub const HFL_SPLIT_BEFORE: u64 = 0x0008;
pub const HFL_SPLIT_AFTER: u64 = 0x0010;

// RAR5 File Flags
pub const FILE_FLAG_DIR: u64 = 0x0001;
pub const FILE_FLAG_UTIME: u64 = 0x0002;
pub const FILE_FLAG_CRC32: u64 = 0x0004;
pub const FILE_FLAG_UNP_UNKNOWN: u64 = 0x0008;

// RAR4 Block Types & Flags
pub const RAR4_TYPE_MAIN: u8 = 0x73;
pub const RAR4_TYPE_FILE: u8 = 0x74;
pub const RAR4_TYPE_ENDARC: u8 = 0x7B;
pub const RAR4_FLAG_LONG_BLOCK: u16 = 0x8000;

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn matchSig7(data: []const u8): bool {
    if (data.len < 7) {
        return false;
    }
    let i: usize = 0;
    while (i < 7) {
        if (data[i] != RAR4_SIGNATURE[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn matchSig8(data: []const u8): bool {
    if (data.len < 8) {
        return false;
    }
    let i: usize = 0;
    while (i < 8) {
        if (data[i] != RAR5_SIGNATURE[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Detects the RAR archive version (`RAR_VER_5` = 5, `RAR_VER_4` = 4, or `null`).
pub fn detectRarVersion(data: []const u8): u8? {
    if (matchSig8(data)) {
        return RAR_VER_5;
    }
    if (matchSig7(data)) {
        return RAR_VER_4;
    }
    return null;
}

/// Returns `true` if `data` starts with a valid RAR4 (7B) or RAR5 (8B) signature.
pub fn isRar(data: []const u8): bool {
    return detectRarVersion(data) != null;
}

/// Computes standard IEEE 802.3 CRC32 (reflected polynomial `0xEDB88320`, init `0xFFFFFFFF`, xorout `0xFFFFFFFF`).
pub fn rarCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u32);
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

/// Decoded RAR5 7-bit variable-length unsigned integer (`value` and `consumed` byte count).
pub struct Rar5Vint {
    value: u64,
    consumed: usize,
}

/// Decodes a RAR5 little-endian 7-bit varint (1..=10 bytes).
/// Returns `null` on truncation or 64-bit overflow (> 10 bytes or 10th byte > 0x01).
pub fn decodeRar5Vint(data: []const u8): Rar5Vint? {
    if (data.len == 0) {
        return null;
    }
    let val: u64 = 0;
    let shift: u6 = 0;
    let i: usize = 0;
    while (i < data.len && i < 10) {
        const b = data[i];
        if (i == 9 && b > 0x01) {
            return null;
        }
        val = val | (((b & 0x7F) as u64) << shift);
        i += 1;
        if ((b & 0x80) == 0) {
            return Rar5Vint{
                .value = val,
                .consumed = i,
            };
        }
        if (i == 10) {
            return null;
        }
        shift = (shift + 7) as u6;
    }
    return null;
}

/// Encodes `val` as a RAR5 7-bit varint (1..=10 bytes) into `out`.
/// Returns the number of bytes written, or `null` if `out` is too small.
pub fn encodeRar5Vint(val: u64, out: []u8): usize? {
    let v = val;
    let i: usize = 0;
    while (true) {
        if (i >= out.len) {
            return null;
        }
        let b = (v & 0x7F) as u8;
        v = v >> 7;
        if (v != 0) {
            out[i] = b | 0x80;
            i += 1;
        } else {
            out[i] = b;
            return i + 1;
        }
    }
}

/// Parsed RAR5 archive block (zero-copy `body`, `extra_area`, and `data_area` slices).
pub struct Rar5Block {
    crc32: u32,
    header_type: u64,
    flags: u64,
    body: []const u8,
    extra_area: []const u8,
    data_area: []const u8,
    consumed: usize,
}

/// Zero-alloc iterator over RAR5 archive blocks with per-block IEEE CRC32 verification.
pub struct Rar5BlockIter {
    data: []const u8,
    pos: usize,
    done: bool,
    has_err: bool,

    pub static fn init(data: []const u8): Rar5BlockIter {
        if (!matchSig8(data)) {
            return Rar5BlockIter{
                .data = data,
                .pos = 0,
                .done = true,
                .has_err = true,
            };
        }
        return Rar5BlockIter{
            .data = data,
            .pos = 8,
            .done = false,
            .has_err = false,
        };
    }

    fn hasError(): bool {
        return _.has_err;
    }

    fn next(*_): Rar5Block? {
        if (_.done || _.has_err || _.pos >= _.data.len) {
            return null;
        }
        const block_start = _.pos;
        if (_.data.len - block_start < 5) {
            _.has_err = true;
            _.done = true;
            return null;
        }

        const expected_crc = readU32Le(_.data, block_start);
        let hsz_v = Rar5Vint{ .value = 0, .consumed = 0 };
        if (v := decodeRar5Vint(_.data[(block_start + 4).._.data.len])) {
            hsz_v = v;
        } else {
            _.has_err = true;
            _.done = true;
            return null;
        }
        const hdr_content_start = block_start + 4 + hsz_v.consumed;
        if (hsz_v.value == 0 || hsz_v.value > ((_.data.len - hdr_content_start) as u64)) {
            _.has_err = true;
            _.done = true;
            return null;
        }
        const hdr_end = hdr_content_start + (hsz_v.value as usize);

        // Verify IEEE CRC32 over [block_start + 4 .. hdr_end]
        const calc_crc = rarCrc32(_.data[(block_start + 4)..hdr_end]);
        if (calc_crc != expected_crc) {
            _.has_err = true;
            _.done = true;
            return null;
        }

        let cur = hdr_content_start;
        const htype_v = decodeRar5Vint(_.data[cur..hdr_end]) ?? return null;
        cur += htype_v.consumed;
        const flags_v = decodeRar5Vint(_.data[cur..hdr_end]) ?? return null;
        cur += flags_v.consumed;

        let extra_size: usize = 0;
        if ((flags_v.value & HFL_EXTRA) != 0) {
            if (ex_v := decodeRar5Vint(_.data[cur..hdr_end])) {
                cur += ex_v.consumed;
                if (ex_v.value > ((hdr_end - cur) as u64)) {
                    _.has_err = true;
                    _.done = true;
                    return null;
                }
                extra_size = ex_v.value as usize;
            } else {
                _.has_err = true;
                _.done = true;
                return null;
            }
        }

        let data_size: usize = 0;
        if ((flags_v.value & HFL_DATA) != 0) {
            if (ds_v := decodeRar5Vint(_.data[cur..hdr_end])) {
                cur += ds_v.consumed;
                if (ds_v.value > ((_.data.len - hdr_end) as u64)) {
                    _.has_err = true;
                    _.done = true;
                    return null;
                }
                data_size = ds_v.value as usize;
            } else {
                _.has_err = true;
                _.done = true;
                return null;
            }
        }

        if (extra_size > hdr_end - cur) {
            _.has_err = true;
            _.done = true;
            return null;
        }
        const body_end = hdr_end - extra_size;
        const body = _.data[cur..body_end];
        const extra_area = _.data[body_end..hdr_end];
        const data_area = _.data[hdr_end..(hdr_end + data_size)];
        const total_end = hdr_end + data_size;
        const consumed = total_end - block_start;

        _.pos = total_end;
        if (htype_v.value == RAR5_HEAD_ENDARC) {
            _.done = true;
        }

        return Rar5Block{
            .crc32 = expected_crc,
            .header_type = htype_v.value,
            .flags = flags_v.value,
            .body = body,
            .extra_area = extra_area,
            .data_area = data_area,
            .consumed = consumed,
        };
    }
}

/// Creates a zero-alloc iterator over RAR5 blocks (validating the 8-byte RAR5 signature).
pub fn iterRar5Blocks(data: []const u8): Rar5BlockIter {
    return Rar5BlockIter.init(data);
}

/// Parsed RAR5 File Header entry (`header_type == RAR5_HEAD_FILE`).
pub struct Rar5FileEntry {
    file_flags: u64,
    unpacked_size: u64,
    attributes: u64,
    mtime: u32,
    data_crc32: u32,
    comp_info: u64,
    host_os: u64,
    name: []const u8,
    data: []const u8,

    fn isDir(): bool {
        return (_.file_flags & FILE_FLAG_DIR) != 0;
    }

    fn hasMtime(): bool {
        return (_.file_flags & FILE_FLAG_UTIME) != 0;
    }

    fn hasCrc32(): bool {
        return (_.file_flags & FILE_FLAG_CRC32) != 0;
    }

    /// Compression method (`0` = Store, `1..=5` = Fastest..Best).
    fn compMethod(): u8 {
        return ((_.comp_info >> 7) & 0x07) as u8;
    }

    /// Returns `true` if solid compression flag (bit 6 of `comp_info`) is set.
    fn isSolid(): bool {
        return (_.comp_info & 0x40) != 0;
    }
}

/// Parses a `Rar5FileEntry` from a `Rar5Block` whose `header_type == RAR5_HEAD_FILE`.
pub fn parseRar5FileEntry(block: Rar5Block): Rar5FileEntry? {
    if (block.header_type != RAR5_HEAD_FILE) {
        return null;
    }
    const b = block.body;
    let cur: usize = 0;

    const ff_v = decodeRar5Vint(b[cur..b.len]) ?? return null;
    cur += ff_v.consumed;
    const unp_v = decodeRar5Vint(b[cur..b.len]) ?? return null;
    cur += unp_v.consumed;
    const attr_v = decodeRar5Vint(b[cur..b.len]) ?? return null;
    cur += attr_v.consumed;

    let mtime: u32 = 0;
    if ((ff_v.value & FILE_FLAG_UTIME) != 0) {
        if (b.len - cur < 4) {
            return null;
        }
        mtime = readU32Le(b, cur);
        cur += 4;
    }

    let data_crc32: u32 = 0;
    if ((ff_v.value & FILE_FLAG_CRC32) != 0) {
        if (b.len - cur < 4) {
            return null;
        }
        data_crc32 = readU32Le(b, cur);
        cur += 4;
    }

    const comp_v = decodeRar5Vint(b[cur..b.len]) ?? return null;
    cur += comp_v.consumed;
    const os_v = decodeRar5Vint(b[cur..b.len]) ?? return null;
    cur += os_v.consumed;
    const nlen_v = decodeRar5Vint(b[cur..b.len]) ?? return null;
    cur += nlen_v.consumed;

    if (nlen_v.value > ((b.len - cur) as u64)) {
        return null;
    }
    const nlen = nlen_v.value as usize;
    const name = b[cur..(cur + nlen)];

    return Rar5FileEntry{
        .file_flags = ff_v.value,
        .unpacked_size = unp_v.value,
        .attributes = attr_v.value,
        .mtime = mtime,
        .data_crc32 = data_crc32,
        .comp_info = comp_v.value,
        .host_os = os_v.value,
        .name = name,
        .data = block.data_area,
    };
}

/// Parsed RAR4 archive block (zero-copy `header_body` and `data_area` slices).
pub struct Rar4Block {
    head_crc: u16,
    head_type: u8,
    head_flags: u16,
    head_size: u16,
    add_size: u32,
    header_body: []const u8,
    data_area: []const u8,
}

/// Zero-alloc iterator over RAR4 archive blocks with low-16-bit IEEE CRC32 header verification.
pub struct Rar4BlockIter {
    data: []const u8,
    pos: usize,
    done: bool,
    has_err: bool,

    pub static fn init(data: []const u8): Rar4BlockIter {
        if (!matchSig7(data)) {
            return Rar4BlockIter{
                .data = data,
                .pos = 0,
                .done = true,
                .has_err = true,
            };
        }
        return Rar4BlockIter{
            .data = data,
            .pos = 7,
            .done = false,
            .has_err = false,
        };
    }

    fn hasError(): bool {
        return _.has_err;
    }

    fn next(*_): Rar4Block? {
        if (_.done || _.has_err || _.pos >= _.data.len) {
            return null;
        }
        const start = _.pos;
        if (_.data.len - start < 7) {
            _.has_err = true;
            _.done = true;
            return null;
        }

        const head_crc = readU16Le(_.data, start);
        const head_type = _.data[start + 2];
        const head_flags = readU16Le(_.data, start + 3);
        const head_size = readU16Le(_.data, start + 5);
        const hsz = head_size as usize;

        if (hsz < 7 || hsz > _.data.len - start) {
            _.has_err = true;
            _.done = true;
            return null;
        }

        // Verify low 16 bits of IEEE CRC32 over block[2..head_size]
        const calc_crc16 = (rarCrc32(_.data[(start + 2)..(start + hsz)]) & 0xFFFF) as u16;
        if (calc_crc16 != head_crc) {
            _.has_err = true;
            _.done = true;
            return null;
        }

        let add_size: u32 = 0;
        let body_start = start + 7;
        const has_add = (head_flags & RAR4_FLAG_LONG_BLOCK) != 0 || head_type == RAR4_TYPE_FILE;
        if (has_add) {
            if (hsz < 11) {
                _.has_err = true;
                _.done = true;
                return null;
            }
            add_size = readU32Le(_.data, start + 7);
            body_start = start + 11;
        }

        const hdr_end = start + hsz;
        const add_sz = add_size as usize;
        if (add_sz > _.data.len - hdr_end) {
            _.has_err = true;
            _.done = true;
            return null;
        }

        const header_body = _.data[body_start..hdr_end];
        const data_area = _.data[hdr_end..(hdr_end + add_sz)];
        _.pos = hdr_end + add_sz;
        if (head_type == RAR4_TYPE_ENDARC) {
            _.done = true;
        }

        return Rar4Block{
            .head_crc = head_crc,
            .head_type = head_type,
            .head_flags = head_flags,
            .head_size = head_size,
            .add_size = add_size,
            .header_body = header_body,
            .data_area = data_area,
        };
    }
}

/// Creates a zero-alloc iterator over RAR4 blocks (validating the 7-byte RAR4 signature).
pub fn iterRar4Blocks(data: []const u8): Rar4BlockIter {
    return Rar4BlockIter.init(data);
}
