// Migrated from TypeScript (page/page-network-sizes.spec.ts) via cap from -ts

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
import zlib from "cap:zlib";

import { test as it, expect } from "./pageTest.cp";









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] test("NE-058 should set bodySize and headersSize", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request"),
    // [ts] page.evaluate(() => fetch("./get", { method: "POST", body: "12345" }).then(r => r.text())),
    // [ts] ]);
    // [ts] const sizes = request.sizes();
    // [ts] expect_eq(sizes.requestBodySize, 5);
    // [ts] expect_eq(sizes.requestHeadersSize >= 250, true);
    // [ts] });
    // [ts] test("NE-059 should set bodySize to 0 if there was no body", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request"),
    // [ts] page.evaluate(() => fetch("./get").then(r => r.text())),
    // [ts] ]);
    // [ts] const sizes = request.sizes();
    // [ts] expect_eq(sizes.requestBodySize, 0);
    // [ts] expect_eq(sizes.requestHeadersSize >= 190, true);
    // [ts] });
    // [ts] test("NE-060 should set bodySize, headersSize, and transferSize", () => {
    // [ts] server.setRoute("/get", (req, res) => {
    // In Firefox, |fetch| will be hanging until it receives |Content-Type| header
    // from server.
    // [ts] res.setHeader("Content-Type", "text/plain; charset=utf-8");
    // [ts] res.end("abc134");
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response"),
    // [ts] page.evaluate(() => fetch("./get").then(r => r.text())),
    // [ts] server.waitForRequest("/get"),
    // [ts] ]);
    // [ts] const sizes = response.request().sizes();
    // [ts] expect_eq(sizes.responseBodySize, 6);
    // [ts] expect_eq(sizes.responseHeadersSize >= 100, true);
    // [ts] });
    // [ts] test("NE-061 should set bodySize to 0 when there was no response body", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const sizes = response.request().sizes();
    // [ts] expect_eq(sizes.responseBodySize, 0);
    // [ts] expect_eq(sizes.responseHeadersSize >= 150, true);
    // [ts] });
    // [ts] test("NE-062 should have the correct responseBodySize", () => {
    // [ts] const response = page.goto(server.PREFIX + "/simplezip.json");
    // [ts] const sizes = response.request().sizes();
    // [ts] expect_eq(sizes.responseBodySize, fs.statSync(asset("simplezip.json")).size);
    // [ts] });
    // [ts] test("NE-063 should have the correct responseBodySize for chunked request", () => {
    // [ts] it.fixme(browserName == "firefox");
    // [ts] it.fixme(browserName == "webkit" && platform != "darwin");
    // [ts] const content = fs.readFileSync(asset("simplezip.json"));
    // [ts] const AMOUNT_OF_CHUNKS = 10;
    // [ts] const CHUNK_SIZE = Math.ceil(content.length / AMOUNT_OF_CHUNKS);
    // [ts] server.setRoute("/chunked-simplezip.json", (req, resp) => {
    // [ts] resp.writeHead(200, { "Content-Type": "text/html; charset=utf-8", "Transfer-Encoding": "chunked" });
    // [ts] let start = 0;
    // [ts] while (start < content.length) { // [cap-migrate] for-step: start += CHUNK_SIZE
    // [ts] const end = Math.min(start + CHUNK_SIZE, content.length);
    // Wait for each chunk to be flushed, otherwise Node 26 merges synchronous writes into a single chunk.
    // [ts] Promise.init(f => resp.write(content.slice(start, end), () => f()));
    // [ts] }
    // [ts] resp.end();
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/chunked-simplezip.json");
    // [ts] const sizes = response.request().sizes();
    // The actual file size is 5100 bytes. The extra 75 bytes are coming from the chunked encoding headers and end bytes.
    // [ts] if (browserName == "webkit")
    // WebKit on macOS reports 5173 with the legacy CFNetwork loader (builds <= 2346) and the
    // correct 5175 with NWLoader. TODO: expect 5175 once the NWLoader-based build ships.
    // [ts] expect_eq(std.mem.indexOf(u8, [5173, 5175], sizes.responseBodySize) != null, true);
    // [ts] else {
    // [ts] expect_eq(sizes.responseBodySize, 5175);
    // [ts] });
    // [ts] test("NE-064 should have the correct responseBodySize with gzip compression", () => {
    // [ts] server.enableGzip("/simplezip.json");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response"),
    // [ts] page.evaluate(() => fetch("./simplezip.json").then(r => r.text()))
    // [ts] ]);
    // [ts] const sizes = response.request().sizes();
    // [ts] const chunks: [][]u8 = [];
    // [ts] const gzip = fs.createReadStream(asset("simplezip.json")).pipe(zlib.createGzip());
    // [ts] const done = Promise.init(resolve => gzip.on("end", resolve));
    // [ts] gzip.on("data", o => chunks.push(o));
    // [ts] done;
    // [ts] expect_eq(sizes.responseBodySize, Buffer.concat(chunks).length);
    // [ts] });
    // [ts] test("NE-065 should handle redirects", () => {
    // [ts] server.setRedirect("/foo", "/bar");
    // [ts] server.setRoute("/bar", (req, resp) => resp.end("bar"));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response"),
    // [ts] page.evaluate(() => fetch("/foo", {
    // [ts] method: "POST",
    // [ts] body: "12345",
    // [ts] }).then(r => r.text())),
    // [ts] ]);
    // [ts] expect_eq((response.request().sizes()).requestBodySize, 5);
    // [ts] const newRequest = response.request().redirectedTo();
    // [ts] expect_eq((newRequest.sizes()).responseBodySize, 3);
    // [ts] });
    // [ts] test("NE-066 should throw for failed requests", () => {
    // [ts] server.setRoute("/one-style.css", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/css");
    // [ts] res.connection.destroy();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("requestfailed"),
    // [ts] page.goto(server.PREFIX + "/one-style.html")
    // [ts] ]);
    // [ts] expect(request.sizes()).rejects.toThrow("Unable to fetch sizes for failed request");
    // [ts] });
    // [ts] for (const statusCode of [200, 401, 404, 500]) {
    // [ts] }
    // [ts] test(`NE-067 should work with ${statusCode} status code`, () => {
    // [ts] server.setRoute("/foo", (req, resp) => {
    // [ts] resp.writeHead(statusCode, {
    // [ts] "Content-Type": "text/plain; charset=utf-8",
    // [ts] "Content-Length": "3",
    // [ts] });
    // [ts] resp.end("bar");
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response"),
    // [ts] page.evaluate(() => fetch("/foo", {
    // [ts] method: "POST",
    // [ts] body: "12345",
    // [ts] }).then(r => r.text())),
    // [ts] ]);
    // [ts] expect_eq(response.status(), statusCode);
    // [ts] const sizes = response.request().sizes();
    // [ts] expect_eq(sizes.requestBodySize, 5);
    // [ts] expect_eq(sizes.responseBodySize, 3);
    // [ts] });
    // [ts] }
    // [ts] test("NE-068 should have correct responseBodySize for 404 with content", () => {
    // [ts] it.fixme(browserName == "chromium");
    // [ts] server.setRoute("/broken-image.png", (req, resp) => {
    // [ts] resp.writeHead(404);
    // [ts] resp.end(`<p>this should have a non-negative size</p>`);
    // [ts] });
    // [ts] server.setRoute("/page-with-404-image.html", (req, resp) => {
    // [ts] resp.end(`
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <head><title>Page with Broken Image</title></head>
    // [ts] <body>
    // [ts] <img src="/broken-image.png" />
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] });
    // [ts] const [req] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest("broken-image\\.png$"),
    // [ts] page.goto(server.PREFIX + "/page-with-404-image.html"),
    // [ts] ]);
    // [ts] const { responseBodySize } = req.sizes();
    // [ts] expect_eq(responseBodySize >= 0, true);
    // [ts] });
    // [ts] test("NE-069 should return sizes without hanging", () => {
    // [ts] it.fixme(browserName == "chromium");
    // [ts] server.setRoute("/has-abandoned-fetch", (req, resp) => {
    // [ts] resp.end(`
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <head><title>t</title></head>
    // [ts] <body>
    // [ts] <script>
    // [ts] fetch("./404");
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] });
    // [ts] const [req] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest("404$"),
    // [ts] page.goto(server.PREFIX + "/has-abandoned-fetch")
    // [ts] ]);
    // [ts] req.sizes();
    // [ts] });
}
