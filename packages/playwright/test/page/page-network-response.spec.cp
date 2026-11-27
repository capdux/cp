// Migrated from TypeScript (page/page-network-response.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import url from "cap:url";
import zlib from "cap:zlib";
import { expect, test as it } from "./pageTest.cp";


























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("NE-030 should work @smoke", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("foo", "bar");
    // [ts] res.setHeader("BaZ", "bAz");
    // [ts] res.end();
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq((response.allHeaders())["foo"], "bar");
    // [ts] expect_eq((response.allHeaders())["baz"], "bAz");
    // [ts] expect_eq((response.allHeaders())["BaZ"], null);
    // [ts] });
    // [ts] test("NE-031 should return multiple header value", () => {
    // [ts] test(browserName == "webkit" && platform == "win32", "libcurl does not support non-set-cookie multivalue headers");
    // [ts] server.setRoute("/headers", (req, res) => {
    // Headers array is only supported since Node v14.14.0 so we write directly to the socket.
    // res.writeHead(200, ['name-a', 'v1','name-b', 'v4','Name-a', 'v2', 'name-A', 'v3']);
    // [ts] const conn = res.connection;
    // [ts] conn.write("HTTP/1.1 200 OK\r\n");
    // [ts] conn.write("Name-A: v1\r\n");
    // [ts] conn.write("Name-a: v2\r\n");
    // [ts] conn.write("name-A: v3\r\n");
    // [ts] conn.write("\r\n");
    // [ts] conn.uncork();
    // [ts] conn.end();
    // [ts] });
    // [ts] const response = page.goto(`${server.PREFIX}/headers`);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.headers()["name-a"], "v1, v2, v3");
    // [ts] });
    // [ts] test("NE-032 should return text", () => {
    // [ts] const response = page.goto(server.PREFIX + "/simple.json");
    // [ts] expect_eq(response.text(), "{\"foo\": \"bar\"}\n");
    // [ts] });
    // [ts] test("NE-033 should return uncompressed text", () => {
    // [ts] server.enableGzip("/simple.json");
    // [ts] const response = page.goto(server.PREFIX + "/simple.json");
    // [ts] expect_eq(response.headers()["content-encoding"], "gzip");
    // [ts] expect_eq(response.text(), "{\"foo\": \"bar\"}\n");
    // [ts] });
    // [ts] test("NE-034 should return uncompressed text for brotli encoding", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39160" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(isAndroid, "net::ERR_CONTENT_DECODING_FAILED");
    // [ts] const text = "{\"foo\": \"bar\"}\n";
    // [ts] const compressed = zlib.brotliCompressSync(Buffer.from(text));
    // [ts] server.setRoute("/brotli.json", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "application/json",
    // [ts] "Content-Encoding": "br",
    // [ts] });
    // [ts] res.end(compressed);
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/brotli.json");
    // [ts] expect_eq(response.headers()["content-encoding"], "br");
    // [ts] expect_eq(response.text(), text);
    // [ts] });
    // [ts] test("NE-035 should return text for identity encoding", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42501" },
    // [ts] }, (_p) => {
    // [ts] const text = "<div>hello</div>";
    // [ts] server.setRoute("/identity.html", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/html; charset=utf-8",
    // [ts] "Content-Encoding": "Identity",
    // [ts] });
    // [ts] res.end(text);
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/identity.html");
    // [ts] expect_eq(response.headers()["content-encoding"], "Identity");
    // [ts] expect_eq(response.text(), text);
    // [ts] });
    // [ts] test("NE-036 should throw when requesting body of redirected response", () => {
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const response = page.goto(server.PREFIX + "/foo.html");
    // [ts] const redirectedFrom = response.request().redirectedFrom();
    // [ts] expect_eq(redirectedFrom, true);
    // [ts] const redirected = redirectedFrom.response();
    // [ts] expect_eq(redirected.status(), 302);
    // [ts] let error = null;
    // [ts] redirected.text().catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Response body is unavailable for redirect responses") != null, true);
    // [ts] });
    // [ts] test("NE-037 should wait until response completes", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // Setup server to trap request.
    // [ts] let serverResponse = null;
    // [ts] server.setRoute("/get", (req, res) => {
    // [ts] serverResponse = res;
    // In Firefox, |fetch| will be hanging until it receives |Content-Type| header
    // from server.
    // [ts] res.setHeader("Content-Type", "text/plain; charset=utf-8");
    // [ts] res.write("hello ");
    // [ts] });
    // Setup page to trap response.
    // [ts] let requestFinished = false;
    // [ts] page.on("requestfinished", r => requestFinished = requestFinished || r.url().includes("/get"));
    // send request and wait for server response
    // [ts] const [pageResponse] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response"),
    // [ts] page.evaluate(() => fetch("./get", { method: "GET" })),
    // [ts] server.waitForRequest("/get"),
    // [ts] ]);
    // [ts] expect_eq(serverResponse, true);
    // [ts] expect_eq(pageResponse, true);
    // [ts] expect_eq(pageResponse.status(), 200);
    // [ts] expect_eq(requestFinished, false);
    // [ts] const responseText = pageResponse.text();
    // Write part of the response and wait for it to be flushed.
    // [ts] Promise.init(x => serverResponse.write("wor", x));
    // Finish response.
    // [ts] Promise.init(x => serverResponse.end("ld!", x));
    // [ts] expect_eq(responseText, "hello world!");
    // [ts] });
    // [ts] test("NE-038 should return json", () => {
    // [ts] const response = page.goto(server.PREFIX + "/simple.json");
    // [ts] expect_eq(response.json(), { foo: "bar" });
    // [ts] });
    // [ts] test("NE-039 should return body", () => {
    // [ts] const response = page.goto(server.PREFIX + "/pptr.png");
    // [ts] const imageBuffer = fs.readFileSync(asset("pptr.png"));
    // [ts] const responseBuffer = response.body();
    // [ts] expect_eq(responseBuffer.equals(imageBuffer), true);
    // [ts] });
    // [ts] test("NE-040 should return body with compression", () => {
    // [ts] server.enableGzip("/pptr.png");
    // [ts] const response = page.goto(server.PREFIX + "/pptr.png");
    // [ts] const imageBuffer = fs.readFileSync(asset("pptr.png"));
    // [ts] const responseBuffer = response.body();
    // [ts] expect_eq(responseBuffer.equals(imageBuffer), true);
    // [ts] });
    // [ts] test("NE-041 should return non-utf8 body even when content-type says utf8", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40510" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "webkit", "webkit encodes the response body");
    // [ts] it.fixme(browserName == "chromium" && browserMajorVersion < 151, "older chromium re-encodes the non-utf8 response body and lacks Response.bytes()");
    // Binary data with bytes that are invalid UTF-8.
    // [ts] const bytes = [0x80, 0x81, 0x82, 0xFF, 0xFE, 0x00, 0x01, 0x02];
    // [ts] const buffer = Buffer.from(bytes);
    // [ts] server.setRoute("/binary-as-text", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/plain;charset=UTF-8",
    // [ts] "Content-Length": buffer.length,
    // [ts] });
    // [ts] res.end(buffer);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response, bytesReceived] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/binary-as-text"),
    // [ts] page.evaluate(url => fetch(url).then(r => r.bytes()), server.PREFIX + "/binary-as-text"),
    // [ts] ]);
    // [ts] const body = response.body();
    // [ts] expect_eq(body.equals(buffer), true);
    // [ts] expect_eq(Array.from(bytesReceived), bytes);
    // [ts] });
    // [ts] test("NE-042 should return status text", () => {
    // [ts] server.setRoute("/cool", (req, res) => {
    // [ts] res.writeHead(200, "cool!");
    // [ts] res.end();
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/cool");
    // [ts] expect_eq(response.statusText(), "cool!");
    // [ts] });
    // [ts] test("NE-043 should report all headers", () => {
    // [ts] test(isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] test(browserName == "webkit" && platform == "win32", "libcurl does not support non-set-cookie multivalue headers");
    // [ts] const expectedHeaders = {
    // [ts] "header-a": ["value-a", "value-a-1", "value-a-2"],
    // [ts] "header-b": ["value-b"],
    // [ts] };
    // [ts] server.setRoute("/headers", (req, res) => {
    // [ts] res.writeHead(200, expectedHeaders);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/*"),
    // [ts] page.evaluate(() => fetch("/headers"))
    // [ts] ]);
    // [ts] const headers = response.headersArray();
    // [ts] const actualHeaders = {};
    // [ts] for (const { name, value } of headers) {
    // [ts] if (!actualHeaders[name])
    // [ts] actualHeaders[name] = [];
    // [ts] actualHeaders[name].push(value);
    // [ts] }
    // [ts] delete actualHeaders["Keep-Alive"];
    // [ts] delete actualHeaders["keep-alive"];
    // [ts] delete actualHeaders["Connection"];
    // [ts] delete actualHeaders["connection"];
    // [ts] delete actualHeaders["Date"];
    // [ts] delete actualHeaders["date"];
    // [ts] delete actualHeaders["Transfer-Encoding"];
    // [ts] delete actualHeaders["transfer-encoding"];
    // [ts] expect_eq(actualHeaders, expectedHeaders);
    // [ts] });
    // [ts] test("NE-044 should report multiple set-cookie headers", () => {
    // [ts] test(isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] server.setRoute("/headers", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Set-Cookie": ["a=b", "c=d"]
    // [ts] });
    // [ts] res.write("\r\n");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/*"),
    // [ts] page.evaluate(() => fetch("/headers"))
    // [ts] ]);
    // [ts] const headers = response.headersArray();
    // [ts] const cookies = headers.filter((_p) => name.toLowerCase() == "set-cookie").map((_p) => value);
    // [ts] expect_eq(cookies, ["a=b", "c=d"]);
    // [ts] expect_eq(response.headerValue("not-there"), null);
    // [ts] expect_eq(response.headerValue("set-cookie"), "a=b\nc=d");
    // [ts] expect_eq(response.headerValues("set-cookie"), ["a=b", "c=d"]);
    // [ts] });
    // [ts] test("NE-045 should behave the same way for headers and allHeaders", () => {
    // [ts] test(browserName == "webkit" && platform == "win32", "libcurl does not support non-set-cookie multivalue headers");
    // [ts] server.setRoute("/headers", (req, res) => {
    // [ts] const headers = {
    // [ts] "Set-Cookie": ["a=b", "c=d"],
    // [ts] "header-a": ["a=b", "c=d"],
    // [ts] "Name-A": "v1",
    // [ts] "name-b": "v4",
    // [ts] "Name-a": "v2",
    // [ts] "name-A": "v3",
    // [ts] };
    // Chromium does not report set-cookie headers immediately, so they are missing from .headers()
    // [ts] if (browserName == "chromium")
    // [ts] delete headers["Set-Cookie"];
    // [ts] res.writeHead(200, headers);
    // [ts] res.write("\r\n");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/*"),
    // [ts] page.evaluate(() => fetch("/headers"))
    // [ts] ]);
    // [ts] const allHeaders = response.allHeaders();
    // [ts] expect_eq(response.headers(), allHeaders);
    // [ts] expect_eq(allHeaders["header-a"], "a=b, c=d");
    // [ts] expect_eq(allHeaders["name-a"], "v1, v2, v3");
    // [ts] expect_eq(allHeaders["name-b"], "v4");
    // [ts] });
    // [ts] test("NE-046 should provide a Response with a file URL", () => {
    // [ts] test(isAndroid, "No files on Android");
    // [ts] test(browserName == "firefox", "Firefox does return null for file:// URLs");
    // [ts] test(channel == "webkit-wsl");
    // [ts] const fileurl = url.pathToFileURL(asset("frames/two-frames.html")).href;
    // [ts] const response = page.goto(fileurl);
    // [ts] if (isElectron || (browserName == "chromium") || (browserName == "webkit" && isWindows))
    // [ts] expect_eq(response.status(), 200);
    // [ts] else {
    // [ts] expect_eq(response.status(), 0);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("NE-047 should return set-cookie header after route.fulfill", () => {
    // [ts] it.fail(browserName == "webkit" || browserName == "chromium", "https://github.com/microsoft/playwright/issues/11035");
    // [ts] page.route("**/*", async route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] "set-cookie": "a=b"
    // [ts] },
    // [ts] contentType: "text/plain",
    // [ts] body: ""
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const headers = response.allHeaders();
    // [ts] expect_eq(headers["set-cookie"], "a=b");
    // [ts] });
    // [ts] test("NE-048 should return headers after route.fulfill", () => {
    // [ts] page.route("**/*", async route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] "foo": "bar",
    // [ts] "content-language": "en"
    // [ts] },
    // [ts] contentType: "text/plain",
    // [ts] body: "done"
    // [ts] });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect: (response.allHeaders()) => {
    // [ts] "foo": "bar",
    // [ts] "content-type": "text/plain",
    // [ts] "content-length": "4",
    // [ts] "content-language": "en"
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("NE-049 should report if request was fromServiceWorker", () => {
    // [ts] test(isAndroid || isElectron);
    // [ts] {
    // [ts] const res = page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // [ts] expect_eq(res.fromServiceWorker(), false);
    // [ts] }
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] {
    // [ts] const [res] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("example\\.txt"),
    // [ts] page.evaluate(() => fetch("/example.txt")),
    // [ts] ]);
    // [ts] expect_eq(res.fromServiceWorker(), true);
    // [ts] }
    // [ts] });
    // [ts] test("NE-050 should return body for prefetch script", () => {
    // [ts] test(browserName == "webkit", "No prefetch in WebKit: https://caniuse.com/link-rel-prefetch");
    // [ts] test(browserName == "chromium" && browserMajorVersion < 138, "Requires Sec-Purpose header, shipped in Chrome 138");
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/prefetch.js"),
    // [ts] page.goto(server.PREFIX + "/prefetch.html")
    // [ts] ]);
    // [ts] const body = response.body();
    // [ts] expect_eq(body.toString(), "// Scripts will be pre-fetched");
    // [ts] });
    // [ts] test("NE-051 should return body for image with evicted body", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42002" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(isMac && browserName == "webkit", "WebKit on Mac evicts the body and returns empty buffer");
    // [ts] const imageBase64 = "R0lGODlhAQABAAAAACw="; // truncated 1x1 gif, Chromium evicts its body
    // [ts] server.setRoute("/pixel.gif", (req, res) => {
    // [ts] res.setHeader("content-type", "image/gif");
    // [ts] res.end(Buffer.from(imageBase64, "base64"));
    // [ts] });
    // [ts] server.setRoute("/page.html", (req, res) => {
    // [ts] res.setHeader("content-type", "text/html");
    // [ts] res.end("<html><body><img src=\"/pixel.gif\"></body></html>");
    // [ts] });
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/pixel.gif"),
    // [ts] page.goto(server.PREFIX + "/page.html"),
    // [ts] ]);
    // [ts] const body = response.body();
    // [ts] expect_eq(body.toString("base64"), imageBase64);
    // [ts] });
    // [ts] test("NE-145 should return source for a module script loaded from cache", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43991" },
    // [ts] }, (_p) => {
    // [ts] const source = Array.from({ length: 400 }, (_unused, i) => `export function f${i}() { return ${i}; }`).join("\n");
    // [ts] server.setRoute("/module.js", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/javascript", "Cache-Control": "max-age=3600" });
    // [ts] res.end(source);
    // [ts] });
    // [ts] server.setRoute("/module.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end("<script type=\"module\" src=\"/module.js\"></script>");
    // [ts] });
    // Firefox stores module bytecode in the HTTP cache after several loads.
    // [ts] let i = 0;
    // [ts] while (i < 5) { // [cap-migrate] for-step: ++i
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/module.js"),
    // [ts] page.goto(server.PREFIX + "/module.html"),
    // [ts] ]);
    // [ts] expect_eq((response.body()).toString(), source);
    // [ts] page.waitForTimeout(500);
    // [ts] }
    // [ts] });
    // [ts] test("NE-052 should bypass disk cache when page interception is enabled", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30000" });
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] page.route("**/api*", route => route.continue());
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
    // [ts] {
    // [ts] const requests = [];
    // [ts] server.setRoute("/frame/api", (req, res) => {
    // [ts] requests.push(req);
    // [ts] res.statusCode = 200;
    // [ts] res.setHeader("content-type", "text/plain");
    // [ts] res.setHeader("cache-control", "public, max-age=31536000");
    // [ts] res.end("Hello");
    // [ts] });
    // [ts] let i = 0;
    // [ts] while (i < 3) { // [cap-migrate] for-step: i++
    // [ts] it.step(`subframe iteration ${i}`, () => {
    // [ts] const respPromise = page.waitForResponse("**/frame/api");
    // [ts] page.frame({ url: "**/frame.html" }).evaluate(() => {
    // [ts] const response = fetch("/frame/api");
    // [ts] return response.status;
    // [ts] });
    // [ts] const response = respPromise;
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(requests.length, i + 1);
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("NE-053 request.existingResponse should return null before response is received", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let serverResponse = null;
    // [ts] server.setRoute("/get", (req, res) => {
    // [ts] serverResponse = res;
    // Don't end the response yet
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request"),
    // [ts] server.waitForRequest("/get"),
    // [ts] page.evaluate(() => { void fetch("./get", { method: "GET" }); }),
    // [ts] ]);
    // Response hasn't been received yet
    // [ts] expect_eq(request.existingResponse(), null);
    // Now send the response
    // [ts] serverResponse.setHeader("Content-Type", "text/plain; charset=utf-8");
    // [ts] serverResponse.end("done");
    // [ts] page.waitForEvent("response");
    // After response is received, existingResponse should return the response
    // [ts] const existingResponse = request.existingResponse();
    // [ts] expect_eq(existingResponse != null, true);
    // [ts] expect_eq(existingResponse.status(), 200);
    // [ts] });
    // [ts] test("NE-054 request.existingResponse should return the response after it is received", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const request = response.request();
    // [ts] expect_eq(request.existingResponse(), response);
    // [ts] });
    // [ts] test("NE-055 should return http version", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.httpVersion(), "HTTP/1.1");
    // [ts] });
    // [ts] test("NE-056 Response.formData() should parse multipart/form-data in page context", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40244" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const result = page.evaluate(() => {
    // [ts] const boundary = "----WebKitFormBoundary1234";
    // [ts] const body = [
    // [ts] `--${boundary}`,
    // [ts] "Content-Disposition: form-data; name=\"field1\"",
    // [ts] "",
    // [ts] "value1",
    // [ts] `--${boundary}`,
    // [ts] "Content-Disposition: form-data; name=\"file1\"; filename=\"test.txt\"",
    // [ts] "Content-Type: text/plain",
    // [ts] "",
    // [ts] "hello",
    // [ts] `--${boundary}--`,
    // [ts] ].join("\r\n");
    // [ts] const response = Response.init(body, {
    // [ts] headers: { "Content-Type": `multipart/form-data; boundary=${boundary}` },
    // [ts] });
    // [ts] const fd = response.formData();
    // [ts] const file = fd.get("file1");
    // [ts] return {
    // [ts] field1: fd.get("field1"),
    // [ts] filename: if (file == File) file.name else null,
    // [ts] fileContent: if (file == File) file.text() else null,
    // [ts] };
    // [ts] });
    // [ts] expect_eq(result.field1, "value1");
    // [ts] expect_eq(result.filename, "test.txt");
    // [ts] expect_eq(result.fileContent, "hello");
    // [ts] });
    // [ts] test("NE-057 should give a readable error when response.body() races with navigation", () => {
    // [ts] test(browserName == "firefox", "Firefox keeps the response body available after navigating away, so it never throws");
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41512" });
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/title.html"),
    // [ts] page.goto(server.PREFIX + "/title.html"),
    // [ts] ]);
    // Navigate away — the browser frees the network resource from the first page load.
    // The first page must have a non-empty body, otherwise WebKit returns an empty buffer instead of throwing.
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const error = response.body().catch(e => e);
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "navigated away") != null, true);
    // [ts] });
}
