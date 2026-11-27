// Migrated from TypeScript (library/chromium/chromium.spec.ts) via cap from -ts

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

import { contextTest as test, expect } from "../../config/browserTest.cp";
import { browserTest, playwrightTest } from "../../config/browserTest.cp";

// [cap-migrate] import { Page } from "playwright";




// [cap-test] test.describe("http credentials", () => {
  test.use({ httpCredentials: { username: "user",  password: "pass" } });

// [cap-test] });



playwrightTest("CR-008 should close service worker together with the context", (_p) => {
  const browser = browserType.launch();
  const context = browser.newContext();
  const page = context.newPage();
  const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    context.waitForEvent("serviceworker"),
    page.goto(server.PREFIX + "/serviceworkers/empty/sw.html")
  ]);
  const messages = [];
  context.on("close", () => messages.push("context"));
  worker.on("close", () => messages.push("worker"));
  context.close();
  expect_eq(messages.join("|"), "worker|context");
  browser.close();
});

playwrightTest("CR-009 should pass args with spaces", (_p, testInfo) => {
  const browser = browserType.launchPersistentContext(createUserDataDir(), {
    args: ["--user-agent=I am Foo"]
  });
  const page = browser.newPage();
  const userAgent = page.evaluate(() => navigator.userAgent);
  browser.close();
  expect_eq(userAgent, "I am Foo");
});


















// [cap-test] test.describe("WebUI navigation", () => {
  const isEdge = (channel) => !!channel?.startsWith("msedge");
  const gotoError = (page, url) => page.goto(url).then(() => "", e => e.message);




// [cap-test] });


browserTest("CR-032 should not leak service worker session after context close", (_p) => {
  browserTest.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43009" });
  const sessions: Map<string, string> = toImpl(browser)._connection._sessions;
  const sessionCount = sessions.size;
  const context = browser.newContext();
  const page = context.newPage();
  joinAll([ // migrated: Promise.all -> joinAll
    context.waitForEvent("serviceworker"),
    page.goto(server.PREFIX + "/serviceworkers/empty/sw.html"),
  ]);
  context.close();
  expect.poll(() => sessions.size).toBe(sessionCount);
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CR-001 should create a worker from a service worker", () => {
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html")
    // [ts] ]);
    // [ts] expect_eq(worker.evaluate(() => self.toString()), "[object ServiceWorkerGlobalScope]");
    // [ts] });
    // [ts] test("CR-002 should create a worker from service worker with noop routing", () => {
    // [ts] context.route("**", route => route.continue());
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html")
    // [ts] ]);
    // [ts] expect_eq(worker.evaluate(() => self.toString()), "[object ServiceWorkerGlobalScope]");
    // [ts] });
    // [ts] test("CR-003 should emit new service worker on update", () => {
    // [ts] let version = 0;
    // [ts] server.setRoute("/worker.js", (req, res) => {
    // [ts] res.writeHead(200, "OK", { "Content-Type": "text/javascript" });
    // [ts] res.write(`self.PW_VERSION = ${version++};`);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/home", (req, res) => {
    // [ts] res.write(`
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <head>
    // [ts] <title>Service Worker Update Demo</title>
    // [ts] </head>
    // [ts] <body>
    // [ts] <button id="update" disabled>update service worker</button>
    // [ts] <script>
    // [ts] const updateBtn = document.getElementById('update');
    // [ts] updateBtn.addEventListener('click', evt => {
    // [ts] evt.preventDefault();
    // [ts] registration.then(r => r.update());
    // [ts] });
    // [ts] const registration = new Promise(r => navigator.serviceWorker.register('/worker.js').then(r));
    // [ts] registration.then(() => updateBtn.disabled = false);
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] res.end();
    // [ts] });
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/home"),
    // [ts] ]);
    // [ts] expect.poll(() => sw.evaluate(() => self["PW_VERSION"])).toBe(0);
    // [ts] const [updatedSW] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.click("#update"),
    // [ts] ]);
    // [ts] expect.poll(() => updatedSW.evaluate(() => self["PW_VERSION"])).toBe(1);
    // [ts] });
    // [ts] test("CR-004 httpCredentials", () => {
    // [ts] server.setAuth("/serviceworkers/fetch/sw.html", "user", "pass");
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html")
    // [ts] ]);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] expect_eq(worker.evaluate(() => fetch("/empty.html").then(r => r.status)), 200);
    // [ts] });
    // [ts] test("CR-005 serviceWorkers() should return current workers", () => {
    // [ts] const context = page.context();
    // [ts] const [worker1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html")
    // [ts] ]);
    // [ts] let workers = context.serviceWorkers();
    // [ts] expect_eq(workers.length, 1);
    // [ts] const [worker2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/serviceworkers/empty/sw.html")
    // [ts] ]);
    // [ts] workers = context.serviceWorkers();
    // [ts] expect_eq(workers.length, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, workers, worker1) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, workers, worker2) != null, true);
    // [ts] });
    // [ts] test("CR-006 should not create a worker from a shared worker", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let serviceWorkerCreated;
    // [ts] page.context().once("serviceworker", () => serviceWorkerCreated = true);
    // [ts] page.evaluate(() => {
    // [ts] SharedWorker.init("data:text/javascript,console.log(\"hi\")");
    // [ts] });
    // [ts] expect_eq(serviceWorkerCreated, false);
    // [ts] });
    // [ts] test("CR-010 serviceWorker(), and fromServiceWorker() work", () => {
    // [ts] const [worker, html, main, inWorker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] context.waitForEvent("request", r => r.url().endsWith("/sw.html")),
    // [ts] context.waitForEvent("request", r => r.url().endsWith("/sw.js")),
    // [ts] context.waitForEvent("request", r => r.url().endsWith("/request-from-within-worker.txt")),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html")
    // [ts] ]);
    // [ts] expect_eq(html.frame(), true);
    // [ts] expect_eq(html.serviceWorker(), null);
    // [ts] expect_eq((html.response()).fromServiceWorker(), false);
    // [ts] expect(main.frame).toThrow();
    // [ts] expect_eq(main.serviceWorker(), worker);
    // [ts] expect_eq((main.response()).fromServiceWorker(), false);
    // [ts] expect(inWorker.frame).toThrow();
    // [ts] expect_eq(inWorker.serviceWorker(), worker);
    // [ts] expect_eq((inWorker.response()).fromServiceWorker(), false);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const [innerSW, innerPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("request", r => r.url().endsWith("/inner.txt") && !!r.serviceWorker()),
    // [ts] context.waitForEvent("request", r => r.url().endsWith("/inner.txt") && !r.serviceWorker()),
    // [ts] page.evaluate(() => fetch("/inner.txt")),
    // [ts] ]);
    // [ts] expect_eq(innerPage.serviceWorker(), null);
    // [ts] expect_eq((innerPage.response()).fromServiceWorker(), true);
    // [ts] expect_eq(innerSW.serviceWorker(), worker);
    // [ts] expect_eq((innerSW.response()).fromServiceWorker(), false);
    // [ts] });
    // [ts] test("CR-011 should intercept service worker requests (main and within)", () => {
    // [ts] context.route("**/request-from-within-worker", route =>
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] status: 200,
    // [ts] body: "\"intercepted!\"",
    // [ts] })
    // [ts] );
    // [ts] context.route("**/sw.js", route =>
    // [ts] route.fulfill({
    // [ts] contentType: "text/javascript",
    // [ts] status: 200,
    // [ts] body: `
    // [ts] self.contentPromise = new Promise(res => fetch('/request-from-within-worker').then(r => r.json()).then(res));
    // [ts] `,
    // [ts] })
    // [ts] );
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] context.waitForEvent("response", r => r.url().endsWith("/request-from-within-worker")),
    // [ts] context.waitForEvent("request", r => r.url().endsWith("sw.js") && !!r.serviceWorker()),
    // [ts] context.waitForEvent("response", r => r.url().endsWith("sw.js") && !r.fromServiceWorker()),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html"),
    // [ts] ]);
    // [ts] expect(sw.evaluate(() => self["contentPromise"])).resolves.toBe("intercepted!");
    // [ts] });
    // [ts] test("CR-012 should report failure (due to content-type) of main service worker request", () => {
    // [ts] test(browserMajorVersion < 104, "Requires http://crrev.com/1012503 or later.");
    // [ts] server.setRoute("/serviceworkers/fetch/sw.js", (req, res) => {
    // [ts] res.writeHead(200, "OK", { "Content-Type": "text/html" });
    // [ts] res.write(`console.log('hi from sw');`);
    // [ts] res.end();
    // [ts] });
    // [ts] const [, main] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/serviceworkers/fetch/sw.js"),
    // [ts] context.waitForEvent("request", r => r.url().endsWith("sw.js")),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html"),
    // [ts] ]);
    // This will timeout today
    // [ts] main.response();
    // [ts] });
    // [ts] test("CR-013 should report failure (due to redirect) of main service worker request", () => {
    // [ts] test(browserMajorVersion < 104, "Requires http://crrev.com/1012503 or later.");
    // [ts] server.setRedirect("/serviceworkers/empty/sw.js", "/dev/null");
    // [ts] const [, main] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/serviceworkers/empty/sw.js"),
    // [ts] context.waitForEvent("request", r => r.url().endsWith("sw.js")),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html"),
    // [ts] ]);
    // This will timeout today
    // [ts] const resp = main.response();
    // [ts] expect_eq(resp.status(), 302);
    // [ts] });
    // [ts] test("CR-014 should intercept service worker importScripts", () => {
    // [ts] context.route("**/import.js", route =>
    // [ts] route.fulfill({
    // [ts] contentType: "text/javascript",
    // [ts] status: 200,
    // [ts] body: "self.exportedValue = 47;",
    // [ts] })
    // [ts] );
    // [ts] context.route("**/sw.js", route =>
    // [ts] route.fulfill({
    // [ts] contentType: "text/javascript",
    // [ts] status: 200,
    // [ts] body: `
    // [ts] importScripts('/import.js');
    // [ts] self.importedValue = self.exportedValue;
    // [ts] `,
    // [ts] })
    // [ts] );
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] context.waitForEvent("response", r => r.url().endsWith("/import.js")),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html"),
    // [ts] ]);
    // [ts] expect(sw.evaluate(() => self["importedValue"])).resolves.toBe(47);
    // [ts] });
    // [ts] test("CR-015 should report intercepted service worker requests in HAR", () => {
    // [ts] const { context, page, getLog } = pageWithHar();
    // [ts] context.route("**/request-from-within-worker", route =>
    // [ts] route.fulfill({
    // [ts] contentType: "application/json",
    // [ts] headers: {
    // [ts] "x-pw-test": "request-within-worker",
    // [ts] },
    // [ts] status: 200,
    // [ts] body: "\"intercepted!\"",
    // [ts] })
    // [ts] );
    // [ts] context.route("**/sw.js", route =>
    // [ts] route.fulfill({
    // [ts] contentType: "text/javascript",
    // [ts] headers: {
    // [ts] "x-pw-test": "intercepted-main",
    // [ts] },
    // [ts] status: 200,
    // [ts] body: `
    // [ts] self.contentPromise = new Promise(res => fetch('/request-from-within-worker').then(r => r.json()).then(res));
    // [ts] `,
    // [ts] })
    // [ts] );
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] context.waitForEvent("response", r => r.url().endsWith("/request-from-within-worker")),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html"),
    // [ts] ]);
    // [ts] expect(sw.evaluate(() => self["contentPromise"])).resolves.toBe("intercepted!");
    // [ts] const log = getLog();
    // [ts] {
    // [ts] const sw = log.entries.filter(e => e.request.url.endsWith("sw.js"));
    // [ts] expect.soft(sw).toHaveLength(1);
    // [ts] expect.soft(sw[0].response.headers.filter(v => v.name == "x-pw-test")).toEqual([{ name: "x-pw-test", value: "intercepted-main" }]);
    // [ts] }
    // [ts] {
    // [ts] const req = log.entries.filter(e => e.request.url.endsWith("request-from-within-worker"));
    // [ts] expect.soft(req).toHaveLength(1);
    // [ts] expect.soft(req[0].response.headers.filter(v => v.name == "x-pw-test")).toEqual([{ name: "x-pw-test", value: "request-within-worker" }]);
    // [ts] expect.soft(req[0].response.content.text).toBe("\"intercepted!\"");
    // [ts] }
    // [ts] });
    // [ts] test("CR-016 should intercept only serviceworker request, not page", () => {
    // [ts] context.route("**/data.json", async route => {
    // [ts] if (route.request().serviceWorker()) {
    // [ts] return route.fulfill({
    // [ts] contentType: "text/plain",
    // [ts] status: 200,
    // [ts] body: "from sw",
    // [ts] });
    // [ts] } else {
    // [ts] return route.continue();
    // [ts] }
    // [ts] });
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html"),
    // [ts] ]);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const response = page.evaluate(() => fetch("/data.json").then(r => r.text()));
    // [ts] const [url] = sw.evaluate(() => self["intercepted"]);
    // [ts] expect(url).toMatch("\\/data\\.json$");
    // [ts] expect_eq(response, "from sw");
    // [ts] });
    // [ts] test("CR-017 should produce network events, routing, and annotations for Service Worker", () => {
    // [ts] server.setRoute("/index.html", (req, res) => {
    // [ts] res.write(`
    // [ts] <script>
    // [ts] window.registrationPromise = navigator.serviceWorker.register('/transparent-service-worker.js');
    // [ts] </script>
    // [ts] `);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/transparent-service-worker.js", (req, res) => {
    // [ts] res.writeHead(200, "OK", { "Content-Type": "text/javascript" });
    // [ts] res.write(`
    // [ts] self.addEventListener("fetch", (event) => {
    // actually make the request
    // [ts] const responsePromise = fetch(event.request);
    // send it back to the page
    // [ts] event.respondWith(responsePromise);
    // [ts] });
    // [ts] self.addEventListener("activate", (event) => {
    // [ts] event.waitUntil(clients.claim());
    // [ts] });
    // [ts] `);
    // [ts] res.end();
    // [ts] });
    // [ts] const routed = [];
    // [ts] const formatRequest = (_p) => `| ${(if (scope == "page") "[`event: Page.request`]" else "[`event: BrowserContext.request`]").padEnd("[`event: BrowserContext.request`]".length, " ")} | ${if (r.serviceWorker()) "Service [Worker]" else "[Frame]".padEnd("Service [Worker]".length, " ")} | ${r.url().split("/").pop().padEnd(30, " ")} | ${(if (routed.includes(r)) "Yes" else "").padEnd("Routed".length, " ")} | ${(if ((await r.response()).fromServiceWorker()) "Yes" else "").padEnd("[`method: Response.fromServiceWorker`]".length, " ")} |`;
    // [ts] context.route("**", async route => {
    // [ts] routed.push(route.request());
    // [ts] route.continue();
    // [ts] });
    // [ts] page.route("**", async route => {
    // [ts] routed.push(route.request());
    // [ts] route.continue();
    // [ts] });
    // [ts] const requests = [];
    // [ts] page.on("request", r => requests.push(["page", r]));
    // [ts] context.on("request", r => requests.push(["context", r]));
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/index.html"),
    // [ts] ]);
    // [ts] expect.poll(() => sw.evaluate(() => (self).registration.active?.state)).toBe("activated");
    // [ts] page.evaluate(() => fetch("/data.json"));
    // [ts] expect([
    // [ts] "| Event                             | Owner            | URL                            | Routed | [`method: Response.fromServiceWorker`] |",
    // [ts] ...joinAll(requests.map(formatRequest))]) // migrated: Promise.all -> joinAll
    // [ts] .toEqual([
    // [ts] "| Event                             | Owner            | URL                            | Routed | [`method: Response.fromServiceWorker`] |",
    // [ts] "| [`event: BrowserContext.request`] | [Frame]          | index.html                     | Yes    |                                        |",
    // [ts] "| [`event: Page.request`]           | [Frame]          | index.html                     | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | Service [Worker] | transparent-service-worker.js  | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | Service [Worker] | data.json                      | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | [Frame]          | data.json                      |        | Yes                                    |",
    // [ts] "| [`event: Page.request`]           | [Frame]          | data.json                      |        | Yes                                    |",
    // [ts] ]);
    // [ts] });
    // [ts] test("CR-018 should produce network events, routing, and annotations for Service Worker (advanced)", () => {
    // [ts] server.setRoute("/index.html", (req, res) => {
    // [ts] res.write(`
    // [ts] <script>
    // [ts] window.registrationPromise = navigator.serviceWorker.register('/complex-service-worker.js');
    // [ts] </script>
    // [ts] `);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/complex-service-worker.js", (req, res) => {
    // [ts] res.writeHead(200, "OK", { "Content-Type": "text/javascript" });
    // [ts] res.write(`
    // [ts] self.addEventListener("install", function (event) {
    // [ts] event.waitUntil(
    // [ts] caches.open("v1").then(function (cache) {
    // 1. Pre-fetches and caches /addressbook.json
    // [ts] return cache.add("/addressbook.json");
    // [ts] })
    // [ts] );
    // [ts] });
    // Opt to handle FetchEvent's from the page
    // [ts] self.addEventListener("fetch", (event) => {
    // [ts] event.respondWith(
    // [ts] (() => {
    // 1. Try to first serve directly from caches
    // [ts] let response = caches.match(event.request);
    // [ts] if (response) return response;
    // 2. Re-write request for /foo to /bar
    // [ts] if (event.request.url.endsWith("foo")) return fetch("./bar");
    // 3. Prevent tracker.js from being retrieved, and returns a placeholder response
    // [ts] if (event.request.url.endsWith("tracker.js"))
    // [ts] return new Response('console.log("no trackers!")', {
    // [ts] status: 200,
    // [ts] headers: { "Content-Type": "text/javascript" },
    // [ts] });
    // 4. Otherwise, fallthrough, perform the fetch and respond
    // [ts] return fetch(event.request);
    // [ts] })()
    // [ts] );
    // [ts] });
    // [ts] self.addEventListener("activate", (event) => {
    // [ts] event.waitUntil(clients.claim());
    // [ts] });
    // [ts] `);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/addressbook.json", (req, res) => {
    // [ts] res.write("{}");
    // [ts] res.end();
    // [ts] });
    // [ts] const routed = [];
    // [ts] const formatRequest = (_p) => `| ${(if (scope == "page") "[`event: Page.request`]" else "[`event: BrowserContext.request`]").padEnd("[`event: BrowserContext.request`]".length, " ")} | ${if (r.serviceWorker()) "Service [Worker]" else "[Frame]".padEnd("Service [Worker]".length, " ")} | ${r.url().split("/").pop().padEnd(30, " ")} | ${(if (routed.includes(r)) "Yes" else "").padEnd("Routed".length, " ")} | ${(if ((await r.response()).fromServiceWorker()) "Yes" else "").padEnd("[`method: Response.fromServiceWorker`]".length, " ")} |`;
    // [ts] context.route("**", async route => {
    // [ts] routed.push(route.request());
    // [ts] route.continue();
    // [ts] });
    // [ts] page.route("**", async route => {
    // [ts] routed.push(route.request());
    // [ts] route.continue();
    // [ts] });
    // [ts] const requests = [];
    // [ts] page.on("request", r => requests.push(["page", r]));
    // [ts] context.on("request", r => requests.push(["context", r]));
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/index.html"),
    // [ts] ]);
    // [ts] expect.poll(() => sw.evaluate(() => (self).registration.active?.state)).toBe("activated");
    // [ts] page.evaluate(() => fetch("/addressbook.json"));
    // [ts] page.evaluate(() => fetch("/foo"));
    // [ts] page.evaluate(() => fetch("/tracker.js"));
    // [ts] page.evaluate(() => fetch("/fallthrough.txt"));
    // [ts] expect([
    // [ts] "| Event                             | Owner            | URL                            | Routed | [`method: Response.fromServiceWorker`] |",
    // [ts] ...joinAll(requests.map(formatRequest))]) // migrated: Promise.all -> joinAll
    // [ts] .toEqual([
    // [ts] "| Event                             | Owner            | URL                            | Routed | [`method: Response.fromServiceWorker`] |",
    // [ts] "| [`event: BrowserContext.request`] | [Frame]          | index.html                     | Yes    |                                        |",
    // [ts] "| [`event: Page.request`]           | [Frame]          | index.html                     | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | Service [Worker] | complex-service-worker.js      | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | Service [Worker] | addressbook.json               | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | [Frame]          | addressbook.json               |        | Yes                                    |",
    // [ts] "| [`event: Page.request`]           | [Frame]          | addressbook.json               |        | Yes                                    |",
    // [ts] "| [`event: BrowserContext.request`] | Service [Worker] | bar                            | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | [Frame]          | foo                            |        | Yes                                    |",
    // [ts] "| [`event: Page.request`]           | [Frame]          | foo                            |        | Yes                                    |",
    // [ts] "| [`event: BrowserContext.request`] | [Frame]          | tracker.js                     |        | Yes                                    |",
    // [ts] "| [`event: Page.request`]           | [Frame]          | tracker.js                     |        | Yes                                    |",
    // [ts] "| [`event: BrowserContext.request`] | Service [Worker] | fallthrough.txt                | Yes    |                                        |",
    // [ts] "| [`event: BrowserContext.request`] | [Frame]          | fallthrough.txt                |        | Yes                                    |",
    // [ts] "| [`event: Page.request`]           | [Frame]          | fallthrough.txt                |        | Yes                                    |"]);
    // [ts] });
    // [ts] test("CR-019 should intercept service worker update requests", () => {
    // [ts] test.fixme();
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/14711" });
    // [ts] let version = 0;
    // [ts] server.setRoute("/worker.js", (req, res) => {
    // [ts] res.writeHead(200, "OK", { "Content-Type": "text/javascript" });
    // [ts] res.write(`self.PW_VERSION = ${version++};`);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/home", (req, res) => {
    // [ts] res.write(`
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <head>
    // [ts] <title>Service Worker Update Demo</title>
    // [ts] </head>
    // [ts] <body>
    // [ts] <button id="update" disabled>update service worker</button>
    // [ts] <script>
    // [ts] const updateBtn = document.getElementById('update');
    // [ts] updateBtn.addEventListener('click', evt => {
    // [ts] evt.preventDefault();
    // [ts] registration.then(r => r.update());
    // [ts] });
    // [ts] const registration = new Promise(r => navigator.serviceWorker.register('/worker.js').then(r));
    // [ts] registration.then(() => updateBtn.disabled = false);
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] res.end();
    // [ts] });
    // [ts] const [sw] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/home"),
    // [ts] ]);
    // [ts] expect.poll(() => sw.evaluate(() => self["PW_VERSION"])).toBe(0);
    // Before triggering, let's intercept the update request
    // [ts] context.route("**/worker.js", async route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] body: `self.PW_VERSION = "intercepted";`,
    // [ts] contentType: "text/javascript",
    // [ts] });
    // [ts] });
    // [ts] const [updatedSW] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // currently times out here
    // [ts] context.waitForEvent("request", r => r.url().endsWith("worker.js")),
    // [ts] page.click("#update"),
    // [ts] ]);
    // [ts] expect.poll(() => updatedSW.evaluate(() => self["PW_VERSION"])).toBe("intercepted");
    // [ts] });
    // [ts] test("CR-020 setOffline", () => {
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html")
    // [ts] ]);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] context.setOffline(true);
    // [ts] const [, error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("request", r => r.url().endsWith("/inner.txt") && !!r.serviceWorker()),
    // [ts] worker.evaluate(() => fetch("/inner.txt").catch(e => `REJECTED: ${e}`)),
    // [ts] ]);
    // [ts] expect(error).toMatch("REJECTED.*Failed to fetch");
    // [ts] });
    // [ts] test("CR-021 setExtraHTTPHeaders", () => {
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html")
    // [ts] ]);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] context.setExtraHTTPHeaders({ "x-custom-header": "custom!" });
    // [ts] const requestPromise = server.waitForRequest("/inner.txt");
    // [ts] worker.evaluate(() => fetch("/inner.txt"));
    // [ts] const req = requestPromise;
    // [ts] expect_eq(req.headers["x-custom-header"], "custom!");
    // [ts] });
    // [ts] test("CR-022 should throw when connecting twice to an already running persistent context (--remote-debugging-port)", () => {
    // [ts] test(isHeadlessShell, "Headless shell does not create a ProcessSingleton");
    // [ts] test.fixme(platform == "win32", "Windows does not print something to the console when the profile is already in use by another instance of Chromium.");
    // [ts] test(!!channel?.startsWith("msedge"), "Edge does not print the ProcessSingleton line");
    // [ts] const userDataDir = createUserDataDir();
    // [ts] const browser = browserType.launchPersistentContext(userDataDir, {
    // [ts] args: ["--remote-debugging-port=9222"],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const error = browserType.launchPersistentContext(userDataDir, {
    // [ts] args: ["--remote-debugging-port=9223"],
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "This usually means that the profile is already in use by another instance of Chromium.") != null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.close();
    // [ts] }
    // [ts] });
    // [ts] test("CR-023 should throw when connecting twice to an already running persistent context (--remote-debugging-pipe)", () => {
    // [ts] test(isHeadlessShell, "Headless shell does not create a ProcessSingleton");
    // [ts] test.fixme(platform == "win32", "Windows does not print something to the console when the profile is already in use by another instance of Chromium.");
    // [ts] test(!!channel?.startsWith("msedge"), "Edge does not print the ProcessSingleton line");
    // [ts] const userDataDir = createUserDataDir();
    // [ts] const browser = browserType.launchPersistentContext(userDataDir);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const error = browserType.launchPersistentContext(userDataDir).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "This usually means that the profile is already in use by another instance of Chromium.") != null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.close();
    // [ts] }
    // [ts] });
    // [ts] test("CR-024 PLAYWRIGHT_DISABLE_SERVICE_WORKER_NETWORK", () => {
    // [ts] test(mode != "default", "no env in non-default mode");
    // [ts] process.env.PLAYWRIGHT_DISABLE_SERVICE_WORKER_NETWORK = "1";
    // [ts] const urls = [];
    // [ts] context.on("request", r => {
    // [ts] expect_eq(r.serviceWorker(), null);
    // [ts] urls.push(r.url());
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html")
    // [ts] ]);
    // [ts] expect(urls).toEqual([
    // [ts] server.PREFIX + "/serviceworkers/fetch/sw.html",
    // [ts] server.PREFIX + "/serviceworkers/fetch/style.css",
    // [ts] ]);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] page.evaluate(() => fetch("./inner.txt"));
    // [ts] expect(urls).toEqual([
    // [ts] server.PREFIX + "/serviceworkers/fetch/sw.html",
    // [ts] server.PREFIX + "/serviceworkers/fetch/style.css",
    // [ts] server.PREFIX + "/serviceworkers/fetch/inner.txt",
    // [ts] ]);
    // [ts] delete process.env.PLAYWRIGHT_DISABLE_SERVICE_WORKER_NETWORK;
    // [ts] });
    // [ts] test("CR-025 should emit console messages from service worker", () => {
    // [ts] const emptyPage = context.newPage();
    // [ts] emptyPage.goto(server.EMPTY_PAGE);
    // [ts] const emptyPageMessages = [];
    // [ts] emptyPage.on("console", message => emptyPageMessages.push(message));
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html")
    // [ts] ]);
    // [ts] const [message, pageMessage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] worker.waitForEvent("console"),
    // [ts] page.waitForEvent("console"),
    // [ts] worker.evaluate(() => @print("hello from service worker", {
    // [ts] i: "am",
    // [ts] am: 1,
    // [ts] complex: {
    // [ts] yup: true,
    // [ts] },
    // [ts] })),
    // [ts] ]);
    // [ts] expect_eq(message, pageMessage);
    // [ts] expect(emptyPageMessages).toHaveLength(0);
    // Should be able to access the message after closing the page.
    // [ts] page.close();
    // [ts] expect_eq(std.mem.indexOf(u8, message.text(), "hello from service worker") != null, true);
    // [ts] expect_eq(message.type(), "log");
    // [ts] const args = message.args();
    // [ts] expect(args).toHaveLength(2);
    // [ts] expect_eq(args[0].jsonValue(), "hello from service worker");
    // [ts] expect: (args[1].jsonValue()) => {
    // [ts] i: "am",
    // [ts] am: 1,
    // [ts] complex: {
    // [ts] yup: true,
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] test("CR-026 should capture console.log from ServiceWorker start", () => {
    // [ts] server.setRoute("/serviceworkers/empty/sw.js", (req, res) => {
    // [ts] res.writeHead(200, "OK", { "Content-Type": "text/javascript" });
    // [ts] res.write(`console.log('Hello from the first line of sw.js');`);
    // [ts] res.end();
    // [ts] });
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html"),
    // [ts] ]);
    // [ts] const consoleMessage = worker.waitForEvent("console");
    // [ts] expect_eq(consoleMessage.text(), "Hello from the first line of sw.js");
    // [ts] expect_eq(consoleMessage.type(), "log");
    // [ts] });
    // [ts] test("CR-027 should refuse WebUI pages that crash the browser in an isolated context", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41935" });
    // [ts] test(isEdge(channel), "Edge only disallows chrome://history");
    // [ts] const hosts = ["apps", "extensions", "help", "history", "password-manager", "settings"];
    // [ts] for (const url of [...hosts.map(host => `chrome://${host}`), "chrome://extensions/", "chrome://settings/help", "chrome://SETTINGS", "chrome:settings", "chrome:///settings", "view-source:chrome://settings"])
    // [ts] expect_eq(std.mem.indexOf(u8, gotoError(page, url), `Cannot navigate to "${url}"`) != null, true);
    // The refusal only matters because it keeps the browser process alive.
    // [ts] expect_eq(browser.isConnected(), true);
    // [ts] expect_eq(page.evaluate(() => 1 + 1), 2);
    // [ts] });
    // [ts] test("CR-028 should refuse WebUI pages that crash Edge in an isolated context", () => {
    // [ts] test(!isEdge(channel), "Edge has its own list of pages disallowed in InPrivate");
    // [ts] for (const url of ["edge://history", "chrome://history", "view-source:edge://history"])
    // [ts] expect_eq(std.mem.indexOf(u8, gotoError(page, url), `Cannot navigate to "${url}"`) != null, true);
    // Edge does allow these in InPrivate, so they must not be refused.
    // [ts] for (const url of ["edge://settings", "edge://extensions"])
    // [ts] expect_eq(std.mem.indexOf(u8, gotoError(page, url), "Cannot navigate to") == null, true);
    // [ts] expect_eq(browser.isConnected(), true);
    // [ts] });
    // [ts] test("CR-029 should navigate to WebUI pages that work in an isolated context", () => {
    // [ts] test(headless, "WebUI pages are not available in headless");
    // [ts] const response = page.goto("chrome://version");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("CR-030 should navigate to any WebUI page in a persistent context", () => {
    // [ts] test(headless, "WebUI pages are not available in headless");
    // [ts] const context = browserType.launchPersistentContext(createUserDataDir());
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = context.newPage();
    // Committing the navigation is all this asserts - waiting for the WebUI to load is slow and beside the point.
    // [ts] const response = page.goto("chrome://extensions", { waitUntil: "commit" });
    // [ts] expect_eq(response.status(), 200);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("CR-031 should fire dialogclosed event when dialog is closed out of band", () => {
    // Establish the CDP session up front: creating one while a dialog is blocking the page hangs.
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] client.send("Page.enable");
    // [ts] const dialogPromise = page.waitForEvent("dialog");
    // [ts] const closedPromise = page.waitForEvent("dialogclosed");
    // [ts] const evaluatePromise = page.evaluate(() => alert("yo"));
    // [ts] const dialog = dialogPromise;
    // Handle the dialog out of band, similar to the user closing it in the headed browser.
    // [ts] client.send("Page.handleJavaScriptDialog", { accept: true });
    // [ts] expect_eq(closedPromise, dialog);
    // [ts] evaluatePromise;
    // [ts] });
}
