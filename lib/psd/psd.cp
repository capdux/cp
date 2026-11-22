// Origin: Pure-CP Stdlib (Zero-Alloc Adobe Photoshop PSD/PSB Container Parser) | Created: 2026-11-22

pub const PSD_HEADER_SIZE: usize = 26;
pub const PSD_VERSION: u16 = 1;
pub const PSB_VERSION: u16 = 2;

pub const PSD_RES_RESOLUTION_INFO: u16 = 0x03ED;
pub const PSD_RES_ICC_PROFILE: u16 = 0x040F;
pub const PSD_RES_XMP_METADATA: u16 = 0x0424;

pub enum PsdColorMode: u16 {
    bitmap = 0,
    grayscale = 1,
    indexed = 2,
    rgb = 3,
    cmyk = 4,
    multichannel = 7,
    duotone = 8,
    lab = 9,
    unknown = 65535,
}

pub enum PsdCompression: u16 {
    raw = 0,
    rle = 1,
    zip = 2,
    zip_pred = 3,
    unknown = 65535,
}

pub fn psdColorModeFromInt(v: u16): PsdColorMode {
    if (v == 0) return PsdColorMode.bitmap;
    if (v == 1) return PsdColorMode.grayscale;
    if (v == 2) return PsdColorMode.indexed;
    if (v == 3) return PsdColorMode.rgb;
    if (v == 4) return PsdColorMode.cmyk;
    if (v == 7) return PsdColorMode.multichannel;
    if (v == 8) return PsdColorMode.duotone;
    if (v == 9) return PsdColorMode.lab;
    return PsdColorMode.unknown;
}

pub fn psdCompressionFromInt(v: u16): PsdCompression {
    if (v == 0) return PsdCompression.raw;
    if (v == 1) return PsdCompression.rle;
    if (v == 2) return PsdCompression.zip;
    if (v == 3) return PsdCompression.zip_pred;
    return PsdCompression.unknown;
}

pub struct PsdHeader {
    version: u16,
    channels: u16,
    height: u32,
    width: u32,
    depth: u16,
    color_mode: PsdColorMode,

    fn isPsb(): bool {
        return _.version == PSB_VERSION;
    }
}

pub struct PsdSections {
    header: PsdHeader,
    color_mode_data: []const u8,
    image_resources: []const u8,
    layer_and_mask: []const u8,
    image_data_compression: PsdCompression,
    image_data: []const u8,
}

pub struct PsdResource {
    id: u16,
    name: []const u8,
    payload: []const u8,
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
    const hi = readU32BE(buf, off) as u64;
    const lo = readU32BE(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

pub fn isPsd(data: []const u8): bool {
    if (data.len < PSD_HEADER_SIZE) {
        return false;
    }
    if (data[0] != '8' || data[1] != 'B' || data[2] != 'P' || data[3] != 'S') {
        return false;
    }
    const ver = readU16BE(data, 4);
    if (ver != PSD_VERSION && ver != PSB_VERSION) {
        return false;
    }
    // 6-byte reserved field must be all zeros
    let i: usize = 6;
    while (i < 12) {
        if (data[i] != 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn parsePsdHeader(data: []const u8): PsdHeader? {
    if (!isPsd(data)) {
        return null;
    }
    const version = readU16BE(data, 4);
    const channels = readU16BE(data, 12);
    const height = readU32BE(data, 14);
    const width = readU32BE(data, 18);
    const depth = readU16BE(data, 22);
    const raw_cm = readU16BE(data, 24);

    if (channels < 1 || channels > 56) {
        return null;
    }
    if (height == 0 || width == 0) {
        return null;
    }
    if (depth != 1 && depth != 8 && depth != 16 && depth != 32) {
        return null;
    }

    return PsdHeader{
        .version = version,
        .channels = channels,
        .height = height,
        .width = width,
        .depth = depth,
        .color_mode = psdColorModeFromInt(raw_cm),
    };
}

pub fn parsePsdSections(data: []const u8): PsdSections? {
    const hdr = parsePsdHeader(data) ?? return null;
    let pos: usize = PSD_HEADER_SIZE;

    // Section 2: Color Mode Data (4B length + payload)
    if (data.len - pos < 4) {
        return null;
    }
    const cm_len: usize = readU32BE(data, pos) as usize;
    pos += 4;
    if (cm_len > data.len - pos) {
        return null;
    }
    const color_mode_data = data[pos..(pos + cm_len)];
    pos += cm_len;

    // Section 3: Image Resources (4B length + payload)
    if (data.len - pos < 4) {
        return null;
    }
    const ir_len: usize = readU32BE(data, pos) as usize;
    pos += 4;
    if (ir_len > data.len - pos) {
        return null;
    }
    const image_resources = data[pos..(pos + ir_len)];
    pos += ir_len;

    // Section 4: Layer and Mask Information (4B for PSD v1, 8B for PSB v2)
    let lm_len: usize = 0;
    if (hdr.isPsb()) {
        if (data.len - pos < 8) {
            return null;
        }
        const lm64 = readU64BE(data, pos);
        pos += 8;
        if (lm64 > ((data.len - pos) as u64)) {
            return null;
        }
        lm_len = lm64 as usize;
    } else {
        if (data.len - pos < 4) {
            return null;
        }
        const lm32: usize = readU32BE(data, pos) as usize;
        pos += 4;
        if (lm32 > data.len - pos) {
            return null;
        }
        lm_len = lm32;
    }
    const layer_and_mask = data[pos..(pos + lm_len)];
    pos += lm_len;

    // Section 5: Image Data (2B compression + remaining bytes)
    if (data.len - pos < 2) {
        return null;
    }
    const raw_comp = readU16BE(data, pos);
    pos += 2;
    const image_data = data[pos..data.len];

    return PsdSections{
        .header = hdr,
        .color_mode_data = color_mode_data,
        .image_resources = image_resources,
        .layer_and_mask = layer_and_mask,
        .image_data_compression = psdCompressionFromInt(raw_comp),
        .image_data = image_data,
    };
}

pub struct PsdResourceIter {
    data: []const u8,
    pos: usize,
    done: bool,

    fn init(image_resources: []const u8): PsdResourceIter {
        return PsdResourceIter{
            .data = image_resources,
            .pos = 0,
            .done = false,
        };
    }

    fn next(*_): PsdResource? {
        if (_.done || _.pos >= _.data.len || _.data.len - _.pos < 12) {
            _.done = true;
            return null;
        }

        // 4-byte signature must be "8BIM"
        if (_.data[_.pos] != '8' || _.data[_.pos + 1] != 'B' || _.data[_.pos + 2] != 'I' || _.data[_.pos + 3] != 'M') {
            _.done = true;
            return null;
        }

        const id = readU16BE(_.data, _.pos + 4);
        let cur = _.pos + 6;

        // Pascal string name padded to even total length (1 + name_len)
        if (cur >= _.data.len) {
            _.done = true;
            return null;
        }
        const name_len: usize = _.data[cur] as usize;
        let name_field_len: usize = name_len + 1;
        if ((name_field_len & 1) != 0) {
            name_field_len += 1;
        }
        if (name_field_len > _.data.len - cur) {
            _.done = true;
            return null;
        }
        const name = _.data[(cur + 1)..(cur + 1 + name_len)];
        cur += name_field_len;

        // 4-byte payload length + payload padded to even length
        if (_.data.len - cur < 4) {
            _.done = true;
            return null;
        }
        const payload_len: usize = readU32BE(_.data, cur) as usize;
        cur += 4;
        if (payload_len > _.data.len - cur) {
            _.done = true;
            return null;
        }
        const payload = _.data[cur..(cur + payload_len)];
        cur += payload_len;
        if ((payload_len & 1) != 0 && cur < _.data.len) {
            cur += 1;
        }

        _.pos = cur;
        return PsdResource{
            .id = id,
            .name = name,
            .payload = payload,
        };
    }
}

pub fn findPsdResource(sections: PsdSections, id: u16): PsdResource? {
    let it = PsdResourceIter.init(sections.image_resources);
    while (res := it.next()) {
        if (res.id == id) {
            return res;
        }
    }
    return null;
}
