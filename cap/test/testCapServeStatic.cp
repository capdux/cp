import { test, expect_eq } from "cap:test";
import {
    ServeStaticOptions, resolveStaticRequest, hasDotfileSegment,
    formatCacheControl, formatStaticEtag, formatStaticContentType,
    DOTFILE_IGNORE, DOTFILE_DENY, DOTFILE_ALLOW,
    STATIC_ACTION_SERVE, STATIC_ACTION_FORBIDDEN, STATIC_ACTION_NOT_FOUND, STATIC_ACTION_METHOD_NOT_ALLOWED
} from "../../packages/@cap/serve-static/src/index.cp";

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
    test("1. @cap/serve-static — Resolves static file & trailing-slash index.html", () => {
        let opts = ServeStaticOptions.defaultOpts();
        let out: [128]u8 = undefined;
        let r1 = resolveStaticRequest("GET", "/assets/app.js?v=2", opts, out[0..128]);
        expect_eq(r1.status, STATIC_ACTION_SERVE);
        expect_eq(eqBuf(out[0..r1.rel_path_len], r1.rel_path_len, "assets/app.js"), true);
        expect_eq(r1.mime_type, "text/javascript");

        let r2 = resolveStaticRequest("HEAD", "/docs/", opts, out[0..128]);
        expect_eq(r2.status, STATIC_ACTION_SERVE);
        expect_eq(eqBuf(out[0..r2.rel_path_len], r2.rel_path_len, "docs/index.html"), true);
        expect_eq(r2.mime_type, "text/html");
    });

    test("2. @cap/serve-static — Rejects non-GET/HEAD methods with 405", () => {
        let opts = ServeStaticOptions.defaultOpts();
        let out: [64]u8 = undefined;
        expect_eq(resolveStaticRequest("POST", "/index.html", opts, out[0..64]).status, STATIC_ACTION_METHOD_NOT_ALLOWED);
        expect_eq(resolveStaticRequest("DELETE", "/a.txt", opts, out[0..64]).status, STATIC_ACTION_METHOD_NOT_ALLOWED);
    });

    test("3. @cap/serve-static — Blocks path traversal '..' with 403 Forbidden", () => {
        let opts = ServeStaticOptions.defaultOpts();
        let out: [64]u8 = undefined;
        expect_eq(resolveStaticRequest("GET", "/../../etc/passwd", opts, out[0..64]).status, STATIC_ACTION_FORBIDDEN);
    });

    test("4. @cap/serve-static — Dotfile policies: ignore (404), deny (403), allow (200)", () => {
        let out: [64]u8 = undefined;
        expect_eq(hasDotfileSegment("/.env"), true);
        expect_eq(hasDotfileSegment("/a/.git/HEAD"), true);
        expect_eq(hasDotfileSegment("/normal/file.txt"), false);

        let o_ign = ServeStaticOptions.defaultOpts();
        o_ign.dotfiles = DOTFILE_IGNORE;
        expect_eq(resolveStaticRequest("GET", "/.env", o_ign, out[0..64]).status, STATIC_ACTION_NOT_FOUND);

        let o_deny = ServeStaticOptions.defaultOpts();
        o_deny.dotfiles = DOTFILE_DENY;
        expect_eq(resolveStaticRequest("GET", "/.env", o_deny, out[0..64]).status, STATIC_ACTION_FORBIDDEN);

        let o_allow = ServeStaticOptions.defaultOpts();
        o_allow.dotfiles = DOTFILE_ALLOW;
        expect_eq(resolveStaticRequest("GET", "/.well-known/security.txt", o_allow, out[0..64]).status, STATIC_ACTION_SERVE);
    });

    test("5. @cap/serve-static — formatCacheControl with max-age & immutable", () => {
        let out: [128]u8 = undefined;
        let n1 = formatCacheControl(3600, false, out[0..128]);
        expect_eq(eqBuf(out[0..n1], n1, "public, max-age=3600"), true);
        let n2 = formatCacheControl(31536000, true, out[0..128]);
        expect_eq(eqBuf(out[0..n2], n2, "public, max-age=31536000, immutable"), true);
    });

    test("6. @cap/serve-static — formatStaticEtag & formatStaticContentType delegation", () => {
        let out: [128]u8 = undefined;
        let ne = formatStaticEtag(256, 4096, out[0..128]);
        expect_eq(eqBuf(out[0..ne], ne, "W/\"100-1000\""), true);
        let nc = formatStaticContentType("style.css", out[0..128]);
        expect_eq(eqBuf(out[0..nc], nc, "text/css; charset=utf-8"), true);
    });
}
