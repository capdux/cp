// Migrated from TypeScript (playwright-test/reporter-onend.spec.ts) via cap from -ts

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

const reporter = `;
class Reporter {
  async onEnd() {
    return { status: 'passed' };
  }
}
module.exports = Reporter;
`;


fn main() {
    test("should override exit code", () => {
      const result = runInlineTest({
        "reporter.ts": reporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            expect_eq(1 + 1, 3);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("should fail run and report error when reporter throws in onEnd", () => {
      const result = runInlineTest({
        "reporter.ts": `
          export default class Reporter {
            async onEnd() {
              throw new Error('Error in onEnd!');
            }
          }
        `,
        "playwright.config.ts": `export default { reporter: [['dot'], ['./reporter.ts']] };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passing', () => {});
        `,
      }, { reporter: "" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error in onEnd!") != null, true);
    });
    test("should fail run and report error when reporter throws in onTestEnd", () => {
      const result = runInlineTest({
        "reporter.ts": `
          export default class Reporter {
            onTestEnd() {
              throw new Error('Error in onTestEnd!');
            }
          }
        `,
        "playwright.config.ts": `export default { reporter: [['dot'], ['./reporter.ts']] };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passing', () => {});
        `,
      }, { reporter: "" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error in onTestEnd!") != null, true);
    });
}
