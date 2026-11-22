// Source: cp/cap/test/test_audit_production_deep.cp
// Deep Production Verification Suite for cap:audit & cap.audit
// Covers:
//   1. Append events across AuditOutcome/AuditSeverity, GENESIS_HASH & prev_hash chaining
//   2. Tamper detection: payload modification, prev_hash corruption, and middle-row deletion
//   3. queryByActor, queryByAction, queryRange filtering & RFC-4180 CSV export
//   4. exportJsonl() -> importJsonl() roundtrip verification & tampered JSONL detection
//   5. `import cap from "cap"; cap.audit` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GENESIS_HASH,
    AuditError,
    AuditOutcome,
    AuditSeverity,
    AuditEntry,
    ChainVerifyResult,
    AuditTrail,
} from "cap:finance";

test("cap:audit - append events, GENESIS_HASH, prev_hash chaining & verifyChain") {
    const alloc = std.heap.page_allocator;
    let trail = AuditTrail.init(alloc);
    defer trail.deinit();

    const empty_check = trail.verifyChain();
    expect_eq(empty_check.valid, true);
    expect_eq(empty_check.checked_count, 0);
    expect_eq(trail.isEmpty(), true);

    const e1 = trail.append(
        1700000001000,
        "agent:planner",
        "tool.read",
        "fs:/etc/config.json",
        AuditOutcome.Success,
        AuditSeverity.Info,
        "{\"bytes\":256}",
    ) catch unreachable;

    const e2 = trail.append(
        1700000002000,
        "user:bob",
        "rbac.grant",
        "role:superadmin",
        AuditOutcome.Denied,
        AuditSeverity.Critical,
        "{\"reason\":\"missing \\\"mfa\\\" token\"}",
    ) catch unreachable;

    const e3 = trail.append(
        1700000003000,
        "agent:executor",
        "shell.run",
        "cmd:deploy",
        AuditOutcome.Error,
        AuditSeverity.Warn,
        "{\"exit_code\":137}",
    ) catch unreachable;

    const e4 = trail.append(
        1700000004000,
        "agent:planner",
        "tool.finish",
        "session:s-99",
        AuditOutcome.Success,
        AuditSeverity.Info,
        "{}",
    ) catch unreachable;

    expect_eq(trail.len(), 4);
    expect_eq(e1.seq, 1);
    expect_eq(e1.prev_hash, GENESIS_HASH);
    expect_eq(e1.entry_hash.len, 64);

    expect_eq(e2.seq, 2);
    expect_eq(e2.prev_hash, e1.entry_hash);
    expect_eq(e3.seq, 3);
    expect_eq(e3.prev_hash, e2.entry_hash);
    expect_eq(e4.seq, 4);
    expect_eq(e4.prev_hash, e3.entry_hash);
    expect_eq(trail.lastHash(), e4.entry_hash);

    const res = trail.verifyChain();
    expect_eq(res.valid, true);
    expect_eq(res.checked_count, 4);
    expect_eq(res.first_corrupt_seq, 0);
}

test("cap:audit - tamper detection (payload, prev_hash, and middle row deletion)") {
    const alloc = std.heap.page_allocator;
    let trail = AuditTrail.init(alloc);
    defer trail.deinit();

    * = trail.append(100, "alice", "login", "auth", AuditOutcome.Success, AuditSeverity.Info, "{}") catch unreachable;
    * = trail.append(200, "bob", "read", "secret.db", AuditOutcome.Success, AuditSeverity.Warn, "{}") catch unreachable;
    * = trail.append(300, "carol", "delete", "audit.log", AuditOutcome.Denied, AuditSeverity.Critical, "{}") catch unreachable;
    * = trail.append(400, "dave", "logout", "auth", AuditOutcome.Success, AuditSeverity.Info, "{}") catch unreachable;

    // 1. Tamper payload (actor/resource) on seq 2
    const orig_actor = trail.entries.items[1].actor;
    trail.entries.items[1].actor = "mallory";
    const r1 = trail.verifyChain();
    expect_eq(r1.valid, false);
    expect_eq(r1.first_corrupt_seq, 2);
    expect_eq(r1.checked_count, 1);
    trail.entries.items[1].actor = orig_actor;

    const orig_res = trail.entries.items[1].resource;
    trail.entries.items[1].resource = "public.db";
    const r1b = trail.verifyChain();
    expect_eq(r1b.valid, false);
    expect_eq(r1b.first_corrupt_seq, 2);
    trail.entries.items[1].resource = orig_res;
    expect_eq(trail.verifyChain().valid, true);

    // 2. Corrupt prev_hash on seq 3
    const orig_prev = trail.entries.items[2].prev_hash;
    trail.entries.items[2].prev_hash = GENESIS_HASH;
    const r2 = trail.verifyChain();
    expect_eq(r2.valid, false);
    expect_eq(r2.first_corrupt_seq, 3);
    expect_eq(r2.checked_count, 2);
    trail.entries.items[2].prev_hash = orig_prev;
    expect_eq(trail.verifyChain().valid, true);

    // 3. Delete middle row (seq 2) -> chain breaks at seq 3
    * = trail.entries.orderedRemove(1);
    const r3 = trail.verifyChain();
    expect_eq(r3.valid, false);
    expect_eq(r3.first_corrupt_seq, 3);
    expect_eq(r3.checked_count, 1);
}

test("cap:audit - queryByActor, queryByAction, queryRange & exportCsv") {
    const alloc = std.heap.page_allocator;
    let trail = AuditTrail.init(alloc);
    defer trail.deinit();

    * = trail.append(1000, "agent:coder", "tool.exec", "fs:/a", AuditOutcome.Success, AuditSeverity.Info, "{}") catch unreachable;
    * = trail.append(2000, "user:alice", "rbac.check", "role:dev", AuditOutcome.Success, AuditSeverity.Info, "{}") catch unreachable;
    * = trail.append(3000, "agent:coder", "fs.write", "fs:/b,c", AuditOutcome.Success, AuditSeverity.Warn, "{\"k\":\"v\"}") catch unreachable;
    * = trail.append(4000, "agent:coder", "tool.exec", "cmd:test", AuditOutcome.Error, AuditSeverity.Critical, "{}") catch unreachable;
    * = trail.append(5000, "user:alice", "fs.write", "fs:/d", AuditOutcome.Denied, AuditSeverity.Critical, "{}") catch unreachable;

    const by_actor = trail.queryByActor(alloc, "agent:coder") catch unreachable;
    defer alloc.free(by_actor);
    expect_eq(by_actor.len, 3);
    expect_eq(by_actor[0].seq, 1);
    expect_eq(by_actor[1].seq, 3);
    expect_eq(by_actor[2].seq, 4);

    const by_action = trail.queryByAction(alloc, "tool.exec") catch unreachable;
    defer alloc.free(by_action);
    expect_eq(by_action.len, 2);
    expect_eq(by_action[0].seq, 1);
    expect_eq(by_action[1].seq, 4);

    const in_range = trail.queryRange(alloc, 2000, 4000) catch unreachable;
    defer alloc.free(in_range);
    expect_eq(in_range.len, 3);
    expect_eq(in_range[0].seq, 2);
    expect_eq(in_range[2].seq, 4);

    const empty_range = trail.queryRange(alloc, 5000, 1000) catch unreachable;
    defer alloc.free(empty_range);
    expect_eq(empty_range.len, 0);

    const csv = trail.exportCsv() catch unreachable;
    defer alloc.free(csv);
    expect_eq(std.mem.startsWith(u8, csv, "seq,timestamp_ms,actor,action,resource,outcome,severity,metadata_json,prev_hash,entry_hash\n"), true);
    expect_eq(std.mem.indexOf(u8, csv, "\"fs:/b,c\"") != null, true);
}

test("cap:audit - exportJsonl / importJsonl roundtrip & tampered JSONL detection") {
    const alloc = std.heap.page_allocator;
    let trail = AuditTrail.init(alloc);
    defer trail.deinit();

    * = trail.append(101, "svc:auth", "token.issue", "user:1", AuditOutcome.Success, AuditSeverity.Info, "{\"scope\":\"read\"}") catch unreachable;
    * = trail.append(202, "svc:auth", "rbac.grant", "role:ops", AuditOutcome.Denied, AuditSeverity.Critical, "{\"note\":\"line1\\nline2\"}") catch unreachable;
    * = trail.append(303, "svc:bill", "invoice.pay", "inv:42", AuditOutcome.Success, AuditSeverity.Warn, "{}") catch unreachable;

    const jsonl = trail.exportJsonl() catch unreachable;
    defer alloc.free(jsonl);

    let restored = AuditTrail.importJsonl(alloc, jsonl) catch unreachable;
    defer restored.deinit();

    expect_eq(restored.len(), 3);
    const v = restored.verifyChain();
    expect_eq(v.valid, true);
    expect_eq(v.checked_count, 3);
    expect_eq(restored.entries.items[2].entry_hash, trail.entries.items[2].entry_hash);
    expect_eq(restored.entries.items[1].outcome, AuditOutcome.Denied);
    expect_eq(restored.entries.items[1].severity, AuditSeverity.Critical);

    // Mutate exported JSONL payload on seq 2 and verify import + verifyChain catches it
    let tampered_jsonl = alloc.dupe(u8, jsonl) catch unreachable;
    defer alloc.free(tampered_jsonl);
    if (pos := std.mem.indexOf(u8, tampered_jsonl, "role:ops")) {
        tampered_jsonl[pos + 5] = 'x';
        tampered_jsonl[pos + 6] = 'y';
        tampered_jsonl[pos + 7] = 'z';
    } else {
        unreachable;
    }

    let tampered_trail = AuditTrail.importJsonl(alloc, tampered_jsonl) catch unreachable;
    defer tampered_trail.deinit();
    const tv = tampered_trail.verifyChain();
    expect_eq(tv.valid, false);
    expect_eq(tv.first_corrupt_seq, 2);

    // EmptyLog & InvalidJsonl error checks
    let got_empty = false;
    * = AuditTrail.importJsonl(alloc, "   \n\n") catch e => {
        if (e == AuditError.EmptyLog) got_empty = true;
    };
    expect_eq(got_empty, true);

    let got_invalid = false;
    * = AuditTrail.importJsonl(alloc, "{\"seq\":1}\n") catch e => {
        if (e == AuditError.InvalidJsonl) got_invalid = true;
    };
    expect_eq(got_invalid, true);
}

test("cap:audit - dynamic cap.audit mounting") {
    const alloc = std.heap.page_allocator;
    let trail = cap.finance.AuditTrail.init(alloc);
    defer trail.deinit();

    const e1 = trail.append(
        9001,
        "agent:orchestrator",
        "spawn.worker",
        "worker:audit",
        cap.finance.AuditOutcome.Success,
        cap.finance.AuditSeverity.Info,
        "{}",
    ) catch unreachable;

    const e2 = trail.append(
        9002,
        "agent:orchestrator",
        "policy.check",
        "cap:finance",
        cap.finance.AuditOutcome.Success,
        cap.finance.AuditSeverity.Critical,
        "{}",
    ) catch unreachable;

    expect_eq(e1.prev_hash, cap.finance.GENESIS_HASH);
    expect_eq(e2.prev_hash, e1.entry_hash);

    const res = trail.verifyChain();
    expect_eq(res.valid, true);
    expect_eq(res.checked_count, 2);
    expect_eq(trail.lastHash(), e2.entry_hash);
}
