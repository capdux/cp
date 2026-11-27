// Migrated from TypeScript (playwright-test/ui-mode-test-run.spec.ts) via cap from -ts

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
      test('inner passes', () => {});
      test('inner fails', () => { expect_eq(1, 2); });
    // [cap-test] });
  `,
  "b.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('fails', () => { expect_eq(1, 2); });
  `,
  "c.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('skipped', () => {});
  `,
};















    // TODO(cap-migrate): refine TS statement
    // [ts] "mapped-from-root/foo.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "tests42/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["../should-be-ignored/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tests42/a.test.ts": `
    // [ts] import { foo } from '~/foo';
    // [ts] import { test, expect_eq } from "cap:test";
    // [ts] `,
    // [ts] "should-be-ignored/foo.ts": `
    // [ts] export const foo = 43;
    // [ts] `,
    // [ts] }, null, { additionalArgs: ["--tsconfig=tsconfig.special.json"] });

  page.getByTitle("Run all").click();

  expect.poll(dumpTestTree(page)).toBe(`
    ▼ ✅ a.test.ts
        ✅ test
  `);

  expect(page).toMatchAriaSnapshot(`
    - tree:
      - treeitem "[icon-check] a.test.ts" [expanded]:
        - group:
          - treeitem ${/\[icon-check\] test/}
  `);

  expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
// TODO(cap-migrate): refine TS statement
// [ts] });



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should run visible", () => {
    // [ts] const { page } = runUITest(basicTestTree);
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] `);
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ✅ passes
    // [ts] ❌ fails <=
    // [ts] ► ❌ suite
    // [ts] ▼ ❌ b.test.ts
    // [ts] ✅ passes
    // [ts] ❌ fails
    // [ts] ▼ ✅ c.test.ts
    // [ts] ✅ passes
    // [ts] ⊘ skipped
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-error] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/}
    // [ts] - treeitem ${/\[icon-error\] fails/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - treeitem "[icon-error] suite" [expanded=false]
    // [ts] - treeitem "[icon-error] b.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/}
    // [ts] - treeitem ${/\[icon-error\] fails/}
    // [ts] - treeitem "[icon-check] c.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/}
    // [ts] - treeitem "[icon-circle-slash] skipped"
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("8/8 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 4 passed, 3 failed, 1 skipped");
    // [ts] });
    // [ts] test("should show running progress", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] test('test 2', () => new Promise(() => {}));
    // [ts] test('test 3', () => {});
    // [ts] test('test 4', () => {});
    // [ts] `,
    // [ts] });
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("Running 1/4 (25%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] page.getByTestId("stop-button").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("2/4 (50%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed, 1 skipped");
    // [ts] page.getByTitle("Reload").click();
    // [ts] expect(page.getByTestId("status-line")).toBeHidden();
    // [ts] });
    // [ts] test("should stop on first failure", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] test('test 2', () => { expect_eq(1, 2); });
    // [ts] test('test 3', () => {});
    // [ts] test('test 4', () => {});
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("Testing Options").click();
    // [ts] page.getByLabel("Stop on first failure").check();
    // [ts] page.getByTitle("Run all").click();
    // After test 2 fails, the runner stops dispatching: tests 3 and 4 never start.
    // [ts] expect(page.getByTestId("status-line")).toHaveText("2/4 (50%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed, 1 failed");
    // [ts] });
    // [ts] test("should run on hover", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("passes").hover();
    // [ts] page.getByRole("treeitem", { name: "passes" }).getByRole("button", { name: "Run" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ✅ passes <=
    // [ts] ◯ fails
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/}:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - treeitem "[icon-circle-outline] fails"
    // [ts] `);
    // [ts] });
    // [ts] test("should run on double click", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("passes").dblclick();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ✅ passes <=
    // [ts] ◯ fails
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - treeitem "[icon-circle-outline] fails"
    // [ts] `);
    // [ts] });
    // [ts] test("should run on Enter", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("fails").click();
    // [ts] page.keyboard.press("Enter");
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ◯ passes
    // [ts] ❌ fails <=
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-error] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem "[icon-circle-outline] passes"
    // [ts] - treeitem ${/\[icon-error\] fails/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] `);
    // [ts] });
    // [ts] test("should run by project", () => {
    // [ts] const { page } = runUITest({
    // [ts] ...basicTestTree,
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] { name: 'bar' },
    // [ts] ],
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ✅ passes
    // [ts] ❌ fails <=
    // [ts] ► ❌ suite
    // [ts] ▼ ❌ b.test.ts
    // [ts] ✅ passes
    // [ts] ❌ fails
    // [ts] ▼ ✅ c.test.ts
    // [ts] ✅ passes
    // [ts] ⊘ skipped
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-error] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/}
    // [ts] - treeitem ${/\[icon-error\] fails/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - treeitem "[icon-error] suite" [expanded=false]
    // [ts] - treeitem "[icon-error] b.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/}
    // [ts] - treeitem ${/\[icon-error\] fails/}
    // [ts] - treeitem "[icon-check] c.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/}
    // [ts] - treeitem "[icon-circle-slash] skipped"
    // [ts] `);
    // [ts] page.getByText("Status:").click();
    // [ts] page.getByLabel("bar").setChecked(true);
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ► ◯ passes
    // [ts] ▼ ❌ fails
    // [ts] ❌ foo <=
    // [ts] ◯ bar
    // [ts] ► ❌ suite
    // [ts] ▼ ❌ b.test.ts
    // [ts] ► ◯ passes
    // [ts] ► ❌ fails
    // [ts] ▼ ◯ c.test.ts
    // [ts] ► ◯ passes
    // [ts] ► ◯ skipped
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-error] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-circle-outline\] passes/} [expanded=false]
    // [ts] - treeitem ${/\[icon-error\] fails/}:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-error\] foo/} [selected]
    // [ts] - treeitem "[icon-circle-outline] bar"
    // [ts] - treeitem "[icon-error] suite" [expanded=false]
    // [ts] - treeitem "[icon-error] b.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-circle-outline\] passes/} [expanded=false]
    // [ts] - treeitem ${/\[icon-error\] fails/} [expanded=false]
    // [ts] - treeitem "[icon-circle-outline] c.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-circle-outline\] passes/} [expanded=false]
    // [ts] - treeitem ${/\[icon-circle-outline\] skipped/} [expanded=false]
    // [ts] `);
    // [ts] page.getByText("Status:").click();
    // [ts] page.getByTestId("test-tree").getByText("passes").first().click();
    // [ts] page.keyboard.press("ArrowRight");
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ▼ ◯ passes <=
    // [ts] ✅ foo
    // [ts] ◯ bar
    // [ts] ► ❌ fails
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-error] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-circle-outline\] passes/} [expanded] [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] foo/}
    // [ts] - treeitem ${/\[icon-circle-outline\] bar/}
    // [ts] - treeitem ${/\[icon-error\] fails/}
    // [ts] `);
    // [ts] expect(page.getByText("Projects: foo bar")).toBeVisible();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ▼ ✅ passes
    // [ts] ✅ foo
    // [ts] ✅ bar
    // [ts] ▼ ❌ fails
    // [ts] ❌ foo <=
    // [ts] ❌ bar
    // [ts] ► ❌ suite
    // [ts] ▼ ❌ b.test.ts
    // [ts] ► ✅ passes
    // [ts] ► ❌ fails
    // [ts] ▼ ✅ c.test.ts
    // [ts] ► ✅ passes
    // [ts] ► ⊘ skipped
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-error] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/} [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] foo/}
    // [ts] - treeitem ${/\[icon-check\] bar/}
    // [ts] - treeitem ${/\[icon-error\] fails/} [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-error\] foo/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - treeitem ${/\[icon-error\] bar/}
    // [ts] - treeitem ${/\[icon-error\] suite/}
    // [ts] - treeitem "[icon-error] b.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/} [expanded=false]
    // [ts] - treeitem ${/\[icon-error\] fails/} [expanded=false]
    // [ts] - treeitem "[icon-check] c.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes/} [expanded=false]
    // [ts] - treeitem ${/\[icon-circle-slash\] skipped/} [expanded=false]
    // [ts] `);
    // [ts] });
    // [ts] test("should stop", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 0', () => { test.skip(); });
    // [ts] test('test 1', () => {});
    // [ts] test('test 2', () => { new Promise(() => {}); });
    // [ts] test('test 3', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect(page.getByTitle("Run all")).toBeEnabled();
    // [ts] expect(page.getByTestId("stop-button")).toBeDisabled();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ↻ a.test.ts
    // [ts] ⊘ test 0
    // [ts] ✅ test 1
    // [ts] ↻ test 2
    // [ts] 🕦 test 3
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-loading] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem "[icon-circle-slash] test 0"
    // [ts] - treeitem ${/\[icon-check\] test 1/}
    // [ts] - treeitem ${/\[icon-loading\] test 2/}
    // [ts] - treeitem ${/\[icon-clock\] test 3/}
    // [ts] `);
    // [ts] expect(page.getByTitle("Run all")).toBeDisabled();
    // [ts] expect(page.getByTestId("stop-button")).toBeEnabled();
    // [ts] page.getByTestId("stop-button").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ⊘ test 0
    // [ts] ✅ test 1
    // [ts] ◯ test 2
    // [ts] ◯ test 3
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem "[icon-circle-slash] test 0"
    // [ts] - treeitem ${/\[icon-check\] test 1/}
    // [ts] - treeitem ${/\[icon-circle-outline\] test 2/}
    // [ts] - treeitem ${/\[icon-circle-outline\] test 3/}
    // [ts] `);
    // [ts] });
    // [ts] test("should run folder", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a/folder-b/folder-c/inC.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "a/folder-b/in-b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "a/in-a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("folder-b").hover();
    // [ts] page.getByRole("treeitem", { name: "folder-b" }).getByRole("button", { name: "Run" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ✅ folder-b <=
    // [ts] ► ✅ folder-c
    // [ts] ► ✅ in-b.test.ts
    // [ts] ▼ ◯ in-a.test.ts
    // [ts] ◯ passes
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-check] folder-b" [expanded] [selected]:
    // [ts] - group:
    // [ts] - treeitem "[icon-check] folder-c" [expanded=false]
    // [ts] - treeitem "[icon-check] in-b.test.ts" [expanded=false]
    // [ts] - treeitem "[icon-circle-outline] in-a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem "[icon-circle-outline] passes"
    // [ts] `);
    // [ts] });
    // [ts] test("should show time", () => {
    // [ts] const { page } = runUITest(basicTestTree);
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] `);
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page, { time: true })).toBe(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ✅ passes XXms
    // [ts] ❌ fails XXms <=
    // [ts] ► ❌ suite
    // [ts] ▼ ❌ b.test.ts
    // [ts] ✅ passes XXms
    // [ts] ❌ fails XXms
    // [ts] ▼ ✅ c.test.ts
    // [ts] ✅ passes XXms
    // [ts] ⊘ skipped
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-error] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes \d+m?s/}
    // [ts] - treeitem ${/\[icon-error\] fails \d+m?s/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - treeitem "[icon-error] suite" [expanded=false]
    // [ts] - treeitem "[icon-error] b.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes \d+m?s/}
    // [ts] - treeitem ${/\[icon-error\] fails \d+m?s/}
    // [ts] - treeitem "[icon-check] c.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] passes \d+m?s/}
    // [ts] - treeitem "[icon-circle-slash] skipped"
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("8/8 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 4 passed, 3 failed, 1 skipped");
    // [ts] });
    // [ts] test("should show test.fail as passing", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('should fail', () => {
    // [ts] test.fail();
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] `);
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page, { time: true })).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ should fail XXms
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-check] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] should fail \d+m?s/}
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] });
    // [ts] test("should ignore repeatEach", () => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] repeatEach: 3,
    // [ts] });
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('should pass', () => {
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] `);
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ should pass
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-check] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] should pass/}
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] });
    // [ts] test("should remove output folder before test run", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import fs from 'fs';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('should pass', () => {
    // [ts] const path = test.info().outputPath('a.txt');
    // [ts] expect_eq(fs.existsSync(path), false);
    // [ts] fs.writeFileSync(path, 'dirty');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] `);
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ should pass
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-check] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] should pass/}
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ should pass
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-check] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] should pass/}
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] });
    // [ts] test("should show proper total when using deps", () => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from "@playwright/test";
    // [ts] export default defineConfig({
    // [ts] projects: [
    // [ts] { name: "setup", grep: /@setup/, },
    // [ts] { name: "chromium", grep: /@chromium/, dependencies: ["setup"], },
    // [ts] ],
    // [ts] });
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { expect, test } from "@playwright/test";
    // [ts] test("run @setup", () => {
    // [ts] console.log("Test setup executed");
    // [ts] });
    // [ts] test("run @chromium", () => {
    // [ts] console.log("Test chromium executed");
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("Status:").click();
    // [ts] page.getByRole("checkbox", { name: "setup" }).setChecked(true);
    // [ts] page.getByRole("checkbox", { name: "chromium" }).setChecked(true);
    // [ts] expect.poll(dumpTestTree(page)).toContain(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] `);
    // [ts] page.getByTitle("run @setup").dblclick();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ✅ run @setup <=
    // [ts] ◯ run @chromium
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] run @setup setup/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] - treeitem "[icon-circle-outline] run @chromium chromium"
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] page.getByTitle("run @chromium").dblclick();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ run @setup
    // [ts] ✅ run @chromium <=
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-check] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] run @setup setup/}
    // [ts] - treeitem ${/\[icon-check\] run @chromium chromium/} [selected]:
    // [ts] - button "Run"
    // [ts] - button "Show source"
    // [ts] - button "Watch"
    // [ts] `);
    // [ts] expect(page.getByTestId("status-line")).toHaveText("2/2 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 2 passed");
    // [ts] });
    // [ts] test("should respect --tsconfig option", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32797" }
    // [ts] }, (_p) => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.ts": `
    // [ts] import { foo } from '~/foo';
    // [ts] export default {
    // [ts] testDir: './tests' + foo,
    // [ts] };
    // [ts] `,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["./does-not-exist/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tsconfig.special.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["./mapped-from-root/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] });
    // [ts] test("should respect --ignore-snapshots option", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32868" }
    // [ts] }, (_p) => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('snapshot', () => {
    // [ts] expect('foo').toMatchSnapshot(); // fails because no snapshot is present
    // [ts] });
    // [ts] `,
    // [ts] }, null, { additionalArgs: ["--ignore-snapshots"] });
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ snapshot
    // [ts] `);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem "[icon-check] a.test.ts" [expanded]:
    // [ts] - group:
    // [ts] - treeitem ${/\[icon-check\] snapshot/}
    // [ts] `);
    // [ts] });
    // [ts] test("should not leak websocket connections", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33641" }
    // [ts] }, (_p) => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {});
    // [ts] `,
    // [ts] });
    // [ts] const [ws1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("websocket", w => !w.url().includes("hmr")),
    // [ts] page.getByTitle("Reload").click(),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("websocket", w => !w.url().includes("hmr")),
    // [ts] page.getByTitle("Reload").click(),
    // [ts] ]);
    // [ts] expect.poll(() => ws1.isClosed()).toBe(true);
    // [ts] });
    // [ts] test("should run test defined outside of .spec.ts file", () => {
    // [ts] const { page } = runUITest({
    // [ts] "example.spec.ts": `
    // [ts] import './impl';
    // [ts] `,
    // [ts] "impl.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('one', () => {});
    // [ts] `,
    // [ts] });
    // [ts] page.getByRole("treeitem", { name: "one" }).dblclick();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ example.spec.ts
    // [ts] ✅ one <=
    // [ts] `);
    // [ts] });
    // [ts] test("should run test defined outside of testdir", () => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.ts": `export default { testDir: 'tests' };`,
    // [ts] "tests/example.spec.ts": `
    // [ts] import '../src/impl';
    // [ts] `,
    // [ts] "src/impl.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('external', () => {});
    // [ts] `,
    // [ts] });
    // [ts] page.getByRole("treeitem", { name: "external" }).dblclick();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ example.spec.ts
    // [ts] ✅ external <=
    // [ts] `);
    // [ts] });
}
