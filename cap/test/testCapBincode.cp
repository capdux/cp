// cap/test/testCapBincode.cp
// Comprehensive unit tests for @cap/bincode (Rust bincode v2 parity)

import { test, expect_eq } from "cap:test";
import {
    BincodeEncoder,
    BincodeDecoder,
} from "../../packages/bincode/src/index.cp";

fn main() {
    test("1. @cap/bincode — varint boundary thresholds (0, 250, 251, 65535, 65536, u64::MAX)", () => {
        let buf: [64]u8 = undefined;
        let enc = BincodeEncoder.init(buf[0..64]);

        expect_eq(enc.encodeU64(0 as u64), true);          // 1 byte
        expect_eq(enc.encodeU64(250 as u64), true);        // 1 byte
        expect_eq(enc.encodeU64(251 as u64), true);        // 3 bytes (tag 251 + u16)
        expect_eq(enc.encodeU64(65535 as u64), true);      // 3 bytes
        expect_eq(enc.encodeU64(65536 as u64), true);      // 5 bytes (tag 252 + u32)
        const max64: u64 = 0xFFFFFFFFFFFFFFFF as u64;
        expect_eq(enc.encodeU64(max64), true);             // 9 bytes (tag 253 + u64)

        // Total expected wire size: 1 + 1 + 3 + 3 + 5 + 9 = 22 bytes
        expect_eq(enc.asSlice().len, 22 as usize);

        let decoder = BincodeDecoder.init(enc.asSlice());
        expect_eq(decoder.decodeU64().?, 0 as u64);
        expect_eq(decoder.decodeU64().?, 250 as u64);
        expect_eq(decoder.decodeU64().?, 251 as u64);
        expect_eq(decoder.decodeU64().?, 65535 as u64);
        expect_eq(decoder.decodeU64().?, 65536 as u64);
        expect_eq(decoder.decodeU64().?, max64);
        expect_eq(decoder.atEnd(), true);
    });

    test("2. @cap/bincode — ZigZag signed integers including negative extremes", () => {
        let buf: [64]u8 = undefined;
        let enc = BincodeEncoder.init(buf[0..64]);
        const min64: i64 = -9223372036854775807 - 1;
        const max64: i64 = 9223372036854775807;

        expect_eq(enc.encodeI64(0), true);
        expect_eq(enc.encodeI64(-1), true);
        expect_eq(enc.encodeI64(1), true);
        expect_eq(enc.encodeI64(-100000), true);
        expect_eq(enc.encodeI64(min64), true);
        expect_eq(enc.encodeI64(max64), true);

        let decoder = BincodeDecoder.init(enc.asSlice());
        expect_eq(decoder.decodeI64().?, 0 as i64);
        expect_eq(decoder.decodeI64().?, -1 as i64);
        expect_eq(decoder.decodeI64().?, 1 as i64);
        expect_eq(decoder.decodeI64().?, -100000 as i64);
        expect_eq(decoder.decodeI64().?, min64);
        expect_eq(decoder.decodeI64().?, max64);
        expect_eq(decoder.atEnd(), true);
    });

    test("3. @cap/bincode — zero-copy length-prefixed strings & fixed LE integers", () => {
        let buf: [128]u8 = undefined;
        let enc = BincodeEncoder.init(buf[0..128]);
        expect_eq(enc.encodeStr(""), true);
        expect_eq(enc.encodeStr("hello, 世界!"), true);
        expect_eq(enc.encodeFixedU32(0x12345678 as u32), true);
        expect_eq(enc.encodeFixedU64(0x0102030405060708 as u64), true);

        let decoder = BincodeDecoder.init(enc.asSlice());
        expect_eq(decoder.decodeStr().?, "");
        expect_eq(decoder.decodeStr().?, "hello, 世界!");
        expect_eq(decoder.decodeFixedU32().?, 0x12345678 as u32);
        expect_eq(decoder.decodeFixedU64().?, 0x0102030405060708 as u64);
        expect_eq(decoder.remaining(), 0 as usize);
    });

    test("4. @cap/bincode — Option<string> & bool round-trip", () => {
        let buf: [128]u8 = undefined;
        let enc = BincodeEncoder.init(buf[0..128]);
        expect_eq(enc.encodeBool(true), true);
        expect_eq(enc.encodeBool(false), true);
        expect_eq(enc.encodeOptStr(null), true);
        expect_eq(enc.encodeOptStr("payload"), true);

        let decoder = BincodeDecoder.init(enc.asSlice());
        expect_eq(decoder.decodeBool().?, true);
        expect_eq(decoder.decodeBool().?, false);

        const o1 = decoder.decodeOptStr();
        expect_eq(o1.ok, true);
        expect_eq(o1.has_value, false);

        const o2 = decoder.decodeOptStr();
        expect_eq(o2.ok, true);
        expect_eq(o2.has_value, true);
        expect_eq(o2.value, "payload");
        expect_eq(decoder.atEnd(), true);
    });

    test("5. @cap/bincode — composite multi-field RPC record round-trip", () => {
        let buf: [256]u8 = undefined;
        let enc = BincodeEncoder.init(buf[0..256]);
        expect_eq(enc.encodeU64(1001 as u64), true);
        expect_eq(enc.encodeStr("user.create"), true);
        expect_eq(enc.encodeBool(true), true);
        expect_eq(enc.encodeI64(-42), true);
        expect_eq(enc.encodeOptStr("trace-abc-123"), true);

        let decoder = BincodeDecoder.init(enc.asSlice());
        expect_eq(decoder.decodeU64().?, 1001 as u64);
        expect_eq(decoder.decodeStr().?, "user.create");
        expect_eq(decoder.decodeBool().?, true);
        expect_eq(decoder.decodeI64().?, -42 as i64);
        const trace = decoder.decodeOptStr();
        expect_eq(trace.ok && trace.has_value, true);
        expect_eq(trace.value, "trace-abc-123");
        expect_eq(decoder.atEnd(), true);
    });

    test("6. @cap/bincode — malformed tags, invalid bool, truncated stream & encoder overflow", () => {
        // Invalid bool byte (2)
        let bad_bool: [1]u8 = [2];
        let d1 = BincodeDecoder.init(bad_bool[0..1]);
        expect_eq(d1.decodeBool() == null, true);

        // Invalid varint tag (254)
        let bad_tag: [1]u8 = [254];
        let d2 = BincodeDecoder.init(bad_tag[0..1]);
        expect_eq(d2.decodeU64() == null, true);

        // Truncated string (claims 10 bytes, only 2 follow)
        let trunc_str: [3]u8 = [10, 'a', 'b'];
        let d3 = BincodeDecoder.init(trunc_str[0..3]);
        expect_eq(d3.decodeStr() == null, true);

        // Truncated u16 varint
        let trunc_u16: [2]u8 = [251, 0x01];
        let d4 = BincodeDecoder.init(trunc_u16[0..2]);
        expect_eq(d4.decodeU64() == null, true);

        // Encoder overflow protection
        let tiny: [2]u8 = undefined;
        let enc = BincodeEncoder.init(tiny[0..2]);
        expect_eq(enc.encodeStr("too long"), false);
        expect_eq(enc.overflow, true);
    });
}
