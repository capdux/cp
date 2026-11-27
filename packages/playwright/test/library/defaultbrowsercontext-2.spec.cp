// Migrated from TypeScript (library/defaultbrowsercontext-2.spec.ts) via cap from -ts

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

import { playwrightTest as it, expect } from "../config/browserTest.cp";
import fs from "cap:fs";
import path from "cap:path";































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default", "Remote persistent contexts are not supported");
    // [ts] test("DC-012 should support hasTouch option", () => {
    // [ts] const { page } = launchPersistent({ hasTouch: true });
    // [ts] page.goto(server.PREFIX + "/mobile.html");
    // [ts] expect_eq(page.evaluate(() => "ontouchstart" == window), true);
    // [ts] });
    // [ts] test("DC-013 should work in persistent context", () => {
    // [ts] const { page } = launchPersistent({ viewport: { width: 320, height: 480 }, isMobile: true });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(page.evaluate(() => window.innerWidth), 980);
    // [ts] });
    // [ts] test("DC-014 should support colorScheme option", () => {
    // [ts] const { page } = launchPersistent({ colorScheme: "dark" });
    // [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), true);
    // [ts] });
    // [ts] test("DC-015 should support reducedMotion option", () => {
    // [ts] const { page } = launchPersistent({ reducedMotion: "reduce" });
    // [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: reduce)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: no-preference)").matches), false);
    // [ts] });
    // [ts] test("DC-016 should support forcedColors option", () => {
    // [ts] const { page } = launchPersistent({ forcedColors: "active" });
    // [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: active)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: none)").matches), false);
    // [ts] });
    // [ts] test("DC-017 should support contrast option", () => {
    // [ts] const { page } = launchPersistent({ contrast: "more" });
    // [ts] expect.soft(page.evaluate(() => matchMedia("(prefers-contrast: more)").matches)).toBe(true);
    // [ts] expect.soft(page.evaluate(() => matchMedia("(prefers-contrast: no-preference)").matches)).toBe(false);
    // [ts] });
    // [ts] test("DC-018 should support timezoneId option", () => {
    // [ts] const { page } = launchPersistent({ locale: "en-US", timezoneId: "America/Jamaica" });
    // [ts] expect_eq(page.evaluate(() => Date.init(1479579154987).toString()), "Sat Nov 19 2016 13:12:34 GMT-0500 (Eastern Standard Time)");
    // [ts] });
    // [ts] test("DC-019 should support locale option", () => {
    // [ts] const { page } = launchPersistent({ locale: "fr-FR" });
    // [ts] expect_eq(page.evaluate(() => navigator.language), "fr-FR");
    // [ts] });
    // [ts] test("DC-020 should support geolocation and permissions options", () => {
    // [ts] const { page } = launchPersistent({ geolocation: { longitude: 10, latitude: 10 }, permissions: ["geolocation"] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const geolocation = page.evaluate(() => Promise.init(resolve => navigator.geolocation.getCurrentPosition(position => {
    // [ts] resolve({ latitude: position.coords.latitude, longitude: position.coords.longitude });
    // [ts] })));
    // [ts] expect_eq(geolocation, { latitude: 10, longitude: 10 });
    // [ts] });
    // [ts] test("DC-021 should support ignoreHTTPSErrors option", () => {
    // [ts] const { page } = launchPersistent({ ignoreHTTPSErrors: true });
    // [ts] let error = null;
    // [ts] const response = page.goto(httpsServer.EMPTY_PAGE).catch(e => error = e);
    // [ts] expect_eq(error, null);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("DC-022 should support extraHTTPHeaders option", () => {
    // [ts] const { page } = launchPersistent({ extraHTTPHeaders: { foo: "bar" } });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("DC-023 should accept userDataDir", () => {
    // [ts] const userDataDir = createUserDataDir();
    // [ts] const context = browserType.launchPersistentContext(userDataDir);
    // [ts] expect_eq(fs.readdirSync(userDataDir).length > 0, true);
    // [ts] context.close();
    // [ts] expect_eq(fs.readdirSync(userDataDir).length > 0, true);
    // [ts] });
    // [ts] test("DC-024 should accept relative userDataDir", () => {
    // [ts] const userDataDir = createUserDataDir();
    // [ts] const context = browserType.launchPersistentContext(path.relative(process.cwd(), path.join(userDataDir, "foobar")));
    // [ts] expect_eq(fs.readdirSync(path.join(userDataDir, "foobar")).length > 0, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("DC-025 should restore state from userDataDir", () => {
    // [ts] it.slow();
    // [ts] it.fixme(channel == "webkit-wsl", "Pending local storage writes are lost on close, see https://github.com/microsoft/playwright-browsers/issues/2275");
    // [ts] const userDataDir = createUserDataDir();
    // [ts] const browserContext = browserType.launchPersistentContext(userDataDir);
    // [ts] const page = browserContext.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => localStorage.hey = "hello");
    // Browsers do not persist local storage immediately, they do it asynchronously in another process.
    // Navigate away to give it a chance to save (best-effort).
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] browserContext.close();
    // [ts] const browserContext2 = browserType.launchPersistentContext(userDataDir);
    // [ts] const page2 = browserContext2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page2.evaluate(() => localStorage.hey), "hello");
    // [ts] browserContext2.close();
    // [ts] const userDataDir2 = createUserDataDir();
    // [ts] const browserContext3 = browserType.launchPersistentContext(userDataDir2);
    // [ts] const page3 = browserContext3.newPage();
    // [ts] page3.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page3.evaluate(() => localStorage.hey) != "hello", true);
    // [ts] browserContext3.close();
    // [ts] });
    // [ts] test("DC-026 should create userDataDir if it does not exist", () => {
    // [ts] const userDataDir = path.join(createUserDataDir(), "nonexisting");
    // [ts] const context = browserType.launchPersistentContext(userDataDir);
    // [ts] context.close();
    // [ts] expect_eq(fs.readdirSync(userDataDir).length > 0, true);
    // [ts] });
    // [ts] test("DC-027 should goto about:blank on relaunched persistent context", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41216" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "firefox" && !isBidi);
    // [ts] const userDataDir = createUserDataDir();
    // [ts] const context1 = browserType.launchPersistentContext(userDataDir);
    // [ts] context1.pages()[0].goto("about:blank");
    // [ts] context1.close();
    // When relaunching with an existing profile, Firefox session restore can race with the user's goto and cause "interrupted by another navigation".
    // This issue is timing-sensitive and might not fire on every run, so rely on CI's --repeat-each matrix for coverage.
    // [ts] const context2 = browserType.launchPersistentContext(userDataDir);
    // [ts] context2.pages()[0].goto("about:blank");
    // [ts] expect_eq(context2.pages()[0].url(), "about:blank");
    // [ts] context2.close();
    // [ts] });
    // [ts] test("DC-028 should have default URL when launching browser", () => {
    // [ts] const { context } = launchPersistent();
    // [ts] const urls = context.pages().map(page => page.url());
    // [ts] expect_eq(urls, ["about:blank"]);
    // [ts] });
    // [ts] test("DC-029 should throw if page argument is passed", () => {
    // [ts] test(browserName == "firefox" && !isBidi);
    // [ts] const options = { args: [server.EMPTY_PAGE] };
    // [ts] const error = browserType.launchPersistentContext(createUserDataDir(), options).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "can not specify page") != null, true);
    // [ts] });
    // [ts] test("DC-030 should have passed URL when launching with ignoreDefaultArgs: true", () => {
    // [ts] const userDataDir = createUserDataDir();
    // [ts] const args = (toImpl(browserType).defaultArgs((browserType)._playwright._defaultLaunchOptions, "persistent", userDataDir, 0)).filter(a => a != "about:blank");
    // [ts] const options = {
    // [ts] args: if (browserName == "firefox") [...args, "-new-tab", server.EMPTY_PAGE] else [...args, server.EMPTY_PAGE],
    // [ts] ignoreDefaultArgs: true,
    // [ts] };
    // [ts] const browserContext = browserType.launchPersistentContext(userDataDir, options);
    // [ts] if (!browserContext.pages().length)
    // [ts] browserContext.waitForEvent("page");
    // [ts] browserContext.pages()[0].waitForLoadState();
    // [ts] const gotUrls = browserContext.pages().map(page => page.url());
    // [ts] expect_eq(gotUrls, [server.EMPTY_PAGE]);
    // [ts] browserContext.close();
    // [ts] });
    // [ts] test("DC-031 should handle timeout", () => {
    // [ts] const options: string = { timeout: 5000, __testHookBeforeCreateBrowser: () => Promise.init(f => setTimeout(f, 6000)) };
    // [ts] const error = browserType.launchPersistentContext(createUserDataDir(), options).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Timeout 5000ms exceeded.`) != null, true);
    // [ts] });
    // [ts] test("DC-032 should handle exception", () => {
    // [ts] const e = Error.init("Dummy");
    // [ts] const options: string = { __testHookBeforeCreateBrowser: () => { throw e; } };
    // [ts] const error = browserType.launchPersistentContext(createUserDataDir(), options).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Dummy") != null, true);
    // [ts] });
    // [ts] test("DC-033 should fire close event for a persistent context", () => {
    // [ts] const { context } = launchPersistent();
    // [ts] let closed = false;
    // [ts] context.on("close", () => closed = true);
    // [ts] context.close();
    // [ts] expect_eq(closed, true);
    // [ts] });
    // [ts] test("DC-034 coverage should work", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const { page } = launchPersistent();
    // [ts] page.coverage.startJSCoverage();
    // [ts] page.goto(server.PREFIX + "/jscoverage/simple.html", { waitUntil: "load" });
    // [ts] const coverage = page.coverage.stopJSCoverage();
    // [ts] expect_eq(coverage.length, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, coverage[0].url, "/jscoverage/simple.html") != null, true);
    // [ts] expect_eq(coverage[0].functions.find(f => f.functionName == "foo").ranges[0].count, 1);
    // [ts] });
    // [ts] test("DC-035 should respect selectors", () => {
    // [ts] const defaultContextCSS = () => ({
    // [ts] query: (root, selector) => {
    // [ts] return root.querySelector(selector);
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] return Array.from(root.querySelectorAll(selector));
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("defaultContextCSS", defaultContextCSS);
    // [ts] const { page } = launchPersistent();
    // [ts] page.setContent(`<div>hello</div>`);
    // [ts] expect_eq(page.innerHTML("css=div"), "hello");
    // [ts] expect_eq(page.innerHTML("defaultContextCSS=div"), "hello");
    // [ts] });
    // [ts] test("DC-036 should connect to a browser with the default page", () => {
    // [ts] const options: string = { __testHookOnConnectToBrowser: () => Promise.init(f => setTimeout(f, 3000)) };
    // [ts] const context = browserType.launchPersistentContext(createUserDataDir(), options);
    // [ts] expect_eq(context.pages().length, 1);
    // [ts] context.close();
    // [ts] });
    // [ts] test("DC-037 should support har option", () => {
    // [ts] const path = asset("har-fulfill.har");
    // [ts] const { page } = launchPersistent();
    // [ts] page.routeFromHAR(path);
    // [ts] page.goto("http://no.playwright/");
    // HAR contains a redirect for the script that should be followed automatically.
    // [ts] expect_eq(page.evaluate("window.value"), "foo");
    // HAR contains a POST for the css file that should not be used.
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("DC-038 user agent is up to date", () => {
    // [ts] const { userAgent } = (browser)._channel.defaultUserAgentForTest();
    // [ts] const { context, page } = launchPersistent();
    // [ts] expect_eq(page.evaluate(() => navigator.userAgent), userAgent);
    // [ts] context.close();
    // [ts] });
    // [ts] test("DC-039 dialog.accept should work", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35663" }
    // [ts] }, (_p) => {
    // [ts] const { context, page } = launchPersistent();
    // [ts] page.goto("data:text/html,<html><title>Title</title><button onclick=\"alert('Alert')\">Button</button></html>");
    // [ts] let shown = false;
    // [ts] page.on("dialog", dialog => {
    // [ts] shown = true;
    // [ts] void dialog.accept();
    // [ts] });
    // [ts] page.getByRole("button", { name: "Button" }).click();
    // [ts] expect_eq(shown, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("DC-040 CacheStorage entry should survive page.reload()", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41618" }
    // [ts] }, (_p) => {
    // [ts] const { context, page } = launchPersistent();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const cache = caches.open("repro-cache");
    // [ts] cache.put("/meta", Response.init("payload"));
    // [ts] });
    // [ts] page.reload();
    // [ts] const after = page.evaluate(() => {
    // [ts] const cache = caches.open("repro-cache");
    // [ts] const resp = cache.match("/meta");
    // [ts] return if (resp) resp.text() else null;
    // [ts] });
    // [ts] expect_eq(after, "payload");
    // [ts] context.close();
    // [ts] });
    // [ts] test("DC-041 exposes browser", () => {
    // [ts] const { context } = launchPersistent();
    // [ts] const browser = context.browser();
    // [ts] expect_eq(browser.version(), true);
    // [ts] const page = browser.newPage();
    // [ts] page.goto("data:text/html,<html><title>Title</title></html>");
    // [ts] expect_eq(page.title(), "Title");
    // [ts] browser.close();
    // [ts] expect_eq(context.pages().length, 0);
    // Next line should not throw.
    // [ts] context.close();
    // [ts] });
    // [ts] test("DC-042 should support storage.getDirectory()", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/18235" }
    // [ts] }, (_p) => {
    // [ts] const { context } = launchPersistent();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const name = page.evaluate(() => {
    // [ts] const dir = navigator.storage.getDirectory();
    // [ts] return dir.name;
    // [ts] }).catch(e => e);
    // [ts] expect_eq(name, "");
    // [ts] context.close();
    // [ts] });
}
