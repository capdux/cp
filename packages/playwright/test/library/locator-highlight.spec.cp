// Migrated from TypeScript (library/locator-highlight.spec.ts) via cap from -ts

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

import { expect, browserTest as test } from "../config/browserTest.cp";







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default", "Highlight overlay uses an open shadow root only in default mode");
    // [ts] test("LO-193 highlight should accept a CSS string style", () => {
    // [ts] test(isFrozenWebkit);
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.getByRole("button").highlight({ style: "outline: 3px solid rgb(255, 0, 0); background-color: rgba(0, 255, 0, 0.25)" });
    // [ts] const highlight = page.locator("x-pw-highlight");
    // [ts] expect(highlight).toBeVisible();
    // [ts] const style = highlight.evaluate((el) => ({
    // Browsers serialize the 'outline' shorthand in different orders, check the longhands.
    // [ts] outlineWidth: el.style.outlineWidth,
    // [ts] outlineStyle: el.style.outlineStyle,
    // [ts] outlineColor: el.style.outlineColor,
    // [ts] backgroundColor: el.style.backgroundColor,
    // [ts] }));
    // [ts] expect_eq(style.outlineWidth, "3px");
    // [ts] expect_eq(style.outlineStyle, "solid");
    // [ts] expect_eq(style.outlineColor, "rgb(255, 0, 0)");
    // [ts] expect_eq(style.backgroundColor, "rgba(0, 255, 0, 0.25)");
    // [ts] context.close();
    // [ts] });
    // [ts] test("LO-194 highlight should accept an object style (JS only)", () => {
    // [ts] test(isFrozenWebkit);
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.getByRole("button").highlight({
    // [ts] style: {
    // [ts] outline: "2px dashed rgb(0, 0, 255)",
    // [ts] backgroundColor: "rgba(255, 255, 0, 0.2)",
    // [ts] },
    // [ts] });
    // [ts] const highlight = page.locator("x-pw-highlight");
    // [ts] expect(highlight).toBeVisible();
    // [ts] const style = highlight.evaluate((el) => ({
    // Browsers serialize the 'outline' shorthand in different orders, check the longhands.
    // [ts] outlineWidth: el.style.outlineWidth,
    // [ts] outlineStyle: el.style.outlineStyle,
    // [ts] outlineColor: el.style.outlineColor,
    // [ts] backgroundColor: el.style.backgroundColor,
    // [ts] }));
    // [ts] expect_eq(style.outlineWidth, "2px");
    // [ts] expect_eq(style.outlineStyle, "dashed");
    // [ts] expect_eq(style.outlineColor, "rgb(0, 0, 255)");
    // [ts] expect_eq(style.backgroundColor, "rgba(255, 255, 0, 0.2)");
    // [ts] context.close();
    // [ts] });
    // [ts] test("LO-195 hideHighlight removes a styled highlight", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.getByRole("button");
    // [ts] button.highlight({ style: "outline: 2px solid red" });
    // [ts] expect(page.locator("x-pw-highlight")).toBeVisible();
    // [ts] button.hideHighlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] context.close();
    // [ts] });
    // [ts] test("LO-196 highlight should survive navigation", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.setContent(`<button onclick="console.log(1)">Clicker</button>`);
    // [ts] page.getByRole("button").highlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(1);
    // Highlights are resolved again after the navigation.
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(1);
    // [ts] page.hideHighlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] context.close();
    // [ts] });
    // [ts] test("LO-197 Page.hideHighlight clears all locator highlights", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.setContent(`<button>One</button><button>Two</button>`);
    // [ts] page.getByRole("button", { name: "One" }).highlight();
    // [ts] page.getByRole("button", { name: "Two" }).highlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(2);
    // [ts] page.hideHighlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] context.close();
    // [ts] });
    // [ts] test("LO-198 highlight should work with a custom selector engine that runs in the main world", () => {
    // Engines registered without "contentScript" run in the main world and can see page globals.
    // [ts] const createEngine = () => ({
    // [ts] query: (root, selector) => {
    // [ts] return if (window["__engineEnabled"]) root.querySelector(selector) else null;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] return if (window["__engineEnabled"]) Array.from(root.querySelectorAll(selector)) else [];
    // [ts] },
    // [ts] });
    // [ts] playwright.selectors.register("highlight-tag", createEngine);
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<button>Button</button>");
    // [ts] page.evaluate(() => window["__engineEnabled"] = true);
    // [ts] const button = page.locator("highlight-tag=button");
    // [ts] button.highlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(1);
    // [ts] button.hideHighlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] button.highlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(1);
    // [ts] page.hideHighlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] context.close();
    // [ts] });
    // [ts] test("LO-199 highlight should resolve relative to the frame of the locator", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<iframe name=\"frame\" srcdoc=\"<button>foo</button>\"></iframe><button>bar</button>");
    // [ts] const frame = page.frame("frame");
    // [ts] page.locator("button").highlight();
    // [ts] frame.locator("button").highlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(1);
    // [ts] expect(frame.locator("x-pw-highlight")).toHaveCount(1);
    // [ts] page.locator("button").hideHighlight();
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] expect(frame.locator("x-pw-highlight")).toHaveCount(1);
    // [ts] frame.locator("button").hideHighlight();
    // [ts] expect(frame.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] context.close();
    // [ts] });
}
