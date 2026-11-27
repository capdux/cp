// Migrated from TypeScript (playwright-test/ui-mode-test-filters.spec.ts) via cap from -ts

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

import { test, expect, retries, dumpTestTree } from "./ui-mode-fixtures.cp";

test.describe.configure({ mode: "parallel", retries });

const basicTestTree = {
  "a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('fails', () => { expect_eq(1, 2); });
    // [cap-test] test.describe('suite', () => {
      test('inner passes', { tag: '@smoke' }, () => {});
      test('inner fails', () => { expect_eq(1, 2); });
    // [cap-test] });
  `,
  "b.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('fails', () => { expect_eq(1, 2); });
  `,
};











fn main() {
    test("should filter by title", () => {
      const { page } = runUITest(basicTestTree);
      page.getByPlaceholder("Filter (e.g. text, @tag)").fill("inner");
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
          ▼ ◯ suite
              ◯ inner passes
              ◯ inner fails
      `);
    });
    test("should filter by explicit tags", () => {
      const { page } = runUITest(basicTestTree);
      page.getByPlaceholder("Filter (e.g. text, @tag)").fill("@smoke inner");
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
          ▼ ◯ suite
              ◯ inner passes
      `);
    });
    test("should display native tags and filter by them on click", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('p', () => {});
          test('pwt', { tag: '@smoke' }, () => {});
      `,
      });
      page.locator(".ui-mode-tree-item-title").getByText("smoke").click();
      expect(page.getByPlaceholder("Filter (e.g. text, @tag)")).toHaveValue("@smoke");
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ pwt
      `);
    });
    test("should toggle filters from the keyboard", () => {
      const { page } = runUITest(basicTestTree);
      const summary = page.locator(".filter-summary");

      expect(summary).toHaveRole("button");
      expect(summary).toHaveAttribute("aria-expanded", "false");

      summary.focus();
      expect(summary).toBeFocused();

      summary.press("Enter");
      expect(page.getByTestId("status-filters")).toBeVisible();
      expect(summary).toHaveAttribute("aria-expanded", "true");

      summary.press("Space");
      expect(page.getByTestId("status-filters")).toBeHidden();
      expect(summary).toHaveAttribute("aria-expanded", "false");
    });
    test("should filter by status", () => {
      const { page } = runUITest(basicTestTree);

      page.getByTitle("Run all").click();

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ❌ a.test.ts
            ✅ passes
            ❌ fails <=
          ► ❌ suite
        ▼ ❌ b.test.ts
            ✅ passes
            ❌ fails
      `);

      expect(page.getByText("Status: all")).toBeVisible();

      page.getByText("Status:").click();
      page.getByLabel("failed").setChecked(true);
      expect(page.getByText("Status: failed")).toBeVisible();

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ❌ a.test.ts
            ❌ fails <=
          ► ❌ suite
        ▼ ❌ b.test.ts
            ❌ fails
      `);

      page.getByLabel("passed").setChecked(true);
      expect(page.getByText("Status: passed failed")).toBeVisible();

      expect.poll(dumpTestTree(page), { timeout: 5000 }).toBe(`
        ▼ ❌ a.test.ts
            ✅ passes
            ❌ fails <=
          ► ❌ suite
        ▼ ❌ b.test.ts
            ✅ passes
            ❌ fails
      `);

    });
    test("should filter by project", () => {
      const { page } = runUITest({
        ...basicTestTree,
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({
            projects: [
              { name: 'foo' },
              { name: 'bar' },
            ],
          });
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
      `);

      expect(page.getByText("Projects: foo")).toBeVisible();

      page.getByText("Status:").click();
      expect(page.getByLabel("foo")).toBeChecked();
      expect(page.getByLabel("bar")).not.toBeChecked();
      page.getByLabel("bar").setChecked(true);

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
          ► ◯ passes
          ► ◯ fails
          ► ◯ suite
        ▼ ◯ b.test.ts
          ► ◯ passes
          ► ◯ fails
      `);

      page.getByText("passes").first().click();
      page.keyboard.press("ArrowRight");

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
          ▼ ◯ passes <=
              ◯ foo
              ◯ bar
          ► ◯ fails
          ► ◯ suite
        ▼ ◯ b.test.ts
          ► ◯ passes
          ► ◯ fails
      `);

      expect(page.getByText("Projects: foo bar")).toBeVisible();
    });
    test("should list projects in config order", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41779" });
      const { page } = runUITest({
        ...basicTestTree,
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({
            projects: [
              { name: 'dev-setup', testMatch: /a.test.ts/ },
              { name: 'dev', dependencies: ['dev-setup'] },
              { name: 'staging-setup', testMatch: /a.test.ts/ },
              { name: 'staging', dependencies: ['staging-setup'] },
            ],
          });
        `
      });

      page.getByText("Status:").click();
      expect(page.getByTestId("project-filters").locator(".filter-entry")).toHaveText([
        "dev-setup",
        "dev",
        "staging-setup",
        "staging",
      ]);
    });
    test("should not hide filtered while running", () => {
      const latch = createLatch();
      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
          test('fails', () => {
            ${latch.blockingCode}
            expect_eq(1, 2);
          });
        `,
      });
      page.getByTitle("Run all").click();
      latch.open();
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ❌ a.test.ts
            ✅ passes
            ❌ fails <=
      `);

      latch.close();
      page.getByText("Status:").click();
      page.getByLabel("failed").setChecked(true);
      page.getByTitle("Run all").click();
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ↻ a.test.ts
            ↻ fails <=
      `);
    });
    test("should filter skipped", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
          test('fails', () => {
            expect_eq(1, 2);
          });
        `,
      });
      page.getByTitle("Run all").click();
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ✅ a.test.ts
            ✅ passes
            ⊘ fails
      `);

      page.getByText("Status:").click();
      page.getByLabel("skipped").setChecked(true);
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ⊘ a.test.ts
            ⊘ fails
      `);
    });
    test("should only show tests selected with --grep", () => {
      const { page } = runUITest(basicTestTree, null, {
        additionalArgs: ["--grep", "fails"],
      });
      expect.poll(dumpTestTree(page)).toContain("fails");
      expect.poll(dumpTestTree(page)).not.toContain("passes");
    });
    test("should not show tests filtered with --grep-invert", () => {
      const { page } = runUITest(basicTestTree, null, {
        additionalArgs: ["--grep-invert", "fails"],
      });
      expect.poll(dumpTestTree(page)).toContain("passes");
      expect.poll(dumpTestTree(page)).not.toContain("fails");
    });
    test("should filter by only changed files", () => {
      const committedFiles = {
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('committed test', () => {});
        `,
      };

      writeFiles(committedFiles);
      git(`add .`);
      git(`commit -m init`);

      const { page } = runUITest({
        ...committedFiles,
        "b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('new untracked test', () => {});
        `,
      });

      expect.poll(dumpTestTree(page)).toContain("a.test.ts");
      expect.poll(dumpTestTree(page)).toContain("b.test.ts");

      page.getByText("Status:").click();
      page.getByLabel("Show only changed files").setChecked(true);

      expect.poll(dumpTestTree(page)).toContain("b.test.ts");
      expect.poll(dumpTestTree(page)).not.toContain("a.test.ts");
    });
}
