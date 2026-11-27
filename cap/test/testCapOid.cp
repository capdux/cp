// cap/test/testCapOid.cp
// Comprehensive unit tests for @cap/const-oid (X.690 ASN.1 ObjectIdentifier)

import { test, expect_eq } from "cap:test";
import {
    OID_SHA256_WITH_RSA,
    OID_COMMON_NAME,
    ObjectIdentifier,
} from "../../packages/const-oid/src/index.cp";

fn oidStrEq(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/const-oid — fromStr, toStr, toDer & fromDer round-trip", () => {
        const r = ObjectIdentifier.fromStr("1.2.840.113549.1.1.11");
        expect_eq(r.ok, true);
        expect_eq(r.oid.arcCount(), 7 as usize);
        expect_eq(r.oid.arcAt(0), 1 as u32);
        expect_eq(r.oid.arcAt(2), 840 as u32);
        expect_eq(r.oid.arcAt(6), 11 as u32);

        let sbuf: [64]u8 = [0; 64];
        const sn: usize = r.oid.toStr(sbuf[0..64]);
        expect_eq(oidStrEq(sbuf[0..sn], "1.2.840.113549.1.1.11"), true);

        let dbuf: [32]u8 = [0; 32];
        const dn: usize = r.oid.toDer(dbuf[0..32]);
        expect_eq(dn, 9 as usize);
        expect_eq(dbuf[0], 0x2A as u8);
        expect_eq(dbuf[8], 0x0B as u8);

        const dec_res = ObjectIdentifier.fromDer(dbuf[0..dn]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.oid.eq(r.oid), true);
    });

    test("2. @cap/const-oid — well-known OIDs & invalid rejection", () => {
        const rsa = ObjectIdentifier.fromStr(OID_SHA256_WITH_RSA);
        const cn = ObjectIdentifier.fromStr(OID_COMMON_NAME);
        expect_eq(rsa.ok, true);
        expect_eq(cn.ok, true);
        expect_eq(rsa.oid.eq(cn.oid), false);

        expect_eq(ObjectIdentifier.fromStr("").ok, false);
        expect_eq(ObjectIdentifier.fromStr("3.1.2").ok, false);
        expect_eq(ObjectIdentifier.fromStr("1.40.2").ok, false);
    });
}
