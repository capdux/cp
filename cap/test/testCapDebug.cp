// cap/test/testCapDebug.cp
// Comprehensive unit tests for @cap/debug

import { test, expect_eq } from "cap:test";
import {
    DebugFilter,
    DebugLogger,
    isNamespaceEnabled,
    selectColor,
    formatDiffMs,
    formatDebugLine,
} from "../../packages/debug/src/index.cp";

fn main() {
    test("1. @cap/debug — Wildcard '*' all-match & empty pattern disabled", () => {
        expect_eq(isNamespaceEnabled("*", "http:req"), true);
        expect_eq(isNamespaceEnabled("*", "db:pool:conn"), true);
        expect_eq(isNamespaceEnabled("", "http:req"), false);
        expect_eq(isNamespaceEnabled("*", ""), false);
    });

    test("2. @cap/debug — Multi-prefix & '-' exclusion precedence", () => {
        const pat = "http:*,db:*,-http:verbose";
        expect_eq(isNamespaceEnabled(pat, "http:req"), true);
        expect_eq(isNamespaceEnabled(pat, "db:query"), true);
        expect_eq(isNamespaceEnabled(pat, "http:verbose"), false);
        expect_eq(isNamespaceEnabled(pat, "cache:get"), false);

        // Exclusion overrides '*' even when listed before '*'
        const pat2 = "-noisy:*, *";
        expect_eq(isNamespaceEnabled(pat2, "noisy:heartbeat"), false);
        expect_eq(isNamespaceEnabled(pat2, "app:startup"), true);

        const filter = DebugFilter.from(pat);
        expect_eq(filter.isEnabled("http:res"), true);
        expect_eq(filter.isEnabled("http:verbose"), false);
    });

    test("3. @cap/debug — Middle & multi-star glob patterns (a:*:c)", () => {
        expect_eq(isNamespaceEnabled("a:*:c", "a:b:c"), true);
        expect_eq(isNamespaceEnabled("a:*:c", "a:foo:bar:c"), true);
        expect_eq(isNamespaceEnabled("a:*:c", "a:b:d"), false);
        expect_eq(isNamespaceEnabled("*error*", "net:error:timeout"), true);
        expect_eq(isNamespaceEnabled("app:auth app:billing", "app:billing"), true);
    });

    test("4. @cap/debug — Deterministic ANSI color selection", () => {
        const c1 = selectColor("http:req");
        const c2 = selectColor("http:req");
        expect_eq(c1, c2);
        expect_eq(c1 >= (31 as u8) && c1 <= (36 as u8), true);

        const c3 = selectColor("db:query");
        expect_eq(c3 >= (31 as u8) && c3 <= (36 as u8), true);
    });

    test("5. @cap/debug — formatDiffMs across ms/s/m/h boundaries", () => {
        let out: [32]u8 = undefined;
        expect_eq(formatDiffMs(-5, out[0..32]), "+0ms");
        expect_eq(formatDiffMs(0, out[0..32]), "+0ms");
        expect_eq(formatDiffMs(42, out[0..32]), "+42ms");
        expect_eq(formatDiffMs(2500, out[0..32]), "+2s");
        expect_eq(formatDiffMs(180000, out[0..32]), "+3m");
        expect_eq(formatDiffMs(3600000, out[0..32]), "+1h");
    });

    test("6. @cap/debug — formatDebugLine (plain & ANSI colored) and DebugLogger", () => {
        let out: [256]u8 = undefined;
        const plain = formatDebugLine("http:req", "GET /api", 12, false, out[0..256]);
        expect_eq(plain, "http:req GET /api +12ms");

        const colored = formatDebugLine("http:req", "GET /api", 12, true, out[0..256]);
        expect_eq(colored.len > plain.len, true);
        expect_eq(colored[0], 0x1B as u8);

        const log_on = DebugLogger.create("http:*", "http:req");
        expect_eq(log_on.enabled, true);
        expect_eq(log_on.formatLine("POST /v1", 5, false, out[0..256]), "http:req POST /v1 +5ms");

        const log_off = DebugLogger.create("db:*", "http:req");
        expect_eq(log_off.enabled, false);
        expect_eq(log_off.formatLine("POST /v1", 5, false, out[0..256]), "");
    });
}
