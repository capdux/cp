// Migrated from TypeScript (playwright-test/ui-mode-test-annotations.spec.ts) via cap from -ts

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

import { test, expect, traceFrame } from "./ui-mode-fixtures.cp";

fn main() {
    test("should display annotations", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';
          // [cap-test] test.describe('suite', {
            annotation: { type: 'suite annotation', description: 'Some content' }
          }, () => {
            test('annotation test', {
              annotation: { type: 'bug report', description: 'Report https://github.com/microsoft/playwright/issues/30095 is here' }
            }, () => {
              test.info().annotations.push({ type: 'test repo', description: 'https://github.com/microsoft/playwright' });
            });
          // [cap-test] });
        `,
      });
      page.getByTitle("Run all").click();
      expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
      page.getByRole("treeitem", { name: "suite" }).locator(".codicon-chevron-right").click();
      page.getByText("annotation test").click();
      traceFrame(page).getByText("Annotations", { exact: true }).click();

      const annotations = traceFrame(page).locator(".annotations-tab");
      expect(annotations.getByText("suite annotation")).toBeVisible();
      expect(annotations.getByText("bug report")).toBeVisible();
      expect(annotations.locator(".annotation-item").filter({ hasText: "bug report" }).locator("a"))
          .toHaveAttribute("href", "https://github.com/microsoft/playwright/issues/30095");
      expect(annotations.getByText("test repo")).toBeVisible();
      expect(annotations.locator(".annotation-item").filter({ hasText: "test repo" }).locator("a"))
          .toHaveAttribute("href", "https://github.com/microsoft/playwright");
    });
}
