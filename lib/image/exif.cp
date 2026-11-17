// Source: Custom implementation by Antigravity AI
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/exif.cp
// Upstream Sync: Refer to bun/src/runtime/image/exif.zig for future upstream updates.
// ============================================================================
import { ExifOrientation, ExifTransform, Codecs } from "./codecs";

pub class Exif {
    /// 解析 JPEG 二进制中的 APP1 EXIF IFD0 0x0112 旋转标签
    pub static fn readJpeg(bytes: []const u8): ExifOrientation {
        if (bytes.len < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) {
            return ExifOrientation.Normal;
        }

        let i: usize = 2;
        while (i + 4 <= bytes.len) {
            if (bytes[i] != 0xFF) {
                return ExifOrientation.Normal;
            }
            let marker = bytes[i + 1];
            if (marker == 0xFF) {
                i = i + 1;
                continue;
            }
            if (marker >= 0xD0 && marker <= 0xD8) {
                i = i + 2;
                continue;
            }
            if (marker == 0xDA || marker == 0xD9) {
                return ExifOrientation.Normal;
            }

            let seglen = ((bytes[i + 2] as usize) << 8) | (bytes[i + 3] as usize);
            if (seglen < 2 || i + 2 + seglen > bytes.len) {
                return ExifOrientation.Normal;
            }

            if (marker == 0xE1 && seglen >= 8) {
                let seg = bytes[(i + 4) .. (i + 2 + seglen)];
                if (seg.len >= 6 && seg[0] == 'E' && seg[1] == 'x' && seg[2] == 'i' && seg[3] == 'f' && seg[4] == 0 && seg[5] == 0) {
                    if (orient := Exif.parseTiff(seg[6..])) {
                        return orient;
                    }
                }
            }
            i = i + 2 + seglen;
        }
        return ExifOrientation.Normal;
    }

    /// 解析 TIFF 标头与 IFD0 目录
    pub static fn parseTiff(tiff: []const u8): ExifOrientation? {
        if (tiff.len < 8) {
            return null;
        }
        let is_big = tiff[0] == 'M' && tiff[1] == 'M';
        let is_little = tiff[0] == 'I' && tiff[1] == 'I';
        if (!is_big && !is_little) {
            return null;
        }

        let magic = Exif.readU16(tiff, 2, is_big);
        if (magic != 42) {
            return null;
        }

        let ifd0_offset = Exif.readU32(tiff, 4, is_big);
        if (ifd0_offset + 2 > tiff.len) {
            return null;
        }

        let num_entries = Exif.readU16(tiff, ifd0_offset, is_big);
        let entries_start = ifd0_offset + 2;

        for (let idx in 0..num_entries) {
            let entry_off = entries_start + (idx as usize) * 12;
            if (entry_off + 12 > tiff.len) {
                break;
            }
            let tag = Exif.readU16(tiff, entry_off, is_big);
            if (tag == 0x0112) {
                // Orientation Tag
                let val = Exif.readU16(tiff, entry_off + 8, is_big);
                if (val >= 1 && val <= 8) {
                    return switch (val) {
                        1 => ExifOrientation.Normal,
                        2 => ExifOrientation.Flop,
                        3 => ExifOrientation.Rotate180,
                        4 => ExifOrientation.Flip,
                        5 => ExifOrientation.FlopRotate90,
                        6 => ExifOrientation.Rotate90,
                        7 => ExifOrientation.FlopRotate270,
                        8 => ExifOrientation.Rotate270,
                        else => ExifOrientation.Normal,
                    };
                }
                return ExifOrientation.Normal;
            }
        }
        return null;
    }

    inline fn readU16(buf: []const u8, off: usize, is_big: bool): u16 {
        if (off + 2 > buf.len) {
            return 0;
        }
        if (is_big) {
            return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
        } else {
            return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
        }
    }

    inline fn readU32(buf: []const u8, off: usize, is_big: bool): usize {
        if (off + 4 > buf.len) {
            return 0;
        }
        if (is_big) {
            return ((buf[off] as usize) << 24) |
                ((buf[off + 1] as usize) << 16) |
                ((buf[off + 2] as usize) << 8) |
                (buf[off + 3] as usize);
        } else {
            return (buf[off] as usize) |
                ((buf[off + 1] as usize) << 8) |
                ((buf[off + 2] as usize) << 16) |
                ((buf[off + 3] as usize) << 24);
        }
    }
}
