// Comprehensive MessagePack Specification & Edge-Case Regression Suite for @cap/msgpack
import {
    MAX_SKIP_DEPTH,
    MsgPackError,
    MsgPackType,
    MsgPackExt,
    MsgPackEncoder,
    MsgPackDecoder,
} from "../../packages/@cap/msgpack/src/index.cp";

fn assert(cond: bool) {
    if (!cond) {
        @panic("MsgPack test assertion failed");
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

fn testUintCompactBoundaries() {
    let buf: [64]u8 = [0; 64];

    // 0 -> positive fixint [0x00]
    let enc = MsgPackEncoder.init(buf[0..]);
    enc.writeUint(0) catch unreachable;
    const exp0: [1]u8 = [0x00];
    assert(sliceEql(enc.written(), exp0[0..]));
    let r = MsgPackDecoder.init(enc.written());
    assert((r.peekType() catch unreachable) == MsgPackType.Uint);
    assert((r.readUint() catch unreachable) == 0);
    assert(r.remaining() == 0);

    // 127 -> positive fixint [0x7F]
    enc.reset();
    enc.writeUint(127) catch unreachable;
    const exp127: [1]u8 = [0x7F];
    assert(sliceEql(enc.written(), exp127[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 127);

    // 128 -> uint8 [0xCC, 0x80]
    enc.reset();
    enc.writeUint(128) catch unreachable;
    const exp128: [2]u8 = [0xCC, 0x80];
    assert(sliceEql(enc.written(), exp128[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 128);

    // 255 -> uint8 [0xCC, 0xFF]
    enc.reset();
    enc.writeUint(255) catch unreachable;
    const exp255: [2]u8 = [0xCC, 0xFF];
    assert(sliceEql(enc.written(), exp255[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 255);

    // 256 -> uint16 [0xCD, 0x01, 0x00]
    enc.reset();
    enc.writeUint(256) catch unreachable;
    const exp256: [3]u8 = [0xCD, 0x01, 0x00];
    assert(sliceEql(enc.written(), exp256[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 256);

    // 65535 -> uint16 [0xCD, 0xFF, 0xFF]
    enc.reset();
    enc.writeUint(65535) catch unreachable;
    const exp65535: [3]u8 = [0xCD, 0xFF, 0xFF];
    assert(sliceEql(enc.written(), exp65535[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 65535);

    // 65536 -> uint32 [0xCE, 0x00, 0x01, 0x00, 0x00]
    enc.reset();
    enc.writeUint(65536) catch unreachable;
    const exp65536: [5]u8 = [0xCE, 0x00, 0x01, 0x00, 0x00];
    assert(sliceEql(enc.written(), exp65536[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 65536);

    // u32 max -> uint32 [0xCE, 0xFF, 0xFF, 0xFF, 0xFF]
    enc.reset();
    enc.writeUint(0xFFFFFFFF) catch unreachable;
    const exp_u32_max: [5]u8 = [0xCE, 0xFF, 0xFF, 0xFF, 0xFF];
    assert(sliceEql(enc.written(), exp_u32_max[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == 0xFFFFFFFF);

    // u64 max -> uint64 [0xCF, 0xFF x 8]
    const u64_max: u64 = (u64(0xFFFFFFFF) << 32) | u64(0xFFFFFFFF);
    enc.reset();
    enc.writeUint(u64_max) catch unreachable;
    const exp_u64_max: [9]u8 = [0xCF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF];
    assert(sliceEql(enc.written(), exp_u64_max[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readUint() catch unreachable) == u64_max);

    // u64 max read via readInt must return Overflow
    r = MsgPackDecoder.init(enc.written());
    let got_int_ovf = false;
    * = r.readInt() catch e => {
        assert(e == MsgPackError.Overflow);
        got_int_ovf = true;
    };
    assert(got_int_ovf);
}

fn testIntCompactBoundaries() {
    let buf: [64]u8 = [0; 64];
    let enc = MsgPackEncoder.init(buf[0..]);

    // -1 -> negative fixint [0xFF]
    enc.writeInt(-1) catch unreachable;
    const exp_m1: [1]u8 = [0xFF];
    assert(sliceEql(enc.written(), exp_m1[0..]));
    let r = MsgPackDecoder.init(enc.written());
    assert((r.peekType() catch unreachable) == MsgPackType.Int);
    assert((r.readInt() catch unreachable) == -1);

    // -32 -> negative fixint [0xE0]
    enc.reset();
    enc.writeInt(-32) catch unreachable;
    const exp_m32: [1]u8 = [0xE0];
    assert(sliceEql(enc.written(), exp_m32[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -32);

    // -33 -> int8 [0xD0, 0xDF]
    enc.reset();
    enc.writeInt(-33) catch unreachable;
    const exp_m33: [2]u8 = [0xD0, 0xDF];
    assert(sliceEql(enc.written(), exp_m33[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -33);

    // -128 -> int8 [0xD0, 0x80]
    enc.reset();
    enc.writeInt(-128) catch unreachable;
    const exp_m128: [2]u8 = [0xD0, 0x80];
    assert(sliceEql(enc.written(), exp_m128[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -128);

    // -129 -> int16 [0xD1, 0xFF, 0x7F]
    enc.reset();
    enc.writeInt(-129) catch unreachable;
    const exp_m129: [3]u8 = [0xD1, 0xFF, 0x7F];
    assert(sliceEql(enc.written(), exp_m129[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -129);

    // -32768 -> int16 [0xD1, 0x80, 0x00]
    enc.reset();
    enc.writeInt(-32768) catch unreachable;
    const exp_m32768: [3]u8 = [0xD1, 0x80, 0x00];
    assert(sliceEql(enc.written(), exp_m32768[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -32768);

    // -32769 -> int32 [0xD2, 0xFF, 0xFF, 0x7F, 0xFF]
    enc.reset();
    enc.writeInt(-32769) catch unreachable;
    const exp_m32769: [5]u8 = [0xD2, 0xFF, 0xFF, 0x7F, 0xFF];
    assert(sliceEql(enc.written(), exp_m32769[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -32769);

    // -2147483648 -> int32 [0xD2, 0x80, 0x00, 0x00, 0x00]
    enc.reset();
    enc.writeInt(-2147483648) catch unreachable;
    const exp_i32_min: [5]u8 = [0xD2, 0x80, 0x00, 0x00, 0x00];
    assert(sliceEql(enc.written(), exp_i32_min[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == -2147483648);

    // i64 min (-9223372036854775808) -> int64 [0xD3, 0x80, 0x00 x 7]
    const i64_min: i64 = @as(i64, @bitCast(u64(1) << 63));
    enc.reset();
    enc.writeInt(i64_min) catch unreachable;
    const exp_i64_min: [9]u8 = [0xD3, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];
    assert(sliceEql(enc.written(), exp_i64_min[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == i64_min);

    // Positive via writeInt uses compact uint encoding (e.g., 42 -> [0x2A])
    enc.reset();
    enc.writeInt(42) catch unreachable;
    const exp_42: [1]u8 = [0x2A];
    assert(sliceEql(enc.written(), exp_42[0..]));
    r = MsgPackDecoder.init(enc.written());
    assert((r.readInt() catch unreachable) == 42);
}

fn testNilBoolFloats() {
    let buf: [64]u8 = [0; 64];
    let enc = MsgPackEncoder.init(buf[0..]);

    enc.writeNil() catch unreachable;
    enc.writeBool(false) catch unreachable;
    enc.writeBool(true) catch unreachable;
    enc.writeF32(1.5) catch unreachable;
    enc.writeF64(-3.25) catch unreachable;

    // Exact wire bytes check:
    // nil=0xC0, false=0xC2, true=0xC3,
    // f32(1.5) = 0x3FC00000 -> [0xCA, 0x3F, 0xC0, 0x00, 0x00]
    // f64(-3.25) = 0xC00A000000000000 -> [0xCB, 0xC0, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00]
    const expected: [17]u8 = [
        0xC0,
        0xC2,
        0xC3,
        0xCA, 0x3F, 0xC0, 0x00, 0x00,
        0xCB, 0xC0, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    ];
    assert(sliceEql(enc.written(), expected[0..]));

    let r = MsgPackDecoder.init(enc.written());
    assert((r.peekType() catch unreachable) == MsgPackType.Nil);
    r.readNil() catch unreachable;

    assert((r.peekType() catch unreachable) == MsgPackType.Bool);
    assert((r.readBool() catch unreachable) == false);

    assert((r.peekType() catch unreachable) == MsgPackType.Bool);
    assert((r.readBool() catch unreachable) == true);

    assert((r.peekType() catch unreachable) == MsgPackType.Float32);
    assert((r.readF32() catch unreachable) == 1.5);

    assert((r.peekType() catch unreachable) == MsgPackType.Float64);
    assert((r.readF64() catch unreachable) == -3.25);

    assert(r.remaining() == 0);
}

fn testStrBinExtFormats() {
    let payload: [300]u8 = [0; 300];
    let i: usize = 0;
    while (i < 300) : (i += 1) {
        payload[i] = ('a' + ((i % 26) as u8)) as u8;
    }

    let buf: [1024]u8 = [0; 1024];
    let enc = MsgPackEncoder.init(buf[0..]);

    // 1. fixstr (len 5)
    enc.writeStr("hello") catch unreachable;
    assert(enc.written()[0] == 0xA5);

    // 2. str8 (len 40)
    const str8_off = enc.written().len;
    enc.writeStr(payload[0..40]) catch unreachable;
    assert(enc.written()[str8_off] == 0xD9);
    assert(enc.written()[str8_off + 1] == 40);

    // 3. str16 (len 300)
    const str16_off = enc.written().len;
    enc.writeStr(payload[0..300]) catch unreachable;
    assert(enc.written()[str16_off] == 0xDA);
    assert(enc.written()[str16_off + 1] == 0x01);
    assert(enc.written()[str16_off + 2] == 0x2C);

    // 4. bin8 (len 10)
    const bin8_off = enc.written().len;
    enc.writeBin(payload[0..10]) catch unreachable;
    assert(enc.written()[bin8_off] == 0xC4);
    assert(enc.written()[bin8_off + 1] == 10);

    // 5. bin16 (len 300)
    const bin16_off = enc.written().len;
    enc.writeBin(payload[0..300]) catch unreachable;
    assert(enc.written()[bin16_off] == 0xC5);
    assert(enc.written()[bin16_off + 1] == 0x01);
    assert(enc.written()[bin16_off + 2] == 0x2C);

    let r = MsgPackDecoder.init(enc.written());
    assert((r.peekType() catch unreachable) == MsgPackType.Str);
    assert(sliceEql(r.readStr() catch unreachable, "hello"));
    assert(sliceEql(r.readStr() catch unreachable, payload[0..40]));
    assert(sliceEql(r.readStr() catch unreachable, payload[0..300]));
    assert((r.peekType() catch unreachable) == MsgPackType.Bin);
    assert(sliceEql(r.readBin() catch unreachable, payload[0..10]));
    assert(sliceEql(r.readBin() catch unreachable, payload[0..300]));
    assert(r.remaining() == 0);

    // 6. Ext: fixext 1 / 2 / 4 / 8 / 16 and ext8 (len 3)
    enc.reset();
    const ext1: [1]u8 = [0x11];
    const ext2: [2]u8 = [0x21, 0x22];
    const ext4: [4]u8 = [0x41, 0x42, 0x43, 0x44];
    const ext8_fix: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
    const ext16_fix: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
    const ext3: [3]u8 = [0xAA, 0xBB, 0xCC];

    const o1 = enc.written().len;
    enc.writeExt(-1, ext1[0..]) catch unreachable;
    assert(enc.written()[o1] == 0xD4);

    const o2 = enc.written().len;
    enc.writeExt(2, ext2[0..]) catch unreachable;
    assert(enc.written()[o2] == 0xD5);

    const o4 = enc.written().len;
    enc.writeExt(4, ext4[0..]) catch unreachable;
    assert(enc.written()[o4] == 0xD6);

    const o8 = enc.written().len;
    enc.writeExt(8, ext8_fix[0..]) catch unreachable;
    assert(enc.written()[o8] == 0xD7);

    const o16 = enc.written().len;
    enc.writeExt(16, ext16_fix[0..]) catch unreachable;
    assert(enc.written()[o16] == 0xD8);

    const o3 = enc.written().len;
    enc.writeExt(42, ext3[0..]) catch unreachable;
    assert(enc.written()[o3] == 0xC7);
    assert(enc.written()[o3 + 1] == 3);
    assert(enc.written()[o3 + 2] == 42);

    r = MsgPackDecoder.init(enc.written());
    assert((r.peekType() catch unreachable) == MsgPackType.Ext);
    const e1 = r.readExt() catch unreachable;
    assert(e1.type_code == -1 && sliceEql(e1.data, ext1[0..]));

    const e2 = r.readExt() catch unreachable;
    assert(e2.type_code == 2 && sliceEql(e2.data, ext2[0..]));

    const e4 = r.readExt() catch unreachable;
    assert(e4.type_code == 4 && sliceEql(e4.data, ext4[0..]));

    const e8 = r.readExt() catch unreachable;
    assert(e8.type_code == 8 && sliceEql(e8.data, ext8_fix[0..]));

    const e16 = r.readExt() catch unreachable;
    assert(e16.type_code == 16 && sliceEql(e16.data, ext16_fix[0..]));

    const e3 = r.readExt() catch unreachable;
    assert(e3.type_code == 42 && sliceEql(e3.data, ext3[0..]));
    assert(r.remaining() == 0);
}

fn testNestedArrayMapAndSkipValue() {
    let buf: [512]u8 = [0; 512];
    let enc = MsgPackEncoder.init(buf[0..]);

    // Encode:
    // {
    //   "id": 1001,
    //   "nested": {
    //     "arr": [nil, true, -500, 2.5, "deep", bin([9,8,7]), ext(7, [1,2])],
    //     "sub": { "k": "v" }
    //   },
    //   "tail": "ok"
    // }
    enc.writeMapHeader(3) catch unreachable;

    enc.writeStr("id") catch unreachable;
    enc.writeUint(1001) catch unreachable;

    enc.writeStr("nested") catch unreachable;
    enc.writeMapHeader(2) catch unreachable;

    enc.writeStr("arr") catch unreachable;
    enc.writeArrayHeader(7) catch unreachable;
    enc.writeNil() catch unreachable;
    enc.writeBool(true) catch unreachable;
    enc.writeInt(-500) catch unreachable;
    enc.writeF64(2.5) catch unreachable;
    enc.writeStr("deep") catch unreachable;
    const raw_bin: [3]u8 = [9, 8, 7];
    enc.writeBin(raw_bin[0..]) catch unreachable;
    const raw_ext: [2]u8 = [1, 2];
    enc.writeExt(7, raw_ext[0..]) catch unreachable;

    enc.writeStr("sub") catch unreachable;
    enc.writeMapHeader(1) catch unreachable;
    enc.writeStr("k") catch unreachable;
    enc.writeStr("v") catch unreachable;

    enc.writeStr("tail") catch unreachable;
    enc.writeStr("ok") catch unreachable;

    // Decode & skip "nested" entirely using skipValue()
    let r = MsgPackDecoder.init(enc.written());
    assert((r.peekType() catch unreachable) == MsgPackType.Map);
    const map_n = r.readMapHeader() catch unreachable;
    assert(map_n == 3);

    assert(sliceEql(r.readStr() catch unreachable, "id"));
    assert((r.readUint() catch unreachable) == 1001);

    assert(sliceEql(r.readStr() catch unreachable, "nested"));
    assert((r.peekType() catch unreachable) == MsgPackType.Map);
    r.skipValue() catch unreachable;

    assert(sliceEql(r.readStr() catch unreachable, "tail"));
    assert(sliceEql(r.readStr() catch unreachable, "ok"));
    assert(r.remaining() == 0);
}

fn checkUnexpectedEofOnReadUint(data: []const u8) {
    let r = MsgPackDecoder.init(data);
    let got_eof = false;
    * = r.readUint() catch e => {
        assert(e == MsgPackError.UnexpectedEof);
        got_eof = true;
    };
    assert(got_eof);
    assert(r.remaining() == data.len);
}

fn testMalformedAndTruncatedInputs() {
    // 1. BufferTooSmall on encoder
    let tiny: [2]u8 = [0; 2];
    let enc = MsgPackEncoder.init(tiny[0..]);
    let got_small = false;
    enc.writeUint(0x10000) catch e => {
        assert(e == MsgPackError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);
    assert(enc.written().len == 0);

    // 2. Empty input -> UnexpectedEof
    const empty: []const u8 = "";
    checkUnexpectedEofOnReadUint(empty);

    // 3. Truncated uint16 / uint32 / uint64
    const trunc_u16: [2]u8 = [0xCD, 0x01];
    checkUnexpectedEofOnReadUint(trunc_u16[0..]);

    const trunc_u32: [3]u8 = [0xCE, 0x01, 0x02];
    checkUnexpectedEofOnReadUint(trunc_u32[0..]);

    const trunc_u64: [5]u8 = [0xCF, 0x01, 0x02, 0x03, 0x04];
    checkUnexpectedEofOnReadUint(trunc_u64[0..]);

    // 4. Truncated str payload
    const trunc_str: [3]u8 = [0xA5, 'h', 'i'];
    let rs = MsgPackDecoder.init(trunc_str[0..]);
    let got_str_eof = false;
    * = rs.readStr() catch e => {
        assert(e == MsgPackError.UnexpectedEof);
        got_str_eof = true;
    };
    assert(got_str_eof);
    assert(rs.remaining() == 3);

    // 5. Invalid marker 0xC1
    const bad_marker: [1]u8 = [0xC1];
    let rm = MsgPackDecoder.init(bad_marker[0..]);
    let got_invalid = false;
    * = rm.peekType() catch e => {
        assert(e == MsgPackError.InvalidMarker);
        got_invalid = true;
    };
    assert(got_invalid);

    // 6. TypeMismatch (reading bool when data is nil)
    const nil_bytes: [1]u8 = [0xC0];
    let rt = MsgPackDecoder.init(nil_bytes[0..]);
    let got_mismatch = false;
    * = rt.readBool() catch e => {
        assert(e == MsgPackError.TypeMismatch);
        got_mismatch = true;
    };
    assert(got_mismatch);
    assert(rt.remaining() == 1);

    // 7. Malformed huge array32 count (0x7FFFFFFF elements with only 5 bytes) -> UnexpectedEof in skipValue
    const huge_arr: [5]u8 = [0xDD, 0x7F, 0xFF, 0xFF, 0xFF];
    let rh = MsgPackDecoder.init(huge_arr[0..]);
    let got_huge_eof = false;
    rh.skipValue() catch e => {
        assert(e == MsgPackError.UnexpectedEof);
        got_huge_eof = true;
    };
    assert(got_huge_eof);
    assert(rh.remaining() == 5);

    // 8. Excessive nesting depth (> MAX_SKIP_DEPTH) -> Overflow (stack guard)
    let deep: [140]u8 = [0x91; 140];
    deep[139] = 0xC0; // innermost nil
    let rd = MsgPackDecoder.init(deep[0..]);
    let got_depth_ovf = false;
    rd.skipValue() catch e => {
        assert(e == MsgPackError.Overflow);
        got_depth_ovf = true;
    };
    assert(got_depth_ovf);
    assert(rd.remaining() == 140);
    assert(MAX_SKIP_DEPTH == 128);
}

pub fn main() {
    testUintCompactBoundaries();
    testIntCompactBoundaries();
    testNilBoolFloats();
    testStrBinExtFormats();
    testNestedArrayMapAndSkipValue();
    testMalformedAndTruncatedInputs();
}
