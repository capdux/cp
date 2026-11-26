// cap/test/testCapLinkHeader.cp
// Comprehensive unit tests for @cap/link-header

import { test, expect_eq } from "cap:test";
import {
    LinkEntry,
    LinkHeader,
    parseLinkHeader,
    formatLinkHeader,
    formatPaginationLinks,
} from "../../packages/@cap/link-header/src/index.cp";

fn main() {
    test("1. @cap/link-header — GitHub-style next/prev/first/last pagination parsing", () => {
        const hdr = "<https://api.github.com/user/repos?page=3&per_page=100>; rel=\"next\", "
            ++ "<https://api.github.com/user/repos?page=50&per_page=100>; rel=\"last\", "
            ++ "<https://api.github.com/user/repos?page=1&per_page=100>; rel=\"first\", "
            ++ "<https://api.github.com/user/repos?page=1&per_page=100>; rel=\"prev\"";
        const parsed = parseLinkHeader(hdr);
        expect_eq(parsed.count, 4 as usize);
        expect_eq(parsed.nextUrl() ?? "", "https://api.github.com/user/repos?page=3&per_page=100");
        expect_eq(parsed.lastUrl() ?? "", "https://api.github.com/user/repos?page=50&per_page=100");
        expect_eq(parsed.firstUrl() ?? "", "https://api.github.com/user/repos?page=1&per_page=100");
        expect_eq(parsed.prevUrl() ?? "", "https://api.github.com/user/repos?page=1&per_page=100");
    });

    test("2. @cap/link-header — Quoted commas/semicolons, unquoted params & multi-rel", () => {
        const hdr = "<https://example.com/p2>; REL=\"next prefetch\"; title=\"Part 2, Section A; Revised\", "
            ++ "<https://example.com/p9>; rel=last";
        const parsed = parseLinkHeader(hdr);
        expect_eq(parsed.count, 2 as usize);

        const next_entry = parsed.getByRel("next");
        expect_eq(next_entry != null, true);
        expect_eq(next_entry.?.url, "https://example.com/p2");
        expect_eq(next_entry.?.title, "Part 2, Section A; Revised");

        const prefetch_entry = parsed.getByRel("prefetch");
        expect_eq(prefetch_entry != null, true);
        expect_eq(prefetch_entry.?.url, "https://example.com/p2");

        expect_eq(parsed.lastUrl() ?? "", "https://example.com/p9");
    });

    test("3. @cap/link-header — Multiple parameters (title, type, hreflang) & entryAt", () => {
        const hdr = "<https://example.com/docs/zh>; rel=\"alternate\"; title=\"Chinese Docs\"; type=\"text/html\"; hreflang=\"zh-CN\"";
        const parsed = parseLinkHeader(hdr);
        expect_eq(parsed.count, 1 as usize);

        const e0 = parsed.entryAt(0);
        expect_eq(e0 != null, true);
        expect_eq(e0.?.url, "https://example.com/docs/zh");
        expect_eq(e0.?.rel, "alternate");
        expect_eq(e0.?.title, "Chinese Docs");
        expect_eq(e0.?.type_val, "text/html");
        expect_eq(e0.?.hreflang, "zh-CN");

        expect_eq(parsed.entryAt(1) == null, true);
        expect_eq(parsed.getByRel("missing") == null, true);
    });

    test("4. @cap/link-header — Empty & malformed headers safely ignored", () => {
        const empty = parseLinkHeader("");
        expect_eq(empty.count, 0 as usize);
        expect_eq(empty.nextUrl() == null, true);

        const malformed = parseLinkHeader("garbage without angle, <>; rel=bad, <https://ok.dev/1>; rel=next, <unterminated");
        expect_eq(malformed.count, 1 as usize);
        expect_eq(malformed.nextUrl() ?? "", "https://ok.dev/1");
    });

    test("5. @cap/link-header — formatLinkHeader round-trip", () => {
        let entries: [2]LinkEntry = [
            LinkEntry{
                url: "https://api.capdux.org/v1/items?page=2",
                rel: "next",
                title: "Next Page",
                type_val: "application/json",
                hreflang: "",
            },
            LinkEntry{
                url: "https://api.capdux.org/v1/items?page=10",
                rel: "last",
                title: "",
                type_val: "",
                hreflang: "en",
            },
        ];
        let out: [512]u8 = undefined;
        const formatted = formatLinkHeader(entries[0..2], out[0..512]);
        expect_eq(
            formatted,
            "<https://api.capdux.org/v1/items?page=2>; rel=\"next\"; title=\"Next Page\"; type=\"application/json\", "
                ++ "<https://api.capdux.org/v1/items?page=10>; rel=\"last\"; hreflang=\"en\"",
        );

        const reparsed = parseLinkHeader(formatted);
        expect_eq(reparsed.count, 2 as usize);
        expect_eq(reparsed.nextUrl() ?? "", "https://api.capdux.org/v1/items?page=2");
        expect_eq(reparsed.lastUrl() ?? "", "https://api.capdux.org/v1/items?page=10");
        expect_eq(reparsed.getByRel("next").?.type_val, "application/json");
        expect_eq(reparsed.getByRel("last").?.hreflang, "en");
    });

    test("6. @cap/link-header — formatPaginationLinks (first, middle, last, query preservation)", () => {
        let out: [512]u8 = undefined;

        // First page of 5 -> next & last only
        const p1 = formatPaginationLinks("https://api.example.com/posts", 1, 5, out[0..512]);
        expect_eq(
            p1,
            "<https://api.example.com/posts?page=2>; rel=\"next\", <https://api.example.com/posts?page=5>; rel=\"last\"",
        );

        // Middle page with existing query string -> preserves '&page='
        const p3 = formatPaginationLinks("https://api.example.com/posts?limit=20", 3, 5, out[0..512]);
        const parsed_p3 = parseLinkHeader(p3);
        expect_eq(parsed_p3.count, 4 as usize);
        expect_eq(parsed_p3.prevUrl() ?? "", "https://api.example.com/posts?limit=20&page=2");
        expect_eq(parsed_p3.nextUrl() ?? "", "https://api.example.com/posts?limit=20&page=4");
        expect_eq(parsed_p3.lastUrl() ?? "", "https://api.example.com/posts?limit=20&page=5");
        expect_eq(parsed_p3.firstUrl() ?? "", "https://api.example.com/posts?limit=20&page=1");

        // Last page -> prev & first only
        const p5 = formatPaginationLinks("https://api.example.com/posts", 5, 5, out[0..512]);
        expect_eq(
            p5,
            "<https://api.example.com/posts?page=4>; rel=\"prev\", <https://api.example.com/posts?page=1>; rel=\"first\"",
        );

        // Single page -> empty
        expect_eq(formatPaginationLinks("https://api.example.com/posts", 1, 1, out[0..512]), "");
    });
}
