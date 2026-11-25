// cap/test/testCapCookie.cp
// RFC 6265 Cookie parser, Set-Cookie serializer, clear-cookie helper, and CRLF/semicolon injection defense tests.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CookieError,
    SameSite,
    SetCookieOptions,
    CookieIter,
    getCookie,
    countCookies,
    isValidCookieName,
    isValidCookieValue,
    serializeSetCookie,
    serializeClearCookie,
} from "../../packages/@cap/cookie/src/index.cp";

fn expectInvalidName(name: []const u8): bool {
    let buf: [128]u8 = [0; 128];
    * = serializeSetCookie(name, "ok", SetCookieOptions{}, buf[0..]) catch |e| {
        return e == CookieError.InvalidName;
    };
    return false;
}

fn expectInvalidAttr(path: []const u8): bool {
    let buf: [128]u8 = [0; 128];
    * = serializeSetCookie("sid", "ok", SetCookieOptions{ path: path }, buf[0..]) catch |e| {
        return e == CookieError.InvalidAttribute;
    };
    return false;
}

fn main() {
    test("1. @cap/cookie — Parse Cookie Header, Quoted Values & Iteration", () => {
        const hdr = "sid=abc123; theme=\"dark\" ; empty=; broken ; lang=zh-CN";
        expect_eq(countCookies(hdr), 4 as usize);
        expect_eq(std.mem.eql(u8, getCookie(hdr, "sid").?, "abc123"), true);
        expect_eq(std.mem.eql(u8, getCookie(hdr, "theme").?, "dark"), true);
        expect_eq(std.mem.eql(u8, getCookie(hdr, "empty").?, ""), true);
        expect_eq(std.mem.eql(u8, getCookie(hdr, "lang").?, "zh-CN"), true);
        expect_eq(getCookie(hdr, "missing") == null, true);
    });

    test("2. @cap/cookie — Serialize Set-Cookie & Clear Cookie", () => {
        let buf: [256]u8 = [0; 256];
        const hdr = serializeSetCookie(
            "session_id",
            "tok_9f8e7d",
            SetCookieOptions{
                path: "/",
                domain: "example.com",
                max_age: 3600,
                http_only: true,
                secure: true,
                partitioned: true,
                same_site: SameSite.Strict,
            },
            buf[0..],
        ) catch unreachable;
        const expected = "session_id=tok_9f8e7d; Max-Age=3600; Domain=example.com; Path=/; HttpOnly; Secure; Partitioned; SameSite=Strict";
        expect_eq(std.mem.eql(u8, hdr, expected), true);

        let clr_buf: [128]u8 = [0; 128];
        const clr = serializeClearCookie("session_id", "/", clr_buf[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, clr, "session_id=; Max-Age=0; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT"), true);
    });

    test("3. @cap/cookie — RFC 6265 Validation & Header Injection Defense", () => {
        expect_eq(isValidCookieName("session_id"), true);
        expect_eq(isValidCookieName("bad name"), false);
        expect_eq(isValidCookieName("bad;name"), false);
        expect_eq(isValidCookieValue("abc123"), true);
        expect_eq(isValidCookieValue("bad;val"), false);

        expect_eq(expectInvalidName("bad\r\nname"), true);
        expect_eq(expectInvalidAttr("/; HttpOnly"), true);
    });
}
