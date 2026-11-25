// cap/test/testLogfmtDeep.cp
// Deep unit tests for cap:logfmt (Rust `logfmt` / Go `go-logfmt` parity: LogfmtEncoder,
// typed field parsing, CRLF injection escaping, atomic overflow rollback, and malformed detection).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    LogfmtField,
    LogfmtIter,
    LogfmtEncoder,
    getLogfmtValue,
    getLogfmtInt,
    getLogfmtBool,
    countLogfmtFields,
    appendLogfmtPair,
} from "cap:logfmt";

fn main() {
    test("1. cap:logfmt — LogfmtEncoder & Anti-Injection Escaping", () => {
        let buf: [256]u8 = [0; 256];
        let enc = LogfmtEncoder.init(buf[0..]);
        expect_eq(enc.kvStr("level", "info"), true);
        expect_eq(enc.kvStr("msg", "hello \"cp\"\nline2"), true);
        expect_eq(enc.kvInt("status", 200), true);
        expect_eq(enc.kvUint("bytes", 4096), true);
        expect_eq(enc.kvFloat("ms", 1.25), true);
        expect_eq(enc.kvBool("ok", true), true);
        expect_eq(enc.kvFlag("prod"), true);

        const expected = "level=info msg=\"hello \\\"cp\\\"\\nline2\" status=200 bytes=4096 ms=1.250 ok=true prod";
        expect_eq(std.mem.eql(u8, enc.written(), expected), true);
        expect_eq(countLogfmtFields(enc.written()), 7 as usize);
    });

    test("2. cap:logfmt — Typed Lookups & Unescape Roundtrip", () => {
        const line = "level=error status=503 retries=-2 latency=12.500 ok=false msg=\"bad\\tval\\nnext\" debug";
        expect_eq(getLogfmtInt(line, "status").?, 503 as i64);
        expect_eq(getLogfmtInt(line, "retries").?, -2 as i64);
        expect_eq(getLogfmtBool(line, "ok").?, false);
        expect_eq(getLogfmtBool(line, "debug").?, true);

        let vbuf: [64]u8 = [0; 64];
        const msg = getLogfmtValue(line, "msg", vbuf[0..]).?;
        expect_eq(std.mem.eql(u8, msg, "bad\tval\nnext"), true);

        let it = LogfmtIter.init(line);
        let saw_float = false;
        while (f := it.next()) {
            if (std.mem.eql(u8, f.key, "latency")) {
                const fv: f64 = f.asFloat().?;
                expect_eq(fv > 12.4 && fv < 12.6, true);
                saw_float = true;
            }
        }
        expect_eq(saw_float, true);
        expect_eq(it.hasError(), false);
    });

    test("3. cap:logfmt — Atomic Buffer Rollback & Malformed Detection", () => {
        let small: [16]u8 = [0; 16];
        let enc = LogfmtEncoder.init(small[0..]);
        expect_eq(enc.kvStr("a", "1"), true);
        expect_eq(std.mem.eql(u8, enc.written(), "a=1"), true);
        // Oversized pair must fail atomically without corrupting "a=1"
        expect_eq(enc.kvStr("longkey", "very-long-value-overflow"), false);
        expect_eq(std.mem.eql(u8, enc.written(), "a=1"), true);

        let bad = LogfmtIter.init("key=\"unclosed");
        expect_eq(bad.next() == null, true);
        expect_eq(bad.hasError(), true);
    });
}
