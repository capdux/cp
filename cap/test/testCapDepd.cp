import { test, expect_eq } from "cap:test";
import { Deprecator, formatDeprecation } from "../../packages/@cap/depd/src/index.cp";

fn eqBuf(buf: []u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("formatDeprecation with and without callsite", () => {
        let out: [128]u8 = [0; 128];
        let n1 = formatDeprecation("express", "res.send(status)", "app.cp:42", out[0..128]);
        expect_eq(eqBuf(out[0..128], n1, "express deprecated res.send(status) at app.cp:42"), true);
        let n2 = formatDeprecation("body-parser", "undefined extended", "", out[0..128]);
        expect_eq(eqBuf(out[0..128], n2, "body-parser deprecated undefined extended"), true);
    });

    test("warnOnce deduplicates repeated messages", () => {
        let d = Deprecator.new("express");
        let out: [128]u8 = [0; 128];
        let n1 = d.warnOnce("old api", "main.cp:10", out[0..128]);
        expect_eq(n1 > 0, true);
        expect_eq(d.hasWarned("old api"), true);
        let n2 = d.warnOnce("old api", "main.cp:20", out[0..128]);
        expect_eq(n2, 0 as usize);
        expect_eq(d.emitted_count, 1 as u64);
    });

    test("distinct messages emit independently", () => {
        let d = Deprecator.new("send");
        let out: [128]u8 = [0; 128];
        expect_eq(d.warnOnce("msg-a", "", out[0..128]) > 0, true);
        expect_eq(d.warnOnce("msg-b", "", out[0..128]) > 0, true);
        expect_eq(d.emitted_count, 2 as u64);
    });

    test("silenced mode suppresses all warnings", () => {
        let d = Deprecator.new("serve-static");
        d.setSilenced(true);
        let out: [128]u8 = [0; 128];
        expect_eq(d.warnOnce("legacy opt", "srv.cp:1", out[0..128]), 0 as usize);
        expect_eq(d.hasWarned("legacy opt"), false);
        expect_eq(d.emitted_count, 0 as u64);
    });

    test("reset clears seen cache and counter", () => {
        let d = Deprecator.new("koa");
        let out: [128]u8 = [0; 128];
        expect_eq(d.warnOnce("req.foo", "", out[0..128]) > 0, true);
        d.reset();
        expect_eq(d.hasWarned("req.foo"), false);
        expect_eq(d.warnOnce("req.foo", "", out[0..128]) > 0, true);
    });

    test("small output buffer truncates safely", () => {
        let out: [10]u8 = [0; 10];
        let n = formatDeprecation("express", "very long deprecation message", "a.cp:1", out[0..10]);
        expect_eq(n, 10 as usize);
        expect_eq(eqBuf(out[0..10], n, "express de"), true);
    });
}
