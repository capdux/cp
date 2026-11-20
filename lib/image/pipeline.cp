// Source: /data/dev/bun/src/runtime/image/Image.cp
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/Image.cp
// Upstream Sync: Refer to bun/src/runtime/image/Image.zig for future upstream updates.
// ============================================================================
const std = @import("std");
import { Png, PngImage, RgbaColor } from "./png";
import { Bmp } from "./bmp";
import { Gif } from "./gif";
import { ThumbHash } from "./thumbhash";
import { Codecs, ImageFormat, ExifOrientation } from "./codecs";

fn readImageFileC(path: string): ([]const u8)? {
    if (path.len == 0 || path.len >= 4096) { return null; }
    let path_buf: [4096]u8 = undefined;
    zig {
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
    }
    let path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    let z_flags = std.c.O{ .CLOEXEC = true };
    let fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0));
    if (fd < 0) { return null; }

    let stx: std.os.linux.Statx = undefined;
    const rc = std.c.statx(fd, "", std.os.linux.AT.EMPTY_PATH, std.os.linux.STATX{ .SIZE = true }, &stx);
    if (rc != 0 || stx.size == 0) {
        * = std.c.close(fd);
        return null;
    }
    const sz = stx.size as usize;
    let ptr: [*]u8 = @alloc(sz);
    let buf = ptr[0..sz];
    let total: usize = 0;
    while (total < sz) {
        let n = std.c.read(fd, buf[total..sz].ptr, sz - total);
        if (n <= 0) break;
        total += (n as usize);
    }
    * = std.c.close(fd);
    return buf[0..total];
}

fn writeImageFileC(path: string, data: []const u8): bool {
    if (path.len == 0 || path.len >= 4096 || data.len == 0) { return false; }
    let path_buf: [4096]u8 = undefined;
    zig {
        @memcpy(path_buf[0..path.len], path);
        path_buf[path.len] = 0;
    }
    let path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    let z_flags = std.c.O{ .CREAT = true, .TRUNC = true, .CLOEXEC = true };
    z_flags.ACCMODE = .WRONLY;
    let fd = std.c.open(path_z, z_flags, @as(std.c.mode_t, 0o644));
    if (fd < 0) { return false; }
    let written: usize = 0;
    let ok = true;
    while (written < data.len) {
        let n = std.c.write(fd, data[written..data.len].ptr, data.len - written);
        if (n <= 0) { ok = false; break; }
        written += (n as usize);
    }
    * = std.c.close(fd);
    return ok;
}

fn clampChannel(v: i32): u8 {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return v as u8;
}

pub struct Image {
    pub width: usize,
    pub height: usize,
    pub rgba: []u8,

    pub static fn init(width: usize, height: usize, rgba: []u8): Image {
        return Image{
            width: width,
            height: height,
            rgba: rgba,
        };
    }

    /// 从编码字节流自动嗅探并解码图像 (支持 PNG, BMP, GIF)
    pub static fn decode(bytes: []const u8): Image? {
        const fmt = ImageFormat.detect(bytes) ?? return null;
        if (fmt == ImageFormat.Png) {
            if (img := Png.decode(bytes)) {
                return Image.init(img.width as usize, img.height as usize, img.rgba);
            }
            return null;
        }
        if (fmt == ImageFormat.Bmp) {
            if (img := Bmp.decode(bytes)) {
                return Image.init(img.width as usize, img.height as usize, img.rgba);
            }
            return null;
        }
        if (fmt == ImageFormat.Gif) {
            if (img := Gif.decode(bytes)) {
                return Image.init(img.width, img.height, img.rgba);
            }
            return null;
        }
        return null;
    }

    /// 从文件路径加载图片 (自动识别 PNG, BMP, GIF)
    pub static fn load(path: string): Image? {
        if (bytes := readImageFileC(path)) {
            return Image.decode(bytes);
        }
        return null;
    }

    /// 水平镜像翻转 (Flop)
    pub fn flop(*_): Image {
        let w = _.width;
        let h = _.height;
        let len = w * h * 4;
        if (len == 0) { return Image.init(0, 0, ""); }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        for (let y in 0..h) {
            let uy = y as usize;
            for (let x in 0..w) {
                let ux = x as usize;
                let src_idx = (uy * w + ux) * 4;
                let dst_idx = (uy * w + (w - 1 - ux)) * 4;
                out[dst_idx + 0] = _.rgba[src_idx + 0];
                out[dst_idx + 1] = _.rgba[src_idx + 1];
                out[dst_idx + 2] = _.rgba[src_idx + 2];
                out[dst_idx + 3] = _.rgba[src_idx + 3];
            }
        }
        return Image.init(w, h, out);
    }

    /// 垂直镜像翻转 (Flip)
    pub fn flip(*_): Image {
        let w = _.width;
        let h = _.height;
        let len = w * h * 4;
        if (len == 0) { return Image.init(0, 0, ""); }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        for (let y in 0..h) {
            let uy = y as usize;
            let dst_y = h - 1 - uy;
            for (let x in 0..w) {
                let ux = x as usize;
                let src_idx = (uy * w + ux) * 4;
                let dst_idx = (dst_y * w + ux) * 4;
                out[dst_idx + 0] = _.rgba[src_idx + 0];
                out[dst_idx + 1] = _.rgba[src_idx + 1];
                out[dst_idx + 2] = _.rgba[src_idx + 2];
                out[dst_idx + 3] = _.rgba[src_idx + 3];
            }
        }
        return Image.init(w, h, out);
    }

    /// 顺时针旋转 (90, 180, 270 度)
    pub fn rotate(*_, degrees: u16): Image {
        if (degrees == 180) {
            let img_flop = _.flop();
            return img_flop.flip();
        }
        if (degrees == 90) {
            let old_w = _.width;
            let old_h = _.height;
            let new_w = old_h;
            let new_h = old_w;
            let len = new_w * new_h * 4;
            if (len == 0) { return Image.init(0, 0, ""); }
            let out_ptr: [*]u8 = @alloc(len);
            let out = out_ptr[0..len];

            for (let y in 0..old_h) {
                let uy = y as usize;
                for (let x in 0..old_w) {
                    let ux = x as usize;
                    let src_idx = (uy * old_w + ux) * 4;
                    let dst_x = old_h - 1 - uy;
                    let dst_y = ux;
                    let dst_idx = (dst_y * new_w + dst_x) * 4;
                    out[dst_idx + 0] = _.rgba[src_idx + 0];
                    out[dst_idx + 1] = _.rgba[src_idx + 1];
                    out[dst_idx + 2] = _.rgba[src_idx + 2];
                    out[dst_idx + 3] = _.rgba[src_idx + 3];
                }
            }
            return Image.init(new_w, new_h, out);
        }
        if (degrees == 270) {
            let old_w = _.width;
            let old_h = _.height;
            let new_w = old_h;
            let new_h = old_w;
            let len = new_w * new_h * 4;
            if (len == 0) { return Image.init(0, 0, ""); }
            let out_ptr: [*]u8 = @alloc(len);
            let out = out_ptr[0..len];

            for (let y in 0..old_h) {
                let uy = y as usize;
                for (let x in 0..old_w) {
                    let ux = x as usize;
                    let src_idx = (uy * old_w + ux) * 4;
                    let dst_x = uy;
                    let dst_y = old_w - 1 - ux;
                    let dst_idx = (dst_y * new_w + dst_x) * 4;
                    out[dst_idx + 0] = _.rgba[src_idx + 0];
                    out[dst_idx + 1] = _.rgba[src_idx + 1];
                    out[dst_idx + 2] = _.rgba[src_idx + 2];
                    out[dst_idx + 3] = _.rgba[src_idx + 3];
                }
            }
            return Image.init(new_w, new_h, out);
        }
        return Image.init(_.width, _.height, _.rgba);
    }

    /// 应用 EXIF Orientation 方向变换自动矫正图片方向
    pub fn applyExifOrientation(*_, ori: ExifOrientation): Image {
        const tf = Codecs.getOrientationTransform(ori);
        let cur = Image.init(_.width, _.height, _.rgba);
        if (tf.flop) {
            cur = cur.flop();
        }
        if (tf.flip) {
            cur = cur.flip();
        }
        if (tf.rotate != 0) {
            cur = cur.rotate(tf.rotate);
        }
        return cur;
    }

    /// 区域裁切 (Crop)
    pub fn crop(*_, x: usize, y: usize, crop_w: usize, crop_h: usize): Image {
        if (x >= _.width || y >= _.height || crop_w == 0 || crop_h == 0) {
            return Image.init(0, 0, "");
        }
        let actual_w = if (x + crop_w > _.width) (_.width - x) else crop_w;
        let actual_h = if (y + crop_h > _.height) (_.height - y) else crop_h;

        let len = actual_w * actual_h * 4;
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        for (let row in 0..actual_h) {
            let urow = row as usize;
            let src_row = y + urow;
            for (let col in 0..actual_w) {
                let ucol = col as usize;
                let src_col = x + ucol;
                let src_idx = (src_row * _.width + src_col) * 4;
                let dst_idx = (urow * actual_w + ucol) * 4;
                out[dst_idx + 0] = _.rgba[src_idx + 0];
                out[dst_idx + 1] = _.rgba[src_idx + 1];
                out[dst_idx + 2] = _.rgba[src_idx + 2];
                out[dst_idx + 3] = _.rgba[src_idx + 3];
            }
        }
        return Image.init(actual_w, actual_h, out);
    }

    /// 最邻近图像缩放 (Nearest-Neighbor Resize)
    pub fn resizeNearest(*_, target_w: usize, target_h: usize): Image {
        if (target_w == 0 || target_h == 0 || _.width == 0 || _.height == 0) {
            return Image.init(0, 0, "");
        }
        let len = target_w * target_h * 4;
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        for (let dy in 0..target_h) {
            let udy = dy as usize;
            let sy = (udy * _.height) / target_h;
            for (let dx in 0..target_w) {
                let udx = dx as usize;
                let sx = (udx * _.width) / target_w;
                let src_idx = (sy * _.width + sx) * 4;
                let dst_idx = (udy * target_w + udx) * 4;
                out[dst_idx + 0] = _.rgba[src_idx + 0];
                out[dst_idx + 1] = _.rgba[src_idx + 1];
                out[dst_idx + 2] = _.rgba[src_idx + 2];
                out[dst_idx + 3] = _.rgba[src_idx + 3];
            }
        }
        return Image.init(target_w, target_h, out);
    }

    /// 双线性插值图像缩放 (Bilinear Resize)
    pub fn resize(*_, target_w: usize, target_h: usize): Image {
        if (target_w == 0 || target_h == 0 || _.width == 0 || _.height == 0) {
            return Image.init(0, 0, "");
        }
        if (_.width == 1 && _.height == 1) {
            return _.resizeNearest(target_w, target_h);
        }
        let len = target_w * target_h * 4;
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        const src_w_f = (_.width - 1) as f32;
        const src_h_f = (_.height - 1) as f32;

        let dy: usize = 0;
        while (dy < target_h) {
            const gy = if (target_h > 1) (((dy as f32) * src_h_f) / ((target_h - 1) as f32)) else 0.0;
            const y0 = gy as usize;
            const y1 = if (y0 + 1 < _.height) (y0 + 1) else y0;
            const ty = gy - (y0 as f32);

            let dx: usize = 0;
            while (dx < target_w) {
                const gx = if (target_w > 1) (((dx as f32) * src_w_f) / ((target_w - 1) as f32)) else 0.0;
                const x0 = gx as usize;
                const x1 = if (x0 + 1 < _.width) (x0 + 1) else x0;
                const tx = gx - (x0 as f32);

                const idx00 = (y0 * _.width + x0) * 4;
                const idx10 = (y0 * _.width + x1) * 4;
                const idx01 = (y1 * _.width + x0) * 4;
                const idx11 = (y1 * _.width + x1) * 4;

                const w00 = (1.0 - tx) * (1.0 - ty);
                const w10 = tx * (1.0 - ty);
                const w01 = (1.0 - tx) * ty;
                const w11 = tx * ty;

                const dst_idx = (dy * target_w + dx) * 4;
                let c: usize = 0;
                while (c < 4) {
                    const v = w00 * (_.rgba[idx00 + c] as f32)
                        + w10 * (_.rgba[idx10 + c] as f32)
                        + w01 * (_.rgba[idx01 + c] as f32)
                        + w11 * (_.rgba[idx11 + c] as f32);
                    out[dst_idx + c] = clampChannel((v + 0.5) as i32);
                    c += 1;
                }
                dx += 1;
            }
            dy += 1;
        }
        return Image.init(target_w, target_h, out);
    }

    /// 按宽高比自适应生成缩略图（限制在 max_w x max_h 框内）
    pub fn thumbnail(*_, max_w: usize, max_h: usize): Image {
        if (max_w == 0 || max_h == 0 || _.width == 0 || _.height == 0) {
            return Image.init(0, 0, "");
        }
        let dst_w = _.width;
        let dst_h = _.height;
        if (dst_w > max_w) {
            dst_h = (dst_h * max_w) / dst_w;
            dst_w = max_w;
        }
        if (dst_h > max_h) {
            dst_w = (dst_w * max_h) / dst_h;
            dst_h = max_h;
        }
        if (dst_w == 0) { dst_w = 1; }
        if (dst_h == 0) { dst_h = 1; }
        return _.resize(dst_w, dst_h);
    }

    /// 灰度滤镜 (ITU-R BT.601 亮度公式, 保留 Alpha)
    pub fn grayscale(*_): Image {
        const npix = _.width * _.height;
        const len = npix * 4;
        if (len == 0 || _.rgba.len < len) { return Image.init(0, 0, ""); }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        let p: usize = 0;
        while (p < npix) {
            const idx = p * 4;
            const r = _.rgba[idx + 0] as u32;
            const g = _.rgba[idx + 1] as u32;
            const b = _.rgba[idx + 2] as u32;
            const y = ((77 * r + 150 * g + 29 * b) >> 8) as u8;
            out[idx + 0] = y;
            out[idx + 1] = y;
            out[idx + 2] = y;
            out[idx + 3] = _.rgba[idx + 3];
            p += 1;
        }
        return Image.init(_.width, _.height, out);
    }

    /// 反色滤镜 (Invert RGB, 保留 Alpha)
    pub fn invert(*_): Image {
        const npix = _.width * _.height;
        const len = npix * 4;
        if (len == 0 || _.rgba.len < len) { return Image.init(0, 0, ""); }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        let p: usize = 0;
        while (p < npix) {
            const idx = p * 4;
            out[idx + 0] = (255 as u8) - _.rgba[idx + 0];
            out[idx + 1] = (255 as u8) - _.rgba[idx + 1];
            out[idx + 2] = (255 as u8) - _.rgba[idx + 2];
            out[idx + 3] = _.rgba[idx + 3];
            p += 1;
        }
        return Image.init(_.width, _.height, out);
    }

    /// 亮度调整滤镜 (`delta` in `-255..255`, 保留 Alpha)
    pub fn brightness(*_, delta: i16): Image {
        const npix = _.width * _.height;
        const len = npix * 4;
        if (len == 0 || _.rgba.len < len) { return Image.init(0, 0, ""); }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];
        const d = delta as i32;

        let p: usize = 0;
        while (p < npix) {
            const idx = p * 4;
            out[idx + 0] = clampChannel((_.rgba[idx + 0] as i32) + d);
            out[idx + 1] = clampChannel((_.rgba[idx + 1] as i32) + d);
            out[idx + 2] = clampChannel((_.rgba[idx + 2] as i32) + d);
            out[idx + 3] = _.rgba[idx + 3];
            p += 1;
        }
        return Image.init(_.width, _.height, out);
    }

    /// 复古棕褐色滤镜 (Sepia)
    pub fn sepia(*_): Image {
        const npix = _.width * _.height;
        const len = npix * 4;
        if (len == 0 || _.rgba.len < len) { return Image.init(0, 0, ""); }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        let p: usize = 0;
        while (p < npix) {
            const idx = p * 4;
            const r = _.rgba[idx + 0] as u32;
            const g = _.rgba[idx + 1] as u32;
            const b = _.rgba[idx + 2] as u32;
            const sr = (393 * r + 769 * g + 189 * b) / 1000;
            const sg = (349 * r + 686 * g + 168 * b) / 1000;
            const sb = (272 * r + 534 * g + 131 * b) / 1000;
            out[idx + 0] = if (sr > 255) 255 else (sr as u8);
            out[idx + 1] = if (sg > 255) 255 else (sg as u8);
            out[idx + 2] = if (sb > 255) 255 else (sb as u8);
            out[idx + 3] = _.rgba[idx + 3];
            p += 1;
        }
        return Image.init(_.width, _.height, out);
    }

    /// 3x3 均值模糊滤镜 (Box Blur)
    pub fn blur(*_): Image {
        const w = _.width;
        const h = _.height;
        const len = w * h * 4;
        if (len == 0 || _.rgba.len < len) { return Image.init(0, 0, ""); }
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        let y: usize = 0;
        while (y < h) {
            let x: usize = 0;
            while (x < w) {
                let sum_r: u32 = 0;
                let sum_g: u32 = 0;
                let sum_b: u32 = 0;
                let sum_a: u32 = 0;
                let count: u32 = 0;

                let ky: isize = -1;
                while (ky <= 1) {
                    const ny = (y as isize) + ky;
                    if (ny >= 0 && ny < (h as isize)) {
                        let kx: isize = -1;
                        while (kx <= 1) {
                            const nx = (x as isize) + kx;
                            if (nx >= 0 && nx < (w as isize)) {
                                const sidx = ((ny as usize) * w + (nx as usize)) * 4;
                                sum_r += _.rgba[sidx + 0] as u32;
                                sum_g += _.rgba[sidx + 1] as u32;
                                sum_b += _.rgba[sidx + 2] as u32;
                                sum_a += _.rgba[sidx + 3] as u32;
                                count += 1;
                            }
                            kx += 1;
                        }
                    }
                    ky += 1;
                }
                const didx = (y * w + x) * 4;
                out[didx + 0] = (sum_r / count) as u8;
                out[didx + 1] = (sum_g / count) as u8;
                out[didx + 2] = (sum_b / count) as u8;
                out[didx + 3] = (sum_a / count) as u8;
                x += 1;
            }
            y += 1;
        }
        return Image.init(w, h, out);
    }

    /// 生成微缩 ThumbHash
    pub fn thumbhash(*_): []u8 {
        return ThumbHash.encode(_.rgba, _.width, _.height);
    }

    /// 导出为 PNG 二进制
    pub fn toPng(*_): []u8 {
        return Png.encodeRgba(_.rgba, _.width as u32, _.height as u32);
    }

    /// 导出为 BMP 二进制
    pub fn toBmp(*_): []u8 {
        return Bmp.encodeRgba(_.rgba, _.width as u32, _.height as u32);
    }

    /// 直接编码并保存为 PNG 文件
    pub fn savePng(*_, path: string): bool {
        const bytes = _.toPng();
        return writeImageFileC(path, bytes);
    }

    /// 直接编码并保存为 BMP 文件
    pub fn saveBmp(*_, path: string): bool {
        const bytes = _.toBmp();
        return writeImageFileC(path, bytes);
    }
}
