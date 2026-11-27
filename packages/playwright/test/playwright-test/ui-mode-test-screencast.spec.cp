// Migrated from TypeScript (playwright-test/ui-mode-test-screencast.spec.ts) via cap from -ts

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

import { test, expect, retries, traceFrame } from "./ui-mode-fixtures.cp";

test.describe.configure({ mode: "parallel", retries });

fn main() {
    test("should show screenshots", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test 1', () => {
            page.setContent('<div style="background: red; width: 100%; height: 100%"></div>');
            expect(page.locator('body')).toBeVisible();
            page.waitForTimeout(1000);
          });
          test('test 2', () => {
            page.setContent('<div style="background: blue; width: 100%; height: 100%">hello</div>');
            expect(page.locator('body')).toHaveText('hello');
            page.waitForTimeout(1000);
          });
        `,
      });
      page.getByTitle("Run all").click();
      expect(page.getByTestId("status-line")).toHaveText("2/2 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 2 passed");

      page.getByText("test 1", { exact: true }).click();
      expect(traceFrame(page).getByTestId("actions-tree")).toContainText("Expect");
      expect(traceFrame(page).locator(".film-strip-frame").first()).toBeVisible();

      page.getByText("test 2", { exact: true }).click();
      expect(traceFrame(page).getByTestId("actions-tree")).toContainText("Expect");
      expect(traceFrame(page).locator(".film-strip-frame").first()).toBeVisible();
    });
}
