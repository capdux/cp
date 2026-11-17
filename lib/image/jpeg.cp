// Source: Custom implementation by Antigravity AI
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
        let offset = 2;

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
            if (marker >= 0xD0 && marker <= 0xD8) {
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

            // SOF0 (0xC0: Baseline), SOF2 (0xC2: Progressive)
            if (marker == 0xC0 || marker == 0xC2) {
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
