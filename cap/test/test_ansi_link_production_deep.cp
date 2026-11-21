import {
    Osc8Link,
    osc8EncodedLen,
    osc8BelEncodedLen,
    formatOsc8,
    formatOsc8Bel,
    parseOsc8,
    stripOsc8,
    hasOsc8,
} from "cap:ansi_link";
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

test("formatOsc8 and parseOsc8 roundtrip with ST terminator and params", () => {
    let buf: [256]u8 = [0; 256];
    const url = "https://cp-lang.dev/docs";
    const text = "CP Documentation";
    const params = "id=doc1";

    const expected_len = osc8EncodedLen(url, text, params);
    const encoded_opt = formatOsc8(url, text, params, buf[0..]);
    expect(encoded_opt != null);
    if (encoded := encoded_opt) {
        expect(encoded.len == expected_len);
        expect(hasOsc8(encoded));
        expect(strEq(encoded, "\x1b]8;id=doc1;https://cp-lang.dev/docs\x1b\\CP Documentation\x1b]8;;\x1b\\"));

        const parsed_opt = parseOsc8(encoded);
        expect(parsed_opt != null);
        if (link := parsed_opt) {
            expect(strEq(link.url, url));
            expect(strEq(link.text, text));
            expect(strEq(link.params, params));
        }
    }

    // Empty params and empty text/url edge case
    const empty_params_opt = formatOsc8("https://example.com", "Click", "", buf[0..]);
    expect(empty_params_opt != null);
    if (encoded := empty_params_opt) {
        expect(strEq(encoded, "\x1b]8;;https://example.com\x1b\\Click\x1b]8;;\x1b\\"));
        if (link := parseOsc8(encoded)) {
            expect(strEq(link.params, ""));
            expect(strEq(link.url, "https://example.com"));
            expect(strEq(link.text, "Click"));
        } else {
            expect(false);
        }
    }
});

test("formatOsc8Bel and parseOsc8 roundtrip with BEL terminator and UTF-8", () => {
    let buf: [256]u8 = [0; 256];
    const url = "https://cp-lang.dev/zh";
    const text = "中文文档链接";

    const expected_len = osc8BelEncodedLen(url, text);
    const encoded_opt = formatOsc8Bel(url, text, buf[0..]);
    expect(encoded_opt != null);
    if (encoded := encoded_opt) {
        expect(encoded.len == expected_len);
        expect(hasOsc8(encoded));
        expect(strEq(encoded, "\x1b]8;;https://cp-lang.dev/zh\x07中文文档链接\x1b]8;;\x07"));

        if (link := parseOsc8(encoded)) {
            expect(strEq(link.url, url));
            expect(strEq(link.text, text));
            expect(strEq(link.params, ""));
        } else {
            expect(false);
        }
    }

    // Mixed open (BEL) and close (ST) terminators + styled inner text
    const mixed = "\x1b]8;id=mix:k=v;https://a.io\x07\x1b[1mBold\x1b[0m\x1b]8;;\x1b\\";
    if (link := parseOsc8(mixed)) {
        expect(strEq(link.params, "id=mix:k=v"));
        expect(strEq(link.url, "https://a.io"));
        expect(strEq(link.text, "\x1b[1mBold\x1b[0m"));
    } else {
        expect(false);
    }
});

test("stripOsc8 removes multiple ST and BEL sequences while preserving plain text", () => {
    let out: [256]u8 = [0; 256];

    // Plain text without OSC 8
    const plain = "no links here";
    expect(!hasOsc8(plain));
    if (res := stripOsc8(plain, out[0..])) {
        expect(strEq(res, plain));
    } else {
        expect(false);
    }

    // Multiple hyperlinks with both ST and BEL + surrounding text + UTF-8
    const mixed = "See \x1b]8;id=1;https://a.dev\x1b\\Alpha\x1b]8;;\x1b\\ and \x1b]8;;https://b.dev\x07贝塔\x1b]8;;\x07!";
    expect(hasOsc8(mixed));
    if (res := stripOsc8(mixed, out[0..])) {
        expect(strEq(res, "See Alpha and 贝塔!"));
    } else {
        expect(false);
    }

    // Non-OSC-8 CSI styling is preserved while OSC 8 wrappers are stripped
    const with_sgr = "\x1b[32m\x1b]8;;https://cp.dev\x1b\\OK\x1b]8;;\x1b\\\x1b[0m";
    if (res := stripOsc8(with_sgr, out[0..])) {
        expect(strEq(res, "\x1b[32mOK\x1b[0m"));
    } else {
        expect(false);
    }
});

test("buffer capacity boundary checks for formatOsc8, formatOsc8Bel, and stripOsc8", () => {
    const url = "https://x.io";
    const text = "Go";
    const params = "id=1";

    // Exact buffer size succeeds, one byte smaller returns null
    const st_len = osc8EncodedLen(url, text, params);
    let exact_st: [64]u8 = [0; 64];
    expect(formatOsc8(url, text, params, exact_st[0..st_len]) != null);
    expect(formatOsc8(url, text, params, exact_st[0..(st_len - 1)]) == null);

    const bel_len = osc8BelEncodedLen(url, text);
    let exact_bel: [64]u8 = [0; 64];
    expect(formatOsc8Bel(url, text, exact_bel[0..bel_len]) != null);
    expect(formatOsc8Bel(url, text, exact_bel[0..(bel_len - 1)]) == null);

    // stripOsc8 exact fit vs short buffer
    const input = "\x1b]8;;https://x.io\x07Hello\x1b]8;;\x07";
    let small: [5]u8 = [0; 5];
    if (res := stripOsc8(input, small[0..])) {
        expect(strEq(res, "Hello"));
    } else {
        expect(false);
    }
    expect(stripOsc8(input, small[0..4]) == null);
});

test("malformed, unterminated, and control-byte boundary protection", () => {
    // Reject unterminated opening sequence
    expect(parseOsc8("\x1b]8;;https://example.com") == null);
    // Reject missing closing sequence
    expect(parseOsc8("\x1b]8;;https://example.com\x07unclosed text") == null);
    expect(parseOsc8("\x1b]8;;https://example.com\x1b\\unclosed text") == null);
    // Reject missing second semicolon
    expect(parseOsc8("\x1b]8;no-semicolon\x07text\x1b]8;;\x07") == null);
    // Reject trailing bytes after closing sequence
    expect(parseOsc8("\x1b]8;;https://a.io\x07A\x1b]8;;\x07extra") == null);
    // Reject nested OSC 8 opening inside text
    expect(parseOsc8("\x1b]8;;https://a.io\x07A\x1b]8;;https://b.io\x07B\x1b]8;;\x07") == null);
    // Reject non-OSC-8 input and short strings
    expect(parseOsc8("") == null);
    expect(parseOsc8("\x1b]8") == null);
    expect(!hasOsc8("\x1b]8"));
    expect(!hasOsc8("plain text"));

    // Reject control bytes in URL / params during formatting
    let buf: [128]u8 = [0; 128];
    expect(formatOsc8("https://a.io\x07bad", "text", "", buf[0..]) == null);
    expect(formatOsc8("https://a.io\x1b\\bad", "text", "", buf[0..]) == null);
    expect(formatOsc8("https://a.io", "text", "id=1\n2", buf[0..]) == null);
    expect(formatOsc8Bel("https://a.io\x07", "text", buf[0..]) == null);

    // stripOsc8 safely handles trailing unterminated OSC 8 header without leaking control bytes
    const truncated = "prefix\x1b]8;;https://broken";
    if (res := stripOsc8(truncated, buf[0..])) {
        expect(strEq(res, "prefix"));
    } else {
        expect(false);
    }
});
