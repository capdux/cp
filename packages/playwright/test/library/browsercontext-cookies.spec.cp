// Migrated from TypeScript (library/browsercontext-cookies.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { contextTest as it, expect } from "../config/browserTest.cp";










  expect(context.cookies("https://sub.foo.com")).toEqual([{
    name: "doggo",
    value: "woofs",
    domain: ".foo.com",
    path: "/",
    expires: -1,
    httpOnly: false,
    secure: true,
    sameSite: "Lax",
  }]);
// TODO(cap-migrate): refine TS statement
// [ts] });



// TODO(cap-migrate): refine TS statement
// [ts] });

for (const host of ["127.0.0.1", "[::1]"]) {
  test(`CO-014 should return secure cookies for http ${host}`, { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43115" } }, (_p) => {
    context.addCookies([{ name: "a", value: "v", domain: host, path: "/", secure: true }]);
    expect_eq((context.cookies(`http://${host}/`)).map(c => c.name), ["a"]);
  });
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CO-001 should return no cookies in pristine browser context", () => {
    // [ts] expect_eq(context.cookies(), []);
    // [ts] });
    // [ts] test("CO-002 should get a cookie @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const documentCookie = page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] return document.cookie;
    // [ts] });
    // [ts] expect_eq(documentCookie, "username=John Doe");
    // [ts] expect: (context.cookies()) => {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] }]);
    // [ts] });
    // [ts] test("CO-003 should get a non-session cookie", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // @see https://en.wikipedia.org/wiki/Year_2038_problem
    // [ts] const date = (Date.init("1/1/2038"));
    // [ts] const documentCookie = page.evaluate(timestamp => {
    // [ts] const date = Date.init(timestamp);
    // [ts] document.cookie = `username=John Doe;expires=${date.toUTCString()}`;
    // [ts] return document.cookie;
    // [ts] }, date);
    // [ts] expect_eq(documentCookie, "username=John Doe");
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // [ts] expect: (cookies[0]) => {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // We will check this separately.
    // [ts] expires: expect.anything(),
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] });
    // Browsers start to cap cookies with 400 days max expires value.
    // See https://github.com/httpwg/http-extensions/pull/1732
    // Chromium patch: https://chromium.googlesource.com/chromium/src/+/aaa5d2b55478eac2ee642653dcd77a50ac3faff6
    // We want to make sure that expires date is at least 400 days in future.
    // [ts] const FOUR_HUNDRED_DAYS = 1000 * 60 * 60 * 24 * 400;
    // [ts] const FIVE_MINUTES = 1000 * 60 * 5; // relax condition a bit to make sure test is not flaky.
    // [ts] expect_eq(cookies[0].expires > (Date.now() + FOUR_HUNDRED_DAYS - FIVE_MINUTES) / 1000, true);
    // [ts] });
    // [ts] test("CO-004 should allow adding cookies with >400 days expiration", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37903" }
    // [ts] }, (_p) => {
    // Browsers start to cap cookies with 400 days max expires value.
    // See https://github.com/httpwg/http-extensions/pull/1732
    // Chromium patch: https://chromium.googlesource.com/chromium/src/+/aaa5d2b55478eac2ee642653dcd77a50ac3faff6
    // [ts] const expire = Date.now() / 1000 + 401 * 24 * 3600;
    // [ts] context.addCookies([
    // [ts] {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: expire,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }
    // [ts] ]);
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // [ts] expect: (cookies[0]) => {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/"
    // [ts] }));
    // [ts] expect_eq(cookies[0].expires > 0, true);
    // [ts] expect_eq(cookies[0].expires <= expire, true);
    // [ts] });
    // [ts] test("CO-005 should properly report httpOnly cookie", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "name=value;HttpOnly; Path=/");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // [ts] expect_eq(cookies[0].httpOnly, true);
    // [ts] });
    // [ts] test("CO-006 should properly report \"Strict\" sameSite cookie", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "name=value;SameSite=Strict");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // [ts] expect_eq(cookies[0].sameSite, "Strict");
    // [ts] });
    // [ts] test("CO-007 should properly report \"Lax\" sameSite cookie", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "name=value;SameSite=Lax");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // [ts] expect_eq(cookies[0].sameSite, "Lax");
    // [ts] });
    // [ts] test("CO-008 should get multiple cookies", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const documentCookie = page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] document.cookie = "password=1234";
    // [ts] return document.cookie.split("; ").sort().join("; ");
    // [ts] });
    // [ts] const cookies = Set.init(context.cookies());
    // [ts] expect_eq(documentCookie, "password=1234; username=John Doe");
    // [ts] expect(cookies).toEqual(Set.init([
    // [ts] {
    // [ts] name: "password",
    // [ts] value: "1234",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] },
    // [ts] {
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] },
    // [ts] ]));
    // [ts] });
    // [ts] test("CO-009 should get cookies from multiple urls", () => {
    // [ts] context.addCookies([{
    // [ts] url: "https://foo.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] sameSite: "None",
    // [ts] }, {
    // [ts] url: "https://bar.com",
    // [ts] name: "catto",
    // [ts] value: "purrs",
    // [ts] sameSite: "Lax",
    // [ts] }, {
    // [ts] url: "https://baz.com",
    // [ts] name: "birdo",
    // [ts] value: "tweets",
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] const cookies = Set.init(context.cookies(["https://foo.com", "https://baz.com"]));
    // [ts] expect: (cookies) => {
    // [ts] name: "birdo",
    // [ts] value: "tweets",
    // [ts] domain: "baz.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "Lax",
    // [ts] }, {
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "None",
    // [ts] }]));
    // [ts] });
    // [ts] test("CO-010 should work with subdomain cookie", () => {
    // [ts] context.addCookies([{
    // [ts] domain: ".foo.com",
    // [ts] path: "/",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] sameSite: "Lax",
    // [ts] secure: true
    // [ts] }]);
    // [ts] expect: (context.cookies('https) => {
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] domain: ".foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] test("CO-011 should only return cookies that apply to the url", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43078" },
    // [ts] }, (_p) => {
    // [ts] context.addCookies([
    // [ts] { name: "hostonly", value: "v", domain: "foo.com", path: "/api" },
    // [ts] { name: "subdomains", value: "v", domain: ".foo.com", path: "/api" },
    // [ts] { name: "slash", value: "v", domain: "foo.com", path: "/foo/" },
    // [ts] ]);
    // [ts] const names = (url) => (context.cookies(url)).map(c => c.name).sort();
    // [ts] expect_eq(names("http://foo.com/api"), ["hostonly", "subdomains"]);
    // [ts] expect_eq(names("http://foo.com/api/x"), ["hostonly", "subdomains"]);
    // [ts] expect_eq(names("http://foo.com/apiv2"), []);
    // [ts] expect_eq(names("http://sub.foo.com/api"), ["subdomains"]);
    // [ts] expect_eq(names("http://notfoo.com/api"), []);
    // [ts] expect_eq(names("http://foo.com/foo"), []);
    // [ts] expect_eq(names("http://foo.com/foo/"), ["slash"]);
    // [ts] expect_eq(names("http://foo.com/foo/bar"), ["slash"]);
    // [ts] });
    // [ts] test("CO-012 should return cookies with empty value", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "name=;Path=/");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = context.cookies();
    // [ts] expect(cookies).toEqual([
    // [ts] expect.objectContaining({
    // [ts] name: "name",
    // [ts] value: ""
    // [ts] })
    // [ts] ]);
    // [ts] });
    // [ts] test("CO-013 should return secure cookies based on HTTP(S) protocol", () => {
    // [ts] context.addCookies([{
    // [ts] url: "https://foo.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] sameSite: "Lax",
    // [ts] secure: true
    // [ts] }, {
    // [ts] url: "http://foo.com",
    // [ts] name: "catto",
    // [ts] value: "purrs",
    // [ts] sameSite: "Lax",
    // [ts] secure: false
    // [ts] }]);
    // [ts] const cookies = Set.init(context.cookies("https://foo.com"));
    // [ts] expect: (cookies) => {
    // [ts] name: "catto",
    // [ts] value: "purrs",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }, {
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "Lax",
    // [ts] }]));
    // [ts] expect: (context.cookies('http) => {
    // [ts] name: "catto",
    // [ts] value: "purrs",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] test("CO-015 should add cookies with an expiration", () => {
    // [ts] const expires = Math.floor((Date.now() / 1000)) + 3600;
    // [ts] context.addCookies([{
    // [ts] url: "https://foo.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] sameSite: "None",
    // [ts] expires,
    // [ts] }]);
    // [ts] const cookies = context.cookies(["https://foo.com"]);
    // [ts] expect_eq(cookies.length, 1);
    // [ts] expect: (cookies) => {
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "None",
    // [ts] }]);
    // [ts] {
    // Rollover to 5-digit year
    // [ts] context.addCookies([{
    // [ts] url: "https://foo.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] sameSite: "None",
    // [ts] expires: 253402300799, // Fri, 31 Dec 9999 23:59:59 +0000 (UTC)
    // [ts] }]);
    // [ts] expect(context.addCookies([{
    // [ts] url: "https://foo.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] sameSite: "None",
    // [ts] expires: 253402300800, // Sat,  1 Jan 1000 00:00:00 +0000 (UTC)
    // [ts] }])).rejects.toThrow("Cookie should have a valid expires");
    // [ts] }
    // [ts] expect(context.addCookies([{
    // [ts] url: "https://foo.com",
    // [ts] name: "doggo",
    // [ts] value: "woofs",
    // [ts] sameSite: "None",
    // [ts] expires: -42,
    // [ts] }])).rejects.toThrow("Cookie should have a valid expires");
    // [ts] });
    // [ts] test("CO-016 should support requestStorageAccess", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/17285" });
    // [ts] test(browserName == "chromium", "requestStorageAccess API is not available in Chromium");
    // [ts] server.setRoute("/set-cookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "name=value; Path=/");
    // [ts] res.end();
    // [ts] });
    // Navigate once to the domain as top level.
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/set-cookie.html");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<iframe src="${server.CROSS_PROCESS_PREFIX + "/empty.html"}"></iframe>`);
    // [ts] const frame = page.frames()[1];
    // [ts] if (browserName == "firefox") {
    // [ts] expect_eq(frame.evaluate(() => document.hasStorageAccess()), true);
    // [ts] {
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/title.html"),
    // [ts] frame.evaluate(() => fetch("/title.html"))
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.cookie, "name=value");
    // [ts] }
    // [ts] } else {
    // [ts] if (browserName == "webkit" && (isLinux || channel == "webkit-wsl"))
    // [ts] expect_eq(frame.evaluate(() => document.hasStorageAccess()), true);
    // [ts] else {
    // [ts] expect_eq(frame.evaluate(() => document.hasStorageAccess()), false);
    // [ts] {
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/title.html"),
    // [ts] frame.evaluate(() => fetch("/title.html"))
    // [ts] ]);
    // [ts] if (isWindows && browserName == "webkit" && channel != "webkit-wsl")
    // [ts] expect_eq(serverRequest.headers.cookie, "name=value");
    // [ts] else {
    // [ts] expect_eq(serverRequest.headers.cookie, false);
    // [ts] }
    // [ts] expect_eq(frame.evaluate(() => document.requestStorageAccess().then(() => true, e => false)), true);
    // [ts] expect_eq(frame.evaluate(() => document.hasStorageAccess()), true);
    // [ts] {
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/title.html"),
    // [ts] frame.evaluate(() => fetch("/title.html"))
    // [ts] ]);
    // [ts] if (browserName == "webkit" && (isLinux || channel == "webkit-wsl"))
    // [ts] expect_eq(serverRequest.headers.cookie, null);
    // [ts] else {
    // [ts] expect_eq(serverRequest.headers.cookie, "name=value");
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("CO-017 should parse cookie with large Max-Age correctly", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30305" });
    // [ts] server.setRoute("/foobar", (req, res) => {
    // [ts] res.setHeader("set-cookie", [
    // [ts] "cookie1=value1; Path=/; Expires=Thu, 08 Sep 2270 15:06:12 GMT; Max-Age=7776000000"
    // [ts] ]);
    // [ts] res.statusCode = 200;
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/foobar");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "cookie1=value1");
    // [ts] expect(page.context().cookies()).toEqual([
    // [ts] {
    // [ts] name: "cookie1",
    // [ts] value: "value1",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: expect.any(Number),
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("CO-018 iframe should inherit cookies from parent", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35439" } }, (_p) => {
    // [ts] page.route("**/*", (route, request) => {
    // [ts] if (request.url().includes("sub.example.test")) {
    // [ts] route.fulfill({
    // [ts] body: `
    // [ts] <p id="result"></p>
    // [ts] <script>document.getElementById('result').textContent = document.cookie || 'no cookies';</script>
    // [ts] `,
    // [ts] contentType: "text/html",
    // [ts] });
    // [ts] return;
    // [ts] }
    // [ts] route.fulfill({
    // [ts] headers: { "set-cookie": "testCookie=value; SameSite=Lax; Domain=example.test" },
    // [ts] contentType: "text/html",
    // [ts] body: `
    // [ts] <p id="result"></p>
    // [ts] <script>document.getElementById('result').textContent = document.cookie || 'no cookies';</script>
    // [ts] <iframe src="http://sub.example.test"></iframe>
    // [ts] `
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://example.test");
    // [ts] expect(page.locator("body")).toContainText("testCookie=value");
    // [ts] expect(page.frameLocator("iframe").locator("body")).toContainText("testCookie=value");
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
}
