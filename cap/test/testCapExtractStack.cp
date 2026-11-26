// cap/test/testCapExtractStack.cp
// Comprehensive unit tests for @cap/extract-stack (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    extractStack,
    extractStackLines,
    extractErrorHeader,
} from "../../packages/@cap/extract-stack/src/index.cp";

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
}
