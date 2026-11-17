// Source: Custom implementation by Antigravity AI
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codecs.cp
// Upstream Sync: Refer to bun/src/runtime/image/codecs.zig for future upstream updates.
// ============================================================================
// ============================================================================
// lib/image/codecs.cp — 图像格式探测器、MIME 映射与 EXIF 方向解析
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
        };
    }

    static fn hasSuffix(s: string, suffix: string): bool {
        if (s.len < suffix.len) return false;
        let start = s.len - suffix.len;
        let i: usize = 0;
        while (i < suffix.len) {
            if (s[start + i] != suffix[i]) return false;
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
