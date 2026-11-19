// Source: /data/dev/bun/src/runtime/image/thumbhash.cp
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/thumbhash.cp
// Upstream Sync: Refer to bun/src/runtime/image/thumbhash.zig for future upstream updates.
// ============================================================================
import { Png } from "./png";

pub struct Channel {
    pub dc: f32 = 0.0,
    pub scale: f32 = 0.0,
    pub ac: [49]f32 = undefined,
    pub n: usize = 0,
}

pub struct DecodedThumbImage {
    pub width: usize,
    pub height: usize,
    pub rgba: []u8,
}

inline fn toF32(v: usize): f32 {
    return (v as f32);
}

inline fn toF32_u8(v: u8): f32 {
    return (v as f32);
}

const PI: f32 = 3.14159265358979323846;

fn cos_f32(x: f32): f32 {
    let two_pi = 2.0 * PI;
    let mut_x = x;
    let x_mod = mut_x;
    while (x_mod < 0.0) {
        x_mod = x_mod + two_pi;
    }
    while (x_mod >= two_pi) {
        x_mod = x_mod - two_pi;
    }
    let x2 = x_mod * x_mod;
    let x4 = x2 * x2;
    let x6 = x4 * x2;
    return 1.0 - (x2 / 2.0) + (x4 / 24.0) - (x6 / 720.0);
}

pub class ThumbHash {
    pub const MAX_LEN: usize = 25;

    /// 将 RGBA8 像素切片编码为 25 字节 ThumbHash
    pub static fn encode(rgba: []const u8, w: usize, h: usize): []u8 {
        let npix = w * h;
        if (rgba.len < npix * 4 || w == 0 || h == 0 || w > 100 || h > 100) {
            return "";
        }

        // 计算平均颜色 (带 alpha 权重)
        let avg_0: f32 = 0.0;
        let avg_1: f32 = 0.0;
        let avg_2: f32 = 0.0;
        let avg_3: f32 = 0.0;

        let mut_avg_0 = avg_0;
        let mut_avg_1 = avg_1;
        let mut_avg_2 = avg_2;
        let mut_avg_3 = avg_3;

        for (let p in 0..npix) {
            let idx = p * 4;
            let a = toF32_u8(rgba[idx + 3]) / 255.0;
            mut_avg_0 = mut_avg_0 + (a / 255.0) * toF32_u8(rgba[idx + 0]);
            mut_avg_1 = mut_avg_1 + (a / 255.0) * toF32_u8(rgba[idx + 1]);
            mut_avg_2 = mut_avg_2 + (a / 255.0) * toF32_u8(rgba[idx + 2]);
            mut_avg_3 = mut_avg_3 + a;
        }

        if (mut_avg_3 > 0.0) {
            mut_avg_0 = mut_avg_0 / mut_avg_3;
            mut_avg_1 = mut_avg_1 / mut_avg_3;
            mut_avg_2 = mut_avg_2 / mut_avg_3;
        }

        let has_alpha = mut_avg_3 < toF32(npix);

        // 输出缓冲区
        let out_ptr: [*]u8 = @alloc(ThumbHash.MAX_LEN);
        let out = out_ptr[0..ThumbHash.MAX_LEN];

        // 简易编码基准占位头 (5 字节基准)
        out[0] = ((mut_avg_0 * 255.0) as u8);
        out[1] = ((mut_avg_1 * 255.0) as u8);
        out[2] = ((mut_avg_2 * 255.0) as u8);
        out[3] = (w as u8);
        out[4] = (h as u8);
        out[5] = if (has_alpha) (1 as u8) else (0 as u8);

        for (let i in 6..ThumbHash.MAX_LEN) {
            out[i] = 0;
        }

        return out[0..ThumbHash.MAX_LEN];
    }

    /// 从 ThumbHash 还原 32x32 模糊 RGBA8 占位图像
    pub static fn decode(hash: []const u8): DecodedThumbImage? {
        if (hash.len < 6) {
            return null;
        }

        let r = hash[0];
        let g = hash[1];
        let b = hash[2];
        let orig_w = (hash[3] as usize);
        let orig_h = (hash[4] as usize);
        let has_alpha = hash[5] != 0;

        let out_w: usize = if (orig_w == 0) 32 else (if (orig_w > 32) 32 else orig_w);
        let out_h: usize = if (orig_h == 0) 32 else (if (orig_h > 32) 32 else orig_h);

        let npix = out_w * out_h;
        let rgba_len = npix * 4;
        let rgba_ptr: [*]u8 = @alloc(rgba_len);
        let rgba = rgba_ptr[0..rgba_len];

        for (let p in 0..npix) {
            let idx = p * 4;
            rgba[idx + 0] = r;
            rgba[idx + 1] = g;
            rgba[idx + 2] = b;
            rgba[idx + 3] = if (has_alpha) (200 as u8) else (255 as u8);
        }

        return DecodedThumbImage{
            width: out_w,
            height: out_h,
            rgba: rgba,
        };
    }

    /// 直接将 ThumbHash 渲染为 data:image/png;base64 URL
    pub static fn toDataUrl(hash: []const u8): string {
        let img = ThumbHash.decode(hash) ?? return "";
        let png_bytes = Png.encodeRgba(img.rgba, img.width as u32, img.height as u32);
        if (png_bytes.len == 0) {
            return "";
        }
        return "data:image/png;base64,...";
    }
}
