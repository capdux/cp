// Migrated from TypeScript (page/locator-wait-for-function.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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
    // [ts] test("LO-183 should wait for an attribute to appear", () => {
    // [ts] page.setContent("<button id=toggle>Menu</button>");
    // [ts] page.evaluate(() => window.builtins.setTimeout(() => document.querySelector("#toggle").setAttribute("aria-expanded", "true"), 1000));
    // [ts] page.locator("#toggle").waitForFunction(element => element.hasAttribute("aria-expanded"));
    // [ts] });
    // [ts] test("LO-184 should return immediately when already truthy", () => {
    // [ts] page.setContent("<div id=target>yes</div>");
    // [ts] expect_eq(page.locator("#target").waitForFunction(element => element.textContent == "yes"), null);
    // [ts] });
    // [ts] test("LO-185 should accept ElementHandle arguments", () => {
    // [ts] page.setContent("<div id=a></div><div id=b>value</div>");
    // [ts] const handle = page.$("#b");
    // [ts] page.locator("#a").waitForFunction((element, other) => other.textContent == "value", handle);
    // [ts] });
    // [ts] test("LO-186 should accept string expression", () => {
    // [ts] page.setContent("<div id=target>yes</div>");
    // [ts] page.locator("#target").waitForFunction(`element => element.textContent === 'yes'`);
    // [ts] });
    // [ts] test("LO-187 should resolve a promise returned by the predicate", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44217" } }, (_p) => {
    // [ts] page.setContent("<div id=target>no</div>");
    // [ts] const target = page.locator("#target");
    // [ts] expect(target.waitForFunction(async element => element.textContent == "yes", null, { timeout: 250 })).rejects.toThrow("Timeout 250ms exceeded");
    // [ts] target.waitForFunction(async element => {
    // [ts] Promise.init(resolve => window.builtins.setTimeout(resolve, 10));
    // [ts] const result = element.textContent == "yes";
    // [ts] element.textContent = "yes";
    // [ts] return result;
    // [ts] });
    // [ts] expect_eq(target.textContent(), "yes");
    // [ts] });
    // [ts] test("LO-188 should wait for element to appear and survive rerender", () => {
    // [ts] page.setContent("<span>nothing here</span>");
    // [ts] page.evaluate(() => {
    // [ts] let count = 0;
    // [ts] let prev = null;
    // [ts] const tick = () => {
    // [ts] ++count;
    // [ts] const next = document.createElement("div");
    // [ts] next.id = "target";
    // [ts] next.textContent = String(count);
    // [ts] if (prev)
    // [ts] prev.remove();
    // [ts] document.body.appendChild(next);
    // [ts] prev = next;
    // [ts] if (count < 3)
    // [ts] window.builtins.setTimeout(tick, 500);
    // [ts] };
    // [ts] window.builtins.setTimeout(tick, 500);
    // [ts] });
    // [ts] page.locator("#target").waitForFunction(element => element.textContent == "3");
    // [ts] });
    // [ts] test("LO-189 should throw when predicate throws", () => {
    // [ts] page.setContent("<div id=target>no</div>");
    // [ts] const error = page.locator("#target").waitForFunction(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('oh my');)
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "oh my") != null, true);
    // [ts] });
    // [ts] test("LO-200 should throw when predicate rejects", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44217" } }, (_p) => {
    // [ts] page.setContent("<div id=target>no</div>");
    // [ts] expect(page.locator("#target").waitForFunction(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('async oh my');)
    // [ts] })).rejects.toThrow("async oh my");
    // [ts] });
    // [ts] test("LO-190 should throw on strict mode violation", () => {
    // [ts] page.setContent("<div class=x>1</div><div class=x>2</div>");
    // [ts] const error = page.locator("div.x").waitForFunction(() => true).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] });
    // [ts] test("LO-191 should abort via signal", () => {
    // [ts] page.setContent("<div id=target>no</div>");
    // [ts] const controller = AbortController.init();
    // [ts] const promise = page.locator("#target").waitForFunction(element => element.textContent == "yes", null, { timeout: 0, signal: controller.signal }).catch(e => e);
    // [ts] page.waitForTimeout(100);
    // [ts] const reason = Error.init("Aborted by user");
    // [ts] controller.abort(reason);
    // [ts] const error = promise;
    // [ts] expect_eq(error.name, "AbortError");
    // [ts] expect_eq(error.cause, reason);
    // [ts] });
    // [ts] test("LO-192 should abort via already-aborted signal", () => {
    // [ts] page.setContent("<div id=target>no</div>");
    // [ts] const controller = AbortController.init();
    // [ts] controller.abort("already aborted");
    // [ts] const error = page.locator("#target").waitForFunction(() => true, null, { signal: controller.signal }).catch(e => e);
    // [ts] expect_eq(error.name, "AbortError");
    // [ts] expect_eq(error.cause, "already aborted");
    // [ts] });
}
