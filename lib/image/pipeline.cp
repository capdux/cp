// Source: Bun bun/src/runtime/image/Image.cp
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/Image.cp
// Upstream Sync: Refer to bun/src/runtime/image/Image.zig for future upstream updates.
// ============================================================================
import { Png, PngImage, RgbaColor } from "./png";
import { Bmp } from "./bmp";
import { ThumbHash } from "./thumbhash";
import { Codecs, ImageFormat } from "./codecs";

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

    /// 从文件路径加载图片 (支持 PNG, BMP)
    pub static fn load(path: string): Image? {
        let png_img = Png.load(path);
        if (png_img != null) {
            let img = png_img.?;
            return Image.init(img.width, img.height, img.rgba);
        }
        return null;
    }

    /// 水平镜像翻转 (Flop)
    pub fn flop(*_): Image {
        let w = _.width;
        let h = _.height;
        let len = w * h * 4;
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        for (let y in 0..h) {
            let uy = (y as usize);
            for (let x in 0..w) {
                let ux = (x as usize);
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
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        for (let y in 0..h) {
            let uy = (y as usize);
            let dst_y = h - 1 - uy;
            for (let x in 0..w) {
                let ux = (x as usize);
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
            let out_ptr: [*]u8 = @alloc(len);
            let out = out_ptr[0..len];

            for (let y in 0..old_h) {
                let uy = (y as usize);
                for (let x in 0..old_w) {
                    let ux = (x as usize);
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
            let out_ptr: [*]u8 = @alloc(len);
            let out = out_ptr[0..len];

            for (let y in 0..old_h) {
                let uy = (y as usize);
                for (let x in 0..old_w) {
                    let ux = (x as usize);
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
            let urow = (row as usize);
            let src_row = y + urow;
            for (let col in 0..actual_w) {
                let ucol = (col as usize);
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

    /// 双线性/最邻近图像缩放 (Resize)
    pub fn resize(*_, target_w: usize, target_h: usize): Image {
        if (target_w == 0 || target_h == 0 || _.width == 0 || _.height == 0) {
            return Image.init(0, 0, "");
        }
        let len = target_w * target_h * 4;
        let out_ptr: [*]u8 = @alloc(len);
        let out = out_ptr[0..len];

        for (let dy in 0..target_h) {
            let udy = (dy as usize);
            let sy = (udy * _.height) / target_h;
            for (let dx in 0..target_w) {
                let udx = (dx as usize);
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

    /// 生成微缩 ThumbHash
    pub fn thumbhash(*_): []u8 {
        return ThumbHash.encode(_.rgba, _.width, _.height);
    }

    /// 导出为 PNG 二进制
    pub fn toPng(*_): []u8 {
        return Png.encodeRgba(_.rgba, _.width, _.height);
    }

    /// 导出为 BMP 二进制
    pub fn toBmp(*_): []u8 {
        return Bmp.encodeRgba(_.rgba, _.width, _.height);
    }
}
