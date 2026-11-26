// cap/test/testCapUrlJoin.cp
// Comprehensive unit tests for @cap/url-join

import { test, expect_eq } from "cap:test";
import {
    urlJoin,
    normalizeUrlPath,
    withQuery,
    withTrailingSlash,
    withoutTrailingSlash,
} from "../../packages/@cap/url-join/src/index.cp";

fn main() {
    test("1. @cap/url-join — HTTP/HTTPS/WS/file:/// and protocol-relative joining", () => {
        let buf: [512]u8 = undefined;

        let p1: [3]string = ["https://capdux.org/", "/packages/", "@cap/lru"];
        expect_eq(urlJoin(p1[0..3], buf[0..512]), "https://capdux.org/packages/@cap/lru");

        let p2: [3]string = ["file:///", "/var/log/", "app.log"];
        expect_eq(urlJoin(p2[0..3], buf[0..512]), "file:///var/log/app.log");

        let p3: [3]string = ["//cdn.example.com/", "/static/", "main.js"];
        expect_eq(urlJoin(p3[0..3], buf[0..512]), "//cdn.example.com/static/main.js");

        let p4: [2]string = ["wss://rt.example.com/", "/stream"];
        expect_eq(urlJoin(p4[0..2], buf[0..512]), "wss://rt.example.com/stream");

        let p5: [2]string = ["mailto:team@capdux.org", "?subject=hello"];
        expect_eq(urlJoin(p5[0..2], buf[0..512]), "mailto:team@capdux.org?subject=hello");
    });

    test("2. @cap/url-join — redundant slashes & empty segments skipped cleanly", () => {
        let buf: [512]u8 = undefined;

        let parts: [6]string = ["https://api.example.com///", "", "///v1///", "", "///users", "42"];
        expect_eq(urlJoin(parts[0..6], buf[0..512]), "https://api.example.com/v1/users/42");

        let rel: [4]string = ["", "/api/", "", "health/"];
        expect_eq(urlJoin(rel[0..4], buf[0..512]), "/api/health/");
    });

    test("3. @cap/url-join — multiple ?query merging & #hash moved to end", () => {
        let buf: [512]u8 = undefined;

        let parts: [5]string = ["https://capdux.org/search", "?q=cp", "?page=2", "&sort=desc", "#results"];
        expect_eq(urlJoin(parts[0..5], buf[0..512]), "https://capdux.org/search?q=cp&page=2&sort=desc#results");

        // Hash appearing in an earlier segment is moved to the final end after query
        let reordered: [3]string = ["https://capdux.org/docs#intro", "guide", "?lang=zh"];
        expect_eq(urlJoin(reordered[0..3], buf[0..512]), "https://capdux.org/docs/guide?lang=zh#intro");
    });

    test("4. @cap/url-join — normalizeUrlPath collapses //, . and ..", () => {
        let buf: [256]u8 = undefined;

        expect_eq(normalizeUrlPath("/a/b/../c//./d", buf[0..256]), "/a/c/d");
        expect_eq(normalizeUrlPath("/a/b/../../..", buf[0..256]), "/");
        expect_eq(normalizeUrlPath("/a/./b/c/", buf[0..256]), "/a/b/c/");
        expect_eq(normalizeUrlPath("a/b/../c", buf[0..256]), "a/c");
        expect_eq(normalizeUrlPath("../a/../b", buf[0..256]), "../b");
        expect_eq(normalizeUrlPath("", buf[0..256]), "");
    });

    test("5. @cap/url-join — withQuery auto ? vs & while preserving #hash", () => {
        let buf: [256]u8 = undefined;

        expect_eq(withQuery("https://capdux.org/api", "limit", "10", buf[0..256]), "https://capdux.org/api?limit=10");
        expect_eq(withQuery("https://capdux.org/api?limit=10", "page", "2", buf[0..256]), "https://capdux.org/api?limit=10&page=2");
        expect_eq(withQuery("https://capdux.org/api#top", "v", "1", buf[0..256]), "https://capdux.org/api?v=1#top");
        expect_eq(withQuery("https://capdux.org/api?v=1#top", "lang", "cp", buf[0..256]), "https://capdux.org/api?v=1&lang=cp#top");
    });

    test("6. @cap/url-join — withTrailingSlash & withoutTrailingSlash", () => {
        let buf: [256]u8 = undefined;

        expect_eq(withTrailingSlash("https://capdux.org/docs", buf[0..256]), "https://capdux.org/docs/");
        expect_eq(withTrailingSlash("https://capdux.org/docs/", buf[0..256]), "https://capdux.org/docs/");
        expect_eq(withTrailingSlash("https://capdux.org/docs?v=1#top", buf[0..256]), "https://capdux.org/docs/?v=1#top");

        expect_eq(withoutTrailingSlash("https://capdux.org/docs///", buf[0..256]), "https://capdux.org/docs");
        expect_eq(withoutTrailingSlash("https://capdux.org/docs/?v=1#top", buf[0..256]), "https://capdux.org/docs?v=1#top");
        expect_eq(withoutTrailingSlash("/", buf[0..256]), "/");
        expect_eq(withoutTrailingSlash("https://", buf[0..256]), "https://");
    });
}
