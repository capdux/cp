// Migrated from TypeScript (playwright-test/ui-mode-test-watch.spec.ts) via cap from -ts

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








  expect.poll(dumpTestTree(page)).toBe(`
    ▼ ◯ a.test.ts
        ◯ test
    ▼ ◯ b.test.ts
        ◯ test
    ▼ ◯ c.test.ts
        ◯ test
    ▼ ◯ d.test.ts
        ◯ test
  `);

  page.getByTitle("Watch all").click();
  page.getByTitle("Run all").click();

  expect(page.getByTestId("status-line")).toHaveText("Running 1/4 (25%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");

  writeFiles({
    "a.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    "b.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    "c.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
  });

  // Now watches should not kick in.
  Promise.init(f => setTimeout(f, 1000));
  expect(page.getByTestId("status-line")).toHaveText("Running 1/4 (25%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");

  // Allow test to finish and new watch to  kick in.
  latch.open();

  expect(page.getByTestId("status-line")).toHaveText("3/3 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 3 passed");
// TODO(cap-migrate): refine TS statement
// [ts] });



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should watch files", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("fails").click();
    // [ts] page.getByRole("treeitem", { name: "fails" }).getByRole("button", { name: "Watch" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ passes
    // [ts] ◯ fails 👁 <=
    // [ts] `);
    // [ts] page.getByRole("treeitem", { name: "fails" }).getByRole("button", { name: "Run" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ❌ a.test.ts
    // [ts] ◯ passes
    // [ts] ❌ fails 👁 <=
    // [ts] `);
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('fails', () => { expect_eq(1, 1); });
    // [ts] `
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ passes
    // [ts] ✅ fails 👁 <=
    // [ts] `);
    // [ts] });
    // [ts] test("should watch e2e deps", () => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({ testDir: 'tests' });
    // [ts] `,
    // [ts] "src/helper.ts": `
    // [ts] export const answer = 41;
    // [ts] `,
    // [ts] "tests/a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { answer } from '../src/helper';
    // [ts] test('answer', () => { expect_eq(answer, 42); });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("answer").click();
    // [ts] page.getByRole("treeitem", { name: "answer" }).getByRole("button", { name: "Watch" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ answer 👁 <=
    // [ts] `);
    // [ts] writeFiles({
    // [ts] "src/helper.ts": `
    // [ts] export const answer = 42;
    // [ts] `
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ answer 👁 <=
    // [ts] `);
    // [ts] });
    // [ts] test("should batch watch updates", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "b.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "c.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "d.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] page.getByText("a.test.ts").click();
    // [ts] page.getByRole("treeitem", { name: "a.test.ts" }).getByRole("button", { name: "Watch" }).click();
    // [ts] page.getByText("b.test.ts").click();
    // [ts] page.getByRole("treeitem", { name: "b.test.ts" }).getByRole("button", { name: "Watch" }).click();
    // [ts] page.getByText("c.test.ts").click();
    // [ts] page.getByRole("treeitem", { name: "c.test.ts" }).getByRole("button", { name: "Watch" }).click();
    // [ts] page.getByText("d.test.ts").click();
    // [ts] page.getByRole("treeitem", { name: "d.test.ts" }).getByRole("button", { name: "Watch" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts 👁
    // [ts] ◯ test
    // [ts] ▼ ◯ b.test.ts 👁
    // [ts] ◯ test
    // [ts] ▼ ◯ c.test.ts 👁
    // [ts] ◯ test
    // [ts] ▼ ◯ d.test.ts 👁 <=
    // [ts] ◯ test
    // [ts] `);
    // [ts] writeFiles({
    // [ts] "a.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "b.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "c.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "d.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] expect(page.getByTestId("status-line")).toHaveText("4/4 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 4 passed");
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts 👁
    // [ts] ✅ test
    // [ts] ▼ ✅ b.test.ts 👁
    // [ts] ✅ test
    // [ts] ▼ ✅ c.test.ts 👁
    // [ts] ✅ test
    // [ts] ▼ ✅ d.test.ts 👁 <=
    // [ts] ✅ test
    // [ts] `);
    // [ts] });
    // [ts] test("should watch all", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "b.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "c.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "d.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] page.getByTitle("Watch all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ test
    // [ts] ▼ ◯ b.test.ts
    // [ts] ◯ test
    // [ts] ▼ ◯ c.test.ts
    // [ts] ◯ test
    // [ts] ▼ ◯ d.test.ts
    // [ts] ◯ test
    // [ts] `);
    // [ts] writeFiles({
    // [ts] "a.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "d.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] expect(page.getByTestId("status-line")).toHaveText("2/2 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 2 passed");
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts
    // [ts] ✅ test
    // [ts] ▼ ◯ b.test.ts
    // [ts] ◯ test
    // [ts] ▼ ◯ c.test.ts
    // [ts] ◯ test
    // [ts] ▼ ✅ d.test.ts
    // [ts] ✅ test
    // [ts] `);
    // [ts] });
    // [ts] test("should watch new file", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] page.getByTitle("Watch all").click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ test
    // [ts] `);
    // First time add file.
    // [ts] writeFiles({
    // [ts] "b.test.ts": ` import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ test
    // [ts] ▼ ◯ b.test.ts
    // [ts] ◯ test
    // [ts] `);
    // Second time run file.
    // [ts] writeFiles({
    // [ts] "b.test.ts": ` import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ test
    // [ts] ▼ ✅ b.test.ts
    // [ts] ✅ test
    // [ts] `);
    // [ts] });
    // [ts] test("should run added test in watched file", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('foo', () => {});
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("a.test.ts").click();
    // [ts] page.getByRole("treeitem", { name: "a.test.ts" }).getByRole("button", { name: "Watch" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts 👁 <=
    // [ts] ◯ foo
    // [ts] `);
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('foo', () => {});
    // [ts] test('bar', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ a.test.ts 👁 <=
    // [ts] ✅ foo
    // [ts] ✅ bar
    // [ts] `);
    // [ts] });
    // [ts] test("should queue watches", () => {
    // [ts] const latch = createLatch();
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "b.test.ts": `import { test } from '@playwright/test'; test('test', async () => {
    // [ts] ${latch.blockingCode}
    // [ts] });`,
    // [ts] "c.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] "d.test.ts": `import { test } from '@playwright/test'; test('test', () => {});`,
    // [ts] });
    // [ts] test("should not watch output", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {
    // [ts] require('fs').writeFileSync(testInfo.outputPath('output.txt'), 'DATA');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts
    // [ts] ◯ passes
    // [ts] `);
    // [ts] const commands: []string = [];
    // [ts] page.exposeBinding("__logForTest", (source, arg) => {
    // [ts] commands.push(arg.method);
    // [ts] });
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] expect_eq(std.mem.indexOf(u8, commands, "runTests") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, commands, "listTests") == null, true);
    // [ts] });
    // [ts] test("should have watch icon highlighted when a test is focused and watch on the test is enabled", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] const watchButton = page.getByRole("treeitem", { name: "passes" }).getByRole("button", { name: "Watch" });
    // [ts] page.getByTestId("test-tree").getByText("a.test.ts").click();
    // watch icon should not be highlight till the watch icon is clicked
    // [ts] page.getByRole("treeitem", { name: "passes" }).hover();
    // [ts] expect(watchButton).toHaveCSS("outline-style", "none");
    // [ts] watchButton.click();
    // [ts] expect(watchButton.locator(".codicon-eye")).toHaveCSS("color", "rgb(255, 255, 255)");
    // [ts] expect(watchButton).toHaveCSS("outline-style", "solid");
    // [ts] expect(watchButton).toHaveCSS("outline-width", "1px");
    // [ts] expect(watchButton).toHaveCSS("outline-color", "rgb(255, 255, 255)");
    // deselection of the tree-row should still show the watch icon when watch on tree row is active
    // [ts] page.getByTestId("test-tree").getByText("a.test.ts").click();
    // [ts] expect(watchButton).toBeVisible();
    // [ts] expect(watchButton).toHaveCSS("outline-style", "none");
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ a.test.ts <=
    // [ts] ◯ passes 👁
    // [ts] `);
    // [ts] });
    // [ts] test("should watch test defined outside of .spec.ts file", () => {
    // [ts] const { page } = runUITest({
    // [ts] "example.spec.ts": `
    // [ts] import './impl';
    // [ts] `,
    // [ts] "impl.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('one', () => {});
    // [ts] `,
    // [ts] });
    // [ts] page.getByRole("treeitem", { name: "one" }).click();
    // [ts] page.getByRole("treeitem", { name: "one" }).getByRole("button", { name: "Watch" }).click();
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ◯ example.spec.ts
    // [ts] ◯ one 👁 <=
    // [ts] `);
    // [ts] writeFiles({
    // [ts] "impl.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('one', () => { /* modified */ });
    // [ts] `,
    // [ts] });
    // [ts] expect.poll(dumpTestTree(page)).toBe(`
    // [ts] ▼ ✅ example.spec.ts
    // [ts] ✅ one 👁 <=
    // [ts] `);
    // [ts] });
}
