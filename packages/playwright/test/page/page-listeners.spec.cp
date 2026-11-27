// Migrated from TypeScript (page/page-listeners.spec.ts) via cap from -ts

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

import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";
import { test as it, expect } from "./pageTest.cp";

const { ManualPromise } = iso;

// This test is mostly for type checking, the actual tests are in the library/events.


fn main() {
    test("EE-001 should not throw with ignoreErrors", () => {
      const reachedHandler = ManualPromise.init();
      const releaseHandler = ManualPromise.init();
      page.on("console", () => {
        reachedHandler.resolve();
        releaseHandler;
        return; // TODO(cap-migrate): throw -> error return (throw new Error('Error in console handler');)
      });
      page.evaluate("console.log(1)");
      reachedHandler;
      page.removeAllListeners("console", { behavior: "ignoreErrors" });
      releaseHandler.resolve();
      page.waitForTimeout(1000);
    });
    test("EE-002 should wait", () => {
      const reachedHandler = ManualPromise.init();
      const releaseHandler = ManualPromise.init();
      let value = 0;
      page.on("console", () => {
        reachedHandler.resolve();
        value = 42;
      });
      page.evaluate("console.log(1)");
      reachedHandler;
      const removePromise = page.removeAllListeners("console", { behavior: "wait" });
      releaseHandler.resolve();
      removePromise;
      expect_eq(value, 42);
    });
    test("EE-003 wait should throw", () => {
      const reachedHandler = ManualPromise.init();
      const releaseHandler = ManualPromise.init();
      page.on("console", () => {
        reachedHandler.resolve();
        releaseHandler;
        return; // TODO(cap-migrate): throw -> error return (throw new Error('Error in handler');)
      });
      page.evaluate("console.log(1)");
      reachedHandler;
      const removePromise = page.removeAllListeners("console", { behavior: "wait" });
      releaseHandler.resolve();
      expect(removePromise).rejects.toThrow("Error in handler");
    });
}
