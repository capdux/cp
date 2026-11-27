// Migrated from TypeScript (playwright-test/ui-mode-trace.spec.ts) via cap from -ts

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

import * as fs from "cap:fs";
import * as os from "cap:os";
import * as path from "cap:path";

import { createImage } from "./playwright-test-fixtures.cp";
import { test, expect, retries, traceFrame } from "./ui-mode-fixtures.cp";

test.describe.configure({ mode: "parallel", retries });



























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should merge trace events", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('trace test', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] expect_eq(1, 1);
    // [ts] page.getByRole('button').click();
    // [ts] expect_eq(2, 2);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("trace test").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Set content",
    // [ts] "Expect \"toBe\"",
    // [ts] "Click.*getByRole",
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] });
    // [ts] test("should work with non-existing rootDir and testDir outside of it", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/38367" },
    // [ts] }, (_p) => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] testDir: './nothingness',
    // [ts] projects: [
    // [ts] { name: 'main', testDir: './tests' },
    // [ts] ],
    // [ts] });
    // [ts] `,
    // [ts] "tests/a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example test', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("example test").dblclick();
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - treeitem /Before Hooks/
    // [ts] - treeitem /Set content/ [selected]
    // [ts] - treeitem /After Hooks/
    // [ts] `);
    // [ts] });
    // [ts] test("should merge web assertion events", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('trace test', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] expect(page.locator('button')).toBeVisible();
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("trace test").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Set content",
    // [ts] "Expect \"toBeVisible\".*locator",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] });
    // [ts] test("should merge screenshot assertions", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('trace test', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] expect(page.locator('button')).toHaveScreenshot();
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("trace test").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Set content",
    // [ts] "Expect \"toHaveScreenshot\"",
    // [ts] "After Hooks",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] ]);
    // [ts] });
    // [ts] test("should locate sync assertions in source", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('trace test', () => {
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("trace test").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
    // [ts] traceFrame(page).getByText("Expect \"toBe\"").click();
    // [ts] expect(
    // [ts] traceFrame(page).locator(".CodeMirror .source-line-running"),
    // [ts] "check source tab",
    // [ts] ).toHaveText("4        expect(1).toBe(1);");
    // [ts] });
    // [ts] test("should show snapshots for sync assertions", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('trace test', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] page.getByRole('button').click();
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("trace test").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Set content",
    // [ts] "Click.*getByRole",
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] expect(
    // [ts] traceFrame(page).frameLocator("iframe.snapshot-visible[name=snapshot]").locator("button"),
    // [ts] "verify snapshot"
    // [ts] ).toHaveText("Submit");
    // [ts] });
    // [ts] test("should show snapshots for steps", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35285" }
    // [ts] }, (_p) => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeEach(({ page }) => {
    // [ts] page.setContent('<div>initial</div>');
    // [ts] });
    // [ts] test('steps test', () => {
    // [ts] test.step('first', () => {
    // [ts] page.setContent("<div>foo</div>");
    // [ts] });
    // [ts] test.step('middle', () => {
    // [ts] page.setContent("<div>bar</div>");
    // [ts] });
    // [ts] test.step('last', () => {
    // [ts] page.setContent("<div>baz</div>");
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("steps test").dblclick();
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem /Before Hooks/
    // [ts] - treeitem /first/
    // [ts] - treeitem /middle/
    // [ts] - treeitem /last/
    // [ts] - treeitem /After Hooks/
    // [ts] `);
    // [ts] traceFrame(page).getByTestId("actions-tree").getByText("first").click();
    // [ts] const snapshot = traceFrame(page).frameLocator("iframe.snapshot-visible[name=snapshot]").locator("div");
    // [ts] traceFrame(page).getByText("After", { exact: true }).click();
    // [ts] expect(snapshot).toHaveText("foo");
    // [ts] traceFrame(page).getByText("Before", { exact: true }).click();
    // [ts] expect(snapshot).toHaveText("initial");
    // [ts] traceFrame(page).getByTestId("actions-tree").getByText("middle").click();
    // [ts] traceFrame(page).getByText("After", { exact: true }).click();
    // [ts] expect(snapshot).toHaveText("bar");
    // [ts] traceFrame(page).getByText("Before", { exact: true }).click();
    // [ts] expect(snapshot).toHaveText("foo");
    // [ts] traceFrame(page).getByTestId("actions-tree").getByText("last").click();
    // [ts] traceFrame(page).getByText("After", { exact: true }).click();
    // [ts] expect(snapshot).toHaveText("baz");
    // [ts] traceFrame(page).getByText("Before", { exact: true }).click();
    // [ts] expect(snapshot).toHaveText("bar");
    // [ts] });
    // [ts] test("should show image diff", () => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] snapshotPathTemplate: '{arg}{ext}'
    // [ts] };
    // [ts] `,
    // [ts] "snapshot.png": createImage(100, 100, 255, 0, 0),
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('vrt test', () => {
    // [ts] page.setViewportSize({ width: 100, height: 100 });
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("vrt test").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Failed");
    // [ts] traceFrame(page).getByText("Attachments").click();
    // [ts] expect(traceFrame(page).getByText("Diff", { exact: true })).toBeVisible();
    // [ts] expect(traceFrame(page).getByText("Actual", { exact: true })).toBeVisible();
    // [ts] expect(traceFrame(page).getByText("Expected", { exact: true })).toBeVisible();
    // [ts] expect(traceFrame(page).getByTestId("test-result-image-mismatch").locator("img")).toBeVisible();
    // [ts] });
    // [ts] test("should show screenshot", () => {
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] use: {
    // [ts] screenshot: 'on',
    // [ts] viewport: { width: 100, height: 100 }
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('vrt test', () => {
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("vrt test").dblclick();
    // [ts] traceFrame(page).getByText("Attachments").click();
    // [ts] expect(traceFrame(page).getByText("Screenshots", { exact: true })).toBeVisible();
    // [ts] expect(traceFrame(page).locator(".attachment-item img")).toHaveCount(1);
    // [ts] });
    // [ts] test("should not fail on internal page logs", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] const context = browser.newContext({ storageState: { cookies: [], origins: [] } });
    // [ts] const page = context.newPage();
    // [ts] page.goto("${server.EMPTY_PAGE}");
    // [ts] page.context().storageState({ path: testInfo.outputPath('storage.json') });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("pass").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Create context",
    // [ts] "Create page",
    // [ts] "Navigate.*\\/empty.html",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] });
    // [ts] test("should not show caught errors in the errors tab", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent("<input id='checkbox' type='checkbox'></input>");
    // [ts] expect(page.locator('input')).toBeChecked({ timeout: 1 }).catch(() => {});
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("pass").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Set content",
    // [ts] "Expect \"toBeChecked\".*locator",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] traceFrame(page).getByText("Source", { exact: true }).click();
    // [ts] expect(traceFrame(page).locator(".source-line-running")).toContainText("toBeChecked");
    // [ts] expect(traceFrame(page).locator(".CodeMirror-linewidget")).toHaveCount(0);
    // [ts] traceFrame(page).getByText("Errors", { exact: true }).click();
    // [ts] expect(traceFrame(page).locator(".tab-errors")).toHaveText("No errors");
    // [ts] });
    // [ts] test("should show errors with causes in the error tab", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] try {
    // [ts] try {
    // [ts] const error = new Error('my-message');
    // [ts] error.name = 'SpecialError';
    // [ts] throw error;
    // [ts] } catch (e) {
    // [ts] try {
    // [ts] throw new Error('inner-message', { cause: e });
    // [ts] } catch (e) {
    // [ts] throw new Error('outer-message', { cause: e });
    // [ts] }
    // [ts] }
    // [ts] } catch (e) {
    // [ts] throw new Error('wrapper-message', { cause: e });
    // [ts] }
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("pass").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Failed");
    // [ts] traceFrame(page).getByText("Errors", { exact: true }).click();
    // [ts] expect(traceFrame(page).locator(".tab-errors")).toContainText(`Error: wrapper-message
    // [ts] [cause]: Error: outer-message
    // [ts] [cause]: Error: inner-message
    // [ts] [cause]: SpecialError: my-message`);
    // [ts] });
    // [ts] test("should reveal errors in the sourcetab", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] throw new Error('Oh my');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("pass").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toContainText([
    // [ts] "Before Hooks",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] traceFrame(page).getByText("Errors", { exact: true }).click();
    // [ts] traceFrame(page).getByText("a.spec.ts:4", { exact: true }).click();
    // [ts] expect(traceFrame(page).locator(".source-line-running")).toContainText(`throw new Error('Oh my');`);
    // [ts] });
    // [ts] test("should show request source context id", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.goto('${server.EMPTY_PAGE}');
    // [ts] const page2 = context.newPage();
    // [ts] page2.goto('${server.EMPTY_PAGE}');
    // [ts] request.get('${server.EMPTY_PAGE}');
    // [ts] console.log('log from node');
    // [ts] console.error('error from node');
    // [ts] page.evaluate(() => console.log('here from page1'));
    // [ts] page2.evaluate(() => console.log('here from page2'));
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("pass").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
    // [ts] traceFrame(page).getByText("Network", { exact: true }).click();
    // [ts] expect(traceFrame(page).locator("span").filter({ hasText: "Source" })).toBeVisible();
    // [ts] expect(traceFrame(page).getByText("page#1")).toBeVisible();
    // [ts] expect(traceFrame(page).getByText("page#2")).toBeVisible();
    // [ts] expect(traceFrame(page).getByText("api#1")).toBeVisible();
    // [ts] traceFrame(page).getByText("Console", { exact: true }).click();
    // [ts] const consoleLines = traceFrame(page).getByRole("tabpanel", { name: "Console" }).getByRole("option");
    // [ts] expect(consoleLines.filter({ hasText: "log from node" }).locator(".console-source")).toHaveText("test");
    // [ts] expect(consoleLines.filter({ hasText: "error from node" }).locator(".console-source")).toHaveText("test");
    // [ts] expect(consoleLines.filter({ hasText: "here from page1" }).locator(".console-source")).toHaveText("page#1");
    // [ts] expect(consoleLines.filter({ hasText: "here from page2" }).locator(".console-source")).toHaveText("page#2");
    // [ts] });
    // [ts] test("should work behind reverse proxy", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33705" } }, (_p) => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('trace test', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] page.getByRole('button').click();
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const uiModeUrl = URL.init(page.url());
    // [ts] reverseProxy.forwardTo(uiModeUrl.port, { removePrefix: "/subdir", preserveHostname: true });
    // [ts] page.goto(`${reverseProxy.URL}/subdir${uiModeUrl.pathname}?${uiModeUrl.searchParams}`);
    // [ts] page.getByText("trace test").dblclick();
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem /Before Hooks/
    // [ts] - treeitem /Set content/
    // [ts] - treeitem /Click.*getByRole/
    // [ts] - treeitem /Expect "toBe"/
    // [ts] - treeitem /After Hooks/
    // [ts] `);
    // [ts] expect(
    // [ts] traceFrame(page).frameLocator("iframe.snapshot-visible[name=snapshot]").locator("button"),
    // [ts] ).toHaveText("Submit");
    // [ts] });
    // [ts] test("should filter actions tab on double-click", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.goto('${server.EMPTY_PAGE}');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("pass").dblclick();
    // [ts] const actionsTree = traceFrame(page).getByTestId("actions-tree");
    // [ts] expect(actionsTree.getByRole("treeitem")).toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Navigate.*\\/empty.html",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] actionsTree.getByRole("treeitem", { name: "Navigate" }).dblclick();
    // [ts] expect(actionsTree.getByRole("treeitem")).toHaveText([
    // [ts] "Navigate.*\\/empty.html",
    // [ts] ]);
    // [ts] });
    // [ts] test("should show custom fixture titles in actions tree", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture1: [({}, use) => {
    // [ts] use();
    // [ts] }, { title: 'My Custom Fixture' }],
    // [ts] fixture2: ({}, use) => {
    // [ts] use();
    // [ts] },
    // [ts] });
    // [ts] test('fixture test', () => {
    // Empty test using both fixtures
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("fixture test").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(listItem, "action list").toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Fixture \"My Custom Fixture\"",
    // [ts] "Fixture \"fixture2\"",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] });
    // [ts] test("should collapse log entries to a single line", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('multiline test', () => {
    // [ts] page.keyboard.type(\`line1
    // [ts] line2\`);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("multiline test").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(listItem, "action list").toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Type \"line1\\\\nline2\"",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] });
    // [ts] test("should hide boxed fixtures and contents, reveal upon show all actions setting", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: [({ page }, use) => {
    // [ts] page.setContent('<div>hello</div>');
    // [ts] use();
    // [ts] }, { auto: true, boxed: true }],
    // [ts] });
    // [ts] test('example', () => {
    // [ts] expect(page.locator('body')).toBeVisible();
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("example").dblclick();
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem /Before Hooks/
    // [ts] - treeitem /Expect "toBeVisible"/
    // [ts] - treeitem /After Hooks/
    // [ts] `);
    // [ts] traceFrame(page).getByRole("button", { name: "Filter actions" }).click();
    // [ts] traceFrame(page).locator(".setting").getByText("Network routes").click();
    // [ts] traceFrame(page).locator(".setting").getByText("Getters").click();
    // [ts] traceFrame(page).locator(".setting").getByText("Configuration").click();
    // [ts] traceFrame(page).getByRole("button", { name: "Filter actions" }).click();
    // [ts] traceFrame(page).getByTestId("actions-tree").getByRole("treeitem", { name: "Before Hooks" }).locator(".codicon-chevron-right").click();
    // [ts] traceFrame(page).getByTestId("actions-tree").getByRole("treeitem", { name: "Fixture \"fixture\"" }).locator(".codicon-chevron-right").click();
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem /Before Hooks/:
    // [ts] - group:
    // [ts] - treeitem /Fixture "browser"/
    // [ts] - treeitem /Fixture "context"/
    // [ts] - treeitem /Fixture "page"/
    // [ts] - treeitem /Fixture "fixture"/:
    // [ts] - group:
    // [ts] - treeitem /Set content/
    // [ts] - treeitem /Expect "toBeVisible"/
    // [ts] - treeitem /After Hooks/
    // [ts] `);
    // [ts] });
    // [ts] test("attachments tab shows all but top-level .push attachments", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('attachment test', () => {
    // [ts] test.step('step', () => {
    // [ts] test.info().attachments.push({
    // [ts] name: 'foo-push',
    // [ts] body: Buffer.from('foo-content'),
    // [ts] contentType: 'text/plain'
    // [ts] });
    // [ts] test.info().attach('foo-attach', { body: 'foo-content' })
    // [ts] });
    // [ts] test.info().attachments.push({
    // [ts] name: 'bar-push',
    // [ts] body: Buffer.from('bar-content'),
    // [ts] contentType: 'text/plain'
    // [ts] });
    // [ts] test.info().attach('bar-attach', { body: 'bar-content' })
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByRole("treeitem", { name: "attachment test" }).dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
    // [ts] const actionsTree = traceFrame(page).getByTestId("actions-tree");
    // [ts] actionsTree.getByRole("treeitem", { name: "step" }).click();
    // [ts] page.keyboard.press("ArrowRight");
    // [ts] expect(traceFrame(page).locator("body"), "attach() and top-level attachments.push calls are shown as actions").toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem /step/:
    // [ts] - group:
    // [ts] - treeitem /Attach \\"foo-attach\\"/
    // [ts] - treeitem /Attach \\"bar-attach\\"/
    // [ts] `);
    // [ts] traceFrame(page).getByRole("tab", { name: "Attachments" }).click();
    // [ts] expect(traceFrame(page).getByRole("tabpanel", { name: "Attachments" })).toMatchAriaSnapshot(`
    // [ts] - tabpanel:
    // [ts] - button /foo-push/
    // [ts] - button /foo-attach/
    // [ts] - button /bar-push/
    // [ts] - button /bar-attach/
    // [ts] `);
    // [ts] });
    // [ts] test("skipped steps should have an indicator", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test with steps', () => {
    // [ts] test.step('outer', () => {
    // [ts] test.step.skip('skipped1', () => {});
    // [ts] });
    // [ts] test.step.skip('skipped2', () => {});
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByRole("treeitem", { name: "test with steps" }).dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
    // [ts] const actionsTree = traceFrame(page).getByTestId("actions-tree");
    // [ts] actionsTree.getByRole("treeitem", { name: "outer" }).click();
    // [ts] page.keyboard.press("ArrowRight");
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem /outer/ [expanded]:
    // [ts] - group:
    // [ts] - treeitem /skipped1/
    // [ts] - treeitem /skipped2/
    // [ts] `);
    // [ts] const skippedMarker = actionsTree.getByRole("treeitem", { name: "skipped1" }).locator(".action-skipped");
    // [ts] expect(skippedMarker).toBeVisible();
    // [ts] expect(skippedMarker).toHaveAccessibleName("skipped");
    // [ts] });
    // [ts] test("should show copy prompt button in errors tab", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `.trim(),
    // [ts] });
    // [ts] page.getByText("fails").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Failed");
    // The trace viewer frame can write to the clipboard, but it cannot read it.
    // [ts] const traceBody = traceFrame(page).locator("body");
    // [ts] traceBody.evaluate(() => {
    // [ts] const writeText = navigator.clipboard.writeText.bind(navigator.clipboard);
    // [ts] navigator.clipboard.writeText = (text) => {
    // [ts] writeText(text);
    // [ts] (window).__clipboardCall = text;
    // [ts] };
    // [ts] });
    // [ts] page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
    // [ts] traceFrame(page).getByText("Errors", { exact: true }).click();
    // [ts] traceFrame(page).locator(".tab-errors").getByRole("button", { name: "Copy prompt" }).click();
    // [ts] expect.poll(() => traceBody.evaluate(() => (window).__clipboardCall)).toBeTruthy();
    // [ts] const prompt = traceBody.evaluate(() => (window).__clipboardCall);
    // [ts] expect_eq(std.mem.indexOf(u8, prompt, "contains error", "expect(received).toBe(expected)") != null, true);
    // [ts] expect(prompt.replaceAll("\r\n", "\n"), "contains test sources").toContain(`
    // [ts] 1 | import { test, expect } from '@playwright/test';
    // [ts] 2 | test('fails', ({ page }) => {
    // [ts] 3 |   page.setContent('<button>Submit</button>');
    // [ts] > 4 |   expect_eq(1, 2);
    // [ts] |             ^ Error: expect_eq(received, expected) // Object.is equality
    // [ts] 5 | });
    // [ts] `.trim());
    // [ts] });
    // [ts] test("should indicate current test status", () => {
    // [ts] const latch = createLatch();
    // [ts] const { page } = runUITest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('basic pass', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] ${latch.blockingCode}
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] test('basic fail', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `.trim(),
    // [ts] });
    // [ts] page.getByTestId("test-tree").getByText("basic pass").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Running");
    // [ts] latch.open();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
    // [ts] page.getByTestId("test-tree").getByText("basic fail").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Failed");
    // [ts] });
    // [ts] test("should be able to create and dispose APIRequestContext inside Promise.all", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, request } from '@playwright/test';
    // [ts] test('create api request contexts', () => {
    // [ts] Promise.all(Array.from({ length: 100 }).map(() => {
    // [ts] let delay = Math.floor(Math.random() * 501);
    // [ts] new Promise(res => setTimeout(res, delay));
    // [ts] const apiContext = request.newContext();
    // [ts] delay = Math.floor(Math.random() * 501);
    // [ts] new Promise(res => setTimeout(res, delay));
    // [ts] apiContext.dispose();
    // [ts] }));
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("create api request contexts").dblclick();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] traceFrame(page).getByText("Errors", { exact: true }).click();
    // [ts] expect(traceFrame(page).locator(".tab-errors")).toHaveText("No errors");
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(
    // [ts] listItem,
    // [ts] "action list"
    // [ts] ).toHaveText([
    // [ts] "Before Hooks",
    // [ts] ...Array.from({ length: 100 }).map(() => /Create request context/),
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] });
    // [ts] test("should partition action tree state by test", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test1', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] page.evaluate('1+1');
    // [ts] });
    // [ts] test('test2', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] page.evaluate('1+1');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByTitle("Run all").click();
    // [ts] page.getByTestId("test-tree").getByText("test1").click();
    // [ts] expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
    // [ts] const actionsTree = traceFrame(page).getByTestId("actions-tree");
    // [ts] actionsTree.getByRole("treeitem", { name: "After Hooks" }).click();
    // [ts] page.keyboard.press("ArrowRight");
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - treeitem /After Hooks/ [expanded] [selected]:
    // [ts] - group:
    // [ts] - treeitem /Fixture \"page\"/
    // [ts] - treeitem /Fixture \"context\"/
    // [ts] `);
    // [ts] page.getByTestId("test-tree").getByText("test2").click();
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - treeitem /Evaluate/ [selected]
    // [ts] - treeitem /After Hooks/ [expanded=false]:
    // [ts] - /children: equal
    // [ts] `);
    // [ts] page.getByTestId("test-tree").getByText("test1").click();
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - treeitem /Evaluate/ [selected=false]
    // [ts] - treeitem /After Hooks/ [expanded] [selected]:
    // [ts] - group:
    // [ts] - treeitem /Fixture \"page\"/
    // [ts] - treeitem /Fixture \"context\"/
    // [ts] `);
    // [ts] });
    // [ts] test("should update state on subsequent run", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test1', () => {
    // [ts] page.evaluate('1+1');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByTestId("test-tree").getByText("test1").click();
    // [ts] page.keyboard.press("Enter");
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - treeitem /Evaluate/ [selected]
    // [ts] `);
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] page.evaluate('1+1');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.keyboard.press("Enter");
    // [ts] expect(traceFrame(page).locator("body")).toMatchAriaSnapshot(`
    // [ts] - treeitem /Expect \"toBe\"/
    // [ts] `);
    // [ts] });
    // [ts] test("should load trace when outputDir is outside cwd", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40950" },
    // [ts] }, (_p) => {
    // [ts] const outputDir = fs.promises.mkdtemp(path.join(os.tmpdir(), "pw-outputdir-"));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const { page } = runUITest({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] outputDir: ${JSON.stringify(outputDir)},
    // [ts] });
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('trace test', () => {
    // [ts] page.setContent('<button>Submit</button>');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("trace test").dblclick();
    // [ts] const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
    // [ts] expect(listItem, "action list").toHaveText([
    // [ts] "Before Hooks",
    // [ts] "Set content",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] fs.promises.rm(outputDir, { recursive: true, force: true });
    // [ts] }
    // [ts] });
}
