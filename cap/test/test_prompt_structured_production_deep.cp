// Origin: Pure-CP Stdlib Test Suite (lib/prompt/structured.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation AI Agent structured output extraction,
// think/reasoning tag splitting, lexical JSON top-level key pre-checking, and PII/secret redaction.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ThoughtSplit,
    ExtractedBlock,
    RedactStats,
    splitThoughtBlock,
    extractCodeFence,
    extractFirstJsonSlice,
    jsonHasTopLevelKeys,
    redactSensitiveText,
} from "cap:prompt";

fn testThinkAndReasoningSplit()! {
    // 1. Standard <think>...</think> with surrounding whitespace
    const r1 = splitThoughtBlock("  <think>\n  Step 1: verify input.\n  Step 2: compute.\n</think>\n\nFinal answer: 42  ");
    expect_eq(r1.has_thought, true);
    expect_eq(r1.unclosed, false);
    expect_eq(r1.thought, "Step 1: verify input.\n  Step 2: compute.");
    expect_eq(r1.answer, "Final answer: 42");

    // 2. Case-insensitive <Reasoning>...</REASONING> with nested same-kind tags
    const r2 = splitThoughtBlock("<Reasoning>outer <reasoning>inner</reasoning> tail</REASONING>\nReady.");
    expect_eq(r2.has_thought, true);
    expect_eq(r2.unclosed, false);
    expect_eq(r2.thought, "outer <reasoning>inner</reasoning> tail");
    expect_eq(r2.answer, "Ready.");

    // 3. Unclosed <think> tag (e.g. truncated stream)
    const r3 = splitThoughtBlock("<think>Still thinking about tool args...");
    expect_eq(r3.has_thought, true);
    expect_eq(r3.unclosed, true);
    expect_eq(r3.thought, "Still thinking about tool args...");
    expect_eq(r3.answer, "");

    // 4. Unclosed tag with leading prefix text
    const r4 = splitThoughtBlock("Prefix note\n<think>partial reasoning");
    expect_eq(r4.has_thought, true);
    expect_eq(r4.unclosed, true);
    expect_eq(r4.thought, "partial reasoning");
    expect_eq(r4.answer, "Prefix note");

    // 5. No think/reasoning tags at all
    const r5 = splitThoughtBlock("  Direct plain answer without tags.  ");
    expect_eq(r5.has_thought, false);
    expect_eq(r5.unclosed, false);
    expect_eq(r5.thought, "");
    expect_eq(r5.answer, "Direct plain answer without tags.");

    // 6. Empty input
    const r6 = splitThoughtBlock("");
    expect_eq(r6.has_thought, false);
    expect_eq(r6.unclosed, false);
    expect_eq(r6.thought, "");
    expect_eq(r6.answer, "");
}

fn testExtractCodeFenceMultiLang()! {
    const reply = "Intro text\n```python\nprint('hi')\n```\nMiddle text\n```JSON\n{\"tool\":\"search\",\"q\":\"cp\"}\n```\nDone.";

    // 1. Exact preferred_lang match (case-insensitive) skips earlier python block
    const json_blk_opt = extractCodeFence(reply, "json");
    if (blk := json_blk_opt) {
        expect_eq(blk.lang, "JSON");
        expect_eq(blk.content, "{\"tool\":\"search\",\"q\":\"cp\"}");
        expect_eq(blk.start < blk.end, true);
        expect_eq(reply[blk.start .. blk.start + 7], "```JSON");
    } else {
        expect_eq(true, false);
    }

    // 2. Empty preferred_lang returns the very first closed fence
    const first_opt = extractCodeFence(reply, "");
    if (blk := first_opt) {
        expect_eq(blk.lang, "python");
        expect_eq(blk.content, "print('hi')");
    } else {
        expect_eq(true, false);
    }

    // 3. Non-existent preferred language returns null
    const miss = extractCodeFence(reply, "rust");
    expect_eq(miss == null, true);

    // 4. Tilde fence ~~~ works cleanly
    const tilde_doc = "~~~sql\nSELECT 1;\n~~~";
    const sql_opt = extractCodeFence(tilde_doc, "sql");
    if (blk := sql_opt) {
        expect_eq(blk.lang, "sql");
        expect_eq(blk.content, "SELECT 1;");
    } else {
        expect_eq(true, false);
    }

    // 5. Unclosed fence returns null
    const unclosed = "```json\n{\"a\":1}\nno closing fence";
    expect_eq(extractCodeFence(unclosed, "json") == null, true);
    expect_eq(extractCodeFence("", "") == null, true);
}

fn testExtractFirstJsonSliceBalancedAndEscaped()! {
    // 1. Mixed natural language + string containing braces, brackets, and escaped quotes
    const msg1 = "Sure! Here is the call: {\"action\":\"echo\",\"text\":\"say \\\"hello {world}\\\" and [ok]\\\\\",\"nested\":{\"k\":[1,2]}} hope it helps.";
    const s1_opt = extractFirstJsonSlice(msg1);
    if (s1 := s1_opt) {
        expect_eq(s1, "{\"action\":\"echo\",\"text\":\"say \\\"hello {world}\\\" and [ok]\\\\\",\"nested\":{\"k\":[1,2]}}");
    } else {
        expect_eq(true, false);
    }

    // 2. Top-level JSON array extraction
    const msg2 = "Items: [{\"id\":1}, {\"id\":2, \"note\":\"] still string\"}] end.";
    const s2_opt = extractFirstJsonSlice(msg2);
    if (s2 := s2_opt) {
        expect_eq(s2, "[{\"id\":1}, {\"id\":2, \"note\":\"] still string\"}]");
    } else {
        expect_eq(true, false);
    }

    // 3. Skips stray unbalanced leading bracket in prose and finds real JSON later
    const msg3 = "Note [draft: see payload -> {\"status\":\"ok\",\"count\":3}";
    const s3_opt = extractFirstJsonSlice(msg3);
    if (s3 := s3_opt) {
        expect_eq(s3, "{\"status\":\"ok\",\"count\":3}");
    } else {
        expect_eq(true, false);
    }

    // 4. Unbalanced / unclosed / empty inputs return null
    expect_eq(extractFirstJsonSlice("") == null, true);
    expect_eq(extractFirstJsonSlice("No json here at all") == null, true);
    expect_eq(extractFirstJsonSlice("{\"unclosed\": [1, 2, 3}") == null, true);
    expect_eq(extractFirstJsonSlice("{\"unclosed_str\": \"abc}") == null, true);
}

fn testJsonHasTopLevelKeysIsolation()! {
    const payload = "{\"meta\":{\"action\":\"nested_only\",\"target\":\"inner\"},\"id\":101,\"items\":[{\"secret\":\"hidden\"}],\"tool\":\"search\"}";

    // 1. All present top-level keys -> true
    const req_ok: []const string = ["id", "tool", "meta"];
    expect_eq(jsonHasTopLevelKeys(payload, req_ok), true);

    // 2. Key exists ONLY inside nested object -> MUST return false
    const req_nested_obj: []const string = ["action"];
    expect_eq(jsonHasTopLevelKeys(payload, req_nested_obj), false);

    // 3. Key exists ONLY inside nested array element -> MUST return false
    const req_nested_arr: []const string = ["secret"];
    expect_eq(jsonHasTopLevelKeys(payload, req_nested_arr), false);

    // 4. One present + one missing -> false
    const req_partial: []const string = ["id", "missing_field"];
    expect_eq(jsonHasTopLevelKeys(payload, req_partial), false);

    // 5. Empty required_keys on valid object -> true; on array or malformed -> false
    const empty_req: []const string = [];
    expect_eq(jsonHasTopLevelKeys("{}", empty_req), true);
    expect_eq(jsonHasTopLevelKeys("[1, 2, 3]", empty_req), false);
    expect_eq(jsonHasTopLevelKeys("", empty_req), false);
    expect_eq(jsonHasTopLevelKeys("{\"a\":1} trailing", empty_req), false);
    expect_eq(jsonHasTopLevelKeys("{\"a\":1", req_ok), false);

    // 6. Escaped key name matching
    const escaped_json = "{\"a\\\"b\": 42, \"ok\": true}";
    const req_esc: []const string = ["a\"b", "ok"];
    expect_eq(jsonHasTopLevelKeys(escaped_json, req_esc), true);
}

fn testRedactSensitiveTextAndBufferGuards()! {
    // 1. Redacts email, sk-/ghp_/AKIA keys, and phone/long digits while preserving normal text
    const raw = "Contact alice.dev+1@example.org, key=sk-proj1234567890, gh=ghp_abcdEFGH123456, aws=AKIAIOSFODNN7EXAMPLE, tel=+1-415-555-0199, num=13812345678, safe=v2026 port:8080.";
    let buf_arr: [512]u8 = undefined;
    let out_slice: []u8 = buf_arr[0..];

    const stats_opt = redactSensitiveText(raw, out_slice);
    if (st := stats_opt) {
        expect_eq(st.redacted_emails, 1);
        expect_eq(st.redacted_keys, 3);
        expect_eq(st.redacted_phones, 2);
        const expected = "Contact [REDACTED], key=[REDACTED], gh=[REDACTED], aws=[REDACTED], tel=[REDACTED], num=[REDACTED], safe=v2026 port:8080.";
        expect_eq(out_slice[0..st.bytes_written], expected);
    } else {
        expect_eq(true, false);
    }

    // 2. Clean text with no PII remains byte-for-byte identical
    const clean = "Order #4096 ready at 2026-11-22 (status: ok).";
    let clean_arr: [128]u8 = undefined;
    let clean_slice: []u8 = clean_arr[0..];
    if (st := redactSensitiveText(clean, clean_slice)) {
        expect_eq(st.redacted_emails, 0);
        expect_eq(st.redacted_keys, 0);
        expect_eq(st.redacted_phones, 0);
        expect_eq(clean_slice[0..st.bytes_written], clean);
    } else {
        expect_eq(true, false);
    }

    // 3. Empty input succeeds even with zero-length buffer
    let zero_arr: [1]u8 = undefined;
    let zero_slice: []u8 = zero_arr[0..0];
    if (st := redactSensitiveText("", zero_slice)) {
        expect_eq(st.bytes_written, 0);
        expect_eq(st.redacted_emails, 0);
        expect_eq(st.redacted_keys, 0);
        expect_eq(st.redacted_phones, 0);
    } else {
        expect_eq(true, false);
    }

    // 4. Small buffer overflow guard returns null cleanly
    let tiny_arr: [8]u8 = undefined;
    let tiny_slice: []u8 = tiny_arr[0..];
    expect_eq(redactSensitiveText("user@example.com", tiny_slice) == null, true);
    expect_eq(redactSensitiveText("hello world 12345", tiny_slice) == null, true);
}

fn testEndToEndAgentPipelineAndCapNamespace()! {
    // Verify dynamic `cap.prompt.*` namespace mounting alongside direct `cap:prompt` imports
    const llm_out = "<think>Check user alice@corp.io with key sk-live9988776655</think>\nHere is the call:\n```json\n{\"tool\":\"notify\",\"args\":{\"user\":\"alice\"},\"priority\":1}\n```";

    const split = cap.prompt.splitThoughtBlock(llm_out);
    expect_eq(split.has_thought, true);
    expect_eq(split.unclosed, false);

    // Redact thought before logging
    let log_arr: [256]u8 = undefined;
    let log_slice: []u8 = log_arr[0..];
    if (st := cap.prompt.redactSensitiveText(split.thought, log_slice)) {
        expect_eq(st.redacted_emails, 1);
        expect_eq(st.redacted_keys, 1);
        expect_eq(log_slice[0..st.bytes_written], "Check user [REDACTED] with key [REDACTED]");
    } else {
        expect_eq(true, false);
    }

    // Extract JSON from answer fence and validate required top-level tool keys
    if (blk := cap.prompt.extractCodeFence(split.answer, "json")) {
        if (js := cap.prompt.extractFirstJsonSlice(blk.content)) {
            const required: []const string = ["tool", "args", "priority"];
            expect_eq(cap.prompt.jsonHasTopLevelKeys(js, required), true);
            const nested_only: []const string = ["user"];
            expect_eq(cap.prompt.jsonHasTopLevelKeys(js, nested_only), false);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn main() {
    test("1. structured: splitThoughtBlock handles complete, nested, unclosed, and plain responses", () => {
        testThinkAndReasoningSplit()?;
    });

    test("2. structured: extractCodeFence selects preferred language and guards unclosed fences", () => {
        testExtractCodeFenceMultiLang()?;
    });

    test("3. structured: extractFirstJsonSlice skips prose, escaped quotes, and nested string braces", () => {
        testExtractFirstJsonSliceBalancedAndEscaped()?;
    });

    test("4. structured: jsonHasTopLevelKeys isolates top-level keys from nested objects and arrays", () => {
        testJsonHasTopLevelKeysIsolation()?;
    });

    test("5. structured: redactSensitiveText redacts emails, API keys, phones, and guards small buffers", () => {
        testRedactSensitiveTextAndBufferGuards()?;
    });

    test("6. structured: end-to-end AI Agent pipeline via cap.prompt namespace", () => {
        testEndToEndAgentPipelineAndCapNamespace()?;
    });
}
