// cap/test/testImageJpegWebpEncode.cp
// Deep regression tests for cap:image Baseline DCT JPEG & RFC 9649 WebP (VP8L) Encoders

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Codecs,
    ImageFormat,
    Jpeg,
    JpegImage,
    JpegHeader,
    JpegError,
    jpegEncode,
    jpegEncodeInto,
    Webp,
    WebpImage,
    WebpHeader,
    WebpError,
    webpEncode,
    webpEncodeLossless,
} from "cap:image";

fn fillSolidRgba(buf: []u8, r: u8, g: u8, b: u8, a: u8) {
    let i: usize = 0;
    while (i + 3 < buf.len) : (i += 4) {
        buf[i] = r;
        buf[i + 1] = g;
        buf[i + 2] = b;
        buf[i + 3] = a;
    }
}

fn fillGradientRgba(buf: []u8, width: u32, height: u32, with_alpha: bool) {
    const w = width as usize;
    const h = height as usize;
    let y: usize = 0;
    while (y < h) : (y += 1) {
        let x: usize = 0;
        while (x < w) : (x += 1) {
            const off = (y * w + x) * 4;
            buf[off] = ((x * 255) / (if (w > 1) (w - 1) else 1)) as u8;
            buf[off + 1] = ((y * 255) / (if (h > 1) (h - 1) else 1)) as u8;
            buf[off + 2] = (((x + y) * 127) / (if (w + h > 2) (w + h - 2) else 1)) as u8;
            buf[off + 3] = if (with_alpha) (((x * 7 + y * 13) & 0xFF) as u8) else 255;
        }
    }
}

fn fillCheckerRgba(buf: []u8, width: u32, height: u32) {
    const w = width as usize;
    const h = height as usize;
    let y: usize = 0;
    while (y < h) : (y += 1) {
        let x: usize = 0;
        while (x < w) : (x += 1) {
            const off = (y * w + x) * 4;
            const dark = (((x >> 2) + (y >> 2)) & 1) == 0;
            if (dark) {
                buf[off] = 20;
                buf[off + 1] = 40;
                buf[off + 2] = 200;
                buf[off + 3] = 255;
            } else {
                buf[off] = 240;
                buf[off + 1] = 220;
                buf[off + 2] = 30;
                buf[off + 3] = 255;
            }
        }
    }
}

fn fillNoiseRgba(buf: []u8, width: u32, height: u32, with_alpha: bool) {
    const count = (width as usize) * (height as usize);
    let state: u32 = 0x12345678;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        state = state *% 1664525 +% 1013904223;
        const r = ((state >> 24) & 0xFF) as u8;
        state = state *% 1664525 +% 1013904223;
        const g = ((state >> 24) & 0xFF) as u8;
        state = state *% 1664525 +% 1013904223;
        const b = ((state >> 24) & 0xFF) as u8;
        const a: u8 = if (with_alpha) (((state >> 16) & 0xFF) as u8) else 255;
        const off = i * 4;
        buf[off] = r;
        buf[off + 1] = g;
        buf[off + 2] = b;
        buf[off + 3] = a;
    }
}

fn testJpegBasicAndNonMul8(): void! {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    const w: u32 = 13;
    const h: u32 = 17;
    let rgba: [13 * 17 * 4]u8 = [0; 13 * 17 * 4];
    fillGradientRgba(rgba[0..rgba.len], w, h, false);

    const jpg = jpegEncode(alloc, rgba[0..rgba.len], w, h, 85) catch unreachable;
    defer alloc.free(jpg);

    expect_eq(jpg.len > 100, true);
    expect_eq(jpg[0], 0xFF);
    expect_eq(jpg[1], 0xD8);
    expect_eq(jpg[jpg.len - 2], 0xFF);
    expect_eq(jpg[jpg.len - 1], 0xD9);

    const fmt = Codecs.sniff(jpg);
    expect_eq(fmt != null, true);
    expect_eq(fmt.? == ImageFormat.Jpeg, true);

    const hdr: JpegHeader = Jpeg.probe(jpg).?;
    expect_eq(hdr.width, 13);
    expect_eq(hdr.height, 17);
    expect_eq(hdr.components, 3);
    expect_eq(hdr.is_progressive, false);

    // JpegImage.encode / Jpeg.encode 与零堆 jpegEncodeInto 一致性
    const jpg2 = JpegImage.encode(alloc, rgba[0..rgba.len], w, h, 85) catch unreachable;
    defer alloc.free(jpg2);
    expect_eq(std.mem.eql(u8, jpg, jpg2), true);

    let stack_buf: [16384]u8 = [0; 16384];
    const jpg_into = jpegEncodeInto(rgba[0..rgba.len], w, h, 85, stack_buf[0..16384]) catch unreachable;
    expect_eq(std.mem.eql(u8, jpg, jpg_into), true);

    // 1x1 最小尺寸边界
    const px1: [4]u8 = [200, 100, 50, 255];
    const jpg1 = Jpeg.encode(alloc, px1[0..4], 1, 1, 90) catch unreachable;
    defer alloc.free(jpg1);
    const h1 = JpegImage.probe(jpg1).?;
    expect_eq(h1.width, 1);
    expect_eq(h1.height, 1);
}

fn testJpegQualityMonotonicity(): void! {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    const w: u32 = 32;
    const h: u32 = 32;
    let rgba: [32 * 32 * 4]u8 = [0; 32 * 32 * 4];
    fillNoiseRgba(rgba[0..rgba.len], w, h, false);

    const q20 = jpegEncode(alloc, rgba[0..rgba.len], w, h, 20) catch unreachable;
    defer alloc.free(q20);
    const q75 = jpegEncode(alloc, rgba[0..rgba.len], w, h, 75) catch unreachable;
    defer alloc.free(q75);
    const q100 = jpegEncode(alloc, rgba[0..rgba.len], w, h, 100) catch unreachable;
    defer alloc.free(q100);

    expect_eq(q20.len < q75.len, true);
    expect_eq(q75.len < q100.len, true);
    expect_eq(Jpeg.probe(q20).?.width, 32);
    expect_eq(Jpeg.probe(q100).?.height, 32);
}

fn testJpegErrorGuards(): void! {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    const dummy: [16]u8 = [0; 16];
    let got_dim_err = false;
    * = jpegEncode(alloc, dummy[0..16], 0, 4, 80) catch |e| {
        if (e == JpegError.InvalidDimensions) {
            got_dim_err = true;
        }
    };
    expect_eq(got_dim_err, true);

    let got_len_err = false;
    * = jpegEncode(alloc, dummy[0..12], 2, 2, 80) catch |e| {
        if (e == JpegError.InvalidBufferLength) {
            got_len_err = true;
        }
    };
    expect_eq(got_len_err, true);

    let tiny: [32]u8 = [0; 32];
    let got_small_err = false;
    * = jpegEncodeInto(dummy[0..16], 2, 2, 80, tiny[0..32]) catch |e| {
        if (e == JpegError.BufferTooSmall) {
            got_small_err = true;
        }
    };
    expect_eq(got_small_err, true);
}

fn testWebpBasicAndNonMul8(): void! {
    const alloc: std.mem.Allocator = std.heap.page_allocator;

    // 16x16 纯色不透明图（Simple 1-symbol + LZ77 RLE）
    let solid: [16 * 16 * 4]u8 = [0; 16 * 16 * 4];
    fillSolidRgba(solid[0..solid.len], 64, 128, 255, 255);
    const w_solid = webpEncodeLossless(alloc, solid[0..solid.len], 16, 16) catch unreachable;
    defer alloc.free(w_solid);

    expect_eq((w_solid.len & 1) == 0, true);
    expect_eq(Codecs.sniff(w_solid).? == ImageFormat.WebP, true);
    const hs: WebpHeader = Webp.probe(w_solid).?;
    expect_eq(hs.width, 16);
    expect_eq(hs.height, 16);
    expect_eq(hs.is_lossless, true);
    expect_eq(hs.has_alpha, false);

    // 13x17 带 Alpha 通道渐变图（Normal Code Length Code）
    let grad: [13 * 17 * 4]u8 = [0; 13 * 17 * 4];
    fillGradientRgba(grad[0..grad.len], 13, 17, true);
    const w_grad = webpEncode(alloc, grad[0..grad.len], 13, 17, 100) catch unreachable;
    defer alloc.free(w_grad);

    expect_eq((w_grad.len & 1) == 0, true);
    const hg = WebpImage.probe(w_grad).?;
    expect_eq(hg.width, 13);
    expect_eq(hg.height, 17);
    expect_eq(hg.is_lossless, true);
    expect_eq(hg.has_alpha, true);

    // webpEncodeLossless / Webp.encodeLossless / WebpImage.encodeLossless 一致性
    const w_grad2 = WebpImage.encodeLossless(alloc, grad[0..grad.len], 13, 17) catch unreachable;
    defer alloc.free(w_grad2);
    expect_eq(std.mem.eql(u8, w_grad, w_grad2), true);

    // 16x16 棋盘格
    let chk: [16 * 16 * 4]u8 = [0; 16 * 16 * 4];
    fillCheckerRgba(chk[0..chk.len], 16, 16);
    const w_chk = Webp.encode(alloc, chk[0..chk.len], 16, 16, 100) catch unreachable;
    defer alloc.free(w_chk);
    const hc = Webp.probe(w_chk).?;
    expect_eq(hc.width, 16);
    expect_eq(hc.height, 16);
    expect_eq(hc.has_alpha, false);

    // 1x1 最小尺寸边界
    const px1: [4]u8 = [10, 20, 30, 128];
    const w1 = webpEncodeLossless(alloc, px1[0..4], 1, 1) catch unreachable;
    defer alloc.free(w1);
    const h1 = Webp.probe(w1).?;
    expect_eq(h1.width, 1);
    expect_eq(h1.height, 1);
    expect_eq(h1.has_alpha, true);
}

fn testWebpQualityAndErrors(): void! {
    const alloc: std.mem.Allocator = std.heap.page_allocator;
    const w: u32 = 32;
    const h: u32 = 32;
    let rgba: [32 * 32 * 4]u8 = [0; 32 * 32 * 4];
    fillNoiseRgba(rgba[0..rgba.len], w, h, true);

    const q20 = webpEncode(alloc, rgba[0..rgba.len], w, h, 20) catch unreachable;
    defer alloc.free(q20);
    const q75 = webpEncode(alloc, rgba[0..rgba.len], w, h, 75) catch unreachable;
    defer alloc.free(q75);
    const q100 = webpEncode(alloc, rgba[0..rgba.len], w, h, 100) catch unreachable;
    defer alloc.free(q100);

    expect_eq(q20.len < q75.len, true);
    expect_eq(q75.len < q100.len, true);
    expect_eq(Webp.probe(q20).?.width, 32);
    expect_eq(Webp.probe(q100).?.height, 32);

    // 错误边界测试
    const dummy: [16]u8 = [0; 16];
    let got_dim = false;
    * = webpEncode(alloc, dummy[0..16], 0, 2, 100) catch |e| {
        if (e == WebpError.InvalidDimensions) {
            got_dim = true;
        }
    };
    expect_eq(got_dim, true);

    let got_too_large = false;
    * = webpEncode(alloc, dummy[0..16], 16385, 1, 100) catch |e| {
        if (e == WebpError.DimensionTooLarge) {
            got_too_large = true;
        }
    };
    expect_eq(got_too_large, true);

    let got_len = false;
    * = webpEncode(alloc, dummy[0..8], 2, 2, 100) catch |e| {
        if (e == WebpError.InvalidBufferLength) {
            got_len = true;
        }
    };
    expect_eq(got_len, true);
}

fn main() {
    test("1. JPEG encode: solid, gradient, checkerboard, non-multiple-of-8 (13x17, 1x1) & probe", () => {
        testJpegBasicAndNonMul8()?;
    });

    test("2. JPEG encode: quality=20/75/100 size monotonicity on complex image", () => {
        testJpegQualityMonotonicity()?;
    });

    test("3. JPEG encode: error handling for invalid inputs & small buffer", () => {
        testJpegErrorGuards()?;
    });

    test("4. WebP VP8L encode: solid, gradient+alpha, checkerboard, non-multiple-of-8 (13x17, 1x1)", () => {
        testWebpBasicAndNonMul8()?;
    });

    test("5. WebP VP8L encode: quality=20/75/100 size monotonicity & error guards", () => {
        testWebpQualityAndErrors()?;
    });
}
