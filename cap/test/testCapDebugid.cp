import { test, expect_eq } from "cap:test";
import {
    parseDebugId,
    formatBreakpad,
    formatCanonical,
} from "../../packages/@cap/debugid/src/index.cp";

fn main() {
    test("1. @cap/debugid — parses Breakpad debug ID and formats canonical", () => {
        const id = parseDebugId("DFB8E43AF2423D73A453AEB6A777EF751");
        expect_eq(id.valid, true);
        expect_eq(id.appendix, 1 as u32);
        expect_eq(id.isNil(), false);

        let buf: [64]u8 = [0; 64];
        const n = formatCanonical(id, buf[0..64]);
        expect_eq(buf[0..n] as string, "dfb8e43a-f242-3d73-a453-aeb6a777ef75-1");
    });

    test("2. @cap/debugid — parses canonical Sentry ID and formats Breakpad", () => {
        const id = parseDebugId("dfb8e43a-f242-3d73-a453-aeb6a777ef75-a");
        expect_eq(id.valid, true);
        expect_eq(id.appendix, 10 as u32);

        let buf: [64]u8 = [0; 64];
        const n = formatBreakpad(id, buf[0..64]);
        expect_eq(buf[0..n] as string, "DFB8E43AF2423D73A453AEB6A777EF75A");
    });

    test("3. @cap/debugid — handles zero appendix omission in canonical format", () => {
        const id = parseDebugId("dfb8e43a-f242-3d73-a453-aeb6a777ef75");
        expect_eq(id.valid, true);
        expect_eq(id.appendix, 0 as u32);

        let cbuf: [64]u8 = [0; 64];
        const cn = formatCanonical(id, cbuf[0..64]);
        expect_eq(cbuf[0..cn] as string, "dfb8e43a-f242-3d73-a453-aeb6a777ef75");

        let bbuf: [64]u8 = [0; 64];
        const bn = formatBreakpad(id, bbuf[0..64]);
        expect_eq(bbuf[0..bn] as string, "DFB8E43AF2423D73A453AEB6A777EF750");
    });

    test("4. @cap/debugid — detects nil DebugId", () => {
        const nil_id = parseDebugId("00000000-0000-0000-0000-000000000000");
        expect_eq(nil_id.valid, true);
        expect_eq(nil_id.isNil(), true);
    });

    test("5. @cap/debugid — supports multi-hex PDB age appendix", () => {
        const id = parseDebugId("DFB8E43AF2423D73A453AEB6A777EF751A2B");
        expect_eq(id.valid, true);
        expect_eq(id.appendix, 0x1A2B as u32);
    });

    test("6. @cap/debugid — rejects too-short or non-hex inputs", () => {
        expect_eq(parseDebugId("short").valid, false);
        expect_eq(parseDebugId("ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ0").valid, false);
        expect_eq(parseDebugId("dfb8e43a-f242-3d73-a453-aeb6a777ef75-").valid, false);
    });
}
