import { test, expect_eq } from "cap:test";
import {
    parseMediaType, formatMediaType, testMediaType, mediaTypeEq
} from "../../packages/media-typer/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/media-typer — Parse standard media types", () => {
        let m = parseMediaType("application/json");
        expect_eq(m.valid, true);
        expect_eq(m.main_type, "application");
        expect_eq(m.subtype, "json");
        expect_eq(m.suffix, "");
    });

    test("2. @cap/media-typer — Parse structured syntax suffix (+xml / +json)", () => {
        let m = parseMediaType("image/svg+xml");
        expect_eq(m.valid, true);
        expect_eq(m.main_type, "image");
        expect_eq(m.subtype, "svg");
        expect_eq(m.suffix, "xml");

        let m2 = parseMediaType("application/vnd.api+json");
        expect_eq(m2.valid, true);
        expect_eq(m2.subtype, "vnd.api");
        expect_eq(m2.suffix, "json");
    });

    test("3. @cap/media-typer — formatMediaType normalizes to lowercase", () => {
        let out: [64]u8 = undefined;
        let n1 = formatMediaType("Image", "SVG", "XML", out[0..64]);
        expect_eq(eqBuf(out[0..n1], n1, "image/svg+xml"), true);
        let n2 = formatMediaType("TEXT", "HTML", "", out[0..64]);
        expect_eq(eqBuf(out[0..n2], n2, "text/html"), true);
    });

    test("4. @cap/media-typer — Rejects malformed RFC 6838 media types", () => {
        expect_eq(testMediaType(""), false);
        expect_eq(testMediaType("text"), false);
        expect_eq(testMediaType("text/"), false);
        expect_eq(testMediaType("/plain"), false);
        expect_eq(testMediaType("text/plain/extra"), false);
        expect_eq(testMediaType("text/plain; charset=utf-8"), false);
        expect_eq(testMediaType("-bad/type"), false);
    });

    test("5. @cap/media-typer — formatMediaType rejects invalid components", () => {
        let out: [64]u8 = undefined;
        expect_eq(formatMediaType("", "json", "", out[0..64]), 0);
        expect_eq(formatMediaType("text", "bad type", "", out[0..64]), 0);
        expect_eq(formatMediaType("image", "svg", "bad!", out[0..64]) > 0, true);
    });

    test("6. @cap/media-typer — Case-insensitive mediaTypeEq via @cap/unicase", () => {
        expect_eq(mediaTypeEq("APPLICATION/VND.API+JSON", "application/vnd.api+json"), true);
        expect_eq(mediaTypeEq("text/html", "text/plain"), false);
        expect_eq(mediaTypeEq("invalid", "invalid"), false);
    });
}
