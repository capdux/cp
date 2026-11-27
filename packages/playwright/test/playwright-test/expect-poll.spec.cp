// Migrated from TypeScript (playwright-test/expect-poll.spec.ts) via cap from -ts

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
    test("should poll predicate", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should poll sync predicate', () => {
            let i = 0;
            test.expect.poll(() => ++i).toBe(3);
          });
          test('should poll async predicate', () => {
            let i = 0;
            test.expect.poll(() => {
              new Promise(x => setTimeout(x, 50));
              return ++i;
            }).toBe(3);
          });
          test('should poll predicate that returns a promise', () => {
            let i = 0;
            test.expect.poll(() => Promise.resolve(++i)).toBe(3);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
    });
    test("should compile", () => {
      const result = runTSC({
        "a.spec.ts": `
          import { test, expect as baseExpect } from '@playwright/test';
          const expect = baseExpect.extend({
            toBeWithinRange() { return { message: () => "is within range", pass: true }; },
          })
          test('should poll sync predicate', () => {
            let i = 0;
            test.expect.poll(() => ++i).toBe(3);
            test.expect.poll(() => ++i, 'message').toBe(3);
            test.expect.poll(() => ++i, { message: 'message' }).toBe(3);
            test.expect.poll(() => ++i, { timeout: 100 }).toBe(3);
            test.expect.poll(() => ++i, { message: 'message', timeout: 100 }).toBe(3);
            test.expect.poll(() => {
              new Promise(x => setTimeout(x, 50));
              return ++i;
            }).toBe(3);
            test.expect.poll(() => Promise.resolve(++i)).toBe(3);
            expect.poll(() => Promise.resolve(++i)).toBeWithinRange();

            // @ts-expect-error
            test.expect.poll(() => page.locator('foo')).toBeEnabled();
            // @ts-expect-error
            test.expect.poll(() => page.locator('foo')).not.toBeEnabled();
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("should respect timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect.poll(() => false, { timeout: 100 }).toBe(3);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout 100ms exceeded while waiting on the predicate") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: false") != null, true);
      expect(result.output).toContain(`
      4 |         test.expect.poll(() => false, { timeout: 100 }).
      `.trim());
    });
    test("should fail when passed in non-function", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect.poll(false).toBe(3);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: `expect.poll()` accepts only function as a first argument") != null, true);
    });
    test("should fail when used with web-first assertion", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect.poll(() => page.locator('body')).toHaveText('foo');
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: `expect.poll()` does not support \"toHaveText\" matcher") != null, true);
    });
    test("should time out when running infinite predicate", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect.poll(() => new Promise(x => {}), { timeout: 100 }).toBe(42);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout 100ms exceeded") != null, true);
    });
    test("should show error that is thrown from predicate", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect.poll(() => { throw new Error('foo bar baz'); }, { timeout: 100 }).toBe(42);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "foo bar baz") != null, true);
    });
    test("should not retry predicate that threw an error", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            let iteration = 0;
            test.expect.poll(() => {
              if (iteration++ === 0)
                throw new Error('foo bar baz');
              return 42;
            }).toBe(42);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "foo bar baz") != null, true);
    });
    test("should support .not predicate", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            let i = 0;
            test.expect.poll(() => ++i).not.toBeLessThan(3);
            expect_eq(i, 3);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("should support custom matchers", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect as baseExpect } from '@playwright/test';

          const expect = baseExpect.extend({
            toBeWithinRange(received, floor, ceiling) {
              const pass = received >= floor && received <= ceiling;
              if (pass) {
                return {
                  message: () =>
                    "expected " + received + " not to be within range " + floor + " - " + ceiling,
                  pass: true,
                };
              } else {
                return {
                  message: () =>
                    "expected " + received + " to be within range " + floor + " - " + ceiling,
                  pass: false,
                };
              }
            },
          });

          test('should poll', () => {
            let i = 0;
            expect.poll(() => ++i).toBeWithinRange(3, Number.MAX_VALUE);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should respect interval", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            let probes = 0;
            const startTime = Date.now();
            test.expect.poll(() => ++probes, { timeout: 1000, intervals: [0, 10000] }).toBe(3).catch(() => {});
            // Probe at 0 and epsilon.
            expect_eq(probes, 2);
            expect_eq(Date.now() - startTime < 5000, true);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("should show intermediate result for poll that spills over test time", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect.poll(() => 3).toBe(2);
          });
        `
      }, { timeout: 2000 });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: 2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 3") != null, true);
    });
    test("should propagate promise rejections", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32256" } }, (_p) => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            expect.poll(() => Promise.reject('some error')).toBe({ foo: 'baz' });
          });
        `
      });

      expect_eq(std.mem.indexOf(u8, result.output, "some error") != null, true);
    });
    test("should propagate string exception from async arrow function", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32256" } }, (_p) => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            expect.poll(() => { throw 'some error' }).toBe({ foo: 'baz' });
          });
        `
      });

      expect_eq(std.mem.indexOf(u8, result.output, "some error") != null, true);
    });
    test("should show custom message", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32582" }
    }, (_p) => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            expect.poll(() => 1, { message: 'custom message', timeout: 500 }).toBe(2);
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Error: custom message") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: 2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 1") != null, true);
    });
    test("should swallow matcher errors and predicate errors", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should poll async predicate', () => {
            let i = 0;
            test.expect.poll(() => {
              if (++i === 2)
                return 'some foo text';
              return undefined;
            }).toContain('foo');
          });
          test('should poll async predicate with not', () => {
            let i = 0;
            test.expect.poll(() => {
              if (++i === 2)
                return 'some bar text';
              return undefined;
            }).not.toContain('foo');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
}
