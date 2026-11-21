// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc HEIF / HEIC ISO BMFF Container Parser)

pub const HEIF_BOX_HEADER_SIZE: usize = 8;
pub const HEIF_EXT_BOX_HEADER_SIZE: usize = 16;
pub const HEIF_HVCC_MIN_SIZE: usize = 23;

pub struct HeifBrandInfo {
    major_brand: [4]u8,
    minor_version: u32,
    is_sequence: bool,
    is_heic: bool,
}

pub struct HevcConfig {
    configuration_version: u8,
    general_profile_space: u8,
    general_tier_flag: bool,
    general_profile_idc: u8,
    general_level_idc: u8,
    chroma_format_idc: u8,
    bit_depth_luma: u8,
    bit_depth_chroma: u8,
    length_size_minus_one: u8,
    num_of_arrays: u8,
}

pub struct HeifColr {
    color_type: [4]u8,
    color_primaries: u16,
    transfer_characteristics: u16,
    matrix_coefficients: u16,
    full_range: bool,
}

pub struct HeifInfo {
    brand: HeifBrandInfo,
    width: u32,
    height: u32,
    pixi_channels: u8,
    pixi_bit_depth: u8,
    hvcc: HevcConfig?,
    colr: HeifColr?,
    has_icc_profile: bool,
    rotation_degrees: u16,
    has_alpha: bool,
    has_depth: bool,
}

struct BmffBox {
    box_type: [4]u8,
    payload: []const u8,
    next_pos: usize,
}

fn readU16BE(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readU32BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn readU64BE(buf: []const u8, off: usize): u64 {
    const hi: u64 = readU32BE(buf, off) as u64;
    const lo: u64 = readU32BE(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn fourccEq(a: [4]u8, b0: u8, b1: u8, b2: u8, b3: u8): bool {
    return a[0] == b0 && a[1] == b1 && a[2] == b2 && a[3] == b3;
}

fn readFourcc(buf: []const u8, off: usize): [4]u8 {
    return [buf[off], buf[off + 1], buf[off + 2], buf[off + 3]];
}

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn trimNul(bytes: []const u8): []const u8 {
    let end: usize = 0;
    while (end < bytes.len && bytes[end] != 0) {
        end += 1;
    }
    return bytes[0..end];
}

fn isAlphaUrn(urn_bytes: []const u8): bool {
    const t = trimNul(urn_bytes);
    if (sliceEq(t, "urn:mpeg:hevc:2015:auxid:1")) {
        return true;
    }
    if (sliceEq(t, "urn:mpeg:mpegB:cicp:systems:auxiliary:alpha")) {
        return true;
    }
    return false;
}

fn isDepthUrn(urn_bytes: []const u8): bool {
    const t = trimNul(urn_bytes);
    if (sliceEq(t, "urn:mpeg:hevc:2015:auxid:2")) {
        return true;
    }
    if (sliceEq(t, "urn:mpeg:mpegB:cicp:systems:auxiliary:depth")) {
        return true;
    }
    return false;
}

fn readBoxAt(data: []const u8, pos: usize): BmffBox? {
    if (pos > data.len || data.len - pos < HEIF_BOX_HEADER_SIZE) {
        return null;
    }

    const size32: u32 = readU32BE(data, pos);
    const btype = readFourcc(data, pos + 4);
    const rem: usize = data.len - pos;

    let hdr_len: usize = HEIF_BOX_HEADER_SIZE;
    let box_len: usize = 0;

    if (size32 == 0) {
        box_len = rem;
    } else if (size32 == 1) {
        if (rem < HEIF_EXT_BOX_HEADER_SIZE) {
            return null;
        }
        const size64: u64 = readU64BE(data, pos + 8);
        if (size64 < (HEIF_EXT_BOX_HEADER_SIZE as u64) || size64 > (rem as u64)) {
            return null;
        }
        hdr_len = HEIF_EXT_BOX_HEADER_SIZE;
        box_len = size64 as usize;
    } else {
        const s: usize = size32 as usize;
        if (s < HEIF_BOX_HEADER_SIZE || s > rem) {
            return null;
        }
        box_len = s;
    }

    const payload_start = pos + hdr_len;
    const box_end = pos + box_len;

    return BmffBox{
        .box_type = btype,
        .payload = data[payload_start..box_end],
        .next_pos = box_end,
    };
}

fn isExplicitHevcBrand(b: [4]u8): bool {
    return fourccEq(b, 'h', 'e', 'i', 'c') ||
        fourccEq(b, 'h', 'e', 'i', 'x') ||
        fourccEq(b, 'h', 'e', 'v', 'c') ||
        fourccEq(b, 'h', 'e', 'v', 'x') ||
        fourccEq(b, 'h', 'e', 'i', 'm') ||
        fourccEq(b, 'h', 'e', 'i', 's') ||
        fourccEq(b, 'h', 'e', 'v', 'm') ||
        fourccEq(b, 'h', 'e', 'v', 's');
}

fn isGenericHeifBrand(b: [4]u8): bool {
    return fourccEq(b, 'm', 'i', 'f', '1') || fourccEq(b, 'm', 's', 'f', '1');
}

fn isAvifBrand(b: [4]u8): bool {
    return fourccEq(b, 'a', 'v', 'i', 'f') || fourccEq(b, 'a', 'v', 'i', 's');
}

fn isSequenceBrand(b: [4]u8): bool {
    return fourccEq(b, 'm', 's', 'f', '1') ||
        fourccEq(b, 'h', 'e', 'v', 'c') ||
        fourccEq(b, 'h', 'e', 'v', 's');
}

fn isHeicBrand(b: [4]u8): bool {
    return fourccEq(b, 'h', 'e', 'i', 'c') || fourccEq(b, 'h', 'e', 'i', 'x');
}

pub fn parseHeifBrand(data: []const u8): HeifBrandInfo? {
    const first = readBoxAt(data, 0) ?? return null;
    if (!fourccEq(first.box_type, 'f', 't', 'y', 'p')) {
        return null;
    }
    if (first.payload.len < 8) {
        return null;
    }

    const major = readFourcc(first.payload, 0);
    const minor_version = readU32BE(first.payload, 4);

    let has_explicit_hevc = isExplicitHevcBrand(major);
    let has_generic_heif = isGenericHeifBrand(major);
    let has_avif = isAvifBrand(major);
    let is_sequence = isSequenceBrand(major);
    let is_heic = isHeicBrand(major);

    let off: usize = 8;
    while (off + 4 <= first.payload.len) {
        const b = readFourcc(first.payload, off);
        if (isExplicitHevcBrand(b)) {
            has_explicit_hevc = true;
        }
        if (isGenericHeifBrand(b)) {
            has_generic_heif = true;
        }
        if (isAvifBrand(b)) {
            has_avif = true;
        }
        if (isSequenceBrand(b)) {
            is_sequence = true;
        }
        if (isHeicBrand(b)) {
            is_heic = true;
        }
        off += 4;
    }

    // Reject pure AVIF/AVIS containers (avif/avis without explicit HEVC/HEIC brands)
    if (has_avif && !has_explicit_hevc) {
        return null;
    }

    if (!has_explicit_hevc && !has_generic_heif) {
        return null;
    }

    return HeifBrandInfo{
        .major_brand = major,
        .minor_version = minor_version,
        .is_sequence = is_sequence,
        .is_heic = is_heic,
    };
}

pub fn isHeif(data: []const u8): bool {
    if (_brand := parseHeifBrand(data)) {
        return true;
    }
    return false;
}

pub fn parseHvcC(payload: []const u8): HevcConfig? {
    if (payload.len < HEIF_HVCC_MIN_SIZE) {
        return null;
    }
    const configuration_version = payload[0];
    if (configuration_version != 1) {
        return null;
    }

    const b1 = payload[1];
    const general_profile_space: u8 = (b1 >> 6) & 0x03;
    const general_tier_flag = (b1 & 0x20) != 0;
    const general_profile_idc: u8 = b1 & 0x1F;

    const general_level_idc: u8 = payload[12];
    const chroma_format_idc: u8 = payload[16] & 0x03;
    const bit_depth_luma: u8 = (payload[17] & 0x07) + (8 as u8);
    const bit_depth_chroma: u8 = (payload[18] & 0x07) + (8 as u8);
    const length_size_minus_one: u8 = payload[21] & 0x03;
    const num_of_arrays: u8 = payload[22];

    return HevcConfig{
        .configuration_version = configuration_version,
        .general_profile_space = general_profile_space,
        .general_tier_flag = general_tier_flag,
        .general_profile_idc = general_profile_idc,
        .general_level_idc = general_level_idc,
        .chroma_format_idc = chroma_format_idc,
        .bit_depth_luma = bit_depth_luma,
        .bit_depth_chroma = bit_depth_chroma,
        .length_size_minus_one = length_size_minus_one,
        .num_of_arrays = num_of_arrays,
    };
}

fn parseColrPayload(payload: []const u8): HeifColr? {
    if (payload.len < 4) {
        return null;
    }
    const ctype = readFourcc(payload, 0);
    if (fourccEq(ctype, 'n', 'c', 'l', 'x')) {
        if (payload.len < 11) {
            return null;
        }
        return HeifColr{
            .color_type = ctype,
            .color_primaries = readU16BE(payload, 4),
            .transfer_characteristics = readU16BE(payload, 6),
            .matrix_coefficients = readU16BE(payload, 8),
            .full_range = (payload[10] & 0x80) != 0,
        };
    }
    if (fourccEq(ctype, 'r', 'I', 'C', 'C') || fourccEq(ctype, 'p', 'r', 'o', 'f')) {
        return HeifColr{
            .color_type = ctype,
            .color_primaries = 0,
            .transfer_characteristics = 0,
            .matrix_coefficients = 0,
            .full_range = false,
        };
    }
    return null;
}

fn parseIpco(ipco_payload: []const u8, info: *HeifInfo) {
    let pos: usize = 0;
    while (pos < ipco_payload.len) {
        const prop = readBoxAt(ipco_payload, pos) ?? break;
        pos = prop.next_pos;

        if (fourccEq(prop.box_type, 'i', 's', 'p', 'e')) {
            if (prop.payload.len >= 12 && info.width == 0 && info.height == 0) {
                info.width = readU32BE(prop.payload, 4);
                info.height = readU32BE(prop.payload, 8);
            }
        } else if (fourccEq(prop.box_type, 'p', 'i', 'x', 'i')) {
            if (prop.payload.len >= 5 && info.pixi_channels == 0) {
                const nch = prop.payload[4];
                if (nch > 0 && prop.payload.len >= 5 + (nch as usize)) {
                    info.pixi_channels = nch;
                    info.pixi_bit_depth = prop.payload[5];
                }
            }
        } else if (fourccEq(prop.box_type, 'h', 'v', 'c', 'C')) {
            if (info.hvcc == null) {
                if (cfg := parseHvcC(prop.payload)) {
                    info.hvcc = cfg;
                }
            }
        } else if (fourccEq(prop.box_type, 'c', 'o', 'l', 'r')) {
            if (c := parseColrPayload(prop.payload)) {
                if (fourccEq(c.color_type, 'r', 'I', 'C', 'C') || fourccEq(c.color_type, 'p', 'r', 'o', 'f')) {
                    info.has_icc_profile = true;
                }
                if (info.colr == null) {
                    info.colr = c;
                }
            }
        } else if (fourccEq(prop.box_type, 'i', 'r', 'o', 't')) {
            if (prop.payload.len >= 1) {
                const angle_code: u16 = (prop.payload[0] & 0x03) as u16;
                info.rotation_degrees = angle_code * 90;
            }
        } else if (fourccEq(prop.box_type, 'a', 'u', 'x', 'C')) {
            if (prop.payload.len > 4) {
                const urn = prop.payload[4..];
                if (isAlphaUrn(urn)) {
                    info.has_alpha = true;
                } else if (isDepthUrn(urn)) {
                    info.has_depth = true;
                }
            }
        }
    }
}

fn parseIprp(iprp_payload: []const u8, info: *HeifInfo) {
    let pos: usize = 0;
    while (pos < iprp_payload.len) {
        const child = readBoxAt(iprp_payload, pos) ?? break;
        pos = child.next_pos;
        if (fourccEq(child.box_type, 'i', 'p', 'c', 'o')) {
            parseIpco(child.payload, info);
        }
    }
}

pub fn parseHeifInfo(data: []const u8): HeifInfo? {
    const brand = parseHeifBrand(data) ?? return null;
    const ftyp_box = readBoxAt(data, 0) ?? return null;

    let info = HeifInfo{
        .brand = brand,
        .width = 0,
        .height = 0,
        .pixi_channels = 0,
        .pixi_bit_depth = 0,
        .hvcc = null,
        .colr = null,
        .has_icc_profile = false,
        .rotation_degrees = 0,
        .has_alpha = false,
        .has_depth = false,
    };

    let found_meta = false;
    let pos: usize = ftyp_box.next_pos;
    while (pos < data.len) {
        const top = readBoxAt(data, pos) ?? return null;
        pos = top.next_pos;

        if (fourccEq(top.box_type, 'm', 'e', 't', 'a')) {
            if (top.payload.len < 4) {
                return null;
            }
            found_meta = true;
            const meta_children = top.payload[4..];
            let mpos: usize = 0;
            while (mpos < meta_children.len) {
                const sub = readBoxAt(meta_children, mpos) ?? return null;
                mpos = sub.next_pos;
                if (fourccEq(sub.box_type, 'i', 'p', 'r', 'p')) {
                    parseIprp(sub.payload, *info);
                }
            }
            break;
        }
    }

    if (!found_meta) {
        return null;
    }

    return info;
}
