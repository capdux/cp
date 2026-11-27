import { test, expect_eq } from "cap:test";
import {
    SHA256_DIGEST_LEN, SHA224_DIGEST_LEN,
    Sha256, Sha224, sha256Hex, sha224Hex
} from "../../packages/sha2/src/index.cp";

fn main() {
    test("1. @cap/sha2-crate — SHA-256 FIPS 180-4 vectors for '' and 'abc'", () => {
        let hex: [64]u8 = [0; 64];
        expect_eq(sha256Hex("", hex[0..64]), 64);
        expect_eq(hex[0..64] as string, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

        expect_eq(sha256Hex("abc", hex[0..64]), 64);
        expect_eq(hex[0..64] as string, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    });

    test("2. @cap/sha2-crate — SHA-224 FIPS 180-4 vectors for '' and 'abc'", () => {
        let hex: [56]u8 = [0; 56];
        expect_eq(sha224Hex("", hex[0..56]), 56);
        expect_eq(hex[0..56] as string, "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f");

        expect_eq(sha224Hex("abc", hex[0..56]), 56);
        expect_eq(hex[0..56] as string, "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7");
    });

    test("3. @cap/sha2-crate — SHA-256 56-byte multi-block vector", () => {
        let hex: [64]u8 = [0; 64];
        * = sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", hex[0..64]);
        expect_eq(hex[0..64] as string, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    });

    test("4. @cap/sha2-crate — streaming chunked update & reset()", () => {
        let h = Sha256.new();
        h.updateStr("a");
        h.updateStr("b");
        h.updateStr("c");
        let hex: [64]u8 = [0; 64];
        * = h.finalizeHex(hex[0..64]);
        expect_eq(hex[0..64] as string, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

        h.reset();
        * = h.finalizeHex(hex[0..64]);
        expect_eq(hex[0..64] as string, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    });

    test("5. @cap/sha2-crate — SHA-224 56-byte multi-block vector", () => {
        let hex: [56]u8 = [0; 56];
        * = sha224Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", hex[0..56]);
        expect_eq(hex[0..56] as string, "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525");
    });

    test("6. @cap/sha2-crate — raw digest lengths & short output buffer guard", () => {
        expect_eq(SHA256_DIGEST_LEN, 32);
        expect_eq(SHA224_DIGEST_LEN, 28);
        let tiny: [8]u8 = [0; 8];
        expect_eq(sha256Hex("abc", tiny[0..8]), 0);
        expect_eq(sha224Hex("abc", tiny[0..8]), 0);
    });
}
