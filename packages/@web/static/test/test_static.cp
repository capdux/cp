// Self-contained unit & real-disk integration tests for @web/static

const std = @import("std");
import { App, AppMethod, Request, Response, RequestArena } from "../../../../lib/web/app.cp";
import {
    StaticConfig,
    sanitizeRelPath,
    detectMimeType,
    computeStrongEtag,
    etagMatches,
    serveStaticRequest,
    serveFileFromDir,
    registerStatic,
} from "../src/index.cp";

fn assertTrue(cond: bool, msg: string) {
    if (!cond) {
        std.debug.print("FAIL: {s}\n", .{msg});
        std.process.exit(1);
    }
}

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn writeTestFile(path: string, data: string): bool {
    let path_buf: [512]u8 = [0; 512];
    if (path.len == 0 || path.len >= 512) return false;
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        path_buf[i] = path[i];
    }
    path_buf[path.len] = 0;
    const path_z: [*:0]const u8 = path_buf[0..path.len :0].ptr;
    const flags = std.c.O{ .ACCMODE = .WRONLY, .CREAT = true, .TRUNC = true, .CLOEXEC = true };
    const fd = std.c.open(path_z, flags, @as(std.c.mode_t, 0o644));
    if (fd < 0) return false;
    let written: usize = 0;
    while (written < data.len) {
        const n = std.c.write(fd, data[written..data.len].ptr, data.len - written);
        if (n <= 0) {
            * = std.c.close(fd);
            return false;
        }
        written += (n as usize);
    }
    * = std.c.close(fd);
    return true;
}

fn makeDir(path: string) {
    let path_buf: [512]u8 = [0; 512];
    let i: usize = 0;
    while (i < path.len && i < 511) : (i += 1) {
        path_buf[i] = path[i];
    }
    path_buf[i] = 0;
    const path_z: [*:0]const u8 = path_buf[0..i :0].ptr;
    * = std.os.linux.mkdir(path_z, 0o755);
}

fn testSanitizeRelPath() {
    let buf: [256]u8 = [0; 256];

    // Valid paths
    const r1 = sanitizeRelPath("css/app.css", buf[0..]);
    assertTrue(r1 != null && strEql(buf[0..r1.?], "css/app.css"), "valid nested path");

    const r2 = sanitizeRelPath("///assets//logo.png/", buf[0..]);
    assertTrue(r2 != null && strEql(buf[0..r2.?], "assets/logo.png"), "normalize redundant slashes");

    const r3 = sanitizeRelPath("", buf[0..]);
    assertTrue(r3 != null && r3.? == 0, "empty path maps to root");

    // Path traversal & dangerous inputs must be rejected (null -> 403)
    assertTrue(sanitizeRelPath("../secret.txt", buf[0..]) == null, "reject leading ..");
    assertTrue(sanitizeRelPath("a/../../etc/passwd", buf[0..]) == null, "reject nested ..");
    assertTrue(sanitizeRelPath("a\\b.txt", buf[0..]) == null, "reject backslash");
    assertTrue(sanitizeRelPath("a%2e%2e/secret", buf[0..]) == null, "reject %2e%2e");
    assertTrue(sanitizeRelPath("a%2Eb", buf[0..]) == null, "reject uppercase %2E");
    assertTrue(sanitizeRelPath("a%2fb", buf[0..]) == null, "reject %2f");
    assertTrue(sanitizeRelPath("a%5cb", buf[0..]) == null, "reject %5c");
    assertTrue(sanitizeRelPath("a%00.png", buf[0..]) == null, "reject %00");

    let nul_raw: [5]u8 = [ 'a', 0, 'b', '.', 'c' ];
    assertTrue(sanitizeRelPath(nul_raw[0..], buf[0..]) == null, "reject raw NUL byte");
}

fn testMimeAndEtag() {
    assertTrue(strEql(detectMimeType("index.html", "hi"), "text/html; charset=utf-8"), "mime html");
    assertTrue(strEql(detectMimeType("APP.JS", "hi"), "application/javascript; charset=utf-8"), "mime js uppercase");
    assertTrue(strEql(detectMimeType("mod.wasm", "hi"), "application/wasm"), "mime wasm");

    // Unknown extension -> fallback to lib/mime/fileType.cp magic-number detection
    let png_magic: [8]u8 = [ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A ];
    assertTrue(strEql(detectMimeType("blob.unknown", png_magic[0..]), "image/png"), "magic fallback png");
    assertTrue(strEql(detectMimeType("readme.bin", "hello"), "application/octet-stream"), "unknown fallback octet-stream");

    let et1: [64]u8 = [0; 64];
    let et2: [64]u8 = [0; 64];
    const l1 = computeStrongEtag("hello world", et1[0..]).?;
    const l2 = computeStrongEtag("hello world", et2[0..]).?;
    assertTrue(strEql(et1[0..l1], et2[0..l2]), "deterministic ETag");
    assertTrue(etagMatches(et1[0..l1], et1[0..l1]), "exact ETag match");
    assertTrue(etagMatches("*", et1[0..l1]), "wildcard ETag match");
    assertTrue(!etagMatches("\"deadbeef-1\"", et1[0..l1]), "mismatched ETag");
}

fn testDiskServeAndAppDispatch() {
    const root = "/data/dev/cp/temp/test_static_root";
    makeDir("/data/dev/cp/temp");
    makeDir(root);

    assertTrue(writeTestFile("/data/dev/cp/temp/test_static_root/index.html", "<h1>Hello CP Static</h1>"), "write index.html");
    assertTrue(writeTestFile("/data/dev/cp/temp/test_static_root/app.css", "body{margin:0;}"), "write app.css");

    let png_magic: [8]u8 = [ 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A ];
    assertTrue(writeTestFile("/data/dev/cp/temp/test_static_root/icon.dat", png_magic[0..]), "write icon.dat");

    const cfg = StaticConfig{
        root_dir: root,
        url_prefix: "/static",
        index_file: "index.html",
        cache_control: "public, max-age=7200",
        enable_etag: true,
    };

    // 1. Direct serveStaticRequest: 200 OK + MIME + ETag + Cache-Control
    let buf: [4096]u8 = [0; 4096];
    const r200 = serveStaticRequest(cfg, "app.css", null, buf[0..]);
    assertTrue(r200.status == 200, "css 200");
    assertTrue(strEql(r200.content_type, "text/css; charset=utf-8"), "css content-type");
    assertTrue(strEql(r200.body, "body{margin:0;}"), "css body");
    const etag_hdr = r200.header("ETag");
    assertTrue(etag_hdr != null, "ETag header present");
    const cc_hdr = r200.header("Cache-Control");
    assertTrue(cc_hdr != null && strEql(cc_hdr.?, "public, max-age=7200"), "Cache-Control header");

    // 2. Conditional GET with matching If-None-Match -> 304 Not Modified
    let saved_etag: [64]u8 = [0; 64];
    const et_s = etag_hdr.?;
    let k: usize = 0;
    while (k < et_s.len) : (k += 1) {
        saved_etag[k] = et_s[k];
    }
    const r304 = serveStaticRequest(cfg, "app.css", saved_etag[0..et_s.len], buf[0..]);
    assertTrue(r304.status == 304, "ETag 304 Not Modified");
    assertTrue(r304.body.len == 0, "304 empty body");

    // 3. Unknown extension -> magic-number PNG MIME
    const r_magic = serveFileFromDir(root, "icon.dat", buf[0..]);
    assertTrue(r_magic.status == 200, "icon.dat 200");
    assertTrue(strEql(r_magic.content_type, "image/png"), "icon.dat magic image/png");

    // 4. 404 Not Found
    const r404 = serveStaticRequest(cfg, "missing.txt", null, buf[0..]);
    assertTrue(r404.status == 404, "missing file 404");

    // 5. 403 Forbidden on traversal
    const r403 = serveStaticRequest(cfg, "../etc/passwd", null, buf[0..]);
    assertTrue(r403.status == 403, "traversal 403");
    const r403_enc = serveStaticRequest(cfg, "%2e%2e/secret", null, buf[0..]);
    assertTrue(r403_enc.status == 403, "encoded traversal 403");

    // 6. Full App.dispatch integration via registerStatic(app, cfg)
    let app = App.init();
    * = registerStatic(*app, cfg);

    const req_idx = Request.init(AppMethod.GET, "/static/index.html");
    const resp_idx = app.dispatch(req_idx);
    assertTrue(resp_idx.status == 200, "app dispatch index.html 200");
    assertTrue(strEql(resp_idx.content_type, "text/html; charset=utf-8"), "app dispatch html mime");
    assertTrue(strEql(resp_idx.body, "<h1>Hello CP Static</h1>"), "app dispatch html body");

    // App dispatch with If-None-Match -> 304
    const idx_etag = resp_idx.header("ETag").?;
    let idx_etag_copy: [64]u8 = [0; 64];
    let j: usize = 0;
    while (j < idx_etag.len) : (j += 1) {
        idx_etag_copy[j] = idx_etag[j];
    }
    let req_304 = Request.init(AppMethod.GET, "/static/index.html");
    * = req_304.withHeader("If-None-Match", idx_etag_copy[0..idx_etag.len]);
    const resp_304 = app.dispatch(req_304);
    assertTrue(resp_304.status == 304, "app dispatch 304");

    // App dispatch traversal -> 403
    const req_403 = Request.init(AppMethod.GET, "/static/%2e%2e/secret.txt");
    const resp_403 = app.dispatch(req_403);
    assertTrue(resp_403.status == 403, "app dispatch 403");

    // App dispatch missing -> 404
    const req_404 = Request.init(AppMethod.GET, "/static/nope.js");
    const resp_404 = app.dispatch(req_404);
    assertTrue(resp_404.status == 404, "app dispatch 404");
}

pub fn main() {
    testSanitizeRelPath();
    testMimeAndEtag();
    testDiskServeAndAppDispatch();
    std.debug.print("ALL @web/static TESTS PASSED!\n", .{});
}
