// Origin: Pure-CP Stdlib Test Suite (lib/net/blockList.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Node/Bun `net.BlockList` (IPv4/IPv6):
//   1. Single-address rules (IPv4/IPv6 canonical equivalence, LIFO prepend order, checkIp)
//   2. Inclusive range rules (IPv4 & 128-bit IPv6 bounds, reversed-range & cross-family rejection)
//   3. CIDR subnet rules (/0, /24, /32, /64, /128, addSubnetCidr, invalid prefix rejection)
//   4. IPv4-mapped IPv6 (`::ffff:a.b.c.d`) bidirectional interop across Address/Range/Subnet
//   5. Node/Bun-compatible rule formatting (`Address:`, `Range:`, `Subnet:`, `formatAllRules`)
//   6. Malformed input resilience, rule removal, mergeFrom, clear & capacity bound guard

import { test, expect_eq, expect_true } from "cap:test";
import { IpAddr } from "../../lib/net/ipaddr.cp";
import {
    BLOCKLIST_MAX_RULES,
    BlockListFamily,
    BlockListRuleKind,
    BlockListError,
    BlockList,
} from "../../lib/net/blockList.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: Single-address rules, canonical IPv6 equality, LIFO order & checkIp
// ============================================================================
test("blockList: single-address IPv4/IPv6 rules, LIFO ordering & checkIp", () => {
    let bl = BlockList.init();
    expect_true(bl.isEmpty());
    expect_eq(bl.len(), 0);
    expect_eq(bl.capacity(), BLOCKLIST_MAX_RULES);

    bl.addAddress("127.0.0.1", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addAddress("2001:0db8:0000:0000:0000:0000:0000:0001", BlockListFamily.Ipv6) catch {
        expect_true(false);
    };
    bl.addAddressAuto("10.20.30.40") catch {
        expect_true(false);
    };

    expect_eq(bl.len(), 3);
    expect_true(bl.check("127.0.0.1", BlockListFamily.Ipv4));
    expect_true(!bl.check("127.0.0.2", BlockListFamily.Ipv4));
    expect_true(bl.check("2001:db8::1", BlockListFamily.Ipv6));
    expect_true(!bl.check("2001:db8::2", BlockListFamily.Ipv6));
    expect_true(bl.checkAuto("10.20.30.40"));
    expect_true(bl.checkAuto("2001:db8::1"));

    // Verify checkIp with pre-parsed IpAddr
    if (ip := IpAddr.parse("2001:db8::1")) {
        expect_true(bl.checkIp(ip));
    } else {
        expect_true(false);
    }

    // Verify LIFO prepend order: latest added rule ("10.20.30.40") is at index 0
    let buf: [128]u8 = [0; 128];
    if (r0 := bl.formatRule(0, buf[0..])) {
        expect_true(strEq(r0, "Address: IPv4 10.20.30.40"));
    } else {
        expect_true(false);
    }

    // Family parser case-insensitivity
    expect_true(BlockList.parseFamily("ipv4") == BlockListFamily.Ipv4);
    expect_true(BlockList.parseFamily("IPv4") == BlockListFamily.Ipv4);
    expect_true(BlockList.parseFamily("IPV6") == BlockListFamily.Ipv6);
    expect_true(BlockList.parseFamily("ipv5") == null);
});

// ============================================================================
// Test 2: Inclusive IPv4/IPv6 ranges & invalid range / cross-family guards
// ============================================================================
test("blockList: inclusive IPv4/IPv6 ranges & boundary validation", () => {
    let bl = BlockList.init();

    bl.addRange("10.0.0.1", "10.0.0.10", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addRange("2001:db8::10", "2001:db8::20", BlockListFamily.Ipv6) catch {
        expect_true(false);
    };

    // IPv4 boundary checks
    expect_true(!bl.check("10.0.0.0", BlockListFamily.Ipv4));
    expect_true(bl.check("10.0.0.1", BlockListFamily.Ipv4));
    expect_true(bl.check("10.0.0.5", BlockListFamily.Ipv4));
    expect_true(bl.check("10.0.0.10", BlockListFamily.Ipv4));
    expect_true(!bl.check("10.0.0.11", BlockListFamily.Ipv4));

    // IPv6 boundary checks
    expect_true(!bl.check("2001:db8::f", BlockListFamily.Ipv6));
    expect_true(bl.check("2001:db8::10", BlockListFamily.Ipv6));
    expect_true(bl.check("2001:db8::1a", BlockListFamily.Ipv6));
    expect_true(bl.check("2001:db8::20", BlockListFamily.Ipv6));
    expect_true(!bl.check("2001:db8::21", BlockListFamily.Ipv6));

    // Reversed range must be rejected
    let rejected_rev = false;
    bl.addRange("10.0.0.20", "10.0.0.10", BlockListFamily.Ipv4) catch {
        rejected_rev = true;
    };
    expect_true(rejected_rev);

    // Cross-family auto range must be rejected
    let rejected_cross = false;
    bl.addRangeAuto("10.0.0.1", "2001:db8::1") catch {
        rejected_cross = true;
    };
    expect_true(rejected_cross);
});

// ============================================================================
// Test 3: CIDR subnet matching (/0, /24, /32, /64, /128) & prefix guards
// ============================================================================
test("blockList: CIDR subnet matching & prefix overflow protection", () => {
    let bl = BlockList.init();

    bl.addSubnet("192.168.1.0", 24, BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addSubnet("172.16.5.9", 32, BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addSubnetCidr("fe80::/10") catch {
        expect_true(false);
    };
    bl.addSubnetCidr("2001:db8:abcd::1/128") catch {
        expect_true(false);
    };

    expect_true(bl.check("192.168.1.0", BlockListFamily.Ipv4));
    expect_true(bl.check("192.168.1.255", BlockListFamily.Ipv4));
    expect_true(!bl.check("192.168.2.1", BlockListFamily.Ipv4));
    expect_true(bl.check("172.16.5.9", BlockListFamily.Ipv4));
    expect_true(!bl.check("172.16.5.10", BlockListFamily.Ipv4));

    expect_true(bl.check("fe80::1", BlockListFamily.Ipv6));
    expect_true(bl.check("febf:ffff::1", BlockListFamily.Ipv6));
    expect_true(!bl.check("fec0::1", BlockListFamily.Ipv6));
    expect_true(bl.check("2001:db8:abcd::1", BlockListFamily.Ipv6));
    expect_true(!bl.check("2001:db8:abcd::2", BlockListFamily.Ipv6));

    // Invalid prefixes (/33 for IPv4, /129 for IPv6, malformed CIDR)
    let bad_v4_prefix = false;
    bl.addSubnet("10.0.0.0", 33, BlockListFamily.Ipv4) catch {
        bad_v4_prefix = true;
    };
    expect_true(bad_v4_prefix);

    let bad_v6_prefix = false;
    bl.addSubnet("2001:db8::", 129, BlockListFamily.Ipv6) catch {
        bad_v6_prefix = true;
    };
    expect_true(bad_v6_prefix);

    let bad_cidr = false;
    bl.addSubnetCidr("192.168.1.0") catch {
        bad_cidr = true;
    };
    expect_true(bad_cidr);

    // /0 prefix must match all addresses of that family without shift overflow
    let all_v4 = BlockList.init();
    all_v4.addSubnet("0.0.0.0", 0, BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    expect_true(all_v4.check("8.8.8.8", BlockListFamily.Ipv4));
    expect_true(all_v4.check("255.255.255.255", BlockListFamily.Ipv4));
    expect_true(!all_v4.check("2001:db8::1", BlockListFamily.Ipv6));
});

// ============================================================================
// Test 4: IPv4-mapped IPv6 (`::ffff:a.b.c.d`) bidirectional interop
// ============================================================================
test("blockList: IPv4-mapped IPv6 bidirectional interop", () => {
    let bl = BlockList.init();
    bl.addAddress("192.168.10.5", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addRange("10.1.0.1", "10.1.0.50", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addSubnet("172.20.0.0", 16, BlockListFamily.Ipv4) catch {
        expect_true(false);
    };

    // Querying IPv4-mapped IPv6 against IPv4 rules
    expect_true(bl.check("::ffff:192.168.10.5", BlockListFamily.Ipv6));
    expect_true(!bl.check("::ffff:192.168.10.6", BlockListFamily.Ipv6));
    expect_true(bl.check("::ffff:10.1.0.25", BlockListFamily.Ipv6));
    expect_true(bl.check("::ffff:172.20.99.1", BlockListFamily.Ipv6));
    expect_true(!bl.check("::ffff:172.21.0.1", BlockListFamily.Ipv6));

    // Reverse direction: IPv6 mapped subnet rule matching native IPv4 query
    let bl2 = BlockList.init();
    bl2.addSubnet("::ffff:198.51.100.0", 120, BlockListFamily.Ipv6) catch {
        expect_true(false);
    };
    expect_true(bl2.check("198.51.100.42", BlockListFamily.Ipv4));
    expect_true(!bl2.check("198.51.101.42", BlockListFamily.Ipv4));

    // Helper roundtrip
    const mapped = BlockList.ipv4ToMappedV6(0x7F000001);
    if (back := BlockList.mappedV6ToIpv4(mapped)) {
        expect_eq(back, 0x7F000001);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 5: Node/Bun-compatible rule formatting & small-buffer guard
// ============================================================================
test("blockList: Node/Bun rule formatting and formatAllRules", () => {
    let bl = BlockList.init();
    bl.addAddress("127.0.0.1", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addRange("10.0.0.1", "10.0.0.9", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.addSubnet("2001:db8::", 32, BlockListFamily.Ipv6) catch {
        expect_true(false);
    };

    let buf: [256]u8 = [0; 256];
    if (n := bl.formatAllRules(buf[0..])) {
        const expected = "Subnet: IPv6 2001:db8::/32\nRange: IPv4 10.0.0.1-10.0.0.9\nAddress: IPv4 127.0.0.1";
        expect_true(strEq(buf[0..n], expected));
    } else {
        expect_true(false);
    }

    // Small buffer must safely return null instead of overflowing
    let tiny: [8]u8 = [0; 8];
    expect_true(bl.formatRule(0, tiny[0..]) == null);
    expect_true(bl.formatAllRules(tiny[0..]) == null);
});

// ============================================================================
// Test 6: Malformed inputs, removeAt, mergeFrom, clear & capacity limit
// ============================================================================
test("blockList: malformed input guard, removeAt, mergeFrom & capacity bound", () => {
    let bl = BlockList.init();
    bl.addAddress("1.2.3.4", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };

    // Malformed addresses return false in check (never panic)
    expect_true(!bl.check("not-an-ip", BlockListFamily.Ipv4));
    expect_true(!bl.check("999.0.0.1", BlockListFamily.Ipv4));
    expect_true(!bl.check("1.2.3.4", BlockListFamily.Ipv6));
    expect_true(!bl.checkAuto(""));

    // Invalid address rejected by addAddress
    let bad_addr = false;
    bl.addAddress("256.1.1.1", BlockListFamily.Ipv4) catch {
        bad_addr = true;
    };
    expect_true(bad_addr);

    // mergeFrom & removeAt
    let other = BlockList.init();
    other.addAddress("5.6.7.8", BlockListFamily.Ipv4) catch {
        expect_true(false);
    };
    bl.mergeFrom(other) catch {
        expect_true(false);
    };
    expect_eq(bl.len(), 2);
    expect_true(bl.check("5.6.7.8", BlockListFamily.Ipv4));

    expect_true(bl.removeAt(0));
    expect_eq(bl.len(), 1);
    expect_true(!bl.check("1.2.3.4", BlockListFamily.Ipv4));
    expect_true(bl.check("5.6.7.8", BlockListFamily.Ipv4));
    expect_true(!bl.removeAt(99));

    bl.clear();
    expect_true(bl.isEmpty());

    // Capacity bound enforcement
    let full = BlockList.init();
    let idx: usize = 0;
    while (idx < BLOCKLIST_MAX_RULES) {
        full.addAddress("127.0.0.1", BlockListFamily.Ipv4) catch {
            expect_true(false);
        };
        idx += 1;
    }
    expect_eq(full.len(), BLOCKLIST_MAX_RULES);

    let overflowed = false;
    full.addAddress("127.0.0.2", BlockListFamily.Ipv4) catch {
        overflowed = true;
    };
    expect_true(overflowed);
});
