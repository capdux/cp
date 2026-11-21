import std from "std";
import { test, expect } from "cap:test";
import { MAX_LEN, encode, decode, approxAspectRatio, averageRGBA } from "cap:thumbhash";

test("thumbhash: solid color opaque image roundtrip and averageRGBA", () => {
    // 4x4 solid red-ish image (200, 50, 30, 255)
    let rgba: [64]u8 = undefined;
    let i: usize = 0;
    while (i < 64) : (i += 4) {
        rgba[i + 0] = 200;
        rgba[i + 1] = 50;
        rgba[i + 2] = 30;
        rgba[i + 3] = 255;
    }

    let hash_buf: [MAX_LEN]u8 = undefined;
    const hash = encode(hash_buf[0..], 4, 4, rgba[0..]) catch unreachable;
    expect(hash.len >= 5 && hash.len <= MAX_LEN).toBeTrue();

    const avg = averageRGBA(hash) catch unreachable;
    expect(avg[0] >= 185 && avg[0] <= 215).toBeTrue();
    expect(avg[1] >= 35 && avg[1] <= 65).toBeTrue();
    expect(avg[2] >= 15 && avg[2] <= 45).toBeTrue();
    expect(avg[3]).toBe(255);

    const ratio = approxAspectRatio(hash) catch unreachable;
    expect(ratio > 0.9 && ratio < 1.1).toBeTrue();

    let img = decode(std.testing.allocator, hash) catch unreachable;
    expect(img.width).toBe(32);
    expect(img.height).toBe(32);
    expect(img.rgba.len).toBe(32 * 32 * 4);
    // img.~init() frees rgba with zero leaks
});

test("thumbhash: landscape alpha image preserves aspect ratio and transparency", () => {
    // 8x4 semi-transparent blue
    let rgba: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 128) : (i += 4) {
        rgba[i + 0] = 20;
        rgba[i + 1] = 80;
        rgba[i + 2] = 220;
        rgba[i + 3] = 128;
    }

    let hash_buf: [MAX_LEN]u8 = undefined;
    const hash = encode(hash_buf[0..], 8, 4, rgba[0..]) catch unreachable;
    const ratio = approxAspectRatio(hash) catch unreachable;
    expect(ratio > 1.5).toBeTrue();

    const avg = averageRGBA(hash) catch unreachable;
    expect(avg[3] >= 110 && avg[3] <= 145).toBeTrue();

    let img = decode(std.testing.allocator, hash) catch unreachable;
    expect(img.width).toBe(32);
    expect(img.height < 32).toBeTrue();
});

test("thumbhash: invalid dimensions and truncated hashes rejected", () => {
    let hash_buf: [MAX_LEN]u8 = undefined;
    const empty: [0]u8 = [];
    const e1 = encode(hash_buf[0..], 0, 4, empty[0..]) catch null;
    expect(e1 == null).toBeTrue();

    const short3: [3]u8 = [1, 2, 3];
    const r1 = approxAspectRatio(short3[0..]) catch null;
    expect(r1 == null).toBeTrue();
    const d1 = decode(std.testing.allocator, short3[0..]) catch null;
    expect(d1 == null).toBeTrue();
});
