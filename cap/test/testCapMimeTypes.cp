import { test, expect_eq } from "cap:test";
import {
    MimeParts,
    lookup,
    extension,
    charset,
    contentType,
    isCompressible,
    parseMimeType,
} from "../../packages/@cap/mime-types/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn optEq(opt: string?, expected: string): bool {
    if (opt == null) {
        return false;
    }
    return strEq(opt ?? "", expected);
}

fn main() {
    test("1. @cap/mime-types — lookup paths, dotted extensions & bare extensions (case-insensitive)", () => {
        expect_eq(optEq(lookup("/var/www/index.HTML"), "text/html"), true);
        expect_eq(optEq(lookup("C:\\assets\\data.JSON"), "application/json"), true);
        expect_eq(optEq(lookup(".mjs"), "text/javascript"), true);
        expect_eq(optEq(lookup("png"), "image/png"), true);
        expect_eq(optEq(lookup("app.wasm"), "application/wasm"), true);
        expect_eq(optEq(lookup("font.woff2"), "font/woff2"), true);
        expect_eq(optEq(lookup("video.m3u8"), "application/vnd.apple.mpegurl"), true);
        expect_eq(optEq(lookup("main.cp"), "text/x-cp"), true);
        expect_eq(optEq(lookup("build.zig"), "text/x-zig"), true);
        expect_eq(optEq(lookup("archive.zstd"), "application/zstd"), true);
        expect_eq(lookup("/no/extension/README") == null, true);
        expect_eq(lookup("unknown_xyz_ext") == null, true);
    });

    test("2. @cap/mime-types — extension reverse lookup with parameters & aliases", () => {
        expect_eq(optEq(extension("text/html; charset=utf-8"), "html"), true);
        expect_eq(optEq(extension("application/json"), "json"), true);
        expect_eq(optEq(extension("application/javascript"), "js"), true);
        expect_eq(optEq(extension("image/svg+xml"), "svg"), true);
        expect_eq(optEq(extension("IMAGE/JPEG"), "jpeg"), true);
        expect_eq(optEq(extension("application/vnd.openxmlformats-officedocument.wordprocessingml.document"), "docx"), true);
        expect_eq(optEq(extension("audio/flac"), "flac"), true);
        expect_eq(extension("application/x-nonexistent") == null, true);
        expect_eq(extension("not-a-mime") == null, true);
    });

    test("3. @cap/mime-types — charset detection for text/*, structured +json/+xml & binary types", () => {
        expect_eq(optEq(charset("text/plain"), "UTF-8"), true);
        expect_eq(optEq(charset("text/css"), "UTF-8"), true);
        expect_eq(optEq(charset("application/json"), "UTF-8"), true);
        expect_eq(optEq(charset("application/javascript"), "UTF-8"), true);
        expect_eq(optEq(charset("application/xml"), "UTF-8"), true);
        expect_eq(optEq(charset("image/svg+xml"), "UTF-8"), true);
        expect_eq(optEq(charset("application/problem+json"), "UTF-8"), true);
        expect_eq(optEq(charset("application/atom+xml"), "UTF-8"), true);
        expect_eq(charset("image/png") == null, true);
        expect_eq(charset("application/octet-stream") == null, true);
        expect_eq(charset("font/woff2") == null, true);
    });

    test("4. @cap/mime-types — contentType auto-appends charset & preserves existing params", () => {
        let out: [128]u8 = undefined;

        const n1 = contentType("html", &out);
        expect_eq(strEq(out[0..n1], "text/html; charset=utf-8"), true);

        const n2 = contentType("/static/api.json", &out);
        expect_eq(strEq(out[0..n2], "application/json; charset=utf-8"), true);

        const n3 = contentType("image/png", &out);
        expect_eq(strEq(out[0..n3], "image/png"), true);

        const n4 = contentType("text/html; charset=iso-8859-1", &out);
        expect_eq(strEq(out[0..n4], "text/html; charset=iso-8859-1"), true);

        const n5 = contentType("unknown_ext", &out);
        expect_eq(n5, 0);
    });

    test("5. @cap/mime-types — isCompressible distinguishes compressible vs pre-compressed formats", () => {
        expect_eq(isCompressible("text/html; charset=utf-8"), true);
        expect_eq(isCompressible("application/json"), true);
        expect_eq(isCompressible("application/javascript"), true);
        expect_eq(isCompressible("image/svg+xml"), true);
        expect_eq(isCompressible("application/wasm"), true);
        expect_eq(isCompressible("application/ld+json"), true);
        expect_eq(isCompressible("font/ttf"), true);

        expect_eq(isCompressible("image/png"), false);
        expect_eq(isCompressible("image/jpeg"), false);
        expect_eq(isCompressible("image/webp"), false);
        expect_eq(isCompressible("application/zip"), false);
        expect_eq(isCompressible("application/gzip"), false);
        expect_eq(isCompressible("video/mp4"), false);
        expect_eq(isCompressible("font/woff2"), false);
        expect_eq(isCompressible("invalid"), false);
    });

    test("6. @cap/mime-types — parseMimeType splits essence, type_part, subtype & quoted charset", () => {
        const p1: MimeParts = parseMimeType("  text/html; version=5; charset=\"UTF-8\"  ");
        expect_eq(p1.valid, true);
        expect_eq(strEq(p1.essence, "text/html"), true);
        expect_eq(strEq(p1.type_part, "text"), true);
        expect_eq(strEq(p1.subtype, "html"), true);
        expect_eq(strEq(p1.charset_val, "UTF-8"), true);

        const p2: MimeParts = parseMimeType("application/vnd.api+json");
        expect_eq(p2.valid, true);
        expect_eq(strEq(p2.essence, "application/vnd.api+json"), true);
        expect_eq(strEq(p2.type_part, "application"), true);
        expect_eq(strEq(p2.subtype, "vnd.api+json"), true);
        expect_eq(p2.charset_val.len, 0);

        const bad1: MimeParts = parseMimeType("");
        expect_eq(bad1.valid, false);
        const bad2: MimeParts = parseMimeType("text/html/extra");
        expect_eq(bad2.valid, false);
        const bad3: MimeParts = parseMimeType("text/");
        expect_eq(bad3.valid, false);
    });
}
