// Migrated from TypeScript (page/selectors-get-by.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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
    // TODO(cap-migrate): refine TS body
    // [ts] test("SE-071 getByTestId should work", () => {
    // [ts] page.setContent("<div><div data-testid=\"Hello\">Hello world</div></div>");
    // [ts] expect(page.getByTestId("Hello")).toHaveText("Hello world");
    // [ts] expect(page.mainFrame().getByTestId("Hello")).toHaveText("Hello world");
    // [ts] expect(page.locator("div").getByTestId("Hello")).toHaveText("Hello world");
    // [ts] });
    // [ts] test("SE-072 getByTestId with custom testId should work", () => {
    // [ts] page.setContent("<div><div data-my-custom-testid=\"Hello\">Hello world</div></div>");
    // [ts] playwright.selectors.setTestIdAttribute("data-my-custom-testid");
    // [ts] expect(page.getByTestId("Hello")).toHaveText("Hello world");
    // [ts] expect(page.mainFrame().getByTestId("Hello")).toHaveText("Hello world");
    // [ts] expect(page.locator("div").getByTestId("Hello")).toHaveText("Hello world");
    // [ts] });
    // [ts] test("SE-073 getByTestId with comma-separated testIdAttributes should match any", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div data-pw="Hello">first</div>
    // [ts] <div data-ti="Hello">second</div>
    // [ts] <div data-testid="Hello">third</div>
    // [ts] </section>
    // [ts] `);
    // [ts] playwright.selectors.setTestIdAttribute("data-pw,data-ti");
    // [ts] expect(page.getByTestId("Hello")).toHaveCount(2);
    // [ts] expect(page.getByTestId("Hello")).toHaveText(["first", "second"]);
    // [ts] expect(page.mainFrame().getByTestId("Hello")).toHaveCount(2);
    // [ts] expect(page.locator("section").getByTestId("Hello")).toHaveCount(2);
    // [ts] });
    // [ts] test("SE-074 getByTestId should escape id", () => {
    // [ts] page.setContent(`<div><div data-testid='He"llo'>Hello world</div></div>`);
    // [ts] expect(page.getByTestId("He\"llo")).toHaveText("Hello world");
    // [ts] });
    // [ts] test("SE-075 getByTestId should work for regex", () => {
    // [ts] page.setContent("<div><div data-testid=\"Hello\">Hello world</div></div>");
    // [ts] expect(page.getByTestId("He[l]*o")).toHaveText("Hello world");
    // [ts] expect(page.mainFrame().getByTestId("Hello")).toHaveText("Hello world");
    // [ts] expect(page.locator("div").getByTestId("Hello")).toHaveText("Hello world");
    // [ts] });
    // [ts] test("SE-076 getByText should work", () => {
    // [ts] page.setContent(`<div>yo</div><div>ya</div><div>\nye  </div>`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.getByText("ye").evaluate(e => e.outerHTML), ">\nye  </div>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, page.getByText("ye").evaluate(e => e.outerHTML), ">\nye  </div>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, page.getByText("e").evaluate(e => e.outerHTML), ">\nye  </div>") != null, true);
    // [ts] page.setContent(`<div> ye </div><div>ye</div>`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.getByText("ye", { exact: true }).first().evaluate(e => e.outerHTML), "> ye </div>") != null, true);
    // [ts] page.setContent(`<div>Hello world</div><div>Hello</div>`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.getByText("Hello", { exact: true }).evaluate(e => e.outerHTML), ">Hello</div>") != null, true);
    // [ts] });
    // [ts] test("SE-077 getByLabel should work", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43898" } }, (_p) => {
    // [ts] page.setContent(`<div><label for=target>Name</label><input id=target type=text></div>`);
    // [ts] expect_eq(page.getByText("Name").evaluate(e => e.nodeName), "LABEL");
    // [ts] expect_eq(page.getByLabel("Name").evaluate(e => e.nodeName), "INPUT");
    // [ts] expect_eq(page.mainFrame().getByLabel("Name").evaluate(e => e.nodeName), "INPUT");
    // [ts] expect_eq(page.locator("div").getByLabel("Name").evaluate(e => e.nodeName), "INPUT");
    // [ts] page.setContent(`
    // [ts] <label for="city">City</label><my-field id="city" role="combobox">Pick one</my-field>
    // [ts] <label>Street <my-field id="street" role="combobox">Pick one</my-field></label>
    // [ts] `);
    // [ts] page.evaluate(() => {
    // [ts] customElements.define("my-field", class extends HTMLElement {
    // [ts] static formAssociated = true;
    // [ts] constructor: () => {
    // [ts] super();
    // [ts] _.attachInternals();
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] expect_eq(page.evaluate(() => [...document.querySelectorAll("label")].map(label => label.control?.id)), ["city", "street"]);
    // [ts] expect.soft(page.getByLabel("City").evaluateAll(elements => elements.map(e => e.id))).toEqual(["city"]);
    // [ts] expect.soft(page.getByLabel("Street").evaluateAll(elements => elements.map(e => e.id))).toEqual(["street"]);
    // [ts] expect.soft(page.getByRole("combobox", { name: "City", exact: true }).evaluateAll(elements => elements.map(e => e.id))).toEqual(["city"]);
    // [ts] expect.soft(page.getByRole("combobox", { name: "Street", exact: true }).evaluateAll(elements => elements.map(e => e.id))).toEqual(["street"]);
    // [ts] });
    // [ts] test("SE-078 getByLabel should work with nested elements", () => {
    // [ts] page.setContent(`<label for=target>Last <span>Name</span></label><input id=target type=text>`);
    // [ts] expect(page.getByLabel("last name")).toHaveAttribute("id", "target");
    // [ts] expect(page.getByLabel("st na")).toHaveAttribute("id", "target");
    // [ts] expect(page.getByLabel("Name")).toHaveAttribute("id", "target");
    // [ts] expect(page.getByLabel("Last Name", { exact: true })).toHaveAttribute("id", "target");
    // [ts] expect(page.getByLabel("Last\\s+name")).toHaveAttribute("id", "target");
    // [ts] expect_eq(page.getByLabel("Last", { exact: true }).elementHandles(), []);
    // [ts] expect_eq(page.getByLabel("last name", { exact: true }).elementHandles(), []);
    // [ts] expect_eq(page.getByLabel("Name", { exact: true }).elementHandles(), []);
    // [ts] expect_eq(page.getByLabel("what?").elementHandles(), []);
    // [ts] expect_eq(page.getByLabel("last name").elementHandles(), []);
    // [ts] });
    // [ts] test("SE-079 getByLabel should work with multiply-labelled input", () => {
    // [ts] page.setContent(`<label for=target>Name</label><input id=target type=text><label for=target>First or Last</label>`);
    // [ts] expect_eq(page.getByLabel("Name").evaluate(e => e.id), "target");
    // [ts] expect_eq(page.getByLabel("First or Last").evaluate(e => e.id), "target");
    // [ts] });
    // [ts] test("SE-080 getByLabel should work with ancestor label and multiple controls", () => {
    // [ts] page.setContent(`<label>Name<button id=target>Click me</button><input type=text></label>`);
    // [ts] expect_eq(page.getByLabel("Name").evaluate(e => e.id), "target");
    // [ts] });
    // [ts] test("SE-081 getByLabel should work with ancestor label and for", () => {
    // [ts] page.setContent(`
    // [ts] <label for=target>Name<input type=text id=nontarget></label>
    // [ts] <input type=text id=target>
    // [ts] `);
    // [ts] expect_eq(page.getByLabel("Name").evaluate(e => e.id), "target");
    // [ts] });
    // [ts] test("SE-082 getByLabel should work with aria-labelledby", () => {
    // [ts] page.setContent(`<label id=name-label>Name</label><button aria-labelledby=name-label>Click me</button>`);
    // [ts] expect_eq(page.getByLabel("Name").evaluate(e => e.textContent), "Click me");
    // [ts] });
    // [ts] test("SE-083 getByLabel should prioritize aria-labelledby over native label", () => {
    // [ts] page.setContent(`
    // [ts] <label id=name-label>Name</label>
    // [ts] <label>Wrong<button aria-labelledby=name-label>Click me</button></label>
    // [ts] `);
    // [ts] expect_eq(page.getByLabel("Name").evaluate(e => e.textContent), "Click me");
    // [ts] });
    // [ts] test("SE-084 getByLabel should work with aria-label", () => {
    // [ts] page.setContent(`<input id=target aria-label="Name">`);
    // [ts] expect_eq(page.getByLabel("Name").evaluate(e => e.id), "target");
    // [ts] });
    // [ts] test("SE-085 getByLabel should ignore empty aria-label", () => {
    // [ts] page.setContent(`<label for=target>Last Name</label><input id=target type=text aria-label>`);
    // [ts] expect_eq(page.getByLabel("Last Name").evaluate(e => e.id), "target");
    // [ts] });
    // [ts] test("SE-086 getByLabel should prioritize aria-labelledby over aria-label", () => {
    // [ts] page.setContent(`<label id=other-label>Other</label><input id=target aria-label="Name" aria-labelledby=other-label>`);
    // [ts] expect_eq(page.getByLabel("Other").evaluate(e => e.id), "target");
    // [ts] });
    // [ts] test("SE-087 getByPlaceholder should work", () => {
    // [ts] page.setContent(`<div>
    // [ts] <input placeholder='Hello'>
    // [ts] <input placeholder='Hello World'>
    // [ts] </div>`);
    // [ts] expect(page.getByPlaceholder("hello")).toHaveCount(2);
    // [ts] expect(page.getByPlaceholder("Hello", { exact: true })).toHaveCount(1);
    // [ts] expect(page.getByPlaceholder("wor")).toHaveCount(1);
    // Coverage
    // [ts] expect(page.mainFrame().getByPlaceholder("hello")).toHaveCount(2);
    // [ts] expect(page.locator("div").getByPlaceholder("hello")).toHaveCount(2);
    // [ts] });
    // [ts] test("SE-088 getByAltText should work", () => {
    // [ts] page.setContent(`<div>
    // [ts] <input alt='Hello'>
    // [ts] <input alt='Hello World'>
    // [ts] </div>`);
    // [ts] expect(page.getByAltText("hello")).toHaveCount(2);
    // [ts] expect(page.getByAltText("Hello", { exact: true })).toHaveCount(1);
    // [ts] expect(page.getByAltText("wor")).toHaveCount(1);
    // Coverage
    // [ts] expect(page.mainFrame().getByAltText("hello")).toHaveCount(2);
    // [ts] expect(page.locator("div").getByAltText("hello")).toHaveCount(2);
    // [ts] });
    // [ts] test("SE-089 getByTitle should work", () => {
    // [ts] page.setContent(`<div>
    // [ts] <input title='Hello'>
    // [ts] <input title='Hello World'>
    // [ts] </div>`);
    // [ts] expect(page.getByTitle("hello")).toHaveCount(2);
    // [ts] expect(page.getByTitle("Hello", { exact: true })).toHaveCount(1);
    // [ts] expect(page.getByTitle("wor")).toHaveCount(1);
    // Coverage
    // [ts] expect(page.mainFrame().getByTitle("hello")).toHaveCount(2);
    // [ts] expect(page.locator("div").getByTitle("hello")).toHaveCount(2);
    // [ts] });
    // [ts] test("SE-090 getBy escaping", () => {
    // [ts] page.setContent(`<label id=label for=control>Hello my
    // [ts] wo"rld</label><input id=control />`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
    // [ts-skip] input.setAttribute('placeholder', 'hello my\nwo"rld');
    // [ts-skip] input.setAttribute('title', 'hello my\nwo"rld');
    // [ts-skip] input.setAttribute('alt', 'hello my\nwo"rld');
    // [ts-skip] });
    // [ts] expect.soft(page.getByText("hello my\nwo\"rld")).toHaveAttribute("id", "label");
    // [ts] expect.soft(page.getByText("hello       my     wo\"rld")).toHaveAttribute("id", "label");
    // [ts] expect.soft(page.getByLabel("hello my\nwo\"rld")).toHaveAttribute("id", "control");
    // [ts] expect.soft(page.getByPlaceholder("hello my\nwo\"rld")).toHaveAttribute("id", "control");
    // [ts] expect.soft(page.getByAltText("hello my\nwo\"rld")).toHaveAttribute("id", "control");
    // [ts] expect.soft(page.getByTitle("hello my\nwo\"rld")).toHaveAttribute("id", "control");
    // [ts] page.setContent(`<label id=label for=control>Hello my
    // [ts] world</label><input id=control />`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
    // [ts-skip] input.setAttribute('placeholder', 'hello my\nworld');
    // [ts-skip] input.setAttribute('title', 'hello my\nworld');
    // [ts-skip] input.setAttribute('alt', 'hello my\nworld');
    // [ts-skip] });
    // [ts] expect.soft(page.getByText("hello my\nworld")).toHaveAttribute("id", "label");
    // [ts] expect.soft(page.getByText("hello        my    world")).toHaveAttribute("id", "label");
    // [ts] expect.soft(page.getByLabel("hello my\nworld")).toHaveAttribute("id", "control");
    // [ts] expect.soft(page.getByPlaceholder("hello my\nworld")).toHaveAttribute("id", "control");
    // [ts] expect.soft(page.getByAltText("hello my\nworld")).toHaveAttribute("id", "control");
    // [ts] expect.soft(page.getByTitle("hello my\nworld")).toHaveAttribute("id", "control");
    // [ts] page.setContent(`<div id=target title="my title">Text here</div>`);
    // [ts] expect.soft(page.getByTitle("my title", { exact: true })).toHaveCount(1, { timeout: 500 });
    // [ts] expect.soft(page.getByTitle("my t\itle", { exact: true })).toHaveCount(1, { timeout: 500 });
    // [ts] expect.soft(page.getByTitle("my t\\itle", { exact: true })).toHaveCount(0, { timeout: 500 });
    // [ts] expect.soft(page.getByTitle("my t\\\itle", { exact: true })).toHaveCount(0, { timeout: 500 });
    // [ts] expect.soft(page.getByTitle("my t\\\\itle", { exact: true })).toHaveCount(0, { timeout: 500 });
    // [ts] page.setContent(`<label for=target>foo &gt;&gt; bar</label><input id=target>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
    // [ts-skip] input.setAttribute('placeholder', 'foo >> bar');
    // [ts-skip] input.setAttribute('title', 'foo >> bar');
    // [ts-skip] input.setAttribute('alt', 'foo >> bar');
    // [ts-skip] });
    // [ts] expect.soft(page.getByText("foo >> bar").textContent()).toBe("foo >> bar");
    // [ts] expect.soft(page.locator("label")).toHaveText("foo >> bar");
    // [ts] expect.soft(page.getByText("foo >> bar")).toHaveText("foo >> bar");
    // [ts] expect.soft(page.getByText("foo >> bar").textContent()).toBe("foo >> bar");
    // [ts] expect.soft(page.getByLabel("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] expect.soft(page.getByLabel("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] expect.soft(page.getByPlaceholder("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] expect.soft(page.getByAltText("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] expect.soft(page.getByTitle("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] expect.soft(page.getByPlaceholder("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] expect.soft(page.getByAltText("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] expect.soft(page.getByTitle("foo >> bar")).toHaveAttribute("id", "target");
    // [ts] });
    // [ts] test("SE-091 getByRole escaping", () => {
    // [ts] page.setContent(`
    // [ts] <a href="https://playwright.dev">issues 123</a>
    // [ts] <a href="https://playwright.dev">he llo 56</a>
    // [ts] <button>Click me</button>
    // [ts] `);
    // [ts] expect.soft(page.getByRole("button").evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Click me</button>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("link").evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<a href="https://playwright.dev">issues 123</a>`,
    // [ts] `<a href="https://playwright.dev">he llo 56</a>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("link", { name: "issues 123" }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<a href="https://playwright.dev">issues 123</a>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("link", { name: "sues" }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<a href="https://playwright.dev">issues 123</a>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("link", { name: "  he    \n  llo " }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<a href="https://playwright.dev">he llo 56</a>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { name: "issues" }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("link", { name: "sues", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("link", { name: "   he \n llo 56 ", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<a href="https://playwright.dev">he llo 56</a>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { name: "Click me", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Click me</button>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { name: "Click \me", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Click me</button>`,
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { name: "Click \\me", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { name: "Click \\\me", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { name: "Click \\\\me", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-092 getByRole should accept regexp with v flag", () => {
    // Regression test for https://github.com/microsoft/playwright/issues/41457
    // [ts] page.setContent(`<button>Click me</button><button>Submit</button>`);
    // [ts] expect(page.getByRole("button", { name: "Click me" })).toHaveCount(1);
    // [ts] expect(page.getByRole("button", { name: "click me" })).toHaveCount(1);
    // [ts] expect(page.getByRole("button", { name: "Missing" })).toHaveCount(0, { timeout: 1000 });
    // [ts] });
    // [ts] test("SE-093 getByRole with description", () => {
    // [ts] page.setContent(`
    // [ts] <div role="alert" aria-label="Upload successful" aria-description="File doc-2025.pdf was uploaded successfully">Alert 1</div>
    // [ts] <div role="alert" aria-label="Upload successful" aria-description="File report-2026.pdf was uploaded successfully">Alert 2</div>
    // [ts] <div role="alert" aria-label="Invalid file" aria-description="File demo.doc has an invalid file format">Alert 3</div>
    // [ts] `);
    // description substring match (default)
    // [ts] expect.soft(page.getByRole("alert", { description: "doc-2025" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert 1",
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("alert", { description: "report-2026" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert 2",
    // [ts] ]);
    // description with name combined
    // [ts] expect.soft(page.getByRole("alert", { name: "Upload successful", description: "doc-2025" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert 1",
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("alert", { name: "Upload successful", description: "report-2026" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert 2",
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("alert", { name: "Invalid file", description: "doc-2025" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] ]);
    // description exact match
    // [ts] expect.soft(page.getByRole("alert", { description: "doc-2025", exact: true }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("alert", { description: "File doc-2025.pdf was uploaded successfully", exact: true }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert 1",
    // [ts] ]);
    // description regex match
    // [ts] expect.soft(page.getByRole("alert", { description: "report-\\d+" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert 2",
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("alert", { description: "uploaded successfully$" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert 1",
    // [ts] "Alert 2",
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-094 getByRole with description via aria-describedby", () => {
    // [ts] page.setContent(`
    // [ts] <button aria-describedby="desc1">Submit</button>
    // [ts] <span id="desc1">Submits the form data</span>
    // [ts] <button aria-describedby="desc2">Submit</button>
    // [ts] <span id="desc2">Saves as draft</span>
    // [ts] `);
    // [ts] expect.soft(page.getByRole("button", { name: "Submit", description: "form data" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Submit",
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { name: "Submit", description: "draft" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Submit",
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-095 getByRole with description via title fallback", () => {
    // [ts] page.setContent(`
    // [ts] <button title="Submits the form">Submit</button>
    // [ts] <button title="Resets the form">Reset</button>
    // [ts] `);
    // [ts] expect.soft(page.getByRole("button", { description: "Submits" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Submit",
    // [ts] ]);
    // [ts] expect.soft(page.getByRole("button", { description: "Resets" }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Reset",
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-096 getByRole with description whitespace normalization", () => {
    // [ts] page.setContent(`
    // [ts] <div role="alert" aria-description="File  doc-2025.pdf   was uploaded   successfully">Alert</div>
    // [ts] `);
    // [ts] expect.soft(page.getByRole("alert", { description: "  doc-2025.pdf \n was  uploaded " }).evaluateAll(els => els.map(e => e.textContent))).toEqual([
    // [ts] "Alert",
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-097 locators should not be affected by global and sticky regex flags", () => {
    // [ts] page.setContent(`
    // [ts] <div>foo</div><div>foo</div><div>foo</div>
    // [ts] <label>foo<input></label><label>foo<input></label><label>foo<input></label>
    // [ts] <button>foo</button><button>foo</button><button>foo</button>
    // [ts] <input placeholder=foo><input placeholder=foo><input placeholder=foo>
    // [ts] `);
    // [ts] expect(page.getByText("foo")).toHaveCount(9);
    // [ts] expect(page.getByText("foo")).toHaveCount(9);
    // [ts] expect(page.getByText("foo")).toHaveCount(9);
    // [ts] expect(page.getByLabel("foo")).toHaveCount(3);
    // [ts] expect(page.getByLabel("foo")).toHaveCount(3);
    // [ts] expect(page.getByLabel("foo")).toHaveCount(3);
    // [ts] expect(page.getByRole("button", { name: "foo" })).toHaveCount(3);
    // [ts] expect(page.getByRole("button", { name: "foo" })).toHaveCount(3);
    // [ts] expect(page.getByRole("button", { name: "foo" })).toHaveCount(3);
    // [ts] expect(page.getByPlaceholder("foo")).toHaveCount(3);
    // [ts] expect(page.getByPlaceholder("foo")).toHaveCount(3);
    // [ts] expect(page.getByPlaceholder("foo")).toHaveCount(3);
    // [ts] });
}
