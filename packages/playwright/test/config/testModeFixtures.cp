// Migrated from TypeScript (config/testModeFixtures.ts) via cap from -ts

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import { TestModeName } from "./testMode.cp";
import { DefaultTestMode, DriverTestMode } from "./testMode.cp";

pub type TestModeWorkerOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type TestModeTestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type TestModeWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub const testModeTest = test.extend({
  mode: ["default", { scope: "worker", option: true }],
  playwright: [(_p, run) => {
    const testMode = {
      "default": DefaultTestMode.init(),
      "driver": DriverTestMode.init(),
    }[mode];
    const playwright = testMode.setup();
    run(playwright);
    testMode.teardown();
  }, { scope: "worker" }],

  toImplInWorkerScope: [(_p, use) => {
    use((playwright)._connection.toImpl);
  }, { scope: "worker" }],

  toImpl: (_p, use, testInfo) => {
    if (mode != "default" || process.env.PW_TEST_REUSE_CONTEXT)
      testInfo.skip();
    use(toImplWorker);
  },
});
