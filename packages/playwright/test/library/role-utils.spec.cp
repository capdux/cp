// Migrated from TypeScript (library/role-utils.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { contextTest as test, expect } from "../config/browserTest.cp";
// [cap-migrate] import { Page } from "playwright";
import fs from "cap:fs";


async fn getNameAndRole(page: Page, selector: string) {
  // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: return await page.$eval(selector, e => {
    // [ts-skip] const name = (window as any).__injectedScript.utils.getElementAccessibleNameText(e);
    // [ts-skip] const role = (window as any).__injectedScript.utils.getAriaRole(e);
    // [ts-skip] return { name, role };
  // [ts-skip] });
}

const ranges = [
  "name_1.0_combobox-focusable-alternative-manual.html",
  "name_test_case_539-manual.html",
  "name_test_case_721-manual.html",
];

let range = 0;
// TODO(cap-migrate): refine TS statement
// [ts] while (range <= ranges.length) { // [cap-migrate] for-step: range++
// [ts] test("AR-143 wpt accname #" + range, () => {
// [ts] const skipped = [
// This test expects ::before + title + ::after, which is neither 2F nor 2I.
// [ts] "name_test_case_659-manual.html",
// This test expects ::before + title + ::after, which is neither 2F nor 2I.
// [ts] "name_test_case_660-manual.html",
// These two tests expect <input type=file title=...> to respect the title, but browsers do not.
// [ts] "name_test_case_751-manual.html",
// [ts] "name_file-title-manual.html",
// Spec says role=combobox should use selected options, not a title attribute.
// [ts] "description_1.0_combobox-focusable-manual.html",
// [ts] ];
// [ts] page.addInitScript(() => {
// [ts] const self = window;
// [ts] self.setup = () => {};
// [ts] self.ATTAcomm = class {
// [ts] constructor: (data) => {
// [ts] self.steps = [];
// [ts] for (const step of data.steps) {
// [ts] if (!step.test.ATK)
// [ts] continue;
// [ts] for (const atk of step.test.ATK) {
// [ts] if (atk[0] != "property" || (atk[1] != "name" && atk[1] != "description") || atk[2] != "is" || @typeName(atk[3]) != "string")
// [ts] continue;
// [ts] self.steps.push({ selector: "#" + step.element, property: atk[1], value: atk[3] });
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] });
// [ts] const testDir = asset("wpt/accname/manual");
// [ts] const testFiles = fs.readdirSync(testDir, { withFileTypes: true }).filter(e => e.isFile() && e.name.endsWith(".html")).map(e => e.name);
// [ts] for (const testFile of testFiles) {
// [ts] if (skipped.includes(testFile))
// [ts] continue;
// [ts] const included = (range == 0 || testFile >= ranges[range - 1]) && (range == ranges.length || testFile < ranges[range]);
// [ts] if (!included)
// [ts] continue;
// [ts] test.step(testFile, () => {
// [ts] page.goto(server.PREFIX + `/wpt/accname/manual/` + testFile);
// Use $eval to force injected script.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const result = await page.$eval('body', () => {
// [ts-skip] const result = [];
// [ts-skip] for (const step of (window as any).steps) {
// [ts-skip] const element = document.querySelector(step.selector);
// [ts-skip] if (!element)
// [ts-skip] throw new Error(`Unable to resolve "${step.selector}"`);
// [ts-skip] const injected = (window as any).__injectedScript;
// [ts-skip] const received = step.property === 'name' ? injected.utils.getElementAccessibleNameText(element) : injected.utils.getElementAccessibleDescription(element).text;
// [ts-skip] result.push({ selector: step.selector, expected: step.value, received });
// [ts-skip] }
// [ts-skip] return result;
// [ts-skip] });
// [ts] for (const { selector, expected, received } of result)
// [ts] expect.soft(received, `checking "${selector}" in ${testFile}`).toBe(expected);
// [ts] });
// [ts] }
// [ts] });
// [ts] }




























fn toArray(x: string): []string {
  return if (Array.isArray(x)) x else [x];
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test("AR-144 wpt accname non-manual", () => {
    // [ts] page.addInitScript(() => {
    // [ts] const self = window;
    // [ts] self.AriaUtils = {};
    // [ts] self.AriaUtils.verifyLabelsBySelector = selector => self.__selector = selector;
    // [ts] });
    // [ts] const failing = [
    // Chromium thinks it should use "3" from the span, but Safari does not. Spec is unclear.
    // [ts] "checkbox label with embedded combobox (span)",
    // [ts] "checkbox label with embedded combobox (div)",
    // We do not allow nested visible elements inside parent invisible. Chromium does, but Safari does not. Spec is unclear.
    // [ts] "heading with name from content, containing element that is visibility:hidden with nested content that is visibility:visible",
    // TODO: dd/dt elements have roles that prohibit naming. However, both Chromium and Safari still support naming.
    // [ts] "label valid on dd element",
    // [ts] "label valid on dt element",
    // TODO: recursive bugs
    // [ts] "heading with link referencing image using aria-labelledby, that in turn references text element via aria-labelledby",
    // [ts] "heading with link referencing image using aria-labelledby, that in turn references itself and another element via aria-labelledby",
    // [ts] "button's hidden referenced name (visibility:hidden) with hidden aria-labelledby traversal falls back to aria-label",
    // TODO: preserve "tab" character and non-breaking-spaces from "aria-label" attribute
    // [ts] "link with text node, with tab char",
    // [ts] "nav with trailing nbsp char aria-label is valid (nbsp is preserved in name)",
    // [ts] "button with leading nbsp char in aria-label is valid (and uses aria-label)",
    // [ts] ];
    // [ts] const testDir = asset("wpt/accname/name");
    // [ts] const testFiles = fs.readdirSync(testDir, { withFileTypes: true }).filter(e => e.isFile() && e.name.endsWith(".html")).map(e => `/wpt/accname/name/` + e.name);
    // [ts] testFiles.push(...fs.readdirSync(testDir + "/shadowdom", { withFileTypes: true }).filter(e => e.isFile() && e.name.endsWith(".html")).map(e => `/wpt/accname/name/shadowdom` + e.name));
    // [ts] for (const testFile of testFiles) {
    // [ts] test.step(testFile, () => {
    // [ts] page.goto(server.PREFIX + testFile);
    // Use $eval to force injected script.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const result = await page.$eval('body', () => {
    // [ts-skip] const result = [];
    // [ts-skip] for (const element of document.querySelectorAll((window as any).__selector)) {
    // [ts-skip] const injected = (window as any).__injectedScript;
    // [ts-skip] const title = element.getAttribute('data-testname');
    // [ts-skip] const expected = element.getAttribute('data-expectedlabel');
    // [ts-skip] const received = injected.utils.getElementAccessibleNameText(element);
    // [ts-skip] result.push({ title, expected, received });
    // [ts-skip] }
    // [ts-skip] return result;
    // [ts-skip] });
    // [ts] for (const { title, expected, received } of result) {
    // [ts] if (!failing.includes(title))
    // [ts] expect.soft(received, `${testFile}: ${title}`).toBe(expected);
    // [ts] }
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("AR-145 axe-core implicit-role", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const testCases = require(asset("axe-core/implicit-role"));
    // [ts] for (const testCase of testCases) {
    // [ts] test.step(`checking ${JSON.stringify(testCase)}`, () => {
    // [ts] page.setContent(`
    // [ts] <body>
    // [ts] ${testCase.html}
    // [ts] </body>
    // [ts] `);
    // Use $eval to force injected script.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const received = await page.$eval('body', (_, selector) => {
    // [ts-skip] const element = document.querySelector(selector);
    // [ts-skip] if (!element)
    // [ts-skip] throw new Error(`Unable to resolve "${selector}"`);
    // [ts-skip] return (window as any).__injectedScript.utils.getAriaRole(element);
    // [ts-skip] }, testCase.target);
    // [ts] expect.soft(received, `checking ${JSON.stringify(testCase)}`).toBe(testCase.role);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("AR-146 axe-core accessible-text", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const testCases = require(asset("axe-core/accessible-text"));
    // [ts] for (const testCase of testCases) {
    // [ts] test.step(`checking ${JSON.stringify(testCase)}`, () => {
    // [ts] page.setContent(`
    // [ts] <body>
    // [ts] ${testCase.html}
    // [ts] </body>
    // [ts] <script>
    // [ts] for (const template of document.querySelectorAll("template[shadow]")) {
    // [ts] const shadowRoot = template.parentElement.attachShadow({ mode: 'open' });
    // [ts] shadowRoot.appendChild(template.content);
    // [ts] template.remove();
    // [ts] }
    // [ts] </script>
    // [ts] `);
    // Use $eval to force injected script.
    // [ts] const targets = toArray(testCase.target);
    // [ts] const expected = toArray(testCase.accessibleText);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const received = await page.$eval('body', (_, selectors) => {
    // [ts-skip] return selectors.map(selector => {
    // [ts-skip] const injected = (window as any).__injectedScript;
    // [ts-skip] const element = injected.querySelector(injected.parseSelector('css=' + selector), document, false);
    // [ts-skip] if (!element)
    // [ts-skip] throw new Error(`Unable to resolve "${selector}"`);
    // [ts-skip] return injected.utils.getElementAccessibleNameText(element);
    // [ts-skip] });
    // [ts-skip] }, targets);
    // [ts] expect.soft(received, `checking ${JSON.stringify(testCase)}`).toEqual(expected);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("AR-147 accessible name with slots", () => {
    // Text "foo" is assigned to the slot, should not be used twice.
    // [ts] page.setContent(`
    // [ts] <button><div>foo</div></button>
    // [ts] <script>
    // [ts] (() => {
    // [ts] const container = document.querySelector('div');
    // [ts] const shadow = container.attachShadow({ mode: 'open' });
    // [ts] const slot = document.createElement('slot');
    // [ts] shadow.appendChild(slot);
    // [ts] })();
    // [ts] </script>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "button")).toEqual({ role: "button", name: "foo" });
    // Text "foo" is assigned to the slot, should be used instead of slot content.
    // [ts] page.setContent(`
    // [ts] <div>foo</div>
    // [ts] <script>
    // [ts] (() => {
    // [ts] const container = document.querySelector('div');
    // [ts] const shadow = container.attachShadow({ mode: 'open' });
    // [ts] const button = document.createElement('button');
    // [ts] shadow.appendChild(button);
    // [ts] const slot = document.createElement('slot');
    // [ts] button.appendChild(slot);
    // [ts] const span = document.createElement('span');
    // [ts] span.textContent = 'pre';
    // [ts] slot.appendChild(span);
    // [ts] })();
    // [ts] </script>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "button")).toEqual({ role: "button", name: "foo" });
    // Nothing is assigned to the slot, should use slot content.
    // [ts] page.setContent(`
    // [ts] <div></div>
    // [ts] <script>
    // [ts] (() => {
    // [ts] const container = document.querySelector('div');
    // [ts] const shadow = container.attachShadow({ mode: 'open' });
    // [ts] const button = document.createElement('button');
    // [ts] shadow.appendChild(button);
    // [ts] const slot = document.createElement('slot');
    // [ts] button.appendChild(slot);
    // [ts] const span = document.createElement('span');
    // [ts] span.textContent = 'pre';
    // [ts] slot.appendChild(span);
    // [ts] })();
    // [ts] </script>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "button")).toEqual({ role: "button", name: "pre" });
    // Text "foo" is not assigned to any slot, should not be used.
    // [ts] page.setContent(`
    // [ts] <div role=button>foo</div>
    // [ts] <script>
    // [ts] (() => {
    // [ts] const container = document.querySelector('div');
    // [ts] const shadow = container.attachShadow({ mode: 'open' });
    // [ts] shadow.appendChild(document.createTextNode('bar'));
    // [ts] })();
    // [ts] </script>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "div")).toEqual({ role: "button", name: "bar" });
    // [ts] });
    // [ts] test("AR-148 accessible name with closed details", () => {
    // [ts] page.setContent(`
    // [ts] <button aria-labelledby="details"></button>
    // [ts] <details id="details">
    // [ts] <summary>Summary</summary>
    // [ts] Details
    // [ts] </details>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "button")).toEqual({ role: "button", name: "Summary" });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('details', details => details.open = true);
    // [ts] expect.soft(getNameAndRole(page, "button")).toEqual({ role: "button", name: "Summary Details" });
    // [ts] });
    // [ts] test("AR-149 accessible name nested treeitem", () => {
    // [ts] page.setContent(`
    // [ts] <div role=treeitem id=target>
    // [ts] <span>Top-level</span>
    // [ts] <div role=group>
    // [ts] <div role=treeitem><span>Nested 1</span></div>
    // [ts] <div role=treeitem><span>Nested 2</span></div>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#target")).toEqual({ role: "treeitem", name: "Top-level" });
    // [ts] });
    // [ts] test("AR-150 svg title", () => {
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <svg width="162" height="30" viewBox="0 0 162 30" fill="none" xmlns="http://www.w3.org/2000/svg">
    // [ts] <title>Submit</title>
    // [ts] <g>
    // [ts] <title>Hello</title>
    // [ts] </g>
    // [ts] <a href="example.com" xlink:title="a link"><circle cx="50" cy="40" r="35" /></a>
    // [ts] </svg>
    // [ts] </div>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "svg")).toEqual({ role: "img", name: "Submit" });
    // [ts] expect.soft(getNameAndRole(page, "g")).toEqual({ role: null, name: "Hello" });
    // [ts] expect.soft(getNameAndRole(page, "a")).toEqual({ role: "link", name: "a link" });
    // [ts] });
    // [ts] test("AR-151 native controls", () => {
    // [ts] page.setContent(`
    // [ts] <label for="text1">TEXT1</label><input id="text1" type=text>
    // [ts] <input id="text2" type=text title="TEXT2">
    // [ts] <input id="text3" type=text placeholder="TEXT3">
    // [ts] <input id="number1" type=number placeholder="NUMBER1">
    // [ts] <label for="image1">IMAGE1</label><input id="image1" type=image>
    // [ts] <input id="image2" type=image alt="IMAGE2">
    // [ts] <label for="image3">IMAGE3</label><input id="image3" type=image alt="MORE3">
    // [ts] <label for="button1">BUTTON1</label><button id="button1" role="combobox">button</button>
    // [ts] <button id="button2" role="combobox">BUTTON2</button>
    // [ts] <button id="button3">BUTTON3</button>
    // [ts] <button id="button4" title="BUTTON4"></button>
    // [ts] <input id="file1" type=file>
    // [ts] <label for="file2">FILE2</label><input id="file2" type=file>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#text1")).toEqual({ role: "textbox", name: "TEXT1" });
    // [ts] expect.soft(getNameAndRole(page, "#text2")).toEqual({ role: "textbox", name: "TEXT2" });
    // [ts] expect.soft(getNameAndRole(page, "#text3")).toEqual({ role: "textbox", name: "TEXT3" });
    // [ts] expect.soft(getNameAndRole(page, "#number1")).toEqual({ role: "spinbutton", name: "NUMBER1" });
    // [ts] expect.soft(getNameAndRole(page, "#image1")).toEqual({ role: "button", name: "IMAGE1" });
    // [ts] expect.soft(getNameAndRole(page, "#image2")).toEqual({ role: "button", name: "IMAGE2" });
    // [ts] expect.soft(getNameAndRole(page, "#image3")).toEqual({ role: "button", name: "IMAGE3" });
    // [ts] expect.soft(getNameAndRole(page, "#button1")).toEqual({ role: "combobox", name: "BUTTON1" });
    // [ts] expect.soft(getNameAndRole(page, "#button2")).toEqual({ role: "combobox", name: "" });
    // [ts] expect.soft(getNameAndRole(page, "#button3")).toEqual({ role: "button", name: "BUTTON3" });
    // [ts] expect.soft(getNameAndRole(page, "#button4")).toEqual({ role: "button", name: "BUTTON4" });
    // [ts] expect.soft(getNameAndRole(page, "#file1")).toEqual({ role: "button", name: "Choose File" });
    // [ts] expect.soft(getNameAndRole(page, "#file2")).toEqual({ role: "button", name: "FILE2" });
    // [ts] });
    // [ts] test("AR-152 input type=search maps to searchbox unless list points at a datalist", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41899" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <input id="search1" type=search>
    // [ts] <input id="search2" type=search list=nope>
    // [ts] <input id="search3" type=search list=dv><div id=dv></div>
    // [ts] <input id="search4" type=search list=dl><datalist id=dl></datalist>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#search1")).toEqual({ role: "searchbox", name: "" });
    // [ts] expect.soft(getNameAndRole(page, "#search2")).toEqual({ role: "searchbox", name: "" });
    // [ts] expect.soft(getNameAndRole(page, "#search3")).toEqual({ role: "searchbox", name: "" });
    // [ts] expect.soft(getNameAndRole(page, "#search4")).toEqual({ role: "combobox", name: "" });
    // [ts] });
    // [ts] test("AR-153 meter and progress get their name from an associated label", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41891" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <label for="meter1">Battery</label><meter id="meter1" value=0.5></meter>
    // [ts] <label for="progress1">Loading</label><progress id="progress1" value=0.3></progress>
    // [ts] <label>Charge <meter id="meter2" value=0.5></meter></label>
    // [ts] <label for="meter3">Ignored</label><meter id="meter3" aria-label="Overridden" value=0.5></meter>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#meter1")).toEqual({ role: "meter", name: "Battery" });
    // [ts] expect.soft(getNameAndRole(page, "#progress1")).toEqual({ role: "progressbar", name: "Loading" });
    // [ts] expect.soft(getNameAndRole(page, "#meter2")).toEqual({ role: "meter", name: "Charge" });
    // [ts] expect.soft(getNameAndRole(page, "#meter3")).toEqual({ role: "meter", name: "Overridden" });
    // [ts] });
    // [ts] test("AR-154 native controls labelled-by", () => {
    // [ts] page.setContent(`
    // [ts] <label id="for-text1">TEXT1</label><input aria-labelledby="for-text1" id="text1" type=text>
    // [ts] <label id="for-text2">TEXT2</label><input aria-labelledby="for-text2 text2" id="text2" type=text>
    // [ts] <label id="for-text3" for="text3">TEXT3</label><input aria-labelledby="for-text3 text3" id="text3" type=text>
    // [ts] <label id="for-submit1" for="submit1">SUBMIT1</label><input aria-labelledby="for-submit1 submit1" id="submit1" type=submit>
    // [ts] <label id="for-image1" for="image1">IMAGE1</label><input aria-labelledby="for-image1 image1" id="image1" type=image alt="MORE1">
    // [ts] <label id="for-image2" for="image2">IMAGE2</label><img aria-labelledby="for-image2 image2" id="image2" alt="MORE2" src="data:image/svg,<g></g>">
    // [ts] <label id="for-button1">BUTTON1</label><button aria-labelledby="for-button1" id="button1">MORE1</button>
    // [ts] <label id="for-button2">BUTTON2</label><button aria-labelledby="for-button2 button2" id="button2">MORE2</button>
    // [ts] <label id="for-button3" for="button3">BUTTON3</label><button aria-labelledby="for-button3 button3" id="button3">MORE3</button>
    // [ts] <label id="for-button4" for="button4">BUTTON4</label><button aria-labelledby="for-button4" id="button4">MORE4</button>
    // [ts] <label id="for\u00a0button5">BUTTON5</label><button aria-labelledby="for\u00a0button5\n\tbutton5" id="button5">MORE5</button>
    // [ts] <label id="for-textarea1" for="textarea1">TEXTAREA1</label><textarea aria-labelledby="for-textarea1 textarea1" id="textarea1" placeholder="MORE1">MORE2</textarea>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#text1")).toEqual({ role: "textbox", name: "TEXT1" });
    // [ts] expect.soft(getNameAndRole(page, "#text2")).toEqual({ role: "textbox", name: "TEXT2" });
    // [ts] expect.soft(getNameAndRole(page, "#text3")).toEqual({ role: "textbox", name: "TEXT3" });
    // [ts] expect.soft(getNameAndRole(page, "#submit1")).toEqual({ role: "button", name: "SUBMIT1 Submit" });
    // [ts] expect.soft(getNameAndRole(page, "#image1")).toEqual({ role: "button", name: "IMAGE1 MORE1" });
    // [ts] expect.soft(getNameAndRole(page, "#image2")).toEqual({ role: "img", name: "IMAGE2 MORE2" });
    // [ts] expect.soft(getNameAndRole(page, "#button1")).toEqual({ role: "button", name: "BUTTON1" });
    // [ts] expect.soft(getNameAndRole(page, "#button2")).toEqual({ role: "button", name: "BUTTON2 MORE2" });
    // [ts] expect.soft(getNameAndRole(page, "#button3")).toEqual({ role: "button", name: "BUTTON3 MORE3" });
    // [ts] expect.soft(getNameAndRole(page, "#button4")).toEqual({ role: "button", name: "BUTTON4" });
    // [ts] expect.soft(getNameAndRole(page, "#button5")).toEqual({ role: "button", name: "BUTTON5 MORE5" });
    // [ts] expect.soft(getNameAndRole(page, "#textarea1")).toEqual({ role: "textbox", name: "TEXTAREA1 MORE2" });
    // [ts] });
    // [ts] test("AR-155 display:contents should be visible when contents are visible", () => {
    // [ts] page.setContent(`
    // [ts] <button style='display: contents;'>yo</button>
    // [ts] `);
    // [ts] expect(page.getByRole("button")).toHaveCount(1);
    // [ts] });
    // [ts] test("AR-156 should remove soft hyphens and zero-width spaces", () => {
    // [ts] page.setContent(`
    // [ts] <button>1\u00ad2\u200b3</button>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "button")).toEqual({ role: "button", name: "123" });
    // [ts] });
    // [ts] test("AR-157 label/labelled-by aria-hidden with descendants", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29796" });
    // [ts] page.setContent(`
    // [ts] <body>
    // [ts] <div id="case1">
    // [ts] <button aria-labelledby="label1" type="button"></button>
    // [ts] <tool-tip id="label1" for="button-preview" popover="manual" aria-hidden="true" role="tooltip">Label1</tool-tip>
    // [ts] </div>
    // [ts] <div id="case2">
    // [ts] <label for="button2" aria-hidden="true"><div id="label2">Label2</div></label>
    // [ts] <button id="button2" type="button"></button>
    // [ts] </div>
    // [ts] </body>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$$eval('#label1, #label2', els => {
    // [ts-skip] els.forEach(el => el.attachShadow({ mode: 'open' }).appendChild(document.createElement('slot')));
    // [ts-skip] });
    // [ts] expect.soft(getNameAndRole(page, "#case1 button")).toEqual({ role: "button", name: "Label1" });
    // [ts] expect.soft(getNameAndRole(page, "#case2 button")).toEqual({ role: "button", name: "Label2" });
    // [ts] });
    // [ts] test("AR-158 own aria-label concatenated with aria-labelledby", () => {
    // This is taken from https://w3c.github.io/accname/#example-5-0
    // [ts] page.setContent(`
    // [ts] <h1>Files</h1>
    // [ts] <ul>
    // [ts] <li>
    // [ts] <a id="file_row1" href="./files/Documentation.pdf">Documentation.pdf</a>
    // [ts] <span role="button" tabindex="0" id="del_row1" aria-label="Delete" aria-labelledby="del_row1 file_row1"></span>
    // [ts] </li>
    // [ts] <li>
    // [ts] <a id="file_row2" href="./files/HolidayLetter.pdf">HolidayLetter.pdf</a>
    // [ts] <span role="button" tabindex="0" id="del_row2" aria-label="Delete" aria-labelledby="del_row2 file_row2"></span>
    // [ts] </li>
    // [ts] </ul>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#del_row1")).toEqual({ role: "button", name: "Delete Documentation.pdf" });
    // [ts] expect.soft(getNameAndRole(page, "#del_row2")).toEqual({ role: "button", name: "Delete HolidayLetter.pdf" });
    // [ts] });
    // [ts] test("AR-159 control embedded in a label", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43785" } }, (_p) => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28848" });
    // [ts] page.setContent(`
    // [ts] <label for="flash">
    // [ts] <input type="checkbox" id="flash">
    // [ts] Flash the screen <span tabindex="0" role="textbox" aria-label="number of times" contenteditable>5</span> times.
    // [ts] </label>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "input")).toEqual({ role: "checkbox", name: "Flash the screen 5 times." });
    // [ts] expect.soft(getNameAndRole(page, "span")).toEqual({ role: "textbox", name: "number of times" });
    // [ts] expect.soft(getNameAndRole(page, "label")).toEqual({ role: null, name: "" });
    // [ts] page.setContent(`
    // [ts] <input type="text" id="flash">
    // [ts] <label for="flash">
    // [ts] Flash the screen
    // [ts] <div role="combobox">
    // [ts] <div role="textbox"></div>
    // [ts] <ul role="listbox" style="list-style-type: none">
    // [ts] <li role="option" aria-selected="True">1</li>
    // [ts] <li role="option" aria-selected="false">2</li>
    // [ts] <li role="option">3</li>
    // [ts] </ul>
    // [ts] </div>
    // [ts] times.
    // [ts] </label>
    // [ts] `);
    // [ts] expect.soft(page.locator("#flash").ariaSnapshot()).toBe("- textbox \"Flash the screen 1 times.\"");
    // [ts] expect(page.getByRole("textbox", { name: "Flash the screen 1 times.", exact: true })).toHaveAttribute("id", "flash");
    // [ts] });
    // [ts] test("AR-160 control embedded in a target element", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28848" });
    // [ts] page.setContent(`
    // [ts] <h1>
    // [ts] <input type="text" value="Foo bar">
    // [ts] </h1>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "h1")).toEqual({ role: "heading", name: "Foo bar" });
    // [ts] });
    // [ts] test("AR-161 searchbox embedded control should contribute its value", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42341" });
    // [ts] page.setContent(`
    // [ts] <button id="b1" aria-labelledby="l1"></button><div id="l1" hidden><input type="text" value="Query"></div>
    // [ts] <button id="b2" aria-labelledby="l2"></button><div id="l2" hidden><input type="search" value="Query"></div>
    // [ts] <label for="c1">Flash the screen <input type="search" value="5"> times.</label>
    // [ts] <input type="checkbox" id="c1">
    // [ts] <h1><input type="search" value="Foo bar"></h1>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#b1")).toEqual({ role: "button", name: "Query" });
    // [ts] expect.soft(getNameAndRole(page, "#b2")).toEqual({ role: "button", name: "Query" });
    // [ts] expect.soft(getNameAndRole(page, "#c1")).toEqual({ role: "checkbox", name: "Flash the screen 5 times." });
    // [ts] expect.soft(getNameAndRole(page, "h1")).toEqual({ role: "heading", name: "Foo bar" });
    // [ts] });
    // [ts] test("AR-162 svg role=presentation", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26809" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <img src="pptr.png" alt="Code is Poetry." />
    // [ts] <svg viewBox="0 0 100 100" width="16" height="16" xmlns="http://www.w3.org/2000/svg" role="presentation" focusable="false"><circle cx="50" cy="50" r="50"></circle></svg>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "img")).toEqual({ role: "img", name: "Code is Poetry." });
    // [ts] expect.soft(getNameAndRole(page, "svg")).toEqual({ role: "presentation", name: "" });
    // [ts] });
    // [ts] test("AR-163 should work with form and tricky input names", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30616" });
    // [ts] page.setContent(`
    // [ts] <form aria-label="my form">
    // [ts] <input name="tagName" value="hello" title="tagName input">
    // [ts] <input name="localName" value="hello" title="localName input">
    // [ts] </form>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "form")).toEqual({ role: "form", name: "my form" });
    // [ts] });
    // [ts] test("AR-164 should ignore stylesheet from hidden aria-labelledby subtree", () => {
    // [ts] page.setContent(`
    // [ts] <div id=mylabel style="display:none">
    // [ts] <template shadowrootmode=open>
    // [ts] <style>span { color: red; }</style>
    // [ts] <span>hello</span>
    // [ts] </template>
    // [ts] </div>
    // [ts] <input aria-labelledby=mylabel type=text>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "input")).toEqual({ role: "textbox", name: "hello" });
    // [ts] });
    // [ts] test("AR-165 should not include hidden pseudo into accessible name", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] span:before {
    // [ts] content: 'world';
    // [ts] display: none;
    // [ts] }
    // [ts] div:after {
    // [ts] content: 'bye';
    // [ts] visibility: hidden;
    // [ts] }
    // [ts] </style>
    // [ts] <a href="http://example.com">
    // [ts] <span>hello</span>
    // [ts] <div>hello</div>
    // [ts] </a>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "a")).toEqual({ role: "link", name: "hello hello" });
    // [ts] });
    // [ts] test("AR-166 should resolve pseudo content from attr", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] .stars:before {
    // [ts] display: block;
    // [ts] content: attr(data-hello);
    // [ts] }
    // [ts] </style>
    // [ts] <a href="http://example.com">
    // [ts] <div class="stars" data-hello="hello">world</div>
    // [ts] </a>
    // [ts] `);
    // [ts] expect_eq(getNameAndRole(page, "a"), { role: "link", name: "hello world" });
    // [ts] });
    // [ts] test("AR-167 should resolve pseudo content alternative text", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] .with-content:before {
    // [ts] content: url("data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg'></svg>") / "alternative text";
    // [ts] }
    // [ts] </style>
    // [ts] <div role="button" class="with-content"> inner text</div>
    // [ts] `);
    // [ts] expect_eq(getNameAndRole(page, "div"), { role: "button", name: "alternative text inner text" });
    // [ts] });
    // [ts] test("AR-168 should resolve css content property for an element", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] .with-content-1 {
    // [ts] content: url("data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg'></svg>") / "alternative text";
    // [ts] }
    // [ts] .with-content-2 {
    // [ts] content: url("data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg'></svg>");
    // [ts] }
    // [ts] </style>
    // [ts] <div id="button1" role="button" class="with-content-1">inner text</div>
    // [ts] <div id="button2" role="button" class="with-content-2">inner text</div>
    // [ts] `);
    // [ts] expect_eq(getNameAndRole(page, "#button1"), { role: "button", name: "alternative text" });
    // [ts] expect_eq(getNameAndRole(page, "#button2"), { role: "button", name: "inner text" });
    // [ts] });
    // [ts] test("AR-169 should ignore invalid aria-labelledby", () => {
    // [ts] page.setContent(`
    // [ts] <label>
    // [ts] <span>Text here</span>
    // [ts] <input type=text aria-labelledby="does-not-exist">
    // [ts] </label>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "input")).toEqual({ role: "textbox", name: "Text here" });
    // [ts] });
    // [ts] test("AR-170 should support search element", () => {
    // [ts] page.setContent(`
    // [ts] <search id=search1 aria-label="example">
    // [ts] Hello
    // [ts] </search>
    // [ts] <search id=search2>
    // [ts] World
    // [ts] </search>
    // [ts] `);
    // [ts] expect.soft(getNameAndRole(page, "#search1")).toEqual({ role: "search", name: "example" });
    // [ts] expect.soft(getNameAndRole(page, "#search2")).toEqual({ role: "search", name: "" });
    // [ts] expect.soft(page.getByRole("search", { name: "example" })).toBeVisible();
    // [ts] });
}
