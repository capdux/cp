// Migrated from TypeScript (library/chromium/extensions.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2019 Google Inc. All rights reserved.
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

// [cap-migrate] import { BrowserType, BrowserContext } from "playwright";
import { playwrightTest as base, expect } from "../../config/browserTest.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const it = base.extend<{
// [ts] launchPersistentContext: (extensionPath, options) => Promise<BrowserContext>;
// [ts] }>({
// [ts] launchPersistentContext: (_p, use) => {
// [ts] const browsers: []BrowserContext = [];
// [ts] use((extensionPath, options) => {
// [ts] const extensionOptions = {
// [ts] ...options,
// [ts] args: [
// [ts] `--disable-extensions-except=${extensionPath}`,
// [ts] `--load-extension=${extensionPath}`,
// [ts] ],
// [ts] };
// [ts] return browserType.launchPersistentContext("", extensionOptions);
// [ts] });
// [ts] joinAll(browsers.map(browser => browser.close())); // migrated: Promise.all -> joinAll
// [ts] }
// [ts] });


// TODO(cap-migrate): refine TS statement
// [ts] it.describe("MV3", () => {
// [ts] test((_p) => channel?.startsWith("chrome"), "--load-extension is not supported in Chrome anymore. https://groups.google.com/a/chromium.org/g/chromium-extensions/c/1-g8EFx2BBY/m/S0ET5wPjCAAJ");
// [ts] test("CR-036 should support service worker stop and restart lifecycle", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39475" }
// [ts] }, (_p) => {
// [ts] const extensionPath = asset("extension-mv3-sw-lifecycle");
// [ts] const context = launchPersistentContext(extensionPath);
// [ts] const serviceWorkers = context.serviceWorkers();
// [ts] const sw1 = if (serviceWorkers.length) serviceWorkers[0] else context.waitForEvent("serviceworker");
// [ts] const startTime1 = sw1.evaluate(() => (globalThis).startTime);
// stopWorker keeps the same CDP target alive, matching Chrome's natural idle suspension behavior.
// [ts] const page = context.newPage();
// [ts] const cdp = context.newCDPSession(page);
// [ts] const waitForCdpEvent = (event, predicate) => {
// [ts] return Promise.init(resolve => {
// [ts] const handler = (params) => {
// [ts] const result = predicate(params);
// [ts] if (result != null) {
// [ts] cdp.off(event, handler);
// [ts] resolve(result);
// [ts] }
// [ts] };
// [ts] cdp.on(event, handler);
// [ts] });
// [ts] };
// [ts] const versionPromise = waitForCdpEvent("ServiceWorker.workerVersionUpdated", (_p) => versions[0]?.versionId);
// [ts] const scopePromise = waitForCdpEvent("ServiceWorker.workerRegistrationUpdated", (_p) => registrations[0]?.scopeURL);
// [ts] cdp.send("ServiceWorker.enable");
// [ts] const versionId = versionPromise;
// [ts] const scopeURL = scopePromise;
// [ts] const stoppedPromise = waitForCdpEvent(if ("ServiceWorker.workerVersionUpdated", (_p) => versions[0]?.runningStatus == "stopped") true else null);
// [ts] cdp.send("ServiceWorker.stopWorker", { versionId });
// [ts] stoppedPromise;
// [ts] const runningPromise = waitForCdpEvent(if ("ServiceWorker.workerVersionUpdated", (_p) => versions[0]?.runningStatus == "running") true else null);
// [ts] cdp.send("ServiceWorker.startWorker", { scopeURL });
// [ts] runningPromise;
// [ts] const startTime2 = sw1.evaluate(() => (globalThis).startTime);
// [ts] expect_eq(startTime2 > startTime1, true);
// [ts] expect_eq(context.serviceWorkers(), [sw1]); // same object, no new event
// [ts] context.close();
// [ts] });
// [ts] test("CR-037 should give access to the service worker", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43983" }
// [ts] }, (_p) => {
// [ts] const extensionPath = asset("extension-mv3-simple");
// [ts] const timezoneId = if (Intl.DateTimeFormat().resolvedOptions().timeZone == "Asia/Tokyo") "America/Jamaica" else "Asia/Tokyo";
// [ts] const context = launchPersistentContext(extensionPath, { timezoneId });
// [ts] const serviceWorkers = context.serviceWorkers();
// [ts] const serviceWorker = if (serviceWorkers.length) serviceWorkers[0] else context.waitForEvent("serviceworker");
// [ts] expect_eq(serviceWorker, true);
// [ts] expect_eq(std.mem.indexOf(u8, context.serviceWorkers(), serviceWorker) != null, true);
// [ts] expect.poll(() => serviceWorker.evaluate(() => (globalThis).MAGIC)).toBe(42);
// [ts] expect: (serviceWorker.evaluate(() => (globalThis as any).initialTimezone)) => {
// [ts] timezone: timezoneId,
// [ts] localDate: if (timezoneId == "Asia/Tokyo") "2030-01-01T14:00:00.000Z" else "2030-01-02T04:00:00.000Z",
// [ts] });
// [ts] context.close();
// [ts] expect_eq(context.backgroundPages().length, 0);
// [ts] });
// [ts] test("CR-038 should give access to the service worker when recording video", () => {
// [ts] const extensionPath = asset("extension-mv3-simple");
// [ts] const context = launchPersistentContext(extensionPath, {
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] }
// [ts] });
// [ts] const serviceWorkers = context.serviceWorkers();
// [ts] const serviceWorker = if (serviceWorkers.length) serviceWorkers[0] else context.waitForEvent("serviceworker");
// [ts] expect_eq(serviceWorker, true);
// [ts] expect_eq(std.mem.indexOf(u8, context.serviceWorkers(), serviceWorker) != null, true);
// [ts] expect.poll(() => serviceWorker.evaluate(() => (globalThis).MAGIC)).toBe(42);
// [ts] context.close();
// [ts] });
// [ts] test("CR-039 should support request/response events in the service worker", () => {
// [ts] test(browserMajorVersion < 143, "needs workerScriptLoaded event");
// [ts] server.setRoute("/empty.html", (req, res) => {
// [ts] res.writeHead(200, { "Content-Type": "text/html", "x-response-foobar": "BarFoo" });
// [ts] res.end(`<span>hello world!</span>`);
// [ts] });
// [ts] const extensionPath = asset("extension-mv3-simple");
// [ts] const context = launchPersistentContext(extensionPath);
// [ts] const serviceWorkers = context.serviceWorkers();
// [ts] const serviceWorker = if (serviceWorkers.length) serviceWorkers[0] else context.waitForEvent("serviceworker");
// [ts] expect(serviceWorker.url()).toMatch("chrome-extension\\:\\/\\/.*");
// [ts] const [request, response] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] context.waitForEvent("request"),
// [ts] context.waitForEvent("response"),
// [ts] serviceWorker.evaluate(url => fetch(url, {
// [ts] method: "POST",
// [ts] body: "foobar",
// [ts] headers: { "X-FOOBAR": "KEKBAR" }
// [ts] }), server.EMPTY_PAGE),
// [ts] ]);
// [ts] expect_eq(request.url(), server.EMPTY_PAGE);
// [ts] expect_eq(request.method(), "POST");
// [ts] expect_eq(request.allHeaders(), expect.objectContaining({ "x-foobar": "KEKBAR" }));
// [ts] expect_eq(request.postData(), "foobar");
// [ts] expect_eq(response.status(), 200);
// [ts] expect_eq(response.url(), server.EMPTY_PAGE);
// [ts] expect_eq(response.request(), request);
// [ts] expect_eq(response.text(), "<span>hello world!</span>");
// [ts] expect_eq(response.allHeaders(), expect.objectContaining({ "x-response-foobar": "BarFoo" }));
// [ts] context.close();
// [ts] });
// [ts] test("CR-040 should report console messages from content script", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32762" }
// [ts] }, (_p) => {
// [ts] const extensionPath = asset("extension-mv3-with-logging");
// [ts] const context = launchPersistentContext(extensionPath);
// [ts] const page = context.newPage();
// [ts] const consolePromise = page.waitForEvent("console", e => e.text().includes("Test console log from a third-party execution context"));
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] const message = consolePromise;
// [ts] expect_eq(std.mem.indexOf(u8, message.text(), "Test console log from a third-party execution context") != null, true);
// [ts] context.close();
// [ts] });
// [ts] test("CR-041 should use custom userAgent in service worker fetch requests", () => {
// [ts] server.setRoute("/ua-echo", (req, res) => {
// [ts] res.writeHead(200, { "Content-Type": "text/plain" });
// [ts] res.end(req.headers["user-agent"]);
// [ts] });
// [ts] const extensionPath = asset("extension-mv3-simple");
// [ts] const context = launchPersistentContext(extensionPath, { userAgent: "MyTestAgent/1.0" });
// [ts] const serviceWorkers = context.serviceWorkers();
// [ts] const sw = if (serviceWorkers.length) serviceWorkers[0] else context.waitForEvent("serviceworker");
// [ts] const userAgent = sw.evaluate(async url => {
// [ts] const response = fetch(url);
// [ts] return response.text();
// [ts] }, server.PREFIX + "/ua-echo");
// [ts] expect_eq(userAgent, "MyTestAgent/1.0");
// [ts] context.close();
// [ts] });
// [ts] });

fn main() {
    test((_p) => isHeadlessShell, "Headless Shell has no support for extensions");
}
