// Migrated from TypeScript (library/locator-dispatchevent-touch.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";

it.use({ hasTouch: true });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("TA-010 should support touch points in touch event arguments", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <div data-testid='outer' style="position: absolute; width: 120px; height: 120px; background-color: red;">
    // [ts] <div data-testid='inner' style="position: absolute; width: 100px; height: 100px; top: 10px; left: 10px; background-color: green; z-index: 3;">inner</div>
    // [ts] </div>`);
    // [ts] const outer = page.getByTestId("outer");
    // [ts] outer.evaluate((el) => {
    // [ts] const events = [];
    // [ts] (window).events = events;
    // [ts] el.addEventListener("touchstart", (e) => events.push("touchstart: " + [...e.touches].map(t => `${t.constructor.name}(id: ${t.identifier}, clientX: ${t.clientX}, clientY: ${t.clientY})`)));
    // [ts] el.addEventListener("touchmove", (e) => events.push("touchmove: " + [...e.touches].map(t => `${t.constructor.name}(id: ${t.identifier}, clientX: ${t.clientX}, clientY: ${t.clientY})`)));
    // [ts] el.addEventListener("touchend", (e) => events.push("touchend: " + [...e.touches].map(t => `${t.constructor.name}(id: ${t.identifier}, clientX: ${t.clientX}, clientY: ${t.clientY})`)));
    // [ts] });
    // [ts] const touches = [{ identifier: 0, clientX: 61, clientY: 60 }, { identifier: 1, clientX: 59, clientY: 60 }];
    // [ts] const inner = page.getByTestId("inner");
    // [ts] inner.dispatchEvent("touchstart", {
    // [ts] touches,
    // [ts] changedTouches: touches,
    // [ts] targetTouches: touches,
    // [ts] });
    // [ts] inner.dispatchEvent("touchmove", {
    // [ts] touches,
    // [ts] changedTouches: touches,
    // [ts] targetTouches: touches,
    // [ts] });
    // [ts] inner.dispatchEvent("touchend", {
    // [ts] touches: [],
    // [ts] changedTouches: touches,
    // [ts] targetTouches: [],
    // [ts] });
    // [ts] expect(page.evaluate(() => (window).events)).toEqual([
    // [ts] "touchstart: Touch(id: 0, clientX: 61, clientY: 60),Touch(id: 1, clientX: 59, clientY: 60)",
    // [ts] "touchmove: Touch(id: 0, clientX: 61, clientY: 60),Touch(id: 1, clientX: 59, clientY: 60)",
    // [ts] "touchend: ",
    // [ts] ]);
    // [ts] });
}
