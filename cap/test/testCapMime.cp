// Comprehensive Unit Test Suite for @cap/mime
// Covers extension & path lookup, case-insensitivity, reverse MIME-to-ext mapping,
// parameter stripping, default charset, compressibility heuristics, Content-Type formatting,
// and small-buffer / invalid-input protection.
import {
    MimeError,
    MimeInfo,
    lookupInfo,
    lookupInfoByMime,
    lookupByExt,
    lookupOrOctetStream,
    extensionForMime,
    defaultCharset,
    isCompressible,
    contentTypeHeader,
} from "../../packages/@cap/mime/src/index.cp";

fn assert(cond: bool) {
    if (!cond) {
        @panic("@cap/mime test assertion failed");
    }
}

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn expectOptStr(actual: ([]const u8)?, expected: []const u8) {
    if (v := actual) {
        assert(sliceEql(v, expected));
    } else {
        @panic("@cap/mime expected non-null string");
    }
}

fn expectHeaderErr(input: []const u8, out: []u8, expected: MimeError) {
    let matched = false;
    * = contentTypeHeader(input, out) catch e => {
        assert(e == expected);
        matched = true;
    };
    assert(matched);
}

fn testLookupWebAndImages() {
    // Bare extensions, dotted extensions, paths, and case-insensitivity
    expectOptStr(lookupByExt("html"), "text/html");
    expectOptStr(lookupByExt(".HTM"), "text/html");
    expectOptStr(lookupByExt("/var/www/index.html"), "text/html");
    expectOptStr(lookupByExt("css"), "text/css");
    expectOptStr(lookupByExt("/a/b/file.min.js"), "text/javascript");
    expectOptStr(lookupByExt("mjs"), "text/javascript");
    expectOptStr(lookupByExt("cjs"), "text/javascript");
    expectOptStr(lookupByExt("tsx"), "text/typescript");
    expectOptStr(lookupByExt("json"), "application/json");
    expectOptStr(lookupByExt(".JSON"), "application/json");
    expectOptStr(lookupByExt("bundle.js.map"), "application/json");
    expectOptStr(lookupByExt("xml"), "application/xml");
    expectOptStr(lookupByExt("svg"), "image/svg+xml");
    expectOptStr(lookupByExt("wasm"), "application/wasm");
    expectOptStr(lookupByExt("txt"), "text/plain");
    expectOptStr(lookupByExt("md"), "text/markdown");
    expectOptStr(lookupByExt("csv"), "text/csv");
    expectOptStr(lookupByExt("yaml"), "application/yaml");
    expectOptStr(lookupByExt("YML"), "application/yaml");
    expectOptStr(lookupByExt("toml"), "application/toml");

    // Images
    expectOptStr(lookupByExt("ico"), "image/vnd.microsoft.icon");
    expectOptStr(lookupByExt("webp"), "image/webp");
    expectOptStr(lookupByExt("avif"), "image/avif");
    expectOptStr(lookupByExt("png"), "image/png");
    expectOptStr(lookupByExt("jpg"), "image/jpeg");
    expectOptStr(lookupByExt("JPEG"), "image/jpeg");
    expectOptStr(lookupByExt("gif"), "image/gif");
    expectOptStr(lookupByExt("bmp"), "image/bmp");
    expectOptStr(lookupByExt("tiff"), "image/tiff");

    // Windows path separators
    expectOptStr(lookupByExt("C:\\Users\\demo\\assets\\logo.PNG"), "image/png");
}

fn testLookupFontsMediaDocsArchivesCode() {
    // Fonts
    expectOptStr(lookupByExt("woff"), "font/woff");
    expectOptStr(lookupByExt("woff2"), "font/woff2");
    expectOptStr(lookupByExt("ttf"), "font/ttf");
    expectOptStr(lookupByExt("otf"), "font/otf");
    expectOptStr(lookupByExt("eot"), "application/vnd.ms-fontobject");

    // Audio & Video
    expectOptStr(lookupByExt("mp3"), "audio/mpeg");
    expectOptStr(lookupByExt("wav"), "audio/wav");
    expectOptStr(lookupByExt("ogg"), "audio/ogg");
    expectOptStr(lookupByExt("flac"), "audio/flac");
    expectOptStr(lookupByExt("aac"), "audio/aac");
    expectOptStr(lookupByExt("m4a"), "audio/mp4");
    expectOptStr(lookupByExt("mp4"), "video/mp4");
    expectOptStr(lookupByExt("webm"), "video/webm");
    expectOptStr(lookupByExt("mkv"), "video/x-matroska");
    expectOptStr(lookupByExt("avi"), "video/x-msvideo");
    expectOptStr(lookupByExt("mov"), "video/quicktime");
    expectOptStr(lookupByExt("m3u8"), "application/vnd.apple.mpegurl");
    expectOptStr(lookupByExt("ts"), "video/mp2t");

    // Documents, Archives & Binaries
    expectOptStr(lookupByExt("pdf"), "application/pdf");
    expectOptStr(lookupByExt("zip"), "application/zip");
    expectOptStr(lookupByExt("gz"), "application/gzip");
    expectOptStr(lookupByExt("tar"), "application/x-tar");
    expectOptStr(lookupByExt("tgz"), "application/gzip");
    expectOptStr(lookupByExt("bz2"), "application/x-bzip2");
    expectOptStr(lookupByExt("xz"), "application/x-xz");
    expectOptStr(lookupByExt("zst"), "application/zstd");
    expectOptStr(lookupByExt("7z"), "application/x-7z-compressed");
    expectOptStr(lookupByExt("rar"), "application/vnd.rar");
    expectOptStr(lookupByExt("bin"), "application/octet-stream");
    expectOptStr(lookupByExt("exe"), "application/vnd.microsoft.portable-executable");
    expectOptStr(lookupByExt("dll"), "application/octet-stream");
    expectOptStr(lookupByExt("so"), "application/octet-stream");
    expectOptStr(lookupByExt("dylib"), "application/octet-stream");
    expectOptStr(lookupByExt("dmg"), "application/x-apple-diskimage");
    expectOptStr(lookupByExt("deb"), "application/vnd.debian.binary-package");
    expectOptStr(lookupByExt("rpm"), "application/x-redhat-package-manager");
    expectOptStr(lookupByExt("apk"), "application/vnd.android.package-archive");
    expectOptStr(lookupByExt("doc"), "application/msword");
    expectOptStr(lookupByExt("docx"), "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
    expectOptStr(lookupByExt("xls"), "application/vnd.ms-excel");
    expectOptStr(lookupByExt("xlsx"), "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
    expectOptStr(lookupByExt("ppt"), "application/vnd.ms-powerpoint");
    expectOptStr(lookupByExt("pptx"), "application/vnd.openxmlformats-officedocument.presentationml.presentation");
    expectOptStr(lookupByExt("epub"), "application/epub+zip");
    expectOptStr(lookupByExt("rtf"), "application/rtf");
    expectOptStr(lookupByExt("ics"), "text/calendar");
    expectOptStr(lookupByExt("vcf"), "text/vcard");

    // Source code & schema files
    expectOptStr(lookupByExt("sql"), "application/sql");
    expectOptStr(lookupByExt("graphql"), "application/graphql");
    expectOptStr(lookupByExt("proto"), "text/x-protobuf");
    expectOptStr(lookupByExt("cp"), "text/x-cp");
    expectOptStr(lookupByExt("zig"), "text/x-zig");
    expectOptStr(lookupByExt("rs"), "text/x-rust");
    expectOptStr(lookupByExt("py"), "text/x-python");
    expectOptStr(lookupByExt("go"), "text/x-go");
    expectOptStr(lookupByExt("c"), "text/x-c");
    expectOptStr(lookupByExt("h"), "text/x-c-header");
    expectOptStr(lookupByExt("cpp"), "text/x-c++");
    expectOptStr(lookupByExt("sh"), "application/x-sh");
}

fn testLookupEdgeCasesAndFallback() {
    assert(lookupByExt("") == null);
    assert(lookupByExt("   ") == null);
    assert(lookupByExt(".") == null);
    assert(lookupByExt("file.") == null);
    assert(lookupByExt("/a/b/") == null);
    assert(lookupByExt("/a/b/Makefile") == null);
    assert(lookupByExt("dir.with.dot/README") == null);
    assert(lookupByExt("/home/user/.json") == null);
    assert(lookupByExt("unknown_xyz") == null);

    // Fallback to application/octet-stream
    assert(sliceEql(lookupOrOctetStream("unknown_xyz"), "application/octet-stream"));
    assert(sliceEql(lookupOrOctetStream("/a/b/Makefile"), "application/octet-stream"));
    assert(sliceEql(lookupOrOctetStream("/a/b/data.json"), "application/json"));

    // Full MimeInfo inspection
    if (info := lookupInfo("svg")) {
        assert(sliceEql(info.mime, "image/svg+xml"));
        assert(sliceEql(info.default_ext, "svg"));
        assert(info.compressible);
        assert(info.utf8_charset);
    } else {
        @panic("expected svg MimeInfo");
    }
}

fn testReverseExtensionForMime() {
    expectOptStr(extensionForMime("text/html"), "html");
    expectOptStr(extensionForMime("TEXT/HTML; charset=utf-8"), "html");
    expectOptStr(extensionForMime("  application/json ; charset=UTF-8 "), "json");
    expectOptStr(extensionForMime("text/javascript"), "js");
    expectOptStr(extensionForMime("application/javascript"), "js");
    expectOptStr(extensionForMime("application/javascript; charset=utf-8"), "js");
    expectOptStr(extensionForMime("image/jpeg"), "jpg");
    expectOptStr(extensionForMime("image/svg+xml"), "svg");
    expectOptStr(extensionForMime("application/wasm"), "wasm");
    expectOptStr(extensionForMime("text/xml"), "xml");
    expectOptStr(extensionForMime("text/yaml"), "yaml");
    expectOptStr(extensionForMime("image/x-icon"), "ico");
    expectOptStr(extensionForMime("application/zstd"), "zst");
    expectOptStr(extensionForMime("video/mp2t"), "ts");

    assert(extensionForMime("") == null);
    assert(extensionForMime("application/x-unknown-type") == null);
    assert(extensionForMime("application/x-www-form-urlencoded") == null);

    if (info := lookupInfoByMime("application/javascript; charset=utf-8")) {
        assert(sliceEql(info.default_ext, "js"));
        assert(info.compressible);
        assert(info.utf8_charset);
    } else {
        @panic("expected alias lookupInfoByMime hit");
    }
}

fn testDefaultCharset() {
    expectOptStr(defaultCharset("text/plain"), "utf-8");
    expectOptStr(defaultCharset("text/html; charset=utf-8"), "utf-8");
    expectOptStr(defaultCharset("text/x-custom-log"), "utf-8");
    expectOptStr(defaultCharset("application/json"), "utf-8");
    expectOptStr(defaultCharset("APPLICATION/JAVASCRIPT"), "utf-8");
    expectOptStr(defaultCharset("text/javascript"), "utf-8");
    expectOptStr(defaultCharset("application/xml"), "utf-8");
    expectOptStr(defaultCharset("image/svg+xml"), "utf-8");
    expectOptStr(defaultCharset("application/yaml"), "utf-8");
    expectOptStr(defaultCharset("application/problem+json"), "utf-8");
    expectOptStr(defaultCharset("application/vnd.custom+xml"), "utf-8");

    // Binary types have no default charset
    assert(defaultCharset("application/octet-stream") == null);
    assert(defaultCharset("application/wasm") == null);
    assert(defaultCharset("application/pdf") == null);
    assert(defaultCharset("image/png") == null);
    assert(defaultCharset("image/webp") == null);
    assert(defaultCharset("video/mp4") == null);
    assert(defaultCharset("font/woff2") == null);
    assert(defaultCharset("") == null);
}

fn testIsCompressible() {
    // Compressible types
    assert(isCompressible("text/plain"));
    assert(isCompressible("text/html; charset=utf-8"));
    assert(isCompressible("text/css"));
    assert(isCompressible("text/csv"));
    assert(isCompressible("text/x-custom"));
    assert(isCompressible("application/json"));
    assert(isCompressible("application/javascript"));
    assert(isCompressible("text/javascript"));
    assert(isCompressible("application/xml"));
    assert(isCompressible("image/svg+xml"));
    assert(isCompressible("application/wasm"));
    assert(isCompressible("application/yaml"));
    assert(isCompressible("application/vnd.api+json"));
    assert(isCompressible("application/atom+xml"));
    assert(isCompressible("font/ttf"));
    assert(isCompressible("font/otf"));

    // Non-compressible (already compressed or binary)
    assert(!isCompressible("image/png"));
    assert(!isCompressible("image/jpeg"));
    assert(!isCompressible("image/webp"));
    assert(!isCompressible("image/avif"));
    assert(!isCompressible("font/woff2"));
    assert(!isCompressible("application/zip"));
    assert(!isCompressible("application/gzip"));
    assert(!isCompressible("application/zstd"));
    assert(!isCompressible("video/mp4"));
    assert(!isCompressible("audio/mpeg"));
    assert(!isCompressible("application/octet-stream"));
    assert(!isCompressible("invalid-mime"));
    assert(!isCompressible(""));
}

fn testContentTypeHeaderAndBuffers() {
    let buf: [128]u8 = [0; 128];

    // Extensions and paths
    assert(sliceEql(contentTypeHeader("html", buf[0..]) catch unreachable, "text/html; charset=utf-8"));
    assert(sliceEql(contentTypeHeader(".json", buf[0..]) catch unreachable, "application/json; charset=utf-8"));
    assert(sliceEql(contentTypeHeader("/static/app.min.js", buf[0..]) catch unreachable, "text/javascript; charset=utf-8"));
    assert(sliceEql(contentTypeHeader("assets/style.css", buf[0..]) catch unreachable, "text/css; charset=utf-8"));
    assert(sliceEql(contentTypeHeader("logo.svg", buf[0..]) catch unreachable, "image/svg+xml; charset=utf-8"));
    assert(sliceEql(contentTypeHeader("photo.png", buf[0..]) catch unreachable, "image/png"));
    assert(sliceEql(contentTypeHeader("module.wasm", buf[0..]) catch unreachable, "application/wasm"));
    assert(sliceEql(contentTypeHeader("archive.zst", buf[0..]) catch unreachable, "application/zstd"));

    // Direct MIME types
    assert(sliceEql(contentTypeHeader("text/plain", buf[0..]) catch unreachable, "text/plain; charset=utf-8"));
    assert(sliceEql(contentTypeHeader("application/json", buf[0..]) catch unreachable, "application/json; charset=utf-8"));
    assert(sliceEql(contentTypeHeader("application/problem+json", buf[0..]) catch unreachable, "application/problem+json; charset=utf-8"));
    assert(sliceEql(contentTypeHeader("image/webp", buf[0..]) catch unreachable, "image/webp"));

    // Preserve existing charset parameter without duplicating
    assert(sliceEql(
        contentTypeHeader("text/html; charset=iso-8859-1", buf[0..]) catch unreachable,
        "text/html; charset=iso-8859-1"
    ));
    assert(sliceEql(
        contentTypeHeader("text/plain; foo=bar; CHARSET=utf-16", buf[0..]) catch unreachable,
        "text/plain; foo=bar; CHARSET=utf-16"
    ));
    // Non-charset parameter still gets charset appended
    assert(sliceEql(
        contentTypeHeader("text/csv; header=present", buf[0..]) catch unreachable,
        "text/csv; header=present; charset=utf-8"
    ));

    // Exact-fit buffer ("image/png" = 9 bytes)
    let exact: [9]u8 = [0; 9];
    assert(sliceEql(contentTypeHeader("png", exact[0..]) catch unreachable, "image/png"));

    // BufferTooSmall protection
    let tiny: [8]u8 = [0; 8];
    expectHeaderErr("png", tiny[0..], MimeError.BufferTooSmall);
    expectHeaderErr("json", tiny[0..], MimeError.BufferTooSmall);
    expectHeaderErr("text/html", tiny[0..], MimeError.BufferTooSmall);
    expectHeaderErr("png", tiny[0..0], MimeError.BufferTooSmall);

    // InvalidMime protection
    expectHeaderErr("", buf[0..], MimeError.InvalidMime);
    expectHeaderErr("   ", buf[0..], MimeError.InvalidMime);
    expectHeaderErr("unknown_ext", buf[0..], MimeError.InvalidMime);
    expectHeaderErr("/a/b/Makefile", buf[0..], MimeError.InvalidMime);
    expectHeaderErr("invalid; charset=utf-8", buf[0..], MimeError.InvalidMime);
}

pub fn main() {
    testLookupWebAndImages();
    testLookupFontsMediaDocsArchivesCode();
    testLookupEdgeCasesAndFallback();
    testReverseExtensionForMime();
    testDefaultCharset();
    testIsCompressible();
    testContentTypeHeaderAndBuffers();
}
