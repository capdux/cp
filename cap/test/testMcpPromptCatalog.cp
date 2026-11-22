// Origin: Pure-CP Stdlib Test Suite (lib/mcp/promptCatalog.cp) | Created: 2026-11-22
// Production Deep Verification Suite for MCP Prompt Catalog & Argument Completion Engine:
//   1. Prompt registration, inline storage isolation, & lookup APIs
//   2. Cursor-paginated prompts/list traversal, buffer clamping, & invalid cursor/limit guards
//   3. prompts/get valid invocations (required + optional + non-enforced completion choices)
//   4. prompts/get contract violations (not found, missing required, unknown, duplicate, too long, bad enum)
//   5. completion/complete case-insensitive ASCII prefix matching & unknown prompt/arg handling
//   6. completion/complete output truncation & exact has_more / total_matches reporting
//   7. Registration boundary guards (empty/overlong name, duplicate, >8 args, invalid arg specs)
//   8. Full-catalog (32 entries) capacity_full guard & clear() reuse recovery

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MCP_PROMPT_MAX_ENTRIES,
    MCP_PROMPT_MAX_ARGS,
    McpPromptRegisterStatus,
    McpPromptArgSpec,
    McpPromptArgValue,
    McpPromptViolationKind,
    McpCompletionCandidate,
    McpPromptCatalog,
} from "cap:mcp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("mcpPromptCatalog: registration, inline storage & exact lookup", () => {
    let cat = McpPromptCatalog.init();
    const empty_args: [0]McpPromptArgSpec = [];
    expect_eq(cat.registerPrompt("daily_standup", "Generate daily standup summary", empty_args[0..]), McpPromptRegisterStatus.ok);

    const langs: [3][]const u8 = ["rust", "cp", "zig"];
    const review_args: [2]McpPromptArgSpec = [
        McpPromptArgSpec.withChoices("language", true, true, langs[0..]),
        McpPromptArgSpec.plain("focus", false),
    ];
    expect_eq(cat.registerPrompt("code_review", "Review pull request diff", review_args[0..]), McpPromptRegisterStatus.ok);

    expect_eq(cat.count(), 2 as usize);
    const idx = cat.findPromptIndex("code_review") ?? unreachable;
    expect_eq(idx, 1 as usize);
    expect_eq(cat.findPromptIndex("missing") == null, true);

    const entry = cat.getPrompt(idx) ?? unreachable;
    expect_eq(strEq(entry.nameSlice(), "code_review"), true);
    expect_eq(strEq(entry.descSlice(), "Review pull request diff"), true);
    expect_eq(entry.argCount(), 2 as usize);
    expect_eq(cat.getPrompt(99) == null, true);
});

test("mcpPromptCatalog: prompts/list cursor pagination & invalid cursor/limit guards", () => {
    let cat = McpPromptCatalog.init();
    const no_args: [0]McpPromptArgSpec = [];
    const names: [5][]const u8 = ["p0", "p1", "p2", "p3", "p4"];
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        expect_eq(cat.registerPrompt(names[i], "desc", no_args[0..]), McpPromptRegisterStatus.ok);
    }

    let page_buf: [4]usize = [0; 4];
    let page_s: []usize = page_buf[0..];

    // Page 1: cursor=0, limit=2 -> indices [0, 1], next_cursor=2, has_more=true
    const p1 = cat.listPrompts(0, 2, *page_s);
    expect_eq(p1.valid, true);
    expect_eq(p1.count, 2 as usize);
    expect_eq(p1.total, 5 as usize);
    expect_eq(p1.has_more, true);
    expect_eq(p1.next_cursor, 2 as usize);
    expect_eq(page_buf[0], 0 as usize);
    expect_eq(page_buf[1], 1 as usize);

    // Page 2: cursor=2, limit=2 -> indices [2, 3], next_cursor=4, has_more=true
    const p2 = cat.listPrompts(p1.next_cursor, 2, *page_s);
    expect_eq(p2.valid, true);
    expect_eq(p2.count, 2 as usize);
    expect_eq(p2.has_more, true);
    expect_eq(p2.next_cursor, 4 as usize);
    expect_eq(page_buf[0], 2 as usize);
    expect_eq(page_buf[1], 3 as usize);

    // Page 3: cursor=4, limit=2 -> index [4], has_more=false, next_cursor=0
    const p3 = cat.listPrompts(p2.next_cursor, 2, *page_s);
    expect_eq(p3.valid, true);
    expect_eq(p3.count, 1 as usize);
    expect_eq(p3.has_more, false);
    expect_eq(p3.next_cursor, 0 as usize);
    expect_eq(page_buf[0], 4 as usize);

    // Invalid limit=0 & out-of-range cursor=6
    expect_eq(cat.listPrompts(0, 0, *page_s).valid, false);
    expect_eq(cat.listPrompts(6, 2, *page_s).valid, false);

    // Boundary cursor == count returns valid empty page
    const p_end = cat.listPrompts(5, 2, *page_s);
    expect_eq(p_end.valid, true);
    expect_eq(p_end.count, 0 as usize);
    expect_eq(p_end.has_more, false);
});

test("mcpPromptCatalog: prompts/get valid invocations with required, optional & soft-choice args", () => {
    let cat = McpPromptCatalog.init();
    const modes: [2][]const u8 = ["strict", "relaxed"];
    const hints: [2][]const u8 = ["brief", "detailed"];
    const args: [3]McpPromptArgSpec = [
        McpPromptArgSpec.withChoices("mode", true, true, modes[0..]),
        McpPromptArgSpec.withChoices("style", false, false, hints[0..]), // completion hints only, not enforced
        McpPromptArgSpec.plain("note", false),
    ];
    expect_eq(cat.registerPrompt("audit", "Security audit prompt", args[0..]), McpPromptRegisterStatus.ok);

    // Required only
    const v1: [1]McpPromptArgValue = [McpPromptArgValue { name: "mode", value: "strict" }];
    const r1 = cat.validatePromptGet("audit", v1[0..]);
    expect_eq(r1.ok, true);
    expect_eq(r1.violation, McpPromptViolationKind.none);
    expect_eq(r1.violation_count, 0 as usize);

    // Non-enforced choice arg accepts custom value
    const v2: [2]McpPromptArgValue = [
        McpPromptArgValue { name: "mode", value: "relaxed" },
        McpPromptArgValue { name: "style", value: "custom-executive-format" },
    ];
    const r2 = cat.validatePromptGet("audit", v2[0..]);
    expect_eq(r2.ok, true);
});

test("mcpPromptCatalog: prompts/get violation detection (missing, unknown, duplicate, too long, enum)", () => {
    let cat = McpPromptCatalog.init();
    const envs: [2][]const u8 = ["staging", "prod"];
    let short_arg = McpPromptArgSpec.plain("tag", true);
    short_arg.max_value_len = 5;

    const args: [2]McpPromptArgSpec = [
        McpPromptArgSpec.withChoices("env", true, true, envs[0..]),
        short_arg,
    ];
    expect_eq(cat.registerPrompt("deploy", "Deploy checklist", args[0..]), McpPromptRegisterStatus.ok);

    // 1. Prompt not found
    const none_v: [0]McpPromptArgValue = [];
    const r_nf = cat.validatePromptGet("unknown_prompt", none_v[0..]);
    expect_eq(r_nf.ok, false);
    expect_eq(r_nf.violation, McpPromptViolationKind.prompt_not_found);

    // 2. Missing required arg
    const v_miss: [1]McpPromptArgValue = [McpPromptArgValue { name: "env", value: "prod" }];
    const r_miss = cat.validatePromptGet("deploy", v_miss[0..]);
    expect_eq(r_miss.ok, false);
    expect_eq(r_miss.violation, McpPromptViolationKind.missing_required_arg);
    expect_eq(strEq(r_miss.firstBadArg(), "tag"), true);

    // 3. Unknown arg
    const v_unk: [3]McpPromptArgValue = [
        McpPromptArgValue { name: "env", value: "prod" },
        McpPromptArgValue { name: "tag", value: "v1.0" },
        McpPromptArgValue { name: "extra", value: "123" },
    ];
    const r_unk = cat.validatePromptGet("deploy", v_unk[0..]);
    expect_eq(r_unk.ok, false);
    expect_eq(r_unk.violation, McpPromptViolationKind.unknown_arg);
    expect_eq(strEq(r_unk.firstBadArg(), "extra"), true);

    // 4. Duplicate arg
    const v_dup: [3]McpPromptArgValue = [
        McpPromptArgValue { name: "env", value: "prod" },
        McpPromptArgValue { name: "env", value: "staging" },
        McpPromptArgValue { name: "tag", value: "v1.0" },
    ];
    const r_dup = cat.validatePromptGet("deploy", v_dup[0..]);
    expect_eq(r_dup.ok, false);
    expect_eq(r_dup.violation, McpPromptViolationKind.duplicate_arg);
    expect_eq(strEq(r_dup.firstBadArg(), "env"), true);

    // 5. Value too long (> 5 bytes)
    const v_long: [2]McpPromptArgValue = [
        McpPromptArgValue { name: "env", value: "prod" },
        McpPromptArgValue { name: "tag", value: "v1.2.3-rc1" },
    ];
    const r_long = cat.validatePromptGet("deploy", v_long[0..]);
    expect_eq(r_long.ok, false);
    expect_eq(r_long.violation, McpPromptViolationKind.value_too_long);
    expect_eq(strEq(r_long.firstBadArg(), "tag"), true);

    // 6. Invalid enum value
    const v_enum: [2]McpPromptArgValue = [
        McpPromptArgValue { name: "env", value: "dev" },
        McpPromptArgValue { name: "tag", value: "v1.0" },
    ];
    const r_enum = cat.validatePromptGet("deploy", v_enum[0..]);
    expect_eq(r_enum.ok, false);
    expect_eq(r_enum.violation, McpPromptViolationKind.invalid_enum_value);
    expect_eq(strEq(r_enum.firstBadArg(), "env"), true);
});

test("mcpPromptCatalog: completion/complete case-insensitive ASCII prefix matching", () => {
    let cat = McpPromptCatalog.init();
    const langs: [5][]const u8 = ["Rust", "ruby", "python", "Perl", "cp"];
    const args: [1]McpPromptArgSpec = [
        McpPromptArgSpec.withChoices("lang", true, false, langs[0..]),
    ];
    expect_eq(cat.registerPrompt("explain", "Explain syntax", args[0..]), McpPromptRegisterStatus.ok);

    let out: [8]McpCompletionCandidate = [McpCompletionCandidate {}; 8];
    let out_s: []McpCompletionCandidate = out[0..];

    // Case-insensitive "RU" matches "Rust" and "ruby"
    const c1 = cat.completePromptArg("explain", "lang", "RU", *out_s);
    expect_eq(c1.found, true);
    expect_eq(c1.count, 2 as usize);
    expect_eq(c1.total_matches, 2 as usize);
    expect_eq(c1.has_more, false);
    expect_eq(strEq(out[0].valueSlice(), "Rust"), true);
    expect_eq(strEq(out[1].valueSlice(), "ruby"), true);

    // Empty prefix returns all 5 choices
    const c_all = cat.completePromptArg("explain", "lang", "", *out_s);
    expect_eq(c_all.found, true);
    expect_eq(c_all.count, 5 as usize);
    expect_eq(c_all.has_more, false);

    // Non-matching prefix returns found=true, count=0
    const c_none = cat.completePromptArg("explain", "lang", "zzz", *out_s);
    expect_eq(c_none.found, true);
    expect_eq(c_none.count, 0 as usize);

    // Unknown prompt or arg returns found=false
    expect_eq(cat.completePromptArg("missing", "lang", "", *out_s).found, false);
    expect_eq(cat.completePromptArg("explain", "missing_arg", "", *out_s).found, false);
});

test("mcpPromptCatalog: completion/complete output truncation & has_more flag", () => {
    let cat = McpPromptCatalog.init();
    const models: [5][]const u8 = ["gpt-4o", "gpt-4o-mini", "gpt-4.1", "gpt-5", "claude-sonnet"];
    const args: [1]McpPromptArgSpec = [
        McpPromptArgSpec.withChoices("model", true, true, models[0..]),
    ];
    expect_eq(cat.registerPrompt("bench", "Run benchmark", args[0..]), McpPromptRegisterStatus.ok);

    // Only 2 candidate slots in output slice, but 4 models start with "gpt"
    let small: [2]McpCompletionCandidate = [McpCompletionCandidate {}; 2];
    let small_s: []McpCompletionCandidate = small[0..];
    const res = cat.completePromptArg("bench", "model", "GPT", *small_s);
    expect_eq(res.found, true);
    expect_eq(res.count, 2 as usize);
    expect_eq(res.total_matches, 4 as usize);
    expect_eq(res.has_more, true);
    expect_eq(strEq(small[0].valueSlice(), "gpt-4o"), true);
    expect_eq(strEq(small[1].valueSlice(), "gpt-4o-mini"), true);
});

test("mcpPromptCatalog: registration boundary guards (empty, overlong, duplicate, >8 args, bad specs)", () => {
    let cat = McpPromptCatalog.init();
    const no_args: [0]McpPromptArgSpec = [];

    // Empty name
    expect_eq(cat.registerPrompt("", "desc", no_args[0..]), McpPromptRegisterStatus.empty_name);

    // Overlong name (> 64 bytes)
    const long_name: [65]u8 = [97; 65];
    expect_eq(cat.registerPrompt(long_name[0..], "desc", no_args[0..]), McpPromptRegisterStatus.name_too_long);

    // Duplicate name
    expect_eq(cat.registerPrompt("dup", "first", no_args[0..]), McpPromptRegisterStatus.ok);
    expect_eq(cat.registerPrompt("dup", "second", no_args[0..]), McpPromptRegisterStatus.duplicate_name);

    // Too many args (> 8)
    let nine_args: [9]McpPromptArgSpec = [McpPromptArgSpec.plain("a", false); 9];
    expect_eq(cat.registerPrompt("too_many", "desc", nine_args[0..]), McpPromptRegisterStatus.too_many_args);

    // Invalid arg spec: empty arg name
    const bad1: [1]McpPromptArgSpec = [McpPromptArgSpec.plain("", true)];
    expect_eq(cat.registerPrompt("bad1", "desc", bad1[0..]), McpPromptRegisterStatus.invalid_arg_spec);

    // Invalid arg spec: duplicate arg names in same prompt
    const bad2: [2]McpPromptArgSpec = [
        McpPromptArgSpec.plain("x", true),
        McpPromptArgSpec.plain("x", false),
    ];
    expect_eq(cat.registerPrompt("bad2", "desc", bad2[0..]), McpPromptRegisterStatus.invalid_arg_spec);

    // Invalid arg spec: enforce_enum=true with 0 choices
    const no_choices: [0][]const u8 = [];
    const bad3: [1]McpPromptArgSpec = [McpPromptArgSpec.withChoices("mode", true, true, no_choices[0..])];
    expect_eq(cat.registerPrompt("bad3", "desc", bad3[0..]), McpPromptRegisterStatus.invalid_arg_spec);
});

test("mcpPromptCatalog: full-catalog (32 entries) capacity_full guard & clear() reuse", () => {
    let cat = McpPromptCatalog.init();
    const no_args: [0]McpPromptArgSpec = [];

    let i: usize = 0;
    while (i < MCP_PROMPT_MAX_ENTRIES) : (i += 1) {
        let name_buf: [4]u8 = [112, 95, (65 + (i / 26)) as u8, (65 + (i % 26)) as u8];
        expect_eq(cat.registerPrompt(name_buf[0..], "slot", no_args[0..]), McpPromptRegisterStatus.ok);
    }
    expect_eq(cat.count(), MCP_PROMPT_MAX_ENTRIES);
    expect_eq(cat.registerPrompt("overflow_prompt", "full", no_args[0..]), McpPromptRegisterStatus.capacity_full);

    cat.clear();
    expect_eq(cat.count(), 0 as usize);
    expect_eq(cat.registerPrompt("overflow_prompt", "reused", no_args[0..]), McpPromptRegisterStatus.ok);
    expect_eq(cat.count(), 1 as usize);
});
