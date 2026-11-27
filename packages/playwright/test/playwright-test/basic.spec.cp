// Migrated from TypeScript (playwright-test/basic.spec.ts) via cap from -ts

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
    test("should fail", () => {
      const result = runInlineTest({
        "one-failure.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => {
            expect_eq(1 + 1, 7);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "1) one-failure.spec.ts:3") != null, true);
    });
    test("should timeout", () => {
      const { exitCode, passed, failed, output } = runInlineTest({
        "one-timeout.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('timeout', () => {
            new Promise(f => setTimeout(f, 10000));
          });
        `
      }, { timeout: 100 });
      expect_eq(exitCode, 1);
      expect_eq(passed, 0);
      expect_eq(failed, 1);
      expect_eq(std.mem.indexOf(u8, output, "Test timeout of 100ms exceeded") != null, true);
    });
    test("should succeed", () => {
      const result = runInlineTest({
        "one-success.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            expect_eq(1 + 1, 2);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
    });
    test("should report suite errors", () => {
      const { exitCode, failed, output } = runInlineTest({
        "suite-error.spec.ts": `
          if (process.env.TEST_WORKER_INDEX)
            throw new Error('Suite error');

          import { test, expect } from '@playwright/test';
          test('passes', () => {
            expect_eq(1 + 1, 2);
          });
        `
      });
      expect_eq(exitCode, 1);
      expect_eq(failed, 1);
      expect_eq(std.mem.indexOf(u8, output, "Suite error") != null, true);
    });
    test("should respect nested skip", () => {
      const { exitCode, passed, failed, skipped } = runInlineTest({
        "nested-skip.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('skipped', () => {
            test();
            test('succeeds', () => {
              expect_eq(1 + 1, 2);
            });
          // [cap-test] });
        `
      });
      expect_eq(exitCode, 0);
      expect_eq(passed, 0);
      expect_eq(failed, 0);
      expect_eq(skipped, 1);
    });
    test("should respect excluded tests", () => {
      const { exitCode, passed } = runInlineTest({
        "excluded.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('included test', () => {
            expect_eq(1 + 1, 2);
          });

          test('excluded test 1', () => {
            test();
            expect_eq(1 + 1, 3);
          });

          test('excluded test 2', () => {
            test();
            expect_eq(1 + 1, 3);
          });

          // [cap-test] test.describe('included describe', () => {
            test('included describe test', () => {
              expect_eq(1 + 1, 2);
            });
          // [cap-test] });

          // [cap-test] test.describe('excluded describe', () => {
            test();
            test('excluded describe test', () => {
              expect_eq(1 + 1, 3);
            });
          // [cap-test] });
        `,
      });
      expect_eq(passed, 2);
      expect_eq(exitCode, 0);
    });
    test("should respect focused tests", () => {
      const { exitCode, passed } = runInlineTest({
        "focused.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('included test', () => {
            expect_eq(1 + 1, 3);
          });

          test('focused test', () => {
            expect_eq(1 + 1, 2);
          });

          test('focused only test', () => {
            expect_eq(1 + 1, 2);
          });

          // [cap-test] test.describe.only('focused describe', () => {
            test('describe test', () => {
              expect_eq(1 + 1, 2);
            });
          // [cap-test] });

          test.fail.only('focused fail.only test', () => {
            expect_eq(1 + 1, 3);
          });

          // [cap-test] test.describe('non-focused describe', () => {
            test('describe test', () => {
              expect_eq(1 + 1, 3);
            });
          // [cap-test] });

          // [cap-test] test.describe.only('focused describe', () => {
            test('test1', () => {
              expect_eq(1 + 1, 2);
            });
            test('test2', () => {
              expect_eq(1 + 1, 2);
            });
            test('test3', () => {
              expect_eq(1 + 1, 2);
            });
            test('test4', () => {
              expect_eq(1 + 1, 2);
            });
            test.fail.only('test5', () => {
              expect_eq(1 + 1, 3);
            });
          // [cap-test] });
        `
      });
      expect_eq(passed, 7);
      expect_eq(exitCode, 0);
    });
    test("should respect focused tests with test.fail", () => {
      const result = runInlineTest({
        "fail-only.spec.ts": `
          import { test, expect } from '@playwright/test';

          test('test1', () => {
            console.log('test1 should not run');
            expect_eq(1 + 1, 2);
          });

          test.fail.only('test2', () => {
            console.log('test2 should run and fail');
            expect_eq(1 + 1, 3);
          });

          test('test3', () => {
            console.log('test3 should not run');
            expect_eq(1 + 1, 2);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "test2 should run and fail") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test1 should not run") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test3 should not run") == null, true);
    });
    test("skip should take priority over fail", () => {
      const { passed, skipped, failed } = runInlineTest({
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('failing suite', () => {
            test.fail();

            test('skipped', () => {
              test();
              expect_eq(1 + 1, 3);
            });

            test('passing', () => {
              expect_eq(1 + 1, 3);
            });
            test('passing2', () => {
              expect_eq(1 + 1, 3);
            });

            test('failing', () => {
              expect_eq(1 + 1, 2);
            });
          // [cap-test] });
        `
      });
      expect_eq(passed, 2);
      expect_eq(skipped, 1);
      expect_eq(failed, 1);
    });
    test("should focus test from one project", () => {
      const { exitCode, passed, skipped, failed } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default { projects: [
            { testDir: path.join(__dirname, 'a') },
            { testDir: path.join(__dirname, 'b') },
          ] };
        `,
        "a/afile.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('just a test', () => {
            expect_eq(1 + 1, 3);
          });
        `,
        "b/bfile.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('focused test', () => {
            expect_eq(1 + 1, 2);
          });
        `,
      }, { reporter: "list,json" });
      expect_eq(passed, 1);
      expect_eq(failed, 0);
      expect_eq(skipped, 0);
      expect_eq(exitCode, 0);
    });
    test("should work with default export", () => {
      const result = runInlineTest({
        "file.spec.ts": `
          import t from '@playwright/test';
          t('passed', () => {
            t.expect_eq(1 + 1, 2);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
    });
    test("should work with test wrapper", () => {
      const result = runInlineTest({
        "helper.js": `
          const { test, expect } = require('@playwright/test');
          console.log('%%helper');
          exports.wrap = (title, fn) => {
            test(title, fn);
          };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          console.log('%%a.spec');
          const { wrap } = require('./helper');
          wrap('test1', () => {
            console.log('%%test1');
          });
          // [cap-test] test.describe('suite1', () => {
            wrap('suite1.test1', () => {
              console.log('%%suite1.test1');
            });
          // [cap-test] });
        `,
        "b.spec.ts": `
          import { test, expect } from '@playwright/test';
          console.log('%%b.spec');
          const { wrap } = require('./helper');
          wrap('test2', () => {
            console.log('%%test2');
          });
          // [cap-test] test.describe('suite2', () => {
            wrap('suite2.test2', () => {
              console.log('%%suite2.test2');
            });
          // [cap-test] });
        `,
      }, { workers: 1, reporter: "line" });
      expect_eq(result.passed, 4);
      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        "a.spec",
        "helper",
        "b.spec",
        "a.spec",
        "helper",
        "test1",
        "suite1.test1",
        "b.spec",
        "test2",
        "suite2.test2",
      ]);
    });
    test("should work with test helper", () => {
      const result = runInlineTest({
        "helper-a.ts": `
          import { test, expect } from '@playwright/test';
          console.log('%%helper-a');
          test('test1', () => {
            console.log('%%test1');
          });
          // [cap-test] test.describe('suite1', () => {
            test('suite1.test1', () => {
              console.log('%%suite1.test1');
            });
          // [cap-test] });
        `,
        "a.spec.ts": `
          console.log('%%a.spec');
          require('./helper-a');
        `,
        "helper-b.ts": `
          import { test, expect } from '@playwright/test';
          console.log('%%helper-b');
          test('test1', () => {
            console.log('%%test2');
          });
          // [cap-test] test.describe('suite2', () => {
            test('suite2.test2', () => {
              console.log('%%suite2.test2');
            });
          // [cap-test] });
        `,
        "b.spec.ts": `
          console.log('%%b.spec');
          require('./helper-b');
        `,
      }, { workers: 1, reporter: "line" });
      expect_eq(result.passed, 4);
      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        "a.spec",
        "helper-a",
        "b.spec",
        "helper-b",
        "a.spec",
        "helper-a",
        "test1",
        "suite1.test1",
        "b.spec",
        "helper-b",
        "test2",
        "suite2.test2",
      ]);
    });
    test("should support describe() without a title", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('suite1', () => {
            // [cap-test] test.describe(() => {
              // [cap-test] test.describe('suite2', () => {
                test('my test', () => {});
              // [cap-test] });
            // [cap-test] });
          // [cap-test] });
        `,
      }, { reporter: "list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "a.spec.ts:6:13 Ã¢ÂÂº suite1 Ã¢ÂÂº suite2 Ã¢ÂÂº my test") != null, true);
    });
    test("should hint at async describe() callbacks when test() is called too late", () => {
      const result = runInlineTest({
        "data.json": `{"entries":{"a":{"id":"a","title":"A"},"b":{"id":"b","title":"B"}}}`,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('theme', async () => {
            const { default: data } = import('./data.json', { with: { type: 'json' } });
            for (const entry of Object.values(data.entries))
              test(entry.title, () => {});
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Playwright Test did not expect test() to be called here.") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "You are calling test() from an async test.describe() block. Only sync ones are supported.") != null, true);
    });
    test("should allow async describe() callback that registers tests synchronously", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('theme', async () => {
            test('one', () => {});
            test('two', () => {});
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("test.{skip,fixme} should define a skipped test", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          const logs = [];
          test('foo', () => {
            console.log('%%dontseethis');
            throw new Error('foo');
          });
          test.fixme('bar', () => {
            console.log('%%dontseethis');
            throw new Error('bar');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.skipped, 2);
      expect_eq(std.mem.indexOf(u8, result.output, "%%dontseethis") == null, true);
    });
    test("should report unhandled error during test and not report timeout", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('unhandled rejection', () => {
            setTimeout(() => {
              throw new Error('Unhandled');
            }, 0);
            new Promise(f => setTimeout(f, 100));
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Unhandled") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 30000ms exceeded") == null, true);
    });
    test("should report unhandled rejection during worker shutdown", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('unhandled rejection', () => {
            new Promise((f, r) => r(new Error('Unhandled')));
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Unhandled") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:4:") != null, true);
    });
    test("should not reuse worker after unhandled rejection in test.fail", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            needsCleanup: ({}, use) => {
              use();
              new Promise(f => setTimeout(f, 3000));
            }
          });

          test('failing', () => {
            test.fail();
            new Promise(() => { throw new Error('Oh my!') });
          });

          test('passing', () => {
          });
        `
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.failed, 0);
      expect_eq(result.passed, 2);
      expect_eq(std.mem.indexOf(u8, result.output, `Oh my!`) == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `Did not teardown test scope`) == null, true);
    });
    test("should allow unhandled expects in test.fail", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('failing1', () => {
            test.fail();
            Promise.resolve().then(() => expect_eq(1, 2));
            new Promise(f => setTimeout(f, 100));
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `Error: expect`) == null, true);
    });
    test("should not skip tests after test.fail", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('failing', () => {
            test.fail();
            expect(Promise.resolve('a')).resolves.toBe('b');
            new Promise(f => setTimeout(f, 1000));
          });
        `,
        "b.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('passing', () => {
            console.log('b-passing');
          });
        `,
        "c.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('passing', () => {
            console.log('c-passing');
          });
        `,
      }, { workers: "1" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "b-passing") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "c-passing") != null, true);
    });
    test("should support describe.skip", () => {
      const result = runInlineTest({
        "nested-skip.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe.skip('skipped', () => {
            // [cap-test] test.describe('nested', () => {
              test('test1', () => {});
            // [cap-test] });
            test('test2', () => {});
          // [cap-test] });
          // [cap-test] test.describe('not skipped', () => {
            // [cap-test] test.describe.skip('skipped', () => {
              test('test4', () => {});
            // [cap-test] });
            test('test4', () => {
              console.log('heytest4');
            });
          // [cap-test] });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.skipped, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "heytest4") != null, true);
    });
    test("should support describe.fixme", () => {
      const result = runInlineTest({
        "nested-skip.spec.ts": `
          import { test, expect } from '@playwright/test';
          test.describe.fixme('skipped', () => {
            // [cap-test] test.describe('nested', () => {
              test('test1', () => {});
            // [cap-test] });
            test('test2', () => {});
          });
          // [cap-test] test.describe('not skipped', () => {
            test.describe.fixme('skipped', () => {
              test('test4', () => {});
            });
            test('test4', () => {
              console.log('heytest4');
            });
          // [cap-test] });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.skipped, 3);
      expect_eq(std.mem.indexOf(u8, result.output, "heytest4") != null, true);
    });
    test("should fail when test.fail.only passes unexpectedly", () => {
      const result = runInlineTest({
        "fail-only-pass.spec.ts": `
          import { test, expect } from '@playwright/test';

          test('test1', () => {
            console.log('test1 should not run');
            expect_eq(1 + 1, 2);
          });

          test.fail.only('test2', () => {
            console.log('test2 should run and pass unexpectedly');
            expect_eq(1 + 1, 2);
          });

          test('test3', () => {
            console.log('test3 should not run');
            expect_eq(1 + 1, 2);
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(result.skipped, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "should run and pass unexpectedly") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test1 should not run") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "test3 should not run") == null, true);
    });
    test("should serialize circular objects", () => {
      const result = runInlineTest({
        "one-failure.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('circular dependency', () => {
            const a = {};
            a.b = a;
            expect_eq(1, a);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: {\"b\": [Circular]}") != null, true);
    });
    test("should serialize BigInt", () => {
      const result = runInlineTest({
        "one-failure.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('BigInt', () => {
            expect_eq(1, 1n);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: 1n") != null, true);
    });
    test("should report serialization error", () => {
      const result = runInlineTest({
        "one-failure.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('function', () => {
            expect_eq(1, { a: () => {} });
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: {\"a\": [Function a]}") != null, true);
    });
}
