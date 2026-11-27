// Migrated from TypeScript (page/page-select-option.spec.ts) via cap from -ts

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
























// @see https://github.com/GoogleChrome/puppeteer/issues/3327








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("SO-001 should select single option @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", "blue");
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue"]);
    // [ts] });
    // [ts] test("SO-002 should select single option by value", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", { value: "blue" });
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue"]);
    // [ts] });
    // [ts] test("SO-003 should fall back to selecting by label", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", "Blue");
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue"]);
    // [ts] });
    // [ts] test("SO-004 should select single option by label", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", { label: "Indigo" });
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["indigo"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["indigo"]);
    // [ts] });
    // [ts] test("SO-005 should select single option by label with html whitespace", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", { label: "HTML" });
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["html"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["html"]);
    // [ts] });
    // [ts] test("SO-006 should select single option by handle", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", page.$("[id=whiteOption]"));
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["white"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["white"]);
    // [ts] });
    // [ts] test("SO-007 should select single option by index", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", { index: 2 });
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["brown"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["brown"]);
    // [ts] });
    // [ts] test("SO-008 should select single option by multiple attributes", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", { value: "green", label: "Green" });
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["green"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["green"]);
    // [ts] });
    // [ts] test("SO-009 should not select single option when some attributes do not match", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', s => s.value = undefined);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.selectOption("select", { value: "green", label: "Brown" }, { timeout: 1000 });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, "Timeout") != null, true);
    // [ts] }
    // [ts] expect_eq(page.evaluate(() => document.querySelector("select").value), "");
    // [ts] });
    // [ts] test("SO-010 should select only first option", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", ["blue", "green", "red"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue"]);
    // [ts] });
    // [ts] test("SO-011 should not throw when select causes navigation", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', select => select.addEventListener('input', () => window.location.href = '/empty.html'));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.selectOption("select", "blue"),
    // [ts] page.waitForNavigation(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, page.url(), "empty.html") != null, true);
    // [ts] });
    // [ts] test("SO-012 should select multiple options", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window["makeMultiple"]());
    // [ts] page.selectOption("select", ["blue", "green", "red"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue", "green", "red"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue", "green", "red"]);
    // [ts] });
    // [ts] test("SO-013 should select multiple options with attributes", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window["makeMultiple"]());
    // [ts] page.selectOption("select", [{ value: "blue" }, { label: "Green" }, { index: 4 }]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue", "gray", "green"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue", "gray", "green"]);
    // [ts] });
    // [ts] test("SO-014 should respect event bubbling", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", "blue");
    // [ts] expect_eq(page.evaluate(() => window["result"].onBubblingInput), ["blue"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onBubblingChange), ["blue"]);
    // [ts] });
    // [ts] test("SO-015 should throw when element is not a <select>", () => {
    // [ts] let error = null;
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("body", "").catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not a <select> element") != null, true);
    // [ts] });
    // [ts] test("SO-016 should return [] on no matched values", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const result = page.selectOption("select", []);
    // [ts] expect_eq(result, []);
    // [ts] });
    // [ts] test("SO-017 should return an array of matched values", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window["makeMultiple"]());
    // [ts] const result = page.selectOption("select", ["blue", "black", "magenta"]);
    // [ts] expect_eq(result.reduce((accumulator, current) => ["blue", "black", "magenta"].includes(current) && accumulator, true), true);
    // [ts] });
    // [ts] test("SO-018 should return an array of one element when multiple is not set", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const result = page.selectOption("select", ["42", "blue", "black", "magenta"]);
    // [ts] expect_eq(result.length, 1);
    // [ts] });
    // [ts] test("SO-019 should return [] on no values", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const result = page.selectOption("select", []);
    // [ts] expect_eq(result, []);
    // [ts] });
    // [ts] test("SO-020 should not allow null items", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window["makeMultiple"]());
    // [ts] let error = null;
    // [ts] page.selectOption("select", ["blue", null, "black", "magenta"]).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "options[1]: expected object, got null") != null, true);
    // [ts] });
    // [ts] test("SO-021 should unselect with null", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window["makeMultiple"]());
    // [ts] const result = page.selectOption("select", ["blue", "black", "magenta"]);
    // [ts] expect_eq(result.reduce((accumulator, current) => ["blue", "black", "magenta"].includes(current) && accumulator, true), true);
    // [ts] page.selectOption("select", null);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('select', select => Array.from(select.options).every(option => !option.selected))).toEqual(true);
    // [ts] });
    // [ts] test("SO-022 should deselect all options when passed no values for a multiple select", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window["makeMultiple"]());
    // [ts] page.selectOption("select", ["blue", "black", "magenta"]);
    // [ts] page.selectOption("select", []);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('select', select => Array.from(select.options).every(option => !option.selected))).toEqual(true);
    // [ts] });
    // [ts] test("SO-023 should deselect all options when passed no values for a select without multiple", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.selectOption("select", ["blue", "black", "magenta"]);
    // [ts] page.selectOption("select", []);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('select', select => Array.from(select.options).every(option => !option.selected))).toEqual(true);
    // [ts] });
    // [ts] test("SO-024 should throw if passed wrong types", () => {
    // [ts] let error;
    // [ts] page.setContent("<select><option value=\"12\"/></select>");
    // [ts] error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error cannot select numbers
    // [ts] page.selectOption("select", 12);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "options[0]: expected object, got number") != null, true);
    // [ts] error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error cannot select numbers
    // [ts] page.selectOption("select", { value: 12 });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "options[0].value: expected string, got number") != null, true);
    // [ts] error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error cannot select numbers
    // [ts] page.selectOption("select", { label: 12 });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "options[0].label: expected string, got number") != null, true);
    // [ts] error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error cannot select string indices
    // [ts] page.selectOption("select", { index: "12" });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "options[0].index: expected integer, got string") != null, true);
    // [ts] });
    // [ts] test("SO-025 should work when re-defining top-level Event class", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window.Event = null);
    // [ts] page.selectOption("select", "blue");
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue"]);
    // [ts] });
    // [ts] test("SO-026 should wait for option to be present", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const selectPromise  = page.selectOption("select", "scarlet");
    // [ts] let didSelect = false;
    // [ts] void selectPromise.then(() => didSelect = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(didSelect, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', select => {
    // [ts-skip] const option = document.createElement('option');
    // [ts-skip] option.value = 'scarlet';
    // [ts-skip] option.textContent = 'Scarlet';
    // [ts-skip] select.appendChild(option);
    // [ts-skip] });
    // [ts] const items = selectPromise;
    // [ts] expect_eq(items, ["scarlet"]);
    // [ts] });
    // [ts] test("SO-027 should wait for option index to be present", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const len = await page.$eval('select', select => select.options.length);
    // [ts] const selectPromise  = page.selectOption("select", { index: len });
    // [ts] let didSelect = false;
    // [ts] void selectPromise.then(() => didSelect = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(didSelect, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', select => {
    // [ts-skip] const option = document.createElement('option');
    // [ts-skip] option.value = 'scarlet';
    // [ts-skip] option.textContent = 'Scarlet';
    // [ts-skip] select.appendChild(option);
    // [ts-skip] });
    // [ts] const items = selectPromise;
    // [ts] expect_eq(items, ["scarlet"]);
    // [ts] });
    // [ts] test("SO-028 should wait for multiple options to be present", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => window["makeMultiple"]());
    // [ts] const selectPromise  = page.selectOption("select", ["green", "scarlet"]);
    // [ts] let didSelect = false;
    // [ts] void selectPromise.then(() => didSelect = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(didSelect, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', select => {
    // [ts-skip] const option = document.createElement('option');
    // [ts-skip] option.value = 'scarlet';
    // [ts-skip] option.textContent = 'Scarlet';
    // [ts-skip] select.appendChild(option);
    // [ts-skip] });
    // [ts] const items = selectPromise;
    // [ts] expect_eq(items, ["green", "scarlet"]);
    // [ts] });
    // [ts] test("SO-029 input event.composed should be true and cross shadow dom boundary", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28726" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<body><script>
    // [ts] const div = document.createElement('div');
    // [ts] const shadowRoot = div.attachShadow({mode: 'open'});
    // [ts] shadowRoot.innerHTML = \`<select>
    // [ts] <option value="black">Black</option>
    // [ts] <option value="blue">Blue</option>
    // [ts] </select>\`;
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
    // [ts] page.locator("select").evaluate(select => {
    // [ts] (window).firedEvents = [];
    // [ts] for (const event of ["input", "change"]) {
    // [ts] select.addEventListener(event, e => {
    // [ts] (window).firedEvents.push(e.type + ":" + e.composed);
    // [ts] }, false);
    // [ts] }
    // [ts] });
    // [ts] page.selectOption("select", "blue");
    // [ts] expect_eq(page.evaluate(() => window["firedEvents"]), ["input:true", "change:false"]);
    // [ts] expect_eq(page.evaluate(() => window["firedBodyEvents"]), ["input:true"]);
    // [ts] });
    // [ts] test("SO-030 should wait for select to be enabled", () => {
    // [ts] page.setContent(`
    // [ts] <select disabled>
    // [ts] <option>one</option>
    // [ts] <option>two</option>
    // [ts] </select>
    // [ts] <script>
    // [ts] function hydrate() {
    // [ts] const select = document.querySelector('select');
    // [ts] select.removeAttribute('disabled');
    // [ts] select.addEventListener('change', () => {
    // [ts] window['result'] = select.value;
    // [ts] });
    // [ts] }
    // [ts] </script>
    // [ts] `);
    // [ts] const selectPromise = page.locator("select").selectOption("two");
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] page.evaluate(() => (window).hydrate());
    // [ts] selectPromise;
    // [ts] expect_eq(page.evaluate(() => window["result"]), "two");
    // [ts] expect(page.locator("select")).toHaveValue("two");
    // [ts] });
    // [ts] test("SO-031 should wait for option to be enabled", () => {
    // [ts] page.setContent(`
    // [ts] <select>
    // [ts] <option>one</option>
    // [ts] <option disabled id=myoption>two</option>
    // [ts] </select>
    // [ts] <script>
    // [ts] function hydrate() {
    // [ts] const option = document.querySelector('#myoption');
    // [ts] option.removeAttribute('disabled');
    // [ts] const select = document.querySelector('select');
    // [ts] select.addEventListener('change', () => {
    // [ts] window['result'] = select.value;
    // [ts] });
    // [ts] }
    // [ts] </script>
    // [ts] `);
    // [ts] const error = page.locator("select").selectOption("two", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "option being selected is not enabled") != null, true);
    // [ts] const selectPromise = page.locator("select").selectOption("two");
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] page.evaluate(() => (window).hydrate());
    // [ts] selectPromise;
    // [ts] expect_eq(page.evaluate(() => window["result"]), "two");
    // [ts] expect(page.locator("select")).toHaveValue("two");
    // [ts] });
    // [ts] test("SO-032 should wait for optgroup to be enabled", () => {
    // [ts] page.setContent(`
    // [ts] <select>
    // [ts] <option>one</option>
    // [ts] <optgroup label="Group" disabled id=mygroup>
    // [ts] <option>two</option>
    // [ts] </optgroup>
    // [ts] </select>
    // [ts] <script>
    // [ts] function hydrate() {
    // [ts] const group = document.querySelector('#mygroup');
    // [ts] group.removeAttribute('disabled');
    // [ts] const select = document.querySelector('select');
    // [ts] select.addEventListener('change', () => {
    // [ts] window['result'] = select.value;
    // [ts] });
    // [ts] }
    // [ts] </script>
    // [ts] `);
    // [ts] const error = page.locator("select").selectOption("two", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "option being selected is not enabled") != null, true);
    // [ts] const selectPromise = page.locator("select").selectOption("two");
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] page.evaluate(() => (window).hydrate());
    // [ts] selectPromise;
    // [ts] expect_eq(page.evaluate(() => window["result"]), "two");
    // [ts] expect(page.locator("select")).toHaveValue("two");
    // [ts] });
    // [ts] test("SO-033 should wait for select to be swapped", () => {
    // [ts] page.setContent(`
    // [ts] <select disabled>
    // [ts] <option>one</option>
    // [ts] <option>two</option>
    // [ts] </select>
    // [ts] <script>
    // [ts] function hydrate() {
    // [ts] const select = document.querySelector('select');
    // [ts] select.remove();
    // [ts] const newSelect = document.createElement('select');
    // [ts] const option1 = document.createElement('option');
    // [ts] option1.textContent = 'one';
    // [ts] newSelect.appendChild(option1);
    // [ts] const option2 = document.createElement('option');
    // [ts] option2.textContent = 'two';
    // [ts] newSelect.appendChild(option2);
    // [ts] document.body.appendChild(newSelect);
    // [ts] newSelect.addEventListener('change', () => {
    // [ts] window['result'] = newSelect.value;
    // [ts] });
    // [ts] }
    // [ts] </script>
    // [ts] `);
    // [ts] const selectPromise = page.locator("select").selectOption("two");
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] page.evaluate(() => (window).hydrate());
    // [ts] selectPromise;
    // [ts] expect(page.locator("select")).toHaveValue("two");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "two");
    // [ts] });
    // [ts] test("SO-034 should wait for select to become non-inert", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42967" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div inert id=container>
    // [ts] <select>
    // [ts] <option>one</option>
    // [ts] <option>two</option>
    // [ts] </select>
    // [ts] </div>
    // [ts] `);
    // [ts] let done = false;
    // [ts] const selectPromise = page.locator("select").selectOption("two").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] expect(page.locator("select")).toHaveValue("one");
    // [ts] page.evaluate(() => document.getElementById("container").removeAttribute("inert"));
    // [ts] selectPromise;
    // [ts] expect(page.locator("select")).toHaveValue("two");
    // [ts] page.evaluate(() => document.getElementById("container").setAttribute("inert", ""));
    // [ts] const error = page.locator("select").selectOption("one", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is inert") != null, true);
    // [ts] });
}
