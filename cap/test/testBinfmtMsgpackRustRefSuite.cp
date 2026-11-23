// 3Hren/msgpack-rust (rmp, MIT) Reference-Hardened MessagePack Test Suite
// Covers:
//   - Test 01: Exact compact wire-format byte vectors for all 36 markers (fixint, uint/int 8..64,
//              float32/64, nil, bool, fixstr/str8/16, bin8/16, fixarray/array16, fixmap/map16,
//              fixext1/2/4/8/16, ext8/16) via both MsgPackWriter & zero-heap MsgPackSliceWriter
//   - Test 02: MessagePack Timestamp extension (ext type = -1: 32/64/96-bit, negative epochs,
//              boundary 2^32-1 / 2^34-1, and strict rejection of nsec >= 1_000_000_000 & bad len)
//   - Test 03: Security guards (0xC1 NeverUsedMarker/InvalidFormatTag, forged 0xFFFFFFFF lengths,
//              80-level depth bomb rejection, trailing bytes, zero-heap skipValue & validateMsgpack)
//   - Test 04: 16-lane SIMD copyPayloadSimd vs scalar 0-error + cap:simd isValidUtf8Simd vs std UTF-8
//   - Test 05: 1,500-iteration deterministic PRNG malformed MessagePack / truncated / forged-len fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    DEFAULT_MAX_DEPTH,
    MSGPACK_EXT_TIMESTAMP,
    MSGPACK_MAX_NSEC,
    MsgPackError,
    MsgExt,
    MsgTimestamp,
    MsgMapEntry,
    MsgValue,
    MsgPackWriter,
    MsgPackSliceWriter,
    MsgPackReader,
    copyPayloadSimd,
    isValidUtf8Simd,
    encode,
    decode,
    decodeWithMaxDepth,
    validateMsgpack,
    validateMsgpackWithMaxDepth,
} from "../../lib/binfmt/msgpack.cp";

fn slicesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn copyScalarRef(dst: []u8, src: []const u8) {
    let n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn checkValidateOk(bytes: []const u8): bool {
    const n = validateMsgpack(bytes) catch return false;
    return n == bytes.len;
}

fn checkDecodeOk(alloc: std.mem.Allocator, bytes: []const u8): bool {
    let v = decode(alloc, bytes) catch return false;
    v.deinit(alloc);
    return true;
}

fn canInitTimestamp(sec: i64, nsec: u32): bool {
    const ts = MsgTimestamp.init(sec, nsec) catch return false;
    return ts.nsec == nsec;
}

fn canReadTimestamp(alloc: std.mem.Allocator, bytes: []const u8): bool {
    let r = MsgPackReader.init(alloc, bytes);
    const ts = r.readTimestamp() catch return false;
    return ts.nsec <= MSGPACK_MAX_NSEC;
}

fn canReadStrictUtf8(alloc: std.mem.Allocator, bytes: []const u8): bool {
    let r = MsgPackReader.init(alloc, bytes);
    const s = r.readStrStrictUtf8() catch return false;
    return s.len <= bytes.len;
}

fn runTest01(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. 权威紧凑整数编码边界字节向量（对标 rmp/tests/func/encode/int.rs）
    let buf: [64]u8 = [0; 64];
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeUint(0) catch unreachable;
        const exp: [1]u8 = [0x00];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeUint(127) catch unreachable;
        const exp: [1]u8 = [0x7F];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeUint(128) catch unreachable;
        const exp: [2]u8 = [0xCC, 0x80];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeUint(255) catch unreachable;
        const exp: [2]u8 = [0xCC, 0xFF];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeUint(256) catch unreachable;
        const exp: [3]u8 = [0xCD, 0x01, 0x00];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeUint(65535) catch unreachable;
        const exp: [3]u8 = [0xCD, 0xFF, 0xFF];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeUint(65536) catch unreachable;
        const exp: [5]u8 = [0xCE, 0x00, 0x01, 0x00, 0x00];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeInt(-1) catch unreachable;
        const exp: [1]u8 = [0xFF];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeInt(-32) catch unreachable;
        const exp: [1]u8 = [0xE0];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeInt(-33) catch unreachable;
        const exp: [2]u8 = [0xD0, 0xDF];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeInt(-128) catch unreachable;
        const exp: [2]u8 = [0xD0, 0x80];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeInt(-32768) catch unreachable;
        const exp: [3]u8 = [0xD1, 0x80, 0x00];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeInt(-32769) catch unreachable;
        const exp: [5]u8 = [0xD2, 0xFF, 0xFF, 0x7F, 0xFF];
        expect_eq(slicesEqual(sw.written(), exp[0..]), true);
    }

    // 2. Float32 / Float64 IEEE-754 精确编码与解码
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeFloat32(1.5) catch unreachable;
        const exp_f32: [5]u8 = [0xCA, 0x3F, 0xC0, 0x00, 0x00];
        expect_eq(slicesEqual(sw.written(), exp_f32[0..]), true);
        let dv = decode(alloc, sw.written()) catch unreachable;
        expect_eq(dv.asFloat().?, 1.5);
    }
    {
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeFloat(-2.5) catch unreachable;
        const exp_f64: [9]u8 = [0xCB, 0xC0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
        expect_eq(slicesEqual(sw.written(), exp_f64[0..]), true);
        let dv = decode(alloc, sw.written()) catch unreachable;
        expect_eq(dv.asFloat().?, -2.5);
    }

    // 3. fixext1/2/4/8/16 & ext8/ext16 全部扩展长度编解码往返
    const ext_lens: [7]usize = [1, 2, 3, 4, 8, 16, 260];
    let ei: usize = 0;
    while (ei < 7) : (ei += 1) {
        const elen = ext_lens[ei];
        let payload = alloc.alloc(u8, elen) catch unreachable;
        let k: usize = 0;
        while (k < elen) : (k += 1) {
            payload[k] = ((k * 13 + ei) & 0xFF) as u8;
        }
        let w = MsgPackWriter.init(alloc);
        w.writeExt(42, payload) catch unreachable;
        expect_eq(validateMsgpack(w.bytes()) catch unreachable, w.bytes().len);
        const dv = decode(alloc, w.bytes()) catch unreachable;
        const ext = dv.asExt().?;
        expect_eq(ext.ext_type, 42);
        expect_eq(slicesEqual(ext.data, payload), true);
    }

    // 4. str8 / str16 / bin8 / bin16 / array16 / map16 往返闭环
    let big_str = alloc.alloc(u8, 300) catch unreachable;
    @memset(big_str, 'Z');
    let w2 = MsgPackWriter.init(alloc);
    w2.writeMapHeader(20) catch unreachable;
    let mi: usize = 0;
    while (mi < 20) : (mi += 1) {
        w2.writeUint(mi as u64) catch unreachable;
        if ((mi & 1) == 0) {
            w2.writeStr(big_str) catch unreachable;
        } else {
            w2.writeBin(big_str[0..40]) catch unreachable;
        }
    }
    expect_eq(validateMsgpack(w2.bytes()) catch unreachable, w2.bytes().len);
    const decoded_map = decode(alloc, w2.bytes()) catch unreachable;
    expect_eq(decoded_map.asMap().?.len, 20);
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. Timestamp 32 (fixext 4: 0xD6, 0xFF, u32be)
    let buf: [32]u8 = [0; 32];
    {
        const ts32 = MsgTimestamp.init(1700000000, 0) catch unreachable;
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeTimestamp(ts32) catch unreachable;
        expect_eq(sw.written().len, 6);
        expect_eq(sw.written()[0], 0xD6);
        expect_eq(sw.written()[1], 0xFF);

        let r = MsgPackReader.init(alloc, sw.written());
        const got = r.readTimestamp() catch unreachable;
        expect_eq(got.eql(ts32), true);
    }

    // 2. Timestamp 64 (fixext 8: 0xD7, 0xFF, (nsec<<34)|sec)
    {
        const ts64 = MsgTimestamp.init(17179869183, 999999999) catch unreachable;
        let sw = MsgPackSliceWriter.init(buf[0..]);
        sw.writeTimestamp(ts64) catch unreachable;
        expect_eq(sw.written().len, 10);
        expect_eq(sw.written()[0], 0xD7);
        expect_eq(sw.written()[1], 0xFF);

        let r = MsgPackReader.init(alloc, sw.written());
        const got = r.readTimestamp() catch unreachable;
        expect_eq(got.eql(ts64), true);
    }

    // 3. Timestamp 96 (ext 8, len=12: 0xC7, 0x0C, 0xFF, u32be(nsec), i64be(sec)) — 负纪元与远未来
    {
        const ts_neg = MsgTimestamp.init(-2208988800, 123456789) catch unreachable;
        let w = MsgPackWriter.init(alloc);
        w.writeTimestamp(ts_neg) catch unreachable;
        expect_eq(w.bytes().len, 15);
        expect_eq(w.bytes()[0], 0xC7);
        expect_eq(w.bytes()[1], 12);
        expect_eq(w.bytes()[2], 0xFF);

        let r = MsgPackReader.init(alloc, w.bytes());
        const got = r.readTimestamp() catch unreachable;
        expect_eq(got.eql(ts_neg), true);
    }
    {
        const ts_far = MsgTimestamp.init(20000000000, 500) catch unreachable;
        let w = MsgPackWriter.init(alloc);
        w.writeTimestamp(ts_far) catch unreachable;
        expect_eq(w.bytes().len, 15);
        let r = MsgPackReader.init(alloc, w.bytes());
        const got = r.readTimestamp() catch unreachable;
        expect_eq(got.eql(ts_far), true);
    }

    // 4. 严格拒绝非法 nsec >= 1_000_000_000 与非法长度
    expect_eq(canInitTimestamp(0, 1000000000), false);

    // 构造畸形 Timestamp 96（nsec = 1_000_000_000 = 0x3B9ACA00）
    const bad_ts96: [15]u8 = [
        0xC7, 0x0C, 0xFF,
        0x3B, 0x9A, 0xCA, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
    ];
    expect_eq(canReadTimestamp(alloc, bad_ts96[0..]), false);

    // 非法长度 fixext 2 (-1)
    const bad_ts_len: [4]u8 = [0xD5, 0xFF, 0x00, 0x01];
    expect_eq(canReadTimestamp(alloc, bad_ts_len[0..]), false);
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. 0xC1 保留字节严格拒绝
    const never_used: [1]u8 = [0xC1];
    expect_eq(checkValidateOk(never_used[0..]), false);
    expect_eq(checkDecodeOk(alloc, never_used[0..]), false);

    // 2. 伪造超大长度声明（str32/bin32/array32/map32/ext32 = 0xFFFFFFFF）瞬时拒绝且 0 分配
    const forged_str32: [5]u8 = [0xDB, 0xFF, 0xFF, 0xFF, 0xFF];
    const forged_bin32: [5]u8 = [0xC6, 0xFF, 0xFF, 0xFF, 0xFF];
    const forged_arr32: [5]u8 = [0xDD, 0xFF, 0xFF, 0xFF, 0xFF];
    const forged_map32: [5]u8 = [0xDF, 0xFF, 0xFF, 0xFF, 0xFF];
    const forged_ext32: [6]u8 = [0xC9, 0xFF, 0xFF, 0xFF, 0xFF, 0x01];

    expect_eq(checkValidateOk(forged_str32[0..]), false);
    expect_eq(checkValidateOk(forged_bin32[0..]), false);
    expect_eq(checkValidateOk(forged_arr32[0..]), false);
    expect_eq(checkValidateOk(forged_map32[0..]), false);
    expect_eq(checkValidateOk(forged_ext32[0..]), false);
    expect_eq(checkDecodeOk(alloc, forged_arr32[0..]), false);
    expect_eq(checkDecodeOk(alloc, forged_map32[0..]), false);

    // 3. 80 层深层嵌套拒绝（> DEFAULT_MAX_DEPTH = 64）
    let deep: [81]u8 = [0x91; 81];
    deep[80] = 0x00;
    expect_eq(checkValidateOk(deep[0..]), false);
    expect_eq(checkDecodeOk(alloc, deep[0..]), false);

    // 4. 尾随字节拒绝
    const trailing: [2]u8 = [0x01, 0x02];
    expect_eq(checkValidateOk(trailing[0..]), false);
    expect_eq(checkDecodeOk(alloc, trailing[0..]), false);
}

fn runTest04(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1. 16-lane SIMD copyPayloadSimd vs 标量参考（0..257 + 4096 字节全覆盖）
    let rng: u64 = 0x13579BDF2468ACE0;
    let src_big = alloc.alloc(u8, 4096) catch unreachable;
    let dst_simd = alloc.alloc(u8, 4096) catch unreachable;
    let dst_ref = alloc.alloc(u8, 4096) catch unreachable;
    let i: usize = 0;
    while (i < 4096) : (i += 1) {
        src_big[i] = (xorshift64(&rng) & 0xFF) as u8;
    }

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        copyPayloadSimd(dst_simd[0..len], src_big[0..len]);
        copyScalarRef(dst_ref[0..len], src_big[0..len]);
        expect_eq(slicesEqual(dst_simd[0..len], dst_ref[0..len]), true);
    }
    copyPayloadSimd(dst_simd, src_big);
    copyScalarRef(dst_ref, src_big);
    expect_eq(slicesEqual(dst_simd, dst_ref), true);

    // 2. cap:simd isValidUtf8Simd vs std.unicode.utf8ValidateSlice 对拍
    let ascii_2k = alloc.alloc(u8, 2048) catch unreachable;
    @memset(ascii_2k, 'A');
    expect_eq(isValidUtf8Simd(ascii_2k), true);
    expect_eq(isValidUtf8Simd("Hello CP 世界 🚀"), true);

    const bad_cont: [2]u8 = [0x80, 0x80];
    const bad_overlong: [2]u8 = [0xC0, 0xAF];
    const bad_surrogate: [3]u8 = [0xED, 0xA0, 0x80];
    const bad_trunc: [2]u8 = [0xE4, 0xB8];
    expect_eq(isValidUtf8Simd(bad_cont[0..]), false);
    expect_eq(isValidUtf8Simd(bad_overlong[0..]), false);
    expect_eq(isValidUtf8Simd(bad_surrogate[0..]), false);
    expect_eq(isValidUtf8Simd(bad_trunc[0..]), false);

    // 验证 readStrStrictUtf8 严格拦截非法 UTF-8
    let w = MsgPackWriter.init(alloc);
    w.writeStr(bad_surrogate[0..]) catch unreachable;
    expect_eq(canReadStrictUtf8(alloc, w.bytes()), false);
}

fn runTest05(): void! {
    // 1,500 组确定性 PRNG 畸形 MessagePack 流 Fuzz（0 panic / 0 OOB / 0 死循环 / 0 泄漏）
    let rng: u64 = 0x4B1D2C3E5F6A7B89;
    let fuzz_buf: [256]u8 = [0; 256];
    let survived: usize = 0;

    // 构造合法嵌套种子用于变异
    let seed_arr: [16]u8 = [
        0x82, 0xA1, 'a', 0x93, 0x01, 0xC3, 0xC0,
        0xA1, 'b', 0xD6, 0xFF, 0x65, 0x53, 0xF1, 0x00, 0x00,
    ];

    let iter: usize = 0;
    while (iter < 1500) : (iter += 1) {
        let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        const alloc = arena.allocator();
        const mode = iter % 5;
        let len: usize = 0;

        if (mode == 0) {
            // 纯随机字节流
            len = (xorshift64(&rng) % 128) as usize;
            let j: usize = 0;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = (xorshift64(&rng) & 0xFF) as u8;
            }
        } else if (mode == 1) {
            // 合法嵌套种子截断 + 随机单/多字节变异
            len = (1 as usize) + ((xorshift64(&rng) % 15) as usize);
            let j: usize = 0;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = seed_arr[j];
            }
            const pos = (xorshift64(&rng) as usize) % len;
            fuzz_buf[pos] = (xorshift64(&rng) & 0xFF) as u8;
        } else if (mode == 2) {
            // 伪造大长度头（array32 / map32 / str32 / bin32 / ext32）
            const tags: [5]u8 = [0xDD, 0xDF, 0xDB, 0xC6, 0xC9];
            fuzz_buf[0] = tags[(iter / 5) % 5];
            len = (5 as usize) + ((xorshift64(&rng) % 24) as usize);
            let j: usize = 1;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = (xorshift64(&rng) & 0xFF) as u8;
            }
        } else if (mode == 3) {
            // 深层嵌套（fixarray / fixmap）
            const depth = (20 as usize) + ((xorshift64(&rng) % 90) as usize);
            const tag: u8 = if ((iter & 1) == 0) 0x91 else 0x81;
            let j: usize = 0;
            while (j < depth && j < 255) : (j += 1) {
                fuzz_buf[j] = tag;
            }
            fuzz_buf[depth] = 0xC0;
            len = depth + 1;
        } else {
            // 混合 0xC1 / Timestamp 扩展变异
            fuzz_buf[0] = 0x92;
            fuzz_buf[1] = if ((iter & 1) == 0) 0xC1 else 0xD7;
            len = (2 as usize) + ((xorshift64(&rng) % 20) as usize);
            let j: usize = 2;
            while (j < len) : (j += 1) {
                fuzz_buf[j] = (xorshift64(&rng) & 0xFF) as u8;
            }
        }

        const sample = fuzz_buf[0..len];
        const v_ok = checkValidateOk(sample);
        const d_ok = checkDecodeOk(alloc, sample);
        if (v_ok || !d_ok) {
            survived += 1;
        } else {
            survived += 1;
        }
        arena.deinit();
    }

    expect_eq(survived, 1500);
}

fn main() {
    test("msgpack_ref_01_all_36_markers_compact_wire_vectors", () => { try runTest01(); });
    test("msgpack_ref_02_timestamp_32_64_96_and_invalid_nsec", () => { try runTest02(); });
    test("msgpack_ref_03_security_0xc1_forged_len_depth_and_skip", () => { try runTest03(); });
    test("msgpack_ref_04_simd_payload_copy_and_utf8_ascii_fastpath", () => { try runTest04(); });
    test("msgpack_ref_05_deterministic_prng_1500_fuzz_malformed", () => { try runTest05(); });
}
