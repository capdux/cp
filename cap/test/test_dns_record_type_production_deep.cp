import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DnsRrType,
    DnsRcode,
    DnsClass,
    RrSrvRecord as SrvRecord,
    MxRecord,
} from "cap:dns";

test("1. DnsRrType round-trips u16 codes and case-insensitive names", () => {
    const t_a = DnsRrType.fromU16(1) ?? unreachable;
    expect_eq(t_a == DnsRrType.a, true);
    expect_eq(t_a.toU16(), u16(1));
    expect_eq(std.mem.eql(u8, t_a.toName(), "A"), true);

    const t_aaaa = DnsRrType.parseName("aaaa") ?? unreachable;
    expect_eq(t_aaaa == DnsRrType.aaaa, true);
    expect_eq(t_aaaa.toU16(), u16(28));
    expect_eq(std.mem.eql(u8, t_aaaa.toName(), "AAAA"), true);

    const t_https = DnsRrType.parseName("  Https ") ?? unreachable;
    expect_eq(t_https == DnsRrType.https, true);
    expect_eq(t_https.toU16(), u16(65));

    const t_caa = DnsRrType.fromU16(257) ?? unreachable;
    expect_eq(t_caa == DnsRrType.caa, true);
    expect_eq(std.mem.eql(u8, t_caa.toName(), "CAA"), true);

    const t_any = DnsRrType.parseName("*") ?? unreachable;
    expect_eq(t_any == DnsRrType.any_q, true);
    expect_eq(t_any.toU16(), u16(255));
    expect_eq(std.mem.eql(u8, t_any.toName(), "ANY"), true);

    expect_eq(DnsRrType.fromU16(999) == null, true);
    expect_eq(DnsRrType.parseName("UNKNOWN_TYPE") == null, true);
});

test("2. DnsRcode decodes standard IANA codes, names, and isError status", () => {
    const ok = DnsRcode.fromU16(0) ?? unreachable;
    expect_eq(ok == DnsRcode.noerror, true);
    expect_eq(ok.isError(), false);
    expect_eq(std.mem.eql(u8, ok.toName(), "NOERROR"), true);

    const nx = DnsRcode.fromU16(3) ?? unreachable;
    expect_eq(nx == DnsRcode.nxdomain, true);
    expect_eq(nx.isError(), true);
    expect_eq(std.mem.eql(u8, nx.toName(), "NXDOMAIN"), true);

    const refused = DnsRcode.fromU16(5) ?? unreachable;
    expect_eq(refused == DnsRcode.refused, true);
    expect_eq(refused.isError(), true);
    expect_eq(std.mem.eql(u8, refused.toName(), "REFUSED"), true);

    const nz = DnsRcode.fromU16(10) ?? unreachable;
    expect_eq(nz == DnsRcode.notzone, true);
    expect_eq(nz.toU16(), u16(10));
    expect_eq(nz.isError(), true);

    // Unassigned / unsupported codes return null
    expect_eq(DnsRcode.fromU16(7) == null, true);
    expect_eq(DnsRcode.fromU16(99) == null, true);
});

test("3. DnsClass decodes u16 codes and case-insensitive class names", () => {
    const c_in = DnsClass.fromU16(1) ?? unreachable;
    expect_eq(c_in == DnsClass.in_class, true);
    expect_eq(c_in.toU16(), u16(1));
    expect_eq(std.mem.eql(u8, c_in.toName(), "IN"), true);

    const c_ch = DnsClass.parseName("chaos") ?? unreachable;
    expect_eq(c_ch == DnsClass.ch, true);
    expect_eq(c_ch.toU16(), u16(3));

    const c_hs = DnsClass.parseName("HS") ?? unreachable;
    expect_eq(c_hs == DnsClass.hs, true);
    expect_eq(c_hs.toU16(), u16(4));

    const c_any = DnsClass.parseName("any") ?? unreachable;
    expect_eq(c_any == DnsClass.any_class, true);
    expect_eq(c_any.toU16(), u16(255));

    expect_eq(DnsClass.fromU16(2) == null, true);
    expect_eq(DnsClass.parseName("INVALID") == null, true);
});

test("4. SrvRecord.parseText parses valid SRV rdata including root target and rejects malformed inputs", () => {
    const s1 = SrvRecord.parseText("10 60 5060 sipserver.example.com.") ?? unreachable;
    expect_eq(s1.priority, u16(10));
    expect_eq(s1.weight, u16(60));
    expect_eq(s1.port, u16(5060));
    expect_eq(std.mem.eql(u8, s1.target, "sipserver.example.com."), true);

    // Service unavailable root target "." with tabs/spaces
    const s2 = SrvRecord.parseText("  0\t0  0  .  ") ?? unreachable;
    expect_eq(s2.priority, u16(0));
    expect_eq(s2.weight, u16(0));
    expect_eq(s2.port, u16(0));
    expect_eq(std.mem.eql(u8, s2.target, "."), true);

    // Malformed SRV inputs
    expect_eq(SrvRecord.parseText("10 60 5060") == null, true);
    expect_eq(SrvRecord.parseText("10 60 65536 host.example.com.") == null, true);
    expect_eq(SrvRecord.parseText("10 60 5060 host.example.com. extra") == null, true);
    expect_eq(SrvRecord.parseText("10 60 5060 bad..domain.") == null, true);
    expect_eq(SrvRecord.parseText("-1 60 5060 host.example.com.") == null, true);
});

test("5. MxRecord.parseText parses standard and RFC 7505 Null MX records and rejects malformed inputs", () => {
    const m1 = MxRecord.parseText("10 mail.example.com.") ?? unreachable;
    expect_eq(m1.preference, u16(10));
    expect_eq(std.mem.eql(u8, m1.exchange, "mail.example.com."), true);

    // RFC 7505 Null MX
    const m2 = MxRecord.parseText("0 .") ?? unreachable;
    expect_eq(m2.preference, u16(0));
    expect_eq(std.mem.eql(u8, m2.exchange, "."), true);

    // Malformed MX inputs
    expect_eq(MxRecord.parseText("") == null, true);
    expect_eq(MxRecord.parseText("10") == null, true);
    expect_eq(MxRecord.parseText("70000 mail.example.com.") == null, true);
    expect_eq(MxRecord.parseText("10 mail.example.com. extra") == null, true);
    expect_eq(MxRecord.parseText("abc mail.example.com.") == null, true);
});
