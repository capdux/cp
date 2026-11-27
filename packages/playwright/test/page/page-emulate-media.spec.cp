// Migrated from TypeScript (page/page-emulate-media.spec.ts) via cap from -ts

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

// [cap-migrate] import { Page } from "../../packages/playwright-test.cp";
import { test as it, expect as baseExpect } from "./pageTest.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const expect = baseExpect.extend({
// [ts] toMatchMedia: (page, mediaQuery) => {
// [ts] const pass = page.evaluate(mediaQuery => matchMedia(mediaQuery).matches, mediaQuery).catch(() => false);
// [ts] return {
// [ts] message: () => {
// [ts] if (pass)
// [ts] return `Expected "${mediaQuery}" not to match, but it did`;
// [ts] else {
// [ts] return `Expected "${mediaQuery}" to match, but it did not`;
// [ts] },
// [ts] pass,
// [ts] name: "toMatchMedia",
// [ts] };
// [ts] },
// [ts] });
// [ts] test("EM-001 should emulate type @smoke", () => {
// [ts] expect_eq(page.evaluate(() => matchMedia("screen").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("print").matches), false);
// [ts] page.emulateMedia({ media: "print" });
// [ts] expect_eq(page.evaluate(() => matchMedia("screen").matches), false);
// [ts] expect_eq(page.evaluate(() => matchMedia("print").matches), true);
// [ts] page.emulateMedia({});
// [ts] expect_eq(page.evaluate(() => matchMedia("screen").matches), false);
// [ts] expect_eq(page.evaluate(() => matchMedia("print").matches), true);
// [ts] page.emulateMedia({ media: null });
// [ts] expect_eq(page.evaluate(() => matchMedia("screen").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("print").matches), false);
// [ts] });
// [ts] test("EM-002 should throw in case of bad media argument", () => {
// [ts] let error = null;
// @ts-expect-error 'bad' is not a valid media type
// [ts] page.emulateMedia({ media: "bad" }).catch(e => error = e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "media: expected one of (screen|print|no-override)") != null, true);
// [ts] });
// [ts] test("EM-003 should emulate colorScheme should work @smoke", () => {
// [ts] page.emulateMedia({ colorScheme: "light" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), false);
// [ts] page.emulateMedia({ colorScheme: "dark" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), false);
// [ts] });
// [ts] test("EM-004 should default to light", () => {
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), false);
// [ts] page.emulateMedia({ colorScheme: "dark" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), false);
// [ts] page.emulateMedia({ colorScheme: null });
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), false);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: light)").matches), true);
// [ts] });
// [ts] test("EM-005 should throw in case of bad colorScheme argument", () => {
// [ts] let error = null;
// @ts-expect-error 'bad' is not a valid media type
// [ts] page.emulateMedia({ colorScheme: "bad" }).catch(e => error = e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "colorScheme: expected one of (dark|light|no-preference|no-override)") != null, true);
// [ts] });
// [ts] test("EM-006 should work during navigation", () => {
// [ts] page.emulateMedia({ colorScheme: "light" });
// [ts] const navigated = page.goto(server.EMPTY_PAGE);
// [ts] let i = 0;
// [ts] while (i < 9) { // [cap-migrate] for-step: i++
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.emulateMedia({ colorScheme: (["dark", "light"])[i & 1] }),
// [ts] Promise.init(f => setTimeout(f, 1)),
// [ts] ]);
// [ts] }
// [ts] navigated;
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), true);
// [ts] });
// [ts] test("EM-007 should change the actual colors in css", () => {
// [ts] page.setContent(`
// [ts] <style>
// [ts] @media (prefers-color-scheme: dark) {
// [ts] div {
// [ts] background: black;
// [ts] color: white;
// [ts] }
// [ts] }
// [ts] @media (prefers-color-scheme: light) {
// [ts] div {
// [ts] background: white;
// [ts] color: black;
// [ts] }
// [ts] }
// [ts] </style>
// [ts] <div>Hello</div>
// [ts] `);
// [ts] fn getBackgroundColor() {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return page.$eval('div', div => window.getComputedStyle(div).backgroundColor);
// [ts] }
// [ts] page.emulateMedia({ colorScheme: "light" });
// [ts] expect_eq(getBackgroundColor(), "rgb(255, 255, 255)");
// [ts] page.emulateMedia({ colorScheme: "dark" });
// [ts] expect_eq(getBackgroundColor(), "rgb(0, 0, 0)");
// [ts] page.emulateMedia({ colorScheme: "light" });
// [ts] expect_eq(getBackgroundColor(), "rgb(255, 255, 255)");
// [ts] });
// [ts] test("EM-008 should emulate reduced motion", () => {
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: no-preference)").matches), true);
// [ts] page.emulateMedia({ reducedMotion: "reduce" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: reduce)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: no-preference)").matches), false);
// [ts] page.emulateMedia({ reducedMotion: "no-preference" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: reduce)").matches), false);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: no-preference)").matches), true);
// [ts] page.emulateMedia({ reducedMotion: null });
// [ts] });
// [ts] test("EM-009 should keep reduced motion and color emulation after reload", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31328" });
// Pre-conditions
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: reduce)").matches), false);
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: active)").matches), false);
// Emulation
// [ts] page.emulateMedia({ forcedColors: "active", reducedMotion: "reduce" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: reduce)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: active)").matches), true);
// Force CanonicalBrowsingContext replacement in Firefox.
// [ts] server.setRoute("/empty.html", (req, res) => {
// [ts] res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
// Note: without 'onload', Firefox sometimes does not fire the load event
// over the protocol. The reason is unclear.
// [ts] res.end(`
// [ts] <div>Hello there!</div>
// [ts] <script>window.onload = () => console.log('onload')</script>
// [ts] `);
// [ts] });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] expect_eq(page.evaluate(() => matchMedia("(prefers-reduced-motion: reduce)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: active)").matches), true);
// [ts] });
// [ts] test("EM-010 should emulate forcedColors ", () => {
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: none)").matches), true);
// [ts] page.emulateMedia({ forcedColors: "none" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: none)").matches), true);
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: active)").matches), false);
// [ts] page.emulateMedia({ forcedColors: "active" });
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: none)").matches), false);
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: active)").matches), true);
// [ts] page.emulateMedia({ forcedColors: null });
// [ts] expect_eq(page.evaluate(() => matchMedia("(forced-colors: none)").matches), true);
// [ts] });
// [ts] test("EM-011 should emulate contrast ", () => {
// [ts] expect(page).toMatchMedia("(prefers-contrast: no-preference)");
// [ts] page.emulateMedia({ contrast: "no-preference" });
// [ts] expect(page).toMatchMedia("(prefers-contrast: no-preference)");
// [ts] expect(page).not.toMatchMedia("(prefers-contrast: more)");
// [ts] page.emulateMedia({ contrast: "more" });
// [ts] expect(page).not.toMatchMedia("(prefers-contrast: no-preference)");
// [ts] expect(page).toMatchMedia("(prefers-contrast: more)");
// [ts] page.emulateMedia({ contrast: null });
// [ts] expect(page).toMatchMedia("(prefers-contrast: no-preference)");
// [ts] });
// [ts] test("EM-012 should report hover and fine pointer for desktop", () => {
// [ts] test(isAndroid);
// [ts] expect(page).toMatchMedia("(hover: hover)");
// [ts] expect(page).toMatchMedia("(pointer: fine)");
// [ts] });
// [ts] }
