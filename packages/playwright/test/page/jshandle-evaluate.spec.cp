// Migrated from TypeScript (page/jshandle-evaluate.spec.ts) via cap from -ts

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
    test("JH-005 should work with function @smoke", () => {
      const windowHandle = page.evaluateHandle(() => {
        window["foo"] = [1, 2];
        return window;
      });
      expect_eq(windowHandle.evaluate(w => w["foo"]), [1, 2]);
    });
    test("JH-006 should work with expression", () => {
      const windowHandle = page.evaluateHandle(() => {
        window["foo"] = [1, 2];
        return window;
      });
      expect_eq(windowHandle.evaluate("window.foo"), [1, 2]);
    });
}
