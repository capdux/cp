// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_jpeg.cp | Synced: 2026-11-21T23:35:11Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_jpeg.cp
// Upstream Sync: Refer to bun/src/runtime/image/codec_jpeg.zig for future upstream updates.
// ============================================================================
import { Codecs, ExifOrientation } from "./codecs";
import { Exif } from "./exif";

pub struct JpegHeader {
    pub width: usize,
    pub height: usize,
    pub components: usize,
    pub is_progressive: bool,
    pub orientation: ExifOrientation,
}

pub class Jpeg {
    /// 快速探测 JPEG 图像尺寸、采样与 EXIF 方向
    pub static fn probe(bytes: []const u8): JpegHeader? {
        if (bytes.len < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) {
            return null;
        }

        let orientation = Exif.readJpeg(bytes);
        let offset: usize = 2;

        while (offset + 4 <= bytes.len) {
            if (bytes[offset] != 0xFF) {
                offset = offset + 1;
                continue;
            }
            let marker = bytes[offset + 1];
            if (marker == 0xFF) {
                offset = offset + 1;
                continue;
            }
            if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) {
                offset = offset + 2;
                continue;
            }
            if (marker == 0xDA || marker == 0xD9) {
                // SOS / EOI
                break;
            }

            let seglen = ((bytes[offset + 2] as usize) << 8) | (bytes[offset + 3] as usize);
            if (seglen < 2 || offset + 2 + seglen > bytes.len) {
                break;
            }

            // All SOF markers (SOF0..SOF3, SOF5..SOF7, SOF9..SOF11, SOF13..SOF15)
            const is_sof = (marker >= 0xC0 && marker <= 0xC3) ||
                (marker >= 0xC5 && marker <= 0xC7) ||
                (marker >= 0xC9 && marker <= 0xCB) ||
                (marker >= 0xCD && marker <= 0xCF);
            if (is_sof) {
                if (seglen < 8) {
                    return null;
                }
                let h = ((bytes[offset + 5] as usize) << 8) | (bytes[offset + 6] as usize);
                let w = ((bytes[offset + 7] as usize) << 8) | (bytes[offset + 8] as usize);
                let comp = (bytes[offset + 9] as usize);
                let is_prog = (marker == 0xC2);

                return JpegHeader{
                    width: w,
                    height: h,
                    components: comp,
                    is_progressive: is_prog,
                    orientation: orientation,
                };
            }

            offset = offset + 2 + seglen;
        }
        return null;
    }
}
