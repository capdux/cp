import std from "std";
import { test, expect_eq } from "cap:test";
import { MimeCategory, ContentType, mimeFromExtension, categoryFromMime } from "cap:mime";

test("1. mimeFromExtension handles bare extensions, full paths, query strings, and case-insensitivity", () => {
    expect_eq(std.mem.eql(u8, mimeFromExtension("html"), "text/html; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension(".CSS"), "text/css; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("/var/www/app/main.min.MJS?v=42#top"), "text/javascript; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("C:\\assets\\hero.WEBP"), "image/webp"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("module.wasm"), "application/wasm"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("archive.tar.zst"), "application/zstd"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("fonts/inter.woff2"), "font/woff2"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("src/main.cp"), "text/plain; charset=utf-8"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("deploy.sh"), "application/x-sh"), true);
});

test("2. mimeFromExtension falls back to application/octet-stream on unknown or missing extensions", () => {
    expect_eq(std.mem.eql(u8, mimeFromExtension(""), "application/octet-stream"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("Makefile"), "application/octet-stream"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("/usr/bin/cap"), "application/octet-stream"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("file.unknown_ext"), "application/octet-stream"), true);
    expect_eq(std.mem.eql(u8, mimeFromExtension("trailing."), "application/octet-stream"), true);
});

test("3. ContentType.parse extracts media_type, charset, and quoted boundary with embedded semicolons", () => {
    const ct1 = ContentType.parse("TEXT/HTML; Charset=\"UTF-8\"") ?? unreachable;
    expect_eq(std.mem.eql(u8, ct1.media_type, "TEXT/HTML"), true);
    expect_eq(std.mem.eql(u8, ct1.type_part, "TEXT"), true);
    expect_eq(std.mem.eql(u8, ct1.subtype, "HTML"), true);
    expect_eq(std.mem.eql(u8, ct1.charset ?? unreachable, "UTF-8"), true);
    expect_eq(ct1.boundary == null, true);
    expect_eq(ct1.isText(), true);
    expect_eq(ct1.isJson(), false);
    expect_eq(ct1.isMultipart(), false);
    expect_eq(ct1.isCompressible(), true);

    const ct2 = ContentType.parse("multipart/form-data; BOUNDARY=\"---abc;123\"; charset=utf-8") ?? unreachable;
    expect_eq(ct2.isMultipart(), true);
    expect_eq(std.mem.eql(u8, ct2.boundary ?? unreachable, "---abc;123"), true);
    expect_eq(std.mem.eql(u8, ct2.charset ?? unreachable, "utf-8"), true);
    expect_eq(ct2.isCompressible(), false);

    // 畸形 Content-Type 拒绝
    expect_eq(ContentType.parse("") == null, true);
    expect_eq(ContentType.parse("text") == null, true);
    expect_eq(ContentType.parse("text/") == null, true);
    expect_eq(ContentType.parse("/html") == null, true);
    expect_eq(ContentType.parse("text/html/extra") == null, true);
});

test("4. ContentType isJson and isCompressible cover structured suffixes and binary exclusions", () => {
    const json_std = ContentType.parse("application/json; charset=utf-8") ?? unreachable;
    expect_eq(json_std.isJson(), true);
    expect_eq(json_std.isCompressible(), true);

    const json_ld = ContentType.parse("application/problem+json") ?? unreachable;
    expect_eq(json_ld.isJson(), true);
    expect_eq(json_ld.isCompressible(), true);

    const svg = ContentType.parse("image/svg+xml") ?? unreachable;
    expect_eq(svg.isText(), false);
    expect_eq(svg.isCompressible(), true);

    const wasm = ContentType.parse("application/wasm") ?? unreachable;
    expect_eq(wasm.isCompressible(), true);

    const png = ContentType.parse("image/png") ?? unreachable;
    expect_eq(png.isCompressible(), false);

    const woff2 = ContentType.parse("font/woff2") ?? unreachable;
    expect_eq(woff2.isCompressible(), false);

    const gzip = ContentType.parse("application/gzip") ?? unreachable;
    expect_eq(gzip.isCompressible(), false);
});

test("5. categoryFromMime maps all 12 MimeCategory buckets accurately", () => {
    expect_eq(categoryFromMime("text/plain; charset=utf-8") == MimeCategory.text, true);
    expect_eq(categoryFromMime("text/javascript; charset=utf-8") == MimeCategory.javascript, true);
    expect_eq(categoryFromMime("application/javascript") == MimeCategory.javascript, true);
    expect_eq(categoryFromMime("application/json") == MimeCategory.json, true);
    expect_eq(categoryFromMime("application/ld+json") == MimeCategory.json, true);
    expect_eq(categoryFromMime("application/wasm") == MimeCategory.wasm, true);
    expect_eq(categoryFromMime("image/avif") == MimeCategory.image, true);
    expect_eq(categoryFromMime("audio/flac") == MimeCategory.audio, true);
    expect_eq(categoryFromMime("video/mp4") == MimeCategory.video, true);
    expect_eq(categoryFromMime("font/woff2") == MimeCategory.font, true);
    expect_eq(categoryFromMime("application/zstd") == MimeCategory.archive, true);
    expect_eq(categoryFromMime("application/pdf") == MimeCategory.application, true);
    expect_eq(categoryFromMime("multipart/form-data; boundary=x") == MimeCategory.multipart, true);
    expect_eq(categoryFromMime("model/gltf-binary") == MimeCategory.other, true);
    expect_eq(categoryFromMime("not-a-mime") == MimeCategory.other, true);
});
