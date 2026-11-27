// Migrated from TypeScript (page/page-strict.spec.ts) via cap from -ts

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
    // [ts] test("SM-001 should fail page.textContent in strict mode", () => {
    // [ts] page.setContent(`<span>span1</span><div><span>target</span></div>`);
    // [ts] const error = page.textContent("span", { strict: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `1) <span>span1</span> aka getByText('span1')`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `2) <span>target</span> aka getByText('target')`) != null, true);
    // [ts] });
    // [ts] test("SM-002 should fail page.getAttribute in strict mode", () => {
    // [ts] page.setContent(`<span>span1</span><div><span>target</span></div>`);
    // [ts] const error = page.getAttribute("span", "id", { strict: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] });
    // [ts] test("SM-003 should fail page.fill in strict mode", () => {
    // [ts] page.setContent(`<input></input><div><input></input></div>`);
    // [ts] const error = page.fill("input", "text", { strict: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `1) <input/> aka getByRole('textbox').first()`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `2) <input/> aka locator('div').getByRole('textbox')`) != null, true);
    // [ts] });
    // [ts] test("SM-004 should fail page.$ in strict mode", () => {
    // [ts] page.setContent(`<span>span1</span><div><span>target</span></div>`);
    // [ts] const error = page.$("span", { strict: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] });
    // [ts] test("SM-005 should fail page.waitForSelector in strict mode", () => {
    // [ts] page.setContent(`<span>span1</span><div><span>target</span></div>`);
    // [ts] const error = page.waitForSelector("span", { strict: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] });
    // [ts] test("SM-006 should fail page.dispatchEvent in strict mode", () => {
    // [ts] page.setContent(`<span></span><div><span></span></div>`);
    // [ts] const error = page.dispatchEvent("span", "click", {}, { strict: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `1) <span></span> aka locator('span').first()`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `2) <span></span> aka locator('div span')`) != null, true);
    // [ts] });
    // [ts] test("SM-007 should properly format :nth-child() in strict mode message", () => {
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <div>
    // [ts] </div>
    // [ts] <div>
    // [ts] <div>
    // [ts] </div>
    // [ts] <div>
    // [ts] </div>
    // [ts] </div>
    // [ts] </div>
    // [ts] <div>
    // [ts] <div class='foo'>
    // [ts] </div>
    // [ts] <div class='foo'>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] const error = page.locator(".foo").hover().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // Second div has body > div:nth-child(2) > div:nth-child(2) selector which would be ambiguous
    // if '>' were ' '.
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "body > div:nth-child(2) > div:nth-child(2)") != null, true);
    // [ts] });
    // [ts] test("SM-008 should escape class names", () => {
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <div></div>
    // [ts] <div>
    // [ts] <div></div>
    // [ts] <div></div>
    // [ts] </div>
    // [ts] </div>
    // [ts] <div>
    // [ts] <div class='foo bar:0'>
    // [ts] </div>
    // [ts] <div class='foo bar:1'>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] const error = page.locator(".foo").hover().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div class=\"foo bar:0") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div class=\"foo bar:1") != null, true);
    // [ts] });
    // [ts] test("SM-009 should escape tag names", () => {
    // [ts] page.setContent(`
    // [ts] <q:template> </q:template>
    // [ts] <span>special test description</span>
    // [ts] <q:template hidden="" aria-hidden="true">
    // [ts] <span>special test description</span>
    // [ts] </q:template>
    // [ts] `);
    // [ts] const error = expect(page.getByText("special test description")).toBeVisible().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `getByText('special test description').first()`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `locator('q\\\\:template').filter({ hasText: 'special test description' })`) != null, true);
    // [ts] });
    // [ts] test("SM-010 should keep the engine name for a \"text=..\" selector in strict mode", () => {
    // [ts] page.setContent(`<div>..loading</div><div>..loading</div>`);
    // [ts] const error = page.locator("text=..loading").hover().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `locator('text=..loading')`) != null, true);
    // [ts] });
}
