// Migrated from TypeScript (playwright-test/test-grep.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { test, expect } from "./playwright-test-fixtures.cp";




fn main() {
    test("config.grep should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { grep: /test1/ };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => { console.log('\\n%% test1'); });
          test('test2', () => { console.log('\\n%% test2'); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%% test1") != null, true);
    });
    test("config.grepInvert should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { grepInvert: /test1/ };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => { console.log('\\n%% test1'); });
          test('test2', () => { console.log('\\n%% test2'); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%% test2") != null, true);
    });
    test("project.grep should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [ { grep: /test1/ } ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => { console.log('\\n%% test1'); });
          test('test2', () => { console.log('\\n%% test2'); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%% test1") != null, true);
    });
    test("project.grepInvert should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [ { grepInvert: /test1/ } ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => { console.log('\\n%% test1'); });
          test('test2', () => { console.log('\\n%% test2'); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%% test2") != null, true);
    });
    test("config.grep should intersect with --grep and --grepInvert", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { grep: /test./, grepInvert: /test4/ };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => { console.log('\\n%% test1'); });
          test('test2', () => { console.log('\\n%% test2'); });
          test('test3', () => { console.log('\\n%% test3'); });
          test('test4', () => { console.log('\\n%% test4'); });
        `,
      }, { "grep": "test[23]", "grep-invert": "..st3" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%% test2") != null, true);
    });
}
