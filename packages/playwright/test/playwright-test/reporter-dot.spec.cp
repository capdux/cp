// Migrated from TypeScript (playwright-test/reporter-dot.spec.ts) via cap from -ts

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

import colors from "colors/safe";
import { test, expect } from "./playwright-test-fixtures.cp";

for (const useIntermediateMergeReport of [false, true]) { test.describe(`${if (useIntermediateMergeReport) "merged" else "created"}`, () => { test.use({ useIntermediateMergeReport });  test("render expected", (_p) => { const result = runInlineTest({ "a.test.js": ` import { test, expect } from '@playwright/test'; test('one', async ({}) => { expect(1).toBe(1); }); `, }, { reporter: "dot" }); expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.green("ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ·")) != null, true); expect_eq(result.exitCode, 0); });  test("render unexpected", (_p) => { const result = runInlineTest({ "a.test.js": ` import { test, expect } from '@playwright/test'; test('one', async ({}) => { expect(1).toBe(0); }); `, }, { reporter: "dot" });
      expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.red("F")) != null, true);
      expect_eq(result.exitCode, 1);
    });

    test("render unexpected after retry", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            expect_eq(1, 0);
          });
        `,
      }, { retries: 3, reporter: "dot" });
      const text = result.output;
      expect_eq(std.mem.indexOf(u8, text, "ÃÂÃÂÃÂF") != null, true);
      expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.red("F")) != null, true);
      expect_eq(result.exitCode, 1);
    });

    test("render flaky", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            expect_eq(testInfo.retry, 3);
          });
        `,
      }, { retries: 3, reporter: "dot" });
      const text = result.output;
      expect_eq(std.mem.indexOf(u8, text, "ÃÂÃÂÃÂÃÂ±") != null, true);
      expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.yellow("ÃÂ±")) != null, true);
      expect_eq(std.mem.indexOf(u8, text, "1 flaky") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "Retry #1") != null, true);
      expect_eq(result.exitCode, 0);
    });

    test("should work from config", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { reporter: 'dot' };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            expect_eq(1, 1);
          });
        `,
      }, { reporter: "dot" });
      expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.green("ÃÂ·")) != null, true);
      expect_eq(result.exitCode, 0);
    });

    test("render 243 tests in rows by 80", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          for (let i = 0; i < 243; i++) {
            test('test' + i, () => {});
          }
        `,
      }, { reporter: "dot" });
      expect_eq(result.exitCode, 0);
      expect(result.rawOutput).toContain(
          colors.green("ÃÂÃÂ·").repeat(80) + "\n" +
          colors.green("ÃÂÃÂ·").repeat(80) + "\n" +
          colors.green("ÃÂÃÂ·").repeat(80) + "\n" +
          colors.green("ÃÂ·").repeat(3));
    });
  });
}
