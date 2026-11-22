// Production Deep Test Suite for cap:prompt Guardrail Scanner (lib/prompt/guardrail_scanner.cp)
import { test, expect_eq, expect_true } from "cap:test";
import {
    GUARDRAIL_MAX_CUSTOM_TERMS,
    GuardrailCategory,
    GuardrailAction,
    GuardrailFinding,
    GuardrailPolicy,
    luhnCheckDigits,
    redactSensitiveSpans,
    PromptGuardrailScanner,
} from "cap:prompt";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Clean allow, email & RFC 1918 / loopback private IPv4 detection
// ============================================================================
test("guardrail_scanner: clean allow, email & private IPv4 detection", fn() {
    let scanner = PromptGuardrailScanner.init(GuardrailPolicy.defaults()) ?? return;
    let findings: [16]GuardrailFinding = [GuardrailFinding{}; 16];
    let f_s: []GuardrailFinding = findings[0..];

    // Clean text with public IP (8.8.8.8) -> allow
    const clean = "Querying public resolver 8.8.8.8 for status check.";
    const r_clean = scanner.scan(clean, *f_s);
    expect_eq(r_clean.action, GuardrailAction.allow);
    expect_eq(r_clean.finding_count, 0 as usize);
    expect_eq(r_clean.risk_score, 0 as u32);

    // Email + private IPv4 (10.x and 192.168.x) -> redact (20 + 25 + 25 = 70 < 80 block)
    const msg = "Notify alice.ops@example.org on 10.20.30.40 or 192.168.1.5";
    const rep = scanner.scan(msg, *f_s);
    expect_eq(rep.action, GuardrailAction.redact);
    expect_eq(rep.email_hits, 1 as usize);
    expect_eq(rep.private_ip_hits, 2 as usize);
    expect_eq(rep.risk_score, 70 as u32);
});

// ============================================================================
// Test 2: API credential prefixes & 3-segment JWT token detection
// ============================================================================
test("guardrail_scanner: API credential prefixes & 3-segment JWT token detection", fn() {
    let scanner = PromptGuardrailScanner.init(GuardrailPolicy.defaults()) ?? return;
    let findings: [16]GuardrailFinding = [GuardrailFinding{}; 16];
    let f_s: []GuardrailFinding = findings[0..];

    // Synthetic dummy placeholders matching prefix patterns
    const text = "key=sk-test0000000000000000 and token=eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxMjM0NTYifQ.sig0123456789";
    const rep = scanner.scan(text, *f_s);
    expect_eq(rep.action, GuardrailAction.block);
    expect_eq(rep.secret_hits, 1 as usize);
    expect_eq(rep.jwt_hits, 1 as usize);

    // Also test AKIA and Bearer prefixes
    const text2 = "aws=AKIAIOSFODNN7EXAMPLE header=Bearer dummyTokenValue12345";
    const rep2 = scanner.scan(text2, *f_s);
    expect_eq(rep2.secret_hits, 2 as usize);
    expect_eq(rep2.action, GuardrailAction.block);
});

// ============================================================================
// Test 3: Luhn payment card verification & non-Luhn rejection
// ============================================================================
test("guardrail_scanner: Luhn payment card verification & non-Luhn rejection", fn() {
    // Classic synthetic Luhn test numbers (4111...1111 passes, ...1112 fails)
    expect_true(luhnCheckDigits("4111111111111111"));
    expect_true(!luhnCheckDigits("4111111111111112"));
    expect_true(!luhnCheckDigits("12345")); // too short

    let scanner = PromptGuardrailScanner.init(GuardrailPolicy.defaults()) ?? return;
    let findings: [16]GuardrailFinding = [GuardrailFinding{}; 16];
    let f_s: []GuardrailFinding = findings[0..];

    // Formatted with dashes passes Luhn -> flagged as payment_card (block)
    const with_card = "Test card: 4111-1111-1111-1111 ok";
    const rep1 = scanner.scan(with_card, *f_s);
    expect_eq(rep1.card_hits, 1 as usize);
    expect_eq(rep1.action, GuardrailAction.block);

    // Non-Luhn 16-digit order number is NOT falsely flagged
    const non_card = "Order ref: 4111-1111-1111-1112 ok";
    const rep2 = scanner.scan(non_card, *f_s);
    expect_eq(rep2.card_hits, 0 as usize);
    expect_eq(rep2.action, GuardrailAction.allow);
});

// ============================================================================
// Test 4: Custom terms, control/zero-width ratio anomaly & max-length guard
// ============================================================================
test("guardrail_scanner: custom terms, control/zero-width anomaly & max-length truncation", fn() {
    let pol = GuardrailPolicy.defaults();
    pol.max_input_bytes = 40;
    pol.max_control_ratio_bps = 500; // 5%
    let scanner = PromptGuardrailScanner.init(pol) ?? return;
    expect_true(scanner.addCustomTerm("ProjectApollo"));

    let findings: [16]GuardrailFinding = [GuardrailFinding{}; 16];
    let f_s: []GuardrailFinding = findings[0..];

    // Case-insensitive custom term match
    const t_custom = "Briefing on projectapollo milestone";
    const r_custom = scanner.scan(t_custom, *f_s);
    expect_eq(r_custom.custom_term_hits, 1 as usize);
    expect_eq(r_custom.action, GuardrailAction.redact);

    // Zero-width U+200B (0xE2 0x80 0x8B) anomaly exceeding 5% ratio
    const zw_buf: [12]u8 = [0x61, 0xE2, 0x80, 0x8B, 0x62, 0xE2, 0x80, 0x8B, 0x63, 0x64, 0x65, 0x66];
    const r_zw = scanner.scan(zw_buf[0..], *f_s);
    expect_eq(r_zw.control_anomaly_count, 2 as usize);
    expect_true(r_zw.control_ratio_bps > 500);
    expect_eq(r_zw.action, GuardrailAction.redact);

    // Oversized input (> 40 bytes) triggers truncated_input & length_exceeded (block)
    const long_text = "This input exceeds forty bytes and therefore triggers deterministic truncation.";
    const r_long = scanner.scan(long_text, *f_s);
    expect_true(r_long.truncated_input);
    expect_eq(r_long.scanned_bytes, 40 as usize);
    expect_eq(r_long.action, GuardrailAction.block);
});

// ============================================================================
// Test 5: redactSensitiveSpans typed placeholders & overlapping span merging
// ============================================================================
test("guardrail_scanner: redactSensitiveSpans typed placeholders & overlapping span merging", fn() {
    let scanner = PromptGuardrailScanner.init(GuardrailPolicy.defaults()) ?? return;
    const text = "Mail bob@example.com via 172.16.0.10";
    let findings: [8]GuardrailFinding = [GuardrailFinding{}; 8];
    let f_s: []GuardrailFinding = findings[0..];
    const rep = scanner.scan(text, *f_s);
    expect_eq(rep.finding_count, 2 as usize);

    let out: [128]u8 = [0; 128];
    let out_s: []u8 = out[0..];
    if (n := redactSensitiveSpans(text, findings[0..rep.finding_count], *out_s)) {
        expect_true(bytesEq(out[0..n], "Mail [REDACTED:EMAIL] via [REDACTED:PRIVATE_IP]"));
    } else {
        expect_true(false);
    }

    // Overlapping spans: [5, 15) weight=20 (email) and [10, 20) weight=90 (api_secret) merge into [5, 20) SECRET
    const overlap_in = "0123456789abcdefghijXYZ";
    const ov_spans: [2]GuardrailFinding = [
        GuardrailFinding{ category: GuardrailCategory.email, start: 5, end: 15, weight: 20 },
        GuardrailFinding{ category: GuardrailCategory.api_secret, start: 10, end: 20, weight: 90 },
    ];
    if (n2 := redactSensitiveSpans(overlap_in, ov_spans[0..], *out_s)) {
        expect_true(bytesEq(out[0..n2], "01234[REDACTED:SECRET]XYZ"));
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 6: Custom term table capacity guard (16 terms)
// ============================================================================
test("guardrail_scanner: custom term table capacity guard (16 terms)", fn() {
    let scanner = PromptGuardrailScanner.init(GuardrailPolicy.defaults()) ?? return;
    expect_true(!scanner.addCustomTerm("")); // empty rejected

    let i: usize = 0;
    while (i < GUARDRAIL_MAX_CUSTOM_TERMS) : (i += 1) {
        let tb: [4]u8 = [0; 4];
        tb[0] = 116;
        tb[1] = (48 + (i / 10)) as u8;
        tb[2] = (48 + (i % 10)) as u8;
        tb[3] = 120;
        expect_true(scanner.addCustomTerm(tb[0..]));
    }
    expect_true(!scanner.addCustomTerm("overflow"));
});

// ============================================================================
// Test 7: Strict boundary guards (empty input, invalid policy, undersized out)
// ============================================================================
test("guardrail_scanner: strict boundary guards (empty, invalid policy, undersized buffer)", fn() {
    let bad_pol1 = GuardrailPolicy.defaults();
    bad_pol1.max_control_ratio_bps = 10001;
    expect_true(PromptGuardrailScanner.init(bad_pol1) == null);

    let bad_pol2 = GuardrailPolicy.defaults();
    bad_pol2.redact_score_threshold = 90;
    bad_pol2.block_score_threshold = 50;
    expect_true(PromptGuardrailScanner.init(bad_pol2) == null);

    let scanner = PromptGuardrailScanner.init(GuardrailPolicy.defaults()) ?? return;
    let findings: [4]GuardrailFinding = [GuardrailFinding{}; 4];
    let f_s: []GuardrailFinding = findings[0..];

    const empty_rep = scanner.scan("", *f_s);
    expect_eq(empty_rep.action, GuardrailAction.allow);
    expect_eq(empty_rep.finding_count, 0 as usize);

    // Undersized redaction buffer returns null cleanly
    const msg = "user@example.com";
    const rep = scanner.scan(msg, *f_s);
    let tiny: [4]u8 = [0; 4];
    let tiny_s: []u8 = tiny[0..];
    expect_true(redactSensitiveSpans(msg, findings[0..rep.finding_count], *tiny_s) == null);
});
