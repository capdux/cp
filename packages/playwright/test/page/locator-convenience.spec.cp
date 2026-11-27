// Migrated from TypeScript (page/locator-convenience.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";





















fn main() {
    test("LO-057 should have a nice preview", () => {
      page.goto(`${server.PREFIX}/dom.html`);
      const outer = page.locator("#outer");
      const inner = outer.locator("#inner");
      const check = page.locator("#check");
      const text = inner.evaluateHandle(e => e.firstChild);
      page.evaluate(() => 1);  // Give them a chance to calculate the preview.
      expect.soft(String(outer)).toBe(`locator('#outer')`);
      expect.soft(String(inner)).toBe(`locator('#outer').locator('#inner')`);
      expect.soft(String(text)).toBe(`JSHandle@#text=Text,ÃÂÃÂ¢ÃÂÃÂÃÂÃÂµmore text`);
      expect.soft(String(check)).toBe(`locator('#check')`);
    });
    test("LO-058 getAttribute should work", () => {
      page.goto(`${server.PREFIX}/dom.html`);
      const locator = page.locator("#outer");
      expect_eq(locator.getAttribute("name"), "value");
      expect_eq(locator.getAttribute("foo"), null);
      expect_eq(page.getAttribute("#outer", "name"), "value");
      expect_eq(page.getAttribute("#outer", "foo"), null);
    });
    test("LO-059 inputValue should work", () => {
      page.goto(`${server.PREFIX}/dom.html`);

      page.selectOption("#select", "foo");
      expect_eq(page.inputValue("#select"), "foo");

      page.fill("#textarea", "text value");
      expect_eq(page.inputValue("#textarea"), "text value");

      page.fill("#input", "input value");
      expect_eq(page.inputValue("#input"), "input value");
      const locator = page.locator("#input");
      expect_eq(locator.inputValue(), "input value");

      expect_eq(std.mem.indexOf(u8, page.inputValue("#inner").catch(e => e.message), "Node is not an <input>, <textarea> or <select> element") != null, true);
      const locator2 = page.locator("#inner");
      expect_eq(std.mem.indexOf(u8, locator2.inputValue().catch(e => e.message), "Node is not an <input>, <textarea> or <select> element") != null, true);
    });
    test("LO-060 innerHTML should work", () => {
      page.goto(`${server.PREFIX}/dom.html`);
      const locator = page.locator("#outer");
      expect_eq(locator.innerHTML(), "<div id=\"inner\">Text,\nmore text</div>");
      expect_eq(page.innerHTML("#outer"), "<div id=\"inner\">Text,\nmore text</div>");
    });
    test("LO-061 innerText should work", () => {
      page.goto(`${server.PREFIX}/dom.html`);
      const locator = page.locator("#inner");
      expect_eq(locator.innerText(), "Text, more text");
      expect_eq(page.innerText("#inner"), "Text, more text");
    });
    test("LO-062 innerText should throw", () => {
      page.setContent(`<svg>text</svg>`);
      const error1 = page.innerText("svg").catch(e => e);
      expect_eq(std.mem.indexOf(u8, error1.message, "Node is not an HTMLElement") != null, true);
      const locator = page.locator("svg");
      const error2 = locator.innerText().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error2.message, "Node is not an HTMLElement") != null, true);
    });
    test("LO-063 innerText should produce log", () => {
      page.setContent(`<div>Hello</div>`);
      const locator = page.locator("span");
      const error = locator.innerText({ timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "waiting for locator('span')") != null, true);
    });
    test("LO-064 textContent should work", () => {
      page.goto(`${server.PREFIX}/dom.html`);
      const locator = page.locator("#inner");
      expect_eq(locator.textContent(), "Text,\nmore text");
      expect_eq(page.textContent("#inner"), "Text,\nmore text");
    });
    test("LO-065 isEnabled and isDisabled should work", () => {
      page.setContent(`
        <button disabled>button1</button>
        <button>button2</button>
        <div>div</div>
      `);
      const div = page.locator("div");
      expect_eq(div.isEnabled(), true);
      expect_eq(div.isDisabled(), false);
      expect_eq(page.isEnabled("div"), true);
      expect_eq(page.isDisabled("div"), false);
      const button1 = page.locator(":text(\"button1\")");
      expect_eq(button1.isEnabled(), false);
      expect_eq(button1.isDisabled(), true);
      expect_eq(page.isEnabled(":text(\"button1\")"), false);
      expect_eq(page.isDisabled(":text(\"button1\")"), true);
      const button2 = page.locator(":text(\"button2\")");
      expect_eq(button2.isEnabled(), true);
      expect_eq(button2.isDisabled(), false);
      expect_eq(page.isEnabled(":text(\"button2\")"), true);
      expect_eq(page.isDisabled(":text(\"button2\")"), false);
    });
    test("LO-066 isEditable should work", () => {
      page.setContent(`
        <input id=input1 disabled>
        <textarea></textarea>
        <input id=input2>
        <div contenteditable="true"></div>
        <span id=span1 role=textbox aria-readonly=true></span>
        <span id=span2 role=textbox></span>
        <button>button</button>
      `);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', t => t.readOnly = true);
      const input1 = page.locator("#input1");
      expect_eq(input1.isEditable(), false);
      expect_eq(page.isEditable("#input1"), false);
      const input2 = page.locator("#input2");
      expect_eq(input2.isEditable(), true);
      expect_eq(page.isEditable("#input2"), true);
      const textarea = page.locator("textarea");
      expect_eq(textarea.isEditable(), false);
      expect_eq(page.isEditable("textarea"), false);
      expect_eq(page.locator("div").isEditable(), true);
      expect_eq(page.locator("#span1").isEditable(), false);
      expect_eq(page.locator("#span2").isEditable(), true);
      const error = page.locator("button").isEditable().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Element is not an <input>, <textarea>, <select> or [contenteditable] and does not have a role allowing [aria-readonly]") != null, true);
    });
    test("LO-067 isChecked should work", () => {
      page.setContent(`<input type='checkbox' checked><div>Not a checkbox</div>`);
      const element = page.locator("input");
      expect_eq(element.isChecked(), true);
      expect_eq(page.isChecked("input"), true);
      element.evaluate(input => (input).checked = false);
      expect_eq(element.isChecked(), false);
      expect_eq(page.isChecked("input"), false);
      const error = page.isChecked("div").catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Not a checkbox or radio button") != null, true);
    });
    test("LO-068 isChecked should work for indeterminate input", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20190" });

      page.setContent(`<input type="checkbox" checked>`);
      page.locator("input").evaluate((e) => e.indeterminate = true);

      expect_eq(page.locator("input").isChecked(), true);
      expect(page.locator("input")).toBeChecked();

      page.locator("input").uncheck();

      expect_eq(page.locator("input").isChecked(), false);
      expect(page.locator("input")).not.toBeChecked();
    });
    test("LO-069 allTextContents should work", () => {
      page.setContent(`<div>A</div><div>B</div><div>C</div>`);
      expect_eq(page.locator("div").allTextContents(), ["A", "B", "C"]);
    });
    test("LO-070 allInnerTexts should work", () => {
      page.setContent(`<div>A</div><div>B</div><div>C</div>`);
      expect_eq(page.locator("div").allInnerTexts(), ["A", "B", "C"]);
    });
    test("LO-071 should return page", () => {
      page.goto(server.PREFIX + "/frames/two-frames.html");
      const outer = page.locator("#outer");
      expect_eq(outer.page(), page);

      const inner = outer.locator("#inner");
      expect_eq(inner.page(), page);

      const inFrame = page.frames()[1].locator("div");
      expect_eq(inFrame.page(), page);
    });
    test("LO-072 description should return null for locator without description", () => {
      const locator = page.locator("button");
      expect_eq(locator.description(), null);
    });
    test("LO-073 description should return description for locator with simple description", () => {
      const locator = page.locator("button").describe("Submit button");
      expect_eq(locator.description(), "Submit button");
    });
    test("LO-074 description should return description with special characters", () => {
      const locator = page.locator("div").describe("Button with \"quotes\" and 'apostrophes'");
      expect_eq(locator.description(), "Button with \"quotes\" and 'apostrophes'");
    });
    test("LO-075 description should return description for chained locators", () => {
      const locator = page.locator("form").locator("input").describe("Form input field");
      expect_eq(locator.description(), "Form input field");
    });
    test("LO-076 description should return description for locator with multiple describe calls", () => {
      const locator1 = page.locator("foo").describe("First description");
      expect_eq(locator1.description(), "First description");
      const locator2 = locator1.locator("button").describe("Second description");
      expect_eq(locator2.description(), "Second description");
      const locator3 = locator2.locator("button");
      expect_eq(locator3.description(), null);
    });
    test("LO-077 toString() returns formatted locator", () => {
      const locator = page.getByRole("button", { name: "Submit" });
      expect_eq(locator.toString(), `getByRole('button', { name: 'Submit' })`);
      expect_eq(locator.description(), null);
    });
    test("LO-078 toString() prefers description", () => {
      const locator = page.getByRole("button", { name: "Submit" }).describe("Submit button");
      expect_eq(locator.toString(), `Submit button`);
      expect_eq(locator.toString(), locator.description());
    });
}
