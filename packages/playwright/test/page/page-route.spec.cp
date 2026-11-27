// Migrated from TypeScript (page/page-route.spec.ts) via cap from -ts

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

// [cap-migrate] import * as http from "cap:http";
// [cap-migrate] import { Route } from "playwright";
import { test as it, expect } from "./pageTest.cp";





// @see https://github.com/GoogleChrome/puppeteer/issues/3973

// @see https://github.com/GoogleChrome/puppeteer/issues/4743





// @see https://github.com/GoogleChrome/puppeteer/issues/4337

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RO-001 should intercept @smoke", () => {
    // [ts] let intercepted = false;
    // [ts] page.route("**/empty.html", (route, request) => {
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
    // [ts] test("RO-002 should unroute", () => {
    // [ts] let intercepted = [];
    // [ts] page.route("**/*", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.route("**/empty.html", route => {
    // [ts] intercepted.push(2);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] page.route("**/empty.html", route => {
    // [ts] intercepted.push(3);
    // [ts] void route.fallback();
    // [ts] });
    // [ts] const handler4 = route => {
    // [ts] intercepted.push(4);
    // [ts] void route.fallback();
    // [ts] };
    // [ts] page.route("empty.html", handler4);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [4, 3, 2, 1]);
    // [ts] intercepted = [];
    // [ts] page.unroute("empty.html", handler4);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [3, 2, 1]);
    // [ts] intercepted = [];
    // [ts] page.unroute("**/empty.html");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [1]);
    // [ts] });
    // [ts] test("RO-003 should not support ? in glob pattern", () => {
    // [ts] server.setRoute("/index", (req, res) => res.end("index-no-hello"));
    // [ts] server.setRoute("/index123hello", (req, res) => res.end("index123hello"));
    // [ts] server.setRoute("/index?hello", (req, res) => res.end("index?hello"));
    // [ts] server.setRoute("/index1hello", (req, res) => res.end("index1hello"));
    // [ts] page.route("**/index?hello", (route, request) => {
    // [ts] route.fulfill({ body: "intercepted any character" });
    // [ts] });
    // [ts] page.route("**/index\\?hello", (route, request) => {
    // [ts] route.fulfill({ body: "intercepted question mark" });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/index?hello");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "intercepted question mark") != null, true);
    // [ts] page.goto(server.PREFIX + "/index");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "index-no-hello") != null, true);
    // [ts] page.goto(server.PREFIX + "/index1hello");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "index1hello") != null, true);
    // [ts] page.goto(server.PREFIX + "/index123hello");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "index123hello") != null, true);
    // [ts] });
    // [ts] test("RO-004 should work when POST is redirected with 302", () => {
    // [ts] server.setRedirect("/rredirect", "/empty.html");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/*", route => route.continue());
    // [ts] page.setContent(`
    // [ts] <form action='/rredirect' method='post'>
    // [ts] <input type="hidden" id="foo" name="foo" value="FOOBAR">
    // [ts] </form>
    // [ts] `);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('form', form => form.submit()),
    // [ts] page.waitForNavigation()
    // [ts] ]);
    // [ts] });
    // [ts] test("RO-005 should work when header manipulation headers with redirect", () => {
    // [ts] server.setRedirect("/rrredirect", "/empty.html");
    // [ts] page.route("**/*", route => {
    // [ts] const headers = Object.assign({}, route.request().headers(), {
    // [ts] foo: "bar"
    // [ts] });
    // [ts] void route.continue({ headers });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/rrredirect");
    // [ts] });
    // [ts] test("RO-006 should be able to remove headers", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/*", async route => {
    // [ts] const headers = { ...route.request().headers() };
    // [ts] delete headers["foo"];
    // [ts] void route.continue({ headers });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/title.html"),
    // [ts] page.evaluate(url => fetch(url, { headers: { foo: "bar" } }), server.PREFIX + "/title.html")
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.foo, null);
    // [ts] });
    // [ts] test("RO-007 should contain referer header", () => {
    // [ts] const requests = [];
    // [ts] page.route("**/*", route => {
    // [ts] requests.push(route.request());
    // [ts] void route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, requests[1].url(), "/one-style.css") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requests[1].headers().referer, "/one-style.html") != null, true);
    // [ts] });
    // [ts] test("RO-008 should properly return navigation response when URL has cookies", () => {
    // [ts] test(isAndroid, "No isolated context");
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // Setup cookie.
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.context().addCookies([{ url: server.EMPTY_PAGE, name: "foo", value: "bar" }]);
    // Setup request interception.
    // [ts] page.route("**/*", route => route.continue());
    // [ts] const response = page.reload();
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("RO-009 should not override cookie header", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16773" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => document.cookie = "original=value");
    // [ts] let cookieValueInRoute;
    // [ts] page.route("**", async route => {
    // [ts] const headers = route.request().allHeaders();
    // [ts] cookieValueInRoute = headers["cookie"];
    // [ts] headers["cookie"] = "overridden=value";
    // [ts] void route.continue({ headers });
    // [ts] });
    // [ts] const [serverReq] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] if (browserName != "webkit")
    // [ts] expect.soft(cookieValueInRoute).toBe("original=value");
    // [ts] expect.soft(serverReq.headers["cookie"]).toBe("original=value");
    // [ts] });
    // [ts] test("RO-010 should show custom HTTP headers", () => {
    // [ts] page.setExtraHTTPHeaders({
    // [ts] foo: "bar"
    // [ts] });
    // [ts] page.route("**/*", route => {
    // [ts] expect_eq(route.request().headers()["foo"], "bar");
    // [ts] void route.continue();
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("RO-011 should work with redirect inside sync XHR", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28461" });
    // [ts] it.fixme(browserName == "webkit", "No Network.requestIntercepted for the request");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRedirect("/logo.png", "/pptr.png");
    // [ts] let continuePromise;
    // [ts] page.route("**/*", route => {
    // [ts] continuePromise = route.continue();
    // [ts] });
    // [ts] const status = page.evaluate(() => {
    // [ts] const request = XMLHttpRequest.init();
    // [ts] request.open("GET", "/logo.png", false);  // `false` makes the request synchronous
    // [ts] request.send(null);
    // [ts] return request.status;
    // [ts] });
    // [ts] expect_eq(status, 200);
    // [ts] expect_eq(continuePromise, true);
    // [ts] continuePromise;
    // [ts] });
    // [ts] test("RO-012 should pause intercepted XHR until continue", () => {
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] it.fixme(browserName == "webkit", "Redirected request is not paused in WebKit");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let resolveRoute;
    // [ts] const routePromise = Promise.init(r => resolveRoute = r);
    // [ts] page.route("**/global-var.html", async route => resolveRoute(route));
    // [ts] let xhrFinished = false;
    // [ts] page.on("requestfinished", request => {
    // [ts] if (request.url().endsWith("/global-var.html"))
    // [ts] xhrFinished = true;
    // [ts] });
    // The synchronous request blocks the page, so it is started from a timer.
    // [ts] page.evaluate(() => {
    // [ts] (window).__statusPromise = Promise.init(resolve => setTimeout(() => { const request = XMLHttpRequest.init(); request.open("GET", "/global-var.html", false);  // `false` makes the request synchronous request.send(null); resolve(request.status); }, 0));
    // [ts] });
    // [ts] const route = routePromise;
    // Check that intercepted request is actually paused.
    // [ts] Promise.init(r => setTimeout(r, 500));
    // [ts] expect_eq(xhrFinished, false);
    // [ts] (route).continue();
    // [ts] expect_eq(page.evaluate(() => (window).__statusPromise), 200);
    // [ts] });
    // [ts] test("RO-013 should pause intercepted fetch request until continue", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let resolveRoute;
    // [ts] const routePromise = Promise.init(r => resolveRoute = r);
    // [ts] page.route("**/global-var.html", async route => resolveRoute(route));
    // [ts] page.evaluate(() => {
    // [ts] (window).__fetchFinished = false;
    // [ts] (window).__statusPromise = fetch("/global-var.html").then(response => { (window).__fetchFinished = true; return response.status; });
    // [ts] });
    // [ts] const route = routePromise;
    // Check that intercepted request is actually paused.
    // [ts] Promise.init(r => setTimeout(r, 500));
    // [ts] expect_eq(page.evaluate(() => (window).__fetchFinished), false);
    // [ts] (route).continue();
    // [ts] expect_eq(page.evaluate(() => (window).__statusPromise), 200);
    // [ts] });
    // [ts] test("RO-014 should work with custom referer headers", () => {
    // [ts] page.setExtraHTTPHeaders({ "referer": server.EMPTY_PAGE });
    // [ts] page.route("**/*", route => {
    // See https://github.com/microsoft/playwright/issues/8999
    // [ts] if (browserName == "chromium" && browserMajorVersion < 154)
    // [ts] expect_eq(route.request().headers()["referer"], server.EMPTY_PAGE + ", " + server.EMPTY_PAGE);
    // [ts] else {
    // [ts] expect_eq(route.request().headers()["referer"], server.EMPTY_PAGE);
    // [ts] void route.continue();
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("RO-015 should be abortable", () => {
    // [ts] page.route("\\.css$", route => route.abort());
    // [ts] let failed = false;
    // [ts] page.on("requestfailed", request => {
    // [ts] if (request.url().includes(".css"))
    // [ts] failed = true;
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.request().failure(), null);
    // [ts] expect_eq(failed, true);
    // [ts] });
    // [ts] test("RO-016 should be abortable with custom error codes", () => {
    // [ts] page.route("**/*", route => route.abort("internetdisconnected"));
    // [ts] let failedRequest = null;
    // [ts] page.on("requestfailed", request => failedRequest = request);
    // [ts] page.goto(server.EMPTY_PAGE).catch(e => {});
    // [ts] expect_eq(failedRequest, true);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(failedRequest.failure().errorText, if (isMac && macVersion < 11) "Request intercepted" else "Blocked by Web Inspector");
    // [ts] else if (browserName == "firefox")
    // [ts] expect_eq(failedRequest.failure().errorText, "NS_ERROR_OFFLINE");
    // [ts] else {
    // [ts] expect_eq(failedRequest.failure().errorText, "net::ERR_INTERNET_DISCONNECTED");
    // [ts] });
    // [ts] test("RO-017 should not throw if request was cancelled by the page", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28490" });
    // [ts] let interceptCallback;
    // [ts] const interceptPromise = Promise.init(f => interceptCallback = f);
    // [ts] page.route("**/data.json", route => interceptCallback(route));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(url => {
    // [ts] globalThis.controller = AbortController.init();
    // [ts] return fetch(url, { signal: globalThis.controller.signal });
    // [ts] }, server.PREFIX + "/data.json").catch(() => {});
    // [ts] const route = interceptPromise;
    // [ts] const failurePromise = page.waitForEvent("requestfailed");
    // [ts] page.evaluate(() => globalThis.controller.abort());
    // [ts] const cancelledRequest = failurePromise;
    // [ts] expect(cancelledRequest.failure().errorText).toMatch("cancelled|aborted");
    // [ts] route.abort(); // Should not throw.
    // [ts] });
    // [ts] test("RO-018 should send referer", () => {
    // [ts] page.setExtraHTTPHeaders({
    // [ts] referer: "http://google.com/"
    // [ts] });
    // [ts] page.route("**/*", route => route.continue());
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/grid.html"),
    // [ts] page.goto(server.PREFIX + "/grid.html"),
    // [ts] ]);
    // [ts] expect_eq(request.headers["referer"], "http://google.com/");
    // [ts] });
    // [ts] test("RO-019 should fail navigation when aborting main resource", () => {
    // [ts] page.route("**/*", route => route.abort());
    // [ts] let error = null;
    // [ts] page.goto(server.EMPTY_PAGE).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, if (isMac && macVersion < 11) "Request intercepted" else "Blocked by Web Inspector") != null, true);
    // [ts] else if (browserName == "firefox" && isBidi)
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_ERROR_ABORT") != null, true);
    // [ts] else if (browserName == "firefox")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_ERROR_FAILURE") != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "net::ERR_FAILED") != null, true);
    // [ts] });
    // [ts] test("RO-020 should not work with redirects", () => {
    // [ts] const intercepted = [];
    // [ts] page.route("**/*", route => {
    // [ts] void route.continue();
    // [ts] intercepted.push(route.request());
    // [ts] });
    // [ts] server.setRedirect("/non-existing-page.html", "/non-existing-page-2.html");
    // [ts] server.setRedirect("/non-existing-page-2.html", "/non-existing-page-3.html");
    // [ts] server.setRedirect("/non-existing-page-3.html", "/non-existing-page-4.html");
    // [ts] server.setRedirect("/non-existing-page-4.html", "/empty.html");
    // [ts] const response = page.goto(server.PREFIX + "/non-existing-page.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.url(), "empty.html") != null, true);
    // [ts] expect_eq(intercepted.length, 1);
    // [ts] expect_eq(intercepted[0].resourceType(), "document");
    // [ts] expect_eq(intercepted[0].isNavigationRequest(), true);
    // [ts] expect_eq(std.mem.indexOf(u8, intercepted[0].url(), "/non-existing-page.html") != null, true);
    // [ts] const chain = [];
    // [ts] let r = response.request();
    // [ts] while (r) { // [cap-migrate] for-step: r = r.redirectedFrom()
    // [ts] chain.push(r);
    // [ts] expect_eq(r.isNavigationRequest(), true);
    // [ts] }
    // [ts] expect_eq(chain.length, 5);
    // [ts] expect_eq(std.mem.indexOf(u8, chain[0].url(), "/empty.html") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, chain[1].url(), "/non-existing-page-4.html") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, chain[2].url(), "/non-existing-page-3.html") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, chain[3].url(), "/non-existing-page-2.html") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, chain[4].url(), "/non-existing-page.html") != null, true);
    // [ts] let i = 0;
    // [ts] while (i < chain.length) // [cap-migrate] for-step: i++
    // [ts] expect_eq(chain[i].redirectedTo(), if (i) chain[i - 1] else null);
    // [ts] });
    // [ts] test("RO-021 should chain fallback w/ dynamic URL", () => {
    // [ts] const intercepted = [];
    // [ts] page.route("**/bar", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.fallback({ url: server.EMPTY_PAGE });
    // [ts] });
    // [ts] page.route("**/foo", route => {
    // [ts] intercepted.push(2);
    // [ts] void route.fallback({ url: "http://localhost/bar" });
    // [ts] });
    // [ts] page.route("**/empty.html", route => {
    // [ts] intercepted.push(3);
    // [ts] void route.fallback({ url: "http://localhost/foo" });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [3, 2, 1]);
    // [ts] });
    // [ts] test("RO-022 should work with redirects for subresources", () => {
    // [ts] const intercepted = [];
    // [ts] page.route("**/*", route => {
    // [ts] void route.continue();
    // [ts] intercepted.push(route.request());
    // [ts] });
    // [ts] server.setRedirect("/one-style.css", "/two-style.css");
    // [ts] server.setRedirect("/two-style.css", "/three-style.css");
    // [ts] server.setRedirect("/three-style.css", "/four-style.css");
    // [ts] server.setRoute("/four-style.css", (req, res) => res.end("body {box-sizing: border-box; }"));
    // [ts] const response = page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.url(), "one-style.html") != null, true);
    // [ts] expect_eq(intercepted.length, 2);
    // [ts] expect_eq(intercepted[0].resourceType(), "document");
    // [ts] expect_eq(std.mem.indexOf(u8, intercepted[0].url(), "one-style.html") != null, true);
    // [ts] let r = intercepted[1];
    // [ts] for (const url of ["/one-style.css", "/two-style.css", "/three-style.css", "/four-style.css"]) {
    // [ts] expect_eq(r.resourceType(), "stylesheet");
    // [ts] expect_eq(std.mem.indexOf(u8, r.url(), url) != null, true);
    // [ts] r = r.redirectedTo();
    // [ts] }
    // [ts] expect_eq(r, null);
    // [ts] });
    // [ts] test("RO-023 should work with equal requests", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let responseCount = 1;
    // [ts] server.setRoute("/zzz", (req, res) => res.end((responseCount++) * 11 + ""));
    // [ts] let spinner = false;
    // Cancel 2nd request.
    // [ts] page.route("**/*", route => {
    // [ts] void (if (spinner) route.abort() else route.continue());
    // [ts] spinner = !spinner;
    // [ts] });
    // [ts] const results = [];
    // [ts] let i = 0;
    // [ts] while (i < 3) // [cap-migrate] for-step: i++
    // [ts] results.push(page.evaluate(() => fetch("/zzz").then(response => response.text()).catch(e => "FAILED")));
    // [ts] expect_eq(results, ["11", "FAILED", "22"]);
    // [ts] });
    // [ts] test("RO-024 should navigate to dataURL and not fire dataURL requests", () => {
    // [ts] const requests = [];
    // [ts] page.route("**/*", route => {
    // [ts] requests.push(route.request());
    // [ts] void route.continue();
    // [ts] });
    // [ts] const dataURL = "data:text/html,<div>yo</div>";
    // [ts] const response = page.goto(dataURL);
    // [ts] expect_eq(response, null);
    // [ts] expect_eq(requests.length, 0);
    // [ts] });
    // [ts] test("RO-025 should be able to fetch dataURL and not fire dataURL requests", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requests = [];
    // [ts] page.route("**/*", route => {
    // [ts] requests.push(route.request());
    // [ts] void route.continue();
    // [ts] });
    // [ts] const dataURL = "data:text/html,<div>yo</div>";
    // [ts] const text = page.evaluate(url => fetch(url).then(r => r.text()), dataURL);
    // [ts] expect_eq(text, "<div>yo</div>");
    // [ts] expect_eq(requests.length, 0);
    // [ts] });
    // [ts] test("RO-026 should navigate to URL with hash and and fire requests without hash", () => {
    // [ts] const requests = [];
    // [ts] page.route("**/*", route => {
    // [ts] requests.push(route.request());
    // [ts] void route.continue();
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE + "#hash");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(requests.length, 1);
    // [ts] expect_eq(requests[0].url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("RO-027 should work with encoded server", () => {
    // The requestWillBeSent will report encoded URL, whereas interception will
    // report URL as-is. @see crbug.com/759388
    // [ts] page.route("**/*", route => route.continue());
    // [ts] const response = page.goto(server.PREFIX + "/some nonexisting page");
    // [ts] expect_eq(response.status(), 404);
    // [ts] });
    // [ts] test("RO-028 should work with badly encoded server", () => {
    // [ts] server.setRoute("/malformed?rnd=%911", (req, res) => res.end());
    // [ts] page.route("**/*", route => route.continue());
    // [ts] const response = page.goto(server.PREFIX + "/malformed?rnd=%911");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("RO-029 should work with encoded server - 2", () => {
    // The requestWillBeSent will report URL as-is, whereas interception will
    // report encoded URL for stylesheet. @see crbug.com/759388
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requests = [];
    // [ts] page.route("**/*", route => {
    // [ts] void route.continue();
    // [ts] requests.push(route.request());
    // [ts] });
    // [ts] page.setContent(`<link rel="stylesheet" href="${server.PREFIX}/fonts?helvetica|arial"/>`);
    // [ts] expect_eq(requests.length, 1);
    // [ts] expect_eq((requests[0].response()).status(), 404);
    // [ts] });
    // [ts] test("RO-030 should not throw \"Invalid Interception Id\" if the request was cancelled", () => {
    // [ts] page.setContent("<iframe></iframe>");
    // [ts] let route = null;
    // [ts] page.route("**/*", async r => route = r);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: void page.$eval('iframe', (frame, url) => frame.src = url, server.EMPTY_PAGE);
    // Wait for request interception.
    // [ts] page.waitForEvent("request");
    // Delete frame to cause request to be canceled.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', frame => frame.remove());
    // [ts] let error = null;
    // [ts] route.continue().catch(e => error = e);
    // [ts] expect_eq(error, null);
    // [ts] });
    // [ts] test("RO-031 should intercept main resource during cross-process navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let intercepted = false;
    // [ts] page.route(server.CROSS_PROCESS_PREFIX + "/empty.html", route => {
    // [ts] intercepted = true;
    // [ts] void route.continue();
    // [ts] });
    // [ts] const response = page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(intercepted, true);
    // [ts] });
    // [ts] test("RO-032 should fulfill with redirect status", () => {
    // [ts] it.fixme(browserName == "webkit", "in WebKit the redirects are handled by the network stack and we intercept before");
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] server.setRoute("/final", (req, res) => res.end("foo"));
    // [ts] page.route("**/*", (route, request) => {
    // [ts] if (request.url() != server.PREFIX + "/redirect_this")
    // [ts] return route.continue();
    // [ts] route.fulfill({
    // [ts] status: 301,
    // [ts] headers: {
    // [ts] "location": "/final",
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url);
    // [ts] return data.text();
    // [ts] }, server.PREFIX + "/redirect_this");
    // [ts] expect_eq(text, "foo");
    // [ts] });
    // [ts] test("RO-033 should not fulfill with redirect status", () => {
    // [ts] test(browserName != "webkit", "we should support fulfill with redirect in webkit and delete this test");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] let status;
    // [ts] let fulfill;
    // [ts] let reject;
    // [ts] page.route("**/*", (route, request) => {
    // [ts] if (request.url() != server.PREFIX + "/redirect_this")
    // [ts] return route.continue();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] route.fulfill({
    // [ts] status,
    // [ts] headers: {
    // [ts] "location": "/empty.html",
    // [ts] }
    // [ts] });
    // [ts] reject("fulfill didn't throw");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] fulfill(e);
    // [ts] }
    // [ts] });
    // [ts] status = 300;
    // [ts] while (status < 310) { // [cap-migrate] for-step: status++
    // [ts] const [, exception] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(url => location.href = url, server.PREFIX + "/redirect_this"),
    // [ts] Promise.init((f, r) => {fulfill = f; reject = r;})
    // [ts] ]);
    // [ts] expect_eq(exception, true);
    // [ts] expect_eq(exception.message.includes("Cannot fulfill with redirect status"), true);
    // [ts] }
    // [ts] });
    // [ts] test("RO-034 should support cors with GET", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/cars*", (route, request) => {
    // [ts] const headers = { "access-control-allow-origin": request.url().endsWith("allow") ? "*" : "none" };
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] headers,
    // [ts] status: 200,
    // [ts] body: JSON.stringify(["electric", "gas"]),
    // [ts] });
    // [ts] });
    // [ts] {
    // Should succeed
    // [ts] const resp = page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars?allow", { mode: "cors" });
    // [ts] return response.json();
    // [ts] });
    // [ts] expect_eq(resp, ["electric", "gas"]);
    // [ts] }
    // [ts] {
    // Should be rejected
    // [ts] const error = page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars?reject", { mode: "cors" });
    // [ts] return response.json();
    // [ts] }).catch(e => e);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Failed") != null, true);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "TypeError") != null, true);
    // [ts] if (browserName == "firefox")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NetworkError") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("RO-035 should add Access-Control-Allow-Origin by default when fulfill", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/cars", async route => {
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] status: 200,
    // [ts] body: JSON.stringify(["electric", "gas"]),
    // [ts] });
    // [ts] });
    // [ts] const [result, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars", {
    // [ts] method: "POST",
    // [ts] headers: { "Content-Type": "application/json" },
    // [ts] mode: "cors",
    // [ts] body: JSON.stringify({ "number": 1 })
    // [ts] });
    // [ts] return response.json();
    // [ts] }),
    // [ts] page.waitForResponse("https://example.com/cars")
    // [ts] ]);
    // [ts] expect_eq(result, ["electric", "gas"]);
    // [ts] expect_eq(response.headerValue("Access-Control-Allow-Origin"), server.PREFIX);
    // [ts] });
    // [ts] test("RO-036 should allow null origin for about:blank", () => {
    // [ts] page.route("**/something", async route => {
    // [ts] route.fulfill({
    // [ts] contentType: "text/plain",
    // [ts] status: 200,
    // [ts] body: "done",
    // [ts] });
    // [ts] });
    // [ts] const [response, text] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.CROSS_PROCESS_PREFIX + "/something"),
    // [ts] page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] method: "GET",
    // [ts] headers: { "X-PINGOTHER": "pingpong" }
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something")
    // [ts] ]);
    // [ts] expect_eq(text, "done");
    // [ts] expect_eq(response.headerValue("Access-Control-Allow-Origin"), "null");
    // [ts] });
    // [ts] test("RO-037 should respect cors overrides", () => {
    // [ts] it.fail(isAndroid, "no cors error in android emulator");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] if (request.method == "OPTIONS") {
    // [ts] response.writeHead(204, {
    // [ts] "Access-Control-Allow-Origin": "*",
    // [ts] "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
    // [ts] "Access-Control-Allow-Headers": "*",
    // [ts] "Cache-Control": "no-cache"
    // [ts] });
    // [ts] response.end();
    // [ts] return;
    // [ts] }
    // [ts] response.writeHead(404, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("NOT FOUND");
    // [ts] });
    // Fetch request should fail when CORS header doesn't include the origin.
    // [ts] {
    // [ts] page.route("**/something", async route => {
    // [ts] route.fulfill({
    // [ts] contentType: "text/plain",
    // [ts] status: 200,
    // [ts] headers: { "Access-Control-Allow-Origin": "http://non-existent" },
    // [ts] body: "done",
    // [ts] });
    // [ts] });
    // [ts] const error = page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] method: "GET",
    // [ts] headers: { "X-PINGOTHER": "pingpong" }
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something").catch(e => e);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Failed to fetch") != null, true);
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Load failed") != null, true);
    // [ts] else if (browserName == "firefox")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NetworkError when attempting to fetch resource.") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("RO-038 should not auto-intercept non-preflight OPTIONS without network interception", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20469" });
    // [ts] it.fixme(isAndroid);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let requests = [];
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] requests.push(request.method + ":" + request.url);
    // [ts] if (request.method == "OPTIONS") {
    // [ts] response.writeHead(200, {
    // [ts] "Access-Control-Allow-Origin": "*",
    // [ts] "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
    // [ts] "Access-Control-Allow-Headers": "*",
    // [ts] "Cache-Control": "no-cache"
    // [ts] });
    // [ts] response.end(`Hello`);
    // [ts] return;
    // [ts] }
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("World");
    // [ts] });
    // Without interception.
    // [ts] {
    // [ts] requests = [];
    // [ts] const [text1, text2] = page.evaluate(async url => {
    // [ts] const response1 = fetch(url, { method: "OPTIONS" });
    // [ts] const text1 = response1.text();
    // [ts] const response2 = fetch(url, { method: "GET" });
    // [ts] const text2 = response2.text();
    // [ts] return [text1, text2];
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect.soft(text1).toBe("Hello");
    // [ts] expect.soft(text2).toBe("World");
    // Preflight for OPTIONS, then OPTIONS, then GET without preflight.
    // [ts] expect.soft(requests).toEqual(["OPTIONS:\"something", "OPTIONS:\"", "GET:/something"]);
    // [ts] }
    // [ts] });
    // Make sure this runs in a new context as preflight results could be cached.
    // [ts] test("RO-039 should not auto-intercept non-preflight OPTIONS with network interception", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20469" });
    // [ts] it.fixme(isAndroid);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let requests = [];
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] requests.push(request.method + ":" + request.url);
    // [ts] if (request.method == "OPTIONS") {
    // [ts] response.writeHead(200, {
    // [ts] "Access-Control-Allow-Origin": "*",
    // [ts] "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
    // [ts] "Access-Control-Allow-Headers": "*",
    // [ts] "Cache-Control": "no-cache"
    // [ts] });
    // [ts] response.end(`Hello`);
    // [ts] return;
    // [ts] }
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("World");
    // [ts] });
    // With interception.
    // [ts] {
    // [ts] page.route("**/something", route => route.continue());
    // [ts] requests = [];
    // [ts] const [text1, text2] = page.evaluate(async url => {
    // [ts] const response1 = fetch(url, { method: "OPTIONS" });
    // [ts] const text1 = response1.text();
    // [ts] const response2 = fetch(url, { method: "GET" });
    // [ts] const text2 = response2.text();
    // [ts] return [text1, text2];
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect.soft(text1).toBe("Hello");
    // [ts] expect.soft(text2).toBe("World");
    // Preflight for OPTIONS is auto-fulfilled, then OPTIONS, then GET without preflight.
    // [ts] if (browserName == "chromium" || isBidi)
    // [ts] expect.soft(requests).toEqual(["OPTIONS:\"something", "GET:\""]);
    // [ts] else {
    // [ts] expect.soft(requests).toEqual(["OPTIONS:\"something", "OPTIONS:\"", "GET:/something"]);
    // [ts] }
    // [ts] });
    // [ts] test("RO-040 should support cors with POST", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/cars", async route => {
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] headers: { "Access-Control-Allow-Origin": "*" },
    // [ts] status: 200,
    // [ts] body: JSON.stringify(["electric", "gas"]),
    // [ts] });
    // [ts] });
    // [ts] const resp = page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars", {
    // [ts] method: "POST",
    // [ts] headers: { "Content-Type": "application/json" },
    // [ts] mode: "cors",
    // [ts] body: JSON.stringify({ "number": 1 })
    // [ts] });
    // [ts] return response.json();
    // [ts] });
    // [ts] expect_eq(resp, ["electric", "gas"]);
    // [ts] });
    // [ts] test("RO-041 should support cors with credentials", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/cars", async route => {
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] headers: {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true"
    // [ts] },
    // [ts] status: 200,
    // [ts] body: JSON.stringify(["electric", "gas"]),
    // [ts] });
    // [ts] });
    // [ts] const resp = page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars", {
    // [ts] method: "POST",
    // [ts] headers: { "Content-Type": "application/json" },
    // [ts] mode: "cors",
    // [ts] body: JSON.stringify({ "number": 1 }),
    // [ts] credentials: "include"
    // [ts] });
    // [ts] return response.json();
    // [ts] });
    // [ts] expect_eq(resp, ["electric", "gas"]);
    // [ts] });
    // [ts] test("RO-042 should not abort preflighted cross-origin redirect when a route is registered", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43242" },
    // [ts] }, (_p) => {
    // [ts] test(isAndroid, "No cross-process on Android");
    // [ts] const allowCors = (request, response) => {
    // [ts] response.setHeader("Access-Control-Allow-Origin", request.headers.origin || "*");
    // [ts] response.setHeader("Access-Control-Allow-Credentials", "true");
    // [ts] response.setHeader("Access-Control-Allow-Headers", "x-app-version");
    // [ts] if (request.method != "OPTIONS")
    // [ts] return false;
    // [ts] response.writeHead(204);
    // [ts] response.end();
    // [ts] return true;
    // [ts] };
    // [ts] server.setRoute("/download", (request, response) => {
    // [ts] if (allowCors(request, response))
    // [ts] return;
    // [ts] response.writeHead(302, { location: `${server.PREFIX}/file` });
    // [ts] response.end();
    // [ts] });
    // [ts] server.setRoute("/file", (request, response) => {
    // [ts] if (allowCors(request, response))
    // [ts] return;
    // [ts] response.end("file contents");
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/never-matches", route => route.continue());
    // [ts] const result = page.evaluate(async url => {
    // [ts] const response = fetch(url, { credentials: "include", headers: { "X-App-Version": "1.2.3" } });
    // [ts] return `${response.status} ${await response.text()}`;
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/download");
    // [ts] expect_eq(result, "200 file contents");
    // [ts] });
    // [ts] test("RO-043 should reject cors with disallowed credentials", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/cars", async route => {
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] headers: {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // Should fail without this line below!
    // 'Access-Control-Allow-Credentials': 'true'
    // [ts] },
    // [ts] status: 200,
    // [ts] body: JSON.stringify(["electric", "gas"]),
    // [ts] });
    // [ts] });
    // [ts] let error = "";
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars", {
    // [ts] method: "POST",
    // [ts] headers: { "Content-Type": "application/json" },
    // [ts] mode: "cors",
    // [ts] body: JSON.stringify({ "number": 1 }),
    // [ts] credentials: "include"
    // [ts] });
    // [ts] return response.json();
    // [ts] });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("RO-044 should support cors for different methods", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/cars", (route, request) => {
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] headers: { "Access-Control-Allow-Origin": "*" },
    // [ts] status: 200,
    // [ts] body: JSON.stringify([request.method(), "electric", "gas"]),
    // [ts] });
    // [ts] });
    // First POST
    // [ts] {
    // [ts] const resp = page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars", {
    // [ts] method: "POST",
    // [ts] headers: { "Content-Type": "application/json" },
    // [ts] mode: "cors",
    // [ts] body: JSON.stringify({ "number": 1 })
    // [ts] });
    // [ts] return response.json();
    // [ts] });
    // [ts] expect_eq(resp, ["POST", "electric", "gas"]);
    // [ts] }
    // Then DELETE
    // [ts] {
    // [ts] const resp = page.evaluate(() => {
    // [ts] const response = fetch("https://example.com/cars", {
    // [ts] method: "DELETE",
    // [ts] headers: {},
    // [ts] mode: "cors",
    // [ts] body: ""
    // [ts] });
    // [ts] return response.json();
    // [ts] });
    // [ts] expect_eq(resp, ["DELETE", "electric", "gas"]);
    // [ts] }
    // [ts] });
    // [ts] test("RO-045 should support the times parameter with route matching", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43507" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] fn fetchBody() {
    // [ts] return page.evaluate(() => fetch("/empty.html").then(response => response.text()));
    // [ts] }
    // [ts] const intercepted = [];
    // [ts] page.route("**/empty.html", route => {
    // [ts] intercepted.push(1);
    // [ts] void route.fulfill({ body: "first" });
    // [ts] }, { times: 1 });
    // [ts] expect_eq(fetchBody(), "first");
    // [ts] page.route("**/empty.html", route => route.fulfill({ body: "second" }), { times: 1 });
    // [ts] expect_eq(fetchBody(), "second");
    // [ts] fetchBody();
    // [ts] fetchBody();
    // [ts] expect(intercepted).toHaveLength(1);
    // [ts] });
    // [ts] test("RO-046 should work if handler with times parameter was removed from another handler", () => {
    // [ts] const intercepted = [];
    // [ts] const handler = async route => {
    // [ts] intercepted.push("first");
    // [ts] void route.continue();
    // [ts] };
    // [ts] page.route("**/*", handler, { times: 1 });
    // [ts] page.route("**/*", async route => {
    // [ts] intercepted.push("second");
    // [ts] page.unroute("**/*", handler);
    // [ts] route.fallback();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, ["second"]);
    // [ts] intercepted.length = 0;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, ["second"]);
    // [ts] });
    // [ts] test("RO-047 should support async handler w/ times", () => {
    // [ts] page.route("**/empty.html", async route => {
    // [ts] Promise.init(f => setTimeout(f, 100));
    // [ts] route.fulfill({
    // [ts] body: "<html>intercepted</html>",
    // [ts] contentType: "text/html"
    // [ts] });
    // [ts] }, { times: 1 });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.locator("body")).toHaveText("intercepted");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.locator("body")).not.toHaveText("intercepted");
    // [ts] });
    // [ts] test("RO-048 route abort with times: 1 should not affect second sequential fetch", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41802" });
    // [ts] test(browserName == "chromium" && browserMajorVersion < 155, "Chromium drops a request that is intercepted while Fetch.disable is being processed; fix is not rolled yet");
    // [ts] server.setRoute("/data", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/plain" });
    // [ts] res.end("ok");
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/data", route => route.abort("timedout"), { times: 1 });
    // [ts] const results = page.evaluate(() => {
    // [ts] async fn fetchOrHung(url: string) {
    // [ts] const timeout = Promise.init(resolve => setTimeout(() => resolve("hung"), 3000));
    // [ts] const request = fetch(url).then(r => String(r.status)).catch(() => "aborted");
    // [ts] return race([request, timeout]); // migrated: Promise.race -> race
    // [ts] }
    // [ts] const first = fetchOrHung("/data");
    // [ts] const second = fetchOrHung("/data");
    // [ts] return [first, second];
    // [ts] });
    // [ts] expect_eq(results, ["aborted", "200"]);
    // [ts] });
    // [ts] test("RO-049 should contain raw request header", () => {
    // [ts] let headers: any;
    // [ts] page.route("**/*", async route => {
    // [ts] headers = route.request().allHeaders();
    // [ts] void route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(headers.accept, true);
    // [ts] });
    // [ts] test("RO-050 should contain sec-fetch headers in route", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42620" });
    // [ts] it.fail(browserName == "chromium", "Fetch.requestPaused fires before Chromium attaches Fetch Metadata headers");
    // [ts] const secFetch = (headers) => Object.fromEntries(
    // [ts] Object.entries(headers).filter((_p) => name.toLowerCase().startsWith("sec-fetch-")));
    // [ts] const modes = ["cors", "no-cors", "same-origin"];
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const routed = Map.init();
    // [ts] page.route("**/probe-*", async route => {
    // [ts] const request = route.request();
    // [ts] const path = URL.init(request.url()).pathname;
    // [ts] routed.set(path, { headers: secFetch(request.headers()), allHeaders: secFetch(request.allHeaders()) });
    // [ts] void route.continue();
    // [ts] });
    // [ts] const [serverRequests] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] joinAll(modes.map(mode => server.waitForRequest(`/probe-${mode}`))), // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(async modes => {
    // [ts] for (const mode of modes)
    // [ts] (fetch(`/probe-${mode}`, { mode: mode })).text();
    // [ts] }, modes),
    // [ts] ]);
    // [ts] let i = 0;
    // [ts] while (i < modes.length) { // [cap-migrate] for-step: i++
    // [ts] const path = `/probe-${modes[i]}`;
    // [ts] const onServer = secFetch(serverRequests[i].headers);
    // The server does receive the browser-computed Fetch Metadata.
    // [ts] expect.soft(onServer, `${path} on server`).toEqual({
    // [ts] "sec-fetch-site": "same-origin",
    // [ts] "sec-fetch-mode": modes[i],
    // [ts] "sec-fetch-dest": "empty",
    // [ts] });
    // The route handler should see the same headers.
    // [ts] expect.soft(routed.get(path).headers, `${path} route.request().headers()`).toEqual(onServer);
    // [ts] expect.soft(routed.get(path).allHeaders, `${path} route.request().allHeaders()`).toEqual(onServer);
    // [ts] }
    // [ts] });
    // [ts] test("RO-051 should contain raw response header", () => {
    // [ts] let request: any;
    // [ts] page.route("**/*", async route => {
    // [ts] request = route.request();
    // [ts] void route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const response = request.response();
    // [ts] const headers = response.allHeaders();
    // [ts] expect_eq(headers["content-type"], true);
    // [ts] });
    // [ts] test("RO-052 should contain raw response header after fulfill", () => {
    // [ts] let request: any;
    // [ts] page.route("**/*", async route => {
    // [ts] request = route.request();
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "Hello",
    // [ts] contentType: "text/html",
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const response = request.response();
    // [ts] const headers = response.allHeaders();
    // [ts] expect_eq(headers["content-type"], true);
    // [ts] });
    // [ts] for (const method of ["fulfill", "continue", "fallback", "abort"]) { it(`RO-053 route.${method} should throw if called twice`, (_p) => { let resolve; const resolvePromise = Promise.init(f => resolve = f); page.route("**/*", resolve); page.goto(server.PREFIX + "/empty.html").catch(() => {}); const route = resolvePromise; route[method](); const e = route[method]().catch(e => e); expect_eq(std.mem.indexOf(u8, e.message, "Route is already handled!") != null, true); }); }
    // [ts] test("RO-054 should intercept when postData is more than 1MB", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22753" });
    // [ts] it.fixme(isAndroid);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let interceptionCallback;
    // [ts] const interceptionPromise = Promise.init(x => interceptionCallback = x);
    // [ts] const POST_BODY = "0".repeat(2 * 1024 * 1024); // 2MB
    // [ts] page.route("**/404.html", async route => {
    // [ts] route.abort();
    // [ts] interceptionCallback(route.request().postData());
    // [ts] });
    // [ts] page.evaluate(POST_BODY => fetch("/404.html", {
    // [ts] method: "POST",
    // [ts] body: POST_BODY,
    // [ts] }).catch(e => {}), POST_BODY);
    // [ts] expect_eq(interceptionPromise, POST_BODY);
    // [ts] });
    // [ts] test("RO-055 should be able to intercept every navigation to a page controlled by service worker", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/33561" });
    // [ts] test(isElectron);
    // [ts] let interceptions = 0;
    // [ts] const URL = server.PREFIX + "/serviceworkers/bug-33561/index.html";
    // [ts] page.route(URL, async route => {
    // [ts] ++interceptions;
    // [ts] route.continue();
    // [ts] });
    // [ts] page.goto(URL);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] page.goto(URL);
    // [ts] expect_eq(interceptions, 2);
    // [ts] });
    // [ts] test("RO-056 should respect URLPattern ignoreCase", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42918" });
    // [ts] test(globalThis.URLPattern == null, "URLPattern is not supported in this environment");
    // [ts] const intercepted: []string = [];
    // @ts-ignore URLPattern is not in @types/node yet
    // [ts] page.route(URLPattern.init(server.PREFIX + "/EMPTY.html", { ignoreCase: true }), route => {
    // [ts] intercepted.push("ignoreCase");
    // [ts] return route.fulfill({ body: "mock" });
    // [ts] });
    // @ts-ignore URLPattern is not in @types/node yet
    // [ts] page.route(URLPattern.init(server.PREFIX + "/EMPTY.html"), route => {
    // [ts] intercepted.push("caseSensitive");
    // [ts] return route.fulfill({ body: "mock" });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.text(), "mock");
    // [ts] expect_eq(intercepted, ["ignoreCase"]);
    // [ts] });
    // [ts] test("RO-202 should continue a request reported while interception was disabled", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43619" },
    // [ts] }, (_p) => {
    // [ts] test(isElectron);
    // [ts] page.goto(server.PREFIX + "/serviceworkers/slow-fallback/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const handler = route => route.fulfill({ body: "mocked" });
    // [ts] page.route("**/never-requested", handler);
    // [ts] page.unroute("**/never-requested");
    // The service worker holds the request, so it reaches the network
    // after interception is enabled again.
    // [ts] const requestPromise = page.waitForRequest("**/simple.json");
    // [ts] const bodyPromise = page.evaluate(() => fetch("/simple.json").then(response => response.text()));
    // [ts] requestPromise;
    // [ts] page.route("**/never-requested", handler);
    // [ts] expect_eq(bodyPromise, "{\"foo\": \"bar\"}\n");
    // [ts] });
    // [ts] } // [cap-migrate] auto-closed brace
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
}
