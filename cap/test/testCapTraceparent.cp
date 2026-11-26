import { test, expect_eq } from "cap:test";
import {
    parseTraceParent,
    makeChild,
    getTraceState,
    TRACEPARENT_LEN,
    FLAG_SAMPLED,
} from "../../packages/@cap/traceparent/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("parse and format official W3C traceparent vector", () => {
        const raw: []const u8 = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
        let tp = parseTraceParent(raw);
        expect_eq(tp.valid, true);
        expect_eq(tp.version, 0);
        expect_eq(tp.trace_flags, FLAG_SAMPLED);
        expect_eq(tp.isSampled(), true);
        expect_eq(tp.trace_id[0], 0x4b);
        expect_eq(tp.parent_id[7], 0xb7);

        let out: [64]u8 = undefined;
        let n = tp.format(out[0..64]);
        expect_eq(n, TRACEPARENT_LEN as i64);
        expect_eq(eqBytes(out[0..TRACEPARENT_LEN], raw), true);
    });

    test("makeChild preserves trace_id and flags with new span_id", () => {
        let parent = parseTraceParent("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
        const child_span: [8]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];
        let child = makeChild(parent, child_span);
        expect_eq(child.valid, true);
        let out: [64]u8 = undefined;
        let n = child.format(out[0..64]);
        expect_eq(n, 55);
        expect_eq(eqBytes(out[0..55], "00-4bf92f3577b34da6a3ce929d0e0e4736-1122334455667788-01"), true);
    });

    test("reject all-zero trace_id, all-zero parent_id, and version ff", () => {
        expect_eq(parseTraceParent("00-00000000000000000000000000000000-00f067aa0ba902b7-01").valid, false);
        expect_eq(parseTraceParent("00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01").valid, false);
        expect_eq(parseTraceParent("ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01").valid, false);
        const zero_span: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
        let p = parseTraceParent("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
        expect_eq(makeChild(p, zero_span).valid, false);
    });

    test("reject uppercase hex and wrong length for version 00", () => {
        expect_eq(parseTraceParent("00-4BF92F3577B34DA6A3CE929D0E0E4736-00f067aa0ba902b7-01").valid, false);
        expect_eq(parseTraceParent("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-extra").valid, false);
        expect_eq(parseTraceParent("01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-00-future").valid, true);
    });

    test("getTraceState extracts vendor entries with whitespace trimming", () => {
        const ts: []const u8 = "rojo=00f067aa0ba902b7, congo=t61rcWkgMzE , vendor@tenant=abc=123";
        expect_eq(eqBytes(getTraceState(ts, "rojo"), "00f067aa0ba902b7"), true);
        expect_eq(eqBytes(getTraceState(ts, "congo"), "t61rcWkgMzE"), true);
        expect_eq(eqBytes(getTraceState(ts, "vendor@tenant"), "abc=123"), true);
        expect_eq(getTraceState(ts, "missing").len, 0);
    });

    test("undersized buffer and invalid TraceParent format return -1", () => {
        let p = parseTraceParent("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-00");
        expect_eq(p.isSampled(), false);
        let tiny: [32]u8 = undefined;
        expect_eq(p.format(tiny[0..32]), -1);
        let bad = parseTraceParent("invalid");
        let out: [64]u8 = undefined;
        expect_eq(bad.format(out[0..64]), -1);
    });
}
