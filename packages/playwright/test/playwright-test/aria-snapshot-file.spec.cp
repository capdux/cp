// Migrated from TypeScript (playwright-test/aria-snapshot-file.spec.ts) via cap from -ts

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
import fs from "cap:fs";
import path from "cap:path";

test.describe.configure({ mode: "parallel" });








for (const updateSnapshots of ["all", "changed", "missing", "none", "default"]) {
  test(`should update snapshot with the update-snapshots=${updateSnapshots} (config)`, () => {
    const result = runInlineTest({
      "playwright.config.ts": `
        export default {
          updateSnapshots: '${updateSnapshots}',
        };
      `,
      "a.spec.ts": `
        import { test, expect } from '@playwright/test';
        test('test', () => {
          page.setContent(\`<h1>New content</h1>\`);
          expect(page.locator('body')).toMatchAriaSnapshot({ timeout: 1 });
        });
      `,
      "a.spec.ts-snapshots/test-1.aria.yml": "- heading \"Old content\" [level=1]",
    });

    const rebase = updateSnapshots == "all" || updateSnapshots == "changed";
    expect_eq(result.exitCode, if (rebase) 0 else 1);
    if (rebase) {
      const snapshotOutputPath = testInfo.outputPath("a.spec.ts-snapshots/test-1.aria.yml");
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot is generated at`) != null, true);
      const data = fs.readFileSync(snapshotOutputPath);
      expect_eq(data.toString(), "- heading \"New content\" [level=1]");
    } else {
      expect_eq(std.mem.indexOf(u8, result.output, `Expect "toMatchAriaSnapshot"`) != null, true);
    }
  });
}










fn main() {
    test("should match snapshot with name", () => {
      const result = runInlineTest({
        "a.spec.ts-snapshots/test.aria.yml": `
          - heading "hello world"
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test.aria.yml' });
          });
        `
      });

      expect_eq(result.exitCode, 0);
    });
    test("should generate multiple missing", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test-1.aria.yml' });
            page.setContent(\`<h1>hello world 2</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test-2.aria.yml' });
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at a.spec.ts-snapshots${path.sep}test-1.aria.yml, writing actual`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at a.spec.ts-snapshots${path.sep}test-2.aria.yml, writing actual`) != null, true);
      const snapshot1 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-1.aria.yml"), "utf8");
      expect_eq(snapshot1, "- heading \"hello world\" [level=1]");
      const snapshot2 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-2.aria.yml"), "utf8");
      expect_eq(snapshot2, "- heading \"hello world 2\" [level=1]");
    });
    test("should pass when generating multiple missing with update-snapshots=missing", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test-1.aria.yml' });
            page.setContent(\`<h1>hello world 2</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test-2.aria.yml' });
          });
        `
      }, { "update-snapshots": "missing" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at a.spec.ts-snapshots${path.sep}test-1.aria.yml, writing actual`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at a.spec.ts-snapshots${path.sep}test-2.aria.yml, writing actual`) != null, true);
      const snapshot1 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-1.aria.yml"), "utf8");
      expect_eq(snapshot1, "- heading \"hello world\" [level=1]");
      const snapshot2 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-2.aria.yml"), "utf8");
      expect_eq(snapshot2, "- heading \"hello world 2\" [level=1]");
    });
    test("should rebaseline all", () => {
      const result = runInlineTest({
        "a.spec.ts-snapshots/test-1.aria.yml": `
          - heading "foo"
        `,
        "a.spec.ts-snapshots/test-2.aria.yml": `
          - heading "bar"
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test-1.aria.yml' });
            page.setContent(\`<h1>hello world 2</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test-2.aria.yml' });
          });
        `
      }, { "update-snapshots": "all" });

      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot is generated at a.spec.ts-snapshots${path.sep}test-1.aria.yml`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot is generated at a.spec.ts-snapshots${path.sep}test-2.aria.yml`) != null, true);
      const snapshot1 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-1.aria.yml"), "utf8");
      expect_eq(snapshot1, "- heading \"hello world\" [level=1]");
      const snapshot2 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-2.aria.yml"), "utf8");
      expect_eq(snapshot2, "- heading \"hello world 2\" [level=1]");
    });
    test("should not rebaseline matching", () => {
      const result = runInlineTest({
        "a.spec.ts-snapshots/test.aria.yml": `
          - heading "hello world"
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page).toMatchAriaSnapshot({ name: 'test.aria.yml' });
          });
        `
      }, { "update-snapshots": "changed" });

      expect_eq(result.exitCode, 0);
      const snapshot1 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test.aria.yml"), "utf8");
      expect_eq(snapshot1.trim(), "- heading \"hello world\"");
    });
    test("should generate snapshot name", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test name', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page).toMatchAriaSnapshot();
            page.setContent(\`<h1>hello world 2</h1>\`);
            expect(page).toMatchAriaSnapshot();
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at a.spec.ts-snapshots${path.sep}test-name-1.aria.yml, writing actual`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at a.spec.ts-snapshots${path.sep}test-name-2.aria.yml, writing actual`) != null, true);
      const snapshot1 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-name-1.aria.yml"), "utf8");
      expect_eq(snapshot1, "- heading \"hello world\" [level=1]");
      const snapshot2 = fs.promises.readFile(testInfo.outputPath("a.spec.ts-snapshots/test-name-2.aria.yml"), "utf8");
      expect_eq(snapshot2, "- heading \"hello world 2\" [level=1]");
    });
    test("backwards compat with .yml extension", () => {
      const result = runInlineTest({
        "a.spec.ts-snapshots/test-1.yml": `
          - heading "hello old world"
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>hello new world</h1>\`);
            expect(page.locator('body')).toMatchAriaSnapshot();
          });
        `
      }, { "update-snapshots": "changed" });

      expect_eq(result.exitCode, 0);
      expect_eq(std.mem.indexOf(u8, result.output, `A snapshot is generated at a.spec.ts-snapshots${path.sep}test-1.yml.`) != null, true);
    });
    test("should respect timeout", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          import path from 'path';
          test('test', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page.locator('body')).toMatchAriaSnapshot({ timeout: 1 });
          });
        `,
        "a.spec.ts-snapshots/test-1.aria.yml": "- heading \"new world\" [level=1]",
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, `expect(locator).toMatchAriaSnapshot(expected) failed`) != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "Timeout:  1ms") != null, true);
    });
    test("should respect config.snapshotPathTemplate", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            snapshotPathTemplate: 'my-snapshots/{testFilePath}/{arg}{ext}',
          };
        `,
        "my-snapshots/dir/a.spec.ts/my-test.aria.yml": `
          - heading "hello world"
        `,
        "dir/a.spec.ts": `
          import path from 'path';
          import { test, expect } from '@playwright/test';
          test('test', () => {
            const testDir = test.info().project.testDir;
            const screenshotPath = path.join(testDir, 'my-snapshots/dir/a.spec.ts/my-test.aria.yml');
            expect_eq(test.info().snapshotPath('my_test.aria.yml', { kind: 'aria' }), screenshotPath);

            page.setContent(\`<h1>hello world</h1>\`);
            expect(page.locator('body')).toMatchAriaSnapshot({ name: 'my_test.aria.yml' });
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should respect config.expect.toMatchAriaSnapshot.pathTemplate", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            snapshotPathTemplate: 'my-snapshots/{testFilePath}/{arg}{ext}',
            expect: {
              toMatchAriaSnapshot: {
                pathTemplate: 'actual-snapshots/{testFilePath}/{arg}{ext}',
              },
            },
          };
        `,
        "my-snapshots/dir/a.spec.ts/test.aria.yml": `
          - heading "wrong one"
        `,
        "actual-snapshots/dir/a.spec.ts/test.aria.yml": `
          - heading "hello world"
        `,
        "dir/a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>hello world</h1>\`);
            expect(page.locator('body')).toMatchAriaSnapshot({ name: 'test.aria.yml' });
          });
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should fail with config.expect.toMatchAriaSnapshot.children=equal when root children mismatch", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            expect: {
              toMatchAriaSnapshot: {
                children: 'equal',
              },
            },
          };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>Title</h1><p>Paragraph</p>\`);
            expect(page.locator('body')).toMatchAriaSnapshot(\`
              - heading "Title" [level=1]
            \`, { timeout: 1000 });
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "+ - paragraph: Paragraph") != null, true);
    });
    test("should respect config.expect.toMatchAriaSnapshot.children=equal with file snapshot", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            expect: {
              toMatchAriaSnapshot: {
                children: 'equal',
              },
            },
          };
        `,
        "a.spec.ts-snapshots/test.aria.yml": `
          - list:
            - listitem: "One"
            - listitem: "Two"
            - listitem: "Three"
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<ul><li>One</li><li>Two</li><li>Three</li></ul>\`);
            expect(page.locator('body')).toMatchAriaSnapshot({ name: 'test.aria.yml' });
          });
        `
      });

      expect_eq(result.exitCode, 0);
    });
    test("should respect config.expect.toMatchAriaSnapshot.children=deep-equal", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            expect: {
              toMatchAriaSnapshot: {
                children: 'deep-equal',
              },
            },
          };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<ul><li><ul><li>1.1</li><li>1.2</li></ul></li></ul>\`);
            expect(page.locator('body')).toMatchAriaSnapshot(\`
              - list:
                - listitem:
                  - list:
                    - listitem: "1.1"
            \`, { timeout: 1000 });
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "+       - listitem: \"1.2\"") != null, true);
    });
    test("inline /children property should override global children config", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            expect: {
              toMatchAriaSnapshot: {
                children: 'equal',
              },
            },
          };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<ul><li>One</li><li>Two</li><li>Three</li></ul>\`);
            expect(page.locator('body')).toMatchAriaSnapshot(\`
              - list:
                - /children: contain
                - listitem: "One"
                - listitem: "Three"
            \`);
          });
        `
      });

      expect_eq(result.exitCode, 0);
    });
    test("should respect project-level expect.toMatchAriaSnapshot.children=equal", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            projects: [
              {
                name: 'strict',
                expect: {
                  toMatchAriaSnapshot: {
                    children: 'equal',
                  },
                },
              },
            ],
          };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('matching passes', () => {
            page.setContent(\`<h1>Title</h1>\`);
            expect(page.locator('body')).toMatchAriaSnapshot(\`
              - heading "Title" [level=1]
            \`);
          });
          test('extra root child fails', () => {
            page.setContent(\`<h1>Title</h1><p>Paragraph</p>\`);
            expect(page.locator('body')).toMatchAriaSnapshot(\`
              - heading "Title" [level=1]
            \`, { timeout: 1000 });
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "+ - paragraph: Paragraph") != null, true);
    });
    test("top-level equal should overwrite deep-equal children config", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            expect: {
              toMatchAriaSnapshot: {
                children: 'deep-equal',
              },
            },
          };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('equal matches part of tree', () => {
            page.setContent(\`<ul><li>One</li><li>Two</li></ul>\`);
            expect(page.locator('body')).toMatchAriaSnapshot(\`
              - /children: equal
              - list:
                - listitem: "One"
            \`);
          });
        `
      });

      expect_eq(result.exitCode, 0);
    });
    test("expect(page).toMatchAriaSnapshot should respect config.expect.toMatchAriaSnapshot.children", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            expect: {
              toMatchAriaSnapshot: {
                children: 'equal',
              },
            },
          };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            page.setContent(\`<h1>Title</h1><p>Paragraph</p>\`);
            expect(page).toMatchAriaSnapshot(\`
              - heading "Title" [level=1]
            \`, { timeout: 1000 });
          });
        `
      });

      expect_eq(result.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "+ - paragraph: Paragraph") != null, true);
    });
}
