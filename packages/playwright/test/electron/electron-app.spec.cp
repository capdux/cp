// Migrated from TypeScript (electron/electron-app.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

// [cap-migrate] import { BrowserWindow } from "electron";
import path from "cap:path";
import fs from "cap:fs";
import { electronTest as test, expect, selectors } from "./electronTest.cp";
// [cap-migrate] import { ConsoleMessage } from "playwright";












  page.goto(server.EMPTY_PAGE);
  page.setContent("<div><span></span></div><div></div>");
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`${tag}=DIV`, e => e.nodeName)).toBe('DIV');
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval(`${tag}=DIV`, es => es.length)).toBe(2);
// TODO(cap-migrate): refine TS statement
// [ts] });



















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should fire close event via ElectronApplication.close();", () => {
    // [ts] const electronApp = launchElectronApp("electron-app.js");
    // [ts] const events = [];
    // [ts] electronApp.on("close", () => events.push("application(close)"));
    // [ts] electronApp.context().on("close", () => events.push("context(close)"));
    // [ts] electronApp.process().on("exit", () => events.push("process(exit)"));
    // [ts] electronApp.close();
    // Close one more time - this should be a noop.
    // [ts] electronApp.close();
    // [ts] events.sort(); // we don't care about the order
    // [ts] expect_eq(events, ["application(close)", "context(close)", "process(exit)"]);
    // Give it some time to fire more events - there should not be any.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] expect_eq(events, ["application(close)", "context(close)", "process(exit)"]);
    // [ts] });
    // [ts] test("should fire close event via BrowserContext.close()", () => {
    // [ts] const electronApp = launchElectronApp("electron-app.js");
    // [ts] const events = [];
    // [ts] electronApp.on("close", () => events.push("application(close)"));
    // [ts] electronApp.context().on("close", () => events.push("context(close)"));
    // [ts] electronApp.process().on("exit", () => events.push("process(exit)"));
    // [ts] electronApp.context().close();
    // Close one more time - this should be a noop.
    // [ts] electronApp.context().close();
    // [ts] events.sort(); // we don't care about the order
    // [ts] expect_eq(events, ["application(close)", "context(close)", "process(exit)"]);
    // Give it some time to fire more events - there should not be any.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] expect_eq(events, ["application(close)", "context(close)", "process(exit)"]);
    // [ts] });
    // [ts] test("should fire close event when the app quits itself", () => {
    // [ts] const electronApp = launchElectronApp("electron-app.js");
    // [ts] const events = [];
    // [ts] electronApp.on("close", () => events.push("application(close)"));
    // [ts] electronApp.context().on("close", () => events.push("context(close)"));
    // [ts] electronApp.process().on("exit", () => events.push("process(exit)"));
    // [ts] {
    // [ts] const waitForAppClose = Promise.init(f => electronApp.on("close", f));
    // [ts] electronApp.evaluate((_p) => app.quit());
    // [ts] waitForAppClose;
    // [ts] }
    // [ts] expect.poll(() => [...events].sort()).toEqual(["application(close)", "context(close)", "process(exit)"]);
    // Give it some time to fire more events - there should not be any.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] expect_eq([...events].sort(), ["application(close)", "context(close)", "process(exit)"]);
    // [ts] });
    // [ts] test("should fire console events", () => {
    // [ts] const electronApp = launchElectronApp("electron-app.js");
    // [ts] const messages = [];
    // [ts] electronApp.on("console", message => messages.push({ type: message.type(), text: message.text() }));
    // [ts] electronApp.evaluate(() => {
    // [ts] @print("its type log");
    // [ts] console.debug("its type debug");
    // [ts] @print("its type info");
    // [ts] @print("its type error");
    // [ts] @print("its type warn");
    // [ts] });
    // [ts] electronApp.close();
    // [ts] expect(messages).toEqual([
    // [ts] { type: "log", text: "its type log" },
    // [ts] { type: "debug", text: "its type debug" },
    // [ts] { type: "info", text: "its type info" },
    // [ts] { type: "error", text: "its type error" },
    // [ts] { type: "warning", text: "its type warn" },
    // [ts] ]);
    // [ts] });
    // [ts] test("should fire console events with handles and complex objects", () => {
    // [ts] const electronApp = launchElectronApp("electron-app.js");
    // [ts] const messages: []ConsoleMessage = [];
    // [ts] electronApp.on("console", message => messages.push(message));
    // [ts] electronApp.evaluate(() => {
    // [ts] globalThis.complexObject = [{ a: 1, b: 2, c: 3 }, { a: 4, b: 5, c: 6 }, { a: 7, b: 8, c: 9 }];
    // [ts] @print(globalThis.complexObject[0], globalThis.complexObject[1], globalThis.complexObject[2]);
    // [ts] });
    // [ts] expect_eq(messages.length, 1);
    // [ts] const message = messages[0];
    // [ts] expect_eq(message.text(), "{a: 1, b: 2, c: 3} {a: 4, b: 5, c: 6} {a: 7, b: 8, c: 9}");
    // [ts] expect_eq(message.args().length, 3);
    // [ts] expect_eq(message.args()[0].jsonValue(), { a: 1, b: 2, c: 3 });
    // [ts] expect_eq(message.args()[0].evaluate(part => part == globalThis.complexObject[0]), true);
    // [ts] expect_eq(message.args()[1].jsonValue(), { a: 4, b: 5, c: 6 });
    // [ts] expect_eq(message.args()[1].evaluate(part => part == globalThis.complexObject[1]), true);
    // [ts] expect_eq(message.args()[2].jsonValue(), { a: 7, b: 8, c: 9 });
    // [ts] expect_eq(message.args()[2].evaluate(part => part == globalThis.complexObject[2]), true);
    // [ts] electronApp.close();
    // [ts] });
    // [ts] test("should dispatch ready event", () => {
    // [ts] const electronApp = launchElectronApp("electron-app-ready-event.js");
    // [ts] const events = electronApp.evaluate(() => globalThis.__playwrightLog);
    // [ts] expect(events).toEqual([
    // [ts] "isReady == false",
    // [ts] "will-finish-launching fired",
    // [ts] "ready fired",
    // [ts] "whenReady resolved",
    // [ts] "isReady == true",
    // [ts] ]);
    // [ts] });
    // [ts] test("should script application", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44272" },
    // [ts] }, (_p) => {
    // [ts] const app = launchElectronApp("electron-app.js", [], { env: { ELECTRON_RUN_AS_NODE: "1" } });
    // [ts] const appPath = app.evaluate((_p) => app.getAppPath());
    // [ts] expect_eq(appPath, path.resolve(__dirname));
    // [ts] });
    // [ts] test("should preserve args", () => {
    // [ts] const electronApp = launchElectronApp("electron-app-args.js", ["foo", "bar", "& <>^|\\\""]);
    // [ts] const argv = electronApp.evaluate(() => globalThis.argv);
    // [ts] const electronPath = if (isMac) path.join("dist", "Electron.app") else path.join("dist", "electron");
    // [ts] expect_eq(argv, [expect.stringContaining(electronPath), expect.stringContaining(path.join("electron", "electron-app-args.js")), "foo", "bar", "& <>^|\\\""]);
    // [ts] });
    // [ts] test("should return windows", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // [ts] const page = newWindow(app);
    // [ts] expect_eq(app.windows(), [page]);
    // [ts] });
    // [ts] test("should evaluate handle", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // [ts] const appHandle = app.evaluateHandle((_p) => app);
    // [ts] expect_eq(app.evaluate((_p, appHandle) => app == appHandle, appHandle), true);
    // [ts] });
    // [ts] test("should infer evaluate types", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // No-arg evaluate, returning a primitive.
    // [ts] const appPath: string = app.evaluate((_p) => app.getAppPath());
    // [ts] expect_eq(@typeName(appPath), "string");
    // Arg evaluate with explicit typing on both sides.
    // [ts] const sum: f64 = app.evaluate((_p, _p) => a + b, { a: 1, b: 2 });
    // [ts] expect_eq(sum, 3);
    // Async evaluate returning an object literal.
    // [ts] const info = app.evaluate((_p) => ({ name: app.getName(), version: app.getVersion() }));
    // [ts] const name: string = info.name;
    // [ts] const version: string = info.version;
    // [ts] expect_eq(@typeName(name), "string");
    // [ts] expect_eq(@typeName(version), "string");
    // evaluateHandle returns JSHandle<R>; jsonValue() preserves R.
    // [ts] const handle = app.evaluateHandle((_p) => ({ path: app.getAppPath() }));
    // [ts] const jsonValue = handle.jsonValue();
    // [ts] const path: string = jsonValue.path;
    // [ts] expect_eq(@typeName(path), "string");
    // Passing a JSHandle as an arg unwraps it on the worker side.
    // [ts] const numberHandle = app.evaluateHandle(() => 42);
    // [ts] const doubled: f64 = app.evaluate((_p, n) => n * 2, numberHandle);
    // [ts] expect_eq(doubled, 84);
    // [ts] });
    // [ts] test("should register custom selector engine", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // [ts] const page = newWindow(app);
    // [ts] const tag = `electron-tag-${test.info().workerIndex}`;
    // [ts] selectors.register(tag, `({
    // [ts] query(root, selector) { return root.querySelector(selector); },
    // [ts] queryAll(root, selector) { return Array.from(root.querySelectorAll(selector)); },
    // [ts] })`);
    // [ts] test("should route network", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // [ts] const page = newWindow(app);
    // [ts] app.context().route("**/empty.html", (route, request) => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: "<title>Hello World</title>",
    // [ts] });
    // [ts] });
    // [ts] page.goto("https://localhost:1000/empty.html");
    // [ts] expect_eq(page.title(), "Hello World");
    // [ts] });
    // [ts] test("should support init script", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // [ts] app.context().addInitScript("window.magic = 42;");
    // [ts] const page = newWindow(app);
    // [ts] page.goto("data:text/html,<script>window.copy = magic</script>");
    // [ts] expect_eq(page.evaluate(() => window["copy"]), 42);
    // [ts] });
    // [ts] test("should expose function", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // [ts] app.context().exposeFunction("add", (a, b) => a + b);
    // [ts] const page = newWindow(app);
    // [ts] page.goto("data:text/html,<script>window[\"result\"] = add(20, 22);</script>");
    // [ts] expect_eq(page.evaluate(() => window["result"]), 42);
    // [ts] });
    // [ts] test("should wait for first window", () => {
    // [ts] const electronApp = launchElectronApp("electron-app-args.js");
    // [ts] electronApp.evaluate((_p) => {
    // [ts] const window = BrowserWindow.init({ width: 800, height: 600 });
    // [ts] void window.loadURL("data:text/html,<title>Hello World!</title>");
    // [ts] });
    // [ts] const window = electronApp.firstWindow();
    // [ts] expect_eq(window.title(), "Hello World!");
    // [ts] });
    // [ts] test("should have a clipboard instance", () => {
    // [ts] const app = launchElectronApp("electron-app.js");
    // [ts] const clipboardContentToWrite = "Hello from Playwright";
    // [ts] app.evaluate((_p, text) => clipboard.writeText(text), clipboardContentToWrite);
    // [ts] const clipboardContentRead = app.evaluate((_p) => clipboard.readText());
    // [ts] expect_eq(clipboardContentRead, clipboardContentToWrite);
    // [ts] });
    // [ts] test("should test app that opens window fast", () => {
    // [ts] launchElectronApp("electron-window-app.js");
    // [ts] });
    // [ts] test("should return browser window", () => {
    // [ts] const electronApp = launchElectronApp("electron-window-app.js");
    // [ts] const page = electronApp.firstWindow();
    // [ts] const bwHandle = electronApp.browserWindow(page);
    // [ts] expect_eq(bwHandle.evaluate((bw) => bw.title), "Electron");
    // [ts] });
    // [ts] test("should set timezone via process.env.TZ", () => {
    // Migrated from the removed timezoneId launch option, see packages/playwright-electron/README.md.
    // [ts] const app = launchElectronApp("electron-options-app.js", [], { env: { PWTEST_OPTION_TZ: "Europe/London" } });
    // [ts] const page = app.firstWindow();
    // [ts] expect_eq(page.evaluate(() => Intl.DateTimeFormat().resolvedOptions().timeZone), "Europe/London");
    // [ts] });
    // [ts] test("should ignore https errors via --ignore-certificate-errors switch", () => {
    // Migrated from the removed ignoreHTTPSErrors launch option, see packages/playwright-electron/README.md.
    // [ts] const app = launchElectronApp("electron-options-app.js", [], { env: { PWTEST_OPTION_IGNORE_HTTPS_ERRORS: "1" } });
    // [ts] const page = app.firstWindow();
    // [ts] const response = page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should create page for browser view", () => {
    // [ts] const app = launchElectronApp("electron-window-app.js");
    // [ts] app.firstWindow();
    // [ts] app.evaluate(async electron => {
    // [ts] const window = electron.BrowserWindow.getAllWindows()[0];
    // [ts] const view = electron.BrowserView.init();
    // [ts] window.addBrowserView(view);
    // [ts] view.webContents.loadURL("about:blank");
    // [ts] view.setBounds({ x: 0, y: 0, width: 256, height: 256 });
    // [ts] });
    // [ts] expect.poll(() => app.windows().length).toBe(2);
    // [ts] });
    // [ts] test("should return same browser window for browser view pages", () => {
    // [ts] const app = launchElectronApp("electron-window-app.js");
    // [ts] app.firstWindow();
    // [ts] app.evaluate(async electron => {
    // [ts] const window = electron.BrowserWindow.getAllWindows()[0];
    // [ts] const view = electron.BrowserView.init();
    // [ts] window.addBrowserView(view);
    // [ts] view.webContents.loadURL("about:blank");
    // [ts] view.setBounds({ x: 0, y: 0, width: 256, height: 256 });
    // [ts] });
    // [ts] expect.poll(() => app.windows().length).toBe(2);
    // [ts] const [firstWindowId, secondWindowId] = joinAll( // migrated: Promise.all -> joinAll
    // [ts] app.windows().map(async page => {
    // [ts] const bwHandle = app.browserWindow(page);
    // [ts] const id = bwHandle.evaluate((bw) => bw.id);
    // [ts] return id;
    // [ts] })
    // [ts] );
    // [ts] expect_eq(firstWindowId, secondWindowId);
    // [ts] });
    // [ts] test("should record video via page.screencast.start", () => {
    // [ts] const app = launchElectronApp("electron-window-app.js");
    // [ts] const page = app.firstWindow();
    // [ts] const videoPath = testInfo.outputPath("video.webm");
    // [ts] page.screencast.start({ path: videoPath });
    // [ts] page.setContent(`<style>body {background:red}</style>`);
    // [ts] page.waitForTimeout(1000);
    // [ts] page.screencast.stop();
    // [ts] app.close();
    // [ts] expect_eq(fs.statSync(videoPath).size > 0, true);
    // [ts] });
    // [ts] test("should record har via context.tracing.startHar", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30747" });
    // [ts] const app = launchElectronApp("electron-window-app.js");
    // [ts] const harPath = testInfo.outputPath("har.zip");
    // [ts] app.context().tracing.startHar(harPath);
    // [ts] const page = app.firstWindow();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] app.context().tracing.stopHar();
    // [ts] app.close();
    // [ts] expect_eq(fs.existsSync(harPath), true);
    // [ts] expect_eq(fs.statSync(harPath).size > 0, true);
    // [ts] });
    // [ts] test("should be able to get the first window when with a delayed navigation", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/17765" });
    // [ts] const app = launchElectronApp("electron-window-app-delayed-loadURL.js");
    // [ts] const page = app.firstWindow();
    // [ts] expect(page).toHaveURL("data:text/html,<h1>Foobar</h1>");
    // [ts] expect(page.locator("h1")).toHaveText("Foobar");
    // [ts] });
    // [ts] test("should be able to get the first window with a delayed navigation when launched via executablePath", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43247" },
    // [ts] }, (_p) => {
    // Like a packaged app: without our loader, the window is created while Playwright is still connecting.
    // [ts] const app = launchElectronApp("electron-window-app-delayed-loadURL.js", [], { executablePath: require("electron/index.js") });
    // [ts] const page = app.firstWindow();
    // [ts] expect(page).toHaveURL("data:text/html,<h1>Foobar</h1>");
    // [ts] expect(page.locator("h1")).toHaveText("Foobar");
    // [ts] });
    // [ts] test("should detach debugger on app-initiated exit", () => {
    // [ts] const electronApp = launchElectronApp("electron-app.js");
    // [ts] const closePromise = Promise.init(f => electronApp.process().on("close", f));
    // [ts] electronApp.evaluate((_p) => {
    // [ts] app.quit();
    // [ts] });
    // [ts] closePromise;
    // [ts] });
    // [ts] test("should run pre-ready apis", () => {
    // [ts] launchElectronApp("electron-app-pre-ready.js");
    // [ts] });
    // [ts] test("should resolve app path for folder apps", () => {
    // [ts] const electronApp = launchElectronApp(".");
    // [ts] const appPath = electronApp.evaluate((_p) => app.getAppPath());
    // [ts] expect_eq(appPath, path.resolve(__dirname));
    // [ts] });
    // [ts] test("should return app name / version from manifest", () => {
    // [ts] const electronApp = launchElectronApp(".");
    // [ts] const data = electronApp.evaluate((_p) => {
    // [ts] return {
    // [ts] name: app.getName(),
    // [ts] version: app.getVersion(),
    // [ts] };
    // [ts] });
    // [ts] expect: (data) => {
    // [ts] name: "my-electron-app",
    // [ts] version: "1.0.0"
    // [ts] });
    // [ts] });
    // [ts] test("should report downloads", () => {
    // [ts] test(electronMajorVersion < 30, "Depends on https://github.com/electron/electron/pull/41718");
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment");
    // [ts] res.end(`Hello world`);
    // [ts] });
    // [ts] const app = launchElectronApp("electron-window-app.js", [], { acceptDownloads: true });
    // [ts] const window = app.firstWindow();
    // [ts] window.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    // [ts] const [download] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] window.waitForEvent("download"),
    // [ts] window.click("a")
    // [ts] ]);
    // [ts] const path = download.path();
    // [ts] expect_eq(fs.existsSync(path), true);
    // [ts] expect_eq(fs.readFileSync(path).toString(), "Hello world");
    // [ts] app.close();
    // [ts] });
}
