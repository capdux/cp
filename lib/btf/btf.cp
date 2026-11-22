// Origin: Pure-CP Stdlib (Zero-Alloc Linux BTF .BTF Type Graph Parser) | Created: 2026-11-22

pub const BTF_MAGIC: u16 = 0xEB9F;
pub const BTF_VERSION: u8 = 1;
pub const BTF_HEADER_SIZE: usize = 24;
pub const BTF_TYPE_HEADER_SIZE: usize = 12;

// BTF Kind ((info >> 24) & 0x1F)
pub const KIND_UNKN: u8 = 0;
pub const KIND_INT: u8 = 1;
pub const KIND_PTR: u8 = 2;
pub const KIND_ARRAY: u8 = 3;
pub const KIND_STRUCT: u8 = 4;
pub const KIND_UNION: u8 = 5;
pub const KIND_ENUM: u8 = 6;
pub const KIND_FWD: u8 = 7;
pub const KIND_TYPEDEF: u8 = 8;
pub const KIND_VOLATILE: u8 = 9;
pub const KIND_CONST: u8 = 10;
pub const KIND_RESTRICT: u8 = 11;
pub const KIND_FUNC: u8 = 12;
pub const KIND_FUNC_PROTO: u8 = 13;
pub const KIND_VAR: u8 = 14;
pub const KIND_DATASEC: u8 = 15;
pub const KIND_FLOAT: u8 = 16;
pub const KIND_DECL_TAG: u8 = 17;
pub const KIND_TYPE_TAG: u8 = 18;
pub const KIND_ENUM64: u8 = 19;

// INT Encoding Flags
pub const INT_SIGNED: u8 = 1;
pub const INT_CHAR: u8 = 2;
pub const INT_BOOL: u8 = 4;

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

fn kindExtraLen(kind: u8, vlen: u16): usize? {
    const n = vlen as usize;
    if (kind == KIND_INT) {
        return 4;
    }
    if (kind == KIND_PTR || kind == KIND_FWD || kind == KIND_TYPEDEF ||
        kind == KIND_VOLATILE || kind == KIND_CONST || kind == KIND_RESTRICT ||
        kind == KIND_FUNC || kind == KIND_FLOAT || kind == KIND_TYPE_TAG) {
        return 0;
    }
    if (kind == KIND_ARRAY) {
        return 12;
    }
    if (kind == KIND_STRUCT || kind == KIND_UNION) {
        return n * 12;
    }
    if (kind == KIND_ENUM || kind == KIND_FUNC_PROTO) {
        return n * 8;
    }
    if (kind == KIND_VAR || kind == KIND_DECL_TAG) {
        return 4;
    }
    if (kind == KIND_DATASEC || kind == KIND_ENUM64) {
        return n * 12;
    }
    return null;
}

/// Parsed 24-byte `.BTF` header (`struct btf_header`).
pub struct BtfHeader {
    magic: u16,
    version: u8,
    flags: u8,
    hdr_len: u32,
    type_off: u32,
    type_len: u32,
    str_off: u32,
    str_len: u32,
}

/// Decoded BTF type record (`struct btf_type`) with zero-copy trailing payload slice.
pub struct BtfType {
    type_id: u32,
    name_off: u32,
    kind: u8,
    vlen: u16,
    kflag: bool,
    size_or_type: u32,
    extra: []const u8,
}

/// Decoded `KIND_INT` 4-byte encoding metadata.
pub struct BtfIntInfo {
    encoding: u8,
    bit_offset: u8,
    bits: u8,

    fn isSigned(): bool {
        return (_.encoding & INT_SIGNED) != 0;
    }

    fn isChar(): bool {
        return (_.encoding & INT_CHAR) != 0;
    }

    fn isBool(): bool {
        return (_.encoding & INT_BOOL) != 0;
    }
}

/// Decoded `KIND_ARRAY` 12-byte payload (`struct btf_array`).
pub struct BtfArray {
    elem_type: u32,
    index_type: u32,
    nelems: u32,
}

/// Decoded `KIND_STRUCT` / `KIND_UNION` 12-byte member (`struct btf_member`).
pub struct BtfMember {
    name_off: u32,
    type_id: u32,
    raw_offset: u32,

    /// Returns the member's bit offset (masks low 24 bits when `kflag` is set).
    fn bitOffset(kflag: bool): u32 {
        if (kflag) {
            return _.raw_offset & 0x00FFFFFF;
        }
        return _.raw_offset;
    }

    /// Returns the member's bitfield width in bits (0 when `kflag` is false).
    fn bitfieldSize(kflag: bool): u8 {
        if (kflag) {
            return ((_.raw_offset >> 24) & 0xFF) as u8;
        }
        return 0;
    }
}

/// Zero-copy iterator over all types in a `.BTF` type section.
pub struct BtfTypeIter {
    data: []const u8,
    pos: usize,
    next_id: u32,
    err: bool,

    /// Returns true if iteration terminated due to an unknown kind or truncated payload.
    fn hasError(): bool {
        return _.err;
    }

    fn next(*_): BtfType? {
        if (_.pos >= _.data.len) {
            return null;
        }
        if (_.data.len - _.pos < BTF_TYPE_HEADER_SIZE) {
            _.err = true;
            _.pos = _.data.len;
            return null;
        }

        const off = _.pos;
        const name_off = readU32Le(_.data, off);
        const info = readU32Le(_.data, off + 4);
        const size_or_type = readU32Le(_.data, off + 8);

        const vlen = (info & 0xFFFF) as u16;
        const kind = ((info >> 24) & 0x1F) as u8;
        const kflag = (info >> 31) != 0;

        if (extra_len := kindExtraLen(kind, vlen)) {
            const extra_start = off + BTF_TYPE_HEADER_SIZE;
            if (_.data.len - extra_start < extra_len) {
                _.err = true;
                _.pos = _.data.len;
                return null;
            }
            const extra_end = extra_start + extra_len;
            const id = _.next_id;
            _.next_id += 1;
            _.pos = extra_end;
            return BtfType {
                type_id: id,
                name_off: name_off,
                kind: kind,
                vlen: vlen,
                kflag: kflag,
                size_or_type: size_or_type,
                extra: _.data[extra_start..extra_end],
            };
        }

        _.err = true;
        _.pos = _.data.len;
        return null;
    }
}

/// Zero-copy view of a parsed `.BTF` section.
pub struct BtfSection {
    header: BtfHeader,
    type_bytes: []const u8,
    str_bytes: []const u8,

    /// Returns the NUL-terminated string slice at `off` in the BTF string table.
    fn stringAt(off: u32): ([]const u8)? {
        const start = off as usize;
        if (start >= _.str_bytes.len) {
            return null;
        }
        let end = start;
        while (end < _.str_bytes.len && _.str_bytes[end] != 0) {
            end += 1;
        }
        if (end >= _.str_bytes.len) {
            return null;
        }
        return _.str_bytes[start..end];
    }

    /// Returns a zero-copy iterator over the BTF type graph (1-based `type_id`).
    fn types(): BtfTypeIter {
        return BtfTypeIter {
            data: _.type_bytes,
            pos: 0,
            next_id: 1,
            err: false,
        };
    }
}

/// Fast magic and minimum-length check for a `.BTF` section (`0xEB9F` LE at offset 0).
pub fn isBtf(data: []const u8): bool {
    if (data.len < BTF_HEADER_SIZE) {
        return false;
    }
    return readU16Le(data, 0) == BTF_MAGIC;
}

/// Parses and validates a `.BTF` section header and slices its type and string tables.
pub fn parseBtf(data: []const u8): BtfSection? {
    if (!isBtf(data)) {
        return null;
    }

    const version = data[2];
    const flags = data[3];
    const hdr_len = readU32Le(data, 4);
    const type_off = readU32Le(data, 8);
    const type_len = readU32Le(data, 12);
    const str_off = readU32Le(data, 16);
    const str_len = readU32Le(data, 20);

    if (version != BTF_VERSION) {
        return null;
    }
    const hl = hdr_len as usize;
    if (hl < BTF_HEADER_SIZE || hl > data.len) {
        return null;
    }

    const body_len = data.len - hl;
    const t_off = type_off as usize;
    const t_len = type_len as usize;
    const s_off = str_off as usize;
    const s_len = str_len as usize;

    if (t_off > body_len || t_len > body_len - t_off) {
        return null;
    }
    if (s_off > body_len || s_len > body_len - s_off) {
        return null;
    }

    return BtfSection {
        header: BtfHeader {
            magic: BTF_MAGIC,
            version: version,
            flags: flags,
            hdr_len: hdr_len,
            type_off: type_off,
            type_len: type_len,
            str_off: str_off,
            str_len: str_len,
        },
        type_bytes: data[hl + t_off .. hl + t_off + t_len],
        str_bytes: data[hl + s_off .. hl + s_off + s_len],
    };
}

/// Parses `KIND_INT` 4-byte metadata from a `BtfType`.
pub fn parseBtfInt(t: BtfType): BtfIntInfo? {
    if (t.kind != KIND_INT || t.extra.len < 4) {
        return null;
    }
    const word = readU32Le(t.extra, 0);
    return BtfIntInfo {
        encoding: ((word >> 24) & 0x0F) as u8,
        bit_offset: ((word >> 16) & 0xFF) as u8,
        bits: (word & 0xFF) as u8,
    };
}

/// Parses `KIND_ARRAY` 12-byte payload from a `BtfType`.
pub fn parseBtfArray(t: BtfType): BtfArray? {
    if (t.kind != KIND_ARRAY || t.extra.len < 12) {
        return null;
    }
    return BtfArray {
        elem_type: readU32Le(t.extra, 0),
        index_type: readU32Le(t.extra, 4),
        nelems: readU32Le(t.extra, 8),
    };
}

/// Extracts member `idx` from a `KIND_STRUCT` or `KIND_UNION` `BtfType`.
pub fn btfMemberAt(t: BtfType, idx: usize): BtfMember? {
    if (t.kind != KIND_STRUCT && t.kind != KIND_UNION) {
        return null;
    }
    if (idx >= (t.vlen as usize)) {
        return null;
    }
    const off = idx * 12;
    if (off + 12 > t.extra.len) {
        return null;
    }
    return BtfMember {
        name_off: readU32Le(t.extra, off),
        type_id: readU32Le(t.extra, off + 4),
        raw_offset: readU32Le(t.extra, off + 8),
    };
}
