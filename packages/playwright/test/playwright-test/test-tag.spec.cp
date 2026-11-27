// Migrated from TypeScript (playwright-test/test-tag.spec.ts) via cap from -ts

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
    test("should have correct tags", () => {
      const result = runInlineTest({
        "reporter.ts": `
          export default class Reporter {
            onBegin(config, suite) {
              const visit = suite => {
                for (const test of suite.tests || [])
                  console.log('\\n%%title=' + test.title + ', tags=' + test.tags.join(','));
                for (const child of suite.suites || [])
                  visit(child);
              };
              visit(suite);
            }
            onError(error) {
              console.log(error);
            }
          }
        `,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
            tag: '@global',
          };
        `,
        "stdio.spec.js": `
          import { test, expect } from '@playwright/test';
          test('no-tags', () => {
          });
          test('foo-tag @inline', { tag: '@foo' }, () => {
          });
          test('foo-bar-tags', { tag: ['@foo', '@bar'] }, () => {
          });
          test('foo-bar-tags with @inline', { tag: ['@foo', '@bar', '@this is a long tag', '@another', '@long one    again'] }, () => {
          });
          test('skip-foo-tag', { tag: '@foo' }, () => {
          });
          test.fixme('fixme-bar-tag', { tag: '@bar' }, () => {
          });
          test.fail('fail-foo-bar-tags', { tag: ['@foo', '@bar'] }, () => {
            expect_eq(1, 2);
          });
          // [cap-test] test.describe('suite @inline', { tag: '@foo' }, () => {
            test('foo-suite', () => {
            });
            // [cap-test] test.describe('inner', { tag: '@bar' }, () => {
              test('foo-bar-suite', () => {
              });
            // [cap-test] });
          // [cap-test] });
          // [cap-test] test.describe.skip('skip-foo-suite', { tag: '@foo' }, () => {
            test('skip-foo-suite', () => {
            });
          // [cap-test] });
          test.describe.fixme('fixme-bar-suite', { tag: '@bar' }, () => {
            test('fixme-bar-suite', () => {
            });
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        `title=no-tags, tags=@global`,
        `title=foo-tag @inline, tags=@global,@inline,@foo`,
        `title=foo-bar-tags, tags=@global,@foo,@bar`,
        `title=foo-bar-tags with @inline, tags=@global,@inline,@foo,@bar,@this is a long tag,@another,@long one    again`,
        `title=skip-foo-tag, tags=@global,@foo`,
        `title=fixme-bar-tag, tags=@global,@bar`,
        `title=fail-foo-bar-tags, tags=@global,@foo,@bar`,
        `title=foo-suite, tags=@global,@inline,@foo`,
        `title=foo-bar-suite, tags=@global,@inline,@foo,@bar`,
        `title=skip-foo-suite, tags=@global,@foo`,
        `title=fixme-bar-suite, tags=@global,@bar`,
      ]);
    });
    test("config.grep should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { grep: /@tag1/ };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', { tag: '@tag1' }, () => { console.log('\\n%% test1'); });
          test('test2', () => { console.log('\\n%% test2'); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.outputLines, ["test1"]);
    });
    test("config.project.grep should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'p1' },
            { name: 'p2', grep: /@tag1/ }
          ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', { tag: '@tag1' }, () => { console.log('\\n%% test1-' + test.info().project.name); });
          test('test2', () => { console.log('\\n%% test2-' + test.info().project.name); });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.outputLines, ["test1-p1", "test2-p1", "test1-p2"]);
    });
    test("--grep should work", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', { tag: '@tag1' }, () => { console.log('\\n%% test1'); });
          test('test2', () => { console.log('\\n%% test2'); });
        `,
      }, { grep: "@tag1" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.outputLines, ["test1"]);
    });
    test("should enforce @ symbol", () => {
      const result = runInlineTest({
        "stdio.spec.js": `
          import { test, expect } from '@playwright/test';
          test('test1', { tag: 'foo' }, () => {
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `Error: Tag must start with '@'`) != null, true);
    });
    test("should be included in testInfo", () => {
      const result = runInlineTest({
        "a.test.ts": `
        import { test, expect } from '@playwright/test';
        test('test without tag', () => {
          expect_eq(testInfo.tags, []);
        });
        test('test with tag', { tag: '@tag1' }, ({}, testInfo) => {
          expect_eq(testInfo.tags, ["@tag1"]);
        });
        `,
      });
      expect_eq(result.exitCode, 0);
    });
    test("should be included in testInfo if coming from describe or global tag", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { tag: ['@global1', '@global2'] };
        `,
        "a.test.ts": `
        import { test, expect } from '@playwright/test';
        // [cap-test] test.describe('describe with tag', { tag: '@tag2' }, async ()=>{
          test('test with tag', () => {
            expect_eq(testInfo.tags, ["@global1", "@global2", "@tag2"]);
          });
        // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
    });
    test("should not parse file names as tags", () => {
      const result = runInlineTest({
        "reporter.ts": `
          export default class Reporter {
            onBegin(config, suite) {
              const visit = suite => {
                for (const test of suite.tests || [])
                  console.log('\\n%%title=' + test.title + ', tags=' + test.tags.join(','));
                for (const child of suite.suites || [])
                  visit(child);
              };
              visit(suite);
            }
            onError(error) {
              console.log(error);
            }
          }
        `,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "@sample.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test in file', () => {
          });
          test('test with tag @inline', { tag: '@foo' }, () => {
          });
        `,
        "dir/@nested/regular.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test in nested dir', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      // File names should NOT be parsed as tags, only inline tags in describe/test titles
      expect(result.outputLines).toEqual([
        `title=test in file, tags=`,
        `title=test with tag @inline, tags=@inline,@foo`,
        `title=test in nested dir, tags=`,
      ]);
    });
}
