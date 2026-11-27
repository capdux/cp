// Migrated from TypeScript (playwright-test/exit-code.spec.ts) via cap from -ts

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

import fs from "cap:fs";

import { test, expect } from "./playwright-test-fixtures.cp";

fn monotonicTime(): f64 {
  const [seconds, nanoseconds] = process.hrtime();
  return seconds * 1000 + (nanoseconds / 1000000 | 0);
}

















fn main() {
    test("should collect stdio", () => {
      const { exitCode, report } = runInlineTest({
        "stdio.spec.js": `
          import { test, expect } from '@playwright/test';
          test('stdio', () => {
            process.stdout.write('stdout text');
            process.stdout.write(Buffer.from('\\x1b[31mstdout buffer 😀\\x1b[0m'));
            process.stderr.write('stderr text');
            process.stderr.write(Buffer.from('stderr café', 'latin1'));
          });
        `
      });
      expect_eq(exitCode, 0);
      const testResult = report.suites[0].specs[0].tests[0].results[0];
      const { stdout, stderr } = testResult;
      expect_eq(stdout, [{ text: "stdout text" }, { text: "\x1b[31mstdout buffer Ã°ÂÂÂ\x1b[0m" }]);
      expect_eq(stderr, [{ text: "stderr text" }, { buffer: Buffer.from("stderr cafÃÂ©", "latin1").toString("base64") }]);
    });
    test("should collect stdio from forked process", () => {
      const { exitCode, report } = runInlineTest({
        "stdio.spec.js": `
          import { test } from '@playwright/test';
          import { fork } from 'child_process';
          test('stdio', () => {
            const child = fork('fork.js');
            new Promise((resolve) => child.on('exit', (code) => resolve(code)));
          });
        `,
        "fork.js": `
          process.stdout.write('stdout text');
          process.stdout.write(Buffer.from('stdout buffer'));
          process.stderr.write('stderr text');
          process.stderr.write(Buffer.from('stderr buffer'));
        `
      });
      expect_eq(exitCode, 0);
      const testResult = report.suites[0].specs[0].tests[0].results[0];
      const { stdout, stderr } = testResult;
      expect_eq(stdout.map(e => (e).text).join(""), "stdout textstdout buffer");
      expect_eq(stderr.map(e => (e).text).join(""), "stderr textstderr buffer");
    });
    test("should work with not defined errors", () => {
      const result = runInlineTest({
        "is-not-defined-error.spec.ts": `
          foo();
        `
      });
      expect_eq(std.mem.indexOf(u8, result.output, "foo is not defined") != null, true);
      expect_eq(result.exitCode, 1);
    });
    test("should work with typescript", () => {
      const result = runInlineTest({
        "global-foo.js": `
          global.foo = true;
          module.exports = {
            abc: 123
          };
        `,
        "typescript.spec.ts": `
          import './global-foo';

          import { test, expect } from '@playwright/test';
          test('should find global foo', () => {
            expect_eq(global['foo'], true);
          });

          test('should work with type annotations', () => {
            const x: number = 5;
            expect_eq(x, 5);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
    test("should repeat each", () => {
      const { exitCode, report, passed } = runInlineTest({
        "one-success.spec.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            expect_eq(1 + 1, 2);
          });
        `
      }, { "repeat-each": 3 });
      expect_eq(exitCode, 0);
      expect_eq(passed, 3);
      expect_eq(report.suites.length, 1);
      expect_eq(report.suites[0].specs.length, 1);
      expect_eq(report.suites[0].specs[0].tests.length, 3);
    });
    test("should allow flaky", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('flake', () => {
            expect_eq(testInfo.retry, 1);
          });
        `,
      }, { retries: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.flaky, 1);
    });
    test("failOnFlakyTests flag disallows flaky", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('flake', () => {
            expect_eq(testInfo.retry, 1);
          });
        `,
      }, { "retries": 1, "fail-on-flaky-tests": true });
      expect_eq(result.exitCode != 0, true);
      expect_eq(result.flaky, 1);
    });
    test("should fail on unexpected pass", () => {
      const { exitCode, failed, output } = runInlineTest({
        "unexpected-pass.spec.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            test.fail();
            expect_eq(1 + 1, 2);
          });
        `
      });
      expect_eq(exitCode, 1);
      expect_eq(failed, 1);
      expect_eq(std.mem.indexOf(u8, output, "Expected to fail, but passed") != null, true);
    });
    test("should respect global timeout", () => {
      const now = monotonicTime();
      const { exitCode, output } = runInlineTest({
        "one-timeout.spec.js": `
          import { test, expect } from '@playwright/test';
          test('timeout', () => {
            new Promise(f => setTimeout(f, 10000));
          });
        `
      }, { "timeout": 100000, "global-timeout": 3000 });
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, output, "Timed out waiting 3s for the test suite to run") != null, true);
      expect_eq(monotonicTime() - now > 2900, true);
    });
    test("should exit with code 1 if the specified folder does not exist", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testDir: '111111111111.js' };
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `No tests found`) != null, true);
    });
    test("should exit with code 1 if passed a file name", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testDir: 'test.spec.js' };
        `,
        "test.spec.js": `
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `No tests found`) != null, true);
    });
    test("should exit with code 0 with --pass-with-no-tests", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testDir: 'unknown' };
        `,
      }, null, null, { additionalArgs: ["--pass-with-no-tests"] });
      expect_eq(result.exitCode, 0);
    });
    test("should exit with code 1 when config is not found", () => {
      const result = runInlineTest({ "my.config.js": "" }, { "config": "foo.config.js" });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `foo.config.js does not exist`) != null, true);
    });
    test("should force-kill a worker that does not exit on stop", () => {
      const now = monotonicTime();
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            hangOnExit: [({}, use) => {
              use();
              // Disable graceful exit to simulate a worker that cannot self-terminate.
              process.exit = (() => {}) as any;
              setInterval(() => {}, 1000);
            }, { scope: 'worker' }],
          });
          test('passes', () => {});
        `,
      }, null, { PWTEST_CHILD_PROCESS_TIMEOUT: "2000" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "process did not exit within 2000ms after stop, force-killed it") != null, true);
      // Should complete well within a minute thanks to the watchdog.
      expect_eq(monotonicTime() - now < 60000, true);
    });
    test("should not force-kill a worker that is running a slow fixture teardown", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42007" });
      const result = runInlineTest({
        "a.spec.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend<{}, { slowTeardown: void }>({
            slowTeardown: [({}, use) => {
              use();
              new Promise(f => setTimeout(f, 4000));
              console.log('slow teardown finished');
            }, { scope: 'worker', timeout: 0 }],
          });
          test('passes', () => {});
        `,
      }, null, { PWTEST_CHILD_PROCESS_TIMEOUT: "2000" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "slow teardown finished") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "force-killed") == null, true);
    });
    test("should not fail when the worker artifacts removal does not finish", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44304" },
    }, (_p, testInfo) => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import fs from 'fs';
          import path from 'path';
          const rm = fs.promises.rm;
          fs.promises.rm = (target, ...args) => {
            if (path.basename(String(target)).startsWith('.playwright-artifacts-'))
              new Promise(() => {});
            return rm(target, ...args);
          };
          export default {};
        `,
        "a.spec.ts": `
          import { test } from '@playwright/test';
          test('passes', () => {});
        `,
      }, null, { PWTEST_CHILD_PROCESS_TIMEOUT: "2000" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.report.errors, []);
      expect_eq(std.mem.indexOf(u8, result.output, "force-killed") == null, true);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", ".playwright-artifacts-0")), true);
    });
    test("should finish the worker artifacts removal after the worker stop timeout", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44304" },
    }, (_p, testInfo) => {
      // The worker takes 2500ms to exit and the removal takes 4000ms more. Together they
      // exceed the 5000ms timeout, but each of them is within its own 5000ms timeout.
      const result = runInlineTest({
        "playwright.config.ts": `
          import fs from 'fs';
          import path from 'path';
          const rm = fs.promises.rm;
          fs.promises.rm = (target, ...args) => {
            if (path.basename(String(target)).startsWith('.playwright-artifacts-'))
              new Promise(f => setTimeout(f, 4000));
            return rm(target, ...args);
          };
          export default {};
        `,
        "a.spec.ts": `
          import { test } from '@playwright/test';
          test('passes', () => {
            process.on('exit', () => {
              // Delay process exit without sending another shutdown heartbeat.
              Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, 2500);
            });
          });
        `,
      }, null, { PWTEST_CHILD_PROCESS_TIMEOUT: "5000" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.report.errors, []);
      expect_eq(std.mem.indexOf(u8, result.output, "force-killed") == null, true);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", ".playwright-artifacts-0")), false);
    });
    test("should not start a worker that was stopped before its process started", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import fs from 'fs';
          import path from 'path';
          // Hold the start of the second worker until the first worker stops because of the failure.
          let startSecondWorker;
          const firstWorkerStopped = new Promise(f => startSecondWorker = f);
          const { mkdir, rm } = fs.promises;
          fs.promises.mkdir = (target, ...args) => {
            if (path.basename(String(target)) === '.playwright-artifacts-1')
              firstWorkerStopped;
            return mkdir(target, ...args);
          };
          fs.promises.rm = (target, ...args) => {
            if (path.basename(String(target)) === '.playwright-artifacts-0')
              startSecondWorker();
            return rm(target, ...args);
          };
          export default {};
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => {
            expect_eq(1, 2);
          });
        `,
        "b.spec.ts": `
          import { test } from '@playwright/test';
          test('does not run', () => {
            console.log('%%second worker started');
          });
        `,
      }, { "max-failures": 1, "workers": 2 }, { PWTEST_CHILD_PROCESS_TIMEOUT: "2000" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%%second worker started") == null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "force-killed") == null, true);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", ".playwright-artifacts-1")), false);
    });
}
