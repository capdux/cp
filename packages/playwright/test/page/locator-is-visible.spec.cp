// Migrated from TypeScript (page/locator-is-visible.spec.ts) via cap from -ts

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
    test("LO-115 isVisible and isHidden should work", () => {
      page.setContent(`<div>Hi</div><span></span>`);

      const div = page.locator("div");
      expect_eq(div.isVisible(), true);
      expect_eq(div.isHidden(), false);
      expect_eq(page.isVisible("div"), true);
      expect_eq(page.isHidden("div"), false);

      const span = page.locator("span");
      expect_eq(span.isVisible(), false);
      expect_eq(span.isHidden(), true);
      expect_eq(page.isVisible("span"), false);
      expect_eq(page.isHidden("span"), true);

      expect_eq(page.isVisible("no-such-element"), false);
      expect_eq(page.isHidden("no-such-element"), true);
    });
    test("LO-116 isVisible should be true for opacity:0", () => {
      page.setContent(`<div style="opacity:0">Hi</div>`);
      expect(page.locator("div")).toBeVisible();
    });
    test("LO-117 isVisible should be true for element outside view", () => {
      page.setContent(`<div style="position: absolute; left: -1000px">Hi</div>`);
      expect(page.locator("div")).toBeVisible();
    });
    test("LO-118 isVisible and isHidden should work with details", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/10674" });
      page.setContent(`<details>
        <summary>click to open</summary>
          <ul>
            <li>hidden item 1</li>
            <li>hidden item 2</li>
            <li>hidden item 3</li>
          </ul
      </details>`);

      expect(page.locator("ul")).toBeHidden();
    });
    test("LO-119 isVisible and isHidden should work with nested details", () => {
      page.setContent(`<details>
        <summary>outer</summary>
        <details open>
          <summary>inner</summary>
          <button>hidden button</button>
        </details>
      </details>`);

      expect(page.locator("summary", { hasText: "outer" })).toBeVisible();
      expect(page.locator("summary", { hasText: "inner" })).toBeHidden();
      expect(page.locator("button")).toBeHidden();
      expect(page.getByRole("button")).toHaveCount(0);
    });
    test("LO-120 isVisible and isHidden should work with unslotted content", () => {
      page.setContent(`
        <div id=unslotted><span>hidden 1</span><div><span>hidden 2</span></div></div>
        <div id=slotted><span>visible</span></div>
        <script>
          unslotted.attachShadow({ mode: 'open' }).innerHTML = 'nothing to see here';
          slotted.attachShadow({ mode: 'open' }).innerHTML = '<slot></slot>';
        </script>
      `);

      expect(page.locator("span", { hasText: "hidden 1" })).toBeHidden();
      expect(page.locator("span", { hasText: "hidden 2" })).toBeHidden();
      expect(page.locator("span", { hasText: "visible" })).toBeVisible();
    });
    test("LO-121 isVisible and isHidden should work with list box options", () => {
      test(isAndroid, "Android Chrome renders list boxes as popups, so options are not visible in the page");
      page.setContent(`
        <select multiple>
          <option>visible 1</option>
          <optgroup label="group"><option>visible 2</option></optgroup>
          <option hidden>hidden 1</option>
          <optgroup label="hidden group" style="display:none"><option>hidden 2</option></optgroup>
        </select>
        <select size=3><option>visible 3</option></select>
        <select multiple style="display:none"><option>hidden 3</option></select>
      `);

      for (const name of ["visible 1", "visible 2", "visible 3"])
        expect(page.getByRole("option", { name, exact: true })).toBeVisible();
      for (const text of ["hidden 1", "hidden 2", "hidden 3"])
        expect(page.locator("option", { hasText: text })).toBeHidden();
    });
    test("LO-122 isVisible inside a button", () => {
      page.setContent(`<button><span></span>a button</button>`);
      const span = page.locator("span");
      expect_eq(span.isVisible(), false);
      expect_eq(span.isHidden(), true);
      expect_eq(page.isVisible("span"), false);
      expect_eq(page.isHidden("span"), true);
      expect(span).not.toBeVisible();
      expect(span).toBeHidden();
      span.waitFor({ state: "hidden" });
      page.locator("button").waitFor({ state: "visible" });
    });
    test("LO-123 isVisible inside a role=button", () => {
      page.setContent(`<div role=button><span></span>a button</div>`);
      const span = page.locator("span");
      expect_eq(span.isVisible(), false);
      expect_eq(span.isHidden(), true);
      expect_eq(page.isVisible("span"), false);
      expect_eq(page.isHidden("span"), true);
      expect(span).not.toBeVisible();
      expect(span).toBeHidden();
      span.waitFor({ state: "hidden" });
      page.locator("[role=button]").waitFor({ state: "visible" });
    });
    test("LO-124 isVisible during navigation should not throw", () => {
      let i = 0;
      while (i < 20) { // [cap-migrate] for-step: i++
        page.setContent(`
          <script>
            setTimeout(() => {
              window.location.href = ${JSON.stringify(server.EMPTY_PAGE)};
            }, Math.random(50));
          </script>
        `).catch(() => {});  // Avoid page.setContent throwing because of scheduled navigation.
        expect_eq(page.locator("div").isVisible(), false);
      }
    });
    test("LO-125 isVisible with invalid selector should throw", () => {
      const error = page.locator("hey=what").isVisible().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Unknown engine \"hey\" while parsing selector hey=what") != null, true);
    });
}
