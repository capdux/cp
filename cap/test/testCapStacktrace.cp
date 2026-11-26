// cap/test/testCapStacktrace.cp
// Comprehensive unit tests for @cap/stacktrace

import { test, expect_eq } from "cap:test";
import {
    parseFrameLine,
    parseStack,
    formatFrame,
    cleanStack,
} from "../../packages/@cap/stacktrace/src/index.cp";

fn main() {
    test("1. @cap/stacktrace — V8/Node/Chrome/Bun stack frame variants", () => {
        const f1 = parseFrameLine("    at myFunc (/app/src/main.ts:42:13)");
        expect_eq(f1 != null, true);
        expect_eq(f1.?.func_name, "myFunc");
        expect_eq(f1.?.file, "/app/src/main.ts");
        expect_eq(f1.?.line, 42 as u32);
        expect_eq(f1.?.column, 13 as u32);
        expect_eq(f1.?.is_native, false);
        expect_eq(f1.?.is_internal, false);

        const f2 = parseFrameLine("    at /app/src/main.ts:42:13");
        expect_eq(f2 != null, true);
        expect_eq(f2.?.func_name, "<anonymous>");
        expect_eq(f2.?.file, "/app/src/main.ts");
        expect_eq(f2.?.line, 42 as u32);
        expect_eq(f2.?.column, 13 as u32);

        const f3 = parseFrameLine("    at Object.<anonymous> (node:internal/modules:10:5)");
        expect_eq(f3 != null, true);
        expect_eq(f3.?.is_internal, true);

        const f4 = parseFrameLine("    at Array.forEach (<anonymous>)");
        expect_eq(f4 != null, true);
        expect_eq(f4.?.is_internal, true);

        const f5 = parseFrameLine("    at native");
        expect_eq(f5 != null, true);
        expect_eq(f5.?.is_native, true);
        expect_eq(f5.?.is_internal, true);
    });

    test("2. @cap/stacktrace — SpiderMonkey/Firefox/Safari frames", () => {
        const f1 = parseFrameLine("myFunc@https://example.com/app.js:42:13");
        expect_eq(f1 != null, true);
        expect_eq(f1.?.func_name, "myFunc");
        expect_eq(f1.?.file, "https://example.com/app.js");
        expect_eq(f1.?.line, 42 as u32);
        expect_eq(f1.?.column, 13 as u32);

        const f2 = parseFrameLine("@app.js:10:2");
        expect_eq(f2 != null, true);
        expect_eq(f2.?.func_name, "<anonymous>");
        expect_eq(f2.?.file, "app.js");
        expect_eq(f2.?.line, 10 as u32);
        expect_eq(f2.?.column, 2 as u32);

        const f3 = parseFrameLine("forEach@[native code]");
        expect_eq(f3 != null, true);
        expect_eq(f3.?.is_native, true);
        expect_eq(f3.?.is_internal, true);
    });

    test("3. @cap/stacktrace — Rust/Zig/CP stack frame formats", () => {
        const f1 = parseFrameLine("src/main.cp:88:12");
        expect_eq(f1 != null, true);
        expect_eq(f1.?.file, "src/main.cp");
        expect_eq(f1.?.line, 88 as u32);
        expect_eq(f1.?.column, 12 as u32);
        expect_eq(f1.?.is_internal, false);

        const f2 = parseFrameLine("0x1234 in handleRequest (src/server.zig:120)");
        expect_eq(f2 != null, true);
        expect_eq(f2.?.func_name, "handleRequest");
        expect_eq(f2.?.file, "src/server.zig");
        expect_eq(f2.?.line, 120 as u32);
        expect_eq(f2.?.column, 0 as u32);
        expect_eq(f2.?.is_internal, false);
    });

    test("4. @cap/stacktrace — parseStack full trace with error message", () => {
        const raw = "TypeError: Cannot read property 'x'\n    at compute (/app/calc.ts:15:9)\n    at main (/app/index.ts:3:1)\n    at Module._compile (node:internal/modules/cjs/loader:1105:14)";
        const parsed = parseStack(raw);

        expect_eq(parsed.message, "TypeError: Cannot read property 'x'");
        expect_eq(parsed.count, 3 as usize);
        expect_eq(parsed.frameAt(0 as usize).func_name, "compute");
        expect_eq(parsed.frameAt(1 as usize).func_name, "main");
        expect_eq(parsed.frameAt(2 as usize).is_internal, true);
    });

    test("5. @cap/stacktrace — cleanStack filters runtime internal frames", () => {
        const raw = "Error: Database timeout\n    at queryDb (/app/db.ts:77:5)\n    at processTicks (node:internal/process/task_queues:95:5)\n    at Array.forEach (<anonymous>)\n    at handleUser (/app/routes.ts:21:11)\n    at native";
        let buf: [1024]u8 = undefined;
        const cleaned = cleanStack(raw, buf[0..1024]);

        expect_eq(cleaned, "Error: Database timeout\n    at queryDb (/app/db.ts:77:5)\n    at handleUser (/app/routes.ts:21:11)");
    });

    test("6. @cap/stacktrace — formatFrame & non-frame lines handled safely", () => {
        expect_eq(parseFrameLine("") == null, true);
        expect_eq(parseFrameLine("Error: something failed") == null, true);
        expect_eq(parseFrameLine("contact admin@example.com for help") == null, true);

        let buf: [256]u8 = undefined;
        const f1 = parseFrameLine("0x1234 in handleRequest (src/server.zig:120)").?;
        expect_eq(formatFrame(f1, buf[0..256]), "    at handleRequest (src/server.zig:120)");

        const f2 = parseFrameLine("src/main.cp:88:12").?;
        expect_eq(formatFrame(f2, buf[0..256]), "    at src/main.cp:88:12");
    });
}
