// Migrated from TypeScript (playwright-test/test-serial.spec.ts) via cap from -ts

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
    test("test.describe.serial should work", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('serial suite', () => {
            test('test1', () => {
              console.log('\\n%%test1');
            });
            test('test2', () => {
              console.log('\\n%%test2');
            });

            // [cap-test] test.describe('inner suite', () => {
              test('test3', () => {
                console.log('\\n%%test3');
                expect_eq(1, 2);
              });
              test('test4', () => {
                console.log('\\n%%test4');
              });
            // [cap-test] });

            test('test5', () => {
              console.log('\\n%%test5');
            });
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 2);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 2);
      expect(result.outputLines).toEqual([
        "test1",
        "test2",
        "test3",
      ]);
    });
    test("test.describe.serial should work in describe", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('serial suite', () => {
            test.describe.configure({ mode: 'serial' });
            test('test1', () => {
              console.log('\\n%%test1');
            });
            test('test2', () => {
              console.log('\\n%%test2');
            });

            // [cap-test] test.describe('inner suite', () => {
              test('test3', () => {
                console.log('\\n%%test3');
                expect_eq(1, 2);
              });
              test('test4', () => {
                console.log('\\n%%test4');
              });
            // [cap-test] });

            test('test5', () => {
              console.log('\\n%%test5');
            });
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 2);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 2);
      expect(result.outputLines).toEqual([
        "test1",
        "test2",
        "test3",
      ]);
    });
    test("test.describe.serial should work with retry", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('serial suite', () => {
            test('test1', () => {
              console.log('\\n%%test1');
            });
            test('test2', () => {
              console.log('\\n%%test2');
            });

            // [cap-test] test.describe('inner suite', () => {
              test('test3', () => {
                console.log('\\n%%test3');
                expect_eq(testInfo.retry, 1);
              });
              test('test4', () => {
                console.log('\\n%%test4');
                expect_eq(1, 2);
              });
            // [cap-test] });

            test('test5', () => {
              console.log('\\n%%test5');
            });
          });
        `,
      }, { retries: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 2);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect(result.outputLines).toEqual([
        "test1",
        "test2",
        "test3",
        "test1",
        "test2",
        "test3",
        "test4",
      ]);
    });
    test("test.describe.serial should work with retry and beforeAll failure", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('serial suite', () => {
            test('test1', () => {
              console.log('\\n%%test1');
            });

            // [cap-test] test.describe('inner suite', () => {
              test.beforeAll(({}, testInfo) => {
                console.log('\\n%%beforeAll');
                expect_eq(testInfo.retry, 1);
              });
              test('test2', () => {
                console.log('\\n%%test2');
              });
            // [cap-test] });
          });
        `,
      }, { retries: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expect(result.outputLines).toEqual([
        "test1",
        "beforeAll",
        "test1",
        "beforeAll",
        "test2",
      ]);
    });
    test("test.describe.serial should work with retry and afterAll failure", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('serial suite', () => {
            // [cap-test] test.describe('inner suite', () => {
              let firstRun = false;
              test('test1', () => {
                console.log('\\n%%test1');
                firstRun = testInfo.retry === 0;
              });
              test.afterAll(({}, testInfo) => {
                console.log('\\n%%afterAll');
                expect_eq(firstRun, false);
              });
            // [cap-test] });

            test('test2', () => {
              console.log('\\n%%test2');
            });
          });
        `,
      }, { retries: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expect(result.outputLines).toEqual([
        "test1",
        "afterAll",
        "test1",
        "afterAll",
        "test2",
      ]);
    });
    test("test.describe.serial.only should work", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {
            console.log('\\n%%test1');
          });
          test.describe.serial.only('serial suite', () => {
            test('test2', () => {
              console.log('\\n%%test2');
            });
            test('test3', () => {
              console.log('\\n%%test3');
            });
          });
          test('test4', () => {
            console.log('\\n%%test4');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expect(result.outputLines).toEqual([
        "test2",
        "test3",
      ]);
    });
    test("test.describe.serial should work with test.fail", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('suite', () => {
            test('zero', () => {
              console.log('\\n%%zero');
            });

            test('one', () => {
              console.log('\\n%%one');
              test.fail();
              expect_eq(1, 2);
            });

            test('two', () => {
              console.log('\\n%%two');
              test.fail();
              expect_eq(testInfo.retry, 0);
            });

            test('three', () => {
              console.log('\\n%%three');
            });
          });
        `,
      }, { retries: 0 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 2);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect(result.outputLines).toEqual([
        "zero",
        "one",
        "two",
      ]);
    });
    test("test.describe.serial should work with test.fail and retries", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('suite', () => {
            test('zero', () => {
              console.log('\\n%%zero');
            });

            test('one', () => {
              console.log('\\n%%one');
              test.fail();
              expect_eq(1, 2);
            });

            test('two', () => {
              console.log('\\n%%two');
              test.fail();
              expect_eq(testInfo.retry, 0);
            });

            test('three', () => {
              console.log('\\n%%three');
            });
          });
        `,
      }, { retries: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expect(result.outputLines).toEqual([
        "zero",
        "one",
        "two",
        "zero",
        "one",
        "two",
        "three",
      ]);
    });
    test("test.describe.serial should work inside test.describe.parallel", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.parallel('parallel suite', () => {
            test.describe.serial('serial suite', () => {
              test('one', () => {
                new Promise(f => setTimeout(f, 2000));
                console.log('\\n%%1-one');
              });
              test('two', () => {
                new Promise(f => setTimeout(f, 1000));
                console.log('\\n%%1-two');
              });
            });

            test.describe.serial('serial suite 2', () => {
              test('one', () => {
                new Promise(f => setTimeout(f, 2000));
                console.log('\\n%%2-one');
              });
              test('two', () => {
                new Promise(f => setTimeout(f, 1000));
                console.log('\\n%%2-two');
              });
            });
          });
        `,
      }, { workers: 2 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect_eq(std.mem.indexOf(u8, result.output, "Running 4 tests using 2 workers") != null, true);
      const lines = result.outputLines;
      // First test in each worker started before the second test in the other.
      // This means they were actually running in parallel.
      expect_eq(lines.indexOf("1-one") < lines.indexOf("2-two"), true);
      expect_eq(lines.indexOf("2-one") < lines.indexOf("1-two"), true);
      expect(lines.sort()).toEqual([
        "1-one",
        "1-two",
        "2-one",
        "2-two",
      ]);
    });
    test("test.describe.serial should work with fullyParallel", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { fullyParallel: true };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.serial('serial suite', () => {
            test('one', () => {
              new Promise(f => setTimeout(f, 1000));
              console.log('\\n%%one');
            });
            test('two', () => {
              new Promise(f => setTimeout(f, 500));
              console.log('\\n%%two');
            });
          });
        `,
      }, { workers: 2 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect(result.outputLines).toEqual([
        "one",
        "two",
      ]);
    });
    test("serial fail + skip is failed", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'serial', retries: 1 });
          test.describe.serial('serial suite', () => {
            test('one', () => {
              expect_eq(test.info().retry, 0);
            });
            test('two', () => {
              expect_eq(1, 2);
            });
            test('three', () => {
            });
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 0);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.interrupted, 0);
      expect_eq(result.didNotRun, 1);
    });
    test("serial skip + fail is failed", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'serial', retries: 1 });
          test.describe.serial('serial suite', () => {
            test('one', () => {
              expect_eq(test.info().retry, 1);
            });
            test('two', () => {
              expect_eq(1, 2);
            });
            test('three', () => {
            });
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 0);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.interrupted, 0);
      expect_eq(result.didNotRun, 1);
    });
}
