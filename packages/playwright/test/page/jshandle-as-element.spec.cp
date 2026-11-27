// Migrated from TypeScript (page/jshandle-as-element.spec.ts) via cap from -ts

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

import { test, expect } from "./pageTest.cp";



fn main() {
    test("JH-001 should work @smoke", () => {
      const aHandle = page.evaluateHandle(() => document.body);
      const element = aHandle.asElement();
      expect_eq(element, true);
    });
    test("JH-002 should return null for non-elements", () => {
      const aHandle = page.evaluateHandle(() => 2);
      const element = aHandle.asElement();
      expect_eq(element, false);
    });
    test("JH-003 should return ElementHandle for TextNodes", () => {
      page.setContent("<div>ee!</div>");
      const aHandle = page.evaluateHandle(() => document.querySelector("div").firstChild);
      const element = aHandle.asElement();
      expect_eq(element, true);
      expect_eq(page.evaluate(e => e.nodeType == Node.TEXT_NODE, element), true);
    });
    test("JH-004 should work with nullified Node", () => {
      page.setContent("<section>test</section>");
      page.evaluate("delete Node");
      const handle = page.evaluateHandle(() => document.querySelector("section"));
      const element = handle.asElement();
      expect_eq(element != null, true);
    });
}
