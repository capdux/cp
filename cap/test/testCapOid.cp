// cap/test/testCapOid.cp
// Comprehensive unit tests for @cap/oid

import { test, expect_eq } from "cap:test";
import {
    parseOid,
    formatOid,
    oidEquals,
    encodeOidDer,
    decodeOidDer,
    oidName,
} from "../../packages/const-oid/src/index.cp";

fn main() {
    test("1. @cap/oid — parseOid, formatOid & oidEquals round-trip", () => {
        const o1 = parseOid("1.2.840.113549.1.1.11");
        expect_eq(o1.valid, true);
        expect_eq(o1.arc_count, 7 as usize);
        expect_eq(o1.arcs[0], 1 as u32);
        expect_eq(o1.arcs[1], 2 as u32);
        expect_eq(o1.arcs[2], 840 as u32);
        expect_eq(o1.arcs[3], 113549 as u32);
        expect_eq(o1.arcs[6], 11 as u32);

        let out: [64]u8 = undefined;
        expect_eq(formatOid(o1, out[0..64]), "1.2.840.113549.1.1.11");

        const o2 = parseOid("1.2.840.113549.1.1.11");
        const o3 = parseOid("2.5.4.3");
        expect_eq(oidEquals(o1, o2), true);
        expect_eq(oidEquals(o1, o3), false);
    });

    test("2. @cap/oid — X.690 DER exact bytes for sha256WithRSAEncryption (1.2.840.113549.1.1.11)", () => {
        const oid = parseOid("1.2.840.113549.1.1.11");
        let derBuf: [32]u8 = undefined;
        const derOpt = encodeOidDer(oid, derBuf[0..32]);
        expect_eq(derOpt != null, true);
        const der = derOpt ?? "";
        // Expected DER: 2A 86 48 86 F7 0D 01 01 0B
        expect_eq(der.len, 9 as usize);
        expect_eq(der[0], 0x2A as u8);
        expect_eq(der[1], 0x86 as u8);
        expect_eq(der[2], 0x48 as u8);
        expect_eq(der[3], 0x86 as u8);
        expect_eq(der[4], 0xF7 as u8);
        expect_eq(der[5], 0x0D as u8);
        expect_eq(der[6], 0x01 as u8);
        expect_eq(der[7], 0x01 as u8);
        expect_eq(der[8], 0x0B as u8);

        const decoded = decodeOidDer(der);
        expect_eq(decoded.valid, true);
        expect_eq(oidEquals(oid, decoded), true);
    });

    test("3. @cap/oid — Large second arc under root 2 (2.999.1) Base-128 VLQ round-trip", () => {
        const oid = parseOid("2.999.1");
        expect_eq(oid.valid, true);
        expect_eq(oid.arcs[0], 2 as u32);
        expect_eq(oid.arcs[1], 999 as u32);
        expect_eq(oid.arcs[2], 1 as u32);

        let derBuf: [32]u8 = undefined;
        const der = encodeOidDer(oid, derBuf[0..32]) ?? "";
        // 40*2 + 999 = 1079 = 0x08 * 128 + 0x37 => 0x88, 0x37, 0x01
        expect_eq(der.len, 3 as usize);
        expect_eq(der[0], 0x88 as u8);
        expect_eq(der[1], 0x37 as u8);
        expect_eq(der[2], 0x01 as u8);

        const decoded = decodeOidDer(der);
        expect_eq(decoded.valid, true);
        expect_eq(oidEquals(oid, decoded), true);
    });

    test("4. @cap/oid — Reject non-canonical 0x80 padding & truncated DER streams", () => {
        expect_eq(decodeOidDer("").valid, false);

        // Non-canonical leading 0x80 in subidentifier
        let badPad: [3]u8 = undefined;
        badPad[0] = 0x2A as u8;
        badPad[1] = 0x80 as u8;
        badPad[2] = 0x01 as u8;
        expect_eq(decodeOidDer(badPad[0..3]).valid, false);

        // Truncated VLQ (high bit set on final byte)
        let trunc: [2]u8 = undefined;
        trunc[0] = 0x2A as u8;
        trunc[1] = 0x86 as u8;
        expect_eq(decodeOidDer(trunc[0..2]).valid, false);
    });

    test("5. @cap/oid — Reject invalid dot-decimal OID strings", () => {
        expect_eq(parseOid("").valid, false);
        expect_eq(parseOid("1").valid, false);
        expect_eq(parseOid("3.1.2").valid, false);
        expect_eq(parseOid("1.40.2").valid, false);
        expect_eq(parseOid("0.40").valid, false);
        expect_eq(parseOid("1.02.3").valid, false);
        expect_eq(parseOid("1..2").valid, false);
        expect_eq(parseOid("1.2.").valid, false);
        expect_eq(parseOid("1.2.abc").valid, false);
    });

    test("6. @cap/oid — Built-in PKI / X.509 / crypto OID naming table", () => {
        expect_eq(oidName("2.5.4.3") ?? "", "CN");
        expect_eq(oidName("2.5.4.10") ?? "", "O");
        expect_eq(oidName("2.5.4.6") ?? "", "C");
        expect_eq(oidName("1.2.840.113549.1.1.1") ?? "", "rsaEncryption");
        expect_eq(oidName("1.2.840.113549.1.1.11") ?? "", "sha256WithRSAEncryption");
        expect_eq(oidName("1.2.840.10045.2.1") ?? "", "ecPublicKey");
        expect_eq(oidName("1.2.840.10045.3.1.7") ?? "", "prime256v1");
        expect_eq(oidName("1.3.101.112") ?? "", "Ed25519");
        expect_eq(oidName("1.3.101.110") ?? "", "X25519");
        expect_eq(oidName("2.16.840.1.101.3.4.2.1") ?? "", "sha256");
        expect_eq(oidName("2.16.840.1.101.3.4.2.3") ?? "", "sha512");
        expect_eq(oidName("2.5.29.17") ?? "", "subjectAltName");
        expect_eq(oidName("2.5.29.19") ?? "", "basicConstraints");
        expect_eq(oidName("1.3.6.1.5.5.7.3.1") ?? "", "serverAuth");
        expect_eq(oidName("1.3.6.1.5.5.7.3.2") ?? "", "clientAuth");
        expect_eq(oidName("1.2.3.4.5") == null, true);
    });
}
