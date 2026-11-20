// Source: cp/cap/test/test_bitstream_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:bitstream & cap.bitstream

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    BitStreamError,
    BitOrder,
    Uleb128Result,
    Sleb128Result,
    BitWriter,
    BitReader,
    Leb128,
} from "cap:bitstream";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1A. MsbFirst unaligned 1..64-bit mixed writes, peekBits, alignToByte, writeAlignedBytes
    let bw_msb = BitWriter.init(alloc, BitOrder.MsbFirst);
    defer bw_msb.deinit();

    bw_msb.writeBit(1)?;
    bw_msb.writeBits(0b10110, 5)?;
    bw_msb.writeBits(0x123456789ABCDEF0, 64)?;
    bw_msb.writeBits(0b111, 3)?;
    // Total bits so far: 1 + 5 + 64 + 3 = 73 bits (9 full bytes + 1 bit)
    expect_eq(bw_msb.bytes().len, 9 as usize);

    bw_msb.alignToByte()?;
    expect_eq(bw_msb.bytes().len, 10 as usize);

    bw_msb.writeAlignedBytes("CP")?;
    const msb_raw = bw_msb.finish()?;
    defer alloc.free(msb_raw);
    expect_eq(msb_raw.len, 12 as usize);

    let br_msb = BitReader.init(msb_raw, BitOrder.MsbFirst);
    expect_eq(br_msb.bitsRemaining(), 96 as usize);
    expect_eq(br_msb.readBit()?, 1 as u1);
    expect_eq(br_msb.peekBits(5)?, 0b10110 as u64);
    expect_eq(br_msb.readBits(5)?, 0b10110 as u64);
    expect_eq(br_msb.readBits(64)?, 0x123456789ABCDEF0 as u64);
    expect_eq(br_msb.readBits(3)?, 0b111 as u64);
    br_msb.alignToByte();
    expect_eq(br_msb.bitsRemaining(), 16 as usize);
    expect_eq(br_msb.readBits(8)?, 'C' as u64);
    expect_eq(br_msb.readBits(8)?, 'P' as u64);
    expect_eq(br_msb.bitsRemaining(), 0 as usize);

    // 1B. LsbFirst unaligned 1..64-bit mixed writes + skipBits
    let bw_lsb = BitWriter.init(alloc, BitOrder.LsbFirst);
    defer bw_lsb.deinit();

    bw_lsb.writeBits(0b101, 3)?;
    bw_lsb.writeBits(0xCAFEBABE12345678, 64)?;
    bw_lsb.writeBits(0x1F, 5)?;
    // 3 + 64 + 5 = 72 bits = 9 bytes exact
    const lsb_raw = bw_lsb.finish()?;
    defer alloc.free(lsb_raw);
    expect_eq(lsb_raw.len, 9 as usize);

    let br_lsb = BitReader.init(lsb_raw, BitOrder.LsbFirst);
    expect_eq(br_lsb.peekBits(3)?, 0b101 as u64);
    br_lsb.skipBits(3)?;
    expect_eq(br_lsb.readBits(64)?, 0xCAFEBABE12345678 as u64);
    expect_eq(br_lsb.readBits(5)?, 0x1F as u64);
    expect_eq(br_lsb.bitsRemaining(), 0 as usize);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 2A. H.264/HEVC 0-th order Exp-Golomb round-trip (unsigned & signed)
    let bw = BitWriter.init(alloc, BitOrder.MsbFirst);
    defer bw.deinit();

    const u_vals: [7]u32 = [0, 1, 2, 3, 7, 1024, 0xFFFFFFFF];
    const s_vals: [8]i32 = [0, 1, -1, 2, -2, 42, 2147483647, -2147483648];

    let i: usize = 0;
    while (i < u_vals.len) : (i += 1) {
        bw.writeExpGolombU(u_vals[i])?;
    }
    let j: usize = 0;
    while (j < s_vals.len) : (j += 1) {
        bw.writeExpGolombS(s_vals[j])?;
    }

    const encoded = bw.finish()?;
    defer alloc.free(encoded);

    let br = BitReader.init(encoded, BitOrder.MsbFirst);
    i = 0;
    while (i < u_vals.len) : (i += 1) {
        expect_eq(br.readExpGolombU()?, u_vals[i]);
    }
    j = 0;
    while (j < s_vals.len) : (j += 1) {
        expect_eq(br.readExpGolombS()?, s_vals[j]);
    }

    // 2B. Exp-Golomb out-of-range leading zeros (>32 zeros) -> InvalidExpGolomb
    const bad_zeros: [5]u8 = [0, 0, 0, 0, 0];
    let br_bad = BitReader.init(&bad_zeros, BitOrder.MsbFirst);
    let got_invalid_eg = false;
    * = br_bad.readExpGolombU() catch e => err_eg: {
        got_invalid_eg = (e == BitStreamError.InvalidExpGolomb);
        break :err_eg 0;
    };
    expect_eq(got_invalid_eg, true);

    // 2C. 32 leading zeros with non-zero suffix for ue(v) -> > u32::MAX -> InvalidExpGolomb
    // 32 zero bits (4 bytes of 0) + '1' bit + 32 suffix bits with LSB=1
    const overflow_eg: [9]u8 = [0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01];
    let br_ovf = BitReader.init(&overflow_eg, BitOrder.MsbFirst);
    let got_ovf_eg = false;
    * = br_ovf.readExpGolombU() catch e => err_ovf_eg: {
        got_ovf_eg = (e == BitStreamError.InvalidExpGolomb);
        break :err_ovf_eg 0;
    };
    expect_eq(got_ovf_eg, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 3A. ULEB128 round-trip including 0, 127, 128, maxInt(u64)
    const u_cases: [5]u64 = [0, 127, 128, 0x123456789ABCDEF0, 0xFFFFFFFFFFFFFFFF];
    let i: usize = 0;
    while (i < u_cases.len) : (i += 1) {
        const enc = Leb128.encodeUleb128(alloc, u_cases[i])?;
        defer alloc.free(enc);
        const decoded = Leb128.decodeUleb128(enc)?;
        expect_eq(decoded.value, u_cases[i]);
        expect_eq(decoded.bytes_read, enc.len);
    }

    // 3B. SLEB128 round-trip including 0, -1, 63, -64, 64, -65, maxInt(i64), minInt(i64)
    const s_cases: [8]i64 = [
        0,
        -1,
        63,
        -64,
        64,
        -65,
        9223372036854775807,
        -9223372036854775808,
    ];
    let j: usize = 0;
    while (j < s_cases.len) : (j += 1) {
        const enc = Leb128.encodeSleb128(alloc, s_cases[j])?;
        defer alloc.free(enc);
        const decoded = Leb128.decodeSleb128(enc)?;
        expect_eq(decoded.value, s_cases[j]);
        expect_eq(decoded.bytes_read, enc.len);
    }

    // 3C. >10 byte ULEB128 overflow -> IntegerOverflow
    const uleb_11: [11]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01];
    let uleb_ovf = false;
    * = Leb128.decodeUleb128(&uleb_11) catch e => err_uovf: {
        uleb_ovf = (e == BitStreamError.IntegerOverflow);
        break :err_uovf .{ value: 0, bytes_read: 0 };
    };
    expect_eq(uleb_ovf, true);

    // 3D. 10th byte value > 1 in ULEB128 -> IntegerOverflow
    const uleb_10_bad: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02];
    let uleb_10_ovf = false;
    * = Leb128.decodeUleb128(&uleb_10_bad) catch e => err_u10: {
        uleb_10_ovf = (e == BitStreamError.IntegerOverflow);
        break :err_u10 .{ value: 0, bytes_read: 0 };
    };
    expect_eq(uleb_10_ovf, true);

    // 3E. >10 byte SLEB128 overflow -> IntegerOverflow & truncated -> UnexpectedEof
    const sleb_11: [11]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00];
    let sleb_ovf = false;
    * = Leb128.decodeSleb128(&sleb_11) catch e => err_sovf: {
        sleb_ovf = (e == BitStreamError.IntegerOverflow);
        break :err_sovf .{ value: 0, bytes_read: 0 };
    };
    expect_eq(sleb_ovf, true);

    const trunc: [2]u8 = [0x80, 0x80];
    let got_eof = false;
    * = Leb128.decodeUleb128(&trunc) catch e => err_ueof: {
        got_eof = (e == BitStreamError.UnexpectedEof);
        break :err_ueof .{ value: 0, bytes_read: 0 };
    };
    expect_eq(got_eof, true);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 4A. InvalidBitCount (0 and 65) on BitWriter & BitReader
    let bw = BitWriter.init(alloc, BitOrder.MsbFirst);
    defer bw.deinit();

    let bad_w0 = false;
    bw.writeBits(1, 0) catch e => {
        bad_w0 = (e == BitStreamError.InvalidBitCount);
    };
    expect_eq(bad_w0, true);

    let bad_w65 = false;
    bw.writeBits(1, 65) catch e => {
        bad_w65 = (e == BitStreamError.InvalidBitCount);
    };
    expect_eq(bad_w65, true);

    const sample: [2]u8 = [0xAA, 0x55];
    let br = BitReader.init(&sample, BitOrder.MsbFirst);

    let bad_r0 = false;
    * = br.readBits(0) catch e => err_r0: {
        bad_r0 = (e == BitStreamError.InvalidBitCount);
        break :err_r0 0;
    };
    expect_eq(bad_r0, true);

    let bad_r65 = false;
    * = br.peekBits(65) catch e => err_r65: {
        bad_r65 = (e == BitStreamError.InvalidBitCount);
        break :err_r65 0;
    };
    expect_eq(bad_r65, true);

    // 4B. UnexpectedEof on readBits, skipBits, readBit
    let got_eof_read = false;
    * = br.readBits(24) catch e => err_er: {
        got_eof_read = (e == BitStreamError.UnexpectedEof);
        break :err_er 0;
    };
    expect_eq(got_eof_read, true);

    let got_eof_skip = false;
    br.skipBits(17) catch e => {
        got_eof_skip = (e == BitStreamError.UnexpectedEof);
    };
    expect_eq(got_eof_skip, true);

    expect_eq(br.readBits(16)?, 0xAA55 as u64);
    let got_eof_bit = false;
    * = br.readBit() catch e => err_eb: {
        got_eof_bit = (e == BitStreamError.UnexpectedEof);
        break :err_eb 0;
    };
    expect_eq(got_eof_bit, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Dynamic mounting via import cap from "cap"; cap.bitstream
    let bw = cap.bitstream.BitWriter.init(alloc, cap.bitstream.BitOrder.LsbFirst);
    defer bw.deinit();

    bw.writeBits(0b1010, 4)?;
    bw.writeExpGolombS(-5)?;
    bw.alignToByte()?;
    const raw = bw.finish()?;
    defer alloc.free(raw);

    let br = cap.bitstream.BitReader.init(raw, cap.bitstream.BitOrder.LsbFirst);
    expect_eq(br.readBits(4)?, 0b1010 as u64);
    expect_eq(br.readExpGolombS()?, -5 as i32);

    const leb = cap.bitstream.Leb128.encodeUleb128(alloc, 300)?;
    defer alloc.free(leb);
    const decoded = cap.bitstream.Leb128.decodeUleb128(leb)?;
    expect_eq(decoded.value, 300 as u64);
    expect_eq(decoded.bytes_read, 2 as usize);
}

test("cap:bitstream MsbFirst/LsbFirst 1..64-bit unaligned I/O, peekBits, alignToByte & skipBits") {
    runTest1() catch unreachable;
}

test("cap:bitstream H.264/HEVC Exp-Golomb ue(v)/se(v) round-trip & overflow interception") {
    runTest2() catch unreachable;
}

test("cap:bitstream ULEB128/SLEB128 extreme values & >10-byte IntegerOverflow interception") {
    runTest3() catch unreachable;
}

test("cap:bitstream InvalidBitCount & UnexpectedEof boundary interception") {
    runTest4() catch unreachable;
}

test("cap.bitstream dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
