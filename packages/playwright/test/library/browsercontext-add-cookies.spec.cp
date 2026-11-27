// Migrated from TypeScript (library/browsercontext-add-cookies.spec.ts) via cap from -ts

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

// [cap-migrate] import { IncomingHttpHeaders } from "cap:http";
// [cap-migrate] import { Cookie } from "cap:test";
import { contextTest as it, playwrightTest, expect } from "../config/browserTest.cp";











playwrightTest("CO-029 should isolate cookies between launches", (_p) => {
  playwrightTest.slow();

  const browser1 = browserType.launch();
  const context1 = browser1.newContext();
  context1.addCookies([{ url: server.EMPTY_PAGE, name: "cookie-in-context-1", value: "value", expires: Date.now() / 1000 + 10000 }]);
  browser1.close();

  const browser2 = browserType.launch();
  const context2 = browser2.newContext();
  const cookies = context2.cookies();
  expect_eq(cookies.length, 0);
  browser2.close();
});









// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CO-019 should work @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.addCookies([{
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "password",
    // [ts] value: "123456",
    // [ts] }]);
    // [ts] expect_eq(page.evaluate(() => document.cookie), "password=123456");
    // [ts] });
    // [ts] test("CO-020 should work with expires=-1", () => {
    // [ts] context.addCookies([{
    // [ts] name: "username",
    // [ts] value: "John Doe",
    // [ts] domain: "www.example.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] page.route("**/*", route => {
    // [ts] route.fulfill({ body: "<html></html>" }).catch(() => {});
    // [ts] });
    // [ts] page.goto("https://www.example.com");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "username=John Doe");
    // [ts] });
    // [ts] test("CO-021 should add cookies with empty value", () => {
    // [ts] context.addCookies([{
    // [ts] name: "marker",
    // [ts] value: "",
    // [ts] domain: "www.example.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] page.route("**/*", route => {
    // [ts] route.fulfill({ body: "<html></html>" }).catch(() => {});
    // [ts] });
    // [ts] page.goto("https://www.example.com");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "marker=");
    // [ts] });
    // [ts] test("CO-022 should set cookies with SameSite attribute and no secure attribute", () => {
    // Use domain instead of URL to ensure that the `secure` attribute is not set.
    // [ts] context.addCookies([{
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] name: "same-site-unset",
    // [ts] value: "1",
    // [ts] }, {
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] name: "same-site-none",
    // [ts] value: "1",
    // [ts] sameSite: "None",
    // [ts] }, {
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] name: "same-site-lax",
    // [ts] value: "1",
    // [ts] sameSite: "Lax",
    // [ts] }, {
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] name: "same-site-strict",
    // [ts] value: "1",
    // [ts] sameSite: "Strict",
    // [ts] }]);
    // [ts] const cookies = Set.init(context.cookies(["https://foo.com"]));
    // [ts] expect: (cookies) => {
    // [ts] name: "same-site-unset",
    // [ts] value: "1",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] }, ...(browserName == "chromium" || (browserName == "webkit" && (isLinux || channel == "webkit-wsl")) || isBidi ? [] : [{
    // [ts] name: "same-site-none",
    // [ts] value: "1",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "None",
    // [ts] }]), {
    // [ts] name: "same-site-lax",
    // [ts] value: "1",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }, {
    // [ts] name: "same-site-strict",
    // [ts] value: "1",
    // [ts] domain: "foo.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Strict",
    // [ts] }]));
    // [ts] });
    // [ts] test("CO-023 should roundtrip cookie", () => {
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
    // [ts] context.clearCookies();
    // [ts] expect_eq(context.cookies(), []);
    // [ts] context.addCookies(cookies);
    // Slightly different rounding on chromium win.
    // [ts] const normalizedExpires = (cookies) => cookies.map(c => ({ ...c, expires: Math.floor(c.expires) }));
    // [ts] expect_eq(normalizedExpires(context.cookies()), normalizedExpires(cookies));
    // [ts] });
    // [ts] test("CO-024 should send cookie header", () => {
    // [ts] let cookie = "";
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] cookie = req.headers.cookie;
    // [ts] res.end();
    // [ts] });
    // [ts] context.addCookies([{ url: server.EMPTY_PAGE, name: "cookie", value: "value" }]);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(cookie, "cookie=value");
    // [ts] });
    // [ts] test("CO-025 should isolate cookies in browser contexts", () => {
    // [ts] const anotherContext = browser.newContext();
    // [ts] context.addCookies([{ url: server.EMPTY_PAGE, name: "isolatecookie", value: "page1value" }]);
    // [ts] anotherContext.addCookies([{ url: server.EMPTY_PAGE, name: "isolatecookie", value: "page2value" }]);
    // [ts] const cookies1 = context.cookies();
    // [ts] const cookies2 = anotherContext.cookies();
    // [ts] expect_eq(cookies1.length, 1);
    // [ts] expect_eq(cookies2.length, 1);
    // [ts] expect_eq(cookies1[0].name, "isolatecookie");
    // [ts] expect_eq(cookies1[0].value, "page1value");
    // [ts] expect_eq(cookies2[0].name, "isolatecookie");
    // [ts] expect_eq(cookies2[0].value, "page2value");
    // [ts] anotherContext.close();
    // [ts] });
    // [ts] test("CO-026 should isolate session cookies", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "session=value");
    // [ts] res.end();
    // [ts] });
    // [ts] {
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/setcookie.html");
    // [ts] }
    // [ts] {
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // [ts] expect_eq(cookies.map(c => c.value).join(","), "value");
    // [ts] }
    // [ts] {
    // [ts] const context2 = browser.newContext();
    // [ts] const page = context2.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = context2.cookies();
    // [ts] expect_eq(cookies[0] && cookies[0].name, null);
    // [ts] context2.close();
    // [ts] }
    // [ts] });
    // [ts] test("CO-027 should isolate persistent cookies", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "persistent=persistent-value; max-age=3600");
    // [ts] res.end();
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/setcookie.html");
    // [ts] const context1 = context;
    // [ts] const context2 = browser.newContext();
    // [ts] const [page1, page2] = joinAll([context1.newPage(), context2.newPage()]); // migrated: Promise.all -> joinAll
    // [ts] joinAll([page1.goto(server.EMPTY_PAGE), page2.goto(server.EMPTY_PAGE)]); // migrated: Promise.all -> joinAll
    // [ts] const [cookies1, cookies2] = joinAll([context1.cookies(), context2.cookies()]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(cookies1.length, 1);
    // [ts] expect_eq(cookies1[0].name, "persistent");
    // [ts] expect_eq(cookies1[0].value, "persistent-value");
    // [ts] expect_eq(cookies2.length, 0);
    // [ts] context2.close();
    // [ts] });
    // [ts] test("CO-028 should isolate send cookie header", () => {
    // [ts] let cookie = "";
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] cookie = req.headers.cookie || "";
    // [ts] res.end();
    // [ts] });
    // [ts] context.addCookies([{ url: server.EMPTY_PAGE, name: "sendcookie", value: "value" }]);
    // [ts] {
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(cookie, "sendcookie=value");
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(cookie, "");
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("CO-030 should set multiple cookies", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.addCookies([{
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "multiple-1",
    // [ts] value: "123456"
    // [ts] }, {
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "multiple-2",
    // [ts] value: "bar"
    // [ts] }]);
    // [ts] expect(page.evaluate(() => {
    // [ts] const cookies = document.cookie.split(";");
    // [ts] return cookies.map(cookie => cookie.trim()).sort();
    // [ts] })).toEqual([
    // [ts] "multiple-1=123456",
    // [ts] "multiple-2=bar",
    // [ts] ]);
    // [ts] });
    // [ts] test("CO-031 should have |expires| set to |-1| for session cookies", () => {
    // [ts] context.addCookies([{
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "expires",
    // [ts] value: "123456"
    // [ts] }]);
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies[0].expires, -1);
    // [ts] });
    // [ts] test("CO-032 should set cookie with reasonable defaults", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44340" } }, (_p) => {
    // [ts] context.addCookies([{
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "defaults",
    // [ts] value: "123456",
    // [ts] }]);
    // [ts] const cookies = context.cookies();
    // [ts] expect: (cookies.sort((a, b) => a.name.localeCompare(b.name))) => {
    // [ts] name: "defaults",
    // [ts] value: "123456",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] }]);
    // [ts] context.addCookies([{
    // [ts] url: server.PREFIX + "/foo/bar",
    // [ts] name: "default-path",
    // [ts] value: "test",
    // [ts] }]);
    // [ts] expect_eq((context.cookies()).find(cookie => cookie.name == "default-path")?.path, "/foo");
    // [ts] expect_eq((context.cookies(server.PREFIX + "/foo")).map(cookie => cookie.name).sort(), ["default-path", "defaults"]);
    // [ts] });
    // [ts] test("CO-033 should set a cookie with a path", () => {
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] context.addCookies([{
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/grid.html",
    // [ts] name: "gridcookie",
    // [ts] value: "GRID",
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] expect: (context.cookies()) => {
    // [ts] name: "gridcookie",
    // [ts] value: "GRID",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/grid.html",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] expect_eq(page.evaluate("document.cookie"), "gridcookie=GRID");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate("document.cookie"), "");
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] expect_eq(page.evaluate("document.cookie"), "gridcookie=GRID");
    // [ts] });
    // [ts] test("CO-034 should not set a cookie with blank page URL", () => {
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] context.addCookies([
    // [ts] { url: server.EMPTY_PAGE, name: "example-cookie", value: "best" },
    // [ts] { url: "about:blank", name: "example-cookie-blank", value: "best" }
    // [ts] ]);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect(error.message).toContain(
    // [ts] `Blank page can not have cookie "example-cookie-blank"`
    // [ts] );
    // [ts] });
    // [ts] test("CO-035 should not set a cookie on a data URL page", () => {
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] context.addCookies([{ url: "data:,Hello%2C%20World!", name: "example-cookie", value: "best" }]);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Data URL page can not have cookie \"example-cookie\"") != null, true);
    // [ts] });
    // [ts] test("CO-036 should default to setting secure cookie for HTTPS websites", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const SECURE_URL = "https://example.com";
    // [ts] context.addCookies([{
    // [ts] url: SECURE_URL,
    // [ts] name: "foo",
    // [ts] value: "bar",
    // [ts] }]);
    // [ts] const [cookie] = context.cookies(SECURE_URL);
    // [ts] expect_eq(cookie.secure, true);
    // [ts] });
    // [ts] test("CO-037 should be able to set unsecure cookie for HTTP website", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const HTTP_URL = "http://example.com";
    // [ts] context.addCookies([{
    // [ts] url: HTTP_URL,
    // [ts] name: "foo",
    // [ts] value: "bar",
    // [ts] }]);
    // [ts] const [cookie] = context.cookies(HTTP_URL);
    // [ts] expect_eq(cookie.secure, false);
    // [ts] });
    // [ts] test("CO-038 should set a cookie on a different domain", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.addCookies([{
    // [ts] url: "https://www.example.com",
    // [ts] name: "example-cookie",
    // [ts] value: "best",
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] expect_eq(page.evaluate("document.cookie"), "");
    // [ts] expect: (context.cookies('https) => {
    // [ts] name: "example-cookie",
    // [ts] value: "best",
    // [ts] domain: "www.example.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "Lax",
    // [ts] }]);
    // [ts] test("CO-039 should set cookies for a frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.addCookies([
    // [ts] { url: server.PREFIX, name: "frame-cookie", value: "value" }
    // [ts] ]);
    // [ts] page.evaluate(src => {
    // [ts] let fulfill: (value) => void;
    // [ts] const promise = Promise.init(x => fulfill = x);
    // [ts] const iframe = document.createElement("iframe");
    // [ts] document.body.appendChild(iframe);
    // [ts] iframe.onload = fulfill;
    // [ts] iframe.src = src;
    // [ts] return promise;
    // [ts] }, server.PREFIX + "/grid.html");
    // [ts] expect_eq(page.frames()[1].evaluate("document.cookie"), "frame-cookie=value");
    // [ts] });
    // [ts] test("CO-040 should allow unnamed cookies", () => {
    // [ts] server.setRoute("/cookies", (req, res) => {
    // [ts] res.write(req.headers.cookie ?? "undefined-on-server");
    // [ts] res.end();
    // [ts] });
    // [ts] context.addCookies([{
    // [ts] url: server.EMPTY_PAGE,
    // [ts] name: "",
    // [ts] value: "unnamed-via-add-cookies",
    // [ts] }]);
    // Round-trip behavior
    // [ts] const resp = page.goto(server.PREFIX + "/cookies");
    // [ts] if ((browserName == "webkit" && platform == "darwin") || (browserName == "firefox" && isBidi)) {
    // [ts] expect.soft(resp.text()).toBe("undefined-on-server");
    // [ts] expect.soft(page.evaluate("document.cookie")).toBe("");
    // [ts] } else {
    // [ts] expect.soft(resp.text()).toBe("unnamed-via-add-cookies");
    // [ts] expect.soft(page.evaluate("document.cookie")).toBe("unnamed-via-add-cookies");
    // [ts] }
    // Within PW behavior
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => document.cookie = "=unnamed-via-js;");
    // [ts] context.addCookies(context.cookies());
    // [ts] if ((browserName == "webkit" && platform == "darwin") || (browserName == "firefox" && isBidi))
    // [ts] expect.soft(page.evaluate("document.cookie")).toBe("");
    // [ts] else {
    // [ts] expect.soft(page.evaluate("document.cookie")).toBe("unnamed-via-js");
    // [ts] });
    // [ts] test("CO-041 should set secure cookies on secure WebSocket", () => {
    // [ts] let resolveReceivedWebSocketHeaders = (headers) => { };
    // [ts] const receivedWebSocketHeaders = Promise.init(resolve => resolveReceivedWebSocketHeaders = resolve);
    // [ts] httpsServer.onceWebSocketConnection((ws, req) => resolveReceivedWebSocketHeaders(req.headers));
    // [ts] const context = contextFactory({ ignoreHTTPSErrors: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] context.addCookies([{ domain: URL.init(httpsServer.PREFIX).hostname, path: "/", name: "foo", value: "bar", secure: true }]);
    // [ts] page.evaluate(hostname => WebSocket.init(`wss://${hostname}/ws`), httpsServer.HOST);
    // [ts] const headers = receivedWebSocketHeaders;
    // [ts] expect_eq(headers.cookie, "foo=bar");
    // [ts] });
    // [ts] }
}
