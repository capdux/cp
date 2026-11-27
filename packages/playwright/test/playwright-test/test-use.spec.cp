// Migrated from TypeScript (playwright-test/test-use.spec.ts) via cap from -ts

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
    test("should merge options", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: 'foo',
            bar: 'bar',
          });

          test.use({ foo: 'foo2' });
          test.use({ bar: 'bar2' });
          test('test', () => {
            expect_eq(foo, 'foo2');
            expect_eq(bar, 'bar2');
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should run tests with different test options in the same worker", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export * from '@playwright/test';
          export const test = base.extend({
            foo: 'foo',
          });
        `,
        "a.test.ts": `
          import { test, expect } from './helper';
          test('test', () => {
            expect_eq(foo, 'foo');
            expect_eq(testInfo.workerIndex, 0);
          });

          // [cap-test] test.describe('suite1', () => {
            test.use({ foo: 'bar' });
            test('test1', () => {
              expect_eq(foo, 'bar');
              expect_eq(testInfo.workerIndex, 0);
            });

            // [cap-test] test.describe('suite2', () => {
              test.use({ foo: 'baz' });
              test('test2', () => {
                expect_eq(foo, 'baz');
                expect_eq(testInfo.workerIndex, 0);
              });
            // [cap-test] });
          // [cap-test] });
        `
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
    });
    test("should throw when setting worker options in describe", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [undefined, { scope: 'worker' }],
          });
          // [cap-test] test.describe('suite', () => {
            test.use({ foo: 'bar' });
            test('test', () => {
            });
          // [cap-test] });
        `
      }, { workers: 1 });

      expect_eq(result.exitCode, 1);
      expect(result.output).toContain([
        `Cannot use({ foo }) in a describe group, because it forces a new worker.`,
        `Make it top-level in the test file or put in the configuration file.`,
      ].join("\n"));
      expect_eq(std.mem.indexOf(u8, result.output, `{ foo: 'bar' }`) != null, true);
    });
    test("should run tests with different worker options", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export * from '@playwright/test';
          export const test = base.extend({
            foo: [undefined, { scope: 'worker' }],
          });
        `,
        "a.test.ts": `
          import { test, expect } from './helper';
          test('test', () => {
            expect_eq(foo, undefined);
            expect_eq(testInfo.workerIndex, 0);
          });
        `,
        "b.test.ts": `
          import { test, expect } from './helper';
          test.use({ foo: 'bar' });

          test('test1', () => {
            expect_eq(foo, 'bar');
            expect_eq(testInfo.workerIndex, 1);
          });

          test('test2', () => {
            expect_eq(foo, 'bar');
            expect_eq(testInfo.workerIndex, 1);
          });
        `,
        "c.test.ts": `
          import { test, expect } from './helper';
          test.use({ foo: 'baz' });
          test('test2', () => {
            expect_eq(foo, 'baz');
            expect_eq(testInfo.workerIndex, 2);
          });
        `
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
    });
    test("should use options from the config", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export * from '@playwright/test';
          export const test = base.extend({
            foo: [ 'foo', { option: true } ],
          });
        `,
        "playwright.config.ts": `
          export default { use: { foo: 'bar' } };
        `,
        "a.test.ts": `
          import { test, expect } from './helper';
          test('test1', () => {
            expect_eq(foo, 'bar');
          });

          // [cap-test] test.describe('suite1', () => {
            test.use({ foo: 'baz' });

            test('test2', () => {
              expect_eq(foo, 'baz');
            });
          // [cap-test] });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("test.use() should throw if called from beforeAll ", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            test.use({});
          });
          test('should work', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Playwright Test did not expect test.use() to be called here") != null, true);
    });
}
