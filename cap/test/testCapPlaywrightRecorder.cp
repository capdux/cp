// Source: cp/cap/test/testCapPlaywrightRecorder.cp
// Independent Verification Suite for packages/playwright/src/recorder.cp

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { CdpFrameScanner } from "cap:cdp";
import { chromium } from "../../packages/playwright/src/chromium.cp";
import {
    RecordedActionKind,
    RecordedAction,
    CodegenLang,
    RECORDER_INJECTED_SCRIPT,
    ActionRecorder,
    simdScanEscapeIndex,
    escapeJsonIntoBuf,
    escapeJsonString,
} from "../../packages/playwright/src/recorder.cp";

struct RecorderMockTransport {
    sent_count: usize = 0,
    seen_add_script: bool = false,
    seen_runtime_eval: bool = false,
    last_method: [128]u8 = undefined,
    last_method_len: usize = 0,

    fn record(*_, payload: []const u8): bool {
        _.sent_count += 1;
        if (m := CdpFrameScanner.extractTopLevelField(payload, "method", true)) {
            const mn = if (m.len <= 128) m.len else 128;
            @memcpy(_.last_method[0..mn], m[0..mn]);
            _.last_method_len = mn;
            if (std.mem.eql(u8, m, "Page.addScriptToEvaluateOnNewDocument")) {
                _.seen_add_script = true;
            } else if (std.mem.eql(u8, m, "Runtime.evaluate")) {
                _.seen_runtime_eval = true;
            }
        }
        return true;
    }

    fn lastMethod(): []const u8 {
        return _.last_method[0.._.last_method_len];
    }
}

fn recorderMockSend(ctx: usize, payload: []const u8): bool {
    const mt = unsafe { ctx as *RecorderMockTransport! };
    return mt.record(payload);
}

fn testActionRecordingAndFillMerge(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    {
        let rec = ActionRecorder.init(alloc);
        defer rec.deinit();

        rec.recordNavigate("https://app.example.com/login")?;
        // Consecutive duplicate navigate should be deduplicated
        rec.recordNavigate("https://app.example.com/login")?;
        expect_eq(rec.actionCount(), 1);

        // Consecutive fills on the same selector should merge into a single action
        rec.recordFill("#email", "a")?;
        rec.recordFill("#email", "al")?;
        rec.recordFill("#email", "alice@example.com")?;
        expect_eq(rec.actionCount(), 2);
        expect_eq(rec.merged_fill_count, 2);

        // Fill on a different selector must NOT merge with #email
        rec.recordFill("#password", "s3cr3t")?;
        expect_eq(rec.actionCount(), 3);

        rec.recordPress("#password", "Enter")?;
        rec.recordCheck("#remember")?;
        rec.recordUncheck("#newsletter")?;
        rec.recordSelectOption("#region", "us-west")?;
        rec.recordClick("[data-testid=\"submit\"]")?;
        rec.recordDblClick("#avatar")?;
        rec.recordAssertText("#welcome", "Hello, Alice!")?;

        expect_eq(rec.actionCount(), 10);
        const actions = rec.getActions();
        expect_eq(actions[0].kind == RecordedActionKind.Navigate, true);
        expect_eq(std.mem.eql(u8, actions[1].value, "alice@example.com"), true);
        expect_eq(std.mem.eql(u8, actions[2].value, "s3cr3t"), true);
        expect_eq(actions[9].kind == RecordedActionKind.AssertText, true);

        // Enum round-trip helpers
        expect_eq(std.mem.eql(u8, RecordedActionKind.SelectOption.asName(), "selectOption"), true);
        expect_eq(RecordedActionKind.fromName("assertText").? == RecordedActionKind.AssertText, true);
        expect_eq(RecordedActionKind.fromName("unknown") == null, true);
        expect_eq(CodegenLang.fromString("typescript") == CodegenLang.TypeScript, true);
        expect_eq(CodegenLang.fromString("py") == CodegenLang.Python, true);
        expect_eq(CodegenLang.fromString("cp") == CodegenLang.CP, true);

        rec.clear();
        expect_eq(rec.actionCount(), 0);
        expect_eq(rec.merged_fill_count, 0);
    }

    expect_eq(gpa.deinit() == .ok, true);
}

fn testMultiLanguageCodegen(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    {
        let rec = ActionRecorder.init(alloc);
        defer rec.deinit();

        rec.recordNavigate("https://example.com/form")?;
        rec.recordClick("[data-testid=\"open-btn\"]")?;
        rec.recordDblClick("#title")?;
        rec.recordFill("#comment", "Line1\nLine2 \"quoted\" 'single' \\path")?;
        rec.recordPress("#comment", "Enter")?;
        rec.recordCheck("#agree")?;
        rec.recordUncheck("#optout")?;
        rec.recordSelectOption("#plan", "pro")?;
        rec.recordAssertText("#status", "Saved!")?;

        // 1. CP Codegen
        const cp_code = rec.generateCode(alloc, CodegenLang.CP)?;
        defer alloc.free(cp_code);
        expect_eq(std.mem.indexOf(u8, cp_code, "import { chromium } from \"playwright\";") != null, true);
        expect_eq(std.mem.indexOf(u8, cp_code, "page.goto(\"https://example.com/form\", .{})?;") != null, true);
        expect_eq(std.mem.indexOf(u8, cp_code, "[data-testid=\\\"open-btn\\\"]") != null, true);
        expect_eq(std.mem.indexOf(u8, cp_code, ".dblclick(.{})?;") != null, true);
        expect_eq(std.mem.indexOf(u8, cp_code, "Line1\\nLine2 \\\"quoted\\\" 'single' \\\\path") != null, true);
        expect_eq(std.mem.indexOf(u8, cp_code, ".selectOption(\"pro\")?;") != null, true);
        expect_eq(std.mem.indexOf(u8, cp_code, "// Assert text: \"Saved!\"") != null, true);

        // 2. TypeScript Codegen
        const ts_code = rec.generateCode(alloc, CodegenLang.TypeScript)?;
        defer alloc.free(ts_code);
        expect_eq(std.mem.indexOf(u8, ts_code, "import { chromium, expect } from 'playwright';") != null, true);
        expect_eq(std.mem.indexOf(u8, ts_code, "await page.goto('https://example.com/form');") != null, true);
        expect_eq(std.mem.indexOf(u8, ts_code, "await page.locator('[data-testid=\"open-btn\"]').click();") != null, true);
        expect_eq(std.mem.indexOf(u8, ts_code, "Line1\\nLine2 \"quoted\" \\'single\\' \\\\path") != null, true);
        expect_eq(std.mem.indexOf(u8, ts_code, "await expect(page.locator('#status')).toContainText('Saved!');") != null, true);

        // 3. Python Codegen
        const py_code = rec.generateCode(alloc, CodegenLang.Python)?;
        defer alloc.free(py_code);
        expect_eq(std.mem.indexOf(u8, py_code, "from playwright.sync_api import sync_playwright, expect") != null, true);
        expect_eq(std.mem.indexOf(u8, py_code, "page.goto(\"https://example.com/form\")") != null, true);
        expect_eq(std.mem.indexOf(u8, py_code, ".select_option(\"pro\")") != null, true);
        expect_eq(std.mem.indexOf(u8, py_code, "expect(page.locator(\"#status\")).to_contain_text(\"Saved!\")") != null, true);
    }

    expect_eq(gpa.deinit() == .ok, true);
}

fn testSimdJsonEscapeAndCdpInjection(): !void {
    let gpa: std.heap.DebugAllocator(.{}) = .{};
    const alloc = gpa.allocator();

    {
        // SIMD clean span (>16 bytes) returns null
        const clean = "abcdefghijklmnopqrstuvwxyz0123456789_clean_selector";
        expect_eq(simdScanEscapeIndex(clean) == null, true);

        // SIMD detects special char past lane 16
        const dirty = "0123456789abcdef0123\"quote_here";
        expect_eq(simdScanEscapeIndex(dirty).?, 20);

        // Stack-buffer JSON escaping (= undefined)
        let stack_buf: [128]u8 = undefined;
        const escaped_stack = escapeJsonIntoBuf("hello\t\"world\"\\\n\x01", &stack_buf)?;
        expect_eq(std.mem.eql(u8, escaped_stack, "hello\\t\\\"world\\\"\\\\\\n\\u0001"), true);

        // Heap JSON escaping with leak check
        const escaped_heap = escapeJsonString(alloc, "a\"b\\c\nd")?;
        defer alloc.free(escaped_heap);
        expect_eq(std.mem.eql(u8, escaped_heap, "a\\\"b\\\\c\\nd"), true);

        // Verify injected script contains priority selector tiers
        expect_eq(std.mem.indexOf(u8, RECORDER_INJECTED_SCRIPT, "data-testid") != null, true);
        expect_eq(std.mem.indexOf(u8, RECORDER_INJECTED_SCRIPT, "aria-label") != null, true);
        expect_eq(std.mem.indexOf(u8, RECORDER_INJECTED_SCRIPT, "nth-of-type") != null, true);
        expect_eq(std.mem.indexOf(u8, RECORDER_INJECTED_SCRIPT, "__pwDrainRecordedActions") != null, true);

        // CDP injection over MockTransport (Page.addScriptToEvaluateOnNewDocument + Runtime.evaluate)
        let transport = RecorderMockTransport{};
        let browser = chromium.launch(alloc, .{
            headless: true,
            transport_fn: recorderMockSend,
            transport_ctx: unsafe { @intFromPtr(&transport) },
        })?;
        defer browser.deinit();

        const page = browser.newPage()?;
        let rec = ActionRecorder.initWithEndpoint(alloc, page.endpoint, page.session_id);
        defer rec.deinit();

        rec.start()?;
        expect_eq(rec.active, true);
        expect_eq(rec.injected, true);
        expect_eq(transport.seen_add_script, true);
        expect_eq(transport.seen_runtime_eval, true);

        const polled = rec.pollBrowserActions()?;
        expect_eq(polled, 0);
        expect_eq(rec.drained_polls, 1);
        expect_eq(std.mem.eql(u8, transport.lastMethod(), "Runtime.evaluate"), true);

        rec.stop()?;
        expect_eq(rec.active, false);
    }

    expect_eq(gpa.deinit() == .ok, true);
}

fn main() {
    test("Action recording and consecutive fill merge", () => {
        testActionRecordingAndFillMerge() catch unreachable;
    });
    test("Multi-language codegen (CP / TypeScript / Python)", () => {
        testMultiLanguageCodegen() catch unreachable;
    });
    test("16-lane SIMD JSON escape and CDP recorder injection", () => {
        testSimdJsonEscapeAndCdpInjection() catch unreachable;
    });
}
