// Migrated from TypeScript (playwright-test/ui-mode-test-shortcut.spec.ts) via cap from -ts

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
    test('test 0', () => { test.skip(); });
    test('test 1', () => {});
    test('test 2', () => { new Promise(() => {}); });
    test('test 3', () => {});
  `
};


fn main() {
    test("should run tests", () => {
      const { page } = runUITest(basicTestTree);

      expect(page.getByTitle("Run all")).toBeEnabled();
      expect(page.getByTestId("stop-button")).toBeDisabled();

      page.getByPlaceholder("Filter (e.g. text, @tag)").fill("test 3");
      // The shortcut also works when the focus is in the trace viewer frame.
      traceFrame(page).getByTestId("actions-tree").click();
      page.keyboard.press("F5");

      expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
      page.getByPlaceholder("Filter (e.g. text, @tag)").fill("");

      // Only the filtered test was run.
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ test 0
            ◯ test 1
            ◯ test 2
            ✅ test 3
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] test 0"
              - treeitem "[icon-circle-outline] test 1"
              - treeitem "[icon-circle-outline] test 2"
              - treeitem ${/\[icon-check\] test 3/}
      `);
    });
    test("should stop tests", () => {
      const { page } = runUITest(basicTestTree);

      expect(page.getByTitle("Run all")).toBeEnabled();
      expect(page.getByTestId("stop-button")).toBeDisabled();

      page.getByTitle("Run all").click();

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ↻ a.test.ts
            ⊘ test 0
            ✅ test 1
            ↻ test 2
            🕦 test 3
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-loading] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-slash] test 0"
              - treeitem ${/\[icon-check\] test 1/}
              - treeitem ${/\[icon-loading\] test 2/}
              - treeitem ${/\[icon-clock\] test 3/}
      `);

      expect(page.getByTitle("Run all")).toBeDisabled();
      expect(page.getByTestId("stop-button")).toBeEnabled();

      page.keyboard.press("Shift+F5");

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ⊘ test 0
            ✅ test 1
            ◯ test 2
            ◯ test 3
      `);
    });
    test("should toggle Terminal", () => {
      const { page } = runUITest(basicTestTree);

      expect(page.getByTitle("Run all")).toBeEnabled();
      expect(page.getByTestId("stop-button")).toBeDisabled();

      expect(page.getByTestId("output")).toBeHidden();

      page.keyboard.press(if (process.platform == "darwin") "Control+Backquote" else "Control+Shift+Backquote");

      expect(page.getByTestId("output")).toBeVisible();
    });
}
