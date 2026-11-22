// cap/test/testX509AltName.cp
// Production-grade deep tests for cap:crypto x509AltName.cp
// (X.509 SAN safety validator isSafeAltName, parser & injection-safe serializer absorbed from Bun x509.cp)

import { test, assert, assertEq } from "cap:test";
import {
    X509_ALT_NAME_MAX_ENTRIES,
    X509AltNameKind,
    X509AltNameError,
    isSafeAltName,
    X509AltNameEntry,
    parseX509AltNameEntry,
    formatX509AltNameList,
    countSafeX509AltNames,
} from "cap:crypto";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("isSafeAltName: ASCII vs UTF-8 safety rules aligned with Bun x509.cp", () => {
    assertEq(X509_ALT_NAME_MAX_ENTRIES, 32);

    // Safe printable ASCII hostnames, wildcards, IPs, emails, URIs
    assert(isSafeAltName("example.com", false));
    assert(isSafeAltName("*.api-v2.example.org", false));
    assert(isSafeAltName("2001:db8::1", false));
    assert(isSafeAltName("https://cp.dev/cert?id=1", false));
    assert(isSafeAltName("", false));

    // Reject delimiter/quote/backslash characters in both ASCII and UTF-8 modes
    assert(!isSafeAltName("a,DNS:evil.com", false));
    assert(!isSafeAltName("a,DNS:evil.com", true));
    assert(!isSafeAltName("bad\"name", false));
    assert(!isSafeAltName("bad\\name", true));
    assert(!isSafeAltName("bad'name", false));

    // Reject control chars (< 32) and DEL (127)
    const nul_bytes: [4]u8 = ['a', 0, 'b', 'c'];
    const del_bytes: [3]u8 = ['a', 127, 'b'];
    assert(!isSafeAltName(nul_bytes[0..], false));
    assert(!isSafeAltName(nul_bytes[0..], true));
    assert(!isSafeAltName(del_bytes[0..], false));
    assert(!isSafeAltName(del_bytes[0..], true));

    // Non-ASCII bytes (> 126): rejected when utf8=false, allowed when utf8=true
    const utf8_bytes: [4]u8 = [0xC3, 0xA9, 'x', 'y']; // "éxy"
    assert(!isSafeAltName(utf8_bytes[0..], false));
    assert(isSafeAltName(utf8_bytes[0..], true));
});

test("X509AltNameEntry: verbatim safe format & quoted escape for malformed SAN values", () => {
    const safe_e = X509AltNameEntry.init(X509AltNameKind.Dns, "cp.dev");
    assert(safe_e.isSafe());
    let out: [128]u8 = [0; 128];
    const n1 = safe_e.formatInto(out[0..]) catch {
        assert(false);
        return;
    };
    assert(sliceEql(out[0..n1], "DNS:cp.dev"));

    // Entry with comma + quote + NUL gets quoted & escaped
    const raw_bad: [11]u8 = ['a', ',', '"', '\\', 0x00, 'D', 'N', 'S', ':', 'x', 'y'];
    const bad_e = X509AltNameEntry.init(X509AltNameKind.Dns, raw_bad[0..]);
    assert(!bad_e.isSafe());
    const n2 = bad_e.formatInto(out[0..]) catch {
        assert(false);
        return;
    };
    assert(sliceEql(out[0..n2], "DNS:\"a,\\\"\\\\\\x00DNS:xy\""));

    // formatStrictInto rejects unsafe entry
    let got_unsafe = false;
    * = bad_e.formatStrictInto(out[0..]) catch |e| {
        assertEq(e, X509AltNameError.UnsafeAltName);
        got_unsafe = true;
        0
    };
    assert(got_unsafe);
});

test("parseX509AltNameEntry: parses all 6 RFC 5280 prefixes & rejects unknown", () => {
    const e1 = parseX509AltNameEntry("DNS:*.example.com") catch {
        assert(false);
        return;
    };
    assertEq(e1.kind, X509AltNameKind.Dns);
    assert(sliceEql(e1.value, "*.example.com"));

    const e2 = parseX509AltNameEntry("IP Address:192.168.1.1") catch {
        assert(false);
        return;
    };
    assertEq(e2.kind, X509AltNameKind.Ip);
    assert(sliceEql(e2.value, "192.168.1.1"));

    const e3 = parseX509AltNameEntry("email:sec@example.com") catch {
        assert(false);
        return;
    };
    assertEq(e3.kind, X509AltNameKind.Email);
    assert(sliceEql(e3.value, "sec@example.com"));

    const e4 = parseX509AltNameEntry("URI:spiffe://cluster.local/ns/default") catch {
        assert(false);
        return;
    };
    assertEq(e4.kind, X509AltNameKind.Uri);

    const e5 = parseX509AltNameEntry("DirName:CN=root") catch {
        assert(false);
        return;
    };
    assertEq(e5.kind, X509AltNameKind.DirName);

    const e6 = parseX509AltNameEntry("Registered ID:1.2.3.4") catch {
        assert(false);
        return;
    };
    assertEq(e6.kind, X509AltNameKind.RegisteredId);

    let got_invalid = false;
    * = parseX509AltNameEntry("") catch |e| {
        assertEq(e, X509AltNameError.InvalidEntry);
        got_invalid = true;
        X509AltNameEntry{}
    };
    assert(got_invalid);

    let got_unknown = false;
    * = parseX509AltNameEntry("OTHER:abc") catch |e| {
        assertEq(e, X509AltNameError.UnknownPrefix);
        got_unknown = true;
        X509AltNameEntry{}
    };
    assert(got_unknown);
});

test("formatX509AltNameList & countSafeX509AltNames: multi-entry batch serialization", () => {
    const entries: [3]X509AltNameEntry = [
        X509AltNameEntry.init(X509AltNameKind.Dns, "api.cp.dev"),
        X509AltNameEntry.init(X509AltNameKind.Ip, "127.0.0.1"),
        X509AltNameEntry.init(X509AltNameKind.Dns, "odd,host"),
    ];
    assertEq(countSafeX509AltNames(entries[0..]), 2);

    let out: [128]u8 = [0; 128];
    const n = formatX509AltNameList(entries[0..], out[0..]) catch {
        assert(false);
        return;
    };
    assert(sliceEql(out[0..n], "DNS:api.cp.dev, IP Address:127.0.0.1, DNS:\"odd,host\""));
});

test("X509AltName: BufferTooSmall boundary guard", () => {
    const entries: [2]X509AltNameEntry = [
        X509AltNameEntry.init(X509AltNameKind.Dns, "long.subdomain.example.com"),
        X509AltNameEntry.init(X509AltNameKind.Ip, "10.0.0.1"),
    ];
    let tiny: [10]u8 = [0; 10];
    let got_small = false;
    * = formatX509AltNameList(entries[0..], tiny[0..]) catch |e| {
        assertEq(e, X509AltNameError.BufferTooSmall);
        got_small = true;
        0
    };
    assert(got_small);
});
