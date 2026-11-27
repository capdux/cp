// Migrated from TypeScript (playwright-test/repeat-each.spec.ts) via cap from -ts

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
    test("should repeat from command line", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('REPEAT ' + testInfo.repeatEachIndex);
            expect_eq(1, 1);
          });
        `
      }, { "repeat-each": 3 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "REPEAT 0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "REPEAT 1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "REPEAT 2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "REPEAT 3") == null, true);
    });
    test("should repeat based on config", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { projects: [
            { name: 'no-repeats' },
            { repeatEach: 2, name: 'two-repeats' },
          ] };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('my test', () => {});
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      const names = result.report.suites[0].specs[0].tests.map(test => test.projectName);
      expect_eq(names, ["no-repeats", "two-repeats", "two-repeats"]);
    });
}
