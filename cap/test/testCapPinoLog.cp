import { test, expect_eq } from "cap:test";
import {
    PinoLogger, levelLabel,
    PINO_TRACE, PINO_DEBUG, PINO_INFO, PINO_WARN, PINO_ERROR, PINO_FATAL, PINO_SILENT
} from "../../packages/pino/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/pino-log — Basic NDJSON line with level, time, pid & hostname", () => {
        let log = PinoLogger.new(PINO_INFO, "api", 1234, "node1");
        let out: [256]u8 = undefined;
        let n = log.formatLine(PINO_INFO, 1700000000000, "server started", out[0..256]);
        expect_eq(eqBuf(out[0..n], n, "{\"level\":30,\"time\":1700000000000,\"pid\":1234,\"hostname\":\"node1\",\"name\":\"api\",\"msg\":\"server started\"}\n"), true);
    });

    test("2. @cap/pino-log — Level filtering & PINO_SILENT suppression", () => {
        let log = PinoLogger.new(PINO_WARN, "", 0, "");
        let out: [128]u8 = undefined;
        expect_eq(log.isLevelEnabled(PINO_INFO), false);
        expect_eq(log.formatLine(PINO_INFO, 1, "ignored", out[0..128]), 0);
        expect_eq(log.isLevelEnabled(PINO_ERROR), true);
        expect_eq(log.formatLine(PINO_ERROR, 1, "boom", out[0..128]) > 0, true);

        let silent = PinoLogger.new(PINO_SILENT, "", 0, "");
        expect_eq(silent.formatLine(PINO_FATAL, 1, "never", out[0..128]), 0);
    });

    test("3. @cap/pino-log — Child bindings (str, int, bool)", () => {
        let log = PinoLogger.new(PINO_DEBUG, "", 0, "");
        log.bindStr("reqId", "abc-9");
        log.bindInt("status", 200);
        log.bindBool("cached", true);
        let out: [256]u8 = undefined;
        let n = log.formatLine(PINO_DEBUG, 42, "ok", out[0..256]);
        expect_eq(eqBuf(out[0..n], n, "{\"level\":20,\"time\":42,\"reqId\":\"abc-9\",\"status\":200,\"cached\":true,\"msg\":\"ok\"}\n"), true);
    });

    test("4. @cap/pino-log — JSON escaping of quotes, backslashes & newlines", () => {
        let log = PinoLogger.new(PINO_INFO, "", 0, "");
        let out: [256]u8 = undefined;
        let n = log.formatLine(PINO_ERROR, 99, "line1\n\"quoted\"\\end", out[0..256]);
        expect_eq(eqBuf(out[0..n], n, "{\"level\":50,\"time\":99,\"msg\":\"line1\\n\\\"quoted\\\"\\\\end\"}\n"), true);
    });

    test("5. @cap/pino-log — levelLabel canonical names", () => {
        expect_eq(levelLabel(PINO_TRACE), "trace");
        expect_eq(levelLabel(PINO_DEBUG), "debug");
        expect_eq(levelLabel(PINO_INFO), "info");
        expect_eq(levelLabel(PINO_WARN), "warn");
        expect_eq(levelLabel(PINO_ERROR), "error");
        expect_eq(levelLabel(PINO_FATAL), "fatal");
        expect_eq(levelLabel(PINO_SILENT), "silent");
    });

    test("6. @cap/pino-log — Negative integers & small buffer truncation safety", () => {
        let log = PinoLogger.new(PINO_INFO, "", 0, "");
        log.bindInt("delta", -512);
        let out: [128]u8 = undefined;
        let n = log.formatLine(PINO_INFO, 7, "neg", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "{\"level\":30,\"time\":7,\"delta\":-512,\"msg\":\"neg\"}\n"), true);
        let tiny: [10]u8 = undefined;
        expect_eq(log.formatLine(PINO_INFO, 7, "neg", tiny[0..10]), 10);
    });
}
