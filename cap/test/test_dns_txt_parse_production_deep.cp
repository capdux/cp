import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TxtTagIter,
    findTxtTag,
    DmarcPolicy,
    DmarcRecord,
    SpfQualifier,
    SpfIter,
} from "cap:dns_txt_parse";

test("1. TxtTagIter and findTxtTag parse semicolon-delimited DKIM/BIMI TXT records", () => {
    const dkim = "v=DKIM1; k=rsa; t=s; p=MIGfMA0GCSqGSIb3DQEBAQUAA4==; ; malformed_no_eq;";
    let it = TxtTagIter.init(dkim);

    const t0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t0.key, "v"), true);
    expect_eq(std.mem.eql(u8, t0.value, "DKIM1"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t1.key, "k"), true);
    expect_eq(std.mem.eql(u8, t1.value, "rsa"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t2.key, "t"), true);
    expect_eq(std.mem.eql(u8, t2.value, "s"), true);

    const t3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t3.key, "p"), true);
    expect_eq(std.mem.eql(u8, t3.value, "MIGfMA0GCSqGSIb3DQEBAQUAA4=="), true);

    expect_eq(it.next() == null, true);

    // Case-insensitive lookup
    expect_eq(std.mem.eql(u8, findTxtTag(dkim, "K") ?? unreachable, "rsa"), true);
    expect_eq(std.mem.eql(u8, findTxtTag(dkim, "P") ?? unreachable, "MIGfMA0GCSqGSIb3DQEBAQUAA4=="), true);
    expect_eq(findTxtTag(dkim, "missing") == null, true);
    expect_eq(findTxtTag(dkim, "") == null, true);
});

test("2. DmarcRecord.parse parses full and minimal DMARC policies with default pct=100", () => {
    const full =
        "v=DMARC1; p=reject; sp=quarantine; pct=25; rua=mailto:agg@example.com; ruf=mailto:forensic@example.com; adkim=s";
    const d1 = DmarcRecord.parse(full) ?? unreachable;
    expect_eq(d1.policy == DmarcPolicy.reject, true);
    expect_eq((d1.subdomain_policy ?? unreachable) == DmarcPolicy.quarantine, true);
    expect_eq(d1.pct, u8(25));
    expect_eq(std.mem.eql(u8, d1.rua ?? unreachable, "mailto:agg@example.com"), true);
    expect_eq(std.mem.eql(u8, d1.ruf ?? unreachable, "mailto:forensic@example.com"), true);

    const minimal = "v=DMARC1; p=none";
    const d2 = DmarcRecord.parse(minimal) ?? unreachable;
    expect_eq(d2.policy == DmarcPolicy.none, true);
    expect_eq(d2.subdomain_policy == null, true);
    expect_eq(d2.pct, u8(100));
    expect_eq(d2.rua == null, true);
    expect_eq(d2.ruf == null, true);
});

test("3. DmarcRecord.parse rejects missing v=DMARC1, missing p=, invalid policy, and pct > 100", () => {
    expect_eq(DmarcRecord.parse("p=reject; v=DMARC1") == null, true);
    expect_eq(DmarcRecord.parse("v=DMARC2; p=reject") == null, true);
    expect_eq(DmarcRecord.parse("v=DMARC1; rua=mailto:a@b.com") == null, true);
    expect_eq(DmarcRecord.parse("v=DMARC1; p=block") == null, true);
    expect_eq(DmarcRecord.parse("v=DMARC1; p=reject; sp=unknown") == null, true);
    expect_eq(DmarcRecord.parse("v=DMARC1; p=reject; pct=101") == null, true);
    expect_eq(DmarcRecord.parse("v=DMARC1; p=reject; pct=abc") == null, true);
});

test("4. SpfIter validates v=spf1 and iterates qualifiers, mechanisms, and modifiers", () => {
    const spf_txt = "v=spf1 +mx include:_spf.google.com ip4:192.0.2.0/24 ?ptr redirect=spf.example.org -all";
    let it = SpfIter.init(spf_txt);
    expect_eq(it.isValid(), true);

    const s0 = it.next() ?? unreachable;
    expect_eq(s0.qualifier == SpfQualifier.pass, true);
    expect_eq(std.mem.eql(u8, s0.mechanism, "mx"), true);
    expect_eq(s0.value == null, true);

    const s1 = it.next() ?? unreachable;
    expect_eq(s1.qualifier == SpfQualifier.pass, true);
    expect_eq(std.mem.eql(u8, s1.mechanism, "include"), true);
    expect_eq(std.mem.eql(u8, s1.value ?? unreachable, "_spf.google.com"), true);

    const s2 = it.next() ?? unreachable;
    expect_eq(s2.qualifier == SpfQualifier.pass, true);
    expect_eq(std.mem.eql(u8, s2.mechanism, "ip4"), true);
    expect_eq(std.mem.eql(u8, s2.value ?? unreachable, "192.0.2.0/24"), true);

    const s3 = it.next() ?? unreachable;
    expect_eq(s3.qualifier == SpfQualifier.neutral, true);
    expect_eq(std.mem.eql(u8, s3.mechanism, "ptr"), true);
    expect_eq(s3.value == null, true);

    const s4 = it.next() ?? unreachable;
    expect_eq(s4.qualifier == SpfQualifier.pass, true);
    expect_eq(std.mem.eql(u8, s4.mechanism, "redirect"), true);
    expect_eq(std.mem.eql(u8, s4.value ?? unreachable, "spf.example.org"), true);

    const s5 = it.next() ?? unreachable;
    expect_eq(s5.qualifier == SpfQualifier.fail, true);
    expect_eq(std.mem.eql(u8, s5.mechanism, "all"), true);
    expect_eq(s5.value == null, true);

    expect_eq(it.next() == null, true);
});

test("5. SpfIter handles softfail ~all, case-insensitive V=SPF1, and rejects invalid SPF prefixes", () => {
    let it_ok = SpfIter.init("  V=SPF1   ip6:2001:db8::/32   ~all ");
    expect_eq(it_ok.isValid(), true);

    const t0 = it_ok.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, t0.mechanism, "ip6"), true);
    expect_eq(std.mem.eql(u8, t0.value ?? unreachable, "2001:db8::/32"), true);

    const t1 = it_ok.next() ?? unreachable;
    expect_eq(t1.qualifier == SpfQualifier.softfail, true);
    expect_eq(std.mem.eql(u8, t1.mechanism, "all"), true);
    expect_eq(it_ok.next() == null, true);

    // Invalid prefix yields isValid() == false and empty iterator
    let bad1 = SpfIter.init("v=spf2.0/pra -all");
    expect_eq(bad1.isValid(), false);
    expect_eq(bad1.next() == null, true);

    let bad2 = SpfIter.init("google-site-verification=xyz");
    expect_eq(bad2.isValid(), false);
    expect_eq(bad2.next() == null, true);
});
