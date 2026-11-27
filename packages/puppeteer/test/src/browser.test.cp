// Migrated from TypeScript (src/browser.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {HTTPResponse} from "puppeteer-core/internal/api/HTTPResponse.js";
// [cap-migrate] import {WebWorker} from "puppeteer-core/internal/api/WebWorker.js";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";

import { getTestState, launch, setupTestBrowserHooks } from "./mocha-utils.cp";
import { isFavicon, waitEvent } from "./utils.cp";

// [cap-test] describe("Browser specs", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Browser.version", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.userAgent", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.target", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.process", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.connected", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.screens", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.add|removeScreen", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.get|setWindowBounds", () => {

  // [cap-test] });

  // [cap-test] describe("Browser.on(\"console\")", () => {



  // [cap-test] });

  // [cap-test] describe("Browser.on(\"request\")", () => {



  // [cap-test] });

  // [cap-test] describe("Browser network events", () => {






  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should return version", () => {
    // [ts] const {browser} = getTestState();
    // [ts] const version = browser.version();
    // [ts] expect_eq(version.length > 0, true);
    // [ts] expect(version.toLowerCase()).atLeastOneToContain(["firefox", "chrome"]);
    // [ts] });
    // [ts] test("should include Browser engine", () => {
    // [ts] const {browser, isChrome} = getTestState();
    // [ts] const userAgent = browser.userAgent();
    // [ts] expect_eq(userAgent.length > 0, true);
    // [ts] if (isChrome) {
    // [ts] expect_eq(std.mem.indexOf(u8, userAgent, "WebKit") != null, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, userAgent, "Gecko") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should include Browser name", () => {
    // [ts] const {browser, isChrome} = getTestState();
    // [ts] const userAgent = browser.userAgent();
    // [ts] expect_eq(userAgent.length > 0, true);
    // [ts] if (isChrome) {
    // [ts] expect_eq(std.mem.indexOf(u8, userAgent, "Chrome") != null, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, userAgent, "Firefox") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should return browser target", () => {
    // [ts] const {browser} = getTestState();
    // [ts] const target = browser.target();
    // [ts] expect_eq(target.type(), "browser");
    // [ts] });
    // [ts] test("should return child_process instance", () => {
    // [ts] const {browser} = getTestState();
    // [ts] const process = browser.process();
    // [ts] expect_eq(process.pid > 0, true);
    // [ts] });
    // [ts] test("should not return child_process for remote browser", () => {
    // [ts] const {browser, puppeteer} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] const browserWSEndpoint = browser.wsEndpoint();
    // [ts] using remoteBrowser = puppeteer.connect({
    // [ts] browserWSEndpoint,
    // [ts] protocol: browser.protocol,
    // [ts] });
    // [ts] expect_eq(remoteBrowser.process(), null);
    // [ts] });
    // [ts] test("should keep connected after the last page is closed", () => {
    // [ts] const {browser, close} = launch({});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const pages = browser.pages();
    // [ts] joinAll( // migrated: Promise.all -> joinAll
    // [ts] pages.map(page => {
    // [ts] return page.close();
    // [ts] }),
    // [ts] );
    // Verify the browser is still connected.
    // [ts] expect_eq(browser.connected, true);
    // Verify the browser can open a new page.
    // [ts] browser.newPage();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should set the browser connected state", () => {
    // [ts] const {browser, puppeteer, close} = launch({});
    // [ts] const browserWSEndpoint = browser.wsEndpoint();
    // [ts] const protocol = browser.protocol;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(browser.connected, true);
    // [ts] browser.disconnect();
    // [ts] expect_eq(browser.connected, false);
    // [ts] using newBrowser = puppeteer.connect({
    // [ts] browserWSEndpoint,
    // [ts] protocol,
    // [ts] });
    // [ts] expect_eq(newBrowser.connected, true);
    // [ts] newBrowser.disconnect();
    // [ts] expect_eq(newBrowser.connected, false);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cleanupBrowser = puppeteer
    // [ts] .connect({
    // [ts] browserWSEndpoint,
    // [ts] protocol,
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return null;
    // [ts] });
    // [ts] cleanupBrowser?.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] const browserProcess = browser.process();
    // [ts] if (
    // [ts] browserProcess &&
    // [ts] browserProcess.exitCode == null &&
    // [ts] browserProcess.signalCode == null
    // [ts] ) {
    // [ts] const exited = Promise.init(resolve => {
    // [ts] browserProcess.once("exit", () => {
    // [ts] resolve();
    // [ts] });
    // [ts] });
    // [ts] browserProcess.kill();
    // [ts] exited;
    // [ts] }
    // [ts] close();
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("should return default screen info", () => {
    // [ts] const {browser, isHeadless} = getTestState();
    // [ts] if (!isHeadless) {
    // In headful mode 'Browser.screens' returns the real
    // platform screens info which is not stable enough
    // for matching.
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Not testable in headful');)
    // [ts] }
    // [ts] const screenInfos = browser.screens();
    // [ts] expect(screenInfos).toMatchObject([
    // [ts] {
    // [ts] availHeight: 600,
    // [ts] availLeft: 0,
    // [ts] availTop: 0,
    // [ts] availWidth: 800,
    // [ts] colorDepth: 24,
    // [ts] devicePixelRatio: 1,
    // [ts] height: 600,
    // [ts] id: expect.any(String),
    // [ts] isExtended: false,
    // [ts] isInternal: false,
    // [ts] isPrimary: true,
    // [ts] label: "",
    // [ts] left: 0,
    // [ts] orientation: {angle: 0, type: "landscapePrimary"},
    // [ts] top: 0,
    // [ts] width: 800,
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should add and remove a screen", () => {
    // [ts] const {browser} = getTestState();
    // [ts] const screenInfo = browser.addScreen({
    // [ts] left: 800,
    // [ts] top: 0,
    // [ts] width: 1600,
    // [ts] height: 1200,
    // [ts] colorDepth: 32,
    // [ts] workAreaInsets: {bottom: 80},
    // [ts] label: "secondary",
    // [ts] });
    // [ts] expect: (screenInfo) => {
    // [ts] availHeight: 1120,
    // [ts] availLeft: 800,
    // [ts] availTop: 0,
    // [ts] availWidth: 1600,
    // [ts] colorDepth: 32,
    // [ts] devicePixelRatio: 1,
    // [ts] height: 1200,
    // [ts] id: expect.any(String),
    // [ts] isExtended: true,
    // [ts] isInternal: false,
    // [ts] isPrimary: false,
    // [ts] label: "secondary",
    // [ts] left: 800,
    // [ts] orientation: {angle: 0, type: "landscapePrimary"},
    // [ts] top: 0,
    // [ts] width: 1600,
    // [ts] });
    // [ts] expect_eq((browser.screens()).length, 2);
    // [ts] browser.removeScreen(screenInfo.id);
    // [ts] expect_eq((browser.screens()).length, 1);
    // [ts] });
    // [ts] test("should get and set browser window bounds", () => {
    // [ts] const {browser, context} = getTestState();
    // [ts] const initialBounds = {
    // [ts] left: 10,
    // [ts] top: 20,
    // [ts] width: 800,
    // [ts] height: 600,
    // [ts] };
    // [ts] const page = context.newPage({
    // [ts] type: "window",
    // [ts] windowBounds: initialBounds,
    // [ts] });
    // [ts] const windowId = page.windowId();
    // [ts] expect(browser.getWindowBounds(windowId)).toMatchObject(
    // [ts] initialBounds,
    // [ts] );
    // [ts] const setBounds = {
    // [ts] left: 100,
    // [ts] top: 200,
    // [ts] width: 1600,
    // [ts] height: 1200,
    // [ts] };
    // [ts] browser.setWindowBounds(windowId, setBounds);
    // [ts] expect(browser.getWindowBounds(windowId)).toMatchObject(setBounds);
    // [ts] });
    // [ts] test("should set and get browser window maximized state", () => {
    // [ts] const {browser, context} = getTestState();
    // Add a secondary screen.
    // [ts] const screenInfo = browser.addScreen({
    // [ts] left: 800,
    // [ts] top: 0,
    // [ts] width: 1600,
    // [ts] height: 1200,
    // [ts] });
    // Open a window on the secondary screen.
    // [ts] const page = context.newPage({
    // [ts] type: "window",
    // [ts] windowBounds: {
    // [ts] left: screenInfo.availLeft + 50,
    // [ts] top: screenInfo.availTop + 50,
    // [ts] width: screenInfo.availWidth - 100,
    // [ts] height: screenInfo.availHeight - 100,
    // [ts] },
    // [ts] });
    // Maximize the created window.
    // [ts] const windowId = page.windowId();
    // [ts] browser.setWindowBounds(windowId, {windowState: "maximized"});
    // Expect the maximized window to be maximized.
    // [ts] expect: (browser.getWindowBounds(windowId)) => {
    // [ts] windowState: "maximized",
    // [ts] });
    // Cleanup.
    // [ts] browser.removeScreen(screenInfo.id);
    // [ts] });
    // [ts] test("should report console messages from pages", () => {
    // [ts] const {browser, page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "console", message => {
    // [ts] return message.text() == "from-page";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] @print("from-page");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect_eq(message.page(), page);
    // [ts] expect_eq(message.frame(), page.mainFrame());
    // [ts] expect_eq(message.worker() == null, true);
    // [ts] });
    // [ts] test("should report console messages from dedicated workers", () => {
    // [ts] const {browser, page, context} = getTestState();
    // [ts] const received = {
    // [ts] page: [],
    // [ts] context: [],
    // [ts] browser: [],
    // [ts] };
    // [ts] const collect = (into) => {
    // [ts] return (message) => {
    // [ts] if (message.text() == "from-worker") {
    // [ts] into.push(message);
    // [ts] }
    // [ts] };
    // [ts] };
    // [ts] page.on("console", collect(received.page));
    // [ts] context.on("console", collect(received.context));
    // [ts] const browserListener = collect(received.browser);
    // [ts] browser.on("console", browserListener);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // `done` is logged after `from-worker`, so by the time it arrives every
    // delivery of `from-worker` has been emitted.
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] waitEvent(browser, "console", message => {
    // [ts] return message.text() == "done";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(
    // [ts] `data:text/javascript,console.log('from-worker');console.log('done')`,
    // [ts] );
    // [ts] }),
    // [ts] ]);
    // Each level receives the same message exactly once.
    // [ts] expect(received.page).toHaveLength(1);
    // [ts] expect(received.context).toHaveLength(1);
    // [ts] expect(received.browser).toHaveLength(1);
    // [ts] const [message] = received.browser;
    // [ts] expect_eq(received.page[0], message);
    // [ts] expect_eq(received.context[0], message);
    // [ts] expect_eq(message.worker(), worker);
    // [ts] expect_eq(message.page() == null, true);
    // [ts] expect_eq(message.frame() == null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.off("console", browserListener);
    // [ts] }
    // [ts] });
    // [ts] test("should report console messages from service workers", () => {
    // [ts] const {browser, page, server, context} = getTestState();
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "console", message => {
    // [ts] return message.text() == "from-service-worker";
    // [ts] }),
    // [ts] worker.evaluate(() => {
    // [ts] @print("from-service-worker");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.worker(), worker);
    // [ts] expect_eq(message.page() == null, true);
    // [ts] });
    // [ts] test("should report console messages replayed when attaching after connect", () => {
    // [ts] const {browser, page, server, puppeteer} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] @print("logged-before-connect");
    // [ts] });
    // [ts] using remoteBrowser = puppeteer.connect({
    // [ts] browserWSEndpoint: browser.wsEndpoint(),
    // [ts] protocol: browser.protocol,
    // [ts] });
    // The listener is registered before any page is attached, so the
    // messages replayed on attachment must not be dropped.
    // [ts] const messages: []ConsoleMessage = [];
    // [ts] remoteBrowser.on("console", message => {
    // [ts] messages.push(message);
    // [ts] });
    // [ts] const remotePage = (remoteBrowser.pages()).find(page => {
    // [ts] return page.url() == server.EMPTY_PAGE;
    // [ts] });
    // Log a marker after attaching: once it arrives, any duplicate of the
    // replayed message would have arrived too.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(remoteBrowser, "console", message => {
    // [ts] return message.text() == "logged-after-connect";
    // [ts] }),
    // [ts] remotePage.evaluate(() => {
    // [ts] @print("logged-after-connect");
    // [ts] }),
    // [ts] ]);
    // [ts] const replayed = messages.filter(message => {
    // [ts] return message.text() == "logged-before-connect";
    // [ts] });
    // [ts] expect(replayed).toHaveLength(1);
    // [ts] expect_eq(replayed[0].page(), remotePage);
    // [ts] expect_eq(replayed[0].frame(), remotePage.mainFrame());
    // [ts] });
    // [ts] test("should report requests from pages", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "request", request => {
    // [ts] return request.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.page(), page);
    // [ts] expect_eq(request.frame(), page.mainFrame());
    // [ts] });
    // [ts] test("should emit context events before browser events", () => {
    // [ts] const {browser, context, page, server} = getTestState();
    // [ts] const order: []string = [];
    // [ts] const record = (source) => {
    // [ts] return (request) => {
    // [ts] if (request.url() == server.EMPTY_PAGE) {
    // [ts] order.push(source);
    // [ts] }
    // [ts] };
    // [ts] };
    // [ts] const onBrowser = record("browser");
    // [ts] const onContext = record("context");
    // [ts] browser.on("request", onBrowser);
    // [ts] context.on("request", onContext);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(order, ["context", "browser"]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.off("request", onBrowser);
    // [ts] context.off("request", onContext);
    // [ts] }
    // [ts] });
    // [ts] test("should report requests from dedicated workers", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "request", request => {
    // [ts] return request.url() == server.PREFIX + "/one-style.css";
    // [ts] }),
    // [ts] worker.evaluate((url) => {
    // [ts] return fetch(url);
    // [ts] }, server.PREFIX + "/one-style.css"),
    // [ts] ]);
    // [ts] expect_eq(request.worker(), worker);
    // [ts] expect_eq(request.page() == null, true);
    // [ts] });
    // [ts] test("should not interfere with request interception", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] const requests: []HTTPRequest = [];
    // [ts] const listener = (request) => {
    // [ts] if (request.url() == server.EMPTY_PAGE) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] };
    // [ts] browser.on("request", listener);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect(requests).toHaveLength(1);
    // [ts] expect_eq(requests[0].page(), page);
    // [ts] expect_eq(requests[0].isInterceptResolutionHandled(), true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.off("request", listener);
    // [ts] }
    // [ts] });
    // [ts] test("should report responses from pages", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "response", response => {
    // [ts] return response.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.request().page(), page);
    // [ts] expect_eq(response.frame(), page.mainFrame());
    // [ts] });
    // [ts] test("should report finished requests from pages", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "requestfinished", request => {
    // [ts] return request.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.page(), page);
    // [ts] expect_eq(request.response(), true);
    // [ts] });
    // [ts] test("should report failed requests from pages", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith("css")) {
    // [ts] void request.abort();
    // [ts] } else {
    // [ts] void request.continue();
    // [ts] }
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "requestfailed", request => {
    // [ts] return request.url().endsWith("one-style.css");
    // [ts] }),
    // [ts] page.goto(server.PREFIX + "/one-style.html"),
    // [ts] ]);
    // [ts] expect_eq(request.page(), page);
    // [ts] expect_eq(request.response() == null, true);
    // [ts] expect_eq(request.failure(), true);
    // [ts] });
    // [ts] test("should report requests served from cache", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/cached/one-script.html");
    // [ts] Promise.init(res => {
    // [ts] setTimeout(res, 1000);
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "requestservedfromcache", request => {
    // [ts] return request.url().endsWith("one-script.js");
    // [ts] }),
    // [ts] page.reload(),
    // [ts] ]);
    // [ts] expect_eq(request.page(), page);
    // [ts] });
    // [ts] test("should emit context events before browser events", () => {
    // [ts] const {browser, context, page, server} = getTestState();
    // [ts] const order = {response: [], requestfinished: []};
    // [ts] const record = (
    // [ts] event: keyof @typeName(order),
    // [ts] source: "context" | "browser",
    // [ts] ) => {
    // [ts] return (data) => {
    // [ts] if (data.url() == server.EMPTY_PAGE) {
    // [ts] order[event].push(source);
    // [ts] }
    // [ts] };
    // [ts] };
    // [ts] const onContextResponse = record("response", "context");
    // [ts] const onBrowserResponse = record("response", "browser");
    // [ts] const onContextFinished = record("requestfinished", "context");
    // [ts] const onBrowserFinished = record("requestfinished", "browser");
    // [ts] context.on("response", onContextResponse);
    // [ts] browser.on("response", onBrowserResponse);
    // [ts] context.on("requestfinished", onContextFinished);
    // [ts] browser.on("requestfinished", onBrowserFinished);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "requestfinished", request => {
    // [ts] return request.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // Only the order across emitters is checked: the relative order of
    // `response` and `requestfinished` is not guaranteed.
    // [ts] expect_eq(order.response, ["context", "browser"]);
    // [ts] expect_eq(order.requestfinished, ["context", "browser"]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.off("response", onContextResponse);
    // [ts] browser.off("response", onBrowserResponse);
    // [ts] context.off("requestfinished", onContextFinished);
    // [ts] browser.off("requestfinished", onBrowserFinished);
    // [ts] }
    // [ts] });
    // [ts] test("should emit each event once per page event", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const events = ["response", "requestfinished"];
    // [ts] const seen = {page: [], browser: []};
    // [ts] const listeners = events.map(event => {
    // [ts] const onPage = (data) => {
    // [ts] if (!isFavicon(data)) {
    // [ts] seen.page.push(`${event}:${data.url()}`);
    // [ts] }
    // [ts] };
    // [ts] const onBrowser = (data) => {
    // [ts] if (!isFavicon(data)) {
    // [ts] seen.browser.push(`${event}:${data.url()}`);
    // [ts] }
    // [ts] };
    // [ts] page.on(event, onPage);
    // [ts] browser.on(event, onBrowser);
    // [ts] return {event, onPage, onBrowser};
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.goto(server.PREFIX + "/foo.html");
    // [ts] expect_eq(seen.page.length >= 3, true);
    // [ts] expect_eq(seen.browser, seen.page);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] for (const {event, onPage, onBrowser} of listeners) {
    // [ts] page.off(event, onPage);
    // [ts] browser.off(event, onBrowser);
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("should report responses from dedicated workers", () => {
    // [ts] const {browser, page, server} = getTestState();
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const url = server.PREFIX + "/one-style.css";
    // [ts] const [response, request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "response", response => {
    // [ts] return response.url() == url;
    // [ts] }),
    // [ts] waitEvent(browser, "requestfinished", request => {
    // [ts] return request.url() == url;
    // [ts] }),
    // [ts] worker.evaluate((url) => {
    // [ts] (fetch(url)).text();
    // [ts] }, url),
    // [ts] ]);
    // [ts] expect_eq(response.request().worker(), worker);
    // [ts] expect_eq(request.worker(), worker);
    // [ts] expect_eq(request.page() == null, true);
    // [ts] });
}
