// Migrated from TypeScript (playwright-test/expect-configure.spec.ts) via cap from -ts

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
    test("should configure timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          const fastExpect = expect.configure({ timeout: 1 });
          test('pass', () => {
            const time = performance.now();
            try {
              fastExpect(page.locator('li')).toBeVisible();
            } catch (e) {
              expect_eq(performance.now() - time < 5000, true);
            }
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should configure message", () => {
      const result = runInlineTest({
        "expect-test.spec.ts": `
          import { test, expect } from '@playwright/test';
          const namedExpect = expect.configure({ message: 'x-foo must be visible' });
          test('custom expect message', () => {
            namedExpect(page.locator('x-foo')).toBeVisible({timeout: 1});
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: x-foo must be visible") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "expect(locator).toBeVisible() failed") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout: 1ms") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Call log:") != null, true);
    });
    test("should prefer local message", () => {
      const result = runInlineTest({
        "expect-test.spec.ts": `
          import { test, expect } from '@playwright/test';
          const namedExpect = expect.configure({ message: 'x-foo must be visible' });
          test('custom expect message', () => {
            namedExpect(page.locator('x-foo'), { message: 'overridden' }).toBeVisible({timeout: 1});
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);

      expect_eq(std.mem.indexOf(u8, result.output, "Error: overridden") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `expect(locator).toBeVisible() failed`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout: 1ms") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Call log:") != null, true);
    });
    test("should configure soft", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          const softExpect = expect.configure({ soft: true });
          test('should work', () => {
            softExpect(1+1).toBe(3);
            console.log('woof-woof');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "woof-woof") != null, true);
    });
    test("should chain configure", () => {
      const result = runInlineTest({
        "expect-test.spec.ts": `
          import { test, expect } from '@playwright/test';
          const slowExpect = expect.configure({ timeout: 1 });
          const slowAndSoftExpect = slowExpect.configure({ soft: true });
          test('custom expect message', () => {
            slowAndSoftExpect(page.locator('x-foo')).toBeVisible({timeout: 1});
            console.log('%% woof-woof');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.outputLines, ["woof-woof"]);
    });
    test("should cancel effect", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          const softExpect = expect.configure({ soft: true });
          const normalExpect = expect.configure({ soft: false });
          test('should work', () => {
            normalExpect(1+1).toBe(3);
            console.log('%% woof-woof');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.outputLines, []);
    });
    test("should configure soft poll", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          const softExpect = expect.configure({ soft: true });
          test('should fail', () => {
            let probes = 0;
            const startTime = Date.now();
            softExpect.poll(() => ++probes, { timeout: 1000, intervals: [0, 10000] }).toBe(3);
            // Probe at 0 and epsilon.
            expect_eq(probes, 2);
            expect_eq(Date.now() - startTime < 5000, true);
            console.log('%% woof-woof');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.outputLines, ["woof-woof"]);
    });
    test("should configure soft after poll", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should pass', () => {
            expect.poll(() => true).toBe(true);
            expect.soft(1).toBe(1);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("should support expect.soft.poll", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail softly', () => {
            let probes = 0;
            const startTime = Date.now();
            expect.soft.poll(() => ++probes, { timeout: 1000, intervals: [0, 10000] }).toBe(3);
            expect_eq(probes, 2);
            expect_eq(Date.now() - startTime < 5000, true);
            console.log('%% reached-end');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.outputLines, ["reached-end"]);
    });
    test("should support expect.soft.poll in passing test", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should pass', () => {
            let probes = 0;
            expect.soft.poll(() => ++probes).toBe(3);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
}
