import {
    needsJsonEscape,
    escapeJsonContent,
    quoteJsonString,
    unescapeJsonContent,
} from "cap:json";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("escapeJsonContent, quoteJsonString, and unescapeJsonContent round-trip", () => {
    expect(!needsJsonEscape("hello world 123"));
    expect(needsJsonEscape("line1\n\"quoted\""));

    let buf: [128]u8 = [0; 128];
    let dec_buf: [128]u8 = [0; 128];

    const src = "a\"b\\c\nd\te\x01f";
    const esc = escapeJsonContent(src, buf[0..]).?;
    expect(strEq(esc, "a\\\"b\\\\c\\nd\\te\\u0001f"));
    expect(strEq(unescapeJsonContent(esc, dec_buf[0..]).?, src));

    expect(strEq(quoteJsonString("hi\n", buf[0..]).?, "\"hi\\n\""));
});

test("unescapeJsonContent decodes UTF-16 surrogate pairs and rejects invalid escapes", () => {
    let buf: [64]u8 = [0; 64];
    // \uD83D\uDE00 -> U+1F600 (😀) = F0 9F 98 80
    expect(strEq(unescapeJsonContent("\\uD83D\\uDE00", buf[0..]).?, "\xf0\x9f\x98\x80"));
    // \u4E2D -> 中 = E4 B8 AD
    expect(strEq(unescapeJsonContent("\\u4E2D", buf[0..]).?, "中"));

    // Reject lone surrogates, unknown escapes, and raw control chars
    expect(unescapeJsonContent("\\uD83D", buf[0..]) == null);
    expect(unescapeJsonContent("\\uDE00", buf[0..]) == null);
    expect(unescapeJsonContent("\\x41", buf[0..]) == null);
    expect(unescapeJsonContent("raw\nnewline", buf[0..]) == null);
});
