// Origin: Pure-CP Stdlib Test Suite (lib/mcp/elicitation.cp) | Created: 2026-11-23
// Production Deep Verification Suite for MCP 2025-06-18 Human-in-the-Loop Elicitation Gateway:
//   1. Built-in email / URI / ISO-8601 date (including leap-year) format validators
//   2. Full flat primitive schema accept flow (string/enum/integer/number/boolean) & replay guard
//   3. Exact field-level violation reporting (missing/unknown/dup/type/length/format/bounds/enum) + retry
//   4. Decline / cancel transitions, reapExpired() timeout sweeping & purgeTerminal() slot reuse
//   5. Zero-alloc JSON-RPC 2.0 `elicitation/create` serializer & response summary formatter
//   6. Malformed schema / capacity_full guards & cap.mcp namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_ELICIT_MAX_SESSIONS,
    MCP_ELICIT_MAX_FIELDS,
    MCP_ELICIT_MAX_ENUM_VALS,
    elicitValidateEmail,
    elicitValidateUri,
    elicitValidateDate,
    ElicitFieldType,
    ElicitStringFormat,
    ElicitAction,
    ElicitSessionState,
    ElicitStatus,
    ElicitFieldSpec,
    ElicitFieldValue,
    ElicitResponseOutcome,
    ElicitHubStats,
    ElicitationHub,
    encodeElicitationRequest,
    formatElicitationResponseSummary,
} from "cap:mcp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("Elicitation: email, URI & ISO-8601 date (with leap-year) format validators") {
    expect_true(elicitValidateEmail("alice@cap.dev"));
    expect_true(elicitValidateEmail("ops+bot@sub.example.org"));
    expect_eq(elicitValidateEmail("no-at-sign.com"), false);
    expect_eq(elicitValidateEmail("@missing-local.com"), false);
    expect_eq(elicitValidateEmail("a@nodot"), false);
    expect_eq(elicitValidateEmail("a@bad..dot.com"), false);
    expect_eq(elicitValidateEmail("bad space@cap.dev"), false);

    expect_true(elicitValidateUri("https://api.cap.dev/v1/deploy"));
    expect_true(elicitValidateUri("mailto:ops@cap.dev"));
    expect_eq(elicitValidateUri("123://bad-scheme"), false);
    expect_eq(elicitValidateUri("https://has space"), false);
    expect_eq(elicitValidateUri("no-colon"), false);

    expect_true(elicitValidateDate("2024-02-29")); // leap year
    expect_eq(elicitValidateDate("2025-02-29"), false); // non-leap year
    expect_true(elicitValidateDate("2026-11-23"));
    expect_eq(elicitValidateDate("2026-13-01"), false);
    expect_eq(elicitValidateDate("2026-04-31"), false);
    expect_eq(elicitValidateDate("26-11-23"), false);
}

test("Elicitation: full flat schema accept lifecycle & replay protection") {
    let hub = ElicitationHub.init(30000);
    const envs: [2][]const u8 = ["staging", "prod"];
    const schema: [5]ElicitFieldSpec = [
        ElicitFieldSpec.enumField("env", true, envs[0..]),
        ElicitFieldSpec.stringField("approver", true, 5, 64, ElicitStringFormat.email),
        ElicitFieldSpec.integerField("replicas", true, true, 1, true, 16),
        ElicitFieldSpec.numberField("budget_usd", false, true, 0.0, true, 500.0),
        ElicitFieldSpec.booleanField("dry_run", true),
    ];

    expect_eq(hub.createRequest(101, "Confirm production rollout", schema[0..], 0, 1000), ElicitStatus.ok);
    expect_eq(hub.getSessionState(101) orelse ElicitSessionState.cancelled, ElicitSessionState.pending);

    const vals: [5]ElicitFieldValue = [
        ElicitFieldValue{ name: "env", raw_value: "prod" },
        ElicitFieldValue{ name: "approver", raw_value: "lead@cap.dev" },
        ElicitFieldValue{ name: "replicas", raw_value: "4" },
        ElicitFieldValue{ name: "budget_usd", raw_value: "125.5" },
        ElicitFieldValue{ name: "dry_run", raw_value: "false" },
    ];
    const out = hub.respondAccept(101, vals[0..], 2000);
    expect_eq(out.status, ElicitStatus.ok);
    expect_eq(out.state, ElicitSessionState.accepted);
    expect_eq(out.has_failed_field, false);

    // Replay attempt on already accepted session -> already_resolved
    const replay = hub.respondAccept(101, vals[0..], 2100);
    expect_eq(replay.status, ElicitStatus.already_resolved);
    expect_eq(replay.state, ElicitSessionState.accepted);

    const st = hub.stats();
    expect_eq(st.active_pending, 0 as usize);
    expect_eq(st.accepted_count, 1 as u64);
    expect_eq(st.replay_rejections, 1 as u64);
}

test("Elicitation: exact field-level violation reporting & pending retry recovery") {
    let hub = ElicitationHub.init(60000);
    const roles: [2][]const u8 = ["viewer", "editor"];
    const schema: [4]ElicitFieldSpec = [
        ElicitFieldSpec.stringField("email", true, 5, 32, ElicitStringFormat.email), // idx 0
        ElicitFieldSpec.enumField("role", true, roles[0..]),                         // idx 1
        ElicitFieldSpec.integerField("limit", true, true, 1, true, 100),             // idx 2
        ElicitFieldSpec.booleanField("notify", false),                               // idx 3
    ];
    expect_eq(hub.createRequest(202, "Grant access", schema[0..], 0, 1000), ElicitStatus.ok);

    // 1. Missing required field (idx 2: limit)
    const miss: [2]ElicitFieldValue = [
        ElicitFieldValue{ name: "email", raw_value: "u@cap.dev" },
        ElicitFieldValue{ name: "role", raw_value: "viewer" },
    ];
    const r_miss = hub.respondAccept(202, miss[0..], 1100);
    expect_eq(r_miss.status, ElicitStatus.missing_required_field);
    expect_true(r_miss.has_failed_field);
    expect_eq(r_miss.failed_field_index, 2 as usize);

    // 2. Format invalid on email (idx 0)
    const bad_fmt: [3]ElicitFieldValue = [
        ElicitFieldValue{ name: "email", raw_value: "not-an-email" },
        ElicitFieldValue{ name: "role", raw_value: "viewer" },
        ElicitFieldValue{ name: "limit", raw_value: "10" },
    ];
    const r_fmt = hub.respondAccept(202, bad_fmt[0..], 1200);
    expect_eq(r_fmt.status, ElicitStatus.format_invalid);
    expect_eq(r_fmt.failed_field_index, 0 as usize);

    // 3. Enum mismatch on role (idx 1)
    const bad_enum: [3]ElicitFieldValue = [
        ElicitFieldValue{ name: "email", raw_value: "u@cap.dev" },
        ElicitFieldValue{ name: "role", raw_value: "root_admin" },
        ElicitFieldValue{ name: "limit", raw_value: "10" },
    ];
    const r_enum = hub.respondAccept(202, bad_enum[0..], 1300);
    expect_eq(r_enum.status, ElicitStatus.enum_mismatch);
    expect_eq(r_enum.failed_field_index, 1 as usize);

    // 4. Integer out of bounds (idx 2) and type mismatch (float for integer)
    const bad_num: [3]ElicitFieldValue = [
        ElicitFieldValue{ name: "email", raw_value: "u@cap.dev" },
        ElicitFieldValue{ name: "role", raw_value: "editor" },
        ElicitFieldValue{ name: "limit", raw_value: "999" },
    ];
    const r_oob = hub.respondAccept(202, bad_num[0..], 1400);
    expect_eq(r_oob.status, ElicitStatus.number_out_of_bounds);
    expect_eq(r_oob.failed_field_index, 2 as usize);

    const bad_type: [3]ElicitFieldValue = [
        ElicitFieldValue{ name: "email", raw_value: "u@cap.dev" },
        ElicitFieldValue{ name: "role", raw_value: "editor" },
        ElicitFieldValue{ name: "limit", raw_value: "12.5" },
    ];
    expect_eq(hub.respondAccept(202, bad_type[0..], 1500).status, ElicitStatus.type_mismatch);

    // 5. Unknown field & duplicate field
    const unk: [1]ElicitFieldValue = [ElicitFieldValue{ name: "extra", raw_value: "1" }];
    expect_eq(hub.respondAccept(202, unk[0..], 1600).status, ElicitStatus.unknown_field);

    const dup: [2]ElicitFieldValue = [
        ElicitFieldValue{ name: "email", raw_value: "u@cap.dev" },
        ElicitFieldValue{ name: "email", raw_value: "u2@cap.dev" },
    ];
    expect_eq(hub.respondAccept(202, dup[0..], 1700).status, ElicitStatus.duplicate_field);

    // Session remains pending after validation errors -> valid retry succeeds!
    const good: [3]ElicitFieldValue = [
        ElicitFieldValue{ name: "email", raw_value: "u@cap.dev" },
        ElicitFieldValue{ name: "role", raw_value: "editor" },
        ElicitFieldValue{ name: "limit", raw_value: "50" },
    ];
    const r_ok = hub.respondAccept(202, good[0..], 1800);
    expect_eq(r_ok.status, ElicitStatus.ok);
    expect_eq(r_ok.state, ElicitSessionState.accepted);
    expect_eq(hub.stats().validation_failures, 7 as u64);
}

test("Elicitation: decline, cancel, reapExpired timeout & purgeTerminal reuse") {
    let hub = ElicitationHub.init(5000);
    const schema: [1]ElicitFieldSpec = [ElicitFieldSpec.booleanField("confirm", true)];

    expect_eq(hub.createRequest(1, "Action 1", schema[0..], 5000, 1000), ElicitStatus.ok);
    expect_eq(hub.createRequest(2, "Action 2", schema[0..], 5000, 1000), ElicitStatus.ok);
    expect_eq(hub.createRequest(3, "Action 3", schema[0..], 2000, 1000), ElicitStatus.ok); // expires at 3000
    expect_eq(hub.createRequest(4, "Action 4", schema[0..], 2000, 1000), ElicitStatus.ok); // expires at 3000

    expect_eq(hub.respondDecline(1, 1500).state, ElicitSessionState.declined);
    expect_eq(hub.respondCancel(2, 1600).state, ElicitSessionState.cancelled);

    // Responding to expired session #3 directly -> session_expired
    const vals: [1]ElicitFieldValue = [ElicitFieldValue{ name: "confirm", raw_value: "true" }];
    const r_exp = hub.respondAccept(3, vals[0..], 3000);
    expect_eq(r_exp.status, ElicitStatus.session_expired);
    expect_eq(r_exp.state, ElicitSessionState.timed_out);

    // Reap remaining expired session #4
    expect_eq(hub.reapExpired(3500), 1 as usize);
    expect_eq(hub.getSessionState(4) orelse ElicitSessionState.pending, ElicitSessionState.timed_out);

    const st = hub.stats();
    expect_eq(st.active_pending, 0 as usize);
    expect_eq(st.declined_count, 1 as u64);
    expect_eq(st.cancelled_count, 1 as u64);
    expect_eq(st.timed_out_count, 2 as u64);

    // Purge all 4 terminal slots for reuse
    expect_eq(hub.purgeTerminal(), 4 as usize);
    expect_true(hub.getSessionState(1) == null);
    expect_eq(hub.createRequest(1, "Reused ID", schema[0..], 5000, 4000), ElicitStatus.ok);
}

test("Elicitation: JSON-RPC 2.0 request serializer & response summary formatter") {
    const modes: [2][]const u8 = ["fast", "safe"];
    const schema: [2]ElicitFieldSpec = [
        ElicitFieldSpec.enumField("mode", true, modes[0..]),
        ElicitFieldSpec.stringField("contact", true, 3, 64, ElicitStringFormat.email),
    ];

    let buf: [512]u8 = [0; 512];
    let buf_s: []u8 = buf[0..];
    const n = encodeElicitationRequest(77, "Select \"safe\" mode?", schema[0..], *buf_s) orelse {
        expect_true(false);
        return;
    };
    const json = buf[0..n];
    expect_true(containsSubstr(json, "\"jsonrpc\":\"2.0\",\"id\":77"));
    expect_true(containsSubstr(json, "\"method\":\"elicitation/create\""));
    expect_true(containsSubstr(json, "\\\"safe\\\""));
    expect_true(containsSubstr(json, "\"enum\":[\"fast\",\"safe\"]"));
    expect_true(containsSubstr(json, "\"format\":\"email\""));
    expect_true(containsSubstr(json, "\"required\":[\"mode\",\"contact\"]"));

    // Small buffer returns null
    let tiny: [32]u8 = [0; 32];
    let tiny_s: []u8 = tiny[0..];
    expect_true(encodeElicitationRequest(77, "hi", schema[0..], *tiny_s) == null);

    // Response summary formatter
    let sum_buf: [64]u8 = [0; 64];
    let sum_s: []u8 = sum_buf[0..];
    const sn1 = formatElicitationResponseSummary(ElicitAction.accept, 2, *sum_s) orelse { expect_true(false); return; };
    expect_true(bytesEq(sum_buf[0..sn1], "{\"action\":\"accept\",\"fieldCount\":2}"));
    const sn2 = formatElicitationResponseSummary(ElicitAction.decline, 0, *sum_s) orelse { expect_true(false); return; };
    expect_true(bytesEq(sum_buf[0..sn2], "{\"action\":\"decline\"}"));
    const sn3 = formatElicitationResponseSummary(ElicitAction.cancel, 0, *sum_s) orelse { expect_true(false); return; };
    expect_true(bytesEq(sum_buf[0..sn3], "{\"action\":\"cancel\"}"));
}

test("Elicitation: invalid schema, capacity_full guards & cap.mcp facade parity") {
    let hub = ElicitationHub.init(10000);
    const empty_fields: [0]ElicitFieldSpec = [];
    const ok_field: [1]ElicitFieldSpec = [ElicitFieldSpec.booleanField("ok", true)];

    expect_eq(hub.createRequest(0, "msg", ok_field[0..], 0, 100), ElicitStatus.invalid_schema);
    expect_eq(hub.createRequest(1, "", ok_field[0..], 0, 100), ElicitStatus.invalid_schema);
    expect_eq(hub.createRequest(1, "msg", empty_fields[0..], 0, 100), ElicitStatus.invalid_schema);

    // Duplicate field names in schema rejected
    const dup_schema: [2]ElicitFieldSpec = [
        ElicitFieldSpec.booleanField("same", true),
        ElicitFieldSpec.booleanField("same", false),
    ];
    expect_eq(hub.createRequest(1, "msg", dup_schema[0..], 0, 100), ElicitStatus.invalid_schema);

    // min_num > max_num rejected
    const bad_range: [1]ElicitFieldSpec = [ElicitFieldSpec.integerField("x", true, true, 10, true, 5)];
    expect_eq(hub.createRequest(1, "msg", bad_range[0..], 0, 100), ElicitStatus.invalid_schema);

    // Fill all 32 slots -> 33rd returns capacity_full
    let i: usize = 0;
    while (i < MCP_ELICIT_MAX_SESSIONS) : (i += 1) {
        expect_eq(hub.createRequest((i + 1) as u64, "approve", ok_field[0..], 0, 100), ElicitStatus.ok);
    }
    expect_eq(hub.createRequest(1, "dup", ok_field[0..], 0, 100), ElicitStatus.duplicate_request_id);
    expect_eq(hub.createRequest(999, "overflow", ok_field[0..], 0, 100), ElicitStatus.capacity_full);

    // Verify cap.mcp namespace facade parity
    expect_eq(cap.mcp.MCP_ELICIT_MAX_SESSIONS, MCP_ELICIT_MAX_SESSIONS);
    expect_eq(cap.mcp.MCP_ELICIT_MAX_FIELDS, MCP_ELICIT_MAX_FIELDS);
    expect_true(cap.mcp.elicitValidateDate("2026-11-23"));
}
