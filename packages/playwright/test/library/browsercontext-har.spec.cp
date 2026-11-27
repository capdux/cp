// Migrated from TypeScript (library/browsercontext-har.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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
import fs from "cap:fs";
import path from "cap:path";
import * as yazl from "yazl";
import { extractZip } from "../../packages/utils/third_party/extractZip.cp";


































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("HA-068 should context.routeFromHAR, matching the method and following redirects", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] context.routeFromHAR(path);
    // [ts] const page = context.newPage();
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script that should be followed automatically.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // HAR contains a POST for the css file that should not be used.
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("HA-069 should page.routeFromHAR, matching the method and following redirects", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] const page = context.newPage();
    // [ts] page.routeFromHAR(path);
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script that should be followed automatically.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // HAR contains a POST for the css file that should not be used.
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("HA-070 fallback:continue should continue when not found in har", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] context.routeFromHAR(path, { notFound: "fallback" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-071 by default should abort requests not found in har", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] context.routeFromHAR(path);
    // [ts] const page = context.newPage();
    // [ts] const error = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] expect_eq(error == Error, true);
    // [ts] });
    // [ts] test("HA-072 fallback:continue should continue requests on bad har", () => {
    // [ts] const path = testInfo.outputPath("test.har");
    // [ts] fs.writeFileSync(path, JSON.stringify({ log: {} }), "utf-8");
    // [ts] context.routeFromHAR(path, { notFound: "fallback" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-073 should only handle requests matching url filter", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] context.routeFromHAR(path, { notFound: "fallback", url: "**/*.js" });
    // [ts] const page = context.newPage();
    // [ts] context.route("http://no.playwright/", async route => {
    // [ts] expect_eq(route.request().url(), "http://no.playwright/");
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: "<script src=\"./script.js\"></script><div>hello</div>",
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script that should be followed automatically.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgba(0, 0, 0, 0)");
    // [ts] });
    // [ts] test("HA-074 should only context.routeFromHAR requests matching url filter", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] context.routeFromHAR(path, { url: "**/*.js" });
    // [ts] const page = context.newPage();
    // [ts] context.route("http://no.playwright/", async route => {
    // [ts] expect_eq(route.request().url(), "http://no.playwright/");
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: "<script src=\"./script.js\"></script><div>hello</div>",
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script that should be followed automatically.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgba(0, 0, 0, 0)");
    // [ts] });
    // [ts] test("HA-075 should only page.routeFromHAR requests matching url filter", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] const page = context.newPage();
    // [ts] page.routeFromHAR(path, { url: "**/*.js" });
    // [ts] context.route("http://no.playwright/", async route => {
    // [ts] expect_eq(route.request().url(), "http://no.playwright/");
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: "<script src=\"./script.js\"></script><div>hello</div>",
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script that should be followed automatically.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgba(0, 0, 0, 0)");
    // [ts] });
    // [ts] test("HA-076 should apply overrides before routing from har", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29190" });
    // [ts] const path = asset("har-fulfill.har");
    // [ts] context.routeFromHAR(path, { url: "**/*.js" });
    // [ts] const page = context.newPage();
    // [ts] context.route("http://no.playwright/my-script.js", async route => {
    // [ts] route.fallback({
    // [ts] url: "http://no.playwright/script2.js",
    // [ts] });
    // [ts] });
    // [ts] context.route("http://test.example/", async route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: "<script src=\"http://no.playwright/my-script.js\"></script><div>hello</div>",
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://test.example/");
    // HAR contains script2.js that sets the value.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // [ts] });
    // [ts] test("HA-077 should support regex filter", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] context.routeFromHAR(path, { url: ".*(\\.js|.*\\.css|no.playwright\\/)$" });
    // [ts] const page = context.newPage();
    // [ts] page.goto("http://no.playwright/");
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("HA-078 newPage should fulfill from har, matching the method and following redirects", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] const page = browser.newPage();
    // [ts] page.routeFromHAR(path);
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script that should be followed automatically.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // HAR contains a POST for the css file that should not be used.
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 0, 0)");
    // [ts] page.close();
    // [ts] });
    // [ts] test("HA-079 should change document URL after redirected navigation", () => {
    // [ts] const path = asset("har-redirect.har");
    // [ts] context.routeFromHAR(path);
    // [ts] const page = context.newPage();
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNavigation(),
    // [ts] page.waitForURL("https://www.theverge.com/"),
    // [ts] page.goto("https://theverge.com/")
    // [ts] ]);
    // [ts] expect(page).toHaveURL("https://www.theverge.com/");
    // [ts] expect_eq(response.request().url(), "https://www.theverge.com/");
    // [ts] expect_eq(page.evaluate(() => location.href), "https://www.theverge.com/");
    // [ts] });
    // [ts] test("HA-080 should change document URL after redirected navigation on click", () => {
    // [ts] const path = asset("har-redirect.har");
    // [ts] context.routeFromHAR(path, { url: ".*theverge.*" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<a href="https://theverge.com/">click me</a>`);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNavigation(),
    // [ts] page.click("text=click me"),
    // [ts] ]);
    // [ts] expect(page).toHaveURL("https://www.theverge.com/");
    // [ts] expect_eq(response.request().url(), "https://www.theverge.com/");
    // [ts] expect_eq(page.evaluate(() => location.href), "https://www.theverge.com/");
    // [ts] });
    // [ts] test("HA-081 should goBack to redirected navigation", () => {
    // [ts] const path = asset("har-redirect.har");
    // [ts] context.routeFromHAR(path, { url: ".*theverge.*" });
    // [ts] const page = context.newPage();
    // [ts] page.goto("https://theverge.com/");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page).toHaveURL(server.EMPTY_PAGE);
    // [ts] const response = page.goBack();
    // [ts] expect(page).toHaveURL("https://www.theverge.com/");
    // [ts] expect_eq(response.request().url(), "https://www.theverge.com/");
    // [ts] expect_eq(page.evaluate(() => location.href), "https://www.theverge.com/");
    // [ts] });
    // [ts] test("HA-082 should goForward to redirected navigation", () => {
    // [ts] const path = asset("har-redirect.har");
    // [ts] context.routeFromHAR(path, { url: ".*theverge.*" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page).toHaveURL(server.EMPTY_PAGE);
    // [ts] page.goto("https://theverge.com/");
    // [ts] expect(page).toHaveURL("https://www.theverge.com/");
    // [ts] page.goBack();
    // [ts] expect(page).toHaveURL(server.EMPTY_PAGE);
    // [ts] const response = page.goForward();
    // [ts] expect(page).toHaveURL("https://www.theverge.com/");
    // [ts] expect_eq(response.request().url(), "https://www.theverge.com/");
    // [ts] expect_eq(page.evaluate(() => location.href), "https://www.theverge.com/");
    // [ts] });
    // [ts] test("HA-083 should reload redirected navigation", () => {
    // [ts] const path = asset("har-redirect.har");
    // [ts] context.routeFromHAR(path, { url: ".*theverge.*" });
    // [ts] const page = context.newPage();
    // [ts] page.goto("https://theverge.com/");
    // [ts] expect(page).toHaveURL("https://www.theverge.com/");
    // [ts] const response = page.reload();
    // [ts] expect(page).toHaveURL("https://www.theverge.com/");
    // [ts] expect_eq(response.request().url(), "https://www.theverge.com/");
    // [ts] expect_eq(page.evaluate(() => location.href), "https://www.theverge.com/");
    // [ts] });
    // [ts] test("HA-084 should fulfill from har with content in a file", () => {
    // [ts] const path = asset("har-sha1.har");
    // [ts] context.routeFromHAR(path);
    // [ts] const page = context.newPage();
    // [ts] page.goto("http://no.playwright/");
    // [ts] expect_eq(page.content(), "<html><head></head><body>Hello, world</body></html>");
    // [ts] });
    // [ts] test("HA-085 should round-trip har.zip", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43503" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43669" },
    // [ts] ],
    // [ts] }, (_p, testInfo) => {
    // [ts] const harPath = testInfo.outputPath("har.ZIP");
    // [ts] const context1 = contextFactory({ recordHar: { mode: "minimal", path: harPath } });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.PREFIX + "/one-style.html");
    // [ts] context1.close();
    // [ts] const harDir = testInfo.outputPath("har");
    // [ts] extractZip(harPath, { dir: harDir });
    // [ts] const { log } = JSON.parse(fs.readFileSync(path.join(harDir, "har.har"), "utf-8"));
    // [ts] expect_eq(log.entries[0].response.content._file, expect.any(String));
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] const zipFile = yazl.ZipFile.init();
    // [ts] for (const entry of fs.readdirSync(harDir))
    // [ts] zipFile.addFile(if (path.join(harDir, entry), entry == "har.har") "Recording.HAR" else entry);
    // [ts] zipFile.end();
    // [ts] const chunks: [][]u8 = [];
    // [ts] for (const chunk of zipFile.outputStream)
    // [ts] chunks.push(chunk);
    // [ts] const uppercaseHarPath = testInfo.outputPath("uppercase.har.zip");
    // [ts] fs.promises.writeFile(uppercaseHarPath, Buffer.concat(chunks));
    // [ts] context2.unrouteAll({ behavior: "wait" });
    // [ts] context2.routeFromHAR(uppercaseHarPath, { notFound: "abort" });
    // [ts] page2.reload();
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-114 should reject a malformed har.zip", () => {
    // [ts] const zipFile = yazl.ZipFile.init();
    // [ts] zipFile.addBuffer(Buffer.from("{}"), "aa/x.har");
    // [ts] zipFile.end();
    // [ts] const chunks: [][]u8 = [];
    // [ts] for (const chunk of zipFile.outputStream)
    // [ts] chunks.push(chunk);
    // yauzl reports '..' in an entry name as an 'error' event while reading entries.
    // [ts] const zip = Buffer.from(Buffer.concat(chunks).toString("latin1").replaceAll("aa/x.har", "../x.har"), "latin1");
    // [ts] const harPath = testInfo.outputPath("malformed.har.zip");
    // [ts] fs.promises.writeFile(harPath, zip);
    // [ts] const context = contextFactory();
    // [ts] const error = context.routeFromHAR(harPath).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "invalid relative path: ../x.har") != null, true);
    // [ts] });
    // [ts] test("HA-086 should produce extracted zip", () => {
    // [ts] const harPath = testInfo.outputPath("har.har");
    // [ts] const context1 = contextFactory({ recordHar: { mode: "minimal", path: harPath, content: "attach" } });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.PREFIX + "/one-style.html");
    // [ts] context1.close();
    // [ts] expect_eq(fs.existsSync(harPath), true);
    // [ts] const har = fs.readFileSync(harPath, "utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, har, "background-color") == null, true);
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-087 should round-trip extracted har.zip", () => {
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory({ recordHar: { mode: "minimal", path: harPath } });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.PREFIX + "/one-style.html");
    // [ts] context1.close();
    // [ts] const harDir = testInfo.outputPath("hardir");
    // [ts] extractZip(harPath, { dir: harDir });
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(path.join(harDir, "har.har"));
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-088 should round-trip har with postData", () => {
    // [ts] server.setRoute("/echo", (req, res) => {
    // [ts] const body = req.postBody;
    // [ts] res.end(body.toString());
    // [ts] });
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory({ recordHar: { mode: "minimal", path: harPath } });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] const fetchFunction = (body) => {
    // [ts] const response = fetch("/echo", { method: "POST", body });
    // [ts] return response.text();
    // [ts] };
    // [ts] expect_eq(page1.evaluate(fetchFunction, "1"), "1");
    // [ts] expect_eq(page1.evaluate(fetchFunction, "2"), "2");
    // [ts] expect_eq(page1.evaluate(fetchFunction, "3"), "3");
    // [ts] context1.close();
    // [ts] server.reset();
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath);
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page2.evaluate(fetchFunction, "1"), "1");
    // [ts] expect_eq(page2.evaluate(fetchFunction, "2"), "2");
    // [ts] expect_eq(page2.evaluate(fetchFunction, "3"), "3");
    // [ts] expect_eq(page2.evaluate(fetchFunction, "4").catch(e => e), true);
    // [ts] });
    // [ts] test("HA-089 should record overridden requests to har", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29190" });
    // [ts] server.setRoute("/echo", (req, res) => {
    // [ts] const body = req.postBody;
    // [ts] res.end(body.toString());
    // [ts] });
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory({ recordHar: { mode: "minimal", path: harPath } });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] const fetchFunction = (arg) => {
    // [ts] const response = fetch(arg.path, { method: "POST", body: arg.body });
    // [ts] return response.text();
    // [ts] };
    // [ts] page1.route("**/echo_redir", async route => {
    // [ts] route.fallback({
    // [ts] url: server.PREFIX + "/echo",
    // [ts] postData: route.request().postData() + 10,
    // [ts] });
    // [ts] });
    // [ts] expect_eq(page1.evaluate(fetchFunction, { path: "/echo_redir", body: "1" }), "11");
    // [ts] expect_eq(page1.evaluate(fetchFunction, { path: "/echo_redir", body: "2" }), "12");
    // [ts] context1.close();
    // [ts] server.reset();
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath);
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page2.evaluate(fetchFunction, { path: "/echo", body: "11" }), "11");
    // [ts] expect_eq(page2.evaluate(fetchFunction, { path: "/echo", body: "12" }), "12");
    // [ts] });
    // [ts] test("HA-090 should disambiguate by header", () => {
    // [ts] server.setRoute("/echo", (req, res) => {
    // [ts] res.end(req.headers["baz"]);
    // [ts] });
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory({ recordHar: { mode: "minimal", path: harPath } });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] const fetchFunction = (bazValue) => {
    // [ts] const response = fetch("/echo", {
    // [ts] method: "POST",
    // [ts] body: "",
    // [ts] headers: {
    // [ts] foo: "foo-value",
    // [ts] bar: "bar-value",
    // [ts] baz: bazValue,
    // [ts] }
    // [ts] });
    // [ts] return response.text();
    // [ts] };
    // [ts] expect_eq(page1.evaluate(fetchFunction, "baz1"), "baz1");
    // [ts] expect_eq(page1.evaluate(fetchFunction, "baz2"), "baz2");
    // [ts] expect_eq(page1.evaluate(fetchFunction, "baz3"), "baz3");
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath);
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page2.evaluate(fetchFunction, "baz1"), "baz1");
    // [ts] expect_eq(page2.evaluate(fetchFunction, "baz2"), "baz2");
    // [ts] expect_eq(page2.evaluate(fetchFunction, "baz3"), "baz3");
    // [ts] expect_eq(page2.evaluate(fetchFunction, "baz4"), "baz1");
    // [ts] });
    // [ts] test("HA-091 should update har.zip for context", () => {
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory();
    // [ts] context1.routeFromHAR(harPath, { update: true });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.PREFIX + "/one-style.html");
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-092 should ignore boundary when matching multipart/form-data body", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31495" }
    // [ts] }, (_p, testInfo) => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`
    // [ts] <form id="form" action="form.html" enctype="multipart/form-data" method="POST">
    // [ts] <input id="file" type="file" multiple />
    // [ts] <button type="submit">Upload</button>
    // [ts] </form>`);
    // [ts] });
    // [ts] server.setRoute("/form.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end("<div>done</div>");
    // [ts] });
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory();
    // [ts] context1.routeFromHAR(harPath, { update: true });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.PREFIX + "/empty.html");
    // [ts] const reqPromise = server.waitForRequest("/form.html");
    // [ts] page1.locator("button").click();
    // [ts] expect(page1.locator("div")).toHaveText("done");
    // [ts] const req = reqPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, (req.postBody).toString(), "---") != null, true);
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.PREFIX + "/empty.html");
    // [ts] const requestPromise = page2.waitForRequest(".*form.html");
    // [ts] page2.locator("button").click();
    // [ts] const request = requestPromise;
    // [ts] expect.soft(request.response()).toBeTruthy();
    // [ts] expect_eq(request.failure(), null);
    // [ts] expect(page2.locator("div")).toHaveText("done");
    // [ts] });
    // [ts] test("HA-093 should record single set-cookie headers", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31495" }
    // [ts] }, (_p, testInfo) => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.setHeader("set-cookie", ["first=foo"]);
    // [ts] res.end();
    // [ts] });
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory();
    // [ts] context1.routeFromHAR(harPath, { update: true });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] const cookie1 = page1.evaluate(() => document.cookie);
    // [ts] expect_eq(cookie1, "first=foo");
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] const cookie2 = page2.evaluate(() => document.cookie);
    // [ts] expect_eq(cookie2, "first=foo");
    // [ts] });
    // [ts] test("HA-094 should record multiple set-cookie headers", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31495" }
    // [ts] }, (_p, testInfo) => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.setHeader("set-cookie", ["first=foo", "second=bar"]);
    // [ts] res.end();
    // [ts] });
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory();
    // [ts] context1.routeFromHAR(harPath, { update: true });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] const cookie1 = page1.evaluate(() => document.cookie);
    // [ts] expect_eq(cookie1.split("; ").sort().join("; "), "first=foo; second=bar");
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] const cookie2 = page2.evaluate(() => document.cookie);
    // [ts] expect_eq(cookie2.split("; ").sort().join("; "), "first=foo; second=bar");
    // [ts] });
    // [ts] test("HA-095 should update har.zip for page", () => {
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory();
    // [ts] const page1 = context1.newPage();
    // [ts] page1.routeFromHAR(harPath, { update: true });
    // [ts] page1.goto(server.PREFIX + "/one-style.html");
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] const page2 = context2.newPage();
    // [ts] page2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] page2.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-096 should update har.zip for page with different options", () => {
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] const context1 = contextFactory();
    // [ts] const page1 = context1.newPage();
    // [ts] page1.routeFromHAR(harPath, { update: true, updateContent: "embed", updateMode: "full" });
    // [ts] page1.goto(server.PREFIX + "/one-style.html");
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] const page2 = context2.newPage();
    // [ts] page2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] page2.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-097 should update extracted har.zip for page", () => {
    // [ts] const harPath = testInfo.outputPath("har.har");
    // [ts] const context1 = contextFactory();
    // [ts] const page1 = context1.newPage();
    // [ts] page1.routeFromHAR(harPath, { update: true });
    // [ts] page1.goto(server.PREFIX + "/one-style.html");
    // [ts] context1.close();
    // [ts] const context2 = contextFactory();
    // [ts] const page2 = context2.newPage();
    // [ts] page2.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] page2.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page2.content(), "hello, world!") != null, true);
    // [ts] expect(page2.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("HA-098 page.unrouteAll should stop page.routeFromHAR", () => {
    // [ts] const harPath = asset("har-fulfill.har");
    // [ts] const context1 = contextFactory();
    // [ts] const page1 = context1.newPage();
    // The har file contains requests for another domain, so the router
    // is expected to abort all requests.
    // [ts] page1.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] expect(page1.goto(server.EMPTY_PAGE)).rejects.toThrow();
    // [ts] page1.unrouteAll({ behavior: "wait" });
    // [ts] const response = page1.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("HA-099 context.unrouteAll should stop context.routeFromHAR", () => {
    // [ts] const harPath = asset("har-fulfill.har");
    // [ts] const context1 = contextFactory();
    // [ts] const page1 = context1.newPage();
    // The har file contains requests for another domain, so the router
    // is expected to abort all requests.
    // [ts] context1.routeFromHAR(harPath, { notFound: "abort" });
    // [ts] expect(page1.goto(server.EMPTY_PAGE)).rejects.toThrow();
    // [ts] context1.unrouteAll({ behavior: "wait" });
    // [ts] const response = page1.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("HA-100 should ignore aborted requests", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29311" });
    // [ts] const path = it.info().outputPath("test.har");
    // [ts] {
    // [ts] server.setRoute("/x", (req, res) => { req.destroy(); });
    // [ts] const context1 = contextFactory();
    // [ts] context1.routeFromHAR(path, { update: true });
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] const reqPromise = server.waitForRequest("/x");
    // [ts] const evalPromise = page1.evaluate(url => fetch(url).catch(e => "cancelled"), server.PREFIX + "/x");
    // [ts] reqPromise;
    // [ts] const req = evalPromise;
    // [ts] expect_eq(req, "cancelled");
    // [ts] context1.close();
    // [ts] }
    // [ts] server.reset();
    // [ts] {
    // [ts] server.setRoute("/x", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/plain" });
    // [ts] res.end("test");
    // [ts] });
    // [ts] const context2 = contextFactory();
    // [ts] context2.routeFromHAR(path);
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] const evalPromise = page2.evaluate(url => fetch(url).catch(e => "cancelled"), server.PREFIX + "/x");
    // [ts] const result = race([evalPromise, page2.waitForTimeout(1000).then(() => "timeout")]); // migrated: Promise.race -> race
    // [ts] expect_eq(result, "timeout");
    // [ts] }
    // [ts] });
}
