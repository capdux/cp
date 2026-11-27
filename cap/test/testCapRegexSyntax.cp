// cap/test/testCapRegexSyntax.cp
// Comprehensive 6-test suite for @cap/regex-syntax (Rust regex-syntax v0.8 parity)

import { test, expect_eq } from "cap:test";
import {
    RSX_OK,
    RSX_ERR_UNCLOSED_GROUP,
    RSX_ERR_UNOPENED_GROUP,
    RSX_ERR_UNCLOSED_CLASS,
    RSX_ERR_INVALID_CLASS_RANGE,
    RSX_ERR_DANGLING_QUANTIFIER,
    RSX_ERR_DOUBLE_QUANTIFIER,
    RSX_ERR_INVALID_QUANTIFIER,
    RSX_ERR_TRAILING_BACKSLASH,
    RegexSyntaxInfo,
    isMetaCharacter,
    escapeRegex,
    isWordByte,
    extractLiteralPrefix,
    validateRegexSyntax
} from "../../packages/regex-syntax/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
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
    test("1. @cap/regex-syntax — isMetaCharacter, isWordByte & escapeRegex full metacharacter set", () => {
        let metas = "\\.+*?()|[]{}^$#&-~";
        let i: usize = 0;
        while (i < metas.len) {
            expect_eq(isMetaCharacter(metas[i]), true);
            i = i + 1;
        }
        expect_eq(isMetaCharacter(0x61), false); // 'a'
        expect_eq(isMetaCharacter(0x30), false); // '0'
        expect_eq(isMetaCharacter(0x5F), false); // '_'
        expect_eq(isMetaCharacter(0x2F), false); // '/'

        expect_eq(isWordByte(0x41), true);  // 'A'
        expect_eq(isWordByte(0x7A), true);  // 'z'
        expect_eq(isWordByte(0x35), true);  // '5'
        expect_eq(isWordByte(0x5F), true);  // '_'
        expect_eq(isWordByte(0x2D), false); // '-'

        let out: [64]u8 = [0; 64];
        let n = escapeRegex("a+b.(c)*[d]$", out[0..64]);
        expect_eq(n, 20 as usize);
        expect_eq(eqBytes(out[0..n], "a\\+b\\.\\(c\\)\\*\\[d\\]\\$"), true);
    });

    test("2. @cap/regex-syntax — Complex valid regex validation & capture_count == 2", () => {
        let pat = "^(?i:http|https)://([a-z0-9.-]+):(\\d{1,5})/.*$";
        let info = validateRegexSyntax(pat);
        expect_eq(info.valid, true);
        expect_eq(info.isOk(), true);
        expect_eq(info.err_code, RSX_OK);
        expect_eq(info.capture_count, 2 as usize);
        expect_eq(info.has_anchors, true);
        expect_eq(info.has_backref, false);
        // min_len: "http"(4) + "://"(3) + [a-z0-9.-]+(1) + ":"(1) + \d{1,5}(1) + "/"(1) + .*(0) = 11
        expect_eq(info.min_len, 11 as usize);

        // Named capture & non-capturing group & hex escape
        let named = "(?P<scheme>https?)://(?:www\\.)?([a-z]+)\\x2F";
        let info2 = validateRegexSyntax(named);
        expect_eq(info2.valid, true);
        expect_eq(info2.capture_count, 2 as usize);
    });

    test("3. @cap/regex-syntax — extractLiteralPrefix with escapes, optional rollback & alternation", () => {
        let out: [64]u8 = [0; 64];

        let p1 = "^https://example\\.com/[a-z]+";
        let n1 = extractLiteralPrefix(p1, out[0..64]);
        expect_eq(n1, 20 as usize);
        expect_eq(eqBytes(out[0..n1], "https://example.com/"), true);

        let info1 = validateRegexSyntax(p1);
        expect_eq(info1.valid, true);
        expect_eq(info1.literal_prefix_len, 20 as usize);

        // Optional 's?' rolls back 's'
        let n2 = extractLiteralPrefix("^https?://host", out[0..64]);
        expect_eq(n2, 4 as usize);
        expect_eq(eqBytes(out[0..n2], "http"), true);

        // Zero-or-more 'a*' rolls back 'a'
        let n3 = extractLiteralPrefix("a*bc", out[0..64]);
        expect_eq(n3, 0 as usize);

        // Top-level alternation has no single mandatory prefix
        let n4 = extractLiteralPrefix("foo|bar", out[0..64]);
        expect_eq(n4, 0 as usize);
    });

    test("4. @cap/regex-syntax — Character classes valid ranges, reversed [z-a] & unclosed [abc rejection", () => {
        let ok1 = validateRegexSyntax("[a-zA-Z0-9_-]+");
        expect_eq(ok1.valid, true);

        let ok2 = validateRegexSyntax("[]a-z]");
        expect_eq(ok2.valid, true);

        let badRange = validateRegexSyntax("[z-a]");
        expect_eq(badRange.valid, false);
        expect_eq(badRange.err_code, RSX_ERR_INVALID_CLASS_RANGE);

        let unclosed = validateRegexSyntax("foo[abc");
        expect_eq(unclosed.valid, false);
        expect_eq(unclosed.err_code, RSX_ERR_UNCLOSED_CLASS);
        expect_eq(unclosed.err_pos, 3 as usize);
    });

    test("5. @cap/regex-syntax — Invalid quantifiers {5,2}, dangling *abc & double a++ rejection", () => {
        let badBounds = validateRegexSyntax("a{5,2}");
        expect_eq(badBounds.valid, false);
        expect_eq(badBounds.err_code, RSX_ERR_INVALID_QUANTIFIER);

        let dangling = validateRegexSyntax("*abc");
        expect_eq(dangling.valid, false);
        expect_eq(dangling.err_code, RSX_ERR_DANGLING_QUANTIFIER);
        expect_eq(dangling.err_pos, 0 as usize);

        let dbl = validateRegexSyntax("a++");
        expect_eq(dbl.valid, false);
        expect_eq(dbl.err_code, RSX_ERR_DOUBLE_QUANTIFIER);
        expect_eq(dbl.err_pos, 2 as usize);

        // Non-greedy a+? is valid, but a+?? is double quantifier
        let lazyOk = validateRegexSyntax("a+?");
        expect_eq(lazyOk.valid, true);
        let lazyBad = validateRegexSyntax("a+??");
        expect_eq(lazyBad.valid, false);
        expect_eq(lazyBad.err_code, RSX_ERR_DOUBLE_QUANTIFIER);
    });

    test("6. @cap/regex-syntax — Unclosed/unopened parentheses & trailing backslash rejection", () => {
        let unclosed = validateRegexSyntax("foo(bar(baz)");
        expect_eq(unclosed.valid, false);
        expect_eq(unclosed.err_code, RSX_ERR_UNCLOSED_GROUP);
        expect_eq(unclosed.err_pos, 3 as usize);

        let unopened = validateRegexSyntax("foo)bar");
        expect_eq(unopened.valid, false);
        expect_eq(unopened.err_code, RSX_ERR_UNOPENED_GROUP);
        expect_eq(unopened.err_pos, 3 as usize);

        let trailing = validateRegexSyntax("abc\\");
        expect_eq(trailing.valid, false);
        expect_eq(trailing.err_code, RSX_ERR_TRAILING_BACKSLASH);
        expect_eq(trailing.err_pos, 3 as usize);
    });
}
