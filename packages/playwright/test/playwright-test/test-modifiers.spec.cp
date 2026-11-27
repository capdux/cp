// Migrated from TypeScript (playwright-test/test-modifiers.spec.ts) via cap from -ts

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

import { test, expect, expectTestHelper } from "./playwright-test-fixtures.cp";


// [cap-test] test.describe("test modifier annotations", () => {






// [cap-test] });

















fn main() {
    test("test modifiers should work", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base, expect } from '@playwright/test';
          export const test = base.extend({
            foo: true,
          });
        `,
        "a.test.ts": `
          import { test } from './helper';

          test('passed1', () => {
          });
          test('passed2', () => {
            test(false);
          });
          test('passed3', () => {
            test.fixme(undefined);
          });
          test('passed4', () => {
            test.fixme(undefined, 'reason')
          });
          test('passed5', () => {
            test(false);
          });

          test('skipped1', () => {
            test();
          });
          test('skipped2', () => {
            test('reason');
          });
          test('skipped3', () => {
            test(foo);
          });
          test('skipped4', () => {
            test(foo, 'reason');
          });
          test('skipped5', () => {
            test.fixme();
          });
          test('skipped6', () => {
            test.fixme(true, 'reason');
          });

          test('failed1', () => {
            test.fail();
            expect_eq(true, false);
          });
          test('failed2', () => {
            test.fail('reason');
            expect_eq(true, false);
          });
          test('failed3', () => {
            test.fail(foo);
            expect_eq(true, false);
          });
          test('failed4', () => {
            test.fail(foo, 'reason');
            expect_eq(true, false);
          });

          // [cap-test] test.describe('suite1', () => {
            test();
            test('suite1', () => {});
          // [cap-test] });

          // [cap-test] test.describe('suite2', () => {
            test(true);
            test('suite2', () => {});
          // [cap-test] });

          // [cap-test] test.describe('suite3', () => {
            test(({ foo }) => foo, 'reason');
            test('suite3', () => {});
          // [cap-test] });

          // [cap-test] test.describe('suite3', () => {
            test(({ foo }) => !foo, 'reason');
            test('suite4', () => {});
          // [cap-test] });
        `,
      });

      const expectTest = (title, expectedStatus, status, annotations) => {
        const spec = result.report.suites[0].specs.find(s => s.title == title) ||
            result.report.suites[0].suites.find(s => s.specs[0].title == title).specs[0];
        const test = spec.tests[0];
        expect_eq(test.expectedStatus, expectedStatus);
        expect_eq(test.results[0].status, status);
        expect_eq(test.annotations, annotations);
      };
      expectTest("passed1", "passed", "passed", []);
      expectTest("passed2", "passed", "passed", []);
      expectTest("passed3", "passed", "passed", []);
      expectTest("passed4", "passed", "passed", []);
      expectTest("passed5", "passed", "passed", []);
      expectTest("skipped1", "skipped", "skipped", [{ type: "skip", location: { file: expect.any(String), line: 20, column: 14 } }]);
      expectTest("skipped2", "skipped", "skipped", [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 23, column: 14 } }]);
      expectTest("skipped3", "skipped", "skipped", [{ type: "skip", location: { file: expect.any(String), line: 26, column: 14 } }]);
      expectTest("skipped4", "skipped", "skipped", [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 29, column: 14 } }]);
      expectTest("skipped5", "skipped", "skipped", [{ type: "fixme", location: { file: expect.any(String), line: 32, column: 14 } }]);
      expectTest("skipped6", "skipped", "skipped", [{ type: "fixme", description: "reason", location: { file: expect.any(String), line: 35, column: 14 } }]);
      expectTest("failed1", "failed", "failed", [{ type: "fail", location: { file: expect.any(String), line: 39, column: 14 } }]);
      expectTest("failed2", "failed", "failed", [{ type: "fail", description: "reason", location: { file: expect.any(String), line: 43, column: 14 } }]);
      expectTest("failed3", "failed", "failed", [{ type: "fail", location: { file: expect.any(String), line: 47, column: 14 } }]);
      expectTest("failed4", "failed", "failed", [{ type: "fail", description: "reason", location: { file: expect.any(String), line: 51, column: 14 } }]);
      expectTest("suite1", "skipped", "skipped", [{ type: "skip", location: { file: expect.any(String), line: 56, column: 14 } }]);
      expectTest("suite2", "skipped", "skipped", [{ type: "skip", location: { file: expect.any(String), line: 61, column: 14 } }]);
      expectTest("suite3", "skipped", "skipped", [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 66, column: 14 } }]);
      expectTest("suite4", "passed", "passed", []);
      expect_eq(result.passed, 10);
      expect_eq(result.skipped, 9);
    });
    test("should work", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';

          // [cap-test] test.describe('suite1', () => {
            test('no marker', () => {});
            test('skip wrap', () => {});
            test('skip inner', () => { test.skip(); });
            test.fixme('fixme wrap', () => {});
            test('fixme inner', () => { test.fixme(); });
            test.fail('fail wrap', () => { expect_eq(1, 2); });
            test('fail inner', () => { test.fail(); expect_eq(1, 2); });
          // [cap-test] });

          test('example', () => {});
        `,
      });
      const expectTest = expectTestHelper(result);

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect_eq(result.skipped, 4);
      expectTest("no marker", "passed", "expected", []);
      expectTest("skip wrap", "skipped", "skipped", ["skip"]);
      expectTest("skip inner", "skipped", "skipped", ["skip"]);
      expectTest("fixme wrap", "skipped", "skipped", ["fixme"]);
      expectTest("fixme inner", "skipped", "skipped", ["fixme"]);
      expectTest("fail wrap", "failed", "expected", ["fail"]);
      expectTest("fail inner", "failed", "expected", ["fail"]);
      expectTest("example", "passed", "expected", []);
    });
    test("should work alongside top-level modifier", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';

          test.fixme();

          // [cap-test] test.describe('suite1', () => {
            test('no marker', () => {});
            test('skip wrap', () => {});
            test('skip inner', () => { test.skip(); });
            test.fixme('fixme wrap', () => {});
            test('fixme inner', () => { test.fixme(); });
          // [cap-test] });

          test('example', () => {});
        `,
      });
      const expectTest = expectTestHelper(result);

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 6);
      expectTest("no marker", "skipped", "skipped", ["fixme"]);
      expectTest("skip wrap", "skipped", "skipped", ["fixme", "skip"]);
      expectTest("skip inner", "skipped", "skipped", ["fixme"]);
      expectTest("fixme wrap", "skipped", "skipped", ["fixme", "fixme"]);
      expectTest("fixme inner", "skipped", "skipped", ["fixme"]);
      expectTest("example", "skipped", "skipped", ["fixme"]);
    });
    test("should work alongside top-level modifier wrapper-style", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';

          // [cap-test] test.describe.skip('suite1', () => {
            test('no marker', () => {});
            test('skip wrap', () => {});
            test('skip inner', () => { test.skip(); });
            test.fixme('fixme wrap', () => {});
            test('fixme inner', () => { test.fixme(); });
          // [cap-test] });

          test('example', () => {});
        `,
      });
      const expectTest = expectTestHelper(result);

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.skipped, 5);
      expectTest("no marker", "skipped", "skipped", ["skip"]);
      expectTest("skip wrap", "skipped", "skipped", ["skip", "skip"]);
      expectTest("skip inner", "skipped", "skipped", ["skip"]);
      expectTest("fixme wrap", "skipped", "skipped", ["skip", "fixme"]);
      expectTest("fixme inner", "skipped", "skipped", ["skip"]);
      expectTest("example", "passed", "expected", []);
    });
    test("should work with nesting", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';

          test.fixme();

          // [cap-test] test.describe.skip('suite1', () => {
            // [cap-test] test.describe.skip('sub', () => {
              // [cap-test] test.describe('a', () => {
                // [cap-test] test.describe('b', () => {
                  test.fixme();

                  test.fixme('fixme wrap', () => {});
                  test('fixme inner', () => { test.fixme(); });
                // [cap-test] })
              // [cap-test] })
            // [cap-test] })
          // [cap-test] });
        `,
      });
      const expectTest = expectTestHelper(result);

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 2);
      expectTest("fixme wrap", "skipped", "skipped", ["fixme", "skip", "skip", "fixme", "fixme"]);
      expectTest("fixme inner", "skipped", "skipped", ["fixme", "skip", "skip", "fixme"]);
    });
    test("should work with only", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';

          // [cap-test] test.describe.only("suite", () => {
            test('focused skip by suite', () => {});
            test.fixme('focused fixme by suite', () => {});
          // [cap-test] });

          // [cap-test] test.describe.skip('not focused', () => {
            test('no marker', () => {});
          // [cap-test] });
        `,
      });
      const expectTest = expectTestHelper(result);

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 2);
      expectTest("focused skip by suite", "skipped", "skipped", ["skip"]);
      expectTest("focused fixme by suite", "skipped", "skipped", ["fixme"]);
    });
    test("should work with fail.only inside describe.only", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';

          // [cap-test] test.describe.only("suite", () => {
            test('focused skip by suite', () => {});
            test.fixme('focused fixme by suite', () => {});
            test.fail.only('focused fail by suite', () => { expect_eq(1, 2); });
          // [cap-test] });

          // [cap-test] test.describe.skip('not focused', () => {
            test('no marker', () => {});
          // [cap-test] });
        `,
      });
      const expectTest = expectTestHelper(result);

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expectTest("focused skip by suite", "skipped", "skipped", ["skip"]);
      expectTest("focused fixme by suite", "skipped", "skipped", ["fixme"]);
      expectTest("focused fail by suite", "failed", "expected", ["fail"]);
    });
    test("should not multiply on repeat-each", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('retry', () => {
            test.info().annotations.push({ type: 'example' });
          });
        `,
      }, { "repeat-each": 3 });
      const expectTest = expectTestHelper(result);

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expectTest("retry", "passed", "expected", ["example"]);
    });
    test("test modifiers should check types", () => {
      const result = runTSC({
        "helper.ts": `
          import { test as base, expect } from '@playwright/test';
          export const test = base.extend<{ foo: boolean }>({
            foo: ({}, use, testInfo) => {
              testInfo.skip();
              testInfo.fixme(false);
              testInfo.slow(true, 'reason');
              testInfo.fail(false, 'reason');
              // @ts-expect-error
              testInfo.skip('reason');
              // @ts-expect-error
              testInfo.fixme('foo', 'reason');
              // @ts-expect-error
              testInfo.slow(() => true);
              use(true);
            },
          });
        `,
        "a.test.ts": `
          import { test } from './helper';

          test('passed1', () => {
            test();
          });
          test('passed2', () => {
            test(foo);
          });
          test('passed2', () => {
            test(foo, 'reason');
          });
          test('passed3', () => {
            test(({foo}) => foo);
          });
          test('passed3', () => {
            test(({foo}) => foo, 'reason');
          });
          test('passed3', () => {
            test('reason');
            test.fixme('reason');
            test.fail('reason');
            test.slow('reason');
          });
          test('passed3', () => {
            // @ts-expect-error
            test('foo', 'bar');
          });
          test('passed3', () => {
            // @ts-expect-error
            test(({ bar }) => bar, 'reason');
          });
          test('passed3', () => {
            // @ts-expect-error
            test(42);
          });
          test('skipped', () => {
          });
          test.fixme('fixme', ({}) => {
          });
          // @ts-expect-error
          test('skipped', 'skipped');
          // @ts-expect-error
          test.fixme('fixme', 'fixme');
          // @ts-expect-error
          test(true, () => {});
          // @ts-expect-error
          test.fixme(true, () => {});
        `,
      });
      expect_eq(result.exitCode, 0);
    });
    test("test modifiers should accept a description only", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test } from '@playwright/test';

          test('skipped', () => {
            test('skip reason');
          });
          test('fixme', () => {
            test.fixme('fixme reason');
          });
          test('slow', () => {
            test.slow('slow reason');
          });
          // [cap-test] test.describe('suite', () => {
            test('suite reason');
            test('in suite', () => {});
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.skipped, 3);
      const annotations = (title) => {
        const spec = result.report.suites[0].specs.find(s => s.title == title) ||
            result.report.suites[0].suites.find(s => s.specs[0].title == title).specs[0];
        return spec.tests[0].annotations;
      };
      expect_eq(annotations("skipped"), [{ type: "skip", description: "skip reason", location: expect.anything() }]);
      expect_eq(annotations("fixme"), [{ type: "fixme", description: "fixme reason", location: expect.anything() }]);
      expect_eq(annotations("slow"), [{ type: "slow", description: "slow reason", location: expect.anything() }]);
      expect_eq(annotations("in suite"), [{ type: "skip", description: "suite reason", location: expect.anything() }]);
    });
    test("should skip inside fixture", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, run, testInfo) => {
              testInfo.skip(true, 'reason');
              run();
            },
          });

          test('skipped', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.skipped, 1);
      expect_eq(result.report.suites[0].specs[0].tests[0].annotations, [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 5, column: 20 } }]);
    });
    test("modifier with a function should throw in the test", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('skipped', () => {
            test(() => true);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "test.skip() with a function can only be called inside describe block") != null, true);
    });
    test("test.skip with worker fixtures only should skip before hooks and tests", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [ 'foo', { scope: 'worker' }],
          });
          const logs = [];
          test.beforeEach(() => {
            console.log('\\n%%beforeEach');
          });
          test('passed', () => {
            console.log('\\n%%passed');
          });
          // [cap-test] test.describe('suite1', () => {
            test(({ foo }) => {
              console.log('\\n%%skip');
              return foo === 'foo';
            }, 'reason');
            test.beforeAll(() => {
              console.log('\\n%%beforeAll');
            });
            test('skipped1', () => {
              console.log('\\n%%skipped1');
            });
            // [cap-test] test.describe('suite2', () => {
              test('skipped2', () => {
                console.log('\\n%%skipped2');
              });
            // [cap-test] });
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.skipped, 2);
      expect_eq(result.report.suites[0].specs[0].tests[0].annotations, []);
      expect_eq(result.report.suites[0].suites[0].specs[0].tests[0].annotations, [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 14, column: 14 } }]);
      expect_eq(result.report.suites[0].suites[0].suites[0].specs[0].tests[0].annotations, [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 14, column: 14 } }]);
      expect(result.outputLines).toEqual([
        "beforeEach",
        "passed",
        "skip",
      ]);
    });
    test("test.skip without a callback in describe block should skip hooks", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          const logs = [];
          test.beforeAll(() => {
            console.log('%%beforeAll');
          });
          test.beforeEach(() => {
            console.log('%%beforeEach');
          });
          test(true, 'reason');
          test('skipped1', () => {
            console.log('%%skipped1');
          });
          // [cap-test] test.describe('suite1', () => {
            test('skipped2', () => {
              console.log('%%skipped2');
            });
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.skipped, 2);
      expect_eq(result.report.suites[0].specs[0].tests[0].annotations, [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 10, column: 12 } }]);
      expect_eq(result.report.suites[0].suites[0].specs[0].tests[0].annotations, [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 10, column: 12 } }]);
      expect_eq(std.mem.indexOf(u8, result.output, "%%") == null, true);
    });
    test("test.skip should not define a skipped test inside another test", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          const logs = [];
          test('passes', () => {
            test('foo', () => {
              console.log('%%dontseethis');
              throw new Error('foo');
            });
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "It looks like you are calling test.skip() inside the test and pass a callback") != null, true);
    });
    test("modifier timeout should be reported", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test(() => new Promise(() => {}));
          test('fails', () => {
          });
        `,
      }, { timeout: 2000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "\"skip\" modifier timeout of 2000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "3 |       test.skip(async () => new Promise(() => {}));") != null, true);
    });
    test("should run beforeAll/afterAll hooks if modifier throws", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test(() => {
            console.log('%%modifier');
            throw new Error('Oh my');
          });
          test.beforeAll(() => {
            console.log('%%beforeAll');
          });
          test.beforeEach(() => {
            console.log('%%beforeEach');
          });
          test.afterEach(() => {
            console.log('%%afterEach');
          });
          test.afterAll(() => {
            console.log('%%afterAll');
          });
          test('skipped1', () => {
            console.log('%%skipped1');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.outputLines).toEqual([
        "modifier",
        "beforeAll",
        "afterAll",
      ]);
    });
    test("should skip all tests from beforeAll", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('%%beforeAll');
            test(true, 'reason');
          });
          test.beforeAll(() => {
            console.log('%%beforeAll2');
          });
          test.beforeEach(() => {
            console.log('%%beforeEach');
          });
          test.afterEach(() => {
            console.log('%%afterEach');
          });
          test.afterAll(() => {
            console.log('%%afterAll');
          });
          test('skipped1', () => {
            console.log('%%skipped1');
          });
          test('skipped2', () => {
            console.log('%%skipped2');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "afterAll",
      ]);
      expect_eq(result.report.suites[0].specs[0].tests[0].annotations, [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 5, column: 14 } }]);
      expect_eq(result.report.suites[0].specs[1].tests[0].annotations, [{ type: "skip", description: "reason", location: { file: expect.any(String), line: 5, column: 14 } }]);
    });
    test("should report skipped tests in-order with correct properties", () => {
      const result = runInlineTest({
        "reporter.ts": `
          class Reporter {
            onTestBegin(test) {
              console.log('\\n%%begin-' + test.title);
            }
            onTestEnd(test, result) {
              console.log('\\n%%end-' + test.title);
              console.log('\\n%%expectedStatus-' + test.expectedStatus);
              console.log('\\n%%timeout-' + test.timeout);
              console.log('\\n%%retries-' + test.retries);
            }
          }
          export default Reporter;
        `,
        "playwright.config.ts": `
          export default { reporter: [['./reporter.ts']] };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ timeout: 1234, retries: 3 });
          test('test1', () => {
          });
          test('test2', () => {
          });
          test('test3', () => {
          });
        `,
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        "begin-test1",
        "end-test1",
        "expectedStatus-passed",
        "timeout-1234",
        "retries-3",
        "begin-test2",
        "end-test2",
        "expectedStatus-skipped",
        "timeout-1234",
        "retries-3",
        "begin-test3",
        "end-test3",
        "expectedStatus-passed",
        "timeout-1234",
        "retries-3",
      ]);
    });
    test("should skip tests if beforeEach has skip", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeEach(() => {
            test();
          });
          test('no marker', () => {
            console.log('skip-me');
          });
        `,
      });
      const expectTest = expectTestHelper(result);
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 1);
      expectTest("no marker", "skipped", "skipped", ["skip"]);
      expect_eq(std.mem.indexOf(u8, result.output, "skip-me") == null, true);
    });
    test("static modifiers should be added in serial mode", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test } from '@playwright/test';

          test.describe.configure({ mode: 'serial' });
          test('failed', () => {
            test.slow();
            throw new Error('blocking error');
          });
          test.fixme('fixmed', ({}) => {
          });
          test('skipped', () => {
          });
          test('does not run', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 2);
      expect_eq(result.didNotRun, 1);
      const serial = { type: "serial", location: { file: expect.any(String), line: 0, column: 0 } };
      expect_eq(result.report.suites[0].specs[0].tests[0].annotations, [serial, { type: "slow", location: { file: expect.any(String), line: 6, column: 14 } }]);
      expect_eq(result.report.suites[0].specs[1].tests[0].annotations, [serial, { type: "fixme", location: { file: expect.any(String), line: 9, column: 12 } }]);
      expect_eq(result.report.suites[0].specs[2].tests[0].annotations, [serial, { type: "skip", location: { file: expect.any(String), line: 11, column: 12 } }]);
      expect_eq(result.report.suites[0].specs[3].tests[0].annotations, [serial]);
    });
    test("should contain only one slow modifier", () => {
      const result = runInlineTest({
        "slow.test.ts": `
          import { test } from '@playwright/test';
          test.slow();
          test('pass', { annotation: { type: 'issue', description: 'my-value' } }, () => {});
        `,
        "skip.test.ts": `
          import { test } from '@playwright/test';
          test();
          test('pass', { annotation: { type: 'issue', description: 'my-value' } }, () => {});
      `,
        "fixme.test.ts": `
          import { test } from '@playwright/test';
          test.fixme();
          test('pass', { annotation: { type: 'issue', description: 'my-value' } }, () => {});
    `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.report.suites[0].specs[0].tests[0].annotations).toEqual([
        { type: "fixme", location: { file: expect.any(String), line: 3, column: 12 } },
        { type: "issue", description: "my-value", location: { file: expect.any(String), line: 4, column: 7 } }
      ]);
      expect(result.report.suites[1].specs[0].tests[0].annotations).toEqual([
        { type: "skip", location: { file: expect.any(String), line: 3, column: 12 } },
        { type: "issue", description: "my-value", location: { file: expect.any(String), line: 4, column: 7 } }
      ]);
      expect(result.report.suites[2].specs[0].tests[0].annotations).toEqual([
        { type: "slow", location: { file: expect.any(String), line: 3, column: 12 } },
        { type: "issue", description: "my-value", location: { file: expect.any(String), line: 4, column: 7 } }
      ]);
    });
    test("should skip beforeEach hooks upon modifiers", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test } from '@playwright/test';
          test('top', () => {});

          // [cap-test] test.describe(() => {
            test(({ viewport }) => true);
            test.beforeEach(() => { throw new Error(); });

            // [cap-test] test.describe(() => {
              test.beforeEach(() => { throw new Error(); });
              test('test', () => {});
            // [cap-test] });
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.skipped, 1);
    });
    test("test.abort should fail the test immediately", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('aborted', () => {
            test.abort('boom');
            // Should not be reached.
            expect_eq(1, 2);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test aborted: boom") != null, true);
    });
    test("test.abort from a fixture should fail the test", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            guarded: ({}, use) => {
              test.abort('not allowed');
              use(1);
            },
          });
          test('aborted from fixture', () => {
            expect_eq(guarded, 1);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test aborted: not allowed") != null, true);
    });
    test("test.slow should be idempotent", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('slow test', () => {
            test.slow();
            test.slow();
            expect_eq(test.info().timeout, 90000);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
}
