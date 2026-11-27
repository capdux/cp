// cap/test/testCapCidrTools.cp
// Comprehensive unit tests for @cap/cidr-tools

import { test, expect_eq } from "cap:test";
import {
    Cidr4,
    parseIpv4,
    parseCidr4,
    formatCidr4,
    cidrStart,
    cidrEnd,
    cidrHostCount,
    containsIp,
    containsCidr,
    overlaps,
    mergeCidrs,
    excludeCidr,
} from "../../packages/cidr-tools/src/index.cp";

fn main() {
    test("1. @cap/cidr-tools — parseIpv4, parseCidr4, formatCidr4 & invalid rejection", () => {
        expect_eq(parseIpv4("10.0.0.1"), 0x0A000001 as i64);
        expect_eq(parseIpv4("255.255.255.255"), 0xFFFFFFFF as i64);
        expect_eq(parseIpv4("01.0.0.1"), -1 as i64);
        expect_eq(parseIpv4("256.0.0.1"), -1 as i64);
        expect_eq(parseIpv4("1.2.3"), -1 as i64);

        // Non-zero host bits auto-normalized
        const c1 = parseCidr4("10.1.2.3/8");
        expect_eq(c1.valid, true);
        expect_eq(c1.prefix, 8 as u8);
        let buf1: [32]u8 = undefined;
        const n1 = formatCidr4(c1, buf1[0..32]);
        expect_eq(buf1[0..n1], "10.0.0.0/8");

        // Bare IP defaults to /32
        const c2 = parseCidr4("1.2.3.4");
        expect_eq(c2.valid, true);
        expect_eq(c2.prefix, 32 as u8);
        let buf2: [32]u8 = undefined;
        const n2 = formatCidr4(c2, buf2[0..32]);
        expect_eq(buf2[0..n2], "1.2.3.4/32");

        expect_eq(parseCidr4("10.0.0.0/33").valid, false);
        expect_eq(parseCidr4("10.0.0.0/08").valid, false);
    });

    test("2. @cap/cidr-tools — cidrStart, cidrEnd & cidrHostCount including /0 and /32", () => {
        const all = parseCidr4("0.0.0.0/0");
        expect_eq(cidrStart(all), 0 as u32);
        expect_eq(cidrEnd(all), 0xFFFFFFFF as u32);
        expect_eq(cidrHostCount(all), 4294967296 as u64);

        const single = parseCidr4("192.168.1.5/32");
        expect_eq(cidrStart(single), cidrEnd(single));
        expect_eq(cidrHostCount(single), 1 as u64);

        const net24 = parseCidr4("192.168.1.0/24");
        expect_eq(cidrHostCount(net24), 256 as u64);
    });

    test("3. @cap/cidr-tools — containsIp, containsCidr & overlaps", () => {
        const net16 = parseCidr4("10.10.0.0/16");
        const net24 = parseCidr4("10.10.5.0/24");
        const other = parseCidr4("10.11.0.0/16");

        expect_eq(containsIp(net24, parseIpv4("10.10.5.42") as u32), true);
        expect_eq(containsIp(net24, parseIpv4("10.10.6.1") as u32), false);
        expect_eq(containsCidr(net16, net24), true);
        expect_eq(containsCidr(net24, net16), false);
        expect_eq(overlaps(net16, net24), true);
        expect_eq(overlaps(net16, other), false);
    });

    test("4. @cap/cidr-tools — mergeCidrs aggregates adjacent /25 blocks into /24 & deduplicates", () => {
        const inputs: [4]Cidr4 = [
            parseCidr4("192.168.1.128/25"),
            parseCidr4("192.168.1.0/25"),
            parseCidr4("192.168.1.64/26"),
            parseCidr4("10.0.0.0/24"),
        ];
        let out: [8]Cidr4 = undefined;
        const n = mergeCidrs(inputs[0..4], out[0..8]);
        expect_eq(n, 2 as usize);

        let b0: [32]u8 = undefined;
        const l0 = formatCidr4(out[0], b0[0..32]);
        expect_eq(b0[0..l0], "10.0.0.0/24");

        let b1: [32]u8 = undefined;
        const l1 = formatCidr4(out[1], b1[0..32]);
        expect_eq(b1[0..l1], "192.168.1.0/24");
    });

    test("5. @cap/cidr-tools — excludeCidr removes 10.0.0.0/26 from 10.0.0.0/24 -> /26 + /25", () => {
        const base = parseCidr4("10.0.0.0/24");
        const rem = parseCidr4("10.0.0.0/26");
        let out: [8]Cidr4 = undefined;
        const n = excludeCidr(base, rem, out[0..8]);
        expect_eq(n, 2 as usize);

        let b0: [32]u8 = undefined;
        const l0 = formatCidr4(out[0], b0[0..32]);
        expect_eq(b0[0..l0], "10.0.0.64/26");

        let b1: [32]u8 = undefined;
        const l1 = formatCidr4(out[1], b1[0..32]);
        expect_eq(b1[0..l1], "10.0.0.128/25");
    });

    test("6. @cap/cidr-tools — excludeCidr single /32 from /24 yields 8 canonical blocks (255 IPs)", () => {
        const base = parseCidr4("10.0.0.0/24");
        const rem = parseCidr4("10.0.0.0/32");
        let out: [16]Cidr4 = undefined;
        const n = excludeCidr(base, rem, out[0..16]);
        expect_eq(n, 8 as usize);

        let totalIps: u64 = 0;
        let i: usize = 0;
        while (i < n) {
            totalIps += cidrHostCount(out[i]);
            expect_eq(containsIp(out[i], parseIpv4("10.0.0.0") as u32), false);
            i += 1;
        }
        expect_eq(totalIps, 255 as u64);
    });
}
