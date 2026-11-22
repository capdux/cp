// Source: cp/cap/test/test_whois_production_deep.cp (Production deep tests for cap:whois)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WHOIS_TCP_PORT,
    WHOIS_IANA_HOST,
    WHOIS_MAX_QUERY_LEN,
    isValidWhoisDomain,
    encodeWhoisQuery,
    encodeWhoisDomainQuery,
    WhoisField,
    WhoisFieldIter,
    findWhoisField,
    parseWhoisRefer,
    WhoisSummary,
    parseWhoisSummary,
} from "cap:net";

fn expectBytesEq(actual: []const u8, expected: []const u8) {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

test("1. Protocol constants, query encoding, domain validation, and CRLF/NUL injection guards", () => {
    expect_eq(WHOIS_TCP_PORT, 43 as u16);
    expectBytesEq(WHOIS_IANA_HOST, "whois.iana.org");
    expect_eq(WHOIS_MAX_QUERY_LEN, 255);

    let out: [300]u8 = [0; 300];
    const n1 = encodeWhoisQuery("n + 8.8.8.8", out[0..]) ?? unreachable;
    expectBytesEq(out[0..n1], "n + 8.8.8.8\r\n");

    const n2 = encodeWhoisDomainQuery("sub-1.example.org", out[0..]) ?? unreachable;
    expectBytesEq(out[0..n2], "sub-1.example.org\r\n");

    // Reject empty query, CRLF injection, LF injection, NUL byte
    expect_eq(encodeWhoisQuery("", out[0..]) == null, true);
    expect_eq(encodeWhoisQuery("example.com\r\nevil.com", out[0..]) == null, true);
    expect_eq(encodeWhoisQuery("example.com\nevil.com", out[0..]) == null, true);
    expect_eq(encodeWhoisQuery("example.com\r", out[0..]) == null, true);
    const nul_payload: [5]u8 = ['a', '.', 'c', 0, 'm'];
    expect_eq(encodeWhoisQuery(nul_payload[0..], out[0..]) == null, true);

    // Reject invalid domains
    expect_eq(isValidWhoisDomain("-bad.com"), false);
    expect_eq(isValidWhoisDomain("bad-.com"), false);
    expect_eq(isValidWhoisDomain("bad..com"), false);
    expect_eq(isValidWhoisDomain(".leading.com"), false);
    expect_eq(isValidWhoisDomain("trailing.com."), false);
    expect_eq(isValidWhoisDomain("bad space.com"), false);
    expect_eq(encodeWhoisDomainQuery("bad_host.com", out[0..]) == null, true);

    // Reject insufficient buffer
    let tiny: [5]u8 = [0; 5];
    expect_eq(encodeWhoisQuery("abcd", tiny[0..]) == null, true);

    // Reject > 255 bytes query
    let over: [256]u8 = ['a'; 256];
    expect_eq(encodeWhoisQuery(over[0..], out[0..]) == null, true);
});

test("2. IANA and Registrar referral extraction via parseWhoisRefer", () => {
    const iana_body =
        "% IANA WHOIS server\n" +
        "% for more information on IANA, visit http://www.iana.org\n" +
        "\n" +
        "refer:        whois.verisign-grs.com\n" +
        "\n" +
        "domain:       COM\n" +
        "status:       ACTIVE\n";

    const ref1 = parseWhoisRefer(iana_body) ?? unreachable;
    expectBytesEq(ref1, "whois.verisign-grs.com");

    const icann_body =
        "   Domain Name: EXAMPLE.COM\r\n" +
        "   Registrar WHOIS Server: whois.registrar.example\r\n" +
        "   Registrar URL: http://www.registrar.example\r\n";

    const ref2 = parseWhoisRefer(icann_body) ?? unreachable;
    expectBytesEq(ref2, "whois.registrar.example");

    const fallback_body =
        "whois:        whois.nic.io\n" +
        "domain:       IO\n";
    const ref3 = parseWhoisRefer(fallback_body) ?? unreachable;
    expectBytesEq(ref3, "whois.nic.io");

    expect_eq(parseWhoisRefer("Domain Name: NONE.TEST\n") == null, true);
});

test("3. ICANN/Verisign multi-line response parsing, comment skipping, and WhoisSummary", () => {
    const verisign_body =
        "# Leading hash comment\r\n" +
        "% Leading percent comment\r\n" +
        "   Domain Name: EXAMPLE.COM\r\n" +
        "   Registry Domain ID: 2336799_DOMAIN_COM-VRSN\r\n" +
        "   Registrar WHOIS Server: whois.iana.org\r\n" +
        "   Updated Date: 2024-08-14T07:01:34Z\r\n" +
        "   Creation Date: 1995-08-14T04:00:00Z\r\n" +
        "   Registry Expiry Date: 2025-08-13T04:00:00Z\r\n" +
        "   Registrar: RESERVED-Internet Assigned Numbers Authority\r\n" +
        "   Domain Status: clientDeleteProhibited https://icann.org/epp#clientDeleteProhibited\r\n" +
        "   Domain Status: clientTransferProhibited https://icann.org/epp#clientTransferProhibited\r\n" +
        "   Domain Status: clientUpdateProhibited https://icann.org/epp#clientUpdateProhibited\r\n" +
        "   Name Server: A.IANA-SERVERS.NET\r\n" +
        "   Name Server: B.IANA-SERVERS.NET\r\n" +
        ">>> Last update of whois database: 2025-01-01T00:00:00Z <<<\r\n" +
        "NOTICE: The expiration date displayed in this record is the date.\r\n";

    // Case-insensitive lookup preserves colons inside timestamp value
    const created_val = findWhoisField(verisign_body, "creation date") ?? unreachable;
    expectBytesEq(created_val, "1995-08-14T04:00:00Z");

    const summary = parseWhoisSummary(verisign_body);
    expect_eq(summary.hasDomain(), true);
    expect_eq(summary.hasRegistrar(), true);
    expect_eq(summary.hasWhoisServer(), true);
    expectBytesEq(summary.domain_name, "EXAMPLE.COM");
    expectBytesEq(summary.registrar, "RESERVED-Internet Assigned Numbers Authority");
    expectBytesEq(summary.whois_server, "whois.iana.org");
    expectBytesEq(summary.created, "1995-08-14T04:00:00Z");
    expectBytesEq(summary.updated, "2024-08-14T07:01:34Z");
    expectBytesEq(summary.expires, "2025-08-13T04:00:00Z");
    expect_eq(summary.nameserver_count, 2);
    expect_eq(summary.status_count, 3);
});

test("4. RIPE / ccTLD alias normalization (domain, created, last-modified, paid-till, nserver, state)", () => {
    const ripe_body =
        "% This is the RIPE / ccTLD WHOIS database.\n" +
        "\n" +
        "domain:        EXAMPLE.RU\n" +
        "nserver:       ns1.example.ru.\n" +
        "nserver:       ns2.example.ru.\n" +
        "nserver:       ns3.example.ru.\n" +
        "state:         REGISTERED, DELEGATED, VERIFIED\n" +
        "org-name:      Example Foundation\n" +
        "created:       2010-03-15T12:00:00Z\n" +
        "last-modified: 2025-02-01T08:30:00Z\n" +
        "paid-till:     2026-03-15T12:00:00Z\n" +
        "refer:         whois.tcinet.ru\n";

    const s = parseWhoisSummary(ripe_body);
    expectBytesEq(s.domain_name, "EXAMPLE.RU");
    expectBytesEq(s.registrar, "Example Foundation");
    expectBytesEq(s.whois_server, "whois.tcinet.ru");
    expectBytesEq(s.created, "2010-03-15T12:00:00Z");
    expectBytesEq(s.updated, "2025-02-01T08:30:00Z");
    expectBytesEq(s.expires, "2026-03-15T12:00:00Z");
    expect_eq(s.nameserver_count, 3);
    expect_eq(s.status_count, 1);

    // Direct WhoisFieldIter check including keyEqualsIgnoreCase
    let it = WhoisFieldIter.init(ripe_body);
    const f0 = it.next() ?? unreachable;
    expect_eq(f0.keyEqualsIgnoreCase("DOMAIN"), true);
    expectBytesEq(f0.value, "EXAMPLE.RU");
});

test("5. Edge cases: empty response, comment-only, colon-less lines, empty key/value", () => {
    // Empty body
    let it_empty = WhoisFieldIter.init("");
    expect_eq(it_empty.next() == null, true);
    const s_empty = parseWhoisSummary("");
    expect_eq(s_empty.hasDomain(), false);
    expect_eq(s_empty.hasRegistrar(), false);
    expect_eq(s_empty.hasWhoisServer(), false);
    expect_eq(s_empty.nameserver_count, 0);
    expect_eq(s_empty.status_count, 0);

    // Comment-only, narrative lines without ':', and malformed ':onlyval' line
    const noisy =
        "   \r\n" +
        "# comment 1\n" +
        "% comment 2\r\n" +
        ">>> footer comment <<<\n" +
        "No colon on this narrative disclaimer line\n" +
        "   :missing-key\n" +
        "Empty-Val:   \n" +
        "  Real-Key \t :  value:with:colons  ";

    let it = WhoisFieldIter.init(noisy);
    const f1 = it.next() ?? unreachable;
    expectBytesEq(f1.key, "Empty-Val");
    expectBytesEq(f1.value, "");

    const f2 = it.next() ?? unreachable;
    expectBytesEq(f2.key, "Real-Key");
    expectBytesEq(f2.value, "value:with:colons");
    expect_eq(it.next() == null, true);

    expect_eq(findWhoisField(noisy, "missing") == null, true);
});
