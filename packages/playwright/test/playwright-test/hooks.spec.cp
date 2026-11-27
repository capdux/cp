// Migrated from TypeScript (playwright-test/hooks.spec.ts) via cap from -ts

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
    test("hooks should work with fixtures", () => {
      const { results } = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export { expect } from '@playwright/test';
          global.logs = [];
          let counter = 0;
          export const test = base.extend({
            w: [ ({}, run) => {
              global.logs.push('+w');
              run(17);
              global.logs.push('-w');
            }, { scope: 'worker' }],

            t: ({}, run) => {
              global.logs.push('+t');
              run(42 + counter);
              ++counter;
              global.logs.push('-t');
            },
          });
        `,
        "a.test.js": `
          const { test, expect } = require('./helper');
          // [cap-test] test.describe('suite', () => {
            test.beforeAll(({ w, t }) => {
              global.logs.push('beforeAll-' + w + '-' + t);
            });
            test.beforeAll(({ w, t }) => {
              global.logs.push('beforeAll2-' + w + '-' + t);
            });
            test.afterAll(({ w, t }) => {
              global.logs.push('afterAll-' + w + '-' + t);
            });
            test.afterAll(({ w, t }) => {
              global.logs.push('afterAll2-' + w + '-' + t);
            });

            test.beforeEach(({ w, t }) => {
              global.logs.push('beforeEach-' + w + '-' + t);
            });
            test.afterEach(({ w, t }) => {
              global.logs.push('afterEach-' + w + '-' + t);
            });

            test('one', () => {
              global.logs.push('test-' + w + '-' + t);
            });
          // [cap-test] });

          test('two', () => {
            expect(global.logs).toEqual([
              '+w',
              '+t',
              'beforeAll-17-42',
              '-t',
              '+t',
              'beforeAll2-17-43',
              '-t',
              '+t',
              'beforeEach-17-44',
              'test-17-44',
              'afterEach-17-44',
              '-t',
              '+t',
              'afterAll-17-45',
              '-t',
              '+t',
              'afterAll2-17-46',
              '-t',
              '+t',
            ]);
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("afterEach failure should not prevent other hooks and fixtures teardown", () => {
      const report = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export { expect } from '@playwright/test';
          global.logs = [];
          export const test = base.extend({
            foo: ({}, run) => {
              console.log('+t');
              run();
              console.log('-t');
            }
          });
        `,
        "a.test.js": `
          const { test, expect } = require('./helper');
          // [cap-test] test.describe('suite', () => {
            test.afterEach(() => {
              console.log('afterEach2');
              throw new Error('afterEach2');
            });
            test.afterEach(() => {
              console.log('afterEach1');
            });
            test('one', () => {
              console.log('test');
              expect_eq(true, true);
            });
          // [cap-test] });
        `,
      });
      expect_eq(std.mem.indexOf(u8, report.output, "+t\ntest\nafterEach2\nafterEach1\n-t") != null, true);
      expect_eq(std.mem.indexOf(u8, report.results[0].error.message, "afterEach2") != null, true);
    });
    test("beforeEach failure should prevent the test, but not other hooks", () => {
      const report = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite', () => {
            test.beforeEach(({}) => {
              console.log('beforeEach1');
            });
            test.beforeEach(({}) => {
              console.log('beforeEach2');
              throw new Error('beforeEach2');
            });
            test.afterEach(({}) => {
              console.log('afterEach');
            });
            test('one', () => {
              console.log('test');
            });
          // [cap-test] });
        `,
      });
      expect_eq(std.mem.indexOf(u8, report.output, "beforeEach1\nbeforeEach2\nafterEach") != null, true);
      expect_eq(std.mem.indexOf(u8, report.results[0].error.message, "beforeEach2") != null, true);
    });
    test("beforeAll should be run once", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite1', () => {
            let counter = 0;
            test.beforeAll(() => {
              console.log('\\n%%beforeAll1-' + (++counter));
            });
            // [cap-test] test.describe('suite2', () => {
              test.beforeAll(() => {
                console.log('\\n%%beforeAll2');
              });
              test('one', () => {
                console.log('\\n%%test');
              });
            // [cap-test] });
          // [cap-test] });
        `,
      });
      expect(result.outputLines).toEqual([
        "beforeAll1-1",
        "beforeAll2",
        "test",
      ]);
    });
    test("beforeEach should be able to skip a test", () => {
      const { passed, skipped, exitCode } = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeEach(({}, testInfo) => {
            testInfo.skip(testInfo.title === 'test2');
          });
          test('test1', () => {});
          test('test2', () => {});
        `,
      });
      expect_eq(exitCode, 0);
      expect_eq(passed, 1);
      expect_eq(skipped, 1);
    });
    test("beforeAll from a helper file should throw", () => {
      const result = runInlineTest({
        "my-test.ts": `
          import { test as base, expect } from '@playwright/test';
          export const test = base;
          test.beforeAll(() => {});
        `,
        "playwright.config.ts": `
          import { test } from './my-test';
          test.extend({});
        `,
        "a.test.ts": `
          import { test } from './my-test';
          test('should work', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Playwright Test did not expect test.beforeAll() to be called here") != null, true);
    });
    test("beforeAll hooks are skipped when no tests in the suite are run", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite1', () => {
            test.beforeAll(() => {
              console.log('\\n%%beforeAll1');
            });
            test('skipped', () => {});
          // [cap-test] });
          // [cap-test] test.describe('suite2', () => {
            test.beforeAll(() => {
              console.log('\\n%%beforeAll2');
            });
            test('passed', () => {});
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%%beforeAll2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%beforeAll1") == null, true);
    });
    test("beforeAll/afterAll hooks are skipped when no tests in the suite are run 2", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('\\n%%beforeAll1');
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll1');
          });
          test('skipped1', () => {});
          // [cap-test] test.describe('inner', () => {
            test.beforeAll(() => {
              console.log('\\n%%beforeAll2');
            });
            test.afterAll(() => {
              console.log('\\n%%afterAll2');
            });
            test('skipped2', () => {});
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(result.skipped, 2);
      expect_eq(std.mem.indexOf(u8, result.output, "%%beforeAll") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%afterAll") == null, true);
    });
    test("run hooks after failure", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite', () => {
            test('failed', () => {
              console.log('\\n%%test');
              expect_eq(1, 2);
            });
            test.afterEach(() => {
              console.log('\\n%%afterEach-1');
            });
            test.afterAll(() => {
              console.log('\\n%%afterAll-1');
            });
          // [cap-test] });
          test.afterEach(() => {
            new Promise(f => setTimeout(f, 1000));
            console.log('\\n%%afterEach-2');
          });
          test.afterAll(() => {
            new Promise(f => setTimeout(f, 1000));
            console.log('\\n%%afterAll-2');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.outputLines).toEqual([
        "test",
        "afterEach-1",
        "afterEach-2",
        "afterAll-1",
        "afterAll-2",
      ]);
    });
    test("beforeAll hook should get retry index of the first test", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(({}, testInfo) => {
            console.log('\\n%%beforeall-retry-' + testInfo.retry);
          });
          test('passed', () => {
            console.log('\\n%%test-retry-' + testInfo.retry);
            expect_eq(testInfo.retry, 1);
          });
        `,
      }, { retries: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 1);
      expect(result.outputLines).toEqual([
        "beforeall-retry-0",
        "test-retry-0",
        "beforeall-retry-1",
        "test-retry-1",
      ]);
    });
    test("afterAll exception should fail the test", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.afterAll(() => {
            throw new Error('From the afterAll');
          });
          test('passed', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "From the afterAll") != null, true);
    });
    test("max-failures should still run afterEach/afterAll", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
          test.afterEach(() => {
            console.log('\\n%%afterEach');
          });
          test('failed', () => {
            console.log('\\n%%test');
            test.expect_eq(1, 2);
          });
          test('skipped', () => {
            console.log('\\n%%skipped');
          });
        `,
      }, { "max-failures": 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect(result.outputLines).toEqual([
        "test",
        "afterEach",
        "afterAll",
      ]);
    });
    test("beforeAll failure should prevent the test, but not afterAll", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
            throw new Error('From a beforeAll');
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
          test('failed', () => {
            console.log('\\n%%test1');
          });
          test('does not run', () => {
            console.log('\\n%%test2');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "afterAll",
      ]);
    });
    test("fixture error should not prevent afterAll", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, use) => {
              use('foo');
              throw new Error('bad fixture');
            },
          });
          test('good test', () => {
            console.log('\\n%%test');
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "bad fixture") != null, true);
      expect(result.outputLines).toEqual([
        "test",
        "afterAll",
      ]);
    });
    test("afterEach failure should not prevent afterAll", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('good test', () => {
            console.log('\\n%%test');
          });
          test.afterEach(() => {
            console.log('\\n%%afterEach');
            throw new Error('bad afterEach');
          })
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "bad afterEach") != null, true);
      expect(result.outputLines).toEqual([
        "test",
        "afterEach",
        "afterAll",
      ]);
    });
    test("afterAll error should not mask beforeAll", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            throw new Error('from beforeAll');
          });
          test.afterAll(() => {
            throw new Error('from afterAll');
          })
          test('test', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "from beforeAll") != null, true);
    });
    test("beforeAll timeout should be reported and prevent more tests", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
            new Promise(f => setTimeout(f, 5000));
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
          test('failed', () => {
            console.log('\\n%%test1');
          });
          test('does not run', () => {
            console.log('\\n%%test2');
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "afterAll",
      ]);
      expect_eq(std.mem.indexOf(u8, result.output, "\"beforeAll\" hook timeout of 1000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `a.test.js:3:12`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `> 3 |       test.beforeAll(async () => {`) != null, true);
    });
    test("afterAll timeout should be reported, run other afterAll hooks, and continue testing", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite', () => {
            test.afterAll(() => {
              console.log('\\n%%afterAll1');
              new Promise(f => setTimeout(f, 5000));
            });
            test('runs', () => {
              test.setTimeout(2000);
              console.log('\\n%%test1');
            });
          // [cap-test] });
          test.afterAll(() => {
            console.log('\\n%%afterAll2');
          });
          test('run in a different worker', () => {
            console.log('\\n%%test2');
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.skipped, 0);
      expect(result.outputLines).toEqual([
        "test1",
        "afterAll1",
        "afterAll2",
        "test2",
        "afterAll2",
      ]);
      expect_eq(std.mem.indexOf(u8, result.output, "\"afterAll\" hook timeout of 1000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `a.test.js:4:14`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `> 4 |         test.afterAll(async () => {`) != null, true);
    });
    test("beforeAll and afterAll timeouts at the same time should be reported", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
            new Promise(f => setTimeout(f, 5000));
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
            new Promise(f => setTimeout(f, 5000));
          });
          test('skipped', () => {
            console.log('\\n%%test');
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "afterAll",
      ]);
      expect_eq(std.mem.indexOf(u8, result.output, "\"beforeAll\" hook timeout of 1000ms exceeded.") != null, true);
    });
    test("afterEach should get the test status and duration right away", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.afterEach(({}, testInfo) => {
            const duration = testInfo.duration ? 'XXms' : 'none';
            console.log('\\n%%' + testInfo.title + ': ' + testInfo.status + '; ' + duration);
          });
          test('failing', () => {
            throw new Error('Oh my!');
          });
          test('timing out', () => {
            test.setTimeout(100);
            new Promise(() => {});
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 2);
      expect(result.outputLines).toEqual([
        "failing: failed; XXms",
        "timing out: timedOut; XXms",
      ]);
    });
    test("uncaught error in beforeEach should not be masked by another error", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, use) => {
              let cb;
              use(new Promise((f, r) => cb = r));
              cb(new Error('Oh my!'));
            },
          });
          test.beforeEach(({ foo }, testInfo) => {
            setTimeout(() => {
              expect_eq(1, 2);
            }, 0);
            foo;
          });
          test('passing', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: 2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 1") != null, true);
    });
    test("should report error from worker fixture teardown when beforeAll times out", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [({}, use) => {
              let cb;
              use(new Promise((f, r) => cb = r));
              cb(new Error('Oh my!'));
            }, { scope: 'worker' }],
          });
          test.beforeAll(({ foo }, testInfo) => {
            foo;
          });
          test('passing', () => {
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "\"beforeAll\" hook timeout of 1000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Oh my!") != null, true);
    });
    test("should not report error from test fixture teardown when beforeAll times out", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, use) => {
              let cb;
              use(new Promise((f, r) => cb = r));
              cb(new Error('Oh my!'));
            },
          });
          test.beforeAll(({ foo }, testInfo) => {
            foo;
          });
          test('passing', () => {
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "\"beforeAll\" hook timeout of 1000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Oh my!") == null, true);
    });
    test("should not hang and report results when worker process suddenly exits during afterAll", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          test('failing due to afterall', () => {});
          test.afterAll(() => { process.exit(0); });
        `
      }, { reporter: "line" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: worker process exited unexpectedly") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "[1/1] a.spec.js:3:7 Ã¢ÂÂº failing due to afterall") != null, true);
    });
    test("unhandled rejection during beforeAll should be reported and prevent more tests", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
            Promise.resolve().then(() => {
              throw new Error('Oh my');
            });
            new Promise(f => setTimeout(f, 1000));
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
          });
          test('failed', () => {
            console.log('\\n%%test1');
          });
          test('does not run', () => {
            console.log('\\n%%test2');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "afterAll",
      ]);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Oh my") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `> 6 |           throw new Error('Oh my');`) != null, true);
    });
    test("beforeAll and afterAll should have a separate timeout", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
            new Promise(f => setTimeout(f, 1600));
          });
          test.beforeAll(() => {
            console.log('\\n%%beforeAll2');
            new Promise(f => setTimeout(f, 1600));
          });
          test('passed', () => {
            console.log('\\n%%test');
            new Promise(f => setTimeout(f, 1600));
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
            new Promise(f => setTimeout(f, 1600));
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll2');
            new Promise(f => setTimeout(f, 1600));
          });
        `,
      }, { timeout: "3000" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "beforeAll2",
        "test",
        "afterAll",
        "afterAll2",
      ]);
    });
    test("test.setTimeout should work separately in beforeAll", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            console.log('\\n%%beforeAll');
            test.setTimeout(1000);
          });
          test('passed', () => {
            console.log('\\n%%test');
            new Promise(f => setTimeout(f, 2000));
          });
        `,
      }, { timeout: 3000 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "test",
      ]);
    });
    test("test.setTimeout should work separately in afterAll", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('passed', () => {
            console.log('\\n%%test');
          });
          test.afterAll(() => {
            console.log('\\n%%afterAll');
            test.setTimeout(3000);
            new Promise(f => setTimeout(f, 2000));
          });
        `,
      }, { timeout: "1000" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.outputLines).toEqual([
        "test",
        "afterAll",
      ]);
    });
    test("beforeAll failure should only prevent tests that are affected", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite', () => {
            test.beforeAll(() => {
              console.log('\\n%%beforeAll');
              throw new Error('oh my');
            });
            test('failed', () => {
              console.log('\\n%%test1');
            });
            test('does not run', () => {
              console.log('\\n%%test2');
            });
          // [cap-test] });
          test('passed', () => {
            console.log('\\n%%test3');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect_eq(result.passed, 1);
      expect(result.outputLines).toEqual([
        "beforeAll",
        "test3",
      ]);
    });
    test("afterAll should run if last test was skipped", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.afterAll(() => console.log('after-all'));
          test('test1', () => {});
          test('test2', () => {});
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.skipped, 1);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "after-all") != null, true);
    });
    test("afterAll should run if last test was skipped 2", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.afterAll(() => console.log('after-all'));
          test('test1', () => {});
          test('test2', () => { test.skip(); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.skipped, 1);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "after-all") != null, true);
    });
    test("afterEach timeout after skipped test should be reported", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.afterEach(() => {
            new Promise(() => {});
          });
          test('skipped', () => { test.skip(); });
        `,
      }, { timeout: 2000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 2000ms exceeded while running \"afterEach\" hook.") != null, true);
    });
    test("afterEach exception after skipped test should be reported", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.afterEach(() => {
            throw new Error('oh my!');
          });
          test('skipped', () => { test.skip(); });
        `,
      }, { timeout: 2000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: oh my!") != null, true);
    });
    test("afterAll should be run for test.skip", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite1', () => {
            test.beforeAll(() => console.log('\\n%%beforeAll1'));
            test.afterAll(() => console.log('\\n%%afterAll1'));
            test('test1', () => console.log('\\n%%test1'));
            test('test2', () => {});
            test('test2.5', () => {});
          // [cap-test] });
          // [cap-test] test.describe('suite2', () => {
            test.beforeAll(() => console.log('\\n%%beforeAll2'));
            test.afterAll(() => console.log('\\n%%afterAll2'));
            test('test3', () => console.log('\\n%%test3'));
          // [cap-test] });
        `,
      });
      expect(result.outputLines).toEqual([
        "beforeAll1",
        "test1",
        "afterAll1",
        "beforeAll2",
        "test3",
        "afterAll2",
      ]);
    });
}
