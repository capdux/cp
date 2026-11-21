import std from "std";
import { test, expect_eq } from "cap:test";
import { PermissionsPolicyIter, findPermission } from "cap:http_permissions_policy";

test("1. PermissionsPolicyIter parses (), *, self, and multi-origin allowlists", () => {
    const hdr = "camera=(), microphone=*, geolocation=(self \"https://maps.example.com\"), fullscreen=self";
    let it = PermissionsPolicyIter.init(hdr);

    const d0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d0.feature, "camera"), true);
    expect_eq(d0.isDisabled(), true);
    expect_eq(d0.allowsAll(), false);
    expect_eq(d0.allowsSelf(), false);
    expect_eq(d0.allowsOrigin("https://example.com"), false);

    const d1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d1.feature, "microphone"), true);
    expect_eq(d1.is_wildcard, true);
    expect_eq(d1.isDisabled(), false);
    expect_eq(d1.allowsAll(), true);
    expect_eq(d1.allowsSelf(), true);
    expect_eq(d1.allowsOrigin("https://any.origin"), true);

    const d2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d2.feature, "geolocation"), true);
    expect_eq(d2.isDisabled(), false);
    expect_eq(d2.allowsAll(), false);
    expect_eq(d2.allowsSelf(), true);
    expect_eq(d2.allowsOrigin("https://maps.example.com"), true);
    expect_eq(d2.allowsOrigin("https://evil.example.com"), false);

    const d3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, d3.feature, "fullscreen"), true);
    expect_eq(d3.allowsSelf(), true);
    expect_eq(d3.allowsAll(), false);

    expect_eq(it.next() == null, true);
});

test("2. wildcard inside parentheses (*)", () => {
    const hdr = "autoplay=(*)";
    const d = findPermission(hdr, "autoplay") ?? unreachable;
    expect_eq(d.isDisabled(), false);
    expect_eq(d.allowsAll(), true);
    expect_eq(d.allowsSelf(), true);
    expect_eq(d.allowsOrigin("https://cdn.example.org"), true);
});

test("3. findPermission case-insensitive lookup and quoted comma safety", () => {
    const hdr = "payment=(\"https://pay.example.com\" \"https://a,b.example\"), USB=()";
    const pay = findPermission(hdr, "PAYMENT") ?? unreachable;
    expect_eq(pay.allowsOrigin("https://pay.example.com"), true);
    expect_eq(pay.allowsOrigin("https://a,b.example"), true);
    expect_eq(pay.allowsSelf(), false);

    const usb = findPermission(hdr, "usb") ?? unreachable;
    expect_eq(usb.isDisabled(), true);

    expect_eq(findPermission(hdr, "midi") == null, true);
});

test("4. anti-false-positive: quoted \"self\" or \"*\" does not grant bare self/wildcard", () => {
    const hdr = "interest-cohort=(\"self\" \"*\")";
    const d = findPermission(hdr, "interest-cohort") ?? unreachable;
    expect_eq(d.allowsAll(), false);
    expect_eq(d.allowsSelf(), false);
    expect_eq(d.allowsOrigin("self"), true);
});

test("5. malformed directives and empty header resilience", () => {
    expect_eq(findPermission("", "camera") == null, true);
    expect_eq(findPermission("broken_no_equals, unclosed=(self, valid=()", "valid") != null, true);
    const v = findPermission("broken_no_equals, bad feature=(), valid=(self)", "valid") ?? unreachable;
    expect_eq(v.allowsSelf(), true);
});
