// cap/test/testCapDer.cp
// Comprehensive unit tests for @cap/der (ASN.1 DER encoding & decoding)

import { test, expect_eq } from "cap:test";
import {
    TAG_BOOLEAN,
    TAG_INTEGER,
    TAG_OCTET_STRING,
    TAG_NULL,
    TAG_SEQUENCE,
    DerReader,
    DerWriter,
} from "../../packages/der/src/index.cp";

fn main() {
    test("1. @cap/der — DerWriter length & primitive TLV round-trip via DerReader", () => {
        let lbuf: [8]u8 = [0; 8];
        expect_eq(DerWriter.encodeLength(0, lbuf[0..8]), 1 as usize);
        expect_eq(lbuf[0], 0x00 as u8);
        expect_eq(DerWriter.encodeLength(127, lbuf[0..8]), 1 as usize);
        expect_eq(lbuf[0], 0x7F as u8);

        let buf: [64]u8 = [0; 64];
        let w = DerWriter.new(buf[0..64]);
        expect_eq(w.writeBool(true), true);
        expect_eq(w.writeNull(), true);
        expect_eq(w.writeU64(42 as u64), true);
        expect_eq(w.writeOctetString("abc"), true);

        let r = DerReader.new(buf[0..w.len()]);
        const b = r.readBool();
        expect_eq(b.ok, true);
        expect_eq(b.value, true);

        expect_eq(r.readNull(), true);

        const n = r.readU64();
        expect_eq(n.ok, true);
        expect_eq(n.value, 42 as u64);

        const oct = r.readOctetString();
        expect_eq(oct.ok, true);
        expect_eq(oct.data.len, 3 as usize);
        expect_eq(r.remaining(), 0 as usize);
    });

    test("2. @cap/der — tag constants sanity check", () => {
        expect_eq(TAG_BOOLEAN, 0x01 as u8);
        expect_eq(TAG_INTEGER, 0x02 as u8);
        expect_eq(TAG_OCTET_STRING, 0x04 as u8);
        expect_eq(TAG_NULL, 0x05 as u8);
        expect_eq(TAG_SEQUENCE, 0x30 as u8);
    });
}
