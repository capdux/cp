import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MAX_HTML_MANIFEST_FILES,
    HtmlManifestLoader,
    HtmlManifestFile,
    encodeHashEtag,
    HtmlImportManifest,
} from "cap:bundler";

fn main() {
    test("html_manifest: encodeHashEtag produces deterministic 11-char base64url", () => {
        let b1: [11]u8 = [0; 11];
        let b2: [11]u8 = [0; 11];
        let b3: [11]u8 = [0; 11];
        const e1 = encodeHashEtag(0x0102030405060708, &b1);
        const e2 = encodeHashEtag(0x0102030405060708, &b2);
        const e3 = encodeHashEtag(0x1122334455667788, &b3);

        expect_eq(e1.len, 11 as usize);
        expect_eq(std.mem.eql(u8, e1, e2), true);
        expect_eq(std.mem.eql(u8, e1, e3), false);

        // Verify URL-safe base64 alphabet only (no '+', '/', '=')
        let i: usize = 0;
        while (i < e1.len) : (i += 1) {
            const c = e1[i];
            expect_eq(c != '+' && c != '/' && c != '=', true);
        }
    });

    test("html_manifest: loader contentType & asset extension MIME derivation", () => {
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.js.contentType("./a.js"), "text/javascript;charset=utf-8"), true);
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.css.contentType("./a.css"), "text/css;charset=utf-8"), true);
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.html.contentType("./index.html"), "text/html;charset=utf-8"), true);
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.json.contentType("./data.json"), "application/json;charset=utf-8"), true);
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.wasm.contentType("./mod.wasm"), "application/wasm"), true);

        expect_eq(std.mem.eql(u8, HtmlManifestLoader.file.contentType("./logo.SVG"), "image/svg+xml"), true);
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.file.contentType("./pic.png"), "image/png"), true);
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.file.contentType("./font.woff2"), "font/woff2"), true);
        expect_eq(std.mem.eql(u8, HtmlManifestLoader.file.contentType("./raw.bin"), "application/octet-stream"), true);
    });

    test("html_manifest: full manifest JSON serialization with etag & optional input", () => {
        let m = HtmlImportManifest.init("./index.html");
        expect_eq(m.addFile(HtmlManifestFile{
            input: "index.html",
            path: "./index-abc.js",
            loader: HtmlManifestLoader.js,
            is_entry: true,
            hash: 0x123456789abcdef0,
        }), true);
        expect_eq(m.addFile(HtmlManifestFile{
            input: "",
            path: "./logo.svg",
            loader: HtmlManifestLoader.file,
            is_entry: false,
            hash: 0, // hash == 0 omits etag
        }), true);
        expect_eq(m.count(), 2 as usize);

        let buf: [1024]u8 = [0; 1024];
        const json = m.toJson(buf[0..]).?;
        expect_eq(std.mem.indexOf(u8, json, "\"index\":\"./index.html\"") != null, true);
        expect_eq(std.mem.indexOf(u8, json, "\"input\":\"index.html\",\"path\":\"./index-abc.js\"") != null, true);
        expect_eq(std.mem.indexOf(u8, json, "\"loader\":\"js\",\"isEntry\":true") != null, true);
        expect_eq(std.mem.indexOf(u8, json, "\"etag\":\"") != null, true);
        expect_eq(std.mem.indexOf(u8, json, "\"content-type\":\"text/javascript;charset=utf-8\"") != null, true);
        // Second file has no input and no etag
        expect_eq(std.mem.indexOf(u8, json, "{\"path\":\"./logo.svg\",\"loader\":\"file\",\"isEntry\":false,\"headers\":{\"content-type\":\"image/svg+xml\"}}") != null, true);
    });

    test("html_manifest: findByPath & deduplication on duplicate path", () => {
        let m = HtmlImportManifest.init("./app.html");
        expect_eq(m.addFile(HtmlManifestFile{
            path: "./app.css",
            loader: HtmlManifestLoader.css,
            is_entry: false,
            hash: 1,
        }), true);
        expect_eq(m.addFile(HtmlManifestFile{
            path: "./app.css",
            loader: HtmlManifestLoader.css,
            is_entry: true,
            hash: 2,
        }), true);
        expect_eq(m.count(), 1 as usize);

        const found: HtmlManifestFile = m.findByPath("./app.css").?;
        expect_eq(found.is_entry, true);
        expect_eq(found.hash, 2 as u64);
        expect_eq(m.findByPath("./missing.css") == null, true);
        expect_eq(m.addFile(HtmlManifestFile{ path: "" }), false);
    });

    test("html_manifest: RFC 8259 JSON escaping & buffer overflow protection", () => {
        let m = HtmlImportManifest.init("./dir/\"quoted\"\\index.html");
        expect_eq(m.addFile(HtmlManifestFile{
            input: "a\nb\tc",
            path: "./out.js",
            loader: HtmlManifestLoader.js,
            is_entry: true,
            hash: 42,
        }), true);

        let buf: [512]u8 = [0; 512];
        const json = m.toJson(buf[0..]).?;
        expect_eq(std.mem.indexOf(u8, json, "\\\"quoted\\\"\\\\index.html") != null, true);
        expect_eq(std.mem.indexOf(u8, json, "a\\nb\\tc") != null, true);

        // Buffer too small safely returns null
        let tiny: [32]u8 = [0; 32];
        expect_eq(m.toJson(tiny[0..]) == null, true);
    });

    test("html_manifest: capacity limit MAX_HTML_MANIFEST_FILES (64) backpressure", () => {
        let m = HtmlImportManifest.init("./index.html");
        let paths: [64][6]u8 = [[0; 6]; 64];
        let i: usize = 0;
        while (i < MAX_HTML_MANIFEST_FILES) : (i += 1) {
            paths[i][0] = '/';
            paths[i][1] = 'f';
            paths[i][2] = @as(u8, 'a' + @as(u8, @intCast(i / 26)));
            paths[i][3] = @as(u8, 'a' + @as(u8, @intCast(i % 26)));
            paths[i][4] = '.';
            paths[i][5] = 'j';
            expect_eq(m.addFile(HtmlManifestFile{ path: paths[i][0..6], loader: HtmlManifestLoader.js }), true);
        }
        expect_eq(m.count(), MAX_HTML_MANIFEST_FILES);
        expect_eq(m.addFile(HtmlManifestFile{ path: "/overflow.js", loader: HtmlManifestLoader.js }), false);
    });
}
