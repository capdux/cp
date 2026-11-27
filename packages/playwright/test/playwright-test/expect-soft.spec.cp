// Migrated from TypeScript (playwright-test/expect-soft.spec.ts) via cap from -ts

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
    test("soft expects should compile", () => {
      const result = runTSC({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            test.expect.soft(1+1).toBe(3);
            test.expect.soft(1+1, 'custom expect message').toBe(3);
            test.expect.soft(1+1, { message: 'custom expect message' }).toBe(3);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("soft expects should work", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            test.expect.soft(1+1).toBe(3);
            console.log('%% woof-woof');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.outputLines, ["woof-woof"]);
    });
    test("should report a mixture of soft and non-soft errors", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            test.expect.soft(1+1, 'one plus one').toBe(3);
            test.expect.soft(2*2, 'two times two').toBe(5);
            test.expect_eq(3/3, 'three div three', 7);
            test.expect.soft(6-4, { message: 'six minus four' }).toBe(3);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: one plus one") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: two times two") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: three div three") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: six minus four") == null, true);
    });
    test("testInfo should contain all soft expect errors", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            test.expect.soft(1+1, 'one plus one').toBe(3);
            test.expect.soft(2*2, 'two times two').toBe(5);
            test.expect_eq(testInfo.errors.length, 'must be exactly two errors', 2);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: one plus one") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: two times two") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: must be exactly two errors") == null, true);
    });
}
