// Source: cp/cap/test/test_dds_production_deep.cp (Production deep tests for cap:dds)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DdsPixelFormat,
    DdsDx10Header,
    DdsHeader,
    isDds,
    parseDdsHeader,
    calcDdsMipSize,
} from "cap:dds";

fn writeU32Le(buf: []u8, pos: usize, v: u32) {
    buf[pos] = u8(v & 0xFF);
    buf[pos + 1] = u8((v >> 8) & 0xFF);
    buf[pos + 2] = u8((v >> 16) & 0xFF);
    buf[pos + 3] = u8((v >> 24) & 0xFF);
}

fn initBaseDds(buf: []u8, w: u32, h: u32, mips: u32, pf_flags: u32, fourcc: []const u8) {
    buf[0] = 'D';
    buf[1] = 'D';
    buf[2] = 'S';
    buf[3] = ' ';
    writeU32Le(buf, 4, 124);        // dwSize
    writeU32Le(buf, 8, 0x1007);     // flags
    writeU32Le(buf, 12, h);         // height
    writeU32Le(buf, 16, w);         // width
    writeU32Le(buf, 28, mips);      // mip_map_count
    writeU32Le(buf, 76, 32);        // ddspf.dwSize
    writeU32Le(buf, 80, pf_flags);  // ddspf.dwFlags
    if (fourcc.len == 4) {
        buf[84] = fourcc[0];
        buf[85] = fourcc[1];
        buf[86] = fourcc[2];
        buf[87] = fourcc[3];
    }
}

test("1. isDds and parseDdsHeader decode standard 128-byte DXT5 texture header", () => {
    let buf: [160]u8 = std.mem.zeroes([160]u8);
    initBaseDds(buf[0..], 512, 256, 10, 0x4, "DXT5");
    writeU32Le(buf[0..], 20, 131072); // pitch_or_linear_size

    expect_eq(isDds(buf[0..]), true);
    const hdr = parseDdsHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.width, 512);
    expect_eq(hdr.height, 256);
    expect_eq(hdr.mip_map_count, 10);
    expect_eq(hdr.pitch_or_linear_size, 131072);
    expect_eq(hdr.pixel_format.isFourcc(), true);
    expect_eq(hdr.pixel_format.isDx10(), false);
    expect_eq(std.mem.eql(u8, hdr.pixel_format.fourcc[0..], "DXT5"), true);
    expect_eq(hdr.dx10 == null, true);
    expect_eq(hdr.data_offset, 128);
});

test("2. parseDdsHeader decodes 148-byte DX10 extended header for BC7 texture arrays", () => {
    let buf: [164]u8 = std.mem.zeroes([164]u8);
    initBaseDds(buf[0..], 1024, 1024, 11, 0x4, "DX10");
    writeU32Le(buf[0..], 128, 98);  // DXGI_FORMAT_BC7_UNORM = 98
    writeU32Le(buf[0..], 132, 3);   // D3D10_RESOURCE_DIMENSION_TEXTURE2D = 3
    writeU32Le(buf[0..], 136, 0);   // miscFlag
    writeU32Le(buf[0..], 140, 6);   // arraySize = 6
    writeU32Le(buf[0..], 144, 1);   // miscFlags2

    const hdr = parseDdsHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.pixel_format.isDx10(), true);
    expect_eq(hdr.data_offset, 148);

    const ext = hdr.dx10 ?? unreachable;
    expect_eq(ext.dxgi_format, 98);
    expect_eq(ext.resource_dimension, 3);
    expect_eq(ext.array_size, 6);
    expect_eq(ext.misc_flags2, 1);
});

test("3. Cubemap, 3D volume flags, and uncompressed RGBA bitmask pixel format", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    initBaseDds(buf[0..], 64, 64, 1, 0x41, ""); // DDPF_RGB | DDPF_ALPHAPIXELS
    writeU32Le(buf[0..], 24, 16);             // depth = 16
    writeU32Le(buf[0..], 88, 32);             // rgb_bit_count = 32
    writeU32Le(buf[0..], 92, 0x00FF0000);     // r_mask
    writeU32Le(buf[0..], 96, 0x0000FF00);     // g_mask
    writeU32Le(buf[0..], 100, 0x000000FF);    // b_mask
    writeU32Le(buf[0..], 104, 0xFF000000);    // a_mask
    writeU32Le(buf[0..], 112, 0x200 | 0x200000); // cubemap | volume

    const hdr = parseDdsHeader(buf[0..]) ?? unreachable;
    expect_eq(hdr.pixel_format.isFourcc(), false);
    expect_eq(hdr.pixel_format.isDx10(), false);
    expect_eq(hdr.pixel_format.rgb_bit_count, 32);
    expect_eq(hdr.pixel_format.r_mask, 0x00FF0000);
    expect_eq(hdr.pixel_format.a_mask, 0xFF000000);
    expect_eq(hdr.depth, 16);
    expect_eq(hdr.isCubemap(), true);
    expect_eq(hdr.isVolume(), true);
});

test("4. calcDdsMipSize handles BC1(8B), BC7(16B), non-multiple-of-4, 1x1 clamp, and large 64-bit sizes", () => {
    // 1024x1024 BC1 (8B): 256*256*8 = 524288
    expect_eq(calcDdsMipSize(1024, 1024, 8), 524288);
    // 512x256 BC7 (16B): 128*64*16 = 131072
    expect_eq(calcDdsMipSize(512, 256, 16), 131072);
    // Non-multiple of 4: 5x3 -> ceil(5/4)=2, ceil(3/4)=1 -> 2*1*16 = 32
    expect_eq(calcDdsMipSize(5, 3, 16), 32);
    // Small mips 2x2, 1x1, 0x0 clamp to 1 block
    expect_eq(calcDdsMipSize(2, 2, 8), 8);
    expect_eq(calcDdsMipSize(1, 1, 16), 16);
    expect_eq(calcDdsMipSize(0, 0, 16), 16);
    // Zero block bytes -> 0
    expect_eq(calcDdsMipSize(256, 256, 0), 0);
    // Large 65536x65536 BC7 (16B): 16384*16384*16 = 4294967296 (> u32::MAX, no 32-bit overflow)
    expect_eq(calcDdsMipSize(65536, 65536, 16), 4294967296);
});

test("5. Malformed and truncated DDS header protection", () => {
    expect_eq(isDds("PNG "), false);
    expect_eq(parseDdsHeader("") == null, true);

    // Truncated < 128 bytes
    let short_buf: [127]u8 = std.mem.zeroes([127]u8);
    short_buf[0] = 'D'; short_buf[1] = 'D'; short_buf[2] = 'S'; short_buf[3] = ' ';
    expect_eq(parseDdsHeader(short_buf[0..]) == null, true);

    // Invalid dwSize != 124
    let bad_sz: [128]u8 = std.mem.zeroes([128]u8);
    initBaseDds(bad_sz[0..], 64, 64, 1, 0x4, "DXT1");
    writeU32Le(bad_sz[0..], 4, 120);
    expect_eq(parseDdsHeader(bad_sz[0..]) == null, true);

    // Invalid ddspf.dwSize != 32
    let bad_pf: [128]u8 = std.mem.zeroes([128]u8);
    initBaseDds(bad_pf[0..], 64, 64, 1, 0x4, "DXT1");
    writeU32Le(bad_pf[0..], 76, 28);
    expect_eq(parseDdsHeader(bad_pf[0..]) == null, true);

    // DX10 FourCC declared but buffer is only 140 bytes (< 148)
    let trunc_dx10: [140]u8 = std.mem.zeroes([140]u8);
    initBaseDds(trunc_dx10[0..], 64, 64, 1, 0x4, "DX10");
    expect_eq(parseDdsHeader(trunc_dx10[0..]) == null, true);
});
