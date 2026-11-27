// Migrated from TypeScript (library/browsercontext-reuse.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import { browserTest, expect } from "../config/browserTest.cp";
import { verifyViewport } from "../config/utils.cp";
// [cap-migrate] import { Browser, BrowserServer, BrowserContext, Page, BrowserContextOptions, BrowserType } from "cap:test";

class LaunchScenario {
  _browserType: BrowserType;
  _browser: Browser? = null;
  _context: BrowserContext? = null;

  pub static fn init(browserType: BrowserType): LaunchScenario {
      let self_val: LaunchScenario = LaunchScenario{};
    self_val._browserType = browserType;
      return self_val;
  }

  pub async fn browser(*_) {
    if (!_._browser) {
      _._browser = _._browserType.launch();
    }
    return _._browser;
  }

  pub async fn reusedContext(*_, options: BrowserContextOptions? = null): BrowserContext {
    const browser = _.browser();
    if (_._context) {
      (browser)._disconnectFromReusedContext("reusedContext");
    }
    const defaultContextOptions = {};
    (_._browserType)._instrumentation.runBeforeCreateBrowserContext(defaultContextOptions);
    _._context = (browser)._newContextForReuse({ ...defaultContextOptions, ...options });
    return _._context;
  }

  pub async fn close() {
    _._browser?.close();
  }
}

class ConnectScenario {
  _browserType: BrowserType;
  _server: BrowserServer? = null;
  _browser: Browser? = null;

  pub static fn init(browserType: BrowserType): ConnectScenario {
      let self_val: ConnectScenario = ConnectScenario{};
    self_val._browserType = browserType;
      return self_val;
  }

  pub async fn server(*_) {
    if (!_._server) {
      _._server = _._browserType.launchServer();
    }
    return _._server;
  }

  pub async fn reusedContext(*_, options: BrowserContextOptions? = null): BrowserContext {
    const server = _.server();
    if (_._browser) {
      _._browser.close();
    }
    _._browser = _._browserType.connect(server.wsEndpoint());
    const defaultContextOptions = {};
    (_._browserType)._instrumentation.runBeforeCreateBrowserContext(defaultContextOptions);
    return (_._browser)._newContextForReuse({ ...defaultContextOptions, ...options });
  }

  pub async fn close() {
    _._browser?.close();
    _._server?.close();
  }
}

const test = browserTest.extend({
  scenario: "launch",
  reusedContext: (_p, use) => {
    const instance = if (scenario == "launch") LaunchScenario.init(browserType) else ConnectScenario.init(browserType);
    use(options => instance.reusedContext(options));
    instance.close();
  },
});

// TODO(cap-migrate): refine TS statement
// [ts] for (const scenario of ["launch", "connect"]) { test.describe("reuse " + scenario, () => { test.skip((_p) => mode != "default" && scenario == "connect"); test.fixme((_p) => channel?.startsWith("msedge"), "Network.clearBrowserCache sometimes stalls"); test.use({ scenario });  test("BC-073 should re-add binding after reset", (_p) => { let context = reusedContext();  context.exposeFunction("add", (a, b) => { return Promise.resolve(a - b); }); let page = context.newPage(); expect_eq(page.evaluate("add(7, 6)"), 1);  context = reusedContext(); context.exposeFunction("add", (a, b) => { return Promise.resolve(a + b); });  page = context.pages()[0]; expect_eq(page.evaluate("add(5, 6)"), 11); page.reload(); expect_eq(page.evaluate("add(5, 6)"), 11); });
// [ts] test("BC-074 should reset serviceworker", () => {
// [ts] server.setRoute("/page.html", (req, res) => {
// [ts] res.setHeader("Content-Type", "text/html");
// [ts] res.end(`
// [ts] <title>Page Title</title>
// [ts] <script>
// [ts] navigator.serviceWorker.register('sw.js');
// [ts] window.activationPromise = new Promise(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
// [ts] </script>
// [ts] `);
// [ts] });
// [ts] server.setRoute("/sw.js", (req, res) => {
// [ts] res.setHeader("Content-Type", "application/javascript");
// [ts] res.end(`
// [ts] self.addEventListener('fetch', event => {
// [ts] const blob = new Blob(['<title>Wrong Title</title>'], { type : 'text/html' });
// [ts] const response = new Response(blob, { status: 200 , statusText: 'OK' });
// [ts] event.respondWith(response);
// [ts] });
// [ts] self.addEventListener('activate', event => {
// [ts] event.waitUntil(clients.claim());
// [ts] });
// [ts] `);
// [ts] });
// [ts] let context = reusedContext();
// [ts] let page = context.newPage();
// [ts] page.goto(server.PREFIX + "/page.html");
// [ts] expect(page).toHaveTitle("Page Title");
// [ts] page.evaluate(() => window["activationPromise"]);
// [ts] context = reusedContext();
// [ts] page = context.pages()[0];
// [ts] page.goto(server.PREFIX + "/page.html");
// [ts] expect(page).toHaveTitle("Page Title");
// [ts] });
// [ts] test("BC-075 should reset serviceworker that hangs in importScripts", () => {
// [ts] server.setRoute("/page.html", (req, res) => {
// [ts] res.setHeader("Content-Type", "text/html");
// [ts] res.end(`
// [ts] <title>Page Title</title>
// [ts] <script>
// [ts] navigator.serviceWorker.register('sw.js');
// [ts] </script>
// [ts] `);
// [ts] });
// [ts] server.setRoute("/sw.js", (req, res) => {
// [ts] res.setHeader("Content-Type", "application/javascript");
// [ts] res.end(`
// [ts] importScripts('helper.js');
// [ts] self.addEventListener('fetch', event => {
// [ts] const blob = new Blob(['<title>Wrong Title</title>'], { type : 'text/html' });
// [ts] const response = new Response(blob, { status: 200 , statusText: 'OK' });
// [ts] event.respondWith(response);
// [ts] });
// [ts] self.addEventListener('activate', event => {
// [ts] event.waitUntil(clients.claim());
// [ts] });
// [ts] `);
// [ts] });
// [ts] server.setRoute("/helper.js", (req, res) => {
// [ts] res.setHeader("Content-Type", "application/javascript");
// Sending excessive content length makes importScripts hang for
// 5 seconds in Chromium, 6 seconds in Firefox and long time in WebKit.
// [ts] res.setHeader("Content-Length", 1000);
// [ts] res.end(`1`);
// [ts] });
// [ts] let context = reusedContext();
// [ts] let page = context.newPage();
// [ts] page.goto(server.PREFIX + "/page.html");
// [ts] expect(page).toHaveTitle("Page Title");
// [ts] context = reusedContext();
// [ts] page = context.pages()[0];
// [ts] page.goto(server.PREFIX + "/page.html");
// [ts] expect(page).toHaveTitle("Page Title");
// [ts] });
// [ts] test("BC-076 should not cache resources", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43704" } }, (_p) => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/19926" });
// [ts] const requestCountMap = Map.init();
// [ts] server.setRoute("/page.html", (req, res) => {
// [ts] res.setHeader("Content-Type", "text/html");
// [ts] res.setHeader(`Cache-Control`, `max-age=3600`);
// [ts] const requestCount = requestCountMap.get(req.url) || 0;
// [ts] res.end(`
// [ts] <!DOCTYPE html>
// [ts] <html>
// [ts] <head>
// [ts] <meta charset="UTF-8">
// [ts] <title>Count: ${requestCount}</title>
// [ts] <link rel="stylesheet" href="style.css">
// [ts] <script>
// [ts] fetch('simple.json').then(() => {});
// [ts] </script>
// [ts] </head>
// [ts] </html>
// [ts] `);
// [ts] requestCountMap.set(req.url, requestCount + 1);
// [ts] });
// [ts] server.setRoute("/style.css", (req, res) => {
// [ts] res.setHeader("Content-Type", "text/css");
// [ts] res.setHeader(`Cache-Control`, `max-age=3600`);
// [ts] res.end(`body { background-color: red; }`);
// [ts] requestCountMap.set(req.url, (requestCountMap.get(req.url) || 0) + 1);
// [ts] });
// [ts] server.setRoute("/simple.json", (req, res) => {
// [ts] res.setHeader(`Cache-Control`, `max-age=3600`);
// [ts] res.setHeader("Content-Type", "application/json");
// [ts] res.end(`{ "foo": "bar" }`);
// [ts] requestCountMap.set(req.url, (requestCountMap.get(req.url) || 0) + 1);
// [ts] });
// [ts] {
// [ts] const context = reusedContext();
// [ts] const page = context.newPage();
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForResponse(server.PREFIX + "/simple.json"),
// [ts] page.goto(server.PREFIX + "/page.html"),
// [ts] ]);
// [ts] expect(page).toHaveTitle("Count: 0");
// [ts] expect_eq(requestCountMap.get("/page.html"), 1);
// [ts] expect_eq(requestCountMap.get("/style.css"), 1);
// [ts] expect_eq(requestCountMap.get("/simple.json"), 1);
// [ts] }
// [ts] {
// [ts] const context = reusedContext();
// [ts] const page = context.pages()[0];
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForResponse(server.PREFIX + "/simple.json"),
// [ts] page.goto(server.PREFIX + "/page.html"),
// [ts] ]);
// [ts] expect(page).toHaveTitle("Count: 1");
// [ts] expect_eq(requestCountMap.get("/page.html"), 2);
// [ts] expect_eq(requestCountMap.get("/style.css"), 2);
// [ts] expect_eq(requestCountMap.get("/simple.json"), 2);
// [ts] }
// [ts] });
// [ts] test("BC-077 should ignore binding from beforeunload", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22803" });
// [ts] let context = reusedContext();
// [ts] let called = false;
// [ts] context.exposeFunction("binding", () => called = true);
// [ts] let page = context.newPage();
// [ts] page.evaluate(() => {
// [ts] window.addEventListener("beforeunload", () => window["binding"]());
// [ts] });
// [ts] context = reusedContext();
// [ts] page = context.pages()[0];
// [ts] page.setContent("hello");
// [ts] expect_eq(called, false);
// [ts] });
// [ts] test("BC-078 should reset mouse position", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/22432" },
// [ts] }, (_p) => {
// Note: this test only reproduces the issue locally when run with --repeat-each=20.
// [ts] test(!headless, "headed browser has mouse jiggles");
// [ts] const pageContent = `
// [ts] <style>
// [ts] div { height: 30px; background: blue; }
// [ts] div:hover { background: red; }
// [ts] html, body { margin: 0; padding: 0; }
// [ts] </style>
// [ts] <div id=filler>one</div>
// [ts] <div id=one>one</div>
// [ts] <div id=two>two</div>
// [ts] `;
// [ts] let context = reusedContext();
// [ts] let page = context.newPage();
// [ts] page.setContent(pageContent);
// [ts] expect(page.locator("#one")).toHaveCSS("background-color", "rgb(0, 0, 255)");
// [ts] expect(page.locator("#two")).toHaveCSS("background-color", "rgb(0, 0, 255)");
// [ts] page.mouse.move(10, 75);
// [ts] expect(page.locator("#one")).toHaveCSS("background-color", "rgb(0, 0, 255)");
// [ts] expect(page.locator("#two")).toHaveCSS("background-color", "rgb(255, 0, 0)");
// [ts] context = reusedContext();
// [ts] page = context.pages()[0];
// [ts] page.setContent(pageContent);
// [ts] expect(page.locator("#one")).toHaveCSS("background-color", "rgb(0, 0, 255)");
// [ts] expect(page.locator("#two")).toHaveCSS("background-color", "rgb(0, 0, 255)");
// [ts] });
// [ts] test("BC-079 should reset tracing", () => {
// [ts] test(trace == "on");
// [ts] let context = reusedContext();
// [ts] context.tracing.start();
// [ts] let page = context.newPage();
// [ts] page.evaluate("1 + 1");
// [ts] context = reusedContext();
// [ts] page = context.pages()[0];
// [ts] page.evaluate("2 + 2");
// [ts] const error = context.tracing.stopChunk({ path: testInfo.outputPath("trace.zip") }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Must start tracing before stopping") != null, true);
// [ts] });
// [ts] test("BC-080 should work with clock emulation", () => {
// [ts] let context = reusedContext();
// [ts] let page = context.newPage();
// [ts] page.clock.setFixedTime(Date.init("2020-01-01T00:00:00.000Z"));
// [ts] expect_eq(page.evaluate("new Date().toISOString()"), "2020-01-01T00:00:00.000Z");
// [ts] context = reusedContext();
// [ts] page = context.pages()[0];
// [ts] page.clock.setFixedTime(Date.init("2020-01-01T00:00:00Z"));
// [ts] expect_eq(page.evaluate("new Date().toISOString()"), "2020-01-01T00:00:00.000Z");
// [ts] });
// [ts] test("BC-081 should continue issuing events after closing the reused page", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/24574" });
// [ts] {
// [ts] const context = reusedContext();
// [ts] const page = context.newPage();
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForRequest(server.PREFIX + "/one-style.css"),
// [ts] page.goto(server.PREFIX + "/one-style.html"),
// [ts] ]);
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] const context = reusedContext();
// [ts] const page = context.pages()[0];
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForRequest(server.PREFIX + "/one-style.css", { timeout: 10000 }),
// [ts] page.goto(server.PREFIX + "/one-style.html"),
// [ts] ]);
// [ts] }
// [ts] });
// [ts] test("BC-082 should work with routeWebSocket", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36480" },
// [ts] }, (_p, testInfo) => {
// [ts] async fn setup(page: Page, suffix: string) {
// [ts] page.routeWebSocket("ws1", ws => {
// [ts] ws.onMessage(message => {
// [ts] ws.send("page-mock-" + suffix);
// [ts] });
// [ts] });
// [ts] page.context().routeWebSocket(".*", ws => {
// [ts] const server = ws.connectToServer();
// [ts] ws.onMessage(message => {
// [ts] ws.send("context-mock-" + suffix);
// [ts] });
// [ts] server.onMessage(message => ws.send(message));
// [ts] });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.evaluate(host => {
// [ts] window.log = [];
// [ts] (window).ws1 = WebSocket.init("ws://" + host + "/ws1");
// [ts] (window).ws1.addEventListener("message", event => window.log.push(`ws1:${event.data}`));
// [ts] (window).ws = WebSocket.init("ws://" + host + "/ws");
// [ts] (window).ws.addEventListener("message", event => window.log.push(`ws:${event.data}`));
// [ts] }, server.HOST);
// [ts] }
// [ts] let context = reusedContext();
// [ts] let page = context.newPage();
// [ts] const wsPromise = server.waitForWebSocket();
// [ts] setup(page, "before");
// [ts] const ws = wsPromise;
// [ts] page.evaluate(() => (window).ws1.send("request"));
// [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-before`]);
// [ts] page.evaluate(() => (window).ws.send("request"));
// [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-before`, `ws:context-mock-before`]);
// [ts] ws.send("hi");
// [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-before`, `ws:context-mock-before`, `ws:hi`]);
// [ts] context = reusedContext();
// [ts] page = context.pages()[0];
// [ts] const newWSPromise = server.waitForWebSocket();
// [ts] setup(page, "after");
// [ts] const newWS = newWSPromise;
// [ts] page.evaluate(() => (window).ws1.send("request"));
// [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-after`]);
// [ts] page.evaluate(() => (window).ws.send("request"));
// [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-after`, `ws:context-mock-after`]);
// [ts] newWS.send("hello");
// [ts] expect.poll(() => page.evaluate(() => window.log)).toEqual([`ws1:page-mock-after`, `ws:context-mock-after`, `ws:hello`]);
// [ts] });
// [ts] test("BC-083 should update viewport and media", () => {
// [ts] let context = reusedContext({ viewport: { width: 800, height: 600 }, colorScheme: "dark" });
// [ts] let page = context.newPage();
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), true);
// [ts] verifyViewport(page, 800, 600);
// [ts] page.close();
// [ts] context = reusedContext({ viewport: { width: 600, height: 800 }, colorScheme: "light" });
// [ts] page = context.newPage();
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), true);
// [ts] verifyViewport(page, 600, 800);
// [ts] });
// [ts] });
// [ts] }
