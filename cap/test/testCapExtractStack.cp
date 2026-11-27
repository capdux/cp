// cap/test/testCapExtractStack.cp
// Comprehensive unit tests for @cap/extract-stack (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    extractStack,
    extractStackLines,
    extractErrorHeader,
    parseStackFrame,
    parseStackFrames,
    countStackFrames,
    StackFrame,
    STACK_FMT_V8,
    STACK_FMT_ZIG,
} from "../../packages/extract-stack/src/index.cp";

fn main() {
    test("1. @cap/extract-stack — extracts stack frames from standard single-line Error.stack", () => {
        const raw = "Error: something broke\n    at foo (/app/index.js:10:5)\n    at main (/app/index.js:25:1)";
        expect_eq(
            extractStack(raw),
            "    at foo (/app/index.js:10:5)\n    at main (/app/index.js:25:1)",
        );
        expect_eq(extractErrorHeader(raw), "Error: something broke");
    });

    test("2. @cap/extract-stack — skips multi-line error message header", () => {
        const raw = "TypeError: invalid payload\n  detail line 1\n  detail line 2\n    at parse (/src/p.js:42:9)\n    at run (/src/r.js:3:2)";
        expect_eq(
            extractStack(raw),
            "    at parse (/src/p.js:42:9)\n    at run (/src/r.js:3:2)",
        );
        expect_eq(
            extractErrorHeader(raw),
            "TypeError: invalid payload\n  detail line 1\n  detail line 2",
        );
    });

    test("3. @cap/extract-stack — extractStackLines strips indentation and 'at ' prefix", () => {
        const raw = "Error: fail\n    at alpha (a.js:1:1)\n\tat beta (b.js:2:2)\n    at gamma (c.js:3:3)\n";
        let lines: [8]string = undefined;
        const n = extractStackLines(raw, lines[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(lines[0], "alpha (a.js:1:1)");
        expect_eq(lines[1], "beta (b.js:2:2)");
        expect_eq(lines[2], "gamma (c.js:3:3)");
    });

    test("4. @cap/extract-stack — handles CRLF line endings cleanly", () => {
        const raw = "RangeError: out of bounds\r\n    at check (x.js:8:4)\r\n    at init (y.js:1:1)\r\n";
        expect_eq(extractErrorHeader(raw), "RangeError: out of bounds");
        let lines: [4]string = undefined;
        const n = extractStackLines(raw, lines[0..4]);
        expect_eq(n, 2 as usize);
        expect_eq(lines[0], "check (x.js:8:4)");
        expect_eq(lines[1], "init (y.js:1:1)");
    });

    test("5. @cap/extract-stack — handles stack-only input and no-frame input", () => {
        const onlyFrames = "    at direct (d.js:5:5)\n    at top (t.js:1:1)";
        expect_eq(extractStack(onlyFrames), onlyFrames);
        expect_eq(extractErrorHeader(onlyFrames), "");

        const noFrames = "Error: plain message without stack\nsecond line\n";
        expect_eq(extractStack(noFrames), "");
        expect_eq(extractErrorHeader(noFrames), "Error: plain message without stack\nsecond line");
    });

    test("6. @cap/extract-stack — empty input and bounded output slice capacity", () => {
        expect_eq(extractStack(""), "");
        expect_eq(extractErrorHeader(""), "");

        let small: [2]string = undefined;
        expect_eq(extractStackLines("", small[0..2]), 0 as usize);

        const raw = "Error: many\n    at f1 (1.js:1:1)\n    at f2 (2.js:2:2)\n    at f3 (3.js:3:3)";
        const n = extractStackLines(raw, small[0..2]);
        expect_eq(n, 2 as usize);
        expect_eq(small[0], "f1 (1.js:1:1)");
        expect_eq(small[1], "f2 (2.js:2:2)");
    });

    test("7. @cap/extract-stack — parseStackFrame handles V8/Node async, anonymous, native & Zig frames", () => {
        const f1 = parseStackFrame("    at async fetchData (/app/net.js:42:13)");
        expect_eq(f1.valid, true);
        expect_eq(f1.format, STACK_FMT_V8);
        expect_eq(f1.is_async, true);
        expect_eq(f1.function_name, "fetchData");
        expect_eq(f1.file, "/app/net.js");
        expect_eq(f1.line, 42 as u32);
        expect_eq(f1.column, 13 as u32);

        const f2 = parseStackFrame("    at /app/index.js:10:5");
        expect_eq(f2.valid, true);
        expect_eq(f2.format, STACK_FMT_V8);
        expect_eq(f2.function_name, "");
        expect_eq(f2.file, "/app/index.js");
        expect_eq(f2.line, 10 as u32);
        expect_eq(f2.column, 5 as u32);

        const f3 = parseStackFrame("    at Array.map (native)");
        expect_eq(f3.valid, true);
        expect_eq(f3.is_native, true);
        expect_eq(f3.function_name, "Array.map");

        const f4 = parseStackFrame("/data/dev/cp/cap/src/main.zig:2496:50: 0x1281ec8 in compileRunBinary (main.zig)");
        expect_eq(f4.valid, true);
        expect_eq(f4.format, STACK_FMT_ZIG);
        expect_eq(f4.file, "/data/dev/cp/cap/src/main.zig");
        expect_eq(f4.line, 2496 as u32);
        expect_eq(f4.column, 50 as u32);
        expect_eq(f4.function_name, "compileRunBinary");
    });

    test("8. @cap/extract-stack — parseStackFrames & countStackFrames across V8 and Zig traces", () => {
        const zig_trace = "error: CompileFailed\n/src/a.zig:10:2: 0x100 in run (app)\n/src/b.zig:20:4: 0x200 in main (app)\n";
        expect_eq(countStackFrames(zig_trace), 2 as usize);

        let frames: [4]StackFrame = undefined;
        const n = parseStackFrames(zig_trace, frames[0..4]);
        expect_eq(n, 2 as usize);
        expect_eq(frames[0].function_name, "run");
        expect_eq(frames[0].line, 10 as u32);
        expect_eq(frames[1].function_name, "main");
        expect_eq(frames[1].line, 20 as u32);
    });
}
