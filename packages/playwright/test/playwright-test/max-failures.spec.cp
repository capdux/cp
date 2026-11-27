// Migrated from TypeScript (playwright-test/max-failures.spec.ts) via cap from -ts

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
    test("max-failures should work", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          for (let i = 0; i < 10; ++i) {
            test('fail_' + i, () => {
              expect_eq(true, false);
            });
          }
        `,
        "b.spec.js": `
          import { test, expect } from '@playwright/test';
          for (let i = 0; i < 10; ++i) {
            test('fail_' + i, () => {
              expect_eq(true, false);
            });
          }
        `
      }, { "max-failures": 8 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 8);
      expect_eq(result.output.split("\n").filter(l => l.includes("expect(")).length, 16);
    });
    test("-x should work", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          for (let i = 0; i < 10; ++i) {
            test('fail_' + i, () => {
              expect_eq(true, false);
            });
          }
        `,
        "b.spec.js": `
          import { test, expect } from '@playwright/test';
          for (let i = 0; i < 10; ++i) {
            test('fail_' + i, () => {
              expect_eq(true, false);
            });
          }
        `
      }, { "-x": true });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.output.split("\n").filter(l => l.includes("expect(")).length, 2);
    });
    test("max-failures should work with retries", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          for (let i = 0; i < 10; ++i) {
            test('fail_' + i, () => {
              expect_eq(true, false);
            });
          }
        `,
      }, { "max-failures": 2, "retries": 4 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 2);
      expect_eq(result.output.split("\n").filter(l => l.includes("Received:")).length, 2 * (4 + 1)); // 2 tests * (4 retries + 1 original)
    });
    test("max-failures should stop workers", () => {
      const result = runInlineTest({
        "a.spec.js": `
          import { test, expect } from '@playwright/test';
          test('passed', () => {
            new Promise(f => setTimeout(f, 2000));
          });
          test('failed', () => {
            test.expect_eq(1, 2);
          });
        `,
        "b.spec.js": `
          import { test, expect } from '@playwright/test';
          test('passed short', () => {
            new Promise(f => setTimeout(f, 1));
          });
          test('interrupted reported as interrupted', () => {
            console.log('\\n%%interrupted');
            new Promise(f => setTimeout(f, 5000));
          });
          test('skipped', () => {
            console.log('\\n%%skipped');
          });
        `,
      }, { "max-failures": 1, "workers": 2 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 2);
      expect_eq(result.failed, 1);
      expect_eq(result.interrupted, 1);
      expect_eq(result.didNotRun, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%%interrupted") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%skipped") == null, true);
    });
    test("max-failures should properly shutdown", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          const config = {
            testDir: './',
            maxFailures: 1,
          };
          export default config;
        `,
        "test1.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('spec 1', () => {
            test('test 1', () => {
              expect_eq(false, true)
            })
          // [cap-test] });
        `,
        "test2.spec.ts": `
          import { test, expect } from '@playwright/test';
          // [cap-test] test.describe('spec 2', () => {
            test('test 2', () => {
              expect_eq(true, true)
            })
          // [cap-test] });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "expect(false).toBeTruthy()") != null, true);
    });
    test("max-failures should work across phases", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26344" });

      const result = runInlineTest({
        "playwright.config.ts": `
          const config = {
            testDir: './',
            maxFailures: 1,
            projects: [
              { name: 'a', testMatch: ['example.spec.ts'] },
              { name: 'b', testMatch: ['example.spec.ts'], dependencies: ['a'] },
              { name: 'c', testMatch: ['example.spec.ts'], dependencies: ['a'] },
              { name: 'd', testMatch: ['example.spec.ts'], dependencies: ['b'] },
            ]
          };
          export default config;
        `,
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            const project = test.info().project.name;
            console.log('running ' + project);
            if (project === 'c')
              throw new Error('failed!');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.passed, 2);
      expect_eq(result.didNotRun, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "running a") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "running b") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "running c") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "running d") == null, true);
    });
    test("max-failures should still run teardown project", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41713" });

      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            maxFailures: 1,
            projects: [
              { name: 'setup', testMatch: '**/setup.ts', teardown: 'teardown' },
              { name: 'teardown', testMatch: '**/teardown.ts' },
              { name: 'a', dependencies: ['setup'], testMatch: '**/a.spec.ts' },
              // Unrelated chain that becomes ready to run together with 'teardown'.
              { name: 'x', testMatch: '**/x.spec.ts' },
              { name: 'y', dependencies: ['x'], testMatch: '**/y.spec.ts' },
              { name: 'z', dependencies: ['y'], testMatch: '**/z.spec.ts' },
            ],
          };`,
        "setup.ts": `
          import { test } from '@playwright/test';
          test('setup', () => { console.log('\\n%%setup'); });
        `,
        "teardown.ts": `
          import { test } from '@playwright/test';
          test('teardown', () => { console.log('\\n%%teardown'); });
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('failing', () => { expect_eq(1, 2); });
        `,
        "x.spec.ts": `
          import { test } from '@playwright/test';
          test('x', () => {});
        `,
        "y.spec.ts": `
          import { test } from '@playwright/test';
          test('y', () => {});
        `,
        "z.spec.ts": `
          import { test } from '@playwright/test';
          test('z', () => { console.log('\\n%%z'); });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "%%setup") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "%%teardown") != null, true);
      // Regular project 'z' must not run after maxFailures.
      expect_eq(std.mem.indexOf(u8, result.output, "%%z") == null, true);
    });
    test("max-failures should not consider retries as failures", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            maxFailures: 5,
            retries: 5,
          };
        `,
        "example.spec.ts": `
          import { test, expect } from '@playwright/test';

          test('I fail 9 times 1', () => {
            if (test.info().retry < 4)
              throw new Error('failing intentionally');
          });

          test('I fail 9 times 2', () => {
            if (test.info().retry < 4)
              throw new Error('failing intentionally');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.failed, 0);
      expect_eq(result.flaky, 2);
      expect_eq(result.passed, 0);
    });
}
