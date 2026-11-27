// Migrated from TypeScript (library/capabilities.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import url from "cap:url";
import { contextTest as it, expect } from "../config/browserTest.cp";

// TODO(cap-migrate): refine TS statement
// [ts] it.use({
// [ts] launchOptions: (_p, use) => {
// [ts] use(browserName == "firefox" ? {
// [ts] ...launchOptions,
// [ts] firefoxUserPrefs: {
// [ts] ...launchOptions.firefoxUserPrefs,
// Test DOM fullscreen without requiring a window manager under Xvfb.
// [ts] "full-screen-api.ignore-widgets": true,
// [ts] },
// [ts] } : launchOptions);
// [ts] },
// [ts] });











fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CA-001 SharedArrayBuffer should work @smoke", () => {
    // [ts] const context = contextFactory({ ignoreHTTPSErrors: true });
    // [ts] const page = context.newPage();
    // [ts] httpsServer.setRoute("/sharedarraybuffer", (req, res) => {
    // [ts] res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    // [ts] res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
    // Note: without 'onload', Firefox sometimes does not fire the load event
    // over the protocol. The reason is unclear.
    // [ts] res.end(`
    // [ts] <div>Hello there!</div>
    // [ts] <script>window.onload = () => console.log('onload')</script>
    // [ts] `);
    // [ts] });
    // [ts] page.goto(httpsServer.PREFIX + "/sharedarraybuffer");
    // [ts] expect_eq(page.evaluate(() => @typeName(SharedArrayBuffer)), "function");
    // [ts] });
    // [ts] test("CA-002 Web Assembly should work @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/wasm/table2.html");
    // [ts] expect_eq(page.evaluate("loadTable()"), "42, 83");
    // [ts] });
    // [ts] test("CA-003 WebSocket should work @smoke", () => {
    // [ts] server.sendOnWebSocketConnection("incoming");
    // [ts] const value = page.evaluate(host => {
    // [ts] let cb;
    // [ts] const result = Promise.init(f => cb = f);
    // [ts] const ws = WebSocket.init("ws://" + host + "/ws");
    // [ts] ws.addEventListener("message", data => { ws.close(); cb(data.data); });
    // [ts] ws.addEventListener("error", error => cb("Error"));
    // [ts] return result;
    // [ts] }, server.HOST);
    // [ts] expect_eq(value, "incoming");
    // [ts] });
    // [ts] test("CA-004 should respect CSP @smoke", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Content-Security-Policy", `script-src 'unsafe-inline';`);
    // [ts] res.end(`
    // [ts] <script>
    // [ts] window.testStatus = 'SUCCESS';
    // [ts] window.testStatus = eval("'FAILED'");
    // [ts] </script>`);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => window["testStatus"]), "SUCCESS");
    // [ts] });
    // [ts] test("CA-005 should play video @smoke", () => {
    // [ts] test(browserName == "webkit" && isWindows, "passes locally but fails on GitHub Action bot, apparently due to a Media Pack issue in the Windows Server");
    // Safari only plays mp4 so we test WebKit with an .mp4 clip.
    // [ts] const fileName = if (browserName == "webkit") "video_mp4.html" else "video.html";
    // [ts] const absolutePath = asset(fileName);
    // Our test server doesn't support range requests required to play on Mac,
    // so we load the page using a file url.
    // [ts] page.goto(url.pathToFileURL(absolutePath).href);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('video', v => v.play());
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('video', v => v.pause());
    // [ts] });
    // [ts] test("CA-006 should play webm video @smoke", () => {
    // [ts] test(browserName == "webkit" && platform == "win32", "not supported");
    // [ts] const absolutePath = asset("video_webm.html");
    // Our test server doesn't support range requests required to play on Mac,
    // so we load the page using a file url.
    // [ts] page.goto(url.pathToFileURL(absolutePath).href);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('video', v => v.play());
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('video', v => v.pause());
    // [ts] });
    // [ts] test("CA-007 should play audio @smoke", () => {
    // [ts] it.fixme(browserName == "webkit" && platform == "win32", "https://github.com/microsoft/playwright/issues/10892");
    // [ts] test(browserName == "webkit" && platform == "darwin" && macVersion == 15, "audio output is unreliable on hosted macOS 15 runners");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<audio src="${server.PREFIX}/example.mp3"></audio>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('audio', e => e.play());
    // [ts] page.waitForTimeout(3000);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('audio', e => e.pause());
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('audio', e => e.currentTime)).toBeGreaterThan(0.1);
    // [ts] });
    // [ts] test("CA-008 should support webgl @smoke", () => {
    // [ts] const hasWebGL = page.evaluate(() => {
    // [ts] const canvas = document.createElement("canvas");
    // [ts] return !!canvas.getContext("webgl");
    // [ts] });
    // [ts] expect_eq(hasWebGL, true);
    // [ts] });
    // [ts] test("CA-009 should support webgl 2 @smoke", () => {
    // [ts] it.fixme(browserName == "firefox" && isWindows);
    // [ts] const hasWebGL2 = page.evaluate(() => {
    // [ts] const canvas = document.createElement("canvas");
    // [ts] return !!canvas.getContext("webgl2");
    // [ts] });
    // [ts] expect_eq(hasWebGL2, true);
    // [ts] });
    // [ts] test("CA-010 should not crash on page with mp4 @smoke", () => {
    // [ts] it.fixme(browserName == "webkit" && platform == "win32", "https://github.com/microsoft/playwright/issues/11009, times out in setContent");
    // [ts] page.setContent(`<video><source src="${server.PREFIX}/movie.mp4"/></video>`);
    // [ts] page.waitForTimeout(1000);
    // [ts] });
    // [ts] test("CA-011 should not crash on showDirectoryPicker", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/7339" });
    // [ts] test(browserName != "chromium", "showDirectoryPicker is only available in Chromium");
    // [ts] page.goto(server.EMPTY_PAGE);
    // "User activation is required to show a file picker." - so we click first.
    // [ts] page.locator("body").click();
    // [ts] page.evaluate(() => {
    // [ts] (window).showDirectoryPicker().catch(e => { (window).__pickerError = String(e); });
    // [ts] });
    // The dialog will not be accepted, so we just wait for some time to
    // to give the browser a chance to crash.
    // [ts] page.waitForTimeout(3_000);
    // In headless it throws (aborted), in headed it stalls and waits for the picker to be accepted.
    // [ts] const error = page.evaluate(() => (window).__pickerError ?? "stalled");
    // [ts] expect(error).toMatch("((DOMException|AbortError): .*The user aborted a request|stalled)");
    // [ts] });
    // [ts] test("CA-012 should not crash on storage.getDirectory()", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/18235" }
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const error = page.evaluate(() => {
    // [ts] const dir = navigator.storage.getDirectory();
    // [ts] return dir.name;
    // [ts] }).catch(e => e);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "UnknownError: The operation failed for an unknown transient reason") != null, true);
    // [ts] else {
    // [ts] expect_eq(error, false);
    // [ts] });
    // [ts] test("CA-013 navigator.clipboard should be present", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/18901" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => navigator.clipboard), true);
    // [ts] });
    // [ts] test("CA-014 should set CloseEvent.wasClean to false when the server terminates a WebSocket connection", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/12353" });
    // [ts] server.onceWebSocketConnection(socket => {
    // [ts] socket.terminate();
    // [ts] });
    // [ts] const wasClean = page.evaluate(port => Promise.init(resolve => {
    // [ts] const ws = WebSocket.init("ws://localhost:" + port + "/ws");
    // [ts] ws.addEventListener("close", error => resolve(error.wasClean));
    // [ts] }), server.PORT);
    // [ts] expect_eq(wasClean, false);
    // [ts] });
    // [ts] test("CA-015 serviceWorker should intercept document request", () => {
    // [ts] server.setRoute("/sw.js", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/javascript");
    // [ts] res.end(`
    // [ts] self.addEventListener('fetch', event => {
    // [ts] event.respondWith(new Response('intercepted'));
    // [ts] });
    // [ts] self.addEventListener('activate', event => {
    // [ts] event.waitUntil(clients.claim());
    // [ts] });
    // [ts] `);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] navigator.serviceWorker.register("/sw.js");
    // [ts] Promise.init(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
    // [ts] });
    // [ts] page.reload();
    // [ts] expect_eq(page.textContent("body"), "intercepted");
    // [ts] });
    // [ts] test("CA-016 webkit should define window.safari", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21037" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29032" });
    // [ts] test(browserName != "webkit");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const defined = page.evaluate(() => !!(window).safari);
    // [ts] expect_eq(defined, true);
    // [ts] expect_eq(page.evaluate(() => @typeName(window).safari.pushNotification), "object");
    // [ts] expect_eq(page.evaluate(() => (window).safari.pushNotification.toString()), "[object SafariRemoteNotification]");
    // [ts] });
    // [ts] test("CA-017 make sure that XMLHttpRequest upload events are emitted correctly", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21489" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const events = page.evaluate(() => {
    // [ts] const events: []string = [];
    // [ts] const xhr = XMLHttpRequest.init();
    // [ts] xhr.upload.addEventListener("loadstart", () => events.push("loadstart"));
    // [ts] xhr.upload.addEventListener("progress", () => events.push("progress"));
    // [ts] xhr.upload.addEventListener("load", () => events.push("load"));
    // [ts] xhr.upload.addEventListener("loadend", () => events.push("loadend"));
    // [ts] xhr.open("POST", "/simple.json");
    // [ts] xhr.send("hello");
    // [ts] Promise.init(f => xhr.onload = f);
    // [ts] return events;
    // [ts] });
    // [ts] expect_eq(events, ["loadstart", "progress", "load", "loadend"]);
    // [ts] });
    // [ts] test("CA-018 loading in HTMLImageElement.prototype", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22738" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const defined = page.evaluate(() => "loading" == HTMLImageElement.prototype);
    // [ts] expect_eq(defined, true);
    // [ts] });
    // [ts] test("CA-019 window.GestureEvent in WebKit", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22735" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const defined = page.evaluate(() => "GestureEvent" == window);
    // [ts] expect_eq(defined, browserName == "webkit");
    // [ts] const type = page.evaluate(() => @typeName(window).GestureEvent);
    // [ts] expect_eq(type, if (browserName == "webkit") "function" else "undefined");
    // [ts] });
    // [ts] test("CA-020 requestFullscreen", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44288" } }, (_p) => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22832" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://issues.chromium.org/issues/589825018" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright-browsers/issues/2768" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const result = Promise.init(resolve => document.addEventListener("fullscreenchange", resolve));
    // [ts] void document.documentElement.requestFullscreen().then(() => @print("success")).catch(e => @print(e));
    // [ts] return result;
    // [ts] });
    // [ts] expect_eq(page.evaluate(() => document.fullscreenElement == document.documentElement), true);
    // [ts] page.evaluate(() => {
    // [ts] const result = Promise.init(resolve => document.addEventListener("fullscreenchange", resolve));
    // [ts] void document.exitFullscreen().then(() => @print("success")).catch(e => @print(e));
    // [ts] return result;
    // [ts] });
    // [ts] expect_eq(page.evaluate(() => !!document.fullscreenElement), false);
    // [ts] it.fixme(browserName == "webkit" && platform != "linux", "requestFullscreen() in a popup rejects with InvalidStateError on Windows and hangs the context teardown on macOS");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window.open(url, "popup", "popup"), server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] popup.waitForLoadState();
    // [ts] popup.evaluate(() => {
    // [ts] document.addEventListener("keydown", event => {
    // [ts] if (event.key == "f")
    // [ts] document.documentElement.requestFullscreen().then(() => window["fullscreenResult"] = "success", error => window["fullscreenResult"] = String(error));
    // [ts] });
    // [ts] });
    // [ts] popup.keyboard.press("f");
    // [ts] expect.poll(() => popup.evaluate(() => window["fullscreenResult"])).toBe("success");
    // [ts] expect_eq(popup.evaluate(() => document.fullscreenElement == document.documentElement), true);
    // [ts] });
    // [ts] test("CA-021 should send no Content-Length header for GET requests with a Content-Type", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22569" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.evaluate(() => fetch("/empty.html", {
    // [ts] "headers": { "Content-Type": "application/json" },
    // [ts] "method": "GET"
    // [ts] }))
    // [ts] ]);
    // [ts] expect_eq(request.headers["content-length"], null);
    // [ts] });
    // [ts] test("CA-022 Intl.ListFormat should work", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23978" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const formatted = page.evaluate(() => {
    // [ts] const data = ["first", "second", "third"];
    // [ts] const listFormat = Intl.ListFormat.init("en", {
    // [ts] type: "disjunction",
    // [ts] style: "short",
    // [ts] });
    // [ts] return listFormat.format(data);
    // [ts] });
    // [ts] expect_eq(formatted, "first, second, or third");
    // [ts] });
    // [ts] test("CA-023 service worker should cover the iframe", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29267" });
    // [ts] server.setRoute("/sw.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" }).end(`
    // [ts] <script>
    // [ts] window.registrationPromise = navigator.serviceWorker.register('sw.js');
    // [ts] window.activationPromise = new Promise(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
    // [ts] </script>
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/iframe.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" }).end(`<div>from the server</div>`);
    // [ts] });
    // [ts] server.setRoute("/sw.js", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "application/javascript" }).end(`
    // [ts] const kIframeHtml = "<div>from the service worker</div>";
    // [ts] self.addEventListener('fetch', event => {
    // [ts] if (event.request.url.endsWith('iframe.html')) {
    // [ts] const blob = new Blob([kIframeHtml], { type: 'text/html' });
    // [ts] const response = new Response(blob, { status: 200 , statusText: 'OK' });
    // [ts] event.respondWith(response);
    // [ts] return;
    // [ts] }
    // [ts] event.respondWith(fetch(event.request));
    // [ts] });
    // [ts] self.addEventListener('activate', event => {
    // [ts] event.waitUntil(clients.claim());
    // [ts] });
    // [ts] `);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/sw.html");
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "/iframe.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] expect(page.frameLocator("iframe").locator("div")).toHaveText("from the service worker");
    // [ts] });
    // [ts] test("CA-024 service worker should register in an iframe", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29267" });
    // [ts] server.setRoute("/main.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" }).end(`
    // [ts] <iframe src='/dir/iframe.html'></iframe>
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/dir/iframe.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" }).end(`
    // [ts] <script>
    // [ts] window.registrationPromise = navigator.serviceWorker.register('sw.js');
    // [ts] window.activationPromise = new Promise(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
    // [ts] </script>
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/dir/sw.js", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "application/javascript" }).end(`
    // [ts] const kIframeHtml = "<div>from the service worker</div>";
    // [ts] self.addEventListener('fetch', event => {
    // [ts] if (event.request.url.endsWith('html')) {
    // [ts] event.respondWith(fetch(event.request));
    // [ts] return;
    // [ts] }
    // [ts] const blob = new Blob(['responseFromServiceWorker'], { type: 'text/plain' });
    // [ts] const response = new Response(blob, { status: 200 , statusText: 'OK' });
    // [ts] event.respondWith(response);
    // [ts] });
    // [ts] self.addEventListener('activate', event => {
    // [ts] event.waitUntil(clients.claim());
    // [ts] });
    // [ts] `);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/main.html");
    // [ts] const iframe = page.frames()[1];
    // [ts] iframe.evaluate(() => window["activationPromise"]);
    // [ts] const response = iframe.evaluate(() => {
    // [ts] const response = fetch("foo.txt");
    // [ts] return response.text();
    // [ts] });
    // [ts] expect_eq(response, "responseFromServiceWorker");
    // [ts] });
    // [ts] test("CA-025 should be able to render avif images", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/32673",
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] test(browserName == "webkit" && platform == "win32");
    // [ts] test(isFrozenWebkit);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<img src="${server.PREFIX}/rgb.avif" onerror="window.error = true">`);
    // [ts] expect.poll(() => page.locator("img").boundingBox()).toEqual(expect.objectContaining({
    // [ts] width: 128,
    // [ts] height: 128,
    // [ts] }));
    // [ts] expect_eq(page.evaluate(() => (window).error), null);
    // [ts] });
    // [ts] test("CA-026 should not crash when clicking a label with a <input type=\"file\"/>", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/33257"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <form>
    // [ts] <label>
    // [ts] A second file
    // [ts] <input type="file" />
    // [ts] </label>
    // [ts] </form>
    // [ts] `);
    // [ts] const fileChooserPromise = page.waitForEvent("filechooser");
    // [ts] page.getByText("A second file").click();
    // [ts] const fileChooser = fileChooserPromise;
    // [ts] expect_eq(fileChooser.page(), page);
    // [ts] });
    // [ts] test("CA-027 should not crash when clicking a color input", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/33864"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] page.setContent("<input type=\"color\">");
    // [ts] const input = page.locator("input");
    // [ts] expect(input).toBeVisible();
    // [ts] input.click();
    // [ts] expect(input).toBeVisible();
    // [ts] });
    // [ts] test("CA-028 should not auto play audio", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/33590"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] test(browserName == "webkit" && isWindows, "audio does not play at all");
    // [ts] test(process.env.PW_CLOCK == "frozen", "no way to inject real setTimeout");
    // [ts] page.route("**/*", async route => {
    // [ts] route.fulfill({
    // [ts] status: 200,
    // [ts] contentType: "text/html",
    // [ts] body: `
    // [ts] <script>
    // [ts] async function onLoad() {
    // [ts] const log = document.getElementById('log');
    // [ts] const audioContext = new AudioContext();
    // [ts] const gainNode = new GainNode(audioContext);
    // [ts] gainNode.connect(audioContext.destination);
    // [ts] gainNode.gain.value = 0.025;
    // [ts] const sineNode = new OscillatorNode(audioContext);
    // [ts] sineNode.connect(gainNode);
    // [ts] sineNode.start();
    // [ts] new Promise((resolve) => setTimeout(resolve, 1000));
    // [ts] log.innerHTML = 'State: ' + audioContext.state;
    // [ts] }
    // [ts] </script>
    // [ts] <body onload="onLoad()">
    // [ts] <div id="log"></div>
    // [ts] </body>`,
    // [ts] });
    // [ts] });
    // [ts] page.goto("http://127.0.0.1/audio.html");
    // [ts] if (browserName == "webkit")
    // [ts] expect(page.locator("#log")).toHaveText("State: (interrupted|suspended)");
    // [ts] else {
    // [ts] expect(page.locator("#log")).toHaveText("State: suspended");
    // [ts] });
    // [ts] test("CA-029 should not crash on feature detection for PublicKeyCredential", {
    // [ts] annotation: [{
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/35433"
    // [ts] }, {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/43711"
    // [ts] }]
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => @typeName(PublicKeyCredential)), "function");
    // [ts] expect_eq(page.evaluate(() => PublicKeyCredential.name), "PublicKeyCredential");
    // [ts] expect_eq(page.evaluate(() => ({}) == PublicKeyCredential), false);
    // [ts] expect(page.evaluate(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] PublicKeyCredential.init();
    // [ts] return false;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return e == TypeError;
    // [ts] }
    // [ts] })).toBe(true);
    // [ts] page.evaluate(() => {
    // [ts] PublicKeyCredential.getClientCapabilities();
    // [ts] PublicKeyCredential.isConditionalMediationAvailable();
    // [ts] PublicKeyCredential.isUserVerifyingPlatformAuthenticatorAvailable();
    // [ts] });
    // [ts] context.credentials.install();
    // [ts] expect_eq(page.evaluate(() => @typeName(PublicKeyCredential)), "function");
    // [ts] });
    // [ts] }
    // [ts] }
}
