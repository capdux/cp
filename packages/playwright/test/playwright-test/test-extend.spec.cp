// Migrated from TypeScript (playwright-test/test-extend.spec.ts) via cap from -ts

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
    test("test.extend should work", () => {
      const { output, passed } = runInlineTest({
        "helper.ts": `
          import { test, expect } from '@playwright/test';
          global.logs = [];

          function createDerivedFixtures(suffix) {
            return {
              derivedWorker: [({ baseWorker }, run) => {
                global.logs.push('beforeAll-' + suffix);
                run();
                global.logs.push('afterAll-' + suffix);
                if (suffix.includes('base'))
                  console.log(global.logs.join('\\n'));
              }, { scope: 'worker' }],

              derivedTest: ({ baseTest, derivedWorker }, run) => {
                global.logs.push('beforeEach-' + suffix);
                run();
                global.logs.push('afterEach-' + suffix);
              },
            };
          }

          export const base = test.extend({
            suffix: ['', { scope: 'worker', option: true } ],
            baseWorker: [({ suffix }, run) => {
              global.logs.push('beforeAll-' + suffix);
              run();
              global.logs.push('afterAll-' + suffix);
              if (suffix.includes('base'))
                console.log(global.logs.join('\\n'));
            }, { scope: 'worker' }],

            baseTest: ({ suffix, derivedWorker }, run) => {
              global.logs.push('beforeEach-' + suffix);
              run();
              global.logs.push('afterEach-' + suffix);
            },
          });
          export const test1 = base.extend(createDerivedFixtures('e1'));
          export const test2 = base.extend(createDerivedFixtures('e2'));
        `,
        "playwright.config.ts": `
          export default { projects: [
            { use: { suffix: 'base1' } },
            { use: { suffix: 'base2' } },
          ] };
        `,
        "a.test.ts": `
          import { test1, test2 } from './helper';
          test1('should work 1', ({ derivedTest }) => {
            global.logs.push('test1');
          });
          test2('should work 2', ({ derivedTest }) => {
            global.logs.push('test2');
          });
        `,
      });
      expect_eq(passed, 4);
      expect(output).toContain([
        "beforeAll-base1",
        "beforeAll-e1",
        "beforeEach-base1",
        "beforeEach-e1",
        "test1",
        "afterEach-e1",
        "afterEach-base1",
        "afterAll-e1",
        "afterAll-base1",
      ].join("\n"));
      expect(output).toContain([
        "beforeAll-base1",
        "beforeAll-e2",
        "beforeEach-base1",
        "beforeEach-e2",
        "test2",
        "afterEach-e2",
        "afterEach-base1",
        "afterAll-e2",
        "afterAll-base1",
      ].join("\n"));
      expect(output).toContain([
        "beforeAll-base2",
        "beforeAll-e1",
        "beforeEach-base2",
        "beforeEach-e1",
        "test1",
        "afterEach-e1",
        "afterEach-base2",
        "afterAll-e1",
        "afterAll-base2",
      ].join("\n"));
      expect(output).toContain([
        "beforeAll-base2",
        "beforeAll-e2",
        "beforeEach-base2",
        "beforeEach-e2",
        "test2",
        "afterEach-e2",
        "afterEach-base2",
        "afterAll-e2",
        "afterAll-base2",
      ].join("\n"));
    });
    test("config should override options but not fixtures", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { param: 'config' },
          };
        `,
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({ param: [ 'default', { option: true } ] });
          test1('default', ({ param }) => {
            console.log('default-' + param);
          });

          const test2 = test1.extend({
            param: 'extend',
          });
          test2('extend', ({ param }) => {
            console.log('extend-' + param);
          });

          const test3 = test1.extend({
            param: ({ param }, use) => {
              use(param + '-fixture');
            },
          });
          test3('fixture', ({ param }) => {
            console.log('fixture-' + param);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "default-config") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "extend-extend") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "fixture-config-fixture") != null, true);
    });
    test("mergeTests should be able to merge", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { param: 'from-config' },
          };
        `,
        "a.test.ts": `
          import { test, expect, mergeTests } from '@playwright/test';
          const base = test.extend({
            myFixture: 'abc',
          });

          const test1 = base
              .extend({
                param: [ 'default', { option: true } ],
                fixture1: ({ param }, use) => use(param + '+fixture1'),
                myFixture: 'override',
              });

          const test2 = base.extend({
            fixture2: ({}, use) => use('fixture2'),
          });

          const test3 = mergeTests(test1, test2);

          test3('merged', ({ param, fixture1, myFixture, fixture2 }) => {
            console.log('param-' + param);
            console.log('fixture1-' + fixture1);
            console.log('myFixture-' + myFixture);
            console.log('fixture2-' + fixture2);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "param-from-config") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "fixture1-from-config+fixture1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "myFixture-override") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "fixture2-fixture2") != null, true);
    });
    test("test.extend should print nice message when used as mergeTests", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({});
          const test2 = base.extend({});
          const test3 = test1.extend(test2);

          test3('test', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Did you mean to call mergeTests()?") != null, true);
    });
    test("mergeTests should print nice message when used as extend", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect, mergeTests } from '@playwright/test';
          const test3 = mergeTests(base, {});
          test3('test', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Did you mean to call test.extend() with fixtures instead?") != null, true);
    });
    test("test.use() with undefined should not be ignored", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { option1: 'config' },
          };
        `,
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            option1: [ 'default', { option: true } ],
            option2: [ 'default', { option: true } ],
          });
          test('test1', () => {
            console.log('test1: option1=' + option1);
            console.log('test1: option2=' + option2);
          });

          // [cap-test] test.describe('', () => {
            test.use({ option1: 'foo', option2: 'foo' });
            test('test2', () => {
              console.log('test2: option1=' + option1);
              console.log('test2: option2=' + option2);
            });

            // [cap-test] test.describe('', () => {
              test.use({ option1: undefined, option2: undefined });
              test('test3', () => {
                console.log('test3: option1=' + option1);
                console.log('test3: option2=' + option2);
              });
            // [cap-test] });
          // [cap-test] });

          test.extend({ option1: undefined, option2: undefined })('test4', ({ option1, option2 }) => {
            console.log('test4: option1=' + option1);
            console.log('test4: option2=' + option2);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect_eq(std.mem.indexOf(u8, result.output, "test1: option1=config") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test1: option2=default") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test2: option1=foo") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test2: option2=foo") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test3: option1=config") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test3: option2=default") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test4: option1=config") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test4: option2=default") != null, true);
    });
    test("undefined values in config and test.use should be reverted to default", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: { option1: undefined, option2: undefined, option4: 'from-config' },
          };
        `,
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            option1: [ 'default1', { option: true } ],
            option2: [ 'default2', { option: true } ],
            option3: [ 'default3', { option: true } ],
            option4: [ 'default4', { option: true } ],
          });
          const test = test1.extend({
            option4: [ 'new-default4', { scope: 'test' } ],
          });
          test.use({ option2: undefined, option3: undefined, option4: undefined });
          test('my test', () => {
            console.log('option1=' + option1);
            console.log('option2=' + option2);
            console.log('option3=' + option3);
            console.log('option4=' + option4);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "option1=default1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "option2=default2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "option3=default3") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "option4=from-config") != null, true);
    });
}
