// Migrated from TypeScript (page/page-network-idle.spec.ts) via cap from -ts

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
// [cap-migrate] import { Frame } from "playwright";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";


// TODO(cap-migrate): refine TS function
// [ts] async fn networkIdleTest(frame: Frame, server: TestServer, action:  = > Promise<any>, isSetContent: bool? = null) {
// [ts] const waitForRequest = (suffix) => {
// [ts] return joinAll([ // migrated: Promise.all -> joinAll
// [ts] server.waitForRequest(suffix),
// [ts] frame.page().waitForRequest(server.PREFIX + suffix),
// [ts] ]);
// [ts] };
// [ts] let responseA;
// [ts] let responseB;
// Hold on to a bunch of requests without answering.
// [ts] server.setRoute("/fetch-request-a.js", (req, res) => responseA = res);
// [ts] const firstFetchResourceRequested = waitForRequest("/fetch-request-a.js");
// [ts] server.setRoute("/fetch-request-b.js", (req, res) => responseB = res);
// [ts] const secondFetchResourceRequested = waitForRequest("/fetch-request-b.js");
// [ts] const waitForLoadPromise = if (isSetContent) Promise.resolve() else frame.waitForNavigation({ waitUntil: "load" });
// Navigate to a page which loads immediately and then does a bunch of
// requests via javascript's fetch method.
// [ts] const actionPromise = action();
// Track when the action gets completed.
// [ts] let actionFinished = false;
// [ts] void actionPromise.then(() => actionFinished = true);
// Wait for the frame's 'load' event.
// [ts] waitForLoadPromise;
// [ts] expect_eq(actionFinished, false);
// Wait for the initial resource to be requested.
// [ts] firstFetchResourceRequested;
// [ts] expect_eq(actionFinished, false);
// Trigger the second request.
// [ts] frame.page().evaluate(() => window["fetchSecond"]());
// Finish the first request.
// [ts] responseA.statusCode = 404;
// [ts] responseA.end(`File not found`);
// Wait for the second round to be requested.
// [ts] secondFetchResourceRequested;
// [ts] expect_eq(actionFinished, false);
// Finishing the second response should trigger networkidle.
// [ts] let timerTriggered = false;
// [ts] const timer = setTimeout(() => timerTriggered = true, 500);
// [ts] responseB.statusCode = 404;
// [ts] responseB.end(`File not found`);
// [ts] const response = actionPromise;
// [ts] clearTimeout(timer);
// [ts] expect_eq(timerTriggered, true);
// [ts] if (!isSetContent) {
// [ts] expect_eq(response.ok(), true);
// [ts] }
// [ts] }












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("NE-070 should navigate to empty page with networkidle", () => {
    // [ts] const response = page.goto(server.EMPTY_PAGE, { waitUntil: "networkidle" });
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("NE-071 should wait for networkidle to succeed navigation", () => {
    // [ts] networkIdleTest(page.mainFrame(), server, () => {
    // [ts] return page.goto(server.PREFIX + "/networkidle.html", { waitUntil: "networkidle" });
    // [ts] });
    // [ts] });
    // [ts] test("NE-072 should wait for networkidle to succeed navigation with request from previous navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/foo.js", () => {});
    // [ts] page.setContent(`<script>fetch('foo.js');</script>`);
    // [ts] networkIdleTest(page.mainFrame(), server, () => {
    // [ts] return page.goto(server.PREFIX + "/networkidle.html", { waitUntil: "networkidle" });
    // [ts] });
    // [ts] });
    // [ts] test("NE-073 should wait for networkidle in waitForNavigation", () => {
    // [ts] networkIdleTest(page.mainFrame(), server, () => {
    // [ts] const promise = page.waitForNavigation({ waitUntil: "networkidle" });
    // [ts] void page.goto(server.PREFIX + "/networkidle.html");
    // [ts] return promise;
    // [ts] });
    // [ts] });
    // [ts] test("NE-074 should wait for networkidle in setContent", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] networkIdleTest(page.mainFrame(), server, () => {
    // [ts] return page.setContent(`<script src='networkidle.js'></script>`, { waitUntil: "networkidle" });
    // [ts] }, true);
    // [ts] });
    // [ts] test("NE-075 should wait for networkidle in setContent with request from previous navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/foo.js", () => {});
    // [ts] page.setContent(`<script>fetch('foo.js');</script>`);
    // [ts] networkIdleTest(page.mainFrame(), server, () => {
    // [ts] return page.setContent(`<script src='networkidle.js'></script>`, { waitUntil: "networkidle" });
    // [ts] }, true);
    // [ts] });
    // [ts] test("NE-076 should wait for networkidle when navigating iframe", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] const frame = page.mainFrame().childFrames()[0];
    // [ts] networkIdleTest(frame, server, () => frame.goto(server.PREFIX + "/networkidle.html", { waitUntil: "networkidle" }));
    // [ts] });
    // [ts] test("NE-077 should wait for networkidle in setContent from the child frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] networkIdleTest(page.mainFrame(), server, () => {
    // [ts] return page.setContent(`<iframe src='networkidle.html'></iframe>`, { waitUntil: "networkidle" });
    // [ts] }, true);
    // [ts] });
    // [ts] test("NE-078 should wait for networkidle from the child frame", () => {
    // [ts] networkIdleTest(page.mainFrame(), server, () => {
    // [ts] return page.goto(server.PREFIX + "/networkidle-frame.html", { waitUntil: "networkidle" });
    // [ts] });
    // [ts] });
    // [ts] test("NE-079 should wait for networkidle from the popup", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <button id=box1 onclick="window.open('./popup/popup.html')">Button1</button>
    // [ts] <button id=box2 onclick="window.open('./popup/popup.html')">Button2</button>
    // [ts] <button id=box3 onclick="window.open('./popup/popup.html')">Button3</button>
    // [ts] <button id=box4 onclick="window.open('./popup/popup.html')">Button4</button>
    // [ts] <button id=box5 onclick="window.open('./popup/popup.html')">Button5</button>
    // [ts] `);
    // [ts] let i = 1;
    // [ts] while (i < 6) { // [cap-migrate] for-step: ++i
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.click("#box" + i)
    // [ts] ]);
    // [ts] popup.waitForLoadState("networkidle");
    // [ts] }
    // [ts] });
    // [ts] test("NE-080 should wait for networkidle when iframe attaches and detaches", () => {
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] let done = false;
    // [ts] const promise = page.setContent(`
    // [ts] <body>
    // [ts] <script>
    // [ts] const iframe = document.createElement('iframe');
    // [ts] iframe.src = ${JSON.stringify(server.EMPTY_PAGE)};
    // [ts] document.body.appendChild(iframe);
    // [ts] </script>
    // [ts] </body>
    // [ts] `, { waitUntil: 'networkidle' }).then(() => done = true);
    // [ts] page.waitForTimeout(600);
    // [ts] expect_eq(done, false);
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("iframe").remove();
    // [ts] });
    // [ts] promise;
    // [ts] expect_eq(done, true);
    // [ts] });
    // [ts] test("NE-081 should work after repeated navigations in the same page", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/18283" });
    // [ts] let requestCount = 0;
    // [ts] page.route("**/empty.html", route => {
    // [ts] void route.fulfill({
    // [ts] contentType: "text/html",
    // [ts] body: `
    // [ts] <script>
    // [ts] fetch('http://localhost:8000/sample').then(res => console.log(res.json()))
    // [ts] </script>`
    // [ts] });
    // [ts] });
    // [ts] page.route("**/sample", route => {
    // [ts] requestCount++;
    // [ts] void route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] body: JSON.stringify({
    // [ts] content: "sample"
    // [ts] })
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "networkidle" });
    // [ts] expect_eq(requestCount, 1);
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "networkidle" });
    // [ts] expect_eq(requestCount, 2);
    // [ts] });
    // [ts] test("NE-082 should not wait for an open EventSource connection", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/37226" });
    // [ts] server.setRoute("/sse", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/event-stream",
    // [ts] "Connection": "keep-alive",
    // [ts] "Cache-Control": "no-cache",
    // [ts] });
    // [ts] res.write("data: hello\n\n");
    // [ts] });
    // [ts] server.setRoute("/sse-page.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<script>new EventSource('/sse');</script>`);
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/sse-page.html", { waitUntil: "networkidle" });
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("NE-083 should not wait for an open EventSource connection in setContent", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/37226" });
    // [ts] server.setRoute("/sse", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/event-stream",
    // [ts] "Connection": "keep-alive",
    // [ts] "Cache-Control": "no-cache",
    // [ts] });
    // [ts] res.write("data: hello\n\n");
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<script>
    // [ts] window.__sseOpened = new Promise(resolve => {
    // [ts] const es = new EventSource('/sse');
    // [ts] es.onmessage = () => resolve();
    // [ts] });
    // [ts] </script>`, { waitUntil: 'networkidle' });
    // [ts] page.evaluate(() => window["__sseOpened"]);
    // [ts] });
}
