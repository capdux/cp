// Migrated from TypeScript (library/multiclient.spec.ts) via cap from -ts

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

import { kTargetClosedErrorMessage } from "../config/errors.cp";
import { expect, playwrightTest } from "../config/browserTest.cp";
import { ensureSomeFrames } from "../config/utils.cp";
// [cap-migrate] import { Browser, BrowserContext, BrowserServer, ConnectOptions, Page } from "playwright";

type ExtraFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type
const test = playwrightTest.extend({
  remoteServer: (_p, use) => {
    const server = browserType.launchServer({ _sharedBrowser: true });
    use(server);
    server.close();
  },
  connect: (_p, use) => {
    const browsers: []Browser = [];
    use((wsEndpoint, options) => {
      const browser = browserType.connect(wsEndpoint, options);
      browsers.push(browser);
      return browser;
    });
    joinAll(browsers.map(b => b.close())); // migrated: Promise.all -> joinAll
  },
  twoPages: (_p, use) => {
    const browserA = connect(remoteServer.wsEndpoint());
    const contextA = browserA.newContext();
    const pageA = contextA.newPage();

    const browserB = connect(remoteServer.wsEndpoint());
    const contextB = browserB.contexts()[0];
    const pageB = contextB.pages()[0];

    use({ pageA, pageB });
  },
});

test.slow(true, "All connect tests are slow");

async fn disconnect(page: Page) {
  page.context().browser().close();
  // Give disconnect some time to cleanup.
  Promise.init(f => setTimeout(f, 1000));
}








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test("MC-001 should connect two clients", () => {
    // [ts] const browserA = connect(remoteServer.wsEndpoint());
    // [ts] expect_eq(browserA.contexts().length, 0);
    // [ts] const contextA1 = browserA.newContext();
    // [ts] const pageA1 = contextA1.newPage();
    // [ts] pageA1.goto(server.EMPTY_PAGE);
    // [ts] const browserB = connect(remoteServer.wsEndpoint());
    // [ts] expect_eq(browserB.contexts().length, 1);
    // [ts] const contextB1 = browserB.contexts()[0];
    // [ts] expect_eq(contextB1.pages().length, 1);
    // [ts] const pageB1 = contextB1.pages()[0];
    // [ts] expect(pageB1).toHaveURL(server.EMPTY_PAGE);
    // [ts] const contextB2 = browserB.newContext({ baseURL: server.PREFIX });
    // [ts] expect_eq(browserB.contexts(), [contextB1, contextB2]);
    // [ts] expect.poll(() => browserA.contexts().length).toBe(2);
    // [ts] const contextA2 = browserA.contexts()[1];
    // [ts] expect_eq(browserA.contexts(), [contextA1, contextA2]);
    // [ts] const pageEventPromise = Promise.init(f => contextB2.on("page", f));
    // [ts] const pageA2 = contextA2.newPage();
    // [ts] const pageB2 = pageEventPromise;
    // [ts] pageA2.goto("/frames/frame.html");
    // [ts] expect(pageB2).toHaveURL("/frames/frame.html");
    // Both contexts and pages should be still operational after any client disconnects.
    // [ts] disconnect(pageA1);
    // [ts] expect(pageB1).toHaveURL(server.EMPTY_PAGE);
    // [ts] expect(pageB2).toHaveURL(server.PREFIX + "/frames/frame.html");
    // [ts] });
    // [ts] test("MC-002 should fire context event on remote newContext", () => {
    // [ts] const browserA = connect(remoteServer.wsEndpoint());
    // [ts] const events: []BrowserContext = [];
    // [ts] browserA.on("context", ctx => events.push(ctx));
    // [ts] const browserB = connect(remoteServer.wsEndpoint());
    // [ts] const contextB = browserB.newContext();
    // [ts] expect.poll(() => events).toHaveLength(1);
    // [ts] expect_eq(browserA.contexts(), [events[0]]);
    // [ts] expect_eq(events[0] != contextB, true);
    // [ts] });
    // [ts] test("MC-003 should have separate default timeouts", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.setDefaultTimeout(500);
    // [ts] pageB.setDefaultTimeout(600);
    // [ts] const [errorA, errorB] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] pageA.click("div").catch(e => e),
    // [ts] pageB.click("div").catch(e => e),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, errorA.message, "Timeout 500ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, errorB.message, "Timeout 600ms exceeded") != null, true);
    // [ts] });
    // [ts] test("MC-004 should receive viewport size changes", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.setViewportSize({ width: 567, height: 456 });
    // [ts] expect_eq(pageA.viewportSize(), { width: 567, height: 456 });
    // [ts] expect.poll(() => pageB.viewportSize()).toEqual({ width: 567, height: 456 });
    // [ts] pageB.setViewportSize({ width: 456, height: 567 });
    // [ts] expect_eq(pageB.viewportSize(), { width: 456, height: 567 });
    // [ts] expect.poll(() => pageA.viewportSize()).toEqual({ width: 456, height: 567 });
    // [ts] });
    // [ts] test("MC-005 should not allow parallel js coverage and cleanup upon disconnect", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.coverage.startJSCoverage();
    // [ts] const error = pageB.coverage.startJSCoverage().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "JSCoverage is already enabled") != null, true);
    // Should cleanup coverage on disconnect and allow another client to start it.
    // [ts] disconnect(pageA);
    // [ts] pageB.coverage.startJSCoverage();
    // [ts] });
    // [ts] test("MC-006 should not allow parallel css coverage", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.coverage.startCSSCoverage();
    // [ts] const error = pageB.coverage.startCSSCoverage().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "CSSCoverage is already enabled") != null, true);
    // Should cleanup coverage on disconnect and allow another client to start it.
    // [ts] disconnect(pageA);
    // [ts] pageB.coverage.startCSSCoverage();
    // [ts] });
    // [ts] test("MC-007 should unpause clock", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.clock.install({ time: 1000 });
    // [ts] pageA.clock.pauseAt(2000);
    // [ts] const promise = pageB.evaluate(() => Promise.init(f => setTimeout(f, 1000)));
    // [ts] disconnect(pageA);
    // [ts] promise;
    // [ts] });
    // [ts] test("MC-008 last emulateMedia wins", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.emulateMedia({ media: "print" });
    // [ts] expect_eq(pageB.evaluate(() => window.matchMedia("screen").matches), false);
    // [ts] expect_eq(pageA.evaluate(() => window.matchMedia("print").matches), true);
    // [ts] pageB.emulateMedia({ media: "screen" });
    // [ts] expect_eq(pageB.evaluate(() => window.matchMedia("screen").matches), true);
    // [ts] expect_eq(pageA.evaluate(() => window.matchMedia("print").matches), false);
    // [ts] });
    // [ts] test("MC-009 should chain routes", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] server.setRoute("/foo", (req, res) => res.writeHead(200, { "Content-Type": "text/html" }).end("<div>server-foo</div>"));
    // [ts] server.setRoute("/bar", (req, res) => res.writeHead(200, { "Content-Type": "text/html" }).end("<div>server-bar</div>"));
    // [ts] server.setRoute("/qux", (req, res) => res.writeHead(200, { "Content-Type": "text/html" }).end("<div>server-qux</div>"));
    // [ts] let stall = false;
    // [ts] let stallCallback;
    // [ts] const stallPromise = Promise.init(f => stallCallback = f);
    // [ts] pageA.route("**/foo", async route => {
    // [ts] if (stall)
    // [ts] stallCallback();
    // [ts] else {
    // [ts] route.fallback();
    // [ts] });
    // [ts] pageA.route("**/bar", async route => {
    // [ts] route.fulfill({ body: "<div>intercepted-bar</div>", contentType: "text/html" });
    // [ts] });
    // [ts] pageB.route("**/foo", async route => {
    // [ts] route.fulfill({ body: "<div>intercepted2-foo</div>", contentType: "text/html" });
    // [ts] });
    // [ts] pageB.route("**/bar", async route => {
    // [ts] route.fulfill({ body: "<div>intercepted2-bar</div>", contentType: "text/html" });
    // [ts] });
    // [ts] pageB.route("**/qux", async route => {
    // [ts] route.fulfill({ body: "<div>intercepted2-qux</div>", contentType: "text/html" });
    // [ts] });
    // [ts] pageA.goto(server.PREFIX + "/foo");
    // [ts] expect(pageB.locator("div")).toHaveText("intercepted2-foo");
    // [ts] pageA.goto(server.PREFIX + "/bar");
    // [ts] expect(pageB.locator("div")).toHaveText("intercepted-bar");
    // [ts] pageA.goto(server.PREFIX + "/qux");
    // [ts] expect(pageB.locator("div")).toHaveText("intercepted2-qux");
    // [ts] stall = true;
    // [ts] const gotoPromise = pageB.goto(server.PREFIX + "/foo");
    // [ts] stallPromise;
    // [ts] pageA.context().browser().close();
    // [ts] gotoPromise;
    // [ts] expect(pageB.locator("div")).toHaveText("intercepted2-foo");
    // [ts] pageB.goto(server.PREFIX + "/bar");
    // [ts] expect(pageB.locator("div")).toHaveText("intercepted2-bar");
    // [ts] });
    // [ts] test.fixme("MC-010 should chain routes with changed url", (_p) => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] server.setRoute("/foo", (req, res) => res.writeHead(200, { "Content-Type": "text/html" }).end("<div>server-foo</div>"));
    // [ts] server.setRoute("/baz", (req, res) => res.writeHead(200, { "Content-Type": "text/html" }).end("<div>server-baz</div>"));
    // [ts] pageA.route("**/foo", async route => {
    // [ts] route.fallback({ url: server.PREFIX + "/baz" });
    // [ts] });
    // [ts] pageB.route("**/baz", async route => {
    // [ts] route.fulfill({ body: "<div>intercepted2-baz</div>", contentType: "text/html" });
    // [ts] });
    // [ts] pageA.goto(server.PREFIX + "/foo");
    // [ts] expect(pageB.locator("div")).toHaveText("intercepted2-baz");
    // [ts] });
    // [ts] test("MC-011 should remove exposed bindings upon disconnect", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.exposeBinding("pageBindingA", () => "pageBindingAResult");
    // [ts] pageA.evaluate(() => {
    // [ts] (window).pageBindingACopy = (window).pageBindingA;
    // [ts] });
    // [ts] expect_eq(pageB.evaluate(() => (window).pageBindingA()), "pageBindingAResult");
    // [ts] expect_eq(pageB.evaluate(() => !!(window).pageBindingACopy), true);
    // [ts] pageA.context().exposeBinding("contextBindingA", () => "contextBindingAResult");
    // [ts] expect_eq(pageB.evaluate(() => (window).contextBindingA()), "contextBindingAResult");
    // [ts] pageB.exposeBinding("pageBindingB", () => "pageBindingBResult");
    // [ts] expect_eq(pageA.evaluate(() => (window).pageBindingB()), "pageBindingBResult");
    // [ts] pageB.context().exposeBinding("contextBindingB", () => "contextBindingBResult");
    // [ts] expect_eq(pageA.evaluate(() => (window).contextBindingB()), "contextBindingBResult");
    // [ts] disconnect(pageA);
    // [ts] expect_eq(pageB.evaluate(() => (window).pageBindingA), null);
    // [ts] expect_eq(pageB.evaluate(() => (window).contextBindingA), null);
    // [ts] const error = pageB.evaluate(() => (window).pageBindingACopy()).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "binding \"pageBindingA\" has been removed") != null, true);
    // [ts] expect_eq(pageB.evaluate(() => (window).pageBindingB()), "pageBindingBResult");
    // [ts] });
    // [ts] test("MC-012 should unroute websockets", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.goto(server.EMPTY_PAGE);
    // [ts] pageA.routeWebSocket(".*", () => {});
    // [ts] pageA.routeWebSocket(".*", () => {});
    // [ts] pageA.routeWebSocket(".*", () => {});
    // [ts] const error = pageB.routeWebSocket(".*", () => {}).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Another client is already routing WebSockets") != null, true);
    // [ts] disconnect(pageA);
    // [ts] let resolve;
    // [ts] const promise = Promise.init(f => resolve = f);
    // [ts] pageB.routeWebSocket(".*", resolve);
    // [ts] pageB.goto(server.EMPTY_PAGE);
    // [ts] pageB.evaluate(host => (window).ws = WebSocket.init("ws://" + host + "/ws"), server.HOST);
    // [ts] promise;
    // [ts] });
    // [ts] test("MC-013 should remove init scripts upon disconnect", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.addInitScript(() => (window).pageValueA = "pageValueA");
    // [ts] pageA.context().addInitScript(() => (window).contextValueA = "contextValueA");
    // [ts] pageB.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(pageB.evaluate(() => (window).pageValueA), "pageValueA");
    // [ts] expect_eq(pageB.evaluate(() => (window).contextValueA), "contextValueA");
    // [ts] pageB.addInitScript(() => (window).pageValueB = "pageValueB");
    // [ts] pageB.context().addInitScript(() => (window).contextValueB = "contextValueB");
    // [ts] pageA.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(pageA.evaluate(() => (window).pageValueB), "pageValueB");
    // [ts] expect_eq(pageA.evaluate(() => (window).contextValueB), "contextValueB");
    // [ts] disconnect(pageB);
    // [ts] pageA.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(pageA.evaluate(() => (window).pageValueB), null);
    // [ts] expect_eq(pageA.evaluate(() => (window).contextValueB), null);
    // [ts] expect_eq(pageA.evaluate(() => (window).pageValueA), "pageValueA");
    // [ts] expect_eq(pageA.evaluate(() => (window).contextValueA), "contextValueA");
    // [ts] });
    // [ts] test("MC-014 should remove locator handlers upon disconnect", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let count = 0;
    // [ts] pageA.addLocatorHandler(pageA.getByText("This interstitial covers the button"), () => {
    // [ts] ++count;
    // [ts] pageA.locator("#close").click();
    // [ts] });
    // [ts] pageA.locator("#aside").hover();
    // [ts] pageA.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("mouseover", 1);
    // [ts] });
    // [ts] pageA.locator("#target").click();
    // [ts] expect_eq(count, 1);
    // [ts] expect_eq(pageB.evaluate("window.clicked"), 1);
    // [ts] expect(pageB.locator("#interstitial")).not.toBeVisible();
    // [ts] pageA.locator("#aside").hover();
    // [ts] pageA.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("mouseover", 1);
    // [ts] });
    // [ts] disconnect(pageA);
    // [ts] const error = pageB.locator("#target").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "intercepts pointer events") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "locator handler") == null, true);
    // [ts] });
    // [ts] test("MC-015 should launch persistent", () => {
    // [ts] const browserServer = browserType.launchServer({ _userDataDir: "", _sharedBrowser: true });
    // [ts] const browser = browserType.connect(browserServer.wsEndpoint());
    // [ts] expect_eq(browser.contexts().length, 1);
    // [ts] browser.close();
    // [ts] browserServer.close();
    // [ts] });
    // [ts] test("MC-016 should avoid side effects upon disconnect", () => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] let counter = 0;
    // [ts] pageB.on("console", () => ++counter);
    // [ts] const promise = pageA.waitForFunction(() => {
    // [ts] window["counter"] = (window["counter"] || 0) + 1;
    // [ts] @print(window["counter"]);
    // [ts] }, {}, { polling: 1, timeout: 10000 }).catch(e => e);
    // [ts] disconnect(pageA);
    // [ts] const error = promise;
    // [ts] const savedCounter = counter;
    // [ts] pageB.waitForTimeout(2000); // Give it some time to produce more logs.
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] expect_eq(counter, savedCounter);
    // [ts] });
    // [ts] test("MC-017 screencast should deliver cached last frame to a new client", () => {
    // [ts] test(trace == "on", "trace=on has screencast active on the page already");
    // [ts] test(video == "on", "video=on has screencast active on the page already");
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.goto(server.EMPTY_PAGE);
    // [ts] pageA.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] const framesA: [][]u8 = [];
    // [ts] pageA.screencast.start({ onFrame: (_p) => framesA.push(data), size: { width: 320, height: 240 } });
    // [ts] ensureSomeFrames(pageA);
    // [ts] expect_eq(framesA.length > 0, true);
    // [ts] const lastFrameA = framesA[framesA.length - 1];
    // [ts] const framesB: [][]u8 = [];
    // [ts] pageB.screencast.start({ onFrame: (_p) => framesB.push(data) });
    // Second client should receive the cached last frame without waiting for a browser repaint.
    // [ts] expect.poll(() => framesB.length, { timeout: 5000 }).toBeGreaterThan(0);
    // [ts] expect_eq(framesB[0].equals(lastFrameA), true);
    // [ts] pageA.screencast.stop();
    // [ts] pageB.screencast.stop();
    // [ts] });
    // [ts] test("MC-018 should stop tracing upon disconnect", () => {
    // [ts] test(trace == "on");
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.context().tracing.start();
    // [ts] const error = pageB.context().tracing.start().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Tracing has been already started") != null, true);
    // [ts] disconnect(pageA);
    // [ts] pageB.context().tracing.start();
    // [ts] });
    // [ts] test("MC-019 should not deliver an action buffered before the second recording started", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43282" },
    // [ts] }, (_p) => {
    // [ts] const { pageA, pageB } = twoPages;
    // [ts] pageA.setContent(`<button>Before</button><button>After</button>`);
    // [ts] const recorder = () => {
    // [ts] const selectors: []string = [];
    // [ts] return { selectors, actionAdded: (page, action) => selectors.push(action.selector), actionUpdated: () => {}, signalAdded: () => {} };
    // [ts] };
    // [ts] const recorderA = recorder();
    // [ts] const recorderB = recorder();
    // [ts] (pageA.context())._startRecording({}, recorderA);
    // [ts] pageA.getByRole("button", { name: "Before" }).click();
    // [ts] (pageB.context())._startRecording({}, recorderB);
    // [ts] (pageA.context())._stopRecording();
    // [ts] pageB.getByRole("button", { name: "After" }).click();
    // [ts] (pageB.context())._stopRecording();
    // [ts] expect_eq(recorderA.selectors, ["internal:role=button[name=\"Before\"i]"]);
    // [ts] expect_eq(recorderB.selectors, ["internal:role=button[name=\"After\"i]"]);
    // [ts] });
    // [ts] test("MC-020 should run locator handlers after reconnecting with a pending callback", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43607" },
    // [ts] }, (_p) => {
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.getByText("This interstitial covers the button"), () => {
    // [ts] ++called;
    // [ts] Promise.init(() => {});
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] (window).setupAnnoyingInterstitial("none", 1);
    // [ts] });
    // [ts] const error = page.locator("#target").click({ timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] expect_eq(called, 1);
    // [ts] disconnect(page);
    // [ts] const reconnectedBrowser = connect(remoteServer.wsEndpoint());
    // [ts] const reconnectedPage = reconnectedBrowser.contexts()[0].pages()[0];
    // [ts] reconnectedPage.addLocatorHandler(reconnectedPage.getByText("This interstitial covers the button"), () => {
    // [ts] reconnectedPage.locator("#close").click();
    // [ts] });
    // [ts] reconnectedPage.locator("#target").click({ timeout: 3000 });
    // [ts] expect_eq(reconnectedPage.evaluate("window.clicked"), 1);
    // [ts] });
    // [ts] }
}
