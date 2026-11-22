// Source: cp/cap/test/test_ldap_production_deep.cp
// Deep production test suite for pure CP `cap:ldap` & `cap.ldap` dynamic mount.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    LdapFilter,
    LdapAttr,
    DnComponent,
    LdapError,
    parseFilter,
    parseFilterWithMaxDepth,
    formatFilter,
    escapeFilterValue,
    matchesEntry,
    parseDn,
    freeDn,
} from "cap:net";
import cap from "cap";

fn expectTrue(cond: bool): ! {
    expect_eq(cond, true);
}

fn expectStrEq(actual: []const u8, expected: []const u8): ! {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn expectFilterErr(alloc: std.mem.Allocator, expr: []const u8, expected: anyerror): ! {
    let f = parseFilter(alloc, expr) catch e => {
        expect_eq(e, expected);
        return;
    };
    f.deinit(alloc);
    expect_eq(true, false);
}

fn expectDnErr(alloc: std.mem.Allocator, dn: []const u8, expected: anyerror): ! {
    const comps = parseDn(alloc, dn) catch e => {
        expect_eq(e, expected);
        return;
    };
    freeDn(alloc, comps);
    expect_eq(true, false);
}

test("1. Compound boolean filter tree parsing & formatFilter canonical roundtrip") {
    const alloc = std.testing.allocator;
    const expr = "(&(|(a=1)(b=*x*y))(!(c>=10))(mail=*))";

    let f1 = parseFilter(alloc, expr) catch unreachable;
    defer f1.deinit(alloc);

    const formatted1 = formatFilter(alloc, f1) catch unreachable;
    defer alloc.free(formatted1);
    expectStrEq(formatted1, expr) catch unreachable;

    let f2 = parseFilter(alloc, formatted1) catch unreachable;
    defer f2.deinit(alloc);

    const formatted2 = formatFilter(alloc, f2) catch unreachable;
    defer alloc.free(formatted2);
    expectStrEq(formatted2, expr) catch unreachable;
}

test("2. matchesEntry case-insensitive multi-value, substring & escaped literal match") {
    const alloc = std.testing.allocator;

    const cn_vals = [ "Alice Smith", "asmith" ];
    const mail_vals = [ "alice.dev@cp-lang.org" ];
    const age_vals = [ "9" ];
    const secret_vals = [ "a*b(c)\\d" ];
    const alice_attrs = [
        LdapAttr{ name: "CN", values: &cn_vals },
        LdapAttr{ name: "Mail", values: &mail_vals },
        LdapAttr{ name: "Age", values: &age_vals },
        LdapAttr{ name: "Secret", values: &secret_vals },
    ];

    // Substring initial + any + final (case-insensitive)
    let f_sub = parseFilter(alloc, "(mail=ALICE*DEV*ORG)") catch unreachable;
    defer f_sub.deinit(alloc);
    expectTrue(matchesEntry(f_sub, &alice_attrs)) catch unreachable;

    let f_sub_miss = parseFilter(alloc, "(mail=ALICE*ZZZ*ORG)") catch unreachable;
    defer f_sub_miss.deinit(alloc);
    expect_eq(matchesEntry(f_sub_miss, &alice_attrs), false);

    // Escaped literal \2a (*), \28 ((), \29 ()), \5c (\)
    let f_esc = parseFilter(alloc, "(secret=a\\2ab\\28c\\29\\5cd)") catch unreachable;
    defer f_esc.deinit(alloc);
    expectTrue(matchesEntry(f_esc, &alice_attrs)) catch unreachable;

    // Compound boolean + numeric ge/le + present
    let f_comp = parseFilter(alloc, "(&(|(cn=bob)(cn=ASMITH))(!(age>=10))(age<=9)(mail=*))") catch unreachable;
    defer f_comp.deinit(alloc);
    expectTrue(matchesEntry(f_comp, &alice_attrs)) catch unreachable;
}

test("3. escapeFilterValue injection defense & RFC 4514 parseDn with escaped commas/specials") {
    const alloc = std.testing.allocator;

    const raw_injection = "admin)(|(uid=*\\\x00)";
    const escaped = escapeFilterValue(alloc, raw_injection) catch unreachable;
    defer alloc.free(escaped);
    expectStrEq(escaped, "admin\\29\\28|\\28uid=\\2a\\5c\\00\\29") catch unreachable;

    // Ensure escaped value parses safely as a single literal eq filter
    let filter_expr_buf: std.ArrayList(u8) = .empty;
    defer filter_expr_buf.deinit(alloc);
    filter_expr_buf.appendSlice(alloc, "(uid=") catch unreachable;
    filter_expr_buf.appendSlice(alloc, escaped) catch unreachable;
    filter_expr_buf.append(alloc, ')') catch unreachable;

    let f_safe = parseFilter(alloc, filter_expr_buf.items) catch unreachable;
    defer f_safe.deinit(alloc);
    const safe_vals = [ raw_injection ];
    const safe_attrs = [
        LdapAttr{ name: "uid", values: &safe_vals },
    ];
    expectTrue(matchesEntry(f_safe, &safe_attrs)) catch unreachable;

    // RFC 4514 DN with escaped comma, hex space \20, and multiple RDNs
    const dn_comps = parseDn(alloc, "cn=Alice\\, Bob\\20Jr., ou=Core\\+Dev, dc=cp, dc=org") catch unreachable;
    defer freeDn(alloc, dn_comps);
    expect_eq(dn_comps.len, 4);
    expectStrEq(dn_comps[0].rdn_attr, "cn") catch unreachable;
    expectStrEq(dn_comps[0].rdn_value, "Alice, Bob Jr.") catch unreachable;
    expectStrEq(dn_comps[1].rdn_attr, "ou") catch unreachable;
    expectStrEq(dn_comps[1].rdn_value, "Core+Dev") catch unreachable;
    expectStrEq(dn_comps[2].rdn_attr, "dc") catch unreachable;
    expectStrEq(dn_comps[2].rdn_value, "cp") catch unreachable;
    expectStrEq(dn_comps[3].rdn_attr, "dc") catch unreachable;
    expectStrEq(dn_comps[3].rdn_value, "org") catch unreachable;
}

test("4. Malformed inputs: UnbalancedParens, InvalidEscape, MaxDepthExceeded(65), InvalidDn") {
    const alloc = std.testing.allocator;

    expectFilterErr(alloc, "(&(a=1)(b=2)", LdapError.UnbalancedParens) catch unreachable;
    expectFilterErr(alloc, "(a=1))", LdapError.UnbalancedParens) catch unreachable;
    expectFilterErr(alloc, "(cn=foo\\zzbar)", LdapError.InvalidEscape) catch unreachable;
    expectFilterErr(alloc, "(cn=foo\\)", LdapError.InvalidEscape) catch unreachable;

    // 65-level deep nesting should trigger MaxDepthExceeded (default max_depth = 64)
    let deep_buf: std.ArrayList(u8) = .empty;
    defer deep_buf.deinit(alloc);
    let d: usize = 0;
    while (d < 64) : (d += 1) {
        deep_buf.appendSlice(alloc, "(!") catch unreachable;
    }
    deep_buf.appendSlice(alloc, "(a=1)") catch unreachable;
    d = 0;
    while (d < 64) : (d += 1) {
        deep_buf.append(alloc, ')') catch unreachable;
    }
    expectFilterErr(alloc, deep_buf.items, LdapError.MaxDepthExceeded) catch unreachable;

    // 64-level deep nesting (63 '(!' + 1 leaf) should succeed
    let ok_buf: std.ArrayList(u8) = .empty;
    defer ok_buf.deinit(alloc);
    d = 0;
    while (d < 63) : (d += 1) {
        ok_buf.appendSlice(alloc, "(!") catch unreachable;
    }
    ok_buf.appendSlice(alloc, "(a=1)") catch unreachable;
    d = 0;
    while (d < 63) : (d += 1) {
        ok_buf.append(alloc, ')') catch unreachable;
    }
    let f_ok = parseFilterWithMaxDepth(alloc, ok_buf.items, 64) catch unreachable;
    f_ok.deinit(alloc);

    // Malformed DNs
    expectDnErr(alloc, "cn=Alice,", LdapError.InvalidDn) catch unreachable;
    expectDnErr(alloc, "invalid_rdn,dc=org", LdapError.InvalidDn) catch unreachable;
    expectDnErr(alloc, "cn=,dc=org", LdapError.InvalidDn) catch unreachable;
    expectDnErr(alloc, "cn=Alice\\q,dc=org", LdapError.InvalidDn) catch unreachable;
}

test("5. cap.ldap dynamic mount & std.testing.allocator zero memory leak") {
    const alloc = std.testing.allocator;

    let f = cap.net.parseFilter(alloc, "(|(uid=root)(uid=admin*))") catch unreachable;
    defer f.deinit(alloc);

    const s = cap.net.formatFilter(alloc, f) catch unreachable;
    defer alloc.free(s);
    expectStrEq(s, "(|(uid=root)(uid=admin*))") catch unreachable;

    const dn = cap.net.parseDn(alloc, "uid=admin,dc=example,dc=com") catch unreachable;
    defer cap.net.freeDn(alloc, dn);
    expect_eq(dn.len, 3);
    expectStrEq(dn[0].rdn_value, "admin") catch unreachable;
}
