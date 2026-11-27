// Migrated from TypeScript (page/page-transition.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("PA-038 should not crash when filter transition completes", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42637" });
    // [ts] it.fixme(browserName == "webkit", "Web process crashes on the compositor thread when a filter transitions to none");
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] div { transition: filter 0.1s; }
    // [ts] .dim { filter: brightness(0.95); }
    // [ts] </style>
    // [ts] ${'<div>hello</div>'.repeat(10)}
    // [ts] `);
    // [ts] let crashed = false;
    // [ts] page.on("crash", () => crashed = true);
    // [ts] let i = 0;
    // [ts] while (i < 10 && !crashed) { // [cap-migrate] for-step: i++
    // [ts] page.evaluate(() => {
    // [ts] const divs = [...document.querySelectorAll("div")];
    // [ts] const settled = () => joinAll(divs.map(div => Promise.init(f => div.addEventListener("transitionend", f, { once: true })))); // migrated: Promise.all -> joinAll
    // [ts] let promise = settled();
    // [ts] divs.forEach(div => div.classList.add("dim"));
    // [ts] promise;
    // [ts] promise = settled();
    // [ts] divs.forEach(div => div.classList.remove("dim"));
    // [ts] promise;
    // [ts] }).catch(() => {});
    // [ts] }
    // [ts] expect_eq(crashed, false);
    // [ts] });
}
