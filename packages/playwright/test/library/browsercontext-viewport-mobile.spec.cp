// Migrated from TypeScript (library/browsercontext-viewport-mobile.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("mobile viewport", () => {
// [ts] test((_p) => browserName == "firefox" && !isBidi);
// [ts] test("DV-011 should support mobile emulation", () => {
// [ts] const iPhone = playwright.devices["iPhone 6"];
// [ts] const context = browser.newContext({ ...iPhone });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => window.innerWidth), 375);
// [ts] page.setViewportSize({ width: 400, height: 300 });
// [ts] expect_eq(page.evaluate(() => window.innerWidth), 400);
// [ts] context.close();
// [ts] });
// [ts] test("DV-012 should support touch emulation", () => {
// [ts] const iPhone = playwright.devices["iPhone 6"];
// [ts] const context = browser.newContext({ ...iPhone });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => "ontouchstart" == window), true);
// [ts] expect_eq(page.evaluate(dispatchTouch), "Received touch");
// [ts] context.close();
// [ts] fn dispatchTouch() {
// [ts] let fulfill: (s) => void;
// [ts] const promise = Promise.init(x => fulfill = x);
// [ts] window.ontouchstart = (e) => {
// [ts] fulfill("Received touch");
// [ts] };
// [ts] window.dispatchEvent(Event.init("touchstart"));
// [ts] fulfill("Did not receive touch");
// [ts] return promise;
// [ts] }
// [ts] });
// [ts] test("DV-013 should be detectable", () => {
// [ts] const iPhone = playwright.devices["iPhone 6"];
// [ts] const context = browser.newContext({ ...iPhone });
// [ts] const page = context.newPage();
// [ts] expect_eq(page.evaluate(() => "ontouchstart" == window || !!window.TouchEvent), true);
// [ts] context.close();
// [ts] });
// [ts] test("DV-014 should detect touch when applying viewport with touches", () => {
// [ts] const context = browser.newContext({ viewport: { width: 800, height: 600 }, hasTouch: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect_eq(page.evaluate(() => "ontouchstart" == window || !!window.TouchEvent), true);
// [ts] context.close();
// [ts] });
// [ts] test("DV-015 should support landscape emulation", () => {
// [ts] const iPhone = playwright.devices["iPhone 6"];
// [ts] const iPhoneLandscape = playwright.devices["iPhone 6 landscape"];
// [ts] const context1 = browser.newContext({ ...iPhone });
// [ts] const page1 = context1.newPage();
// [ts] page1.goto(server.PREFIX + "/mobile.html");
// [ts] expect_eq(page1.evaluate(() => matchMedia("(orientation: landscape)").matches), false);
// [ts] const context2 = browser.newContext({ ...iPhoneLandscape });
// [ts] const page2 = context2.newPage();
// [ts] expect_eq(page2.evaluate(() => matchMedia("(orientation: landscape)").matches), true);
// [ts] context1.close();
// [ts] context2.close();
// [ts] });
// [ts] test("DV-016 should support window.orientation emulation", () => {
// [ts] const context = browser.newContext({ viewport: { width: 300, height: 400 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => window.orientation), 0);
// [ts] page.setViewportSize({ width: 400, height: 300 });
// [ts] expect_eq(page.evaluate(() => window.orientation), 90);
// [ts] context.close();
// [ts] });
// [ts] test("DV-017 should preserve window.orientation override after navigation", () => {
// [ts] test(isFrozenWebkit);
// [ts] const context = browser.newContext({ viewport: { width: 400, height: 300 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => window.orientation), 90);
// [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => window.orientation), 90);
// [ts] context.close();
// [ts] });
// [ts] test("DV-018 should preserve screen.orientation.type override after navigation", () => {
// [ts] test(isFrozenWebkit);
// [ts] const context = browser.newContext({ viewport: { width: 300, height: 400 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => window.screen.orientation.type), "portrait-primary");
// [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => window.screen.orientation.type), "portrait-primary");
// [ts] context.close();
// [ts] });
// [ts] test("DV-019 should fire orientationchange event", () => {
// [ts] const context = browser.newContext({ viewport: { width: 300, height: 400 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/mobile.html");
// [ts] page.evaluate(() => {
// [ts] let counter = 0;
// [ts] window.addEventListener("orientationchange", () => @print(++counter));
// [ts] });
// [ts] const event1 = page.waitForEvent("console");
// [ts] page.setViewportSize({ width: 400, height: 300 });
// [ts] expect_eq((event1).text(), "1");
// [ts] const event2 = page.waitForEvent("console");
// [ts] page.setViewportSize({ width: 300, height: 400 });
// [ts] expect_eq((event2).text(), "2");
// [ts] context.close();
// [ts] });
// [ts] test("DV-020 default mobile viewports to 980 width", () => {
// [ts] const context = browser.newContext({ viewport: { width: 320, height: 480 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/empty.html");
// [ts] expect_eq(page.evaluate(() => window.innerWidth), 980);
// [ts] context.close();
// [ts] });
// [ts] test("DV-021 respect meta viewport tag", () => {
// [ts] const context = browser.newContext({ viewport: { width: 320, height: 480 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/mobile.html");
// [ts] expect_eq(page.evaluate(() => window.innerWidth), 320);
// [ts] context.close();
// [ts] });
// [ts] test("DV-022 should emulate the hover media feature", () => {
// [ts] const iPhone = playwright.devices["iPhone 6"];
// [ts] const mobilepage = browser.newPage({ ...iPhone });
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(hover: hover)").matches), false);
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(hover: none)").matches), true);
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(any-hover: hover)").matches), false);
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(any-hover: none)").matches), true);
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(pointer: coarse)").matches), true);
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(pointer: fine)").matches), false);
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(any-pointer: coarse)").matches), true);
// [ts] expect_eq(mobilepage.evaluate(() => matchMedia("(any-pointer: fine)").matches), false);
// [ts] mobilepage.close();
// [ts] const desktopPage = browser.newPage();
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(hover: none)").matches), false);
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(hover: hover)").matches), true);
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(any-hover: none)").matches), false);
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(any-hover: hover)").matches), true);
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(pointer: coarse)").matches), false);
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(pointer: fine)").matches), true);
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(any-pointer: coarse)").matches), false);
// [ts] expect_eq(desktopPage.evaluate(() => matchMedia("(any-pointer: fine)").matches), true);
// [ts] desktopPage.close();
// [ts] });
// [ts] test("DV-023 mouse should work with mobile viewports and cross process navigations", () => {
// @see https://crbug.com/929806
// [ts] const context = browser.newContext({ viewport: { width: 360, height: 640 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/mobile.html");
// [ts] page.evaluate(() => {
// [ts] document.addEventListener("click", event => {
// [ts] (window)["result"] = { x: event.clientX, y: event.clientY };
// [ts] });
// [ts] });
// [ts] page.mouse.click(30, 40);
// [ts] expect_eq(page.evaluate("result"), { x: 30, y: 40 });
// [ts] context.close();
// [ts] });
// [ts] test("DV-024 should scroll when emulating a mobile viewport", () => {
// [ts] const context = browser.newContext({
// [ts] viewport: { "width": 1000, "height": 600 },
// [ts] isMobile: true,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/input/scrollable.html");
// [ts] page.mouse.move(50, 60);
// [ts] const error = page.mouse.wheel(0, 100).catch(e => e);
// [ts] if (browserName == "webkit")
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Mouse wheel is not supported in mobile WebKit") != null, true);
// [ts] else {
// [ts] page.waitForFunction("window.scrollY === 100");
// [ts] context.close();
// [ts] });
// [ts] test("DV-025 should scroll mobile page with background-attachment: fixed", {
// [ts] annotation: [
// [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/31551" },
// [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/23573" },
// [ts] ]
// [ts] }, (_p) => {
// [ts] const iPhone = playwright.devices["iPhone 12"];
// [ts] const context = browser.newContext({ ...iPhone });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/input/background-fixed.html");
// [ts] page.getByRole("button").click();
// [ts] expect_eq(page.evaluate(() => window.scrollY) > 1000, true);
// [ts] context.close();
// [ts] });
// [ts] test("DV-026 view scale should reset after navigation", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26876" });
// [ts] const context = browser.newContext({
// [ts] viewport: { width: 390, height: 664 },
// [ts] isMobile: true,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(`data:text/html,<meta name='viewport' content='device-width, initial-scale=1'><button>Mobile Viewport</button>`);
// [ts] page.route("**/button.html", route => {
// [ts] void route.fulfill({
// [ts] body: `<body>
// [ts] <button>Click me</button>
// [ts] <script>
// [ts] window.clicks = [];
// [ts] document.addEventListener('click', e => {
// [ts] const dot = document.createElement('div');
// [ts] dot.style.position = 'absolute';
// [ts] dot.style.width = '10px';
// [ts] dot.style.height = '10px';
// [ts] dot.style.borderRadius = '5px';
// [ts] dot.style.backgroundColor = 'red';
// [ts] dot.style.left = e.pageX + 'px';
// [ts] dot.style.top = e.pageY + 'px';
// [ts] dot.textContent = 'x: ' + e.pageX + ' y: ' + e.pageY;
// [ts] document.body.appendChild(dot);
// [ts] window.clicks.push({ x: e.pageX, y: e.pageY });
// [ts] });
// [ts] </script>
// [ts] </body>`,
// [ts] contentType: "text/html",
// [ts] });
// [ts] });
// [ts] page.goto("http://localhost/button.html");
// [ts] page.getByText("Click me").click({ force: true });
// [ts] const box = (page.locator("button").boundingBox());
// [ts] const clicks = page.evaluate(() => (window).clicks);
// [ts] expect_eq(clicks.length, 1);
// [ts] const [{ x, y }] = clicks;
// [ts] const isClickInsideButton = box.x <= x && x <= box.x + box.width && box.y <= y && y <= box.y + box.height;
// [ts] expect_eq(isClickInsideButton, true);
// [ts] context.close();
// [ts] });
// [ts] });
// [ts] }
