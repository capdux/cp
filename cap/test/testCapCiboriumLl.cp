import { test, expect_eq } from "cap:test";
import {
    CborHeaderKind,
    CborHeader,
    CborDecoder,
    CborEncoder,
} from "../../packages/ciborium-ll/src/index.cp";

fn main() {
    test("1. @cap/ciborium-ll — positive & negative integers (0..u64, -1..-1000000)", () => {
        let buf: [64]u8 = [0 as u8; 64];
        let enc: CborEncoder = CborEncoder.new(buf[0..64]);

        expect_eq(enc.pushPositive(0), true);
        expect_eq(enc.pushPositive(23), true);
        expect_eq(enc.pushPositive(24), true);
        expect_eq(enc.pushPositive(1000), true);
        expect_eq(enc.pushPositive(1000000), true);
        expect_eq(enc.pushNegative(-1), true);
        expect_eq(enc.pushNegative(-100), true);
        expect_eq(enc.pushNegative(-1000000), true);
        // Positive value rejected by pushNegative
        expect_eq(enc.pushNegative(0), false);

        let reader: CborDecoder = CborDecoder.new(enc.slice());
        expect_eq(reader.remaining(), enc.written());

        const h0 = reader.pull();
        expect_eq(h0.kind == CborHeaderKind.Positive, true);
        expect_eq(h0.u64_val, 0 as u64);

        const h1 = reader.pull();
        expect_eq(h1.u64_val, 23 as u64);

        const h2 = reader.pull();
        expect_eq(h2.u64_val, 24 as u64);

        const h3 = reader.pull();
        expect_eq(h3.u64_val, 1000 as u64);

        const h4 = reader.pull();
        expect_eq(h4.u64_val, 1000000 as u64);

        const h5 = reader.pull();
        expect_eq(h5.kind == CborHeaderKind.Negative, true);
        expect_eq(h5.i64_val, -1 as i64);
        expect_eq(h5.u64_val, 0 as u64);

        const h6 = reader.pull();
        expect_eq(h6.kind == CborHeaderKind.Negative, true);
        expect_eq(h6.i64_val, -100 as i64);

        const h7 = reader.pull();
        expect_eq(h7.kind == CborHeaderKind.Negative, true);
        expect_eq(h7.i64_val, -1000000 as i64);
        expect_eq(reader.isEof(), true);
    });

    test("2. @cap/ciborium-ll — bytes & text strings with readBytes()", () => {
        let buf: [64]u8 = [0 as u8; 64];
        let enc: CborEncoder = CborEncoder.new(buf[0..64]);

        const raw: [4]u8 = [0xDE as u8, 0xAD as u8, 0xBE as u8, 0xEF as u8];
        expect_eq(enc.pushBytes(raw[0..4]), true);
        expect_eq(enc.pushText("hello-cbor"), true);

        let reader: CborDecoder = CborDecoder.new(enc.slice());
        const hb = reader.pull();
        expect_eq(hb.kind == CborHeaderKind.Bytes, true);
        expect_eq(hb.len, 4 as i64);
        if (b := reader.readBytes(hb.len as usize)) {
            expect_eq(b.len, 4 as usize);
            expect_eq(b[0], 0xDE as u8);
            expect_eq(b[3], 0xEF as u8);
        } else {
            expect_eq(true, false);
        }

        const ht = reader.pull();
        expect_eq(ht.kind == CborHeaderKind.Text, true);
        expect_eq(ht.len, 10 as i64);
        if (t := reader.readBytes(ht.len as usize)) {
            expect_eq(t.len, 10 as usize);
            expect_eq(t[0], 0x68 as u8); // 'h'
            expect_eq(t[9], 0x72 as u8); // 'r'
        } else {
            expect_eq(true, false);
        }
        expect_eq(reader.remaining(), 0 as usize);
    });

    test("3. @cap/ciborium-ll — arrays, maps, tags & nested structure", () => {
        let buf: [64]u8 = [0 as u8; 64];
        let enc: CborEncoder = CborEncoder.new(buf[0..64]);

        // Tag(1) -> Map(2) { "k": [true, null] }
        expect_eq(enc.pushTag(1), true);
        expect_eq(enc.pushMap(1), true);
        expect_eq(enc.pushText("k"), true);
        expect_eq(enc.pushArray(2), true);
        expect_eq(enc.pushBool(true), true);
        expect_eq(enc.pushNull(), true);

        let reader: CborDecoder = CborDecoder.new(enc.slice());
        const htag = reader.pull();
        expect_eq(htag.kind == CborHeaderKind.Tag, true);
        expect_eq(htag.u64_val, 1 as u64);

        const hmap = reader.pull();
        expect_eq(hmap.kind == CborHeaderKind.Map, true);
        expect_eq(hmap.len, 1 as i64);

        const hk = reader.pull();
        expect_eq(hk.kind == CborHeaderKind.Text, true);
        expect_eq(hk.len, 1 as i64);
        const kbytes = reader.readBytes(1);
        expect_eq(kbytes != null, true);

        const harr = reader.pull();
        expect_eq(harr.kind == CborHeaderKind.Array, true);
        expect_eq(harr.len, 2 as i64);

        const htrue = reader.pull();
        expect_eq(htrue.kind == CborHeaderKind.Bool, true);
        expect_eq(htrue.bool_val, true);

        const hnull = reader.pull();
        expect_eq(hnull.kind == CborHeaderKind.Null, true);
        expect_eq(reader.isEof(), true);
    });

    test("4. @cap/ciborium-ll — indefinite-length containers & Break stop code", () => {
        let buf: [32]u8 = [0 as u8; 32];
        let enc: CborEncoder = CborEncoder.new(buf[0..32]);

        expect_eq(enc.pushArray(-1), true);
        expect_eq(enc.pushPositive(42), true);
        expect_eq(enc.pushMap(-1), true);
        expect_eq(enc.pushBreak(), true);
        expect_eq(enc.pushBreak(), true);

        let reader: CborDecoder = CborDecoder.new(enc.slice());
        const ha = reader.pull();
        expect_eq(ha.kind == CborHeaderKind.Array, true);
        expect_eq(ha.len, -1 as i64);

        const hv = reader.pull();
        expect_eq(hv.kind == CborHeaderKind.Positive, true);
        expect_eq(hv.u64_val, 42 as u64);

        const hm = reader.pull();
        expect_eq(hm.kind == CborHeaderKind.Map, true);
        expect_eq(hm.len, -1 as i64);

        const hb1 = reader.pull();
        expect_eq(hb1.kind == CborHeaderKind.Break, true);
        const hb2 = reader.pull();
        expect_eq(hb2.kind == CborHeaderKind.Break, true);
        expect_eq(reader.isEof(), true);
    });

    test("5. @cap/ciborium-ll — floats (binary64, binary32, binary16) & simple values", () => {
        let buf: [32]u8 = [0 as u8; 32];
        let enc: CborEncoder = CborEncoder.new(buf[0..32]);

        expect_eq(enc.pushFloat64(3.5), true);
        expect_eq(enc.pushBool(false), true);
        expect_eq(enc.pushUndefined(), true);

        let reader: CborDecoder = CborDecoder.new(enc.slice());
        const hf64 = reader.pull();
        expect_eq(hf64.kind == CborHeaderKind.Float, true);
        expect_eq(hf64.f64_val, 3.5);

        const hfalse = reader.pull();
        expect_eq(hfalse.kind == CborHeaderKind.Bool, true);
        expect_eq(hfalse.bool_val, false);

        const hundef = reader.pull();
        expect_eq(hundef.kind == CborHeaderKind.Undefined, true);

        // RFC 8949 half-precision (0xF9 0x3E 0x00 = 1.5) & single-precision (0xFA 0x40 0x20 0x00 0x00 = 2.5)
        const f16_f32: [8]u8 = [
            0xF9 as u8, 0x3E as u8, 0x00 as u8,
            0xFA as u8, 0x40 as u8, 0x20 as u8, 0x00 as u8, 0x00 as u8,
        ];
        let r2: CborDecoder = CborDecoder.new(f16_f32[0..8]);
        const hh = r2.pull();
        expect_eq(hh.kind == CborHeaderKind.Float, true);
        expect_eq(hh.f64_val, 1.5);

        const hs = r2.pull();
        expect_eq(hs.kind == CborHeaderKind.Float, true);
        expect_eq(hs.f64_val, 2.5);
    });

    test("6. @cap/ciborium-ll — truncated input & buffer-overflow atomic protection", () => {
        // Encoder rejects writes that don't fit without corrupting pos
        let tiny: [3]u8 = [0 as u8; 3];
        let enc: CborEncoder = CborEncoder.new(tiny[0..3]);
        expect_eq(enc.pushPositive(10), true); // 1 byte
        expect_eq(enc.pushFloat64(1.0), false); // needs 9 bytes -> rejected
        expect_eq(enc.written(), 1 as usize);

        // Decoder returns Invalid on truncated multi-byte header or reserved ai=28
        const trunc: [2]u8 = [0x19 as u8, 0x01 as u8]; // needs 3 bytes
        let r_trunc: CborDecoder = CborDecoder.new(trunc[0..2]);
        const ht = r_trunc.pull();
        expect_eq(ht.kind == CborHeaderKind.Invalid, true);

        // Invalid indefinite integer (0x1F)
        const bad_indef: [1]u8 = [0x1F as u8];
        let r_bad: CborDecoder = CborDecoder.new(bad_indef[0..1]);
        expect_eq(r_bad.pull().kind == CborHeaderKind.Invalid, true);

        // readBytes returns null when requesting more than remaining
        expect_eq(r_bad.readBytes(5) == null, true);
    });
}
