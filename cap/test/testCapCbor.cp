// Comprehensive RFC 8949 Appendix A & Edge-Case Regression Suite for @cap/cbor
import {
    MAX_CBOR_DEPTH,
    CborError,
    CborMajor,
    CborValueKind,
    CborEncoder,
    CborDecoder,
} from "../../packages/@cap/cbor/src/index.cp";

fn assert(cond: bool) {
    if (!cond) {
        @panic("CBOR test assertion failed");
    }
}

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn testRfc8949UintVectors() {
    let buf: [64]u8 = [0; 64];
    let enc = CborEncoder.init(buf[0..]);

    // 0 -> 0x00
    enc.writeUint(0) catch unreachable;
    const exp0: [1]u8 = [0x00];
    assert(sliceEql(enc.written(), exp0[0..]));
    let r = CborDecoder.init(enc.written());
    assert((r.peekMajor() catch unreachable) == CborMajor.Uint);
    assert((r.peekKind() catch unreachable) == CborValueKind.Uint);
    assert((r.readUint() catch unreachable) == 0);
    assert(r.remaining() == 0);

    // 23 -> 0x17
    enc.reset();
    enc.writeUint(23) catch unreachable;
    const exp23: [1]u8 = [0x17];
    assert(sliceEql(enc.written(), exp23[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 23);

    // 24 -> 0x1818
    enc.reset();
    enc.writeUint(24) catch unreachable;
    const exp24: [2]u8 = [0x18, 0x18];
    assert(sliceEql(enc.written(), exp24[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 24);

    // 100 -> 0x1864
    enc.reset();
    enc.writeUint(100) catch unreachable;
    const exp100: [2]u8 = [0x18, 0x64];
    assert(sliceEql(enc.written(), exp100[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 100);

    // 255 -> 0x18ff
    enc.reset();
    enc.writeUint(255) catch unreachable;
    const exp255: [2]u8 = [0x18, 0xFF];
    assert(sliceEql(enc.written(), exp255[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 255);

    // 1000 -> 0x1903e8
    enc.reset();
    enc.writeUint(1000) catch unreachable;
    const exp1000: [3]u8 = [0x19, 0x03, 0xE8];
    assert(sliceEql(enc.written(), exp1000[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 1000);

    // 1000000 -> 0x1a000f4240
    enc.reset();
    enc.writeUint(1000000) catch unreachable;
    const exp1m: [5]u8 = [0x1A, 0x00, 0x0F, 0x42, 0x40];
    assert(sliceEql(enc.written(), exp1m[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 1000000);

    // 1000000000000 -> 0x1b000000e8d4a51000
    enc.reset();
    enc.writeUint(1000000000000) catch unreachable;
    const exp1t: [9]u8 = [0x1B, 0x00, 0x00, 0x00, 0xE8, 0xD4, 0xA5, 0x10, 0x00];
    assert(sliceEql(enc.written(), exp1t[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 1000000000000);

    // u64 max -> 0x1bffffffffffffffff
    const u64_max: u64 = (u64(0xFFFFFFFF) << 32) | u64(0xFFFFFFFF);
    enc.reset();
    enc.writeUint(u64_max) catch unreachable;
    const exp_u64_max: [9]u8 = [0x1B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF];
    assert(sliceEql(enc.written(), exp_u64_max[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == u64_max);

    // u64 max read via readInt must return Overflow and preserve cursor
    r = CborDecoder.init(enc.written());
    let got_ovf = false;
    * = r.readInt() catch e => {
        assert(e == CborError.Overflow);
        got_ovf = true;
    };
    assert(got_ovf);
    assert(r.remaining() == 9);
}

fn testRfc8949NegIntVectors() {
    let buf: [64]u8 = [0; 64];
    let enc = CborEncoder.init(buf[0..]);

    // -1 -> 0x20
    enc.writeInt(-1) catch unreachable;
    const exp_m1: [1]u8 = [0x20];
    assert(sliceEql(enc.written(), exp_m1[0..]));
    let r = CborDecoder.init(enc.written());
    assert((r.peekMajor() catch unreachable) == CborMajor.NegInt);
    assert((r.peekKind() catch unreachable) == CborValueKind.NegInt);
    assert((r.readInt() catch unreachable) == -1);
    assert(r.remaining() == 0);

    // -10 -> 0x29
    enc.reset();
    enc.writeInt(-10) catch unreachable;
    const exp_m10: [1]u8 = [0x29];
    assert(sliceEql(enc.written(), exp_m10[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -10);

    // -24 -> 0x37
    enc.reset();
    enc.writeInt(-24) catch unreachable;
    const exp_m24: [1]u8 = [0x37];
    assert(sliceEql(enc.written(), exp_m24[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -24);

    // -25 -> 0x3818
    enc.reset();
    enc.writeInt(-25) catch unreachable;
    const exp_m25: [2]u8 = [0x38, 0x18];
    assert(sliceEql(enc.written(), exp_m25[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -25);

    // -100 -> 0x3863
    enc.reset();
    enc.writeInt(-100) catch unreachable;
    const exp_m100: [2]u8 = [0x38, 0x63];
    assert(sliceEql(enc.written(), exp_m100[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -100);

    // -1000 -> 0x3903e7
    enc.reset();
    enc.writeInt(-1000) catch unreachable;
    const exp_m1000: [3]u8 = [0x39, 0x03, 0xE7];
    assert(sliceEql(enc.written(), exp_m1000[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -1000);

    // i64 min (-9223372036854775808) -> 0x3b7fffffffffffffff
    const i64_min: i64 = @as(i64, @bitCast(u64(1) << 63));
    enc.reset();
    enc.writeInt(i64_min) catch unreachable;
    const exp_i64_min: [9]u8 = [0x3B, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF];
    assert(sliceEql(enc.written(), exp_i64_min[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == i64_min);

    // -9223372036854775809 (0x3b80000000000000) overflows i64
    const underflow_neg: [9]u8 = [0x3B, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    r = CborDecoder.init(underflow_neg[0..]);
    let got_ovf = false;
    * = r.readInt() catch e => {
        assert(e == CborError.Overflow);
        got_ovf = true;
    };
    assert(got_ovf);
    assert(r.remaining() == 9);

    // Positive via writeInt uses Major 0
    enc.reset();
    enc.writeInt(42) catch unreachable;
    const exp_42: [2]u8 = [0x18, 0x2A];
    assert(sliceEql(enc.written(), exp_42[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == 42);
}

fn testBytesTextTagsSimpleFloats() {
    let buf: [256]u8 = [0; 256];
    let enc = CborEncoder.init(buf[0..]);

    // h'' -> 0x40
    const empty_buf: [1]u8 = [0];
    enc.writeBytes(empty_buf[0..0]) catch unreachable;
    const exp_b0: [1]u8 = [0x40];
    assert(sliceEql(enc.written(), exp_b0[0..]));
    let r = CborDecoder.init(enc.written());
    assert((r.peekMajor() catch unreachable) == CborMajor.Bytes);
    assert((r.readBytes() catch unreachable).len == 0);

    // h'01020304' -> 0x4401020304
    enc.reset();
    const raw_b: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    enc.writeBytes(raw_b[0..]) catch unreachable;
    const exp_b4: [5]u8 = [0x44, 0x01, 0x02, 0x03, 0x04];
    assert(sliceEql(enc.written(), exp_b4[0..]));
    r = CborDecoder.init(enc.written());
    assert(sliceEql(r.readBytes() catch unreachable, raw_b[0..]));

    // "" -> 0x60
    enc.reset();
    enc.writeText("") catch unreachable;
    const exp_t0: [1]u8 = [0x60];
    assert(sliceEql(enc.written(), exp_t0[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.peekMajor() catch unreachable) == CborMajor.Text);
    assert((r.readText() catch unreachable).len == 0);

    // "IETF" -> 0x6449455446
    enc.reset();
    enc.writeText("IETF") catch unreachable;
    const exp_ietf: [5]u8 = [0x64, 0x49, 0x45, 0x54, 0x46];
    assert(sliceEql(enc.written(), exp_ietf[0..]));
    r = CborDecoder.init(enc.written());
    assert(sliceEql(r.readText() catch unreachable, "IETF"));

    // UTF-8 multi-byte text "\u00fc" (0xC3, 0xBC) -> 0x62c3bc
    enc.reset();
    const utf8_u: [2]u8 = [0xC3, 0xBC];
    enc.writeText(utf8_u[0..]) catch unreachable;
    const exp_utf8: [3]u8 = [0x62, 0xC3, 0xBC];
    assert(sliceEql(enc.written(), exp_utf8[0..]));
    r = CborDecoder.init(enc.written());
    assert(sliceEql(r.readText() catch unreachable, utf8_u[0..]));

    // Tag 1 (Unix Epoch): 1(1363896240) -> 0xc11a514b67b0
    enc.reset();
    enc.writeTag(1) catch unreachable;
    enc.writeUint(1363896240) catch unreachable;
    const exp_tag1: [6]u8 = [0xC1, 0x1A, 0x51, 0x4B, 0x67, 0xB0];
    assert(sliceEql(enc.written(), exp_tag1[0..]));
    r = CborDecoder.init(enc.written());
    assert((r.peekMajor() catch unreachable) == CborMajor.Tag);
    assert((r.readTag() catch unreachable) == 1);
    assert((r.readUint() catch unreachable) == 1363896240);
    assert(r.remaining() == 0);

    // true / false / null / undefined / f32 / f64
    enc.reset();
    enc.writeBool(false) catch unreachable;
    enc.writeBool(true) catch unreachable;
    enc.writeNull() catch unreachable;
    enc.writeUndefined() catch unreachable;
    enc.writeF32(100000.0) catch unreachable;
    enc.writeF64(1.1) catch unreachable;

    const exp_simple_floats: [18]u8 = [
        0xF4,
        0xF5,
        0xF6,
        0xF7,
        0xFA, 0x47, 0xC3, 0x50, 0x00,
        0xFB, 0x3F, 0xF1, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9A,
    ];
    assert(sliceEql(enc.written(), exp_simple_floats[0..]));

    r = CborDecoder.init(enc.written());
    assert((r.peekKind() catch unreachable) == CborValueKind.Bool);
    assert((r.readBool() catch unreachable) == false);
    assert((r.readBool() catch unreachable) == true);
    assert((r.peekKind() catch unreachable) == CborValueKind.Null);
    r.readNull() catch unreachable;
    assert((r.peekKind() catch unreachable) == CborValueKind.Undefined);
    r.readUndefined() catch unreachable;
    assert((r.peekKind() catch unreachable) == CborValueKind.Float32);
    const f32_v = r.readF32() catch unreachable;
    assert(@as(u32, @bitCast(f32_v)) == 0x47C35000);
    assert((r.peekKind() catch unreachable) == CborValueKind.Float64);
    const f64_v = r.readF64() catch unreachable;
    assert(@as(u64, @bitCast(f64_v)) == ((u64(0x3FF19999) << 32) | u64(0x9999999A)));
    assert(r.remaining() == 0);
}

fn testNestedMapArrayAndSkipValue() {
    let buf: [512]u8 = [0; 512];
    let enc = CborEncoder.init(buf[0..]);

    // RFC 8949 Appendix A: {"a": 1, "b": [2, 3]} -> 0xa26161016162820203
    enc.writeMapHeader(2) catch unreachable;
    enc.writeText("a") catch unreachable;
    enc.writeUint(1) catch unreachable;
    enc.writeText("b") catch unreachable;
    enc.writeArrayHeader(2) catch unreachable;
    enc.writeUint(2) catch unreachable;
    enc.writeUint(3) catch unreachable;

    const exp_map: [10]u8 = [0xA2, 0x61, 0x61, 0x01, 0x61, 0x62, 0x82, 0x02, 0x03, 0x00];
    assert(sliceEql(enc.written(), exp_map[0..9]));

    // Complex nested map: skip middle subtree containing Tag + nested Map + Array + f64 + bytes
    enc.reset();
    enc.writeMapHeader(3) catch unreachable;
    // Key 1: "first" -> 42
    enc.writeText("first") catch unreachable;
    enc.writeUint(42) catch unreachable;
    // Key 2: "skip_me" -> Tag(24) { "arr": [true, null, -1000, 1.1], "bin": h'deadbeef' }
    enc.writeText("skip_me") catch unreachable;
    enc.writeTag(24) catch unreachable;
    enc.writeMapHeader(2) catch unreachable;
    enc.writeText("arr") catch unreachable;
    enc.writeArrayHeader(4) catch unreachable;
    enc.writeBool(true) catch unreachable;
    enc.writeNull() catch unreachable;
    enc.writeInt(-1000) catch unreachable;
    enc.writeF64(1.1) catch unreachable;
    enc.writeText("bin") catch unreachable;
    const bin_payload: [4]u8 = [0xDE, 0xAD, 0xBE, 0xEF];
    enc.writeBytes(bin_payload[0..]) catch unreachable;
    // Key 3: "last" -> "ok"
    enc.writeText("last") catch unreachable;
    enc.writeText("ok") catch unreachable;

    let r = CborDecoder.init(enc.written());
    assert((r.readMapHeader() catch unreachable) == 3);
    assert(sliceEql(r.readText() catch unreachable, "first"));
    assert((r.readUint() catch unreachable) == 42);
    assert(sliceEql(r.readText() catch unreachable, "skip_me"));
    // Skip entire tagged nested map subtree
    r.skipValue() catch unreachable;
    assert(sliceEql(r.readText() catch unreachable, "last"));
    assert(sliceEql(r.readText() catch unreachable, "ok"));
    assert(r.remaining() == 0);

    // Skip also handles float16 (0xF9 + 2B) and 1-byte simple >= 32 (0xF8 0x20)
    const misc_items: [6]u8 = [0xF9, 0x3C, 0x00, 0xF8, 0x20, 0x07];
    r = CborDecoder.init(misc_items[0..]);
    assert((r.peekKind() catch unreachable) == CborValueKind.Float16);
    r.skipValue() catch unreachable;
    assert((r.peekKind() catch unreachable) == CborValueKind.Simple);
    r.skipValue() catch unreachable;
    assert((r.readUint() catch unreachable) == 7);
    assert(r.remaining() == 0);
}

fn testMalformedTruncatedAndDeepNesting() {
    // 1. BufferTooSmall on encoder preserves pos
    let small: [2]u8 = [0; 2];
    let enc = CborEncoder.init(small[0..]);
    enc.writeUint(1) catch unreachable;
    let got_small = false;
    enc.writeText("abcd") catch e => {
        assert(e == CborError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);
    assert(enc.written().len == 1);

    // 2. Empty input -> UnexpectedEof
    const empty_src: [1]u8 = [0];
    let r = CborDecoder.init(empty_src[0..0]);
    let got_eof = false;
    * = r.peekMajor() catch e => {
        assert(e == CborError.UnexpectedEof);
        got_eof = true;
    };
    assert(got_eof);

    // 3. Truncated uint32 (0x1A + only 2 bytes)
    const trunc_u32: [3]u8 = [0x1A, 0x00, 0x0F];
    r = CborDecoder.init(trunc_u32[0..]);
    got_eof = false;
    * = r.readUint() catch e => {
        assert(e == CborError.UnexpectedEof);
        got_eof = true;
    };
    assert(got_eof);
    assert(r.remaining() == 3);

    // 4. Truncated bytes payload (claims 10 bytes, only 2 present)
    const trunc_bytes: [3]u8 = [0x4A, 0xAA, 0xBB];
    r = CborDecoder.init(trunc_bytes[0..]);
    got_eof = false;
    * = r.readBytes() catch e => {
        assert(e == CborError.UnexpectedEof);
        got_eof = true;
    };
    assert(got_eof);
    assert(r.remaining() == 3);

    // 5. Reserved additional info (ai = 28 -> 0x1C) -> InvalidHeader
    const bad_ai: [1]u8 = [0x1C];
    r = CborDecoder.init(bad_ai[0..]);
    let got_inv = false;
    * = r.peekMajor() catch e => {
        assert(e == CborError.InvalidHeader);
        got_inv = true;
    };
    assert(got_inv);

    // 6. Major 0 with ai = 31 (0x1F) -> InvalidHeader
    const bad_uint_indef: [1]u8 = [0x1F];
    r = CborDecoder.init(bad_uint_indef[0..]);
    got_inv = false;
    * = r.readUint() catch e => {
        assert(e == CborError.InvalidHeader);
        got_inv = true;
    };
    assert(got_inv);

    // 7. Indefinite array (0x9F) -> UnsupportedIndefinite
    const indef_arr: [2]u8 = [0x9F, 0xFF];
    r = CborDecoder.init(indef_arr[0..]);
    let got_indef = false;
    * = r.readArrayHeader() catch e => {
        assert(e == CborError.UnsupportedIndefinite);
        got_indef = true;
    };
    assert(got_indef);

    // 8. Invalid 1-byte simple value < 32 (0xF8 0x18) -> InvalidHeader
    const bad_simple: [2]u8 = [0xF8, 0x18];
    r = CborDecoder.init(bad_simple[0..]);
    got_inv = false;
    r.skipValue() catch e => {
        assert(e == CborError.InvalidHeader);
        got_inv = true;
    };
    assert(got_inv);
    assert(r.remaining() == 2);

    // 9. TypeMismatch preserves cursor
    const txt: [2]u8 = [0x61, 0x61];
    r = CborDecoder.init(txt[0..]);
    let got_mismatch = false;
    * = r.readUint() catch e => {
        assert(e == CborError.TypeMismatch);
        got_mismatch = true;
    };
    assert(got_mismatch);
    assert(r.remaining() == 2);

    // 10. Malformed huge array length (claims 0xFFFFFFFF elements in 5 bytes) -> UnexpectedEof + cursor rollback
    const huge_arr: [5]u8 = [0x9A, 0xFF, 0xFF, 0xFF, 0xFF];
    r = CborDecoder.init(huge_arr[0..]);
    got_eof = false;
    r.skipValue() catch e => {
        assert(e == CborError.UnexpectedEof);
        got_eof = true;
    };
    assert(got_eof);
    assert(r.remaining() == 5);

    // 11. Excessively deep nested arrays (> MAX_CBOR_DEPTH = 128) -> Overflow + cursor rollback
    let deep_buf: [140]u8 = [0; 140];
    let i: usize = 0;
    while (i < 135) : (i += 1) {
        deep_buf[i] = 0x81; // array of length 1
    }
    deep_buf[135] = 0x00; // inner uint 0
    r = CborDecoder.init(deep_buf[0..136]);
    let got_deep_ovf = false;
    r.skipValue() catch e => {
        assert(e == CborError.Overflow);
        got_deep_ovf = true;
    };
    assert(got_deep_ovf);
    assert(r.remaining() == 136);
    assert(MAX_CBOR_DEPTH == 128);
}

pub fn main() {
    testRfc8949UintVectors();
    testRfc8949NegIntVectors();
    testBytesTextTagsSimpleFloats();
    testNestedMapArrayAndSkipValue();
    testMalformedTruncatedAndDeepNesting();
}
