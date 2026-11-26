import { test, expect_eq } from "cap:test";
import {
    AcceptEntry,
    AcceptHeader,
    negotiateMediaType
} from "../../packages/@cap/accept-header/src/index.cp";

fn main() {
    test("1. @cap/accept-header — parse sorts by quality descending & specificity descending", () => {
        let h: AcceptHeader = AcceptHeader.parse("*/*, text/*, text/html, application/json;q=0.9");
        expect_eq(h.len(), 4);
        expect_eq(h.entries[0].media_range, "text/html");
        expect_eq(h.entries[0].quality_milli, 1000);
        expect_eq(h.entries[0].specificity, 3);

        expect_eq(h.entries[1].media_range, "text/*");
        expect_eq(h.entries[1].specificity, 2);

        expect_eq(h.entries[2].media_range, "*/*");
        expect_eq(h.entries[2].specificity, 1);

        expect_eq(h.entries[3].media_range, "application/json");
        expect_eq(h.entries[3].quality_milli, 900);
        expect_eq(h.entries[3].specificity, 3);
    });

    test("2. @cap/accept-header — filters q=0 entries & ignores malformed media ranges", () => {
        let h: AcceptHeader = AcceptHeader.parse("text/plain;q=0, invalid, */html, application/xml;q=0.0, application/json;q=0.7");
        expect_eq(h.len(), 1);
        expect_eq(h.entries[0].media_range, "application/json");
        expect_eq(h.entries[0].quality_milli, 700);
        expect_eq(h.accepts("text/plain"), false);
        expect_eq(h.accepts("application/json"), true);
    });

    test("3. @cap/accept-header — case-insensitive accepts() & wildcard matching", () => {
        let h: AcceptHeader = AcceptHeader.parse("TEXT/HTML, image/*;q=0.8");
        expect_eq(h.accepts("text/html"), true);
        expect_eq(h.accepts("Text/Html"), true);
        expect_eq(h.accepts("image/png"), true);
        expect_eq(h.accepts("IMAGE/WEBP"), true);
        expect_eq(h.accepts("application/json"), false);
    });

    test("4. @cap/accept-header — bestMatch picks highest quality & specificity", () => {
        let h: AcceptHeader = AcceptHeader.parse("text/html;q=0.8, application/json;q=0.95, */*;q=0.1");
        let avail: [3]string = ["text/plain", "text/html", "application/json"];
        expect_eq(h.bestMatch(avail[0..3]) ?? "", "application/json");

        let avail2: [2]string = ["text/plain", "text/html"];
        expect_eq(h.bestMatch(avail2[0..2]) ?? "", "text/html");

        let avail3: [1]string = ["application/octet-stream"];
        expect_eq(h.bestMatch(avail3[0..1]) ?? "", "application/octet-stream");
    });

    test("5. @cap/accept-header — negotiateMediaType one-shot helper & no-match null", () => {
        let avail: [2]string = ["application/xml", "application/json"];
        expect_eq(negotiateMediaType("application/json;q=0.9, application/xml;q=0.5", avail[0..2]) ?? "", "application/json");
        expect_eq(negotiateMediaType("image/png, image/jpeg", avail[0..2]) == null, true);
        expect_eq(negotiateMediaType("", avail[0..2]) == null, true);
    });

    test("6. @cap/accept-header — specific rule overrides wildcard quality for same candidate", () => {
        let h: AcceptHeader = AcceptHeader.parse("text/*;q=0.5, text/html;q=0.9, */*;q=0.1");
        let avail: [2]string = ["text/plain", "text/html"];
        expect_eq(h.bestMatch(avail[0..2]) ?? "", "text/html");

        let h2: AcceptHeader = AcceptHeader.parse("text/*, text/html");
        let avail2: [2]string = ["text/plain", "text/html"];
        expect_eq(h2.bestMatch(avail2[0..2]) ?? "", "text/html");
    });
}
