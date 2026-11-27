// Migrated from TypeScript (playwright-test/reporter-base.spec.ts) via cap from -ts

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

import { test, expect, stripAnsi } from "./playwright-test-fixtures.cp";
import * as path from "cap:path";

for (const useIntermediateMergeReport of [false, true]) { test.describe(`${if (useIntermediateMergeReport) "merged" else "created"}`, () => { test.use({ useIntermediateMergeReport });  test("handle long test names", (_p) => { const title = "title".repeat(30); const result = runInlineTest({ "a.test.js": ` import { test, expect } from '@playwright/test'; test('${title}', async ({}) => { expect(1).toBe(0); }); `, }); expect_eq(std.mem.indexOf(u8, result.output, "expect(1).toBe") != null, true); expect_eq(result.exitCode, 1); });  test("print the error name", (_p) => { const result = runInlineTest({ "a.spec.ts": ` import { test, expect } from '@playwright/test'; test('foobar', async ({}) => { const error = new Error('my-message'); error.name = 'FooBarError'; throw error;
        });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "FooBarError: my-message") != null, true);
    });

    test("print should print the error name without a message", () => {
      const result = runInlineTest({
        "a.spec.ts": `
        import { test, expect } from '@playwright/test';
        test('foobar', () => {
          const error = new Error();
          error.name = 'FooBarError';
          throw error;
        });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "FooBarError") != null, true);
    });

    test("should print an error in a codeframe", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('foobar', () => {
            const error = new Error('my-message');
            error.name = 'FooBarError';
            throw error;
          });
        `
      }, {}, {
        FORCE_COLOR: "0",
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "FooBarError: my-message") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "at a.spec.ts:5") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `  2 |           import { test, expect } from '@playwright/test';`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `  3 |           test('foobar', async ({}) => {`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `> 4 |             const error = new Error('my-message');`) != null, true);
    });

    test("should filter out node_modules error in a codeframe", () => {
      const result = runInlineTest({
        "node_modules/utils/utils.js": `
          function assert(value) {
            if (!value)
              throw new Error('Assertion error');
          }
          module.exports = { assert };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          const { assert } = require('utils/utils.js');
          test('fail', () => {
            assert(false);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      const output = result.output;
      expect_eq(std.mem.indexOf(u8, output, "Error: Assertion error") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "a.spec.ts:4:11 Ã¢ÂÂº fail") != null, true);
      expect_eq(std.mem.indexOf(u8, output, `  4 |           test('fail', async ({}) => {`) != null, true);
      expect_eq(std.mem.indexOf(u8, output, `> 5 |             assert(false);`) != null, true);
      expect_eq(std.mem.indexOf(u8, output, `    |             ^`) != null, true);
      expect_eq(std.mem.indexOf(u8, output, `utils.js:4`) != null, true);
      expect_eq(std.mem.indexOf(u8, output, `a.spec.ts:5:13`) != null, true);
    });

    test("should print error with a nested cause", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';

          test('foobar', () => {
            try {
              try {
                const error = new Error('my-message');
                error.name = 'SpecialError';
                throw error;
              } catch (e) {
                try {
                  throw new Error('inner-message', { cause: e });
                } catch (e) {
                  throw new Error('outer-message', { cause: e });
                }
              }
            } catch (e) {
              throw new Error('wrapper-message', { cause: e });
            }
          });
          test.afterAll(() => {
            expect_eq(test.info().errors.length, 1);
            expect_eq(test.info().errors[0], test.info().error);
            expect_eq(test.info().error.message, 'Error: wrapper-message');
            expect_eq(test.info().error.cause.message, 'Error: outer-message');
            expect_eq(test.info().error.cause.cause.message, 'Error: inner-message');
            expect_eq(test.info().error.cause.cause.cause.message, 'SpecialError: my-message');
            expect_eq(test.info().error.cause.cause.cause.cause, undefined);
            console.log('afterAll executed successfully');
          })
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      const testFile = path.join(result.report.config.rootDir, result.report.suites[0].specs[0].file);
      expect_eq(std.mem.indexOf(u8, result.output, `${testFile}:18:`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `[cause]: Error: outer-message`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `${testFile}:14:`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `[cause]: Error: inner-message`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `${testFile}:12:`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `[cause]: SpecialError: my-message`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `${testFile}:7:`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "afterAll executed successfully") != null, true);
    });

    test("should print codeframe from a helper", () => {
      const result = runInlineTest({
        "helper.ts": `
          export function ohMy() {
            throw new Error('oh my');
          }
        `,
        "a.spec.ts": `
          import { ohMy } from './helper';
          import { test, expect } from '@playwright/test';
          test('foobar', () => {
            ohMy();
          });
        `
      }, {}, {
        FORCE_COLOR: "0",
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: oh my") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `   2 |           export function ohMy() {`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, ` > 3 |             throw new Error('oh my');`) != null, true);
      expect(result.output).toMatch("^ +\\| +\\^$");
    });

    test("should print slow tests", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'foo' },
              { name: 'bar' },
              { name: 'baz' },
              { name: 'qux' },
            ],
            reportSlowTests: { max: 0, threshold: 2400 },
          };
        `,
        "dir/a.test.js": `
          import { test, expect } from '@playwright/test';
          test('slow test', () => {
            new Promise(f => setTimeout(f, 2500));
          });
        `,
        "dir/b.test.js": `
          import { test, expect } from '@playwright/test';
          test('fast test', () => {
            new Promise(f => setTimeout(f, 1));
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 8);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [foo] ÃÂ¢ÃÂÃÂº dir${path.sep}a.test.js (`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [bar] ÃÂ¢ÃÂÃÂº dir${path.sep}a.test.js (`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [baz] ÃÂ¢ÃÂÃÂº dir${path.sep}a.test.js (`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [qux] ÃÂ¢ÃÂÃÂº dir${path.sep}a.test.js (`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Consider running tests from slow files in parallel`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [foo] ÃÂ¢ÃÂÃÂº dir${path.sep}b.test.js (`) == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [bar] ÃÂ¢ÃÂÃÂº dir${path.sep}b.test.js (`) == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [baz] ÃÂ¢ÃÂÃÂº dir${path.sep}b.test.js (`) == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Slow test file: [qux] ÃÂ¢ÃÂÃÂº dir${path.sep}b.test.js (`) == null, true);
    });

    test("should print if maxFailures is reached", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            maxFailures: 1,
          };
        `,
        "dir/a.test.js": `
          import { test, expect } from '@playwright/test';
          test('failing1', () => {
            expect_eq(1, 2);
          });
          test('failing2', () => {
            expect_eq(1, 2);
          });
          test('failing3', () => {
            expect_eq(1, 2);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.didNotRun, 2);
      expect_eq(std.mem.indexOf(u8, result.output, "Testing stopped early after 1 maximum allowed failures.") != null, true);
    });

    test("should print if globalTimeout is reached", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29768" });
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            globalTimeout: 3000,
          };
        `,
        "dir/a.test.js": `
          import { test, expect } from '@playwright/test';
          test('first', () => {
          });
          test('second (hanging)', () => {
            new Promise(() => {});
          });
          test('third', () => {
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      // TODO: uncomment once https://github.com/microsoft/playwright/issues/30550 is fixed.
      // expect(result.interrupted).toBe(1);
      // expect(result.didNotRun).toBe(1);
      expect_eq(std.mem.indexOf(u8, result.output, "Timed out waiting 3s for the test suite to run") != null, true);
    });

    test("should not print slow parallel tests", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            reportSlowTests: { max: 0, threshold: 500 },
          };
        `,
        "dir/a.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.parallel('suite', () => {
            test('inner slow test', () => {
              new Promise(f => setTimeout(f, 1000));
            });
            test('inner fast test', () => {
              new Promise(f => setTimeout(f, 100));
            });
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(std.mem.indexOf(u8, result.output, "Slow test file") == null, true);
    });

    test("should not print slow tests", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'baz' },
              { name: 'qux' },
            ],
            reportSlowTests: null,
          };
        `,
        "dir/a.test.js": `
          import { test, expect } from '@playwright/test';
          test('slow test', () => {
            new Promise(f => setTimeout(f, 1000));
          });
          test('fast test', () => {
            new Promise(f => setTimeout(f, 100));
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect_eq(std.mem.indexOf(u8, result.output, "Slow test") == null, true);
    });

    test("should print flaky failures", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('foobar', () => {
            expect_eq(testInfo.retry, 1);
          });
        `
      }, { retries: "1", reporter: "list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "expect(testInfo.retry).toBe(1)") != null, true);
    });

    test("should print flaky timeouts", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('foobar', () => {
            if (!testInfo.retry)
              new Promise(f => setTimeout(f, 2000));
          });
        `
      }, { retries: "1", reporter: "list", timeout: "1000" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
    });

    test("should print stack-less errors", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('foobar', () => {
            const e = new Error('Hello');
            delete e.stack;
            throw e;
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Hello") != null, true);
    });

    test("should print errors with inconsistent message/stack", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('foobar', function myTest({}) {
            const e = new Error('Hello');
            // Force stack to contain "Hello".
            // Otherwise it is computed lazy and will get 'foo bar' instead.
            e.stack;
            e.message = 'foo bar';
            throw e;
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      const output = result.output;
      expect_eq(std.mem.indexOf(u8, output, "foo bar") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "function myTest") != null, true);
    });

    test("should print \"no tests found\" error", () => {
      const result = runInlineTest({ });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "No tests found") != null, true);
    });

    test("should not crash on undefined body with manual attachments", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            testInfo.attachments.push({
              name: 'foo.txt',
              body: undefined,
              contentType: 'text/plain'
            });
            expect_eq(1, 2);
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Error in reporter") == null, true);
      expect_eq(result.failed, 1);
      expect_eq(result.exitCode, 1);
    });

    test("should report fatal errors at the end", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: [({ }, use) => {
              use();
              throw new Error('oh my!');
            }, { scope: 'worker' }],
          });
          test('good', () => {
          });
        `,
        "b.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            fixture: [({ }, use) => {
              use();
              throw new Error('oh my!');
            }, { scope: 'worker' }],
          });
          test('good', () => {
          });
        `,
      }, { reporter: "list" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 2);
      expect_eq(std.mem.indexOf(u8, result.output, "2 errors were not a part of any test, see above for details") != null, true);
    });

    test("should contain at most 1 decimal for humanized timing", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {});
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.output).toMatch("\\d+ passed \\(\\d+(\\.\\d)?(ms|s)\\)");
    });

    test("should output tags", () => {
      const result = runInlineTest({
        "a.test.ts": `
          const { test, expect } = require('@playwright/test');
          test('passes', { tag: ['@foo1', '@foo2'] }, ({}) => {
            expect_eq(0, 0);
          });
          test('passes @bar1 @bar2', () => {
            expect_eq(0, 0);
          });
          test('passes @baz1', { tag: ['@baz2'] }, ({}) => {
            expect_eq(0, 0);
          });
        `,
      });
      const text = stripAnsi(result.output);
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes @foo1 @foo2 (") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes @bar1 @bar2 (") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes @baz1 @baz2 (") != null, true);
    });

    test("should omit tags when omitTags is set", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            reporter: [['list', { omitTags: true }]],
          };
        `,
        "a.test.ts": `
          const { test, expect } = require('@playwright/test');
          test('passes', { tag: ['@foo1', '@foo2'] }, ({}) => {
            expect_eq(0, 0);
          });
          test('passes @bar1 @bar2', () => {
            expect_eq(0, 0);
          });
          test('passes @baz1', { tag: ['@baz2'] }, ({}) => {
            expect_eq(0, 0);
          });
        `,
      });
      const text = stripAnsi(result.output);
      // Tags appended from the `tag` annotation are omitted.
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes (") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "@foo1") == null, true);
      expect_eq(std.mem.indexOf(u8, text, "@foo2") == null, true);
      expect_eq(std.mem.indexOf(u8, text, "@baz2") == null, true);
      // Tags authored directly in the title are preserved.
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes @bar1 @bar2 (") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes @baz1 (") != null, true);
      expect_eq(result.exitCode, 0);
    });

    test("should omit tags from failures when omitTags is set", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            reporter: [['list', { omitTags: true }]],
          };
        `,
        "a.test.ts": `
          const { test, expect } = require('@playwright/test');
          test('fails', { tag: ['@qux1'] }, ({}) => {
            expect_eq(1, 2);
          });
        `,
      });
      const text = stripAnsi(result.output);
      const titleLines = text.split("\n").filter(line => line.includes("a.test.ts") && line.includes("ÃÂ¢ÃÂÃÂº fails"));
      expect_eq(titleLines.length > 0, true);
      for (const line of titleLines)
        expect_eq(std.mem.indexOf(u8, line, "@qux1") == null, true);
      expect_eq(result.exitCode, 1);
    });

    test("should omit tags via the PLAYWRIGHT_LIST_OMIT_TAGS environment variable", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            reporter: [['list']],
          };
        `,
        "a.test.ts": `
          const { test, expect } = require('@playwright/test');
          test('passes', { tag: ['@foo1', '@foo2'] }, ({}) => {
            expect_eq(0, 0);
          });
          test('passes @bar1 @bar2', () => {
            expect_eq(0, 0);
          });
        `,
      }, null, { PLAYWRIGHT_LIST_OMIT_TAGS: "1" });
      const text = stripAnsi(result.output);
      // Tags appended from the `tag` annotation are omitted.
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes (") != null, true);
      expect_eq(std.mem.indexOf(u8, text, "@foo1") == null, true);
      expect_eq(std.mem.indexOf(u8, text, "@foo2") == null, true);
      // Tags authored directly in the title are preserved.
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes @bar1 @bar2 (") != null, true);
      expect_eq(result.exitCode, 0);
    });

    test("should let PLAYWRIGHT_LIST_OMIT_TAGS override the omitTags option", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            reporter: [['list', { omitTags: true }]],
          };
        `,
        "a.test.ts": `
          const { test, expect } = require('@playwright/test');
          test('passes', { tag: ['@foo1', '@foo2'] }, ({}) => {
            expect_eq(0, 0);
          });
        `,
      }, null, { PLAYWRIGHT_LIST_OMIT_TAGS: "0" });
      const text = stripAnsi(result.output);
      // The environment variable disables omitTags, so the appended tags are shown.
      expect_eq(std.mem.indexOf(u8, text, "Ã¢ÂÂº passes @foo1 @foo2 (") != null, true);
      expect_eq(result.exitCode, 0);
    });
  });
}
