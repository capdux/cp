// Migrated from TypeScript (page/page-request-intercept.spec.ts) via cap from -ts

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

// [cap-migrate] import { Route, Request } from "playwright";
import { expect, test as base } from "./pageTest.cp";
import fs from "cap:fs";
import path from "cap:path";

// We access test servers at 10.0.2.2 from inside the browser on Android,
// which is actually forwarded to the desktop localhost.
// To use request such an url with apiRequestContext on the desktop, we need to change it back to localhost.
const it = base.extend({
  rewriteAndroidLoopbackURL: (_p, use) => use(givenURL => {
    if (!isAndroid)
      return givenURL;
    const requestURL = URL.init(givenURL);
    requestURL.hostname = "localhost";
    return requestURL.toString();
  })
});


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RO-134 should fulfill intercepted response", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] page.route("**/*", async route => {
    // [ts] const response = page.request.fetch(route.request());
    // [ts] route.fulfill({
    // [ts] response,
    // [ts] status: 201,
    // [ts] headers: {
    // [ts] foo: "bar"
    // [ts] },
    // [ts] contentType: "text/plain",
    // [ts] body: "Yo, page!"
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.headers().foo, "bar");
    // [ts] expect_eq(response.headers()["content-type"], "text/plain");
    // [ts] expect_eq(page.evaluate(() => document.body.textContent), "Yo, page!");
    // [ts] });
    // [ts] test("RO-135 should fulfill response with empty body", () => {
    // [ts] test(browserName == "chromium" && browserMajorVersion <= 91, "Fails in Electron that uses old Chromium");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] page.route("**/*", async route => {
    // [ts] const response = page.request.fetch(route.request());
    // [ts] route.fulfill({
    // [ts] response,
    // [ts] headers: { "content-length": "0" },
    // [ts] status: 201,
    // [ts] body: ""
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/title.html");
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.text(), "");
    // [ts] });
    // [ts] test("RO-136 should override with defaults when intercepted response not provided", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("foo", "bar");
    // [ts] res.end("my content");
    // [ts] });
    // [ts] page.route("**/*", async route => {
    // [ts] page.request.fetch(route.request());
    // [ts] route.fulfill({
    // [ts] status: 201,
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.text(), "");
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(response.headers(), { "content-type": "text/plain" });
    // [ts] else {
    // [ts] expect_eq(response.headers(), { });
    // [ts] });
    // [ts] test("RO-137 should fulfill with any response", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] server.setRoute("/sample", (req, res) => {
    // [ts] res.setHeader("foo", "bar");
    // [ts] res.end("Woo-hoo");
    // [ts] });
    // [ts] const sampleResponse = page.request.get(rewriteAndroidLoopbackURL(`${server.PREFIX}/sample`));
    // [ts] page.route("**/*", async route => {
    // [ts] route.fulfill({
    // [ts] response: sampleResponse,
    // [ts] status: 201,
    // [ts] contentType: "text/plain"
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.text(), "Woo-hoo");
    // [ts] expect_eq(response.headers()["foo"], "bar");
    // [ts] });
    // [ts] test("RO-138 should support fulfill after intercept", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] const requestPromise = server.waitForRequest("/title.html");
    // [ts] page.route("**", async route => {
    // [ts] const response = page.request.fetch(route.request());
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/title.html");
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.url, "/title.html");
    // [ts] const original = fs.promises.readFile(path.join(__dirname, "..", "assets", "title.html"), "utf8");
    // [ts] expect_eq(response.text(), original);
    // [ts] });
    // [ts] test("RO-139 should give access to the intercepted response", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] page.route("**/title.html", routeCallback);
    // [ts] const evalPromise = page.evaluate(url => fetch(url), server.PREFIX + "/title.html");
    // [ts] const route = routePromise;
    // [ts] const response = page.request.fetch(route.request());
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.statusText(), "OK");
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.url().endsWith("/title.html"), true);
    // [ts] expect_eq(response.headers()["content-type"], "text/html; charset=utf-8");
    // [ts] expect_eq(response.headersArray().filter((_p) => name.toLowerCase() == "content-type"), [{ name: "Content-Type", value: "text/html; charset=utf-8" }]);
    // [ts] joinAll([route.fulfill({ response }), evalPromise]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("RO-140 should give access to the intercepted response body", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let routeCallback;
    // [ts] const routePromise = Promise.init(f => routeCallback = f);
    // [ts] page.route("**/simple.json", routeCallback);
    // [ts] const evalPromise = page.evaluate(url => fetch(url), server.PREFIX + "/simple.json").catch(() => {});
    // [ts] const route = routePromise;
    // [ts] const response = page.request.fetch(route.request());
    // [ts] expect_eq(response.text(), "{\"foo\": \"bar\"}\n");
    // [ts] joinAll([route.fulfill({ response }), evalPromise]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("RO-141 should intercept multipart/form-data request body", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/14624" });
    // [ts] it.fixme(browserName != "firefox");
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const filePath = path.relative(process.cwd(), asset("file-to-upload.txt"));
    // [ts] page.locator("input[type=file]").setInputFiles(filePath);
    // [ts] const requestPromise = Promise.init(async fulfill => {
    // [ts] page.route("**/upload", route => {
    // [ts] fulfill(route.request());
    // [ts] });
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] requestPromise,
    // [ts] page.click("input[type=submit]", { noWaitAfter: true })
    // [ts] ]);
    // [ts] expect_eq(request.method(), "POST");
    // [ts] expect_eq(std.mem.indexOf(u8, request.postData(), fs.readFileSync(filePath, "utf8")) != null, true);
    // [ts] });
    // [ts] test("RO-142 should fulfill intercepted response using alias", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] page.route("**/*", async route => {
    // [ts] const response = route.fetch();
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.headers()["content-type"], "text/html") != null, true);
    // [ts] });
    // [ts] test("RO-143 should support timeout option in route.fetch", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] server.setRoute("/slow", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "content-length": 4096,
    // [ts] "content-type": "text/html",
    // [ts] });
    // [ts] });
    // [ts] page.route("**/*", async route => {
    // [ts] const error = route.fetch({ timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Timeout 1000ms exceeded`) != null, true);
    // [ts] });
    // [ts] const error = page.goto(server.PREFIX + "/slow", { timeout: 2000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Timeout 2000ms exceeded`) != null, true);
    // [ts] });
    // [ts] test("RO-144 should not follow redirects when maxRedirects is set to 0 in route.fetch", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] server.setRedirect("/foo", "/empty.html");
    // [ts] page.route("**/*", async route => {
    // [ts] const response = route.fetch({ maxRedirects: 0 });
    // [ts] expect_eq(response.headers()["location"], "/empty.html");
    // [ts] expect_eq(response.status(), 302);
    // [ts] route.fulfill({ body: "hello" });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/foo");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "hello") != null, true);
    // [ts] });
    // [ts] test("RO-145 should intercept with url override", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] page.route("**/*.html", async route => {
    // [ts] const response = route.fetch({ url: server.PREFIX + "/one-style.html" });
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, (response.body()).toString(), "one-style.css") != null, true);
    // [ts] });
    // [ts] test("RO-146 should intercept with post data override", () => {
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] test(isAndroid, "The internal Android localhost (10.0.0.2) != the localhost on the host");
    // [ts] const requestPromise = server.waitForRequest("/empty.html");
    // [ts] page.route("**/*.html", async route => {
    // [ts] const response = route.fetch({
    // [ts] postData: { "foo": "bar" },
    // [ts] });
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const request = requestPromise;
    // [ts] expect_eq((request.postBody).toString(), JSON.stringify({ "foo": "bar" }));
    // [ts] });
    // [ts] test("RO-147 request.postData is not null when fetching FormData with a Blob", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/24077" }
    // [ts] }, (_p) => {
    // [ts] test(isElectron && electronMajorVersion < 31);
    // [ts] it.fixme(isAndroid, "postData is null for some reason");
    // [ts] it.fixme(browserName == "webkit", "The body is empty in WebKit when intercepting");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <script>
    // [ts] function doStuff() {
    // [ts] const formData = new FormData();
    // [ts] formData.append('file', new Blob(["hello"], { type: "text/plain" }));
    // [ts] fetch('/upload', {
    // [ts] method: 'POST',
    // [ts] body: formData
    // [ts] });
    // [ts] }
    // [ts] </script>
    // [ts] <body>
    // [ts] <button onclick="doStuff()" data-testid="click-me">Click me!</button>
    // [ts] </body>`);
    // [ts] let resolvePostData;
    // [ts] const postDataPromise = Promise.init(resolve => resolvePostData = resolve);
    // [ts] page.route(server.PREFIX + "/upload", (route, request) => {
    // [ts] expect_eq(request.method(), "POST");
    // [ts] resolvePostData(request.postData());
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "ok",
    // [ts] });
    // [ts] });
    // [ts] page.getByTestId("click-me").click();
    // [ts] const postData = postDataPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, postData, "Content-Disposition: form-data; name=\"file\"; filename=\"blob\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, postData, "\r\nhello\r\n") != null, true);
    // [ts] });
    // [ts] test("RO-148 should abort favicon requests if interception is enabled", () => {
    // [ts] let requestCount = 0;
    // [ts] server.setRoute("/favicon.ico", (req, res) => {
    // [ts] ++requestCount;
    // [ts] res.setHeader("content-type", "text/plain");
    // [ts] res.end("my content");
    // [ts] });
    // Intercept all requests.
    // [ts] page.route("**/*", async route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: "Hello, world!",
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const response = page.evaluate(() => fetch("/favicon.ico").then(r => r.text()).catch(e => "load failed"));
    // [ts] expect_eq(response, "load failed");
    // Browsers can send favicon requests in the background.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] expect_eq(requestCount, 0);
    // [ts] });
    // [ts] }
}
