// Migrated from TypeScript (library/chromium/connect-over-cdp.spec.ts) via cap from -ts

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

import { playwrightTest as test, expect } from "../../config/browserTest.cp";
import http from "cap:http";
import fs from "cap:fs";
import path from "cap:path";
import { getUserAgent, server as coreServer } from "../../../packages/playwright-core/lib/coreBundle.cp";
import { suppressCertificateWarning } from "../../config/utils.cp";

const { WebSocketTransport, nullProgress } = coreServer;
type Frame = coreServer.Frame;

























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CD-001 should connect to an existing cdp session", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43880" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const configHome = createUserDataDir();
    // [ts] const userDataDir = path.join(configHome, "google-chrome");
    // [ts] const browserServer = browserType.launchPersistentContext(userDataDir, {
    // [ts] args: ["--remote-debugging-port=0"]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const [port] = (fs.promises.readFile(path.join(userDataDir, "DevToolsActivePort"), "utf-8")).split("\n");
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] cdpBrowser.close();
    // [ts] const urlBrowser = browserType.connectOverCDP(`remote-debugging://chromium?${URLSearchParams.init({ userDataDir })}`);
    // [ts] expect_eq(urlBrowser.contexts().length, 1);
    // [ts] urlBrowser.close();
    // [ts] if (process.platform == "linux") {
    // [ts] for (const env of [
    // [ts] { CHROME_USER_DATA_DIR: "", CHROME_CONFIG_HOME: "", XDG_CONFIG_HOME: configHome },
    // [ts] { CHROME_USER_DATA_DIR: "", CHROME_CONFIG_HOME: configHome, XDG_CONFIG_HOME: testInfo.outputPath("unused-config") },
    // [ts] { CHROME_USER_DATA_DIR: userDataDir, CHROME_CONFIG_HOME: testInfo.outputPath("unused-config"), XDG_CONFIG_HOME: testInfo.outputPath("unused-xdg-config") },
    // [ts] ]) {
    // [ts] const child = childProcess({
    // [ts] command: [process.execPath, "-e", `
    // [ts] const { chromium } = require(${JSON.stringify(require.resolve('playwright-core'))});
    // [ts] (() => {
    // [ts] const browser = chromium.connectOverCDP('chrome');
    // [ts] console.log('contexts: ' + browser.contexts().length);
    // [ts] browser.close();
    // [ts] })().catch(error => {
    // [ts] console.error(error);
    // [ts] process.exitCode = 1;
    // [ts] });
    // [ts] `],
    // [ts] env,
    // [ts] });
    // [ts] child.cleanExit();
    // [ts] expect_eq(std.mem.indexOf(u8, child.stdout, "contexts: 1") != null, true);
    // [ts] }
    // [ts] }
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-002 should connect when an existing page has no renderer", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41714" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // Prepare a page without a renderer, e.g. one discarded by the Memory Saver.
    // [ts] const cdpBrowser1 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const context1 = cdpBrowser1.contexts()[0];
    // [ts] const healthyPage1 = context1.newPage();
    // [ts] healthyPage1.goto(server.PREFIX + "/title.html");
    // [ts] const crashedPage1 = context1.newPage();
    // [ts] crashedPage1.goto(server.EMPTY_PAGE);
    // [ts] crashedPage1.goto("chrome://crash").catch(() => {});
    // [ts] crashedPage1.waitForEvent("crash");
    // [ts] cdpBrowser1.close();
    // Connecting again should not hang on the page without a renderer,
    // and should not report it at all.
    // [ts] const cdpBrowser2 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const pages = cdpBrowser2.contexts()[0].pages();
    // [ts] expect_eq(pages.map(page => page.url()), [server.PREFIX + "/title.html"]);
    // [ts] expect_eq(pages[0].title(), "Woof-Woof");
    // [ts] cdpBrowser2.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-003 should connect when the opener of an existing page has no renderer", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41714" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // The popup should be in a different process to survive the crash of the opener.
    // [ts] args: ["--remote-debugging-port=" + port, "--site-per-process"]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser1 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const crashedPage1 = cdpBrowser1.contexts()[0].newPage();
    // [ts] crashedPage1.goto(server.EMPTY_PAGE);
    // [ts] const [popup1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] crashedPage1.waitForEvent("popup"),
    // [ts] crashedPage1.evaluate(url => { window.open(url); }, server.CROSS_PROCESS_PREFIX + "/title.html"),
    // [ts] ]);
    // [ts] popup1.waitForLoadState();
    // [ts] crashedPage1.goto("chrome://crash").catch(() => {});
    // [ts] crashedPage1.waitForEvent("crash");
    // [ts] cdpBrowser1.close();
    // Connecting again should not report the opener, and should not wait for it.
    // [ts] const cdpBrowser2 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const pages = cdpBrowser2.contexts()[0].pages();
    // [ts] expect_eq(pages.map(page => page.url()), [server.CROSS_PROCESS_PREFIX + "/title.html"]);
    // [ts] expect_eq(pages[0].title(), "Woof-Woof");
    // [ts] expect_eq(pages[0].opener(), null);
    // [ts] cdpBrowser2.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-004 should connect when an existing page has been discarded", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41714" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const userDataDir = createUserDataDir();
    // chrome://discards is an internal debug page that is only available with this pref.
    // [ts] fs.writeFileSync(path.join(userDataDir, "Local State"), JSON.stringify({ internal_only_uis_enabled: true }));
    // [ts] const context = browserType.launchPersistentContext(userDataDir, {
    // [ts] headless: false,
    // Emulating the viewport of a discarded tab crashes the browser in
    // WebContentsImpl::SetDeviceEmulationSize, because it has no view.
    // [ts] viewport: null,
    // [ts] args: [
    // [ts] "--remote-debugging-port=" + port,
    // Chrome refuses to discard tabs with DevTools attached, and we attach to all of them.
    // [ts] "--enable-features=AllowDevtoolsConnectedDiscard",
    // [ts] ],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const victimUrl = server.PREFIX + "/title.html";
    // [ts] const victim = context.newPage();
    // [ts] victim.goto(victimUrl);
    // [ts] const discards = context.newPage();
    // [ts] discards.goto("chrome://discards/");
    // Discarding replaces the tab's WebContents, but the url in the table survives.
    // [ts] const row = discards.getByRole("row", { name: victimUrl });
    // This click hangs on the macOS bots sometimes. The timeout makes the error show the call log.
    // [ts] row.getByText("Urgent Discard").click({ timeout: 10000 });
    // [ts] expect(row).toContainText("discarded");
    // The renderer process is shut down asynchronously after the discard.
    // [ts] discards.waitForTimeout(3000);
    // [ts] testInfo.attach("discards rows", { body: (discards.getByRole("row").allInnerTexts()).map(text => text.replace("\\s+", " ")).join("\n") });
    // [ts] const targets = (fetch(`http://127.0.0.1:${port}/json/list`)).json();
    // [ts] testInfo.attach("targets", { body: JSON.stringify(targets, null, 2), contentType: "application/json" });
    // Connecting should not hang on the discarded page, and the page should not be reported.
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const pages = cdpBrowser.contexts()[0].pages();
    // [ts] expect_eq(pages.map(page => page.url()).sort(), ["about:blank", discards.url()].sort());
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-005 should cleanup artifacts dir after connectOverCDP disconnects due to ws close", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const dir = toImpl(cdpBrowser).options.artifactsDir;
    // [ts] const exists1 = fs.existsSync(dir);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => cdpBrowser.on("disconnected", f)),
    // [ts] browserServer.close()
    // [ts] ]);
    // [ts] const exists2 = fs.existsSync(dir);
    // [ts] expect_eq(exists1, true);
    // [ts] expect_eq(exists2, false);
    // [ts] });
    // [ts] test("CD-006 should reject pending CDPSession calls when the browser is killed", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43200" },
    // [ts] }, (_p, testInfo) => {
    // [ts] test(mode != "default", "launchServer is only available in-process");
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launchServer({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const context = cdpBrowser.contexts()[0];
    // [ts] const page = context.newPage();
    // [ts] const session = context.newCDPSession(page);
    // [ts] const sendPromise = session.send("Runtime.evaluate", { expression: "new Promise(() => {})", awaitPromise: true }).catch(e => e);
    // [ts] browserServer.kill();
    // [ts] const error = sendPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Target page, context or browser has been closed") != null, true);
    // [ts] });
    // [ts] test("CD-007 should write traces to provided artifactsDir on connectOverCDP", () => {
    // [ts] test(trace == "on");
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] const artifactsDir = testInfo.outputPath("custom-artifacts");
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] artifactsDir,
    // [ts] });
    // [ts] expect_eq(toImpl(cdpBrowser).options.artifactsDir, artifactsDir);
    // [ts] expect_eq(toImpl(cdpBrowser).options.tracesDir, artifactsDir);
    // [ts] const context = cdpBrowser.contexts()[0];
    // [ts] context.tracing.start({ name: "cdp-trace", snapshots: true, screenshots: true });
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<button>Hello</button>");
    // The trace file of a discarded chunk is removed, check it before the chunk stops.
    // [ts] expect.poll(() => fs.existsSync(path.join(artifactsDir, "cdp-trace.trace"))).toBe(true);
    // [ts] context.tracing.stopChunk();
    // [ts] expect_eq(fs.existsSync(path.join(artifactsDir, "cdp-trace.network")), true);
    // [ts] expect_eq(fs.existsSync(path.join(artifactsDir, "resources")), true);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => cdpBrowser.on("disconnected", f)),
    // [ts] browserServer.close()
    // [ts] ]);
    // [ts] expect_eq(fs.existsSync(artifactsDir), true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close().catch(() => {});
    // [ts] }
    // [ts] });
    // [ts] test("CD-008 should connectOverCDP and manage downloads in default context", () => {
    // [ts] server.setRoute("/downloadWithFilename", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment; filename=file.txt");
    // [ts] res.end(`Hello world`);
    // [ts] });
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const browser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const page = browser.contexts()[0].newPage();
    // [ts] page.setContent(`<a href="${server.PREFIX}/downloadWithFilename">download</a>`);
    // [ts] const [download] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("download"),
    // [ts] page.click("a")
    // [ts] ]);
    // [ts] expect_eq(download.page(), page);
    // [ts] expect_eq(download.url(), `${server.PREFIX}/downloadWithFilename`);
    // [ts] expect_eq(download.suggestedFilename(), `file.txt`);
    // [ts] const userPath = testInfo.outputPath("download.txt");
    // [ts] download.saveAs(userPath);
    // [ts] expect_eq(fs.existsSync(userPath), true);
    // [ts] expect_eq(fs.readFileSync(userPath).toString(), "Hello world");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-009 should connect to an existing cdp session twice", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser1 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const cdpBrowser2 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const contexts1 = cdpBrowser1.contexts();
    // [ts] expect_eq(contexts1.length, 1);
    // [ts] const [context1] = contexts1;
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] const contexts2 = cdpBrowser2.contexts();
    // [ts] expect_eq(contexts2.length, 1);
    // [ts] const [context2] = contexts2;
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(context1.pages().length, 2);
    // [ts] expect_eq(context2.pages().length, 2);
    // [ts] page1.evaluate(() => alert("dialog race"));
    // [ts] expect(context2.pages()[0].title()).resolves.toBe("");
    // [ts] cdpBrowser1.close();
    // [ts] cdpBrowser2.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-010 should connect to existing page with iframe and navigate", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] {
    // [ts] const context1 = browserServer.newContext();
    // [ts] const page = context1.newPage();
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] }
    // [ts] const cdpBrowser = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] contexts[0].pages()[0].goto(server.EMPTY_PAGE);
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-011 should connect to existing service workers", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser1 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}`,
    // [ts] });
    // [ts] const context = cdpBrowser1.contexts()[0];
    // [ts] const page = cdpBrowser1.contexts()[0].newPage();
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("serviceworker"),
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html")
    // [ts] ]);
    // [ts] expect_eq(worker.evaluate(() => self.toString()), "[object ServiceWorkerGlobalScope]");
    // [ts] cdpBrowser1.close();
    // [ts] const cdpBrowser2 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}`,
    // [ts] });
    // [ts] const context2 = cdpBrowser2.contexts()[0];
    // [ts] expect_eq(context2.serviceWorkers().length, 1);
    // [ts] cdpBrowser2.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-012 should connect over a ws endpoint", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const json = Promise.init((resolve, reject) => {
    // [ts] http.get(`http://127.0.0.1:${port}/json/version/`, resp => {
    // [ts] let data = "";
    // [ts] resp.on("data", chunk => data += chunk);
    // [ts] resp.on("end", () => resolve(data));
    // [ts] }).on("error", reject);
    // [ts] });
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: JSON.parse(json).webSocketDebuggerUrl,
    // [ts] });
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] cdpBrowser.close();
    // also connect with the deprecated wsEndpoint option
    // [ts] const cdpBrowser2 = browserType.connectOverCDP({
    // [ts] wsEndpoint: JSON.parse(json).webSocketDebuggerUrl,
    // [ts] });
    // [ts] const contexts2 = cdpBrowser2.contexts();
    // [ts] expect_eq(contexts2.length, 1);
    // [ts] cdpBrowser2.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-013 should send extra headers with connect request", () => {
    // [ts] {
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForWebSocketConnectionRequest(),
    // [ts] browserType.connectOverCDP({
    // [ts] wsEndpoint: `ws://localhost:${server.PORT}/ws`,
    // [ts] headers: {
    // [ts] "User-Agent": "Playwright",
    // [ts] "foo": "bar",
    // [ts] },
    // [ts] timeout: 100,
    // [ts] }).catch(() => {})
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "Playwright");
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] }
    // [ts] {
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/json/version/"),
    // [ts] browserType.connectOverCDP({
    // [ts] endpointURL: `http://localhost:${server.PORT}`,
    // [ts] headers: {
    // [ts] "User-Agent": "Playwright",
    // [ts] "foo": "bar",
    // [ts] },
    // [ts] timeout: 100,
    // [ts] }).catch(() => {})
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "Playwright");
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] }
    // [ts] });
    // [ts] test("CD-014 should keep URL parameters when adding json/version", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/36097"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/browser/json/version/?foo=bar"),
    // [ts] browserType.connectOverCDP(`http://localhost:${server.PORT}/browser/?foo=bar`).catch(() => {})
    // [ts] ]);
    // [ts] });
    // [ts] test("CD-015 should append /json/version with a slash if there isnt one", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/36378"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/browser/json/version/?foo=bar"),
    // [ts] browserType.connectOverCDP(`http://localhost:${server.PORT}/browser?foo=bar`).catch(() => {})
    // [ts] ]);
    // [ts] });
    // [ts] test("CD-016 should send default User-Agent header with connect request", () => {
    // [ts] {
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForWebSocketConnectionRequest(),
    // [ts] browserType.connectOverCDP({
    // [ts] wsEndpoint: `ws://localhost:${server.PORT}/ws`,
    // [ts] headers: {
    // [ts] "foo": "bar",
    // [ts] },
    // [ts] timeout: 100,
    // [ts] }).catch(() => {})
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], getUserAgent());
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] }
    // [ts] });
    // [ts] test("CD-017 should report all pages in an existing browser", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] let i = 0;
    // [ts] while (i < 3) // [cap-migrate] for-step: i++
    // [ts] contexts[0].newPage();
    // [ts] cdpBrowser.close();
    // [ts] const cdpBrowser2 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] expect_eq(cdpBrowser2.contexts()[0].pages().length, 3);
    // [ts] cdpBrowser2.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-018 should connect via https", () => {
    // [ts] test(mode != "default"); // Out of process transport does not allow us to set env vars dynamically.
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] const json = Promise.init((resolve, reject) => {
    // [ts] http.get(`http://127.0.0.1:${port}/json/version/`, resp => {
    // [ts] let data = "";
    // [ts] resp.on("data", chunk => data += chunk);
    // [ts] resp.on("end", () => resolve(data));
    // [ts] }).on("error", reject);
    // [ts] });
    // [ts] httpsServer.setRoute("/json/version/", (req, res) => {
    // [ts] res.writeHead(200);
    // [ts] res.end(json);
    // [ts] });
    // [ts] const oldValue = process.env["NODE_TLS_REJECT_UNAUTHORIZED"];
    // https://stackoverflow.com/a/21961005/552185
    // [ts] process.env["NODE_TLS_REJECT_UNAUTHORIZED"] = "0";
    // [ts] suppressCertificateWarning();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP(`https://localhost:${httpsServer.PORT}/`);
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] let i = 0;
    // [ts] while (i < 3) // [cap-migrate] for-step: i++
    // [ts] contexts[0].newPage();
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] process.env["NODE_TLS_REJECT_UNAUTHORIZED"] = oldValue;
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-019 should return valid browser from context.browser()", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] expect_eq(contexts[0].browser(), cdpBrowser);
    // [ts] const context2 = cdpBrowser.newContext();
    // [ts] expect_eq(context2.browser(), cdpBrowser);
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-055 should work with registered selector engines", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44381" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] playwright.selectors.register("cdpTag", () => ({
    // [ts] query: (root, selector) => root.querySelector(selector),
    // [ts] queryAll: (root, selector) => Array.from(root.querySelectorAll(selector)),
    // [ts] }));
    // [ts] const cdpBrowser = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const page = cdpBrowser.contexts()[0].newPage();
    // [ts] page.setContent("<div>hello</div>");
    // [ts] expect_eq(page.locator("cdpTag=div").textContent(), "hello");
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-056 should respect the test id attribute in strict mode errors", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44381" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] playwright.selectors.setTestIdAttribute("data-foo");
    // [ts] const cdpBrowser = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const page = cdpBrowser.contexts()[0].newPage();
    // [ts] page.setContent("<button data-foo=\"a\">x</button><button data-foo=\"b\">x</button>");
    // [ts] expect(page.locator("button").click()).rejects.toThrow(`aka getByTestId('a')`);
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] playwright.selectors.setTestIdAttribute("data-testid");
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-020 should report an expected error when the endpointURL returns a non-expected status code", () => {
    // [ts] server.setRoute("/json/version/", (req, resp) => {
    // [ts] resp.statusCode = 404;
    // [ts] resp.end(JSON.stringify({
    // [ts] webSocketDebuggerUrl: "dont-use-me",
    // [ts] }));
    // [ts] });
    // [ts] expect(browserType.connectOverCDP({
    // [ts] endpointURL: server.PREFIX,
    // [ts] })).rejects.toThrowError(`Unexpected status 404 when connecting to ${server.PREFIX}/json/version/`);
    // [ts] });
    // [ts] test("CD-021 should report an expected error when the endpoint URL JSON webSocketDebuggerUrl is undefined", () => {
    // [ts] server.setRoute("/json/version/", (req, resp) => {
    // [ts] resp.end(JSON.stringify({
    // [ts] webSocketDebuggerUrl: null,
    // [ts] }));
    // [ts] });
    // [ts] expect(browserType.connectOverCDP({
    // [ts] endpointURL: server.PREFIX,
    // [ts] })).rejects.toThrowError("Invalid URL");
    // [ts] });
    // [ts] test("CD-022 should connect to an existing cdp session when passed as a first argument", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-023 should use proxy with connectOverCDP", () => {
    // [ts] server.setRoute("/target.html", (req, res) => {
    // [ts] res.end("<html><title>Served by the proxy</title></html>");
    // [ts] });
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const context = cdpBrowser.newContext({
    // [ts] proxy: { server: `localhost:${server.PORT}` }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto("http://non-existent.test/target.html");
    // [ts] expect_eq(page.title(), "Served by the proxy");
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-024 should use env proxy with connectOverCDP discovery request", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40394" });
    // [ts] test(mode != "default"); // Out of process transport does not allow us to set env vars dynamically.
    // [ts] proxyServer.forwardTo(server.PORT);
    // [ts] const oldValue = process.env.HTTP_PROXY;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] process.env.HTTP_PROXY = proxyServer.URL;
    // [ts] const error = browserType.connectOverCDP(server.PREFIX).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Unexpected status 404 when connecting to ${server.PREFIX}/json/version/`) != null, true);
    // [ts] expect_eq(proxyServer.requestUrls, [`${server.PREFIX}/json/version/`]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] if (oldValue == null)
    // [ts] delete process.env.HTTP_PROXY;
    // [ts] else {
    // [ts] process.env.HTTP_PROXY = oldValue;
    // [ts] }
    // [ts] });
    // [ts] test("CD-025 should send target Host header when using env HTTP proxy with connectOverCDP", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40811" });
    // [ts] test(mode != "default"); // Out of process transport does not allow us to set env vars dynamically.
    // [ts] proxyServer.forwardTo(server.PORT);
    // [ts] const oldValue = process.env.HTTP_PROXY;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] process.env.HTTP_PROXY = proxyServer.URL;
    // [ts] const error = browserType.connectOverCDP(server.PREFIX).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Unexpected status 404 when connecting to ${server.PREFIX}/json/version/`) != null, true);
    // [ts] expect_eq(proxyServer.requestHosts, [URL.init(server.PREFIX).host]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] if (oldValue == null)
    // [ts] delete process.env.HTTP_PROXY;
    // [ts] else {
    // [ts] process.env.HTTP_PROXY = oldValue;
    // [ts] }
    // [ts] });
    // [ts] test("CD-026 should be able to connect via localhost", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP(`http://localhost:${port}`);
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-027 emulate media should not be affected by second connectOverCDP with noDefaults", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/24109" });
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] async fn isPrint(page: auto) {
    // [ts] return page.evaluate(() => matchMedia("print").matches);
    // [ts] }
    // [ts] const browser1 = browserType.connectOverCDP(`http://localhost:${port}`);
    // [ts] const context1 = browser1.newContext();
    // [ts] const page1 = context1.newPage();
    // [ts] page1.emulateMedia({ media: "print" });
    // [ts] expect_eq(isPrint(page1), true);
    // [ts] const browser2 = browserType.connectOverCDP(`http://localhost:${port}`, { noDefaults: true });
    // [ts] expect_eq(isPrint(page1), true);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] browser1.close(),
    // [ts] browser2.close()
    // [ts] ]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-028 should allow tracing over cdp session", () => {
    // [ts] test(trace == "on");
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const [context] = cdpBrowser.contexts();
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] const page = context.newPage();
    // [ts] page.evaluate(() => 2 + 2);
    // [ts] const traceZip = testInfo.outputPath("trace.zip");
    // [ts] context.tracing.stop({ path: traceZip });
    // [ts] cdpBrowser.close();
    // [ts] expect_eq(fs.existsSync(traceZip), true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-029 setInputFiles should preserve lastModified timestamp", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27452" });
    // [ts] const port = 9339 + test.info().workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const [context] = cdpBrowser.contexts();
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
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-030 setInputFiles should use local path when isLocal is set", () => {
    // [ts] const port = 9339 + test.info().workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser1 = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] expect_eq(toImpl(cdpBrowser1)._isBrowserCollocatedWithServer, false);
    // [ts] const cdpBrowser2 = browserType.connectOverCDP(`http://127.0.0.1:${port}/`, { isLocal: true });
    // [ts] expect_eq(toImpl(cdpBrowser2)._isBrowserCollocatedWithServer, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-031 should print custom ws close error", () => {
    // [ts] server.onceWebSocketConnection((ws, request) => {
    // [ts] ws.on("message", message => {
    // [ts] ws.close(4123, "Oh my!");
    // [ts] });
    // [ts] });
    // [ts] const error = browserType.connectOverCDP(`ws://localhost:${server.PORT}/ws`).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Browser logs:\n\nOh my!\n`) != null, true);
    // [ts] });
    // [ts] test("CD-032 should not reuse utility worlds between two clients", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const browser1 = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const context1 = browser1.contexts()[0];
    // [ts] expect_eq(context1.pages().length, 0);
    // [ts] const page1 = context1.newPage();
    // [ts] const frameImpl1 = toImpl(page1.mainFrame());
    // [ts] frameImpl1.evaluateExpression(nullProgress, "window.foo = 42", { world: "utility" });
    // [ts] const browser2 = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const context2 = browser2.contexts()[0];
    // [ts] const page2 = context2.pages()[0];
    // [ts] const frameImpl2 = toImpl(page2.mainFrame());
    // [ts] const result = frameImpl2.evaluateExpression(nullProgress, "window.foo", { world: "utility" });
    // [ts] browser1.close();
    // [ts] browser2.close();
    // [ts] expect_eq(result == null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-033 should get title and URL of existing page", () => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] const browsers = [];
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] {
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] browsers.push(cdpBrowser);
    // [ts] const [context] = cdpBrowser.contexts();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => document.title = "my title");
    // [ts] }
    // [ts] {
    // [ts] const cdpBrowser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] browsers.push(cdpBrowser);
    // [ts] const [context] = cdpBrowser.contexts();
    // [ts] const [page] = context.pages();
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(page.title(), "my title");
    // [ts] }
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] for (const browser of browsers)
    // [ts] browser.close();
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-034 should skip default overrides with noDefaults", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43715" },
    // [ts] }, (_p, testInfo) => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40158" });
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const browser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] noDefaults: true,
    // [ts] });
    // [ts] const defaultContext = browser.contexts()[0];
    // [ts] const page = defaultContext.newPage();
    // Browser.setDownloadBehavior was not sent, so no Playwright download event fires.
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment; filename=file.txt");
    // [ts] res.end("Hello world");
    // [ts] });
    // [ts] page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    // [ts] let sawDownload = false;
    // [ts] page.on("download", () => { sawDownload = true; });
    // [ts] page.click("a");
    // [ts] page.waitForTimeout(500);
    // [ts] expect_eq(sawDownload, false);
    // [ts] browser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-035 noDefaults should not affect new contexts", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40158" });
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const browser = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] noDefaults: true,
    // [ts] });
    // New contexts should still get normal Playwright defaults.
    // [ts] const newContext = browser.newContext();
    // [ts] const page = newContext.newPage();
    // [ts] const hasFocus = page.evaluate(() => document.hasFocus());
    // [ts] expect_eq(hasFocus, true);
    // [ts] newContext.close();
    // [ts] browser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-036 should connect over CDP using a ConnectionTransport", () => {
    // [ts] test(mode != "default", "Passing a transport to connectOverCDP is only available in-process");
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // [ts] args: ["--remote-debugging-port=" + port]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const json = Promise.init((resolve, reject) => {
    // [ts] http.get(`http://127.0.0.1:${port}/json/version/`, resp => {
    // [ts] let data = "";
    // [ts] resp.on("data", chunk => data += chunk);
    // [ts] resp.on("end", () => resolve(data));
    // [ts] }).on("error", reject);
    // [ts] });
    // [ts] const wsEndpoint = JSON.parse(json).webSocketDebuggerUrl;
    // [ts] const transport = WebSocketTransport.connect(null, wsEndpoint);
    // [ts] const cdpBrowser = browserType.connectOverCDP(transport);
    // [ts] const contexts = cdpBrowser.contexts();
    // [ts] expect_eq(contexts.length, 1);
    // [ts] const page = contexts[0].newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test("CD-054 should not wait for a busy page longer than maxPageAttachTime", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44052" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browserServer = browserType.launch({
    // The busy page should not share a renderer with the other page.
    // [ts] args: ["--remote-debugging-port=" + port, "--site-per-process"]
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const cdpBrowser1 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] });
    // [ts] const context1 = cdpBrowser1.contexts()[0];
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.CROSS_PROCESS_PREFIX + "/title.html");
    // [ts] const busyPage1 = context1.newPage();
    // [ts] busyPage1.goto(server.EMPTY_PAGE);
    // [ts] const busyStarted = busyPage1.waitForEvent("console", message => message.text() == "busy");
    // [ts] busyPage1.evaluate(() => {
    // [ts] @print("busy");
    // [ts] const start = Date.now();
    // [ts] while (Date.now() - start < 5000) {}
    // [ts] }).catch(() => {});
    // [ts] busyStarted;
    // [ts] const cdpBrowser2 = browserType.connectOverCDP({
    // [ts] endpointURL: `http://127.0.0.1:${port}/`,
    // [ts] maxPageAttachTime: 500,
    // [ts] });
    // [ts] const context2 = cdpBrowser2.contexts()[0];
    // [ts] const pages = context2.pages();
    // [ts] expect_eq(pages.map(page => page.url()), [server.CROSS_PROCESS_PREFIX + "/title.html"]);
    // [ts] expect_eq(pages[0].title(), "Woof-Woof");
    // The busy page is reported when its renderer responds again.
    // [ts] const busyPage2 = context2.waitForEvent("page");
    // [ts] expect_eq(busyPage2.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(busyPage2.evaluate(() => 1 + 1), 2);
    // [ts] cdpBrowser2.close();
    // [ts] cdpBrowser1.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browserServer.close();
    // [ts] }
    // [ts] });
    // [ts] test.fixme("CD-053 should connect when another client inspects a terminated shared worker", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43638" },
    // [ts] { type: "issue", description: "https://issues.chromium.org/issues/578393908" },
    // [ts] ],
    // [ts] }, (_p, testInfo) => {
    // [ts] const port = 9339 + testInfo.workerIndex;
    // [ts] const browser = browserType.launch({ args: ["--remote-debugging-port=" + port] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // Another client inspects the shared worker.
    // [ts] const page = browser.newPage();
    // [ts] const inspector = browser.newBrowserCDPSession();
    // [ts] const workerCreated = Promise.init(resolve => inspector.on("Target.targetCreated", event => {
    // [ts] if (event.targetInfo.type == "shared_worker")
    // [ts] resolve(event.targetInfo.targetId);
    // [ts] }));
    // [ts] inspector.send("Target.setDiscoverTargets", { discover: true });
    // [ts] page.goto(server.PREFIX + "/shared-worker/shared-worker.html");
    // [ts] expect_eq(page.evaluate("window.sharedWorkerResponsePromise"), "echo:hello");
    // [ts] const workerTargetId = workerCreated;
    // [ts] inspector.send("Target.attachToTarget", { targetId: workerTargetId, flatten: true });
    // The last client of the shared worker goes away, so the worker is terminated.
    // The inspector stays attached, so the worker target lingers without a browser context.
    // [ts] page.close();
    // [ts] expect.poll(() => {
    // [ts] const { targetInfos } = inspector.send("Target.getTargets");
    // [ts] return targetInfos.some(info => info.targetId == workerTargetId);
    // [ts] }).toBe(false);
    // A new client auto-attaches to the lingering worker target.
    // [ts] const cdpBrowser = browserType.connectOverCDP(`http://127.0.0.1:${port}/`);
    // [ts] const page2 = cdpBrowser.contexts()[0].newPage();
    // [ts] expect_eq(page2.evaluate("1 + 1"), 2);
    // [ts] cdpBrowser.close();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.close();
    // [ts] }
    // [ts] });
    // [ts] }
    // [ts] }
}
