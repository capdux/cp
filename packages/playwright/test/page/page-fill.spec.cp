// Migrated from TypeScript (page/page-fill.spec.ts) via cap from -ts

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

// [cap-migrate] import { Page } from "cap:test";
import { test as it, expect, rafraf } from "./pageTest.cp";

fn giveItAChanceToFill(page: Page) { return rafraf(page, 5); }










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("FI-001 should fill textarea @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.fill("textarea", "some value");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] });
    // [ts] test("FI-002 should fill input", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.fill("input", "some value");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] });
    // [ts] test("FI-003 should throw on unsupported inputs", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] for (const type of ["button", "checkbox", "file", "image", "radio", "reset", "submit"]) {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', (input, type) => input.setAttribute('type', type), type);
    // [ts] let error = null;
    // [ts] page.fill("input", "").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Input of type "${type}" cannot be filled`) != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("FI-004 should fill different input types", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] for (const type of ["password", "search", "tel", "text", "url", "invalid-type"]) {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', (input, type) => input.setAttribute('type', type), type);
    // [ts] page.fill("input", "text " + type);
    // [ts] expect_eq(page.evaluate(() => window["result"]), "text " + type);
    // [ts] }
    // [ts] });
    // [ts] test("FI-005 should fill range input", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43328" },
    // [ts] }, (_p) => {
    // [ts] page.setContent("<input type=range min=0 max=100 value=50>");
    // [ts] page.fill("input", "42");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('42');
    // Valid numbers that the browser may serialize differently.
    // [ts] for (const [value, expected] of [["42.0", 42], ["1e1", 10]]) { page.fill("input", value); expect_eq(page.$eval("input", (input) => input.valueAsNumber), expected); }
    // [ts] });
    // [ts] test("FI-006 should fill color input", () => {
    // [ts] page.setContent("<input type=color value=\"#e66465\">");
    // [ts] page.fill("input", "#aaaaaa");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('#aaaaaa');
    // [ts] });
    // [ts] test("FI-007 should fill color input case insensitive", () => {
    // [ts] page.setContent("<input type=color value=\"#e66465\">");
    // [ts] page.fill("input", "#AbCd00");
    // [ts] if (browserName == "webkit" && isWindows && channel != "webkit-wsl")
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('#AbCd00');
    // [ts] else {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('#abcd00');
    // [ts] });
    // [ts] test("FI-008 should fill date input after clicking", () => {
    // [ts] page.setContent("<input type=date>");
    // [ts] page.click("input");
    // [ts] page.fill("input", "2020-03-02");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('2020-03-02');
    // [ts] });
    // [ts] for (const [type, value] of Object.entries({
    // [ts] "color": "#aaaaaa",
    // [ts] "date": "2020-03-02",
    // [ts] "time": "13:15",
    // [ts] "datetime-local": "2020-03-02T13:15:30",
    // [ts] "month": "2020-03",
    // [ts] "range": "42",
    // [ts] "week": "2020-W50"
    // [ts] })) {
    // [ts] }
    // [ts] test(`FI-009 input event.composed should be true and cross shadow dom boundary - ${type}`, () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28726" });
    // [ts] test(browserName != "chromium" && ["month", "week"].includes(type), "Some browser/platforms do not implement certain input types");
    // [ts] test(browserName == "webkit" && isWindows && ["color", "date", "time", "datetime-local"].includes(type), "Some browser/platforms do not implement certain input types");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<body><script>
    // [ts] const div = document.createElement('div');
    // [ts] const shadowRoot = div.attachShadow({mode: 'open'});
    // [ts] shadowRoot.innerHTML = '<input type=${type}></input>';
    // [ts] document.body.appendChild(div);
    // [ts] </script></body>`);
    // [ts] page.locator("body").evaluate(select => {
    // [ts] (window).firedBodyEvents = [];
    // [ts] for (const event of ["input", "change"]) {
    // [ts] select.addEventListener(event, e => {
    // [ts] (window).firedBodyEvents.push(e.type + ":" + e.composed);
    // [ts] }, false);
    // [ts] }
    // [ts] });
    // [ts] page.locator("input").evaluate(select => {
    // [ts] (window).firedEvents = [];
    // [ts] for (const event of ["input", "change"]) {
    // [ts] select.addEventListener(event, e => {
    // [ts] (window).firedEvents.push(e.type + ":" + e.composed);
    // [ts] }, false);
    // [ts] }
    // [ts] });
    // [ts] page.locator("input").fill(value);
    // [ts] expect_eq(page.evaluate(() => window["firedEvents"]), ["input:true", "change:false"]);
    // [ts] expect_eq(page.evaluate(() => window["firedBodyEvents"]), ["input:true"]);
    // [ts] });
    // [ts] }
    // [ts] test("FI-010 should fill time input", () => {
    // [ts] page.setContent("<input type=time>");
    // [ts] page.fill("input", "13:15");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('13:15');
    // [ts] });
    // [ts] test("FI-011 should fill month input", () => {
    // [ts] page.setContent("<input type=month>");
    // [ts] page.fill("input", "2020-07");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('2020-07');
    // [ts] });
    // [ts] test("FI-012 should fill week input", () => {
    // [ts] page.setContent("<input type=week>");
    // [ts] page.fill("input", "2020-W50");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('2020-W50');
    // [ts] });
    // [ts] test("FI-013 should fill datetime-local input", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43370" },
    // [ts] }, (_p) => {
    // [ts] page.setContent("<input type=datetime-local>");
    // [ts] page.fill("input", "2020-03-02T05:15");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('2020-03-02T05:15');
    // The browser omits zero seconds. WebKit on Windows does not implement datetime-local inputs and keeps the value as is.
    // [ts] page.fill("input", "2020-03-02T06:20:00");
    // [ts] if (browserName == "webkit" && isWindows && channel != "webkit-wsl")
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('2020-03-02T06:20:00');
    // [ts] else {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('2020-03-02T06:20');
    // [ts] });
    // [ts] test("FI-014 should fill contenteditable", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.fill("div[contenteditable]", "some value");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div[contenteditable]', div => div.textContent)).toBe('some value');
    // [ts] });
    // [ts] test("FI-015 should fill contenteditable with new lines", () => {
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "Firefox does not handle new lines in contenteditable");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<div contenteditable="true"></div>`);
    // [ts] page.locator("div[contenteditable]").fill("John\nDoe");
    // [ts] expect_eq(page.locator("div[contenteditable]").innerText(), "John\nDoe");
    // [ts] });
    // [ts] test("FI-016 should not double-fill in contenteditable with beforeinput handler in Firefox", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/36715"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "https://github.com/microsoft/playwright/issues/36715");
    // [ts] page.setContent(`
    // [ts] <div id="editor" contenteditable="true"></div>
    // [ts] <script>
    // [ts] const editor = document.getElementById('editor');
    // [ts] editor.addEventListener('beforeinput', (event) => {
    // [ts] event.preventDefault();
    // [ts] editor.textContent = event.data;
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] const locator = page.locator("#editor");
    // [ts] const testValue = "Playwright";
    // [ts] locator.fill(testValue);
    // [ts] expect(locator).toHaveText(testValue);
    // [ts] });
    // [ts] test("FI-017 should fill elements with existing value and selection", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => input.value = 'value one');
    // [ts] page.fill("input", "another value");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "another value");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
    // [ts-skip] input.selectionStart = 1;
    // [ts-skip] input.selectionEnd = 2;
    // [ts-skip] });
    // [ts] page.fill("input", "maybe this one");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "maybe this one");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div[contenteditable]', div => {
    // [ts-skip] div.innerHTML = 'some text <span>some more text<span> and even more text';
    // [ts-skip] const range = document.createRange();
    // [ts-skip] range.selectNodeContents(div.querySelector('span'));
    // [ts-skip] const selection = window.getSelection();
    // [ts-skip] selection.removeAllRanges();
    // [ts-skip] selection.addRange(range);
    // [ts-skip] });
    // [ts] page.fill("div[contenteditable]", "replace with this");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div[contenteditable]', div => div.textContent)).toBe('replace with this');
    // [ts] });
    // [ts] test("FI-018 should throw nice error without injected script stack when element is not an <input>", () => {
    // [ts] let error = null;
    // [ts] page.setContent(`<select><option>value1</option></select>`);
    // [ts] page.fill("select", "").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Error: Element is not an <input>, <textarea> or [contenteditable] element\nCall log:") != null, true);
    // [ts] });
    // [ts] test("FI-019 should throw if passed a non-string value", () => {
    // [ts] let error = null;
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // @ts-expect-error fill only accepts string values
    // [ts] page.fill("textarea", 123).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "value: expected string, got number") != null, true);
    // [ts] });
    // [ts] test("FI-020 should retry on disabled element", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', i => i.disabled = true);
    // [ts] let done = false;
    // [ts] const promise = page.fill("input", "some value").then(() => done = true);
    // [ts] giveItAChanceToFill(page);
    // [ts] expect_eq(done, false);
    // [ts] expect_eq(page.evaluate(() => window["result"]), "");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', i => i.disabled = false);
    // [ts] promise;
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] });
    // [ts] test("FI-021 should retry on readonly element", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', i => i.readOnly = true);
    // [ts] let done = false;
    // [ts] const promise = page.fill("textarea", "some value").then(() => done = true);
    // [ts] giveItAChanceToFill(page);
    // [ts] expect_eq(done, false);
    // [ts] expect_eq(page.evaluate(() => window["result"]), "");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('textarea', i => i.readOnly = false);
    // [ts] promise;
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] });
    // [ts] test("FI-022 should retry on invisible element", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', i => i.style.display = 'none');
    // [ts] let done = false;
    // [ts] const promise = page.fill("input", "some value").then(() => done = true);
    // [ts] giveItAChanceToFill(page);
    // [ts] expect_eq(done, false);
    // [ts] expect_eq(page.evaluate(() => window["result"]), "");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', i => i.style.display = 'inline');
    // [ts] promise;
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] });
    // [ts] test("FI-023 should be able to fill the body", () => {
    // [ts] page.setContent(`<body contentEditable="true"></body>`);
    // [ts] page.fill("body", "some value");
    // [ts] expect_eq(page.evaluate(() => document.body.textContent), "some value");
    // [ts] });
    // [ts] test("FI-024 should fill fixed position input", () => {
    // [ts] page.setContent(`<input style='position: fixed;' />`);
    // [ts] page.fill("input", "some value");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("input").value), "some value");
    // [ts] });
    // [ts] test("FI-025 should be able to fill when focus is in the wrong frame", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43483" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div contentEditable="true"></div>
    // [ts] <iframe srcdoc="<body contenteditable><p>old</p></body>"></iframe>
    // [ts] `);
    // [ts] page.focus("iframe");
    // [ts] page.fill("div", "some value");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div', d => d.textContent)).toBe('some value');
    // [ts] const paragraph = page.frameLocator("iframe").locator("p");
    // [ts] paragraph.fill("new");
    // [ts] expect(paragraph).toHaveText("new");
    // [ts] });
    // [ts] test("FI-026 should be able to fill the input[type=number]", () => {
    // [ts] page.setContent(`<input id="input" type="number"></input>`);
    // [ts] page.fill("input", "42");
    // [ts] expect_eq(page.evaluate(() => window["input"].value), "42");
    // [ts] });
    // [ts] test("FI-027 should be able to fill exponent into the input[type=number]", () => {
    // [ts] page.setContent(`<input id="input" type="number"></input>`);
    // [ts] page.fill("input", "-10e5");
    // [ts] expect_eq(page.evaluate(() => window["input"].value), "-10e5");
    // [ts] });
    // [ts] test("FI-028 should be able to fill input[type=number] with empty string", () => {
    // [ts] page.setContent(`<input id="input" type="number" value="123"></input>`);
    // [ts] page.fill("input", "");
    // [ts] expect_eq(page.evaluate(() => window["input"].value), "");
    // [ts] });
    // [ts] test("FI-029 should not be able to fill text into the input[type=number]", () => {
    // [ts] page.setContent(`<input id="input" type="number"></input>`);
    // [ts] let error = null;
    // [ts] page.fill("input", "abc").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Cannot type text into input[type=number]") != null, true);
    // [ts] });
    // [ts] test("FI-030 should be able to clear using fill()", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.fill("input", "some value");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] page.fill("input", "");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "");
    // [ts] });
    // [ts] test("FI-031 should not throw when fill causes navigation", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.setContent("<input type=date>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', select => select.addEventListener('input', () => window.location.href = '/empty.html'));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.fill("input", "2020-03-02"),
    // [ts] page.waitForNavigation(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, page.url(), "empty.html") != null, true);
    // [ts] });
    // [ts] test("FI-032 fill back to back", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15925" });
    // [ts] page.setContent(`<input id="one"></input><input id="two"></input>`);
    // [ts] page.fill("id=one", "first value");
    // [ts] page.fill("id=two", "second value");
    // [ts] expect(page.locator("id=one")).toHaveValue("first value");
    // [ts] expect(page.locator("id=two")).toHaveValue("second value");
    // [ts] });
    // [ts] test("FI-033 should fill contenteditable with focus handler that collapses selection", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/39492"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div contenteditable="true">initial text</div>
    // [ts] <script>
    // [ts] const editor = document.querySelector('[contenteditable]');
    // [ts] editor.addEventListener('focus', () => {
    // [ts] const selection = window.getSelection();
    // [ts] if (selection.rangeCount > 0)
    // [ts] selection.collapseToEnd();
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] page.fill("div[contenteditable]", "some value");
    // [ts] expect_eq(page.locator("div[contenteditable]").textContent(), "some value");
    // [ts] });
    // [ts] test("FI-034 should retry on inert element", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42967" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div inert id=container>
    // [ts] <input id=target>
    // [ts] </div>
    // [ts] <input id=outside>
    // [ts] `);
    // [ts] page.focus("#outside");
    // [ts] let done = false;
    // [ts] const promise = page.fill("#target", "some value").then(() => done = true);
    // [ts] giveItAChanceToFill(page);
    // [ts] expect_eq(done, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', (i: HTMLInputElement) => i.value)).toBe('');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#outside', (i: HTMLInputElement) => i.value)).toBe('');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#container', e => e.removeAttribute('inert'));
    // [ts] promise;
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', (i: HTMLInputElement) => i.value)).toBe('some value');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#outside', (i: HTMLInputElement) => i.value)).toBe('');
    // [ts] });
    // [ts] test("FI-036 should fail with a nice error on inert element", () => {
    // [ts] page.setContent(`<div inert><input></div>`);
    // [ts] const error = page.fill("input", "some value", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is inert") != null, true);
    // [ts] });
    // [ts] }
}
