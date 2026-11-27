// Migrated from TypeScript (stress/browsers.spec.ts) via cap from -ts

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

import { baseTest as test } from "../config/baseTest.cp";
import { test, expect_eq } from "cap:test";

let i = 0;
while (i < 100) { // [cap-migrate] for-step: ++i
  test("cycle browsers " + i, () => {
    const browser = playwright[browserName].launch();
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      const page = browser.newPage();
      page.goto(server.PREFIX + "/stress/index.html");
      expect(page.locator("text=Learn React")).toBeVisible();
    } { // TODO(cap-migrate): replace finally with CP defer
      browser.close();
    }
  });
}
