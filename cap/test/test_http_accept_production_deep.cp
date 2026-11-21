import {
    parseQValue,
    AcceptIter,
    negotiateMediaType,
    negotiateEncoding,
    negotiateLanguage,
} from "cap:http_accept";
import { test, expect } from "cap:test";

test("parseQValue strict RFC 9110 validation", () => {
    expect(parseQValue("1").? == 1000);
    expect(parseQValue("1.000").? == 1000);
    expect(parseQValue("0").? == 0);
    expect(parseQValue("0.0").? == 0);
    expect(parseQValue("0.5").? == 500);
    expect(parseQValue("0.75").? == 750);
    expect(parseQValue("0.123").? == 123);
    expect(parseQValue(" 0.8 ").? == 800);
    expect(parseQValue("1.001") == null);
    expect(parseQValue("0.1234") == null);
    expect(parseQValue("2") == null);
    expect(parseQValue("-0.5") == null);
    expect(parseQValue("") == null);
});

test("AcceptIter quoted params and specificity", () => {
    let it = AcceptIter.init("text/html; level=\"1,2\"; q=0.7, application/json, text/*;q=0.5, */*;q=0.1");
    const r1 = it.next().?;
    expect(r1.q_millis == 700);
    expect(r1.specificity() == 3);
    expect(r1.matches("TEXT/HTML; charset=utf-8"));
    expect(!r1.matches("text/plain"));

    const r2 = it.next().?;
    expect(r2.q_millis == 1000);
    expect(r2.specificity() == 3);

    const r3 = it.next().?;
    expect(r3.q_millis == 500);
    expect(r3.specificity() == 2);
    expect(r3.matches("text/plain"));

    const r4 = it.next().?;
    expect(r4.q_millis == 100);
    expect(r4.specificity() == 1);
    expect(r4.matches("image/png"));
    expect(it.next() == null);
});

test("negotiateMediaType specificity and q=0 exclusion", () => {
    const offered: [3][]const u8 = ["text/html", "application/json", "text/plain"];
    expect(negotiateMediaType("text/html;q=0.8, application/json;q=0.9", offered[0..]).? == 1);
    // Exact q=0 overrides wildcard */*
    expect(negotiateMediaType("text/html;q=0, */*;q=0.5", offered[0..]).? == 1);
    // Specificity tie-break when q values are equal
    expect(negotiateMediaType("text/*;q=0.8, application/json;q=0.8", offered[0..]).? == 1);
    // Empty header picks first offered
    expect(negotiateMediaType("", offered[0..]).? == 0);
    // Everything rejected
    expect(negotiateMediaType("*/*;q=0", offered[0..]) == null);
});

test("negotiateEncoding exact override and wildcard", () => {
    const offered: [3][]const u8 = ["gzip", "br", "zstd"];
    expect(negotiateEncoding("gzip;q=0.6, br;q=1.0, zstd;q=0.8", offered[0..]).? == 1);
    // Explicitly reject gzip even when wildcard allows others
    expect(negotiateEncoding("gzip;q=0, *;q=0.5", offered[0..]).? == 1);
    // Case-insensitive match
    expect(negotiateEncoding("ZSTD;Q=0.9, GZIP;q=0.4", offered[0..]).? == 2);
    expect(negotiateEncoding("deflate", offered[0..]) == null);
});

test("negotiateLanguage exact, prefix, and q=0", () => {
    const offered: [3][]const u8 = ["en-US", "zh-CN", "fr-FR"];
    expect(negotiateLanguage("fr;q=0.5, zh-CN;q=0.9, en;q=0.8", offered[0..]).? == 1);
    // Prefix match: "en" matches "en-US"
    expect(negotiateLanguage("en;q=0.9, fr-FR;q=0.5", offered[0..]).? == 0);
    // Exact tag q=0 blocks prefix match
    expect(negotiateLanguage("en-US;q=0, en;q=0.9, fr;q=0.4", offered[0..]).? == 2);
    // Wildcard fallback
    expect(negotiateLanguage("de-DE, *;q=0.2", offered[0..]).? == 0);
});
