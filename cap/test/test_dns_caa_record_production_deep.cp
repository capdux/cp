import std from "std";
import { test, expect_eq } from "cap:test";
import { CaaTag, CaaRecord, isCaAuthorized } from "cap:dns_caa_record";

test("1. CaaRecord.parseText parses issue, issuewild, iodef, flags, params, and forbid-all", () => {
    const r1 = CaaRecord.parseText("0 issue \"letsencrypt.org; accounturi=https://acme.example/1\"") ?? unreachable;
    expect_eq(r1.flags, u8(0));
    expect_eq(r1.isCritical(), false);
    expect_eq(r1.tag == CaaTag.issue, true);
    expect_eq(std.mem.eql(u8, r1.tag_raw, "issue"), true);
    expect_eq(std.mem.eql(u8, r1.issuerDomain(), "letsencrypt.org"), true);
    expect_eq(r1.forbidsAllIssuance(), false);

    const r2 = CaaRecord.parseText("128 issuewild \";\"") ?? unreachable;
    expect_eq(r2.flags, u8(128));
    expect_eq(r2.isCritical(), true);
    expect_eq(r2.tag == CaaTag.issuewild, true);
    expect_eq(r2.forbidsAllIssuance(), true);
    expect_eq(r2.issuerDomain().len, usize(0));

    const r3 = CaaRecord.parseText("0 IODEF \"mailto:sec@example.com\"") ?? unreachable;
    expect_eq(r3.tag == CaaTag.iodef, true);
    expect_eq(std.mem.eql(u8, r3.value, "mailto:sec@example.com"), true);

    // Unquoted single-token value
    const r4 = CaaRecord.parseText("0 issue digicert.com") ?? unreachable;
    expect_eq(std.mem.eql(u8, r4.issuerDomain(), "digicert.com"), true);
});

test("2. CaaRecord.parseWire decodes RFC 8659 §4.1 binary wire format", () => {
    // [flags=0x80][tag_len=5]["issue"]["ca.example.net"]
    const wire: [21]u8 = [
        0x80, 5,
        'i', 's', 's', 'u', 'e',
        'c', 'a', '.', 'e', 'x', 'a', 'm', 'p', 'l', 'e', '.', 'n', 'e', 't',
    ];
    const rec = CaaRecord.parseWire(wire[0..21]) ?? unreachable;
    expect_eq(rec.flags, u8(0x80));
    expect_eq(rec.isCritical(), true);
    expect_eq(rec.tag == CaaTag.issue, true);
    expect_eq(std.mem.eql(u8, rec.value, "ca.example.net"), true);

    // Empty value in wire format is valid (forbids issuance)
    const wire_empty: [7]u8 = [0x00, 5, 'i', 's', 's', 'u', 'e'];
    const rec_empty = CaaRecord.parseWire(wire_empty[0..7]) ?? unreachable;
    expect_eq(rec_empty.forbidsAllIssuance(), true);

    // Truncated / zero tag_len / invalid tag bytes rejected
    const bad1: [1]u8 = [0x00];
    expect_eq(CaaRecord.parseWire(bad1[0..1]) == null, true);
    const bad2: [2]u8 = [0x00, 0];
    expect_eq(CaaRecord.parseWire(bad2[0..2]) == null, true);
    const bad3: [4]u8 = [0x00, 5, 'a', 'b'];
    expect_eq(CaaRecord.parseWire(bad3[0..4]) == null, true);
    const bad4: [4]u8 = [0x00, 2, 'a', '-'];
    expect_eq(CaaRecord.parseWire(bad4[0..4]) == null, true);
});

test("3. isCaAuthorized enforces RFC 8659 issue vs issuewild precedence and fallback", () => {
    const r_issue = CaaRecord.parseText("0 issue \"letsencrypt.org\"") ?? unreachable;
    const r_wild = CaaRecord.parseText("0 issuewild \"digicert.com\"") ?? unreachable;
    const set: [2]CaaRecord = [r_issue, r_wild];

    // Non-wildcard checks only `issue`
    expect_eq(isCaAuthorized(set[0..2], "letsencrypt.org", false), true);
    expect_eq(isCaAuthorized(set[0..2], "LETSENCRYPT.ORG.", false), true);
    expect_eq(isCaAuthorized(set[0..2], "digicert.com", false), false);

    // Wildcard prefers `issuewild` when present
    expect_eq(isCaAuthorized(set[0..2], "digicert.com", true), true);
    expect_eq(isCaAuthorized(set[0..2], "letsencrypt.org", true), false);

    // Wildcard falls back to `issue` when no `issuewild` exists
    const issue_only: [1]CaaRecord = [r_issue];
    expect_eq(isCaAuthorized(issue_only[0..1], "letsencrypt.org", true), true);
    expect_eq(isCaAuthorized(issue_only[0..1], "other.ca", true), false);
});

test("4. isCaAuthorized handles default-allow, forbid-all ';', and unknown critical tag denial", () => {
    // Empty or iodef-only record set allows any valid CA by default
    const empty: [0]CaaRecord = [];
    expect_eq(isCaAuthorized(empty[0..0], "letsencrypt.org", false), true);

    const r_iodef = CaaRecord.parseText("0 iodef \"mailto:ops@example.com\"") ?? unreachable;
    const iodef_only: [1]CaaRecord = [r_iodef];
    expect_eq(isCaAuthorized(iodef_only[0..1], "any.ca", true), true);

    // Forbid-all ';' blocks issuance
    const r_forbid = CaaRecord.parseText("0 issue \";\"") ?? unreachable;
    const forbid_set: [1]CaaRecord = [r_forbid];
    expect_eq(isCaAuthorized(forbid_set[0..1], "letsencrypt.org", false), false);

    // Unknown NON-critical tag is ignored (does not block valid issue)
    const r_issue = CaaRecord.parseText("0 issue \"letsencrypt.org\"") ?? unreachable;
    const r_unk_nc = CaaRecord.parseText("0 futureext \"foo\"") ?? unreachable;
    const set_nc: [2]CaaRecord = [r_issue, r_unk_nc];
    expect_eq(isCaAuthorized(set_nc[0..2], "letsencrypt.org", false), true);

    // Unknown CRITICAL tag (flags=128) MUST deny issuance even if issue matches
    const r_unk_crit = CaaRecord.parseText("128 futureext \"foo\"") ?? unreachable;
    const set_crit: [2]CaaRecord = [r_issue, r_unk_crit];
    expect_eq(isCaAuthorized(set_crit[0..2], "letsencrypt.org", false), false);
});

test("5. Malformed CAA text inputs are rejected cleanly", () => {
    expect_eq(CaaRecord.parseText("") == null, true);
    expect_eq(CaaRecord.parseText("256 issue \"ca.org\"") == null, true);
    expect_eq(CaaRecord.parseText("-1 issue \"ca.org\"") == null, true);
    expect_eq(CaaRecord.parseText("0 bad-tag \"ca.org\"") == null, true);
    expect_eq(CaaRecord.parseText("0 issue \"unterminated") == null, true);
    expect_eq(CaaRecord.parseText("0 issue \"ca.org\" trailing") == null, true);
    expect_eq(CaaRecord.parseText("0 issue") == null, true);
});
