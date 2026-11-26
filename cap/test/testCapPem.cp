// cap/test/testCapPem.cp
// Comprehensive unit tests for @cap/pem

import { test, expect_eq } from "cap:test";
import {
    PemBlock,
    PemBundle,
    parsePem,
    parsePemBundle,
    extractBase64,
    encodePem,
} from "../../packages/@cap/pem/src/index.cp";

fn main() {
    test("1. @cap/pem — parse single PEM block", () => {
        const sample: string = "-----BEGIN PUBLIC KEY-----\nMCowBQYDK2VwAyEA\nYWJjZGVm\n-----END PUBLIC KEY-----\n";
        const blk: PemBlock = parsePem(sample);
        expect_eq(blk.valid, true);
        expect_eq(blk.label, "PUBLIC KEY");

        let b64: [128]u8 = undefined;
        expect_eq(extractBase64(blk, b64[0..128]), "MCowBQYDK2VwAyEAYWJjZGVm");
    });

    test("2. @cap/pem — parse multi-block certificate chain bundle", () => {
        const chain: string = "Leading comment\n-----BEGIN CERTIFICATE-----\nQUFBQQ==\n-----END CERTIFICATE-----\n\n-----BEGIN CERTIFICATE-----\nQkJCQg==\n-----END CERTIFICATE-----\n";
        const bundle: PemBundle = parsePemBundle(chain);
        expect_eq(bundle.count, 2 as usize);
        expect_eq(bundle.blocks[0].label, "CERTIFICATE");
        expect_eq(bundle.blocks[1].label, "CERTIFICATE");

        let b64: [64]u8 = undefined;
        expect_eq(extractBase64(bundle.blocks[0], b64[0..64]), "QUFBQQ==");
        expect_eq(extractBase64(bundle.blocks[1], b64[0..64]), "QkJCQg==");
    });

    test("3. @cap/pem — strip RFC 1421 headers when extracting Base64", () => {
        const encKey: string = "-----BEGIN RSA PRIVATE KEY-----\nProc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,0123456789ABCDEF\n\nSGVsbG8=\n-----END RSA PRIVATE KEY-----";
        const blk: PemBlock = parsePem(encKey);
        expect_eq(blk.valid, true);
        expect_eq(blk.label, "RSA PRIVATE KEY");

        let b64: [64]u8 = undefined;
        expect_eq(extractBase64(blk, b64[0..64]), "SGVsbG8=");
    });

    test("4. @cap/pem — encodePem wraps at 64 columns", () => {
        const payload70: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/ABCDEF";
        let out: [256]u8 = undefined;
        const encoded: string = encodePem("DATA", payload70, out[0..256]);
        const expected: string = "-----BEGIN DATA-----\nABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\nABCDEF\n-----END DATA-----\n";
        expect_eq(encoded, expected);
    });

    test("5. @cap/pem — round-trip encode & parse", () => {
        let out: [256]u8 = undefined;
        const pem: string = encodePem("OPENSSH PRIVATE KEY", "b3BlbnNzaC1rZXktdjE=", out[0..256]);
        const blk: PemBlock = parsePem(pem);
        expect_eq(blk.valid, true);
        expect_eq(blk.label, "OPENSSH PRIVATE KEY");

        let b64: [64]u8 = undefined;
        expect_eq(extractBase64(blk, b64[0..64]), "b3BlbnNzaC1rZXktdjE=");
    });

    test("6. @cap/pem — reject mismatched or malformed PEM", () => {
        expect_eq(parsePem("not a pem block").valid, false);
        expect_eq(parsePem("-----BEGIN CERT-----\nQUFB\n-----END OTHER-----").valid, false);
        expect_eq(parsePem("-----BEGIN lower-----\nQUFB\n-----END lower-----").valid, false);
    });
}
