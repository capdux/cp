import { test, expect_eq } from "cap:test";
import { FormatOptions, formatJson, formatCss } from "../../packages/@cap/prettier-fmt/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/prettier-fmt — Formats compact JSON with 2-space indent", () => {
        let buf: [256]u8 = undefined;
        let n = formatJson("{\"a\":1,\"b\":[true,false]}", FormatOptions.defaults(), buf[0..256]);
        let exp = "{\n  \"a\": 1,\n  \"b\": [\n    true,\n    false\n  ]\n}\n";
        expect_eq(eqBytes(buf[0..256], n, exp), true);
    });

    test("2. @cap/prettier-fmt — Keeps empty objects and arrays compact", () => {
        let buf: [128]u8 = undefined;
        let opts = FormatOptions.defaults();
        opts.trailing_newline = false;
        let n = formatJson("{\"emptyObj\":{},\"emptyArr\":[]}", opts, buf[0..128]);
        let exp = "{\n  \"emptyObj\": {},\n  \"emptyArr\": []\n}";
        expect_eq(eqBytes(buf[0..128], n, exp), true);
    });

    test("3. @cap/prettier-fmt — Preserves strings with colons, commas, braces, and escapes", () => {
        let buf: [128]u8 = undefined;
        let opts = FormatOptions.defaults();
        opts.trailing_newline = false;
        let n = formatJson("{\"msg\":\"a:b, {c} \\\"ok\\\"\"}", opts, buf[0..128]);
        let exp = "{\n  \"msg\": \"a:b, {c} \\\"ok\\\"\"\n}";
        expect_eq(eqBytes(buf[0..128], n, exp), true);
    });

    test("4. @cap/prettier-fmt — Supports tab indentation and 4-space indentation", () => {
        let buf: [128]u8 = undefined;
        let tab_opts = FormatOptions.defaults();
        tab_opts.use_tabs = true;
        tab_opts.trailing_newline = false;
        let n1 = formatJson("{\"x\":1}", tab_opts, buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n1, "{\n\t\"x\": 1\n}"), true);

        let four_opts = FormatOptions.defaults();
        four_opts.tab_width = 4;
        four_opts.trailing_newline = false;
        let n2 = formatJson("{\"x\":1}", four_opts, buf[0..128]);
        expect_eq(eqBytes(buf[0..128], n2, "{\n    \"x\": 1\n}"), true);
    });

    test("5. @cap/prettier-fmt — Formats minified CSS cleanly", () => {
        let buf: [256]u8 = undefined;
        let opts = FormatOptions.defaults();
        opts.trailing_newline = false;
        let n = formatCss("body{color:red;margin:0;}h1{font-size:24px;}", opts, buf[0..256]);
        let exp = "body {\n  color: red;\n  margin: 0;\n}\n\nh1 {\n  font-size: 24px;\n}";
        expect_eq(eqBytes(buf[0..256], n, exp), true);
    });

    test("6. @cap/prettier-fmt — Handles empty input and tiny buffer safely", () => {
        let buf: [8]u8 = undefined;
        expect_eq(formatJson("", FormatOptions.defaults(), buf[0..8]), 0);
        expect_eq(formatCss("", FormatOptions.defaults(), buf[0..8]), 0);
        let n = formatJson("{\"long_key\":12345}", FormatOptions.defaults(), buf[0..8]);
        expect_eq(n <= 8, true);
    });
}
