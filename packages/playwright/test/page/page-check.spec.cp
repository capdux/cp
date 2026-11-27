// Migrated from TypeScript (page/page-check.spec.ts) via cap from -ts

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
    // TODO(cap-migrate): refine TS body
    // [ts] test("CH-001 should check the box @smoke", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox'></input>`);
    // [ts] page.check("input");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), true);
    // [ts] });
    // [ts] test("CH-002 should not check the checked box", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox' checked></input>`);
    // [ts] page.check("input");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), true);
    // [ts] });
    // [ts] test("CH-003 should uncheck the box", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox' checked></input>`);
    // [ts] page.uncheck("input");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), false);
    // [ts] });
    // [ts] test("CH-004 should not uncheck the unchecked box", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox'></input>`);
    // [ts] page.uncheck("input");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), false);
    // [ts] });
    // [ts] test("CH-005 should check radio", () => {
    // [ts] page.setContent(`
    // [ts] <input type='radio'>one</input>
    // [ts] <input id='two' type='radio'>two</input>
    // [ts] <input type='radio'>three</input>`);
    // [ts] page.check("#two");
    // [ts] expect_eq(page.evaluate(() => window["two"].checked), true);
    // [ts] });
    // [ts] test("CH-006 should check radio by aria role", () => {
    // [ts] page.setContent(`<div role='radio' id='checkbox'>CHECKBOX</div>
    // [ts] <script>
    // [ts] checkbox.addEventListener('click', () => checkbox.setAttribute('aria-checked', 'true'));
    // [ts] </script>`);
    // [ts] page.check("div");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].getAttribute("aria-checked")), "true");
    // [ts] });
    // [ts] test("CH-007 should uncheck radio by aria role", () => {
    // [ts] page.setContent(`<div role='radio' id='checkbox' aria-checked="true">CHECKBOX</div>
    // [ts] <script>
    // [ts] checkbox.addEventListener('click', () => checkbox.setAttribute('aria-checked', 'false'));
    // [ts] </script>`);
    // [ts] page.uncheck("div");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].getAttribute("aria-checked")), "false");
    // [ts] });
    // [ts] test("CH-008 should check the box by aria role", () => {
    // [ts] for (const role of ["checkbox", "menuitemcheckbox", "option", "radio", "switch", "menuitemradio", "treeitem"]) {
    // [ts] it.step(`role=${role}`, () => {
    // [ts] page.setContent(`<div role='${role}' id='checkbox'>CHECKBOX</div>
    // [ts] <script>
    // [ts] checkbox.addEventListener('click', () => checkbox.setAttribute('aria-checked', 'true'));
    // [ts] </script>`);
    // [ts] page.check("div");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].getAttribute("aria-checked")), "true");
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("CH-009 should uncheck the box by aria role", () => {
    // [ts] for (const role of ["checkbox", "menuitemcheckbox", "option", "radio", "switch", "menuitemradio", "treeitem"]) {
    // [ts] it.step(`role=${role}`, () => {
    // [ts] page.setContent(`<div role='${role}' id='checkbox' aria-checked="true">CHECKBOX</div>
    // [ts] <script>
    // [ts] checkbox.addEventListener('click', () => checkbox.setAttribute('aria-checked', 'false'));
    // [ts] </script>`);
    // [ts] page.uncheck("div");
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].getAttribute("aria-checked")), "false");
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("CH-010 should throw when not a checkbox", () => {
    // [ts] page.setContent(`<div>Check me</div>`);
    // [ts] const error = page.check("div").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Not a checkbox or radio button") != null, true);
    // [ts] });
    // [ts] test("CH-011 should throw when not a checkbox 2", () => {
    // [ts] page.setContent(`<div role=button>Check me</div>`);
    // [ts] const error = page.check("div").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Not a checkbox or radio button") != null, true);
    // [ts] });
    // [ts] test("CH-012 should check the box inside a button", () => {
    // [ts] page.setContent(`<div role='button'><input type='checkbox'></div>`);
    // [ts] page.check("input");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.checked)).toBe(true);
    // [ts] expect_eq(page.isChecked("input"), true);
    // [ts] expect_eq((page.$("input")).isChecked(), true);
    // [ts] });
    // [ts] test("CH-013 should check the label with position", () => {
    // [ts] page.setContent(`
    // [ts] <input id='checkbox' type='checkbox' style='width: 5px; height: 5px;'>
    // [ts] <label for='checkbox'>
    // [ts] <a href=${JSON.stringify(server.EMPTY_PAGE)}>I am a long link that goes away so that nothing good will happen if you click on me</a>
    // [ts] Click me
    // [ts] </label>`);
    // [ts] const box = (page.$("text=Click me")).boundingBox();
    // [ts] page.check("text=Click me", { position: { x: box.width - 10, y: 2 } });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.checked)).toBe(true);
    // [ts] });
    // [ts] test("CH-014 trial run should not check", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox'></input>`);
    // [ts] page.check("input", { trial: true });
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), false);
    // [ts] });
    // [ts] test("CH-015 trial run should not uncheck", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox' checked></input>`);
    // [ts] page.uncheck("input", { trial: true });
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), true);
    // [ts] });
    // [ts] test("CH-016 should check the box using setChecked", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox'></input>`);
    // [ts] page.setChecked("input", true);
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), true);
    // [ts] page.setChecked("input", false);
    // [ts] expect_eq(page.evaluate(() => window["checkbox"].checked), false);
    // [ts] });
    // [ts] test("CH-017 should throw when trying to uncheck radio button", () => {
    // [ts] page.setContent(`<input type='radio' name='test' checked id='radio'>`);
    // [ts] const error = page.uncheck("#radio").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Cannot uncheck radio button") != null, true);
    // [ts] });
}
