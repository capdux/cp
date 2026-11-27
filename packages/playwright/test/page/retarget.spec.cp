// Migrated from TypeScript (page/retarget.spec.ts) via cap from -ts

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

fn giveItAChanceToResolve(page: Page) { return rafraf(page, 5); }







type Options = string; // TODO(cap-migrate): complex TS conditional/mapped type
fn optionsToAttributes(options: Options?) { return ` ${if (options?.disabled) "disabled" else ""} ${if (options?.hidden) "hidden" else ""} ${if (options?.readonly) "readonly" else ""} `; }
fn domInLabel(dom: string, options: Options? = null) { return `<label ${optionsToAttributes(options)}>Text ${dom}</label>`; }
fn domLabelFor(dom: string, options: Options? = null) { return `<label ${optionsToAttributes(options)} for="target"><h1>Text</h1></label>${dom}`; }
fn domStandalone(dom: string) { return dom; }
fn domInButton(dom: string, options: Options? = null) { return `<button ${optionsToAttributes(options)}>Button ${dom}</button>`; }
fn domInLink(dom: string, options: Options? = null) { return `<button ${optionsToAttributes(options)}>Button ${dom}</button>`; }








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RE-001 element state checks should work as expected for label with zero-sized input", () => {
    // [ts] page.setContent(`
    // [ts] <label>
    // [ts] Click me
    // [ts] <input disabled style="width:0;height:0;padding:0;margin:0;border:0;">
    // [ts] </label>
    // [ts] `);
    // Visible checks the label.
    // [ts] expect_eq(page.isVisible("text=Click me"), true);
    // [ts] expect_eq(page.isHidden("text=Click me"), false);
    // Enabled checks the input.
    // [ts] expect_eq(page.isEnabled("text=Click me"), false);
    // [ts] expect_eq(page.isDisabled("text=Click me"), true);
    // [ts] });
    // [ts] test("RE-002 should wait for enclosing disabled button", () => {
    // [ts] page.setContent("<button><span>Target</span></button>");
    // [ts] const span = (page.$("text=Target"));
    // [ts] let done = false;
    // [ts] const promise = span.waitForElementState("disabled").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] span.evaluate(span => (span.parentElement).disabled = true);
    // [ts] promise;
    // [ts] });
    // [ts] test("RE-003 should wait for enclosing button with a disabled fieldset", () => {
    // [ts] page.setContent("<fieldset disabled=true><button><span>Target</span></button></div>");
    // [ts] const span = (page.$("text=Target"));
    // [ts] let done = false;
    // [ts] const promise = span.waitForElementState("enabled").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] span.evaluate((span) => (span.parentElement.parentElement).disabled = false);
    // [ts] promise;
    // [ts] });
    // [ts] test("RE-004 should wait for enclosing enabled button", () => {
    // [ts] page.setContent("<button disabled><span>Target</span></button>");
    // [ts] const span = (page.$("text=Target"));
    // [ts] let done = false;
    // [ts] const promise = span.waitForElementState("enabled").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] span.evaluate(span => (span.parentElement).disabled = false);
    // [ts] promise;
    // [ts] });
    // [ts] test("RE-005 should check the box outside shadow dom label", () => {
    // [ts] page.setContent("<div></div>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
    // [ts-skip] const root = div.attachShadow({ mode: 'open' });
    // [ts-skip] const label = document.createElement('label');
    // [ts-skip] label.setAttribute('for', 'target');
    // [ts-skip] label.textContent = 'Click me';
    // [ts-skip] root.appendChild(label);
    // [ts-skip] const input = document.createElement('input');
    // [ts-skip] input.setAttribute('type', 'checkbox');
    // [ts-skip] input.setAttribute('id', 'target');
    // [ts-skip] root.appendChild(input);
    // [ts-skip] });
    // [ts] page.check("label");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.checked)).toBe(true);
    // [ts] });
    // [ts] test("RE-006 setInputFiles should work with label", () => {
    // [ts] page.setContent(`<label for=target>Choose a file</label><input id=target type=file>`);
    // [ts] page.setInputFiles("text=Choose a file", asset("file-to-upload.txt"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', (input: HTMLInputElement) => input.files!.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', (input: HTMLInputElement) => input.files?.[0].name)).toBe('file-to-upload.txt');
    // [ts] });
    // [ts] test("RE-007 enabled/disabled retargeting", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43634" } }, (_p) => {
    // [ts] const cases = [
    // [ts] { dom: domInLabel(`<input id=target>`), enabled: true, locator: "label" },
    // [ts] { dom: domLabelFor(`<input id=target>`), enabled: true, locator: "label" },
    // [ts] { dom: domStandalone(`<input id=target>`), enabled: true, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target>`), enabled: true, locator: "input" },
    // [ts] { dom: domInLink(`<input id=target>`), enabled: true, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target>`, { disabled: true }), enabled: true, locator: "input" },
    // [ts] { dom: domInLabel(`<li role=menuitem id=target aria-disabled=false></li>`), enabled: true, locator: "li" },
    // [ts] { dom: domInLabel(`<input id=target disabled>`), enabled: false, locator: "label" },
    // [ts] { dom: domLabelFor(`<input id=target disabled>`), enabled: false, locator: "label" },
    // [ts] { dom: domStandalone(`<input id=target disabled>`), enabled: false, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target disabled>`), enabled: false, locator: "input" },
    // [ts] { dom: domInLink(`<input id=target disabled>`), enabled: false, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target disabled>`, { disabled: true }), enabled: false, locator: "input" },
    // [ts] { dom: domInLabel(`<li role=menuitem id=target aria-disabled=true></li>`), enabled: false, locator: "li" },
    // [ts] { dom: domInLabel(`<select><option disabled>Other employees</option><option>Alex</option></select>`), enabled: false, locator: "option >> nth=0" },
    // [ts] { dom: domInLabel(`<select><optgroup disabled label="Other employees"><option>Alex</option></optgroup></select>`), enabled: false, locator: "optgroup" },
    // [ts] ];
    // [ts] for (const { dom, enabled, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" should be enabled=${enabled}`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] const handle = (page.$(locator));
    // [ts] expect_eq(target.isEnabled(), enabled);
    // [ts] expect_eq(target.isDisabled(), !enabled);
    // [ts] if (enabled) {
    // [ts] expect(target).toBeEnabled();
    // [ts] expect(target).not.toBeDisabled();
    // [ts] handle.waitForElementState("enabled");
    // [ts] } else {
    // [ts] expect(target).not.toBeEnabled();
    // [ts] expect(target).toBeDisabled();
    // [ts] handle.waitForElementState("disabled");
    // [ts] }
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-008 visible/hidden retargeting", () => {
    // [ts] const cases = [
    // [ts] { dom: domInLabel(`<span id=target>content</span>`), visible: true, locator: "label" },
    // [ts] { dom: domInLabel(`<span id=target hidden>content</span>`), visible: true, locator: "label" },
    // [ts] { dom: domLabelFor(`<span id=target>content</span>`), visible: true, locator: "label" },
    // [ts] { dom: domLabelFor(`<span id=target hidden>content</span>`), visible: true, locator: "label" },
    // [ts] { dom: domStandalone(`<span id=target>content</span>`), visible: true, locator: "span" },
    // [ts] { dom: domInButton(`<span id=target>content</span>`), visible: true, locator: "span" },
    // [ts] { dom: domInLink(`<span id=target>content</span>`), visible: true, locator: "span" },
    // [ts] { dom: domInLabel(`<span id=target>content</span>`, { hidden: true }), visible: false, locator: "label" },
    // [ts] { dom: domLabelFor(`<span id=target>content</span>`, { hidden: true }), visible: false, locator: "label" },
    // [ts] { dom: domStandalone(`<span id=target hidden>content</span>`), visible: false, locator: "span" },
    // [ts] { dom: domInButton(`<span id=target hidden>content</span>`), visible: false, locator: "span" },
    // [ts] { dom: domInButton(`<span id=target>content</span>`, { hidden: true }), visible: false, locator: "span" },
    // [ts] { dom: domInLink(`<span id=target hidden>content</span>`), visible: false, locator: "span" },
    // [ts] { dom: domInLink(`<span id=target>content</span>`, { hidden: true }), visible: false, locator: "span" },
    // [ts] ];
    // [ts] for (const { dom, visible, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" should be visible=${visible}`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] const handle = (page.$(locator));
    // [ts] expect_eq(target.isVisible(), visible);
    // [ts] expect_eq(target.isHidden(), !visible);
    // [ts] if (visible) {
    // [ts] expect(target).toBeVisible();
    // [ts] expect(target).not.toBeHidden();
    // [ts] handle.waitForElementState("visible");
    // [ts] } else {
    // [ts] expect(target).not.toBeVisible();
    // [ts] expect(target).toBeHidden();
    // [ts] handle.waitForElementState("hidden");
    // [ts] }
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-009 editable retargeting", () => {
    // [ts] const cases = [
    // [ts] { dom: domInLabel(`<input id=target>`), editable: true, locator: "label" },
    // [ts] { dom: domLabelFor(`<input id=target>`), editable: true, locator: "label" },
    // [ts] { dom: domStandalone(`<input id=target>`), editable: true, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target>`), editable: true, locator: "input" },
    // [ts] { dom: domInLink(`<input id=target>`), editable: true, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target>`, { readonly: true }), editable: true, locator: "input" },
    // [ts] { dom: domInLabel(`<input id=target readonly>`), editable: false, locator: "label" },
    // [ts] { dom: domLabelFor(`<input id=target readonly>`), editable: false, locator: "label" },
    // [ts] { dom: domStandalone(`<input id=target readonly>`), editable: false, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target readonly>`), editable: false, locator: "input" },
    // [ts] { dom: domInLink(`<input id=target readonly>`), editable: false, locator: "input" },
    // [ts] { dom: domInButton(`<input id=target readonly>`, { readonly: true }), editable: false, locator: "input" },
    // [ts] ];
    // [ts] for (const { dom, editable, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" should be editable=${editable}`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] const handle = (page.$(locator));
    // [ts] expect_eq(target.isEditable(), editable);
    // [ts] if (editable) {
    // [ts] expect(target).toBeEditable();
    // [ts] handle.waitForElementState("editable");
    // [ts] } else {
    // [ts] expect(target).not.toBeEditable();
    // [ts] }
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-010 input value retargeting", () => {
    // [ts] const cases = [
    // [ts] { dom: domInLabel(`<input id=target>`), locator: "label" },
    // [ts] { dom: domLabelFor(`<input id=target>`), locator: "label" },
    // [ts] { dom: domStandalone(`<input id=target>`), locator: "input" },
    // [ts] { dom: domInButton(`<input id=target>`), locator: "input" },
    // [ts] { dom: domInLink(`<input id=target>`), locator: "input" },
    // [ts] { dom: domInButton(`<input id=target>`), locator: "input" },
    // [ts] ];
    // [ts] for (const { dom, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" input value`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] const handle = (page.$(locator));
    // [ts] expect_eq(target.inputValue(), "");
    // [ts] expect_eq(handle.inputValue(), "");
    // [ts] expect(target).toHaveValue("");
    // [ts] target.fill("foo");
    // [ts] expect_eq(target.inputValue(), "foo");
    // [ts] expect_eq(handle.inputValue(), "foo");
    // [ts] expect(target).toHaveValue("foo");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#target', (input: HTMLInputElement) => input.value = 'bar');
    // [ts] expect_eq(target.inputValue(), "bar");
    // [ts] expect_eq(handle.inputValue(), "bar");
    // [ts] expect(target).toHaveValue("bar");
    // [ts] target.selectText();
    // [ts] expect_eq(page.evaluate(() => window.getSelection().toString()), "bar");
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-011 selection retargeting", () => {
    // [ts] const cases = [
    // [ts] { dom: domStandalone(`<div contenteditable id=target>content</div>`), locator: "div" },
    // [ts] { dom: domInButton(`<div contenteditable id=target>content</div>`), locator: "div" },
    // [ts] { dom: domInLink(`<div contenteditable id=target>content</div>`), locator: "div" },
    // [ts] { dom: domInButton(`<div contenteditable id=target>content</div>`), locator: "div" },
    // [ts] ];
    // [ts] for (const { dom, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" text selection`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] const handle = (page.$(locator));
    // [ts] expect_eq(target.isEditable(), true);
    // [ts] expect_eq(handle.isEditable(), true);
    // [ts] expect(page.locator("#target")).toHaveText("content");
    // [ts] target.fill("foo");
    // [ts] expect(page.locator("#target")).toHaveText("foo");
    // [ts] target.selectText();
    // [ts] expect_eq(page.evaluate(() => window.getSelection().toString()), "foo");
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-012 select options retargeting", () => {
    // [ts] const cases = [
    // [ts] { dom: domInLabel(`<select id=target multiple><option value=dog selected>Dog</option><option value=cat>Cat</option></select>`), locator: "label" },
    // [ts] { dom: domLabelFor(`<select id=target multiple><option value=dog selected>Dog</option><option value=cat>Cat</option></select>`), locator: "label" },
    // [ts] { dom: domStandalone(`<select id=target multiple><option value=dog selected>Dog</option><option value=cat>Cat</option></select>`), locator: "select" },
    // [ts] { dom: domInButton(`<select id=target multiple><option value=dog selected>Dog</option><option value=cat>Cat</option></select>`), locator: "select" },
    // [ts] { dom: domInLink(`<select id=target multiple><option value=dog selected>Dog</option><option value=cat>Cat</option></select>`), locator: "select" },
    // [ts] { dom: domInButton(`<select id=target multiple><option value=dog selected>Dog</option><option value=cat>Cat</option></select>`), locator: "select" },
    // [ts] ];
    // [ts] for (const { dom, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" select option`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] const handle = (page.$(locator));
    // [ts] expect_eq(target.inputValue(), "dog");
    // [ts] expect_eq(handle.inputValue(), "dog");
    // [ts] expect(target).toHaveValue("dog");
    // [ts] expect(target).toHaveValues(["dog"]);
    // [ts] target.selectOption("cat");
    // [ts] expect_eq(target.inputValue(), "cat");
    // [ts] expect_eq(handle.inputValue(), "cat");
    // [ts] expect(target).toHaveValue("cat");
    // [ts] expect(target).toHaveValues(["cat"]);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-013 direct actions retargeting", () => {
    // [ts] const cases = [
    // [ts] { dom: domInLabel(`<div>content</div><input id=target value=oh>`), locator: "div" },
    // [ts] { dom: domLabelFor(`<div>content</div><input id=target value=oh>`), locator: "div" },
    // [ts] { dom: domStandalone(`<div>content</div>`), locator: "div" },
    // [ts] { dom: domInButton(`<div>content</div>`), locator: "div" },
    // [ts] { dom: domInLink(`<div>content</div>`), locator: "div" },
    // [ts] { dom: domInButton(`<div>content</div>`), locator: "div" },
    // [ts] ];
    // [ts] for (const { dom, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" direct actions`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] expect_eq(target.innerText(), "content");
    // [ts] expect_eq(target.textContent(), "content");
    // [ts] expect(target).toHaveText("content");
    // [ts] expect(target).toContainText("content");
    // [ts] expect(target).not.toBeFocused();
    // [ts] expect(target).toHaveCount(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => (div as any).foo = 'bar');
    // [ts] expect(target).toHaveJSProperty("foo", "bar");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => div.classList.add('cls'));
    // [ts] expect(target).toHaveClass("cls");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => div.id = 'myid');
    // [ts] expect(target).toHaveId("myid");
    // [ts] expect(target).toHaveAttribute("id", "myid");
    // [ts] expect_eq(target.getAttribute("id"), "myid");
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-014 check retargeting", () => {
    // [ts] const cases = [
    // [ts] { dom: domInLabel(`<input type=checkbox id=target>`), locator: "label" },
    // [ts] { dom: domLabelFor(`<input type=checkbox id=target>`), locator: "label" },
    // [ts] { dom: domStandalone(`<input type=checkbox id=target>`), locator: "input" },
    // [ts] { dom: domInButton(`<input type=checkbox id=target>`), locator: "input" },
    // [ts] { dom: domInLink(`<input type=checkbox id=target>`), locator: "input" },
    // [ts] { dom: domInButton(`<input type=checkbox id=target>`), locator: "input" },
    // [ts] ];
    // [ts] for (const { dom, locator } of cases) {
    // [ts] it.step(`"${locator}" in "${dom}" check`, () => {
    // [ts] page.setContent(dom);
    // [ts] const target = page.locator(locator);
    // [ts] expect_eq(target.isChecked(), false);
    // [ts] expect(target).not.toBeChecked();
    // [ts] expect(target).toBeChecked({ checked: false });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', (input: HTMLInputElement) => input.checked = true);
    // [ts] expect_eq(target.isChecked(), true);
    // [ts] expect(target).toBeChecked();
    // [ts] expect(target).toBeChecked({ checked: true });
    // [ts] target.uncheck();
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', (input: HTMLInputElement) => input.checked)).toBe(false);
    // [ts] target.check();
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', (input: HTMLInputElement) => input.checked)).toBe(true);
    // [ts] target.setChecked(false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', (input: HTMLInputElement) => input.checked)).toBe(false);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("RE-015 should not retarget anchor into parent label", () => {
    // [ts] page.setContent(`
    // [ts] <label disabled>Text<a href='#' onclick='window.__clicked=1'>Target</a></label>
    // [ts] `);
    // [ts] page.locator("a").click();
    // [ts] expect_eq(page.evaluate("window.__clicked"), 1);
    // [ts] page.setContent(`
    // [ts] <input type="radio" id="input-id" checked disabled />
    // [ts] <label for="input-id">Text<a href='#' onclick='window.__clicked=2'>Target</a></label>
    // [ts] `);
    // [ts] page.locator("a").click();
    // [ts] expect_eq(page.evaluate("window.__clicked"), 2);
    // [ts] });
}
