import {
    hasAnsi,
    stripAnsi,
    strippedByteLen,
    visibleCodepointCount,
} from "cap:ansi_strip";
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

test("stripAnsi handles CSI SGR colors, cursor moves, and plain text", () => {
    const plain = "hello world";
    expect(!hasAnsi(plain));
    expect(strippedByteLen(plain) == 11);

    const colored = "\x1b[1;31mERROR\x1b[0m: \x1b[38;5;214mfile.cp\x1b[0m";
    expect(hasAnsi(colored));
    expect(strippedByteLen(colored) == 14);
    expect(visibleCodepointCount(colored) == 14);

    let buf: [64]u8 = [0; 64];
    expect(strEq(stripAnsi(colored, buf[0..]).?, "ERROR: file.cp"));

    // Buffer too small returns null
    let tiny: [4]u8 = [0; 4];
    expect(stripAnsi(colored, tiny[0..]) == null);
});

test("stripAnsi handles OSC 8 hyperlinks (BEL and ST) and UTF-8", () => {
    // OSC 8 with BEL terminator
    const link_bel = "\x1b]8;;https://example.com\x07Click\x1b]8;;\x07";
    let buf: [64]u8 = [0; 64];
    expect(strEq(stripAnsi(link_bel, buf[0..]).?, "Click"));

    // OSC 8 with ST (ESC \) terminator + UTF-8 Chinese characters
    const link_st = "\x1b]8;;https://cp-lang.dev\x1b\\文档\x1b]8;;\x1b\\";
    expect(strEq(stripAnsi(link_st, buf[0..]).?, "文档"));
    expect(visibleCodepointCount(link_st) == 2);

    // Charset designation + 2-byte ESC
    const esc2 = "\x1b(B\x1b7OK\x1b8";
    expect(strEq(stripAnsi(esc2, buf[0..]).?, "OK"));
});
