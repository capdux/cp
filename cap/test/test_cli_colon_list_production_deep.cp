// Production Deep Tests for cap:cli ColonList Parser (lib/cli/colon_list.cp)
import { test, expect_eq } from "cap:test";
import {
    ColonListError,
    ColonPair,
    parseColonPair,
    parseColonPairNonEmptyKey,
    parseLoaderMapping,
    parseColonList,
    parseLoaderMappingList,
    findColonValue,
} from "cap:cli";

test("cli colon_list: prefers whichever separator (':' or '=') appears first", fn() {
    // ':' first, value contains '='
    const p1 = parseColonPair("FOO:a=b") catch unreachable;
    expect_eq(p1.key, "FOO");
    expect_eq(p1.value, "a=b");
    expect_eq(p1.separator, @as(u8, ':'));

    // '=' first, value contains ':' (e.g. URL)
    const p2 = parseColonPair("API_URL=https://example.com:8443") catch unreachable;
    expect_eq(p2.key, "API_URL");
    expect_eq(p2.value, "https://example.com:8443");
    expect_eq(p2.separator, @as(u8, '='));

    // Empty value is valid
    const p3 = parseColonPair("DEBUG=") catch unreachable;
    expect_eq(p3.key, "DEBUG");
    expect_eq(p3.value, "");
    expect_eq(p3.separator, @as(u8, '='));

    // Empty key allowed in raw parseColonPair
    const p4 = parseColonPair(":val") catch unreachable;
    expect_eq(p4.key, "");
    expect_eq(p4.value, "val");
});

test("cli colon_list: parseLoaderMapping leading-dot extension validation", fn() {
    const m1 = parseLoaderMapping(".ts:tsx") catch unreachable;
    expect_eq(m1.key, ".ts");
    expect_eq(m1.value, "tsx");
    expect_eq(m1.separator, @as(u8, ':'));

    const m2 = parseLoaderMapping(".svg=text") catch unreachable;
    expect_eq(m2.key, ".svg");
    expect_eq(m2.value, "text");
    expect_eq(m2.separator, @as(u8, '='));

    // Missing leading dot -> InvalidExtensionPrefix
    let bad_ext = false;
    * = parseLoaderMapping("ts:tsx") catch |e| {
        bad_ext = (e == ColonListError.InvalidExtensionPrefix);
        ColonPair{}
    };
    expect_eq(bad_ext, true);

    // Empty key -> EmptyKey
    let empty_k = false;
    * = parseLoaderMapping(":tsx") catch |e| {
        empty_k = (e == ColonListError.EmptyKey);
        ColonPair{}
    };
    expect_eq(empty_k, true);
});

test("cli colon_list: batch parseColonList and last-wins findColonValue", fn() {
    const raw: [4]([]const u8) = [
        "NODE_ENV=development",
        "PORT:3000",
        "NODE_ENV:production",
        "HOST=127.0.0.1:8080",
    ];
    let out: [4]ColonPair = [ColonPair{}, ColonPair{}, ColonPair{}, ColonPair{}];
    const n = parseColonList(raw[0..], out[0..]) catch unreachable;
    expect_eq(n, @as(usize, 4));

    // Duplicate key resolves to last value ("production")
    expect_eq(findColonValue(out[0..n], "NODE_ENV") ?? "", "production");
    expect_eq(findColonValue(out[0..n], "PORT") ?? "", "3000");
    expect_eq(findColonValue(out[0..n], "HOST") ?? "", "127.0.0.1:8080");
    expect_eq(findColonValue(out[0..n], "MISSING") == null, true);
});

test("cli colon_list: batch parseLoaderMappingList normal and empty input", fn() {
    const loaders: [3]([]const u8) = [".png=file", ".txt:text", ".png:dataurl"];
    let out: [3]ColonPair = [ColonPair{}, ColonPair{}, ColonPair{}];
    const n = parseLoaderMappingList(loaders[0..], out[0..]) catch unreachable;
    expect_eq(n, @as(usize, 3));
    expect_eq(findColonValue(out[0..n], ".png") ?? "", "dataurl");
    expect_eq(findColonValue(out[0..n], ".txt") ?? "", "text");

    // Empty input slice
    const empty_in: [0]([]const u8) = [];
    expect_eq(parseLoaderMappingList(empty_in[0..], out[0..]) catch unreachable, @as(usize, 0));
});

test("cli colon_list: InvalidSeparator, EmptyKey and OutputBufferTooSmall guards", fn() {
    // No ':' or '=' -> InvalidSeparator
    let no_sep = false;
    * = parseColonPair("NO_SEPARATOR_HERE") catch |e| {
        no_sep = (e == ColonListError.InvalidSeparator);
        ColonPair{}
    };
    expect_eq(no_sep, true);

    // Empty string -> InvalidSeparator
    let empty_sep = false;
    * = parseColonPair("") catch |e| {
        empty_sep = (e == ColonListError.InvalidSeparator);
        ColonPair{}
    };
    expect_eq(empty_sep, true);

    // parseColonPairNonEmptyKey rejects "=value"
    let empty_key = false;
    * = parseColonPairNonEmptyKey("=value") catch |e| {
        empty_key = (e == ColonListError.EmptyKey);
        ColonPair{}
    };
    expect_eq(empty_key, true);

    // OutputBufferTooSmall when out.len < inputs.len
    const raw: [2]([]const u8) = ["A=1", "B=2"];
    let small: [1]ColonPair = [ColonPair{}];
    let buf_err = false;
    * = parseColonList(raw[0..], small[0..]) catch |e| {
        buf_err = (e == ColonListError.OutputBufferTooSmall);
        @as(usize, 0)
    };
    expect_eq(buf_err, true);
});
