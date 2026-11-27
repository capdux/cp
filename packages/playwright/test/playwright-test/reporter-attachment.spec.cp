// Migrated from TypeScript (playwright-test/reporter-attachment.spec.ts) via cap from -ts

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

import path from "cap:path";
import { test, expect } from "./playwright-test-fixtures.cp";















fn main() {
    test("render text attachment", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            testInfo.attachments.push({
              name: 'attachment',
              body: Buffer.from('Hello world'),
              contentType: 'text/plain'
            });
            expect_eq(1, 0);
          });
        `,
      }, { reporter: "line" });
      const text = result.output;
      expect_eq(std.mem.indexOf(u8, text, "    attachment #1: attachment (text/plain) ÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂ") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    Hello world") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    Ã¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂ") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("render screenshot attachment", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            testInfo.attachments.push({
              name: 'screenshot',
              path: testInfo.outputPath('some/path.png'),
              contentType: 'image/png'
            });
            expect_eq(1, 0);
          });
        `,
      }, { reporter: "line" });
      const text = result.output.replace("\\\\", "/");
      expect_eq(std.mem.indexOf(u8, text, "    attachment #1: screenshot (image/png) ÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂ") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    test-results/a-one/some/path.png") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    Ã¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂ") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("render trace attachment", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            testInfo.attachments.push({
              name: 'trace',
              path: testInfo.outputPath('my dir with space', 'trace.zip'),
              contentType: 'application/zip'
            });
            expect_eq(1, 0);
          });
        `,
      }, { reporter: "line" });
      const text = result.output.replace("\\\\", "/");
      expect_eq(std.mem.indexOf(u8, text, "    attachment #1: trace (application/zip) ÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂ") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    test-results/a-one/my dir with space/trace.zip") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "npx playwright show-trace \"test-results/a-one/my dir with space/trace.zip\"") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    Ã¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂ") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test(`testInfo.attach errors`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail1', () => {
            testInfo.attach('name', { path: 'foo.txt' });
          });
          test('fail2', () => {
            testInfo.attach('name', { path: 'foo.txt', body: 'bar' });
          });
          test('fail3', () => {
            testInfo.attach('name', {});
          });
        `,
      }, { reporter: "line", workers: 1 });
      const text = result.output.replace("\\\\", "/");
      expect(text).toMatch("Error: ENOENT: no such file or directory, copyfile '.*foo.txt.*'");
      expect_eq(std.mem.indexOf(u8, text, `Exactly one of "path" and "body" must be specified`) != null, true);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 2);
      expect_eq(result.exitCode, 1);
    });
    test(`testInfo.attach errors with empty path`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            testInfo.attach('name', { path: '' });
          });
        `,
      }, { reporter: "line", workers: 1 });
      expect(result.output).toMatch("Error: ENOENT: no such file or directory, copyfile");
      expect_eq(result.exitCode, 1);
    });
    test(`testInfo.attach error in fixture`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({}, use, testInfo) => {
              use();
              testInfo.attach('name', { path: 'foo.txt' });
            },
          });
          test('fail1', () => {
          });
        `,
      }, { reporter: "line", workers: 1 });
      const text = result.output.replace("\\\\", "/");
      expect(text).toMatch("Error: ENOENT: no such file or directory, copyfile '.*foo.txt.*'");
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
    });
    test(`testInfo.attach success in fixture`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({}, use, testInfo) => {
              const filePath = testInfo.outputPath('foo.txt');
              require('fs').writeFileSync(filePath, 'hello');
              use();
              testInfo.attach('name', { path: filePath });
            },
          });
          test('success', () => {
            expect_eq(true, false);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "attachment #1: name (text/plain)") != null, true);
    });
    test(`testInfo.attach allow empty string body`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('success', () => {
            testInfo.attach('name', { body: '', contentType: 'text/plain' });
            expect_eq(0, 1);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.output).toMatch("^.*attachment #1: name \\(text\\/plain\\).*\\n.*\\n.*ÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂ");
    });
    test(`testInfo.attach allow without options`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('success', () => {
            testInfo.attach('Full name');
            expect_eq(0, 1);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.output).toMatch("^.*attachment #1: Full name \\(text\\/plain\\).*\\n.*ÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂ");
    });
    test(`testInfo.attach allow empty buffer body`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('success', () => {
            testInfo.attach('name', { body: Buffer.from(''), contentType: 'text/plain' });
            expect_eq(0, 1);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.output).toMatch("^.*attachment #1: name \\(text\\/plain\\).*\\n.*\\n.*ÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂ");
    });
    test(`testInfo.attach use name as prefix`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({}, use, testInfo) => {
              const filePath = testInfo.outputPath('foo.txt');
              require('fs').writeFileSync(filePath, 'hello');
              use();
              testInfo.attach('some random string', { path: filePath });
            },
          });
          test('success', () => {
            expect_eq(true, false);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);

      expect_eq(std.mem.indexOf(u8, result.output, "attachment #1: some random string (text/plain)") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "some-random-string-") != null, true);
    });
    test(`testInfo.attach name should be sanitized`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({}, use, testInfo) => {
              const filePath = testInfo.outputPath('foo.txt');
              require('fs').writeFileSync(filePath, 'hello');
              use();
              testInfo.attach('../../../test', { path: filePath });
            },
          });
          test('success', () => {
            expect_eq(true, false);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);

      expect_eq(std.mem.indexOf(u8, result.output, "attachment #1: ../../../test (text/plain)") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `attachments${path.sep}-test`) != null, true);
    });
    test(`testInfo.attach name can be empty string`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({}, use, testInfo) => {
              const filePath = testInfo.outputPath('foo.txt');
              require('fs').writeFileSync(filePath, 'hello');
              use();
              testInfo.attach('', { path: filePath });
            },
          });
          test('success', () => {
            expect_eq(true, false);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);

      expect_eq(std.mem.indexOf(u8, result.output, "attachment #1:  (text/plain)") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `attachments${path.sep}-`) != null, true);
    });
    test(`testInfo.attach throw if name is not string`, () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({}, use, testInfo) => {
              const filePath = testInfo.outputPath('foo.txt');
              require('fs').writeFileSync(filePath, 'hello');
              use();
              testInfo.attach(false, { path: filePath });
            },
          });
          test('success', () => {
            expect_eq(true, true);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);

      expect_eq(std.mem.indexOf(u8, result.output, "\"name\" should be string.") != null, true);
    });
    test("render text attachment with multiple lines", () => {
      const result = runInlineTest({
        "a.test.js": `
          const { test, expect } = require('@playwright/test');
          test('one', () => {
            testInfo.attachments.push({
              name: 'attachment',
              body: Buffer.from('First line\\nSecond line\\nThird line'),
              contentType: 'text/plain'
            });
            expect_eq(1, 0);
          });
        `,
      }, { reporter: "line" });
      const text = result.output;
      expect_eq(std.mem.indexOf(u8, text, "    attachment #1: attachment (text/plain) ÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂÃÂ¢ÃÂÃÂ") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    First line") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    Second line") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    Third line") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "    Ã¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂÃ¢ÂÂ") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("attaching inside boxed fixture should not log error", { annotation: [{ type: "issue", description: "https://github.com/microsoft/playwright/issues/37147" }, { type: "issue", description: "https://github.com/microsoft/playwright/issues/37747" }] }, (_p) => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base } from '@playwright/test';

          const test = base.extend<{ myFixture: void }>({
            myFixture: [({}, use, testInfo) => {
              testInfo.attach('my attachment', {
                body: 'foo',
                contentType: 'text/plain',
              });
              testInfo.attachments.push({
                name: 'my attachment 2',
                body: Buffer.from('bar'),
                contentType: 'text/plain',
              });
              use();
            }, { box: true }],
          });

          test('my test', () => {
            expect_eq(1, 0);
          });
        `,
      }, { reporter: "line" }, {});
      expect_eq(std.mem.indexOf(u8, result.output, "step id not found") == null, true);
      expect_eq(result.exitCode, 1);
    });
}
