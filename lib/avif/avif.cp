// Origin: Pure-CP Stdlib (Zero-Alloc AVIF / MIAF ISO BMFF Container Parser) | Created: 2026-11-22

pub const AVIF_BOX_HEADER_SIZE: usize = 8;
pub const AVIF_EXT_BOX_HEADER_SIZE: usize = 16;

pub struct AvifBrandInfo {
    major_brand: [4]u8,
    minor_version: u32,
    is_sequence: bool,
    has_mif1: bool,
    has_miaf: bool,
}

pub struct Av1Config {
    marker_ok: bool,
    version: u8,
    seq_profile: u8,
    seq_level_idx_0: u8,
    seq_tier_0: u8,
    high_bitdepth: bool,
    twelve_bit: bool,
    monochrome: bool,
    chroma_subsampling_x: bool,
    chroma_subsampling_y: bool,
    chroma_sample_position: u8,

    fn bitDepth(): u8 {
        if (_.high_bitdepth) {
            if (_.twelve_bit) {
                return 12;
            }
            return 10;
        }
        return 8;
    }
}

pub struct AvifColr {
    color_type: [4]u8,
    color_primaries: u16,
    transfer_characteristics: u16,
    matrix_coefficients: u16,
    full_range: bool,
}

pub struct AvifInfo {
    brand: AvifBrandInfo,
    width: u32,
    height: u32,
    pixi_channels: u8,
    pixi_bit_depth: u8,
    av1c: Av1Config?,
    colr: AvifColr?,
    has_icc_profile: bool,
    rotation_degrees: u16,
    has_alpha: bool,
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

fn sliceStartsWith(haystack: []const u8, needle: []const u8): bool {
    if (haystack.len < needle.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (haystack[i] != needle[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isAlphaAuxUrn(urn_bytes: []const u8): bool {
    // Trim at first NUL terminator if present
    let end: usize = 0;
    while (end < urn_bytes.len && urn_bytes[end] != 0) {
        end += 1;
    }
    const trimmed = urn_bytes[0..end];
    const avif_alpha = "urn:mpeg:mpegB:cicp:systems:auxiliary:alpha";
    const hevc_alpha = "urn:mpeg:hevc:2015:auxid:1";
    if (trimmed.len == avif_alpha.len && sliceStartsWith(trimmed, avif_alpha)) {
        return true;
    }
    if (trimmed.len == hevc_alpha.len && sliceStartsWith(trimmed, hevc_alpha)) {
        return true;
    }
    return false;
}

fn readBoxAt(data: []const u8, pos: usize): BmffBox? {
    if (pos > data.len || data.len - pos < AVIF_BOX_HEADER_SIZE) {
        return null;
    }

    const size32: u32 = readU32BE(data, pos);
    const btype = readFourcc(data, pos + 4);
    const rem: usize = data.len - pos;

    let hdr_len: usize = AVIF_BOX_HEADER_SIZE;
    let box_len: usize = 0;

    if (size32 == 0) {
        box_len = rem;
    } else if (size32 == 1) {
        if (rem < AVIF_EXT_BOX_HEADER_SIZE) {
            return null;
        }
        const size64: u64 = readU64BE(data, pos + 8);
        if (size64 < (AVIF_EXT_BOX_HEADER_SIZE as u64) || size64 > (rem as u64)) {
            return null;
        }
        hdr_len = AVIF_EXT_BOX_HEADER_SIZE;
        box_len = size64 as usize;
    } else {
        const s: usize = size32 as usize;
        if (s < AVIF_BOX_HEADER_SIZE || s > rem) {
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

pub fn parseAvifBrand(data: []const u8): AvifBrandInfo? {
    const first = readBoxAt(data, 0) ?? return null;
    if (!fourccEq(first.box_type, 'f', 't', 'y', 'p')) {
        return null;
    }
    if (first.payload.len < 8) {
        return null;
    }

    const major = readFourcc(first.payload, 0);
    const minor_version = readU32BE(first.payload, 4);

    let matched_avif = false;
    let is_sequence = false;
    let has_mif1 = false;
    let has_miaf = false;

    if (fourccEq(major, 'a', 'v', 'i', 'f')) {
        matched_avif = true;
    } else if (fourccEq(major, 'a', 'v', 'i', 's')) {
        matched_avif = true;
        is_sequence = true;
    } else if (fourccEq(major, 'm', 'i', 'f', '1')) {
        has_mif1 = true;
    } else if (fourccEq(major, 'm', 'i', 'a', 'f')) {
        has_miaf = true;
    }

    let off: usize = 8;
    while (off + 4 <= first.payload.len) {
        const b = readFourcc(first.payload, off);
        if (fourccEq(b, 'a', 'v', 'i', 'f')) {
            matched_avif = true;
        } else if (fourccEq(b, 'a', 'v', 'i', 's')) {
            matched_avif = true;
            is_sequence = true;
        } else if (fourccEq(b, 'm', 'i', 'f', '1')) {
            has_mif1 = true;
        } else if (fourccEq(b, 'm', 'i', 'a', 'f')) {
            has_miaf = true;
        }
        off += 4;
    }

    if (!matched_avif) {
        return null;
    }

    return AvifBrandInfo{
        .major_brand = major,
        .minor_version = minor_version,
        .is_sequence = is_sequence,
        .has_mif1 = has_mif1,
        .has_miaf = has_miaf,
    };
}

pub fn isAvif(data: []const u8): bool {
    if (_brand := parseAvifBrand(data)) {
        return true;
    }
    return false;
}

pub fn parseAv1C(payload: []const u8): Av1Config? {
    if (payload.len < 4) {
        return null;
    }
    const b0 = payload[0];
    const b1 = payload[1];
    const b2 = payload[2];

    const marker_ok = (b0 & 0x80) != 0;
    const version: u8 = b0 & 0x7F;
    const seq_profile: u8 = (b1 >> 5) & 0x07;
    const seq_level_idx_0: u8 = b1 & 0x1F;
    const seq_tier_0: u8 = (b2 >> 7) & 0x01;
    const high_bitdepth = (b2 & 0x40) != 0;
    const twelve_bit = (b2 & 0x20) != 0;
    const monochrome = (b2 & 0x10) != 0;
    const chroma_subsampling_x = (b2 & 0x08) != 0;
    const chroma_subsampling_y = (b2 & 0x04) != 0;
    const chroma_sample_position: u8 = b2 & 0x03;

    return Av1Config{
        .marker_ok = marker_ok,
        .version = version,
        .seq_profile = seq_profile,
        .seq_level_idx_0 = seq_level_idx_0,
        .seq_tier_0 = seq_tier_0,
        .high_bitdepth = high_bitdepth,
        .twelve_bit = twelve_bit,
        .monochrome = monochrome,
        .chroma_subsampling_x = chroma_subsampling_x,
        .chroma_subsampling_y = chroma_subsampling_y,
        .chroma_sample_position = chroma_sample_position,
    };
}

fn parseColrPayload(payload: []const u8): AvifColr? {
    if (payload.len < 4) {
        return null;
    }
    const ctype = readFourcc(payload, 0);
    if (fourccEq(ctype, 'n', 'c', 'l', 'x')) {
        if (payload.len < 11) {
            return null;
        }
        return AvifColr{
            .color_type = ctype,
            .color_primaries = readU16BE(payload, 4),
            .transfer_characteristics = readU16BE(payload, 6),
            .matrix_coefficients = readU16BE(payload, 8),
            .full_range = (payload[10] & 0x80) != 0,
        };
    }
    if (fourccEq(ctype, 'r', 'I', 'C', 'C') || fourccEq(ctype, 'p', 'r', 'o', 'f')) {
        return AvifColr{
            .color_type = ctype,
            .color_primaries = 0,
            .transfer_characteristics = 0,
            .matrix_coefficients = 0,
            .full_range = false,
        };
    }
    return null;
}

fn parseIpco(ipco_payload: []const u8, info: *AvifInfo) {
    let pos: usize = 0;
    while (pos < ipco_payload.len) {
        const prop = readBoxAt(ipco_payload, pos) ?? break;
        pos = prop.next_pos;

        if (fourccEq(prop.box_type, 'i', 's', 'p', 'e')) {
            // FullBox (4B) + width (4B) + height (4B)
            if (prop.payload.len >= 12 && info.width == 0 && info.height == 0) {
                info.width = readU32BE(prop.payload, 4);
                info.height = readU32BE(prop.payload, 8);
            }
        } else if (fourccEq(prop.box_type, 'p', 'i', 'x', 'i')) {
            // FullBox (4B) + num_channels (1B) + bits_per_channel[num_channels]
            if (prop.payload.len >= 5 && info.pixi_channels == 0) {
                const nch = prop.payload[4];
                if (nch > 0 && prop.payload.len >= 5 + (nch as usize)) {
                    info.pixi_channels = nch;
                    info.pixi_bit_depth = prop.payload[5];
                }
            }
        } else if (fourccEq(prop.box_type, 'a', 'v', '1', 'C')) {
            if (info.av1c == null) {
                if (cfg := parseAv1C(prop.payload)) {
                    info.av1c = cfg;
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
            // FullBox (4B) + NUL-terminated aux_type URN
            if (prop.payload.len > 4) {
                if (isAlphaAuxUrn(prop.payload[4..])) {
                    info.has_alpha = true;
                }
            }
        }
    }
}

fn parseIprp(iprp_payload: []const u8, info: *AvifInfo) {
    let pos: usize = 0;
    while (pos < iprp_payload.len) {
        const child = readBoxAt(iprp_payload, pos) ?? break;
        pos = child.next_pos;
        if (fourccEq(child.box_type, 'i', 'p', 'c', 'o')) {
            parseIpco(child.payload, info);
        }
    }
}

pub fn parseAvifInfo(data: []const u8): AvifInfo? {
    const brand = parseAvifBrand(data) ?? return null;
    const ftyp_box = readBoxAt(data, 0) ?? return null;

    let info = AvifInfo{
        .brand = brand,
        .width = 0,
        .height = 0,
        .pixi_channels = 0,
        .pixi_bit_depth = 0,
        .av1c = null,
        .colr = null,
        .has_icc_profile = false,
        .rotation_degrees = 0,
        .has_alpha = false,
    };

    let found_meta = false;
    let pos: usize = ftyp_box.next_pos;
    while (pos < data.len) {
        const top = readBoxAt(data, pos) ?? return null;
        pos = top.next_pos;

        if (fourccEq(top.box_type, 'm', 'e', 't', 'a')) {
            // meta is a FullBox (4-byte version + flags)
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
