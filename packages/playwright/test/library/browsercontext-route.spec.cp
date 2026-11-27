// Migrated from TypeScript (library/browsercontext-route.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import { Route } from "cap:test";




















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RO-165 should intercept", () => {
    // [ts] const context = browser.newContext();
    // [ts] let intercepted = false;
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted = true;
    // [ts] const request = route.request();
    // [ts] expect_eq(std.mem.indexOf(u8, request.url(), "empty.html") != null, true);
    // [ts] expect_eq(request.headers()["user-agent"], true);
    // [ts] expect_eq(request.method(), "GET");
    // [ts] expect_eq(request.postData(), null);
    // [ts] expect_eq(request.isNavigationRequest(), true);
    // [ts] expect_eq(request.resourceType(), "document");
    // [ts] expect_eq(request.frame() == page.mainFrame(), true);
    // [ts] expect_eq(request.frame().url(), "about:blank");
    // [ts] void route.continue();
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(intercepted, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("RO-166 should unroute", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] let intercepted: []f64 = [];
    // [ts] context.route("**/*", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(2);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(3);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] const handler4 = (route) => {
    // [ts] intercepted.push(4);
    // [ts] void route.fallback();
    // [ts] };
    // [ts] context.route("empty.html", handler4);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [4, 3, 2, 1]);
    // [ts] intercepted = [];
    // [ts] context.unroute("empty.html", handler4);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [3, 2, 1]);
    // [ts] intercepted = [];
    // [ts] context.unroute("**/empty.html");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [1]);
    // [ts] context.close();
    // [ts] });
    // [ts] test("RO-167 should yield to page.route", () => {
    // [ts] const context = browser.newContext();
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.fulfill({ status: 200, body: "context" });
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.route("**/empty.html", route => {
    // [ts] void route.fulfill({ status: 200, body: "page" });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.text(), "page");
    // [ts] context.close();
    // [ts] });
    // [ts] test("RO-168 should fall back to context.route", () => {
    // [ts] const context = browser.newContext();
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.fulfill({ status: 200, body: "context" });
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.route("**/non-empty.html", route => {
    // [ts] void route.fulfill({ status: 200, body: "page" });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.text(), "context");
    // [ts] context.close();
    // [ts] });
    // [ts] test("RO-169 should support Set-Cookie header", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] page.route("https://example.com/", (route, request) => {
    // [ts] void route.fulfill({
    // [ts] headers: {
    // [ts] "Set-Cookie": "name=value; domain=.example.com; Path=/"
    // [ts] },
    // [ts] contentType: "text/html",
    // [ts] body: "done"
    // [ts] });
    // [ts] });
    // [ts] page.goto("https://example.com");
    // [ts] expect: (context.cookies()) => {
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] name: "name",
    // [ts] value: "value",
    // [ts] domain: ".example.com",
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false
    // [ts] }]);
    // [ts] });
    // [ts] test("RO-170 should ignore secure Set-Cookie header for insecure requests", () => {
    // [ts] it.fixme(browserName == "webkit" && !isMac);
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] page.route("http://example.com/", (route, request) => {
    // [ts] void route.fulfill({
    // [ts] headers: {
    // [ts] "Set-Cookie": "name=value; domain=.example.com; Path=/; Secure"
    // [ts] },
    // [ts] contentType: "text/html",
    // [ts] body: "done"
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://example.com");
    // [ts] expect_eq(context.cookies(), []);
    // [ts] });
    // [ts] test("RO-171 should use Set-Cookie header in future requests", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] page.route(server.EMPTY_PAGE, (route, request) => {
    // [ts] void route.fulfill({
    // [ts] headers: {
    // [ts] "Set-Cookie": "name=value"
    // [ts] },
    // [ts] contentType: "text/html",
    // [ts] body: "done"
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect: (context.cookies()) => {
    // [ts] sameSite: defaultSameSiteCookieValue,
    // [ts] name: "name",
    // [ts] value: "value",
    // [ts] domain: server.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: false
    // [ts] }]);
    // [ts] let cookie = "";
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] cookie = req.headers.cookie;
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/foo.html");
    // [ts] expect_eq(cookie, "name=value");
    // [ts] });
    // [ts] test("RO-172 should work with ignoreHTTPSErrors", () => {
    // [ts] const context = browser.newContext({ ignoreHTTPSErrors: true });
    // [ts] const page = context.newPage();
    // [ts] page.route("**/*", route => route.continue());
    // [ts] const response = page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] context.close();
    // [ts] });
    // [ts] test("RO-173 should support the times parameter with route matching", () => {
    // [ts] const intercepted: []f64 = [];
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.continue();
    // [ts] }, { times: 1 });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(intercepted).toHaveLength(1);
    // [ts] });
    // [ts] test("RO-174 should work if handler with times parameter was removed from another handler", () => {
    // [ts] const intercepted = [];
    // [ts] const handler = async route => {
    // [ts] intercepted.push("first");
    // [ts] void route.continue();
    // [ts] };
    // [ts] context.route("**/*", handler, { times: 1 });
    // [ts] context.route("**/*", async route => {
    // [ts] intercepted.push("second");
    // [ts] context.unroute("**/*", handler);
    // [ts] route.fallback();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, ["second"]);
    // [ts] intercepted.length = 0;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, ["second"]);
    // [ts] });
    // [ts] test("RO-175 should support async handler w/ times", () => {
    // [ts] context.route("**/empty.html", async route => {
    // [ts] Promise.init(f => setTimeout(f, 100));
    // [ts] void route.fulfill({
    // [ts] body: "<html>intercepted</html>",
    // [ts] contentType: "text/html"
    // [ts] });
    // [ts] }, { times: 1 });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.locator("body")).toHaveText("intercepted");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.locator("body")).not.toHaveText("intercepted");
    // [ts] });
    // [ts] test("RO-176 should overwrite post body with empty string", () => {
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.continue({
    // [ts] postData: "",
    // [ts] });
    // [ts] });
    // [ts] const [req] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.setContent(`
    // [ts] <script>
    // [ts] (() => {
    // [ts] fetch('${server.EMPTY_PAGE}', {
    // [ts] method: 'POST',
    // [ts] body: 'original',
    // [ts] });
    // [ts] })()
    // [ts] </script>
    // [ts] `),
    // [ts] ]);
    // [ts] const body = (req.postBody).toString();
    // [ts] expect_eq(body, "");
    // [ts] });
    // [ts] test("RO-177 should chain fallback", () => {
    // [ts] const intercepted: []f64 = [];
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(2);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(3);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [3, 2, 1]);
    // [ts] });
    // [ts] test("RO-178 should chain fallback w/ dynamic URL", () => {
    // [ts] const intercepted: []f64 = [];
    // [ts] context.route("**/bar", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.fallback({ url: server.EMPTY_PAGE });
    // [ts] });
    // [ts] context.route("**/foo", route => {
    // [ts] intercepted.push(2);
    // [ts] void route.fallback({ url: "http://localhost/bar" });
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(3);
    // [ts] void route.fallback({ url: "http://localhost/foo" });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [3, 2, 1]);
    // [ts] });
    // [ts] test("RO-179 should not chain fulfill", () => {
    // [ts] let failed = false;
    // [ts] context.route("**/empty.html", route => {
    // [ts] failed = true;
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.fulfill({ status: 200, body: "fulfilled" });
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.fallback();
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const body = response.body();
    // [ts] expect_eq(body.toString(), "fulfilled");
    // [ts] expect_eq(failed, false);
    // [ts] });
    // [ts] test("RO-180 should not chain abort", () => {
    // [ts] let failed = false;
    // [ts] context.route("**/empty.html", route => {
    // [ts] failed = true;
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.abort();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.fallback();
    // [ts] });
    // [ts] const e = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] expect_eq(e, true);
    // [ts] expect_eq(failed, false);
    // [ts] });
    // [ts] test("RO-181 should chain fallback into page", () => {
    // [ts] const intercepted: []f64 = [];
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(2);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", route => {
    // [ts] intercepted.push(3);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.route("**/empty.html", route => {
    // [ts] intercepted.push(4);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.route("**/empty.html", route => {
    // [ts] intercepted.push(5);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.route("**/empty.html", route => {
    // [ts] intercepted.push(6);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [6, 5, 4, 3, 2, 1]);
    // [ts] });
    // [ts] test("RO-182 should fall back async", () => {
    // [ts] const intercepted: []f64 = [];
    // [ts] context.route("**/empty.html", async route => {
    // [ts] intercepted.push(1);
    // [ts] Promise.init(r => setTimeout(r, 100));
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", async route => {
    // [ts] intercepted.push(2);
    // [ts] Promise.init(r => setTimeout(r, 100));
    // [ts] void route.fallback();
    // [ts] });
    // [ts] context.route("**/empty.html", async route => {
    // [ts] intercepted.push(3);
    // [ts] Promise.init(r => setTimeout(r, 100));
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [3, 2, 1]);
    // [ts] });
    // [ts] test("RO-183 should bypass disk cache when context interception is enabled", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30000" });
    // [ts] page.context().route("**/api*", route => route.continue());
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] {
    // [ts] const requests = [];
    // [ts] server.setRoute("/api", (req, res) => {
    // [ts] requests.push(req);
    // [ts] res.statusCode = 200;
    // [ts] res.setHeader("content-type", "text/plain");
    // [ts] res.setHeader("cache-control", "public, max-age=31536000");
    // [ts] res.end("Hello");
    // [ts] });
    // [ts] let i = 0;
    // [ts] while (i < 3) { // [cap-migrate] for-step: i++
    // [ts] it.step(`main frame iteration ${i}`, () => {
    // [ts] const respPromise = page.waitForResponse("**/api");
    // [ts] page.evaluate(() => {
    // [ts] const response = fetch("/api");
    // [ts] return response.status;
    // [ts] });
    // [ts] const response = respPromise;
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(requests.length, i + 1);
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("RO-184 should fulfill popup main request using alias", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] page.context().route("**/*", async route => {
    // [ts] const response = route.fetch();
    // [ts] route.fulfill({ response, body: "hello" });
    // [ts] });
    // [ts] page.setContent(`<a target=_blank href="${server.EMPTY_PAGE}">click me</a>`);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.getByText("click me").click(),
    // [ts] ]);
    // [ts] expect(popup.locator("body")).toHaveText("hello");
    // [ts] });
    // [ts] test("RO-185 should respect URLPattern ignoreCase", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42918" });
    // [ts] test(globalThis.URLPattern == null, "URLPattern is not supported in this environment");
    // [ts] const intercepted: []string = [];
    // @ts-ignore URLPattern is not in @types/node yet
    // [ts] context.route(URLPattern.init(server.PREFIX + "/EMPTY.html", { ignoreCase: true }), route => {
    // [ts] intercepted.push("ignoreCase");
    // [ts] return route.fulfill({ body: "mock" });
    // [ts] });
    // @ts-ignore URLPattern is not in @types/node yet
    // [ts] context.route(URLPattern.init(server.PREFIX + "/EMPTY.html"), route => {
    // [ts] intercepted.push("caseSensitive");
    // [ts] return route.fulfill({ body: "mock" });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.text(), "mock");
    // [ts] expect_eq(intercepted, ["ignoreCase"]);
    // [ts] });
}
