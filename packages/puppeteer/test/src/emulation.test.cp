// Migrated from TypeScript (src/emulation.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { KnownDevices, PredefinedNetworkConditions } from "puppeteer";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

const iPhone = KnownDevices["iPhone 6"];
const iPhoneLandscape = KnownDevices["iPhone 6 landscape"];

// [cap-test] describe("Emulation", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.viewport", () => {
  // [cap-test] });

  // [cap-test] describe("Page.emulate", () => {



  // [cap-test] });

  // [cap-test] describe("Page.emulateMediaType", () => {
  // [cap-test] });

  // [cap-test] describe("Page.emulateMediaType and Page.emulateMediaFeatures", () => {
  // [cap-test] });

  // [cap-test] describe("Page.emulateMediaFeatures", () => {
  // [cap-test] });

  // [cap-test] describe("Page.emulateTimezone", () => {


  // [cap-test] });

  // [cap-test] describe("Page.emulateLocale", () => {
  // [cap-test] });

  // [cap-test] describe("Page.emulateVisionDeficiency", () => {

  // [cap-test] });

  // [cap-test] describe("Page.emulateNetworkConditions", () => {


  // [cap-test] });

  // [cap-test] describe("Page.emulateCPUThrottling", () => {
  // [cap-test] });

  // [cap-test] describe("Page.emulateFocusedPage", () => {

  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should get the proper viewport size", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect_eq(page.viewport(), {width: 800, height: 600});
    // [ts] page.setViewport({width: 123, height: 456});
    // [ts] expect_eq(page.viewport(), {width: 123, height: 456});
    // [ts] });
    // [ts] test("should keep a copy of the viewport that was set", () => {
    // [ts] const {page} = getTestState();
    // [ts] const viewport = {width: 123, height: 456};
    // [ts] page.setViewport(viewport);
    // [ts] viewport.width = 789;
    // [ts] expect_eq(page.viewport(), {width: 123, height: 456});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.innerWidth;
    // [ts] }),
    // [ts] ).toBe(123);
    // [ts] });
    // [ts] test("should return a copy of the viewport", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.viewport().width = 123;
    // [ts] expect_eq(page.viewport(), {width: 800, height: 600});
    // [ts] page.setViewport({width: 400, height: 300});
    // [ts] page.viewport().width = 123;
    // [ts] expect_eq(page.viewport(), {width: 400, height: 300});
    // [ts] });
    // [ts] test("should support mobile emulation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/mobile.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.innerWidth;
    // [ts] }),
    // [ts] ).toBe(800);
    // [ts] page.setViewport(iPhone.viewport);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.innerWidth;
    // [ts] }),
    // [ts] ).toBe(375);
    // [ts] page.setViewport({width: 400, height: 300});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.innerWidth;
    // [ts] }),
    // [ts] ).toBe(400);
    // [ts] });
    // [ts] test("should support touch emulation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/mobile.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return "ontouchstart" == window;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.setViewport(iPhone.viewport);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return "ontouchstart" == window;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect_eq(page.evaluate(dispatchTouch), "Received touch");
    // [ts] page.setViewport({width: 100, height: 100});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return "ontouchstart" == window;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] fn dispatchTouch() {
    // [ts] let fulfill: (value) => void;
    // [ts] const promise = Promise.init(x => {
    // [ts] fulfill = x;
    // [ts] });
    // [ts] window.ontouchstart = () => {
    // [ts] fulfill("Received touch");
    // [ts] };
    // [ts] window.dispatchEvent(Event.init("touchstart"));
    // [ts] fulfill("Did not receive touch");
    // [ts] return promise;
    // [ts] }
    // [ts] });
    // [ts] test("should be detectable by Modernizr", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/detect-touch.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent.trim();
    // [ts] }),
    // [ts] ).toBe("NO");
    // [ts] page.setViewport(iPhone.viewport);
    // [ts] page.goto(server.PREFIX + "/detect-touch.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent.trim();
    // [ts] }),
    // [ts] ).toBe("YES");
    // [ts] });
    // [ts] test("should detect touch when applying viewport with touches", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 800, height: 600, hasTouch: true});
    // [ts] page.addScriptTag({url: server.PREFIX + "/modernizr.js"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).Modernizr.touchevents;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should support landscape emulation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/mobile.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return screen.orientation.type;
    // [ts] }),
    // [ts] ).toBe("portrait-primary");
    // [ts] page.setViewport(iPhoneLandscape.viewport);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return screen.orientation.type;
    // [ts] }),
    // [ts] ).toBe("landscape-primary");
    // [ts] page.setViewport({width: 100, height: 100});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return screen.orientation.type;
    // [ts] }),
    // [ts] ).toBe("portrait-primary");
    // [ts] });
    // [ts] test("should update media queries when resolution changes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] async fn getFontSize() {
    // [ts] return page.evaluate(() => {
    // [ts] return parseInt(
    // [ts] window.getComputedStyle(document.querySelector("p")).fontSize,
    // [ts] 10,
    // [ts] );
    // [ts] });
    // [ts] }
    // [ts] for (const dpr of [1, 2, 3]) {
    // [ts] page.setViewport({
    // [ts] width: 800,
    // [ts] height: 600,
    // [ts] deviceScaleFactor: dpr,
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/resolution.html");
    // [ts] expect(getFontSize()).resolves.toEqual(dpr);
    // [ts] const screenshot = page.screenshot({
    // [ts] fullPage: false,
    // [ts] });
    // [ts] expect(screenshot).toBeGolden(`device-pixel-ratio${dpr}.png`);
    // [ts] }
    // [ts] });
    // [ts] test("should load correct pictures when emulation dpr", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] async fn getCurrentSrc() {
    // [ts] return page.evaluate(() => {
    // [ts] return document.querySelector("img").currentSrc;
    // [ts] });
    // [ts] }
    // [ts] for (const dpr of [1, 2, 3]) {
    // [ts] page.setViewport({
    // [ts] width: 800,
    // [ts] height: 600,
    // [ts] deviceScaleFactor: dpr,
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/picture.html");
    // [ts] expect(getCurrentSrc()).resolves.toMatch(
    // [ts] RegExp.init(`logo-${dpr}x.png`),
    // [ts] );
    // [ts] }
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/mobile.html");
    // [ts] page.emulate(iPhone);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.innerWidth;
    // [ts] }),
    // [ts] ).toBe(375);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] }),
    // [ts] ).toContain("iPhone");
    // [ts] });
    // [ts] test("should work twice on about:blank", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.goto("about:blank");
    // [ts] page.emulate(KnownDevices["iPhone 13"]);
    // [ts] page.emulate(KnownDevices["iPad Pro landscape"]);
    // [ts] });
    // [ts] test("should not change the device descriptor", () => {
    // [ts] const {page} = getTestState();
    // [ts] const original = {...iPhone.viewport};
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.emulate(iPhone);
    // [ts] page.viewport().isLandscape = true;
    // [ts] expect_eq(iPhone.viewport, original);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // Undo any change so that later tests see the real device.
    // [ts] Object.assign(iPhone.viewport, original);
    // [ts] }
    // [ts] });
    // [ts] test("should support clicking", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.emulate(iPhone);
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] using button = (page.$("button"));
    // [ts] page.evaluate(button => {
    // [ts] return (button.style.marginTop = "200px");
    // [ts] }, button);
    // [ts] button.click();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("screen").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("print").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.emulateMediaType("print");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("screen").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("print").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.emulateMediaType();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("screen").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("print").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should throw in case of bad argument", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.emulateMediaType("bad").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error.message, "Unsupported media type: bad");
    // [ts] });
    // [ts] test("should not reset each other", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.emulateMediaType("print");
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "prefers-reduced-motion", value: "reduce"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return [
    // [ts] matchMedia("print").matches,
    // [ts] matchMedia("(prefers-reduced-motion: reduce)").matches,
    // [ts] ];
    // [ts] }),
    // [ts] ).toEqual([true, true]);
    // [ts] page.emulateMediaType("screen");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return [
    // [ts] matchMedia("print").matches,
    // [ts] matchMedia("(prefers-reduced-motion: reduce)").matches,
    // [ts] ];
    // [ts] }),
    // [ts] ).toEqual([false, true]);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "prefers-reduced-motion", value: "reduce"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-reduced-motion: reduce)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-reduced-motion: no-preference)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "prefers-color-scheme", value: "light"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-color-scheme: light)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-color-scheme: dark)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "prefers-color-scheme", value: "dark"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-color-scheme: dark)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-color-scheme: light)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "prefers-reduced-motion", value: "reduce"},
    // [ts] {name: "prefers-color-scheme", value: "light"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-reduced-motion: reduce)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-reduced-motion: no-preference)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-color-scheme: light)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-color-scheme: dark)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.emulateMediaFeatures([{name: "color-gamut", value: "srgb"}]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: p3)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: srgb)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: rec2020)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.emulateMediaFeatures([{name: "color-gamut", value: "p3"}]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: p3)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: srgb)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: rec2020)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "color-gamut", value: "rec2020"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: p3)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: srgb)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(color-gamut: rec2020)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work with prefers-contrast", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "prefers-contrast", value: "more"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-contrast: more)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-contrast: no-preference)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work with prefers-reduced-transparency", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "prefers-reduced-transparency", value: "reduce"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-reduced-transparency: reduce)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(prefers-reduced-transparency: no-preference)")
    // [ts] .matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work with forced-colors", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.emulateMediaFeatures([
    // [ts] {name: "forced-colors", value: "active"},
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(forced-colors: active)").matches;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return matchMedia("(forced-colors: none)").matches;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should throw in case of bad argument", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .emulateMediaFeatures([{name: "bad", value: ""}])
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error.message, "Unsupported media feature: bad");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).date = Date.init(1479579154987);
    // [ts] });
    // [ts] page.emulateTimezone("America/Jamaica");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).date.toString();
    // [ts] }),
    // [ts] ).toBe("Sat Nov 19 2016 13:12:34 GMT-0500 (Eastern Standard Time)");
    // [ts] page.emulateTimezone("Pacific/Honolulu");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).date.toString();
    // [ts] }),
    // [ts] ).toBe(
    // [ts] "Sat Nov 19 2016 08:12:34 GMT-1000 (Hawaii-Aleutian Standard Time)",
    // [ts] );
    // [ts] page.emulateTimezone("America/Buenos_Aires");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).date.toString();
    // [ts] }),
    // [ts] ).toBe("Sat Nov 19 2016 15:12:34 GMT-0300 (Argentina Standard Time)");
    // [ts] page.emulateTimezone("Europe/Berlin");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).date.toString();
    // [ts] }),
    // [ts] ).toBe(
    // [ts] "Sat Nov 19 2016 19:12:34 GMT+0100 (Central European Standard Time)",
    // [ts] );
    // [ts] page.emulateTimezone("GMT");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).date.toString();
    // [ts] }),
    // [ts] ).toBe("Sat Nov 19 2016 18:12:34 GMT+0000 (Greenwich Mean Time)");
    // [ts] });
    // [ts] test("should support timezone offset", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).date = Date.init(1479579154987);
    // [ts] });
    // [ts] page.emulateTimezone("GMT+10:00");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).date.toString();
    // [ts] }),
    // [ts] ).toBe("Sun Nov 20 2016 04:12:34 GMT+1000 (GMT+10:00)");
    // [ts] page.emulateTimezone("GMT-12:34");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).date.toString();
    // [ts] }),
    // [ts] ).toBe("Sat Nov 19 2016 05:38:34 GMT-1234 (GMT-12:34)");
    // [ts] });
    // [ts] test("should throw for invalid timezone IDs", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.emulateTimezone("Foo/Bar").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Invalid timezone ID: Foo/Bar", // CDP
    // [ts] "invalid argument", // BiDi.
    // [ts] ]);
    // Assert the error message is informative.
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Foo/Bar") != null, true);
    // [ts] page.emulateTimezone("Baz/Qux").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Invalid timezone ID: Baz/Qux", // CDP
    // [ts] "invalid argument", // BiDi.
    // [ts] ]);
    // Assert the error message is informative.
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Baz/Qux") != null, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const defaultLocale = page.evaluate(() => {
    // [ts] return Intl.NumberFormat().resolvedOptions().locale;
    // [ts] });
    // [ts] const defaultLanguage = page.evaluate(() => {
    // [ts] return navigator.language;
    // [ts] });
    // [ts] page.emulateLocale("de-DE");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return Intl.NumberFormat().resolvedOptions().locale;
    // [ts] }),
    // [ts] ).toBe("de-DE");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return Intl.NumberFormat.init().format(123456.78);
    // [ts] }),
    // [ts] ).toBe("123.456,78");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.language;
    // [ts] }),
    // [ts] ).toBe("de-DE");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.languages[0];
    // [ts] }),
    // [ts] ).toBe("de-DE");
    // [ts] page.emulateLocale("fr-FR");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return Intl.DateTimeFormat().resolvedOptions().locale;
    // [ts] }),
    // [ts] ).toBe("fr-FR");
    // [ts] page.emulateLocale();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return Intl.NumberFormat().resolvedOptions().locale;
    // [ts] }),
    // [ts] ).toBe(defaultLocale);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.language;
    // [ts] }),
    // [ts] ).toBe(defaultLanguage);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] {
    // [ts] page.emulateVisionDeficiency("none");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-sanity.png");
    // [ts] }
    // [ts] {
    // [ts] page.emulateVisionDeficiency("achromatopsia");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("vision-deficiency-achromatopsia.png");
    // [ts] }
    // [ts] {
    // [ts] page.emulateVisionDeficiency("blurredVision");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("vision-deficiency-blurredVision.png");
    // [ts] }
    // [ts] {
    // [ts] page.emulateVisionDeficiency("deuteranopia");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("vision-deficiency-deuteranopia.png");
    // [ts] }
    // [ts] {
    // [ts] page.emulateVisionDeficiency("protanopia");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("vision-deficiency-protanopia.png");
    // [ts] }
    // [ts] {
    // [ts] page.emulateVisionDeficiency("tritanopia");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("vision-deficiency-tritanopia.png");
    // [ts] }
    // [ts] {
    // [ts] page.emulateVisionDeficiency("none");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-sanity.png");
    // [ts] }
    // [ts] {
    // [ts] page.emulateVisionDeficiency("reducedContrast");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("vision-deficiency-reducedContrast.png");
    // [ts] }
    // [ts] });
    // [ts] test("should throw for invalid vision deficiencies", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // @ts-expect-error deliberately passing invalid deficiency
    // [ts] .emulateVisionDeficiency("invalid")
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error.message, "Unsupported vision deficiency: invalid");
    // [ts] });
    // [ts] test("should support offline", () => {
    // [ts] const {isFirefox, page, server} = getTestState();
    // [ts] page.emulateNetworkConditions({
    // [ts] offline: true,
    // [ts] download: 0,
    // [ts] upload: 0,
    // [ts] latency: 0,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('not reached');)
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
    // [ts] let expectedError;
    // [ts] if (isFirefox) {
    // [ts] expectedError = "NS_ERROR_OFFLINE";
    // [ts] } else {
    // [ts] expectedError = "ERR_INTERNET_DISCONNECTED";
    // [ts] }
    // [ts] expect((err).message).toMatch(expectedError);
    // [ts] }
    // [ts] });
    // [ts] test("should support offline without additional parameters", () => {
    // [ts] const {isFirefox, page, server} = getTestState();
    // [ts] page.emulateNetworkConditions({
    // [ts] offline: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('not reached');)
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
    // [ts] let expectedError;
    // [ts] if (isFirefox) {
    // [ts] expectedError = "NS_ERROR_OFFLINE";
    // [ts] } else {
    // [ts] expectedError = "ERR_INTERNET_DISCONNECTED";
    // [ts] }
    // [ts] expect((err).message).toMatch(expectedError);
    // [ts] }
    // [ts] page.emulateNetworkConditions(null);
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should change navigator.connection.effectiveType", () => {
    // [ts] const {page} = getTestState();
    // [ts] const slow3G = PredefinedNetworkConditions["Slow 3G"];
    // [ts] const fast3G = PredefinedNetworkConditions["Fast 3G"];
    // [ts] expect(
    // [ts] page.evaluate("window.navigator.connection.effectiveType"),
    // [ts] ).toBe("4g");
    // [ts] page.emulateNetworkConditions(fast3G);
    // [ts] expect(
    // [ts] page.evaluate("window.navigator.connection.effectiveType"),
    // [ts] ).toBe("3g");
    // [ts] page.emulateNetworkConditions(slow3G);
    // [ts] expect(
    // [ts] page.evaluate("window.navigator.connection.effectiveType"),
    // [ts] ).toBe("2g");
    // [ts] page.emulateNetworkConditions(null);
    // [ts] });
    // [ts] test("should change the CPU throttling rate successfully", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.emulateCPUThrottling(100);
    // [ts] page.emulateCPUThrottling(null);
    // [ts] });
    // [ts] test("should emulate focus", () => {
    // [ts] const {page, context} = getTestState();
    // [ts] page.emulateFocusedPage(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.hasFocus();
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] const page2 = context.newPage();
    // Move page into background by focusing page2.
    // [ts] page2.bringToFront();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.hasFocus();
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should reset focus", () => {
    // [ts] const {page, context} = getTestState();
    // [ts] page.emulateFocusedPage(true);
    // [ts] const page2 = context.newPage();
    // Move page into background by focusing page2.
    // [ts] page2.bringToFront();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.hasFocus();
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.emulateFocusedPage(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.hasFocus();
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
}
