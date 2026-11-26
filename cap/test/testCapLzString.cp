import { test, expect_eq } from "cap:test";
import {
    compressBytes,
    decompressBytes,
    compressToBase64,
    decompressFromBase64,
    compressToEncodedURIComponent,
    decompressFromEncodedURIComponent,
} from "../../packages/@cap/lz-string/src/index.cp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn main() {
    test("1. @cap/lz-string — Basic compressBytes / decompressBytes round-trip & ratio", () => {
        const text = "TOBEORNOTTOBEORTOBEORNOT! The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog.";
        let comp: [1024]u8 = undefined;
        let decoded: [1024]u8 = undefined;

        const clen = compressBytes(text, comp[0..]);
        expect_eq(clen > 0, true);
        expect_eq(clen < text.len, true);

        const dlen = decompressBytes(comp[0..clen], decoded[0..]);
        expect_eq(dlen, i64(text.len));
        expect_eq(bytesEq(decoded[0..usize(dlen)], text), true);
    });

    test("2. @cap/lz-string — Empty & single-byte & all-256-byte binary round-trip", () => {
        let comp: [1024]u8 = undefined;
        let decoded: [1024]u8 = undefined;

        // Empty
        expect_eq(compressBytes("", comp[0..]), 0);
        expect_eq(decompressBytes("", decoded[0..]), 0);

        // Single byte
        const single = "Z";
        const c1 = compressBytes(single, comp[0..]);
        expect_eq(c1 > 0, true);
        const d1 = decompressBytes(comp[0..c1], decoded[0..]);
        expect_eq(d1, 1);
        expect_eq(decoded[0], 'Z');

        // All 256 byte values (0x00..0xFF)
        let bin: [512]u8 = undefined;
        let i: usize = 0;
        while (i < 512) : (i += 1) {
            bin[i] = u8(i & 0xFF);
        }
        const c2 = compressBytes(bin[0..], comp[0..]);
        expect_eq(c2 > 0, true);
        const d2 = decompressBytes(comp[0..c2], decoded[0..]);
        expect_eq(d2, 512);
        expect_eq(bytesEq(decoded[0..512], bin[0..]), true);
    });

    test("3. @cap/lz-string — Large stream exercising 9->10->11->12-bit & 4096 dict reset", () => {
        let src: [8192]u8 = undefined;
        let state: u32 = 0x1234_5678;
        let i: usize = 0;
        while (i < 8192) : (i += 1) {
            // Mix pseudo-random and repetitive segments to force >4096 dict entries
            if ((i & 31) < 20) {
                state = state *% 1664525 +% 1013904223;
                src[i] = u8((state >> 24) & 0xFF);
            } else {
                src[i] = u8('A' + (i % 7));
            }
        }

        let comp: [16384]u8 = undefined;
        let decoded: [8192]u8 = undefined;
        const clen = compressBytes(src[0..], comp[0..]);
        expect_eq(clen > 0, true);

        const dlen = decompressBytes(comp[0..clen], decoded[0..]);
        expect_eq(dlen, 8192);
        expect_eq(bytesEq(decoded[0..8192], src[0..]), true);
    });

    test("4. @cap/lz-string — compressToBase64 / decompressFromBase64", () => {
        const msg = "{\"agent\":\"cap\",\"status\":\"ready\",\"items\":[\"alpha\",\"alpha\",\"alpha\",\"beta\",\"beta\"]}";
        let b64: [1024]u8 = undefined;
        let decoded: [1024]u8 = undefined;

        const blen = compressToBase64(msg, b64[0..]);
        expect_eq(blen > 0, true);
        expect_eq((blen & 3), 0);

        const dlen = decompressFromBase64(b64[0..blen], decoded[0..]);
        expect_eq(dlen, i64(msg.len));
        expect_eq(bytesEq(decoded[0..usize(dlen)], msg), true);

        // Empty Base64
        expect_eq(compressToBase64("", b64[0..]), 0);
        expect_eq(decompressFromBase64("", decoded[0..]), 0);
    });

    test("5. @cap/lz-string — compressToEncodedURIComponent / decompressFromEncodedURIComponent", () => {
        const msg = "Hello, 世界! URL-safe LZW test: foo=bar&baz=qux/12345 foo=bar&baz=qux/12345";
        let uri: [1024]u8 = undefined;
        let decoded: [1024]u8 = undefined;

        const ulen = compressToEncodedURIComponent(msg, uri[0..]);
        expect_eq(ulen > 0, true);

        // Verify every char belongs to A-Za-z0-9+-$
        let ok_alpha = true;
        let i: usize = 0;
        while (i < ulen) : (i += 1) {
            const ch = uri[i];
            const is_ok = (ch >= 'A' && ch <= 'Z') ||
                (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') ||
                ch == '+' || ch == '-' || ch == '$';
            if (!is_ok) {
                ok_alpha = false;
            }
        }
        expect_eq(ok_alpha, true);

        const dlen = decompressFromEncodedURIComponent(uri[0..ulen], decoded[0..]);
        expect_eq(dlen, i64(msg.len));
        expect_eq(bytesEq(decoded[0..usize(dlen)], msg), true);
    });

    test("6. @cap/lz-string — Invalid/truncated streams & small buffer safely return -1", () => {
        let decoded: [64]u8 = undefined;

        // Truncated LZW stream (1 byte cannot hold 9-bit code + EOF)
        const trunc: [1]u8 = [0x41];
        expect_eq(decompressBytes(trunc[0..], decoded[0..]), -1);

        // Corrupt out-of-range LZW code (0xFF, 0xFF => 9-bit code 511 > next_code 258)
        const bad_code: [3]u8 = [0xFF, 0xFF, 0xFF];
        expect_eq(decompressBytes(bad_code[0..], decoded[0..]), -1);

        // Invalid Base64 characters / bad length
        expect_eq(decompressFromBase64("!!!not-base64!!!", decoded[0..]), -1);
        expect_eq(decompressFromBase64("abc", decoded[0..]), -1);

        // Invalid URI characters ('/' and '=' not allowed in URI alphabet)
        expect_eq(decompressFromEncodedURIComponent("ab/cd=ef", decoded[0..]), -1);

        // Output buffer too small
        const big = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        let comp: [128]u8 = undefined;
        let tiny: [4]u8 = undefined;
        const clen = compressBytes(big, comp[0..]);
        expect_eq(clen > 0, true);
        expect_eq(decompressBytes(comp[0..clen], tiny[0..]), -1);
    });
}
