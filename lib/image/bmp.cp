// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_bmp.cp | Synced: 2026-11-22T02:31:50Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_bmp.cp
// Upstream Sync: Refer to bun/src/runtime/image/codec_bmp.zig for future upstream updates.
// ============================================================================
// ============================================================================
// lib/image/bmp.cp — Windows DIB / BMP 图像编解码器
// 吸收自 Bun src/runtime/image/codec_bmp.cp
// 支持 BITMAPINFOHEADER (40 字节)、24/32 bpp、Top-Down / Bottom-Up 扫描
// ============================================================================

import { PngImage, PngColorType } from "./png.cp";

pub struct BmpHeader {
    pub width: u32 = 0,
    pub height: u32 = 0,
    pub top_down: bool = false,
    pub bpp: u16 = 32,
    pub pix_off: u32 = 54,
}

pub class Bmp {
    static fn readU16Le(b: []const u8, off: usize): u16 {
        if (off + 2 > b.len) return 0;
        return (b[off] as u16) | ((b[off + 1] as u16) << 8);
    }

    static fn readU32Le(b: []const u8, off: usize): u32 {
        if (off + 4 > b.len) return 0;
        let b0 = b[off] as u32;
        let b1 = b[off + 1] as u32;
        let b2 = b[off + 2] as u32;
        let b3 = b[off + 3] as u32;
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    static fn readI32Le(b: []const u8, off: usize): i32 {
        return Bmp.readU32Le(b, off) as i32;
    }

    static fn writeU16Le(b: []u8, off: usize, val: u16) {
        if (off + 2 > b.len) return;
        b[off] = (val & 0xFF) as u8;
        b[off + 1] = ((val >> 8) & 0xFF) as u8;
    }

    static fn writeU32Le(b: []u8, off: usize, val: u32) {
        if (off + 4 > b.len) return;
        b[off] = (val & 0xFF) as u8;
        b[off + 1] = ((val >> 8) & 0xFF) as u8;
        b[off + 2] = ((val >> 16) & 0xFF) as u8;
        b[off + 3] = ((val >> 24) & 0xFF) as u8;
    }

    /// 解析 BMP 头部
    pub static fn parseHeader(b: []const u8): BmpHeader? {
        if (b.len < 54 || b[0] != 0x42 || b[1] != 0x4D) return null; // 'B' 'M'
        let pix_off = Bmp.readU32Le(b, 10);
        let ih_size = Bmp.readU32Le(b, 14);
        if (ih_size < 40 || 14 + (ih_size as usize) > b.len) return null;

        let w_raw = Bmp.readI32Le(b, 18);
        let h_raw = Bmp.readI32Le(b, 22);
        if (w_raw <= 0 || h_raw == 0) return null;

        let bpp = Bmp.readU16Le(b, 28);
        if (bpp != 24 && bpp != 32) return null;

        let top_down = h_raw < 0;
        let abs_h: u32 = if (h_raw < 0) (-h_raw) as u32 else h_raw as u32;

        return BmpHeader{
            width: w_raw as u32,
            height: abs_h,
            top_down: top_down,
            bpp: bpp,
            pix_off: pix_off,
        };
    }

    /// 解码 BMP 文件字节流，输出统一 RGBA8 像素图像
    pub static fn decode(bytes: []const u8): PngImage? {
        let hdr = Bmp.parseHeader(bytes) ?? return null;
        let w = hdr.width;
        let h = hdr.height;
        let out_len = (w as usize) * (h as usize) * 4;
        let out_rgba_ptr: [*]u8 = @alloc(out_len);
        let out_rgba = unsafe { out_rgba_ptr[0..out_len] };

        let is_32 = hdr.bpp == 32;
        let bytes_per_pixel: usize = if (is_32) (4 as usize) else (3 as usize);
        let row_bytes_raw = (w as usize) * bytes_per_pixel;
        // BMP 行长度必须按 4 字节向上对齐
        let row_stride = ((row_bytes_raw + 3) >> 2) << 2;

        // For 32bpp BMP, detect whether any pixel has non-zero alpha (to distinguish BGRA vs BGRX)
        let has_nonzero_alpha = false;
        if (is_32) {
            let sy_chk: usize = 0;
            while (sy_chk < h as usize && !has_nonzero_alpha) {
                let row_off = (hdr.pix_off as usize) + sy_chk * row_stride;
                if (row_off + row_bytes_raw > bytes.len) break;
                let sx_chk: usize = 0;
                while (sx_chk < w as usize) {
                    if (bytes[row_off + sx_chk * 4 + 3] != 0) {
                        has_nonzero_alpha = true;
                        break;
                    }
                    sx_chk += 1;
                }
                sy_chk += 1;
            }
        }

        let y: usize = 0;
        while (y < h as usize) {
            // Windows BMP 默认 bottom-up（倒序）
            let src_y = if (hdr.top_down) y else ((h as usize) - 1 - y);
            let src_row_off = (hdr.pix_off as usize) + src_y * row_stride;
            if (src_row_off + row_bytes_raw > bytes.len) break;

            let x: usize = 0;
            while (x < w as usize) {
                let dst_idx = (y * (w as usize) + x) * 4;
                if (is_32) {
                    let s = src_row_off + x * 4;
                    // Windows BMP 32bpp 是 BGRA
                    out_rgba[dst_idx] = bytes[s + 2];     // R
                    out_rgba[dst_idx + 1] = bytes[s + 1]; // G
                    out_rgba[dst_idx + 2] = bytes[s];     // B
                    let a = bytes[s + 3];
                    out_rgba[dst_idx + 3] = if (has_nonzero_alpha) a else 255;
                } else {
                    let s = src_row_off + x * 3;
                    // Windows BMP 24bpp 是 BGR
                    out_rgba[dst_idx] = bytes[s + 2];     // R
                    out_rgba[dst_idx + 1] = bytes[s + 1]; // G
                    out_rgba[dst_idx + 2] = bytes[s];     // B
                    out_rgba[dst_idx + 3] = 255;          // A
                }
                x += 1;
            }
            y += 1;
        }

        return PngImage{
            width: w,
            height: h,
            color_type: PngColorType.Rgba,
            bit_depth: 8,
            rgba: out_rgba,
        };
    }

    /// 将 RGBA8 像素编码为 32-bit BMP 格式字节流
    pub static fn encodeRgba(rgba: []const u8, width: u32, height: u32): []u8 {
        if (width == 0 || height == 0 || rgba.len < (width * height * 4) as usize) {
            return "";
        }
        let w = width as usize;
        let h = height as usize;
        let row_stride = w * 4; // 32 bpp 已经是 4 字节对齐
        let image_size = row_stride * h;
        let total_file_size = 54 + image_size;
        let out_ptr: [*]u8 = @alloc(total_file_size);
        let out = unsafe { out_ptr[0..total_file_size] };

        // 1. BITMAPFILEHEADER (14 字节)
        out[0] = 0x42; // 'B'
        out[1] = 0x4D; // 'M'
        Bmp.writeU32Le(out, 2, total_file_size as u32);
        Bmp.writeU16Le(out, 6, 0); // reserved1
        Bmp.writeU16Le(out, 8, 0); // reserved2
        Bmp.writeU32Le(out, 10, 54); // pixel offset

        // 2. BITMAPINFOHEADER (40 字节)
        Bmp.writeU32Le(out, 14, 40); // biSize
        Bmp.writeU32Le(out, 18, width); // biWidth
        Bmp.writeU32Le(out, 22, height); // biHeight (正数代表 bottom-up)
        Bmp.writeU16Le(out, 26, 1); // biPlanes
        Bmp.writeU16Le(out, 28, 32); // biBitCount (32 bpp)
        Bmp.writeU32Le(out, 30, 0); // biCompression (BI_RGB)
        Bmp.writeU32Le(out, 34, image_size as u32); // biSizeImage
        Bmp.writeU32Le(out, 38, 2835); // biXPelsPerMeter (72 DPI)
        Bmp.writeU32Le(out, 42, 2835); // biYPelsPerMeter (72 DPI)
        Bmp.writeU32Le(out, 46, 0); // biClrUsed
        Bmp.writeU32Le(out, 50, 0); // biClrImportant

        // 3. 填充像素数据 (倒序从底行到顶行，BGRA 顺序)
        let y: usize = 0;
        while (y < h) {
            let src_y = h - 1 - y;
            let src_row = src_y * (w * 4);
            let dst_row = 54 + y * row_stride;

            let x: usize = 0;
            while (x < w) {
                let s = src_row + x * 4;
                let d = dst_row + x * 4;
                out[d] = rgba[s + 2];     // B
                out[d + 1] = rgba[s + 1]; // G
                out[d + 2] = rgba[s];     // R
                out[d + 3] = rgba[s + 3]; // A
                x += 1;
            }
            y += 1;
        }

        return out;
    }
}
