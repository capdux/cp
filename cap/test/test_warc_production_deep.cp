// Source: cp/cap/test/test_warc_production_deep.cp
// Deep Production Test Suite for cap:warc
// Covers:
//   1. Common Crawl multi-record stream: warcinfo + request + response (binary payload with \r\n\r\n) + conversion
//   2. Case-insensitive findHeader / findWarcHeader custom fields & WarcFieldIter
//   3. WarcRecordType complete 8-type mapping + unknown + case-insensitivity
//   4. summarizeWarc single-pass statistics & LF-only (\n\n) compatibility
//   5. Wrong version header, missing Content-Length, truncated payload & overflow Content-Length guards

import { test, expect_eq } from "cap:test";
import {
    WARC_VERSION_1_0,
    WARC_VERSION_1_1,
    WarcRecordType,
    warcRecordTypeFromSlice,
    WarcField,
    WarcFieldIter,
    findWarcHeader,
    WarcRecord,
    WarcIter,
    WarcStats,
    isWarc,
    summarizeWarc,
} from "cap:binfmt";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("warc: Common Crawl multi-record stream (warcinfo + request + response + conversion)") {
    const info_body = "software: CommonCrawl/1.0\r\n"; // 27 bytes
    const req_body = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n"; // 37 bytes
    // Response payload contains embedded \r\n\r\n and binary bytes (56 bytes total)
    const resp_body = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hi</h1>\x00";
    const conv_body = "Hi"; // 2 bytes

    const stream =
        "WARC/1.0\r\n" ++
        "WARC-Type: warcinfo\r\n" ++
        "WARC-Date: 2026-11-21T00:00:00Z\r\n" ++
        "WARC-Record-ID: <urn:uuid:0001>\r\n" ++
        "Content-Type: application/warc-fields\r\n" ++
        "Content-Length: 27\r\n" ++
        "\r\n" ++
        info_body ++
        "\r\n\r\n" ++
        "WARC/1.1\r\n" ++
        "WARC-Type: request\r\n" ++
        "WARC-Date: 2026-11-21T00:00:01Z\r\n" ++
        "WARC-Record-ID: <urn:uuid:0002>\r\n" ++
        "WARC-Target-URI: https://example.com/\r\n" ++
        "Content-Type: application/http; msgtype=request\r\n" ++
        "Content-Length: 37\r\n" ++
        "\r\n" ++
        req_body ++
        "\r\n\r\n" ++
        "WARC/1.0\r\n" ++
        "WARC-Type: response\r\n" ++
        "WARC-Date: 2026-11-21T00:00:02Z\r\n" ++
        "WARC-Record-ID: <urn:uuid:0003>\r\n" ++
        "WARC-Target-URI: https://example.com/\r\n" ++
        "WARC-IP-Address: 93.184.216.34\r\n" ++
        "WARC-Payload-Digest: sha1:ABCDEF123456\r\n" ++
        "Content-Type: application/http; msgtype=response\r\n" ++
        "Content-Length: 56\r\n" ++
        "\r\n" ++
        resp_body ++
        "\r\n\r\n" ++
        "WARC/1.0\r\n" ++
        "WARC-Type: conversion\r\n" ++
        "WARC-Date: 2026-11-21T00:00:03Z\r\n" ++
        "WARC-Record-ID: <urn:uuid:0004>\r\n" ++
        "WARC-Target-URI: https://example.com/\r\n" ++
        "Content-Type: text/plain\r\n" ++
        "Content-Length: 2\r\n" ++
        "\r\n" ++
        conv_body ++
        "\r\n\r\n";

    expect_eq(isWarc(stream), true);

    let it = WarcIter.init(stream);
    let idx: u32 = 0;
    while (rec := it.next()) {
        idx += 1;
        if (idx == 1) {
            expect_eq(strEq(rec.version, WARC_VERSION_1_0), true);
            expect_eq(rec.record_type == WarcRecordType.warcinfo, true);
            expect_eq(strEq(rec.record_id, "<urn:uuid:0001>"), true);
            expect_eq(rec.content_length, 27);
            expect_eq(strEq(rec.payload, info_body), true);
        } else if (idx == 2) {
            expect_eq(strEq(rec.version, WARC_VERSION_1_1), true);
            expect_eq(rec.record_type == WarcRecordType.request, true);
            expect_eq(strEq(rec.target_uri, "https://example.com/"), true);
            expect_eq(rec.content_length, 37);
        } else if (idx == 3) {
            expect_eq(rec.record_type == WarcRecordType.response, true);
            expect_eq(strEq(rec.payload_digest, "sha1:ABCDEF123456"), true);
            expect_eq(rec.content_length, 56);
            expect_eq(rec.payload.len, 56);
            expect_eq(rec.payload[55], 0);
            if (ip := rec.findHeader("warc-ip-address")) {
                expect_eq(strEq(ip, "93.184.216.34"), true);
            } else {
                expect_eq(true, false);
            }
        } else if (idx == 4) {
            expect_eq(rec.record_type == WarcRecordType.conversion, true);
            expect_eq(strEq(rec.payload, "Hi"), true);
        }
    }
    expect_eq(idx, 4);
    expect_eq(it.had_error, false);

    if (stats := summarizeWarc(stream)) {
        expect_eq(stats.record_count, 4);
        expect_eq(stats.warcinfo_count, 1);
        expect_eq(stats.request_count, 1);
        expect_eq(stats.response_count, 1);
        expect_eq(stats.conversion_count, 1);
        expect_eq(stats.total_payload_bytes, 27 + 37 + 56 + 2);
    } else {
        expect_eq(true, false);
    }
}

test("warc: case-insensitive findWarcHeader and WarcFieldIter") {
    const hdrs =
        "WARC-Type: metadata\r\n" ++
        "WARC-Identified-Payload-Type: application/pdf\r\n" ++
        "X-Custom-Tag:  value-with-spaces  \r\n";

    if (pt := findWarcHeader(hdrs, "warc-identified-payload-type")) {
        expect_eq(strEq(pt, "application/pdf"), true);
    } else {
        expect_eq(true, false);
    }

    if (ct := findWarcHeader(hdrs, "x-custom-tag")) {
        expect_eq(strEq(ct, "value-with-spaces"), true);
    } else {
        expect_eq(true, false);
    }

    if (missing := findWarcHeader(hdrs, "WARC-Missing")) {
        * = missing;
        expect_eq(true, false);
    }

    let fit = WarcFieldIter.init(hdrs);
    let count: u32 = 0;
    while (f := fit.next()) {
        * = f;
        count += 1;
    }
    expect_eq(count, 3);
}

test("warc: WarcRecordType complete mapping & case-insensitivity") {
    expect_eq(warcRecordTypeFromSlice("warcinfo") == WarcRecordType.warcinfo, true);
    expect_eq(warcRecordTypeFromSlice("RESPONSE") == WarcRecordType.response, true);
    expect_eq(warcRecordTypeFromSlice("Resource") == WarcRecordType.resource, true);
    expect_eq(warcRecordTypeFromSlice("request") == WarcRecordType.request, true);
    expect_eq(warcRecordTypeFromSlice("metadata") == WarcRecordType.metadata, true);
    expect_eq(warcRecordTypeFromSlice("revisit") == WarcRecordType.revisit, true);
    expect_eq(warcRecordTypeFromSlice("conversion") == WarcRecordType.conversion, true);
    expect_eq(warcRecordTypeFromSlice("continuation") == WarcRecordType.continuation, true);
    expect_eq(warcRecordTypeFromSlice("custom") == WarcRecordType.unknown, true);
}

test("warc: LF-only (\\n and \\n\\n) stream compatibility") {
    const lf_warc =
        "WARC/1.1\n" ++
        "WARC-Type: resource\n" ++
        "Content-Length: 5\n" ++
        "\n" ++
        "hello" ++
        "\n\n" ++
        "WARC/1.1\n" ++
        "WARC-Type: response\n" ++
        "Content-Length: 3\n" ++
        "\n" ++
        "bye" ++
        "\n\n";

    expect_eq(isWarc(lf_warc), true);
    if (stats := summarizeWarc(lf_warc)) {
        expect_eq(stats.record_count, 2);
        expect_eq(stats.response_count, 1);
        expect_eq(stats.total_payload_bytes, 8);
    } else {
        expect_eq(true, false);
    }
}

test("warc: wrong version, missing Content-Length, truncated payload & overflow guards") {
    // 1. Wrong version header
    const bad_ver = "WARC/2.0\r\nWARC-Type: response\r\nContent-Length: 0\r\n\r\n\r\n\r\n";
    expect_eq(isWarc(bad_ver), false);
    if (s := summarizeWarc(bad_ver)) {
        * = s;
        expect_eq(true, false);
    }

    // 2. Missing Content-Length header
    const no_cl =
        "WARC/1.0\r\n" ++
        "WARC-Type: response\r\n" ++
        "\r\n" ++
        "abc\r\n\r\n";
    let it1 = WarcIter.init(no_cl);
    if (r := it1.next()) {
        * = r;
        expect_eq(true, false);
    }
    expect_eq(it1.had_error, true);

    // 3. Truncated payload (claims 100 bytes, only 4 available)
    const trunc =
        "WARC/1.0\r\n" ++
        "WARC-Type: response\r\n" ++
        "Content-Length: 100\r\n" ++
        "\r\n" ++
        "abcd";
    if (s := summarizeWarc(trunc)) {
        * = s;
        expect_eq(true, false);
    }
    let it2 = WarcIter.init(trunc);
    if (r := it2.next()) {
        * = r;
        expect_eq(true, false);
    }
    expect_eq(it2.had_error, true);

    // 4. Overflowing Content-Length
    const huge_cl =
        "WARC/1.0\r\n" ++
        "WARC-Type: response\r\n" ++
        "Content-Length: 999999999999999999999999\r\n" ++
        "\r\n";
    let it3 = WarcIter.init(huge_cl);
    if (r := it3.next()) {
        * = r;
        expect_eq(true, false);
    }
    expect_eq(it3.had_error, true);
}
