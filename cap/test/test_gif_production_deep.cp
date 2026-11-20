// Source: cp/cap/test/test_gif_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:gif & cap.gif

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { LzwBitOrder, LzwConfig, compress } from "cap:lzw";
import {
    GifError,
    GifColor,
    DisposalMethod,
    GifFrame,
    GifIndexedFrame,
    GifImage,
    encodeIndexed,
    decode,
    decodeWithLimits,
} from "cap:gif";

fn dummyGifImage(): GifImage {
    const empty_pal = std.heap.page_allocator.alloc(GifColor, 0) catch unreachable;
    const empty_frames = std.heap.page_allocator.alloc(GifFrame, 0) catch unreachable;
    return GifImage{
        width: 0,
        height: 0,
        loop_count: 0,
        global_palette: empty_pal,
        frames: empty_frames,
    };
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Single-frame static GIF 2x2 roundtrip
    const pal1: [4]GifColor = [
        GifColor{ r: 255, g: 0, b: 0 },
        GifColor{ r: 0, g: 255, b: 0 },
        GifColor{ r: 0, g: 0, b: 255 },
        GifColor{ r: 255, g: 255, b: 0 },
    ];
    const idx1: [4]u8 = [0, 1, 2, 3];
    const sf: [1]GifIndexedFrame = [
        GifIndexedFrame{
            width: 2,
            height: 2,
            indices: &idx1,
        },
    ];
    const single_bytes = encodeIndexed(alloc, 2, 2, 0, &pal1, &sf)?;
    defer alloc.free(single_bytes);

    let img1 = decode(alloc, single_bytes)?;
    defer img1.deinit(alloc);
    expect_eq(img1.width, 2);
    expect_eq(img1.height, 2);
    expect_eq(img1.frames.len, 1);
    expect_eq(img1.frames[0].pixels_rgba.len, 16);
    // Pixel 0: Red (255, 0, 0, 255)
    expect_eq(img1.frames[0].pixels_rgba[0], 255);
    expect_eq(img1.frames[0].pixels_rgba[1], 0);
    expect_eq(img1.frames[0].pixels_rgba[2], 0);
    expect_eq(img1.frames[0].pixels_rgba[3], 255);
    // Pixel 3: Yellow (255, 255, 0, 255)
    expect_eq(img1.frames[0].pixels_rgba[12], 255);
    expect_eq(img1.frames[0].pixels_rgba[13], 255);
    expect_eq(img1.frames[0].pixels_rgba[14], 0);
    expect_eq(img1.frames[0].pixels_rgba[15], 255);

    // 2. Multi-frame animated GIF with transparent_index, delay_cs, disposal, and loop_count
    const f0_idx: [4]u8 = [0, 1, 1, 2];
    const f1_idx: [4]u8 = [3, 0, 2, 1];
    const af: [2]GifIndexedFrame = [
        GifIndexedFrame{
            width: 2,
            height: 2,
            delay_cs: 15,
            transparent_index: 1,
            disposal: DisposalMethod.RestoreBg,
            indices: &f0_idx,
        },
        GifIndexedFrame{
            width: 2,
            height: 2,
            delay_cs: 25,
            transparent_index: 0,
            disposal: DisposalMethod.Keep,
            indices: &f1_idx,
        },
    ];
    const anim_bytes = encodeIndexed(alloc, 2, 2, 3, &pal1, &af)?;
    defer alloc.free(anim_bytes);

    let anim = decode(alloc, anim_bytes)?;
    defer anim.deinit(alloc);
    expect_eq(anim.loop_count, 3);
    expect_eq(anim.frames.len, 2);
    expect_eq(anim.frames[0].delay_cs, 15);
    expect_eq(anim.frames[0].disposal == DisposalMethod.RestoreBg, true);
    expect_eq(anim.frames[1].delay_cs, 25);
    expect_eq(anim.frames[1].disposal == DisposalMethod.Keep, true);

    // Frame 0 pixel 1 is transparent (index 1 -> RGBA 0,0,0,0)
    expect_eq(anim.frames[0].pixels_rgba[4], 0);
    expect_eq(anim.frames[0].pixels_rgba[5], 0);
    expect_eq(anim.frames[0].pixels_rgba[6], 0);
    expect_eq(anim.frames[0].pixels_rgba[7], 0);
    // Frame 0 pixel 3 is index 2 -> Blue (0,0,255,255)
    expect_eq(anim.frames[0].pixels_rgba[14], 255);
    expect_eq(anim.frames[0].pixels_rgba[15], 255);
    // Frame 1 pixel 0 is transparent (index 0 -> RGBA 0,0,0,0)
    expect_eq(anim.frames[1].pixels_rgba[3], 0);
    // Indices preserved losslessly
    expect_eq(std.mem.eql(u8, anim.frames[0].indices, &f0_idx), true);
    expect_eq(std.mem.eql(u8, anim.frames[1].indices, &f1_idx), true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // Build a 4x9 interlaced GIF89a with Global Color Table (black/white)
    // and a Local Color Table (red/green/blue/yellow) overriding GCT.
    // Interlaced pass order for height=9:
    // Pass 1 (start=0, step=8): rows 0, 8
    // Pass 2 (start=4, step=8): row 4
    // Pass 3 (start=2, step=4): rows 2, 6
    // Pass 4 (start=1, step=2): rows 1, 3, 5, 7
    const w: usize = 4;
    const h: usize = 9;
    let logical_rows: [36]u8 = undefined;
    let y: usize = 0;
    while (y < h) : (y += 1) {
        const val: u8 = (y % 4) as u8;
        let x: usize = 0;
        while (x < w) : (x += 1) {
            logical_rows[y * w + x] = val;
        }
    }

    let interlaced_rows: [36]u8 = undefined;
    let dst_row: usize = 0;
    const starts: [4]usize = [0, 4, 2, 1];
    const steps: [4]usize = [8, 8, 4, 2];
    let pass: usize = 0;
    while (pass < 4) : (pass += 1) {
        let ry: usize = starts[pass];
        while (ry < h) : (ry += steps[pass]) {
            @memcpy(interlaced_rows[(dst_row * w) .. ((dst_row + 1) * w)], logical_rows[(ry * w) .. ((ry + 1) * w)]);
            dst_row += 1;
        }
    }

    const lzw_data = compress(alloc, &interlaced_rows, LzwConfig{
        min_code_size: 2,
        bit_order: LzwBitOrder.LsbFirst,
        max_code_size: 12,
    })?;
    defer alloc.free(lzw_data);

    let raw = std.ArrayList(u8).empty;
    defer raw.deinit(alloc);

    // Header + LSD (4x9, GCT flag=1, size_code=0 -> 2 entries)
    raw.appendSlice(alloc, "GIF89a") catch unreachable;
    raw.appendSlice(alloc, &[4, 0, 9, 0, 0xF0, 0, 0]) catch unreachable;
    // GCT: 2 colors (black, white)
    raw.appendSlice(alloc, &[0, 0, 0, 255, 255, 255]) catch unreachable;
    // Image Descriptor: 0,0, 4x9, packed = 0x80 (LCT) | 0x40 (Interlaced) | 0x01 (4 entries) = 0xC1
    raw.appendSlice(alloc, &[0x2C, 0, 0, 0, 0, 4, 0, 9, 0, 0xC1]) catch unreachable;
    // LCT: 4 colors: Red, Green, Blue, Cyan
    raw.appendSlice(alloc, &[
        250, 10, 10,
        10, 250, 10,
        10, 10, 250,
        10, 250, 250,
    ]) catch unreachable;
    // min_code_size = 2
    raw.append(alloc, 2) catch unreachable;
    raw.append(alloc, lzw_data.len as u8) catch unreachable;
    raw.appendSlice(alloc, lzw_data) catch unreachable;
    raw.append(alloc, 0x00) catch unreachable;
    raw.append(alloc, 0x3B) catch unreachable;

    let img = decode(alloc, raw.items)?;
    defer img.deinit(alloc);
    expect_eq(img.frames.len, 1);
    expect_eq(std.mem.eql(u8, img.frames[0].indices, &logical_rows), true);
    // Verify LCT overrode GCT: row 0 -> index 0 -> Red (250, 10, 10, 255)
    expect_eq(img.frames[0].pixels_rgba[0], 250);
    expect_eq(img.frames[0].pixels_rgba[1], 10);
    // Row 1 -> index 1 -> Green (10, 250, 10, 255)
    expect_eq(img.frames[0].pixels_rgba[16], 10);
    expect_eq(img.frames[0].pixels_rgba[17], 250);
    // Row 3 -> index 3 -> Cyan (10, 250, 250, 255)
    expect_eq(img.frames[0].pixels_rgba[48], 10);
    expect_eq(img.frames[0].pixels_rgba[49], 250);
    expect_eq(img.frames[0].pixels_rgba[50], 250);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // GIF87a signature + Comment Extension (0xFE) + Unknown Application Extension (0xFF)
    const pixels: [4]u8 = [0, 1, 1, 0];
    const lzw_data = compress(alloc, &pixels, LzwConfig{
        min_code_size: 2,
        bit_order: LzwBitOrder.LsbFirst,
        max_code_size: 12,
    })?;
    defer alloc.free(lzw_data);

    let raw = std.ArrayList(u8).empty;
    defer raw.deinit(alloc);

    raw.appendSlice(alloc, "GIF87a") catch unreachable;
    raw.appendSlice(alloc, &[2, 0, 2, 0, 0x80, 0, 0]) catch unreachable;
    // GCT (2 colors: 11,22,33 and 44,55,66)
    raw.appendSlice(alloc, &[11, 22, 33, 44, 55, 66]) catch unreachable;
    // Comment Extension: 0x21 0xFE <len=5> "hello" 0x00
    raw.appendSlice(alloc, &[0x21, 0xFE, 5]) catch unreachable;
    raw.appendSlice(alloc, "hello") catch unreachable;
    raw.append(alloc, 0x00) catch unreachable;
    // Unknown Application Extension: 0x21 0xFF 0x0B "CUSTOMAPP10" <len=3> "xyz" 0x00
    raw.appendSlice(alloc, &[0x21, 0xFF, 11]) catch unreachable;
    raw.appendSlice(alloc, "CUSTOMAPP10") catch unreachable;
    raw.appendSlice(alloc, &[3, 'x', 'y', 'z', 0x00]) catch unreachable;
    // Image Descriptor
    raw.appendSlice(alloc, &[0x2C, 0, 0, 0, 0, 2, 0, 2, 0, 0x00]) catch unreachable;
    raw.append(alloc, 2) catch unreachable;
    raw.append(alloc, lzw_data.len as u8) catch unreachable;
    raw.appendSlice(alloc, lzw_data) catch unreachable;
    raw.append(alloc, 0x00) catch unreachable;
    raw.append(alloc, 0x3B) catch unreachable;

    let img = decode(alloc, raw.items)?;
    defer img.deinit(alloc);
    expect_eq(img.frames.len, 1);
    expect_eq(img.frames[0].pixels_rgba[0], 11);
    expect_eq(img.frames[0].pixels_rgba[1], 22);
    expect_eq(img.frames[0].pixels_rgba[2], 33);
    expect_eq(img.frames[0].pixels_rgba[4], 44);
    expect_eq(img.frames[0].pixels_rgba[5], 55);
    expect_eq(img.frames[0].pixels_rgba[6], 66);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. InvalidSignature
    let got_bad_sig = false;
    * = decode(alloc, "PNG89a1234567") catch e => errSig: {
        got_bad_sig = (e == GifError.InvalidSignature);
        break :errSig dummyGifImage();
    };
    expect_eq(got_bad_sig, true);

    // Build a valid 2-frame 4x4 GIF for limit & truncation tests
    const pal: [2]GifColor = [
        GifColor{ r: 10, g: 20, b: 30 },
        GifColor{ r: 40, g: 50, b: 60 },
    ];
    const idx: [16]u8 = [0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0];
    const frames: [2]GifIndexedFrame = [
        GifIndexedFrame{ width: 4, height: 4, delay_cs: 5, indices: &idx },
        GifIndexedFrame{ width: 4, height: 4, delay_cs: 5, indices: &idx },
    ];
    const valid = encodeIndexed(alloc, 4, 4, 0, &pal, &frames)?;
    defer alloc.free(valid);

    // 2. TruncatedData (chopped before trailer / inside sub-block)
    let got_trunc = false;
    * = decode(alloc, valid[0 .. (valid.len - 2)]) catch e => errTrunc: {
        got_trunc = (e == GifError.TruncatedData);
        break :errTrunc dummyGifImage();
    };
    expect_eq(got_trunc, true);

    // 3. ImageTooLarge (max_w / max_h exceeded)
    let got_too_large = false;
    * = decodeWithLimits(alloc, valid, 2, 4, 10) catch e => errLarge: {
        got_too_large = (e == GifError.ImageTooLarge);
        break :errLarge dummyGifImage();
    };
    expect_eq(got_too_large, true);

    // 4. TooManyFrames (max_frames = 1 on 2-frame GIF)
    let got_too_many = false;
    * = decodeWithLimits(alloc, valid, 100, 100, 1) catch e => errFrames: {
        got_too_many = (e == GifError.TooManyFrames);
        break :errFrames dummyGifImage();
    };
    expect_eq(got_too_many, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;
    const pal: [2]cap.gif.GifColor = [
        cap.gif.GifColor{ r: 12, g: 34, b: 56 },
        cap.gif.GifColor{ r: 78, g: 90, b: 123 },
    ];
    const idx: [4]u8 = [0, 1, 0, 1];
    const frames: [1]cap.gif.GifIndexedFrame = [
        cap.gif.GifIndexedFrame{
            width: 2,
            height: 2,
            delay_cs: 10,
            disposal: cap.gif.DisposalMethod.Keep,
            indices: &idx,
        },
    ];
    const encoded = cap.gif.encodeIndexed(alloc, 2, 2, 2, &pal, &frames)?;
    defer alloc.free(encoded);

    let img = cap.gif.decodeWithLimits(alloc, encoded, 64, 64, 8)?;
    defer img.deinit(alloc);
    expect_eq(img.loop_count, 2);
    expect_eq(img.frames[0].pixels_rgba[0], 12);
    expect_eq(img.frames[0].pixels_rgba[4], 78);
}

test("1. Static & animated GIF encodeIndexed/decode RGBA & index roundtrip with transparency and delay_cs", () => {
    runTest1() catch unreachable;
});

test("2. Interlaced 4-pass GIF decoding and local palette overriding global palette", () => {
    runTest2() catch unreachable;
});

test("3. GIF87a signature and safe skipping of Comment & unknown Application extensions", () => {
    runTest3() catch unreachable;
});

test("4. InvalidSignature, TruncatedData, ImageTooLarge, and TooManyFrames protection", () => {
    runTest4() catch unreachable;
});

test("5. cap.gif dynamic mount and std.testing.allocator zero-leak verification", () => {
    runTest5() catch unreachable;
});
