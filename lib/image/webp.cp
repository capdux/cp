// Source: /data/dev/bun/src/runtime/image/codec_webp.cp
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_webp.cp
// Upstream Sync: Refer to bun/src/runtime/image/codec_webp.zig for future upstream updates.
// ============================================================================
import { Codecs } from "./codecs";

pub struct WebpHeader {
    pub width: usize,
    pub height: usize,
    pub has_alpha: bool,
    pub is_lossless: bool,
    pub is_animation: bool,
}

pub class Webp {
    /// 快速探测 WebP RIFF 容器、VP8/VP8L/VP8X 标志与尺寸
    pub static fn probe(bytes: []const u8): WebpHeader? {
        if (bytes.len < 16) {
            return null;
        }
        // RIFF .... WEBP
        let is_riff = bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F';
        let is_webp = bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P';
        if (!is_riff || !is_webp) {
            return null;
        }

        let chunk = bytes[12..16];
        if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == ' ') {
            // VP8 Simple Lossy (check VP8 keyframe sync code 0x9D 0x01 0x2A at offset 23..25)
            if (bytes.len < 30 || bytes[23] != 0x9D || bytes[24] != 0x01 || bytes[25] != 0x2A) {
                return null;
            }
            let w = ((bytes[26] as usize) | ((bytes[27] as usize) << 8)) & 0x3FFF;
            let h = ((bytes[28] as usize) | ((bytes[29] as usize) << 8)) & 0x3FFF;
            return WebpHeader{
                width: w,
                height: h,
                has_alpha: false,
                is_lossless: false,
                is_animation: false,
            };
        } else if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'L') {
            // VP8L Lossless
            if (bytes.len < 25 || bytes[20] != 0x2F) {
                return null;
            }
            let b0 = (bytes[21] as usize);
            let b1 = (bytes[22] as usize);
            let b2 = (bytes[23] as usize);
            let b3 = (bytes[24] as usize);
            let w = (b0 | ((b1 & 0x3F) << 8)) + 1;
            let h = (((b1 >> 6) | (b2 << 2) | ((b3 & 0x0F) << 10))) + 1;
            return WebpHeader{
                width: w,
                height: h,
                has_alpha: (b3 & 0x10) != 0,
                is_lossless: true,
                is_animation: false,
            };
        } else if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'X') {
            // VP8X Extended
            if (bytes.len < 30) {
                return null;
            }
            let flags = bytes[20];
            let has_anim = (flags & 0x02) != 0;
            let has_alpha = (flags & 0x10) != 0;
            let w = ((bytes[24] as usize) | ((bytes[25] as usize) << 8) | ((bytes[26] as usize) << 16)) + 1;
            let h = ((bytes[27] as usize) | ((bytes[28] as usize) << 8) | ((bytes[29] as usize) << 16)) + 1;
            return WebpHeader{
                width: w,
                height: h,
                has_alpha: has_alpha,
                is_lossless: false,
                is_animation: has_anim,
            };
        }

        return null;
    }
}
