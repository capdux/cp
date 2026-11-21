import { RfcIpv4Addr as Ipv4Addr, RfcIpv6Addr as Ipv6Addr, RfcIpAddr as IpAddr, IpVersion } from "cap:net";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("Ipv4Addr parse, classification, subnet, and format", () => {
    const lo = Ipv4Addr.parse("127.0.0.1").?;
    expect(lo.isLoopback());
    expect(!lo.isPrivate());

    const lan_ip = Ipv4Addr.parse("192.168.1.42").?;
    expect(lan_ip.isPrivate());
    const net = Ipv4Addr.parse("192.168.1.0").?;
    expect(lan_ip.inSubnet(net, 24));
    expect(!lo.inSubnet(net, 24));

    let buf: [64]u8 = [0; 64];
    expect(strEq(lan_ip.format(buf[0..]).?, "192.168.1.42"));

    // Reject invalid forms
    expect(Ipv4Addr.parse("01.2.3.4") == null);
    expect(Ipv4Addr.parse("256.0.0.1") == null);
    expect(Ipv4Addr.parse("1.2.3") == null);
});

test("Ipv6Addr RFC 5952 canonical compression and IPv4-mapped", () => {
    let buf: [64]u8 = [0; 64];

    const lo = Ipv6Addr.parse("0000:0000:0000:0000:0000:0000:0000:0001").?;
    expect(lo.isLoopback());
    expect(strEq(lo.format(buf[0..]).?, "::1"));

    // Longest run of >=2 zeros compressed; single zero NOT compressed (RFC 5952 §4.2.2)
    const a1 = Ipv6Addr.parse("2001:0db8:0000:0001:0000:0000:0000:0001").?;
    expect(strEq(a1.format(buf[0..]).?, "2001:db8:0:1::1"));

    const single_zero = Ipv6Addr.parse("2001:db8:0:1:1:1:1:1").?;
    expect(strEq(single_zero.format(buf[0..]).?, "2001:db8:0:1:1:1:1:1"));

    // IPv4-mapped IPv6
    const mapped = Ipv6Addr.parse("::FFFF:192.0.2.128").?;
    const v4 = mapped.toIpv4Mapped().?;
    expect(v4.octets[0] == 192 && v4.octets[3] == 128);
    expect(strEq(mapped.format(buf[0..]).?, "::ffff:192.0.2.128"));

    // CIDR subnet matching
    const sub = Ipv6Addr.parse("2001:db8::").?;
    expect(a1.inSubnet(sub, 32));
    expect(!lo.inSubnet(sub, 32));

    // Reject double :: and bad syntax
    expect(Ipv6Addr.parse("2001::db8::1") == null);
    expect(Ipv6Addr.parse("2001:db8:") == null);
    expect(Ipv6Addr.parse("1:2:3:4:5:6:7") == null);
});

test("IpAddr unified parse and format", () => {
    let buf: [64]u8 = [0; 64];
    const ip4 = IpAddr.parse("10.0.0.1").?;
    expect(ip4.version == IpVersion.v4);
    expect(strEq(ip4.format(buf[0..]).?, "10.0.0.1"));

    const ip6 = IpAddr.parse("fe80::1").?;
    expect(ip6.version == IpVersion.v6);
    expect(ip6.v6.isLinkLocal());
    expect(strEq(ip6.format(buf[0..]).?, "fe80::1"));
});
