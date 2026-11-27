// cap/test/testCapSec1.cp
// Comprehensive unit tests for @cap/sec1 (SEC1 v2.0 & RFC 5915)

import { test, expect_eq } from "cap:test";
import { OID_PRIME256V1, ObjectIdentifier } from "../../packages/const-oid/src/index.cp";
import {
    SEC1_IDENTITY,
    SEC1_COMPRESSED_EVEN,
    SEC1_COMPRESSED_ODD,
    SEC1_UNCOMPRESSED,
    SEC1_P256_COORD_LEN,
    SEC1_P384_COORD_LEN,
    SEC1_ERR_INVALID_POINT_TAG,
    SEC1_ERR_INVALID_POINT_LEN,
    SEC1_ERR_INVALID_VERSION,
    SEC1_ERR_INVALID_PEM_LABEL,
    PointResult,
    EncodedPoint,
    makeEcPrivateKey,
    encodeEcPrivateKeyDer,
    decodeEcPrivateKeyDer,
    encodeEcPrivateKeyPem,
    decodeEcPrivateKeyPem,
} from "../../packages/sec1/src/index.cp";

fn tBytesEq(a: []const u8, b: []const u8): bool {
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

fn allZero(a: []const u8): bool {
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != 0) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/sec1 — P-256 uncompressed 0x04||X||Y encode, parse & compress()", () => {
        let x: [32]u8 = undefined;
        let y_odd: [32]u8 = undefined;
        let y_even: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            x[i] = (0x10 + i) as u8;
            y_odd[i] = (0x50 + i) as u8;
            y_even[i] = (0x50 + i) as u8;
            i = i + 1;
        }
        y_odd[31] = 0x7F;  // odd Y -> 0x03
        y_even[31] = 0x7E; // even Y -> 0x02

        const p_odd = EncodedPoint.fromUncompressed(x[0..32], y_odd[0..32]);
        expect_eq(p_odd.ok, true);
        expect_eq(p_odd.point.tag(), SEC1_UNCOMPRESSED);
        expect_eq(p_odd.point.len, 65 as usize);
        expect_eq(p_odd.point.coord_size, 32 as usize);
        expect_eq(p_odd.point.isCompressed(), false);
        expect_eq(p_odd.point.isIdentity(), false);

        let rx: [32]u8 = undefined;
        let ry: [32]u8 = undefined;
        expect_eq(p_odd.point.xBytes(rx[0..32]), 32 as usize);
        expect_eq(p_odd.point.yBytes(ry[0..32]), 32 as usize);
        expect_eq(tBytesEq(rx[0..32], x[0..32]), true);
        expect_eq(tBytesEq(ry[0..32], y_odd[0..32]), true);

        // Compress odd Y -> tag 0x03, len 33
        const c_odd: PointResult = p_odd.point.compress();
        expect_eq(c_odd.ok, true);
        expect_eq(c_odd.point.tag(), SEC1_COMPRESSED_ODD);
        expect_eq(c_odd.point.len, 33 as usize);
        expect_eq(c_odd.point.isCompressed(), true);
        expect_eq(c_odd.point.xBytes(rx[0..32]), 32 as usize);
        expect_eq(tBytesEq(rx[0..32], x[0..32]), true);
        expect_eq(c_odd.point.yBytes(ry[0..32]), 0 as usize);

        // Compress even Y -> tag 0x02, len 33
        const p_even = EncodedPoint.fromUncompressed(x[0..32], y_even[0..32]);
        const c_even: PointResult = p_even.point.compress();
        expect_eq(c_even.ok, true);
        expect_eq(c_even.point.tag(), SEC1_COMPRESSED_EVEN);
        expect_eq(c_even.point.len, 33 as usize);
    });

    test("2. @cap/sec1 — P-384 compressed points 0x02 / 0x03 encode & parse", () => {
        let x384: [48]u8 = undefined;
        let i: usize = 0;
        while (i < 48) {
            x384[i] = ((i * 3 + 1) & 0xFF) as u8;
            i = i + 1;
        }

        const c_even = EncodedPoint.fromCompressed(x384[0..48], false);
        expect_eq(c_even.ok, true);
        expect_eq(c_even.point.tag(), SEC1_COMPRESSED_EVEN);
        expect_eq(c_even.point.len, 49 as usize);
        expect_eq(c_even.point.coord_size, SEC1_P384_COORD_LEN);

        let raw: [49]u8 = undefined;
        expect_eq(c_even.point.toBytes(raw[0..49]), 49 as usize);
        expect_eq(raw[0], SEC1_COMPRESSED_EVEN);

        const parsed_even = EncodedPoint.fromBytesForCoord(raw[0..49], SEC1_P384_COORD_LEN);
        expect_eq(parsed_even.ok, true);
        expect_eq(parsed_even.point.eq(c_even.point), true);

        const c_odd = EncodedPoint.fromCompressed(x384[0..48], true);
        expect_eq(c_odd.ok, true);
        expect_eq(c_odd.point.tag(), SEC1_COMPRESSED_ODD);
        expect_eq(c_odd.point.toBytes(raw[0..49]), 49 as usize);
        const parsed_odd = EncodedPoint.fromBytesForCoord(raw[0..49], SEC1_P384_COORD_LEN);
        expect_eq(parsed_odd.ok, true);
        expect_eq(parsed_odd.point.eq(c_odd.point), true);
    });

    test("3. @cap/sec1 — Point at infinity (identity 0x00) handling", () => {
        const id = EncodedPoint.identity();
        expect_eq(id.isIdentity(), true);
        expect_eq(id.isCompressed(), false);
        expect_eq(id.tag(), SEC1_IDENTITY);
        expect_eq(id.len, 1 as usize);
        expect_eq(id.coord_size, 0 as usize);

        let b: [4]u8 = [0x00, 0x00, 0x00, 0x00];
        const parsed = EncodedPoint.fromBytes(b[0..1]);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.point.isIdentity(), true);
        expect_eq(parsed.point.eq(id), true);

        // Compressing identity yields identity
        const cid: PointResult = id.compress();
        expect_eq(cid.ok, true);
        expect_eq(cid.point.isIdentity(), true);

        // Trailing byte after 0x00 rejected
        expect_eq(EncodedPoint.fromBytes(b[0..2]).ok, false);
    });

    test("4. @cap/sec1 — RFC 5915 EcPrivateKey DER round-trip with prime256v1 OID & public key", () => {
        let priv32: [32]u8 = undefined;
        let x: [32]u8 = undefined;
        let y: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            priv32[i] = (0xA0 + i) as u8;
            x[i] = (0x20 + i) as u8;
            y[i] = (0x60 + i) as u8;
            i = i + 1;
        }

        const curve_oid = ObjectIdentifier.fromStr(OID_PRIME256V1).oid;
        const pub_pt = EncodedPoint.fromUncompressed(x[0..32], y[0..32]).point;
        const mk = makeEcPrivateKey(priv32[0..32], true, curve_oid, true, pub_pt);
        expect_eq(mk.ok, true);

        let der: [256]u8 = undefined;
        const dlen = encodeEcPrivateKeyDer(mk.key, der[0..256]);
        // version(3) + priv(2+32=34) + [0](2 + 10 = 12) + [1](2 + 2 + 1 + 65 = 70) = 119 content -> 30 77 (121 bytes)
        expect_eq(dlen, 121 as i64);
        expect_eq(der[0], 0x30 as u8);
        expect_eq(der[1], 119 as u8);
        expect_eq(der[2], 0x02 as u8);
        expect_eq(der[3], 0x01 as u8);
        expect_eq(der[4], 0x01 as u8);

        const dec_res = decodeEcPrivateKeyDer(der[0..(dlen as usize)]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.key.priv_len, 32 as usize);
        expect_eq(tBytesEq(dec_res.key.priv_key[0..32], priv32[0..32]), true);
        expect_eq(dec_res.key.has_curve, true);
        expect_eq(dec_res.key.curve_oid.eq(curve_oid), true);
        expect_eq(dec_res.key.has_pub_key, true);
        expect_eq(dec_res.key.pub_key.eq(pub_pt), true);
    });

    test("5. @cap/sec1 — EC PRIVATE KEY PEM round-trip & zeroize()", () => {
        let priv32: [32]u8 = undefined;
        let x: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            priv32[i] = (0x33 + i) as u8;
            x[i] = (0x77 + i) as u8;
            i = i + 1;
        }
        const curve_oid = ObjectIdentifier.fromStr(OID_PRIME256V1).oid;
        const pub_pt = EncodedPoint.fromCompressed(x[0..32], true).point;
        const mk = makeEcPrivateKey(priv32[0..32], true, curve_oid, true, pub_pt);
        expect_eq(mk.ok, true);

        let pem_buf: [512]u8 = undefined;
        const plen = encodeEcPrivateKeyPem(mk.key, false, pem_buf[0..512]);
        expect_eq(plen > (60 as i64), true);

        // Verify header starts with "-----BEGIN EC PRIVATE KEY-----"
        const hdr = "-----BEGIN EC PRIVATE KEY-----";
        let hdr_ok = true;
        i = 0;
        while (i < hdr.len) {
            if (pem_buf[i] != hdr[i]) {
                hdr_ok = false;
            }
            i = i + 1;
        }
        expect_eq(hdr_ok, true);

        // Decode PEM back to EcPrivateKey
        let dec_res = decodeEcPrivateKeyPem(pem_buf[0..(plen as usize)]);
        expect_eq(dec_res.ok, true);
        expect_eq(dec_res.key.priv_len, 32 as usize);
        expect_eq(tBytesEq(dec_res.key.priv_key[0..32], priv32[0..32]), true);
        expect_eq(dec_res.key.curve_oid.eq(curve_oid), true);
        expect_eq(dec_res.key.pub_key.eq(pub_pt), true);

        // Zeroize private key material and verify all bytes cleared
        dec_res.key.zeroize();
        expect_eq(dec_res.key.priv_len, 0 as usize);
        expect_eq(allZero(dec_res.key.priv_key[0..66]), true);
        expect_eq(allZero(dec_res.key.pub_key.buf[0..133]), true);
    });

    test("6. @cap/sec1 — Reject invalid point prefix 0x05, wrong length, non-v1 version & wrong PEM label", () => {
        // Invalid point prefix 0x05
        let bad_pt: [33]u8 = undefined;
        bad_pt[0] = 0x05;
        let i: usize = 1;
        while (i < 33) {
            bad_pt[i] = i as u8;
            i = i + 1;
        }
        const r_tag = EncodedPoint.fromBytes(bad_pt[0..33]);
        expect_eq(r_tag.ok, false);
        expect_eq(r_tag.err, SEC1_ERR_INVALID_POINT_TAG);

        // Wrong length for P-256 (32 bytes with 0x02 prefix -> coord_size 31 != 32)
        bad_pt[0] = SEC1_COMPRESSED_EVEN;
        const r_len = EncodedPoint.fromBytesForCoord(bad_pt[0..32], SEC1_P256_COORD_LEN);
        expect_eq(r_len.ok, false);
        expect_eq(r_len.err, SEC1_ERR_INVALID_POINT_LEN);

        // Non-v1 version in RFC 5915 DER (version = 2 -> 02 01 02)
        let bad_ver: [9]u8 = [0x30, 0x07, 0x02, 0x01, 0x02, 0x04, 0x02, 0xAA, 0xBB];
        const r_ver = decodeEcPrivateKeyDer(bad_ver[0..9]);
        expect_eq(r_ver.ok, false);
        expect_eq(r_ver.err, SEC1_ERR_INVALID_VERSION);

        // Wrong PEM label ("PRIVATE KEY" instead of "EC PRIVATE KEY")
        const wrong_pem = "-----BEGIN PRIVATE KEY-----\nMAcCAQEEAqur\n-----END PRIVATE KEY-----\n";
        const r_pem = decodeEcPrivateKeyPem(wrong_pem);
        expect_eq(r_pem.ok, false);
        expect_eq(r_pem.err, SEC1_ERR_INVALID_PEM_LABEL);
    });
}
