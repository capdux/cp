// Migrated from TypeScript (page/page-request-fulfill.spec.ts) via cap from -ts

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

import { test as base, expect } from "./pageTest.cp";
import fs from "cap:fs";
// [cap-migrate] import * as har from "../../packages/isomorphic/trace/versions/har.cp";
// [cap-migrate] import { Route } from "playwright";

// TODO(cap-migrate): refine TS statement
// [ts] const it = base.extend<{
// We access test servers at 10.0.2.2 from inside the browser on Android,
// which is actually forwarded to the desktop localhost.
// To use request such an url with apiRequestContext on the desktop, we need to change it back to localhost.
// [ts] rewriteAndroidLoopbackURL(url: string): string;
// [ts] }>({
// [ts] rewriteAndroidLoopbackURL: (_p, use) => use(givenURL => {
// [ts] if (!isAndroid)
// [ts] return givenURL;
// [ts] const requestURL = URL.init(givenURL);
// [ts] requestURL.hostname = "localhost";
// [ts] return requestURL.toString();
// [ts] })
// [ts] });




















fn findResponse(har: har.HARFile, url: string): har.Response {
  let entry;
  const originalUrl = url;
  while (url.trim()) {
    entry = har.log.entries.find(entry => entry.request.url == url);
    url = entry?.response.redirectURL;
  }
  expect_eq(entry, originalUrl, true);
  return entry?.response;
}




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RO-096 should work", () => {
    // [ts] page.route("**/*", route => {
    // [ts] void route.fulfill({
    // [ts] status: 201,
    // [ts] headers: {
    // [ts] foo: "bar"
    // [ts] },
    // [ts] contentType: "text/html",
    // [ts] body: "Yo, page!"
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.headers().foo, "bar");
    // [ts] expect_eq(page.evaluate(() => document.body.textContent), "Yo, page!");
    // [ts] });
    // [ts] test("RO-097 should work with buffer as body", () => {
    // [ts] page.route("**/*", route => {
    // [ts] void route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/plain",
    // [ts] body: Buffer.from("Yo, page!")
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(page.evaluate(() => document.body.textContent), "Yo, page!");
    // [ts] });
    // [ts] test("RO-098 should work with status code 422", () => {
    // [ts] page.route("**/*", route => {
    // [ts] void route.fulfill({
    // [ts] status: 422,
    // [ts] body: "Yo, page!"
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 422);
    // [ts] expect_eq(response.statusText(), "Unprocessable Entity");
    // [ts] expect_eq(page.evaluate(() => document.body.textContent), "Yo, page!");
    // [ts] });
    // [ts] test("RO-099 should fulfill with unuassigned status codes", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28490" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30773" });
    // [ts] let fulfillPromiseCallback;
    // [ts] const fulfillPromise = Promise.init(f => fulfillPromiseCallback = f);
    // [ts] page.route("**/data.json", route => {
    // [ts] fulfillPromiseCallback(route.fulfill({
    // [ts] status: 430,
    // [ts] body: "Yo, page!"
    // [ts] }).catch(e => e));
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const response = page.evaluate(async url => {
    // [ts] const { status, statusText } = fetch(url);
    // [ts] return { status, statusText };
    // [ts] }, server.PREFIX + "/data.json");
    // [ts] const error = fulfillPromise;
    // [ts] expect_eq(error, null);
    // [ts] expect_eq(response.status, 430);
    // [ts] expect_eq(response.statusText, "Unknown");
    // [ts] });
    // [ts] test("RO-100 should not throw if request was cancelled by the page", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28490" });
    // [ts] let interceptCallback;
    // [ts] const interceptPromise = Promise.init(f => interceptCallback = f);
    // [ts] page.route("**/data.json", route => interceptCallback(route));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(url => {
    // [ts] globalThis.controller = AbortController.init();
    // [ts] fetch(url, { signal: globalThis.controller.signal }).catch(() => {});
    // [ts] }, server.PREFIX + "/data.json");
    // [ts] const route = interceptPromise;
    // [ts] const failurePromise = page.waitForEvent("requestfailed");
    // [ts] page.evaluate(() => globalThis.controller.abort());
    // [ts] const cancelledRequest = failurePromise;
    // [ts] expect_eq(cancelledRequest.failure(), true);
    // [ts] expect(cancelledRequest.failure().errorText).toMatch("cancelled|aborted");
    // [ts] route.fulfill({ status: 200 }); // Should not throw.
    // [ts] });
    // [ts] test("RO-101 should allow mocking binary responses", () => {
    // [ts] test(browserName == "firefox" && !headless, "Firefox headed produces a different image.");
    // [ts] test(isAndroid);
    // [ts] page.route("**/*", route => {
    // [ts] const imageBuffer = fs.readFileSync(asset("pptr.png"));
    // [ts] void route.fulfill({
    // [ts] contentType: "image/png",
    // [ts] body: imageBuffer
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(PREFIX => {
    // [ts] const img = document.createElement("img");
    // [ts] img.src = PREFIX + "/does-not-exist.png";
    // [ts] document.body.appendChild(img);
    // [ts] return Promise.init(fulfill => img.onload = fulfill);
    // [ts] }, server.PREFIX);
    // [ts] const img = page.$("img");
    // [ts] expect(img.screenshot()).toMatchSnapshot("mock-binary-response.png");
    // [ts] });
    // [ts] test("RO-102 should allow mocking svg with charset", () => {
    // [ts] test(browserName == "firefox" && !headless, "Firefox headed produces a different image.");
    // [ts] test(isAndroid);
    // [ts] test(isElectron && electronMajorVersion < 30, "Protocol error (Storage.getCookies): Browser context management is not supported");
    // [ts] page.route("**/*", route => {
    // [ts] void route.fulfill({
    // [ts] contentType: "image/svg+xml ; charset=utf-8",
    // [ts] body: "<svg width=\"50\" height=\"50\" version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\"><rect x=\"10\" y=\"10\" width=\"30\" height=\"30\" stroke=\"black\" fill=\"transparent\" stroke-width=\"5\"/></svg>"
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(PREFIX => {
    // [ts] const img = document.createElement("img");
    // [ts] img.src = PREFIX + "/does-not-exist.svg";
    // [ts] document.body.appendChild(img);
    // [ts] return Promise.init((f, r) => { img.onload = f; img.onerror = r; });
    // [ts] }, server.PREFIX);
    // [ts] const img = page.$("img");
    // [ts] expect(img.screenshot()).toMatchSnapshot("mock-svg.png");
    // [ts] });
    // [ts] test("RO-103 should work with file path", () => {
    // [ts] test(isAndroid);
    // [ts] page.route("**/*", route => route.fulfill({ contentType: "shouldBeIgnored", path: asset("pptr.png") }));
    // [ts] page.evaluate(PREFIX => {
    // [ts] const img = document.createElement("img");
    // [ts] img.src = PREFIX + "/does-not-exist.png";
    // [ts] document.body.appendChild(img);
    // [ts] return Promise.init(fulfill => img.onload = fulfill);
    // [ts] }, server.PREFIX);
    // [ts] const img = page.$("img");
    // [ts] expect(img.screenshot()).toMatchSnapshot("mock-binary-response.png");
    // [ts] });
    // [ts] test("RO-104 should stringify intercepted request response headers", () => {
    // [ts] page.route("**/*", route => {
    // [ts] void route.fulfill({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] "foo": "true"
    // [ts] },
    // [ts] body: "Yo, page!"
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] const headers = response.headers();
    // [ts] expect_eq(headers.foo, "true");
    // [ts] expect_eq(page.evaluate(() => document.body.textContent), "Yo, page!");
    // [ts] });
    // [ts] test("RO-105 should not modify the headers sent to the server", () => {
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const interceptedRequests = [];
    // this is just to enable request interception, which disables caching in chromium
    // [ts] page.route(server.PREFIX + "/unused", () => {});
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] interceptedRequests.push(request);
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("done");
    // [ts] });
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url);
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect_eq(text, "done");
    // [ts] page.route(server.CROSS_PROCESS_PREFIX + "/something", (route, request) => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...request.headers()
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const textAfterRoute = page.evaluate(async url => {
    // [ts] const data = fetch(url);
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect_eq(textAfterRoute, "done");
    // [ts] expect_eq(interceptedRequests.length, 2);
    // [ts] expect_eq(interceptedRequests[1].headers, interceptedRequests[0].headers);
    // [ts] });
    // [ts] test("RO-106 should include the origin header", () => {
    // [ts] test(isAndroid, "No cross-process on Android");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] let interceptedRequest;
    // [ts] page.route(server.CROSS_PROCESS_PREFIX + "/something", (route, request) => {
    // [ts] interceptedRequest = request;
    // [ts] void route.fulfill({
    // [ts] headers: {
    // [ts] "Access-Control-Allow-Origin": "*",
    // [ts] },
    // [ts] contentType: "text/plain",
    // [ts] body: "done"
    // [ts] });
    // [ts] });
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url);
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect_eq(text, "done");
    // [ts] expect_eq(interceptedRequest.headers()["origin"], server.PREFIX);
    // [ts] });
    // [ts] test("RO-107 should fulfill with global fetch result", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] page.route("**/*", async route => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.get(rewriteAndroidLoopbackURL(server.PREFIX + "/simple.json"));
    // [ts] void route.fulfill({ response });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.json(), { "foo": "bar" });
    // [ts] });
    // [ts] test("RO-108 should fulfill with fetch result", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] page.route("**/*", async route => {
    // [ts] const response = page.request.get(rewriteAndroidLoopbackURL(server.PREFIX + "/simple.json"));
    // [ts] void route.fulfill({ response });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.json(), { "foo": "bar" });
    // [ts] });
    // [ts] test("RO-109 should fulfill with fetch result and overrides", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] page.route("**/*", async route => {
    // [ts] const response = page.request.get(rewriteAndroidLoopbackURL(server.PREFIX + "/simple.json"));
    // [ts] void route.fulfill({
    // [ts] response,
    // [ts] status: 201,
    // [ts] headers: {
    // [ts] "Content-Type": "application/json", // Case matters for the tested behavior
    // [ts] "foo": "bar"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq((response.allHeaders()).foo, "bar");
    // [ts] expect_eq(response.json(), { "foo": "bar" });
    // [ts] });
    // [ts] test("RO-110 should fetch original request and fulfill", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] page.route("**/*", async route => {
    // [ts] const response = page.request.fetch(route.request());
    // [ts] void route.fulfill({
    // [ts] response,
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/title.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(page.title(), "Woof-Woof");
    // [ts] });
    // [ts] test("RO-111 should fulfill with multiple set-cookie", () => {
    // [ts] test(isElectron && electronMajorVersion < 14, "Electron 14+ is required");
    // [ts] test(isAndroid, "Android does not have an isolated context per test, so we get cookies from other tests");
    // [ts] const cookies = ["a=b", "c=d"];
    // [ts] page.route("**/multiple-set-cookie.html", async route => {
    // [ts] void route.fulfill({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] "X-Header-1": "v1",
    // [ts] "Set-Cookie": cookies.join("\n"),
    // [ts] "X-Header-2": "v2",
    // [ts] },
    // [ts] body: ""
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/multiple-set-cookie.html");
    // [ts] expect_eq((page.evaluate(() => document.cookie)).split(";").map(s => s.trim()).sort(), cookies);
    // [ts] expect_eq(response.headerValue("X-Header-1"), "v1");
    // [ts] expect_eq(response.headerValue("X-Header-2"), "v2");
    // [ts] });
    // [ts] test("RO-112 should fulfill with fetch response that has multiple set-cookie", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=b", "c=d"]);
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end();
    // [ts] });
    // [ts] page.route("**/empty.html", async route => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.fetch(route.request());
    // [ts] void route.fulfill({ response });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookie = page.evaluate(() => document.cookie);
    // [ts] expect_eq(cookie.split(";").map(s => s.trim()).sort(), ["a=b", "c=d"]);
    // [ts] });
    // [ts] test("RO-113 headerValue should return set-cookie from intercepted response", () => {
    // [ts] it.fail(browserName == "chromium", "Set-Cookie is missing in response after interception");
    // [ts] test(browserName == "webkit", "Set-Cookie with \n in intercepted response does not pass validation in WebCore, see also https://github.com/microsoft/playwright/pull/9273");
    // [ts] page.route("**/empty.html", async route => {
    // [ts] void route.fulfill({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] "Set-Cookie": "a=b",
    // [ts] },
    // [ts] body: ""
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.headerValue("Set-Cookie"), "a=b");
    // [ts] });
    // [ts] test("RO-114 should fulfill with har response", () => {
    // [ts] const harPath = asset("har-fulfill.har");
    // [ts] const har = JSON.parse(fs.promises.readFile(harPath, "utf-8"));
    // [ts] page.route("**/*", async route => {
    // [ts] const response = findResponse(har, route.request().url());
    // [ts] const headers = {};
    // [ts] for (const { name, value } of response.headers)
    // [ts] headers[name] = value;
    // [ts] route.fulfill({
    // [ts] status: response.status,
    // [ts] headers,
    // [ts] body: Buffer.from(response.content.text || "", (response.content.encoding) || "utf-8"),
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // HAR contains a POST for the css file but we match ignoring the method, so the file should be served.
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(0, 255, 255)");
    // [ts] });
    // [ts] test("RO-115 should fulfill preload link requests", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16745" });
    // [ts] let intercepted = false;
    // [ts] page.route("**/one-style.css", route => {
    // [ts] intercepted = true;
    // [ts] void route.fulfill({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] "content-type": "text/css; charset=utf-8",
    // [ts] "cache-control": "no-cache, no-store",
    // [ts] "custom": "value"
    // [ts] },
    // [ts] body: "body { background-color: green; }"
    // [ts] });
    // [ts] });
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/one-style.css"),
    // [ts] page.goto(server.PREFIX + "/preload.html")
    // [ts] ]);
    // [ts] expect_eq(response.headerValue("custom"), "value");
    // [ts] page.waitForFunction(() => (window).preloadedStyles);
    // [ts] expect_eq(intercepted, true);
    // [ts] const color = page.evaluate(() => window.getComputedStyle(document.body).backgroundColor);
    // [ts] expect_eq(color, "rgb(0, 128, 0)");
    // [ts] });
    // [ts] test("RO-116 should fulfill json", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44109" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cases = [
    // [ts] { options: { json: { bar: "baz" } }, contentType: "application/json" },
    // [ts] { options: { json: 1 }, contentType: "application/json" },
    // [ts] { options: { json: null }, contentType: "application/json" },
    // [ts] { options: { json: false }, contentType: "application/json" },
    // [ts] { options: { json: 0 }, contentType: "application/json" },
    // [ts] { options: { json: "" }, contentType: "application/json" },
    // [ts] { options: { json: { bar: "baz" }, headers: { "content-type": "application/problem+json" } }, contentType: "application/problem+json" },
    // [ts] { options: { json: { bar: "baz" }, headers: { "Content-Type": "application/problem+json" } }, contentType: "application/problem+json" },
    // [ts] { options: { json: { bar: "baz" }, headers: { "Content-Type": "application/problem+json" }, contentType: "text/plain" }, contentType: "text/plain" },
    // [ts] ];
    // [ts] for (const { options, contentType } of cases) {
    // [ts] page.route("**/data.json", route => route.fulfill({
    // [ts] ...options,
    // [ts] status: 201,
    // [ts] headers: { foo: "bar", ...options.headers },
    // [ts] }), { times: 1 });
    // [ts] const [response, body] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/data.json"),
    // [ts] page.evaluate(() => fetch("./data.json").then(r => r.text()))
    // [ts] ]);
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect.soft(response.headers()["content-type"], JSON.stringify(options)).toBe(contentType);
    // [ts] expect_eq(body, JSON.stringify(options.json));
    // [ts] }
    // [ts] });
    // [ts] test("RO-117 should fulfill with gzip and readback", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/29261" },
    // [ts] }, (_p) => {
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] server.enableGzip("/one-style.html");
    // [ts] page.route("**/one-style.html", async route => {
    // [ts] const response = route.fetch();
    // [ts] expect_eq(response.headers()["content-encoding"], "gzip");
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect(page.locator("div")).toHaveText("hello, world!");
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), `<div>hello, world!</div>`) != null, true);
    // [ts] });
    // [ts] test("RO-118 should not go to the network for fulfilled requests body", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/30760" },
    // [ts] }, (_p) => {
    // [ts] page.route("**/one-style.css", async route => {
    // [ts] return route.fulfill({
    // [ts] status: 404,
    // [ts] contentType: "text/plain",
    // [ts] body: "Not Found! (mocked)",
    // [ts] });
    // [ts] });
    // [ts] let serverHit = false;
    // [ts] server.setRoute("/one-style.css", (req, res) => {
    // [ts] serverHit = true;
    // [ts] res.setHeader("Content-Type", "text/css");
    // [ts] res.end("body { background-color: green; }");
    // [ts] });
    // [ts] const responsePromise = page.waitForResponse("**/one-style.css");
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] const response = responsePromise;
    // [ts] const body = response.body();
    // [ts] expect_eq(body.toString(), "Not Found! (mocked)");
    // [ts] expect_eq(serverHit, false);
    // [ts] });
    // [ts] test("RO-119 should return body for fulfilled responses", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-java/issues/1792" },
    // [ts] }, (_p) => {
    // [ts] for (const status of [100, 200, 404, 500]) {
    // [ts] it.step(`status ${status}`, () => {
    // [ts] const bodyOverride = `Custom body ${status}`;
    // [ts] page.route("**/one-style.css", async route => {
    // [ts] return route.fulfill({
    // [ts] status,
    // [ts] contentType: "text/plain",
    // [ts] body: bodyOverride,
    // [ts] });
    // [ts] });
    // [ts] const responsePromise = page.waitForResponse("**/one-style.css");
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] const response = responsePromise;
    // [ts] const body = response.body();
    // [ts] expect_eq(body.toString(), bodyOverride);
    // [ts] });
    // [ts] page.unrouteAll();
    // [ts] }
    // [ts] });
}
