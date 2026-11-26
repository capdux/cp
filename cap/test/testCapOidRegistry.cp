import { test, expect_eq } from "cap:test";
import {
    OidEntry,
    lookupOid,
    lookupShortName,
    oidEntryCount,
    encodeOidDer,
    decodeOidDer,
} from "../../packages/@cap/oid-registry/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) { return false; }
    }
    return true;
}

fn main() {
    test("1. @cap/oid-registry — entry count >= 30 & X.500/X.509 lookups", () => {
        expect_eq(oidEntryCount() >= 30, true);

        if (cn := lookupOid("2.5.4.3")) {
            expect_eq(eqStr(cn.short_name, "CN"), true);
            expect_eq(eqStr(cn.category, "X.500"), true);
        } else {
            expect_eq(true, false);
        }

        if (san := lookupOid("2.5.29.17")) {
            expect_eq(eqStr(san.short_name, "subjectAltName"), true);
            expect_eq(eqStr(san.category, "X.509"), true);
        } else {
            expect_eq(true, false);
        }

        expect_eq(lookupOid("9.9.9.9") == null, true);
    });

    test("2. @cap/oid-registry — lookupShortName for RSA, EC, Ed25519 & NIST AES-GCM", () => {
        if (rsa := lookupShortName("rsaEncryption")) {
            expect_eq(eqStr(rsa.oid, "1.2.840.113549.1.1.1"), true);
        } else {
            expect_eq(true, false);
        }

        if (ed := lookupShortName("Ed25519")) {
            expect_eq(eqStr(ed.oid, "1.3.101.112"), true);
        } else {
            expect_eq(true, false);
        }

        if (gcm := lookupShortName("aes256-GCM")) {
            expect_eq(eqStr(gcm.oid, "2.16.840.1.101.3.4.1.46"), true);
        } else {
            expect_eq(true, false);
        }

        expect_eq(lookupShortName("nonexistent") == null, true);
    });

    test("3. @cap/oid-registry — encodeOidDer exact DER bytes for CN & rsaEncryption", () => {
        let der: [32]u8 = undefined;

        // 2.5.4.3 -> 0x55 0x04 0x03
        const n1 = encodeOidDer("2.5.4.3", der[0..32]);
        expect_eq(n1, 3);
        expect_eq(der[0], 0x55);
        expect_eq(der[1], 0x04);
        expect_eq(der[2], 0x03);

        // 1.2.840.113549.1.1.1 -> 2A 86 48 86 F7 0D 01 01 01
        const n2 = encodeOidDer("1.2.840.113549.1.1.1", der[0..32]);
        expect_eq(n2, 9);
        expect_eq(der[0], 0x2A);
        expect_eq(der[1], 0x86);
        expect_eq(der[2], 0x48);
        expect_eq(der[3], 0x86);
        expect_eq(der[4], 0xF7);
        expect_eq(der[5], 0x0D);
        expect_eq(der[6], 0x01);
        expect_eq(der[7], 0x01);
        expect_eq(der[8], 0x01);
    });

    test("4. @cap/oid-registry — decodeOidDer exact roundtrip for complex OIDs", () => {
        let der: [32]u8 = undefined;
        let txt: [64]u8 = undefined;

        const o1 = "1.2.840.113549.1.1.11";
        const n1 = encodeOidDer(o1, der[0..32]);
        expect_eq(n1, 9);
        const m1 = decodeOidDer(der[0..n1], txt[0..64]);
        expect_eq(eqStr(txt[0..m1], o1), true);

        const o2 = "2.16.840.1.101.3.4.2.1";
        const n2 = encodeOidDer(o2, der[0..32]);
        expect_eq(n2, 9);
        const m2 = decodeOidDer(der[0..n2], txt[0..64]);
        expect_eq(eqStr(txt[0..m2], o2), true);
    });

    test("5. @cap/oid-registry — joint-iso-itu-t large second arc (2.999.3) DER codec", () => {
        let der: [32]u8 = undefined;
        let txt: [64]u8 = undefined;

        // 2.999.3 -> first = 2*40 + 999 = 1079 = (8 << 7) | 55 -> 0x88 0x37 0x03
        const n = encodeOidDer("2.999.3", der[0..32]);
        expect_eq(n, 3);
        expect_eq(der[0], 0x88);
        expect_eq(der[1], 0x37);
        expect_eq(der[2], 0x03);

        const m = decodeOidDer(der[0..n], txt[0..64]);
        expect_eq(eqStr(txt[0..m], "2.999.3"), true);
    });

    test("6. @cap/oid-registry — malformed OID strings & non-canonical DER rejection", () => {
        let der: [32]u8 = undefined;
        let txt: [64]u8 = undefined;

        expect_eq(encodeOidDer("", der[0..32]), 0);
        expect_eq(encodeOidDer("1", der[0..32]), 0);
        expect_eq(encodeOidDer("3.1.2", der[0..32]), 0);
        expect_eq(encodeOidDer("1.40.2", der[0..32]), 0);
        expect_eq(encodeOidDer("1.2..3", der[0..32]), 0);
        expect_eq(encodeOidDer("1.02.3", der[0..32]), 0);

        // Non-canonical leading 0x80 & truncated continuation byte
        const bad1: [2]u8 = [0x80, 0x01];
        expect_eq(decodeOidDer(bad1[0..2], txt[0..64]), 0);
        const bad2: [2]u8 = [0x2A, 0x86];
        expect_eq(decodeOidDer(bad2[0..2], txt[0..64]), 0);
    });
}
