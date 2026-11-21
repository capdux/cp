// Source: Absorbed from /data/dev/bun/src/runtime/image/thumbhash.cp | Synced: 2026-11-20T11:42:22Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/thumbhash.cp
// Upstream Sync: Refer to bun/src/runtime/image/thumbhash.zig for future upstream updates.
// ============================================================================
import { Png } from "./png";

const B64_ALPHABET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

pub struct DecodedThumbImage {
    pub width: usize,
    pub height: usize,
    pub rgba: []u8,
}

fn encodeBase64Alloc(raw: []const u8): []u8 {
    if (raw.len == 0) {
        return "";
    }
    const out_len = ((raw.len + 2) / 3) * 4;
    let out_ptr: [*]u8 = @alloc(out_len);
    let out = unsafe { out_ptr[0..out_len] };

    let i: usize = 0;
    let o: usize = 0;
    while (i + 3 <= raw.len) {
        const n = ((raw[i] as u32) << 16) | ((raw[i + 1] as u32) << 8) | (raw[i + 2] as u32);
        out[o + 0] = B64_ALPHABET[((n >> 18) & 0x3F) as usize];
        out[o + 1] = B64_ALPHABET[((n >> 12) & 0x3F) as usize];
        out[o + 2] = B64_ALPHABET[((n >> 6) & 0x3F) as usize];
        out[o + 3] = B64_ALPHABET[(n & 0x3F) as usize];
        i += 3;
        o += 4;
    }

    const rem = raw.len - i;
    if (rem == 1) {
        const n = (raw[i] as u32) << 16;
        out[o + 0] = B64_ALPHABET[((n >> 18) & 0x3F) as usize];
        out[o + 1] = B64_ALPHABET[((n >> 12) & 0x3F) as usize];
        out[o + 2] = '=';
        out[o + 3] = '=';
    } else if (rem == 2) {
        const n = ((raw[i] as u32) << 16) | ((raw[i + 1] as u32) << 8);
        out[o + 0] = B64_ALPHABET[((n >> 18) & 0x3F) as usize];
        out[o + 1] = B64_ALPHABET[((n >> 12) & 0x3F) as usize];
        out[o + 2] = B64_ALPHABET[((n >> 6) & 0x3F) as usize];
        out[o + 3] = '=';
    }
    return out;
}

fn clampByte(v: i32): u8 {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v as u8;
}

pub class ThumbHash {
    pub const MAX_LEN: usize = 25;

    /// 将 RGBA8 像素切片编码为 25 字节 ThumbHash（全局平均色 + 2x2 空间色度网格）
    pub static fn encode(rgba: []const u8, w: usize, h: usize): []u8 {
        let npix = w * h;
        if (rgba.len < npix * 4 || w == 0 || h == 0 || w > 100 || h > 100) {
            return "";
        }

        let sum_r: u64 = 0;
        let sum_g: u64 = 0;
        let sum_b: u64 = 0;
        let sum_a: u64 = 0;

        // 2x2 quadrant accumulators for spatial detail
        let q_r: [4]u64 = [0; 4];
        let q_g: [4]u64 = [0; 4];
        let q_b: [4]u64 = [0; 4];
        let q_n: [4]u64 = [0; 4];

        let has_alpha = false;
        let y: usize = 0;
        while (y < h) {
            const qy: usize = if (y * 2 < h) 0 else 1;
            let x: usize = 0;
            while (x < w) {
                const qx: usize = if (x * 2 < w) 0 else 1;
                const qi = qy * 2 + qx;
                const idx = (y * w + x) * 4;
                const r = rgba[idx + 0] as u64;
                const g = rgba[idx + 1] as u64;
                const b = rgba[idx + 2] as u64;
                const a = rgba[idx + 3] as u64;
                if (a < 255) { has_alpha = true; }
                sum_r += r;
                sum_g += g;
                sum_b += b;
                sum_a += a;
                q_r[qi] += r;
                q_g[qi] += g;
                q_b[qi] += b;
                q_n[qi] += 1;
                x += 1;
            }
            y += 1;
        }

        const avg_r = (sum_r / (npix as u64)) as u8;
        const avg_g = (sum_g / (npix as u64)) as u8;
        const avg_b = (sum_b / (npix as u64)) as u8;
        const avg_a = (sum_a / (npix as u64)) as u8;

        let out_ptr: [*]u8 = @alloc(ThumbHash.MAX_LEN);
        let out = unsafe { out_ptr[0..ThumbHash.MAX_LEN] };

        // Bytes 0..5: average RGB, width, height, alpha flag
        out[0] = avg_r;
        out[1] = avg_g;
        out[2] = avg_b;
        out[3] = w as u8;
        out[4] = h as u8;
        out[5] = if (has_alpha) 1 else 0;
        out[6] = avg_a;
        out[7] = 1; // version=1 (2x2 quadrant colors in bytes 8..19)

        // Bytes 8..19: 4 quadrants x (R, G, B)
        let qi: usize = 0;
        while (qi < 4) {
            const cnt = if (q_n[qi] > 0) q_n[qi] else 1;
            out[8 + qi * 3 + 0] = (q_r[qi] / cnt) as u8;
            out[8 + qi * 3 + 1] = (q_g[qi] / cnt) as u8;
            out[8 + qi * 3 + 2] = (q_b[qi] / cnt) as u8;
            qi += 1;
        }

        let tail: usize = 20;
        while (tail < ThumbHash.MAX_LEN) {
            out[tail] = 0;
            tail += 1;
        }

        return out[0..ThumbHash.MAX_LEN];
    }

    /// 从 ThumbHash 还原平滑模糊 RGBA8 占位图像 (最大 32x32)
    pub static fn decode(hash: []const u8): DecodedThumbImage? {
        if (hash.len < 6) {
            return null;
        }

        let avg_r = hash[0];
        let avg_g = hash[1];
        let avg_b = hash[2];
        let orig_w = hash[3] as usize;
        let orig_h = hash[4] as usize;
        let has_alpha = hash[5] != 0;

        let out_w: usize = if (orig_w == 0) 32 else (if (orig_w > 32) 32 else orig_w);
        let out_h: usize = if (orig_h == 0) 32 else (if (orig_h > 32) 32 else orig_h);

        let npix = out_w * out_h;
        let rgba_len = npix * 4;
        let rgba_ptr: [*]u8 = @alloc(rgba_len);
        let rgba = unsafe { rgba_ptr[0..rgba_len] };

        const has_quads = (hash.len >= 20 && hash[7] == 1);
        const alpha_val: u8 = if (has_alpha) (if (hash.len >= 7 && hash[6] > 0) hash[6] else 200) else 255;

        if (!has_quads) {
            for (let p in 0..npix) {
                let idx = p * 4;
                rgba[idx + 0] = avg_r;
                rgba[idx + 1] = avg_g;
                rgba[idx + 2] = avg_b;
                rgba[idx + 3] = alpha_val;
            }
        } else {
            // Bilinear blend across 2x2 quadrant centers
            let y: usize = 0;
            while (y < out_h) {
                const fy: f32 = if (out_h > 1) ((y as f32) / ((out_h - 1) as f32)) else 0.5;
                let x: usize = 0;
                while (x < out_w) {
                    const fx: f32 = if (out_w > 1) ((x as f32) / ((out_w - 1) as f32)) else 0.5;
                    const w00 = (1.0 - fx) * (1.0 - fy);
                    const w10 = fx * (1.0 - fy);
                    const w01 = (1.0 - fx) * fy;
                    const w11 = fx * fy;

                    const r_f = w00 * (hash[8] as f32) + w10 * (hash[11] as f32) + w01 * (hash[14] as f32) + w11 * (hash[17] as f32);
                    const g_f = w00 * (hash[9] as f32) + w10 * (hash[12] as f32) + w01 * (hash[15] as f32) + w11 * (hash[18] as f32);
                    const b_f = w00 * (hash[10] as f32) + w10 * (hash[13] as f32) + w01 * (hash[16] as f32) + w11 * (hash[19] as f32);

                    const idx = (y * out_w + x) * 4;
                    rgba[idx + 0] = clampByte(r_f as i32);
                    rgba[idx + 1] = clampByte(g_f as i32);
                    rgba[idx + 2] = clampByte(b_f as i32);
                    rgba[idx + 3] = alpha_val;
                    x += 1;
                }
                y += 1;
            }
        }

        return DecodedThumbImage{
            width: out_w,
            height: out_h,
            rgba: rgba,
        };
    }

    /// 将 ThumbHash 编码为完整合法的 `data:image/png;base64,...` URL
    pub static fn toDataUrl(hash: []const u8): string {
        let img = ThumbHash.decode(hash) ?? return "";
        let png_bytes = Png.encodeRgba(img.rgba, img.width as u32, img.height as u32);
        if (png_bytes.len == 0) {
            return "";
        }
        const b64 = encodeBase64Alloc(png_bytes);
        const prefix = "data:image/png;base64,";
        const total_len = prefix.len + b64.len;
        let out_ptr: [*]u8 = @alloc(total_len);
        let out = unsafe { out_ptr[0..total_len] };
        zig {
            @memcpy(out[0..prefix.len], prefix);
            @memcpy(out[prefix.len..total_len], b64);
        }
        return out;
    }
}
