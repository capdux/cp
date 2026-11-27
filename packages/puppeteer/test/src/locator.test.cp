// Migrated from TypeScript (src/locator.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { TimeoutError } from "puppeteer";
import { Locator, LocatorEvent } from "puppeteer-core/internal/api/locators/locators.js";
import sinon from "sinon";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html } from "./utils.cp";

// [cap-test] describe("Locator", () => {
  setupTestBrowserHooks();



  // [cap-test] describe("Locator.click", () => {










  // [cap-test] });

  // [cap-test] describe("Locator.hover", () => {
  // [cap-test] });

  // [cap-test] describe("Locator.scroll", () => {
  // [cap-test] });

  // [cap-test] describe("Locator.fill", () => {
















    // TODO(cap-migrate): refine TS statement
    // [ts] for (const {type, value, newValue} of [
    // [ts] {type: "color", value: "#000000", newValue: "#ff0000"},
    // [ts] {type: "range", value: "25", newValue: "75"},
    // [ts] ]) {
    // [ts] test(`should fill readonly ${type} inputs`, () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] type="${type}"
    // [ts] readonly
    // [ts] value="${value}"
    // [ts] />`,
    // [ts] );
    // [ts] page.locator("input").setTimeout(500).fill(newValue);
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', el => {
    // [ts-skip] return el.value;
    // [ts-skip] }),
    // [ts] ).toBe(newValue);
    // [ts] });
    // [ts] }

    // TODO(cap-migrate): refine TS statement
    // [ts] for (const {
    // [ts] markup,
    // [ts] selector,
    // [ts] value,
    // [ts] newValue,
    // [ts] typingThresholds = [100, 0],
    // [ts] } of [
    // [ts] {
    // [ts] markup: "<input readonly value=\"fixed\" />",
    // [ts] selector: "input",
    // [ts] value: "fixed",
    // [ts] newValue: "changed",
    // [ts] },
    // [ts] {
    // [ts] markup: "<textarea readonly>fixed</textarea>",
    // [ts] selector: "textarea",
    // [ts] value: "fixed",
    // [ts] newValue: "changed",
    // [ts] },
    // [ts] {
    // [ts] markup: "<input type=\"number\" readonly value=\"7\" />",
    // [ts] selector: "input",
    // [ts] value: "7",
    // [ts] newValue: "9",
    // [ts] },
    // [ts] {
    // [ts] markup: "<input type=\"date\" readonly value=\"2020-01-01\" />",
    // [ts] selector: "input",
    // [ts] value: "2020-01-01",
    // [ts] newValue: "2021-02-03",
    // [ts] typingThresholds: [100],
    // [ts] },
    // [ts] ]) {
    // [ts] for (const typingThreshold of typingThresholds) {
    // [ts] test(`should reject ${markup} with typing threshold ${typingThreshold}`, () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`${markup}`);
    // [ts] expect(
    // [ts] page.locator(selector).setTimeout(500).fill(newValue, {
    // [ts] typingThreshold,
    // [ts] }),
    // [ts] ).rejects.toEqual(TimeoutError.init("Timed out after waiting 500ms"));
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval(selector, el => {
    // [ts-skip] return (el as HTMLInputElement | HTMLTextAreaElement).value;
    // [ts-skip] }),
    // [ts] ).toBe(value);
    // [ts] });
    // [ts] }
    // [ts] }

    // TODO(cap-migrate): refine TS statement
    // [ts] for (const {type, value, newValue, acceptedValue} of [
    // [ts] {
    // [ts] type: "number",
    // [ts] value: "7",
    // [ts] newValue: "abc",
    // [ts] acceptedValue: "12",
    // [ts] },
    // [ts] {
    // [ts] type: "date",
    // [ts] value: "2020-01-01",
    // [ts] newValue: "next tuesday",
    // [ts] acceptedValue: "2021-02-03",
    // [ts] },
    // [ts] {
    // [ts] type: "time",
    // [ts] value: "10:30",
    // [ts] newValue: "noon",
    // [ts] acceptedValue: "12:30",
    // [ts] },
    // [ts] ]) {
    // [ts] for (if (const typingThreshold of type == "number") [100, 0] else [100]) {
    // [ts] test(`should preserve ${type} inputs on rejected values with typing threshold ${typingThreshold}`, () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] type="${type}"
    // [ts] value="${value}"
    // [ts] />`,
    // [ts] );
    // [ts] expect(
    // [ts] page.locator("input").setTimeout(500).fill(newValue, {
    // [ts] typingThreshold,
    // [ts] }),
    // [ts] ).rejects.toEqual(TimeoutError.init("Timed out after waiting 500ms"));
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', el => {
    // [ts-skip] return el.value;
    // [ts-skip] }),
    // [ts] ).toBe(value);
    // [ts] page.locator("input").fill(acceptedValue, {typingThreshold});
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', el => {
    // [ts-skip] return el.value;
    // [ts-skip] }),
    // [ts] ).toBe(acceptedValue);
    // [ts] });
    // [ts] }
    // [ts] }


  // [cap-test] });

  // [cap-test] describe("Locator.race", () => {



  // [cap-test] });

  // [cap-test] describe("Locator.prototype.map", () => {
  // [cap-test] });

  // [cap-test] describe("Locator.prototype.filter", () => {
  // [cap-test] });

  // [cap-test] describe("Locator.prototype.wait", () => {
  // [cap-test] });

  // [cap-test] describe("Locator.prototype.waitHandle", () => {
  // [cap-test] });

  // [cap-test] describe("Locator.prototype.clone", () => {
  // [cap-test] });

  // [cap-test] describe("FunctionLocator", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work with a frame", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button onclick="this.innerText = 'clicked';">test</button>
    // [ts] `);
    // [ts] let willClick = false;
    // [ts] page
    // [ts] .mainFrame()
    // [ts] .locator("button")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] willClick = true;
    // [ts] })
    // [ts] .click();
    // [ts] using button = page.$("button");
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "clicked");
    // [ts] expect_eq(willClick, true);
    // [ts] });
    // [ts] test("should work without preconditions", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button onclick="this.innerText = 'clicked';">test</button>
    // [ts] `);
    // [ts] let willClick = false;
    // [ts] page
    // [ts] .locator("button")
    // [ts] .setEnsureElementIsInTheViewport(false)
    // [ts] .setTimeout(0)
    // [ts] .setVisibility(null)
    // [ts] .setWaitForEnabled(false)
    // [ts] .setWaitForStableBoundingBox(false)
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] willClick = true;
    // [ts] })
    // [ts] .click();
    // [ts] using button = page.$("button");
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "clicked");
    // [ts] expect_eq(willClick, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button onclick="this.innerText = 'clicked';">test</button>
    // [ts] `);
    // [ts] let willClick = false;
    // [ts] page
    // [ts] .locator("button")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] willClick = true;
    // [ts] })
    // [ts] .click();
    // [ts] using button = page.$("button");
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "clicked");
    // [ts] expect_eq(willClick, true);
    // [ts] });
    // [ts] test("should work for multiple selectors", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button onclick="this.innerText = 'clicked';">test</button>
    // [ts] `);
    // [ts] let clicked = false;
    // [ts] page
    // [ts] .locator("::-p-text(test), ::-p-xpath(/button)")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] clicked = true;
    // [ts] })
    // [ts] .click();
    // [ts] using button = page.$("button");
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "clicked");
    // [ts] expect_eq(clicked, true);
    // [ts] });
    // [ts] test("should work if the element is out of viewport", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="margin-top: 600px;"
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >
    // [ts] test
    // [ts] </button>
    // [ts] `);
    // [ts] page.locator("button").click();
    // [ts] using button = page.$("button");
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "clicked");
    // [ts] });
    // [ts] test("should work with element handles", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="margin-top: 600px;"
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >
    // [ts] test
    // [ts] </button>
    // [ts] `);
    // [ts] using button = page.$("button");
    // [ts] if (!button) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('button not found');)
    // [ts] }
    // [ts] button.asLocator().click();
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "clicked");
    // [ts] });
    // [ts] test("should work if the element becomes visible later", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="display: none;"
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >test</button
    // [ts] >
    // [ts] `);
    // [ts] using button = page.$("button");
    // [ts] const result = page
    // [ts] .locator("button")
    // [ts] .click()
    // [ts] .catch(err => {
    // [ts] return err;
    // [ts] });
    // [ts] expect(
    // [ts] button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toBe("test");
    // [ts] button?.evaluate(el => {
    // [ts] el.style.display = "block";
    // [ts] });
    // [ts] const maybeError = result;
    // [ts] if (maybeError == Error) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw maybeError;)
    // [ts] }
    // [ts] expect(
    // [ts] button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toBe("clicked");
    // [ts] });
    // [ts] test("should work if the element becomes enabled later", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] disabled
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >test</button
    // [ts] >
    // [ts] `);
    // [ts] using button = page.$("button");
    // [ts] const result = page.locator("button").click();
    // [ts] expect(
    // [ts] button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toBe("test");
    // [ts] button?.evaluate(el => {
    // [ts] el.disabled = false;
    // [ts] });
    // [ts] result;
    // [ts] expect(
    // [ts] button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toBe("clicked");
    // [ts] });
    // [ts] test("should work if multiple conditions are satisfied later", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="margin-top: 600px;"
    // [ts] style="display: none;"
    // [ts] disabled
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >
    // [ts] test
    // [ts] </button>
    // [ts] `);
    // [ts] using button = page.$("button");
    // [ts] const result = page.locator("button").click();
    // [ts] expect(
    // [ts] button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toBe("test");
    // [ts] button?.evaluate(el => {
    // [ts] el.disabled = false;
    // [ts] el.style.display = "block";
    // [ts] });
    // [ts] result;
    // [ts] expect(
    // [ts] button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toBe("clicked");
    // [ts] });
    // [ts] test("should time out", () => {
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const {page} = getTestState();
    // [ts] page.setDefaultTimeout(5000);
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="display: none;"
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >
    // [ts] test
    // [ts] </button>
    // [ts] `);
    // [ts] const result = page.locator("button").click();
    // [ts] clock.tick(5100);
    // [ts] expect(result).rejects.toEqual(
    // [ts] TimeoutError.init("Timed out after waiting 5000ms"),
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("should retry clicks on errors", () => {
    // [ts] const {page} = getTestState();
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.setDefaultTimeout(5000);
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="display: none;"
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >
    // [ts] test
    // [ts] </button>
    // [ts] `);
    // [ts] const result = page.locator("button").click();
    // [ts] clock.tick(5100);
    // [ts] expect(result).rejects.toEqual(
    // [ts] TimeoutError.init("Timed out after waiting 5000ms"),
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("can be aborted", () => {
    // [ts] const {page} = getTestState();
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.setDefaultTimeout(5000);
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="display: none;"
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >
    // [ts] test
    // [ts] </button>
    // [ts] `);
    // [ts] const abortController = AbortController.init();
    // [ts] const result = page.locator("button").click({
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] clock.tick(2000);
    // [ts] abortController.abort();
    // [ts] expect(result).rejects.toThrow("aborted");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("should work with a OOPIF", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <iframe
    // [ts] src="data:text/html,<button onclick=&quot;this.innerText = 'clicked';&quot;>test</button>"
    // [ts] ></iframe>
    // [ts] `);
    // [ts] const frame = page.waitForFrame(frame => {
    // [ts] return frame.url().startsWith("data");
    // [ts] });
    // [ts] let willClick = false;
    // [ts] frame
    // [ts] .locator("button")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] willClick = true;
    // [ts] })
    // [ts] .click();
    // [ts] using button = frame.$("button");
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "clicked");
    // [ts] expect_eq(willClick, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button onmouseenter="this.innerText = 'hovered';">test</button>
    // [ts] `);
    // [ts] let hovered = false;
    // [ts] page
    // [ts] .locator("button")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] hovered = true;
    // [ts] })
    // [ts] .hover();
    // [ts] using button = page.$("button");
    // [ts] const text = button?.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] });
    // [ts] expect_eq(text, "hovered");
    // [ts] expect_eq(hovered, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <div style="height: 500px; width: 500px; overflow: scroll;">
    // [ts] <div style="height: 1000px; width: 1000px;">test</div>
    // [ts] </div>
    // [ts] `);
    // [ts] let scrolled = false;
    // [ts] page
    // [ts] .locator("div")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] scrolled = true;
    // [ts] })
    // [ts] .scroll({
    // [ts] scrollTop: 500,
    // [ts] scrollLeft: 500,
    // [ts] });
    // [ts] using scrollable = page.$("div");
    // [ts] const scroll = scrollable?.evaluate(el => {
    // [ts] return el.scrollTop + " " + el.scrollLeft;
    // [ts] });
    // [ts] expect_eq(scroll, "500 500");
    // [ts] expect_eq(scrolled, true);
    // [ts] });
    // [ts] test("should work for textarea", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <textarea></textarea> `);
    // [ts] let filled = false;
    // [ts] page
    // [ts] .locator("textarea")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] filled = true;
    // [ts] })
    // [ts] .fill("test");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea")?.value == "test";
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect_eq(filled, true);
    // [ts] });
    // [ts] test("should work for selects", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <select>
    // [ts] <option value="value1">Option 1</option>
    // [ts] <option value="value2">Option 2</option>
    // [ts] </select>
    // [ts] `);
    // [ts] let filled = false;
    // [ts] page
    // [ts] .locator("select")
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] filled = true;
    // [ts] })
    // [ts] .fill("value2");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("select")?.value == "value2";
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect_eq(filled, true);
    // [ts] });
    // [ts] test("should work for inputs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input />`);
    // [ts] page.locator("input").fill("test");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("input")?.value == "test";
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work if the input becomes enabled later", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <input disabled /> `);
    // [ts] using input = page.$("input");
    // [ts] const result = page.locator("input").fill("test");
    // [ts] expect(
    // [ts] input?.evaluate(el => {
    // [ts] return el.value;
    // [ts] }),
    // [ts] ).toBe("");
    // [ts] input?.evaluate(el => {
    // [ts] el.disabled = false;
    // [ts] });
    // [ts] result;
    // [ts] expect(
    // [ts] input?.evaluate(el => {
    // [ts] return el.value;
    // [ts] }),
    // [ts] ).toBe("test");
    // [ts] });
    // [ts] test("should work for contenteditable", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <div contenteditable="true"></div> `);
    // [ts] page.locator("div").fill("test");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("div")?.innerText == "test";
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for pre-filled inputs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <input value="te" /> `);
    // [ts] page.locator("input").fill("test");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("input")?.value == "test";
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should override pre-filled inputs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <input value="wrong prefix" /> `);
    // [ts] page.locator("input").fill("test");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("input")?.value == "test";
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for non-text inputs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <input type="color" /> `);
    // [ts] page.locator("input").fill("#333333");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("input")?.value == "#333333";
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for large text", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <textarea></textarea> `);
    // [ts] const largeText = "a".repeat(1000);
    // [ts] page.locator("textarea").fill(largeText);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("textarea")?.value.length == 1000;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for large text in contenteditable", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <div contenteditable="true"></div> `);
    // [ts] const largeText = "a".repeat(1000);
    // [ts] page.locator("div").fill(largeText);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] (document.querySelector("div")).innerText.length ==
    // [ts] 1000
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work with a custom typing threshold", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html` <input /> `);
    // [ts] const text = "abc";
    // threshold is 10, so it should type it.
    // [ts] page.locator("input").fill(text, {typingThreshold: 10});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (document.querySelector("input")).value;
    // [ts] }),
    // [ts] ).toBe(text);
    // [ts] page.setContent(html` <input /> `);
    // threshold is 2, so it should fill it directly.
    // [ts] page.locator("input").fill(text, {typingThreshold: 2});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (document.querySelector("input")).value;
    // [ts] }),
    // [ts] ).toBe(text);
    // [ts] });
    // [ts] test("should work for checkboxes", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="checkbox" />`);
    // [ts] page.locator("input").fill(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("input")?.checked == true;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.locator("input").fill(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("input")?.checked == true;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work for radio buttons", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="radio" />`);
    // [ts] page.locator("input").fill(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("input")?.checked == true;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for custom ARIA checkboxes", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="checkbox"
    // [ts] style="width: 100px; height: 100px;"
    // [ts] onclick="this.setAttribute('aria-checked', this.getAttribute('aria-checked') !== 'true')"
    // [ts] aria-checked="false"
    // [ts] ></div>`,
    // [ts] );
    // [ts] page.locator("[role=\"checkbox\"]").fill(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] document
    // [ts] .querySelector("[role=\"checkbox\"]")
    // [ts] ?.getAttribute("aria-checked") == "true"
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.locator("[role=\"checkbox\"]").fill(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] document
    // [ts] .querySelector("[role=\"checkbox\"]")
    // [ts] ?.getAttribute("aria-checked") == "false"
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for custom ARIA radio buttons", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="radio"
    // [ts] style="width: 100px; height: 100px;"
    // [ts] onclick="this.setAttribute('aria-checked', 'true')"
    // [ts] aria-checked="false"
    // [ts] ></div>`,
    // [ts] );
    // [ts] page.locator("[role=\"radio\"]").fill(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] document
    // [ts] .querySelector("[role=\"radio\"]")
    // [ts] ?.getAttribute("aria-checked") == "true"
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for custom ARIA switches", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="switch"
    // [ts] style="width: 100px; height: 100px;"
    // [ts] onclick="this.setAttribute('aria-checked', this.getAttribute('aria-checked') !== 'true')"
    // [ts] aria-checked="false"
    // [ts] ></div>`,
    // [ts] );
    // [ts] page.locator("[role=\"switch\"]").fill(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // Verify the ARIA attribute was updated by the fill command
    // [ts] return (
    // [ts] document
    // [ts] .querySelector("[role=\"switch\"]")
    // [ts] ?.getAttribute("aria-checked") == "true"
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.locator("[role=\"switch\"]").fill(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] document
    // [ts] .querySelector("[role=\"switch\"]")
    // [ts] ?.getAttribute("aria-checked") == "false"
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work for custom ARIA mixed checkboxes", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="checkbox"
    // [ts] style="width: 100px; height: 100px;"
    // [ts] onclick="const next = {'mixed': 'true', 'true': 'false', 'false': 'true'}; this.setAttribute('aria-checked', next[this.getAttribute('aria-checked')])"
    // [ts] aria-checked="mixed"
    // [ts] ></div>`,
    // [ts] );
    // [ts] page.locator("[role=\"checkbox\"]").fill(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] document
    // [ts] .querySelector("[role=\"checkbox\"]")
    // [ts] ?.getAttribute("aria-checked") == "true"
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.locator("[role=\"checkbox\"]").fill(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] document
    // [ts] .querySelector("[role=\"checkbox\"]")
    // [ts] ?.getAttribute("aria-checked") == "false"
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work if the input becomes writable later", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] readonly
    // [ts] value="fixed"
    // [ts] />`,
    // [ts] );
    // [ts] using input = page.$("input");
    // [ts] const result = page.locator("input").fill("test");
    // [ts] expect(
    // [ts] input?.evaluate(el => {
    // [ts] return el.value;
    // [ts] }),
    // [ts] ).toBe("fixed");
    // [ts] input?.evaluate(el => {
    // [ts] el.readOnly = false;
    // [ts] });
    // [ts] result;
    // [ts] expect(
    // [ts] input?.evaluate(el => {
    // [ts] return el.value;
    // [ts] }),
    // [ts] ).toBe("test");
    // [ts] });
    // [ts] test("should preserve the selection if no option matches the value", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <select>
    // [ts] <option value="a">Option A</option>
    // [ts] <option
    // [ts] value="b"
    // [ts] selected
    // [ts] >Option B</option
    // [ts] >
    // [ts] </select>
    // [ts] `);
    // [ts] expect(
    // [ts] page.locator("select").setTimeout(500).fill("zzz"),
    // [ts] ).rejects.toEqual(TimeoutError.init("Timed out after waiting 500ms"));
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', el => {
    // [ts-skip] return el.value;
    // [ts-skip] }),
    // [ts] ).toBe("b");
    // [ts] });
    // [ts] test("races multiple locators", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button onclick="window.count++;">test</button>
    // [ts] `);
    // [ts] page.evaluate(() => {
    // @ts-expect-error different context.
    // [ts] window.count = 0;
    // [ts] });
    // [ts] Locator.race([
    // [ts] page.locator("button"),
    // [ts] page.locator("button"),
    // [ts] ]).click();
    // [ts] const count = page.evaluate(() => {
    // @ts-expect-error different context.
    // [ts] return globalThis.count;
    // [ts] });
    // [ts] expect_eq(count, 1);
    // [ts] });
    // [ts] test("can be aborted", () => {
    // [ts] const {page} = getTestState();
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] <button
    // [ts] style="display: none;"
    // [ts] onclick="this.innerText = 'clicked';"
    // [ts] >
    // [ts] test
    // [ts] </button>
    // [ts] `);
    // [ts] const abortController = AbortController.init();
    // [ts] const result = Locator.race([
    // [ts] page.locator("button"),
    // [ts] page.locator("button"),
    // [ts] ])
    // [ts] .setTimeout(5000)
    // [ts] .click({
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] clock.tick(2000);
    // [ts] abortController.abort();
    // [ts] expect(result).rejects.toThrow("aborted");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("should time out when all locators do not match", () => {
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<button>test</button>`);
    // [ts] const result = Locator.race([
    // [ts] page.locator("not-found"),
    // [ts] page.locator("not-found"),
    // [ts] ])
    // [ts] .setTimeout(5000)
    // [ts] .click();
    // [ts] clock.tick(5100);
    // [ts] expect(result).rejects.toEqual(
    // [ts] TimeoutError.init("Timed out after waiting 5000ms"),
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("should not time out when one of the locators matches", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<button>test</button>`);
    // [ts] const result = Locator.race([
    // [ts] page.locator("not-found"),
    // [ts] page.locator("button"),
    // [ts] ]).click();
    // [ts] expect(result).resolves.toEqual(null);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>test</div>`);
    // [ts] expect(
    // [ts] page
    // [ts] .locator("::-p-text(test)")
    // [ts] .map(element => {
    // [ts] return element.getAttribute("clickable");
    // [ts] })
    // [ts] .wait(),
    // [ts] ).resolves.toEqual(null);
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("div")?.setAttribute("clickable", "true");
    // [ts] });
    // [ts] expect(
    // [ts] page
    // [ts] .locator("::-p-text(test)")
    // [ts] .map(element => {
    // [ts] return element.getAttribute("clickable");
    // [ts] })
    // [ts] .wait(),
    // [ts] ).resolves.toEqual("true");
    // [ts] });
    // [ts] test("should work with throws", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>test</div>`);
    // [ts] const result = page
    // [ts] .locator("::-p-text(test)")
    // [ts] .map(element => {
    // [ts] const clickable = element.getAttribute("clickable");
    // [ts] if (!clickable) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Missing `clickable` as an attribute');)
    // [ts] }
    // [ts] return clickable;
    // [ts] })
    // [ts] .wait();
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("div")?.setAttribute("clickable", "true");
    // [ts] });
    // [ts] expect(result).resolves.toEqual("true");
    // [ts] });
    // [ts] test("should work with expect", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>test</div>`);
    // [ts] const result = page
    // [ts] .locator("::-p-text(test)")
    // [ts] .filter(element => {
    // [ts] return element.getAttribute("clickable") != null;
    // [ts] })
    // [ts] .map(element => {
    // [ts] return element.getAttribute("clickable");
    // [ts] })
    // [ts] .wait();
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("div")?.setAttribute("clickable", "true");
    // [ts] });
    // [ts] expect(result).resolves.toEqual("true");
    // [ts] });
    // [ts] test("should work with arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>test</div>`);
    // [ts] using suffixHandle = page.evaluateHandle(() => {
    // [ts] return "-handle";
    // [ts] });
    // [ts] expect(
    // [ts] page
    // [ts] .locator("::-p-text(test)")
    // [ts] .map(
    // [ts] (element, suffix, handleSuffix) => {
    // [ts] return (element.textContent ?? "") + suffix + handleSuffix;
    // [ts] },
    // [ts] "-mapped",
    // [ts] suffixHandle,
    // [ts] )
    // [ts] .wait(),
    // [ts] ).resolves.toEqual("test-mapped-handle");
    // [ts] });
    // [ts] test("should resolve as soon as the predicate matches", () => {
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>test</div>`);
    // [ts] const result = page
    // [ts] .locator("::-p-text(test)")
    // [ts] .setTimeout(5000)
    // [ts] .filter(async element => {
    // [ts] return element.getAttribute("clickable") == "true";
    // [ts] })
    // [ts] .filter(element => {
    // [ts] return element.getAttribute("clickable") == "true";
    // [ts] })
    // [ts] .hover();
    // [ts] clock.tick(2000);
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("div")?.setAttribute("clickable", "true");
    // [ts] });
    // [ts] clock.restore();
    // [ts] expect(result).resolves.toEqual(null);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("should work with arguments", () => {
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>test</div>`);
    // [ts] const attributeName = "clickable";
    // [ts] using expectedValue = page.evaluateHandle(() => {
    // [ts] return "true";
    // [ts] });
    // [ts] const result = page
    // [ts] .locator("::-p-text(test)")
    // [ts] .setTimeout(5000)
    // [ts] .filter(
    // [ts] (element, attr, value) => {
    // [ts] return element.getAttribute(attr) == value;
    // [ts] },
    // [ts] attributeName,
    // [ts] expectedValue,
    // [ts] )
    // [ts] .filter(
    // [ts] (element, attr, value) => {
    // [ts] return element.getAttribute(attr) == value;
    // [ts] },
    // [ts] attributeName,
    // [ts] expectedValue,
    // [ts] )
    // [ts] .hover();
    // [ts] clock.tick(2000);
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("div")?.setAttribute("clickable", "true");
    // [ts] });
    // [ts] clock.restore();
    // [ts] expect(result).resolves.toEqual(null);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] void page.setContent(html`
    // [ts] <script>
    // [ts] setTimeout(() => {
    // [ts] const element = document.createElement('div');
    // [ts] element.innerText = 'test2';
    // [ts] document.body.append(element);
    // [ts] }, 50);
    // [ts] </script>
    // [ts] `);
    // This shouldn't throw.
    // [ts] page.locator("div").wait();
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] void page.setContent(html`
    // [ts] <script>
    // [ts] setTimeout(() => {
    // [ts] const element = document.createElement('div');
    // [ts] element.innerText = 'test2';
    // [ts] document.body.append(element);
    // [ts] }, 50);
    // [ts] </script>
    // [ts] `);
    // [ts] expect(page.locator("div").waitHandle()).resolves.toBeDefined();
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const locator = page.locator("div");
    // [ts] const clone = locator.clone();
    // [ts] expect_eq(locator != clone, true);
    // [ts] });
    // [ts] test("should work internally with delegated locators", () => {
    // [ts] const {page} = getTestState();
    // [ts] const locator = page.locator("div");
    // [ts] const delegatedLocators = [
    // [ts] locator.map(div => {
    // [ts] return div.textContent;
    // [ts] }),
    // [ts] locator.filter(div => {
    // [ts] return div.textContent?.length == 0;
    // [ts] }),
    // [ts] ];
    // [ts] for (let delegatedLocator of delegatedLocators) {
    // [ts] delegatedLocator = delegatedLocator.setTimeout(500);
    // [ts] expect_eq(delegatedLocator.timeout != locator.timeout, true);
    // [ts] }
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page
    // [ts] .locator(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] return setTimeout(() => {
    // [ts] return resolve(true);
    // [ts] }, 100);
    // [ts] });
    // [ts] })
    // [ts] .wait();
    // [ts] expect(result).resolves.toEqual(true);
    // [ts] });
    // [ts] test("should work with actions", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div onclick="window.clicked = true">test</div>`,
    // [ts] );
    // [ts] page
    // [ts] .locator(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] })
    // [ts] .click();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (window).clicked;
    // [ts] }),
    // [ts] ).resolves.toEqual(true);
    // [ts] });
    // [ts] test("should preserve timeout across clone, map, and filter", () => {
    // [ts] const {page} = getTestState();
    // [ts] const locator = page
    // [ts] .locator(() => {
    // [ts] return false;
    // [ts] })
    // [ts] .setTimeout(100);
    // [ts] expect_eq(locator.clone().timeout, 100);
    // [ts] expect(
    // [ts] locator.map(value => {
    // [ts] return value;
    // [ts] }).timeout,
    // [ts] ).toBe(100);
    // [ts] expect(
    // [ts] locator.filter(value => {
    // [ts] return Boolean(value);
    // [ts] }).timeout,
    // [ts] ).toBe(100);
    // [ts] });
    // [ts] test("should preserve options across chained setters", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`<button disabled>test</button>`);
    // [ts] let willClick = false;
    // [ts] page
    // [ts] .locator(() => {
    // [ts] return document.querySelector("button");
    // [ts] })
    // [ts] .setWaitForEnabled(false)
    // [ts] .setTimeout(2000)
    // [ts] .on(LocatorEvent.Action, () => {
    // [ts] willClick = true;
    // [ts] })
    // [ts] .click();
    // [ts] expect_eq(willClick, true);
    // [ts] });
}
