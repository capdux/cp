// Origin: Pure-CP Stdlib Test Suite (lib/time/deadlines.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Hierarchical Request Deadlines & gRPC Timeout Propagation:
//   1. `grpc-timeout` wire codec: `parseGrpcTimeoutHeader` (n/u/m/S/M/H) & `formatGrpcTimeoutHeader` optimal unit selection
//   2. `grpc-timeout` malformed/negative/>8-digit/overflow & tiny-buffer boundary guards
//   3. `RequestDeadline`: monotonic `deriveChild` clamping, `canFitStage` & `reserveForRetry` margin slicing
//   4. `DeadlineTree`: multi-level hierarchy, monotonic child truncation & cascade cancellation (`parent_cancelled`)
//   5. `DeadlineTree`: expired node sweeping (`sweepExpired`) & 64-slot full/reclaim lifecycle
//   6. Global telemetry stats (`DeadlineTreeStats`) & `cap.time` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    DEADLINE_MAX_NODES,
    DEADLINE_NO_NODE,
    GRPC_TIMEOUT_MAX_DIGITS,
    DeadlineStatus,
    DeadlineUnit,
    RequestDeadline,
    DeadlineTree,
    parseGrpcTimeoutHeader,
    formatGrpcTimeoutHeader,
    formatDeadlineStatus,
    formatDeadlineUnit,
} from "cap:time";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("Deadlines: grpc-timeout wire parse & compact format across all 6 units") {
    expect_eq(parseGrpcTimeoutHeader("0n") ?? -1, @as(i64, 0));
    expect_eq(parseGrpcTimeoutHeader("500n") ?? -1, @as(i64, 1)); // sub-microsecond positive rounds up to 1us
    expect_eq(parseGrpcTimeoutHeader("2500n") ?? -1, @as(i64, 2));
    expect_eq(parseGrpcTimeoutHeader("750u") ?? -1, @as(i64, 750));
    expect_eq(parseGrpcTimeoutHeader("250m") ?? -1, @as(i64, 250_000));
    expect_eq(parseGrpcTimeoutHeader("15S") ?? -1, @as(i64, 15_000_000));
    expect_eq(parseGrpcTimeoutHeader("2M") ?? -1, @as(i64, 120_000_000));
    expect_eq(parseGrpcTimeoutHeader("1H") ?? -1, @as(i64, 3_600_000_000));

    let buf: [16]u8 = [0; 16];
    let buf_s: []u8 = buf[0..];

    const n0 = formatGrpcTimeoutHeader(-50, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n0], "0u"));

    const n1 = formatGrpcTimeoutHeader(750, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n1], "750u"));

    const n2 = formatGrpcTimeoutHeader(250_000, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n2], "250m"));

    const n3 = formatGrpcTimeoutHeader(15_000_000, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n3], "15S"));

    const n4 = formatGrpcTimeoutHeader(120_000_000, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n4], "2M"));

    const n5 = formatGrpcTimeoutHeader(3_600_000_000, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n5], "1H"));

    // Non-divisible >99_999_999 us falls back cleanly to `m` within 8-digit cap
    const n6 = formatGrpcTimeoutHeader(123_456_789, *buf_s) ?? 0;
    expect_true(bytesEq(buf[0..n6], "123456m"));

    expect_eq(GRPC_TIMEOUT_MAX_DIGITS, @as(usize, 8));
    expect_true(bytesEq(formatDeadlineStatus(DeadlineStatus.parent_cancelled), "parent_cancelled"));
    expect_true(bytesEq(formatDeadlineUnit(DeadlineUnit.sec), "S"));
}

test("Deadlines: grpc-timeout malformed, negative, >8 digits & tiny buffer guards") {
    expect_true(parseGrpcTimeoutHeader("") == null);
    expect_true(parseGrpcTimeoutHeader("S") == null);
    expect_true(parseGrpcTimeoutHeader("-10m") == null);
    expect_true(parseGrpcTimeoutHeader("10s") == null);        // lowercase 's' invalid in gRPC
    expect_true(parseGrpcTimeoutHeader("10 m") == null);       // whitespace invalid
    expect_true(parseGrpcTimeoutHeader("100000000m") == null); // 9 digits exceeds gRPC 8-digit cap
    expect_true(parseGrpcTimeoutHeader("12x") == null);        // unknown unit

    let tiny: [1]u8 = [0; 1];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatGrpcTimeoutHeader(500, *tiny_s) == null);
}

test("Deadlines: RequestDeadline monotonic deriveChild, canFitStage & reserveForRetry") {
    const parent = RequestDeadline.fromTimeoutUs(1_000_000, 500_000); // deadline = 1_500_000
    expect_eq(parent.remainingUs(1_200_000), @as(i64, 300_000));
    expect_true(!parent.isExpired(1_499_999));
    expect_true(parent.isExpired(1_500_000));
    expect_eq(parent.remainingUs(1_600_000), @as(i64, 0));

    // Local timeout (1_000_000) exceeds parent remaining -> clamped to parent deadline 1_500_000
    const c_clamped = parent.deriveChild(1_000_000, 1_100_000);
    expect_eq(c_clamped.deadline_us, @as(i64, 1_500_000));

    // Tighter local timeout (100_000) takes effect -> deadline = 1_200_000
    const c_tight = parent.deriveChild(100_000, 1_100_000);
    expect_eq(c_tight.deadline_us, @as(i64, 1_200_000));

    // Stage admission check
    expect_true(parent.canFitStage(1_200_000, 300_000));
    expect_true(!parent.canFitStage(1_200_000, 300_001));

    // Retry reserve slicing: remaining 300_000, reserve 100_000 -> attempt deadline = 1_400_000
    if (attempt := parent.reserveForRetry(1_200_000, 100_000)) {
        expect_eq(attempt.deadline_us, @as(i64, 1_400_000));
        expect_eq(attempt.remainingUs(1_200_000), @as(i64, 200_000));
    } else {
        expect_true(false);
    }
    // Exhausted reserve (remaining 300_000 <= reserve 300_000) -> returns null
    expect_true(parent.reserveForRetry(1_200_000, 300_000) == null);
}

test("Deadlines: DeadlineTree hierarchy, monotonic truncation & cascade cancellation") {
    let tree = DeadlineTree.init();
    const root = tree.createRoot(1000, 500, "http.in") ?? DEADLINE_NO_NODE;
    expect_true(root != DEADLINE_NO_NODE);

    // Child requests 1000us at t=1100, clamped to parent's 1500
    const c1 = tree.createChild(root, 1100, 1000, "rpc.auth") ?? DEADLINE_NO_NODE;
    const c2 = tree.createChild(root, 1100, 200, "rpc.db") ?? DEADLINE_NO_NODE;
    const gc1 = tree.createChild(c1, 1150, 900, "rpc.kms") ?? DEADLINE_NO_NODE;
    expect_true(c1 != DEADLINE_NO_NODE && c2 != DEADLINE_NO_NODE && gc1 != DEADLINE_NO_NODE);

    expect_eq(tree.remainingUs(c1, 1200) ?? -1, @as(i64, 300));
    expect_eq(tree.remainingUs(c2, 1200) ?? -1, @as(i64, 100));
    expect_eq(tree.remainingUs(gc1, 1200) ?? -1, @as(i64, 300));
    expect_eq(tree.statusAt(gc1, 1200) ?? DeadlineStatus.expired, DeadlineStatus.active);

    // Cancel root -> root becomes `cancelled`, c1/c2/gc1 become `parent_cancelled`
    const cancelled_cnt = tree.cancel(root);
    expect_eq(cancelled_cnt, @as(usize, 4));
    expect_eq(tree.statusAt(root, 1200) ?? DeadlineStatus.active, DeadlineStatus.cancelled);
    expect_eq(tree.statusAt(c1, 1200) ?? DeadlineStatus.active, DeadlineStatus.parent_cancelled);
    expect_eq(tree.statusAt(c2, 1200) ?? DeadlineStatus.active, DeadlineStatus.parent_cancelled);
    expect_eq(tree.statusAt(gc1, 1200) ?? DeadlineStatus.active, DeadlineStatus.parent_cancelled);
    expect_eq(tree.remainingUs(gc1, 1200) ?? -1, @as(i64, 0));

    // Cannot derive new child under a cancelled parent
    expect_true(tree.createChild(c1, 1200, 50, "late") == null);
}

test("Deadlines: sweepExpired reclamation & 64-slot capacity recycling") {
    let tree = DeadlineTree.init();

    // Fill all 64 slots: first 32 expire at t=1000, next 32 expire at t=5000
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        expect_true(tree.createRoot(0, 1000, "short") != null);
    }
    while (i < DEADLINE_MAX_NODES) : (i += 1) {
        expect_true(tree.createRoot(0, 5000, "long") != null);
    }

    // 65th node rejected when full
    expect_true(tree.createRoot(0, 5000, "overflow") == null);

    // Sweep at t=2000 reclaims exactly 32 expired short nodes
    const swept = tree.sweepExpired(2000);
    expect_eq(swept, @as(usize, 32));
    expect_eq(tree.stats().active_slots, @as(usize, 32));
    expect_eq(tree.stats().expired_swept_total, @as(u64, 32));

    // Reclaimed slots can immediately host new roots
    const reused = tree.createRoot(2000, 3000, "reused") ?? DEADLINE_NO_NODE;
    expect_true(reused != DEADLINE_NO_NODE);
    if (node := tree.getNode(reused)) {
        expect_true(bytesEq(node.tag(), "reused"));
    } else {
        expect_true(false);
    }
}

test("Deadlines: global telemetry stats & cap.time facade parity") {
    let tree = cap.time.DeadlineTree.init();
    const r = tree.createRoot(100, 1000, "root") ?? DEADLINE_NO_NODE;
    const c = tree.createChild(r, 200, 500, "child") ?? DEADLINE_NO_NODE;
    expect_true(r != DEADLINE_NO_NODE && c != DEADLINE_NO_NODE);

    * = tree.cancel(r);
    const st = tree.stats();
    expect_eq(st.capacity, DEADLINE_MAX_NODES);
    expect_eq(st.active_slots, @as(usize, 2));
    expect_eq(st.running_count, @as(usize, 0));
    expect_eq(st.cancelled_count, @as(usize, 2));
    expect_eq(st.created_total, @as(u64, 2));
    expect_eq(st.cascade_cancelled_total, @as(u64, 1));

    expect_true(tree.release(c));
    expect_eq(tree.stats().active_slots, @as(usize, 1));
}
