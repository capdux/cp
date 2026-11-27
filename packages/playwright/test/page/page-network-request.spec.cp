// Migrated from TypeScript (page/page-network-request.spec.ts) via cap from -ts

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
import { attachFrame } from "../config/utils.cp";
import fs from "cap:fs";

fn adjustServerHeaders(headers: string, browserName: string) {
  if (browserName == "firefox") {
    delete headers["priority"];
  }
  return headers;
}





// https://github.com/microsoft/playwright/issues/3993




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("NE-001 should work for main frame navigation request", () => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(requests.length, 1);
    // [ts] expect_eq(requests[0].frame(), page.mainFrame());
    // [ts] });
    // [ts] test("NE-002 should work for subframe navigation request", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect_eq(requests.length, 1);
    // [ts] expect_eq(requests[0].frame(), page.frames()[1]);
    // [ts] });
    // [ts] test("NE-003 should work for fetch requests @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.evaluate(() => fetch("/digits/1.png"));
    // [ts] expect_eq(requests.length, 1);
    // [ts] expect_eq(requests[0].frame(), page.mainFrame());
    // [ts] });
    // [ts] test("NE-004 should work for a redirect", () => {
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.goto(server.PREFIX + "/foo.html");
    // [ts] expect_eq(requests.length, 2);
    // [ts] expect_eq(requests[0].url(), server.PREFIX + "/foo.html");
    // [ts] expect_eq(requests[1].url(), server.PREFIX + "/empty.html");
    // [ts] });
    // [ts] test("NE-005 should not work for a redirect and interception", () => {
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const requests = [];
    // [ts] page.route("**", route => {
    // [ts] requests.push(route.request());
    // [ts] void route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/foo.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/empty.html");
    // [ts] expect_eq(requests.length, 1);
    // [ts] expect_eq(requests[0].url(), server.PREFIX + "/foo.html");
    // [ts] });
    // [ts] test("NE-006 should return headers", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, response.request().headers()["user-agent"], "Chrome") != null, true);
    // [ts] else if (browserName == "firefox")
    // [ts] expect_eq(std.mem.indexOf(u8, response.request().headers()["user-agent"], "Firefox") != null, true);
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, response.request().headers()["user-agent"], "WebKit") != null, true);
    // [ts] });
    // [ts] test("NE-007 should get the same headers as the server", () => {
    // [ts] test(isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] it.fail(browserName == "webkit" && platform == "win32" && channel != "webkit-wsl", "Curl does not show accept-encoding and accept-language");
    // [ts] let serverRequest;
    // [ts] server.setRoute("/empty.html", (request, response) => {
    // [ts] serverRequest = request;
    // [ts] response.end("done");
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/empty.html");
    // [ts] const headers = response.request().allHeaders();
    // [ts] expect_eq(headers, adjustServerHeaders(serverRequest.headers, browserName));
    // [ts] });
    // [ts] test("NE-008 should not return allHeaders() until they are available", () => {
    // [ts] test(isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] it.fail(browserName == "webkit" && platform == "win32" && channel != "webkit-wsl", "Curl does not show accept-encoding and accept-language");
    // [ts] let requestHeadersPromise;
    // [ts] page.on("request", request => requestHeadersPromise = request.allHeaders());
    // [ts] let responseHeadersPromise;
    // [ts] page.on("response", response => responseHeadersPromise = response.allHeaders());
    // [ts] let serverRequest;
    // [ts] server.setRoute("/empty.html", (request, response) => {
    // [ts] serverRequest = request;
    // [ts] response.writeHead(200, { "foo": "bar" });
    // [ts] Promise.init(f => setTimeout(f, 3000));
    // [ts] response.end("done");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const requestHeaders = requestHeadersPromise;
    // [ts] expect_eq(requestHeaders, adjustServerHeaders(serverRequest.headers, browserName));
    // [ts] const responseHeaders = responseHeadersPromise;
    // [ts] expect_eq(responseHeaders["foo"], "bar");
    // [ts] });
    // [ts] test("NE-009 should get the same headers as the server CORS", () => {
    // [ts] test(isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] it.fail(browserName == "webkit" && platform == "win32" && channel != "webkit-wsl", "Curl does not show accept-encoding and accept-language");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] let serverRequest;
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] serverRequest = request;
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("done");
    // [ts] });
    // [ts] const responsePromise = page.waitForEvent("response");
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url);
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect_eq(text, "done");
    // [ts] const response = responsePromise;
    // [ts] const headers = response.request().allHeaders();
    // [ts] expect_eq(headers, adjustServerHeaders(serverRequest.headers, browserName));
    // [ts] });
    // [ts] test("NE-010 should not get preflight CORS requests when intercepting", () => {
    // [ts] it.fail(isAndroid, "Playwright does not get CORS pre-flight on Android");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const requests = [];
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] requests.push(request.method);
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
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("done");
    // [ts] });
    // First check the browser will send preflight request when interception is OFF.
    // [ts] {
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] method: "DELETE",
    // [ts] headers: { "X-PINGOTHER": "pingpong" }
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect_eq(text, "done");
    // [ts] expect_eq(requests, ["OPTIONS", "DELETE"]);
    // [ts] }
    // Now check the browser will NOT send preflight request when interception is ON.
    // [ts] {
    // [ts] requests.length = 0;
    // [ts] const routed = [];
    // [ts] page.route("**/something", route => {
    // [ts] routed.push(route.request().method());
    // [ts] void route.continue();
    // [ts] });
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] method: "DELETE",
    // [ts] headers: { "X-PINGOTHER": "pingpong" }
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect_eq(text, "done");
    // Check that there was no preflight (OPTIONS) request.
    // [ts] expect_eq(routed, ["DELETE"]);
    // [ts] if (browserName == "firefox" && !isBidi)
    // [ts] expect_eq(requests, ["OPTIONS", "DELETE"]);
    // [ts] else {
    // [ts] expect_eq(requests, ["DELETE"]);
    // [ts] }
    // [ts] });
    // [ts] test("NE-011 should return postData", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (req, res) => res.end());
    // [ts] let request = null;
    // [ts] page.on("request", r => request = r);
    // [ts] page.evaluate(() => fetch("./post", { method: "POST", body: JSON.stringify({ foo: "bar" }) }));
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.postData(), "{\"foo\":\"bar\"}");
    // [ts] });
    // [ts] test("NE-012 should work with binary post data", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (req, res) => res.end());
    // [ts] let request = null;
    // [ts] page.on("request", r => request = r);
    // [ts] page.evaluate(() => {
    // [ts] fetch("./post", { method: "POST", body: Uint8Array.init(Array.from(Array(256).keys())) });
    // [ts] });
    // [ts] expect_eq(request, true);
    // [ts] const buffer = request.postDataBuffer();
    // [ts] expect_eq(buffer.length, 256);
    // [ts] let i = 0;
    // [ts] while (i < 256) // [cap-migrate] for-step: ++i
    // [ts] expect_eq(buffer[i], i);
    // [ts] });
    // [ts] test("NE-013 should work with binary post data and interception", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (req, res) => res.end());
    // [ts] let request = null;
    // [ts] page.route("/post", route => route.continue());
    // [ts] page.on("request", r => request = r);
    // [ts] page.evaluate(() => {
    // [ts] fetch("./post", { method: "POST", body: Uint8Array.init(Array.from(Array(256).keys())) });
    // [ts] });
    // [ts] expect_eq(request, true);
    // [ts] const buffer = request.postDataBuffer();
    // [ts] expect_eq(buffer.length, 256);
    // [ts] let i = 0;
    // [ts] while (i < 256) // [cap-migrate] for-step: ++i
    // [ts] expect_eq(buffer[i], i);
    // [ts] });
    // [ts] test("NE-014 should override post data content type", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let request = null;
    // [ts] server.setRoute("/post", (req, res) => {
    // [ts] request = req;
    // [ts] res.end();
    // [ts] });
    // [ts] page.route("**/post", (route, request) => {
    // [ts] const headers = request.headers();
    // [ts] headers["content-type"] = "application/x-www-form-urlencoded; charset=UTF-8";
    // [ts] void route.continue({
    // [ts] headers,
    // [ts] postData: request.postData()
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] fetch("./post", { method: "POST", body: "foo=bar" });
    // [ts] });
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.headers["content-type"], "application/x-www-form-urlencoded; charset=UTF-8");
    // [ts] });
    // [ts] test("NE-015 should get |undefined| with postData() when there is no post data", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.request().postData(), null);
    // [ts] });
    // [ts] test("NE-016 should parse the json post data", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (req, res) => res.end());
    // [ts] let request = null;
    // [ts] page.on("request", r => request = r);
    // [ts] page.evaluate(() => fetch("./post", { method: "POST", body: JSON.stringify({ foo: "bar" }) }));
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.postDataJSON(), { "foo": "bar" });
    // [ts] });
    // [ts] test("NE-017 should parse the data if content-type is application/x-www-form-urlencoded", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (req, res) => res.end());
    // [ts] let request = null;
    // [ts] page.on("request", r => request = r);
    // [ts] page.setContent(`<form method='POST' action='/post'><input type='text' name='foo' value='bar'><input type='number' name='baz' value='123'><input type='submit'></form>`);
    // [ts] page.click("input[type=submit]");
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.postDataJSON(), { "foo": "bar", "baz": "123" });
    // [ts] });
    // [ts] test("NE-018 should parse the data if content-type is application/x-www-form-urlencoded; charset=UTF-8", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29872" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requestPromise = page.waitForRequest("**/post");
    // [ts] page.evaluate(() => fetch("./post", {
    // [ts] method: "POST",
    // [ts] headers: {
    // [ts] "content-type": "application/x-www-form-urlencoded; charset=UTF-8",
    // [ts] },
    // [ts] body: "foo=bar&baz=123"
    // [ts] }));
    // [ts] expect_eq((requestPromise).postDataJSON(), { "foo": "bar", "baz": "123" });
    // [ts] });
    // [ts] test("NE-019 should get |undefined| with postDataJSON() when there is no post data", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.request().postDataJSON(), null);
    // [ts] });
    // [ts] test("NE-020 should return multipart/form-data", () => {
    // [ts] it.fixme(browserName == "webkit", "File content is missing in WebKit");
    // [ts] test(browserName == "chromium" && browserMajorVersion < 126, "Requires a recent enough protocol");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (req, res) => res.end());
    // [ts] page.route("**/*", route => route.continue());
    // [ts] const requestPromise = page.waitForRequest("**/post");
    // [ts] page.evaluate(() => {
    // [ts] const body = FormData.init();
    // [ts] body.set("name1", "value1");
    // [ts] body.set("file", File.init(["file-value"], "foo.txt"));
    // [ts] body.set("name2", "value2");
    // [ts] body.append("name2", "another-value2");
    // [ts] fetch("/post", { method: "POST", body });
    // [ts] });
    // [ts] const request = requestPromise;
    // [ts] const contentType = request.headerValue("Content-Type");
    // [ts] const re = "^multipart\\/form-data; boundary=(.*)$";
    // [ts] expect(contentType).toMatch(re);
    // [ts] const b = contentType.match(re)[1];
    // [ts] const expected = `--${b}\r\nContent-Disposition: form-data; name=\"name1\"\r\n\r\nvalue1\r\n--${b}\r\nContent-Disposition: form-data; name=\"file\"; filename=\"foo.txt\"\r\nContent-Type: application/octet-stream\r\n\r\nfile-value\r\n--${b}\r\nContent-Disposition: form-data; name=\"name2\"\r\n\r\nvalue2\r\n--${b}\r\nContent-Disposition: form-data; name=\"name2\"\r\n\r\nanother-value2\r\n--${b}--\r\n`;
    // [ts] expect_eq(request.postDataBuffer().toString("utf8"), expected);
    // [ts] });
    // [ts] test("NE-021 should return event source", () => {
    // [ts] const SSE_MESSAGE = { foo: "bar" };
    // 1. Setup server-sent events on server that immediately sends a message to the client.
    // [ts] server.setRoute("/sse", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/event-stream",
    // [ts] "Connection": "keep-alive",
    // [ts] "Cache-Control": "no-cache",
    // [ts] });
    // [ts] res.write(`data: ${JSON.stringify(SSE_MESSAGE)}\n\n`);
    // [ts] });
    // 2. Subscribe to page request events.
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // 3. Connect to EventSource in browser and return first message.
    // [ts] expect(page.evaluate(() => {
    // [ts] const eventSource = EventSource.init("/sse");
    // [ts] return Promise.init(resolve => {
    // [ts] eventSource.onmessage = e => resolve(JSON.parse(e.data));
    // [ts] });
    // [ts] })).toEqual(SSE_MESSAGE);
    // [ts] expect_eq(requests[0].resourceType(), "eventsource");
    // [ts] });
    // [ts] test("NE-022 should return navigation bit", () => {
    // [ts] const requests = Map.init();
    // [ts] page.on("request", request => requests.set(request.url().split("/").pop(), request));
    // [ts] server.setRedirect("/rrredirect", "/frames/one-frame.html");
    // [ts] page.goto(server.PREFIX + "/rrredirect");
    // [ts] expect_eq(requests.get("rrredirect").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("one-frame.html").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("frame.html").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("script.js").isNavigationRequest(), false);
    // [ts] expect_eq(requests.get("style.css").isNavigationRequest(), false);
    // [ts] });
    // [ts] test("NE-023 should return navigation bit when navigating to image", () => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.goto(server.PREFIX + "/pptr.png");
    // [ts] expect_eq(requests[0].isNavigationRequest(), true);
    // [ts] });
    // [ts] test("NE-024 should report raw headers", () => {
    // [ts] test(isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] let expectedHeaders: { name: string, value: string }[];
    // [ts] server.setRoute("/headers", (req, res) => {
    // [ts] expectedHeaders = [];
    // [ts] let i = 0;
    // [ts] while (i < req.rawHeaders.length) // [cap-migrate] for-step: i += 2
    // [ts] expectedHeaders.push({ name: req.rawHeaders[i], value: req.rawHeaders[i + 1] });
    // [ts] if (browserName == "webkit" && platform == "win32" && channel != "webkit-wsl") {
    // [ts] expectedHeaders = expectedHeaders.filter((_p) => name.toLowerCase() != "accept-encoding");
    // Convert "value": "en-US, en-US" => "en-US"
    // [ts] expectedHeaders = expectedHeaders.map(e => {
    // [ts] const { name, value } = e;
    // [ts] if (name.toLowerCase() != "accept-language")
    // [ts] return e;
    // [ts] const values = value.split(",").map(v => v.trim());
    // [ts] if (values.length == 1)
    // [ts] return e;
    // [ts] if (values[0] != values[1])
    // [ts] return e;
    // [ts] return { name, value: values[0] };
    // [ts] });
    // [ts] }
    // [ts] if (browserName == "firefox")
    // [ts] expectedHeaders = expectedHeaders.filter((_p) => name.toLowerCase() != "priority");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest("**/*"),
    // [ts] page.evaluate(() => fetch("/headers", {
    // [ts] headers: [
    // [ts] ["header-a", "value-a"],
    // [ts] ["header-b", "value-b"],
    // [ts] ["header-a", "value-a-1"],
    // [ts] ["header-a", "value-a-2"],
    // [ts] ]
    // [ts] }))
    // [ts] ]);
    // [ts] const headers = request.headersArray();
    // [ts] expect_eq(headers.sort((a, b) => a.name.localeCompare(b.name)), expectedHeaders.sort((a, b) => a.name.localeCompare(b.name)));
    // [ts] expect_eq(request.headerValue("header-a"), "value-a, value-a-1, value-a-2");
    // [ts] expect_eq(request.headerValue("not-there"), null);
    // [ts] });
    // [ts] test("NE-025 should report raw response headers in redirects", () => {
    // [ts] test(browserName == "webkit", `WebKit won't give us raw headers for redirects`);
    // [ts] server.setExtraHeaders("/redirect/1.html", { "sec-test-header": "1.html" });
    // [ts] server.setExtraHeaders("/redirect/2.html", { "sec-test-header": "2.html" });
    // [ts] server.setExtraHeaders("/empty.html", { "sec-test-header": "empty.html" });
    // [ts] server.setRedirect("/redirect/1.html", "/redirect/2.html");
    // [ts] server.setRedirect("/redirect/2.html", "/empty.html");
    // [ts] const expectedUrls = ["/redirect/1.html", "/redirect/2.html", "/empty.html"].map(s => server.PREFIX + s);
    // [ts] const expectedHeaders = ["1.html", "2.html", "empty.html"];
    // [ts] const response = page.goto(server.PREFIX + "/redirect/1.html");
    // [ts] const redirectChain = [];
    // [ts] const headersChain = [];
    // [ts] let req = response.request();
    // [ts] while (req) { // [cap-migrate] for-step: req = req.redirectedFrom()
    // [ts] redirectChain.unshift(req.url());
    // [ts] const res = req.response();
    // [ts] const headers = res.allHeaders();
    // [ts] headersChain.unshift(headers["sec-test-header"]);
    // [ts] }
    // [ts] expect_eq(redirectChain, expectedUrls);
    // [ts] expect_eq(headersChain, expectedHeaders);
    // [ts] });
    // [ts] test("NE-026 should report all cookies in one header", () => {
    // [ts] test(isElectron && browserMajorVersion < 99, "This needs Chromium >= 99");
    // [ts] const expectedHeaders = {};
    // [ts] server.setRoute("/headers", (req, res) => {
    // [ts] let i = 0;
    // [ts] while (i < req.rawHeaders.length) // [cap-migrate] for-step: i += 2
    // [ts] expectedHeaders[req.rawHeaders[i]] = req.rawHeaders[i + 1];
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] document.cookie = "myCookie=myValue";
    // [ts] document.cookie = "myOtherCookie=myOtherValue";
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const cookie = (response.request().allHeaders())["cookie"];
    // [ts] expect_eq(cookie, "myCookie=myValue; myOtherCookie=myOtherValue");
    // [ts] });
    // [ts] test("NE-027 should not allow to access frame on popup main request", () => {
    // [ts] page.setContent(`<a target=_blank href="${server.EMPTY_PAGE}">click me</a>`);
    // [ts] const requestPromise = page.context().waitForEvent("request");
    // [ts] const popupPromise = page.context().waitForEvent("page");
    // [ts] const clicked = page.getByText("click me").click();
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.isNavigationRequest(), true);
    // [ts] let error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] request.frame();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Frame for this navigation request is not available") != null, true);
    // [ts] const response = request.response();
    // [ts] response.finished();
    // [ts] popupPromise;
    // [ts] clicked;
    // [ts] });
    // [ts] test("NE-028 page.reload return 304 status code", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28779" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29441" });
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "Does not send second request");
    // [ts] let requestNumber = 0;
    // [ts] server.setRoute("/test.html", (req, res) => {
    // [ts] ++requestNumber;
    // [ts] const headers = {
    // [ts] "cf-cache-status": "DYNAMIC",
    // [ts] "Content-Type": "text/html;charset=UTF-8",
    // [ts] "Last-Modified": "Fri, 05 Jan 2024 01:56:20 GMT",
    // [ts] "Vary": "Access-Control-Request-Headers",
    // [ts] };
    // [ts] if (requestNumber == 1)
    // [ts] res.writeHead(200, headers);
    // [ts] else {
    // [ts] res.writeHead(304, "Not Modified", headers);
    // [ts] res.write(`<div>Test</div>`);
    // [ts] res.end();
    // [ts] });
    // [ts] const response1 = page.goto(server.PREFIX + "/test.html");
    // [ts] expect_eq(response1.status(), 200);
    // [ts] const response2 = page.reload();
    // [ts] expect_eq(requestNumber, 2);
    // [ts] if (browserName == "chromium") {
    // [ts] expect_eq(response2.status(), 200);
    // [ts] expect_eq(response2.statusText(), "OK");
    // [ts] expect_eq(response2.text(), "<div>Test</div>");
    // [ts] } else {
    // [ts] expect_eq(response2.status(), 304);
    // [ts] expect_eq(response2.statusText(), "Not Modified");
    // [ts] }
    // [ts] });
    // [ts] test("NE-029 should handle mixed-content blocked requests", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29833" });
    // [ts] test(browserName != "chromium", "FF and WK actually succeed with the request, and block afterwards");
    // [ts] page.route("**/mixedcontent.html", route => {
    // [ts] void route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: `
    // [ts] <!doctype html>
    // [ts] <meta charset="utf-8">
    // [ts] <style>
    // [ts] @font-face {
    // [ts] font-family: 'pwtest-iconfont';
    // [ts] src: url('http://another.com/iconfont.woff2') format('woff2');
    // [ts] }
    // [ts] body {
    // [ts] font-family: 'pwtest-iconfont';
    // [ts] }
    // [ts] </style>
    // [ts] <span>+-</span>
    // [ts] `,
    // [ts] });
    // [ts] });
    // [ts] page.route("**/iconfont.woff2", async route => {
    // [ts] const body = fs.promises.readFile(asset("webfont/iconfont2.woff"));
    // [ts] route.fulfill({ body });
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("requestfailed", r => r.url().includes("iconfont.woff2")),
    // [ts] page.goto("https://example.com/mixedcontent.html"),
    // [ts] ]);
    // [ts] const headers = request.allHeaders();
    // [ts] expect_eq(headers["origin"], true);
    // [ts] expect_eq(request.failure().errorText, "mixed-content");
    // [ts] });
    // [ts] }
    // [ts] }
}
