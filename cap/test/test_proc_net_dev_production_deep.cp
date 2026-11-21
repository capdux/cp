// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/net/dev Network Interface Stats Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NetDevStats,
    ProcNetDevIter,
    findNetDev,
    sumNonLoopbackBytes,
} from "cap:sys";

test("1. ProcNetDevIter skips 2-line kernel header and parses standard 16-column interface stats", () => {
    const sample =
        "Inter-|   Receive                                                |  Transmit\n"
        ++ " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        ++ "    lo: 1048576    1200    0    0    0     0          0         0  1048576    1200    0    0    0     0       0          0\n"
        ++ "  eth0: 9876543210  54321    3    5    0     0          0         0 1234567890   65432    2    7    0     0       0          0\n";

    let it = ProcNetDevIter.init(sample);

    const lo = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, lo.iface, "lo"), true);
    expect_eq(lo.isLoopback(), true);
    expect_eq(lo.rx_bytes, 1048576);
    expect_eq(lo.tx_bytes, 1048576);
    expect_eq(lo.totalBytes(), 2097152);
    expect_eq(lo.totalErrors(), 0);

    const eth0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, eth0.iface, "eth0"), true);
    expect_eq(eth0.isLoopback(), false);
    expect_eq(eth0.rx_bytes, 9876543210);
    expect_eq(eth0.rx_packets, 54321);
    expect_eq(eth0.rx_errs, 3);
    expect_eq(eth0.rx_drop, 5);
    expect_eq(eth0.tx_bytes, 1234567890);
    expect_eq(eth0.tx_packets, 65432);
    expect_eq(eth0.tx_errs, 2);
    expect_eq(eth0.tx_drop, 7);
    expect_eq(eth0.totalBytes(), 11111111100);
    expect_eq(eth0.totalErrors(), 5);

    expect_eq(it.next() == null, true);
});

test("2. Handles glued colon counters (iface:12345) and aliased interface names (eth0:0:999)", () => {
    const sample =
        "Inter-|   Receive | Transmit\n"
        ++ " face |bytes packets errs drop fifo frame compressed multicast|bytes packets errs drop fifo colls carrier compressed\n"
        ++ "ens33:999999999999 100 1 2 0 0 0 0 888888888888 200 3 4 0 0 0 0\r\n"
        ++ "eth0:0:42 1 0 0 0 0 0 0 58 2 0 0 0 0 0 0\n";

    let it = ProcNetDevIter.init(sample);

    const ens = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, ens.iface, "ens33"), true);
    expect_eq(ens.rx_bytes, 999999999999);
    expect_eq(ens.tx_bytes, 888888888888);
    expect_eq(ens.totalErrors(), 4);

    const alias = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, alias.iface, "eth0:0"), true);
    expect_eq(alias.rx_bytes, 42);
    expect_eq(alias.tx_bytes, 58);
    expect_eq(alias.totalBytes(), 100);
});

test("3. findNetDev locates specific interfaces and returns null when absent", () => {
    const sample =
        "Inter-| Receive | Transmit\n"
        ++ " face | ...\n"
        ++ "    lo: 100 1 0 0 0 0 0 0 200 2 0 0 0 0 0 0\n"
        ++ " wlan0: 500 10 0 1 0 0 0 0 700 20 1 0 0 0 0 0\n"
        ++ "docker0: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n";

    const w = findNetDev(sample, "wlan0") ?? unreachable;
    expect_eq(w.rx_bytes, 500);
    expect_eq(w.tx_bytes, 700);
    expect_eq(w.rx_drop, 1);
    expect_eq(w.tx_errs, 1);

    expect_eq(findNetDev(sample, "eth9") == null, true);
    expect_eq(findNetDev(sample, "") == null, true);
});

test("4. sumNonLoopbackBytes aggregates all non-lo interfaces and excludes lo", () => {
    const sample =
        "Inter-| Receive | Transmit\n"
        ++ " face | ...\n"
        ++ "    lo: 999999 10 0 0 0 0 0 0 999999 10 0 0 0 0 0 0\n"
        ++ "  eth0: 1000 10 0 0 0 0 0 0 2000 20 0 0 0 0 0 0\n"
        ++ " wlan0: 300 5 0 0 0 0 0 0 400 6 0 0 0 0 0 0\n";

    expect_eq(sumNonLoopbackBytes(sample), 3700);
    expect_eq(sumNonLoopbackBytes(""), 0);
});

test("5. Malformed lines, fewer than 16 columns, non-numeric tokens, and u64 overflow are safely skipped", () => {
    const broken =
        "Inter-| Receive | Transmit\n"
        ++ " face | ...\n"
        ++ "   : 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16\n"
        ++ "short0: 1 2 3 4 5\n"
        ++ "badnum: 1 abc 0 0 0 0 0 0 1 2 0 0 0 0 0 0\n"
        ++ "overflow: 99999999999999999999999999 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
        ++ "good0: 11 1 0 0 0 0 0 0 22 2 0 0 0 0 0 0";

    const g = findNetDev(broken, "good0") ?? unreachable;
    expect_eq(g.totalBytes(), 33);
    expect_eq(sumNonLoopbackBytes(broken), 33);
});
