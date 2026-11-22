// Source: cp/cap/test/test_cidr_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:cidr & cap.cidr

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CidrError,
    Ipv4Addr,
    Ipv6Addr,
    Cidr4,
    Cidr6,
    LpmTable4,
} from "../../lib/net/cidr_.cp";

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // 1. Ipv4Addr parsing, formatting & classification
    const priv1 = Ipv4Addr.parse("10.0.0.1")?;
    const priv2 = Ipv4Addr.parse("172.16.5.4")?;
    const priv3 = Ipv4Addr.parse("192.168.100.254")?;
    const pub1 = Ipv4Addr.parse("172.32.0.1")?;
    const lo = Ipv4Addr.parse("127.0.0.1")?;
    const ll = Ipv4Addr.parse("169.254.10.20")?;

    expect_eq(priv1.isPrivate(), true);
    expect_eq(priv2.isPrivate(), true);
    expect_eq(priv3.isPrivate(), true);
    expect_eq(pub1.isPrivate(), false);
    expect_eq(lo.isLoopback(), true);
    expect_eq(lo.isPrivate(), false);
    expect_eq(ll.isLinkLocal(), true);

    const fmt_v4 = priv3.format(alloc)?;
    defer alloc.free(fmt_v4);
    expect_eq(std.mem.eql(u8, fmt_v4, "192.168.100.254"), true);

    // 2. Cidr4 host-bit normalization & strict parsing
    const c_norm = Cidr4.parse("192.168.1.42/24")?;
    expect_eq(c_norm.network().eql(Ipv4Addr.parse("192.168.1.0")?), true);
    expect_eq(c_norm.broadcast().eql(Ipv4Addr.parse("192.168.1.255")?), true);
    expect_eq(c_norm.hostCount(), 256 as u64);
    expect_eq(c_norm.contains(Ipv4Addr.parse("192.168.1.200")?), true);
    expect_eq(c_norm.contains(Ipv4Addr.parse("192.168.2.1")?), false);

    const c_strict = Cidr4.parseStrict("10.10.0.0/16")?;
    expect_eq(c_strict.hostCount(), 65536 as u64);

    // 3. Boundary prefix lengths: /0, /31, /32
    const c_zero = Cidr4.parse("1.2.3.4/0")?;
    expect_eq(c_zero.network().eql(Ipv4Addr.parse("0.0.0.0")?), true);
    expect_eq(c_zero.broadcast().eql(Ipv4Addr.parse("255.255.255.255")?), true);
    expect_eq(c_zero.hostCount(), 4294967296 as u64);

    const c_31 = Cidr4.parse("10.0.0.0/31")?;
    expect_eq(c_31.network().eql(Ipv4Addr.parse("10.0.0.0")?), true);
    expect_eq(c_31.broadcast().eql(Ipv4Addr.parse("10.0.0.1")?), true);
    expect_eq(c_31.hostCount(), 2 as u64);

    const c_32 = Cidr4.parse("8.8.8.8/32")?;
    expect_eq(c_32.network().eql(Ipv4Addr.parse("8.8.8.8")?), true);
    expect_eq(c_32.broadcast().eql(Ipv4Addr.parse("8.8.8.8")?), true);
    expect_eq(c_32.hostCount(), 1 as u64);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // 1. splitSubnets: /24 -> four /26 subnets
    const base = Cidr4.parse("192.168.10.0/24")?;
    const subs = base.splitSubnets(alloc, 26)?;
    defer alloc.free(subs);
    expect_eq(subs.len, 4 as usize);
    expect_eq(subs[0].eql(Cidr4.parse("192.168.10.0/26")?), true);
    expect_eq(subs[1].eql(Cidr4.parse("192.168.10.64/26")?), true);
    expect_eq(subs[2].eql(Cidr4.parse("192.168.10.128/26")?), true);
    expect_eq(subs[3].eql(Cidr4.parse("192.168.10.192/26")?), true);

    // 2. summarize: adjacent /25 siblings -> /24, plus pruning contained child subnets
    const inputs: [5]Cidr4 = [
        Cidr4.parse("10.1.0.128/25")?,
        Cidr4.parse("10.1.0.0/25")?,
        Cidr4.parse("10.2.0.0/16")?,
        Cidr4.parse("10.2.5.0/24")?,
        Cidr4.parse("10.2.5.128/25")?,
    ];
    const summarized = Cidr4.summarize(alloc, &inputs)?;
    defer alloc.free(summarized);
    expect_eq(summarized.len, 2 as usize);
    expect_eq(summarized[0].eql(Cidr4.parse("10.1.0.0/24")?), true);
    expect_eq(summarized[1].eql(Cidr4.parse("10.2.0.0/16")?), true);

    // 3. summarize cascading 4 /26 -> /24
    const reagg = Cidr4.summarize(alloc, subs)?;
    defer alloc.free(reagg);
    expect_eq(reagg.len, 1 as usize);
    expect_eq(reagg[0].eql(base), true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // 1. Ipv6Addr parsing: "::", "::1", trailing "::", mixed-case hex, full 8 hextets
    const unspec = Ipv6Addr.parse("::")?;
    const fmt_unspec = unspec.formatCanonical(alloc)?;
    defer alloc.free(fmt_unspec);
    expect_eq(std.mem.eql(u8, fmt_unspec, "::"), true);

    const lo6 = Ipv6Addr.parse("0000:0000:0000:0000:0000:0000:0000:0001")?;
    expect_eq(lo6.isLoopback(), true);
    expect_eq(lo6.isUniqueLocal(), false);
    const fmt_lo6 = lo6.formatCanonical(alloc)?;
    defer alloc.free(fmt_lo6);
    expect_eq(std.mem.eql(u8, fmt_lo6, "::1"), true);

    // RFC 5952: longest zero run (leftmost tie-break), single zero NOT compressed
    const tie_ip = Ipv6Addr.parse("2001:0DB8:0000:0000:0001:0000:0000:0001")?;
    const fmt_tie = tie_ip.formatCanonical(alloc)?;
    defer alloc.free(fmt_tie);
    expect_eq(std.mem.eql(u8, fmt_tie, "2001:db8::1:0:0:1"), true);

    const single_zero = Ipv6Addr.parse("2001:db8:0:1:1:1:1:1")?;
    const fmt_single = single_zero.formatCanonical(alloc)?;
    defer alloc.free(fmt_single);
    expect_eq(std.mem.eql(u8, fmt_single, "2001:db8:0:1:1:1:1:1"), true);

    const ula = Ipv6Addr.parse("fd12:3456:789a::1")?;
    expect_eq(ula.isUniqueLocal(), true);

    // 2. Reject double "::"
    let double_dc_err = false;
    * = Ipv6Addr.parse("2001::db8::1") catch e => err_dc: {
        double_dc_err = (e == CidrError.InvalidIpAddress);
        break :err_dc Ipv6Addr.fromU128(0);
    };
    expect_eq(double_dc_err, true);

    // 3. Cidr6 contains, overlaps, network & lastAddr
    const c6 = Cidr6.parse("2001:db8:1::42/48")?;
    expect_eq(c6.network().eql(Ipv6Addr.parse("2001:db8:1::")?), true);
    expect_eq(c6.lastAddr().eql(Ipv6Addr.parse("2001:db8:1:ffff:ffff:ffff:ffff:ffff")?), true);
    expect_eq(c6.contains(Ipv6Addr.parse("2001:db8:1:abcd::99")?), true);
    expect_eq(c6.contains(Ipv6Addr.parse("2001:db8:2::1")?), false);

    const c6_sub = Cidr6.parse("2001:db8:1:ff00::/56")?;
    const c6_other = Cidr6.parse("2001:db8:2::/48")?;
    expect_eq(c6.overlaps(c6_sub), true);
    expect_eq(c6.overlaps(c6_other), false);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;

    // 1. LpmTable4 multi-level longest prefix match (/0, /16, /24, /32)
    let rt = LpmTable4<u32>.init(alloc);
    defer rt.deinit();

    rt.insert(Cidr4.parse("0.0.0.0/0")?, 100)?;
    rt.insert(Cidr4.parse("10.20.0.0/16")?, 200)?;
    rt.insert(Cidr4.parse("10.20.30.0/24")?, 300)?;
    rt.insert(Cidr4.parse("10.20.30.40/32")?, 400)?;

    expect_eq(rt.lookup(Ipv4Addr.parse("10.20.30.40")?).?, 400 as u32);
    expect_eq(rt.lookup(Ipv4Addr.parse("10.20.30.99")?).?, 300 as u32);
    expect_eq(rt.lookup(Ipv4Addr.parse("10.20.99.1")?).?, 200 as u32);
    expect_eq(rt.lookup(Ipv4Addr.parse("8.8.4.4")?).?, 100 as u32);

    // 2. Malformed IP & out-of-range prefix interception
    let bad_ip = false;
    * = Ipv4Addr.parse("256.1.1.1") catch e => err_ip: {
        bad_ip = (e == CidrError.InvalidIpAddress);
        break :err_ip Ipv4Addr.fromU32(0);
    };
    expect_eq(bad_ip, true);

    let bad_prefix4 = false;
    * = Cidr4.parse("192.168.1.0/33") catch e => err_p4: {
        bad_prefix4 = (e == CidrError.InvalidPrefixLength);
        break :err_p4 Cidr4{ addr: Ipv4Addr.fromU32(0), prefix_len: 0 };
    };
    expect_eq(bad_prefix4, true);

    let bad_prefix6 = false;
    * = Cidr6.parse("2001:db8::/129") catch e => err_p6: {
        bad_prefix6 = (e == CidrError.InvalidPrefixLength);
        break :err_p6 Cidr6{ addr: Ipv6Addr.fromU128(0), prefix_len: 0 };
    };
    expect_eq(bad_prefix6, true);

    let strict_fail = false;
    * = Cidr4.parseStrict("192.168.1.5/24") catch e => err_st: {
        strict_fail = (e == CidrError.InvalidCidrNotation);
        break :err_st Cidr4{ addr: Ipv4Addr.fromU32(0), prefix_len: 0 };
    };
    expect_eq(strict_fail, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Reference-archived cidr_.cp direct verification with std.testing.allocator
    const c4 = Cidr4.parse("172.16.0.5/12")?;
    expect_eq(c4.network().isPrivate(), true);

    const subs = c4.splitSubnets(alloc, 14)?;
    defer alloc.free(subs);
    expect_eq(subs.len, 4 as usize);

    const agg = Cidr4.summarize(alloc, subs)?;
    defer alloc.free(agg);
    expect_eq(agg.len, 1 as usize);
    expect_eq(agg[0].eql(c4), true);

    const ip6 = Ipv6Addr.parse("fe80::1")?;
    const s6 = ip6.formatCanonical(alloc)?;
    defer alloc.free(s6);
    expect_eq(std.mem.eql(u8, s6, "fe80::1"), true);

    let lpm = LpmTable4<u16>.init(alloc);
    defer lpm.deinit();
    lpm.insert(c4, 42)?;
    expect_eq(lpm.lookup(Ipv4Addr.parse("172.20.1.1")?).?, 42 as u16);
    expect_eq(lpm.lookup(Ipv4Addr.parse("192.168.1.1")?) == null, true);
}

test("cap:cidr Ipv4Addr/Cidr4 parsing, normalization, RFC1918 & /0,/31,/32 boundaries") {
    runTest1() catch unreachable;
}

test("cap:cidr Cidr4.splitSubnets & summarize supernet aggregation") {
    runTest2() catch unreachable;
}

test("cap:cidr Ipv6Addr/Cidr6 :: compression, RFC5952 formatting & overlaps") {
    runTest3() catch unreachable;
}

test("cap:cidr LpmTable4 multi-level LPM routing & malformed input interception") {
    runTest4() catch unreachable;
}

test("cap.cidr dynamic mount & std.testing.allocator zero-leak verification") {
    runTest5() catch unreachable;
}
