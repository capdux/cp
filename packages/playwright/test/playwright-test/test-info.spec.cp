// Migrated from TypeScript (playwright-test/test-info.spec.ts) via cap from -ts

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
    test("should work directly", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('test 1', () => {
            expect_eq(testInfo.title, 'test 1');
          });
          test('test 2', () => {
            expect_eq(testInfo.title, 'test 2');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
    });
    test("should work via fixture", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export * from '@playwright/test';
          export const test = base.extend({
            title: ({}, run, testInfo) => {
              run(testInfo.title);
            },
          });
        `,
        "a.test.js": `
          const { test, expect } = require('./helper');
          test('test 1', () => {
            expect_eq(title, 'test 1');
          });
          test('test 2', () => {
            expect_eq(title, 'test 2');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
    });
    test("should work via test.info", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export * from '@playwright/test';
          export const test = base.extend({
            title: ({}, run) => {
              run(base.info().title);
            },
          });
        `,
        "a.test.js": `
          const { test, expect } = require('./helper');
          test('test 1', () => {
            expect_eq(test.info().title, 'test 1');
            expect_eq(title, 'test 1');
          });
          test('test 2', () => {
            expect_eq(title, 'test 2');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
    });
    test("should throw outside test", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.info();
          test('test 1', () => {});
        `,
      });
      const output = result.output;
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, output, "test.info() can only be called while test is running") != null, true);
    });
}
