// Origin: Pure-CP Stdlib Test Suite (lib/web/contentNeg.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 9110 HTTP Content Negotiation & Vary Header Engine:
//   1. RFC 9110 §12.4.2 exact integer q-value parser (0..1000) & malformed rejection
//   2. Accept media-type negotiation: specificity, q-value, media params, wildcard & q=0 exclusion
//   3. Accept-Encoding negotiation: zstd/br/gzip/deflate/identity/*, identity;q=0 & *;q=0 406 handling
//   4. Accept-Language BCP 47 negotiation: case-insensitive exact, subtag prefix fallback & q=0 exclusion
//   5. Zero-alloc Vary header case-insensitive dedup, RFC 9110 §12.5.5 '*' collapse & buffer guards
//   6. Malformed headers, CRLF injection guards, invalid wildcards & cap.web facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    NEG_Q_MAX,
    NEG_MAX_ITEMS,
    NEG_VARY_MAX_HEADERS,
    parseQValue,
    ContentEncoding,
    NegotiationMatchKind,
    MediaMatchResult,
    EncodingMatchResult,
    LanguageMatchResult,
    negotiateMediaType,
    negotiateEncoding,
    negotiateLanguage,
    formatVaryHeader,
} from "cap:web";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("ContentNeg: RFC 9110 §12.4.2 integer q-value parsing & invalid rejection") {
    expect_eq(parseQValue("1") orelse 9999, 1000 as u16);
    expect_eq(parseQValue("1.000") orelse 9999, 1000 as u16);
    expect_eq(parseQValue("1.") orelse 9999, 1000 as u16);
    expect_eq(parseQValue("0") orelse 9999, 0 as u16);
    expect_eq(parseQValue("0.") orelse 9999, 0 as u16);
    expect_eq(parseQValue("0.8") orelse 9999, 800 as u16);
    expect_eq(parseQValue("0.75") orelse 9999, 750 as u16);
    expect_eq(parseQValue("0.125") orelse 9999, 125 as u16);

    // Invalid q-values rejected
    expect_true(parseQValue("") == null);
    expect_true(parseQValue("1.1") == null);
    expect_true(parseQValue("1.001") == null);
    expect_true(parseQValue("0.1234") == null);
    expect_true(parseQValue("-0.5") == null);
    expect_true(parseQValue("abc") == null);
}

test("ContentNeg: Accept media-type specificity, q-value, params & q=0 exclusion") {
    const offered_basic: [2][]const u8 = ["application/json", "text/html"];

    // Higher q-value wins
    const r1 = negotiateMediaType("text/html;q=0.5, application/json;q=0.9", offered_basic[0..]);
    expect_eq(r1.status, NegotiationMatchKind.matched);
    expect_eq(r1.offered_index, 0 as usize);
    expect_eq(r1.q_value, 900 as u16);
    expect_eq(r1.specificity, 2 as u8);

    // Explicit q=0 on text/html overrides */*;q=1.0 -> falls back to application/json
    const r2 = negotiateMediaType("text/html;q=0, */*;q=1.0", offered_basic[0..]);
    expect_eq(r2.status, NegotiationMatchKind.matched);
    expect_eq(r2.offered_index, 0 as usize);
    expect_eq(r2.q_value, 1000 as u16);
    expect_eq(r2.specificity, 0 as u8);

    // Media parameter specificity breaks equal q-value tie (specificity 3 > 2)
    const offered_param: [2][]const u8 = ["text/html", "text/html;level=1"];
    const r3 = negotiateMediaType("text/html;level=1;q=1.0, text/html;q=1.0", offered_param[0..]);
    expect_eq(r3.status, NegotiationMatchKind.matched);
    expect_eq(r3.offered_index, 1 as usize);
    expect_eq(r3.specificity, 3 as u8);

    // Type wildcard `text/*` matches text/html, rejects application/json
    const r4 = negotiateMediaType("text/*;q=0.7", offered_basic[0..]);
    expect_eq(r4.status, NegotiationMatchKind.matched);
    expect_eq(r4.offered_index, 1 as usize);
    expect_eq(r4.specificity, 1 as u8);

    // Empty Accept header defaults to */*;q=1.0 -> server preference #0
    const r5 = negotiateMediaType("", offered_basic[0..]);
    expect_eq(r5.status, NegotiationMatchKind.matched);
    expect_eq(r5.offered_index, 0 as usize);

    // Unmatched media types -> 406 not_acceptable
    const r6 = negotiateMediaType("image/png", offered_basic[0..]);
    expect_eq(r6.status, NegotiationMatchKind.not_acceptable);
}

test("ContentNeg: Accept-Encoding zstd/br/gzip/identity, server order & 406 exclusion") {
    const offered_all: [4]ContentEncoding = [
        ContentEncoding.zstd,
        ContentEncoding.br,
        ContentEncoding.gzip,
        ContentEncoding.identity,
    ];

    // Tied q=1.0 between br and zstd -> server preference order picks zstd (#0)
    const r1 = negotiateEncoding("gzip;q=0.8, br;q=1.0, zstd;q=1.0", offered_all[0..]);
    expect_eq(r1.status, NegotiationMatchKind.matched);
    expect_eq(r1.encoding, ContentEncoding.zstd);
    expect_eq(r1.offered_index, 0 as usize);
    expect_eq(r1.q_value, 1000 as u16);

    // x-gzip alias maps to gzip
    const offered_gz: [2]ContentEncoding = [ContentEncoding.gzip, ContentEncoding.identity];
    const r_xgz = negotiateEncoding("x-gzip;q=0.9", offered_gz[0..]);
    expect_eq(r_xgz.status, NegotiationMatchKind.matched);
    expect_eq(r_xgz.encoding, ContentEncoding.gzip);

    // Implicit identity fallback when no compression matches
    const offered_br_id: [2]ContentEncoding = [ContentEncoding.br, ContentEncoding.identity];
    const r2 = negotiateEncoding("gzip;q=0.9, deflate;q=0.5", offered_br_id[0..]);
    expect_eq(r2.status, NegotiationMatchKind.matched);
    expect_eq(r2.encoding, ContentEncoding.identity);

    // Explicit identity;q=0 with no matching coding -> 406 not_acceptable
    const r3 = negotiateEncoding("gzip, identity;q=0", offered_br_id[0..]);
    expect_eq(r3.status, NegotiationMatchKind.not_acceptable);

    // Wildcard *;q=0 blocks even implicit identity -> 406 not_acceptable
    const r4 = negotiateEncoding("zstd, *;q=0", offered_br_id[0..]);
    expect_eq(r4.status, NegotiationMatchKind.not_acceptable);

    // Empty Accept-Encoding selects identity if offered, else not_acceptable
    const r5 = negotiateEncoding("", offered_br_id[0..]);
    expect_eq(r5.status, NegotiationMatchKind.matched);
    expect_eq(r5.encoding, ContentEncoding.identity);

    const offered_no_id: [1]ContentEncoding = [ContentEncoding.br];
    const r6 = negotiateEncoding("", offered_no_id[0..]);
    expect_eq(r6.status, NegotiationMatchKind.not_acceptable);
}

test("ContentNeg: Accept-Language BCP 47 exact match, prefix fallback & q=0 exclusion") {
    // Prefix fallback: zh-Hans-CN matches zh-Hans (len 7) over zh (len 2)
    const offered_zh: [3][]const u8 = ["en", "zh", "zh-Hans"];
    const r1 = negotiateLanguage("zh-Hans-CN, en-US;q=0.8", offered_zh[0..]);
    expect_eq(r1.status, NegotiationMatchKind.matched);
    expect_eq(r1.offered_index, 2 as usize); // zh-Hans
    expect_eq(r1.q_value, 1000 as u16);
    expect_eq(r1.matched_prefix_len, 7 as usize);

    // Case-insensitive exact match
    const offered_case: [2][]const u8 = ["fr", "zh-CN"];
    const r2 = negotiateLanguage("ZH-cn;q=0.9, fr;q=0.5", offered_case[0..]);
    expect_eq(r2.status, NegotiationMatchKind.matched);
    expect_eq(r2.offered_index, 1 as usize);
    expect_eq(r2.q_value, 900 as u16);

    // q=0 exclusion overrides wildcard '*'
    const offered_ex: [2][]const u8 = ["en-US", "de"];
    const r3 = negotiateLanguage("en;q=0, *;q=0.5", offered_ex[0..]);
    expect_eq(r3.status, NegotiationMatchKind.matched);
    expect_eq(r3.offered_index, 1 as usize); // de
    expect_eq(r3.q_value, 500 as u16);

    // All languages rejected -> 406 not_acceptable
    const r4 = negotiateLanguage("ja, ko;q=0.8", offered_ex[0..]);
    expect_eq(r4.status, NegotiationMatchKind.not_acceptable);
}

test("ContentNeg: zero-alloc Vary header dedup, '*' collapse & buffer guards") {
    let buf: [128]u8 = [0; 128];
    let buf_s: []u8 = buf[0..];

    const add: [3][]const u8 = ["Accept-Encoding", "Accept-Language", "ACCEPT"];
    const n1 = formatVaryHeader("Accept, accept-encoding", add[0..], *buf_s) orelse {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(buf[0..n1], "Accept, accept-encoding, Accept-Language"));

    // RFC 9110 §12.5.5: presence of '*' collapses entire Vary header to "*"
    const add_star: [2][]const u8 = ["Origin", "*"];
    const n2 = formatVaryHeader("Accept-Encoding", add_star[0..], *buf_s) orelse {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(buf[0..n2], "*"));

    // Too-small buffer returns null
    let tiny: [5]u8 = [0; 5];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatVaryHeader("Accept-Encoding", add[0..], *tiny_s) == null);

    // CRLF injection in Vary header returns null
    const bad_add: [1][]const u8 = ["Accept\r\nX-Evil: 1"];
    expect_true(formatVaryHeader("", bad_add[0..], *buf_s) == null);
}

test("ContentNeg: malformed headers, CRLF injection guards & cap.web facade parity") {
    const offered_m: [1][]const u8 = ["application/json"];
    // Invalid wildcard `*/json`
    expect_eq(negotiateMediaType("*/json", offered_m[0..]).status, NegotiationMatchKind.invalid_header);
    // Invalid q-value > 1
    expect_eq(negotiateMediaType("application/json;q=1.5", offered_m[0..]).status, NegotiationMatchKind.invalid_header);
    // CRLF injection in Accept / Accept-Encoding / Accept-Language
    expect_eq(negotiateMediaType("text/html\r\nX-Inject: 1", offered_m[0..]).status, NegotiationMatchKind.invalid_header);

    const offered_e: [1]ContentEncoding = [ContentEncoding.gzip];
    expect_eq(negotiateEncoding("gzip\nX-Bad: 1", offered_e[0..]).status, NegotiationMatchKind.invalid_header);

    const offered_l: [1][]const u8 = ["en"];
    expect_eq(negotiateLanguage("en\r\nBad: 1", offered_l[0..]).status, NegotiationMatchKind.invalid_header);

    // Empty offered list -> not_acceptable
    const empty_m: [0][]const u8 = [];
    expect_eq(negotiateMediaType("*/*", empty_m[0..]).status, NegotiationMatchKind.not_acceptable);

    // Verify cap.web namespace facade parity
    expect_eq(cap.web.NEG_Q_MAX, NEG_Q_MAX);
    expect_eq(cap.web.NEG_MAX_ITEMS, NEG_MAX_ITEMS);
    expect_eq(cap.web.NEG_VARY_MAX_HEADERS, NEG_VARY_MAX_HEADERS);
    expect_eq(cap.web.parseQValue("0.5") orelse 0, 500 as u16);
}
