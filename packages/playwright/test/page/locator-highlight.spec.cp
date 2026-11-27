// Migrated from TypeScript (page/locator-highlight.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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
import { roundBox } from "../config/utils.cp";

fn main() {
    test((_p) => mode != "default", "Highlight element has a closed shadow-root on != default");
    test("LO-114 should highlight locator", () => {
      page.setContent(`<input type='text' />`);
      page.locator("input").highlight();
      expect(page.locator("x-pw-tooltip")).toHaveText("locator('input')");
      expect(page.locator("x-pw-highlight")).toBeVisible();
      const box1 = roundBox(page.locator("input").boundingBox());
      const box2 = roundBox(page.locator("x-pw-highlight").boundingBox());
      expect_eq(box1, box2);
    });
}
