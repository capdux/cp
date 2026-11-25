// cap/test/testCapQs.cp
// Comprehensive unit tests for @cap/qs: nested brackets, dot notation, indexed & empty-bracket
// arrays, comma arrays, strict null handling, RFC 3986 percent-encoding, QsBuilder round-trip,
// and max_depth / max_params resource exhaustion protection.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    QsError,
    QsParseOptions,
    QsPathSegment,
    QsEntry,
    QsParsed,
    QsBuilder,
    percentDecode,
    percentEncode,
} from "../../packages/@cap/qs/src/index.cp";

fn expectParseError(input: []const u8, opts: QsParseOptions, expected: QsError): bool {
    * = QsParsed.parse(input, opts) catch |e| {
        return e == expected;
    };
    return false;
}

fn expectDecodeError(input: []const u8, plus_as_space: bool, buf_len: usize, expected: QsError): bool {
    let buf: [64]u8 = undefined;
    * = percentDecode(input, plus_as_space, buf[0..buf_len]) catch |e| {
        return e == expected;
    };
    return false;
}

fn expectEncodeTooSmall(input: []const u8, buf_len: usize): bool {
    let buf: [64]u8 = undefined;
    * = percentEncode(input, false, buf[0..buf_len]) catch |e| {
        return e == QsError.BufferTooSmall;
    };
    return false;
}

fn expectBuilderRollback(): bool {
    let tiny: [6]u8 = undefined;
    let tb = QsBuilder.init(tiny[0..]);
    tb.add("a", "1") catch return false;
    let hit_small = false;
    tb.add("longkey", "longval") catch |e| {
        hit_small = (e == QsError.BufferTooSmall);
    };
    return hit_small && std.mem.eql(u8, tb.written(), "a=1");
}

fn main() {
    test("1. @cap/qs — Basic Flat Key-Value, Prefix & Plus-as-Space", () => {
        let parsed = QsParsed.parse("?foo=bar&hello=world+cp&empty=&a=1&&b=2", QsParseOptions{}) catch unreachable;
        expect_eq(parsed.len(), 5 as usize);
        expect_eq(std.mem.eql(u8, parsed.get("foo").?, "bar"), true);
        expect_eq(std.mem.eql(u8, parsed.get("hello").?, "world cp"), true);
        expect_eq(std.mem.eql(u8, parsed.get("empty").?, ""), true);
        expect_eq(std.mem.eql(u8, parsed.get("a").?, "1"), true);
        expect_eq(std.mem.eql(u8, parsed.get("b").?, "2"), true);
        expect_eq(parsed.get("missing") == null, true);

        // When ignore_query_prefix is false, '?' remains part of the first key
        let raw_p = QsParsed.parse("?foo=bar", QsParseOptions{ ignore_query_prefix: false }) catch unreachable;
        expect_eq(std.mem.eql(u8, raw_p.get("?foo").?, "bar"), true);
        expect_eq(raw_p.get("foo") == null, true);

        // When plus_as_space is false, '+' is preserved literally
        let lit_p = QsParsed.parse("expr=1+1", QsParseOptions{ plus_as_space: false }) catch unreachable;
        expect_eq(std.mem.eql(u8, lit_p.get("expr").?, "1+1"), true);
    });

    test("2. @cap/qs — Nested Brackets & Dot Notation Paths", () => {
        const q = "?a[b][c]=1&user[name]=alice&user[role]=admin&cfg.host=localhost&cfg.net.port=8080";
        let parsed = QsParsed.parse(q, QsParseOptions{ allow_dots: true }) catch unreachable;

        expect_eq(std.mem.eql(u8, parsed.getNested3("a", "b", "c").?, "1"), true);
        expect_eq(std.mem.eql(u8, parsed.getNested2("user", "name").?, "alice"), true);
        expect_eq(std.mem.eql(u8, parsed.getNested2("user", "role").?, "admin"), true);
        expect_eq(std.mem.eql(u8, parsed.getNested2("cfg", "host").?, "localhost"), true);
        expect_eq(std.mem.eql(u8, parsed.getNested3("cfg", "net", "port").?, "8080"), true);

        // Percent-encoded brackets in key (e.g. a%5Bb%5D%5Bc%5D=42)
        let enc_p = QsParsed.parse("a%5Bb%5D%5Bc%5D=42", QsParseOptions{}) catch unreachable;
        expect_eq(std.mem.eql(u8, enc_p.getNested3("a", "b", "c").?, "42"), true);

        // When allow_dots is false, dots remain part of a flat key
        let no_dots = QsParsed.parse("user.name=bob", QsParseOptions{ allow_dots: false }) catch unreachable;
        expect_eq(std.mem.eql(u8, no_dots.get("user.name").?, "bob"), true);
        expect_eq(no_dots.getNested2("user", "name") == null, true);
    });

    test("3. @cap/qs — Indexed, Empty-Bracket, Mixed & Comma Arrays", () => {
        const q = "?arr[0]=x&arr[]=y&arr[2]=z&tags=alpha,beta,gamma&items[]=one&items[]=two&csv=a%2Cb,c";
        let parsed = QsParsed.parse(q, QsParseOptions{ comma_arrays: true }) catch unreachable;

        // Mixed indexed + empty bracket array
        expect_eq(parsed.countArrayItems("arr"), 3 as usize);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("arr", 0).?, "x"), true);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("arr", 1).?, "y"), true);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("arr", 2).?, "z"), true);
        expect_eq(parsed.getArrayItem("arr", 3) == null, true);

        // Pure empty-bracket array
        expect_eq(parsed.countArrayItems("items"), 2 as usize);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("items", 0).?, "one"), true);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("items", 1).?, "two"), true);

        // Comma-split array
        expect_eq(parsed.countArrayItems("tags"), 3 as usize);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("tags", 0).?, "alpha"), true);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("tags", 1).?, "beta"), true);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("tags", 2).?, "gamma"), true);

        // Percent-encoded comma (%2C) stays intact inside its element
        expect_eq(parsed.countArrayItems("csv"), 2 as usize);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("csv", 0).?, "a,b"), true);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("csv", 1).?, "c"), true);
    });

    test("4. @cap/qs — Strict Null Handling vs Empty String", () => {
        let strict_p = QsParsed.parse("flag&empty=&val=ok", QsParseOptions{
            strict_null_handling: true,
        }) catch unreachable;
        expect_eq(strict_p.len(), 3 as usize);
        expect_eq(strict_p.isNull("flag"), true);
        expect_eq(strict_p.get("flag") == null, true);
        expect_eq(strict_p.isNull("empty"), false);
        expect_eq(std.mem.eql(u8, strict_p.get("empty").?, ""), true);
        expect_eq(std.mem.eql(u8, strict_p.get("val").?, "ok"), true);

        // Non-strict mode treats bare key as empty string
        let loose_p = QsParsed.parse("flag&empty=", QsParseOptions{
            strict_null_handling: false,
        }) catch unreachable;
        expect_eq(loose_p.isNull("flag"), false);
        expect_eq(std.mem.eql(u8, loose_p.get("flag").?, ""), true);
        expect_eq(std.mem.eql(u8, loose_p.get("empty").?, ""), true);
    });

    test("5. @cap/qs — RFC 3986 Percent Encode/Decode Round-Trip & Error Cases", () => {
        let enc_buf: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        const sample = "AZaz09-._~ hello/world?a=1&b=2%!";
        const enc = percentEncode(sample, false, enc_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, enc, "AZaz09-._~%20hello%2Fworld%3Fa%3D1%26b%3D2%25%21"), true);

        const decoded = percentDecode(enc, false, dec_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, decoded, sample), true);

        // space_as_plus round-trip
        const enc_plus = percentEncode("hello world+more", true, enc_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, enc_plus, "hello+world%2Bmore"), true);
        const dec_plus = percentDecode(enc_plus, true, dec_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, dec_plus, "hello world+more"), true);

        // Invalid percent sequences & BufferTooSmall
        expect_eq(expectDecodeError("abc%2", false, 64, QsError.InvalidPercentEncoding), true);
        expect_eq(expectDecodeError("abc%ZZ", false, 64, QsError.InvalidPercentEncoding), true);
        expect_eq(expectDecodeError("abcdef", false, 3, QsError.BufferTooSmall), true);
        expect_eq(expectEncodeTooSmall("a b", 4), true);
    });

    test("6. @cap/qs — QsBuilder Serialization & Full Round-Trip", () => {
        let buf: [256]u8 = undefined;
        let b = QsBuilder.initWithOptions(buf[0..], true, true);
        b.add("q", "hello world") catch unreachable;
        b.addNested2("user", "name", "alice") catch unreachable;
        b.addNested3("a", "b", "c", "1") catch unreachable;
        b.addArrayItem("arr", 0, "x") catch unreachable;
        b.addBracketArrayItem("arr", "y") catch unreachable;
        b.addNull("debug") catch unreachable;

        const out = b.written();
        expect_eq(
            std.mem.eql(u8, out, "?q=hello+world&user[name]=alice&a[b][c]=1&arr[0]=x&arr[]=y&debug"),
            true,
        );

        // Round-trip parse back into QsParsed
        let parsed = QsParsed.parse(out, QsParseOptions{
            strict_null_handling: true,
        }) catch unreachable;
        expect_eq(std.mem.eql(u8, parsed.get("q").?, "hello world"), true);
        expect_eq(std.mem.eql(u8, parsed.getNested2("user", "name").?, "alice"), true);
        expect_eq(std.mem.eql(u8, parsed.getNested3("a", "b", "c").?, "1"), true);
        expect_eq(parsed.countArrayItems("arr"), 2 as usize);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("arr", 0).?, "x"), true);
        expect_eq(std.mem.eql(u8, parsed.getArrayItem("arr", 1).?, "y"), true);
        expect_eq(parsed.isNull("debug"), true);

        // Reset and reuse
        b.reset();
        expect_eq(b.written().len, 0 as usize);
        b.add("k", "v") catch unreachable;
        expect_eq(std.mem.eql(u8, b.written(), "?k=v"), true);

        // BufferTooSmall rollback safety
        expect_eq(expectBuilderRollback(), true);
    });

    test("7. @cap/qs — Resource Exhaustion Limits (max_depth, max_params) & Syntax Validation", () => {
        // DepthExceeded: 4 segments > max_depth(3)
        expect_eq(
            expectParseError("a[b][c][d]=1", QsParseOptions{ max_depth: 3 }, QsError.DepthExceeded),
            true,
        );
        // Dot depth exceeded
        expect_eq(
            expectParseError("a.b.c=1", QsParseOptions{ allow_dots: true, max_depth: 2 }, QsError.DepthExceeded),
            true,
        );
        // TooManyParams: 3 params > max_params(2)
        expect_eq(
            expectParseError("a=1&b=2&c=3", QsParseOptions{ max_params: 2 }, QsError.TooManyParams),
            true,
        );
        // Comma array cannot bypass max_params
        expect_eq(
            expectParseError("tags=1,2,3,4", QsParseOptions{ comma_arrays: true, max_params: 3 }, QsError.TooManyParams),
            true,
        );
        // Invalid syntax: unclosed bracket, stray closing bracket, trailing chars after bracket
        expect_eq(expectParseError("a[b=1", QsParseOptions{}, QsError.InvalidSyntax), true);
        expect_eq(expectParseError("a]b=1", QsParseOptions{}, QsError.InvalidSyntax), true);
        expect_eq(expectParseError("a[b]c=1", QsParseOptions{}, QsError.InvalidSyntax), true);
        expect_eq(expectParseError("[a]=1", QsParseOptions{}, QsError.InvalidSyntax), true);
    });
}
