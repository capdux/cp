import {
    isBareTomlKey,
    formatTomlBasicString,
    formatTomlKey,
    unescapeTomlBasic,
} from "cap:toml";
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

test("TOML key formatting, basic string escaping, and Unicode scalar unescaping", () => {
    expect(isBareTomlKey("my_key-1"));
    expect(!isBareTomlKey("has.dot"));
    expect(!isBareTomlKey(""));

    let buf: [128]u8 = [0; 128];
    expect(strEq(formatTomlKey("simple-key", buf[0..]).?, "simple-key"));
    expect(strEq(formatTomlKey("a.b/c", buf[0..]).?, "\"a.b/c\""));

    const src = "line1\n\"quoted\"\\\x7F";
    const enc = formatTomlBasicString(src, buf[0..]).?;
    expect(strEq(enc, "\"line1\\n\\\"quoted\\\"\\\\\\u007F\""));

    let dec_buf: [128]u8 = [0; 128];
    expect(strEq(unescapeTomlBasic(enc[1..enc.len - 1], dec_buf[0..]).?, src));

    // \U0001F600 -> U+1F600 (😀)
    expect(strEq(unescapeTomlBasic("\\U0001F600\\u4E2D", dec_buf[0..]).?, "\xf0\x9f\x98\x80中"));
    // Reject surrogate scalar value & unknown escape
    expect(unescapeTomlBasic("\\uD800", dec_buf[0..]) == null);
    expect(unescapeTomlBasic("\\q", dec_buf[0..]) == null);
});
