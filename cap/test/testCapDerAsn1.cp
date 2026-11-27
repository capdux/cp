// cap/test/testCapDerAsn1.cp
// Comprehensive unit tests for @cap/der-asn1

import { test, expect_eq } from "cap:test";
import { ObjectIdentifier, OID_SHA256_WITH_RSA } from "../../packages/const-oid/src/index.cp";
import {
    TAG_BOOLEAN,
    TAG_INTEGER,
    TAG_BIT_STRING,
    TAG_OCTET_STRING,
    TAG_NULL,
    TAG_OID,
    TAG_SEQUENCE,
    DerReader,
    DerWriter,
} from "../../packages/der/src/index.cp";

fn derTestEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/der-asn1 — Short & long canonical length encoding/decoding", () => {
        // Short form (< 128)
        let lbuf: [8]u8 = [0; 8];
        expect_eq(DerWriter.encodeLength(0, lbuf[0..8]), 1 as usize);
        expect_eq(lbuf[0], 0x00 as u8);
        expect_eq(DerWriter.encodeLength(127, lbuf[0..8]), 1 as usize);
        expect_eq(lbuf[0], 0x7F as u8);

        // Long form 1-byte (128..255)
        expect_eq(DerWriter.encodeLength(128, lbuf[0..8]), 2 as usize);
        expect_eq(lbuf[0], 0x81 as u8);
        expect_eq(lbuf[1], 0x80 as u8);

        // Long form 2-byte (256..65535): 300 = 0x012C
        expect_eq(DerWriter.encodeLength(300, lbuf[0..8]), 3 as usize);
        expect_eq(lbuf[0], 0x82 as u8);
        expect_eq(lbuf[1], 0x01 as u8);
        expect_eq(lbuf[2], 0x2C as u8);

        // Round-trip a 200-byte OCTET STRING using long-form length (0x81 0xC8)
        let payload: [200]u8 = [0; 200];
        let i: usize = 0;
        while (i < 200) {
            payload[i] = (i & 0xFF) as u8;
            i = i + 1;
        }
        let wire: [256]u8 = [0; 256];
        let w = DerWriter.new(wire[0..256]);
        expect_eq(w.writeOctetString(payload[0..200]), true);
        expect_eq(w.len(), 203 as usize);
        expect_eq(wire[0], TAG_OCTET_STRING);
        expect_eq(wire[1], 0x81 as u8);
        expect_eq(wire[2], 0xC8 as u8);

        let r = DerReader.new(w.written());
        const os = r.readOctetString();
        expect_eq(os.ok, true);
        expect_eq(derTestEqBytes(os.data, payload[0..200]), true);
        expect_eq(r.isEmpty(), true);
    });

    test("2. @cap/der-asn1 — Canonical INTEGER encoding with 0x80 sign-bit padding", () => {
        let buf: [64]u8 = [0; 64];
        let w = DerWriter.new(buf[0..64]);

        // 0 -> 02 01 00
        expect_eq(w.writeU64(0 as u64), true);
        // 127 -> 02 01 7F
        expect_eq(w.writeU64(127 as u64), true);
        // 128 -> 02 02 00 80 (must prepend 0x00 to keep unsigned!)
        expect_eq(w.writeU64(128 as u64), true);
        // 0xFFFF -> 02 03 00 FF FF
        expect_eq(w.writeU64(0xFFFF as u64), true);
        // 0x8000000000000001 -> 9 content bytes (0x00 + 8 bytes)
        expect_eq(w.writeU64(0x8000000000000001 as u64), true);

        // Verify exact wire bytes for 128 (at offset 6: 02 02 00 80)
        expect_eq(buf[6], TAG_INTEGER);
        expect_eq(buf[7], 0x02 as u8);
        expect_eq(buf[8], 0x00 as u8);
        expect_eq(buf[9], 0x80 as u8);

        let r = DerReader.new(w.written());
        const v0 = r.readU64();
        expect_eq(v0.ok, true);
        expect_eq(v0.value, 0 as u64);
        const v1 = r.readU64();
        expect_eq(v1.ok, true);
        expect_eq(v1.value, 127 as u64);
        const v2 = r.readU64();
        expect_eq(v2.ok, true);
        expect_eq(v2.value, 128 as u64);
        const v3 = r.readU64();
        expect_eq(v3.ok, true);
        expect_eq(v3.value, 0xFFFF as u64);
        const v4 = r.readU64();
        expect_eq(v4.ok, true);
        expect_eq(v4.value, 0x8000000000000001 as u64);
        expect_eq(r.isEmpty(), true);
    });

    test("3. @cap/der-asn1 — BOOLEAN, NULL, OCTET_STRING, BIT_STRING round-trip", () => {
        let buf: [128]u8 = [0; 128];
        let w = DerWriter.new(buf[0..128]);

        expect_eq(w.writeBool(false), true);
        expect_eq(w.writeBool(true), true);
        expect_eq(w.writeNull(), true);
        expect_eq(w.writeOctetString("DER-bytes"), true);
        let bits: [2]u8 = [0xAA as u8, 0xF0 as u8]; // 4 unused trailing bits are 0
        expect_eq(w.writeBitString(4 as u8, bits[0..2]), true);

        // Verify canonical BOOLEAN wire bytes: 01 01 00 and 01 01 FF
        expect_eq(buf[0], TAG_BOOLEAN);
        expect_eq(buf[2], 0x00 as u8);
        expect_eq(buf[3], TAG_BOOLEAN);
        expect_eq(buf[5], 0xFF as u8);

        let r = DerReader.new(w.written());
        const b0 = r.readBool();
        expect_eq(b0.ok, true);
        expect_eq(b0.value, false);
        const b1 = r.readBool();
        expect_eq(b1.ok, true);
        expect_eq(b1.value, true);
        expect_eq(r.peekTag(), TAG_NULL);
        expect_eq(r.readNull(), true);

        const os = r.readOctetString();
        expect_eq(os.ok, true);
        expect_eq(derTestEqBytes(os.data, "DER-bytes"), true);

        const bs = r.readBitString();
        expect_eq(bs.ok, true);
        expect_eq(bs.unused_bits, 4 as u8);
        expect_eq(derTestEqBytes(bs.bytes, bits[0..2]), true);
        expect_eq(r.isEmpty(), true);
    });

    test("4. @cap/der-asn1 — Nested SEQUENCE AlgorithmIdentifier reusing @cap/const-oid", () => {
        // Build X.509 AlgorithmIdentifier ::= SEQUENCE { algorithm OID(sha256WithRSAEncryption), parameters NULL }
        const oid_res = ObjectIdentifier.fromStr(OID_SHA256_WITH_RSA);
        expect_eq(oid_res.ok, true);

        let inner_buf: [64]u8 = [0; 64];
        let inner_w = DerWriter.new(inner_buf[0..64]);
        expect_eq(inner_w.writeOid(oid_res.oid), true);
        expect_eq(inner_w.writeNull(), true);

        let outer_buf: [128]u8 = [0; 128];
        let outer_w = DerWriter.new(outer_buf[0..128]);
        expect_eq(outer_w.writeSequence(inner_w.written()), true);

        // Decode outer SEQUENCE -> inner OID + NULL
        let outer_r = DerReader.new(outer_w.written());
        expect_eq(outer_r.peekTag(), TAG_SEQUENCE);
        const seq = outer_r.readSequence();
        expect_eq(seq.ok, true);
        expect_eq(outer_r.isEmpty(), true);

        let sub = seq.reader;
        expect_eq(sub.peekTag(), TAG_OID);
        const parsed_oid = sub.readOid();
        expect_eq(parsed_oid.ok, true);
        expect_eq(parsed_oid.oid.eq(oid_res.oid), true);
        expect_eq(sub.readNull(), true);
        expect_eq(sub.isEmpty(), true);
    });

    test("5. @cap/der-asn1 — Strictly reject non-canonical DER & malformed frames", () => {
        // Indefinite length (0x80)
        let indef: [4]u8 = [0x30 as u8, 0x80 as u8, 0x00 as u8, 0x00 as u8];
        let r1 = DerReader.new(indef[0..4]);
        expect_eq(r1.readHeader().ok, false);

        // Non-minimal long-form length (0x81 0x05 instead of 0x05)
        let non_min_len: [8]u8 = [0x04 as u8, 0x81 as u8, 0x05 as u8, 1 as u8, 2 as u8, 3 as u8, 4 as u8, 5 as u8];
        let r2 = DerReader.new(non_min_len[0..8]);
        expect_eq(r2.readOctetString().ok, false);

        // Leading zero in long-form length (0x82 0x00 0x80)
        let lead_zero_len: [4]u8 = [0x04 as u8, 0x82 as u8, 0x00 as u8, 0x80 as u8];
        let r3 = DerReader.new(lead_zero_len[0..4]);
        expect_eq(r3.readHeader().ok, false);

        // Non-canonical BOOLEAN (0x01 instead of 0xFF)
        let bad_bool: [3]u8 = [0x01 as u8, 0x01 as u8, 0x01 as u8];
        let r4 = DerReader.new(bad_bool[0..3]);
        expect_eq(r4.readBool().ok, false);

        // Redundant leading 0x00 in INTEGER (02 02 00 7F)
        let bad_int: [4]u8 = [0x02 as u8, 0x02 as u8, 0x00 as u8, 0x7F as u8];
        let r5 = DerReader.new(bad_int[0..4]);
        expect_eq(r5.readU64().ok, false);

        // Negative INTEGER rejected by readU64 (02 01 FF)
        let neg_int: [3]u8 = [0x02 as u8, 0x01 as u8, 0xFF as u8];
        let r6 = DerReader.new(neg_int[0..3]);
        expect_eq(r6.readU64().ok, false);

        // Truncated payload (declared length 10, only 2 bytes present)
        let trunc: [4]u8 = [0x04 as u8, 0x0A as u8, 0xAA as u8, 0xBB as u8];
        let r7 = DerReader.new(trunc[0..4]);
        expect_eq(r7.readOctetString().ok, false);
        expect_eq(r7.remaining(), 4 as usize); // Cursor restored on failure

        // Non-zero unused trailing bit in BIT STRING
        let bad_bits: [4]u8 = [0x03 as u8, 0x02 as u8, 0x04 as u8, 0xF1 as u8];
        let r8 = DerReader.new(bad_bits[0..4]);
        expect_eq(r8.readBitString().ok, false);
    });

    test("6. @cap/der-asn1 — Empty SEQUENCE, empty BIT_STRING & buffer bounds", () => {
        let buf: [16]u8 = [0; 16];
        let w = DerWriter.new(buf[0..16]);

        let empty_content: [1]u8 = [0 as u8];
        expect_eq(w.writeSequence(empty_content[0..0]), true);
        expect_eq(w.writeBitString(0 as u8, empty_content[0..0]), true);
        expect_eq(w.len(), 5 as usize); // 30 00 + 03 01 00

        let r = DerReader.new(w.written());
        const seq = r.readSequence();
        expect_eq(seq.ok, true);
        expect_eq(seq.reader.isEmpty(), true);

        const bs = r.readBitString();
        expect_eq(bs.ok, true);
        expect_eq(bs.unused_bits, 0 as u8);
        expect_eq(bs.bytes.len, 0 as usize);
        expect_eq(r.isEmpty(), true);

        // Buffer too small on writer
        let tiny: [1]u8 = [0 as u8];
        let wt = DerWriter.new(tiny[0..1]);
        expect_eq(wt.writeNull(), false);
        expect_eq(wt.len(), 0 as usize);
    });
}
