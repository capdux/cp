import std from "std";
import { test, expect_eq } from "cap:test";
import { W3cTraceParent as TraceParent, TraceStateIter, findTraceState } from "cap:obs";

test("1. TraceParent.parse and format canonical 55-byte round-trip", () => {
    const raw = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    const tp = TraceParent.parse(raw) ?? unreachable;

    expect_eq(tp.version, u8(0x00));
    expect_eq(tp.trace_id[0], u8(0x4B));
    expect_eq(tp.trace_id[15], u8(0x36));
    expect_eq(tp.parent_id[0], u8(0x00));
    expect_eq(tp.parent_id[7], u8(0xB7));
    expect_eq(tp.trace_flags, u8(0x01));
    expect_eq(tp.isSampled(), true);

    let out: [64]u8 = std.mem.zeroes([64]u8);
    const formatted = tp.format(&out) ?? unreachable;
    expect_eq(formatted.len, usize(55));
    expect_eq(std.mem.eql(u8, formatted, raw), true);
});

test("2. TraceParent withSampled and withParentId immutable span derivation", () => {
    const raw = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-00";
    const tp = TraceParent.parse(raw) ?? unreachable;
    expect_eq(tp.isSampled(), false);

    const sampled = tp.withSampled(true);
    expect_eq(sampled.isSampled(), true);
    expect_eq(sampled.withSampled(false).isSampled(), false);

    const next_parent: [8]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];
    const child = sampled.withParentId(next_parent);
    let out: [64]u8 = std.mem.zeroes([64]u8);
    const formatted = child.format(&out) ?? unreachable;
    expect_eq(
        std.mem.eql(u8, formatted, "00-4bf92f3577b34da6a3ce929d0e0e4736-1122334455667788-01"),
        true
    );
});

test("3. TraceParent rejects 0xff version, all-zero IDs, uppercase hex, and v0 trailing junk; allows v>0 extra", () => {
    // 拒绝 version == 0xff
    expect_eq(TraceParent.parse("ff-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") == null, true);
    // 拒绝全零 trace_id
    expect_eq(TraceParent.parse("00-00000000000000000000000000000000-00f067aa0ba902b7-01") == null, true);
    // 拒绝全零 parent_id
    expect_eq(TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01") == null, true);
    // 拒绝大写十六进制
    expect_eq(TraceParent.parse("00-4BF92F3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") == null, true);
    // version 00 拒绝尾部附加字段
    expect_eq(TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-extra") == null, true);

    // version > 00 允许 -extra 向前兼容
    const future = TraceParent.parse("01-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01-futureext") ?? unreachable;
    expect_eq(future.version, u8(0x01));
    expect_eq(future.isSampled(), true);
});

test("4. TraceStateIter and findTraceState handle simple keys, multi-tenant keys, and malformed skipping", () => {
    const hdr = "rojo=00f067aa0ba902b7, , BADKEY=123, tenant1@vendor_a=t61rcWkgMzE, congo=ucfJifl5GOE";
    let it = TraceStateIter.init(hdr);

    const e0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.key, "rojo"), true);
    expect_eq(std.mem.eql(u8, e0.value, "00f067aa0ba902b7"), true);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.key, "tenant1@vendor_a"), true);
    expect_eq(std.mem.eql(u8, e1.value, "t61rcWkgMzE"), true);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.key, "congo"), true);
    expect_eq(std.mem.eql(u8, e2.value, "ucfJifl5GOE"), true);
    expect_eq(it.next() == null, true);

    expect_eq(std.mem.eql(u8, findTraceState(hdr, "tenant1@vendor_a") ?? unreachable, "t61rcWkgMzE"), true);
    expect_eq(findTraceState(hdr, "BADKEY") == null, true);
    expect_eq(findTraceState(hdr, "missing") == null, true);
});

test("5. TraceParent.format buffer-too-small guard", () => {
    const tp = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01") ?? unreachable;
    let short_buf: [54]u8 = std.mem.zeroes([54]u8);
    expect_eq(tp.format(&short_buf) == null, true);

    let exact_buf: [55]u8 = std.mem.zeroes([55]u8);
    expect_eq((tp.format(&exact_buf) ?? unreachable).len, usize(55));
});
