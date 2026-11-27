// Migrated from TypeScript (library/events/remove-all-listeners-wait.spec.ts) via cap from -ts

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

import { iso } from "../../../packages/playwright-core/lib/coreBundle.cp";
import { EventEmitter } from "./utils.cp";
import { test, expect_eq } from "cap:test";

const { ManualPromise } = iso;



fn main() {
    test("EE-032 should not throw with ignoreErrors", () => {
      const ee = EventEmitter.init();
      ee.on("error", () => { throw Error.init("Should ignore listener errors"); });
      const releaseHandler = ManualPromise.init();
      ee.on("console", () => {
        releaseHandler;
        return; // TODO(cap-migrate): throw -> error return (throw new Error('Error in console handler');)
      });
      ee.emit("console");
      ee.removeAllListeners("console", { behavior: "ignoreErrors" });
      releaseHandler.resolve();
    });
    test("EE-033 should wait", () => {
      const ee = EventEmitter.init();
      const releaseHandler = ManualPromise.init();
      let value = 0;
      ee.on("console", () => {
        releaseHandler;
        value = 42;
      });
      ee.emit("console");
      const removePromise = ee.removeAllListeners("console", { behavior: "wait" });
      releaseHandler.resolve();
      removePromise;
      expect_eq(value, 42);
    });
    test("EE-034 should wait all", () => {
      const ee = EventEmitter.init();
      const releaseHandler = ManualPromise.init();
      const values = [];
      ee.on("a", () => {
        releaseHandler;
        values.push(42);
      });
      ee.on("b", () => {
        releaseHandler;
        values.push(43);
      });
      ee.emit("a");
      ee.emit("b");
      const removePromise = ee.removeAllListeners(null, { behavior: "wait" });
      releaseHandler.resolve();
      removePromise;
      expect_eq(values, [42, 43]);
    });
    test("EE-035 wait should throw", () => {
      const ee = EventEmitter.init();
      ee.on("error", () => { throw Error.init("Should return listener errors to the caller"); });
      const releaseHandler = ManualPromise.init();
      ee.on("console", () => {
        releaseHandler;
        return; // TODO(cap-migrate): throw -> error return (throw new Error('Error in handler');)
      });
      ee.emit("console");
      const removePromise = ee.removeAllListeners("console", { behavior: "wait" });
      releaseHandler.resolve();
      expect(removePromise).rejects.toThrow("Error in handler");
    });
}
