// Migrated from TypeScript (library/har.spec.ts) via cap from -ts

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
import fs from "cap:fs";
import path from "cap:path";
// [cap-migrate] import { BrowserContext, BrowserContextOptions } from "playwright";
// [cap-migrate] import { AddressInfo } from "cap:net";
// [cap-migrate] import { Log } from "../../packages/isomorphic/trace/versions/har.cp";
import { parseHar } from "../config/utils.cp";
import { TestServer } from "../config/testserver.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
const { createHttp2Server } = utils;

// TODO(cap-migrate): refine TS function
// [ts] async fn pageWithHar(contextFactory: options?: BrowserContextOptions = > Promise<BrowserContext>, testInfo: string, options: string = {}) {
// [ts] const harPath = testInfo.outputPath(options.outputPath || "test.har");
// [ts] const context = contextFactory({ recordHar: { path: harPath, ...options }, ignoreHTTPSErrors: true });
// [ts] const page = context.newPage();
// [ts] return {
// [ts] page,
// [ts] context,
// [ts] getLog: () => {
// [ts] context.close();
// [ts] return JSON.parse(fs.readFileSync(harPath).toString())["log"];
// [ts] },
// [ts] getZip: () => {
// [ts] context.close();
// [ts] return parseHar(harPath);
// [ts] },
// [ts] };
// [ts] }










































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("HA-001 should have version and creator", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.version, "1.2");
    // [ts] expect_eq(log.creator.name, "Playwright");
    // [ts] expect_eq(std.mem.indexOf(u8, log.creator.version, process.env.PW_VERSION_OVERRIDE || require("../../package.json")["version"]) != null, true);
    // [ts] });
    // [ts] test("HA-002 should have browser", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.browser.name, browserName);
    // [ts] expect_eq(log.browser.version, browser.version());
    // [ts] });
    // [ts] test("HA-003 should have pages", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44242" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto("data:text/html,<title>Hello</title>");
    // For data: load comes before domcontentloaded...
    // [ts] page.waitForLoadState("domcontentloaded");
    // [ts] const pageWithDialog = page.context().newPage();
    // [ts] const dialogPromise = pageWithDialog.waitForEvent("dialog");
    // [ts] pageWithDialog.goto("data:text/html,<body onload=\"alert(1)\">", { waitUntil: "commit" });
    // [ts] dialogPromise;
    // [ts] const log = getLog();
    // [ts] expect_eq(log.pages.length, 2);
    // [ts] const pageEntry = log.pages[0];
    // [ts] expect_eq(pageEntry.id, true);
    // [ts] expect_eq(pageEntry.title, "Hello");
    // [ts] expect_eq(Date.init(pageEntry.startedDateTime).valueOf() > Date.now() - 3600 * 1000, true);
    // [ts] expect_eq(pageEntry.pageTimings.onContentLoad > 0, true);
    // [ts] expect_eq(pageEntry.pageTimings.onLoad > 0, true);
    // [ts] });
    // [ts] test("HA-004 should have pages in persistent context", () => {
    // [ts] const harPath = testInfo.outputPath("test.har");
    // [ts] const { context, page } = launchPersistent({ recordHar: { path: harPath } });
    // [ts] page.goto("data:text/html,<title>Hello</title>");
    // For data: load comes before domcontentloaded...
    // [ts] page.waitForLoadState("domcontentloaded");
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString())["log"];
    // [ts] expect_eq(log.pages.length, 1);
    // [ts] const pageEntry = log.pages[0];
    // [ts] expect_eq(pageEntry.id, true);
    // [ts] expect_eq(pageEntry.title, "Hello");
    // [ts] });
    // [ts] test("HA-005 should include request", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries.length, 1);
    // [ts] const entry = log.entries[0];
    // [ts] expect_eq(entry.pageref, log.pages[0].id);
    // [ts] expect_eq(entry.request.url, server.EMPTY_PAGE);
    // [ts] expect_eq(entry.request.method, "GET");
    // [ts] expect_eq(entry.request.httpVersion, "HTTP/1.1");
    // [ts] expect_eq(entry.request.headers.length > 1, true);
    // [ts] expect_eq(entry.request.headers.find(h => h.name.toLowerCase() == "user-agent"), true);
    // [ts] expect_eq(entry.request.bodySize, 0);
    // [ts] });
    // [ts] test("HA-006 should populate entry startedDateTime from the browser", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // The browser issues a request after a short delay, then we deliberately
    // block Node's event loop. The protocol event for the request therefore
    // queues up while Node is busy and is only processed by the harTracer after
    // the block ends. If `startedDateTime` is populated from Node's clock at
    // observation time it will land inside the busy-loop window (i.e. close to
    // `unblockedAt`); if it comes from the browser via the debugging protocol
    // it will be tied to when the browser actually sent the request.
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => { void fetch("/delayed-fetch"); }, 50);
    // [ts] });
    // [ts] const blockUntil = Date.now() + 300;
    // [ts] while (Date.now() < blockUntil) {
    // Busy loop to prevent Node from processing protocol events.
    // [ts] }
    // [ts] const unblockedAt = Date.now();
    // [ts] page.waitForResponse("**/delayed-fetch");
    // [ts] const log = getLog();
    // [ts] const entry = log.entries.find(e => e.request.url.endsWith("/delayed-fetch"));
    // [ts] const startedAt = Date.init(entry.startedDateTime).valueOf();
    // [ts] expect_eq(Number.isFinite(startedAt), true);
    // The recorded time should be tied to when the browser actually sent the
    // request (during the busy loop), not to when Node observed the protocol
    // event (after the busy loop).
    // [ts] expect_eq(startedAt < unblockedAt - 100, true);
    // [ts] });
    // [ts] test("HA-007 should include response", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] const entry = log.entries[0];
    // [ts] expect_eq(entry.response.status, 200);
    // [ts] expect_eq(entry.response.statusText, "OK");
    // [ts] expect_eq(entry.response.httpVersion, "HTTP/1.1");
    // [ts] expect_eq(entry.response.headers.length > 1, true);
    // [ts] expect_eq(std.mem.indexOf(u8, entry.response.headers.find(h => h.name.toLowerCase() == "content-type").value, "text/html") != null, true);
    // [ts] });
    // [ts] test("HA-008 should include redirectURL", () => {
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/foo.html");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries.length, 2);
    // [ts] const entry = log.entries[0];
    // [ts] expect_eq(entry.response.status, 302);
    // [ts] expect_eq(entry.response.redirectURL, server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("HA-009 should include query params", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/har.html?name=value");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].request.queryString, [{ name: "name", value: "value" }]);
    // [ts] });
    // [ts] test("HA-010 should include postData", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => fetch("./post", { method: "POST", body: "Hello" }));
    // [ts] const log = getLog();
    // [ts] expect: (log.entries[1].request.postData) => {
    // [ts] mimeType: "text/plain;charset=UTF-8",
    // [ts] params: [],
    // [ts] text: "Hello"
    // [ts] });
    // [ts] });
    // [ts] test("HA-011 should include binary postData", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] fetch("./post", { method: "POST", body: Uint8Array.init(Array.from(Array(16).keys())) });
    // [ts] });
    // [ts] const log = getLog();
    // [ts] expect: (log.entries[1].request.postData) => {
    // [ts] mimeType: "application/octet-stream",
    // [ts] params: [],
    // [ts] text: ""
    // [ts] });
    // [ts] });
    // [ts] test("HA-012 should include form params", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<form method='POST' action='/post'><input type='text' name='foo' value='bar'><input type='number' name='baz' value='123'><input type='submit'></form>`);
    // [ts] page.click("input[type=submit]");
    // [ts] const log = getLog();
    // [ts] expect: (log.entries[1].request.postData) => {
    // [ts] mimeType: "application/x-www-form-urlencoded",
    // [ts] params: [
    // [ts] { name: "foo", value: "bar" },
    // [ts] { name: "baz", value: "123" }
    // [ts] ],
    // [ts] text: "foo=bar&baz=123"
    // [ts] });
    // [ts] });
    // [ts] test("HA-013 should include cookies", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] const context = page.context();
    // [ts] context.addCookies([
    // [ts] { name: "name1", value: "\"value1\"", domain: "localhost", path: "/", httpOnly: true },
    // [ts] { name: "name2", value: "val\"ue2", domain: "localhost", path: "/", sameSite: "Lax" },
    // [ts] { name: "name3", value: "val=ue3", domain: "localhost", path: "/" },
    // [ts] { name: "name4", value: "val,ue4", domain: "localhost", path: "/" },
    // [ts] ]);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] expect(log.entries[0].request.cookies).toEqual([
    // [ts] { name: "name1", value: "\"value1\"" },
    // [ts] { name: "name2", value: "val\"ue2" },
    // [ts] { name: "name3", value: "val=ue3" },
    // [ts] { name: "name4", value: "val,ue4" },
    // [ts] ]);
    // [ts] });
    // [ts] test("HA-014 should include set-cookies", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [
    // [ts] "name1=value1; HttpOnly",
    // [ts] "name2=\"value2\"",
    // [ts] "name3=value4; Path=/; Domain=example.com; Max-Age=1500",
    // [ts] ]);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] const cookies = log.entries[0].response.cookies;
    // [ts] expect_eq(cookies[0], { name: "name1", value: "value1", httpOnly: true });
    // [ts] expect_eq(cookies[1], { name: "name2", value: "\"value2\"" });
    // [ts] expect_eq(Date.init(cookies[2].expires).valueOf() > Date.now(), true);
    // [ts] });
    // [ts] test("HA-015 should include set-cookies without equals sign as nameless", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43155" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["token; HttpOnly"]);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].response.cookies, [{ name: "", value: "token", httpOnly: true }]);
    // [ts] });
    // [ts] test("HA-016 should include set-cookies with lowercase attributes", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["name=value; path=/; httponly; secure; samesite=Lax"]);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] const cookies = log.entries[0].response.cookies;
    // [ts] expect_eq(cookies[0], { name: "name", value: "value", path: "/", httpOnly: true, secure: true, sameSite: "Lax" });
    // [ts] });
    // [ts] test("HA-017 should skip invalid Expires", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [
    // [ts] "name=value;Expires=Sat Sep 14 01:02:27 CET 2024",
    // [ts] ]);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] const cookies = log.entries[0].response.cookies;
    // [ts] expect_eq(cookies[0], { name: "name", value: "value" });
    // [ts] });
    // [ts] test("HA-018 should include set-cookies with comma", () => {
    // [ts] it.fixme(browserName == "webkit", "We get \"name1=val, ue1, name2=val, ue2\" as a header value");
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [
    // [ts] "name1=val, ue1", "name2=val, ue2",
    // [ts] ]);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] const cookies = log.entries[0].response.cookies;
    // [ts] expect_eq(cookies[0], { name: "name1", value: "val, ue1" });
    // [ts] expect_eq(cookies[1], { name: "name2", value: "val, ue2" });
    // [ts] });
    // [ts] test("HA-019 should include secure set-cookies", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] httpsServer.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [
    // [ts] "name1=value1; Secure",
    // [ts] ]);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] const cookies = log.entries[0].response.cookies;
    // [ts] expect_eq(cookies[0], { name: "name1", value: "value1", secure: true });
    // [ts] });
    // [ts] test("HA-020 should record request overrides", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.route("**/foo", route => {
    // [ts] void route.fallback({
    // [ts] url: server.EMPTY_PAGE,
    // [ts] method: "POST",
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] "content-type": "text/plain",
    // [ts] "cookie": "foo=bar",
    // [ts] "custom": "value"
    // [ts] },
    // [ts] postData: "Hi!"
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/foo");
    // [ts] const log = getLog();
    // [ts] const request = log.entries[0].request;
    // [ts] expect_eq(request.url, server.EMPTY_PAGE);
    // [ts] expect_eq(request.method, "POST");
    // [ts] expect(request.headers).toContainEqual({ name: "custom", value: "value" });
    // [ts] expect_eq(request.cookies, []);
    // [ts] expect_eq(request.postData, { "mimeType": "text/plain", "params": [], "text": "Hi!" });
    // [ts] });
    // [ts] test("HA-021 should include content @smoke", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] page.evaluate(() => fetch("/pptr.png").then(r => r.arrayBuffer()));
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].response.content.encoding, null);
    // [ts] expect_eq(log.entries[0].response.content.mimeType, "text/html; charset=utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, log.entries[0].response.content.text, "HAR Page") != null, true);
    // [ts] expect_eq(log.entries[0].response.content.size >= 96, true);
    // [ts] expect_eq(log.entries[0].response.content.compression, 0);
    // [ts] expect_eq(log.entries[1].response.content.encoding, null);
    // [ts] expect_eq(log.entries[1].response.content.mimeType, "text/css; charset=utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, log.entries[1].response.content.text, "pink") != null, true);
    // [ts] expect_eq(log.entries[1].response.content.size >= 37, true);
    // [ts] expect_eq(log.entries[1].response.content.compression, 0);
    // [ts] expect_eq(log.entries[2].response.content.encoding, "base64");
    // [ts] expect_eq(log.entries[2].response.content.mimeType, "image/png");
    // [ts] expect_eq(Buffer.from(log.entries[2].response.content.text, "base64").byteLength > 0, true);
    // [ts] expect_eq(log.entries[2].response.content.size >= 6000, true);
    // [ts] expect_eq(log.entries[2].response.content.compression, 0);
    // [ts] });
    // [ts] test("HA-113 should store binary content with a textual mime type as base64", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43493" },
    // [ts] }, (_p, testInfo) => {
    // [ts] it.fixme(browserName == "webkit", "WebKit decodes the body of a textual resource before it gets to us");
    // [ts] const body = Buffer.from([0x80, 0xc3, 0x28, 0xff, 0x41]);
    // [ts] server.setRoute("/binary.js", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/javascript");
    // [ts] res.end(body);
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => fetch("/binary.js").then(r => r.arrayBuffer()));
    // [ts] const log = getLog();
    // [ts] const content = log.entries.find(entry => entry.request.url.endsWith("/binary.js")).response.content;
    // [ts] expect_eq(content.encoding, "base64");
    // [ts] expect_eq(Buffer.from(content.text, "base64"), body);
    // [ts] });
    // [ts] test("HA-022 should use attach mode for zip extension", () => {
    // [ts] const { page, getZip } = pageWithHar(contextFactory, testInfo, { outputPath: "test.har.zip" });
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] page.evaluate(() => fetch("/pptr.png").then(r => r.arrayBuffer()));
    // [ts] const zip = getZip();
    // [ts] const log = JSON.parse(zip.get("har.har").toString())["log"];
    // [ts] expect_eq(log.entries[0].response.content.text, null);
    // [ts] expect_eq(std.mem.indexOf(u8, zip.get("75841480e2606c03389077304342fac2c58ccb1b.html").toString(), "HAR Page") != null, true);
    // [ts] });
    // [ts] test("HA-023 should omit content", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo, { content: "omit", outputPath: "test.har" });
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] page.evaluate(() => fetch("/pptr.png").then(r => r.arrayBuffer()));
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].response.content.text, null);
    // [ts] expect_eq(log.entries[0].response.content._file, null);
    // [ts] });
    // [ts] test("HA-024 should omit content legacy", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo, { omitContent: true, outputPath: "test.har" });
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] page.evaluate(() => fetch("/pptr.png").then(r => r.arrayBuffer()));
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].response.content.text, null);
    // [ts] expect_eq(log.entries[0].response.content._file, null);
    // [ts] });
    // [ts] test("HA-025 should attach content", () => {
    // [ts] const { page, getZip } = pageWithHar(contextFactory, testInfo, { content: "attach", outputPath: "test.har.zip" });
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] page.evaluate(() => fetch("/pptr.png").then(r => r.arrayBuffer()));
    // [ts] const zip = getZip();
    // [ts] const log = JSON.parse(zip.get("har.har").toString())["log"];
    // [ts] expect_eq(log.entries[0].response.content.encoding, null);
    // [ts] expect_eq(log.entries[0].response.content.mimeType, "text/html; charset=utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, log.entries[0].response.content._file, "75841480e2606c03389077304342fac2c58ccb1b") != null, true);
    // [ts] expect_eq(log.entries[0].response.content.size >= 96, true);
    // [ts] expect_eq(log.entries[0].response.content.compression, 0);
    // [ts] expect_eq(log.entries[1].response.content.encoding, null);
    // [ts] expect_eq(log.entries[1].response.content.mimeType, "text/css; charset=utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, log.entries[1].response.content._file, "79f739d7bc88e80f55b9891a22bf13a2b4e18adb") != null, true);
    // [ts] expect_eq(log.entries[1].response.content.size >= 37, true);
    // [ts] expect_eq(log.entries[1].response.content.compression, 0);
    // [ts] expect_eq(log.entries[2].response.content.encoding, null);
    // [ts] expect_eq(log.entries[2].response.content.mimeType, "image/png");
    // [ts] expect_eq(std.mem.indexOf(u8, log.entries[2].response.content._file, "a4c3a18f0bb83f5d9fe7ce561e065c36205762fa") != null, true);
    // [ts] expect_eq(log.entries[2].response.content.size >= 6000, true);
    // [ts] expect_eq(log.entries[2].response.content.compression, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, zip.get("75841480e2606c03389077304342fac2c58ccb1b.html").toString(), "HAR Page") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, zip.get("79f739d7bc88e80f55b9891a22bf13a2b4e18adb.css").toString(), "pink") != null, true);
    // [ts] expect_eq(zip.get("a4c3a18f0bb83f5d9fe7ce561e065c36205762fa.png").byteLength, log.entries[2].response.content.size);
    // [ts] });
    // [ts] test("HA-026 should filter by glob", () => {
    // [ts] const harPath = testInfo.outputPath("test.har");
    // [ts] const context = contextFactory({ baseURL: server.PREFIX, recordHar: { path: harPath, urlFilter: "/*.css" }, ignoreHTTPSErrors: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto("/har.html");
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString())["log"];
    // [ts] expect_eq(log.entries.length, 1);
    // [ts] expect_eq(log.entries[0].request.url.endsWith("one-style.css"), true);
    // [ts] });
    // [ts] test("HA-027 should filter by regexp", () => {
    // [ts] const harPath = testInfo.outputPath("test.har");
    // [ts] const context = contextFactory({ recordHar: { path: harPath, urlFilter: "HAR.X?HTML" }, ignoreHTTPSErrors: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString())["log"];
    // [ts] expect_eq(log.entries.length, 1);
    // [ts] expect_eq(log.entries[0].request.url.endsWith("har.html"), true);
    // [ts] });
    // [ts] test("HA-028 should include sizes", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries.length, 2);
    // [ts] expect_eq(log.entries[0].request.url.endsWith("har.html"), true);
    // [ts] expect_eq(log.entries[0].request.headersSize >= 100, true);
    // [ts] expect_eq(log.entries[0].response.bodySize, fs.statSync(asset("har.html")).size);
    // [ts] expect_eq(log.entries[0].response.headersSize >= 100, true);
    // [ts] expect_eq(log.entries[0].response._transferSize >= 250, true);
    // [ts] expect_eq(log.entries[1].request.url.endsWith("one-style.css"), true);
    // [ts] expect_eq(log.entries[1].response.bodySize, fs.statSync(asset("one-style.css")).size);
    // [ts] expect_eq(log.entries[1].response.headersSize >= 100, true);
    // [ts] expect_eq(log.entries[1].response._transferSize >= 150, true);
    // [ts] });
    // [ts] test("HA-029 should work with gzip compression", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.enableGzip("/simplezip.json");
    // [ts] const response = page.goto(server.PREFIX + "/simplezip.json");
    // [ts] expect_eq(response.headers()["content-encoding"], "gzip");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries.length, 1);
    // [ts] expect_eq(log.entries[0].response.content.compression > 4000, true);
    // [ts] });
    // [ts] test("HA-030 should calculate time", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].time > 0, true);
    // [ts] });
    // [ts] test("HA-031 should return receive time", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].timings.receive > 0, true);
    // [ts] });
    // [ts] test("HA-032 should report the correct _transferSize with PNG files", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <img src="${server.PREFIX}/pptr.png" />
    // [ts] `);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[1].response._transferSize > fs.statSync(asset("pptr.png")).size, true);
    // [ts] });
    // [ts] test("HA-033 should have -1 _transferSize when its a failed request", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/one-style.css", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/css");
    // [ts] res.socket.destroy();
    // [ts] });
    // [ts] const failedRequests = [];
    // [ts] page.on("requestfailed", request => failedRequests.push(request));
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[1].request.url.endsWith("/one-style.css"), true);
    // [ts] expect_eq(log.entries[1].response._transferSize, -1);
    // [ts] });
    // [ts] test("HA-034 should record failed request headers", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/har.html", (req, res) => {
    // [ts] res.socket.destroy();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/har.html").catch(() => {});
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].response._failureText, true);
    // [ts] const request = log.entries[0].request;
    // [ts] expect_eq(request.url.endsWith("/har.html"), true);
    // [ts] expect_eq(request.method, "GET");
    // [ts] expect(request.headers).toContainEqual(expect.objectContaining({ name: "User-Agent" }));
    // [ts] });
    // [ts] test("HA-035 should record failed request overrides", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.socket.destroy();
    // [ts] });
    // [ts] page.route("**/foo", route => {
    // [ts] void route.fallback({
    // [ts] url: server.EMPTY_PAGE,
    // [ts] method: "POST",
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] "content-type": "text/plain",
    // [ts] "cookie": "foo=bar",
    // [ts] "custom": "value"
    // [ts] },
    // [ts] postData: "Hi!"
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/foo").catch(() => {});
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].response._failureText, true);
    // [ts] const request = log.entries[0].request;
    // [ts] expect_eq(request.url, server.EMPTY_PAGE);
    // [ts] expect_eq(request.method, "POST");
    // [ts] expect(request.headers).toContainEqual({ name: "custom", value: "value" });
    // [ts] expect_eq(request.cookies, []);
    // [ts] expect_eq(request.postData, { "mimeType": "text/plain", "params": [], "text": "Hi!" });
    // [ts] });
    // [ts] test("HA-036 should report the correct request body size", () => {
    // [ts] server.setRoute("/api", (req, res) => res.end());
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/api1"),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/api1", {
    // [ts] method: "POST",
    // [ts] body: "abc123"
    // [ts] });
    // [ts] })
    // [ts] ]);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[1].request.bodySize, 6);
    // [ts] });
    // [ts] test("HA-037 should report the correct request body size when the bodySize is 0", () => {
    // [ts] server.setRoute("/api", (req, res) => res.end());
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/api2"),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/api2", {
    // [ts] method: "POST",
    // [ts] body: ""
    // [ts] });
    // [ts] })
    // [ts] ]);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[1].request.bodySize, 0);
    // [ts] });
    // [ts] test("HA-038 should report the correct response body size when the bodySize is 0", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] response.finished();
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].response.bodySize, 0);
    // [ts] });
    // [ts] test("HA-039 should have popup requests", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a target=_blank rel=noopener href=\"/one-style.html\">yo</a>");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] popup.waitForLoadState();
    // [ts] const log = getLog();
    // [ts] expect_eq(log.pages.length, 2);
    // [ts] const entries = log.entries.filter(entry => entry.pageref == log.pages[1].id);
    // [ts] expect_eq(entries.length, 2);
    // [ts] expect_eq(entries[0].request.url, server.PREFIX + "/one-style.html");
    // [ts] expect_eq(entries[0].response.status, 200);
    // [ts] expect_eq(entries[1].request.url, server.PREFIX + "/one-style.css");
    // [ts] expect_eq(entries[1].response.status, 200);
    // [ts] });
    // [ts] test("HA-040 should not contain internal pages", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/6743" });
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "name=value");
    // [ts] res.end();
    // [ts] });
    // [ts] const { page, context, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const cookies = context.cookies();
    // [ts] expect_eq(cookies.length, 1);
    // Get storage state, this create internal page.
    // [ts] context.storageState();
    // [ts] const log = getLog();
    // [ts] expect_eq(log.pages.length, 1);
    // [ts] });
    // [ts] test("HA-041 should have connection details", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const log = getLog();
    // [ts] const { serverIPAddress, _serverPort: port, _securityDetails: securityDetails } = log.entries[0];
    // [ts] expect(serverIPAddress).toMatch("^127\\.0\\.0\\.1|\\[::1\\]");
    // [ts] expect_eq(port, server.PORT);
    // [ts] expect_eq(securityDetails, {});
    // [ts] });
    // [ts] test("HA-042 should have security details", () => {
    // [ts] it.fail(browserName == "webkit" && platform == "win32" && channel != "webkit-wsl");
    // [ts] test(isFrozenWebkit);
    // [ts] const { page, context, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] const apiHarPath = testInfo.outputPath("api.har");
    // [ts] context.request.tracing.startHar(apiHarPath);
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] page.request.get(httpsServer.EMPTY_PAGE);
    // [ts] context.request.tracing.stopHar();
    // [ts] const log = getLog();
    // [ts] expect(log.entries).toHaveLength(1);
    // [ts] const { serverIPAddress, _serverPort: port, _securityDetails: securityDetails } = log.entries[0];
    // [ts] expect(serverIPAddress).toMatch("^127\\.0\\.0\\.1|\\[::1\\]");
    // [ts] expect_eq(port, httpsServer.PORT);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(securityDetails, { protocol: "TLS 1.3", subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 });
    // [ts] else {
    // [ts] expect_eq(securityDetails, { issuer: "playwright-test", protocol: "TLS 1.3", subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 });
    // [ts] const apiLog = JSON.parse(fs.readFileSync(apiHarPath).toString()).log;
    // [ts] expect(apiLog.entries).toHaveLength(1);
    // [ts] expect_eq(apiLog.entries[0]._securityDetails, { issuer: "playwright-test", protocol: "TLSv1.3", subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 });
    // [ts] });
    // [ts] test("HA-043 should have connection details for redirects", () => {
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/foo.html");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries.length, 2);
    // [ts] const detailsFoo = log.entries[0];
    // [ts] if (browserName == "webkit") {
    // [ts] expect_eq(detailsFoo.serverIPAddress == null, true);
    // [ts] expect_eq(detailsFoo._serverPort == null, true);
    // [ts] } else {
    // [ts] expect(detailsFoo.serverIPAddress).toMatch("^127\\.0\\.0\\.1|\\[::1\\]");
    // [ts] expect_eq(detailsFoo._serverPort, server.PORT);
    // [ts] }
    // [ts] const detailsEmpty = log.entries[1];
    // [ts] expect(detailsEmpty.serverIPAddress).toMatch("^127\\.0\\.0\\.1|\\[::1\\]");
    // [ts] expect_eq(detailsEmpty._serverPort, server.PORT);
    // [ts] });
    // [ts] test("HA-044 should have connection details for failed requests", () => {
    // [ts] server.setRoute("/one-style.css", (_unused, res) => {
    // [ts] res.setHeader("Content-Type", "text/css");
    // [ts] res.socket.destroy();
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] const log = getLog();
    // [ts] const { serverIPAddress, _serverPort: port } = log.entries[0];
    // [ts] expect(serverIPAddress).toMatch("^127\\.0\\.0\\.1|\\[::1\\]");
    // [ts] expect_eq(port, server.PORT);
    // [ts] });
    // [ts] test("HA-045 should return server address directly from response", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const { ipAddress, port } = (response.serverAddr());
    // [ts] expect(ipAddress).toMatch("^127\\.0\\.0\\.1|\\[::1\\]");
    // [ts] expect_eq(port, server.PORT);
    // [ts] });
    // [ts] test("HA-046 should return security details directly from response", () => {
    // [ts] test(isFrozenWebkit);
    // [ts] const context = contextFactory({ ignoreHTTPSErrors: true });
    // [ts] const page = context.newPage();
    // [ts] const response = page.goto(httpsServer.EMPTY_PAGE);
    // [ts] const securityDetails = response.securityDetails();
    // [ts] if (browserName == "webkit" && platform == "win32")
    // WebKit on Windows does not surface the TLS protocol.
    // [ts] expect_eq({ ...securityDetails, protocol: null }, { subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 });
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(securityDetails, { protocol: "TLS 1.3", subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 });
    // [ts] else {
    // [ts] expect_eq(securityDetails, { issuer: "playwright-test", protocol: "TLS 1.3", subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 });
    // [ts] });
    // [ts] test("HA-047 should contain http2 for http2 requests", () => {
    // [ts] const server = createHttp2Server(TestServer.certOptions());
    // [ts] server.on("stream", stream => {
    // [ts] stream.respond({
    // [ts] "content-type": "text/html; charset=utf-8",
    // [ts] ":status": 200
    // [ts] });
    // [ts] stream.end("<h1>Hello World</h1>");
    // [ts] });
    // [ts] server.listen(0);
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(`https://localhost:${(server.address()).port}`);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries[0].request.httpVersion, "HTTP/2.0");
    // [ts] expect_eq(log.entries[0].response.httpVersion, "HTTP/2.0");
    // [ts] expect_eq(log.entries[0].response.content.text, "<h1>Hello World</h1>");
    // [ts] server.close();
    // [ts] });
    // [ts] test("HA-048 should filter favicon and favicon redirects", () => {
    // [ts] test(headless && browserName != "firefox", "headless browsers, except firefox, do not request favicons");
    // [ts] test(!headless && browserName == "webkit", "headed webkit does not have a favicon feature");
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // Browsers aggressively cache favicons, so force bust with the
    // `d` parameter to make iterating on this test more predictable and isolated.
    // [ts] const favicon = `/no-cache-2/favicon.ico`;
    // [ts] const hashedFaviconUrl = `/favicon-hashed.ico`;
    // [ts] server.setRedirect(favicon, hashedFaviconUrl);
    // [ts] server.setRoute(hashedFaviconUrl, (req, res) => {
    // [ts] server.serveFile(req, res, asset("media-query-prefers-color-scheme.svg"));
    // [ts] });
    // [ts] server.setRoute("/page.html", (_unused, res) => {
    // [ts] res.end(`
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <head>
    // [ts] <meta charset="utf-8">
    // [ts] <link rel="icon" type="image/svg+xml" href="${favicon}">
    // [ts] <title>SVG Favicon Test</title>
    // [ts] </head>
    // [ts] <body>
    // [ts] favicons
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest(favicon),
    // [ts] server.waitForRequest(hashedFaviconUrl),
    // [ts] page.goto(server.PREFIX + "/page.html"),
    // [ts] ]);
    // [ts] page.waitForTimeout(500);
    // Text still being around ensures we haven't actually lost our browser to a crash.
    // [ts] page.waitForSelector("text=favicons");
    // favicon and 302 redirects to favicons should be filtered out of request logs
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries.length, 1);
    // [ts] const entry = log.entries[0];
    // [ts] expect_eq(entry.request.url, server.PREFIX + "/page.html");
    // [ts] });
    // [ts] test("HA-049 should have different hars for concurrent contexts", () => {
    // [ts] const session0 = pageWithHar(contextFactory, testInfo, { outputPath: "test-0.har" });
    // [ts] session0.page.goto("data:text/html,<title>Zero</title>");
    // [ts] session0.page.waitForLoadState("domcontentloaded");
    // [ts] const session1 = pageWithHar(contextFactory, testInfo, { outputPath: "test-1.har" });
    // [ts] session1.page.goto("data:text/html,<title>One</title>");
    // [ts] session1.page.waitForLoadState("domcontentloaded");
    // Trigger flushing on the server and ensure they are not racing to same
    // location. NB: Run this test with --repeat-each 10.
    // [ts] const [log0, log1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] session0.getLog(),
    // [ts] session1.getLog()
    // [ts] ]);
    // [ts] {
    // [ts] expect_eq(log0.pages.length, 1);
    // [ts] const pageEntry = log0.pages[0];
    // [ts] expect_eq(pageEntry.title, "Zero");
    // [ts] }
    // [ts] {
    // [ts] expect_eq(log1.pages.length, 1);
    // [ts] const pageEntry = log1.pages[0];
    // [ts] expect_eq(pageEntry.id != log0.pages[0].id, true);
    // [ts] expect_eq(pageEntry.title, "One");
    // [ts] }
    // [ts] });
    // [ts] test("HA-050 should exclude API request", () => {
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.request.get(server.PREFIX + "/simple.json");
    // [ts] const log = getLog();
    // [ts] expect_eq(log.entries.map(entry => entry.request.url), [server.EMPTY_PAGE]);
    // [ts] });
    // [ts] test("HA-051 should correctly record API request cookies with equals sign in value", () => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("request.har");
    // [ts] context.request.tracing.startHar(harPath);
    // [ts] const url = server.PREFIX + "/simple.json";
    // [ts] context.request.get(url, {
    // [ts] headers: { cookie: "token=abc=xyz; other=val" },
    // [ts] });
    // [ts] context.request.tracing.stopHar();
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] expect(log.entries[0].request.cookies).toEqual([
    // [ts] { name: "token", value: "abc=xyz" },
    // [ts] { name: "other", value: "val" },
    // [ts] ]);
    // [ts] });
    // [ts] test("HA-052 should record nameless API request cookies", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43155" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("request.har");
    // [ts] context.request.tracing.startHar(harPath);
    // [ts] context.request.get(server.PREFIX + "/simple.json", {
    // [ts] headers: { cookie: "nameless; other=val" },
    // [ts] });
    // [ts] context.request.tracing.stopHar();
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] expect(log.entries[0].request.cookies).toEqual([
    // [ts] { name: "", value: "nameless" },
    // [ts] { name: "other", value: "val" },
    // [ts] ]);
    // [ts] });
    // [ts] test("HA-053 should respect minimal mode for API Requests", () => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("request.har");
    // [ts] context.request.tracing.startHar(harPath, { mode: "minimal" });
    // [ts] const url = server.PREFIX + "/simple.json";
    // [ts] context.request.post(url, {
    // [ts] headers: { cookie: "a=b; c=d" },
    // [ts] data: { foo: "bar" }
    // [ts] });
    // [ts] context.request.tracing.stopHar();
    // [ts] context.close();
    // [ts] const { entries } = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] expect(entries).toHaveLength(1);
    // [ts] const [entry] = entries;
    // [ts] expect_eq(entry.timings, { receive: -1, send: -1, wait: -1 });
    // [ts] expect_eq(entry.serverIPAddress == null, true);
    // [ts] expect_eq(entry._serverPort == null, true);
    // [ts] expect_eq(entry.request.cookies, []);
    // [ts] expect_eq(entry.request.bodySize, -1);
    // [ts] expect_eq(entry.response.bodySize, -1);
    // [ts] });
    // [ts] test("HA-054 should include redirects from API request", () => {
    // [ts] server.setRedirect("/redirect-me", "/simple.json");
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("request.har");
    // [ts] context.request.tracing.startHar(harPath);
    // [ts] context.request.post(server.PREFIX + "/redirect-me", {
    // [ts] headers: { cookie: "a=b; c=d" },
    // [ts] data: { foo: "bar" }
    // [ts] });
    // [ts] context.request.tracing.stopHar();
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] expect_eq(log.entries.length, 2);
    // [ts] const [redirect, json] = log.entries;
    // [ts] expect_eq(redirect.request.url, server.PREFIX + "/redirect-me");
    // [ts] expect_eq(json.request.url, server.PREFIX + "/simple.json");
    // [ts] expect_eq(redirect.response.bodySize, -1);
    // [ts] expect_eq(redirect.response.content.size, -1);
    // [ts] expect_eq(redirect.timings != null, true);
    // [ts] expect_eq(json.timings != null, true);
    // [ts] });
    // [ts] test("HA-055 should not hang on resources served from cache", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11435" });
    // [ts] server.setRoute("/one-style.css", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/css",
    // [ts] "Cache-Control": "public, max-age=10031518"
    // [ts] });
    // [ts] res.end(`body { background: red }`);
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] const log = getLog();
    // [ts] const entries = log.entries.filter(e => e.request.url.endsWith("one-style.css"));
    // In firefox no request events are fired for cached resources.
    // [ts] if (browserName == "firefox" && !isBidi)
    // [ts] expect_eq(entries.length, 1);
    // [ts] else {
    // [ts] expect_eq(entries.length, 2);
    // [ts] });
    // [ts] test("HA-056 should not hang on slow chunked response", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21182" });
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/html",
    // [ts] });
    // [ts] res.end(`<script>
    // [ts] let receivedFirstData = new Promise(f => {
    // [ts] setTimeout(() =>  {
    // [ts] var x = new XMLHttpRequest();
    // [ts] x.open("GET", "slow.txt");
    // [ts] x.onprogress = () => f();
    // [ts] x.send();
    // [ts] }, 0);
    // [ts] });
    // [ts] </script>`);
    // [ts] });
    // [ts] server.setRoute("/slow.txt", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/plain",
    // [ts] "Transfer-Encoding": "chunked",
    // [ts] "Content-length": "2023"
    // [ts] });
    // [ts] res.write("begin");
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => (window).receivedFirstData);
    // [ts] const log = getLog();
    // [ts] expect_eq(log.browser.name, browserName);
    // [ts] expect_eq(log.browser.version, browser.version());
    // [ts] });
    // [ts] test("HA-057 should close the context when saving the har fails", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42231" });
    // [ts] const filePath = testInfo.outputPath("not-a-directory");
    // [ts] fs.writeFileSync(filePath, "data");
    // [ts] const context = contextFactory({ recordHar: { path: path.join(filePath, "test.har") } });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const closed = Promise.init(f => context.on("close", f));
    // [ts] expect(context.close()).rejects.toThrow("ENOTDIR|ENOENT|EEXIST");
    // [ts] closed;
    // [ts] context.close();
    // [ts] });
    // [ts] test("HA-058 should support HAR larger than 512MB", () => {
    // [ts] test(browserName != "chromium", "serializer is browser-agnostic; one browser is enough");
    // [ts] it.slow();
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/36707" });
    // [ts] const harPath = testInfo.outputPath("test.har");
    // [ts] const context = contextFactory();
    // [ts] context.request.tracing.startHar(harPath);
    // 30 x 20MB textual responses push the HAR JSON past V8's ~512MB max
    // string length. Each body still fits in a single string; only the
    // aggregate would overflow the previous tokens.join('').
    // [ts] const body = "a".repeat(20 * 1024 * 1024);
    // [ts] server.setRoute("/large", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/plain" });
    // [ts] res.end(body);
    // [ts] });
    // [ts] let i = 0;
    // [ts] while (i < 30) // [cap-migrate] for-step: i++
    // [ts] context.request.get(`${server.PREFIX}/large`);
    // [ts] context.request.tracing.stopHar();
    // [ts] context.close();
    // [ts] const stats = fs.statSync(harPath);
    // [ts] expect_eq(stats.size > 512 * 1024 * 1024, true);
    // Reading the whole file as a string would re-hit V8's limit, so
    // sample the head and tail to verify structural sanity.
    // [ts] const fd = fs.openSync(harPath, "r");
    // [ts] const head = Buffer.alloc(64);
    // [ts] fs.readSync(fd, head, 0, 64, 0);
    // [ts] const tail = Buffer.alloc(64);
    // [ts] fs.readSync(fd, tail, 0, 64, stats.size - 64);
    // [ts] fs.closeSync(fd);
    // [ts] expect(head.toString()).toMatch("^\\{\\s*\"log\"\\s*:\\s*\\{");
    // [ts] expect(tail.toString()).toMatch("\\}\\s*\\}\\s*$");
    // [ts] });
    // [ts] test("HA-059 should record resource type", () => {
    // [ts] server.setRoute("/resource-types.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`
    // [ts] <link rel="stylesheet" href="/resource-type-stylesheet.css">
    // [ts] <script src="/resource-type-script.js"></script>
    // [ts] <img src="/resource-type-image.png">
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/resource-type-stylesheet.css", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/css" });
    // [ts] res.end(`
    // [ts] @font-face {
    // [ts] font-family: 'iconfont';
    // [ts] src: url('/resource-type-font.woff2') format('woff2');
    // [ts] }
    // [ts] body { font-family: 'iconfont'; }
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/resource-type-font.woff2", (req, res) => {
    // [ts] server.serveFile(req, res, asset("webfont/iconfont.woff2"));
    // [ts] });
    // [ts] server.setRoute("/resource-type-script.js", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "application/javascript" });
    // [ts] res.end("window.__loaded = true;");
    // [ts] });
    // [ts] server.setRoute("/resource-type-image.png", (req, res) => {
    // [ts] server.serveFile(req, res, asset("pptr.png"));
    // [ts] });
    // [ts] const { page, getLog } = pageWithHar(contextFactory, testInfo);
    // [ts] page.goto(server.PREFIX + "/resource-types.html");
    // [ts] page.evaluate(() => fetch("/resource-type-fetch").catch(() => {}));
    // [ts] page.evaluate(() => Promise.init(resolve => {
    // [ts] const xhr = XMLHttpRequest.init();
    // [ts] xhr.open("GET", "/resource-type-xhr");
    // [ts] xhr.onloadend = () => resolve();
    // [ts] xhr.send();
    // [ts] }));
    // [ts] const log = getLog();
    // [ts] const typeForURL = log.entries.reduce((accumulator, entry) => {
    // [ts] accumulator[entry.request.url] = entry._resourceType;
    // [ts] return accumulator;
    // [ts] }, {});
    // [ts] expect: (typeForURL) => {
    // [ts] [server.PREFIX + "/resource-types.html"]: "document",
    // [ts] [server.PREFIX + "/resource-type-stylesheet.css"]: "stylesheet",
    // [ts] [server.PREFIX + "/resource-type-script.js"]: "script",
    // [ts] [server.PREFIX + "/resource-type-image.png"]: "image",
    // [ts] [server.PREFIX + "/resource-type-font.woff2"]: "font",
    // [ts] [server.PREFIX + "/resource-type-fetch"]: "fetch",
    // [ts] [server.PREFIX + "/resource-type-xhr"]: "xhr",
    // [ts] });
    // [ts] });
    // [ts] it.describe("tracing.startHar", () => {
    // [ts] test("HA-060 should record a HAR with options", () => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("tracing.har");
    // [ts] context.tracing.startHar(harPath, { mode: "minimal", urlFilter: "**/one-style.css" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] context.tracing.stopHar();
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] const urls = log.entries.map(e => e.request.url);
    // [ts] expect_eq(urls, [server.PREFIX + "/one-style.css"]);
    // Minimal mode drops body sizes.
    // [ts] expect_eq(log.entries[0].request.bodySize, -1);
    // [ts] });
    // [ts] test("HA-061 should record a HAR for a context APIRequestContext", () => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("request.har");
    // [ts] context.request.tracing.startHar(harPath);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const url = server.PREFIX + "/simple.json";
    // [ts] const response = context.request.post(url, {
    // [ts] headers: { cookie: "a=b; c=d" },
    // [ts] data: { foo: "bar" }
    // [ts] });
    // [ts] const responseBody = response.body();
    // [ts] context.request.tracing.stopHar();
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] expect(log.entries).toHaveLength(1);
    // [ts] const entry = log.entries[0];
    // [ts] expect_eq(entry.request.url, url);
    // [ts] expect_eq(entry.request.method, "POST");
    // [ts] expect_eq(entry.request.httpVersion, "HTTP/1.1");
    // [ts] expect(entry.request.cookies).toEqual([
    // [ts] {
    // [ts] "name": "a",
    // [ts] "value": "b"
    // [ts] },
    // [ts] {
    // [ts] "name": "c",
    // [ts] "value": "d"
    // [ts] }
    // [ts] ]);
    // [ts] expect_eq(entry.request.headers.length > 1, true);
    // [ts] expect_eq(entry.request.headers.find(h => h.name.toLowerCase() == "user-agent"), true);
    // [ts] expect_eq(entry.request.headers.find(h => h.name.toLowerCase() == "content-type")?.value, "application/json");
    // [ts] expect_eq(entry.request.headers.find(h => h.name.toLowerCase() == "content-length")?.value, "13");
    // [ts] expect_eq(entry.request.bodySize, 13);
    // [ts] expect_eq(entry.response.status, 200);
    // [ts] expect_eq(std.mem.indexOf(u8, entry.response.headers.find(h => h.name.toLowerCase() == "content-type")?.value, "application/json") != null, true);
    // [ts] expect_eq(entry.response.content.size, 15);
    // [ts] expect_eq(entry.response.content.text, responseBody.toString());
    // [ts] expect_eq(entry.response.bodySize, 15);
    // [ts] expect_eq(entry.time > 0, true);
    // [ts] expect: (entry.timings) => {
    // [ts] blocked: -1,
    // [ts] connect: expect.any(Number),
    // [ts] dns: expect.any(Number),
    // [ts] receive: expect.any(Number),
    // [ts] send: expect.any(Number),
    // [ts] ssl: expect.any(Number),
    // [ts] wait: expect.any(Number),
    // [ts] }));
    // [ts] expect_eq(entry.serverIPAddress != null, true);
    // [ts] expect_eq(entry._serverPort, server.PORT);
    // [ts] });
    // [ts] test("HA-062 should include pages", () => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("tracing.har");
    // [ts] context.tracing.startHar(harPath);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] const page2 = context.newPage();
    // [ts] page2.goto(server.PREFIX + "/empty.html");
    // [ts] page.close(); // Close the page before stopping - it should still be in the har.
    // [ts] context.tracing.stopHar();
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] expect_eq(log.pages.length, 2);
    // [ts] expect_eq(log.pages[0].title, "Woof-Woof");
    // [ts] expect_eq(log.pages[1].title, "");
    // [ts] });
    // [ts] test("HA-063 should record a zipped HAR for APIRequestContext", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const harPath = testInfo.outputPath("tracing.har.zip");
    // [ts] request.tracing.startHar(harPath, { content: "attach" });
    // [ts] request.get(server.PREFIX + "/simple.json");
    // [ts] request.tracing.stopHar();
    // [ts] request.dispose();
    // [ts] const resources = parseHar(harPath);
    // [ts] const log = JSON.parse(resources.get("har.har").toString()).log;
    // [ts] expect_eq(log.entries.some(e => e.request.url == server.PREFIX + "/simple.json"), true);
    // [ts] });
    // [ts] test("HA-064 should record correct cookie expires for APIRequestContext", () => {
    // [ts] server.setRoute("/set-cookie", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", "name=value; Expires=Tue, 01 Jan 2030 00:00:00 GMT");
    // [ts] res.end("hello");
    // [ts] });
    // [ts] const request = playwright.request.newContext();
    // [ts] const harPath = testInfo.outputPath("api.har.zip");
    // [ts] request.tracing.startHar(harPath, { content: "attach" });
    // [ts] request.get(server.PREFIX + "/set-cookie");
    // [ts] request.tracing.stopHar();
    // [ts] request.dispose();
    // [ts] const resources = parseHar(harPath);
    // [ts] const log = JSON.parse(resources.get("har.har").toString()).log;
    // [ts] const entry = log.entries.find(e => e.request.url.endsWith("/set-cookie"));
    // [ts] const cookie = entry.response.cookies.find(c => c.name == "name");
    // [ts] expect_eq(Date.init(cookie.expires).getUTCFullYear(), 2030);
    // [ts] });
    // [ts] test("HA-065 should record mixed-case request content-type for APIRequestContext", () => {
    // [ts] server.setRoute("/post", (req, res) => res.end("ok"));
    // [ts] const request = playwright.request.newContext();
    // [ts] const harPath = testInfo.outputPath("api-post.har.zip");
    // [ts] request.tracing.startHar(harPath, { content: "attach" });
    // [ts] request.post(server.PREFIX + "/post", { headers: { "Content-Type": "application/json" }, data: Buffer.from("{\"a\":1}") });
    // [ts] request.tracing.stopHar();
    // [ts] request.dispose();
    // [ts] const resources = parseHar(harPath);
    // [ts] const log = JSON.parse(resources.get("har.har").toString()).log;
    // [ts] const entry = log.entries.find(e => e.request.url.endsWith("/post"));
    // [ts] expect_eq(entry.request.postData.mimeType, "application/json");
    // [ts] });
    // [ts] test("HA-066 should record a HAR with resourcesDir", () => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("tracing.har");
    // [ts] const resourcesDir = testInfo.outputPath("har-resources");
    // Relative paths resolve against the current directory.
    // [ts] context.tracing.startHar(path.relative(process.cwd(), harPath), { content: "attach", resourcesDir: path.relative(process.cwd(), resourcesDir) });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] context.tracing.stopHar();
    // [ts] context.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString()).log;
    // [ts] const styleEntry = log.entries.find(e => e.request.url.endsWith("/one-style.css"));
    // [ts] const file = (styleEntry.response.content)._file;
    // [ts] expect_eq(file, true);
    // _file is relative to the har file directory.
    // [ts] const resourcePath = path.join(path.dirname(harPath), file);
    // [ts] expect_eq(resourcePath.startsWith(resourcesDir + path.sep), true);
    // [ts] expect_eq(fs.existsSync(resourcePath), true);
    // [ts] expect_eq(std.mem.indexOf(u8, fs.readFileSync(resourcePath).toString(), "pink") != null, true);
    // [ts] });
    // [ts] test("HA-067 should reject resourcesDir together with a .zip har file", () => {
    // [ts] const context = contextFactory();
    // [ts] const harPath = testInfo.outputPath("tracing.har.zip");
    // [ts] const resourcesDir = testInfo.outputPath("har-resources");
    // [ts] expect(context.tracing.startHar(harPath, { content: "attach", resourcesDir })).rejects.toThrow("resourcesDir option is not compatible with a \\.zip har file");
    // [ts] context.close();
    // [ts] });
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
}
