// cap/test/testCapPemParse.cp
// Comprehensive unit tests for @cap/pem-parse (RFC 7468 PEM codec)

import { test, expect_eq } from "cap:test";
import {
    PemBlock,
    isValidLabel,
    nextBlock,
    countBlocks,
    decodeBase64Body,
    encodePem,
} from "../../packages/@cap/pem-parse/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/pem-parse — single PEM block parsing", () => {
        const pem = "-----BEGIN CERTIFICATE-----\nSGVsbG8=\n-----END CERTIFICATE-----\n";
        const blk: PemBlock = nextBlock(pem, 0);
        expect_eq(blk.valid, true);
        expect_eq(sliceEq(blk.label, "CERTIFICATE"), true);

        let der: [32]u8 = undefined;
        const dlen: i64 = decodeBase64Body(blk.body_b64, der[0..32]);
        expect_eq(dlen, 5 as i64);
        expect_eq(sliceEq(der[0..5], "Hello"), true);
    });

    test("2. @cap/pem-parse — multi-block chain countBlocks & sequential nextBlock", () => {
        const chain = "note\n-----BEGIN BLOCK ONE-----\nQUJD\n-----END BLOCK ONE-----\n\n-----BEGIN BLOCK TWO-----\nREVGRw==\n-----END BLOCK TWO-----\n";
        expect_eq(countBlocks(chain), 2 as usize);

        const b1: PemBlock = nextBlock(chain, 0);
        expect_eq(b1.valid, true);
        expect_eq(sliceEq(b1.label, "BLOCK ONE"), true);

        const b2: PemBlock = nextBlock(chain, b1.next_pos);
        expect_eq(b2.valid, true);
        expect_eq(sliceEq(b2.label, "BLOCK TWO"), true);

        const b3: PemBlock = nextBlock(chain, b2.next_pos);
        expect_eq(b3.valid, false);
    });

    test("3. @cap/pem-parse — decodeBase64Body ignores CRLF, LF & spaces", () => {
        const body = "  SGVs\r\nbG8g\nV29y\t bGQh  \r\n";
        let der: [32]u8 = undefined;
        const n: i64 = decodeBase64Body(body, der[0..32]);
        expect_eq(n, 12 as i64);
        expect_eq(sliceEq(der[0..12], "Hello World!"), true);
    });

    test("4. @cap/pem-parse — encodePem 64-column line wrap & parse round-trip", () => {
        // 60 bytes -> 80 Base64 chars -> wraps into 64 + 16 chars
        let payload: [60]u8 = undefined;
        let i: usize = 0;
        while (i < 60) {
            payload[i] = (i * 3 + 1) as u8;
            i += 1;
        }
        let pem_buf: [256]u8 = undefined;
        const n: usize = encodePem("PUBLIC KEY", payload[0..60], pem_buf[0..256]);
        expect_eq(n > 0, true);

        const blk: PemBlock = nextBlock(pem_buf[0..n], 0);
        expect_eq(blk.valid, true);
        expect_eq(sliceEq(blk.label, "PUBLIC KEY"), true);
        // Verify 64-column newline exists at index 64 of body_b64
        expect_eq(blk.body_b64[64], '\n' as u8);

        let decoded: [64]u8 = undefined;
        const dlen: i64 = decodeBase64Body(blk.body_b64, decoded[0..64]);
        expect_eq(dlen, 60 as i64);
        expect_eq(sliceEq(decoded[0..60], payload[0..60]), true);
    });

    test("5. @cap/pem-parse — rejects mismatched BEGIN/END labels & invalid labels", () => {
        const bad = "-----BEGIN FOO-----\nQUJD\n-----END BAR-----\n";
        expect_eq(nextBlock(bad, 0).valid, false);
        expect_eq(countBlocks(bad), 0 as usize);

        expect_eq(isValidLabel("RSA PRIVATE KEY"), true);
        expect_eq(isValidLabel("X509-CERT"), true);
        expect_eq(isValidLabel(""), false);
        expect_eq(isValidLabel("lowercase"), false);
        expect_eq(isValidLabel("BAD!LABEL"), false);
    });

    test("6. @cap/pem-parse — rejects invalid Base64 & tiny buffers", () => {
        let out: [16]u8 = undefined;
        expect_eq(decodeBase64Body("!!!bad!!!", out[0..16]), -1 as i64);
        expect_eq(decodeBase64Body("QUJ", out[0..16]), -1 as i64);
        // Output buffer too small
        let tiny: [2]u8 = undefined;
        expect_eq(decodeBase64Body("SGVsbG8=", tiny[0..2]), -1 as i64);
        // encodePem tiny buffer or invalid label
        expect_eq(encodePem("TEST", "Hello", tiny[0..2]), 0 as usize);
        expect_eq(encodePem("bad_label", "Hello", out[0..16]), 0 as usize);
    });
}
