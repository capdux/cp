// cap/test/testCapLdapDn.cp
// Comprehensive unit tests for @cap/ldap-dn (RFC 4514 LDAP DN)

import { test, expect_eq } from "cap:test";
import {
    RdnAttr,
    parseDn,
    escapeValue,
    unescapeValue,
    findAttr,
    isValidDn,
} from "../../packages/ldapjs/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/ldap-dn — standard AD/LDAP multi-RDN parsing with escaped comma", () => {
        const dn = "CN=John\\, Doe, OU=Engineering, DC=example, DC=com";
        let out: [8]RdnAttr = undefined;
        const n: usize = parseDn(dn, out[0..8]);
        expect_eq(n, 4 as usize);
        expect_eq(sliceEq(out[0].attr, "CN"), true);
        expect_eq(sliceEq(out[0].raw_value, "John\\, Doe"), true);
        expect_eq(sliceEq(out[1].attr, "OU"), true);
        expect_eq(sliceEq(out[1].raw_value, "Engineering"), true);
        expect_eq(sliceEq(out[2].attr, "DC"), true);
        expect_eq(sliceEq(out[2].raw_value, "example"), true);
        expect_eq(sliceEq(out[3].attr, "DC"), true);
        expect_eq(sliceEq(out[3].raw_value, "com"), true);
    });

    test("2. @cap/ldap-dn — escapeValue & unescapeValue with ',', '#', leading/trailing spaces & \\20", () => {
        let esc: [64]u8 = undefined;
        let dec_buf: [64]u8 = undefined;

        const orig = "#Admin, <Root> + \"Dev\" ";
        const en: usize = escapeValue(orig, esc[0..64]);
        expect_eq(en > orig.len, true);
        // Leading '#' and trailing ' ' must be escaped
        expect_eq(esc[0], '\\' as u8);
        expect_eq(esc[1], '#' as u8);
        expect_eq(esc[en - 2], '\\' as u8);
        expect_eq(esc[en - 1], ' ' as u8);

        const un: usize = unescapeValue(esc[0..en], dec_buf[0..64]);
        expect_eq(sliceEq(dec_buf[0..un], orig), true);

        // Hex escape \20 -> space, \2C -> comma
        const hn: usize = unescapeValue("John\\2C\\20Doe", dec_buf[0..64]);
        expect_eq(sliceEq(dec_buf[0..hn], "John, Doe"), true);
    });

    test("3. @cap/ldap-dn — findAttr case-insensitive lookup with automatic unescape", () => {
        const dn = "CN=John\\, Doe,OU=Engineering,DC=example,DC=com";
        let val: [64]u8 = undefined;
        let vlen: usize = 0;

        expect_eq(findAttr(dn, "cn", val[0..64], &vlen), true);
        expect_eq(sliceEq(val[0..vlen], "John, Doe"), true);

        expect_eq(findAttr(dn, "ou", val[0..64], &vlen), true);
        expect_eq(sliceEq(val[0..vlen], "Engineering"), true);

        expect_eq(findAttr(dn, "mail", val[0..64], &vlen), false);
    });

    test("4. @cap/ldap-dn — multiple DC domain components extraction", () => {
        const dn = "UID=alice,DC=corp,DC=example,DC=org";
        let out: [8]RdnAttr = undefined;
        const n: usize = parseDn(dn, out[0..8]);
        expect_eq(n, 4 as usize);
        expect_eq(sliceEq(out[1].raw_value, "corp"), true);
        expect_eq(sliceEq(out[2].raw_value, "example"), true);
        expect_eq(sliceEq(out[3].raw_value, "org"), true);
    });

    test("5. @cap/ldap-dn — isValidDn accepts valid & rejects malformed DNs", () => {
        expect_eq(isValidDn("CN=Alice,DC=example,DC=com"), true);
        expect_eq(isValidDn("2.5.4.3=OIDUser"), true);
        expect_eq(isValidDn(""), false);
        expect_eq(isValidDn("NoEqualsSign"), false);
        expect_eq(isValidDn("=MissingAttr"), false);
        expect_eq(isValidDn("CN=Alice,"), false);
        expect_eq(isValidDn("CN=Dangling\\"), false);
    });

    test("6. @cap/ldap-dn — tiny buffer protection", () => {
        let tiny: [4]u8 = undefined;
        expect_eq(escapeValue("John, Doe", tiny[0..4]), 0 as usize);
        expect_eq(unescapeValue("John\\, Doe", tiny[0..4]), 0 as usize);

        let vlen: usize = 0;
        expect_eq(findAttr("CN=VeryLongName", "cn", tiny[0..4], &vlen), false);
    });
}
