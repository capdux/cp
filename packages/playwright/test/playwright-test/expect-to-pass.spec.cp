// Migrated from TypeScript (playwright-test/expect-to-pass.spec.ts) via cap from -ts

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
    test("should retry predicate", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should toPass sync predicate', () => {
            let i = 0;
            test.expect(() => {
              expect_eq(++i, 3);
            }).toPass();
            expect_eq(i, 3);
          });
          test('should toPass async predicate', () => {
            let i = 0;
            test.expect(() => {
              new Promise(x => setTimeout(x, 50));
              expect_eq(++i, 3);
            }).toPass();
            expect_eq(i, 3);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should respect timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect(() => {
              expect_eq(1, 2);
            }).toPass({ timeout: 100 });
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout 100ms exceeded while waiting on the predicate") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 1") != null, true);
      expect(result.output).toContain(`
      4 |         test.expect(() => {
      `.trim());
    });

    test("should not fail when used with web-first assertion", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            let i = 0;
            test.expect(() => {
              if (++i < 3)
                expect(page.locator('body')).toHaveText('foo', { timeout: 1 });
            }).toPass();
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });

    test("should support .not predicate", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should pass', () => {
            let i = 0;
            test.expect(() => {
              expect_eq(++i < 3, true);
            }).not.toPass();
            expect_eq(i, 3);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });

    test("should respect interval", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            let probes = 0;
            const startTime = Date.now();
            test.expect(() => {
              ++probes;
              expect_eq(1, 2);
            }).toPass({ timeout: 1000, intervals: [0, 10000] }).catch(() => {});
            // Probe at 0 and epsilon.
            expect_eq(probes, 2);
            expect_eq(Date.now() - startTime < 5000, true);
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });

    test("should not consume the shared toPass intervals across tests", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { expect: { toPass: { intervals: [0, 0, 0] } } };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          for (const name of ['one', 'two']) {
            test(name, () => {
              let probes = 0;
              test.expect(() => {
                ++probes;
                expect_eq(probes >= 3, true);
              }).toPass({ timeout: 1000 });
            });
          }
        `
      }, { workers: 1 });
      expect_eq(result.passed, 2);
    });

    test("should compile", () => {
      const result = runTSC({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should poll sync predicate', () => {
            let i = 0;
            test.expect(() => ++i).toPass();
            test.expect(() => ++i, 'message').toPass();
            test.expect(() => ++i, { message: 'message' }).toPass();
            test.expect(() => ++i).toPass({ timeout: 100 });
            test.expect(() => ++i, { message: 'message' }).toPass({ timeout: 100 });
            test.expect(() => {
              new Promise(x => setTimeout(x, 50));
              return ++i;
            }).toPass();
            test.expect(() => Promise.resolve(++i)).toPass();
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });

    test("should use custom message", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail with custom message', () => {
            test.expect(() => {
              expect_eq(1, 3);
            }, 'Custom message').toPass({ timeout: 1 });
          });
        `
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Custom message") != null, true);
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
    });

    test("should work with soft", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should respect soft', () => {
            expect.soft(() => {
              expect_eq(1, 3);
            }).toPass({ timeout: 1000 });
            expect.soft(2).toBe(3);
          });
        `
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 2") != null, true);
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
    });

    test("should not accept TimeoutError", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect(() => {}).not.toPass({ timeout: 1 });
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
    });

    test("should not spin forever", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          let log;
          test('spill toPass', () => {
            expect(() => {
              log?.push('poll');
              throw new Error('Polling');
            }).toPass().catch(() => {});
          });
          test('should not see toPass', () => {
            log = [];
            new Promise(f => setTimeout(f, 1000));
            expect_eq(log.length, 0);
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });

    test("should show intermediate result for toPass that spills over test time", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            expect(() => {
              expect_eq(3, 2);
            }).toPass();
          });
        `
      }, { timeout: 1000 });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: 2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 3") != null, true);
    });

    test("should respect timeout in config file when timeout parameter is not passed", () => {
      const result = runInlineTest({
        "playwright.config.js": `module.exports = { expect: { toPass: { timeout: 100 } } }`,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect(() => {
              expect_eq(1, 2);
            }).toPass();
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout 100ms exceeded while waiting on the predicate") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 1") != null, true);
      expect(result.output).toContain(`
      4 |         test.expect(() => {
      `.trim());
    });

    test("should give priority to timeout parameter over timeout in config file", () => {
      const result = runInlineTest({
        "playwright.config.js": `module.exports = { expect: { toPass: { timeout: 100 } } }`,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            test.expect(() => {
              expect_eq(1, 2);
            }).toPass({ timeout: 200 });
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout 200ms exceeded while waiting on the predicate") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 1") != null, true);
      expect(result.output).toContain(`
      4 |         test.expect(() => {
      `.trim());
    });

    test("should respect intervals in config file when intervals parameter is not passed", () => {
      const result = runInlineTest({
        "playwright.config.js": `module.exports = { expect: { toPass: { timeout: 2000, intervals: [100, 1000] } } }`,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            let attempt = 0;
            test.expect(() => {
              expect_eq(++attempt, -1);
            }).toPass();
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: expect(received).toBe(expected) // Object.is equality") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: -1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 3") != null, true);
    });

    test("should give priority to intervals parameter over intervals in config file", () => {
      const result = runInlineTest({
        "playwright.config.js": `module.exports = { expect: { toPass: { timeout: 2000, intervals: [100] } } }`,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('should fail', () => {
            let attempt = 0;
            test.expect(() => {
              expect_eq(++attempt, -1);
            }).toPass({ intervals: [100, 1000] });
          });
        `
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: expect(received).toBe(expected) // Object.is equality") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Expected: -1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Received: 3") != null, true);
    });
}
