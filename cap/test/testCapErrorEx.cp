import { test, expect_eq } from "cap:test";
import {
    ErrorField,
    StructuredError,
    formatStructuredError,
    formatErrorTemplate,
} from "../../packages/error-ex/src/index.cp";

fn main() {
    test("1. @cap/error-ex — formats basic StructuredError with name, code, and message", () => {
        const e = StructuredError{
            name: "JSONError",
            code: 400,
            message: "Unexpected token",
        };
        let fields: [1]ErrorField = undefined;
        let buf: [128]u8 = undefined;
        const n = formatStructuredError(e, fields[0..0], buf[0..128]);
        expect_eq(buf[0..n], "JSONError [400]: Unexpected token");
    });

    test("2. @cap/error-ex — omits code when zero and defaults empty name to Error", () => {
        const e = StructuredError{
            name: "",
            code: 0,
            message: "Something went wrong",
        };
        let fields: [1]ErrorField = undefined;
        let buf: [128]u8 = undefined;
        const n = formatStructuredError(e, fields[0..0], buf[0..128]);
        expect_eq(buf[0..n], "Error: Something went wrong");
    });

    test("3. @cap/error-ex — formats inline context fields in parentheses", () => {
        const e = StructuredError{
            name: "ParseError",
            code: 102,
            message: "Invalid syntax",
        };
        let fields: [2]ErrorField = undefined;
        fields[0] = ErrorField{ key: "file", value: "app.cp", line_mode: false };
        fields[1] = ErrorField{ key: "line", value: "42", line_mode: false };

        let buf: [256]u8 = undefined;
        const n = formatStructuredError(e, fields[0..2], buf[0..256]);
        expect_eq(buf[0..n], "ParseError [102]: Invalid syntax (file=app.cp, line=42)");
    });

    test("4. @cap/error-ex — formats mixed inline and line_mode fields", () => {
        const e = StructuredError{
            name: "IOError",
            code: -2,
            message: "Cannot open file",
        };
        let fields: [3]ErrorField = undefined;
        fields[0] = ErrorField{ key: "op", value: "read", line_mode: false };
        fields[1] = ErrorField{ key: "path", value: "/etc/hosts", line_mode: true };
        fields[2] = ErrorField{ key: "hint", value: "check permissions", line_mode: true };

        let buf: [256]u8 = undefined;
        const n = formatStructuredError(e, fields[0..3], buf[0..256]);
        expect_eq(buf[0..n], "IOError [-2]: Cannot open file (op=read)\n  path: /etc/hosts\n  hint: check permissions");
    });

    test("5. @cap/error-ex — formatErrorTemplate replaces all {key} placeholders", () => {
        let buf: [128]u8 = undefined;
        const n = formatErrorTemplate("File {file} failed while reading {file}", "file", "cap.json", buf[0..128]);
        expect_eq(buf[0..n], "File cap.json failed while reading cap.json");
    });

    test("6. @cap/error-ex — formatErrorTemplate preserves unknown placeholders and truncates safely", () => {
        let buf: [64]u8 = undefined;
        const n1 = formatErrorTemplate("Err in {other}: {msg}", "msg", "EOF", buf[0..64]);
        expect_eq(buf[0..n1], "Err in {other}: EOF");

        let small: [10]u8 = undefined;
        const n2 = formatErrorTemplate("Hello {name}!", "name", "Antigravity", small[0..10]);
        expect_eq(n2, 10 as usize);
        expect_eq(small[0..n2], "Hello Anti");
    });
}
