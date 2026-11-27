// Migrated from TypeScript (playwright-test/deps.spec.ts) via cap from -ts

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
    test("should run projects with dependencies", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A' },
              { name: 'B', dependencies: ['A'] },
              { name: 'C', dependencies: ['A'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 }, null, { additionalArgs: ["--project=B", "--project=C"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.outputLines, ["A", "B", "C"]);
    });
    test("should inherit env changes from dependencies", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'A', testMatch: '**/a.spec.ts' },
            { name: 'B', testMatch: '**/b.spec.ts', teardown: 'E' },
            { name: 'C', testMatch: '**/c.spec.ts', dependencies: ['A'] },
            { name: 'D', testMatch: '**/d.spec.ts', dependencies: ['B'] },
            { name: 'E', testMatch: '**/e.spec.ts' },
          ] };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            process.env.SET_IN_A = 'valuea';
            delete process.env.SET_OUTSIDE;
            console.log('\\n%%A');
          });
        `,
        "b.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            process.env.SET_IN_B = 'valueb';
            console.log('\\n%%B');
          });
        `,
        "c.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('\\n%%C-' + process.env.SET_IN_A + '-' + process.env.SET_IN_B + '-' + process.env.SET_OUTSIDE);
          });
        `,
        "d.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('\\n%%D-' + process.env.SET_IN_A + '-' + process.env.SET_IN_B + '-' + process.env.SET_OUTSIDE);
          });
        `,
        "e.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('\\n%%E-' + process.env.SET_IN_A + '-' + process.env.SET_IN_B + '-' + process.env.SET_OUTSIDE);
          });
        `,
      }, {}, { SET_OUTSIDE: "outside" });
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expect_eq(result.outputLines.sort(), ["A", "B", "C-valuea-undefined-undefined", "D-undefined-valueb-outside", "E-undefined-valueb-outside"]);
    });
    test("should not run projects with dependencies when --no-deps is passed", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A' },
              { name: 'B', dependencies: ['A'] },
              { name: 'C', dependencies: ['A'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 }, null, { additionalArgs: ["--no-deps", "--project=B", "--project=C"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(result.outputLines, ["B", "C"]);
    });
    test("should not run project if dependency failed", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A' },
              { name: 'B', dependencies: ['A'] },
              { name: 'C', dependencies: ['B'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
            if (testInfo.project.name === 'B')
              throw new Error('Failed project B');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Failed project B") != null, true);
      expect_eq(result.outputLines, ["A", "B"]);
    });
    test("should not run project if dependency failed (2)", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A1' },
              { name: 'A2', dependencies: ['A1'] },
              { name: 'A3', dependencies: ['A2'] },
              { name: 'B1' },
              { name: 'B2', dependencies: ['B1'] },
              { name: 'B3', dependencies: ['B2'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
            if (testInfo.project.name === 'B1')
              throw new Error('Failed project B1');
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.outputLines.sort(), ["A1", "A2", "A3", "B1"]);
    });
    test("should filter by project list, but run deps", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'A' },
            { name: 'B' },
            { name: 'C', dependencies: ['A'] },
            { name: 'D' },
          ] };
        `,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `
      }, { project: ["C", "D"] });
      expect_eq(result.passed, 3);
      expect_eq(result.failed, 0);
      expect_eq(result.skipped, 0);
      expect_eq(result.outputLines.sort(), ["A", "C", "D"]);
    });
    test("should not filter dependency by file name", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'A' },
            { name: 'B', dependencies: ['A'] },
          ] };
        `,
        "one.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
        "two.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => { });
        `,
      }, null, null, { additionalArgs: ["two.spec.ts"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "1) [A] Ã¢ÂÂº one.spec.ts:3:7 Ã¢ÂÂº fails") != null, true);
    });
    test("should filter dependency by only", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'setup', testMatch: /setup.ts/ },
            { name: 'browser', dependencies: ['setup'] },
          ] };
        `,
        "setup.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {
            console.log('\\n%% setup in ' + test.info().project.name);
          });
          test('passes 2', () => {
            console.log('\\n%% setup 2 in ' + test.info().project.name);
          });
        `,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('\\n%% test in ' + test.info().project.name);
          });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.outputLines, ["setup 2 in setup"]);
    });
    test("should filter dependency by only when running explicitly", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'setup', testMatch: /setup.ts/ },
            { name: 'browser', dependencies: ['setup'] },
          ] };
        `,
        "setup.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {
            console.log('\\n%% setup in ' + test.info().project.name);
          });
          test('passes 2', () => {
            console.log('\\n%% setup 2 in ' + test.info().project.name);
          });
        `,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('\\n%% test in ' + test.info().project.name);
          });
        `,
      }, { project: ["setup"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.outputLines, ["setup 2 in setup"]);
    });
    test("should not filter dependency by only 3", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'setup', testMatch: /setup.*.ts/ },
            { name: 'browser', dependencies: ['setup'] },
          ] };
        `,
        "setup-1.ts": `
          import { test, expect } from '@playwright/test';
          test('setup 1', () => {
            console.log('\\n%% setup in ' + test.info().project.name);
          });
        `,
        "setup-2.ts": `
          import { test, expect } from '@playwright/test';
          test('setup 2', () => {
            console.log('\\n%% setup 2 in ' + test.info().project.name);
          });
        `,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            console.log('\\n%% test in ' + test.info().project.name);
          });
        `,
      }, null, null, { additionalArgs: ["setup-2.ts"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.outputLines, ["setup 2 in setup"]);
    });
    test("should not report skipped dependent tests", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'setup', testMatch: /setup.ts/ },
            { name: 'browser', dependencies: ['setup'] },
          ] };
        `,
        "setup.ts": `
          import { test, expect } from '@playwright/test';
          test('setup', () => {
            expect_eq(1, 2);
          });
        `,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.didNotRun, 1);
      expect_eq(result.results.length, 1);
    });
    test("should report circular dependencies", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [
            { name: 'A', dependencies: ['B'] },
            { name: 'B', dependencies: ['A'] },
          ] };
        `,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Circular dependency detected between projects.") != null, true);
    });
    test("should run dependency in each shard", () => {
      const files = {
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'setup', testMatch: /setup.ts/ },
              { name: 'chromium', dependencies: ['setup'] },
            ],
          };
        `,
        "setup.ts": `
          import { test, expect } from '@playwright/test';
          test('setup', () => {
            console.log('\\n%%setup');
          });
        `,
        "test1.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {
            console.log('\\n%%test1');
          });
        `,
        "test2.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test2', () => {
            console.log('\\n%%test2');
          });
        `,
      };
      {
        const result = runInlineTest(files, { workers: 1, shard: "1/2" });
        expect_eq(result.exitCode, 0);
        expect_eq(result.passed, 2);
        expect_eq(result.outputLines, ["setup", "test1"]);
      }
      {
        const result = runInlineTest(files, { workers: 1, shard: "2/2" });
        expect_eq(result.exitCode, 0);
        expect_eq(result.passed, 2);
        expect_eq(result.outputLines, ["setup", "test2"]);
      }
    });
    test("should run setup project with zero tests", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'setup', testMatch: /not-matching/ },
              { name: 'real', dependencies: ['setup'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.outputLines, ["real"]);
    });
    test("should run setup project with zero tests recursively", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A', testMatch: /a.spec/ },
              { name: 'B', testMatch: /not-matching/, dependencies: ['A'] },
              { name: 'C', testMatch: /c.spec/, dependencies: ['B'] },
            ],
          };`,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
        "c.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1, project: "C" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(result.outputLines, ["A", "C"]);
    });
    test("should run project with teardown", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A', teardown: 'B' },
              { name: 'B' },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 }, null, { additionalArgs: ["--project=A"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(result.outputLines, ["A", "B"]);
    });
    test("should run non-default dependency and teardown", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'setup', teardown: 'teardown', default: false },
              { name: 'teardown', default: false },
              { name: 'A', dependencies: ['setup'] },
              { name: 'B', default: false },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.outputLines, ["setup", "A", "teardown"]);
    });
    test("should run teardown after dependents", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A', teardown: 'E' },
              { name: 'B', dependencies: ['A'] },
              { name: 'C', dependencies: ['B'], teardown: 'D' },
              { name: 'D' },
              { name: 'E' },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 }, null, { additionalArgs: ["--project=C"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 5);
      expect_eq(result.outputLines, ["A", "B", "C", "D", "E"]);
    });
    test("should run teardown after failure", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A', teardown: 'D' },
              { name: 'B', dependencies: ['A'] },
              { name: 'C', dependencies: ['B'] },
              { name: 'D' },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
            if (testInfo.project.name === 'A')
              throw new Error('ouch');
          });
        `,
      }, { workers: 1 }, null, { additionalArgs: ["--project=C"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 1);
      expect_eq(result.didNotRun, 2);
      expect_eq(result.outputLines, ["A", "D"]);
    });
    test("should complain about teardown being a dependency", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A', teardown: 'B' },
              { name: 'B' },
              { name: 'C', dependencies: ['B'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `Project C must not depend on a teardown project B`) != null, true);
    });
    test("should complain about teardown having a dependency", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A', teardown: 'B' },
              { name: 'B', dependencies: ['C'] },
              { name: 'C' },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `Teardown project B must not have dependencies`) != null, true);
    });
    test("should support the same teardown used multiple times", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A', teardown: 'D' },
              { name: 'B', teardown: 'D' },
              { name: 'D' },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.outputLines, ["A", "B", "D"]);
    });
    test("should only apply --repeat-each to top-level", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'A' },
              { name: 'B', dependencies: ['A'] },
              { name: 'C', dependencies: ['A'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { "workers": 1, "repeat-each": 2 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 5);
      expect_eq(result.outputLines, ["A", "B", "B", "C", "C"]);
    });
    test("should run teardown when all projects are top-level at run point", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'setup', teardown: 'teardown' },
              { name: 'teardown' },
              { name: 'project', dependencies: ['setup'] },
            ],
          };`,
        "test.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 }, null, { additionalArgs: ["test.spec.ts"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.outputLines, ["setup", "project", "teardown"]);
    });
    test("should not run deps for projects filtered with grep", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'setupA', teardown: 'teardownA', testMatch: '**/hook.spec.ts' },
              { name: 'teardownA', testMatch: '**/hook.spec.ts' },
              { name: 'projectA', dependencies: ['setupA'], testMatch: '**/a.spec.ts' },
              { name: 'setupB', teardown: 'teardownB', testMatch: '**/hook.spec.ts' },
              { name: 'teardownB', testMatch: '**/hook.spec.ts' },
              { name: 'projectB', dependencies: ['setupB'], testMatch: '**/b.spec.ts' },
            ],
          };`,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
        "hook.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
        "b.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            console.log('\\n%%' + testInfo.project.name);
          });
        `,
      }, { workers: 1 }, null, { additionalArgs: ["--grep=b.spec.ts"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.outputLines, ["setupB", "projectB", "teardownB"]);
    });
    test("should allow only in dependent", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'setup', testMatch: '**/setup.ts' },
              { name: 'project', dependencies: ['setup'] },
            ],
          };`,
        "setup.ts": `
          import { test, expect } from '@playwright/test';
          test('setup', () => {});
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
          });
          test('test 2', () => { expect_eq(1, 2); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should allow only in dependent (2)", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              { name: 'setup', testMatch: '**/setup.ts' },
              { name: 'project', dependencies: ['setup'] },
            ],
          };`,
        "setup.ts": `
          import { test, expect } from '@playwright/test';
          test('setup', () => {});
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => { expect_eq(1, 2); });
          test('test 2', () => { expect_eq(1, 2); });
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
}
