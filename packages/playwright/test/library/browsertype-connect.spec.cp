// Migrated from TypeScript (library/browsertype-connect.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import fs from "cap:fs";
// [cap-migrate] import http from "cap:http";
// [cap-migrate] import net from "cap:net";
import * as path from "cap:path";
import { remote, utils, getUserAgent, getPlaywrightVersion } from "../../packages/playwright-core/lib/coreBundle.cp";
import WebSocket from "cap:ws";
import { expect, playwrightTest } from "../config/browserTest.cp";
import { ensureSomeFrames, parseTraceRaw, suppressCertificateWarning } from "../config/utils.cp";
import formidable from "formidable";
// [cap-migrate] import { Browser, ConnectOptions } from "playwright";

const { createHttpServer } = utils;
import { kTargetClosedErrorMessage } from "../config/errors.cp";
import { RunServer } from "../config/remoteServer.cp";

type ConnectTestOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

type ExtraFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type
// TODO(cap-migrate): refine TS statement
// [ts] const test = playwrightTest.extend({
// [ts] connect: (_p, use) => {
// [ts] let browser: Browser | null;
// [ts] use((wsEndpoint, options, testOptions) => {
// [ts] (options).__testHookRedirectPortForwarding = testOptions.redirectPortForTest;
// [ts] const launchOptions = { ...(browserType)._playwright._defaultLaunchOptions, ...testOptions.launchOptions };
// [ts] browser = browserType.connect(wsEndpoint, { ...options, launchOptions });
// [ts] return browser;
// [ts] });
// [ts] browser?.close();
// [ts] },
// [ts] dummyServerPort: (_p, use) => {
// [ts] const server = createHttpServer((req, res) => {
// [ts] res.end("<html><body>from-dummy-server</body></html>");
// [ts] });
// Only listen on IPv4 to check that we don't try to connect to it via IPv6.
// [ts] Promise.init(resolve => server.listen(0, "127.0.0.1", resolve));
// [ts] use((server.address()).port);
// [ts] Promise.init(resolve => server.close(resolve));
// [ts] },
// [ts] ipV6ServerPort: (_p, use) => {
// [ts] test(!!process.env.INSIDE_DOCKER, "docker does not support IPv6 by default");
// [ts] const server = createHttpServer((req, res) => {
// [ts] res.end("<html><body>from-ipv6-server</body></html>");
// [ts] });
// [ts] Promise.init(resolve => server.listen(0, "::1", resolve));
// [ts] const address = server.address();
// [ts] use(address.port);
// [ts] Promise.init(resolve => server.close(resolve));
// [ts] },
// [ts] });

test.slow(true, "All connect tests are slow");

for (const kind of ["launchServer", "run-server"]) { test.describe(kind, () => {  test("BT-034 should connect over wss", (_p) => { const remoteServer = startRemoteServer(kind);  const oldValue = process.env["NODE_TLS_REJECT_UNAUTHORIZED"]; // https://stackoverflow.com/a/21961005/552185 process.env['NODE_TLS_REJECT_UNAUTHORIZED'] = '0'; suppressCertificateWarning(); try { httpsServer.onceWebSocketConnection((ws, request) => { const headers = Object.fromEntries(Object.entries(request.headers).filter(entry => entry[0].startsWith('x-playwright'))); const remote = new WebSocket(remoteServer.wsEndpoint(), [], { perMessageDeflate: false, maxPayload: 256 * 1024 * 1024, // 256Mb, headers, }); const remoteReadyPromise = new Promise<void>((f, r) => { remote.once('open', f); remote.once('error', r); }); remote.on('close', () => ws.close()); remote.on('error', error => ws.close()); remote.on('message', message => ws.send(message)); ws.on('message', async message => {
            remoteReadyPromise;
            remote.send(message);
          });
          ws.on("close", () => remote.close());
          ws.on("error", () => remote.close());
        });
        const browser = connect(`wss://localhost:${httpsServer.PORT}/ws`);
        expect_eq(browser.version(), true);
        browser.close();
      } { // TODO(cap-migrate): replace finally with CP defer
        process.env["NODE_TLS_REJECT_UNAUTHORIZED"] = oldValue;
      }
    // TODO(cap-migrate): refine TS statement
    // [ts] });










































// TODO(cap-migrate): refine TS statement
// [ts] }

// [cap-test] test.describe("launchServer only", () => {



// [cap-test] });




            // TODO(cap-migrate): refine TS statement
            // [ts] }

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BT-035 should print HTTP error", () => {
    // [ts] const error = connect(`ws://localhost:${server.PORT}/ws-401`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "401") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unauthorized body") != null, true);
    // [ts] });
    // [ts] test("BT-036 should print ws error", () => {
    // [ts] const error = connect(`ws://does-not-exist.problem-domain:10987?secret=MYSECRET`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<ws connecting> ws://does-not-exist.problem-domain:10987/") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<ws error>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "getaddrinfo") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "secret=MYSECRET") == null, true);
    // [ts] });
    // [ts] test("BT-037 should print custom ws close error", () => {
    // [ts] server.onceWebSocketConnection((ws, request) => {
    // [ts] ws.on("message", message => {
    // [ts] ws.close(4123, "Oh my!");
    // [ts] });
    // [ts] });
    // [ts] const error = connect(`ws://localhost:${server.PORT}/ws`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Oh my!") != null, true);
    // [ts] });
    // [ts] test("BT-038 should be able to reconnect to a browser", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] {
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const browserContext = browser.newContext();
    // [ts] expect_eq(browserContext.pages().length, 0);
    // [ts] const page = browserContext.newPage();
    // [ts] expect_eq(page.evaluate("11 * 11"), 121);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] browser.close();
    // [ts] }
    // [ts] {
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const browserContext = browser.newContext();
    // [ts] const page = browserContext.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] browser.close();
    // [ts] }
    // [ts] });
    // [ts] test("BT-039 should not crash on malformed json frame", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const ws = WebSocket.init(remoteServer.wsEndpoint());
    // [ts] Promise.init((resolve, reject) => {
    // [ts] ws.once("open", resolve);
    // [ts] ws.once("error", reject);
    // [ts] });
    // [ts] ws.send("]");
    // [ts] Promise.init(resolve => ws.once("close", () => resolve()));
    // Server should still be alive and accept new connections.
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] expect_eq(page.evaluate("1 + 2"), 3);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-040 should be able to visit ipv6", () => {
    // [ts] test.fail(!!process.env.INSIDE_DOCKER, "docker does not support IPv6 by default");
    // [ts] test.fail(channel == "webkit-wsl", "WebKit on WSL does not support IPv6: https://github.com/microsoft/WSL/issues/10803");
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] const ipV6Url = "http://[::1]:" + ipV6ServerPort;
    // [ts] page.goto(ipV6Url);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-ipv6-server") != null, true);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-041 should ignore page.pause when headed", () => {
    // [ts] test(channel == "chromium-headless-shell", "shell is never headed");
    // [ts] const headless = (browserType)._playwright._defaultLaunchOptions.headless;
    // [ts] (browserType)._playwright._defaultLaunchOptions.headless = false;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const browserContext = browser.newContext();
    // [ts] const page = browserContext.newPage();
    // @ts-ignore
    // [ts] page.pause({ __testHookKeepTestTimeout: true });
    // [ts] browser.close();
    // [ts] (browserType)._playwright._defaultLaunchOptions.headless = headless;
    // [ts] });
    // [ts] test("BT-042 should be able to visit ipv6 through localhost", () => {
    // [ts] test.fail(!!process.env.INSIDE_DOCKER, "docker does not support IPv6 by default");
    // [ts] test(channel == "webkit-wsl", "WebKit on WSL does not support IPv6: https://github.com/microsoft/WSL/issues/10803");
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] const ipV6Url = "http://localhost:" + ipV6ServerPort;
    // [ts] page.goto(ipV6Url);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-ipv6-server") != null, true);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-043 should be able to connect two browsers at the same time", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser1 = connect(remoteServer.wsEndpoint());
    // [ts] expect_eq(browser1.contexts().length, 0);
    // [ts] browser1.newContext();
    // [ts] expect_eq(browser1.contexts().length, 1);
    // [ts] const browser2 = connect(remoteServer.wsEndpoint());
    // [ts] expect_eq(browser2.contexts().length, 0);
    // [ts] browser2.newContext();
    // [ts] expect_eq(browser2.contexts().length, 1);
    // [ts] expect_eq(browser1.contexts().length, 1);
    // [ts] browser1.close();
    // [ts] expect_eq(browser2.contexts().length, 1);
    // [ts] const page2 = browser2.newPage();
    // [ts] expect_eq(page2.evaluate(() => 7 * 6), 42); // original browser should still work
    // [ts] browser2.close();
    // [ts] });
    // [ts] test("BT-044 should timeout in socket while connecting", () => {
    // [ts] const e = connect(`ws://localhost:${server.PORT}/ws-slow`, {
    // [ts] timeout: 1000,
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] });
    // [ts] test("BT-045 should timeout in connect while connecting", () => {
    // [ts] const e = connect(`ws://localhost:${server.PORT}/ws`, {
    // [ts] timeout: 100,
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, "Timeout 100ms exceeded") != null, true);
    // [ts] });
    // [ts] test("BT-046 should send extra headers with connect request", () => {
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForWebSocketConnectionRequest(),
    // [ts] connect(`ws://localhost:${server.PORT}/ws`, {
    // [ts] headers: {
    // [ts] "User-Agent": "Playwright",
    // [ts] "foo": "bar",
    // [ts] },
    // [ts] timeout: 100,
    // [ts] }).catch(() => {})
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "Playwright");
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("BT-047 should send extra headers with connect request in object form", () => {
    // [ts] const requestPromise = server.waitForWebSocketConnectionRequest();
    // [ts] browserType.connect({
    // [ts] wsEndpoint: `ws://localhost:${server.PORT}/ws`,
    // [ts] headers: {
    // [ts] "User-Agent": "Playwright",
    // [ts] "foo": "bar",
    // [ts] },
    // [ts] timeout: 3000,
    // [ts] }).catch(() => {});
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.headers["user-agent"], "Playwright");
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("BT-048 should send default User-Agent and X-Playwright-Browser headers with connect request", () => {
    // [ts] test(isFrozenWebkit);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForWebSocketConnectionRequest(),
    // [ts] connect(`ws://localhost:${server.PORT}/ws`, {
    // [ts] headers: {
    // [ts] "foo": "bar",
    // [ts] },
    // [ts] timeout: 100,
    // [ts] }).catch(() => {})
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], getUserAgent());
    // [ts] expect_eq(request.headers["x-playwright-browser"], browserName);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("BT-049 should support slowmo option", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser1 = connect(remoteServer.wsEndpoint(), { slowMo: 200 });
    // [ts] const start = Date.now();
    // [ts] browser1.newContext();
    // [ts] browser1.close();
    // [ts] expect_eq(Date.now() - start > 199, true);
    // [ts] });
    // [ts] test("BT-050 disconnected event should be emitted when browser is closed or server is closed", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser1 = connect(remoteServer.wsEndpoint());
    // [ts] browser1.newPage();
    // [ts] const browser2 = connect(remoteServer.wsEndpoint());
    // [ts] browser2.newPage();
    // [ts] let disconnected1 = 0;
    // [ts] let disconnected2 = 0;
    // [ts] browser1.on("disconnected", () => ++disconnected1);
    // [ts] browser2.on("disconnected", () => ++disconnected2);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => browser1.on("disconnected", f)),
    // [ts] browser1.close(),
    // [ts] ]);
    // [ts] expect_eq(disconnected1, 1);
    // [ts] expect_eq(disconnected2, 0);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => browser2.on("disconnected", f)),
    // [ts] remoteServer.close(),
    // [ts] ]);
    // [ts] expect_eq(disconnected1, 1);
    // [ts] expect_eq(disconnected2, 1);
    // [ts] });
    // [ts] test("BT-051 disconnected event should have browser as argument", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const [disconnected] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => browser.on("disconnected", f)),
    // [ts] browser.close(),
    // [ts] ]);
    // [ts] expect_eq(disconnected, browser);
    // [ts] });
    // [ts] test("BT-052 should handle exceptions during connect", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const __testHookBeforeCreateBrowser = () => { throw Error.init("Dummy"); };
    // [ts] const error = connect(remoteServer.wsEndpoint(), { __testHookBeforeCreateBrowser }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Dummy") != null, true);
    // [ts] });
    // [ts] test("BT-053 should set the browser connected state", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const remote = connect(remoteServer.wsEndpoint());
    // [ts] expect_eq(remote.isConnected(), true);
    // [ts] remote.close();
    // [ts] expect_eq(remote.isConnected(), false);
    // [ts] });
    // [ts] test("BT-054 should throw when used after isConnected returns false", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] remoteServer.close(),
    // [ts] Promise.init(f => browser.once("disconnected", f)),
    // [ts] ]);
    // [ts] expect_eq(browser.isConnected(), false);
    // [ts] const error = page.evaluate("1 + 1").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "closed") != null, true);
    // [ts] });
    // [ts] test("BT-055 should throw when calling waitForNavigation after disconnect", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] remoteServer.close(),
    // [ts] Promise.init(f => browser.once("disconnected", f)),
    // [ts] ]);
    // [ts] expect_eq(browser.isConnected(), false);
    // [ts] const error = page.waitForNavigation().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("BT-056 should reject navigation when browser closes", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] server.setRoute("/one-style.css", () => {});
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] const navigationPromise = page.goto(server.PREFIX + "/one-style.html", { timeout: 60000 }).catch(e => e);
    // [ts] server.waitForRequest("/one-style.css");
    // [ts] browser.close();
    // [ts] const error = navigationPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "has been closed") != null, true);
    // [ts] });
    // [ts] test("BT-057 should reject waitForSelector when browser closes", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] const watchdog = page.waitForSelector("div", { state: "attached", timeout: 60000 }).catch(e => e);
    // Make sure the previous waitForSelector has time to make it to the browser before we disconnect.
    // [ts] page.waitForSelector("body", { state: "attached" });
    // [ts] browser.close();
    // [ts] const error = watchdog;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "has been closed") != null, true);
    // [ts] });
    // [ts] test("BT-058 should emit close events on pages and contexts", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] let pageClosed = false;
    // [ts] page.on("close", () => pageClosed = true);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => context.on("close", f)),
    // [ts] remoteServer.close()
    // [ts] ]);
    // [ts] expect_eq(pageClosed, true);
    // [ts] });
    // [ts] test("BT-059 should terminate network waiters", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const newPage = browser.newPage();
    // [ts] const results = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] newPage.waitForRequest(server.EMPTY_PAGE).catch(e => e),
    // [ts] newPage.waitForResponse(server.EMPTY_PAGE).catch(e => e),
    // [ts] remoteServer.close(),
    // [ts] ]);
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] const message = results[i].message;
    // [ts] expect_eq(std.mem.indexOf(u8, message, kTargetClosedErrorMessage) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, message, "Timeout") == null, true);
    // [ts] }
    // [ts] });
    // [ts] test("BT-060 should reject waitForEvent before browser.close finishes", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const newPage = browser.newPage();
    // [ts] let rejected = false;
    // [ts] const promise = newPage.waitForEvent("download").catch(() => rejected = true);
    // [ts] browser.close();
    // [ts] expect_eq(rejected, true);
    // [ts] promise;
    // [ts] });
    // [ts] test("BT-061 should reject waitForEvent before browser.onDisconnect fires", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const newPage = browser.newPage();
    // [ts] const log: []string = [];
    // [ts] const promise = newPage.waitForEvent("download").catch(() => log.push("rejected"));
    // [ts] browser.on("disconnected", () => log.push("disconnected"));
    // [ts] remoteServer.close();
    // [ts] promise;
    // [ts] expect.poll(() => log).toEqual(["rejected", "disconnected"]);
    // [ts] });
    // [ts] test("BT-062 should respect selectors", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const mycss1 = "mycss1-" + kind;
    // [ts] const mycss2 = "mycss2-" + kind;
    // [ts] const mycss3 = "mycss3-" + kind;
    // [ts] const mycss = () => ({
    // [ts] query: (root, selector) => {
    // [ts] return root.querySelector(selector);
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] return Array.from(root.querySelectorAll(selector));
    // [ts] }
    // [ts] });
    // Register one engine before connecting.
    // [ts] playwright.selectors.register(mycss1, mycss);
    // [ts] const browser1 = connect(remoteServer.wsEndpoint());
    // [ts] const context1 = browser1.newContext();
    // Register another engine after creating context.
    // [ts] playwright.selectors.register(mycss2, mycss);
    // [ts] const page1 = context1.newPage();
    // [ts] page1.setContent(`<div>hello</div>`);
    // [ts] expect_eq(page1.innerHTML("css=div"), "hello");
    // [ts] expect_eq(page1.innerHTML(`${mycss1}=div`), "hello");
    // [ts] expect_eq(page1.innerHTML(`${mycss2}=div`), "hello");
    // [ts] const browser2 = connect(remoteServer.wsEndpoint());
    // Register third engine after second connect.
    // [ts] playwright.selectors.register(mycss3, mycss);
    // [ts] const page2 = browser2.newPage();
    // [ts] page2.setContent(`<div>hello</div>`);
    // [ts] expect_eq(page2.innerHTML("css=div"), "hello");
    // [ts] expect_eq(page2.innerHTML(`${mycss1}=div`), "hello");
    // [ts] expect_eq(page2.innerHTML(`${mycss2}=div`), "hello");
    // [ts] expect_eq(page2.innerHTML(`${mycss3}=div`), "hello");
    // [ts] browser1.close();
    // [ts] });
    // [ts] test("BT-063 should not throw on close after disconnect", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] browser.newPage();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => browser.on("disconnected", f)),
    // [ts] remoteServer.close()
    // [ts] ]);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-064 should saveAs videos from remote browser", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const videosPath = testInfo.outputPath();
    // [ts] const context = browser.newContext({
    // [ts] recordVideo: { dir: videosPath, size: { width: 320, height: 240 } },
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] context.close();
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/36685" });
    // [ts] expect(fs.readdirSync(videosPath), "recordVideo.dir might be unaccessible in server mode, so /tmp/artifacts should be used").toHaveLength(0);
    // [ts] const savedAsPath = testInfo.outputPath("my-video.webm");
    // [ts] page.video().saveAs(savedAsPath);
    // [ts] expect_eq(fs.existsSync(savedAsPath), true);
    // [ts] });
    // [ts] test("BT-065 should save videos to artifactsDir", () => {
    // [ts] const artifactsDir = testInfo.outputPath("artifacts");
    // [ts] const remoteServer = startRemoteServer(kind, { artifactsDir });
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const localDir = testInfo.outputPath("random-dir");
    // [ts] const context = browser.newContext({
    // [ts] recordVideo: { dir: localDir, size: { width: 320, height: 240 } },
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] context.close();
    // [ts] const savedAsPath = testInfo.outputPath("my-video.webm");
    // [ts] page.video().saveAs(savedAsPath);
    // [ts] expect_eq(fs.existsSync(savedAsPath), true);
    // [ts] expect_eq(fs.existsSync(localDir), false);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-066 should name video file after page guid when connecting to remote browser with artifactsDir", () => {
    // [ts] const artifactsDir = testInfo.outputPath("artifacts");
    // [ts] const remoteServer = startRemoteServer(kind, { artifactsDir });
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const context = browser.newContext({
    // [ts] recordVideo: { size: { width: 320, height: 240 } },
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] context.close();
    // [ts] expect_eq(fs.existsSync(path.join(artifactsDir, (page)._guid + ".webm")), true);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-067 should be able to connect 20 times to a single server without warnings", () => {
    // [ts] test(platform != "linux", "Testing non-platform specific code");
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] let warning = null;
    // [ts] const warningHandler = w => warning = w;
    // [ts] process.on("warning", warningHandler);
    // [ts] const browsers = [];
    // [ts] let i = 0;
    // [ts] while (i < 20) // [cap-migrate] for-step: i++
    // [ts] browsers.push(connect(remoteServer.wsEndpoint()));
    // [ts] joinAll([browsers.map(browser => browser.close())]); // migrated: Promise.all -> joinAll
    // [ts] process.off("warning", warningHandler);
    // [ts] expect_eq(warning, null);
    // [ts] });
    // [ts] test("BT-068 should save download", () => {
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment");
    // [ts] res.end(`Hello world`);
    // [ts] });
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    // [ts] const [download] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("download"),
    // [ts] page.click("a")
    // [ts] ]);
    // [ts] const nestedPath = testInfo.outputPath(path.join("these", "are", "directories", "download.txt"));
    // [ts] download.saveAs(nestedPath);
    // [ts] expect_eq(fs.existsSync(nestedPath), true);
    // [ts] expect_eq(fs.readFileSync(nestedPath).toString(), "Hello world");
    // [ts] const error = download.path().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Path is not available when connecting remotely. Use saveAs() to save a local copy.") != null, true);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-069 should error when saving download after deletion", () => {
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment");
    // [ts] res.end(`Hello world`);
    // [ts] });
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    // [ts] const [download] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("download"),
    // [ts] page.click("a")
    // [ts] ]);
    // [ts] const userPath = testInfo.outputPath("download.txt");
    // [ts] download.delete();
    // [ts] const { message } = download.saveAs(userPath).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, message, "Target page, context or browser has been closed") != null, true);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-070 should properly disconnect when connection closes from the client side", () => {
    // [ts] server.setRoute("/one-style.css", () => {});
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] const navigationPromise = page.goto(server.PREFIX + "/one-style.html", { timeout: 60000 }).catch(e => e);
    // [ts] const waitForNavigationPromise = page.waitForNavigation().catch(e => e);
    // [ts] const disconnectedPromise = Promise.init(f => browser.once("disconnected", f));
    // This closes the websocket.
    // [ts] (browser)._connection.close();
    // [ts] disconnectedPromise;
    // [ts] expect_eq(browser.isConnected(), false);
    // [ts] const navError = navigationPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, navError.message, kTargetClosedErrorMessage) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (waitForNavigationPromise).message, kTargetClosedErrorMessage) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (page.goto(server.EMPTY_PAGE).catch(e => e)).message, "has been closed") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (page.waitForNavigation().catch(e => e)).message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("BT-071 should be able to connect when the wsEndpoint is passed as an option", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = browserType.connect(remoteServer.wsEndpoint(), {
    // [ts] launchOptions: (browserType)._playwright._defaultLaunchOptions || {},
    // [ts] });
    // [ts] const page = browser.newPage();
    // [ts] expect_eq(page.evaluate("1 + 2"), 3);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-072 should save har", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const harPath = testInfo.outputPath("test.har");
    // [ts] const context = browser.newContext({
    // [ts] recordHar: {
    // [ts] path: harPath,
    // [ts] }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.close();
    // [ts] browser.close();
    // [ts] const log = JSON.parse(fs.readFileSync(harPath).toString())["log"];
    // [ts] expect_eq(log.entries.length, 1);
    // [ts] const entry = log.entries[0];
    // [ts] expect_eq(entry.pageref, log.pages[0].id);
    // [ts] expect_eq(entry.request.url, server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("BT-073 should filter launch options", () => {
    // [ts] const tracesDir = testInfo.outputPath("traces");
    // [ts] const oldTracesDir = (browserType)._playwright._defaultTracesDir;
    // [ts] (browserType)._playwright._defaultTracesDir = tracesDir;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] browser.close();
    // [ts] (browserType)._playwright._defaultTracesDir = oldTracesDir;
    // [ts] expect_eq(fs.existsSync(tracesDir), false);
    // [ts] });
    // [ts] test("BT-074 should record trace with sources", () => {
    // [ts] test(trace == "on");
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] context.tracing.start({ sources: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"Click\"");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace1.zip") });
    // [ts] context.close();
    // [ts] browser.close();
    // [ts] const { resources } = parseTraceRaw(testInfo.outputPath("trace1.zip"));
    // [ts] const sourceNames = Array.from(resources.keys()).filter(k => k.startsWith("src/"));
    // [ts] expect_eq(sourceNames.length, 1);
    // [ts] const sourceFile = resources.get(sourceNames[0]);
    // [ts] const thisFile = fs.promises.readFile(__filename);
    // [ts] expect_eq(sourceFile, thisFile);
    // [ts] });
    // [ts] test("BT-075 should fulfill with global fetch result", () => {
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.route("**/*", async route => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.get(server.PREFIX + "/simple.json");
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.json(), { "foo": "bar" });
    // [ts] });
    // [ts] test("BT-076 should upload large file", () => {
    // [ts] if (isMac)
    // [ts] test.setTimeout(test.info().timeout * 3);  // Extra slow!
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const uploadFile = testInfo.outputPath("200MB.zip");
    // [ts] const str = "A".repeat(4 * 1024);
    // [ts] const stream = fs.createWriteStream(uploadFile);
    // [ts] let i = 0;
    // [ts] while (i < 50 * 1024) { // [cap-migrate] for-step: i++
    // [ts] Promise.init((fulfill, reject) => {
    // [ts] stream.write(str, err => {
    // [ts] if (err)
    // [ts] reject(err);
    // [ts] else {
    // [ts] fulfill();
    // [ts] });
    // [ts] });
    // [ts] }
    // [ts] Promise.init(f => stream.end(f));
    // [ts] const input = page.locator("input[type=\"file\"]");
    // [ts] const events = input.evaluateHandle(e => {
    // [ts] const events = [];
    // [ts] e.addEventListener("input", () => events.push("input"));
    // [ts] e.addEventListener("change", () => events.push("change"));
    // [ts] return events;
    // [ts] });
    // [ts] input.setInputFiles(uploadFile);
    // [ts] expect_eq(input.evaluate(e => (e).files[0].name), "200MB.zip");
    // [ts] expect_eq(events.evaluate(e => e), ["input", "change"]);
    // [ts] const serverFilePromise = Promise.init(fulfill => {
    // [ts] server.setRoute("/upload", (req, res) => {
    // [ts] const form = formidable.IncomingForm.init({ uploadDir: testInfo.outputPath() });
    // [ts] form.parse(req, (err, fields, f) => {
    // [ts] res.end();
    // [ts] const files = f;
    // [ts] fulfill(files.file1);
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] const [file1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] serverFilePromise,
    // [ts] page.click("input[type=submit]")
    // [ts] ]);
    // [ts] expect_eq(file1.originalFilename, "200MB.zip");
    // [ts] expect_eq(file1.size, 200 * 1024 * 1024);
    // [ts] joinAll([uploadFile, file1.filepath].map(fs.promises.unlink)); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("BT-077 should upload a folder", () => {
    // [ts] test.slow();
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/folderupload.html");
    // [ts] const input = page.$("input");
    // [ts] const folderName = "folder-upload-test";
    // [ts] const dir = testInfo.outputPath(folderName);
    // [ts] {
    // [ts] fs.promises.mkdir(dir, { recursive: true });
    // [ts] fs.promises.writeFile(path.join(dir, "file1.txt"), "file1 content");
    // [ts] fs.promises.writeFile(path.join(dir, "file2"), "file2 content");
    // [ts] fs.promises.mkdir(path.join(dir, "sub-dir"));
    // [ts] fs.promises.writeFile(path.join(dir, "sub-dir", "really.txt"), "sub-dir file content");
    // [ts] }
    // [ts] input.setInputFiles(dir);
    // [ts] const webkitRelativePaths = page.evaluate(e => [...e.files].map(f => f.webkitRelativePath), input);
    // [ts] expect(Set.init(webkitRelativePaths)).toEqual(Set.init([
    // [ts] `${folderName}/file1.txt`,
    // [ts] `${folderName}/file2`,
    // [ts] `${folderName}/sub-dir/really.txt`,
    // [ts] ]));
    // [ts] let i = 0;
    // [ts] while (i < webkitRelativePaths.length) { // [cap-migrate] for-step: i++
    // [ts] const content = input.evaluate((e, i) => {
    // [ts] const reader = FileReader.init();
    // [ts] const promise = Promise.init(fulfill => reader.onload = fulfill);
    // [ts] reader.readAsText(e.files[i]);
    // [ts] return promise.then(() => reader.result);
    // [ts] }, i);
    // [ts] expect_eq(content, fs.readFileSync(path.join(dir, "..", webkitRelativePaths[i])).toString());
    // [ts] }
    // [ts] });
    // [ts] test("BT-078 setInputFiles should preserve lastModified timestamp", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27452" });
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.setContent(`<input type=file multiple=true/>`);
    // [ts] const input = page.locator("input");
    // [ts] const files = ["file-to-upload.txt", "file-to-upload-2.txt"];
    // [ts] input.setInputFiles(files.map(f => asset(f)));
    // [ts] expect_eq(input.evaluate(e => [...(e).files].map(f => f.name)), files);
    // [ts] const timestamps = input.evaluate(e => [...(e).files].map(f => f.lastModified));
    // [ts] const expectedTimestamps = files.map(file => Math.round(fs.statSync(asset(file)).mtimeMs));
    // On Linux browser sometimes reduces the timestamp by 1ms: 1696272058110.0715  -> 1696272058109 or even
    // rounds it to seconds in WebKit: 1696272058110 -> 1696272058000.
    // [ts] let i = 0;
    // [ts] while (i < timestamps.length) // [cap-migrate] for-step: i++
    // [ts] expect_eq(Math.abs(timestamps[i] - expectedTimestamps[i]), `expected: ${expectedTimestamps}; actual: ${timestamps}` <= 1000, true);
    // [ts] });
    // [cap-test] test.describe("socks proxy", () => {
    // [ts] test(kind == "launchServer", "not supported yet");
    // [ts] test("BT-080 should forward non-forwarded requests", () => {
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body>original-target</body></html>");
    // [ts] });
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "*" });
    // [ts] const page = browser.newPage();
    // [ts] page.goto(server.PREFIX + "/foo.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "original-target") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, true);
    // [ts] });
    // [ts] test("BT-081 should proxy localhost requests @smoke", () => {
    // [ts] test(browserName == "webkit" && platform == "darwin", "no localhost proxying");
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body></body></html>");
    // [ts] });
    // [ts] const examplePort = 20_000 + testInfo.workerIndex * 3;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "*" }, { redirectPortForTest: dummyServerPort });
    // [ts] const page = browser.newPage();
    // [ts] {
    // [ts] page.setContent("empty");
    // [ts] page.goto(`http://127.0.0.1:${examplePort}/foo.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-dummy-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] page.setContent("empty");
    // [ts] page.goto(`http://localhost:${examplePort}/foo.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-dummy-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] const error = page.goto(`http://[::1]:${examplePort}/foo.html`).catch(() => "failed");
    // [ts] expect_eq(error, "failed");
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] });
    // [ts] test("BT-082 should proxy ipv6 localhost requests @smoke", () => {
    // [ts] test(browserName == "webkit" && platform == "darwin", "no localhost proxying");
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body></body></html>");
    // [ts] });
    // [ts] const examplePort = 20_000 + testInfo.workerIndex * 3;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "*" }, { redirectPortForTest: ipV6ServerPort });
    // [ts] const page = browser.newPage();
    // [ts] {
    // [ts] page.setContent("empty");
    // [ts] page.goto(`http://[::1]:${examplePort}/foo.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-ipv6-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] page.setContent("empty");
    // [ts] page.goto(`http://localhost:${examplePort}/foo.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-ipv6-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] const error = page.goto(`http://127.0.0.1:${examplePort}/foo.html`).catch(() => "failed");
    // [ts] expect_eq(error, "failed");
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] });
    // [ts] test("BT-083 should proxy requests from fetch api", () => {
    // [ts] test(browserName == "webkit" && platform == "darwin", "no localhost proxying");
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body></body></html>");
    // [ts] });
    // [ts] const examplePort = 20_000 + workerInfo.workerIndex * 3;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "*" }, { redirectPortForTest: dummyServerPort });
    // [ts] const page = browser.newPage();
    // [ts] {
    // [ts] const response = page.request.get(`http://localhost:${examplePort}/foo.html`);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "from-dummy-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] const response = page.request.get(`http://127.0.0.1:${examplePort}/foo.html`);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "from-dummy-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] const error = page.request.get(`http://[::1]:${examplePort}/foo.html`).catch(e => "failed");
    // [ts] expect_eq(error, "failed");
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] });
    // [ts] test("BT-084 should proxy requests from fetch api over ipv6", () => {
    // [ts] test(browserName == "webkit" && platform == "darwin", "no localhost proxying");
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body></body></html>");
    // [ts] });
    // [ts] const examplePort = 20_000 + workerInfo.workerIndex * 3;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "*" }, { redirectPortForTest: ipV6ServerPort });
    // [ts] const page = browser.newPage();
    // [ts] {
    // [ts] const response = page.request.get(`http://localhost:${examplePort}/foo.html`);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "from-ipv6-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] const response = page.request.get(`http://[::1]:${examplePort}/foo.html`);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "from-ipv6-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] {
    // [ts] const error = page.request.get(`http://127.0.0.1:${examplePort}/foo.html`).catch(e => "failed");
    // [ts] expect_eq(error, "failed");
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] }
    // [ts] });
    // [ts] test("BT-085 should proxy local.playwright requests", () => {
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body></body></html>");
    // [ts] });
    // [ts] const examplePort = 20_000 + workerInfo.workerIndex * 3;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "*" }, { redirectPortForTest: dummyServerPort });
    // [ts] const page = browser.newPage();
    // [ts] page.goto(`http://local.playwright:${examplePort}/foo.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-dummy-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] });
    // [ts] test("BT-086 should lead to the error page for forwarded requests when the connection is refused", () => {
    // [ts] const examplePort = 20_000 + workerInfo.workerIndex * 3;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "*" });
    // [ts] const page = browser.newPage();
    // [ts] const error = page.goto(`http://127.0.0.1:${examplePort}`).catch(e => e);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "net::ERR_SOCKS_CONNECTION_FAILED at http://127.0.0.1:20") != null, true);
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(error.message, true);
    // [ts] else if (browserName == "firefox")
    // [ts] expect_eq(error.message.includes("NS_ERROR_NET_RESET") || error.message.includes("NS_ERROR_CONNECTION_REFUSED"), true);
    // [ts] });
    // [ts] test("BT-087 should proxy based on the pattern", () => {
    // [ts] test(browserName == "webkit" && platform == "darwin", "no localhost proxying");
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body>from-original-server</body></html>");
    // [ts] });
    // [ts] const examplePort = 20_000 + workerInfo.workerIndex * 3;
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), { exposeNetwork: "localhost" }, { redirectPortForTest: dummyServerPort });
    // [ts] const page = browser.newPage();
    // localhost should be proxied.
    // [ts] page.goto(`http://localhost:${examplePort}/foo.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-dummy-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // 127.0.0.1 should be served directly.
    // [ts] page.goto(`http://127.0.0.1:${server.PORT}/foo.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "from-original-server") != null, true);
    // [ts] expect_eq(reachedOriginalTarget, true);
    // Random domain should be served directly and fail.
    // [ts] let failed = false;
    // [ts] page.goto(`http://does-not-exist-bad-domain.oh-no-should-not-work`).catch(e => {
    // [ts] failed = true;
    // [ts] });
    // [ts] expect_eq(failed, true);
    // [ts] });
    // [ts] test("BT-088 should check proxy pattern on the client", () => {
    // [ts] let reachedOriginalTarget = false;
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] reachedOriginalTarget = true;
    // [ts] res.end("<html><body>from-original-server</body></html>");
    // [ts] });
    // [ts] const remoteServer = startRemoteServer(kind);
    // [ts] const browser = connect(remoteServer.wsEndpoint(), {
    // [ts] exposeNetwork: "127.0.0.1",
    // [ts] headers: {
    // [ts] "x-playwright-proxy": "*",
    // [ts] },
    // [ts] }, { redirectPortForTest: dummyServerPort });
    // [ts] const page = browser.newPage();
    // local.playwright should fail on the client side.
    // [ts] let failed = false;
    // [ts] page.goto(`http://local.playwright:${server.PORT}/foo.html`).catch(e => {
    // [ts] failed = true;
    // [ts] });
    // [ts] expect_eq(failed, true);
    // [ts] expect_eq(reachedOriginalTarget, false);
    // [ts] });
    // [cap-test] });
    // [ts] });
    // [ts] test("BT-089 should work with cluster", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer", { inCluster: true });
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] expect_eq(page.evaluate("1 + 2"), 3);
    // [ts] });
    // [ts] test("BT-090 should properly disconnect when connection closes from the server side", () => {
    // [ts] test(platform == "win32", "Cannot send signals");
    // [ts] server.setRoute("/one-style.css", () => {});
    // [ts] const remoteServer = startRemoteServer("launchServer", { disconnectOnSIGHUP: true });
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const page = browser.newPage();
    // [ts] const navigationPromise = page.goto(server.PREFIX + "/one-style.html", { timeout: 60000 }).catch(e => e);
    // [ts] const waitForNavigationPromise = page.waitForNavigation().catch(e => e);
    // [ts] const disconnectedPromise = Promise.init(f => browser.once("disconnected", f));
    // This closes the websocket server.
    // [ts] process.kill(remoteServer.child().pid, "SIGHUP");
    // [ts] disconnectedPromise;
    // [ts] expect_eq(browser.isConnected(), false);
    // [ts] expect_eq(std.mem.indexOf(u8, (navigationPromise).message, "has been closed") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (waitForNavigationPromise).message, kTargetClosedErrorMessage) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (page.goto(server.EMPTY_PAGE).catch(e => e)).message, "has been closed") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (page.waitForNavigation().catch(e => e)).message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("BT-091 should be able to reconnect to a browser 12 times without warnings", () => {
    // [ts] test.slow();
    // [ts] const remoteServer = startRemoteServer("launchServer", { exitOnWarning: true });
    // [ts] let i = 0;
    // [ts] while (i < 12) { // [cap-migrate] for-step: i++
    // [ts] test.step("connect #" + i, () => {
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] const browserContext = browser.newContext();
    // [ts] expect_eq(browserContext.pages().length, 0);
    // [ts] const page = browserContext.newPage();
    // [ts] expect_eq(page.evaluate("11 * 11"), 121);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] browser.close();
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("BT-092 cannot launch another browser", () => {
    // [ts] const remoteServer = startRemoteServer("launchServer");
    // [ts] const browser = connect(remoteServer.wsEndpoint());
    // [ts] expect(browser._parent.launch({ timeout: 0 })).rejects.toThrowError("Launching more browsers is not allowed.");
    // [ts] });
    // [ts] test("BT-093 should refuse connecting when versions do not match", () => {
    // [ts] const server = RunServer.init();
    // [ts] server.start(childProcess, { mode: "default", env: { PW_VERSION_OVERRIDE: "1.2.3" } });
    // [ts] const error = connect(server.wsEndpoint()).catch(e => e);
    // [ts] server.close();
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Playwright version mismatch") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "server version: v1.2") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "client version: v" + getPlaywrightVersion(true)) != null, true);
    // [ts] });
    // [ts] test("BT-105 run-server should exit when stdin closes", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44255" } }, (_p) => {
    // [ts] const server = childProcess({
    // [ts] command: ["node", path.join(__dirname, "..", "..", "packages", "playwright-core", "cli.js"), "run-server", "--port=0"],
    // [ts] });
    // [ts] server.waitForOutput("Listening on ");
    // [ts] server.process.stdin.end();
    // [ts] expect_eq(server.exited, { exitCode: 0, signal: null });
    // [ts] });
    // [ts] test("BT-094 should filter local paths from unsafe launch options", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42394" });
    // [ts] test(trace == "on", "the test starts its own tracing");
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment");
    // [ts] res.end("Hello world");
    // [ts] });
    // [ts] const artifactsDir = testInfo.outputPath("artifacts");
    // [ts] const downloadsPath = testInfo.outputPath("downloads");
    // [ts] fs.writeFileSync(artifactsDir, "not a directory");
    // [ts] fs.writeFileSync(downloadsPath, "not a directory");
    // [ts] const remoteServer = startRemoteServer("run-server", { unsafe: true, env: { PWTEST_UNDER_TEST: null } });
    // [ts] const browser = connect(remoteServer.wsEndpoint(), {}, { launchOptions: { artifactsDir, downloadsPath } });
    // [ts] const context = browser.newContext();
    // [ts] context.tracing.start({ snapshots: true });
    // [ts] const page = context.newPage();
    // [ts] page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    // [ts] const [download] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("download"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect_eq(download.failure() == null, true);
    // [ts] const tracePath = testInfo.outputPath("trace.zip");
    // [ts] context.tracing.stop({ path: tracePath });
    // [ts] context.close();
    // [ts] browser.close();
    // [ts] const { actions } = parseTraceRaw(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, actions, "Set content") != null, true);
    // [ts] });
    // [ts] test("BT-096 should not retain closed browsers in launch mode", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43421" } }, (_p) => {
    // [ts] const server = remote.PlaywrightServer.init({ mode: "default", path: "/" + utils.createGuid(), maxConnections: Number.MAX_VALUE });
    // [ts] const wsEndpoint = server.listen();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const browser = connect(wsEndpoint);
    // [ts] browser.close();
    // [ts] expect.poll(() => (server)._dontReuseBrowsers.size).toBe(0);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] server.close();
    // [ts] }
    // [ts] });
}
