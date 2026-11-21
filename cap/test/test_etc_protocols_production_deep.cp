import {
    ProtocolEntry,
    EtcProtocolsIter,
    findProtocolNumber,
    findProtocolByNumber,
} from "cap:sys";
import { test, expect, expect_eq } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

const SAMPLE_PROTOCOLS =
    "# Internet (IP) protocols\n" ++
    "\n" ++
    "   # indented comment\r\n" ++
    "ip      0       IP              # internet protocol, pseudo protocol number\n" ++
    "icmp    1       ICMP            # internet control message protocol\r\n" ++
    "tcp     6       TCP             # transmission control protocol\n" ++
    "broken_no_num\n" ++
    "broken_neg      -1      NEG\n" ++
    "broken_over     65536   OVER\n" ++
    "broken_alpha    17x     BAD\n" ++
    "udp     17      UDP             # user datagram protocol\n" ++
    "ipv6-icmp 58    IPv6-ICMP icmpv6 # ICMP for IPv6\n" ++
    "sctp    132     SCTP\n" ++
    "maxproto 65535  MAXPROTO";

test("EtcProtocolsIter streams valid entries and skips comments, blanks, and malformed lines", () => {
    let it = EtcProtocolsIter.init(SAMPLE_PROTOCOLS);

    const p0 = it.next().?;
    expect(strEq(p0.name, "ip"));
    expect_eq(p0.number, 0);
    expect(strEq(p0.aliases, "IP"));
    expect_eq(p0.aliasCount(), 1);

    const p1 = it.next().?;
    expect(strEq(p1.name, "icmp"));
    expect_eq(p1.number, 1);

    const p2 = it.next().?;
    expect(strEq(p2.name, "tcp"));
    expect_eq(p2.number, 6);

    const p3 = it.next().?;
    expect(strEq(p3.name, "udp"));
    expect_eq(p3.number, 17);

    const p4 = it.next().?;
    expect(strEq(p4.name, "ipv6-icmp"));
    expect_eq(p4.number, 58);
    expect(strEq(p4.aliases, "IPv6-ICMP icmpv6"));
    expect_eq(p4.aliasCount(), 2);

    const p5 = it.next().?;
    expect(strEq(p5.name, "sctp"));
    expect_eq(p5.number, 132);

    const p6 = it.next().?;
    expect(strEq(p6.name, "maxproto"));
    expect_eq(p6.number, 65535);

    expect(it.next() == null);
    expect(it.next() == null);
});

test("ProtocolEntry.hasNameOrAlias is case-insensitive and avoids substring false positives", () => {
    const v6 = findProtocolByNumber(SAMPLE_PROTOCOLS, 58).?;
    expect(v6.hasNameOrAlias("ipv6-icmp"));
    expect(v6.hasNameOrAlias("IPV6-ICMP"));
    expect(v6.hasNameOrAlias("IPv6-ICMP"));
    expect(v6.hasNameOrAlias("icmpv6"));
    expect(v6.hasNameOrAlias("ICMPV6"));

    // Substrings and empty queries must not match
    expect(!v6.hasNameOrAlias("icmp"));
    expect(!v6.hasNameOrAlias("ipv6"));
    expect(!v6.hasNameOrAlias("IPv6-ICMP icmpv6"));
    expect(!v6.hasNameOrAlias(""));
});

test("findProtocolNumber resolves primary names and aliases case-insensitively", () => {
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "ip").?, 0);
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "IP").?, 0);
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "icmp").?, 1);
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "TCP").?, 6);
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "Udp").?, 17);
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "ICMPv6").?, 58);
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "sctp").?, 132);
    expect_eq(findProtocolNumber(SAMPLE_PROTOCOLS, "maxproto").?, 65535);

    expect(findProtocolNumber(SAMPLE_PROTOCOLS, "unknown") == null);
    expect(findProtocolNumber(SAMPLE_PROTOCOLS, "") == null);
});

test("findProtocolByNumber performs reverse lookups including protocol 0 and 65535", () => {
    const ip = findProtocolByNumber(SAMPLE_PROTOCOLS, 0).?;
    expect(strEq(ip.name, "ip"));

    const tcp = findProtocolByNumber(SAMPLE_PROTOCOLS, 6).?;
    expect(strEq(tcp.name, "tcp"));

    const maxp = findProtocolByNumber(SAMPLE_PROTOCOLS, 65535).?;
    expect(strEq(maxp.name, "maxproto"));

    expect(findProtocolByNumber(SAMPLE_PROTOCOLS, 999) == null);
});

test("Malformed line rejection, inline comment stripping, and u16 overflow protection", () => {
    // Blank and comment-only lines
    expect(ProtocolEntry.parseLine("") == null);
    expect(ProtocolEntry.parseLine("   \t  \r") == null);
    expect(ProtocolEntry.parseLine("# tcp 6 TCP") == null);
    expect(ProtocolEntry.parseLine("  # comment") == null);

    // Missing number, negative, non-digit, overflow > 65535
    expect(ProtocolEntry.parseLine("tcp") == null);
    expect(ProtocolEntry.parseLine("tcp # 6") == null);
    expect(ProtocolEntry.parseLine("tcp -6 TCP") == null);
    expect(ProtocolEntry.parseLine("tcp 6a TCP") == null);
    expect(ProtocolEntry.parseLine("tcp 65536 TCP") == null);
    expect(ProtocolEntry.parseLine("tcp 99999999 TCP") == null);

    // No aliases and inline `#` without separating space
    const no_alias = ProtocolEntry.parseLine("\tgre\t47#Generic Routing Encapsulation\r").?;
    expect(strEq(no_alias.name, "gre"));
    expect_eq(no_alias.number, 47);
    expect_eq(no_alias.aliases.len, 0);
    expect_eq(no_alias.aliasCount(), 0);

    // Empty content edge cases
    let empty_it = EtcProtocolsIter.init("");
    expect(empty_it.next() == null);
    expect(findProtocolNumber("", "tcp") == null);
    expect(findProtocolByNumber("", 6) == null);
});
