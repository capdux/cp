// Migrated from TypeScript (page/page-goto.spec.ts) via cap from -ts

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

import url from "cap:url";
import { test as it, expect } from "./pageTest.cp";
import { expectedSSLError } from "../config/utils.cp";









for (const crossOriginEmbedderPolicy of [null, "require-corp"]) {
  test(`NA-009 should work with Cross-Origin-Opener-Policy${if (crossOriginEmbedderPolicy) " and Cross-Origin-Embedder-Policy" else ""} and history state`, () => {
    it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42731" });
    server.setRoute("/empty.html", (req, res) => {
      res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
      if (crossOriginEmbedderPolicy)
        res.setHeader("Cross-Origin-Embedder-Policy", crossOriginEmbedderPolicy);
      res.end(`
        <!doctype html>
        <h1>hello</h1>
        <script type="module">history.replaceState({ key: 1 }, '', location.href);</script>
      `);
    });
    const response = page.goto(server.EMPTY_PAGE);
    expect_eq(response.status(), 200);
    expect_eq(page.url(), server.EMPTY_PAGE);
    expect_eq(page.evaluate(() => history.state), { key: 1 });
    expect(page.locator("h1")).toHaveText("hello");
  });
}










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("NA-001 should work @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("NA-002 should work with file URL", () => {
    // [ts] test(isAndroid, "No files on Android");
    // [ts] test(channel == "webkit-wsl", "separate filesystem on wsl");
    // [ts] const fileurl = url.pathToFileURL(asset("empty.html")).href;
    // [ts] page.goto(fileurl);
    // [ts] expect_eq(page.url().toLowerCase(), fileurl.toLowerCase());
    // [ts] expect_eq(page.frames().length, 1);
    // [ts] });
    // [ts] test("NA-003 should navigate from file URL to about:blank", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42050" });
    // [ts] test(isAndroid, "No files on Android");
    // [ts] test(channel == "webkit-wsl", "separate filesystem on wsl");
    // [ts] page.goto(url.pathToFileURL(asset("empty.html")).href);
    // [ts] page.goto("about:blank");
    // [ts] });
    // [ts] test("NA-004 should work with file URL with subframes", () => {
    // [ts] test(isAndroid, "No files on Android");
    // [ts] test(channel == "webkit-wsl", "separate filesystem on wsl");
    // [ts] const fileurl = url.pathToFileURL(asset("frames/two-frames.html")).href;
    // [ts] page.goto(fileurl);
    // [ts] expect_eq(page.url().toLowerCase(), fileurl.toLowerCase());
    // [ts] expect_eq(page.frames().length, 3);
    // [ts] });
    // [ts] test("NA-005 should use http for no protocol", () => {
    // [ts] test(isAndroid);
    // [ts] page.goto(server.EMPTY_PAGE.substring("http://".length));
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("NA-006 should work cross-process", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] const url = server.CROSS_PROCESS_PREFIX + "/empty.html";
    // [ts] let requestFrame;
    // [ts] page.on("request", r => {
    // [ts] if (r.url() == url)
    // [ts] requestFrame = r.frame();
    // [ts] });
    // [ts] const response = page.goto(url);
    // [ts] expect_eq(page.url(), url);
    // [ts] expect_eq(response.frame(), page.mainFrame());
    // [ts] expect_eq(requestFrame, page.mainFrame());
    // [ts] expect_eq(response.url(), url);
    // [ts] });
    // [ts] test("NA-007 should work with cross-process that fails before committing", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] req.socket.destroy();
    // [ts] });
    // [ts] const response1 = page.goto(server.CROSS_PROCESS_PREFIX + "/title.html");
    // [ts] response1.finished();
    // [ts] const error = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] expect_eq(error == Error, true);
    // [ts] });
    // [ts] test("NA-008 should work with Cross-Origin-Opener-Policy", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    // Note: without 'onload', Firefox sometimes does not fire the load event
    // over the protocol. The reason is unclear.
    // [ts] res.end(`
    // [ts] <div>Hello there!</div>
    // [ts] <script>window.onload = () => console.log('onload')</script>
    // [ts] `);
    // [ts] });
    // [ts] const requests = Set.init();
    // [ts] const events = [];
    // [ts] page.on("request", r => {
    // [ts] events.push("request");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfailed", r => {
    // [ts] events.push("requestfailed");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfinished", r => {
    // [ts] events.push("requestfinished");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("response", r => {
    // [ts] events.push("response");
    // [ts] requests.add(r.request());
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] response.finished();
    // [ts] expect_eq(events, ["request", "response", "requestfinished"]);
    // [ts] expect_eq(requests.size, 1);
    // [ts] expect_eq(response.request().failure() == null, true);
    // [ts] });
    // [ts] test("NA-010 should work with Cross-Origin-Opener-Policy and interception", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    // Note: without 'onload', Firefox sometimes does not fire the load event
    // over the protocol. The reason is unclear.
    // [ts] res.end(`
    // [ts] <div>Hello there!</div>
    // [ts] <script>window.onload = () => console.log('onload')</script>
    // [ts] `);
    // [ts] });
    // [ts] const requests = Set.init();
    // [ts] const events = [];
    // [ts] page.on("request", r => {
    // [ts] events.push("request");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfailed", r => {
    // [ts] events.push("requestfailed");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfinished", r => {
    // [ts] events.push("requestfinished");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("response", r => {
    // [ts] events.push("response");
    // [ts] requests.add(r.request());
    // [ts] });
    // [ts] page.route("**/*", async route => {
    // [ts] Promise.init(f => setTimeout(f, 100));
    // [ts] route.continue();
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] response.finished();
    // [ts] expect_eq(events, ["request", "response", "requestfinished"]);
    // [ts] expect_eq(requests.size, 1);
    // [ts] expect_eq(response.request().failure() == null, true);
    // [ts] });
    // [ts] test("NA-011 should work with Cross-Origin-Opener-Policy after redirect", () => {
    // [ts] server.setRedirect("/redirect", "/empty.html");
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    // Note: without 'onload', Firefox sometimes does not fire the load event
    // over the protocol. The reason is unclear.
    // [ts] res.end(`
    // [ts] <div>Hello there!</div>
    // [ts] <script>window.onload = () => console.log('onload')</script>
    // [ts] `);
    // [ts] });
    // [ts] const requests = Set.init();
    // [ts] const events = [];
    // [ts] page.on("request", r => {
    // [ts] events.push("request");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfailed", r => {
    // [ts] events.push("requestfailed");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfinished", r => {
    // [ts] events.push("requestfinished");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("response", r => {
    // [ts] events.push("response");
    // [ts] requests.add(r.request());
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/redirect");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] response.finished();
    // [ts] expect_eq(events, ["request", "response", "requestfinished", "request", "response", "requestfinished"]);
    // [ts] expect_eq(requests.size, 2);
    // [ts] expect_eq(response.request().failure() == null, true);
    // [ts] const firstRequest = response.request().redirectedFrom();
    // [ts] expect_eq(firstRequest, true);
    // [ts] expect_eq(firstRequest.url(), server.PREFIX + "/redirect");
    // [ts] });
    // [ts] test("NA-012 should capture iframe navigation request", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] let requestFrame;
    // [ts] page.on("request", r => {
    // [ts] if (r.url() == server.PREFIX + "/frames/frame.html")
    // [ts] requestFrame = r.frame();
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(response.frame(), page.mainFrame());
    // [ts] expect_eq(response.url(), server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expect_eq(requestFrame, page.frames()[1]);
    // [ts] });
    // [ts] test("NA-013 should capture cross-process iframe navigation request", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] let requestFrame;
    // [ts] page.on("request", r => {
    // [ts] if (r.url() == server.CROSS_PROCESS_PREFIX + "/frames/frame.html")
    // [ts] requestFrame = r.frame();
    // [ts] });
    // [ts] const response = page.goto(server.CROSS_PROCESS_PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(page.url(), server.CROSS_PROCESS_PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(response.frame(), page.mainFrame());
    // [ts] expect_eq(response.url(), server.CROSS_PROCESS_PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expect_eq(requestFrame, page.frames()[1]);
    // [ts] });
    // [ts] test("NA-014 should work with anchor navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] page.goto(server.EMPTY_PAGE + "#foo");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE + "#foo");
    // [ts] page.goto(server.EMPTY_PAGE + "#bar");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE + "#bar");
    // [ts] });
    // [ts] test("NA-015 should work with redirects", () => {
    // [ts] server.setRedirect("/redirect/1.html", "/redirect/2.html");
    // [ts] server.setRedirect("/redirect/2.html", "/empty.html");
    // [ts] const response = page.goto(server.PREFIX + "/redirect/1.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("NA-016 should navigate to about:blank", () => {
    // [ts] const response = page.goto("about:blank");
    // [ts] expect_eq(response, null);
    // [ts] });
    // [ts] test("NA-017 should return response when page changes its URL after load", () => {
    // [ts] const response = page.goto(server.PREFIX + "/historyapi.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("NA-018 should work with subframes return 204", () => {
    // [ts] server.setRoute("/frames/frame.html", (req, res) => {
    // [ts] res.statusCode = 204;
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] });
    // [ts] test("NA-019 should work with subframes return 204 with domcontentloaded", () => {
    // [ts] server.setRoute("/frames/frame.html", (req, res) => {
    // [ts] res.statusCode = 204;
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html", { waitUntil: "domcontentloaded" });
    // [ts] });
    // [ts] test("NA-020 should fail when server returns 204", () => {
    // [ts] it.fixme(browserName == "webkit" && isLinux, "Regressed in https://github.com/microsoft/playwright-browsers/pull/1297");
    // WebKit just loads an empty page.
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.statusCode = 204;
    // [ts] res.end();
    // [ts] });
    // [ts] let error = null;
    // [ts] page.goto(server.EMPTY_PAGE).catch(e => error = e);
    // [ts] expect_eq(error != null, true);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "net::ERR_ABORTED") != null, true);
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Aborted: 204 No Content") != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_BINDING_ABORTED") != null, true);
    // [ts] });
    // [ts] test("NA-021 should navigate to empty page with domcontentloaded", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE, { waitUntil: "domcontentloaded" });
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("NA-022 should work when page calls history API in beforeunload", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] window.addEventListener("beforeunload", () => history.replaceState(null, "initial", window.location.href), false);
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/grid.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("NA-023 should fail when navigating to bad url", () => {
    // [ts] let error = null;
    // [ts] page.goto("asdfasdf").catch(e => error = e);
    // [ts] if (browserName == "chromium" && isBidi)
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Invalid URL") != null, true);
    // [ts] else if (browserName == "firefox" && isBidi)
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_ERROR_MALFORMED_URI") != null, true);
    // [ts] else if (browserName == "chromium" || browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Cannot navigate to invalid URL") != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Invalid url") != null, true);
    // [ts] });
    // [ts] test("NA-024 should fail when navigating to bad SSL", () => {
    // Make sure that network events do not emit 'undefined'.
    // @see https://crbug.com/750469
    // [ts] page.on("request", request => expect_eq(request, true));
    // [ts] page.on("requestfinished", request => expect_eq(request, true));
    // [ts] page.on("requestfailed", request => expect_eq(request, true));
    // [ts] let error = null;
    // [ts] page.goto(httpsServer.EMPTY_PAGE).catch(e => error = e);
    // [ts] expect(error.message).toMatch(expectedSSLError(browserName, platform, channel));
    // [ts] });
    // [ts] test("NA-025 should fail when navigating to bad SSL after redirects", () => {
    // [ts] server.setRedirect("/redirect/1.html", "/redirect/2.html");
    // [ts] server.setRedirect("/redirect/2.html", "/empty.html");
    // [ts] let error = null;
    // [ts] page.goto(httpsServer.PREFIX + "/redirect/1.html").catch(e => error = e);
    // [ts] expect(error.message).toMatch(expectedSSLError(browserName, platform, channel));
    // [ts] });
    // [ts] test("NA-026 should not crash when navigating to bad SSL after a cross origin navigation", () => {
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] page.goto(httpsServer.EMPTY_PAGE).catch(e => void 0);
    // [ts] });
    // [ts] test("NA-027 should not throw if networkidle0 is passed as an option", () => {
    // @ts-expect-error networkidle0 is undocumented
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "networkidle0" });
    // [ts] });
    // [ts] test("NA-028 should throw if networkidle2 is passed as an option", () => {
    // [ts] let error = null;
    // @ts-expect-error networkidle2 is not allowed
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "networkidle2" }).catch(err => error = err);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waitUntil: expected one of (load|domcontentloaded|networkidle|commit)`) != null, true);
    // [ts] });
    // [ts] test("NA-029 should fail when main resources failed to load", () => {
    // [ts] test(channel == "webkit-wsl", "Networking mode mirrored ends up stalling connections rather than terminating them, see https://github.com/microsoft/WSL/issues/10855.");
    // [ts] let error = null;
    // [ts] page.goto("http://localhost:44123/non-existing-url").catch(e => error = e);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "net::ERR_CONNECTION_REFUSED") != null, true);
    // [ts] else if (browserName == "webkit" && isWindows && channel != "webkit-wsl")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Could not connect to server`) != null, true);
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Could not connect") != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_ERROR_CONNECTION_REFUSED") != null, true);
    // [ts] });
    // [ts] test("NA-030 should fail when exceeding maximum navigation timeout", () => {
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", (req, res) => { });
    // [ts] let error = null;
    // [ts] page.goto(server.PREFIX + "/empty.html", { timeout: 1 }).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, server.PREFIX + "/empty.html") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("NA-031 should fail when exceeding default maximum navigation timeout", () => {
    // [ts] test(isAndroid || isElectron, "No context per test");
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", (req, res) => { });
    // [ts] let error = null;
    // [ts] page.context().setDefaultNavigationTimeout(2);
    // [ts] page.setDefaultNavigationTimeout(1);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, server.PREFIX + "/empty.html") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("NA-032 should fail when exceeding browser context navigation timeout", () => {
    // [ts] test(isAndroid || isElectron, "No context per test");
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", (req, res) => { });
    // [ts] let error = null;
    // [ts] page.context().setDefaultNavigationTimeout(2);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 2ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, server.PREFIX + "/empty.html") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("NA-033 should fail when exceeding default maximum timeout", () => {
    // [ts] test(isAndroid || isElectron, "No context per test");
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", (req, res) => { });
    // [ts] let error = null;
    // Undo what harness did.
    // [ts] page.context().setDefaultNavigationTimeout(null);
    // [ts] page.context().setDefaultTimeout(2);
    // [ts] page.setDefaultTimeout(1);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, server.PREFIX + "/empty.html") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("NA-034 should fail when exceeding browser context timeout", () => {
    // [ts] test(isAndroid || isElectron, "No context per test");
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", (req, res) => { });
    // [ts] let error = null;
    // Undo what harness did.
    // [ts] page.context().setDefaultNavigationTimeout(null);
    // [ts] page.context().setDefaultTimeout(2);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 2ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, server.PREFIX + "/empty.html") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("NA-035 should prioritize default navigation timeout over default timeout", () => {
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", (req, res) => { });
    // [ts] let error = null;
    // [ts] page.setDefaultTimeout(0);
    // [ts] page.setDefaultNavigationTimeout(1);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, server.PREFIX + "/empty.html") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("NA-036 should disable timeout when its set to 0", () => {
    // [ts] let error = null;
    // [ts] let loaded = false;
    // [ts] page.once("load", () => loaded = true);
    // [ts] page.goto(server.PREFIX + "/grid.html", { timeout: 0, waitUntil: "load" }).catch(e => error = e);
    // [ts] expect_eq(error, null);
    // [ts] expect_eq(loaded, true);
    // [ts] });
    // [ts] test("NA-037 should fail when replaced by another navigation", () => {
    // [ts] let anotherPromise;
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] anotherPromise = page.goto(server.PREFIX + "/one-style.html");
    // Hang request to empty.html.
    // [ts] });
    // [ts] const error = page.goto(server.PREFIX + "/empty.html").catch(e => e);
    // [ts] anotherPromise;
    // [ts] if (browserName == "chromium") {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "net::ERR_ABORTED") != null, true);
    // [ts] } else if (browserName == "webkit") {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Navigation to "${server.PREFIX + "/empty.html"}" is interrupted by another navigation to "${server.PREFIX + "/one-style.html"}"`) != null, true);
    // [ts] } else if (browserName == "firefox") {
    // [ts] if (isBidi)
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Protocol error (browsingContext.navigate): unknown error") != null, true);
    // [ts] else {
    // Firefox might yield either NS_BINDING_ABORTED or 'navigation interrupted by another one'
    // [ts] expect_eq(error.message.includes(`Navigation to "${server.PREFIX + "/empty.html"}" is interrupted by another navigation to "${server.PREFIX + "/one-style.html"}"`) || error.message.includes("NS_BINDING_ABORTED"), true);
    // [ts] }
    // [ts] });
    // [ts] test("NA-038 js redirect overrides url bar navigation ", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20749" });
    // [ts] test(trace == "on", "tracing waits for snapshot that never arrives because pending navigation");
    // [ts] server.setRoute("/a", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`
    // [ts] <body>
    // [ts] <script>
    // [ts] setTimeout(() => {
    // [ts] window.location.pathname = '/c';
    // [ts] }, 1000);
    // [ts] </script>
    // [ts] </body>
    // [ts] `);
    // [ts] });
    // [ts] const events = [];
    // [ts] server.setRoute("/b", (req, res) => {
    // [ts] events.push("started b");
    // [ts] Promise.init(f => setTimeout(f, 2000));
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`BBB`);
    // [ts] events.push("finished b");
    // [ts] });
    // [ts] server.setRoute("/c", (req, res) => {
    // [ts] events.push("started c");
    // [ts] Promise.init(f => setTimeout(f, 2000));
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`CCC`);
    // [ts] events.push("finished c");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/a");
    // [ts] const error = page.goto(server.PREFIX + "/b").then(r => null, e => e);
    // [ts] const expectEvents = (browserName == "chromium") ?
    // [ts] ["started b", "finished b"] :
    // [ts] ["started b", "started c", "finished b", "finished c"];
    // [ts] expect(() => expect_eq(events, expectEvents)).toPass();
    // [ts] expect_eq(events, expectEvents);
    // [ts] if (browserName == "chromium") {
    // Chromium prioritizes the url bar navigation over the js redirect.
    // [ts] expect_eq(error, false);
    // [ts] expect(page).toHaveURL(server.PREFIX + "/b");
    // [ts] } else if (browserName == "webkit") {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Navigation to "${server.PREFIX + "/b"}" is interrupted by another navigation to "${server.PREFIX + "/c"}"`) != null, true);
    // [ts] expect(page).toHaveURL(server.PREFIX + "/c");
    // [ts] } else if (browserName == "firefox") {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_BINDING_ABORTED") != null, true);
    // [ts] expect(page).toHaveURL(server.PREFIX + "/c");
    // [ts] }
    // [ts] });
    // [ts] test("NA-039 should succeed on url bar navigation when there is pending navigation", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21574" });
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] server.setRoute("/a", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`
    // [ts] <body>
    // [ts] <script>
    // [ts] setTimeout(() => {
    // [ts] window.location.pathname = '/c';
    // [ts] }, 10);
    // [ts] </script>
    // [ts] </body>
    // [ts] `);
    // [ts] });
    // [ts] const events = [];
    // [ts] server.setRoute("/b", (req, res) => {
    // [ts] events.push("started b");
    // [ts] Promise.init(f => setTimeout(f, 2000));
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`BBB`);
    // [ts] events.push("finished b");
    // [ts] });
    // [ts] server.setRoute("/c", (req, res) => {
    // [ts] events.push("started c");
    // [ts] Promise.init(f => setTimeout(f, 2000));
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`CCC`);
    // [ts] events.push("finished c");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/a");
    // [ts] page.waitForTimeout(1000);
    // [ts] const error = page.goto(server.PREFIX + "/b").then(r => null, e => e);
    // [ts] const expectEvents = ["started c", "started b", "finished c", "finished b"];
    // [ts] expect(() => expect_eq(events, expectEvents)).toPass({ timeout: 5000 });
    // [ts] expect_eq(events, expectEvents);
    // [ts] expect_eq(error, false);
    // [ts] expect_eq(page.url(), server.PREFIX + "/b");
    // [ts] });
    // [ts] test("NA-040 should work when navigating to valid url", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("NA-041 should work when navigating to data url", () => {
    // [ts] const response = page.goto("data:text/html,hello");
    // [ts] expect_eq(response, null);
    // [ts] });
    // [ts] test("NA-042 should work when navigating to 404", () => {
    // [ts] const response = page.goto(server.PREFIX + "/not-found");
    // [ts] expect_eq(response.ok(), false);
    // [ts] expect_eq(response.status(), 404);
    // [ts] });
    // [ts] test("NA-043 should return last response in redirect chain", () => {
    // [ts] server.setRedirect("/redirect/1.html", "/redirect/2.html");
    // [ts] server.setRedirect("/redirect/2.html", "/redirect/3.html");
    // [ts] server.setRedirect("/redirect/3.html", server.EMPTY_PAGE);
    // [ts] const response = page.goto(server.PREFIX + "/redirect/1.html");
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("NA-044 should not leak listeners during navigation", () => {
    // [ts] let warning = null;
    // [ts] const warningHandler = w => warning = w;
    // [ts] process.on("warning", warningHandler);
    // [ts] let i = 0;
    // [ts] while (i < 20) // [cap-migrate] for-step: ++i
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] process.off("warning", warningHandler);
    // [ts] expect_eq(warning, null);
    // [ts] });
    // [ts] test("NA-045 should not leak listeners during bad navigation", () => {
    // [ts] let warning = null;
    // [ts] const warningHandler = w => warning = w;
    // [ts] process.on("warning", warningHandler);
    // [ts] let i = 0;
    // [ts] while (i < 20) // [cap-migrate] for-step: ++i
    // [ts] page.goto("asdf").catch(e => {/* swallow navigation error */});
    // [ts] process.off("warning", warningHandler);
    // [ts] expect_eq(warning, null);
    // [ts] });
    // [ts] test("NA-046 should not leak listeners during 20 waitForNavigation", () => {
    // [ts] let warning = null;
    // [ts] const warningHandler = w => warning = w;
    // [ts] process.on("warning", warningHandler);
    // [ts] const promises = [...Array(20)].map(() => page.waitForNavigation());
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] joinAll(promises); // migrated: Promise.all -> joinAll
    // [ts] process.off("warning", warningHandler);
    // [ts] expect_eq(warning, null);
    // [ts] });
    // [ts] test("NA-047 should navigate to dataURL and not fire dataURL requests", () => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] const dataURL = "data:text/html,<div>yo</div>";
    // [ts] const response = page.goto(dataURL);
    // [ts] expect_eq(response, null);
    // [ts] expect_eq(requests.length, 0);
    // [ts] });
    // [ts] test("NA-048 should navigate to URL with hash and fire requests without hash", () => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] const response = page.goto(server.EMPTY_PAGE + "#hash");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(requests.length, 1);
    // [ts] expect_eq(requests[0].url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("NA-049 should work with self requesting page", () => {
    // [ts] const response = page.goto(server.PREFIX + "/self-request.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.url(), "self-request.html") != null, true);
    // [ts] });
    // [ts] test("NA-050 should fail when navigating and show the url at the error message", () => {
    // [ts] const url = httpsServer.PREFIX + "/redirect/1.html";
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.goto(url);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, url) != null, true);
    // [ts] });
    // [ts] test("NA-051 should be able to navigate to a page controlled by service worker", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42694" },
    // [ts] }, (_p) => {
    // [ts] test(isElectron);
    // [ts] test(isAndroid, "Service workers need a secure context, but the Android test server is reached over http://10.0.2.2");
    // [ts] async fn transaction(value: string?) {
    // [ts] const db = Promise.init((resolve, reject) => {
    // [ts] const request = indexedDB.open("roundtrip", 1);
    // [ts] request.onupgradeneeded = () => request.result.createObjectStore("items");
    // [ts] request.onsuccess = () => resolve(request.result);
    // [ts] request.onerror = () => reject(request.error);
    // [ts] });
    // [ts] return Promise.init((resolve, reject) => {
    // [ts] const tx = db.transaction("items", "readwrite");
    // [ts] const request = tx.objectStore("items").get("name");
    // [ts] request.onsuccess = () => {
    // [ts] if (value)
    // [ts] tx.objectStore("items").put(value, "name");
    // [ts] };
    // [ts] tx.oncomplete = () => resolve(request.result);
    // [ts] tx.onerror = tx.onabort = () => reject(tx.error);
    // [ts] });
    // [ts] }
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // [ts] page.waitForFunction(() => !!navigator.serviceWorker.controller);
    // [ts] page.evaluate(transaction, "value");
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] page.goBack();
    // [ts] page.waitForFunction(() => !!navigator.serviceWorker.controller);
    // [ts] expect_eq(page.evaluate(transaction, null), "value");
    // [ts] });
    // [ts] test("NA-052 should send referer", () => {
    // [ts] const [request1, request2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/grid.html"),
    // [ts] server.waitForRequest("/digits/1.png"),
    // [ts] page.goto(server.PREFIX + "/grid.html", {
    // [ts] referer: "http://google.com/",
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request1.headers["referer"], "http://google.com/");
    // Make sure subresources do not inherit referer.
    // [ts] expect_eq(request2.headers["referer"], server.PREFIX + "/grid.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/grid.html");
    // [ts] });
    // [ts] test("NA-053 should send referer of cross-origin URL", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27765" });
    // [ts] const [request1, request2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/grid.html"),
    // [ts] server.waitForRequest("/digits/1.png"),
    // [ts] page.goto(server.PREFIX + "/grid.html", {
    // [ts] referer: "https://microsoft.com/xbox/"
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request1.headers["referer"], "https://microsoft.com/xbox/");
    // Make sure subresources do not inherit referer.
    // [ts] expect_eq(request2.headers["referer"], server.PREFIX + "/grid.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/grid.html");
    // [ts] });
    // [ts] test("NA-054 should reject referer option when setExtraHTTPHeaders provides referer", () => {
    // [ts] page.setExtraHTTPHeaders({ "referer": "http://microsoft.com/" });
    // [ts] let error;
    // [ts] page.goto(server.PREFIX + "/grid.html", {
    // [ts] referer: "http://google.com/",
    // [ts] }).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"referer\" is already specified as extra HTTP header") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, server.PREFIX + "/grid.html") != null, true);
    // [ts] });
    // [ts] test("NA-055 should override referrer-policy", () => {
    // [ts] server.setRoute("/grid.html", (req, res) => {
    // [ts] res.setHeader("Referrer-Policy", "no-referrer");
    // [ts] server.serveFile(req, res);
    // [ts] });
    // [ts] const [request1, request2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/grid.html"),
    // [ts] server.waitForRequest("/digits/1.png"),
    // [ts] page.goto(server.PREFIX + "/grid.html", {
    // [ts] referer: "http://microsoft.com/",
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request1.headers["referer"], "http://microsoft.com/");
    // Make sure subresources do not inherit referer.
    // [ts] expect_eq(request2.headers["referer"], null);
    // [ts] expect_eq(page.url(), server.PREFIX + "/grid.html");
    // [ts] });
    // [ts] test("NA-056 should fail when canceled by another navigation", () => {
    // [ts] server.setRoute("/one-style.html", (req, res) => {});
    // [ts] const failed = page.goto(server.PREFIX + "/one-style.html").catch(e => e);
    // [ts] server.waitForRequest("/one-style.html");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const error = failed;
    // [ts] expect_eq(error.message, true);
    // [ts] });
    // [ts] test("NA-057 should work with lazy loading iframes", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] page.goto(server.PREFIX + "/frames/lazy-frame.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] });
    // [ts] test("NA-058 should report raw buffer for main resource", () => {
    // [ts] it.fail(browserName == "chromium", "Chromium sends main resource as text");
    // [ts] it.fail(browserName == "webkit" && platform == "win32" && channel != "webkit-wsl", "Same here");
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.statusCode = 200;
    // [ts] res.end(Buffer.from("ÃÂ (lowercase ÃÂ¼)", "utf-8"));
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/empty.html");
    // [ts] const body = response.body();
    // [ts] expect_eq(body.toString(), "ÃÂ (lowercase ÃÂ¼)");
    // [ts] });
    // [ts] test("NA-059 should not throw unhandled rejections on invalid url", () => {
    // [ts] const e = page.goto("https://www.youtube Panel Title.com/").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.toString(), "Panel Title") != null, true);
    // [ts] });
    // [ts] test("NA-060 should not crash when RTCPeerConnection is used", () => {
    // [ts] server.setRoute("/rtc.html", (_unused, res) => {
    // [ts] res.end(`
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <body>
    // [ts] <script>
    // [ts] window.RTCPeerConnection && new window.RTCPeerConnection({
    // [ts] iceServers: []
    // [ts] });
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/rtc.html");
    // [ts] page.evaluate(() => {
    // RTCPeerConnection is not present on WebKit Linux
    // [ts] window.RTCPeerConnection && window.RTCPeerConnection.init({
    // [ts] iceServers: []
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] test("NA-061 should properly wait for load", () => {
    // [ts] server.setRoute("/slow.js", (req, res) => {
    // [ts] Promise.init(x => setTimeout(x, 100));
    // [ts] res.writeHead(200, { "Content-Type": "application/javascript" });
    // [ts] res.end(`window.results.push('slow module');export const foo = 'slow';`);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/load-event/load-event.html");
    // [ts] const results = page.evaluate("window.results");
    // [ts] expect(results).toEqual([
    // [ts] "script tag after after module",
    // [ts] "slow module",
    // [ts] "module",
    // [ts] "DOMContentLoaded",
    // [ts] "load"
    // [ts] ]);
    // [ts] });
    // [ts] test("NA-062 should not resolve goto upon window.stop()", () => {
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "load/domcontentloaded events are flaky");
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] let response;
    // [ts] server.setRoute("/module.js", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/javascript" });
    // [ts] response = res;
    // [ts] });
    // [ts] let done = false;
    // [ts] page.goto(server.PREFIX + "/window-stop.html").then(() => done = true).catch(() => {});
    // [ts] server.waitForRequest("/module.js");
    // [ts] expect_eq(done, false);
    // [ts] page.waitForTimeout(1000);  // give it some time to erroneously resolve
    // [ts] response.end("");
    // [ts] page.waitForTimeout(1000);  // give it more time to erroneously resolve
    // [ts] expect_eq(done, false);
    // [ts] });
    // [ts] test("NA-063 should return from goto if new navigation is started", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] server.setRoute("/slow.js", (req, res) => void 0);
    // [ts] let finished = false;
    // [ts] const navigation = page.goto(server.PREFIX + "/load-event/load-event.html").then(r => {
    // [ts] finished = true;
    // [ts] return r;
    // [ts] });
    // [ts] Promise.init(r => setTimeout(r, 500));
    // [ts] expect_eq(finished, false);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq((navigation).status(), 200);
    // [ts] });
    // [ts] test("NA-064 should return when navigation is committed if commit is specified", () => {
    // [ts] server.setRoute("/script.js", (req, res) => {});
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("content-type", "text/html");
    // [ts] res.end("<title>Hello</title><script src=\"script.js\"></script>");
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE, { waitUntil: "commit" });
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(page.title(), "Hello");
    // [ts] });
    // [ts] test("NA-065 should wait for load when iframe attaches and detaches", () => {
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`
    // [ts] <body>
    // [ts] <script>
    // [ts] const iframe = document.createElement('iframe');
    // [ts] iframe.src = './iframe.html';
    // [ts] document.body.appendChild(iframe);
    // [ts] setTimeout(() => iframe.remove(), 1000);
    // [ts] </script>
    // [ts] </body>
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/iframe.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`
    // [ts] <link rel="stylesheet" href="./style2.css">
    // [ts] `);
    // [ts] });
    // Stall the css so that 'load' does not fire.
    // [ts] server.setRoute("/style2.css", () => {});
    // [ts] const frameDetached = page.waitForEvent("framedetached");
    // [ts] const done = page.goto(server.EMPTY_PAGE, { waitUntil: "load" });
    // [ts] frameDetached; // Make sure that iframe is gone.
    // [ts] done;
    // [ts] expect_eq(page.$("iframe"), null);
    // [ts] });
    // [ts] test("NA-066 should return url with basic auth info", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23138" });
    // [ts] const url = `http://admin:admin@${loopback || "localhost"}:${server.PORT}/empty.html`;
    // [ts] page.goto(url);
    // [ts] expect_eq(page.url(), url);
    // [ts] });
    // [ts] test("NA-097 should navigate while a new service worker takes over", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44287" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium", "https://issues.chromium.org/issues/589233823");
    // [ts] let version = 0;
    // [ts] server.setRoute("/sw.js", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/javascript");
    // [ts] res.end(`
    // version ${++version}
    // [ts] let release;
    // [ts] self.addEventListener('install', () => self.skipWaiting());
    // [ts] self.addEventListener('fetch', event => event.respondWith(fetch(event.request)));
    // [ts] self.addEventListener('message', event => {
    // [ts] if (event.data === 'hold') {
    // [ts] event.waitUntil(new Promise(f => release = f));
    // [ts] return;
    // [ts] }
    // Block the worker, so that the navigation starts before the worker becomes idle.
    // [ts] const end = Date.now() + 500;
    // [ts] while (Date.now() < end) {}
    // [ts] release();
    // [ts] });
    // [ts] `);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const registration = page.evaluateHandle(() => {
    // [ts] navigator.serviceWorker.register("/sw.js");
    // [ts] const registration = navigator.serviceWorker.ready;
    // [ts] registration.active.postMessage("hold");
    // [ts] return registration;
    // [ts] });
    // [ts] registration.evaluate(r => r.update());
    // [ts] page.waitForFunction(r => r.waiting, registration);
    // [ts] registration.evaluate(r => r.active.postMessage("release"));
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] expect(page).toHaveTitle("Woof-Woof");
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
}
