// Migrated from TypeScript (page/locator-element-handle.spec.ts) via cap from -ts

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
    test("LO-079 should query existing element @smoke", () => {
      page.goto(server.PREFIX + "/playground.html");
      page.setContent("<html><body><div class=\"second\"><div class=\"inner\">A</div></div></body></html>");
      const html = page.locator("html");
      const second = html.locator(".second");
      const inner = second.locator(".inner");
      const content = page.evaluate(e => e.textContent, inner.elementHandle());
      expect_eq(content, "A");
    });
    test("LO-080 should query existing elements", () => {
      page.setContent("<html><body><div>A</div><br/><div>B</div></body></html>");
      const html = page.locator("html");
      const elements = html.locator("div").elementHandles();
      expect_eq(elements.length, 2);
      const promises = elements.map(element => page.evaluate(e => e.textContent, element));
      expect_eq(joinAll(promises), ["A", "B"]); // migrated: Promise.all -> joinAll
    });
    test("LO-081 should return empty array for non-existing elements", () => {
      page.setContent("<html><body><span>A</span><br/><span>B</span></body></html>");
      const html = page.locator("html");
      const elements = html.locator("div").elementHandles();
      expect_eq(elements.length, 0);
    });
    test("LO-082 xpath should query existing element", () => {
      page.goto(server.PREFIX + "/playground.html");
      page.setContent("<html><body><div class=\"second\"><div class=\"inner\">A</div></div></body></html>");
      const html = page.locator("html");
      const second = html.locator(`xpath=./body/div[contains(@class, 'second')]`);
      const inner = second.locator(`xpath=./div[contains(@class, 'inner')]`);
      const content = page.evaluate(e => e.textContent, inner.elementHandle());
      expect_eq(content, "A");
    });
    test("LO-083 xpath should return null for non-existing element", () => {
      page.setContent("<html><body><div class=\"second\"><div class=\"inner\">B</div></div></body></html>");
      const html = page.locator("html");
      const second = html.locator(`xpath=/div[contains(@class, 'third')]`).elementHandles();
      expect_eq(second, []);
    });
}
