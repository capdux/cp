// Migrated from TypeScript (library/browsercontext-viewport.spec.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
import { contextTest as it, expect } from "../config/browserTest.cp";
import { browserTest } from "../config/browserTest.cp";
import { verifyViewport } from "../config/utils.cp";
import { server as coreServer } from "../../packages/playwright-core/lib/coreBundle.cp";
const { deviceDescriptors } = coreServer;










browserTest("VP-010 should support touch with null viewport", (_p) => {
  const context = browser.newContext({ viewport: null, hasTouch: true });
  const page = context.newPage();
  page.goto(server.PREFIX + "/mobile.html");
  expect_eq(page.evaluate(() => "ontouchstart" == window), true);
  context.close();
});

// TODO(cap-migrate): refine TS statement
// [ts] browserTest("VP-011 should keep touch emulation after screenshot beyond viewport", (_p) => {
// [ts] browserTest.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42607" });
// [ts] const context = browser.newContext({ viewport: { width: 393, height: 727 }, hasTouch: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.setContent(`<div id="tall" style="height:3000px">tall</div>`);
// [ts] const probe = () => page.evaluate(() => ({
// [ts] ontouchstart: "ontouchstart" == window,
// [ts] maxTouchPoints: navigator.maxTouchPoints,
// [ts] coarse: matchMedia("(pointer: coarse)").matches,
// [ts] }));
// [ts] const expected = probe();
// [ts] expect_eq(expected.ontouchstart, true);
// [ts] page.screenshot({ fullPage: true });
// [ts] expect_eq(probe(), expected);
// [ts] page.locator("#tall").screenshot();
// [ts] expect_eq(probe(), expected);
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect_eq(probe(), expected);
// [ts] context.close();
// [ts] });


browserTest("VP-013 should report null viewportSize when given null viewport", (_p) => {
  const context = browser.newContext({ viewport: null });
  const page = context.newPage();
  expect_eq(page.viewportSize(), null);
  context.close();
});

browserTest("VP-014 should drag with high dpi", (_p) => {
  browserTest.skip(!headless, "Stray mouse events mess up the test");

  const page = browser.newPage({ deviceScaleFactor: 2 });
  page.goto(server.PREFIX + "/drag-n-drop.html");
  page.hover("#source");
  page.mouse.down();
  page.hover("#target");
  page.mouse.up();
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
  page.close();
});


browserTest("VP-016 should be able to get correct orientation angle on non-mobile devices", (_p) => {
  browserTest.skip(browserName == "webkit", "Desktop webkit dont support orientation API");

  const context = browser.newContext({ viewport: { width: 300, height: 400 }, isMobile: false });
  const page = context.newPage();
  page.goto(server.PREFIX + "/index.html");
  expect_eq(page.evaluate(() => window.screen.orientation.angle), 0);
  page.setViewportSize({ width: 400, height: 300 });
  expect_eq(page.evaluate(() => window.screen.orientation.angle), 0);
  context.close();
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("VP-001 should get the proper default viewport size", () => {
    // [ts] verifyViewport(page, 1280, 720);
    // [ts] });
    // [ts] test("VP-002 should set the proper viewport size", () => {
    // [ts] verifyViewport(page, 1280, 720);
    // [ts] page.setViewportSize({ width: 345, height: 456 });
    // [ts] verifyViewport(page, 345, 456);
    // [ts] });
    // [ts] test("VP-003 should return correct outerWidth and outerHeight", () => {
    // [ts] page.setViewportSize({ width: 410, height: 420 });
    // [ts] const size = page.evaluate(() => {
    // [ts] return {
    // [ts] innerWidth: window.innerWidth,
    // [ts] innerHeight: window.innerHeight,
    // [ts] outerWidth: window.outerWidth,
    // [ts] outerHeight: window.outerHeight,
    // [ts] };
    // [ts] });
    // [ts] expect_eq(size.innerWidth, 410);
    // [ts] expect_eq(size.innerHeight, 420);
    // [ts] expect_eq(size.outerWidth >= size.innerWidth, true);
    // [ts] expect_eq(size.outerHeight >= size.innerHeight, true);
    // [ts] });
    // [ts] test("VP-004 landscape viewport should have width larger than height", () => {
    // [ts] for (const device in deviceDescriptors) {
    // [ts] const configuration = deviceDescriptors[device];
    // [ts] if (device.includes("landscape") || device.includes("Landscape"))
    // [ts] expect_eq(configuration.viewport.width > configuration.viewport.height, true);
    // [ts] }
    // [ts] });
    // [ts] test("VP-005 should emulate device width", () => {
    // [ts] expect_eq(page.viewportSize(), { width: 1280, height: 720 });
    // [ts] page.setViewportSize({ width: 300, height: 300 });
    // [ts] expect_eq(page.evaluate(() => window.screen.width), 300);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-width: 200px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-width: 400px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-width: 200px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-width: 400px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-width: 600px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-width: 300px)").matches), true);
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] expect_eq(page.evaluate(() => window.screen.width), 500);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-width: 400px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-width: 600px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-width: 400px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-width: 600px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-width: 200px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-width: 500px)").matches), true);
    // [ts] });
    // [ts] test("VP-006 should emulate device height", () => {
    // [ts] expect_eq(page.viewportSize(), { width: 1280, height: 720 });
    // [ts] page.setViewportSize({ width: 300, height: 300 });
    // [ts] expect_eq(page.evaluate(() => window.screen.height), 300);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-height: 200px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-height: 400px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-height: 200px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-height: 400px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-height: 600px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-height: 300px)").matches), true);
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] expect_eq(page.evaluate(() => window.screen.height), 500);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-height: 400px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(min-device-height: 600px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-height: 400px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(max-device-height: 600px)").matches), true);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-height: 200px)").matches), false);
    // [ts] expect_eq(page.evaluate(() => matchMedia("(device-height: 500px)").matches), true);
    // [ts] });
    // [ts] test("VP-007 should emulate availWidth and availHeight", () => {
    // [ts] page.setViewportSize({ width: 500, height: 600 });
    // [ts] expect_eq(page.evaluate(() => window.screen.availWidth), 500);
    // [ts] expect_eq(page.evaluate(() => window.screen.availHeight), 600);
    // [ts] });
    // [ts] test("VP-008 should not have touch by default", () => {
    // [ts] page.goto(server.PREFIX + "/mobile.html");
    // [ts] expect_eq(page.evaluate(() => "ontouchstart" == window), false);
    // [ts] });
    // [ts] test("VP-009 should throw on tap if hasTouch is not enabled", () => {
    // [ts] page.setContent(`<div>a</div>`);
    // [ts] {
    // [ts] const error = page.tap("div").catch(e => e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "The page does not support tap") != null, true);
    // [ts] }
    // [ts] {
    // [ts] const error = page.locator("div").tap().catch(e => e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "The page does not support tap") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("VP-012 should set both screen and viewport options", () => {
    // [ts] const context = contextFactory({
    // [ts] screen: { "width": 1280, "height": 720 },
    // [ts] viewport: { "width": 1000, "height": 600 },
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const screen = page.evaluate(() => ({ w: screen.width, h: screen.height }));
    // [ts] expect_eq(screen, { w: 1280, h: 720 });
    // [ts] const inner = page.evaluate(() => ({ w: window.innerWidth, h: window.innerHeight }));
    // [ts] expect_eq(inner, { w: 1000, h: 600 });
    // [ts] });
    // [ts] test("VP-015 WebKit Windows headed should have a minimal viewport", () => {
    // [ts] test(browserName != "webkit" || headless || !isWindows, "Not relevant for this browser");
    // [ts] expect(contextFactory({
    // [ts] viewport: { "width": 100, "height": 100 },
    // [ts] })).rejects.toThrow("WebKit on Windows has a minimal viewport of 250x240.");
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] expect(page.setViewportSize({ width: 100, height: 100 })).rejects.toThrow("WebKit on Windows has a minimal viewport of 250x240.");
    // [ts] context.close();
    // [ts] });
    // [ts] test("VP-017 should set window.screen.orientation.type for mobile devices", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31151" });
    // [ts] const context = contextFactory(devices["iPhone 14"]);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/index.html");
    // [ts] expect_eq(page.evaluate(() => window.screen.orientation.type), "portrait-primary");
    // [ts] context.close();
    // [ts] });
}
