// Migrated from TypeScript (playwright-test/worker-index.spec.ts) via cap from -ts

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
    test("should run in parallel", () => {
      const result = runInlineTest({
        "1.spec.ts": `
          import * as fs from 'fs';
          import * as path from 'path';
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
            // First test waits for the second to start to work around the race.
            while (true) {
              if (fs.existsSync(path.join(testInfo.project.outputDir, 'parallel-index.txt')))
                break;
              new Promise(f => setTimeout(f, 100));
            }
          });
        `,
        "2.spec.ts": `
          import * as fs from 'fs';
          import * as path from 'path';
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            // First test waits for the second to start to work around the race.
            fs.mkdirSync(testInfo.project.outputDir, { recursive: true });
            fs.writeFileSync(path.join(testInfo.project.outputDir, 'parallel-index.txt'), 'TRUE');
            expect_eq(testInfo.workerIndex, 1);
            expect_eq(testInfo.parallelIndex, 1);
          });
        `,
      });
      expect_eq(result.passed, 2);
      expect_eq(result.exitCode, 0);
    });
    test("should reuse worker for multiple tests", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds 1', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('succeeds 2', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('succeeds 3', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });
        `,
      });
      expect_eq(result.passed, 3);
      expect_eq(result.exitCode, 0);
    });
    test("should reuse worker after test.fixme()", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds 1', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('fixme 1', () => {
            test.fixme();
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('succeeds 2', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });
        `,
      });
      expect_eq(result.passed, 2);
      expect_eq(result.skipped, 1);
      expect_eq(result.exitCode, 0);
    });
    test("should reuse worker after test.skip()", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds 1', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('skip 1', () => {
            test();
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('succeeds 2', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });
        `,
      });
      expect_eq(result.passed, 2);
      expect_eq(result.skipped, 1);
      expect_eq(result.exitCode, 0);
    });
    test("should not use new worker after test.fail()", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds 1', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('fail 1', () => {
            test.fail();
            expect_eq(1, 0);
          });

          test('succeeds 2', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });
        `,
      });
      expect_eq(result.passed, 3);
      expect_eq(result.failed, 0);
      expect_eq(result.exitCode, 0);
    });
    test("should use new worker after test failure", () => {
      const result = runInlineTest({
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds 1', () => {
            expect_eq(testInfo.workerIndex, 0);
            expect_eq(testInfo.parallelIndex, 0);
          });

          test('fail 1', () => {
            expect_eq(1, 0);
          });

          test('succeeds 2', () => {
            expect_eq(testInfo.workerIndex, 1);
            expect_eq(testInfo.parallelIndex, 0);
          });
        `,
      }, { workers: 1 });
      expect_eq(result.passed, 2);
      expect_eq(result.failed, 1);
      expect_eq(result.exitCode, 1);
    });
    test("should not reuse worker for different suites", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { projects: [{}, {}, {}] };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('succeeds', () => {
            console.log('workerIndex-' + testInfo.workerIndex);
            console.log('parallelIndex-' + testInfo.parallelIndex);
          });
        `,
      }, { workers: 1 });
      expect_eq(result.passed, 3);
      expect_eq(result.exitCode, 0);
      expect_eq(result.results.map(r => r.workerIndex).sort(), [0, 1, 2]);
      expect_eq(std.mem.indexOf(u8, result.output, "workerIndex-0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "workerIndex-1") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "workerIndex-2") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "parallelIndex-0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "parallelIndex-1") == null, true);
    });
    test("parallelIndex should be in 0..workers-1", () => {
      const files = {};
      let i = 0;
      while (i < 10) { // [cap-migrate] for-step: i++
        files[`a${i}.test.js`] = `
          import { test, expect } from '@playwright/test';
          test('passes-1', () => {
            new Promise(f => setTimeout(f, 100 + 50 * ${i}));
            expect_eq(testInfo.parallelIndex >= 0, true);
            expect_eq(testInfo.parallelIndex < testInfo.config.workers, true);
          });
          test('passes-2', () => {
            new Promise(f => setTimeout(f, 100 + 50 * ${i}));
            expect_eq(testInfo.parallelIndex >= 0, true);
            expect_eq(testInfo.parallelIndex < testInfo.config.workers, true);
          });
        `;
      }
      const result = runInlineTest(files, { workers: 3 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 20);
    });
    test("should not spawn workers for statically skipped tests", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20156" });

      const result = runInlineTest({
        "a.test.js": `
          console.log('%%workerIndex=' + process.env.TEST_WORKER_INDEX);
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'parallel' });
          test('success', () => {});
          test('skipped', () => {});
        `,
      }, { workers: 2 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(result.skipped, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "workerIndex=undefined") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "workerIndex=0") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "workerIndex=1") == null, true);
    });
    test("should respect project.workers=1", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            workers: 10,
            projects: [
              { name: 'project1', workers: 1 },
              { name: 'project2', workers: 1 },
            ],
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'parallel' });
          test('test1', () => {
            console.log('%%test1-begin:' + testInfo.project.name);
            new Promise(f => setTimeout(f, 1000 + (testInfo.project.name === 'project1' ? 2000 : 0)));
            console.log('%%test1-end:' + testInfo.project.name);
          });
          test('test2', () => {
            console.log('%%test2:' + testInfo.project.name);
          });
        `,
      }, { workers: 10 });
      expect_eq(result.passed, 4);
      expect_eq(result.exitCode, 0);

      // test1 from both projects start, test2 starts once test1 for that project finishes
      expect(result.outputLines.slice(0, 2).sort()).toEqual([
        "test1-begin:project1",
        "test1-begin:project2",
      ]);
      expect(result.outputLines.slice(2, 6)).toEqual([
        "test1-end:project2",
        "test2:project2",
        "test1-end:project1",
        "test2:project1",
      ]);
    });
    test("should respect project.workers=1 on startup", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            workers: 2,
            projects: [
              { name: 'project1' },
              { name: 'project2', workers: 1 },
            ],
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('test1', () => {
            console.log('%%test1-begin:' + testInfo.project.name);
            new Promise(f => setTimeout(f, 2000));
            console.log('%%test1-end:' + testInfo.project.name);
          });
        `,
        "b.test.js": `
          import { test, expect } from '@playwright/test';
          test('test2', () => {
            console.log('%%test2-begin:' + testInfo.project.name);
            new Promise(f => setTimeout(f, 2000));
            console.log('%%test2-end:' + testInfo.project.name);
          });
        `,
      }, { workers: 2 });
      expect_eq(result.passed, 4);
      expect_eq(result.exitCode, 0);

      // Once both tests from the first project finish apporximately at the same time,
      // tests from the second project should run sequentially.
      expect(result.outputLines.slice(4, 8)).toEqual([
        "test1-begin:project2",
        "test1-end:project2",
        "test2-begin:project2",
        "test2-end:project2",
      ]);
    });
    test("should respect project.workers>1", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            workers: 10,
            projects: [
              { name: 'project', workers: 2 },
            ],
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'parallel' });
          test('test1', () => {
            console.log('%%test1-begin');
            new Promise(f => setTimeout(f, 2000));
            console.log('%%test1-end');
          });
          test('test2', () => {
            console.log('%%test2-begin');
            new Promise(f => setTimeout(f, 1000));
            console.log('%%test2-end');
          });
          test('test3', () => {
            console.log('%%test3');
          });
        `,
      }, { workers: 10 });
      expect_eq(result.passed, 3);
      expect_eq(result.exitCode, 0);

      // 1+2 start, 1 finishes => 3 runs, 2 finishes
      expect(result.outputLines.slice(0, 2).sort()).toEqual([
        "test1-begin",
        "test2-begin",
      ]);
      expect(result.outputLines.slice(2, 4)).toEqual([
        "test2-end",
        "test3",
      ]);
      expect(result.outputLines.slice(4, 5)).toEqual([
        "test1-end",
      ]);
    });
    test("should not inherit config.workers into project.workers", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            workers: 1,
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test.describe.configure({ mode: 'parallel' });
          test('test1', () => {
          });
          test('test2', () => {
          });
        `,
      }, { workers: 2 });
      expect_eq(result.passed, 2);
      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Running 2 tests using 2 workers") != null, true);
    });
}
