// Migrated from TypeScript (page/selectors-role.spec.ts) via cap from -ts

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

import { test, expect } from "./pageTest.cp";















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("SE-050 should detect roles", () => {
    // [ts] page.setContent(`
    // [ts] <button>Hello</button>
    // [ts] <select multiple="" size="2"></select>
    // [ts] <select></select>
    // [ts] <h3>Heading</h3>
    // [ts] <details><summary>Hello</summary></details>
    // [ts] <div role="dialog">I am a dialog</div>
    // [ts] `);
    // [ts] expect(page.locator(`role=button`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hello</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=listbox`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<select multiple="" size="2"></select>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=combobox`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<select></select>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=heading`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<h3>Heading</h3>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=group`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<details><summary>Hello</summary></details>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=dialog`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="dialog">I am a dialog</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=menuitem`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] ]);
    // [ts] expect(page.getByRole("menuitem").evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-051 should support selected", () => {
    // [ts] page.setContent(`
    // [ts] <select>
    // [ts] <option>Hi</option>
    // [ts] <option selected>Hello</option>
    // [ts] </select>
    // [ts] <div>
    // [ts] <div role="option" aria-selected="true">Hi</div>
    // [ts] <div role="option" aria-selected="false">Hello</div>
    // [ts] </div>
    // [ts] `);
    // [ts] expect(page.locator(`role=option[selected]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option selected="">Hello</option>`,
    // [ts] `<div role="option" aria-selected="true">Hi</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=option[selected=true]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option selected="">Hello</option>`,
    // [ts] `<div role="option" aria-selected="true">Hi</div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("option", { selected: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option selected="">Hello</option>`,
    // [ts] `<div role="option" aria-selected="true">Hi</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=option[selected=false]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option>Hi</option>`,
    // [ts] `<div role="option" aria-selected="false">Hello</div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("option", { selected: false }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option>Hi</option>`,
    // [ts] `<div role="option" aria-selected="false">Hello</div>`,
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-052 should support checked", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43662" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <input type=checkbox>
    // [ts] <input type=checkbox checked>
    // [ts] <input type=checkbox indeterminate>
    // [ts] <div role=checkbox aria-checked="true">Hi</div>
    // [ts] <div role=checkbox aria-checked="false">Hello</div>
    // [ts] <div role=checkbox>Unknown</div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('[indeterminate]', input => (input as HTMLInputElement).indeterminate = true);
    // [ts] expect(page.locator(`role=checkbox[checked]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input type="checkbox" checked="">`,
    // [ts] `<div role="checkbox" aria-checked="true">Hi</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=checkbox[checked=true]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input type="checkbox" checked="">`,
    // [ts] `<div role="checkbox" aria-checked="true">Hi</div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("checkbox", { checked: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input type="checkbox" checked="">`,
    // [ts] `<div role="checkbox" aria-checked="true">Hi</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=checkbox[checked=false]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input type="checkbox">`,
    // [ts] `<div role="checkbox" aria-checked="false">Hello</div>`,
    // [ts] `<div role="checkbox">Unknown</div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("checkbox", { checked: false }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input type="checkbox">`,
    // [ts] `<div role="checkbox" aria-checked="false">Hello</div>`,
    // [ts] `<div role="checkbox">Unknown</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=checkbox[checked="mixed"]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input type="checkbox" indeterminate="">`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=checkbox`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input type="checkbox">`,
    // [ts] `<input type="checkbox" checked="">`,
    // [ts] `<input type="checkbox" indeterminate="">`,
    // [ts] `<div role="checkbox" aria-checked="true">Hi</div>`,
    // [ts] `<div role="checkbox" aria-checked="false">Hello</div>`,
    // [ts] `<div role="checkbox">Unknown</div>`,
    // [ts] ]);
    // [ts] page.setContent(`
    // [ts] <div role="checkbox" aria-checked="True" aria-readonly="True"
    // [ts] onclick="this.setAttribute('aria-checked', String(this.getAttribute('aria-checked').toLowerCase() !== 'true'))">Remember me</div>
    // [ts] <button aria-pressed="True">Bold</button>
    // [ts] <button aria-expanded="True">Menu</button>
    // [ts] <div role="Button" tabindex="0">Save</div>
    // [ts] `);
    // [ts] const checkbox = page.getByRole("checkbox");
    // [ts] expect_eq(page.getByRole("checkbox", { checked: true }).count(), 1);
    // [ts] expect_eq(page.getByRole("button", { pressed: true }).count(), 1);
    // [ts] expect_eq(page.getByRole("button", { expanded: true }).count(), 1);
    // [ts] expect_eq(page.getByRole("button", { name: "Save" }).count(), 1);
    // [ts] expect(checkbox).toBeChecked();
    // [ts] expect(checkbox).not.toBeEditable();
    // [ts] expect(page.locator("body")).toMatchAriaSnapshot(`
    // [ts] - checkbox "Remember me" [checked]
    // [ts] - button "Bold" [pressed]
    // [ts] - button "Menu" [expanded]
    // [ts] - button "Save"
    // [ts] `);
    // [ts] checkbox.check();
    // [ts] expect(checkbox).toHaveAttribute("aria-checked", "True");
    // [ts] checkbox.evaluate(element => element.setAttribute("aria-checked", "Mixed"));
    // [ts] page.getByRole("button", { name: "Bold" }).evaluate(element => element.setAttribute("aria-pressed", "Mixed"));
    // [ts] expect_eq(page.locator("role=checkbox[checked=mixed]").count(), 1);
    // [ts] expect_eq(page.locator("role=button[pressed=mixed]").count(), 1);
    // [ts] });
    // [ts] test("SE-053 should support pressed", () => {
    // [ts] page.setContent(`
    // [ts] <button>Hi</button>
    // [ts] <button aria-pressed="true">Hello</button>
    // [ts] <button aria-pressed="false">Bye</button>
    // [ts] <button aria-pressed="mixed">Mixed</button>
    // [ts] `);
    // [ts] expect(page.locator(`role=button[pressed]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button aria-pressed="true">Hello</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[pressed=true]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button aria-pressed="true">Hello</button>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { pressed: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button aria-pressed="true">Hello</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[pressed=false]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button aria-pressed="false">Bye</button>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { pressed: false }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button aria-pressed="false">Bye</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[pressed="mixed"]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button aria-pressed="mixed">Mixed</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button aria-pressed="true">Hello</button>`,
    // [ts] `<button aria-pressed="false">Bye</button>`,
    // [ts] `<button aria-pressed="mixed">Mixed</button>`,
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-054 should support expanded", () => {
    // [ts] page.setContent(`
    // [ts] <div role="treeitem">Hi</div>
    // [ts] <div role="treeitem" aria-expanded="true">Hello</div>
    // [ts] <div role="treeitem" aria-expanded="false">Bye</div>
    // [ts] `);
    // [ts] expect(page.locator("role=treeitem").evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem">Hi</div>`,
    // [ts] `<div role="treeitem" aria-expanded="true">Hello</div>`,
    // [ts] `<div role="treeitem" aria-expanded="false">Bye</div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("treeitem").evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem">Hi</div>`,
    // [ts] `<div role="treeitem" aria-expanded="true">Hello</div>`,
    // [ts] `<div role="treeitem" aria-expanded="false">Bye</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=treeitem[expanded]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem" aria-expanded="true">Hello</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=treeitem[expanded=true]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem" aria-expanded="true">Hello</div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("treeitem", { expanded: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem" aria-expanded="true">Hello</div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=treeitem[expanded=false]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem" aria-expanded="false">Bye</div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("treeitem", { expanded: false }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem" aria-expanded="false">Bye</div>`,
    // [ts] ]);
    // Workaround for expanded="none".
    // [ts] expect(page.locator(`[role=treeitem]:not([aria-expanded])`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="treeitem">Hi</div>`,
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-055 should support disabled", () => {
    // [ts] page.setContent(`
    // [ts] <button>Hi</button>
    // [ts] <button disabled>Bye</button>
    // [ts] <button aria-disabled="true">Hello</button>
    // [ts] <button aria-disabled="false">Oh</button>
    // [ts] <fieldset disabled>
    // [ts] <button>Yay</button>
    // [ts] </fieldset>
    // [ts] <select>
    // [ts] <optgroup disabled>
    // [ts] <option>one</option>
    // [ts] </optgroup>
    // [ts] <optgroup>
    // [ts] <option>two</option>
    // [ts] </optgroup>
    // [ts] <option disabled>three</option>
    // [ts] </select>
    // [ts] `);
    // [ts] expect(page.locator(`role=button[disabled]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button disabled="">Bye</button>`,
    // [ts] `<button aria-disabled="true">Hello</button>`,
    // [ts] `<button>Yay</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[disabled=true]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button disabled="">Bye</button>`,
    // [ts] `<button aria-disabled="true">Hello</button>`,
    // [ts] `<button>Yay</button>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { disabled: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button disabled="">Bye</button>`,
    // [ts] `<button aria-disabled="true">Hello</button>`,
    // [ts] `<button>Yay</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[disabled=false]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button aria-disabled="false">Oh</button>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { disabled: false }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button aria-disabled="false">Oh</button>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("option", { disabled: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option>one</option>`,
    // [ts] `<option disabled="">three</option>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("option", { disabled: false }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option>two</option>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("option").evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option>one</option>`,
    // [ts] `<option>two</option>`,
    // [ts] `<option disabled="">three</option>`,
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-056 should inherit disabled from the ancestor", () => {
    // [ts] page.setContent(`
    // [ts] <span aria-disabled="true">
    // [ts] <button>Click me!</button>
    // [ts] </span>
    // [ts] `);
    // [ts] expect(page.locator("button")).toBeDisabled();
    // [ts] page.setContent(`
    // [ts] <span aria-disabled="true">
    // [ts] <h1>Heading</h1>
    // [ts] </span>
    // [ts] `);
    // [ts] expect(page.locator("h1")).not.toBeDisabled();
    // [ts] });
    // [ts] test("SE-057 should support disabled fieldset", () => {
    // [ts] page.setContent(`
    // [ts] <fieldset disabled>
    // [ts] <input></input>
    // [ts] <button data-testid="inside-fieldset-element">x</button>
    // [ts] <legend>
    // [ts] <button data-testid="inside-legend-element">legend</button>
    // [ts] </legend>
    // [ts] </fieldset>
    // [ts] <fieldset disabled>
    // [ts] <legend>
    // [ts] <div>
    // [ts] <button data-testid="nested-inside-legend-element">x</button>
    // [ts] </div>
    // [ts] </legend>
    // [ts] </fieldset>
    // [ts] <fieldset disabled>
    // [ts] <div></div>
    // [ts] <legend>
    // [ts] <button data-testid="first-legend-element">x</button>
    // [ts] </legend>
    // [ts] <legend>
    // [ts] <button data-testid="second-legend-element">x</button>
    // [ts] </legend>
    // [ts] </fieldset>
    // [ts] <fieldset disabled>
    // [ts] <fieldset>
    // [ts] <button data-testid="deep-button">x</button>
    // [ts] </fieldset>
    // [ts] </fieldset>
    // [ts] `);
    // [ts] expect.soft(page.getByTestId("inside-legend-element")).toBeEnabled();
    // [ts] expect.soft(page.getByTestId("nested-inside-legend-element")).toBeEnabled();
    // [ts] expect.soft(page.getByTestId("first-legend-element")).toBeEnabled();
    // [ts] expect.soft(page.getByTestId("second-legend-element")).toBeDisabled();
    // [ts] expect.soft(page.getByTestId("deep-button")).toBeDisabled();
    // [ts] });
    // [ts] test("SE-058 should support level", () => {
    // [ts] page.setContent(`
    // [ts] <h1>Hello</h1>
    // [ts] <h3>Hi</h3>
    // [ts] <div role="heading" aria-level="5">Bye</div>
    // [ts] `);
    // [ts] expect(page.locator(`role=heading[level=1]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<h1>Hello</h1>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("heading", { level: 1 }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<h1>Hello</h1>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=heading[level=3]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<h3>Hi</h3>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("heading", { level: 3 }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<h3>Hi</h3>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=heading[level=5]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="heading" aria-level="5">Bye</div>`,
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-059 should filter hidden, unless explicitly asked for", () => {
    // [ts] page.setContent(`
    // [ts] <button>Hi</button>
    // [ts] <button hidden>Hello</button>
    // [ts] <button aria-hidden="true">Yay</button>
    // [ts] <button aria-hidden="false">Nay</button>
    // [ts] <button style="visibility:hidden">Bye</button>
    // [ts] <div style="visibility:hidden">
    // [ts] <button>Oh</button>
    // [ts] </div>
    // [ts] <div style="visibility:hidden">
    // [ts] <button style="visibility:visible">Still here</button>
    // [ts] </div>
    // [ts] <button style="display:none">Never</button>
    // [ts] <div id=host1></div>
    // [ts] <div id=host2 style="display:none"></div>
    // [ts] <input name="one">
    // [ts] <details>
    // [ts] <summary>Open form</summary>
    // [ts] <label>
    // [ts] Label
    // [ts] <input name="two">
    // [ts] </label>
    // [ts] </details>
    // [ts] <select>
    // [ts] <option style="visibility:hidden">One</option>
    // [ts] <option style="display:none">Two</option>
    // [ts] <option>Three</option>
    // [ts] </select>
    // [ts] <script>
    // [ts] function addButton(host, text) {
    // [ts] const root = host.attachShadow({ mode: 'open' });
    // [ts] const button = document.createElement('button');
    // [ts] button.textContent = text;
    // [ts] root.appendChild(button);
    // [ts] }
    // [ts] addButton(document.getElementById('host1'), 'Shadow1');
    // [ts] addButton(document.getElementById('host2'), 'Shadow2');
    // [ts] </script>
    // [ts] `);
    // [ts] expect(page.locator(`role=button`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button aria-hidden="false">Nay</button>`,
    // [ts] `<button style="visibility:visible">Still here</button>`,
    // [ts] `<button>Shadow1</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[include-hidden]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button hidden="">Hello</button>`,
    // [ts] `<button aria-hidden="true">Yay</button>`,
    // [ts] `<button aria-hidden="false">Nay</button>`,
    // [ts] `<button style="visibility:hidden">Bye</button>`,
    // [ts] `<button>Oh</button>`,
    // [ts] `<button style="visibility:visible">Still here</button>`,
    // [ts] `<button style="display:none">Never</button>`,
    // [ts] `<button>Shadow1</button>`,
    // [ts] `<button>Shadow2</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[include-hidden=true]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button hidden="">Hello</button>`,
    // [ts] `<button aria-hidden="true">Yay</button>`,
    // [ts] `<button aria-hidden="false">Nay</button>`,
    // [ts] `<button style="visibility:hidden">Bye</button>`,
    // [ts] `<button>Oh</button>`,
    // [ts] `<button style="visibility:visible">Still here</button>`,
    // [ts] `<button style="display:none">Never</button>`,
    // [ts] `<button>Shadow1</button>`,
    // [ts] `<button>Shadow2</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[include-hidden=false]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>Hi</button>`,
    // [ts] `<button aria-hidden="false">Nay</button>`,
    // [ts] `<button style="visibility:visible">Still here</button>`,
    // [ts] `<button>Shadow1</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=textbox`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<input name="one">`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=option`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<option style="visibility:hidden">One</option>`,
    // [ts] `<option>Three</option>`,
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-060 should support name", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43549" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div role="button" aria-label=" Hello "></div>
    // [ts] <div role="button" aria-label="Hallo"></div>
    // [ts] <div role="button" aria-label="Hello" aria-hidden="true"></div>
    // [ts] <div role="button" aria-label="123" aria-hidden="true"></div>
    // [ts] <div role="button" aria-label='foo"bar' aria-hidden="true"></div>
    // [ts] <span id="title">Chosen name</span>
    // [ts] <div role="group" aria-labelledby="title"></div>
    // [ts] `);
    // [ts] const group = page.getByRole("group");
    // [ts] expect(group).toHaveAccessibleName("Chosen name");
    // [ts] expect(page.locator(`role=button[name="Hello"]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[name=" \n Hello "]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { name: "Hello" }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[name*="all"]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label="Hallo"></div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[name=/^H[ae]llo$/]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] `<div role="button" aria-label="Hallo"></div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { name: "^H[ae]llo$" }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] `<div role="button" aria-label="Hallo"></div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[name=/h.*o/i]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] `<div role="button" aria-label="Hallo"></div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { name: "h.*o" }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] `<div role="button" aria-label="Hallo"></div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[name="Hello"][include-hidden]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] `<div role="button" aria-label="Hello" aria-hidden="true"></div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { name: "Hello", includeHidden: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] `<div role="button" aria-label="Hello" aria-hidden="true"></div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { name: "hello", includeHidden: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] `<div role="button" aria-label="Hello" aria-hidden="true"></div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[name=Hello]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label=" Hello "></div>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[name=123][include-hidden]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label="123" aria-hidden="true"></div>`,
    // [ts] ]);
    // [ts] expect(page.getByRole("button", { name: "123", includeHidden: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="button" aria-label="123" aria-hidden="true"></div>`,
    // [ts] ]);
    // Old electron/android do not have this API.
    // [ts] const isMissingAriaLabelledByElements = browserName == "chromium" && browserMajorVersion < 135;
    // [ts] if (!isMissingAriaLabelledByElements) {
    // [ts] group.evaluate(element => {
    // [ts] element.ariaLabelledByElements = [document.querySelector("#title")];
    // [ts] });
    // [ts] expect(page.getByRole("group", { name: "Chosen name", exact: true }).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<div role="group" aria-labelledby=""></div>`,
    // [ts] ]);
    // [ts] }
    // [ts] });
    // [ts] test("SE-061 should support option name with html whitespace", () => {
    // [ts] page.setContent(`
    // [ts] <select>
    // [ts] <option value="html">&nbsp;HTML</option>
    // [ts] </select>
    // [ts] `);
    // [ts] expect(page.getByRole("option", { name: "HTML" })).toHaveCount(1);
    // [ts] });
    // [ts] test("SE-062 errors", () => {
    // [ts] const e0 = page.$("role=[bar]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e0.message, `Role must not be empty`) != null, true);
    // [ts] const e1 = page.$("role=foo[sElected]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e1.message, `Unknown attribute "sElected", must be one of "checked", "description", "disabled", "expanded", "include-hidden", "level", "name", "pressed", "selected"`) != null, true);
    // [ts] const e2 = page.$("role=foo[bar . qux=true]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e2.message, `Unknown attribute "bar.qux"`) != null, true);
    // [ts] const e3 = page.$("role=heading[level=\"bar\"]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e3.message, `"level" attribute must be compared to a number`) != null, true);
    // [ts] const e4 = page.$("role=checkbox[checked=\"bar\"]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e4.message, `"checked" must be one of true, false, "mixed"`) != null, true);
    // [ts] const e5 = page.$("role=checkbox[checked~=true]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e5.message, `cannot use ~= in attribute with non-string matching value`) != null, true);
    // [ts] const e6 = page.$("role=button[level=3]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e6.message, `"level" attribute is only supported for roles: "heading", "listitem", "row", "treeitem"`) != null, true);
    // [ts] const e7 = page.$("role=button[name]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e7.message, `"name" attribute must have a value`) != null, true);
    // [ts] const e8 = page.$("role=treeitem[expanded=\"none\"]").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e8.message, `"expanded" must be one of true, false`) != null, true);
    // [ts] });
    // [ts] test("SE-063 hidden with shadow dom slots", () => {
    // [ts] page.setContent(`
    // [ts] <div make-hidden>
    // [ts] <button>hidden1</button>
    // [ts] </div>
    // [ts] <div make-hidden>
    // [ts] <span><button>hidden2</button></v>
    // [ts] </div>
    // [ts] <div>
    // [ts] <button>visible1</button>
    // [ts] </div>
    // [ts] <div>
    // [ts] <span><button>visible2</button></span>
    // [ts] </div>
    // [ts] <script>
    // [ts] for (const div of document.querySelectorAll('div')) {
    // [ts] const hidden = div.hasAttribute('make-hidden');
    // [ts] div.attachShadow({ mode: 'open' }).innerHTML = hidden ? 'nothing to see here' : '<slot></slot>';
    // [ts] }
    // [ts] </script>
    // [ts] `);
    // [ts] expect(page.locator(`role=button`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>visible1</button>`,
    // [ts] `<button>visible2</button>`,
    // [ts] ]);
    // [ts] expect(page.locator(`role=button[include-hidden]`).evaluateAll(els => els.map(e => e.outerHTML))).toEqual([
    // [ts] `<button>hidden1</button>`,
    // [ts] `<button>hidden2</button>`,
    // [ts] `<button>visible1</button>`,
    // [ts] `<button>visible2</button>`,
    // [ts] ]);
    // [ts] });
    // [ts] test("SE-064 should support output accessible name", () => {
    // [ts] page.setContent(`<label>Output1<output>output</output></label>`);
    // [ts] expect(page.getByRole("status", { name: "Output1" })).toBeVisible();
    // [ts] });
    // [ts] test("SE-065 should not match scope by default", () => {
    // [ts] page.setContent(`
    // [ts] <ul>
    // [ts] <li aria-label="Parent list">
    // [ts] Parent list
    // [ts] <ul>
    // [ts] <li>child 1</li>
    // [ts] <li>child 2</li>
    // [ts] </ul>
    // [ts] </li>
    // [ts] </ul>
    // [ts] `);
    // [ts] const children = page.getByRole("listitem", { name: "Parent list" }).getByRole("listitem");
    // [ts] expect(children).toHaveCount(2);
    // [ts] expect(children).toHaveText(["child 1", "child 2"]);
    // [ts] });
}
