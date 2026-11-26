import { test, expect_eq } from "cap:test";
import {
    CleanStackOptions,
    isInternalStackFrame,
    cleanStack,
    cleanStackWithOptions,
} from "../../packages/@cap/clean-stack/src/index.cp";

fn main() {
    test("1. @cap/clean-stack — filters node:internal and internal frames", () => {
        const raw = "Error: boom\n    at foo (/app/src/index.js:10:5)\n    at Module._compile (node:internal/modules/cjs/loader:1256:14)\n    at bar (/app/src/util.js:3:1)\n    at processTicks (internal/process/task_queues.js:95:5)";
        let buf: [512]u8 = undefined;
        let n = cleanStack(raw, buf[0..512]);
        expect_eq(buf[0..n], "Error: boom\n    at foo (/app/src/index.js:10:5)\n    at bar (/app/src/util.js:3:1)");
    });

    test("2. @cap/clean-stack — strips file:// URL prefix", () => {
        const raw = "Error: esm\n    at run (file:///srv/app/main.mjs:4:2)";
        let buf: [256]u8 = undefined;
        let n = cleanStack(raw, buf[0..256]);
        expect_eq(buf[0..n], "Error: esm\n    at run (/srv/app/main.mjs:4:2)");
    });

    test("3. @cap/clean-stack — base_path strips project root prefix", () => {
        const raw = "Error: fail\n    at handler (/workspace/proj/src/api.cp:42:9)";
        let opts = CleanStackOptions { pretty: false, base_path: "/workspace/proj", home_dir: "" };
        let buf: [256]u8 = undefined;
        let n = cleanStackWithOptions(raw, opts, buf[0..256]);
        expect_eq(buf[0..n], "Error: fail\n    at handler (src/api.cp:42:9)");
    });

    test("4. @cap/clean-stack — pretty mode replaces home_dir with ~", () => {
        const raw = "Error: home\n    at main (/home/alice/dev/app.cp:12:3)";
        let opts = CleanStackOptions { pretty: true, base_path: "", home_dir: "/home/alice" };
        let buf: [256]u8 = undefined;
        let n = cleanStackWithOptions(raw, opts, buf[0..256]);
        expect_eq(buf[0..n], "Error: home\n    at main (~/dev/app.cp:12:3)");
    });

    test("5. @cap/clean-stack — isInternalStackFrame detects native & anonymous frames", () => {
        expect_eq(isInternalStackFrame("    at Array.map (native)"), true);
        expect_eq(isInternalStackFrame("    at <anonymous>:1:1"), true);
        expect_eq(isInternalStackFrame("    at myFn (/app/a.js:1:1)"), false);
        expect_eq(isInternalStackFrame("Error: node:internal/test"), false);
    });

    test("6. @cap/clean-stack — handles empty input and CRLF line endings", () => {
        let buf: [128]u8 = undefined;
        expect_eq(cleanStack("", buf[0..128]), 0 as usize);
        let n = cleanStack("Error: x\r\n    at a (/a.js:1:1)\r\n    at b (node:internal/foo:1:1)\r\n", buf[0..128]);
        expect_eq(buf[0..n], "Error: x\n    at a (/a.js:1:1)");
    });
}
