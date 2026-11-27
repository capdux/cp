import { test, expect_eq } from "cap:test";
import {
    DepdOptions,
    Deprecator,
    formatDeprecation,
    formatDeprecationLen,
    formatDeprecationWithTrace,
    isNamespaceSilenced,
    isNamespaceTraced,
} from "../../packages/depd/src/index.cp";

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
    test("1. depd — formatDeprecation with and without callsite", () => {
        let out: [128]u8 = undefined;
        let n1 = formatDeprecation("express", "res.send(status)", "app.cp:42", out[0..128]);
        expect_eq(eqBuf(out[0..128], n1, "express deprecated res.send(status) at app.cp:42"), true);
        expect_eq(n1, formatDeprecationLen("express", "res.send(status)", "app.cp:42"));

        let n2 = formatDeprecation("body-parser", "undefined extended", "", out[0..128]);
        expect_eq(eqBuf(out[0..128], n2, "body-parser deprecated undefined extended"), true);
        expect_eq(n2, formatDeprecationLen("body-parser", "undefined extended", ""));
    });

    test("2. depd — warnOnce deduplicates repeated messages", () => {
        let d = Deprecator.new("express");
        let out: [128]u8 = undefined;
        let n1 = d.warnOnce("old api", "main.cp:10", out[0..128]);
        expect_eq(n1 > 0, true);
        expect_eq(d.hasWarned("old api"), true);
        expect_eq(d.seenCount(), 1 as usize);
        let n2 = d.warnOnce("old api", "main.cp:20", out[0..128]);
        expect_eq(n2, 0 as usize);
        expect_eq(d.emitted_count, 1 as u64);
    });

    test("3. depd — distinct messages emit independently", () => {
        let d = Deprecator.new("send");
        let out: [128]u8 = undefined;
        expect_eq(d.warnOnce("msg-a", "", out[0..128]) > 0, true);
        expect_eq(d.warnOnce("msg-b", "", out[0..128]) > 0, true);
        expect_eq(d.emitted_count, 2 as u64);
        expect_eq(d.seenCount(), 2 as usize);
    });

    test("4. depd — silenced & noDeprecation modes suppress all warnings", () => {
        let d = Deprecator.new("serve-static");
        d.setSilenced(true);
        expect_eq(d.isSilenced(), true);
        expect_eq(d.no_deprecation, true);
        let out: [128]u8 = undefined;
        expect_eq(d.warnOnce("legacy opt", "srv.cp:1", out[0..128]), 0 as usize);
        expect_eq(d.hasWarned("legacy opt"), false);
        expect_eq(d.emitted_count, 0 as u64);

        d.setNoDeprecation(false);
        expect_eq(d.isSilenced(), false);
        expect_eq(d.deprecate("legacy opt", "srv.cp:1", out[0..128]) > 0, true);
    });

    test("5. depd — reset clears seen cache and counter", () => {
        let d = Deprecator.new("koa");
        let out: [128]u8 = undefined;
        expect_eq(d.warnOnce("req.foo", "", out[0..128]) > 0, true);
        d.reset();
        expect_eq(d.hasWarned("req.foo"), false);
        expect_eq(d.seenCount(), 0 as usize);
        expect_eq(d.warnOnce("req.foo", "", out[0..128]) > 0, true);
    });

    test("6. depd — small output buffer truncates safely", () => {
        let out: [10]u8 = undefined;
        let n = formatDeprecation("express", "very long deprecation message", "a.cp:1", out[0..10]);
        expect_eq(n, 10 as usize);
        expect_eq(eqBuf(out[0..10], n, "express de"), true);
    });

    test("7. depd — traceDeprecation attaches stack trace & withOptions works", () => {
        const opts = DepdOptions { no_deprecation: false, trace_deprecation: true };
        let d = Deprecator.withOptions("router", opts);
        expect_eq(d.isTraceEnabled(), true);
        expect_eq(d.isSilenced(), false);

        let out: [256]u8 = undefined;
        const trace = "    at handle (router.cp:88)";
        const n = d.warnWithTrace("oldRoute()", "app.cp:12", trace, out[0..256]);
        expect_eq(
            eqBuf(out[0..256], n, "router deprecated oldRoute() at app.cp:12\n    at handle (router.cp:88)"),
            true
        );
        // Second call is deduplicated
        expect_eq(d.warnWithTrace("oldRoute()", "app.cp:12", trace, out[0..256]), 0 as usize);

        // When trace_deprecation is disabled, trace is omitted
        d.setTraceDeprecation(false);
        const n2 = d.warnWithTrace("other()", "app.cp:15", trace, out[0..256]);
        expect_eq(eqBuf(out[0..256], n2, "router deprecated other() at app.cp:15"), true);

        // Direct formatDeprecationWithTrace helper
        const n3 = formatDeprecationWithTrace("ns", "m", "", "stack", true, out[0..256]);
        expect_eq(eqBuf(out[0..256], n3, "ns deprecated m\nstack"), true);
    });

    test("8. depd — deprecateFn & NO_DEPRECATION / TRACE_DEPRECATION env matching", () => {
        let d = Deprecator.new("http");
        let out: [128]u8 = undefined;
        const n1 = d.deprecateFn("createClient", "", "net.cp:5", out[0..128]);
        expect_eq(eqBuf(out[0..128], n1, "http deprecated createClient at net.cp:5"), true);
        const n2 = d.deprecateFn("oldFn", "use newFn instead", "", out[0..128]);
        expect_eq(eqBuf(out[0..128], n2, "http deprecated use newFn instead"), true);

        expect_eq(isNamespaceSilenced("express", "*"), true);
        expect_eq(isNamespaceSilenced("express", "body-parser, EXPRESS , send"), true);
        expect_eq(isNamespaceSilenced("koa", "body-parser, express"), false);
        expect_eq(isNamespaceSilenced("koa", ""), false);

        expect_eq(isNamespaceTraced("router", "router,http"), true);
        expect_eq(isNamespaceTraced("router", "other"), false);
    });
}
