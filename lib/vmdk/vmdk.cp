// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc VMware VMDK SparseExtentHeader & Text Descriptor Parser)

// VMDK Core Protocol Constants
pub const VMDK_MAGIC: u32 = 0x564D444B; // "KDMV" little-endian
pub const VMDK_SECTOR_SIZE: usize = 512;
pub const VMDK_HEADER_SIZE: usize = 512;
pub const VMDK_DEFAULT_GTES_PER_GT: u32 = 512;

// SparseExtentHeader Flags
pub const FLAG_VALID_NEWLINE_DETECTOR: u32 = 0x0001;
pub const FLAG_REDUNDANT_GRAIN_TABLE: u32 = 0x0002;
pub const FLAG_ZEROED_GRAIN_GTE: u32 = 0x0004;
pub const FLAG_COMPRESSED_GRAINS: u32 = 0x10000;
pub const FLAG_MARKERS_IN_STREAM: u32 = 0x20000;

// Compression Algorithms
pub const COMPRESS_NONE: u16 = 0;
pub const COMPRESS_DEFLATE: u16 = 1;

// Descriptor Constants
pub const CID_NONE: u32 = 0xFFFFFFFF;

fn readU16Le(data: []const u8, off: usize): u16 {
    const b0 = data[off] as u16;
    const b1 = (data[off + 1] as u16) << 8;
    return b0 | b1;
}

fn readU32Le(data: []const u8, off: usize): u32 {
    const b0 = data[off] as u32;
    const b1 = (data[off + 1] as u32) << 8;
    const b2 = (data[off + 2] as u32) << 16;
    const b3 = (data[off + 3] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn readU64Le(data: []const u8, off: usize): u64 {
    const lo = readU32Le(data, off) as u64;
    const hi = (readU32Le(data, off + 4) as u64) << 32;
    return lo | hi;
}

fn isSpace(c: u8): bool {
    return c == 0x20 || c == 0x09 || c == 0x0D || c == 0x0A;
}

fn isHspace(c: u8): bool {
    return c == 0x20 || c == 0x09;
}

fn trimSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) {
        start += 1;
    }
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn stripNul(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return s[0..i];
        }
        i += 1;
    }
    return s;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEq(s[0..prefix.len], prefix);
}

fn indexOfByte(s: []const u8, target: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn unquoteValue(s: []const u8): ([]const u8)? {
    const t = trimSpace(s);
    if (t.len == 0) {
        return t;
    }
    if (t[0] == 0x22) { // '"'
        if (t.len < 2 || t[t.len - 1] != 0x22) {
            return null;
        }
        return t[1..(t.len - 1)];
    }
    if (t[t.len - 1] == 0x22) {
        return null;
    }
    return t;
}

fn parseDecU64(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    const max_div10: u64 = 1844674407370955161; // u64::MAX / 10
    const max_mod10: u64 = 5;                   // u64::MAX % 10
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < 0x30 || c > 0x39) {
            return null;
        }
        const digit = (c - 0x30) as u64;
        if (acc > max_div10 || (acc == max_div10 && digit > max_mod10)) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn parseDecU32(s: []const u8): u32? {
    const v = parseDecU64(s) ?? return null;
    if (v > 0xFFFFFFFF) {
        return null;
    }
    return v as u32;
}

fn parseHexU32(s: []const u8): u32? {
    let raw = s;
    if (raw.len >= 2 && raw[0] == 0x30 && (raw[1] == 0x78 || raw[1] == 0x58)) { // "0x" / "0X"
        raw = raw[2..raw.len];
    }
    if (raw.len == 0 || raw.len > 8) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        let nibble: u32 = 0;
        if (c >= 0x30 && c <= 0x39) {
            nibble = (c - 0x30) as u32;
        } else if (c >= 0x61 && c <= 0x66) {
            nibble = (c - 0x61 + 10) as u32;
        } else if (c >= 0x41 && c <= 0x46) {
            nibble = (c - 0x41 + 10) as u32;
        } else {
            return null;
        }
        acc = (acc << 4) | nibble;
        i += 1;
    }
    return acc;
}

/// Parsed 512-byte VMware VMDK SparseExtentHeader (little-endian).
pub struct VmdkSparseHeader {
    version: u32,
    flags: u32,
    capacity_sectors: u64,
    grain_size_sectors: u64,
    descriptor_offset: u64,
    descriptor_size: u64,
    num_gtes_per_gt: u32,
    rgd_offset: u64,
    gd_offset: u64,
    overhead_sectors: u64,
    unclean_shutdown: bool,
    compress_algorithm: u16,

    /// Virtual disk capacity in bytes (`capacity_sectors * 512`).
    fn capacityBytes(): u64 {
        return _.capacity_sectors * (VMDK_SECTOR_SIZE as u64);
    }

    /// Grain size in bytes (`grain_size_sectors * 512`).
    fn grainSizeBytes(): u64 {
        return _.grain_size_sectors * (VMDK_SECTOR_SIZE as u64);
    }

    /// Embedded descriptor byte offset (`descriptor_offset * 512`).
    fn descriptorByteOffset(): u64 {
        return _.descriptor_offset * (VMDK_SECTOR_SIZE as u64);
    }

    /// Embedded descriptor byte length (`descriptor_size * 512`).
    fn descriptorByteSize(): u64 {
        return _.descriptor_size * (VMDK_SECTOR_SIZE as u64);
    }

    /// Returns `true` if compressed grains are enabled.
    fn isCompressed(): bool {
        return (_.flags & FLAG_COMPRESSED_GRAINS) != 0 || _.compress_algorithm != COMPRESS_NONE;
    }

    /// Returns `true` if the extent is streamOptimized (compressed grains + markers in stream).
    fn isStreamOptimized(): bool {
        return (_.flags & FLAG_COMPRESSED_GRAINS) != 0 && (_.flags & FLAG_MARKERS_IN_STREAM) != 0;
    }

    /// Returns `true` if a redundant grain directory/table is present.
    fn hasRedundantGrainTable(): bool {
        return (_.flags & FLAG_REDUNDANT_GRAIN_TABLE) != 0;
    }

    /// Returns `true` if the newline detector flag is set.
    fn hasValidNewlineDetector(): bool {
        return (_.flags & FLAG_VALID_NEWLINE_DETECTOR) != 0;
    }

    /// Returns `true` if zeroed-grain GTE (value 1) support is enabled.
    fn hasZeroedGrainGte(): bool {
        return (_.flags & FLAG_ZEROED_GRAIN_GTE) != 0;
    }

    /// Zero-copy extraction of the embedded descriptor text from `image` (trimmed at first NUL byte).
    fn embeddedDescriptor(image: []const u8): ([]const u8)? {
        if (_.descriptor_offset == 0 || _.descriptor_size == 0) {
            return null;
        }
        const byte_off = _.descriptorByteOffset();
        const byte_len = _.descriptorByteSize();
        if (byte_off > (image.len as u64)) {
            return null;
        }
        const off = byte_off as usize;
        const avail = (image.len - off) as u64;
        if (byte_len > avail) {
            return null;
        }
        const sz = byte_len as usize;
        return stripNul(image[off..(off + sz)]);
    }
}

/// Returns `true` if `data` starts with the 4-byte VMDK sparse magic (`"KDMV"` = `0x564D444B` LE).
pub fn isVmdkSparse(data: []const u8): bool {
    return data.len >= 4 && readU32Le(data, 0) == VMDK_MAGIC;
}

/// Parses a 512-byte VMDK SparseExtentHeader with strict validation:
/// - `"KDMV"` magic (`0x564D444B` LE)
/// - `version` in `1..=3`
/// - `grain_size_sectors >= 8` and a power of 2
/// - `num_gtes_per_gt == 512`
/// - 4-byte newline detector (`\n`, `' '`, `\r`, `\n`) when `FLAG_VALID_NEWLINE_DETECTOR` is set
/// - `compress_algorithm` in `{COMPRESS_NONE, COMPRESS_DEFLATE}`
pub fn parseVmdkSparseHeader(data: []const u8): VmdkSparseHeader? {
    if (data.len < VMDK_HEADER_SIZE || !isVmdkSparse(data)) {
        return null;
    }
    const version = readU32Le(data, 4);
    if (version < 1 || version > 3) {
        return null;
    }
    const flags = readU32Le(data, 8);
    const capacity_sectors = readU64Le(data, 12);
    const grain_size_sectors = readU64Le(data, 20);
    const descriptor_offset = readU64Le(data, 28);
    const descriptor_size = readU64Le(data, 36);
    const num_gtes_per_gt = readU32Le(data, 44);
    const rgd_offset = readU64Le(data, 48);
    const gd_offset = readU64Le(data, 56);
    const overhead_sectors = readU64Le(data, 64);
    const unclean_shutdown = data[72] != 0;

    // Validate grain_size_sectors: >= 8 and power of 2
    if (grain_size_sectors < 8 || (grain_size_sectors & (grain_size_sectors - 1)) != 0) {
        return null;
    }
    // Validate num_gtes_per_gt == 512
    if (num_gtes_per_gt != VMDK_DEFAULT_GTES_PER_GT) {
        return null;
    }
    // Validate 4-byte newline detector if FLAG_VALID_NEWLINE_DETECTOR is set
    if ((flags & FLAG_VALID_NEWLINE_DETECTOR) != 0) {
        if (data[73] != 0x0A || data[74] != 0x20 || data[75] != 0x0D || data[76] != 0x0A) {
            return null;
        }
    }
    const compress_algorithm = readU16Le(data, 77);
    if (compress_algorithm != COMPRESS_NONE && compress_algorithm != COMPRESS_DEFLATE) {
        return null;
    }

    return VmdkSparseHeader{
        .version = version,
        .flags = flags,
        .capacity_sectors = capacity_sectors,
        .grain_size_sectors = grain_size_sectors,
        .descriptor_offset = descriptor_offset,
        .descriptor_size = descriptor_size,
        .num_gtes_per_gt = num_gtes_per_gt,
        .rgd_offset = rgd_offset,
        .gd_offset = gd_offset,
        .overhead_sectors = overhead_sectors,
        .unclean_shutdown = unclean_shutdown,
        .compress_algorithm = compress_algorithm,
    };
}

/// Single parsed VMDK Extent description line (zero-copy slices into descriptor text).
pub struct VmdkExtent {
    access: []const u8,
    sectors: u64,
    extent_type: []const u8,
    filename: []const u8,
    offset: u64,
    has_offset: bool,
}

fn isValidAccess(tok: []const u8): bool {
    return bytesEq(tok, "RW") || bytesEq(tok, "RDONLY") || bytesEq(tok, "NOACCESS");
}

fn isValidExtentType(tok: []const u8): bool {
    if (tok.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < tok.len) {
        const c = tok[i];
        const is_upper = c >= 0x41 && c <= 0x5A;
        const is_digit = c >= 0x30 && c <= 0x39;
        if (!is_upper && !is_digit && c != 0x5F) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parses a single VMDK Extent description line, e.g.:
/// - `RW 20971520 SPARSE "disk-s001.vmdk"`
/// - `RW 4096 FLAT "disk-flat.vmdk" 0`
/// - `RDONLY 1024 ZERO`
pub fn parseVmdkExtentLine(line: []const u8): VmdkExtent? {
    const s = trimSpace(line);
    if (s.len == 0 || s[0] == 0x23) { // '#'
        return null;
    }

    let pos: usize = 0;

    // 1. access token
    let tok_start = pos;
    while (pos < s.len && !isHspace(s[pos])) {
        pos += 1;
    }
    const access = s[tok_start..pos];
    if (!isValidAccess(access)) {
        return null;
    }

    while (pos < s.len && isHspace(s[pos])) {
        pos += 1;
    }
    if (pos >= s.len) {
        return null;
    }

    // 2. sectors token
    tok_start = pos;
    while (pos < s.len && !isHspace(s[pos])) {
        pos += 1;
    }
    const sectors = parseDecU64(s[tok_start..pos]) ?? return null;
    if (sectors == 0) {
        return null;
    }

    while (pos < s.len && isHspace(s[pos])) {
        pos += 1;
    }
    if (pos >= s.len) {
        return null;
    }

    // 3. extent_type token
    tok_start = pos;
    while (pos < s.len && !isHspace(s[pos])) {
        pos += 1;
    }
    const extent_type = s[tok_start..pos];
    if (!isValidExtentType(extent_type)) {
        return null;
    }

    while (pos < s.len && isHspace(s[pos])) {
        pos += 1;
    }

    // Optional filename (only omitted for ZERO extents)
    if (pos >= s.len) {
        if (!bytesEq(extent_type, "ZERO")) {
            return null;
        }
        return VmdkExtent{
            .access = access,
            .sectors = sectors,
            .extent_type = extent_type,
            .filename = s[0..0],
            .offset = 0,
            .has_offset = false,
        };
    }

    // 4. filename (quoted or bare token)
    let filename: []const u8 = s[0..0];
    if (s[pos] == 0x22) { // '"'
        pos += 1;
        const name_start = pos;
        while (pos < s.len && s[pos] != 0x22) {
            pos += 1;
        }
        if (pos >= s.len || s[pos] != 0x22) {
            return null;
        }
        filename = s[name_start..pos];
        pos += 1;
    } else {
        const name_start = pos;
        while (pos < s.len && !isHspace(s[pos])) {
            pos += 1;
        }
        filename = s[name_start..pos];
    }

    while (pos < s.len && isHspace(s[pos])) {
        pos += 1;
    }

    // 5. optional offset
    let offset: u64 = 0;
    let has_offset = false;
    if (pos < s.len) {
        tok_start = pos;
        while (pos < s.len && !isHspace(s[pos])) {
            pos += 1;
        }
        const off_val = parseDecU64(s[tok_start..pos]) ?? return null;
        offset = off_val;
        has_offset = true;

        while (pos < s.len && isHspace(s[pos])) {
            pos += 1;
        }
        // Reject trailing garbage tokens
        if (pos < s.len) {
            return null;
        }
    }

    return VmdkExtent{
        .access = access,
        .sectors = sectors,
        .extent_type = extent_type,
        .filename = filename,
        .offset = offset,
        .has_offset = has_offset,
    };
}

/// Zero-copy iterator over Extent description lines in a VMDK text descriptor.
pub struct VmdkExtentIter {
    data: []const u8,
    pos: usize,

    pub fn init(data: []const u8): VmdkExtentIter {
        return VmdkExtentIter{
            .data = stripNul(data),
            .pos = 0,
        };
    }

    fn next(*_): VmdkExtent? {
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            while (_.pos < _.data.len && _.data[_.pos] != 0x0A) {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.data.len && _.data[_.pos] == 0x0A) {
                _.pos += 1;
            }
            const line = trimSpace(_.data[line_start..line_end]);
            if (line.len == 0 || line[0] == 0x23) {
                continue;
            }
            // Skip key=value lines
            if (indexOfByte(line, 0x3D) != null) {
                continue;
            }
            if (ext := parseVmdkExtentLine(line)) {
                return ext;
            }
        }
        return null;
    }
}

/// Parsed VMDK Text Descriptor metadata (zero-copy slices into `raw`).
pub struct VmdkDescriptor {
    version: u32,
    cid: u32,
    parent_cid: u32,
    create_type: []const u8,
    parent_filename_hint: []const u8,
    raw: []const u8,

    /// Returns `true` if `parent_cid != CID_NONE (0xFFFFFFFF)`.
    fn hasParent(): bool {
        return _.parent_cid != CID_NONE;
    }

    /// Returns a zero-copy iterator over all Extent description lines in the descriptor.
    fn extents(): VmdkExtentIter {
        return VmdkExtentIter.init(_.raw);
    }

    /// Finds a `ddb.<key> = "..."` entry in the descriptor and returns the unquoted value slice.
    /// Accepts `key` either without prefix (e.g. `"adapterType"`, `"geometry.cylinders"`)
    /// or with `"ddb."` prefix (e.g. `"ddb.adapterType"`).
    fn findDdb(key: []const u8): ([]const u8)? {
        if (key.len == 0) {
            return null;
        }
        const key_has_prefix = startsWith(key, "ddb.");
        let pos: usize = 0;
        while (pos < _.raw.len) {
            const line_start = pos;
            while (pos < _.raw.len && _.raw[pos] != 0x0A) {
                pos += 1;
            }
            const line_end = pos;
            if (pos < _.raw.len && _.raw[pos] == 0x0A) {
                pos += 1;
            }
            const line = trimSpace(_.raw[line_start..line_end]);
            if (line.len == 0 || line[0] == 0x23) {
                continue;
            }
            if (eq_pos := indexOfByte(line, 0x3D)) {
                const lhs = trimSpace(line[0..eq_pos]);
                if (!startsWith(lhs, "ddb.")) {
                    continue;
                }
                let matched = false;
                if (key_has_prefix) {
                    matched = bytesEq(lhs, key);
                } else {
                    matched = bytesEq(lhs[4..lhs.len], key);
                }
                if (matched) {
                    const rhs = line[(eq_pos + 1)..line.len];
                    return unquoteValue(rhs);
                }
            }
        }
        return null;
    }
}

/// Parses a VMDK text descriptor (handling embedded NUL padding, CRLF/LF newlines,
/// comments, `version`, `CID`, `parentCID`, `createType`, `parentFileNameHint`).
pub fn parseVmdkDescriptor(text: []const u8): VmdkDescriptor? {
    const raw = stripNul(text);
    if (raw.len == 0) {
        return null;
    }

    let version: u32 = 0;
    let has_version = false;
    let cid: u32 = CID_NONE;
    let has_cid = false;
    let parent_cid: u32 = CID_NONE;
    let create_type: []const u8 = raw[0..0];
    let has_create_type = false;
    let parent_filename_hint: []const u8 = raw[0..0];

    let pos: usize = 0;
    while (pos < raw.len) {
        const line_start = pos;
        while (pos < raw.len && raw[pos] != 0x0A) {
            pos += 1;
        }
        const line_end = pos;
        if (pos < raw.len && raw[pos] == 0x0A) {
            pos += 1;
        }
        const line = trimSpace(raw[line_start..line_end]);
        if (line.len == 0 || line[0] == 0x23) {
            continue;
        }
        if (eq_pos := indexOfByte(line, 0x3D)) {
            const key = trimSpace(line[0..eq_pos]);
            const rhs_raw = line[(eq_pos + 1)..line.len];
            if (bytesEq(key, "version")) {
                const v_str = unquoteValue(rhs_raw) ?? return null;
                const v = parseDecU32(v_str) ?? return null;
                if (v == 0) {
                    return null;
                }
                version = v;
                has_version = true;
            } else if (bytesEq(key, "CID")) {
                const c_str = unquoteValue(rhs_raw) ?? return null;
                const c = parseHexU32(c_str) ?? return null;
                cid = c;
                has_cid = true;
            } else if (bytesEq(key, "parentCID")) {
                const p_str = unquoteValue(rhs_raw) ?? return null;
                const p = parseHexU32(p_str) ?? return null;
                parent_cid = p;
            } else if (bytesEq(key, "createType")) {
                const ct = unquoteValue(rhs_raw) ?? return null;
                if (ct.len == 0) {
                    return null;
                }
                create_type = ct;
                has_create_type = true;
            } else if (bytesEq(key, "parentFileNameHint")) {
                const hint = unquoteValue(rhs_raw) ?? return null;
                parent_filename_hint = hint;
            }
        }
    }

    if (!has_version || !has_cid || !has_create_type) {
        return null;
    }

    return VmdkDescriptor{
        .version = version,
        .cid = cid,
        .parent_cid = parent_cid,
        .create_type = create_type,
        .parent_filename_hint = parent_filename_hint,
        .raw = raw,
    };
}
