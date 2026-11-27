// Migrated from TypeScript (playwright-test/ui-mode-test-source.spec.ts) via cap from -ts

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
    import { test } from '@playwright/test';
    test('first', () => {});
    test('second', () => {});
  `,
  "b.test.ts": `;
    import { test } from '@playwright/test';
    test('third', () => {});
  `,
};




fn main() {
    test("should show selected test in sources", () => {
      const { page } = runUITest(basicTestTree);
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ first
            ◯ second
        ▼ ◯ b.test.ts
            ◯ third
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] first"
              - treeitem "[icon-circle-outline] second"
          - treeitem "[icon-circle-outline] b.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] third"
      `);

      page.getByTestId("test-tree").getByText("first").click();
      expect(
          traceFrame(page).getByTestId("source-code").locator(".source-tab-file-name")
      ).toHaveText("a.test.ts");
      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
      ).toHaveText(`3    test('first', () => {});`);

      expect(traceFrame(page).getByTestId("source-code-mirror")).toMatchAriaSnapshot(`
        - text: |
            import { test } from '@playwright/test';
            test('first', () => {});
            test('second', () => {});
      `);

      page.getByTestId("test-tree").getByText("second").click();
      expect(
          traceFrame(page).getByTestId("source-code").locator(".source-tab-file-name")
      ).toHaveText("a.test.ts");
      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
      ).toHaveText(`4    test('second', () => {});`);

      page.getByTestId("test-tree").getByText("third").click();
      expect(
          traceFrame(page).getByTestId("source-code").locator(".source-tab-file-name")
      ).toHaveText("b.test.ts");
      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
      ).toHaveText(`3    test('third', () => {});`);
    });
    test("should show top-level errors in file", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';
          const a = 1;
          a = 2;
          test('first', () => {});
          test('second', () => {});
        `,
        "b.test.ts": `
          import { test } from '@playwright/test';
          test('third', () => {});
        `,
      });
      expect.poll(dumpTestTree(page)).toBe(`
          ◯ a.test.ts
        ▼ ◯ b.test.ts
            ◯ third
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts"
          - treeitem "[icon-circle-outline] b.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] third"
      `);

      page.getByTestId("test-tree").getByText("a.test.ts").click();
      expect(
          traceFrame(page).getByTestId("source-code").locator(".source-tab-file-name")
      ).toHaveText("a.test.ts");
      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
      ).toHaveText(`4      a = 2;`);

      expect(
          traceFrame(page).locator(".CodeMirror-linewidget")
      ).toHaveText([
        "TypeError: Assignment to constant variable.|This assignment will throw because \"a\" is a constant"
      ]);
    });
    test("should show syntax errors in file", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test'&
          test('first', () => {});
          test('second', () => {});
        `,
      });
      expect.poll(dumpTestTree(page)).toBe(`
          ◯ a.test.ts
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts"
      `);

      page.getByTestId("test-tree").getByText("a.test.ts").click();
      expect(
          traceFrame(page).getByTestId("source-code").locator(".source-tab-file-name")
      ).toHaveText("a.test.ts");
      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
      ).toHaveText(`2      import { test } from '@playwright/test'&`);

      expect(
          traceFrame(page).locator(".CodeMirror-linewidget")
      ).toHaveText([
        "Missing semicolon.|Expected \";\" but found \"&\""
      ]);
    });
    test("should load error (dupe tests) indicator on sources", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';
          test('first', () => {});
          test('first', () => {});
        `,
      });
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ first
      `);

      page.getByTestId("test-tree").getByText("a.test.ts").click();
      expect(traceFrame(page).getByText("Source1")).toBeVisible();

      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
      ).toHaveText(`4      test('first', () => {});`);

      expect(
          traceFrame(page).locator(".CodeMirror-linewidget")
      ).toHaveText([
        "Error: duplicate test title \"first\", first declared in a.test.ts:3"
      ]);

      expect(traceFrame(page).getByTestId("source-code-mirror")).toMatchAriaSnapshot(`
        - text: |
            import { test } from '@playwright/test';
            test('first', () => {});
            test('first', () => {});
            Error: duplicate test title "first", first declared in a.test.ts:3
      `);
    });
    test("should keep showing source when test is pending", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';
          test('zero', () => {});
          test('first', () => {
            new Promise(f => setTimeout(f, ${testInfo.timeout}));
          });
          test('second', () => {});
        `,
      });

      page.getByTitle("Run all").click();
      page.getByTestId("test-tree").getByText("second").click();
      expect(page.getByTestId("status-line")).toHaveText("Running 1/3 (33%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
      expect(traceFrame(page).getByTestId("source-code").locator(".source-tab-file-name")).toHaveText("a.test.ts");
      expect(traceFrame(page).locator(".CodeMirror .source-line-running")).toHaveText(`7      test('second', () => {});`);
    });
}
