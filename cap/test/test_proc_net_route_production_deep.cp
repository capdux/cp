// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/net/route Routing Table Parser)
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    RTF_UP,
    RTF_GATEWAY,
    RTF_HOST,
    parseHexIpv4Le,
    RouteEntry,
    ProcRouteIter,
    findDefaultGateway,
} from "cap:sys";

test("1. parseHexIpv4Le converts 8-hex little-endian IPv4 and rejects invalid inputs", () => {
    const gw = parseHexIpv4Le("0100A8C0") ?? unreachable;
    expect_eq(gw[0], 192);
    expect_eq(gw[1], 168);
    expect_eq(gw[2], 0);
    expect_eq(gw[3], 1);

    const lo = parseHexIpv4Le("0100007f") ?? unreachable;
    expect_eq(lo[0], 127);
    expect_eq(lo[1], 0);
    expect_eq(lo[2], 0);
    expect_eq(lo[3], 1);

    const zero = parseHexIpv4Le("00000000") ?? unreachable;
    expect_eq(zero[0], 0);
    expect_eq(zero[3], 0);

    expect_eq(parseHexIpv4Le("0100A8C") == null, true);
    expect_eq(parseHexIpv4Le("0100A8C00") == null, true);
    expect_eq(parseHexIpv4Le("0100G8C0") == null, true);
});

test("2. ProcRouteIter skips header/blank lines and parses standard 11-column entries", () => {
    const table =
        "Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask\t\tMTU\tWindow\tIRTT\n"
        ++ "\n"
        ++ "eth0\t00000000\t0100A8C0\t0003\t0\t0\t100\t00000000\t1500\t0\t0\n"
        ++ "eth0\t0000A8C0\t00000000\t0001\t0\t0\t100\t00FFFFFF\t0\t0\t0\n"
        ++ "lo\t0100007F\t00000000\t0005\t0\t0\t0\tFFFFFFFF\t65536\t0\t0\r\n";

    let it = ProcRouteIter.init(table);

    const r1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r1.iface, "eth0"), true);
    expect_eq(r1.destination[0], 0);
    expect_eq(r1.gateway[0], 192);
    expect_eq(r1.gateway[1], 168);
    expect_eq(r1.gateway[2], 0);
    expect_eq(r1.gateway[3], 1);
    expect_eq(r1.flags, RTF_UP | RTF_GATEWAY);
    expect_eq(r1.metric, 100);
    expect_eq(r1.mtu, 1500);
    expect_eq(r1.isUp(), true);
    expect_eq(r1.isGateway(), true);
    expect_eq(r1.isDefaultRoute(), true);
    expect_eq(r1.prefixLen(), 0);

    const r2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r2.iface, "eth0"), true);
    expect_eq(r2.destination[0], 192);
    expect_eq(r2.destination[1], 168);
    expect_eq(r2.mask[0], 255);
    expect_eq(r2.mask[1], 255);
    expect_eq(r2.mask[2], 255);
    expect_eq(r2.mask[3], 0);
    expect_eq(r2.isUp(), true);
    expect_eq(r2.isGateway(), false);
    expect_eq(r2.isDefaultRoute(), false);
    expect_eq(r2.prefixLen(), 24);

    const r3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, r3.iface, "lo"), true);
    expect_eq(r3.flags, RTF_UP | RTF_HOST);
    expect_eq(r3.mtu, 65536);
    expect_eq(r3.prefixLen(), 32);

    expect_eq(it.next() == null, true);
});

test("3. RouteEntry.prefixLen computes CIDR prefix lengths across octet boundaries", () => {
    // /0, /8, /16, /20, /24, /30, /32
    const table =
        "Iface\tDestination\tGateway\tFlags\tRefCnt\tUse\tMetric\tMask\tMTU\tWindow\tIRTT\n"
        ++ "a\t0000000A\t00000000\t0001\t0\t0\t0\t000000FF\t0\t0\t0\n"
        ++ "b\t000010AC\t00000000\t0001\t0\t0\t0\t0000FFFF\t0\t0\t0\n"
        ++ "c\t000010AC\t00000000\t0001\t0\t0\t0\t00F0FFFF\t0\t0\t0\n"
        ++ "d\t0000A8C0\t00000000\t0001\t0\t0\t0\tFCFFFFFF\t0\t0\t0\n";

    let it = ProcRouteIter.init(table);
    expect_eq((it.next() ?? unreachable).prefixLen(), 8);
    expect_eq((it.next() ?? unreachable).prefixLen(), 16);
    expect_eq((it.next() ?? unreachable).prefixLen(), 20);
    expect_eq((it.next() ?? unreachable).prefixLen(), 30);
    expect_eq(it.next() == null, true);
});

test("4. findDefaultGateway selects lowest metric active default gateway route", () => {
    const table =
        "Iface\tDestination\tGateway\tFlags\tRefCnt\tUse\tMetric\tMask\tMTU\tWindow\tIRTT\n"
        ++ "wlan0\t00000000\t0101A8C0\t0003\t0\t0\t600\t00000000\t0\t0\t0\n"
        ++ "tun0\t00000000\t00000000\t0001\t0\t0\t50\t00000000\t0\t0\t0\n"
        ++ "eth1\t00000000\t0102A8C0\t0002\t0\t0\t10\t00000000\t0\t0\t0\n"
        ++ "eth0\t00000000\t0100A8C0\t0003\t0\t0\t100\t00000000\t0\t0\t0\n"
        ++ "eth2\t00000000\t0103A8C0\t0003\t0\t0\t100\t00000000\t0\t0\t0\n";

    // tun0 无 RTF_GATEWAY；eth1 无 RTF_UP；eth0 metric=100 最小且先于 eth2
    const best = findDefaultGateway(table) ?? unreachable;
    expect_eq(std.mem.eql(u8, best.iface, "eth0"), true);
    expect_eq(best.metric, 100);
    expect_eq(best.gateway[0], 192);
    expect_eq(best.gateway[1], 168);
    expect_eq(best.gateway[2], 0);
    expect_eq(best.gateway[3], 1);
});

test("5. Malformed rows, truncated columns, and empty tables are safely skipped", () => {
    const broken =
        "Iface\tDestination\tGateway\tFlags\tRefCnt\tUse\tMetric\tMask\tMTU\tWindow\tIRTT\n"
        ++ "short\t00000000\t0100A8C0\n"
        ++ "badhex\tZZZZZZZZ\t0100A8C0\t0003\t0\t0\t10\t00000000\t0\t0\t0\n"
        ++ "baddec\t00000000\t0100A8C0\t0003\t0\t0\tabc\t00000000\t0\t0\t0\n"
        ++ "ok0\t00000000\tFE01A8C0\t0003\t0\t0\t200\t00000000\t1500\t0\t0";

    const gw = findDefaultGateway(broken) ?? unreachable;
    expect_eq(std.mem.eql(u8, gw.iface, "ok0"), true);
    expect_eq(gw.gateway[0], 192);
    expect_eq(gw.gateway[1], 168);
    expect_eq(gw.gateway[2], 1);
    expect_eq(gw.gateway[3], 254);

    expect_eq(findDefaultGateway("") == null, true);
    expect_eq(findDefaultGateway("Iface\tDestination\n") == null, true);
});
