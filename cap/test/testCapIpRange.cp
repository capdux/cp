// cap/test/testCapIpRange.cp
// Comprehensive unit tests for @cap/ipnet (IPv4 & CIDR subnet range engine)

import { test, expect_eq } from "cap:test";
import {
    parseIpv4,
    formatIpv4,
    Ipv4Net,
} from "../../packages/ipnet/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ipnet — parseIpv4 & formatIpv4 round-trip & rejects leading zeros", () => {
        const r = parseIpv4("192.168.1.42");
        expect_eq(r != null, true);
        let buf: [24]u8 = undefined;
        const n: usize = formatIpv4(r ?? (0 as u32), buf[0..24]);
        expect_eq(sliceEq(buf[0..n], "192.168.1.42"), true);

        const z = parseIpv4("0.0.0.0");
        expect_eq(z != null, true);
        expect_eq(z ?? (1 as u32), 0 as u32);

        expect_eq(parseIpv4("010.0.0.1") == null, true);
        expect_eq(parseIpv4("192.168.01.1") == null, true);
        expect_eq(parseIpv4("256.0.0.1") == null, true);
        expect_eq(parseIpv4("1.2.3") == null, true);
    });

    test("2. @cap/ipnet — Ipv4Net.parse for /24, /0 & /32", () => {
        const c24Opt = Ipv4Net.parse("192.168.1.130/24");
        expect_eq(c24Opt != null, true);
        const c24 = c24Opt ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(c24.prefix_len, 24 as u8);
        expect_eq(c24.netmask(), 0xFFFFFF00 as u32);
        expect_eq(c24.network(), parseIpv4("192.168.1.0") ?? (0 as u32));
        expect_eq(c24.broadcast(), parseIpv4("192.168.1.255") ?? (0 as u32));

        const c0Opt = Ipv4Net.parse("0.0.0.0/0");
        expect_eq(c0Opt != null, true);
        const c0 = c0Opt ?? Ipv4Net { addr: 0, prefix_len: 32 };
        expect_eq(c0.network(), 0 as u32);
        expect_eq(c0.broadcast(), 0xFFFFFFFF as u32);

        const c32Opt = Ipv4Net.parse("8.8.8.8/32");
        expect_eq(c32Opt != null, true);
        const c32 = c32Opt ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(c32.network(), c32.broadcast());
    });

    test("3. @cap/ipnet — containsIp exact subnet boundaries", () => {
        const c = Ipv4Net.parse("10.10.0.0/16") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(c.containsIp(parseIpv4("10.10.0.0") ?? (0 as u32)), true);
        expect_eq(c.containsIp(parseIpv4("10.10.255.255") ?? (0 as u32)), true);
        expect_eq(c.containsIp(parseIpv4("10.9.255.255") ?? (0 as u32)), false);
        expect_eq(c.containsIp(parseIpv4("10.11.0.0") ?? (0 as u32)), false);
    });

    test("4. @cap/ipnet — containsNet & supernet hierarchy", () => {
        const big = Ipv4Net.parse("10.0.0.0/8") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        const sub = Ipv4Net.parse("10.20.0.0/16") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        const other = Ipv4Net.parse("192.168.0.0/16") ?? Ipv4Net { addr: 0, prefix_len: 0 };

        expect_eq(big.containsNet(sub), true);
        expect_eq(sub.containsNet(big), false);
        expect_eq(big.containsNet(other), false);

        const supOpt = sub.supernet();
        expect_eq(supOpt != null, true);
        const sup = supOpt ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(sup.prefix_len, 15 as u8);
        expect_eq(sup.containsNet(sub), true);
    });

    test("5. @cap/ipnet — rejects invalid CIDR & formats truncated CIDR", () => {
        expect_eq(Ipv4Net.parse("10.0.0.0/33") == null, true);
        expect_eq(Ipv4Net.parse("10.0.0.0") == null, true);
        expect_eq(Ipv4Net.parse("10.0.0.0/08") == null, true);
        expect_eq(Ipv4Net.parse("bad/24") == null, true);

        const net = Ipv4Net.parse("192.168.1.99/24") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        const tr = net.trunc();
        let buf: [24]u8 = undefined;
        const n = tr.formatCidr(buf[0..24]);
        expect_eq(sliceEq(buf[0..n], "192.168.1.0/24"), true);
    });
}
