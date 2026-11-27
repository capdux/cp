// cap/test/testCapCsp.cp
// Comprehensive unit tests for @cap/csp

import { test, expect_eq } from "cap:test";
import {
    CspBuilder,
    CspPolicy,
} from "../../packages/content-security-policy-builder/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/csp — CspBuilder builds strict policy including valueless directive", () => {
        let buf: [256]u8 = undefined;
        let b = CspBuilder.init(buf[0..256]);

        let defSrc: [1][]const u8 = undefined;
        defSrc[0] = "'self'";
        expect_eq(b.addDirective("default-src", defSrc[0..1]), true);

        let scriptSrc: [2][]const u8 = undefined;
        scriptSrc[0] = "'self'";
        scriptSrc[1] = "'nonce-abc123'";
        expect_eq(b.addDirective("script-src", scriptSrc[0..2]), true);

        let frameSrc: [1][]const u8 = undefined;
        frameSrc[0] = "'none'";
        expect_eq(b.addDirective("frame-ancestors", frameSrc[0..1]), true);

        let noSrc: [1][]const u8 = undefined;
        noSrc[0] = "";
        expect_eq(b.addDirective("upgrade-insecure-requests", noSrc[0..0]), true);

        expect_eq(b.isOverflow(), false);
        const expected = "default-src 'self'; script-src 'self' 'nonce-abc123'; frame-ancestors 'none'; upgrade-insecure-requests";
        expect_eq(b.len(), expected.len);
        expect_eq(sliceEq(b.written(), expected), true);
    });

    test("2. @cap/csp — CspPolicy.parse & case-insensitive getDirective/hasDirective", () => {
        const hdr = "  DEFAULT-SRC 'self'; Script-Src 'self' https://cdn.example.com ; img-src data: https:; upgrade-insecure-requests ; ";
        const p = CspPolicy.parse(hdr);
        expect_eq(p.len(), 4 as usize);
        expect_eq(p.hasDirective(hdr, "default-src"), true);
        expect_eq(p.hasDirective(hdr, "SCRIPT-SRC"), true);
        expect_eq(p.hasDirective(hdr, "upgrade-insecure-requests"), true);
        expect_eq(p.hasDirective(hdr, "object-src"), false);

        expect_eq(sliceEq(p.getDirective(hdr, "default-src"), "'self'"), true);
        expect_eq(sliceEq(p.getDirective(hdr, "script-src"), "'self' https://cdn.example.com"), true);
        expect_eq(sliceEq(p.getDirective(hdr, "IMG-SRC"), "data: https:"), true);
        expect_eq(p.getDirective(hdr, "upgrade-insecure-requests").len, 0 as usize);
        expect_eq(p.getDirective(hdr, "missing-src").len, 0 as usize);
    });

    test("3. @cap/csp — Duplicate directives follow W3C first-occurrence precedence", () => {
        const hdr = "default-src 'self'; script-src 'none'; DEFAULT-SRC *; script-src 'unsafe-inline'";
        const p = CspPolicy.parse(hdr);
        expect_eq(p.len(), 2 as usize);
        expect_eq(sliceEq(p.getDirective(hdr, "default-src"), "'self'"), true);
        expect_eq(sliceEq(p.getDirective(hdr, "script-src"), "'none'"), true);
        expect_eq(p.allowsUnsafeInline(hdr, "script-src"), false);
    });

    test("4. @cap/csp — allowsUnsafeInline exact whole-token detection", () => {
        const hdr = "script-src 'self' 'UNSAFE-INLINE' https://a.test; style-src 'self' 'unsafe-inline-custom'; img-src *";
        const p = CspPolicy.parse(hdr);
        expect_eq(p.allowsUnsafeInline(hdr, "script-src"), true);
        expect_eq(p.allowsUnsafeInline(hdr, "style-src"), false);
        expect_eq(p.allowsUnsafeInline(hdr, "img-src"), false);
        expect_eq(p.allowsUnsafeInline(hdr, "connect-src"), false);
    });

    test("5. @cap/csp — Rejects ';' and CRLF header injection characters", () => {
        let buf: [128]u8 = undefined;
        let b = CspBuilder.init(buf[0..128]);

        let goodSrc: [1][]const u8 = undefined;
        goodSrc[0] = "'self'";
        expect_eq(b.addDirective("default-src", goodSrc[0..1]), true);

        let semiSrc: [1][]const u8 = undefined;
        semiSrc[0] = "'self'; script-src *";
        expect_eq(b.addDirective("img-src", semiSrc[0..1]), false);

        let crlfSrc: [1][]const u8 = undefined;
        crlfSrc[0] = "https://a.test\r\nX-Injected: 1";
        expect_eq(b.addDirective("connect-src", crlfSrc[0..1]), false);

        expect_eq(b.addDirective("bad;name", goodSrc[0..1]), false);
        expect_eq(b.addDirective("bad\nname", goodSrc[0..1]), false);
        expect_eq(b.isOverflow(), false);
        expect_eq(sliceEq(b.written(), "default-src 'self'"), true);
    });

    test("6. @cap/csp — Empty header parsing & small buffer overflow protection", () => {
        const emptyPolicy = CspPolicy.parse("   ; ;  ");
        expect_eq(emptyPolicy.len(), 0 as usize);
        expect_eq(emptyPolicy.hasDirective("   ; ;  ", "default-src"), false);

        let tiny: [16]u8 = undefined;
        let b = CspBuilder.init(tiny[0..16]);
        let src1: [1][]const u8 = undefined;
        src1[0] = "'self'";
        expect_eq(b.addDirective("img-src", src1[0..1]), true);
        expect_eq(b.isOverflow(), false);

        // Next directive does not fit in remaining bytes.
        expect_eq(b.addDirective("script-src", src1[0..1]), false);
        expect_eq(b.isOverflow(), true);
        expect_eq(sliceEq(b.written(), "img-src 'self'"), true);
    });
}
