// Migrated from TypeScript (playwright-test/playwright.unhandled.spec.ts) via cap from -ts

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
    test("should produce uncaughtException when page.route raises", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            page.route('**/empty.html', route => {
              throw new Error('foobar');
            });
            page.goto('${server.EMPTY_PAGE}');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "foobar") != null, true);
    });
    test("should produce unhandledRejection when page.route raises", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            page.route('**/empty.html', async route => {
              throw new Error('foobar');
            });
            page.goto('${server.EMPTY_PAGE}');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "foobar") != null, true);
    });
    test("should produce uncaughtException when context.route raises", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            context.route('**/empty.html', route => {
              throw new Error('foobar');
            });
            page.goto('${server.EMPTY_PAGE}');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "foobar") != null, true);
    });
    test("should produce unhandledRejection when context.route raises", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            context.route('**/empty.html', async route => {
              throw new Error('foobar');
            });
            page.goto('${server.EMPTY_PAGE}');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "foobar") != null, true);
    });
}
