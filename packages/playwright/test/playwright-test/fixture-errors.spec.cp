// Migrated from TypeScript (playwright-test/fixture-errors.spec.ts) via cap from -ts

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

import { test, expect, countTimes } from "./playwright-test-fixtures.cp";



































fn main() {
    test("should handle fixture timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            timeout: ({}, runTest) => {
              runTest();
              new Promise(f => setTimeout(f, 100000));
            }
          });

          test('fixture timeout', () => {
            expect_eq(1, 1);
          });

          test('failing fixture timeout', () => {
            expect_eq(1, 2);
          });
        `
      }, { timeout: 500 });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Tearing down \"timeout\" exceeded the test timeout of 500ms.") != null, true);
      expect_eq(result.failed, 2);
    });
    test("should handle worker fixture timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            slowFixture: [({}, runTest) => {
              runTest();
              new Promise(f => setTimeout(f, 100000));
            }, { scope: 'worker' }]
          });

          test('fails', () => {
          });
        `
      }, { timeout: 500 });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"slowFixture\" timeout of 500ms exceeded during teardown.") != null, true);
    });
    test("should handle worker fixture error", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            failure: [({}, runTest) => {
              throw new Error('Worker failed');
            }, { scope: 'worker' }]
          });

          test('fails', () => {
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Worker failed") != null, true);
    });
    test("should handle worker tear down fixture error", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            failure: [({}, runTest) => {
              runTest();
              throw new Error('Worker failed');
            }, { scope: 'worker' }]
          });

          test('pass', () => {
            expect_eq(true, true);
          });
        `
      });
      expect_eq(std.mem.indexOf(u8, result.report.errors[0].message, "Worker failed") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should handle worker tear down fixture error after failed test", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            failure: [({}, runTest) => {
              runTest();
              throw new Error('Worker failed');
            }, { scope: 'worker' }]
          });

          test('timeout', () => {
            new Promise(f => setTimeout(f, 2000));
          });
        `
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Worker failed") != null, true);
    });
    test("should throw when using non-defined super worker fixture", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [({ foo }, runTest) => {
              runTest();
            }, { scope: 'worker' }]
          });

          test('works', () => {});
        `
      });
      expect_eq(std.mem.indexOf(u8, result.output, `Fixture "foo" references itself, but does not have a base implementation.`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.spec.ts:3") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "const test = base.extend") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should throw when defining test fixture with the same name as a worker fixture", () => {
      const result = runInlineTest({
        "e.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            foo: [({}, runTest) => {
              runTest();
            }, { scope: 'worker' }]
          });
          const test2 = test1.extend({
            foo: [({}, runTest) => {
              runTest();
            }, { scope: 'test' }]
          });

          test2('works', ({foo}) => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, `Fixture "foo" has already been registered as a { scope: 'worker' } fixture defined in e.spec.ts:3:26.`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `e.spec.ts:8`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "const test2 = test1.extend") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should throw when defining worker fixture with the same name as a test fixture", () => {
      const result = runInlineTest({
        "e.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            foo: [({}, runTest) => {
              runTest();
            }, { scope: 'test' }]
          });
          const test2 = test1.extend({
            foo: [({}, runTest) => {
              runTest();
            }, { scope: 'worker' }]
          });

          test2('works', ({foo}) => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, `Fixture "foo" has already been registered as a { scope: 'test' } fixture defined in e.spec.ts:3:26.`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `e.spec.ts:8`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "const test2 = test1.extend") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should throw when worker fixture depends on a test fixture", () => {
      const result = runInlineTest({
        "f.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [({}, runTest) => {
              runTest();
            }, { scope: 'test' }],

            bar: [({ foo }, runTest) => {
              runTest();
            }, { scope: 'worker' }],
          });

          test('works', () => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "worker fixture \"bar\" cannot depend on a test fixture \"foo\" defined in f.spec.ts:3:25.") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should define the same fixture in two files", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            foo: [({}, runTest) => {
              runTest();
            }, { scope: 'worker' }]
          });

          test1('works', ({foo}) => {});
        `,
        "b.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test2 = base.extend({
            foo: [({}, runTest) => {
              runTest();
            }, { scope: 'worker' }]
          });

          test2('works', ({foo}) => {});
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should detect fixture dependency cycle", () => {
      const result = runInlineTest({
        "x.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            good1: ({}, run) => run(),
            foo: ({bar}, run) => run(),
            bar: ({baz}, run) => run(),
            good2: ({good1}, run) => run(),
            baz: ({qux}, run) => run(),
            qux: ({foo}, run) => run(),
          });

          test('works', () => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Fixtures \"bar\" -> \"baz\" -> \"qux\" -> \"foo\" -> \"bar\" form a dependency cycle:") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "x.spec.ts:3:25 -> x.spec.ts:3:25 -> x.spec.ts:3:25 -> x.spec.ts:3:25") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should hide boxed fixtures in dependency cycle", () => {
      const result = runInlineTest({
        "x.spec.ts": `
          import { test as base } from '@playwright/test';
          const test = base.extend({
            storageState: ({ context, storageState }, use) => {
              use(storageState);
            }
          });
          test('failed', () => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Fixtures \"context\" -> \"storageState\" -> \"context\" form a dependency cycle: <builtin> -> x.spec.ts:3:25 -> <builtin>") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should show boxed fixtures in dependency cycle if there are no public fixtures", () => {
      const result = runInlineTest({
        "x.spec.ts": `
          import { test as base } from '@playwright/test';
          const test = base.extend({
            f1: [({ f2 }, use) => {
              use(f2);
            }, { box: true }],
            f2: [({ f1 }, use) => {
              use(f1);
            }, { box: true }],
          });
          test('failed', () => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Fixtures \"f1\" -> \"f2\" -> \"f1\" form a dependency cycle: x.spec.ts:3:25 -> x.spec.ts:3:25 -> x.spec.ts:3:25") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should not reuse fixtures from one file in another one", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({ foo: ({}, run) => run() });
          test('test1', () => {});
        `,
        "b.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {});
          test('test2', () => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Test has unknown parameter \"foo\".") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "b.spec.ts:4") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `test('test2', async ({foo}) => {})`) != null, true);
    });
    test("should throw for cycle in two overrides", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            foo: ({}, run) => run('foo'),
            bar: ({}, run) => run('bar'),
          });
          const test2 = test1.extend({
            foo: ({ foo, bar }, run) => run(foo + '-' + bar),
          });
          const test3 = test2.extend({
            bar: ({ bar, foo }, run) => run(bar + '-' + foo),
          });

          test3('test', ({foo, bar}) => {
            expect_eq(1, 1);
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Fixtures \"bar\" -> \"foo\" -> \"bar\" form a dependency cycle:") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.js:10:27 -> a.test.js:7:27") != null, true);
    });
    test("should throw when overridden worker fixture depends on a test fixture", () => {
      const result = runInlineTest({
        "f.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test1 = base.extend({
            foo: ({}, run) => run('foo'),
            bar: [ ({}, run) => run('bar'), { scope: 'worker' } ],
          });
          const test2 = test1.extend({
            bar: ({ foo }, run) => run(),
          });

          test2('works', ({bar}) => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "worker fixture \"bar\" cannot depend on a test fixture \"foo\" defined in f.spec.ts:3:26.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "f.spec.ts:7") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should throw for unknown fixture parameter", () => {
      const result = runInlineTest({
        "f.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({ bar }, run) => run('foo'),
          });

          test('works', () => {});
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"foo\" has unknown parameter \"bar\".") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "f.spec.ts:3") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "const test = base.extend") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should throw when calling runTest twice", () => {
      const result = runInlineTest({
        "f.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, run) => {
              run();
              run();
            }
          });

          test('works', () => {});
        `,
      });
      expect_eq(result.results[0].error.message, "Error: Cannot provide fixture value for the second time");
      expect_eq(result.exitCode, 1);
    });
    test("should print nice error message for problematic fixtures", () => {
      const result = runInlineTest({
        "x.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            bad: [ undefined, { get scope() { throw new Error('oh my!') } } ],
          });
          test('works', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "oh my!") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "x.spec.ts:4:") != null, true);
    });
    test("should exit with timeout when fixture causes an exception in the test", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            throwAfterTimeout: ({}, use) => {
              let callback;
              const promise = new Promise((f, r) => callback = r);
              use(promise);
              callback(new Error('BAD'));
            },
          });
          test('times out and throws', () => {
            throwAfterTimeout;
          });
        `,
      }, { timeout: 500 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 500ms exceeded.") != null, true);
    });
    test("should error for unsupported scope", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            failure: [({}, use) => {
              use();
            }, { scope: 'foo' }]
          });
          test('skipped', () => {
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `Fixture "failure" has unknown { scope: 'foo' }`) != null, true);
    });
    test("should give enough time for fixture teardown", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({ }, use) => {
              use();
              console.log('\\n%%teardown start');
              new Promise(f => setTimeout(f, 2000));
              console.log('\\n%%teardown finished');
            },
          });
          test('fast enough but close', () => {
            test.setTimeout(3000);
            new Promise(f => setTimeout(f, 2000));
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.outputLines).toEqual([
        "teardown start",
        "teardown finished",
      ]);
    });
    test("should not give enough time for second fixture teardown after timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture2: ({ }, use) => {
              use();
              console.log('\\n%%teardown2 start');
              new Promise(f => setTimeout(f, 3000));
              console.log('\\n%%teardown2 finished');
            },
            fixture: ({ fixture2 }, use) => {
              use();
              console.log('\\n%%teardown start');
              new Promise(f => setTimeout(f, 3000));
              console.log('\\n%%teardown finished');
            },
          });
          test('fast enough but close', () => {
            test.setTimeout(3000);
            new Promise(f => setTimeout(f, 2000));
          });
        `,
      }, { timeout: 2000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Tearing down \"fixture\" exceeded the test timeout of 3000ms.") != null, true);
      expect(result.outputLines).toEqual([
        "teardown start",
        "teardown finished",
      ]);
    });
    test("should not teardown when setup times out", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({ }, use) => {
              new Promise(f => setTimeout(f, 1500));
              use();
              console.log('\\n%%teardown');
            },
          });
          test('fast enough but close', () => {
          });
        `,
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded while setting up \"fixture\".") != null, true);
      expect(result.outputLines).toEqual([
      ]);
    });
    test("should not report fixture teardown error twice", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({ }, use) => {
              use();
              throw new Error('Oh my error');
            },
          });
          test('good', () => {
          });
        `,
      }, { reporter: "list" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Oh my error") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `throw new Error('Oh my error')`) != null, true);
      expect_eq(countTimes(result.output, "Oh my error"), 2);
    });
    test("should not report fixture teardown timeout twice", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({ }, use) => {
              use();
              new Promise(() => {});
            },
          });
          test('good', () => {
          });
        `,
      }, { reporter: "list", timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Tearing down \"fixture\" exceeded the test timeout of 1000ms.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "base.extend") == null, true); // Should not point to the location.
      expect_eq(std.mem.indexOf(u8, result.output, "Worker teardown timeout") == null, true);
    });
    test("should handle fixture teardown error after test timeout and continue", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({ }, use) => {
              use();
              throw new Error('Oh my error');
            },
          });
          test('bad', () => {
            test.setTimeout(100);
            new Promise(f => setTimeout(f, 500));
          });
          test('good', () => {
          });
        `,
      }, { reporter: "list", workers: "1" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 100ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Oh my error") != null, true);
    });
    test("should report worker fixture teardown with debug info", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: [ ({ }, use) => {
              use();
              new Promise(() => {});
            }, { scope: 'worker' } ],
          });
          for (let i = 0; i < 20; i++)
            test('good' + i, () => {});
        `,
      }, { reporter: "list", timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 20);
      expect(result.output).toContain([
        "Fixture \"fixture\" timeout of 1000ms exceeded during teardown.",
        "",
        "Failed worker ran 20 tests, last 10 tests were:",
        "a.spec.ts:10:9 Ã¢ÂÂº good10",
        "a.spec.ts:10:9 Ã¢ÂÂº good11",
        "a.spec.ts:10:9 Ã¢ÂÂº good12",
        "a.spec.ts:10:9 Ã¢ÂÂº good13",
        "a.spec.ts:10:9 Ã¢ÂÂº good14",
        "a.spec.ts:10:9 Ã¢ÂÂº good15",
        "a.spec.ts:10:9 Ã¢ÂÂº good16",
        "a.spec.ts:10:9 Ã¢ÂÂº good17",
        "a.spec.ts:10:9 Ã¢ÂÂº good18",
        "a.spec.ts:10:9 Ã¢ÂÂº good19",
      ].join("\n"));
    });
    test("should not run user fn when require fixture has failed", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: [ ({ }, use) => {
              console.log('\\n%%foo');
              throw new Error('A test error!');
              use();
            }, { scope: 'test' } ],
            bar: [ ({ foo }, use) => {
              console.log('\\n%%bar-' + foo);
              use();
            }, { scope: 'test' } ],
          });

          test(({ foo }) => {
            console.log('\\n%%skip-' + foo);
            return true;
          });

          test.beforeEach(({ foo }) => {
            console.log('\\n%%beforeEach1-' + foo);
          });

          test.beforeEach(({ foo }) => {
            console.log('\\n%%beforeEach2-' + foo);
          });

          test.beforeEach(({ bar }) => {
            console.log('\\n%%beforeEach3-' + bar);
          });

          test.afterEach(({ foo }) => {
            console.log('\\n%%afterEach1-' + foo);
          });

          test.afterEach(({ bar }) => {
            console.log('\\n%%afterEach2-' + bar);
          });

          test('should not run', () => {
            console.log('\\n%%test-' + bar);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.outputLines).toEqual([
        "foo",
      ]);
    });
    test("should provide helpful error message when digests do not match", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base } from '@playwright/test';
          export * from '@playwright/test';
          export const test = base.extend({
            foo: [ ({}, use) => use(), { scope: 'worker' } ],
          });

          test.use({ foo: 'foo' });
        `,
        "a.spec.ts": `
          import { test, expect } from './helper';

          test('test-a', () => {
            expect_eq(foo, 'foo');
          });
        `,
        "b.spec.ts": `
          import { test, expect } from './helper';

          test('test-b', () => {
            expect_eq(foo, 'foo');
          });
        `,
        "c.spec.ts": `
          import { test, expect } from './helper';

          test('test-c', () => {
            expect_eq(foo, 'foo');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Playwright detected inconsistent test.use() options.") != null, true);
    });
    test("tear down base fixture after error in derived", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            context: ({}, use, testInfo) => {
              console.log('\\n%%context setup ' + testInfo.status);
              use();
              console.log('\\n%%context teardown ' + testInfo.status);
            },
            page: ({ context }, use, testInfo) => {
              console.log('\\n%%page setup ' + testInfo.status);
              use();
              console.log('\\n%%page teardown ' + testInfo.status);
              throw new Error('Error in page teardown');
            },
          });
          test('test', () => {});
        `
      });
      expect_eq(result.exitCode, 1);
      expect(result.outputLines).toEqual([
        "context setup passed",
        "page setup passed",
        "page teardown passed",
        "context teardown failed",
      ]);
    });
    test("should not continue with scope teardown after fixture teardown timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: ({ }, use) => {
              use();
              console.log('in fixture teardown');
            },
            fixture2: ({ fixture }, use) => {
              use();
              console.log('in fixture2 teardown');
              new Promise(() => {});
            },
          });
          test.use({ trace: 'on' });
          test('good', () => {
          });
        `,
      }, { reporter: "list", timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Tearing down \"fixture2\" exceeded the test timeout of 1000ms.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "in fixture teardown") == null, true);
    });
    test("should report fixture teardown error after test error", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({ }, use) => {
              use();
              throw new Error('Error from the fixture foo');
            },
          });
          test('fails', () => {
            throw new Error('Error from the test');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error from the fixture foo") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Error from the test") != null, true);
    });
    test("should throw when overriding non-option fixture in config", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
              foo: 'overridden',
              headless: true,
              unknownThing: 'ignored',
            },
          };
        `,
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, use) => use('original'),
          });
          test('works', () => {
            expect_eq(foo, 'original');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"foo\" cannot be overridden in the configuration \"use\" section. Only fixtures registered with { option: true } can be set in the config.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"headless\"") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"unknownThing\"") == null, true);
    });
    test("should throw when overriding option fixture with non-option", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {};
        `,
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            headless: [
              ({}, use) => use(true),
              { scope: 'worker', option: false },
            ],
          });
          test('works', () => {
            expect_eq(headless, true);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"headless\" has already been registered as a { option: true } fixture") != null, true);
    });
    test("should allow overriding option both in config and in test.use block", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
              headless: true,
            },
          };
        `,
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            headless: [
              ({}, use) => use(true),
              { scope: 'worker' },
            ],
          });
          test('works', () => {
            expect_eq(headless, true);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
    });
}
