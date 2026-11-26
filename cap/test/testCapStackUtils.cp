import { test, expect_eq } from "cap:test";
import {
    StackFrame,
    parseStackLine,
} from "../../packages/@cap/stack-utils/src/index.cp";

fn main() {
    test("1. @cap/stack-utils — parses named function frame with file:line:col", () => {
        const f = parseStackLine("    at handleRequest (/app/src/server.js:12:34)");
        expect_eq(f.valid, true);
        expect_eq(f.is_native, false);
        expect_eq(f.function_name, "handleRequest");
        expect_eq(f.file, "/app/src/server.js");
        expect_eq(f.line, 12);
        expect_eq(f.column, 34);
    });

    test("2. @cap/stack-utils — parses bare path frame without function name", () => {
        const f = parseStackLine("    at /path/to/file.js:88:7");
        expect_eq(f.valid, true);
        expect_eq(f.is_native, false);
        expect_eq(f.function_name, "");
        expect_eq(f.file, "/path/to/file.js");
        expect_eq(f.line, 88);
        expect_eq(f.column, 7);
    });

    test("3. @cap/stack-utils — parses anonymous frame", () => {
        const f = parseStackLine("    at Array.forEach (<anonymous>)");
        expect_eq(f.valid, true);
        expect_eq(f.is_native, false);
        expect_eq(f.function_name, "Array.forEach");
        expect_eq(f.file, "<anonymous>");
        expect_eq(f.line, 0);
        expect_eq(f.column, 0);
    });

    test("4. @cap/stack-utils — parses native frames", () => {
        const f1 = parseStackLine("    at native");
        expect_eq(f1.valid, true);
        expect_eq(f1.is_native, true);
        expect_eq(f1.function_name, "");

        const f2 = parseStackLine("    at Array.map (native)");
        expect_eq(f2.valid, true);
        expect_eq(f2.is_native, true);
        expect_eq(f2.function_name, "Array.map");
    });

    test("5. @cap/stack-utils — preserves file:// URI paths", () => {
        const f = parseStackLine("    at main (file:///data/dev/app/index.mjs:42:15)");
        expect_eq(f.valid, true);
        expect_eq(f.function_name, "main");
        expect_eq(f.file, "file:///data/dev/app/index.mjs");
        expect_eq(f.line, 42);
        expect_eq(f.column, 15);
    });

    test("6. @cap/stack-utils — skips non-frame error header and blank lines", () => {
        const f1 = parseStackLine("TypeError: Cannot read properties of undefined");
        expect_eq(f1.valid, false);
        const f2 = parseStackLine("   ");
        expect_eq(f2.valid, false);
    });
}
