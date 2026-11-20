// Source: /data/dev/bun/src/runtime/image/codecs.cp
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codecs.cp
// Upstream Sync: Refer to bun/src/runtime/image/codecs.zig for future upstream updates.
// ============================================================================
// ============================================================================
// lib/image/codecs.cp — 图像格式探测器、尺寸头探测、MIME 映射与 EXIF 方向解析
// 吸收自 Bun src/runtime/image/codecs.cp 与 exif.cp
// ============================================================================

/// 图像格式枚举
pub enum ImageFormat: u8 {
    Png = 0,
    Jpeg = 1,
    WebP = 2,
    Bmp = 3,
    Gif = 4,
    Tiff = 5,
    Avif = 6,
    Heic = 7,
    Ico = 8,
    Svg = 9,

    /// 自动识别图像二进制格式 (支持 PNG/JPEG/GIF/WebP/BMP/ICO/SVG/TIFF/AVIF/HEIC)
    pub static fn detect(bytes: []const u8): ImageFormat? {
        return Codecs.sniff(bytes);
    }
}

/// EXIF 方向标号（RFC 0x0112 规范）
pub enum ExifOrientation: u8 {
    Normal = 1,
    Flop = 2,          // 水平镜像翻转
    Rotate180 = 3,     // 顺时针旋转 180 度
    Flip = 4,          // 垂直翻转
    FlopRotate90 = 5,  // 镜像并旋转 90 度
    Rotate90 = 6,      // 顺时针旋转 90 度
    FlopRotate270 = 7, // 镜像并旋转 270 度
    Rotate270 = 8,     // 顺时针旋转 270 度
}

pub struct ExifTransform {
    pub flop: bool = false,
    pub flip: bool = false,
    pub rotate: u16 = 0,
}

pub struct ImageDimensions {
    pub width: u32 = 0,
    pub height: u32 = 0,
}

fn toLowerAsciiByte(c: u8): u8 {
    return if (c >= 'A' && c <= 'Z') (c + 32) else c;
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn startsWithIgnoreCase(haystack: []const u8, offset: usize, lit: string): bool {
    if (offset + lit.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        if (toLowerAsciiByte(haystack[offset + i]) != toLowerAsciiByte(lit[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findIgnoreCase(haystack: []const u8, lit: string, max_scan: usize): usize? {
    if (lit.len == 0 || haystack.len < lit.len) {
        return null;
    }
    const scan_limit = if (haystack.len < max_scan) haystack.len else max_scan;
    if (scan_limit < lit.len) {
        return null;
    }
    let i: usize = 0;
    while (i + lit.len <= scan_limit) {
        if (startsWithIgnoreCase(haystack, i, lit)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn isSvgBytes(bytes: []const u8): bool {
    let i: usize = 0;
    // Strip optional UTF-8 BOM
    if (bytes.len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        i = 3;
    }
    while (i < bytes.len && isWs(bytes[i])) {
        i += 1;
    }
    if (i >= bytes.len || bytes[i] != '<') {
        return false;
    }
    if (startsWithIgnoreCase(bytes, i, "<svg")) {
        if (i + 4 < bytes.len) {
            const after = bytes[i + 4];
            return isWs(after) || after == '>' || after == '/';
        }
        return false;
    }
    if (startsWithIgnoreCase(bytes, i, "<?xml") || startsWithIgnoreCase(bytes, i, "<!doctype svg")) {
        return findIgnoreCase(bytes[i..bytes.len], "<svg", 1024) != null;
    }
    return false;
}

fn parseUintPrefix(s: []const u8): u32? {
    let i: usize = 0;
    while (i < s.len && isWs(s[i])) {
        i += 1;
    }
    if (i >= s.len || s[i] < '0' || s[i] > '9') {
        return null;
    }
    let val: u32 = 0;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        val = val * 10 + ((s[i] - '0') as u32);
        i += 1;
    }
    return val;
}

fn parseSvgAttrUint(tag: []const u8, attr_name: string): u32? {
    let i: usize = 0;
    while (i + attr_name.len + 2 < tag.len) {
        if (startsWithIgnoreCase(tag, i, attr_name)) {
            const prev_ok = (i == 0) || isWs(tag[i - 1]);
            if (prev_ok) {
                let p = i + attr_name.len;
                while (p < tag.len && isWs(tag[p])) { p += 1; }
                if (p < tag.len && tag[p] == '=') {
                    p += 1;
                    while (p < tag.len && isWs(tag[p])) { p += 1; }
                    if (p < tag.len && (tag[p] == '"' || tag[p] == '\'')) {
                        const quote = tag[p];
                        p += 1;
                        let end = p;
                        while (end < tag.len && tag[end] != quote) { end += 1; }
                        return parseUintPrefix(tag[p..end]);
                    }
                }
            }
        }
        i += 1;
    }
    return null;
}

fn parseSvgViewBox(tag: []const u8): ImageDimensions? {
    if (vb_pos := findIgnoreCase(tag, "viewbox", tag.len)) {
        let p = vb_pos + 7;
        while (p < tag.len && isWs(tag[p])) { p += 1; }
        if (p >= tag.len || tag[p] != '=') { return null; }
        p += 1;
        while (p < tag.len && isWs(tag[p])) { p += 1; }
        if (p >= tag.len || (tag[p] != '"' && tag[p] != '\'')) { return null; }
        const quote = tag[p];
        p += 1;
        let end = p;
        while (end < tag.len && tag[end] != quote) { end += 1; }
        const val_slice = tag[p..end];

        // Parse 4 numbers: minx miny width height
        let nums: [4]u32 = [0; 4];
        let count: usize = 0;
        let k: usize = 0;
        while (k < val_slice.len && count < 4) {
            while (k < val_slice.len && (isWs(val_slice[k]) || val_slice[k] == ',')) {
                k += 1;
            }
            if (k >= val_slice.len) { break; }
            let neg = false;
            if (val_slice[k] == '-') {
                neg = true;
                k += 1;
            }
            let num: u32 = 0;
            let has_digit = false;
            while (k < val_slice.len && val_slice[k] >= '0' && val_slice[k] <= '9') {
                num = num * 10 + ((val_slice[k] - '0') as u32);
                has_digit = true;
                k += 1;
            }
            // Skip optional fractional part
            if (k < val_slice.len && val_slice[k] == '.') {
                k += 1;
                while (k < val_slice.len && val_slice[k] >= '0' && val_slice[k] <= '9') {
                    k += 1;
                }
            }
            if (has_digit) {
                nums[count] = if (neg) 0 else num;
                count += 1;
            } else {
                k += 1;
            }
        }
        if (count == 4 && nums[2] > 0 && nums[3] > 0) {
            return ImageDimensions{ width: nums[2], height: nums[3] };
        }
    }
    return null;
}

fn readU32BeLocal(b: []const u8, off: usize): u32 {
    if (off + 4 > b.len) return 0;
    return ((b[off] as u32) << 24) | ((b[off + 1] as u32) << 16) | ((b[off + 2] as u32) << 8) | (b[off + 3] as u32);
}

fn readI32LeLocal(b: []const u8, off: usize): i32 {
    if (off + 4 > b.len) return 0;
    const u = (b[off] as u32) | ((b[off + 1] as u32) << 8) | ((b[off + 2] as u32) << 16) | ((b[off + 3] as u32) << 24);
    return u as i32;
}

fn isJpegSofMarker(m: u8): bool {
    return (m >= 0xC0 && m <= 0xC3) ||
        (m >= 0xC5 && m <= 0xC7) ||
        (m >= 0xC9 && m <= 0xCB) ||
        (m >= 0xCD && m <= 0xCF);
}

pub class Codecs {
    /// 根据二进制魔数（Magic Bytes）极速探测图像格式
    pub static fn sniff(bytes: []const u8): ImageFormat? {
        if (bytes.len >= 8) {
            // PNG 签名: 89 50 4E 47 0D 0A 1A 0A
            if (bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47 &&
                bytes[4] == 0x0D && bytes[5] == 0x0A && bytes[6] == 0x1A && bytes[7] == 0x0A) {
                return ImageFormat.Png;
            }
        }
        if (bytes.len >= 3) {
            // JPEG 签名: FF D8 FF
            if (bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
                return ImageFormat.Jpeg;
            }
        }
        if (bytes.len >= 12) {
            // WebP 签名: RIFF....WEBP
            if (bytes[0] == 0x52 && bytes[1] == 0x49 && bytes[2] == 0x46 && bytes[3] == 0x46 &&
                bytes[8] == 0x57 && bytes[9] == 0x45 && bytes[10] == 0x42 && bytes[11] == 0x50) {
                return ImageFormat.WebP;
            }
        }
        if (bytes.len >= 2) {
            // BMP 签名: BM
            if (bytes[0] == 0x42 && bytes[1] == 0x4D) {
                return ImageFormat.Bmp;
            }
        }
        if (bytes.len >= 6) {
            // GIF 签名: GIF87a 或 GIF89a
            if (bytes[0] == 0x47 && bytes[1] == 0x49 && bytes[2] == 0x46 && bytes[3] == 0x38 &&
                (bytes[4] == 0x37 || bytes[4] == 0x39) && bytes[5] == 0x61) {
                return ImageFormat.Gif;
            }
        }
        if (bytes.len >= 22) {
            // ICO 签名: 00 00 01 00 + count >= 1
            if (bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0x01 && bytes[3] == 0x00) {
                const count = (bytes[4] as u16) | ((bytes[5] as u16) << 8);
                if (count >= 1) {
                    return ImageFormat.Ico;
                }
            }
        }
        if (bytes.len >= 4) {
            // TIFF 签名: II*\0 (Little Endian) 或 MM\0* (Big Endian)
            if ((bytes[0] == 0x49 && bytes[1] == 0x49 && bytes[2] == 0x2A && bytes[3] == 0x00) ||
                (bytes[0] == 0x4D && bytes[1] == 0x4D && bytes[2] == 0x00 && bytes[3] == 0x2A)) {
                return ImageFormat.Tiff;
            }
        }
        if (bytes.len >= 16) {
            // ISO BMFF 容器: ....ftyp
            if (bytes[4] == 0x66 && bytes[5] == 0x74 && bytes[6] == 0x79 && bytes[7] == 0x70) {
                if (bytes[8] == 0x61 && bytes[9] == 0x76 && bytes[10] == 0x69 && bytes[11] == 0x66) {
                    return ImageFormat.Avif;
                }
                if (bytes[8] == 0x68 && bytes[9] == 0x65 && bytes[10] == 0x69 && bytes[11] == 0x63) {
                    return ImageFormat.Heic;
                }
            }
        }
        if (isSvgBytes(bytes)) {
            return ImageFormat.Svg;
        }
        return null;
    }

    /// 零全量解码、仅解析图像头部即可极速读出宽高 (支持 PNG/JPEG/GIF/WebP/BMP/ICO/SVG)
    pub static fn probeDimensions(bytes: []const u8): ImageDimensions? {
        const fmt = Codecs.sniff(bytes) ?? return null;
        if (fmt == ImageFormat.Png) {
            if (bytes.len < 24) return null;
            if (bytes[12] != 'I' || bytes[13] != 'H' || bytes[14] != 'D' || bytes[15] != 'R') return null;
            const w = readU32BeLocal(bytes, 16);
            const h = readU32BeLocal(bytes, 20);
            if (w == 0 || h == 0) return null;
            return ImageDimensions{ width: w, height: h };
        }
        if (fmt == ImageFormat.Bmp) {
            if (bytes.len < 26) return null;
            const ih_size = readI32LeLocal(bytes, 14);
            if (ih_size < 40) return null;
            const w_raw = readI32LeLocal(bytes, 18);
            const h_raw = readI32LeLocal(bytes, 22);
            if (w_raw <= 0 || h_raw == 0) return null;
            const abs_h: u32 = if (h_raw < 0) (-h_raw) as u32 else h_raw as u32;
            return ImageDimensions{ width: w_raw as u32, height: abs_h };
        }
        if (fmt == ImageFormat.Gif) {
            if (bytes.len < 13) return null;
            const w = (bytes[6] as u32) | ((bytes[7] as u32) << 8);
            const h = (bytes[8] as u32) | ((bytes[9] as u32) << 8);
            if (w == 0 || h == 0) return null;
            return ImageDimensions{ width: w, height: h };
        }
        if (fmt == ImageFormat.Ico) {
            if (bytes.len < 22) return null;
            const w: u32 = if (bytes[6] == 0) 256 else (bytes[6] as u32);
            const h: u32 = if (bytes[7] == 0) 256 else (bytes[7] as u32);
            return ImageDimensions{ width: w, height: h };
        }
        if (fmt == ImageFormat.WebP) {
            if (bytes.len < 25) return null;
            const chunk = bytes[12..16];
            if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == ' ') {
                if (bytes.len < 30) return null;
                if (bytes[23] != 0x9D || bytes[24] != 0x01 || bytes[25] != 0x2A) return null;
                const w = ((bytes[26] as u32) | ((bytes[27] as u32) << 8)) & 0x3FFF;
                const h = ((bytes[28] as u32) | ((bytes[29] as u32) << 8)) & 0x3FFF;
                return ImageDimensions{ width: w, height: h };
            } else if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'L') {
                if (bytes.len < 25 || bytes[20] != 0x2F) return null;
                const b0 = bytes[21] as u32;
                const b1 = bytes[22] as u32;
                const b2 = bytes[23] as u32;
                const b3 = bytes[24] as u32;
                const w = (b0 | ((b1 & 0x3F) << 8)) + 1;
                const h = ((b1 >> 6) | (b2 << 2) | ((b3 & 0x0F) << 10)) + 1;
                return ImageDimensions{ width: w, height: h };
            } else if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'X') {
                if (bytes.len < 30) return null;
                const w = ((bytes[24] as u32) | ((bytes[25] as u32) << 8) | ((bytes[26] as u32) << 16)) + 1;
                const h = ((bytes[27] as u32) | ((bytes[28] as u32) << 8) | ((bytes[29] as u32) << 16)) + 1;
                return ImageDimensions{ width: w, height: h };
            }
            return null;
        }
        if (fmt == ImageFormat.Jpeg) {
            let offset: usize = 2;
            while (offset + 4 <= bytes.len) {
                if (bytes[offset] != 0xFF) {
                    offset += 1;
                    continue;
                }
                const marker = bytes[offset + 1];
                if (marker == 0xFF) {
                    offset += 1;
                    continue;
                }
                if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) {
                    offset += 2;
                    continue;
                }
                if (marker == 0xDA || marker == 0xD9) {
                    break;
                }
                const seglen = ((bytes[offset + 2] as usize) << 8) | (bytes[offset + 3] as usize);
                if (seglen < 2 || offset + 2 + seglen > bytes.len) {
                    break;
                }
                if (isJpegSofMarker(marker)) {
                    if (seglen < 8) return null;
                    const h = ((bytes[offset + 5] as u32) << 8) | (bytes[offset + 6] as u32);
                    const w = ((bytes[offset + 7] as u32) << 8) | (bytes[offset + 8] as u32);
                    return ImageDimensions{ width: w, height: h };
                }
                offset += 2 + seglen;
            }
            return null;
        }
        if (fmt == ImageFormat.Svg) {
            const svg_idx = findIgnoreCase(bytes, "<svg", 2048) ?? return null;
            let end_idx = svg_idx + 4;
            while (end_idx < bytes.len && bytes[end_idx] != '>') {
                end_idx += 1;
            }
            const tag = bytes[svg_idx..end_idx];
            const w_opt = parseSvgAttrUint(tag, "width");
            const h_opt = parseSvgAttrUint(tag, "height");
            if (w_opt != null && h_opt != null) {
                return ImageDimensions{ width: w_opt.?, height: h_opt.? };
            }
            return parseSvgViewBox(tag);
        }
        return null;
    }

    /// 获取标准 MIME 媒体类型
    pub static fn mime(fmt: ImageFormat): string {
        return switch (fmt) {
            .Png => "image/png",
            .Jpeg => "image/jpeg",
            .WebP => "image/webp",
            .Bmp => "image/bmp",
            .Gif => "image/gif",
            .Tiff => "image/tiff",
            .Avif => "image/avif",
            .Heic => "image/heic",
            .Ico => "image/x-icon",
            .Svg => "image/svg+xml",
        };
    }

    static fn hasSuffix(s: string, suffix: string): bool {
        if (s.len < suffix.len) return false;
        let start = s.len - suffix.len;
        let i: usize = 0;
        while (i < suffix.len) {
            if (toLowerAsciiByte(s[start + i]) != toLowerAsciiByte(suffix[i])) return false;
            i += 1;
        }
        return true;
    }

    /// 根据文件路径后缀名推导图像格式
    pub static fn fromExtension(path: string): ImageFormat? {
        if (Codecs.hasSuffix(path, ".png")) return ImageFormat.Png;
        if (Codecs.hasSuffix(path, ".jpg") || Codecs.hasSuffix(path, ".jpeg")) return ImageFormat.Jpeg;
        if (Codecs.hasSuffix(path, ".webp")) return ImageFormat.WebP;
        if (Codecs.hasSuffix(path, ".bmp")) return ImageFormat.Bmp;
        if (Codecs.hasSuffix(path, ".gif")) return ImageFormat.Gif;
        if (Codecs.hasSuffix(path, ".tif") || Codecs.hasSuffix(path, ".tiff")) return ImageFormat.Tiff;
        if (Codecs.hasSuffix(path, ".avif")) return ImageFormat.Avif;
        if (Codecs.hasSuffix(path, ".heic") || Codecs.hasSuffix(path, ".heif")) return ImageFormat.Heic;
        if (Codecs.hasSuffix(path, ".ico")) return ImageFormat.Ico;
        if (Codecs.hasSuffix(path, ".svg")) return ImageFormat.Svg;
        return null;
    }

    /// 解析 EXIF Orientation 转换为几何操作
    pub static fn getOrientationTransform(ori: ExifOrientation): ExifTransform {
        return switch (ori) {
            .Normal => ExifTransform{ flop: false, flip: false, rotate: 0 },
            .Flop => ExifTransform{ flop: true, flip: false, rotate: 0 },
            .Rotate180 => ExifTransform{ flop: false, flip: false, rotate: 180 },
            .Flip => ExifTransform{ flop: false, flip: true, rotate: 0 },
            .FlopRotate90 => ExifTransform{ flop: true, flip: false, rotate: 90 },
            .Rotate90 => ExifTransform{ flop: false, flip: false, rotate: 90 },
            .FlopRotate270 => ExifTransform{ flop: true, flip: false, rotate: 270 },
            .Rotate270 => ExifTransform{ flop: false, flip: false, rotate: 270 },
        };
    }
}

/// 顶层便捷函数：无需全量像素解码即可读出图像宽高
pub fn probeDimensions(bytes: []const u8): ImageDimensions? {
    return Codecs.probeDimensions(bytes);
}
