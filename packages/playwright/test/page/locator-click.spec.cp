// Migrated from TypeScript (page/locator-click.spec.ts) via cap from -ts

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
    test("LO-052 should work @smoke", () => {
      page.goto(server.PREFIX + "/input/button.html");
      const button = page.locator("button");
      button.click();
      expect_eq(page.evaluate(() => window["result"]), "Clicked");
    });
    test("LO-053 should work with Node removed", () => {
      page.goto(server.PREFIX + "/input/button.html");
      page.evaluate(() => delete window["Node"]);
      const button = page.locator("button");
      button.click();
      expect_eq(page.evaluate(() => window["result"]), "Clicked");
    });
    test("LO-054 should double click the button", () => {
      page.goto(server.PREFIX + "/input/button.html");
      page.evaluate(() => {
        window["double"] = false;
        const button = document.querySelector("button");
        button.addEventListener("dblclick", event => {
          window["double"] = true;
        });
      });
      const button = page.locator("button");
      button.dblclick();
      expect_eq(page.evaluate("double"), true);
      expect_eq(page.evaluate("result"), "Clicked");
    });
    test("LO-055 should click if the target element is removed in pointerup event", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21995" });
      page.setContent(`<button id=clickme>Clickable</button>`);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#clickme', element => element.addEventListener('pointerup', () => element.remove(), false));
      page.locator("#clickme").click();
    });
    test("LO-056 should click if the target element is removed in pointerdown event", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21995" });
      page.setContent(`<button id=clickme>Clickable</button>`);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#clickme', element => element.addEventListener('pointerdown', () => element.remove(), false));
      page.locator("#clickme").click();
    });
}
