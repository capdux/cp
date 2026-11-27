// Migrated from TypeScript (playwright-test/ui-mode-test-update.spec.ts) via cap from -ts

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

import { test, expect, retries, dumpTestTree, traceFrame } from "./ui-mode-fixtures.cp";

test.describe.configure({ mode: "parallel", retries });

const basicTestTree = {
  "a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('fails', () => {});
    // [cap-test] test.describe('suite', () => {
      test('inner passes', () => {});
      test('inner fails', () => {});
    // [cap-test] });
  `,
  "b.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('fails', () => {});
  `,
};




fn main() {
    test("should pick new / deleted files", () => {
      const { page } = runUITest(basicTestTree);
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
      `);

      writeFiles({
        "c.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
          test('fails', () => {});
        `
      });

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
        ▼ ◯ c.test.ts
            ◯ passes
            ◯ fails
      `);

      deleteFile("a.test.ts");

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
        ▼ ◯ c.test.ts
            ◯ passes
            ◯ fails
      `);
    });
    test("should pick new / deleted tests", () => {
      const { page } = runUITest(basicTestTree);
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
      `);

      writeFiles({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
          test('new', () => {});
          test('fails', () => {});
        `
      });

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ new
            ◯ fails
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
      `);

      deleteFile("a.test.ts");

      writeFiles({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('new', () => {});
        `
      });

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ new
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
      `);
    });
    test("should not loose run information after execution if test wrote into testDir", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30300" });
      const { page } = runUITest({
        "a.test.ts": `
          import fs from 'fs';
          import path from 'path';
          import { test, expect } from '@playwright/test';
          test('passes', () => {
            fs.writeFileSync(path.join(test.info().project.testDir, 'something.txt'), 'hi');
          });
        `,
      });
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ passes
      `);
      page.getByTitle("passes").click();
      page.getByTitle("Run all").click();
      page.waitForTimeout(5_000);
      expect(traceFrame(page).getByText("Did not run")).toBeHidden();
      const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "After Hooks[\\d.]+m?s",
      ]);
    });
    test("should pick new / deleted nested tests", () => {
      const { page } = runUITest(basicTestTree);
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite
      `);

      page.getByText("suite").click();
      page.keyboard.press("ArrowRight");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ▼ ◯ suite <=
              ◯ inner passes
              ◯ inner fails
      `);

      writeFiles({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
          // [cap-test] test.describe('suite', () => {
            test('inner new', () => {});
            test('inner fails', () => {});
          // [cap-test] });
        `
      });

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
          ▼ ◯ suite <=
              ◯ inner new
              ◯ inner fails
      `);
    });
    test("should update test locations", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
        `,
      });

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
      `);

      const messages: []string = [];
      page.exposeBinding("__logForTest", (source, arg) => messages.push(arg));

      const passesItemLocator = page.getByRole("treeitem", { name: "passes" });
      passesItemLocator.hover();
      passesItemLocator.getByTitle("Show source").click();
      traceFrame(page).getByTitle("Open in VS Code").click();

      expect.poll(() => messages).toEqual([{
        method: "open",
        params: {
          location: {
            file: expect.stringContaining("a.test.ts"),
            line: 3,
            column: 7,
          }
        },
      }]);

      writeFiles({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('new-test', () => {});

          test('passes', () => {});
        `
      });

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ new-test
            ◯ passes <=
      `);

      messages.length = 0;
      passesItemLocator.hover();
      passesItemLocator.getByTitle("Show source").click();
      traceFrame(page).getByTitle("Open in VS Code").click();

      expect.poll(() => messages).toEqual([{
        method: "open",
        params: {
          location: {
            file: expect.stringContaining("a.test.ts"),
            line: 5,
            column: 7,
          }
        },
      }]);

      expect(
          traceFrame(page).getByTestId("source-code").locator(".source-tab-file-name")
      ).toHaveText("a.test.ts");
      expect(traceFrame(page).locator(".CodeMirror-code")).toContainText(`3      test('new-test', () => {});`);
    });
}
