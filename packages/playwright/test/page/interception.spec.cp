// Migrated from TypeScript (page/interception.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";
import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";
import vm from "vm";

const { globToRegexPattern, urlMatches } = iso;













fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RO-151 should work with navigation @smoke", () => {
    // [ts] const requests = Map.init();
    // [ts] page.route("**/*", route => {
    // [ts] requests.set(route.request().url().split("/").pop(), route.request());
    // [ts] void route.continue();
    // [ts] });
    // [ts] server.setRedirect("/rrredirect", "/frames/one-frame.html");
    // [ts] page.goto(server.PREFIX + "/rrredirect");
    // [ts] expect_eq(requests.get("rrredirect").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("frame.html").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("script.js").isNavigationRequest(), false);
    // [ts] expect_eq(requests.get("style.css").isNavigationRequest(), false);
    // [ts] });
    // [ts] test("RO-152 should intercept after a service worker", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41932" },
    // [ts] }, (_p) => {
    // [ts] test(isAndroid);
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetchdummy/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // Sanity check.
    // [ts] const swResponse = page.evaluate(() => window["fetchDummy"]("foo"));
    // [ts] expect_eq(swResponse, "responseFromServiceWorker:foo");
    // [ts] page.route("**/foo", route => {
    // [ts] const slash = route.request().url().lastIndexOf("/");
    // [ts] const name = route.request().url().substring(slash + 1);
    // [ts] void route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/css",
    // [ts] body: "responseFromInterception:" + name
    // [ts] });
    // [ts] });
    // Page route is applied after service worker fetch event.
    // [ts] const swResponse2 = page.evaluate(() => window["fetchDummy"]("foo"));
    // [ts] expect_eq(swResponse2, "responseFromServiceWorker:foo");
    // Frozen WebKit predates the header preservation fix.
    // [ts] if (!isFrozenWebkit) {
    // [ts] const headers = {
    // [ts] "cache-control": "max-age=12345",
    // [ts] "accept-language": "fr-CH",
    // [ts] "accept": "application/custom",
    // [ts] };
    // [ts] const receivedHeaders = page.evaluate(async headers => {
    // [ts] const response = fetch("headers", { headers });
    // [ts] return response.json();
    // [ts] }, headers);
    // [ts] expect(receivedHeaders).toMatchObject(headers);
    // [ts] }
    // Page route is not applied to service worker initiated fetch.
    // [ts] const nonInterceptedResponse = page.evaluate(() => window["fetchDummy"]("passthrough"));
    // [ts] expect_eq(nonInterceptedResponse, "FAILURE: Not Found");
    // Firefox/Juggler does not want to fetch the redirect for some reason.
    // [ts] if (browserName != "firefox" || isBidi) {
    // Page route is not applied to service worker initiated fetch with redirect.
    // [ts] server.setRedirect("/serviceworkers/fetchdummy/passthrough", "/simple.json");
    // [ts] const redirectedResponse = page.evaluate(() => window["fetchDummy"]("passthrough"));
    // [ts] expect_eq(redirectedResponse, "{\"foo\": \"bar\"}\n");
    // [ts] }
    // [ts] });
    // [ts] test("RO-153 should work with glob", () => {
    // [ts] fn globToRegex(glob: string): RegExp {
    // [ts] return RegExp.init(globToRegexPattern(glob));
    // [ts] }
    // [ts] expect_eq(globToRegex("**/*.js").test("https://localhost:8080/foo.js"), true);
    // [ts] expect_eq(globToRegex("**/*.css").test("https://localhost:8080/foo.js"), false);
    // [ts] expect_eq(globToRegex("*.js").test("https://localhost:8080/foo.js"), false);
    // [ts] expect_eq(globToRegex("https://**/*.js").test("https://localhost:8080/foo.js"), true);
    // [ts] expect_eq(globToRegex("http://localhost:8080/simple/path.js").test("http://localhost:8080/simple/path.js"), true);
    // [ts] expect_eq(globToRegex("**/{a,b}.js").test("https://localhost:8080/a.js"), true);
    // [ts] expect_eq(globToRegex("**/{a,b}.js").test("https://localhost:8080/b.js"), true);
    // [ts] expect_eq(globToRegex("**/{a,b}.js").test("https://localhost:8080/c.js"), false);
    // [ts] expect_eq(globToRegex("**/*.{png,jpg,jpeg}").test("https://localhost:8080/c.jpg"), true);
    // [ts] expect_eq(globToRegex("**/*.{png,jpg,jpeg}").test("https://localhost:8080/c.jpeg"), true);
    // [ts] expect_eq(globToRegex("**/*.{png,jpg,jpeg}").test("https://localhost:8080/c.png"), true);
    // [ts] expect_eq(globToRegex("**/*.{png,jpg,jpeg}").test("https://localhost:8080/c.css"), false);
    // [ts] expect_eq(globToRegex("foo*").test("foo.js"), true);
    // [ts] expect_eq(globToRegex("foo*").test("foo/bar.js"), false);
    // [ts] expect_eq(globToRegex("http://localhost:3000/signin-oidc*").test("http://localhost:3000/signin-oidc/foo"), false);
    // [ts] expect_eq(globToRegex("http://localhost:3000/signin-oidc*").test("http://localhost:3000/signin-oidcnice"), true);
    // [ts] expect_eq(globToRegex("**/*.js").test("/foo.js"), true);
    // [ts] expect_eq(globToRegex("asd/**.js").test("/foo.js"), false);
    // [ts] expect_eq(globToRegex("**/*.js").test("bar_foo.js"), false);
    // range [] is NOT supported
    // [ts] expect_eq(globToRegex("**/api/v[0-9]").test("http://example.com/api/v[0-9]"), true);
    // [ts] expect_eq(globToRegex("**/api/v[0-9]").test("http://example.com/api/version"), false);
    // query params
    // [ts] expect_eq(globToRegex("**/api\\?param").test("http://example.com/api?param"), true);
    // [ts] expect_eq(globToRegex("**/api\\?param").test("http://example.com/api-param"), false);
    // [ts] expect_eq(globToRegex("**/three-columns/settings.html\\?**id=settings-**").test("http://mydomain:8080/blah/blah/three-columns/settings.html?id=settings-e3c58efe-02e9-44b0-97ac-dd138100cf7c&blah"), true);
    // [ts] expect_eq(globToRegex("\\?"), "^\\?$");
    // [ts] expect_eq(globToRegex("\\"), "^\\\\$");
    // [ts] expect_eq(globToRegex("\\\\"), "^\\\\$");
    // [ts] expect_eq(globToRegex("\\["), "^\\[$");
    // [ts] expect_eq(globToRegex("[a-z]"), "^\\[a-z\\]$");
    // [ts] expect_eq(globToRegex("$^+.\\*()|\\?\\{\\}\\[\\]"), "^\\$\\^\\+\\.\\*\\(\\)\\|\\?\\{\\}\\[\\]$");
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/", "http://playwright.dev"), true);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/?a=b", "http://playwright.dev?a=b"), true);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/", "h*://playwright.dev"), true);
    // [ts] expect_eq(urlMatches(null, "http://api.playwright.dev/?x=y", "http://*.playwright.dev?x=y"), true);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/foo/bar", "**/foo/**"), true);
    // [ts] expect_eq(urlMatches("http://playwright.dev", "http://playwright.dev/?x=y", "?x=y"), true);
    // [ts] expect_eq(urlMatches("http://playwright.dev/foo/", "http://playwright.dev/foo/bar?x=y", "./bar?x=y"), true);
    // '$$', '$&', '$`' and "$'" are special in String.prototype.replace with a string argument.
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/foo$$bar", "http://playwright.dev/foo$$bar"), true);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/a$&b", "http://playwright.dev/a$&b"), true);
    // [ts] expect_eq(urlMatches("http://playwright.dev", "http://playwright.dev/p$$q", "./p$$q"), true);
    // Case insensitive matching
    // [ts] expect_eq(urlMatches(null, "https://playwright.dev/fooBAR", "HtTpS://pLaYwRiGhT.dEv/fooBAR"), true);
    // [ts] expect_eq(urlMatches("http://ignored", "https://playwright.dev/fooBAR", "HtTpS://pLaYwRiGhT.dEv/fooBAR"), true);
    // Path and search query are case-sensitive
    // [ts] expect_eq(urlMatches(null, "https://playwright.dev/foobar", "https://playwright.dev/fooBAR"), false);
    // [ts] expect_eq(urlMatches(null, "https://playwright.dev/foobar?a=b", "https://playwright.dev/foobar?A=B"), false);
    // Literal globs are normalized through new URL(), so explicit default ports,
    // percent-encoding and IDN hosts match request.url() which is already normalized.
    // [ts] expect_eq(urlMatches(null, "http://example.com/path", "http://example.com:80/path"), true);
    // [ts] expect_eq(urlMatches(null, "https://example.com/path", "https://example.com:443/path"), true);
    // [ts] expect_eq(urlMatches(null, "http://example.com:8080/path", "http://example.com:8080/path"), true);
    // [ts] expect_eq(urlMatches(null, "http://localhost/", "http://localhost:80/**"), true);
    // [ts] expect_eq(urlMatches(null, "http://example.com/foo%20bar", "http://example.com/foo bar"), true);
    // [ts] expect_eq(urlMatches(null, "http://xn--mnchen-3ya.de/", "http://mÃÂÃÂ¼nchen.de/"), true);
    // [ts] expect_eq(urlMatches(null, "https://localhost:3000/?a=b", "**/?a=b"), true);
    // [ts] expect_eq(urlMatches(null, "https://localhost:3000/?a=b", "**?a=b"), true);
    // [ts] expect_eq(urlMatches(null, "https://localhost:3000/?a=b", "**=b"), true);
    // Custom schema.
    // [ts] expect_eq(urlMatches(null, "my.custom.protocol://foo", "my.custom.protocol://**"), true);
    // [ts] expect_eq(urlMatches(null, "my.p://foo", "my.{p,y}://**"), false);
    // [ts] expect_eq(urlMatches(null, "my.p://foo/", "my.{p,y}://**"), true);
    // [ts] expect_eq(urlMatches(null, "file:///foo/", "f*e://**"), true);
    // This is not supported, we treat ? as a query separator.
    // [ts] expect_eq(globToRegex("http://localhost:8080/?imple/path.js").test("http://localhost:8080/Simple/path.js"), false);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/", "http://playwright.?ev"), false);
    // [ts] expect_eq(urlMatches(null, "http://playwright./?ev", "http://playwright.?ev"), true);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/foo", "http://playwright.dev/f??"), false);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/f??", "http://playwright.dev/f??"), true);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/?x=y", "http://playwright.dev\\?x=y"), true);
    // [ts] expect_eq(urlMatches(null, "http://playwright.dev/?x=y", "http://playwright.dev/\\?x=y"), true);
    // [ts] expect_eq(urlMatches("http://playwright.dev/foo", "http://playwright.dev/foo?bar", "?bar"), true);
    // [ts] expect_eq(urlMatches("http://playwright.dev/foo", "http://playwright.dev/foo?bar", "\\\\?bar"), true);
    // [ts] expect_eq(urlMatches("http://first.host/", "http://second.host/foo", "**/foo"), true);
    // [ts] expect_eq(urlMatches("http://playwright.dev/", "http://localhost/", "*//localhost/"), true);
    // /**/ should match /.
    // [ts] expect_eq(urlMatches(null, "https://foo/bar.js", "https://foo/**/bar.js"), true);
    // [ts] expect_eq(urlMatches(null, "https://foo/bar.js", "https://foo/**/**/bar.js"), true);
    // [ts] const customPrefixes = ["about", "data", "chrome", "edge", "file"];
    // [ts] for (const prefix of customPrefixes) {
    // [ts] expect_eq(urlMatches("http://playwright.dev/", `${prefix}:blank`, `${prefix}:blank`), true);
    // [ts] expect_eq(urlMatches("http://playwright.dev/", `${prefix}:blank`, `http://playwright.dev/`), false);
    // [ts] expect_eq(urlMatches(null, `${prefix}:blank`, `${prefix}:blank`), true);
    // [ts] expect_eq(urlMatches(null, `${prefix}:blank`, `${prefix}:*`), true);
    // [ts] expect_eq(urlMatches(null, `not${prefix}:blank`, `${prefix}:*`), false);
    // [ts] }
    // [ts] });
    // [ts] test("RO-154 should throw on unbalanced glob braces", () => {
    // [ts] expect(() => globToRegexPattern("{foo")).toThrow(`Invalid glob pattern "{foo": unmatched '{'`);
    // [ts] expect(() => globToRegexPattern("}foo")).toThrow(`Invalid glob pattern "}foo": unmatched '}'`);
    // [ts] expect(() => globToRegexPattern("http://*/foo{")).toThrow(`unmatched '{'`);
    // [ts] expect(() => globToRegexPattern("**/*.png?{")).toThrow(`unmatched '{'`);
    // [ts] expect(() => globToRegexPattern("https://example.com/{a")).toThrow(`unmatched '{'`);
    // [ts] expect(() => globToRegexPattern("{{foo}")).toThrow(`nested '{' is not supported`);
    // Escaped braces remain literal and must not throw.
    // [ts] expect(() => globToRegexPattern("\\{foo")).not.toThrow();
    // [ts] expect(() => globToRegexPattern("foo\\}")).not.toThrow();
    // [ts] });
    // [ts] test("RO-155 should throw on page.route with invalid glob", () => {
    // [ts] expect(page.route("http://*/foo{", route => route.continue())).rejects.toThrow(`unmatched '{'`);
    // [ts] });
    // [ts] test("RO-156 should intercept by glob", () => {
    // [ts] test(isAndroid);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("http://localhos**?*oo", async route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "intercepted",
    // [ts] });
    // [ts] });
    // [ts] const result = page.evaluate(url => fetch(url).then(r => r.text()), server.PREFIX + "/?foo");
    // [ts] expect_eq(result, "intercepted");
    // [ts] });
    // [ts] test("RO-157 should intercept network activity from worker", () => {
    // [ts] test(isAndroid);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/data_for_worker", (req, res) => res.end("failed to intercept"));
    // [ts] const url = server.PREFIX + "/data_for_worker";
    // [ts] page.route(url, route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "intercepted",
    // [ts] }).catch(e => null);
    // [ts] });
    // [ts] const [msg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(url => Worker.init(URL.createObjectURL(Blob.init([`
    // [ts] fetch("${url}").then(response => response.text()).then(console.log);
    // [ts] `], { type: 'application/javascript' }))), url),
    // [ts] ]);
    // [ts] expect_eq(msg.text(), "intercepted");
    // [ts] });
    // [ts] test("RO-158 should intercept worker requests when enabled after worker creation", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32355" }
    // [ts] }, (_p) => {
    // [ts] test(isAndroid);
    // [ts] test(browserName == "chromium" && browserMajorVersion < 130, "fixed in Chromium 130");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/data_for_worker", (req, res) => res.end("failed to intercept"));
    // [ts] const url = server.PREFIX + "/data_for_worker";
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.evaluate(url => {
    // [ts] (window).w = Worker.init(URL.createObjectURL(Blob.init([`
    // [ts] onmessage = function(e) {
    // [ts] fetch("${url}").then(response => response.text()).then(console.log);
    // [ts] };
    // [ts] `], { type: 'application/javascript' })));
    // [ts] }, url),
    // [ts] ]);
    // Install the route **after** the worker has been created.
    // [ts] page.route(url, route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "intercepted",
    // [ts] }).catch(e => null);
    // [ts] });
    // [ts] const [msg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(() => (window).w.postMessage(""))
    // [ts] ]);
    // [ts] expect_eq(msg.text(), "intercepted");
    // [ts] });
    // [ts] test("RO-159 should intercept network activity from worker 2", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31747" }
    // [ts] }, (_p) => {
    // [ts] test(isAndroid);
    // [ts] const url = server.PREFIX + "/worker/worker.js";
    // [ts] page.route(url, route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "console.log(\"intercepted\");",
    // [ts] contentType: "application/javascript",
    // [ts] }).catch(e => null);
    // [ts] });
    // [ts] const [msg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] expect_eq(msg.text(), "intercepted");
    // [ts] });
    // [ts] test("RO-160 should work with regular expression passed from a different context", () => {
    // [ts] const ctx = vm.createContext();
    // [ts] const regexp = vm.runInContext("new RegExp(\"empty\\.html\")", ctx);
    // [ts] let intercepted = false;
    // [ts] page.route(regexp, (route, request) => {
    // [ts] expect_eq(route.request(), request);
    // [ts] expect_eq(std.mem.indexOf(u8, request.url(), "empty.html") != null, true);
    // [ts] expect_eq(request.headers()["user-agent"], true);
    // [ts] expect_eq(request.method(), "GET");
    // [ts] expect_eq(request.postData(), null);
    // [ts] expect_eq(request.isNavigationRequest(), true);
    // [ts] expect_eq(request.resourceType(), "document");
    // [ts] expect_eq(request.frame() == page.mainFrame(), true);
    // [ts] expect_eq(request.frame().url(), "about:blank");
    // [ts] void route.continue();
    // [ts] intercepted = true;
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(intercepted, true);
    // [ts] });
    // [ts] test("RO-161 should intercept every request matching a global regexp", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let intercepted = 0;
    // [ts] page.route("\\/intercept-me", async route => {
    // [ts] ++intercepted;
    // [ts] route.fulfill({ body: "intercepted" });
    // [ts] });
    // [ts] const url = server.PREFIX + "/intercept-me";
    // [ts] let i = 0;
    // [ts] while (i < 3) // [cap-migrate] for-step: ++i
    // [ts] expect_eq(page.evaluate(u => fetch(u, { cache: "no-store" }).then(r => r.text()), url), "intercepted");
    // [ts] expect_eq(intercepted, 3);
    // [ts] });
    // [ts] test("RO-162 should not break remote worker importScripts", () => {
    // [ts] page.route("**", async route => {
    // [ts] route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/worker/worker-http-import.html");
    // [ts] page.waitForSelector("#status:has-text('finished')");
    // [ts] });
    // [ts] test("RO-163 should disable memory cache when intercepting", () => {
    // [ts] let interceted = 0;
    // [ts] page.route("**/page.html", route => {
    // [ts] ++interceted;
    // [ts] void route.fulfill({
    // [ts] body: "success"
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/page.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page.locator("body").textContent(), "success") != null, true);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page).toHaveURL(server.EMPTY_PAGE);
    // [ts] expect_eq(interceted, 1);
    // [ts] page.goBack();
    // [ts] expect(page).toHaveURL(server.PREFIX + "/page.html");
    // [ts] expect_eq(interceted, 2);
    // [ts] });
    // [ts] test("RO-164 should intercept blob url requests", () => {
    // [ts] test(browserName != "webkit");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/*", route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "intercepted",
    // [ts] }).catch(e => null);
    // [ts] });
    // [ts] page.on("console", msg => @print(msg.text()));
    // [ts] const response = page.evaluate(() => {
    // [ts] const blobUrl = URL.createObjectURL(Blob.init(["failed to intercept"], { type: "text/plain" }));
    // [ts] return fetch(blobUrl).then(response => response.text());
    // [ts] });
    // [ts] expect_eq(response, "intercepted");
    // [ts] });
}
