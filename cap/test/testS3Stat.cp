// Origin: Pure-CP Stdlib Test Suite (lib/s3/stat.cp) | Created: 2026-11-22
// Production Deep Verification Suite for S3 Object Metadata (`S3Stat`) & HEAD Parser:
//   1. S3Stat.init direct construction with RFC 7231 IMF-fixdate Last-Modified
//   2. parseHeadResponse case-insensitive HTTP HEAD response parsing (with status line & version-id)
//   3. parseHttpDateEpochMs exact UTC epoch-ms verification (Unix epoch, leap year, ISO-8601 + ms)
//   4. ETag normalization: unquotedEtag, weak ETag (`W/"..."`), multipart ETag (`<md5>-5`), matchesEtag
//   5. Optional/empty Last-Modified & missing version-id handling
//   6. Malformed Content-Length, invalid Last-Modified date & oversized field guards

import { test, expect_eq, expect_true } from "cap:test";
import {
    parseHttpDateEpochMs,
    S3Stat,
} from "../../lib/s3/stat.cp";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Test 1: S3Stat.init direct construction aligned with Bun S3Stat
// ============================================================================
test("s3Stat: direct S3Stat.init parses fields and Last-Modified epoch ms", () => {
    const st = S3Stat.init(
        1024,
        "\"7a30b741503c0b461cc14157e2df4ad8\"",
        "text/plain;charset=utf-8",
        "Tue, 07 Jan 2025 00:19:10 GMT",
    ) catch S3Stat {};

    expect_eq(st.getSize(), 1024);
    expect_true(strEq(st.etag(), "\"7a30b741503c0b461cc14157e2df4ad8\""));
    expect_true(strEq(st.unquotedEtag(), "7a30b741503c0b461cc14157e2df4ad8"));
    expect_true(strEq(st.contentType(), "text/plain;charset=utf-8"));
    expect_true(strEq(st.lastModified(), "Tue, 07 Jan 2025 00:19:10 GMT"));
    expect_true(!st.isWeakEtag());
    expect_true(!st.isMultipartEtag());

    // 2025-01-07T00:19:10.000Z == 1736209150000 ms
    if (ms := st.lastModifiedEpochMs()) {
        expect_eq(ms, 1736209150000);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 2: parseHeadResponse with HTTP status line, mixed-case headers & version id
// ============================================================================
test("s3Stat: parseHeadResponse extracts Content-Length, ETag, Content-Type, Last-Modified, VersionId", () => {
    const raw =
        "HTTP/1.1 200 OK\r\n" ++
        "Content-Length: 5242880\r\n" ++
        "ETag: \"d41d8cd98f00b204e9800998ecf8427e-3\"\r\n" ++
        "content-type: application/octet-stream\r\n" ++
        "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n" ++
        "x-amz-version-id: v9AbCdEf123\r\n\r\n";

    const st = S3Stat.parseHeadResponse(raw) catch S3Stat {};
    expect_eq(st.getSize(), 5242880);
    expect_true(st.isMultipartEtag());
    expect_true(strEq(st.contentType(), "application/octet-stream"));
    expect_true(st.hasVersionId());
    expect_true(strEq(st.versionId(), "v9AbCdEf123"));

    if (ms := st.lastModifiedEpochMs()) {
        expect_eq(ms, 0);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 3: parseHttpDateEpochMs (IMF-fixdate, ISO-8601 with ms, leap year)
// ============================================================================
test("s3Stat: parseHttpDateEpochMs handles IMF-fixdate, ISO-8601 & leap years", () => {
    if (ms1 := parseHttpDateEpochMs("2025-01-07T00:19:10.000Z")) {
        expect_eq(ms1, 1736209150000);
    } else {
        expect_true(false);
    }

    if (ms2 := parseHttpDateEpochMs("2025-01-07T00:19:10.250Z")) {
        expect_eq(ms2, 1736209150250);
    } else {
        expect_true(false);
    }

    // 2024-02-29T12:00:00Z (leap day) -> 1709208000000
    if (ms_leap := parseHttpDateEpochMs("Thu, 29 Feb 2024 12:00:00 GMT")) {
        expect_eq(ms_leap, 1709208000000);
    } else {
        expect_true(false);
    }

    expect_true(parseHttpDateEpochMs("not-a-date") == null);
    expect_true(parseHttpDateEpochMs("Tue, 07 Foo 2025 00:19:10 GMT") == null);
    expect_true(parseHttpDateEpochMs("2025-13-01T00:00:00Z") == null);
});

// ============================================================================
// Test 4: Weak ETag, unquoted ETag & cross-format matchesEtag
// ============================================================================
test("s3Stat: weak ETag detection and normalized ETag comparison", () => {
    const st = S3Stat.init(
        42,
        "W/\"abc123\"",
        "application/json",
        "",
    ) catch S3Stat {};

    expect_true(st.isWeakEtag());
    expect_true(strEq(st.unquotedEtag(), "abc123"));
    expect_true(st.matchesEtag("\"abc123\""));
    expect_true(st.matchesEtag("abc123"));
    expect_true(st.matchesEtag("W/\"abc123\""));
    expect_true(!st.matchesEtag("\"abc124\""));
    expect_true(st.lastModifiedEpochMs() == null);
    expect_true(!st.hasVersionId());
});

// ============================================================================
// Test 5: Malformed Content-Length & Invalid Last-Modified rejection
// ============================================================================
test("s3Stat: rejects malformed Content-Length and invalid Last-Modified", () => {
    let bad_len = false;
    * = S3Stat.parseHeadResponse("Content-Length: -5\r\n") catch brk: {
        bad_len = true;
        break :brk S3Stat {};
    };
    expect_true(bad_len);

    let bad_date = false;
    * = S3Stat.init(10, "\"etag\"", "text/plain", "BadDateValue") catch brk: {
        bad_date = true;
        break :brk S3Stat {};
    };
    expect_true(bad_date);
});

// ============================================================================
// Test 6: FieldTooLong buffer overflow protection
// ============================================================================
test("s3Stat: rejects oversized ETag / Content-Type without buffer overflow", () => {
    let long_etag: [200]u8 = ['a'; 200];
    let too_long = false;
    * = S3Stat.init(1, long_etag[0..], "text/plain", "") catch brk: {
        too_long = true;
        break :brk S3Stat {};
    };
    expect_true(too_long);
});
