// Source: cp/cap/test/test_vcard_production_deep.cp
// Deep production test suite for pure CP `cap:vcard` & `cap.vcard` dynamic mount.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    VCardError,
    VCardVersion,
    StructuredName,
    TypedValue,
    VCardAddress,
    VCardContact,
    VCardCodec,
} from "cap:vcard";
import cap from "cap";

fn expectStrEq(actual: string, expected: string): ! {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn expectParseErr(alloc: std.mem.Allocator, vcf_text: string, expected_err: string): ! {
    let list = VCardCodec.parseAll(alloc, vcf_text) catch e => {
        expectStrEq(@errorName(e), expected_err)?;
        return;
    };
    VCardCodec.freeContacts(alloc, list);
    expect_eq(true, false);
}

test("1. Parse Full vCard 3.0 & 4.0 (Line Unfolding, Escapes, Group Prefix, TYPE, N, ADR)") {
    const alloc = std.testing.allocator;
    const vcf =
        "BEGIN:VCARD\r\n" ++
        "VERSION:3.0\r\n" ++
        "UID:urn:uuid:1111-2222\r\n" ++
        "FN:Dr. Alice\\, M. Smith\r\n" ++
        "N:Smith\\;Jr.;Alice;Marie;Dr.;PhD\r\n" ++
        "ORG:Acme Corp\\, R&D\\; AI\r\n" ++
        "TITLE:Principal Architect\r\n" ++
        "item1.TEL;TYPE=CELL,WORK:+1-555-0101\r\n" ++
        "TEL;TYPE=\"HOME,VOICE\":+1-555-0102\r\n" ++
        "EMAIL;TYPE=WORK;TYPE=PREF:alice@acme.dev\r\n" ++
        "URL;TYPE=HOME:https://alice.example.com\r\n" ++
        "ADR;TYPE=WORK:;;123 Main St\\, Suite 4\\; Floor 2;San\r\n" ++
        "  Francisco;CA;94105;USA\r\n" ++
        "NOTE:Line 1\\nLine 2\\\\Path\r\n" ++
        "\t continued\r\n" ++
        "END:VCARD\r\n";

    let c = VCardCodec.parseOne(alloc, vcf) catch unreachable;
    defer c.deinit();

    expect_eq(c.version == .V3_0, true);
    expectStrEq(c.uid, "urn:uuid:1111-2222") catch unreachable;
    expectStrEq(c.fn_name, "Dr. Alice, M. Smith") catch unreachable;
    expectStrEq(c.org, "Acme Corp, R&D; AI") catch unreachable;
    expectStrEq(c.title, "Principal Architect") catch unreachable;
    expectStrEq(c.note, "Line 1\nLine 2\\Path continued") catch unreachable;

    let sn = c.name.?;
    expectStrEq(sn.family, "Smith;Jr.") catch unreachable;
    expectStrEq(sn.given, "Alice") catch unreachable;
    expectStrEq(sn.additional, "Marie") catch unreachable;
    expectStrEq(sn.prefix, "Dr.") catch unreachable;
    expectStrEq(sn.suffix, "PhD") catch unreachable;

    expect_eq(c.phones.len, 2);
    expect_eq(c.phones[0].types.len, 2);
    expectStrEq(c.phones[0].types[0], "CELL") catch unreachable;
    expectStrEq(c.phones[0].types[1], "WORK") catch unreachable;
    expectStrEq(c.phones[0].value, "+1-555-0101") catch unreachable;

    expect_eq(c.phones[1].types.len, 2);
    expectStrEq(c.phones[1].types[0], "HOME") catch unreachable;
    expectStrEq(c.phones[1].types[1], "VOICE") catch unreachable;
    expectStrEq(c.phones[1].value, "+1-555-0102") catch unreachable;

    expect_eq(c.emails.len, 1);
    expect_eq(c.emails[0].types.len, 2);
    expectStrEq(c.emails[0].types[0], "WORK") catch unreachable;
    expectStrEq(c.emails[0].types[1], "PREF") catch unreachable;
    expectStrEq(c.emails[0].value, "alice@acme.dev") catch unreachable;

    expect_eq(c.urls.len, 1);
    expectStrEq(c.urls[0].value, "https://alice.example.com") catch unreachable;

    expect_eq(c.addresses.len, 1);
    expect_eq(c.addresses[0].types.len, 1);
    expectStrEq(c.addresses[0].types[0], "WORK") catch unreachable;
    expectStrEq(c.addresses[0].street, "123 Main St, Suite 4; Floor 2") catch unreachable;
    expectStrEq(c.addresses[0].city, "San Francisco") catch unreachable;
    expectStrEq(c.addresses[0].region, "CA") catch unreachable;
    expectStrEq(c.addresses[0].postal_code, "94105") catch unreachable;
    expectStrEq(c.addresses[0].country, "USA") catch unreachable;
}

test("2. Batch Multi-Contact .vcf Address Book parseAll Verification") {
    const alloc = std.testing.allocator;
    const batch_vcf =
        "BEGIN:VCARD\n" ++
        "VERSION:4.0\n" ++
        "UID:c-001\n" ++
        "FN:Bob Builder\n" ++
        "N:Builder;Bob;;;\n" ++
        "TEL;TYPE=CELL:+1-555-1001\n" ++
        "END:VCARD\n\n" ++
        "BEGIN:VCARD\r\n" ++
        "VERSION:3.0\r\n" ++
        "UID:c-002\r\n" ++
        "FN:Carol Danvers\r\n" ++
        "EMAIL;TYPE=WORK:carol@starforce.org\r\n" ++
        "END:VCARD\r\n";

    let contacts = VCardCodec.parseAll(alloc, batch_vcf) catch unreachable;
    defer VCardCodec.freeContacts(alloc, contacts);

    expect_eq(contacts.len, 2);
    expect_eq(contacts[0].version == .V4_0, true);
    expectStrEq(contacts[0].fn_name, "Bob Builder") catch unreachable;
    expectStrEq(contacts[0].phones[0].value, "+1-555-1001") catch unreachable;

    expect_eq(contacts[1].version == .V3_0, true);
    expectStrEq(contacts[1].fn_name, "Carol Danvers") catch unreachable;
    expectStrEq(contacts[1].emails[0].value, "carol@starforce.org") catch unreachable;
}

test("3. toVcf Generation (with 75-byte Folding & Escaping) -> parseAll Lossless Round-Trip") {
    const alloc = std.testing.allocator;
    let phone_types: [2]string = ["CELL", "WORK"];
    let email_types: [1]string = ["WORK"];
    let addr_types: [1]string = ["HOME"];

    let phones: [1]TypedValue = [TypedValue{ types: phone_types[0..], value: "+86-138-0013-8000" }];
    let emails: [1]TypedValue = [TypedValue{ types: email_types[0..], value: "linus@kernel.org" }];
    let addrs: [1]VCardAddress = [VCardAddress{
        types: addr_types[0..],
        street: "742 Evergreen Terrace, Apt; 3B",
        city: "Portland",
        region: "OR",
        postal_code: "97201",
        country: "USA",
    }];

    let src: [1]VCardContact = [VCardContact{
        version: .V4_0,
        uid: "rt-uid-999",
        fn_name: "Linus Torvalds, Jr.",
        name: StructuredName{
            family: "Torvalds",
            given: "Linus",
            additional: "Benedict",
            prefix: "Mr.",
            suffix: "Jr.",
        },
        org: "Linux Foundation; Core",
        title: "Creator",
        phones: phones[0..],
        emails: emails[0..],
        addresses: addrs[0..],
        note: "Very long note designed to exceed seventy five octets so RFC 6350 line folding triggers cleanly.\nSecond line with \\backslash, comma; semicolon.",
    }];

    let emitted = VCardCodec.toVcf(alloc, src[0..]) catch unreachable;
    defer alloc.free(emitted);

    // Verify 75-byte folding CRLF + space continuation is present
    expect_eq(std.mem.indexOf(u8, emitted, "\r\n ") != null, true);

    let parsed = VCardCodec.parseAll(alloc, emitted) catch unreachable;
    defer VCardCodec.freeContacts(alloc, parsed);

    expect_eq(parsed.len, 1);
    expect_eq(parsed[0].version == .V4_0, true);
    expectStrEq(parsed[0].uid, src[0].uid) catch unreachable;
    expectStrEq(parsed[0].fn_name, src[0].fn_name) catch unreachable;
    expectStrEq(parsed[0].org, src[0].org) catch unreachable;
    expectStrEq(parsed[0].title, src[0].title) catch unreachable;
    expectStrEq(parsed[0].note, src[0].note) catch unreachable;
    expectStrEq(parsed[0].name.?.family, "Torvalds") catch unreachable;
    expectStrEq(parsed[0].name.?.given, "Linus") catch unreachable;
    expectStrEq(parsed[0].phones[0].value, "+86-138-0013-8000") catch unreachable;
    expectStrEq(parsed[0].emails[0].value, "linus@kernel.org") catch unreachable;
    expectStrEq(parsed[0].addresses[0].street, "742 Evergreen Terrace, Apt; 3B") catch unreachable;
    expectStrEq(parsed[0].addresses[0].city, "Portland") catch unreachable;
}

test("4. Malformed Input Interception (MissingFn, UnbalancedCard, UnsupportedVersion) & Zero-Leak") {
    const alloc = std.testing.allocator;

    // Missing FN
    expectParseErr(alloc, "BEGIN:VCARD\r\nVERSION:4.0\r\nUID:no-fn\r\nEND:VCARD\r\n", "MissingFn") catch unreachable;

    // Unclosed BEGIN:VCARD (after allocating partial fields to verify zero leak)
    expectParseErr(alloc, "BEGIN:VCARD\r\nVERSION:4.0\r\nFN:Partial\r\nTEL;TYPE=CELL:123\r\n", "UnbalancedCard") catch unreachable;

    // Stray END:VCARD without BEGIN
    expectParseErr(alloc, "END:VCARD\r\n", "UnbalancedCard") catch unreachable;

    // Nested BEGIN:VCARD
    expectParseErr(alloc, "BEGIN:VCARD\r\nFN:A\r\nBEGIN:VCARD\r\nEND:VCARD\r\n", "UnbalancedCard") catch unreachable;

    // Unsupported version (2.1)
    expectParseErr(alloc, "BEGIN:VCARD\r\nVERSION:2.1\r\nFN:Old\r\nEND:VCARD\r\n", "UnsupportedVersion") catch unreachable;
}

test("5. cap.vcard Dynamic Namespace Mount Verification") {
    const alloc = std.testing.allocator;
    const vcf =
        "BEGIN:VCARD\r\n" ++
        "VERSION:4.0\r\n" ++
        "FN:Grace Hopper\r\n" ++
        "N:Hopper;Grace;;Rear Admiral;\r\n" ++
        "EMAIL;TYPE=WORK:grace@navy.mil\r\n" ++
        "END:VCARD\r\n";

    let c: VCardContact = cap.vcard.VCardCodec.parseOne(alloc, vcf) catch unreachable;
    defer c.deinit();

    expectStrEq(c.fn_name, "Grace Hopper") catch unreachable;
    expectStrEq(c.name.?.prefix, "Rear Admiral") catch unreachable;
    expectStrEq(c.emails[0].value, "grace@navy.mil") catch unreachable;

    let arr: [1]VCardContact = [c];
    let out = cap.vcard.VCardCodec.toVcf(alloc, arr[0..]) catch unreachable;
    defer alloc.free(out);
    expect_eq(std.mem.indexOf(u8, out, "FN:Grace Hopper") != null, true);
}
