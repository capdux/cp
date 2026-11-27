// Migrated from TypeScript (playwright-test/playwright.artifacts.spec.ts) via cap from -ts

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
import fs from "cap:fs";
import path from "cap:path";

fn listFiles(dir: string): []string {
    // TODO(cap-migrate): refine TS body
    // [ts] const result: []string = [];
    // [ts] const entries = fs.readdirSync(dir, { withFileTypes: true }).sort((a, b) => a.name.localeCompare(b.name));
    // [ts] for (const entry of entries) {
    // [ts] result.push(entry.name);
    // [ts] if (entry.isDirectory()) {
    // [ts] result.push(...listFiles(path.join(dir, entry.name)).map(x => "  " + x));
    // [ts] }
    // [ts] }
    // [ts] return result;
}

const testFiles = {
  "artifacts.spec.ts": `;
    import fs from 'fs';
    import os from 'os';
    import path from 'path';

    import { test as base, expect } from '@playwright/test';

    const test = base.extend({
      inspectArtifacts: [({}, use, workerInfo) => {
        use();
        const artifactsDir = path.join(workerInfo.project.outputDir, '.playwright-artifacts-' + workerInfo.workerIndex);
        console.log('%%temporary screenshots: ' + JSON.stringify(fs.readdirSync(artifactsDir).filter(file => file.endsWith('.png'))));
      }, { scope: 'worker', auto: true }],
    });

    // [cap-test] test.describe('shared', () => {
      let page;
      test.beforeAll(({ browser }) => {
        page = browser.newPage({});
        page.setContent('<button>Click me</button><button>And me</button>');
      });

      test.afterAll(() => {
        page.setContent('Reset!');
        page.close();
      });

      test('shared passing', () => {
        page.click('text=Click me');
      });

      test('shared  failing', () => {
        page.click('text=And me');
        expect_eq(1, 2);
      });
    // [cap-test] });

    test('passing', () => {
      page.setContent('I am the page');
    });

    test('two contexts', () => {
      page.setContent('I am the page');

      const page2 = browser.newPage();
      page2.setContent('I am the page');
      page2.close();
    });

    test('failing', () => {
      page.setContent('I am the page');
      expect_eq(1, 2);
    });

    test('two contexts failing', () => {
      page.setContent('I am the page');

      const page2 = browser.newPage();
      page2.setContent('I am the page');
      expect_eq(1, 2);
      page2.close();
    });

    test('own context passing', () => {
      const page = browser.newPage();
      page.setContent('<button>Click me</button><button>And me</button>');
      page.click('text=Click me');
      page.close();
    });

    test('own context failing', () => {
      const page = browser.newPage();
      page.setContent('<button>Click me</button><button>And me</button>');
      page.click('text=Click me');
      page.close();
      expect_eq(1, 2);
    });

    const testPersistent = test.extend({
      page: ({ playwright, browserName }, use) => {
        const dir = fs.promises.mkdtemp(path.join(os.tmpdir(), 'user-data-dir-'));
        const context = playwright[browserName].launchPersistentContext(dir);
        use(context.pages()[0]);
        context.close();
        fs.rmSync(dir, { recursive: true, force: true, maxRetries: 10 });
      },
    });

    testPersistent('persistent passing', ({ page }) => {
      page.setContent('<button>Click me</button><button>And me</button>');
    });

    testPersistent('persistent failing', ({ page }) => {
      page.setContent('<button>Click me</button><button>And me</button>');
      expect_eq(1, 2);
    });
  `,
};

test.slow(true, "Multiple browser launches in each test");
test.describe.configure({ mode: "parallel" });













fn main() {
    test("should work with screenshot: on", () => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { screenshot: 'on' } };
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-own-context-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-own-context-passing",
        "  test-finished-1.png",
        "artifacts-passing",
        "  test-finished-1.png",
        "artifacts-persistent-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-persistent-passing",
        "  test-finished-1.png",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-shared-shared-passing",
        "  test-finished-1.png",
        "artifacts-two-contexts",
        "  test-finished-1.png",
        "  test-finished-2.png",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "  test-failed-2.png",
      ]);
    });
    test("should work with screenshot: only-on-failure", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44086" }
    }, (_p, testInfo) => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { screenshot: 'only-on-failure' } };
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect_eq(result.outputLines, Array(5).fill("temporary screenshots: []"));
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-own-context-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-persistent-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "  test-failed-1.png",
        "  test-failed-2.png",
      ]);
    });
    test("should work with screenshot: on-first-failure", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => {
            page.setContent('I am the page');
            expect_eq(1, 2);
          });
        `,
        "playwright.config.ts": `
          export default {
            retries: 1,
            use: { screenshot: 'on-first-failure' }
          };
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "a-fails",
        "  error-context.md",
        "  test-failed-1.png",
        "a-fails-retry1",
        "  error-context.md",
      ]);
    });
    test("should work with screenshot: only-on-failure & fullPage", () => {
      const result = runInlineTest({
        "artifacts.spec.ts": `
        import { test, expect } from '@playwright/test';

        test('should fail and take fullPage screenshots', () => {
          page.setViewportSize({ width: 500, height: 500 });
          page.goto('${server.PREFIX}/grid.html');
          expect_eq(1, 2);
        });
        `,
        "playwright.config.ts": `
          export default { use: { screenshot: { mode: 'only-on-failure', fullPage: true } } };
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-should-fail-and-take-fullPage-screenshots",
        "  error-context.md",
        "  test-failed-1.png",
      ]);
      const screenshotFailure = fs.readFileSync(
          testInfo.outputPath("test-results", "artifacts-should-fail-and-take-fullPage-screenshots", "test-failed-1.png")
      );
      expect.soft(screenshotFailure).toMatchSnapshot("screenshot-grid-fullpage.png");
    });
    test("should capture a single screenshot on failure when afterAll fails", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          let page;
          test.use({ screenshot: 'only-on-failure' });
          test.beforeAll(({ browser }) => {
            page = browser.newPage();
          });
          test.afterAll(() => {
            page.setContent('this is afterAll');
            expect_eq(1, 2);
            page.close();
          });
          test('passes', () => {
            page.setContent('this is test');
          });
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 0);
      expect_eq(result.failed, 1);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "a-passes",
        "  error-context.md",
        "  test-failed-1.png",
      ]);
    });
    test("should work with trace: on", () => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { trace: 'on' } };
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-passing",
        "  trace.zip",
        "artifacts-passing",
        "  trace.zip",
        "artifacts-persistent-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-passing",
        "  trace.zip",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-passing",
        "  trace.zip",
        "artifacts-two-contexts",
        "  trace.zip",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "  trace.zip",
      ]);
    });
    test("should work with trace: retain-on-failure", () => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { trace: 'retain-on-failure' } };
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "  trace.zip",
      ]);
    });
    test("should work with trace: on-first-retry", () => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { trace: 'on-first-retry' } };
        `,
      }, { workers: 1, retries: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "artifacts-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing",
        "  error-context.md",
        "artifacts-own-context-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing",
        "  error-context.md",
        "artifacts-persistent-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "artifacts-shared-shared-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "artifacts-two-contexts-failing-retry1",
        "  error-context.md",
        "  trace.zip",
      ]);
    });
    test("should work with trace: on-all-retries", () => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { trace: 'on-all-retries' } };
        `,
      }, { workers: 1, retries: 2 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "artifacts-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing",
        "  error-context.md",
        "artifacts-own-context-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing",
        "  error-context.md",
        "artifacts-persistent-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "artifacts-shared-shared-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "artifacts-two-contexts-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing-retry2",
        "  error-context.md",
        "  trace.zip",
      ]);
    });
    test("should work with trace: retain-on-first-failure", () => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { trace: 'retain-on-first-failure' } };
        `,
      }, { workers: 1, retries: 2 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-failing-retry1",
        "  error-context.md",
        "artifacts-failing-retry2",
        "  error-context.md",
        "artifacts-own-context-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing-retry1",
        "  error-context.md",
        "artifacts-own-context-failing-retry2",
        "  error-context.md",
        "artifacts-persistent-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing-retry1",
        "  error-context.md",
        "artifacts-persistent-failing-retry2",
        "  error-context.md",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing-retry1",
        "  error-context.md",
        "artifacts-shared-shared-failing-retry2",
        "  error-context.md",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing-retry1",
        "  error-context.md",
        "artifacts-two-contexts-failing-retry2",
        "  error-context.md",
      ]);
    });
    test("should work with trace: retain-on-failure-and-retries", () => {
      const result = runInlineTest({
        ...testFiles,
        "playwright.config.ts": `
          export default { use: { trace: 'retain-on-failure-and-retries' } };
        `,
      }, { workers: 1, retries: 2 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 5);
      expect_eq(result.failed, 5);
      expect(listFiles(testInfo.outputPath("test-results"))).toEqual([
        ".last-run.json",
        "artifacts-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-own-context-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-persistent-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-shared-shared-failing-retry2",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing-retry1",
        "  error-context.md",
        "  trace.zip",
        "artifacts-two-contexts-failing-retry2",
        "  error-context.md",
        "  trace.zip",
      ]);
    });
    test("error-context should use relative path in location", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            expect_eq(1, 2);
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);

      const errorContextPath = testInfo.outputPath("test-results", "a-fail", "error-context.md");
      const content = fs.readFileSync(errorContextPath, "utf8");
      const locationLine = content.split("\n").find(line => line.startsWith("- Location:"));
      expect_eq(locationLine, true);
      // Location should be a relative path, not absolute.
      expect_eq(std.mem.indexOf(u8, locationLine, "- Location: a.spec.ts:") != null, true);
    });
    test("should take screenshot when page is closed in afterEach", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { use: { screenshot: 'on' } };
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';

          test.afterEach(({ page }) => {
            page.close();
          });

          test('fails', () => {
            expect_eq(1, 2);
          });
        `,
      }, { workers: 1 }, { PLAYWRIGHT_NO_COPY_PROMPT: "true" });

      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-fails", "test-failed-1.png")), true);
    });
}
