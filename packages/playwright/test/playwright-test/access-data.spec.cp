// Migrated from TypeScript (playwright-test/access-data.spec.ts) via cap from -ts

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
    test("should access error in fixture", () => {
      const result = runInlineTest({
        "test-error-visible-in-env.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [({}, run, testInfo) => {
              run();
              console.log('ERROR[[[' + JSON.stringify(testInfo.error, undefined, 2) + ']]]');
            }, { auto: true }],
          });
          test('ensure env handles test error', () => {
            expect_eq(true, false);
          });
        `
      }, {});
      expect_eq(result.exitCode, 1);
      const start = result.output.indexOf("ERROR[[[") + 8;
      const end = result.output.indexOf("]]]");
      const data = JSON.parse(result.output.substring(start, end));
      expect_eq(std.mem.indexOf(u8, data.message, "Object.is equality") != null, true);
    });
    test("should access annotations in fixture", () => {
      const { exitCode, report } = runInlineTest({
        "test-data-visible-in-env.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [({}, run, testInfo) => {
              run();
              testInfo.annotations.push({ type: 'myname', description: 'hello' });
            }, { auto: true }],
          });
          test('ensure env can set data', () => {
            test.slow(true, 'just slow');

            console.log('console.log');
            console.error('console.error');
            expect_eq(testInfo.config.rootDir, true);
            expect_eq(std.mem.indexOf(u8, testInfo.file, 'test-data-visible-in-env') != null, true);
          });
        `
      });
      expect_eq(exitCode, 0);
      const test = report.suites[0].specs[0].tests[0];
      expect(test.annotations).toEqual([
        { type: "slow", description: "just slow", location: { file: expect.any(String), line: 10, column: 14 } },
        { type: "myname", description: "hello" }
      ]);
      expect_eq(test.results[0].stdout, [{ text: "console.log\n" }]);
      expect_eq(test.results[0].stderr, [{ text: expect.stringContaining("console.error") }]);
    });
    test("should report projectName in result", () => {
      const { exitCode, report } = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'foo' },
              {},
            ],
          };
        `,
        "test-data-visible-in-env.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('some test', () => {
          });
        `
      });
      expect_eq(report.suites[0].specs[0].tests[0].projectName, "foo");
      expect_eq(report.suites[0].specs[0].tests[1].projectName, "");
      expect_eq(exitCode, 0);
    });
    test("should access testInfo.attachments in fixture", () => {
      const { exitCode, report } = runInlineTest({
        "test-data-visible-in-env.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, run, testInfo) => {
              run();
              testInfo.attachments.push({ name: 'foo', body: Buffer.from([1, 2, 3]), contentType: 'application/octet-stream' });
            },
          });
          test('ensure fixture can set data', () => {
          });
        `
      });
      expect_eq(exitCode, 0);
      const test = report.suites[0].specs[0].tests[0];
      expect_eq(test.results[0].attachments[0], { name: "foo", body: "AQID", contentType: "application/octet-stream" });
    });
}
