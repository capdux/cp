// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_png.cp | Synced: 2026-11-21T12:59:27Z

pub err Error {
    UnknownFormat,
    TruncatedHeader,
    InvalidHeader,
    UnsupportedVariant,
}

pub enum ImageFormat: u8 {
    png = 0,
    jpeg = 1,
    webp = 2,
    bmp = 3,
    gif = 4,

    fn mimeType(): string {
        return switch (_) {
            .png => "image/png",
            .jpeg => "image/jpeg",
            .webp => "image/webp",
            .bmp => "image/bmp",
            .gif => "image/gif",
        };
    }

    fn extension(): string {
        return switch (_) {
            .png => "png",
            .jpeg => "jpg",
            .webp => "webp",
            .bmp => "bmp",
            .gif => "gif",
        };
    }
}

pub enum WebpKind: u8 {
    none = 0,
    vp8 = 1,
    vp8l = 2,
    vp8x = 3,
}

/// 零解码图片头探测结果
pub struct ImageInfo {
    format: ImageFormat,
    width: u32,
    height: u32,
    bit_depth: u8 = 8,
    color_type: u8 = 0,
    has_alpha: bool = false,
    is_animated: bool = false,
    is_progressive: bool = false,
    top_down: bool = true,
    webp_kind: WebpKind = .none,

    fn pixelCount(): u64 {
        return (_.width as u64) * (_.height as u64);
    }

    fn mimeType(): string {
        return _.format.mimeType();
    }
}

fn readU16BE(b: []const u8, off: usize): u16 {
    return ((b[off] as u16) << 8) | (b[off + 1] as u16);
}

fn readU32BE(b: []const u8, off: usize): u32 {
    return ((b[off] as u32) << 24) |
        ((b[off + 1] as u32) << 16) |
        ((b[off + 2] as u32) << 8) |
        (b[off + 3] as u32);
}

fn readU16LE(b: []const u8, off: usize): u16 {
    return (b[off] as u16) | ((b[off + 1] as u16) << 8);
}

fn readU24LE(b: []const u8, off: usize): u32 {
    return (b[off] as u32) |
        ((b[off + 1] as u32) << 8) |
        ((b[off + 2] as u32) << 16);
}

fn readU32LE(b: []const u8, off: usize): u32 {
    return (b[off] as u32) |
        ((b[off + 1] as u32) << 8) |
        ((b[off + 2] as u32) << 16) |
        ((b[off + 3] as u32) << 24);
}

fn readI32LE(b: []const u8, off: usize): i32 {
    const u = readU32LE(b, off);
    return @as(i32, @bitCast(u));
}

fn hasPngSignature(b: []const u8): bool {
    return b.len >= 8 &&
        b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G' &&
        b[4] == 0x0D && b[5] == 0x0A && b[6] == 0x1A && b[7] == 0x0A;
}

/// 仅凭魔数快速检测图片格式（零分配）
pub fn detectFormat(bytes: []const u8): ImageFormat? {
    if (hasPngSignature(bytes)) {
        return .png;
    }
    if (bytes.len >= 2 && bytes[0] == 0xFF && bytes[1] == 0xD8) {
        return .jpeg;
    }
    if (bytes.len >= 12 &&
        bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
        bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P')
    {
        return .webp;
    }
    if (bytes.len >= 2 && bytes[0] == 'B' && bytes[1] == 'M') {
        return .bmp;
    }
    if (bytes.len >= 6 &&
        bytes[0] == 'G' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == '8' &&
        (bytes[4] == '7' || bytes[4] == '9') && bytes[5] == 'a')
    {
        return .gif;
    }
    return null;
}

/// 探测 PNG IHDR 头部信息
pub fn probePng(bytes: []const u8): ImageInfo! {
    if (!hasPngSignature(bytes)) {
        return err.InvalidHeader;
    }
    if (bytes.len < 29) {
        return err.TruncatedHeader;
    }
    const ihdr_len = readU32BE(bytes, 8);
    if (ihdr_len < 13) {
        return err.InvalidHeader;
    }
    if (bytes[12] != 'I' || bytes[13] != 'H' || bytes[14] != 'D' || bytes[15] != 'R') {
        return err.InvalidHeader;
    }

    const w = readU32BE(bytes, 16);
    const h = readU32BE(bytes, 20);
    if (w == 0 || h == 0) {
        return err.InvalidHeader;
    }

    const bit_depth = bytes[24];
    const color_type = bytes[25];
    const interlace = bytes[28];

    if (color_type != 0 && color_type != 2 && color_type != 3 && color_type != 4 && color_type != 6) {
        return err.InvalidHeader;
    }

    return ImageInfo{
        format: .png,
        width: w,
        height: h,
        bit_depth: bit_depth,
        color_type: color_type,
        has_alpha: color_type == 4 || color_type == 6,
        is_progressive: interlace != 0,
    };
}

fn isJpegSofMarker(m: u8): bool {
    return m == 0xC0 || m == 0xC1 || m == 0xC2 || m == 0xC3 ||
        m == 0xC5 || m == 0xC6 || m == 0xC7 ||
        m == 0xC9 || m == 0xCA || m == 0xCB ||
        m == 0xCD || m == 0xCE || m == 0xCF;
}

/// 探测 JPEG SOF0 / SOF2 帧头部宽高与渐进式标志
pub fn probeJpeg(bytes: []const u8): ImageInfo! {
    if (bytes.len < 2 || bytes[0] != 0xFF || bytes[1] != 0xD8) {
        return err.InvalidHeader;
    }

    let pos: usize = 2;
    while (pos < bytes.len) {
        // 寻找 0xFF 标记前导字节
        while (pos < bytes.len && bytes[pos] != 0xFF) {
            pos += 1;
        }
        while (pos < bytes.len && bytes[pos] == 0xFF) {
            pos += 1;
        }
        if (pos >= bytes.len) {
            return err.TruncatedHeader;
        }

        const marker = bytes[pos];
        pos += 1;

        // 独立无长度标记
        if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) {
            continue;
        }
        // EOI 或 SOS 到达前尚未找到 SOF
        if (marker == 0xD9 || marker == 0xDA) {
            return err.InvalidHeader;
        }

        if (pos + 2 > bytes.len) {
            return err.TruncatedHeader;
        }
        const seg_len = readU16BE(bytes, pos) as usize;
        if (seg_len < 2) {
            return err.InvalidHeader;
        }
        if (pos + seg_len > bytes.len) {
            return err.TruncatedHeader;
        }

        if (isJpegSofMarker(marker)) {
            if (seg_len < 7) {
                return err.InvalidHeader;
            }
            const precision = bytes[pos + 2];
            const h = readU16BE(bytes, pos + 3) as u32;
            const w = readU16BE(bytes, pos + 5) as u32;
            if (w == 0 || h == 0) {
                return err.InvalidHeader;
            }
            const progressive = marker == 0xC2 || marker == 0xC6 || marker == 0xCA || marker == 0xCE;
            return ImageInfo{
                format: .jpeg,
                width: w,
                height: h,
                bit_depth: precision,
                has_alpha: false,
                is_progressive: progressive,
            };
        }

        pos += seg_len;
    }

    return err.TruncatedHeader;
}

/// 探测 WebP (VP8 / VP8L / VP8X) 宽高、Alpha 与动画标志
pub fn probeWebp(bytes: []const u8): ImageInfo! {
    if (bytes.len < 16) {
        return err.TruncatedHeader;
    }
    if (bytes[0] != 'R' || bytes[1] != 'I' || bytes[2] != 'F' || bytes[3] != 'F' ||
        bytes[8] != 'W' || bytes[9] != 'E' || bytes[10] != 'B' || bytes[11] != 'P')
    {
        return err.InvalidHeader;
    }

    const c0 = bytes[12];
    const c1 = bytes[13];
    const c2 = bytes[14];
    const c3 = bytes[15];

    // VP8 (有损)
    if (c0 == 'V' && c1 == 'P' && c2 == '8' && c3 == ' ') {
        if (bytes.len < 30) {
            return err.TruncatedHeader;
        }
        if (bytes[23] != 0x9D || bytes[24] != 0x01 || bytes[25] != 0x2A) {
            return err.InvalidHeader;
        }
        const w = (readU16LE(bytes, 26) & 0x3FFF) as u32;
        const h = (readU16LE(bytes, 28) & 0x3FFF) as u32;
        if (w == 0 || h == 0) {
            return err.InvalidHeader;
        }
        return ImageInfo{
            format: .webp,
            width: w,
            height: h,
            bit_depth: 8,
            has_alpha: false,
            is_animated: false,
            webp_kind: .vp8,
        };
    }

    // VP8L (无损)
    if (c0 == 'V' && c1 == 'P' && c2 == '8' && c3 == 'L') {
        if (bytes.len < 25) {
            return err.TruncatedHeader;
        }
        if (bytes[20] != 0x2F) {
            return err.InvalidHeader;
        }
        const bits = readU32LE(bytes, 21);
        const w = (bits & 0x3FFF) + 1;
        const h = ((bits >> 14) & 0x3FFF) + 1;
        const has_alpha = ((bits >> 28) & 1) != 0;
        return ImageInfo{
            format: .webp,
            width: w,
            height: h,
            bit_depth: 8,
            has_alpha: has_alpha,
            is_animated: false,
            webp_kind: .vp8l,
        };
    }

    // VP8X (扩展容器，含 Alpha 与动画标志)
    if (c0 == 'V' && c1 == 'P' && c2 == '8' && c3 == 'X') {
        if (bytes.len < 30) {
            return err.TruncatedHeader;
        }
        const flags = bytes[20];
        const is_animated = (flags & 0x02) != 0;
        const has_alpha = (flags & 0x10) != 0;
        const w = readU24LE(bytes, 24) + 1;
        const h = readU24LE(bytes, 27) + 1;
        return ImageInfo{
            format: .webp,
            width: w,
            height: h,
            bit_depth: 8,
            has_alpha: has_alpha,
            is_animated: is_animated,
            webp_kind: .vp8x,
        };
    }

    return err.UnsupportedVariant;
}

/// 探测 BMP BITMAPINFOHEADER 正负高度、bpp 与方向
pub fn probeBmp(bytes: []const u8): ImageInfo! {
    if (bytes.len < 2 || bytes[0] != 'B' || bytes[1] != 'M') {
        return err.InvalidHeader;
    }
    if (bytes.len < 54) {
        return err.TruncatedHeader;
    }
    const ih_size = readU32LE(bytes, 14);
    if (ih_size < 40 || 14 + (ih_size as usize) > bytes.len) {
        return err.InvalidHeader;
    }

    const w_raw = readI32LE(bytes, 18);
    const h_raw = readI32LE(bytes, 22);
    if (w_raw <= 0 || h_raw == 0 || h_raw == std.math.minInt(i32)) {
        return err.InvalidHeader;
    }

    const bpp = readU16LE(bytes, 28);
    if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 && bpp != 32) {
        return err.UnsupportedVariant;
    }

    const compression = readU32LE(bytes, 30);
    let has_alpha = false;
    if (bpp == 32 && compression == 3 && ih_size >= 108 && bytes.len >= 70) {
        const a_mask = readU32LE(bytes, 66);
        has_alpha = a_mask != 0;
    }

    const abs_h: u32 = if (h_raw < 0) ((-h_raw) as u32) else (h_raw as u32);
    return ImageInfo{
        format: .bmp,
        width: w_raw as u32,
        height: abs_h,
        bit_depth: bpp as u8,
        has_alpha: has_alpha,
        top_down: h_raw < 0,
    };
}

/// 探测 GIF87a / GIF89a 逻辑屏幕宽高与色深
pub fn probeGif(bytes: []const u8): ImageInfo! {
    if (bytes.len < 13) {
        return err.TruncatedHeader;
    }
    if (bytes[0] != 'G' || bytes[1] != 'I' || bytes[2] != 'F' || bytes[3] != '8' ||
        (bytes[4] != '7' && bytes[4] != '9') || bytes[5] != 'a')
    {
        return err.InvalidHeader;
    }
    const w = readU16LE(bytes, 6) as u32;
    const h = readU16LE(bytes, 8) as u32;
    if (w == 0 || h == 0) {
        return err.InvalidHeader;
    }
    const packed_fields = bytes[10];
    const color_res = ((packed_fields >> 4) & 0x07) + 1;
    return ImageInfo{
        format: .gif,
        width: w,
        height: h,
        bit_depth: color_res,
        has_alpha: bytes[4] == '9',
    };
}

/// 自动识别格式并零解码探测图片头部元数据
pub fn probe(bytes: []const u8): ImageInfo! {
    if (fmt := detectFormat(bytes)) {
        return switch (fmt) {
            .png => probePng(bytes),
            .jpeg => probeJpeg(bytes),
            .webp => probeWebp(bytes),
            .bmp => probeBmp(bytes),
            .gif => probeGif(bytes),
        };
    }
    return err.UnknownFormat;
}
