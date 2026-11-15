// cap/test/test_image_png.cp
// Comprehensive test suite for CP Native Image Standard Library (cap:image / cap:png)
// Validates PNG screenshot encoding/decoding, pixel verification, format sniffing, and BMP support

import { test, expect_eq } from "cap:test";
import { 
    Png, 
    PngImage, 
    PngColorType, 
    Codecs, 
    ImageFormat, 
    Bmp, 
    Quantizer 
} from "cap:image";

fn main() {
    test("1. cap:image — Codecs format sniffing, MIME & extension mapping", () => {
        // PNG magic sniffing
        const png_magic = "\x89PNG\r\n\x1a\n\x00\x00\x00\x0DIHDR";
        const fmt_png = Codecs.sniff(png_magic);
        expect_eq(fmt_png != null, true);
        expect_eq(fmt_png.? == ImageFormat.Png, true);
        expect_eq(Codecs.mime(ImageFormat.Png), "image/png");
        expect_eq(Codecs.fromExtension("page_screenshot.png") == ImageFormat.Png, true);

        // JPEG magic sniffing
        const jpeg_magic = "\xFF\xD8\xFF\xE0\x00\x10JFIF";
        const fmt_jpeg = Codecs.sniff(jpeg_magic);
        expect_eq(fmt_jpeg != null, true);
        expect_eq(fmt_jpeg.? == ImageFormat.Jpeg, true);
        expect_eq(Codecs.mime(ImageFormat.Jpeg), "image/jpeg");
        expect_eq(Codecs.fromExtension("photo.jpg") == ImageFormat.Jpeg, true);

        // BMP magic sniffing
        const bmp_magic = "BM\x36\x00\x00\x00\x00\x00";
        const fmt_bmp = Codecs.sniff(bmp_magic);
        expect_eq(fmt_bmp != null, true);
        expect_eq(fmt_bmp.? == ImageFormat.Bmp, true);
        expect_eq(Codecs.mime(ImageFormat.Bmp), "image/bmp");

        // WebP magic sniffing
        const webp_magic = "RIFF\x20\x00\x00\x00WEBPVP8 ";
        const fmt_webp = Codecs.sniff(webp_magic);
        expect_eq(fmt_webp != null, true);
        expect_eq(fmt_webp.? == ImageFormat.WebP, true);
    });

    test("2. cap:png — Pure CP RGBA8 Webpage Screenshot Encoding & Header Integrity", () => {
        // Create an 8x8 RGBA canvas simulating a rendered browser element
        const width: u32 = 8;
        const height: u32 = 8;
        const total_len = (width * height * 4) as usize;
        let ptr: [*]u8 = @alloc(total_len);
        let rgba = ptr[0..total_len];

        // Fill with 4 distinct colored 4x4 quadrants:
        // Top-Left: Red (255, 0, 0, 255)
        // Top-Right: Green (0, 255, 0, 255)
        // Bottom-Left: Blue (0, 0, 255, 255)
        // Bottom-Right: Yellow (255, 255, 0, 255)
        let y: u32 = 0;
        while (y < height) {
            let x: u32 = 0;
            while (x < width) {
                let idx = ((y * width + x) * 4) as usize;
                if (x < 4 && y < 4) {
                    rgba[idx] = 255; rgba[idx + 1] = 0; rgba[idx + 2] = 0; rgba[idx + 3] = 255;
                } else if (x >= 4 && y < 4) {
                    rgba[idx] = 0; rgba[idx + 1] = 255; rgba[idx + 2] = 0; rgba[idx + 3] = 255;
                } else if (x < 4 && y >= 4) {
                    rgba[idx] = 0; rgba[idx + 1] = 0; rgba[idx + 2] = 255; rgba[idx + 3] = 255;
                } else {
                    rgba[idx] = 255; rgba[idx + 1] = 255; rgba[idx + 2] = 0; rgba[idx + 3] = 255;
                }
                x += 1;
            }
            y += 1;
        }

        // Encode to standard PNG bytes
        const png_bytes = Png.encodeRgba(rgba, width, height);
        expect_eq(png_bytes.len > 50, true);

        // Verify PNG 8-byte Magic Signature
        expect_eq(png_bytes[0], 0x89);
        expect_eq(png_bytes[1], 0x50); // 'P'
        expect_eq(png_bytes[2], 0x4E); // 'N'
        expect_eq(png_bytes[3], 0x47); // 'G'
        expect_eq(png_bytes[4], 0x0D); // '\r'
        expect_eq(png_bytes[5], 0x0A); // '\n'
        expect_eq(png_bytes[6], 0x1A);
        expect_eq(png_bytes[7], 0x0A); // '\n'

        // Verify IHDR chunk structure
        // IHDR starts at offset 8: length = 13 (offset 8..11), type = "IHDR" (offset 12..15)
        expect_eq(png_bytes[12], 0x49); // 'I'
        expect_eq(png_bytes[13], 0x48); // 'H'
        expect_eq(png_bytes[14], 0x44); // 'D'
        expect_eq(png_bytes[15], 0x52); // 'R'

        // Color Type at offset 25 must be 6 (RGBA Truecolor with Alpha)
        expect_eq(png_bytes[25], 6);
    });

    test("3. cap:png — Roundtrip Decode & Coordinate Pixel Inspection", () => {
        // Construct a 4x4 test matrix
        const width: u32 = 4;
        const height: u32 = 4;
        const total_len = (width * height * 4) as usize;
        let ptr: [*]u8 = @alloc(total_len);
        let original_rgba = ptr[0..total_len];
        @memset(original_rgba, 0);

        // Set pixel (1, 1) = [120, 150, 200, 255]
        let idx = ((1 * width + 1) * 4) as usize;
        original_rgba[idx] = 120;
        original_rgba[idx + 1] = 150;
        original_rgba[idx + 2] = 200;
        original_rgba[idx + 3] = 255;

        // Set pixel (3, 2) = [240, 10, 80, 200]
        let idx2 = ((2 * width + 3) * 4) as usize;
        original_rgba[idx2] = 240;
        original_rgba[idx2 + 1] = 10;
        original_rgba[idx2 + 2] = 80;
        original_rgba[idx2 + 3] = 200;

        // Encode to PNG
        const png_data = Png.encodeRgba(original_rgba, width, height);
        expect_eq(png_data.len > 0, true);

        // Decode back
        const decoded_opt = Png.decode(png_data);
        expect_eq(decoded_opt != null, true);

        const decoded = decoded_opt.?;
        expect_eq(decoded.width as i64, 4);
        expect_eq(decoded.height as i64, 4);
        expect_eq(decoded.color_type == PngColorType.Rgba, true);

        // Check pixel (1, 1)
        const p1 = decoded.getPixel(1, 1);
        expect_eq(p1.r, 120);
        expect_eq(p1.g, 150);
        expect_eq(p1.b, 200);
        expect_eq(p1.a, 255);

        // Check pixel (3, 2)
        const p2 = decoded.getPixel(3, 2);
        expect_eq(p2.r, 240);
        expect_eq(p2.g, 10);
        expect_eq(p2.b, 80);
        expect_eq(p2.a, 200);
    });

    test("4. cap:png — File System Save & Load Screenshot Pipeline", () => {
        const test_path = "temp/cap_screenshot_test.png";
        const w: u32 = 10;
        const h: u32 = 10;
        const total_len = (w * h * 4) as usize;
        let ptr: [*]u8 = @alloc(total_len);
        let pixels = ptr[0..total_len];
        @memset(pixels, 0);

        // Draw a cyan diagonal line
        let i: u32 = 0;
        while (i < 10) {
            let offset = ((i * w + i) * 4) as usize;
            pixels[offset] = 0;       // R
            pixels[offset + 1] = 255; // G
            pixels[offset + 2] = 255; // B
            pixels[offset + 3] = 255; // A
            i += 1;
        }

        // Save screenshot
        const save_ok = Png.save(test_path, pixels, w, h);
        expect_eq(save_ok, true);

        // Load screenshot back
        const loaded_opt = Png.load(test_path);
        expect_eq(loaded_opt != null, true);

        const loaded = loaded_opt.?;
        expect_eq(loaded.width as i64, 10);
        expect_eq(loaded.height as i64, 10);

        // Validate cyan diagonal pixel at (5, 5)
        const px = loaded.getPixel(5, 5);
        expect_eq(px.r, 0);
        expect_eq(px.g, 255);
        expect_eq(px.b, 255);
        expect_eq(px.a, 255);

        // Validate blank pixel at (5, 4)
        const px_blank = loaded.getPixel(5, 4);
        expect_eq(px_blank.r, 0);
        expect_eq(px_blank.g, 0);
        expect_eq(px_blank.b, 0);
    });

    test("5. cap:image — Windows BMP DIB Encode & Decode Pipeline", () => {
        const w: u32 = 4;
        const h: u32 = 4;
        const total_len = (w * h * 4) as usize;
        let ptr: [*]u8 = @alloc(total_len);
        let buf = ptr[0..total_len];
        @memset(buf, 0);

        // Set purple pixel at (2, 2)
        let idx = ((2 * w + 2) * 4) as usize;
        buf[idx] = 128; buf[idx + 1] = 0; buf[idx + 2] = 128; buf[idx + 3] = 255;

        // Encode BMP
        const bmp_bytes = Bmp.encodeRgba(buf, w, h);
        expect_eq(bmp_bytes.len, 54 + 4 * 4 * 4); // 54 header + 64 pixels = 118 bytes
        expect_eq(bmp_bytes[0], 0x42); // 'B'
        expect_eq(bmp_bytes[1], 0x4D); // 'M'

        // Decode BMP
        const decoded_bmp_opt = Bmp.decode(bmp_bytes);
        expect_eq(decoded_bmp_opt != null, true);

        const decoded_bmp = decoded_bmp_opt.?;
        expect_eq(decoded_bmp.width as i64, 4);
        expect_eq(decoded_bmp.height as i64, 4);

        const purple = decoded_bmp.getPixel(2, 2);
        expect_eq(purple.r, 128);
        expect_eq(purple.g, 0);
        expect_eq(purple.b, 128);
        expect_eq(purple.a, 255);
    });

    test("6. cap:image — Color Quantization (Palette Extraction)", () => {
        // Construct a 2x2 image with 3 distinct colors
        const w: u32 = 2;
        const h: u32 = 2;
        const total_len = (w * h * 4) as usize;
        let ptr: [*]u8 = @alloc(total_len);
        let buf = ptr[0..total_len];
        // Pixel 0: Red
        buf[0] = 255; buf[1] = 0; buf[2] = 0; buf[3] = 255;
        // Pixel 1: Green
        buf[4] = 0; buf[5] = 255; buf[6] = 0; buf[7] = 255;
        // Pixel 2: Blue
        buf[8] = 0; buf[9] = 0; buf[10] = 255; buf[11] = 255;
        // Pixel 3: Red (duplicate)
        buf[12] = 255; buf[13] = 0; buf[14] = 0; buf[15] = 255;

        const res = Quantizer.quantize(buf, w, h, 16);
        expect_eq(res.colors as i64, 3);
        expect_eq(res.indices.len as i64, 4);
        expect_eq(res.indices[0], res.indices[3]); // Pixel 0 and 3 share same palette index
    });
}
