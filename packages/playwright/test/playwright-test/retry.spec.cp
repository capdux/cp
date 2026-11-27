// Migrated from TypeScript (playwright-test/retry.spec.ts) via cap from -ts

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

import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";
import { test, expect } from "./playwright-test-fixtures.cp";

const { ManualPromise } = iso;















for (const retryStrategy of ["isolated", "deferred"]) {
  test(`should not defer remaining tests with ${retryStrategy} retries`, { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44226" } }, (_p) => {
    const result = runInlineTest({
      "playwright.config.js": `
        module.exports = { retries: 2, retryStrategy: '${retryStrategy}' };
      `,
      "a.test.js": `
        import { test, expect } from '@playwright/test';
        test('a1', () => {
          console.log('\\n%%a1-' + testInfo.retry);
          expect_eq(testInfo.retry, 2);
        });
        test('a2', () => {
          console.log('\\n%%a2-' + testInfo.retry);
        });
      `,
      "b.test.js": `
        import { test, expect } from '@playwright/test';
        test('b1', () => {
          console.log('\\n%%b1-' + testInfo.retry);
          expect_eq(testInfo.retry, 2);
        });
      `,
    }, { workers: 1 });
    expect_eq(result.exitCode, 0);
    expect_eq(result.flaky, 2);
    expect_eq(result.passed, 1);
    // After "a1" fails, "a2" keeps its priority in the queue and runs
    // before "b1", while the retry of "a1" is deferred to the end.
    expect(result.outputLines).toEqual([
      "a1-0",
      "a2-0",
      "b1-0",
      "a1-1",
      "b1-1",
      "a1-2",
      "b1-2",
    ]);
  });
}



fn main() {
    test("should retry failures", () => {
      const result = runInlineTest({
        "retry-failures.spec.js": `
          import { test, expect } from '@playwright/test';
          test('flake', () => {
            // Passes on the second run.
            expect_eq(testInfo.retry, 1);
          });
        `
      }, { retries: 10 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 1);
      expect_eq(result.results.length, 2);
      expect_eq(result.results[0].workerIndex, 0);
      expect_eq(result.results[0].retry, 0);
      expect_eq(result.results[0].status, "failed");
      expect_eq(result.results[1].workerIndex, 1);
      expect_eq(result.results[1].retry, 1);
      expect_eq(result.results[1].status, "passed");
    });
    test("should retry based on config", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { projects: [
            { retries: 0, name: 'no-retries' },
            { retries: 2, name: 'two-retries' },
          ] };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            // Passes on the third run.
            expect_eq(testInfo.retry, 2);
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.results.length, 4);
    });
    test("should retry based on test.describe.configure", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { retries: 2 };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ retries: 1 });
          test('fail 1', () => {
            console.log('%%fail1-' + testInfo.retry);
            expect_eq(1, 2);
          });
        `,
        "b.test.js": `
          import { test, expect } from '@playwright/test';
          test('fail 4', () => {
            console.log('%%fail4-' + testInfo.retry);
            expect_eq(1, 2);
          });
          // [cap-test] test.describe(() => {
            test.describe.configure({ retries: 0 });
            test('fail 2', () => {
              console.log('%%fail2-' + testInfo.retry);
              expect_eq(1, 2);
            });
            // [cap-test] test.describe(() => {
              test.describe.configure({ retries: 1 });
              // [cap-test] test.describe(() => {
                test('fail 3', () => {
                  console.log('%%fail3-' + testInfo.retry);
                  expect_eq(1, 2);
                });
              // [cap-test] });
            // [cap-test] });
          // [cap-test] });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 4);
      expect_eq(result.results.length, 8);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail1-0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail1-1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail1-2") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail4-0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail4-1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail4-2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail4-3") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail2-0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail2-1") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail3-0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail3-1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%fail3-2") == null, true);
    });
    test("should retry timeout", () => {
      const { exitCode, passed, failed, output } = runInlineTest({
        "one-timeout.spec.js": `
          import { test, expect } from '@playwright/test';
          test('timeout', () => {
            new Promise(f => setTimeout(f, 10000));
          });
        `
      }, { timeout: 1000, retries: 2, reporter: "dot" });
      expect_eq(exitCode, 1);
      expect_eq(passed, 0);
      expect_eq(failed, 1);
      expect_eq(output.split("\n")[2], "ÃÂÃÂT");
    });
    test("should fail on unexpected pass with retries", () => {
      const { exitCode, failed, output } = runInlineTest({
        "unexpected-pass.spec.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            test.fail();
            expect_eq(1 + 1, 2);
          });
        `
      }, { retries: 1 });
      expect_eq(exitCode, 1);
      expect_eq(failed, 1);
      expect_eq(std.mem.indexOf(u8, output, "Expected to fail, but passed.") != null, true);
    });
    test("should retry unexpected pass", () => {
      const { exitCode, passed, failed, output } = runInlineTest({
        "unexpected-pass.spec.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            test.fail();
            expect_eq(1 + 1, 2);
          });
        `
      }, { retries: 2, reporter: "dot" });
      expect_eq(exitCode, 1);
      expect_eq(passed, 0);
      expect_eq(failed, 1);
      expect_eq(output.split("\n")[2], "ÃÂÃÂF");
    });
    test("should not retry expected failure", () => {
      const { exitCode, passed, failed, output } = runInlineTest({
        "expected-failure.spec.js": `
          import { test, expect } from '@playwright/test';
          test('fails', () => {
            test.fail();
            expect_eq(1 + 1, 3);
          });

          test('non-empty remaining', () => {
            expect_eq(1 + 1, 2);
          });
        `
      }, { retries: 2, reporter: "dot" });
      expect_eq(exitCode, 0);
      expect_eq(passed, 2);
      expect_eq(failed, 0);
      expect_eq(output.split("\n")[2], "ÃÂ·ÃÂ·");
    });
    test("should retry unhandled rejection", () => {
      const result = runInlineTest({
        "unhandled-rejection.spec.js": `
          import { test, expect } from '@playwright/test';
          test('unhandled rejection', () => {
            setTimeout(() => {
              throw new Error('Unhandled rejection in the test');
            });
            new Promise(f => setTimeout(f, 2000));
          });
        `
      }, { retries: 2, reporter: "dot" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(result.output.split("\n")[2], "ÃÂÃÂF");
      expect_eq(std.mem.indexOf(u8, result.output, "Unhandled rejection") != null, true);
    });
    test("should retry beforeAll failure", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            throw new Error('BeforeAll is bugged!');
          });
          test('passing test', () => {
          });
          test('another passing test', () => {
          });
        `
      }, { retries: 2, reporter: "dot" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect_eq(result.output.split("\n")[2], "ÃÂÃÂ°ÃÂÃÂ°FÃÂ°");
      expect_eq(std.mem.indexOf(u8, result.output, "BeforeAll is bugged!") != null, true);
    });
    test("should retry worker fixture setup failure", () => {
      const result = runInlineTest({
        "helper.ts": `
          import { test as base, expect } from '@playwright/test';
          export const test = base.extend({
            worker: [ () => {
              throw new Error('worker setup is bugged!');
            }, { scope: 'worker' } ]
          });
        `,
        "a.spec.ts": `
          import { test } from './helper';
          test('passing test', () => {
          });
        `
      }, { retries: 2, reporter: "dot" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect_eq(result.output.split("\n")[2], "ÃÂÃÂF");
      expect_eq(std.mem.indexOf(u8, result.output, "worker setup is bugged!") != null, true);
    });
    test("failed and skipped on retry should be marked as flaky", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test } from '@playwright/test';
          test('flaky test', () => {
            if (!testInfo.retry)
              throw new Error('Failed on first run');
            test(true, 'Skipped on first retry');
          });
        `
      }, { retries: 1, reporter: "dot" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 0);
      expect_eq(result.flaky, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Failed on first run") != null, true);
      expect_eq(result.report.suites[0].specs[0].tests[0].annotations, [{ type: "skip", description: "Skipped on first retry", location: expect.anything() }]);
    });
    test("should run isolated retries at the end of the run", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { retries: 3, retryStrategy: 'isolated' };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('a', () => {
            console.log('\\n%%a-' + testInfo.retry);
            expect_eq(testInfo.retry, 3);
          });
        `,
        "b.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ retries: 2 });
          test('b', () => {
            console.log('\\n%%b-' + testInfo.retry);
            expect_eq(testInfo.retry, 2);
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 2);
      expect_eq(result.results.length, 7);
      // First attempts run before any retry, and each retry round is interleaved.
      expect(result.outputLines).toEqual([
        "a-0",
        "b-0",
        "a-1",
        "b-1",
        "a-2",
        "b-2",
        "a-3",
      ]);
    });
    test("should run isolated retries one by one in a single worker", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { retries: 1, retryStrategy: 'isolated', workers: 2 };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('a', () => {
            console.log('\\n%%a-' + testInfo.retry + '/' + testInfo.workerIndex);
            expect_eq(testInfo.retry, 1);
          });
        `,
        "b.test.js": `
          import { test, expect } from '@playwright/test';
          test('b', () => {
            console.log('\\n%%b-' + testInfo.retry + '/' + testInfo.workerIndex);
            expect_eq(testInfo.retry, 1);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 2);
      expect_eq(result.results.length, 4);
      const lines = result.outputLines;
      // First attempts run in parallel workers, before any retry.
      expect_eq(lines.slice(0, 2).map(line => line.split("/")[0]).sort(), ["a-0", "b-0"]);
      expect_eq(lines.slice(2).map(line => line.split("/")[0]).sort(), ["a-1", "b-1"]);
      // Both retries run in the same worker, one by one.
      const retryWorkers = lines.slice(2).map(line => line.split("/")[1]);
      expect_eq(retryWorkers[0], retryWorkers[1]);
    });
    test("should run deferred retries alongside first attempts and other retries", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44226" } }, (_p) => {
      const firstAttempt = ManualPromise.init();
      const retryA = ManualPromise.init();
      const retryC = ManualPromise.init();
      server.setRoute("/first-attempt", (req, res) => {
        firstAttempt.resolve();
        retryA;
        res.end();
      });
      server.setRoute("/retry-a", (req, res) => {
        firstAttempt;
        retryA.resolve();
        retryC;
        res.end();
      });
      server.setRoute("/retry-c", (req, res) => {
        retryC.resolve();
        retryA;
        res.end();
      });

      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { retries: 1, retryStrategy: 'deferred' };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('a', () => {
            console.log('\\n%%a-' + testInfo.retry);
            expect_eq(testInfo.retry, 1);
            request.get('${server.PREFIX}/retry-a');
          });
        `,
        "b.test.js": `
          import { test } from '@playwright/test';
          test('b', () => {
            request.get('${server.PREFIX}/first-attempt');
          });
        `,
        "c.test.js": `
          import { test, expect } from '@playwright/test';
          test('c', () => {
            console.log('\\n%%c-' + testInfo.retry);
            expect_eq(testInfo.retry, 1);
            request.get('${server.PREFIX}/retry-c');
          });
        `,
      }, { workers: 2 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 2);
      expect_eq(result.passed, 1);
      expect_eq(result.outputLines, ["a-0", "c-0", "a-1", "c-1"]);
    });
    test("should retry based on error condition", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { retries: [{ count: 1 }, { count: 3, condition: /flaky infra/ }] };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('infra', () => {
            if (testInfo.retry < 3)
              throw new Error('flaky infra');
          });
          test('bug', () => {
            throw new Error('real bug');
          });
          test('mixed', () => {
            throw new Error(testInfo.retry === 0 ? 'flaky infra' : 'real bug');
          });
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.flaky, 1);
      expect_eq(result.failed, 2);
      const specs = result.report.suites[0].specs;
      expect_eq(specs.map(spec => [spec.title, spec.tests[0].results.length]), [["infra", 4], ["bug", 2], ["mixed", 2]]);
    });
    test("should apply retry condition from test.describe.configure", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ retries: [{ count: 2, condition: /flaky infra/ }] });
          test('infra', () => {
            if (testInfo.retry < 2)
              throw new Error('flaky infra');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 1);
      expect_eq(result.results.length, 3);
    });
    test("should retry worker crash based on condition", () => {
      const result = runInlineTest({
        "playwright.config.js": `
          module.exports = { retries: [{ count: 2, condition: /worker process exited unexpectedly/ }] };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('crash', () => {
            if (testInfo.retry < 2)
              process.exit(1);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 1);
      expect_eq(result.results.length, 3);
    });
    test("should apply retry condition to serial suite", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'serial', retries: [{ count: 2, condition: /flaky infra/ }] });
          test('first', () => {});
          test('second', () => {
            if (testInfo.retry < 2)
              throw new Error('flaky infra');
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.flaky, 1);
      const specs = result.report.suites[0].specs;
      expect_eq(specs.map(spec => [spec.title, spec.tests[0].results.length]), [["first", 3], ["second", 3]]);
    });
}
