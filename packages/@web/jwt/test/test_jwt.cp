// Unit tests for @web/jwt
const std = @import("std");
import { App, AppMethod, Request, Context, Response } from "../../../../lib/web/app.cp";
import {
    JwtConfig,
    sha256,
    base64UrlEncode,
    base64UrlDecode,
    signHs256,
    verifyHs256,
    registerJwt,
    createJwtPlugin,
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

fn protectedHandler(ctx: *Context): Response {
    const verified = ctx.derived("jwt_verified") orelse "0";
    if (strEql(verified, "1")) {
        return Response.json(200, "{\"ok\":true,\"auth\":\"verified\"}");
    }
    return Response.json(200, "{\"ok\":true,\"auth\":\"guest\"}");
}

pub fn main() {
    // 1. FIPS 180-4 SHA-256 test vector: SHA256("abc") = ba7816bf8f01cfea...
    let digest: [32]u8 = [0; 32];
    sha256("abc", *digest);
    assertTrue(digest[0] == 0xba && digest[1] == 0x78 && digest[2] == 0x16 && digest[3] == 0xbf, "sha256('abc') prefix");
    assertTrue(digest[31] == 0xad, "sha256('abc') last byte");

    // 2. Base64URL roundtrip
    let enc_buf: [64]u8 = [0; 64];
    let dec_buf: [64]u8 = [0; 64];
    const elen = base64UrlEncode("hello+world/test?", enc_buf[0..]) orelse 0;
    assertTrue(elen > 0, "base64UrlEncode len > 0");
    const dlen = base64UrlDecode(enc_buf[0..elen], dec_buf[0..]) orelse 0;
    assertTrue(strEql(dec_buf[0..dlen], "hello+world/test?"), "base64Url roundtrip");
    assertTrue(base64UrlDecode("bad!char", dec_buf[0..]) == null, "base64Url rejects invalid char");

    // 3. HS256 sign & verify
    let tok_buf: [512]u8 = [0; 512];
    let pay_buf: [256]u8 = [0; 256];
    const payload = "{\"sub\":\"u123\",\"iss\":\"cap-auth\",\"role\":\"admin\"}";
    const tlen = signHs256("super-secret-key", payload, tok_buf[0..]) orelse 0;
    assertTrue(tlen > 40, "signHs256 produces token");

    const vlen = verifyHs256("super-secret-key", tok_buf[0..tlen], pay_buf[0..]) orelse 0;
    assertTrue(strEql(pay_buf[0..vlen], payload), "verifyHs256 decodes payload");

    // Wrong secret must fail
    assertTrue(verifyHs256("wrong-secret", tok_buf[0..tlen], pay_buf[0..]) == null, "wrong secret rejected");

    // Tampered token must fail
    let bad_tok: [512]u8 = [0; 512];
    let i: usize = 0;
    while (i < tlen) : (i += 1) {
        bad_tok[i] = tok_buf[i];
    }
    bad_tok[tlen - 1] = if (bad_tok[tlen - 1] == 'A') 'B' else 'A';
    assertTrue(verifyHs256("super-secret-key", bad_tok[0..tlen], pay_buf[0..]) == null, "tampered signature rejected");
    assertTrue(verifyHs256("super-secret-key", "not.a.valid.jwt.token", pay_buf[0..]) == null, "malformed dots rejected");

    // 4. App integration (strict auth)
    let app = App.init();
    * = registerJwt(*app, JwtConfig{
        secret: "super-secret-key",
        issuer: "cap-auth",
        header_name: "Authorization",
        optional_auth: false,
    });
    * = app.get("/me", protectedHandler);

    // 4a. Missing header -> 401
    const r_missing = app.dispatch(Request.init(AppMethod.GET, "/me"));
    assertTrue(r_missing.status == 401, "missing token returns 401");

    // 4b. Invalid token -> 401
    let req_bad = Request.init(AppMethod.GET, "/me");
    * = req_bad.withHeader("Authorization", "Bearer invalid.token.here");
    const r_bad = app.dispatch(req_bad);
    assertTrue(r_bad.status == 401, "invalid token returns 401");

    // 4c. Valid token with matching issuer -> 200 + X-Auth-Scheme: Bearer
    let auth_hdr: [600]u8 = [0; 600];
    const prefix = "Bearer ";
    i = 0;
    while (i < prefix.len) : (i += 1) {
        auth_hdr[i] = prefix[i];
    }
    let j: usize = 0;
    while (j < tlen) : (j += 1) {
        auth_hdr[prefix.len + j] = tok_buf[j];
    }
    let req_ok = Request.init(AppMethod.GET, "/me");
    * = req_ok.withHeader("Authorization", auth_hdr[0 .. prefix.len + tlen]);
    const r_ok = app.dispatch(req_ok);
    assertTrue(r_ok.status == 200, "valid token returns 200");
    assertTrue(strEql(r_ok.body, "{\"ok\":true,\"auth\":\"verified\"}"), "jwt_verified derived injected");
    const scheme = r_ok.header("X-Auth-Scheme") orelse "";
    assertTrue(strEql(scheme, "Bearer"), "X-Auth-Scheme header set");

    // 4d. Optional auth via createPlugin
    let app_opt = App.init();
    const plug = createJwtPlugin(JwtConfig{
        secret: "super-secret-key",
        issuer: "",
        header_name: "Authorization",
        optional_auth: true,
    });
    * = app_opt.usePlugin(&plug);
    * = app_opt.get("/public", protectedHandler);
    const r_guest = app_opt.dispatch(Request.init(AppMethod.GET, "/public"));
    assertTrue(r_guest.status == 200, "optional_auth allows guest");
    assertTrue(strEql(r_guest.body, "{\"ok\":true,\"auth\":\"guest\"}"), "guest path without jwt_verified");

    std.debug.print("PASS: @web/jwt all tests passed\n", .{});
}
