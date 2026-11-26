// cap/test/testCapVcard.cp
// Comprehensive unit tests for @cap/vcard

import { test, expect_eq } from "cap:test";
import {
    VCardData,
    formatVCard,
    parseVCard,
} from "../../packages/@cap/vcard/src/index.cp";

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

fn containsSub(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (haystack.len < needle.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (sliceEq(haystack[i..(i + needle.len)], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/vcard — Full vCard 4.0 formatting with CRLF line endings", () => {
        const card = VCardData{
            full_name: "Ada Lovelace",
            org: "Analytical Engine Labs",
            title: "Lead Architect",
            email: "ada@example.com",
            tel: "+1-555-0100",
            url: "https://example.com/ada",
        };
        let buf: [512]u8 = undefined;
        const n = formatVCard(card, buf[0..512]);
        expect_eq(n > (0 as i64), true);

        const expected = "BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Ada Lovelace\r\nORG:Analytical Engine Labs\r\nTITLE:Lead Architect\r\nEMAIL:ada@example.com\r\nTEL:+1-555-0100\r\nURL:https://example.com/ada\r\nEND:VCARD\r\n";
        expect_eq(sliceEq(buf[0..(n as usize)], expected), true);
    });

    test("2. @cap/vcard — Parameterized TEL and EMAIL property parsing", () => {
        const vcf = "BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Grace Hopper\r\nORG:US Navy\r\nTITLE:Rear Admiral\r\nTEL;TYPE=work,voice:+1-555-0199\r\nEMAIL;TYPE=internet:grace@navy.example\r\nURL:https://example.com/grace\r\nEND:VCARD\r\n";
        const p = parseVCard(vcf);
        expect_eq(p.valid, true);
        expect_eq(sliceEq(vcf[p.version_start..(p.version_start + p.version_len)], "4.0"), true);
        expect_eq(sliceEq(vcf[p.fn_start..(p.fn_start + p.fn_len)], "Grace Hopper"), true);
        expect_eq(sliceEq(vcf[p.org_start..(p.org_start + p.org_len)], "US Navy"), true);
        expect_eq(sliceEq(vcf[p.title_start..(p.title_start + p.title_len)], "Rear Admiral"), true);
        expect_eq(sliceEq(vcf[p.tel_start..(p.tel_start + p.tel_len)], "+1-555-0199"), true);
        expect_eq(sliceEq(vcf[p.email_start..(p.email_start + p.email_len)], "grace@navy.example"), true);
        expect_eq(sliceEq(vcf[p.url_start..(p.url_start + p.url_len)], "https://example.com/grace"), true);
    });

    test("3. @cap/vcard — formatVCard -> parseVCard full round-trip", () => {
        const card = VCardData{
            full_name: "Linus Torvalds",
            org: "Linux Foundation",
            title: "Creator",
            email: "torvalds@example.org",
            tel: "+1-555-0142",
            url: "https://kernel.org",
        };
        let buf: [512]u8 = undefined;
        const n = formatVCard(card, buf[0..512]);
        expect_eq(n > (0 as i64), true);

        const slice = buf[0..(n as usize)];
        const p = parseVCard(slice);
        expect_eq(p.valid, true);
        expect_eq(sliceEq(slice[p.version_start..(p.version_start + p.version_len)], "4.0"), true);
        expect_eq(sliceEq(slice[p.fn_start..(p.fn_start + p.fn_len)], "Linus Torvalds"), true);
        expect_eq(sliceEq(slice[p.org_start..(p.org_start + p.org_len)], "Linux Foundation"), true);
        expect_eq(sliceEq(slice[p.title_start..(p.title_start + p.title_len)], "Creator"), true);
        expect_eq(sliceEq(slice[p.email_start..(p.email_start + p.email_len)], "torvalds@example.org"), true);
        expect_eq(sliceEq(slice[p.tel_start..(p.tel_start + p.tel_len)], "+1-555-0142"), true);
        expect_eq(sliceEq(slice[p.url_start..(p.url_start + p.url_len)], "https://kernel.org"), true);
    });

    test("4. @cap/vcard — Empty optional fields are omitted", () => {
        const card = VCardData{
            full_name: "Minimal User",
            org: "",
            title: "",
            email: "min@example.com",
            tel: "",
            url: "",
        };
        let buf: [256]u8 = undefined;
        const n = formatVCard(card, buf[0..256]);
        expect_eq(n > (0 as i64), true);
        const out = buf[0..(n as usize)];
        expect_eq(containsSub(out, "ORG:"), false);
        expect_eq(containsSub(out, "TITLE:"), false);
        expect_eq(containsSub(out, "TEL:"), false);
        expect_eq(containsSub(out, "URL:"), false);
        expect_eq(containsSub(out, "EMAIL:min@example.com\r\n"), true);
    });

    test("5. @cap/vcard — Escaping special characters ',', ';', '\\', and '\\n'", () => {
        const card = VCardData{
            full_name: "Doe, Jane; PhD",
            org: "R&D\\Core\nDivision",
            title: "",
            email: "",
            tel: "",
            url: "",
        };
        let buf: [256]u8 = undefined;
        const n = formatVCard(card, buf[0..256]);
        expect_eq(n > (0 as i64), true);
        const out = buf[0..(n as usize)];
        expect_eq(containsSub(out, "FN:Doe\\, Jane\\; PhD\r\n"), true);
        expect_eq(containsSub(out, "ORG:R&D\\\\Core\\nDivision\r\n"), true);
    });

    test("6. @cap/vcard — Empty full_name, missing BEGIN:VCARD, and small buffer fail safely", () => {
        const emptyName = VCardData{
            full_name: "",
            org: "Org",
            title: "",
            email: "",
            tel: "",
            url: "",
        };
        let buf: [128]u8 = undefined;
        expect_eq(formatVCard(emptyName, buf[0..128]), -1 as i64);

        const okCard = VCardData{
            full_name: "Alice",
            org: "",
            title: "",
            email: "",
            tel: "",
            url: "",
        };
        let tiny: [16]u8 = undefined;
        expect_eq(formatVCard(okCard, tiny[0..16]), -1 as i64);

        const invalidVcf = parseVCard("VERSION:4.0\r\nFN:No Begin\r\nEND:VCARD\r\n");
        expect_eq(invalidVcf.valid, false);
    });
}
