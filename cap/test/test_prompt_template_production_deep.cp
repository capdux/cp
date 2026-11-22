// Origin: Pure-CP Stdlib Test Suite (Prompt Template Renderer & Token-Budgeted Context Packer) | Created: 2026-11-22
// Deep Production Verification Suite for lib/prompt/template.cp
// Covers:
//   1. {{key}}, {{key|fallback}} default values, whitespace trimming & \{\{ escape sequences
//   2. Strict mode MissingVariable vs lenient mode placeholder preservation
//   3. extractTemplateKeys deduplicated key extraction, escape skipping & capacity/unclosed guards
//   4. packContextWindow System/pinned locking, newest-first greedy packing, chronological order & truncated_pinned
//   5. Unclosed {{, BufferTooSmall, invalid budget, empty messages & u32 token-sum overflow defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TemplateVar,
    TemplateRenderStatus,
    TemplateRenderResult,
    PackRole,
    PackableMessage,
    PackSummary,
    renderPromptTemplate,
    extractTemplateKeys,
    packContextWindow,
} from "../../lib/prompt/index.cp";

fn testTemplateRenderFallbackAndEscape()! {
    const tmpl = "Agent={{ role }}, Lang={{ lang | CP }}, Literal=\\{\\{raw}}";
    const vars: [1]TemplateVar = [
        TemplateVar{ key: "role", value: "Architect" },
    ];

    let out_buf: [128]u8 = [0; 128];
    let out: []u8 = out_buf[0..];
    const res = renderPromptTemplate(tmpl, vars[0..], true, out);
    expect_eq(res.status == TemplateRenderStatus.Ok, true);
    expect_eq(res.substitutions, 2);
    const rendered = out_buf[0..res.bytes_written];
    expect_eq(std.mem.eql(u8, rendered, "Agent=Architect, Lang= CP , Literal={{raw}}"), true);

    // When `lang` IS provided, it overrides the fallback
    const vars2: [2]TemplateVar = [
        TemplateVar{ key: "role", value: "Architect" },
        TemplateVar{ key: "lang", value: "Zig" },
    ];
    const res2 = renderPromptTemplate(tmpl, vars2[0..], true, out);
    expect_eq(res2.status == TemplateRenderStatus.Ok, true);
    expect_eq(res2.substitutions, 2);
    const rendered2 = out_buf[0..res2.bytes_written];
    expect_eq(std.mem.eql(u8, rendered2, "Agent=Architect, Lang=Zig, Literal={{raw}}"), true);
}

fn testStrictVsLenientMissingVariable()! {
    const tmpl = "Hello {{user}}, key={{secret_key}}!";
    const vars: [1]TemplateVar = [
        TemplateVar{ key: "user", value: "Alice" },
    ];

    let out_buf: [128]u8 = [0; 128];
    let out: []u8 = out_buf[0..];

    // Strict mode rejects missing variable without fallback
    const strict_res = renderPromptTemplate(tmpl, vars[0..], true, out);
    expect_eq(strict_res.status == TemplateRenderStatus.MissingVariable, true);
    expect_eq(strict_res.substitutions, 1);

    // Lenient mode preserves unresolved placeholder verbatim
    const lenient_res = renderPromptTemplate(tmpl, vars[0..], false, out);
    expect_eq(lenient_res.status == TemplateRenderStatus.Ok, true);
    expect_eq(lenient_res.substitutions, 1);
    const rendered = out_buf[0..lenient_res.bytes_written];
    expect_eq(std.mem.eql(u8, rendered, "Hello Alice, key={{secret_key}}!"), true);
}

fn testExtractTemplateKeysDedupAndGuards()! {
    const tmpl = "Hi {{user}}, task={{ task | review }}, again={{ user | guest }}, esc=\\{\\{ignored}}, done={{task}}";
    let keys_buf: [4]string = [""; 4];
    let keys: []string = keys_buf[0..];

    const n_opt = extractTemplateKeys(tmpl, keys);
    expect_eq(n_opt != null, true);
    const n = n_opt ?? 0;
    expect_eq(n, 2);
    expect_eq(std.mem.eql(u8, keys_buf[0], "user"), true);
    expect_eq(std.mem.eql(u8, keys_buf[1], "task"), true);

    // Insufficient key buffer capacity -> null
    let small_keys_buf: [1]string = [""];
    let small_keys: []string = small_keys_buf[0..];
    expect_eq(extractTemplateKeys(tmpl, small_keys) == null, true);

    // Unclosed placeholder -> null
    expect_eq(extractTemplateKeys("Broken {{unclosed", keys) == null, true);
}

fn testPackContextWindowBudgetAndOrder()! {
    // 7 messages in chronological order:
    //   0: System (100 tok)          -> locked
    //   1: User pinned few-shot (50) -> locked
    //   2: User old turn 1 (120)     -> dropped
    //   3: Assistant old turn 1 (120)-> dropped
    //   4: User turn 2 (60)          -> kept
    //   5: Tool turn 2 (40)          -> kept
    //   6: Assistant latest (70)     -> kept
    // Budget: max=400, reserve=80 => available=320.
    // Locked: 100 + 50 = 150. Remaining: 170.
    // Newest-first: 70 + 40 + 60 = 170 -> exact fit! Chronological indices: [0, 1, 4, 5, 6].
    const msgs: [7]PackableMessage = [
        PackableMessage{ id: 10, role: PackRole.System, token_count: 100, pinned: false },
        PackableMessage{ id: 11, role: PackRole.User, token_count: 50, pinned: true },
        PackableMessage{ id: 12, role: PackRole.User, token_count: 120, pinned: false },
        PackableMessage{ id: 13, role: PackRole.Assistant, token_count: 120, pinned: false },
        PackableMessage{ id: 14, role: PackRole.User, token_count: 60, pinned: false },
        PackableMessage{ id: 15, role: PackRole.Tool, token_count: 40, pinned: false },
        PackableMessage{ id: 16, role: PackRole.Assistant, token_count: 70, pinned: false },
    ];

    let out_idx_buf: [7]usize = [99; 7];
    let out_idx: []usize = out_idx_buf[0..];
    const sum_opt = packContextWindow(msgs[0..], 400, 80, out_idx);
    expect_eq(sum_opt != null, true);
    const summary = sum_opt ?? PackSummary{};

    expect_eq(summary.selected_count, 5);
    expect_eq(summary.dropped_count, 2);
    expect_eq(summary.used_tokens, 320);
    expect_eq(summary.truncated_pinned, false);

    // Chronological order preserved
    expect_eq(out_idx_buf[0], 0);
    expect_eq(out_idx_buf[1], 1);
    expect_eq(out_idx_buf[2], 4);
    expect_eq(out_idx_buf[3], 5);
    expect_eq(out_idx_buf[4], 6);

    // Tight budget where second pinned message cannot fit -> truncated_pinned == true
    const tight_opt = packContextWindow(msgs[0..], 140, 20, out_idx); // budget = 120
    expect_eq(tight_opt != null, true);
    const tight = tight_opt ?? PackSummary{};
    expect_eq(tight.selected_count, 1);
    expect_eq(tight.used_tokens, 100);
    expect_eq(tight.truncated_pinned, true);
    expect_eq(out_idx_buf[0], 0);
}

fn testBoundaryOverflowAndErrorDefenses()! {
    const vars: [1]TemplateVar = [
        TemplateVar{ key: "k", value: "0123456789" },
    ];
    let tiny_buf: [5]u8 = [0; 5];
    let tiny: []u8 = tiny_buf[0..];

    // 1. Unclosed placeholder ("Hi " is 3 bytes < 5, so scanner reaches `{{k`)
    const r_unclosed = renderPromptTemplate("Hi {{k", vars[0..], false, tiny);
    expect_eq(r_unclosed.status == TemplateRenderStatus.UnclosedPlaceholder, true);

    // 2. BufferTooSmall on literal and on substitution
    const r_lit_small = renderPromptTemplate("abcdef", vars[0..], true, tiny);
    expect_eq(r_lit_small.status == TemplateRenderStatus.BufferTooSmall, true);
    const r_sub_small = renderPromptTemplate("{{k}}", vars[0..], true, tiny);
    expect_eq(r_sub_small.status == TemplateRenderStatus.BufferTooSmall, true);

    // 3. packContextWindow: invalid budget (reserve > max), empty messages, small out_indices
    const big_msgs: [2]PackableMessage = [
        PackableMessage{ id: 1, role: PackRole.User, token_count: 3000000000, pinned: false },
        PackableMessage{ id: 2, role: PackRole.User, token_count: 3000000000, pinned: false },
    ];
    const empty_msgs: []const PackableMessage = big_msgs[0..0];
    let idx2_buf: [2]usize = [0; 2];
    let idx2: []usize = idx2_buf[0..];
    let idx0: []usize = idx2_buf[0..0];

    expect_eq(packContextWindow(big_msgs[0..], 100, 101, idx2) == null, true);
    const empty_sum = packContextWindow(empty_msgs, 100, 10, idx2) ?? PackSummary{ selected_count: 99 };
    expect_eq(empty_sum.selected_count, 0);
    expect_eq(empty_sum.dropped_count, 0);
    expect_eq(empty_sum.used_tokens, 0);

    // u32 overflow guard: 3e9 + 3e9 would wrap u32, but u64 check ensures only 1 message (latest) fits under 4e9!
    const ovf_sum = packContextWindow(big_msgs[0..], 4000000000, 0, idx2) ?? PackSummary{};
    expect_eq(ovf_sum.selected_count, 1);
    expect_eq(ovf_sum.dropped_count, 1);
    expect_eq(ovf_sum.used_tokens, 3000000000);
    expect_eq(idx2_buf[0], 1);

    // Insufficient out_indices buffer -> null
    expect_eq(packContextWindow(big_msgs[0..], 4000000000, 0, idx0) == null, true);
}

fn main() {
    test("1. prompt template: {{key}}, {{key|fallback}} & escape rendering", () => {
        testTemplateRenderFallbackAndEscape()?;
    });
    test("2. prompt template: strict MissingVariable vs lenient preservation", () => {
        testStrictVsLenientMissingVariable()?;
    });
    test("3. prompt template: extractTemplateKeys dedup & guards", () => {
        testExtractTemplateKeysDedupAndGuards()?;
    });
    test("4. prompt template: packContextWindow System/pinned lock & chronological order", () => {
        testPackContextWindowBudgetAndOrder()?;
    });
    test("5. prompt template: unclosed/BufferTooSmall/budget/u32-overflow defenses", () => {
        testBoundaryOverflowAndErrorDefenses()?;
    });
}
