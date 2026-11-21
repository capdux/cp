import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ServerTimingIter,
    findServerTiming,
    appendServerTiming,
} from "cap:http_server_timing";

test("1. ServerTimingIter parses dur_us, quoted desc with commas/semicolons, and bare metrics", () => {
    const hdr = "db;dur=53.2;desc=\"SQL, read; primary\", cache;desc=\"Hit\", missedCache";
    let it = ServerTimingIter.init(hdr);

    const m0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m0.name, "db"), true);
    expect_eq(m0.dur_us ?? unreachable, u64(53200));
    expect_eq(std.mem.eql(u8, m0.desc ?? unreachable, "SQL, read; primary"), true);

    const m1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m1.name, "cache"), true);
    expect_eq(m1.dur_us == null, true);
    expect_eq(std.mem.eql(u8, m1.desc ?? unreachable, "Hit"), true);

    const m2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, m2.name, "missedCache"), true);
    expect_eq(m2.dur_us == null, true);
    expect_eq(m2.desc == null, true);

    expect_eq(it.next() == null, true);
});

test("2. case-insensitive params, first-wins duplicate params, and sub-millisecond precision", () => {
    const hdr = "app;DUR=0.125;dur=999;DESC=first;desc=second, edge;dur=.05";
    let it = ServerTimingIter.init(hdr);

    const m0 = it.next() ?? unreachable;
    expect_eq(m0.dur_us ?? unreachable, u64(125));
    expect_eq(std.mem.eql(u8, m0.desc ?? unreachable, "first"), true);

    const m1 = it.next() ?? unreachable;
    expect_eq(m1.dur_us ?? unreachable, u64(50));
});

test("3. findServerTiming lookup and missing metric handling", () => {
    const hdr = "dns;dur=1.5, tls;dur=12.005;desc=\"TLS 1.3\", total;dur=42";
    const tls = findServerTiming(hdr, "tls") ?? unreachable;
    expect_eq(tls.dur_us ?? unreachable, u64(12005));
    expect_eq(std.mem.eql(u8, tls.desc ?? unreachable, "TLS 1.3"), true);

    expect_eq(findServerTiming(hdr, "TLS") == null, true);
    expect_eq(findServerTiming(hdr, "missing") == null, true);
});

test("4. appendServerTiming formatting, auto-quoting/escaping, and round-trip", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let pos: usize = 0;

    expect_eq(appendServerTiming(&buf, &pos, "db", 53200, "SQL, \"read\""), true);
    expect_eq(appendServerTiming(&buf, &pos, "cache", 12005, "Hit"), true);
    expect_eq(appendServerTiming(&buf, &pos, "miss", null, null), true);

    const header = buf[0..pos];
    expect_eq(
        std.mem.eql(u8, header, "db;dur=53.2;desc=\"SQL, \\\"read\\\"\", cache;dur=12.005;desc=Hit, miss"),
        true
    );

    const found = findServerTiming(header, "cache") ?? unreachable;
    expect_eq(found.dur_us ?? unreachable, u64(12005));
    expect_eq(std.mem.eql(u8, found.desc ?? unreachable, "Hit"), true);
});

test("5. appendServerTiming atomic rollback on overflow and invalid name rejection", () => {
    let buf: [20]u8 = std.mem.zeroes([20]u8);
    let pos: usize = 0;

    expect_eq(appendServerTiming(&buf, &pos, "ok", 1000, null), true);
    const saved = pos;

    // 缓冲区不足时返回 false 且 pos 不被破坏
    expect_eq(appendServerTiming(&buf, &pos, "very_long_metric_name", 12345, "description"), false);
    expect_eq(pos, saved);

    // 非法指标名称拒绝
    expect_eq(appendServerTiming(&buf, &pos, "", 1000, null), false);
    expect_eq(appendServerTiming(&buf, &pos, "bad name", 1000, null), false);
});
