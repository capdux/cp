// cap/test/testCapRawBody.cp
// Comprehensive unit tests for @cap/raw-body (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    RAW_OK,
    RAW_ERR_TOO_LARGE,
    RAW_ERR_LENGTH_MISMATCH,
    RAW_ERR_INVALID_UTF8,
    RAW_ERR_ABORTED,
    RawBodyOptions,
    RawBodyResult,
    RawBodyReader,
    parseByteLimit,
    isValidUtf8
} from "../../packages/raw-body/src/index.cp";

fn main() {
    test("1. RawBodyReader — streaming multi-chunk accumulation & exact Content-Length match", () => {
        let opts = RawBodyOptions {
            limit: 1024,
            expected_len: 17,
            has_expected_len: true,
            validate_utf8: true
        };
        let reader = RawBodyReader.new(opts);
        let out: [128]u8 = undefined;

        expect_eq(reader.checkDeclaredLength(), RAW_OK);
        expect_eq(reader.pushChunk("{\"hello\":", out[0..128]), RAW_OK);
        expect_eq(reader.receivedBytes(), 9 as usize);
        expect_eq(reader.pushChunk("\"world\"}", out[0..128]), RAW_OK);
        expect_eq(reader.receivedBytes(), 17 as usize);

        const res: RawBodyResult = reader.finish(out[0..128]);
        expect_eq(res.status, RAW_OK);
        expect_eq(res.isOk(), true);
        expect_eq(res.bytes_written, 17 as usize);
        expect_eq(out[0..res.bytes_written], "{\"hello\":\"world\"}");
    });

    test("2. RawBodyReader — 413 Payload Too Large on declared length & chunk overflow", () => {
        let bigOpts = RawBodyOptions {
            limit: 64,
            expected_len: 128,
            has_expected_len: true,
            validate_utf8: false
        };
        let r1 = RawBodyReader.new(bigOpts);
        expect_eq(r1.checkDeclaredLength(), RAW_ERR_TOO_LARGE);

        let r2 = RawBodyReader.new(RawBodyOptions.withLimit(10));
        let out: [32]u8 = undefined;
        expect_eq(r2.pushChunk("123456", out[0..32]), RAW_OK);
        expect_eq(r2.pushChunk("78901", out[0..32]), RAW_ERR_TOO_LARGE);
        const res = r2.finish(out[0..32]);
        expect_eq(res.status, RAW_ERR_TOO_LARGE);
        expect_eq(res.bytes_written, 6 as usize);
    });

    test("3. RawBodyReader — 400 Content-Length mismatch & 499 client abort", () => {
        let opts = RawBodyOptions {
            limit: 1024,
            expected_len: 20,
            has_expected_len: true,
            validate_utf8: false
        };
        let r1 = RawBodyReader.new(opts);
        let out: [64]u8 = undefined;
        expect_eq(r1.pushChunk("short body", out[0..64]), RAW_OK);
        const res1 = r1.finish(out[0..64]);
        expect_eq(res1.status, RAW_ERR_LENGTH_MISMATCH);
        expect_eq(res1.bytes_written, 10 as usize);

        let r2 = RawBodyReader.new(RawBodyOptions.defaultOpts());
        expect_eq(r2.pushChunk("partial", out[0..64]), RAW_OK);
        expect_eq(r2.abort(), RAW_ERR_ABORTED);
        expect_eq(r2.pushChunk("more", out[0..64]), RAW_ERR_ABORTED);
        expect_eq(r2.finish(out[0..64]).status, RAW_ERR_ABORTED);
    });

    test("4. RawBodyReader — UTF-8 validation pass & 400 rejection on invalid bytes", () => {
        let opts = RawBodyOptions {
            limit: 256,
            expected_len: 0,
            has_expected_len: false,
            validate_utf8: true
        };
        let out: [64]u8 = undefined;

        // Valid multibyte UTF-8
        let rOk = RawBodyReader.new(opts);
        expect_eq(rOk.pushChunk("你好 CP 🚀", out[0..64]), RAW_OK);
        expect_eq(rOk.finish(out[0..64]).status, RAW_OK);

        // Invalid UTF-8 sequence (0xFF, 0xFE)
        let bad: [3]u8 = [0x61, 0xFF, 0xFE];
        let rBad = RawBodyReader.new(opts);
        expect_eq(rBad.pushChunk(bad[0..3], out[0..64]), RAW_OK);
        expect_eq(rBad.finish(out[0..64]).status, RAW_ERR_INVALID_UTF8);
    });

    test("5. parseByteLimit — parses bytes, kb, mb, gb, fractional & rejects invalid specs", () => {
        const l1 = parseByteLimit("100kb");
        expect_eq(l1 != null, true);
        if (l1 != null) { expect_eq(l1, 102400 as usize); }

        const l2 = parseByteLimit("1MB");
        expect_eq(l2 != null, true);
        if (l2 != null) { expect_eq(l2, 1048576 as usize); }

        const l3 = parseByteLimit("512b");
        expect_eq(l3 != null, true);
        if (l3 != null) { expect_eq(l3, 512 as usize); }

        const l4 = parseByteLimit("1024");
        expect_eq(l4 != null, true);
        if (l4 != null) { expect_eq(l4, 1024 as usize); }

        const l5 = parseByteLimit("1.5kb");
        expect_eq(l5 != null, true);
        if (l5 != null) { expect_eq(l5, 1536 as usize); }

        expect_eq(parseByteLimit("") == null, true);
        expect_eq(parseByteLimit("-10kb") == null, true);
        expect_eq(parseByteLimit("100xb") == null, true);
        expect_eq(parseByteLimit("abc") == null, true);
    });

    test("6. isValidUtf8 — RFC 3629 strict validation (ASCII, CJK, emoji, overlong, surrogates)", () => {
        expect_eq(isValidUtf8("Hello, world!"), true);
        expect_eq(isValidUtf8("中文测试 🚀"), true);
        expect_eq(isValidUtf8(""), true);

        // Overlong 2-byte NUL: 0xC0 0x80
        const overlong: [2]u8 = [0xC0, 0x80];
        expect_eq(isValidUtf8(overlong[0..2]), false);

        // UTF-16 surrogate U+D800: 0xED 0xA0 0x80
        const surrogate: [3]u8 = [0xED, 0xA0, 0x80];
        expect_eq(isValidUtf8(surrogate[0..3]), false);

        // Truncated 3-byte sequence
        const trunc: [2]u8 = [0xE6, 0x8A];
        expect_eq(isValidUtf8(trunc[0..2]), false);

        // Out of Unicode range (> U+10FFFF): 0xF4 0x90 0x80 0x80
        const oob: [4]u8 = [0xF4, 0x90, 0x80, 0x80];
        expect_eq(isValidUtf8(oob[0..4]), false);
    });
}
