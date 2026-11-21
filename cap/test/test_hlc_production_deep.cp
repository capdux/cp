// Source: cp/cap/test/test_hlc_production_deep.cp (Production deep tests for cap:hlc)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HLC_MAX_PHYSICAL_MS,
    HLC_MAX_LOGICAL,
    HLC_DEFAULT_MAX_DRIFT_MS,
    HLC_STR_LEN,
    HlcTimestamp,
    HlcClock,
    formatHlc,
    parseHlc,
} from "cap:hlc";

fn slicesCmp(a: []const u8, b: []const u8): i8 {
    let n: usize = a.len;
    if (b.len < n) {
        n = b.len;
    }
    let i: usize = 0;
    while (i < n) {
        if (a[i] < b[i]) {
            return -1;
        }
        if (a[i] > b[i]) {
            return 1;
        }
        i += 1;
    }
    if (a.len < b.len) {
        return -1;
    }
    if (a.len > b.len) {
        return 1;
    }
    return 0;
}

test("1. Local now() strict monotonicity under forward jumps, same-ms bursts, and clock regressions", () => {
    expect_eq(HLC_MAX_PHYSICAL_MS, 0x0000FFFFFFFFFFFF as u64);
    expect_eq(HLC_MAX_LOGICAL, 0xFFFF as u16);
    expect_eq(HLC_DEFAULT_MAX_DRIFT_MS, 60000 as u64);
    expect_eq(HLC_STR_LEN, 26 as usize);

    let clk: HlcClock = HlcClock.init(42, HLC_DEFAULT_MAX_DRIFT_MS);

    // Initial forward step.
    if (t1 := clk.now(1700000000000)) {
        expect_eq(t1.physical_ms, 1700000000000 as u64);
        expect_eq(t1.logical, 0 as u16);
        expect_eq(t1.node_id, 42 as u32);

        // Same millisecond burst increments logical counter.
        if (t2 := clk.now(1700000000000)) {
            expect_eq(t2.physical_ms, 1700000000000 as u64);
            expect_eq(t2.logical, 1 as u16);
            expect_eq(t1.isBefore(t2), true);
            expect_eq(t2.isAfter(t1), true);

            // Physical clock regresses by 500ms (within 60s max_drift_ms): must stay monotonic.
            if (t3 := clk.now(1699999999500)) {
                expect_eq(t3.physical_ms, 1700000000000 as u64);
                expect_eq(t3.logical, 2 as u16);
                expect_eq(t2.isBefore(t3), true);

                // Physical clock catches up and advances past last.physical_ms -> resets logical to 0.
                if (t4 := clk.now(1700000000005)) {
                    expect_eq(t4.physical_ms, 1700000000005 as u64);
                    expect_eq(t4.logical, 0 as u16);
                    expect_eq(t3.isBefore(t4), true);
                } else {
                    expect_eq(true, false);
                }
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. receive() Kulkarni 4-branch causal ordering verification", () => {
    let clk: HlcClock = HlcClock.init(7, 5000);

    // Seed local clock at (1000, 3, node=7).
    clk.last = HlcTimestamp { physical_ms: 1000, logical: 3, node_id: 7 };

    // Branch 1: local.physical == remote.physical >= wall_ms -> max(local.logical, remote.logical) + 1.
    const rem1: HlcTimestamp = HlcTimestamp { physical_ms: 1000, logical: 9, node_id: 99 };
    if (r1 := clk.receive(950, rem1)) {
        expect_eq(r1.physical_ms, 1000 as u64);
        expect_eq(r1.logical, 10 as u16);
        expect_eq(r1.node_id, 7 as u32);
        expect_eq(r1.isAfter(rem1), true);
    } else {
        expect_eq(true, false);
    }

    // Branch 2: local.physical strictly leads remote.physical and >= wall_ms -> local.logical + 1.
    const rem2: HlcTimestamp = HlcTimestamp { physical_ms: 900, logical: 50, node_id: 99 };
    if (r2 := clk.receive(980, rem2)) {
        expect_eq(r2.physical_ms, 1000 as u64);
        expect_eq(r2.logical, 11 as u16);
        expect_eq(r2.node_id, 7 as u32);
        expect_eq(r2.isAfter(rem2), true);
    } else {
        expect_eq(true, false);
    }

    // Branch 3: remote.physical strictly leads local.physical and >= wall_ms -> remote.logical + 1.
    const rem3: HlcTimestamp = HlcTimestamp { physical_ms: 1200, logical: 4, node_id: 99 };
    if (r3 := clk.receive(1100, rem3)) {
        expect_eq(r3.physical_ms, 1200 as u64);
        expect_eq(r3.logical, 5 as u16);
        expect_eq(r3.node_id, 7 as u32);
        expect_eq(r3.isAfter(rem3), true);
    } else {
        expect_eq(true, false);
    }

    // Branch 4: local wall_ms strictly leads both local.last and remote -> logical resets to 0.
    const rem4: HlcTimestamp = HlcTimestamp { physical_ms: 1300, logical: 8, node_id: 99 };
    if (r4 := clk.receive(1500, rem4)) {
        expect_eq(r4.physical_ms, 1500 as u64);
        expect_eq(r4.logical, 0 as u16);
        expect_eq(r4.node_id, 7 as u32);
        expect_eq(r4.isAfter(rem4), true);
    } else {
        expect_eq(true, false);
    }
});

test("3. Drift bound protection (> max_drift_ms) and logical == 0xFFFF overflow rejection", () => {
    let clk: HlcClock = HlcClock.init(1, 1000);
    clk.last = HlcTimestamp { physical_ms: 5000, logical: 2, node_id: 1 };

    // 1) Local regression exceeding max_drift_ms (5000 - 3999 = 1001 > 1000) -> rejected, state unchanged.
    if (_ := clk.now(3999)) {
        expect_eq(true, false);
    }
    expect_eq(clk.last.physical_ms, 5000 as u64);
    expect_eq(clk.last.logical, 2 as u16);

    // Exact boundary (5000 - 4000 = 1000 <= 1000) -> accepted.
    if (ok_ts := clk.now(4000)) {
        expect_eq(ok_ts.physical_ms, 5000 as u64);
        expect_eq(ok_ts.logical, 3 as u16);
    } else {
        expect_eq(true, false);
    }

    // 2) Remote clock ahead of wall_ms by > max_drift_ms -> rejected, state unchanged.
    const far_remote: HlcTimestamp = HlcTimestamp { physical_ms: 6001, logical: 0, node_id: 2 };
    if (_ := clk.receive(5000, far_remote)) {
        expect_eq(true, false);
    }
    expect_eq(clk.last.physical_ms, 5000 as u64);
    expect_eq(clk.last.logical, 3 as u16);

    // 3) Logical counter overflow (0xFFFF) in both now() and receive() -> rejected cleanly.
    clk.last = HlcTimestamp { physical_ms: 5000, logical: HLC_MAX_LOGICAL, node_id: 1 };
    if (_ := clk.now(5000)) {
        expect_eq(true, false);
    }
    if (_ := clk.receive(5000, HlcTimestamp { physical_ms: 5000, logical: 10, node_id: 2 })) {
        expect_eq(true, false);
    }
    if (_ := clk.receive(5000, HlcTimestamp { physical_ms: 5500, logical: HLC_MAX_LOGICAL, node_id: 2 })) {
        expect_eq(true, false);
    }

    // 4) Physical timestamp exceeding 48-bit max -> rejected.
    if (_ := clk.now(HLC_MAX_PHYSICAL_MS + 1)) {
        expect_eq(true, false);
    }
});

test("4. packU64 / unpackU64 compact 64-bit round-trip and total order predicates", () => {
    const samples: [4]HlcTimestamp = [
        HlcTimestamp { physical_ms: 0, logical: 0, node_id: 0 },
        HlcTimestamp { physical_ms: 1700000000123, logical: 42, node_id: 0x01020304 },
        HlcTimestamp { physical_ms: 1700000000123, logical: 42, node_id: 0x01020305 },
        HlcTimestamp { physical_ms: HLC_MAX_PHYSICAL_MS, logical: HLC_MAX_LOGICAL, node_id: 0xFFFFFFFF },
    ];

    let i: usize = 0;
    while (i < 4) {
        const ts: HlcTimestamp = samples[i];
        const packed: u64 = ts.packU64();
        const unpacked: HlcTimestamp = HlcTimestamp.unpackU64(packed, ts.node_id);
        expect_eq(unpacked.equals(ts), true);
        expect_eq(unpacked.compare(ts), 0 as i8);
        i += 1;
    }

    // Node tie-breaker when physical_ms and logical are equal.
    expect_eq(samples[1].packU64(), samples[2].packU64());
    expect_eq(samples[1].compare(samples[2]), -1 as i8);
    expect_eq(samples[2].compare(samples[1]), 1 as i8);
    expect_eq(samples[1].isBefore(samples[2]), true);
    expect_eq(samples[2].isAfter(samples[1]), true);
    expect_eq(samples[1].equals(samples[2]), false);
});

test("5. 26-byte canonical formatHlc/parseHlc round-trip, lexicographical order, and malformed input rejection", () => {
    const ordered: [6]HlcTimestamp = [
        HlcTimestamp { physical_ms: 0, logical: 0, node_id: 0 },
        HlcTimestamp { physical_ms: 0, logical: 0, node_id: 1 },
        HlcTimestamp { physical_ms: 0, logical: 1, node_id: 0 },
        HlcTimestamp { physical_ms: 0x0000018BFA123456, logical: 0x000A, node_id: 0x000000FF },
        HlcTimestamp { physical_ms: 0x0000018BFA123456, logical: 0x000B, node_id: 0x00000001 },
        HlcTimestamp { physical_ms: HLC_MAX_PHYSICAL_MS, logical: HLC_MAX_LOGICAL, node_id: 0xFFFFFFFF },
    ];

    let bufs: [6][26]u8 = std.mem.zeroes([6][26]u8);

    let i: usize = 0;
    while (i < 6) {
        let slice: []u8 = bufs[i][0..];
        if (n := formatHlc(ordered[i], *slice)) {
            expect_eq(n, HLC_STR_LEN);
            if (parsed := parseHlc(bufs[i][0..n])) {
                expect_eq(parsed.equals(ordered[i]), true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
        i += 1;
    }

    // Verify lexicographical byte order matches HlcTimestamp.compare() across all pairs.
    let a: usize = 0;
    while (a < 6) {
        let b: usize = 0;
        while (b < 6) {
            const cmp_str: i8 = slicesCmp(bufs[a][0..], bufs[b][0..]);
            const cmp_ts: i8 = ordered[a].compare(ordered[b]);
            expect_eq(cmp_str, cmp_ts);
            b += 1;
        }
        a += 1;
    }

    // Verify lowercase hex is also accepted by parseHlc.
    if (lower := parseHlc("0000018bfa12-000a-deadbeef")) {
        expect_eq(lower.physical_ms, 0x0000018BFA12 as u64);
        expect_eq(lower.logical, 0x000A as u16);
        expect_eq(lower.node_id, 0xDEADBEEF as u32);
    } else {
        expect_eq(true, false);
    }

    // Malformed inputs must be rejected.
    if (_ := parseHlc("0000018BFA12-000A-DEADBEE")) {
        expect_eq(true, false);
    }
    if (_ := parseHlc("0000018BFA12_000A-DEADBEEF")) {
        expect_eq(true, false);
    }
    if (_ := parseHlc("0000018BFA12-000A_DEADBEEF")) {
        expect_eq(true, false);
    }
    if (_ := parseHlc("0000018BFA1G-000A-DEADBEEF")) {
        expect_eq(true, false);
    }

    // Short output buffer and out-of-range physical_ms must be rejected by formatHlc.
    let short_arr: [25]u8 = std.mem.zeroes([25]u8);
    let short_buf: []u8 = short_arr[0..];
    if (_ := formatHlc(ordered[0], *short_buf)) {
        expect_eq(true, false);
    }
    let ok_arr: [26]u8 = std.mem.zeroes([26]u8);
    let ok_buf: []u8 = ok_arr[0..];
    const invalid_ts: HlcTimestamp = HlcTimestamp {
        physical_ms: HLC_MAX_PHYSICAL_MS + 1,
        logical: 0,
        node_id: 0,
    };
    if (_ := formatHlc(invalid_ts, *ok_buf)) {
        expect_eq(true, false);
    }
});
