// Migrated from TypeScript (playwright-test/override-timeout.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

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

import { test, expect } from "./playwright-test-fixtures.cp";


fn main() {
    test("should consider dynamically set value", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { timeout: 2000 };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect_eq(testInfo.timeout, 2000);
          })
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should allow different timeouts", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { projects: [
            { timeout: 2000 },
            { timeout: 4000 },
          ] };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('timeout:' + testInfo.timeout);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(std.mem.indexOf(u8, result.output, "timeout:2000") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "timeout:4000") != null, true);
    });
    test("should prioritize value set via command line", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { timeout: 2000 };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect_eq(testInfo.timeout, 1000);
          })
        `
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
}
