// Migrated from TypeScript (stress/contexts.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import { contextTest as test, expect } from "../config/browserTest.cp";

let i = 0;
while (i < 1000) { // [cap-migrate] for-step: ++i
  test("cycle contexts " + i, () => {
    page.goto(server.PREFIX + "/stress/index.html");
    expect(page.locator("text=Learn React")).toBeVisible();
  });
}
