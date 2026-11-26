import { test, expect_eq } from "cap:test";
import {
    parseIpv4,
    formatIpv4,
    Ipv4Net
} from "../../packages/@cap/ipnet/src/index.cp";

fn bytesEqStr(buf: []u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ipnet — strict parseIpv4 accepts valid and rejects malformed inputs", () => {
        // 192.168.1.42 = (192<<24)|(168<<16)|(1<<8)|42 = 3232235818
        expect_eq(parseIpv4("192.168.1.42") ?? 0, 3232235818);
        expect_eq(parseIpv4("0.0.0.0") ?? 1, 0);
        expect_eq(parseIpv4("255.255.255.255") ?? 0, 4294967295);

        // Reject leading zeros, >255, wrong segment count, non-digits
        expect_eq(parseIpv4("192.168.01.1") == null, true);
        expect_eq(parseIpv4("00.0.0.0") == null, true);
        expect_eq(parseIpv4("256.0.0.1") == null, true);
        expect_eq(parseIpv4("1.2.3") == null, true);
        expect_eq(parseIpv4("1.2.3.4.5") == null, true);
        expect_eq(parseIpv4("1.2.3.4.") == null, true);
        expect_eq(parseIpv4("") == null, true);
    });

    test("2. @cap/ipnet — formatIpv4 and formatCidr round-trip into caller buffer", () => {
        let out: [32]u8 = [0; 32];
        let ip: u32 = parseIpv4("192.168.1.42") ?? 0;
        let n1 = formatIpv4(ip, out[0..32]);
        expect_eq(bytesEqStr(out[0..32], n1, "192.168.1.42"), true);

        let net_opt = Ipv4Net.parse("10.20.30.40/24");
        expect_eq(net_opt != null, true);
        let net = net_opt ?? Ipv4Net { addr: 0, prefix_len: 0 };
        let n2 = net.formatCidr(out[0..32]);
        expect_eq(bytesEqStr(out[0..32], n2, "10.20.30.40/24"), true);

        let tr = net.trunc();
        let n3 = tr.formatCidr(out[0..32]);
        expect_eq(bytesEqStr(out[0..32], n3, "10.20.30.0/24"), true);
    });

    test("3. @cap/ipnet — netmask, hostmask, network, broadcast for /24 and /16", () => {
        let net = Ipv4Net.parse("192.168.1.42/24") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(net.netmask(), parseIpv4("255.255.255.0") ?? 0);
        expect_eq(net.hostmask(), parseIpv4("0.0.0.255") ?? 0);
        expect_eq(net.network(), parseIpv4("192.168.1.0") ?? 0);
        expect_eq(net.broadcast(), parseIpv4("192.168.1.255") ?? 0);
        expect_eq(net.hostCount(), 254);

        // Invalid CIDR inputs
        expect_eq(Ipv4Net.new(0, 33) == null, true);
        expect_eq(Ipv4Net.parse("192.168.1.1/33") == null, true);
        expect_eq(Ipv4Net.parse("192.168.1.1/08") == null, true);
        expect_eq(Ipv4Net.parse("192.168.1.1") == null, true);
    });

    test("4. @cap/ipnet — /0, /31, and /32 shift boundary safety and hostCount", () => {
        let all = Ipv4Net.parse("10.1.2.3/0") ?? Ipv4Net { addr: 0, prefix_len: 32 };
        expect_eq(all.netmask(), 0);
        expect_eq(all.hostmask(), 4294967295);
        expect_eq(all.network(), 0);
        expect_eq(all.broadcast(), 4294967295);
        let expected_zero_hosts: u64 = ((1 as u64) << 32) - 2;
        expect_eq(all.hostCount(), expected_zero_hosts);
        expect_eq(all.supernet() == null, true);

        let p2p = Ipv4Net.parse("10.0.0.0/31") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(p2p.hostCount(), 2);

        let host = Ipv4Net.parse("192.168.1.42/32") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(host.netmask(), 4294967295);
        expect_eq(host.hostmask(), 0);
        expect_eq(host.network(), host.addr);
        expect_eq(host.broadcast(), host.addr);
        expect_eq(host.hostCount(), 1);
    });

    test("5. @cap/ipnet — containsIp and containsNet containment relations", () => {
        let net16 = Ipv4Net.parse("10.10.0.0/16") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        let net24 = Ipv4Net.parse("10.10.5.99/24") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        let other = Ipv4Net.parse("10.11.5.0/24") ?? Ipv4Net { addr: 0, prefix_len: 0 };

        expect_eq(net16.containsIp(parseIpv4("10.10.255.254") ?? 0), true);
        expect_eq(net16.containsIp(parseIpv4("10.11.0.1") ?? 0), false);
        expect_eq(net16.containsNet(net24), true);
        expect_eq(net24.containsNet(net16), false);
        expect_eq(net16.containsNet(other), false);
    });

    test("6. @cap/ipnet — supernet widens prefix and masks host bits", () => {
        let n24 = Ipv4Net.parse("192.168.3.42/24") ?? Ipv4Net { addr: 0, prefix_len: 0 };
        let sup_opt = n24.supernet();
        expect_eq(sup_opt != null, true);
        let sup = sup_opt ?? Ipv4Net { addr: 0, prefix_len: 0 };
        expect_eq(sup.prefix_len, 23);
        expect_eq(sup.addr, parseIpv4("192.168.2.0") ?? 0);
        expect_eq(sup.containsNet(n24), true);
    });
}
