// cap/test/test_hex_url_case_router.cp
// E2E Regression Suite for deepened modules in cap:encoding, cap:url, cap:string, and cap:router

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { Hex } from "cap:encoding";
import { PercentEncoding } from "cap:url";
import { CaseConverter } from "cap:string";
import { ParamMap, RouteFormatter } from "cap:router";

fn main() {
    test("1. cap:encoding — Hex table-driven encodeLower, encodeUpper, decode & validation", () => {
        expect_eq(Hex.encodedLen(6), 12 as usize);
        expect_eq(Hex.decodedLen(12), 6 as usize);
        expect_eq(Hex.isValid("48656c6c6f21"), true);
        expect_eq(Hex.isValid("48656C6C6F21"), true);
        expect_eq(Hex.isValid("48656g"), false);
        expect_eq(Hex.isValid("486"), false);

        let lower_buf: [32]u8 = [0; 32];
        const lower_len = Hex.encodeLower("Hello!", lower_buf[0..32]);
        expect_eq(lower_len, 12 as usize);
        expect_eq(std.mem.eql(u8, lower_buf[0..lower_len], "48656c6c6f21"), true);

        let upper_buf: [32]u8 = [0; 32];
        const upper_len = Hex.encodeUpper("Hello!", upper_buf[0..32]);
        expect_eq(upper_len, 12 as usize);
        expect_eq(std.mem.eql(u8, upper_buf[0..upper_len], "48656C6C6F21"), true);

        let dec_buf: [16]u8 = [0; 16];
        const dec_len = Hex.decode("48656C6C6F21", dec_buf[0..16]) catch 0;
        expect_eq(dec_len, 6 as usize);
        expect_eq(std.mem.eql(u8, dec_buf[0..dec_len], "Hello!"), true);
    });

    test("2. cap:url — PercentEncoding RFC 3986 component encoding & decoding", () => {
        expect_eq(PercentEncoding.needsEncodeComponent("safe-key_123.~"), false);
        expect_eq(PercentEncoding.needsEncodeComponent("a b&c=d/e?"), true);

        let enc_buf: [128]u8 = [0; 128];
        const enc_len = PercentEncoding.encodeComponentToBuf("hello world/a+b=c", enc_buf[0..128]);
        expect_eq(std.mem.eql(u8, enc_buf[0..enc_len], "hello%20world%2Fa%2Bb%3Dc"), true);

        let dec_buf: [128]u8 = [0; 128];
        const dec_len = PercentEncoding.decodeToBuf(enc_buf[0..enc_len], false, dec_buf[0..128]);
        expect_eq(std.mem.eql(u8, dec_buf[0..dec_len], "hello world/a+b=c"), true);

        // Form-urlencoded '+' as space decoding
        let form_buf: [64]u8 = [0; 64];
        const form_len = PercentEncoding.decodeToBuf("Tom+and+Jerry%21", true, form_buf[0..64]);
        expect_eq(std.mem.eql(u8, form_buf[0..form_len], "Tom and Jerry!"), true);
    });

    test("3. cap:string — CaseConverter snake_case, kebab-case, camelCase, PascalCase & SCREAMING_SNAKE", () => {
        let out: [64]u8 = [0; 64];

        const n_snake = CaseConverter.toSnakeCase("parseHTMLResponseCode", out[0..64]);
        expect_eq(std.mem.eql(u8, out[0..n_snake], "parse_html_response_code"), true);

        const n_kebab = CaseConverter.toKebabCase("userProfileSettings", out[0..64]);
        expect_eq(std.mem.eql(u8, out[0..n_kebab], "user-profile-settings"), true);

        const n_camel = CaseConverter.toCamelCase("user_profile-settings_v2", out[0..64]);
        expect_eq(std.mem.eql(u8, out[0..n_camel], "userProfileSettingsV2"), true);

        const n_pascal = CaseConverter.toPascalCase("http_server_config", out[0..64]);
        expect_eq(std.mem.eql(u8, out[0..n_pascal], "HttpServerConfig"), true);

        const n_scream = CaseConverter.toScreamingSnakeCase("maxRetryCount", out[0..64]);
        expect_eq(std.mem.eql(u8, out[0..n_scream], "MAX_RETRY_COUNT"), true);
    });

    test("4. cap:router — RouteFormatter reverse URL path formatting & placeholder extraction", () => {
        const pattern = "/api/v1/users/:id/posts/:postId";

        let names: [8][]const u8 = undefined;
        const param_count = RouteFormatter.extractParamNames(pattern, names[0..8]);
        expect_eq(param_count, 2 as usize);
        expect_eq(std.mem.eql(u8, names[0], "id"), true);
        expect_eq(std.mem.eql(u8, names[1], "postId"), true);

        let params: ParamMap = ParamMap.init();
        params.append("id", "42");
        params.append("postId", "9001");

        let path_buf: [128]u8 = [0; 128];
        const path_len = RouteFormatter.formatPath(pattern, &params, path_buf[0..128]) catch 0;
        expect_eq(std.mem.eql(u8, path_buf[0..path_len], "/api/v1/users/42/posts/9001"), true);
    });
}
