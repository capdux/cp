// ============================================================================
// Comprehensive Test Suite for cap:image Full Ecosystem
// Covers: PNG, BMP, GIF, ThumbHash, EXIF, Image Pipeline (Crop, Resize, Rotate)
// ============================================================================

import { test, expect_eq } from "cap:test";
import { 
    Codecs, ImageFormat, ExifOrientation, ExifTransform,
    Png, PngImage, RgbaColor, 
    Bmp, 
    Quantizer, 
    Gif, 
    ThumbHash, 
    Exif, 
    Image, 
    Jpeg, 
    Webp 
} from "cap:image";

fn main() {
    test("1. cap:image — GIF LZW Decode & Logical Screen Descriptor probe", () => {
        // GIF89a 2x2 with GCT
        const gif_data: string = "GIF89a\x02\x00\x02\x00\x80\x00\x00\xFF\x00\x00\x00\x00\xFF\x2C\x00\x00\x00\x00\x02\x00\x02\x00\x00\x02\x03\x04\x15\x05\x00\x3B";

        const header = Gif.probe(gif_data);
        expect_eq(header != null, true);
        if (h := header) {
            expect_eq(h.width, 2);
            expect_eq(h.height, 2);
            expect_eq(h.has_gct, true);
            expect_eq(h.gct_size, 2);
        }
    });

    test("2. cap:image — Evan Wallace ThumbHash LQIP Generation & Decode", () => {
        const w: usize = 4;
        const h: usize = 4;
        const ptr: [*]u8 = @alloc(w * h * 4);
        const rgba = ptr[0..(w * h * 4)];

        for (let i in 0..(w * h)) {
            let p = (i as usize) * 4;
            rgba[p + 0] = 120;
            rgba[p + 1] = 200;
            rgba[p + 2] = 250;
            rgba[p + 3] = 255;
        }

        const hash = ThumbHash.encode(rgba, w, h);
        expect_eq(hash.len > 0, true);
        expect_eq(hash.len <= ThumbHash.MAX_LEN, true);

        const decoded = ThumbHash.decode(hash);
        expect_eq(decoded != null, true);
        if (d := decoded) {
            expect_eq(d.width > 0, true);
            expect_eq(d.height > 0, true);
            expect_eq(d.rgba.len == d.width * d.height * 4, true);
            expect_eq(d.rgba[0] >= 100, true);
            expect_eq(d.rgba[1] >= 180, true);
        }
    });

    test("3. cap:image — EXIF Orientation Parsing & Coordinate Transform", () => {
        const jpeg_exif: string = "\xFF\xD8\xFF\xE1\x00\x22Exif\x00\x00II\x2A\x00\x08\x00\x00\x00\x01\x00\x12\x01\x03\x00\x01\x00\x00\x00\x06\x00\x00\x00\x00\x00\x00\x00\xFF\xDA";

        const orient = Exif.readJpeg(jpeg_exif);
        const transform = Codecs.getOrientationTransform(orient);
        expect_eq(transform.rotate, 90);
        expect_eq(transform.flop, false);
        expect_eq(transform.flip, false);
    });

    test("4. cap:image — High-Level Image Pipeline: Flop, Flip, Rotate & Crop", () => {
        const w: usize = 2;
        const h: usize = 2;
        const ptr: [*]u8 = @alloc(w * h * 4);
        const rgba = ptr[0..(w * h * 4)];

        rgba[0] = 1; rgba[1] = 0; rgba[2] = 0; rgba[3] = 255;
        rgba[4] = 2; rgba[5] = 0; rgba[6] = 0; rgba[7] = 255;
        rgba[8] = 3; rgba[9] = 0; rgba[10] = 0; rgba[11] = 255;
        rgba[12] = 4; rgba[13] = 0; rgba[14] = 0; rgba[15] = 255;

        const img = Image.init(w, h, rgba);

        // 1. 测试 Flop (水平镜像)
        const img_flop = img.flop();
        expect_eq(img_flop.rgba[0], 2);
        expect_eq(img_flop.rgba[4], 1);

        // 2. 测试 Flip (垂直翻转)
        const img_flip = img.flip();
        expect_eq(img_flip.rgba[0], 3);
        expect_eq(img_flip.rgba[8], 1);

        // 3. 测试 Rotate90 顺时针
        const img_rot = img.rotate(90);
        expect_eq(img_rot.rgba[0], 3);
        expect_eq(img_rot.rgba[4], 1);

        // 4. 测试 Crop (裁切 1x1 区域)
        const cropped = img.crop(1, 1, 1, 1);
        expect_eq(cropped.width, 1);
        expect_eq(cropped.height, 1);
        expect_eq(cropped.rgba[0], 4);

        // 5. 测试 Resize (双线性缩放至 4x4)
        const resized = img.resize(4, 4);
        expect_eq(resized.width, 4);
        expect_eq(resized.height, 4);
        expect_eq(resized.rgba.len, 64);
    });

    test("5. cap:image — WebP VP8/VP8L Container Probe", () => {
        const webp_raw: string = "RIFF\x1E\x00\x00\x00WEBPVP8 \x0A\x00\x00\x00\x00\x00\x00\x9D\x01\x2A\x64\x00\x32\x00";

        const wp = Webp.probe(webp_raw);
        expect_eq(wp != null, true);
        if (w := wp) {
            expect_eq(w.width, 100);
            expect_eq(w.height, 50);
            expect_eq(w.has_alpha, false);
            expect_eq(w.is_lossless, false);
        }
    });
}
