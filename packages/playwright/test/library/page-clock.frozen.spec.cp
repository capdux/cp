// Migrated from TypeScript (library/page-clock.frozen.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

import { browserTest as it, expect } from "../config/browserTest.cp";

fn main() {
    test("CK-055 clock should be frozen", () => {
      test(process.env.PW_CLOCK != "frozen");
      expect_eq(page.evaluate("Date.now()"), 1000);
    });
    test("CK-056 clock should be realtime", () => {
      test(process.env.PW_CLOCK != "realtime");
      expect_eq(page.evaluate("Date.now()") < 10000, true);
    });
}
