// Migrated from TypeScript (library/permissions.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";

fn getPermission(page: auto, name: auto) {
  return page.evaluate(name => navigator.permissions.query({ name }).then(result => result.state), name);
}

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("permissions", () => {
// [ts] it.fixme((_p) => browserName == "webkit" && isWindows, "Permissions API is disabled on Windows WebKit");
// [ts] test("PE-001 should be prompt by default", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// [ts] });
// [ts] test("PE-002 should deny permission when not listed", () => {
// [ts] test(isFrozenWebkit);
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions([], { origin: server.EMPTY_PAGE });
// [ts] if (browserName == "webkit") {
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// Since https://github.com/WebKit/WebKit/pull/45470 WebKit only returns actual
// permission value, if the API has been accessed.
// [ts] page.evaluate(() => navigator.geolocation.getCurrentPosition(() => { }));
// [ts] expect_eq(getPermission(page, "geolocation"), "denied");
// [ts] } else if (isBidi) {
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// [ts] } else {
// [ts] expect_eq(getPermission(page, "geolocation"), "denied");
// [ts] }
// [ts] });
// [ts] test("PE-003 should fail when bad permission is given", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] let error: Error;
// [ts] context.grantPermissions(["foo"], { origin: server.EMPTY_PAGE }).catch(e => error = e);
// [ts] if (isBidi)
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Protocol error (permissions.setPermission): invalid argument") != null, true);
// [ts] else {
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unknown permission: foo") != null, true);
// [ts] });
// [ts] test("PE-004 should grant geolocation permission when origin is listed", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions(["geolocation"], { origin: server.EMPTY_PAGE });
// [ts] expect_eq(getPermission(page, "geolocation"), "granted");
// [ts] });
// [ts] test("PE-005 should prompt for geolocation permission when origin is not listed", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions(["geolocation"], { origin: server.EMPTY_PAGE });
// [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// [ts] });
// [ts] test("PE-006 should grant notifications permission when listed", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions(["notifications"], { origin: server.EMPTY_PAGE });
// [ts] expect_eq(getPermission(page, "notifications"), "granted");
// [ts] });
// [ts] test("PE-007 should accumulate when adding", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions(["geolocation"]);
// [ts] context.grantPermissions(["notifications"]);
// [ts] expect_eq(getPermission(page, "geolocation"), "granted");
// [ts] expect_eq(getPermission(page, "notifications"), "granted");
// [ts] });
// [ts] test("PE-008 should clear permissions", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions(["geolocation"]);
// [ts] context.clearPermissions();
// [ts] context.grantPermissions(["notifications"]);
// [ts] expect_eq(getPermission(page, "geolocation") != "granted", true);
// [ts] expect_eq(getPermission(page, "notifications"), "granted");
// [ts] });
// [ts] test("PE-009 should grant permission when listed for all domains", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions(["geolocation"]);
// [ts] expect_eq(getPermission(page, "geolocation"), "granted");
// [ts] });
// [ts] test("PE-010 should grant permission when creating context", () => {
// [ts] const context = browser.newContext({ permissions: ["geolocation"] });
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect_eq(getPermission(page, "geolocation"), "granted");
// [ts] context.close();
// [ts] });
// [ts] test("PE-011 should reset permissions", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] context.grantPermissions(["geolocation"], { origin: server.EMPTY_PAGE });
// [ts] expect_eq(getPermission(page, "geolocation"), "granted");
// [ts] context.clearPermissions();
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// [ts] });
// [ts] test("PE-012 should trigger permission onchange", () => {
// [ts] it.fail(browserName == "webkit");
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.evaluate(() => {
// [ts] window["events"] = [];
// [ts] return navigator.permissions.query({ name: "geolocation" }).then((result) => {
// [ts] window["events"].push(result.state);
// [ts] result.onchange = () => {
// [ts] window["events"].push(result.state);
// [ts] };
// [ts] });
// [ts] });
// [ts] const expectedEvents = ["prompt"];
// [ts] expect_eq(page.evaluate(() => window["events"]), expectedEvents);
// [ts] context.grantPermissions([], { origin: server.EMPTY_PAGE });
// [ts] if (!isBidi)
// [ts] expectedEvents.push("denied");
// [ts] expect_eq(page.evaluate(() => window["events"]), expectedEvents);
// [ts] context.grantPermissions(["geolocation"], { origin: server.EMPTY_PAGE });
// [ts] expectedEvents.push("granted");
// [ts] expect_eq(page.evaluate(() => window["events"]), expectedEvents);
// [ts] context.clearPermissions();
// [ts] expectedEvents.push("prompt");
// [ts] expect_eq(page.evaluate(() => window["events"]), expectedEvents);
// [ts] });
// [ts] test("PE-013 should isolate permissions between browser contexts", () => {
// [ts] test(isFrozenWebkit);
// [ts] const context = browser.newContext();
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] const otherContext = browser.newContext();
// [ts] const otherPage = otherContext.newPage();
// [ts] otherPage.goto(server.EMPTY_PAGE);
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// [ts] expect_eq(getPermission(otherPage, "geolocation"), "prompt");
// [ts] context.grantPermissions([], { origin: server.EMPTY_PAGE });
// [ts] otherContext.grantPermissions(["geolocation"], { origin: server.EMPTY_PAGE });
// [ts] if (browserName == "webkit") {
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// Since https://github.com/WebKit/WebKit/pull/45470 WebKit only returns actual
// permission value, if the API has been accessed.
// [ts] page.evaluate(() => navigator.geolocation.getCurrentPosition(() => { }));
// [ts] expect_eq(getPermission(page, "geolocation"), "denied");
// [ts] } else if (isBidi) {
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// [ts] } else {
// [ts] expect_eq(getPermission(page, "geolocation"), "denied");
// [ts] }
// [ts] expect_eq(getPermission(otherPage, "geolocation"), "granted");
// [ts] context.clearPermissions();
// [ts] if (browserName == "webkit") {
// Since https://github.com/WebKit/WebKit/pull/45470 WebKit returns the cached
// permission value, if the geolocation API has been accessed.
// TODO: We can probably reset the cached state in the Web Process when resetting
// permissions.
// [ts] expect_eq(getPermission(page, "geolocation"), "denied");
// Geolocation API in the new page in the same context has not been accessed yet,
// so the permission status should be prompt.
// [ts] const page2 = context.newPage();
// [ts] page2.goto(server.EMPTY_PAGE);
// [ts] expect_eq(getPermission(page2, "geolocation"), "prompt");
// [ts] page2.close();
// [ts] } else {
// [ts] expect_eq(getPermission(page, "geolocation"), "prompt");
// [ts] }
// [ts] expect_eq(getPermission(otherPage, "geolocation"), "granted");
// [ts] otherContext.close();
// [ts] context.close();
// [ts] });
// [ts] });
// [ts] test("PE-014 should support clipboard read", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27475" });
// [ts] it.fail(browserName == "firefox", "No such permissions (requires flag) in Firefox");
// [ts] page.goto(server.EMPTY_PAGE);
// There is no 'clipboard-read' permission in WebKit Web API.
// [ts] if (browserName != "webkit")
// [ts] expect_eq(getPermission(page, "clipboard-read"), "prompt");
// [ts] if (isHeadlessShell) {
// Chromium (but not headless-shell) shows a dialog and does not resolve the promise.
// [ts] const error = page.evaluate(() => navigator.clipboard.readText()).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.toString(), "denied") != null, true);
// [ts] }
// [ts] context.grantPermissions(["clipboard-read"]);
// [ts] if (browserName != "webkit")
// [ts] expect_eq(getPermission(page, "clipboard-read"), "granted");
// There is no 'clipboard-write' permission in WebKit Web API.
// [ts] if (browserName == "chromium")
// [ts] context.grantPermissions(["clipboard-write"]);
// [ts] page.evaluate(() => navigator.clipboard.writeText("test content"));
// [ts] expect_eq(page.evaluate(() => navigator.clipboard.readText()), "test content");
// [ts] });
// [ts] test("PE-015 should isolate the headless clipboard from the operating system", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/13097" },
// [ts] }, (_p) => {
// [ts] test(isFrozenWebkit, "needs recent webkit");
// Each headless browser gets its own clipboard, so neither can see the clipboard of the operating system nor of another browser.
// [ts] const browser1 = browserType.launch({ headless: true });
// [ts] const browser2 = browserType.launch({ headless: true });
// [ts] const [page1, page2] = joinAll([browser1, browser2].map(async browser => { // migrated: Promise.all -> joinAll
// [ts] const context = browser.newContext();
// There is no 'clipboard-write' permission in WebKit Web API and no clipboard permission at all in Firefox.
// [ts] if (browserName == "chromium")
// [ts] context.grantPermissions(["clipboard-read", "clipboard-write"]);
// [ts] else if (browserName == "webkit")
// [ts] context.grantPermissions(["clipboard-read"]);
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] return page;
// [ts] }));
// [ts] page1.evaluate(() => navigator.clipboard.writeText("first"));
// [ts] expect_eq(page1.evaluate(() => navigator.clipboard.readText()), "first");
// [ts] page2.evaluate(() => navigator.clipboard.writeText("second"));
// [ts] expect_eq(page2.evaluate(() => navigator.clipboard.readText()), "second");
// [ts] expect_eq(page1.evaluate(() => navigator.clipboard.readText()), "first");
// [ts] joinAll([browser1.close(), browser2.close()]); // migrated: Promise.all -> joinAll
// [ts] });
// [ts] test("PE-016 storage access", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31227" }
// [ts] }, (_p) => {
// [ts] test(browserName != "chromium", "chromium-only api");
// [ts] context.grantPermissions(["storage-access"]);
// [ts] expect_eq(getPermission(page, "storage-access"), "granted");
// [ts] server.setRoute("/set-cookie.html", (req, res) => {
// [ts] res.setHeader("Set-Cookie", "name=value; Path=/; SameSite=Strict; Secure");
// [ts] res.end();
// [ts] });
// [ts] server.setRoute("/my-frame.html", (req, res) => {
// [ts] res.setHeader("Content-type", "text/html");
// [ts] res.end(`<iframe src="${server.CROSS_PROCESS_PREFIX + "/empty.html"}"></iframe>`);
// [ts] });
// Navigate once to the domain as top level.
// [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/set-cookie.html");
// [ts] page.goto(server.PREFIX + "/my-frame.html");
// [ts] const frame = page.frames()[1];
// [ts] expect_eq(getPermission(frame, "storage-access"), "granted");
// [ts] const access = frame.evaluate(() => document.requestStorageAccess().then(() => true, () => false));
// [ts] expect_eq(access, true);
// [ts] expect_eq(frame.evaluate(() => document.hasStorageAccess()), true);
// [ts] });
// [ts] it.describe(() => {
// Secure context
// [ts] it.use({ ignoreHTTPSErrors: true, });
// [ts] test("PE-017 should be able to use the local-fonts API", () => {
// [ts] test(browserName != "chromium", "chromium-only api");
// [ts] test(!!channel && channel.startsWith("msedge"), "always times out in edge");
// [ts] it.fixme(!headless, "times out in headed");
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/36113" });
// [ts] page.goto(httpsServer.EMPTY_PAGE);
// [ts] expect_eq(getPermission(page, "local-fonts"), "prompt");
// [ts] context.grantPermissions(["local-fonts"]);
// [ts] expect_eq(getPermission(page, "local-fonts"), "granted");
// [ts] expect_eq(page.evaluate(() => ((window).queryLocalFonts()).length > 0), true);
// [ts] });
// [ts] });
// [ts] test("PE-018 local network request is allowed from public origin", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37861" }
// [ts] }, (_p) => {
// [ts] test(browserName == "webkit");
// [ts] test(browserName == "chromium" && browserMajorVersion < 145, "local-network-access permission support has changed between versions");
// [ts] if (browserName == "chromium" || browserName == "firefox")
// [ts] context.grantPermissions(["local-network-access"]);
// [ts] const serverRequests = [];
// [ts] server.setRoute("/cors", (req, res) => {
// [ts] serverRequests.push(`${req.method} ${req.url}`);
// [ts] if (req.method == "OPTIONS") {
// [ts] res.writeHead(204, {
// [ts] "Access-Control-Allow-Origin": "*",
// [ts] "Access-Control-Allow-Methods": "GET, POST, PUT, OPTIONS",
// [ts] "Access-Control-Allow-Headers": "*",
// [ts] });
// [ts] res.end();
// [ts] return;
// [ts] }
// [ts] res.writeHead(200, { "Content-type": "text/plain", "Access-Control-Allow-Origin": "*" });
// [ts] res.end("Hello there!");
// [ts] });
// [ts] const clientRequests = [];
// Has to be a public origin.
// [ts] page.goto("https://demo.playwright.dev/todomvc/");
// [ts] page.on("request", request => {
// [ts] clientRequests.push(`${request.method()} ${request.url()}`);
// [ts] });
// [ts] const response = page.evaluate(async url => {
// [ts] const response = fetch(url, {
// [ts] method: "POST",
// [ts] body: "",
// [ts] headers: {
// [ts] "Content-Type": "application/json",
// [ts] "X-Custom-Header": "test-value"
// [ts] }
// [ts] });
// [ts] return response.text();
// [ts] }, server.CROSS_PROCESS_PREFIX + "/cors").catch(e => e.message);
// [ts] expect_eq(response, "Hello there!");
// [ts] expect(serverRequests).toEqual([
// [ts] "OPTIONS /cors",
// [ts] "POST /cors",
// [ts] ]);
// [ts] expect(clientRequests).toEqual([
// [ts] `POST ${server.CROSS_PROCESS_PREFIX}/cors`,
// [ts] ]);
// [ts] });
// [ts] test("PE-019 can request screen-wake-lock", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39115" }
// [ts] }, (_p) => {
// [ts] context.grantPermissions(["screen-wake-lock"]);
// [ts] page.route("**/*", route => route.fulfill({ status: 200, body: "<div>Hello there!</div>", contentType: "text/html" }));
// [ts] page.goto("https://example.com");
// [ts] page.evaluate(() => navigator.wakeLock.request("screen"));
// [ts] });
// [ts] it.describe("camera and microphone", () => {
// WebKit exposes deterministic mock capture devices; capture is gated on the
// 'camera'/'microphone' permissions. Chromium needs --use-fake-device-for-media-stream
// and Firefox has no camera/microphone permission mapping, so scope to WebKit.
// [ts] test((_p) => browserName != "webkit", "WebKit mock-capture-device based test");
// Native Windows WebKit is built without a media-capture backend, so getUserMedia is
// unavailable there; WSL runs the Linux build, which does have mock capture.
// [ts] test((_p) => isWindows && channel != "webkit-wsl", "WebKit has no media capture on Windows");
// [ts] async fn getUserMedia(page: auto, constraints: auto) {
// [ts] return page.evaluate(async constraints => {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] const stream = navigator.mediaDevices.getUserMedia(constraints);
// [ts] const tracks = stream.getTracks().map(track => ({ kind: track.kind, live: track.readyState == "live" }));
// [ts] stream.getTracks().forEach(track => track.stop());
// [ts] tracks.sort((a, b) => a.kind.localeCompare(b.kind));
// [ts] return { tracks };
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] return { error: error.name };
// [ts] }
// [ts] }, constraints);
// [ts] }
// [ts] test("PE-020 should capture when camera and microphone are granted", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/2973" }
// [ts] }, (_p) => {
// [ts] test(isFrozenWebkit, "Mock capture device support requires a newer WebKit build");
// [ts] context.grantPermissions(["camera", "microphone"], { origin: server.PREFIX });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect: (getUserMedia(page, { video) => {
// [ts] tracks: [
// [ts] { kind: "audio", live: true },
// [ts] { kind: "video", live: true },
// [ts] ],
// [ts] });
// [ts] });
// [ts] test("PE-021 should reject when no permission is granted", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/2973" }
// [ts] }, (_p) => {
// [ts] test(isFrozenWebkit, "Mock capture device support requires a newer WebKit build");
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect_eq(getUserMedia(page, { video: true, audio: true }), { error: "NotAllowedError" });
// [ts] });
// [ts] test("PE-022 should gate audio and video independently", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/2973" }
// [ts] }, (_p) => {
// [ts] test(isFrozenWebkit, "Mock capture device support requires a newer WebKit build");
// [ts] context.grantPermissions(["camera"], { origin: server.PREFIX });
// [ts] page.goto(server.EMPTY_PAGE);
// Camera-only request is allowed.
// [ts] expect_eq(getUserMedia(page, { video: true }), { tracks: [{ kind: "video", live: true }] });
// Microphone request is rejected because it was not granted.
// [ts] expect_eq(getUserMedia(page, { audio: true }), { error: "NotAllowedError" });
// [ts] });
// [ts] test("PE-023 should stop capturing after permissions are cleared", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/2973" }
// [ts] }, (_p) => {
// [ts] test(isFrozenWebkit, "Mock capture device support requires a newer WebKit build");
// [ts] context.grantPermissions(["camera"], { origin: server.PREFIX });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect_eq(getUserMedia(page, { video: true }), { tracks: [{ kind: "video", live: true }] });
// [ts] context.clearPermissions();
// [ts] expect_eq(getUserMedia(page, { video: true }), { error: "NotAllowedError" });
// [ts] });
// [ts] });
// [ts] } // [cap-migrate] auto-closed brace
// [ts] }
