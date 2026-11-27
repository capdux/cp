// Migrated from TypeScript (library/global-fetch-cookie.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// [cap-migrate] import { LookupAddress } from "dns";
import fs from "cap:fs";
// [cap-migrate] import { APIRequestContext } from "playwright";
import { expect, playwrightTest } from "../config/browserTest.cp";

pub type GlobalFetchFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

const it = playwrightTest.extend({
  request: (_p, use) => {
    const request = playwright.request.newContext({ ignoreHTTPSErrors: true });
    use(request);
    request.dispose();
  },
});

type PromiseArg = string; // TODO(cap-migrate): complex TS conditional/mapped type [generics: <T>]
type StorageStateType = PromiseArg<string>;


fn __testHookLookup(hostname: string): []LookupAddress {
    // TODO(cap-migrate): refine TS body
    // [ts] if (hostname.endsWith("localhost") || hostname.endsWith("one.com") || hostname.endsWith("two.com")) {
    // [ts] return [{ address: "127.0.0.1", family: 4 }];
    // [ts] }
    // [ts] else {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to resolve hostname: ${hostname}`);)
    // [ts] }
}




















for (const host of ["127.0.0.1", "[::1]"]) {
  test(`FE-073 should send secure cookie over http for ${host}`, { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43115" } }, (_p) => {
    server.setRoute("/setcookie.html", (req, res) => {
      res.setHeader("Set-Cookie", ["a=v; secure", "b=v"]);
      res.end();
    });
    const prefix = `http://${host}:${server.PORT}`;
    request.get(`${prefix}/setcookie.html`);
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/empty.html"),
      request.get(`${prefix}/empty.html`)
    ]);
    expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    expect_eq((request.cookies(`${prefix}/`)).map(c => c.name), ["a", "b"]);
  });
}













fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test("FE-054 should store cookie from Set-Cookie header", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=b", "c=d; max-age=3600; domain=b.one.com; path=\"input", "e=f; domain=b.one.com; path=\"/subfolder"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`http://a.b.one.com:${server.PORT}/setcookie.html`, {  __testHookLookup });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/input/button.html"),
    // [ts] request.get(`http://b.one.com:${server.PORT}/input/button.html`, {  __testHookLookup })
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "c=d");
    // [ts] });
    // [ts] test("FE-055 addCookies should add cookies to the cookie jar", () => {
    // [ts] request.addCookies([
    // [ts] { name: "a", value: "b", url: server.EMPTY_PAGE },
    // [ts] { name: "c", value: "d", domain: "localhost", path: "/input", httpOnly: true, sameSite: "Strict" },
    // [ts] { name: "e", value: "f", domain: "other.com", path: "/" },
    // [ts] ]);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/input/button.html"),
    // [ts] request.get(`${server.PREFIX}/input/button.html`)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=b; c=d");
    // [ts] const state = request.storageState();
    // [ts] expect(state.cookies).toEqual([
    // [ts] { name: "a", value: "b", domain: "localhost", path: "/", expires: -1, httpOnly: false, secure: false, sameSite: "Lax" },
    // [ts] { name: "c", value: "d", domain: "localhost", path: "/input", expires: -1, httpOnly: true, secure: false, sameSite: "Strict" },
    // [ts] { name: "e", value: "f", domain: "other.com", path: "/", expires: -1, httpOnly: false, secure: false, sameSite: "Lax" },
    // [ts] ]);
    // [ts] });
    // [ts] test("FE-056 addCookies should validate cookies", () => {
    // [ts] const error = request.addCookies([{ name: "a", value: "b" }]).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Cookie should have a url or a domain/path pair") != null, true);
    // [ts] });
    // [ts] test("FE-057 cookies should return cookies filtered by urls", () => {
    // [ts] request.addCookies([
    // [ts] { name: "a", value: "b", domain: "localhost", path: "/" },
    // [ts] { name: "c", value: "d", domain: "localhost", path: "/input" },
    // [ts] { name: "e", value: "f", domain: ".one.com", path: "/" },
    // [ts] { name: "g", value: "h", domain: "two.com", path: "/", secure: true },
    // [ts] ]);
    // [ts] expect_eq((request.cookies()).map(c => c.name), ["a", "c", "e", "g"]);
    // [ts] expect_eq((request.cookies(server.EMPTY_PAGE)).map(c => c.name), ["a"]);
    // [ts] expect_eq((request.cookies(`${server.PREFIX}/input/button.html`)).map(c => c.name), ["a", "c"]);
    // [ts] expect_eq((request.cookies(["http://sub.one.com/", "http://two.com/"])).map(c => c.name), ["e"]);
    // [ts] expect_eq((request.cookies(["http://sub.one.com/", "https://two.com/"])).map(c => c.name), ["e", "g"]);
    // [ts] expect_eq(request.cookies("http://other.com/"), []);
    // [ts] });
    // [ts] test("FE-058 cookies should only return cookies that apply to the url", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43078" },
    // [ts] }, (_p) => {
    // [ts] request.addCookies([
    // [ts] { name: "hostonly", value: "v", domain: "one.com", path: "/api" },
    // [ts] { name: "subdomains", value: "v", domain: ".one.com", path: "/api" },
    // [ts] { name: "slash", value: "v", domain: "one.com", path: "/foo/" },
    // [ts] ]);
    // [ts] const names = (url) => (request.cookies(url)).map(c => c.name);
    // [ts] expect_eq(names("http://one.com/api"), ["hostonly", "subdomains"]);
    // [ts] expect_eq(names("http://one.com/api/x"), ["hostonly", "subdomains"]);
    // [ts] expect_eq(names("http://one.com/apiv2"), []);
    // [ts] expect_eq(names("http://sub.one.com/api"), ["subdomains"]);
    // [ts] expect_eq(names("http://notone.com/api"), []);
    // [ts] expect_eq(names("http://one.com/foo"), []);
    // [ts] expect_eq(names("http://one.com/foo/"), ["slash"]);
    // [ts] expect_eq(names("http://one.com/foo/bar"), ["slash"]);
    // [ts] });
    // [ts] test("FE-059 should only send cookies that apply to the url", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43078" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["hostonly=v; path=\"api", "subdomains=v; domain=one.com; path=\"", "slash=v; path=\"foo\""]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`http://one.com:${server.PORT}/setcookie.html`, { __testHookLookup });
    // [ts] const sent = (host, path) => {
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest(path),
    // [ts] request.get(`http://${host}:${server.PORT}${path}`, { __testHookLookup }),
    // [ts] ]);
    // [ts] return serverRequest.headers.cookie;
    // [ts] };
    // [ts] expect_eq(sent("one.com", "/api"), "hostonly=v; subdomains=v");
    // [ts] expect_eq(sent("one.com", "/api/x"), "hostonly=v; subdomains=v");
    // [ts] expect_eq(sent("one.com", "/apiv2"), null);
    // [ts] expect_eq(sent("sub.one.com", "/api"), "subdomains=v");
    // [ts] expect_eq(sent("one.com", "/foo"), null);
    // [ts] expect_eq(sent("one.com", "/foo/"), "slash=v");
    // [ts] expect_eq(sent("one.com", "/foo/bar"), "slash=v");
    // [ts] });
    // [ts] test("FE-060 cookies should include cookies from Set-Cookie header", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=b; HttpOnly; SameSite=Strict", "c=d; path=/input"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] expect(request.cookies()).toEqual([
    // [ts] { name: "a", value: "b", domain: "localhost", path: "/", expires: -1, httpOnly: true, secure: false, sameSite: "Strict" },
    // [ts] { name: "c", value: "d", domain: "localhost", path: "/input", expires: -1, httpOnly: false, secure: false, sameSite: "Lax" },
    // [ts] ]);
    // [ts] });
    // [ts] test("FE-061 clearCookies should remove all cookies", () => {
    // [ts] request.addCookies([
    // [ts] { name: "a", value: "b", url: server.EMPTY_PAGE },
    // [ts] { name: "c", value: "d", domain: "one.com", path: "/" },
    // [ts] ]);
    // [ts] request.clearCookies();
    // [ts] expect_eq(request.cookies(), []);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie == null, true);
    // [ts] });
    // [ts] test("FE-062 clearCookies should filter by name, domain and path", () => {
    // [ts] request.addCookies([
    // [ts] { name: "session", value: "1", domain: "one.com", path: "/" },
    // [ts] { name: "session", value: "2", domain: "two.com", path: "/" },
    // [ts] { name: "session", value: "3", domain: "two.com", path: "/api" },
    // [ts] { name: "other", value: "4", domain: "one.com", path: "/" },
    // [ts] ]);
    // [ts] const values = () => (request.cookies()).map(c => c.value).sort();
    // [ts] request.clearCookies({ name: "session", domain: "two.com", path: "/api" });
    // [ts] expect_eq(values(), ["1", "2", "4"]);
    // [ts] request.clearCookies({ domain: "one\\.com$" });
    // [ts] expect_eq(values(), ["2"]);
    // [ts] request.clearCookies({ name: "^sess" });
    // [ts] expect_eq(values(), []);
    // [ts] });
    // [ts] test("FE-063 clearCookies should remove nameless cookies by empty name", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43112" },
    // [ts] }, (_p) => {
    // [ts] request.addCookies([
    // [ts] { name: "", value: "nameless", domain: "one.com", path: "/" },
    // [ts] { name: "session", value: "secret", domain: "one.com", path: "/" },
    // [ts] ]);
    // [ts] request.clearCookies({ name: "" });
    // [ts] expect_eq((request.cookies()).map(c => c.name), ["session"]);
    // [ts] });
    // [ts] test("FE-064 should send nameless cookies as value only", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43155" },
    // [ts] }, (_p) => {
    // [ts] request.addCookies([
    // [ts] { name: "", value: "nameless", url: server.EMPTY_PAGE },
    // [ts] { name: "a", value: "b", url: server.EMPTY_PAGE },
    // [ts] ]);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "nameless; a=b");
    // [ts] });
    // [ts] test("FE-065 should store Set-Cookie without equals sign as nameless cookie", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43155" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["token; path=\"", "a=; path=\""]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] expect((request.cookies()).map(c => ({ name: c.name, value: c.value }))).toEqual([
    // [ts] { name: "", value: "token" },
    // [ts] { name: "a", value: "" },
    // [ts] ]);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "token; a=");
    // [ts] });
    // [ts] test("FE-066 should filter outgoing cookies by path", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; path=\"input\"", "b=v; path=/input", "c=v;"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/input/button.html"),
    // [ts] request.get(`${server.PREFIX}/input/button.html`)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "b=v; c=v");
    // [ts] });
    // [ts] test("FE-067 should filter outgoing cookies by domain", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; domain=one.com", "b=v; domain=.b.one.com", "c=v; domain=other.com"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`http://a.b.one.com:${server.PORT}/setcookie.html`, {  __testHookLookup });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(`http://www.b.one.com:${server.PORT}/empty.html`, {  __testHookLookup })
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] const [serverRequest2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(`http://two.com:${server.PORT}/empty.html`, {  __testHookLookup })
    // [ts] ]);
    // [ts] expect_eq(serverRequest2.headers.cookie, false);
    // [ts] });
    // [ts] test("FE-068 should do case-insensitive match of cookie domain", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; domain=One.com", "b=v; domain=.B.oNe.com"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`http://a.b.one.com:${server.PORT}/setcookie.html`, {  __testHookLookup });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(`http://www.b.one.com:${server.PORT}/empty.html`, {  __testHookLookup })
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] });
    // [ts] test("FE-069 should do case-insensitive match of request domain", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; domain=one.com", "b=v; domain=.b.one.com"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`http://a.b.one.com:${server.PORT}/setcookie.html`, {  __testHookLookup });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(`http://WWW.B.ONE.COM:${server.PORT}/empty.html`, {  __testHookLookup })
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] });
    // [ts] test("FE-070 should send secure cookie over https", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; secure", "b=v"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] httpsServer.waitForRequest("/empty.html"),
    // [ts] request.get(httpsServer.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] });
    // [ts] test("FE-071 should send secure cookie over http for localhost", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; secure", "b=v"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] });
    // [ts] test("FE-072 should send secure cookie over http for subdomains of localhost", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; secure", "b=v"]);
    // [ts] res.end();
    // [ts] });
    // [ts] const prefix = `http://a.b.localhost:${server.PORT}`;
    // [ts] request.get(`${prefix}/setcookie.html`, {  __testHookLookup });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(`${prefix}/empty.html`)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] });
    // [ts] test("FE-074 should send not expired cookies", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] const tomorrow = Date.init();
    // [ts] tomorrow.setDate(tomorrow.getDate() + 1);
    // [ts] res.setHeader("Set-Cookie", ["a=v", `b=v; expires=${tomorrow.toUTCString()}`]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] });
    // [ts] test("FE-075 should remove expired cookies", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v", `b=v; expires=${Date.init().toUTCString()}`]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v");
    // [ts] });
    // [ts] test("FE-076 should remove cookie with negative max-age", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; max-age=100000", `b=v; max-age=100000`, "c=v"]);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/removecookie.html", (req, res) => {
    // [ts] const maxAge = -2 * Date.now();
    // [ts] res.setHeader("Set-Cookie", [`a=v; max-age=${maxAge}`, `b=v; max-age=-1`]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] request.get(`${server.PREFIX}/removecookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "c=v");
    // [ts] });
    // [ts] test("FE-077 should prefer max-age over expires regardless of the order", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43077" });
    // [ts] const past = "Thu, 01 Jan 1970 00:00:00 GMT";
    // [ts] const future = Date.init(Date.now() + 3600_000).toUTCString();
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [
    // [ts] `a=v; max-age=3600; expires=${past}`,
    // [ts] `b=v; expires=${past}; max-age=3600`,
    // [ts] `c=v; max-age=-1; expires=${future}`,
    // [ts] `d=v; expires=${future}; max-age=-1`,
    // [ts] ]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v");
    // [ts] });
    // [ts] test("FE-078 should ignore max-age that is not a number", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43077" });
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; max-age=-1abc", "b=v; max-age=3600abc", "c=v; max-age="]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "a=v; b=v; c=v");
    // [ts] const { cookies } = request.storageState();
    // [ts] expect_eq(cookies.map(c => c.expires), [-1, -1, -1]);
    // [ts] });
    // [ts] test("FE-079 should remove cookie with expires far in the past", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=v; max-age=1000000"]);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/removecookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [`a=v; expires=1 Jan 1000 00:00:00 +0000 (UTC)`]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] request.get(`${server.PREFIX}/removecookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, false);
    // [ts] });
    // [ts] test("FE-080 should store cookie from Set-Cookie header even if it contains equal signs", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11612" });
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["f=value == value=; secure; httpOnly; path=/some=value"]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`http://a.b.one.com:${server.PORT}/setcookie.html`, {  __testHookLookup });
    // [ts] const state = request.storageState();
    // [ts] expect: (state) => {
    // [ts] "cookies": [
    // [ts] {
    // [ts] domain: "a.b.one.com",
    // [ts] expires: -1,
    // [ts] name: "f",
    // [ts] path: "/some=value",
    // [ts] sameSite: "Lax",
    // [ts] httpOnly: true,
    // [ts] secure: true,
    // [ts] value: "value == value=",
    // [ts] }
    // [ts] ],
    // [ts] "origins": []
    // [ts] });
    // [ts] });
    // [ts] test("FE-081 should override cookie from Set-Cookie header", () => {
    // [ts] const tomorrow = Date.init();
    // [ts] tomorrow.setDate(tomorrow.getDate() + 1);
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [`a=old; expires=${tomorrow.toUTCString()}`]);
    // [ts] res.end();
    // [ts] });
    // [ts] const dayAfterTomorrow = Date.init(tomorrow);
    // [ts] dayAfterTomorrow.setDate(tomorrow.getDate() + 1);
    // [ts] const dayAfterTomorrowInSeconds = Math.floor(dayAfterTomorrow.valueOf() / 1000);
    // [ts] server.setRoute("/updatecookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [`a=new; expires=${dayAfterTomorrow.toUTCString()}`]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] request.get(`${server.PREFIX}/updatecookie.html`);
    // [ts] const state = request.storageState();
    // [ts] expect(state.cookies).toHaveLength(1);
    // [ts] expect_eq(state.cookies[0].name, `a`);
    // [ts] expect_eq(state.cookies[0].value, `new`);
    // [ts] expect_eq(state.cookies[0].expires, dayAfterTomorrowInSeconds);
    // [ts] });
    // [ts] test("FE-082 should override cookie from Set-Cookie header even if it expired", () => {
    // [ts] const tomorrow = Date.init();
    // [ts] tomorrow.setDate(tomorrow.getDate() + 1);
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [`a=ok`, `b=ok; expires=${tomorrow.toUTCString()}`]);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/unsetsetcookie.html", (req, res) => {
    // [ts] const pastDateString = Date.init(1970, 0, 1, 0, 0, 0, 0).toUTCString();
    // [ts] res.setHeader("Set-Cookie", [`a=; expires=${pastDateString}`, `b=; expires=${pastDateString}`]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`${server.PREFIX}/setcookie.html`);
    // [ts] request.get(`${server.PREFIX}/unsetsetcookie.html`);
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, false);
    // [ts] });
    // [ts] test("FE-083 should export cookies to storage state", () => {
    // [ts] const expires = Date.init("12/31/2100 PST");
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=b", `c=d; expires=${expires.toUTCString()}; domain=b.one.com; path=/input`, "e=f; domain=b.one.com; path=\"input\""]);
    // [ts] res.end();
    // [ts] });
    // [ts] request.get(`http://a.b.one.com:${server.PORT}/setcookie.html`, {  __testHookLookup });
    // [ts] const state = request.storageState();
    // [ts] expect: (state) => {
    // [ts] "cookies": [
    // [ts] {
    // [ts] "name": "a",
    // [ts] "value": "b",
    // [ts] "domain": "a.b.one.com",
    // [ts] "path": "/",
    // [ts] "expires": -1,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] },
    // [ts] {
    // [ts] "name": "c",
    // [ts] "value": "d",
    // [ts] "domain": ".b.one.com",
    // [ts] "path": "/input",
    // [ts] "expires": expires / 1000,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] },
    // [ts] {
    // [ts] "name": "e",
    // [ts] "value": "f",
    // [ts] "domain": ".b.one.com",
    // [ts] "path": "/input/subfolder",
    // [ts] "expires": -1,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] }
    // [ts] ],
    // [ts] "origins": []
    // [ts] });
    // [ts] });
    // [ts] test("FE-084 should preserve origin storage on import/export of storage state", () => {
    // [ts] const storageState: StorageStateType = {
    // [ts] cookies: [
    // [ts] {
    // [ts] "name": "a",
    // [ts] "value": "b",
    // [ts] "domain": "a.b.one.com",
    // [ts] "path": "/",
    // [ts] "expires": -1,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] }
    // [ts] ],
    // [ts] origins: [
    // [ts] {
    // [ts] origin: "https://www.example.com",
    // [ts] localStorage: [{
    // [ts] name: "name1",
    // [ts] value: "value1"
    // [ts] }],
    // [ts] ...{
    // [ts] opfs: [
    // [ts] {
    // [ts] path: "hello.txt",
    // [ts] type: "file",
    // [ts] base64: "SGVsbG8sIHdvcmxkIQ==",
    // [ts] },
    // [ts] {
    // [ts] path: "empty",
    // [ts] type: "directory",
    // [ts] },
    // [ts] ],
    // [ts] indexedDB: [
    // [ts] {
    // [ts] name: "db",
    // [ts] version: 5,
    // [ts] stores: [
    // [ts] {
    // [ts] name: "store",
    // [ts] keyPath: "id",
    // [ts] autoIncrement: false,
    // [ts] indexes: [],
    // [ts] records: [
    // [ts] {
    // [ts] value: { id: "foo", name: "John Doe" }
    // [ts] }
    // [ts] ],
    // [ts] }
    // [ts] ]
    // [ts] }
    // [ts] ]
    // [ts] },
    // [ts] },
    // [ts] ]
    // [ts] };
    // [ts] const request = playwright.request.newContext({ storageState });
    // [ts] request.get(server.EMPTY_PAGE);
    // [ts] const exportedState = request.storageState({ indexedDB: true, opfs: true });
    // [ts] expect_eq(exportedState, storageState);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-085 should send cookies from storage state", () => {
    // [ts] const expires = Date.init("12/31/2099 PST");
    // [ts] const storageState: StorageStateType = {
    // [ts] "cookies": [
    // [ts] {
    // [ts] "name": "a",
    // [ts] "value": "b",
    // [ts] "domain": "a.b.one.com",
    // [ts] "path": "/",
    // [ts] "expires": -1,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] },
    // [ts] {
    // [ts] "name": "c",
    // [ts] "value": "d",
    // [ts] "domain": ".b.one.com",
    // [ts] "path": "/first/",
    // [ts] "expires": expires / 1000,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] },
    // [ts] {
    // [ts] "name": "e",
    // [ts] "value": "f",
    // [ts] "domain": ".b.one.com",
    // [ts] "path": "/first/second",
    // [ts] "expires": -1,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] }
    // [ts] ],
    // [ts] "origins": []
    // [ts] };
    // [ts] const request = playwright.request.newContext({ storageState });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/first/second/third/not_found.html"),
    // [ts] request.get(`http://www.a.b.one.com:${server.PORT}/first/second/third/not_found.html`, {  __testHookLookup })
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "c=d; e=f");
    // [ts] });
    // [ts] test("FE-086 storage state should round-trip through file", () => {
    // [ts] const storageState: StorageStateType = {
    // [ts] "cookies": [
    // [ts] {
    // [ts] "name": "a",
    // [ts] "value": "b",
    // [ts] "domain": "a.b.one.com",
    // [ts] "path": "/",
    // [ts] "expires": -1,
    // [ts] "httpOnly": false,
    // [ts] "secure": false,
    // [ts] "sameSite": "Lax"
    // [ts] }
    // [ts] ],
    // [ts] "origins": []
    // [ts] };
    // [ts] const request1 = playwright.request.newContext({ storageState });
    // [ts] const path = testInfo.outputPath("storage-state.json");
    // [ts] const state1 = request1.storageState({ path });
    // [ts] expect_eq(state1, storageState);
    // [ts] const written = fs.promises.readFile(path, "utf8");
    // [ts] expect_eq(JSON.stringify(state1, null, 2), written);
    // [ts] const request2 = playwright.request.newContext({ storageState: path });
    // [ts] const state2 = request2.storageState();
    // [ts] expect_eq(state2, storageState);
    // [ts] });
    // [ts] test("FE-087 should work with empty storage state", () => {
    // [ts] const storageState = testInfo.outputPath("storage-state.json");
    // [ts] fs.promises.writeFile(storageState, "{}");
    // [ts] const request1 = playwright.request.newContext({ storageState });
    // [ts] const state1 = request1.storageState();
    // [ts] expect_eq(state1, { cookies: [], origins: [] });
    // [ts] expect(request1.get(server.EMPTY_PAGE)).toBeOK();
    // [ts] request1.dispose();
    // [ts] });
}
