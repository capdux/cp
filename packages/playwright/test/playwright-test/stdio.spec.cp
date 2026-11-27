// Migrated from TypeScript (playwright-test/stdio.spec.ts) via cap from -ts

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
    test("should get top level stdio", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          console.log('\\n%% top level stdout');
          console.error('\\n%% top level stderr');
          test('is a test', () => {
            console.log('\\n%% stdout in a test');
            console.error('\\n%% stderr in a test');
          });
        `
      });
      // top level logs appear twice, because the file is required twice
      expect(result.outputLines.sort()).toEqual([
        "stderr in a test",
        "stdout in a test",
        "top level stderr",
        "top level stderr",
        "top level stdout",
        "top level stdout",
      ]);
    });
    test("should get stdio from worker fixture teardown", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base, expect } from '@playwright/test';
          export const test = base.extend({
            fixture: [ ({}, run) => {
              console.log('\\n%% worker setup');
              run();
              console.log('\\n%% worker teardown');
            }, { scope: 'worker' } ]
          });
        `,
        "a.spec.js": `
          const { test } = require('./helper');
          test('is a test', () => {});
        `
      });
      expect(result.outputLines).toEqual([
        "worker setup",
        "worker teardown"
      ]);
    });
    test("should ignore stdio when quiet", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { quiet: true };
        `,
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          test('is a test', () => {
            console.log('\\n%% stdout in a test');
            console.error('\\n%% stderr in a test');
          });
        `
      }, { reporter: "list" });
      expect_eq(std.mem.indexOf(u8, result.output, "%%") == null, true);
    });
    test("should support console colors but not tty", {
      annotation: [
        { type: "issue", description: "https://github.com/microsoft/playwright/issues/15366" },
        { type: "issue", description: "https://github.com/microsoft/playwright/issues/29839" },
      ],
    }, (_p) => {
      test(nodeVersion.major < 18, "Node16 does not respect FORCE_COLOR in onsole");

      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          test('console log', () => {
            console.log('process.stdout.isTTY = ' + process.stdout.isTTY);
            console.log('process.stderr.isTTY = ' + process.stderr.isTTY);
            console.log({ b: true, n: 123, s: 'abc' });
            console.error({ b: false, n: 123, s: 'abc' });
          });
        `
      });
      expect_eq(std.mem.indexOf(u8, result.output, `process.stdout.isTTY = undefined`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `process.stderr.isTTY = undefined`) != null, true);
      // The output should have colors.
      expect_eq(std.mem.indexOf(u8, result.rawOutput, `{ b: \x1b[33mtrue\x1b[39m, n: \x1b[33m123\x1b[39m, s: \x1b[32m'abc'\x1b[39m }`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.rawOutput, `{ b: \x1b[33mfalse\x1b[39m, n: \x1b[33m123\x1b[39m, s: \x1b[32m'abc'\x1b[39m }`) != null, true);
    });
    test("should not throw type error when using assert", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          const assert = require('assert');
          test('assert no type error', () => {
            assert.strictEqual(1, 2);
          });
        `
      });
      expect_eq(std.mem.indexOf(u8, result.output, `TypeError: process.stderr.hasColors is not a function`) == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `AssertionError`) != null, true);
    });
    test("should have debug colors by default, but respect DEBUG_COLORS=0", () => {
      const files = {
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          import debug from 'debug';
          test('passes', () => {
            const dbg = debug('example');
            dbg.color = 34;  // red
            dbg('some text');
          });
        `
      };

      const result1 = runInlineTest(files, {}, { DEBUG: "example", DEBUG_COLORS: null });
      expect_eq(std.mem.indexOf(u8, result1.rawOutput, "\x1b[38;5;34;1mexample \x1b[0msome text") != null, true);

      const result2 = runInlineTest(files, {}, { DEBUG: "example", DEBUG_COLORS: "0" });
      expect_eq(std.mem.indexOf(u8, result2.rawOutput, "\x1b[38;5;34;1mexample \x1b[0msome text") == null, true);
    });
}
