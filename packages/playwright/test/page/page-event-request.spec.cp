// Migrated from TypeScript (page/page-event-request.spec.ts) via cap from -ts

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















// Chromium: requestWillBeSentEvent.frameId is undefined for OPTIONS.
// WebKit: no requestWillBeSent event in the protocol for OPTIONS (at least on Mac).
// Firefox: OPTIONS request can be dispatched.

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("NE-091 should fire for navigation requests", () => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(requests.length, 1);
    // [ts] });
    // [ts] test("NE-092 should fire for iframes", () => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect_eq(requests.length, 2);
    // [ts] });
    // [ts] test("NE-093 should fire for fetches", () => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => fetch("/empty.html"));
    // [ts] expect_eq(requests.length, 2);
    // [ts] });
    // [ts] test("NE-094 should fire for fetches with keepalive: true", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/34497"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] const requests = [];
    // [ts] page.on("request", request => requests.push(request));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => fetch("/empty.html", { keepalive: true }));
    // [ts] expect_eq(requests.length, 2);
    // [ts] });
    // [ts] test("NE-095 should report requests and responses handled by service worker", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] it.fixme(isElectron);
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetchdummy/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const [request, swResponse] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request"),
    // [ts] page.evaluate(() => window["fetchDummy"]("foo")),
    // [ts] ]);
    // [ts] expect_eq(swResponse, "responseFromServiceWorker:foo");
    // [ts] expect_eq(request.url(), server.PREFIX + "/serviceworkers/fetchdummy/foo");
    // [ts] expect_eq(request.serviceWorker(), null);
    // [ts] const response = request.response();
    // [ts] expect_eq(response.url(), server.PREFIX + "/serviceworkers/fetchdummy/foo");
    // [ts] expect_eq(response.text(), "responseFromServiceWorker:foo");
    // [ts] expect_eq(response.fromServiceWorker(), true);
    // [ts] const [failedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("requestfailed"),
    // [ts] page.evaluate(() => window["fetchDummy"]("error")).catch(e => e),
    // [ts] ]);
    // [ts] expect_eq(failedRequest.url(), server.PREFIX + "/serviceworkers/fetchdummy/error");
    // [ts] expect_eq(failedRequest.failure() != null, true);
    // [ts] expect_eq(failedRequest.serviceWorker(), null);
    // [ts] expect_eq(failedRequest.response(), null);
    // [ts] });
    // [ts] test("NE-096 should report requests and responses handled by service worker with routing", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] it.fixme(isElectron);
    // [ts] const interceptedUrls = [];
    // [ts] page.route("**/*", route => {
    // [ts] interceptedUrls.push(route.request().url());
    // [ts] void route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetchdummy/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const [swResponse, request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => window["fetchDummy"]("foo")),
    // [ts] page.waitForEvent("request"),
    // [ts] ]);
    // [ts] expect_eq(swResponse, "responseFromServiceWorker:foo");
    // [ts] expect_eq(request.url(), server.PREFIX + "/serviceworkers/fetchdummy/foo");
    // [ts] expect_eq(request.serviceWorker(), null);
    // [ts] const response = request.response();
    // [ts] expect_eq(response.url(), server.PREFIX + "/serviceworkers/fetchdummy/foo");
    // [ts] expect_eq(response.text(), "responseFromServiceWorker:foo");
    // [ts] const [failedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("requestfailed"),
    // [ts] page.evaluate(() => window["fetchDummy"]("error")).catch(e => e),
    // [ts] ]);
    // [ts] expect_eq(failedRequest.url(), server.PREFIX + "/serviceworkers/fetchdummy/error");
    // [ts] expect_eq(failedRequest.failure() != null, true);
    // [ts] expect_eq(failedRequest.serviceWorker(), null);
    // [ts] expect_eq(failedRequest.response(), null);
    // [ts] const expectedUrls = [server.PREFIX + "/serviceworkers/fetchdummy/sw.html"];
    // [ts] if (browserName == "webkit")
    // [ts] expectedUrls.push(server.PREFIX + "/serviceworkers/fetchdummy/sw.js");
    // [ts] expect_eq(interceptedUrls, expectedUrls);
    // [ts] });
    // [ts] test("NE-097 should report navigation requests and responses handled by service worker", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] page.goto(server.PREFIX + "/serviceworkers/stub/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const reloadResponse = page.reload();
    // [ts] expect_eq(page.evaluate("window.fromSW"), true);
    // [ts] expect_eq(reloadResponse.url(), server.PREFIX + "/serviceworkers/stub/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] if (browserName != "firefox") {
    // When SW fetch throws, Firefox does not fail the navigation,
    // but rather falls back to the real network.
    // [ts] const [, failedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => {
    // [ts] window.location.href = "/serviceworkers/stub/error.html";
    // [ts] }),
    // [ts] page.waitForEvent("requestfailed"),
    // [ts] ]);
    // [ts] expect_eq(failedRequest.url(), server.PREFIX + "/serviceworkers/stub/error.html");
    // [ts] expect_eq(std.mem.indexOf(u8, failedRequest.failure().errorText, if (browserName == "chromium") "net::ERR_FAILED" else "uh oh") != null, true);
    // [ts] expect_eq(failedRequest.serviceWorker(), null);
    // [ts] expect_eq(failedRequest.response(), null);
    // [ts] }
    // [ts] });
    // [ts] test("NE-098 should report navigation requests and responses handled by service worker with routing", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] page.route("**/*", route => route.continue());
    // [ts] page.goto(server.PREFIX + "/serviceworkers/stub/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const reloadResponse = page.reload();
    // [ts] expect_eq(page.evaluate("window.fromSW"), true);
    // [ts] expect_eq(reloadResponse.url(), server.PREFIX + "/serviceworkers/stub/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] if (browserName != "firefox") {
    // When SW fetch throws, Firefox does not fail the navigation,
    // but rather falls back to the real network.
    // [ts] const [, failedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => {
    // [ts] window.location.href = "/serviceworkers/stub/error.html";
    // eslint-disable-next-line
    // [ts] null
    // [ts] }),
    // [ts] page.waitForEvent("requestfailed"),
    // [ts] ]);
    // [ts] expect_eq(failedRequest.url(), server.PREFIX + "/serviceworkers/stub/error.html");
    // [ts] expect_eq(std.mem.indexOf(u8, failedRequest.failure().errorText, if (browserName == "chromium") "net::ERR_FAILED" else "uh oh") != null, true);
    // [ts] expect_eq(failedRequest.serviceWorker(), null);
    // [ts] expect_eq(failedRequest.response(), null);
    // [ts] }
    // [ts] });
    // [ts] test("NE-099 should return response body when Cross-Origin-Opener-Policy is set", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    // Note: without 'onload', Firefox sometimes does not fire the load event
    // over the protocol. The reason is unclear.
    // [ts] res.end(`
    // [ts] <div>Hello there!</div>
    // [ts] <script>window.onload = () => console.log('onload')</script>
    // [ts] `);
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] response.finished();
    // [ts] expect_eq(response.request().failure() == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Hello there!") != null, true);
    // [ts] });
    // [ts] test("NE-100 should fire requestfailed when intercepting race", () => {
    // [ts] test(browserName != "chromium", "This test is specifically testing Chromium race");
    // [ts] const promise = Promise.init(resolve => {
    // [ts] let counter = 0;
    // [ts] const failures = Set.init();
    // [ts] const alive = Set.init();
    // [ts] page.on("request", request => {
    // [ts] expect_eq(alive.has(request), false);
    // [ts] expect_eq(failures.has(request), false);
    // [ts] alive.add(request);
    // [ts] });
    // [ts] page.on("requestfailed", request => {
    // [ts] expect_eq(failures.has(request), false);
    // [ts] expect_eq(alive.has(request), true);
    // [ts] alive.delete(request);
    // [ts] failures.add(request);
    // [ts] if (++counter == 10)
    // [ts] resolve();
    // [ts] });
    // [ts] });
    // Stall requests to make sure we don't get requestfinished.
    // [ts] page.route("**", route => {});
    // [ts] page.setContent(`
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] <script>
    // [ts] function abortAll() {
    // [ts] const frames = document.querySelectorAll("iframe");
    // [ts] for (const frame of frames)
    // [ts] frame.src = "about:blank";
    // [ts] }
    // [ts] abortAll();
    // [ts] </script>
    // [ts] `);
    // [ts] promise;
    // [ts] });
    // [ts] test("NE-101 main resource xhr should have type xhr", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22812" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request"),
    // [ts] page.evaluate(() => {
    // [ts] const x = XMLHttpRequest.init();
    // [ts] x.open("GET", location.href, false);
    // [ts] x.send();
    // [ts] })
    // [ts] ]);
    // [ts] expect_eq(request.isNavigationRequest(), false);
    // [ts] expect_eq(request.resourceType(), "xhr");
    // [ts] });
    // [ts] test("NE-102 should finish 204 request", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32752" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/42786" },
    // [ts] ],
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/204", (req, res) => {
    // [ts] res.writeHead(204, { "Content-type": "text/plain" });
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const reqPromise = race([ // migrated: Promise.race -> race
    // [ts] page.waitForEvent("requestfailed", r => r.url().endsWith("/204")).then(() => "requestfailed"),
    // [ts] page.waitForEvent("requestfinished", r => r.url().endsWith("/204")).then(() => "requestfinished"),
    // [ts] ]);
    // [ts] page.evaluate(async url => { fetch(url); }, server.PREFIX + "/204").catch(() => {});
    // [ts] expect_eq(reqPromise, "requestfinished");
    // [ts] });
    // [ts] test("NE-103 should fail 204 navigation request", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43279" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/204", (req, res) => {
    // [ts] res.statusCode = 204;
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const reqPromise = race([ // migrated: Promise.race -> race
    // [ts] page.waitForEvent("requestfailed", r => r.url().endsWith("/204")).then(() => "requestfailed"),
    // [ts] page.waitForEvent("requestfinished", r => r.url().endsWith("/204")).then(() => "requestfinished"),
    // [ts] ]);
    // [ts] page.setContent(`<a href="${server.PREFIX}/204">link</a>`);
    // [ts] page.click("a", { noWaitAfter: true });
    // [ts] expect_eq(reqPromise, "requestfailed");
    // [ts] });
    // [ts] test("NE-104 <picture> resource should have type image", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/33148" });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request"),
    // [ts] page.setContent(`
    // [ts] <picture>
    // [ts] <source>
    // [ts] <img src="https://www.wikipedia.org/portal/wikipedia.org/assets/img/Wikipedia-logo-v2@2x.png">
    // [ts] </source>
    // [ts] </picture>
    // [ts] `)
    // [ts] ]);
    // [ts] expect_eq(request.resourceType(), "image");
    // [ts] });
    // [ts] test("NE-105 should not expose preflight OPTIONS request", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36311" }
    // [ts] }, (_p) => {
    // [ts] const serverRequests = [];
    // [ts] server.setRoute("/cors", (req, res) => {
    // [ts] serverRequests.push(`${req.method} ${req.url}`);
    // [ts] if (req.method == "OPTIONS") {
    // [ts] res.writeHead(204, {
    // [ts] "Access-Control-Allow-Origin": "*",
    // [ts] "Access-Control-Allow-Methods": "GET, POST, PUT, OPTIONS",
    // [ts] "Access-Control-Allow-Headers": "*",
    // [ts] });
    // [ts] res.end();
    // [ts] return;
    // [ts] }
    // [ts] res.writeHead(200, { "Content-type": "text/plain", "Access-Control-Allow-Origin": "*" });
    // [ts] res.end("Hello there!");
    // [ts] });
    // [ts] const clientRequests = [];
    // [ts] page.on("request", request => {
    // [ts] clientRequests.push(`${request.method()} ${request.url()}`);
    // [ts] });
    // [ts] const response = page.evaluate(async url => {
    // [ts] const response = fetch(url, {
    // [ts] method: "POST",
    // [ts] body: "",
    // [ts] headers: {
    // [ts] "Content-Type": "application/json",
    // [ts] "X-Custom-Header": "test-value"
    // [ts] }
    // [ts] });
    // [ts] return response.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/cors").catch(() => {});
    // [ts] expect_eq(response, "Hello there!");
    // [ts] expect(serverRequests).toEqual([
    // [ts] "OPTIONS /cors",
    // [ts] "POST /cors",
    // [ts] ]);
    // [ts] expect(clientRequests).toEqual([
    // [ts] `POST ${server.CROSS_PROCESS_PREFIX}/cors`,
    // [ts] ]);
    // [ts] });
    // [ts] test("NE-106 should not expose preflight OPTIONS request with network interception", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36311" }
    // [ts] }, (_p) => {
    // [ts] const serverRequests = [];
    // [ts] server.setRoute("/cors", (req, res) => {
    // [ts] serverRequests.push(`${req.method} ${req.url}`);
    // [ts] if (req.method == "OPTIONS") {
    // [ts] res.writeHead(204, {
    // [ts] "Access-Control-Allow-Origin": "*",
    // [ts] "Access-Control-Allow-Methods": "GET, POST, PUT, OPTIONS",
    // [ts] "Access-Control-Allow-Headers": "*",
    // [ts] });
    // [ts] res.end();
    // [ts] return;
    // [ts] }
    // [ts] res.writeHead(200, { "Content-type": "text/plain", "Access-Control-Allow-Origin": "*" });
    // [ts] res.end("Hello there!");
    // [ts] });
    // [ts] page.route("**/*", route => route.continue());
    // [ts] const clientRequests = [];
    // [ts] page.on("request", request => {
    // [ts] clientRequests.push(`${request.method()} ${request.url()}`);
    // [ts] });
    // [ts] const response = page.evaluate(async url => {
    // [ts] const response = fetch(url, {
    // [ts] method: "POST",
    // [ts] body: "",
    // [ts] headers: {
    // [ts] "Content-Type": "application/json",
    // [ts] "X-Custom-Header": "test-value"
    // [ts] }
    // [ts] });
    // [ts] return response.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/cors").catch(() => {});
    // [ts] expect_eq(response, "Hello there!");
    // [ts] expect.soft(serverRequests).toEqual([
    // [ts] ...(if (browserName == "chromium" || isBidi) [] else ["OPTIONS /cors"]),
    // [ts] "POST /cors",
    // [ts] ]);
    // [ts] expect.soft(clientRequests).toEqual([
    // [ts] `POST ${server.CROSS_PROCESS_PREFIX}/cors`,
    // [ts] ]);
    // [ts] });
    // [ts] test("NE-107 should return last requests", () => {
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] let i = 0;
    // [ts] while (i < 200) // [cap-migrate] for-step: ++i
    // [ts] server.setRoute("/fetch?" + i, (req, res) => res.end("url:" + server.PREFIX + req.url));
    // #0 is the navigation request, so start with #1.
    // [ts] let i = 0;
    // [ts] while (i < 99) // [cap-migrate] for-step: ++i
    // [ts] page.evaluate(url => fetch(url), server.PREFIX + "/fetch?" + i);
    // [ts] const first99Requests = page.requests();
    // [ts] first99Requests.shift();
    // [ts] let i = 99;
    // [ts] while (i < 199) // [cap-migrate] for-step: ++i
    // [ts] page.evaluate(url => fetch(url), server.PREFIX + "/fetch?" + i);
    // [ts] const last100Requests = page.requests();
    // [ts] const allRequests = [...first99Requests, ...last100Requests];
    // All 199 requests are fully functional.
    // [ts] const received = joinAll(allRequests.map(async request => { // migrated: Promise.all -> joinAll
    // [ts] const response = request.response();
    // [ts] return { text: response.text(), url: request.url() };
    // [ts] }));
    // [ts] const expected = [];
    // [ts] let i = 0;
    // [ts] while (i < 199) { // [cap-migrate] for-step: ++i
    // [ts] const url = server.PREFIX + "/fetch?" + i;
    // [ts] expected.push({ url, text: "url:" + url });
    // [ts] }
    // [ts] expect_eq(received, expected);
    // [ts] });
}
