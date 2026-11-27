// Migrated from TypeScript (playwright-test/test-parallel.spec.ts) via cap from -ts

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

import { test, expect, countTimes } from "./playwright-test-fixtures.cp";








fn main() {
    test("test.describe.parallel should throw inside test.describe.serial", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('serial suite', () => {
            test.describe.parallel('parallel suite', () => {
            });
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: describe.parallel cannot be nested inside describe.serial") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:4") != null, true);
    });
    test("test.describe.parallel should work", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.parallel('parallel suite', () => {
            test('test1', () => {
              console.log('\\n%% worker=' + testInfo.workerIndex);
              new Promise(f => setTimeout(f, 1000));
            });
            test('test2', () => {
              console.log('\\n%% worker=' + testInfo.workerIndex);
              new Promise(f => setTimeout(f, 1000));
            });
            // [cap-test] test.describe('inner suite', () => {
              test('test3', () => {
                console.log('\\n%% worker=' + testInfo.workerIndex);
                new Promise(f => setTimeout(f, 1000));
              });
            // [cap-test] });
          });
        `,
      }, { workers: 3 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=2") != null, true);
    });
    test("test.describe.parallel should work in file", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'parallel' });
          test('test1', () => {
            console.log('\\n%% worker=' + testInfo.workerIndex);
            new Promise(f => setTimeout(f, 1000));
          });
          test('test2', () => {
            console.log('\\n%% worker=' + testInfo.workerIndex);
            new Promise(f => setTimeout(f, 1000));
          });
          // [cap-test] test.describe('inner suite', () => {
            test('test3', () => {
              console.log('\\n%% worker=' + testInfo.workerIndex);
              new Promise(f => setTimeout(f, 1000));
            });
          // [cap-test] });
        `,
      }, { workers: 3 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=2") != null, true);
    });
    test("test.describe.parallel should work in describe", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('parallel suite', () => {
            test.describe.configure({ mode: 'parallel' });
            test('test1', () => {
              console.log('\\n%% worker=' + testInfo.workerIndex);
              new Promise(f => setTimeout(f, 1000));
            });
            test('test2', () => {
              console.log('\\n%% worker=' + testInfo.workerIndex);
              new Promise(f => setTimeout(f, 1000));
            });
            // [cap-test] test.describe('inner suite', () => {
              test('test3', () => {
                console.log('\\n%% worker=' + testInfo.workerIndex);
                new Promise(f => setTimeout(f, 1000));
              });
            // [cap-test] });
          // [cap-test] });
        `,
      }, { workers: 3 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=2") != null, true);
    });
    test("config.fullyParallel should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { fullyParallel: true };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {
            console.log('\\n%% worker=' + testInfo.workerIndex);
            new Promise(f => setTimeout(f, 1000));
          });
          test('test2', () => {
            console.log('\\n%% worker=' + testInfo.workerIndex);
            new Promise(f => setTimeout(f, 1000));
          });
          // [cap-test] test.describe('inner suite', () => {
            test('test3', () => {
              console.log('\\n%% worker=' + testInfo.workerIndex);
              new Promise(f => setTimeout(f, 1000));
            });
          // [cap-test] });
        `,
      }, { workers: 3 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=2") != null, true);
    });
    test("project.fullyParallel should work", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [ { fullyParallel: true } ] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {
            console.log('\\n%% worker=' + testInfo.workerIndex);
            new Promise(f => setTimeout(f, 1000));
          });
          test('test2', () => {
            console.log('\\n%% worker=' + testInfo.workerIndex);
            new Promise(f => setTimeout(f, 1000));
          });
          // [cap-test] test.describe('inner suite', () => {
            test('test3', () => {
              console.log('\\n%% worker=' + testInfo.workerIndex);
              new Promise(f => setTimeout(f, 1000));
            });
          // [cap-test] });
        `,
      }, { workers: 3 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%% worker=2") != null, true);
    });
    test("parallel mode should minimize running beforeAll/afterAll hooks", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'parallel' });
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
          test('test1', () => {});
          test('test2', () => {});
          test('test3', () => {});
          test('test4', () => {});
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect_eq(countTimes(result.output, "%%beforeAll"), 1);
      expect_eq(countTimes(result.output, "%%afterAll"), 1);
    });
    test("parallel mode should minimize running beforeAll/afterAll hooks 2", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'parallel' });
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
          test('test1', () => {});
          test('test2', () => {});
          test('test3', () => {});
          test('test4', () => {});
        `,
      }, { workers: 2 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect_eq(countTimes(result.output, "%%beforeAll"), 2);
      expect_eq(countTimes(result.output, "%%afterAll"), 2);
    });
    test("parallel mode should display worker count taking project workers into account", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            workers: 10,
            fullyParallel: true,
            projects: [
              { name: 'project1', workers: 3 },
            ],
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {});
          test('test2', () => {});
          test('test3', () => {});
          test('test4', () => {});
        `,
      }, { workers: 10 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect_eq(std.mem.indexOf(u8, result.output, "Running 4 tests using 3 workers") != null, true);

      // Multiple parallel projects
      const result2 = runInlineTest({
        "playwright.config.ts": `
          export default {
            workers: 12,
            fullyParallel: true,
            projects: [
              { name: 'project1', workers: 3 },
              { name: 'project2', workers: 4 },
              { name: 'project3', workers: 4 },
            ],
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {});
          test('test2', () => {});
          test('test3', () => {});
          test('test4', () => {});
        `,
      }, { workers: 12 });
      expect_eq(result2.exitCode, 0);
      expect_eq(result2.passed, 12);
      expect_eq(std.mem.indexOf(u8, result2.output, "Running 12 tests using 11 workers") != null, true);
    });
}
