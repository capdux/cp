import {
    LogfmtIter,
    getLogfmtValue,
    appendLogfmtPair,
} from "cap:logfmt";
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

test("LogfmtIter parses unquoted, quoted, escaped, and bare boolean keys", () => {
    const line = "level=info msg=\"hello \\\"world\\\"\\nline2\" latency_ms=12 debug empty=";
    let it = LogfmtIter.init(line);
    let buf: [64]u8 = [0; 64];

    const f1 = it.next().?;
    expect(strEq(f1.key, "level") && strEq(f1.raw_value, "info") && !f1.is_quoted);

    const f2 = it.next().?;
    expect(strEq(f2.key, "msg") && f2.is_quoted);
    expect(strEq(f2.decodeValue(buf[0..]).?, "hello \"world\"\nline2"));

    const f3 = it.next().?;
    expect(strEq(f3.key, "latency_ms") && strEq(f3.raw_value, "12"));

    const f4 = it.next().?;
    expect(strEq(f4.key, "debug") && !f4.has_value && strEq(f4.raw_value, "true"));

    const f5 = it.next().?;
    expect(strEq(f5.key, "empty") && f5.has_value && f5.raw_value.len == 0);

    expect(it.next() == null);
    expect(!it.hasError());
});

test("appendLogfmtPair and getLogfmtValue round-trip + error detection", () => {
    let out: [256]u8 = [0; 256];
    let pos: usize = 0;
    expect(appendLogfmtPair(out[0..], &pos, "lvl", "warn"));
    expect(appendLogfmtPair(out[0..], &pos, "err", "path=\"a=b\"\nfail"));
    expect(appendLogfmtPair(out[0..], &pos, "code", "500"));

    const encoded = out[0..pos];
    let dec_buf: [64]u8 = [0; 64];
    expect(strEq(getLogfmtValue(encoded, "lvl", dec_buf[0..]).?, "warn"));
    expect(strEq(getLogfmtValue(encoded, "err", dec_buf[0..]).?, "path=\"a=b\"\nfail"));
    expect(strEq(getLogfmtValue(encoded, "code", dec_buf[0..]).?, "500"));
    expect(getLogfmtValue(encoded, "missing", dec_buf[0..]) == null);

    // Unclosed quote triggers error flag
    let bad = LogfmtIter.init("msg=\"unclosed");
    expect(bad.next() == null);
    expect(bad.hasError());
});
