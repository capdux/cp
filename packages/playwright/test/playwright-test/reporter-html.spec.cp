// Migrated from TypeScript (playwright-test/reporter-html.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import path from "cap:path";
import url from "cap:url";
import * as yazl from "yazl";
import { test as baseTest, expect as baseExpect, cliCommand, createImage } from "./playwright-test-fixtures.cp";
import { iso, utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { mergeServiceWorkerCoverage } from "../config/serviceWorkerCoverage.cp";

type HttpServer = utils.HttpServer;

const { msToString } = iso;
const { spawnAsync } = utils;

// TODO(cap-migrate): refine TS statement
// [ts] const test = baseTest.extend({
// [ts] page: (_p, use) => {
// [ts] use(page);
// Before the page is collected.
// [ts] mergeServiceWorkerCoverage(page);
// [ts] },
// [ts] showReport: (_p, use, testInfo) => {
// [ts] let server: HttpServer | null;
// [ts] use((reportFolder) => {
// [ts] reportFolder ??=  testInfo.outputPath("playwright-report");
// [ts] server = utils.serveFolder(reportFolder);
// [ts] server.start();
// [ts] page.goto(server.urlPrefix("precise"));
// [ts] });
// [ts] server?.stop();
// [ts] }
// [ts] });

test.use({ channel: "chrome" });
test.slow(!!process.env.CI);
// Slow tests are 90s.
// TODO(cap-migrate): refine TS statement
// [ts] const expect = baseExpect.configure({ timeout: process.env.CI ? 75000 : 25000 });

test.describe.configure({ mode: "parallel" });

// TODO(cap-migrate): refine TS statement
// [ts] for (const useIntermediateMergeReport of [true, false]) { test.describe(`${if (useIntermediateMergeReport) "merged" else "created"}`, () => { test.use({ useIntermediateMergeReport });  test("should generate report", (_p) => { runInlineTest({ "playwright.config.ts": ` export default { name: 'project-name' }; `, "a.test.js": ` import { test, expect } from '@playwright/test'; test('passes', async ({}) => {}); test('fails', async ({}) => { expect(1).toBe(2); }); test('skipped', async ({}) => { test.skip('Does not work') }); test('flaky', async ({}, testInfo) => { expect(testInfo.retry).toBe(1); }); `, }, { reporter: "dot,html", retries: 1 }, { PLAYWRIGHT_HTML_OPEN: "never" });  showReport();
// [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("3");
// [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("1");
// [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("1");
// [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("1");
// [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("1");
// [ts] expect(page.locator(".test-file-test-outcome-unexpected >> text=fails")).toBeVisible();
// [ts] expect(page.locator(".test-file-test-outcome-flaky >> text=flaky")).toBeVisible();
// [ts] expect(page.locator(".test-file-test-outcome-expected >> text=passes")).toBeVisible();
// [ts] expect(page.locator(".test-file-test-outcome-skipped >> text=skipped")).not.toBeVisible();
// [ts] expect(page.getByTestId("overall-duration"), "should contain humanized total time with at most 1 decimal place").toContainText("^Total time: \\d+(\\.\\d)?(ms|s|m)$");
// [ts] expect(page.getByTestId("project-name"), "should contain project name").toContainText("project-name");
// [ts] expect(page.getByTestId("metadata-view")).not.toBeVisible();
// [ts] });
// [ts] test("speedboard \"Show more\" button should show the remaining test count", () => {
// [ts] const tests: []string = [];
// [ts] let i = 1;
// [ts] while (i <= 57) // [cap-migrate] for-step: ++i
// [ts] tests.push(`test('test ${String(i).padStart(2, "0")}', async () => { await new Promise(f => setTimeout(f, ${i})); });`);
// [ts] runInlineTest({
// [ts] "a.spec.ts": `
// [ts] import { test } from '@playwright/test';
// [ts] ${tests.join('\n          ')}
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] showReport();
// [ts] page.getByRole("link", { name: "Speedboard" }).click();
// 57 tests with the initial page size of 50 → exactly 7 remain.
// [ts] const button = page.locator("button", { hasText: "Show \\d+ more" });
// [ts] expect(button).toHaveCount(1);
// [ts] expect(button).toContainText("Show 7 more");
// Revealing the remaining tests removes the button.
// [ts] button.click();
// [ts] expect(page.locator("button", { hasText: "Show \\d+ more" })).toHaveCount(0);
// [ts] });
// [ts] test("should allow navigating to testId=test.id", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] console.log('TESTID=' + test.info().testId);
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// Note: using PW_TEST_HTML_REPORT_OPEN to test backwards compatibility.
// [ts] }, { reporter: "dot,html" }, { PW_TEST_HTML_REPORT_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] page.locator("text=stdout").click();
// [ts] expect(page.locator(".attachment-body")).toHaveText("TESTID=.*");
// [ts] const idString = page.locator(".attachment-body").textContent();
// [ts] const testId = idString.match("TESTID=(.*)")[1];
// [ts] expect_eq(std.mem.indexOf(u8, page.url(), "testId=" + testId) != null, true);
// Expect test to be opened.
// [ts] page.reload();
// [ts] page.locator("text=stdout").click();
// [ts] expect(page.locator(".attachment-body")).toHaveText("TESTID=.*");
// [ts] });
// [ts] test("should not throw when PLAYWRIGHT_HTML_OPEN value is invalid", () => {
// [ts] const invalidOption = "invalid-option";
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { preserveOutput: 'failures-only' };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] expect_eq(2, 2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: invalidOption });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] });
// [ts] test("should not throw when attachment is missing", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { preserveOutput: 'failures-only' };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] const screenshot = testInfo.outputPath('screenshot.png');
// [ts] page.screenshot({ path: screenshot });
// [ts] testInfo.attachments.push({ name: 'screenshot', path: screenshot, contentType: 'image/png' });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] expect(page.getByRole("link", { name: "screenshot" })).toBeVisible();
// [ts] });
// [ts] test("should include image diff", () => {
// [ts] const expected = Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAMgAAADICAYAAACtWK6eAAAAAXNSR0IArs4c6QAAAhVJREFUeJzt07ERwCAQwLCQ/Xd+FuDcQiFN4MZrZuYDjv7bAfAyg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAiEDVPZBYx6ffy+AAAAAElFTkSuQmCC", "base64");
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { use: { viewport: { width: 200, height: 200 }} };
// [ts] `,
// [ts] "a.test.js-snapshots/expected-linux.png": expected,
// [ts] "a.test.js-snapshots/expected-darwin.png": expected,
// [ts] "a.test.js-snapshots/expected-win32.png": expected,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] page.setContent('<html>Hello World</html>');
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toMatchSnapshot('expected.png');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] expect(page.locator("text=Image mismatch")).toBeVisible();
// [ts] expect(page.locator("text=Snapshot mismatch")).toHaveCount(0);
// [ts] expect(page.getByTestId("test-screenshot-error-view").getByTestId("test-result-image-mismatch-tabs").getByRole("tab")).toHaveText([
// [ts] "Diff",
// [ts] "Actual",
// [ts] "Expected",
// [ts] "Side by side",
// [ts] "Slider",
// [ts] "Blink",
// [ts] ]);
// [ts] for (const testId of ["test-results-image-diff", "test-screenshot-error-view"]) {
// [ts] test.step(testId, () => {
// [ts] const imageDiff = page.getByTestId(testId).getByTestId("test-result-image-mismatch");
// [ts] test.step("Diff", () => {
// [ts] expect(imageDiff.locator("img")).toHaveAttribute("alt", "Diff");
// [ts] });
// [ts] test.step("Actual", () => {
// [ts] imageDiff.getByText("Actual", { exact: true }).click();
// [ts] expect(imageDiff.locator("img")).toHaveAttribute("alt", "Actual");
// [ts] });
// [ts] test.step("Expected", () => {
// [ts] imageDiff.getByText("Expected", { exact: true }).click();
// [ts] expect(imageDiff.locator("img")).toHaveAttribute("alt", "Expected");
// [ts] });
// [ts] test.step("Side by side", () => {
// [ts] imageDiff.getByText("Side by side").click();
// [ts] expect(imageDiff.locator("img")).toHaveCount(2);
// [ts] expect(imageDiff.locator("img").first()).toHaveAttribute("alt", "Expected");
// [ts] expect(imageDiff.locator("img").last()).toHaveAttribute("alt", "Actual");
// [ts] imageDiff.locator("img").last().click();
// [ts] expect(imageDiff.locator("img").last()).toHaveAttribute("alt", "Diff");
// [ts] });
// [ts] test.step("Slider", () => {
// [ts] imageDiff.getByText("Slider", { exact: true }).click();
// [ts] expect(imageDiff.locator("img")).toHaveCount(2);
// [ts] expect(imageDiff.locator("img").first()).toHaveAttribute("alt", "Expected");
// [ts] expect(imageDiff.locator("img").last()).toHaveAttribute("alt", "Actual");
// [ts] });
// [ts] test.step("Blink", () => {
// [ts] imageDiff.getByRole("tab", { name: "Blink" }).click();
// [ts] expect.poll(() => imageDiff.locator("img").getAttribute("alt")).toBe("Actual");
// [ts] expect.poll(() => imageDiff.locator("img").getAttribute("alt")).toBe("Expected");
// [ts] });
// [ts] });
// [ts] }
// [ts] });
// [ts] test("should include multiple image diffs", () => {
// [ts] const IMG_WIDTH = 200;
// [ts] const IMG_HEIGHT = 200;
// [ts] const redImage = createImage(IMG_WIDTH, IMG_HEIGHT, 255, 0, 0);
// [ts] const whiteImage = createImage(IMG_WIDTH, IMG_HEIGHT, 255, 255, 255);
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] snapshotPathTemplate: '__screenshots__/{testFilePath}/{arg}{ext}',
// [ts] use: { viewport: { width: ${IMG_WIDTH}, height: ${IMG_HEIGHT} }}
// [ts] };
// [ts] `,
// [ts] "__screenshots__/a.test.js/fails-1.png": redImage,
// [ts] "__screenshots__/a.test.js/fails-2.png": whiteImage,
// [ts] "__screenshots__/a.test.js/fails-3.png": redImage,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] testInfo.snapshotSuffix = '';
// [ts] expect.soft(page).toHaveScreenshot({ timeout: 1000 });
// [ts] expect.soft(page).toHaveScreenshot({ timeout: 1000 });
// [ts] expect.soft(page).toHaveScreenshot({ timeout: 1000 });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] expect(page.locator("text=Image mismatch")).toHaveCount(2);
// [ts] expect(page.locator("text=Snapshot mismatch")).toHaveCount(0);
// [ts] expect(page.locator("text=\"Screenshots\"")).toHaveCount(0);
// [ts] let i = 0;
// [ts] while (i < 2) { // [cap-migrate] for-step: ++i
// [ts] const imageDiff = page.locator("data-testid=test-result-image-mismatch").nth(i);
// [ts] const image = imageDiff.locator("img").first();
// [ts] expect(image).toHaveAttribute("src", ".*png");
// [ts] }
// [ts] });
// [ts] test("should include image diffs for same expectation", () => {
// [ts] const expected = Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAMgAAADICAYAAACtWK6eAAAAAXNSR0IArs4c6QAAAhVJREFUeJzt07ERwCAQwLCQ/Xd+FuDcQiFN4MZrZuYDjv7bAfAyg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAiEDVPZBYx6ffy+AAAAAElFTkSuQmCC", "base64");
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { use: { viewport: { width: 200, height: 200 }} };
// [ts] `,
// [ts] "a.test.js-snapshots/expected-linux.png": expected,
// [ts] "a.test.js-snapshots/expected-darwin.png": expected,
// [ts] "a.test.js-snapshots/expected-win32.png": expected,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] page.setContent('<html>Hello World</html>');
// [ts] const screenshot = page.screenshot();
// [ts] expect.soft(screenshot).toMatchSnapshot('expected.png');
// [ts] expect.soft(screenshot).toMatchSnapshot('expected.png');
// [ts] expect.soft(screenshot).toMatchSnapshot('expected.png');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] expect(page.locator(".test-error-view").first()).toContainText(
// [ts] `> 6 |             await expect.soft(screenshot).toMatchSnapshot('expected.png');`,
// [ts] );
// [ts] const imageDiffs = page.getByTestId("test-results-image-diff");
// [ts] expect(imageDiffs.getByTestId("test-result-image-mismatch")).toHaveCount(3);
// [ts] expect(imageDiffs.getByText("Image mismatch:")).toHaveText([
// [ts] "Image mismatch: expected.png",
// [ts] "Image mismatch: expected-1.png",
// [ts] "Image mismatch: expected-2.png",
// [ts] ]);
// [ts] });
// [ts] test("should not include image diff with non-images", () => {
// [ts] const expected = Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAMgAAADICAYAAACtWK6eAAAAAXNSR0IArs4c6QAAAhVJREFUeJzt07ERwCAQwLCQ/Xd+FuDcQiFN4MZrZuYDjv7bAfAyg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAgEg0AwCASDQDAIBINAMAiEDVPZBYx6ffy+AAAAAElFTkSuQmCC", "base64");
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { use: { viewport: { width: 200, height: 200 }} };
// [ts] `,
// [ts] "a.test.js-snapshots/expected-linux": expected,
// [ts] "a.test.js-snapshots/expected-darwin": expected,
// [ts] "a.test.js-snapshots/expected-win32": expected,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] page.setContent('<html>Hello World</html>');
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toMatchSnapshot('expected');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] expect(page.locator("text=Image mismatch")).toHaveCount(0);
// [ts] expect(page.locator("img")).toHaveCount(0);
// [ts] expect(page.locator("a", { hasText: "expected-actual" })).toBeVisible();
// [ts] expect(page.locator("a", { hasText: "expected-expected" })).toBeVisible();
// [ts] });
// [ts] test("should include screenshot on failure", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] use: {
// [ts] viewport: { width: 200, height: 200 },
// [ts] screenshot: 'only-on-failure',
// [ts] }
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] page.setContent('<html>Failed state</html>');
// [ts] expect_eq(true, false);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] expect(page.locator("text=Screenshots")).toBeVisible();
// [ts] expect(page.locator("img")).toBeVisible();
// [ts] const src = page.locator("img").getAttribute("src");
// [ts] expect_eq(src, true);
// [ts] });
// [ts] test("should use different path if attachments base url option is provided", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] use: {
// [ts] viewport: { width: 200, height: 200 },
// [ts] screenshot: 'on',
// [ts] video: 'on',
// [ts] trace: 'on',
// [ts] },
// [ts] reporter: [['html', { attachmentsBaseURL: 'https://some-url.com/' }], ['line']]
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] page.evaluate('2 + 2');
// [ts] });
// [ts] `
// [ts] }, {}, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] expect(page.locator("div").filter({ hasText: "^Screenshotsscreenshot$" }).getByRole("img")).toHaveAttribute("src", "(https:\\/\\/some-url\\.com\\/)[^/\\s]+?\\.[^/\\s]+");
// [ts] expect(page.getByRole("link", { name: "screenshot" })).toHaveAttribute("href", "(https:\\/\\/some-url\\.com\\/)[^/\\s]+?\\.[^/\\s]+");
// [ts] expect(page.locator("video").locator("source")).toHaveAttribute("src", "(https:\\/\\/some-url\\.com\\/)[^/\\s]+?\\.[^/\\s]+");
// [ts] expect(page.getByRole("link", { name: "video" })).toHaveAttribute("href", "(https:\\/\\/some-url\\.com\\/)[^/\\s]+?\\.[^/\\s]+");
// [ts] expect(page.getByRole("link", { name: "trace", exact: true })).toHaveAttribute("href", "(https:\\/\\/some-url\\.com\\/)[^/\\s]+?\\.[^/\\s]+");
// [ts] expect(page.locator("div").filter({ hasText: "^Tracestrace$" }).getByRole("link").first()).toHaveAttribute("href", "trace=(https:\\/\\/some-url\\.com\\/)[^/\\s]+?\\.[^/\\s]+");
// [ts] });
// [ts] test("should display report title if provided", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['html', { title: 'Custom report title' }], ['line']]
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] page.evaluate('2 + 2');
// [ts] });
// [ts] `
// [ts] }, {}, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] expect(page.locator(".header-title")).toHaveText("Custom report title");
// [ts] });
// [ts] test("should process URLs as links in report title", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['html', { title: 'Custom report title https://playwright.dev separator http://microsoft.com end' }], ['line']]
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `
// [ts] }, {}, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 0);
// [ts] showReport();
// [ts] const anchorLocator = page.locator(".header-title a");
// [ts] expect(page.locator(".header-title")).toHaveText("Custom report title https://playwright.dev separator http://microsoft.com end");
// [ts] expect(anchorLocator).toHaveCount(2);
// [ts] expect(anchorLocator.nth(0)).toHaveAttribute("href", "https://playwright.dev");
// [ts] expect(anchorLocator.nth(1)).toHaveAttribute("href", "http://microsoft.com");
// [ts] });
// [ts] test("should allow setting title from env in global teardown", () => {
// [ts] test(useIntermediateMergeReport, "env vars are not available in merge report");
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] globalTeardown: './global-teardown.js',
// [ts] };
// [ts] `,
// [ts] "omega-star.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] import fs from 'fs/promises';
// [ts] test('version check', () => {
// [ts] const apiVersion = 'abcde';
// [ts] fs.writeFile(testInfo.outputPath('omega_star_version'), apiVersion);
// [ts] expect_eq(2, 2);
// [ts] });
// [ts] `,
// [ts] "global-teardown.js": `
// [ts] import fs from 'fs/promises';
// [ts] import path from 'path';
// [ts] export default (config) => {
// [ts] const apiVersion = fs.readFile(path.join('test-results', 'omega-star-version-check', 'omega_star_version'), 'utf-8');
// [ts] process.env.PLAYWRIGHT_HTML_TITLE = 'Omega Star Test Suite (Version: ' + apiVersion + ')';
// [ts] };
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] expect(page.locator(".header-title")).toHaveText("Omega Star Test Suite (Version: abcde)");
// [ts] });
// [ts] test("should include view trace button in header", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { use: { trace: 'on' } };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] page.evaluate('2 + 2');
// [ts] const request = playwright.request.newContext();
// [ts] request.get('${server.EMPTY_PAGE}');
// [ts] request.dispose();
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] expect(page.getByRole("link", { name: "View Trace" })).toBeVisible();
// [ts] });
// [ts] test("should include stdio", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] console.log('First line');
// [ts] console.log('Second line');
// [ts] console.error('Third line');
// [ts] expect_eq(true, false);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] page.getByText("stdout").click();
// [ts] expect(page.locator(".attachment-body").nth(0)).toHaveText("First line\nSecond line");
// [ts] page.getByText("stderr").click();
// [ts] expect(page.locator(".attachment-body").nth(1)).toHaveText("Third line");
// [ts] });
// [ts] test("should include stdout/stderr in copy prompt", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] console.log('Output line 1');
// [ts] process.stdout.write('Output line 2\\n');
// [ts] console.error('Error line 1');
// [ts] process.stderr.write('Error line 2\\n');
// [ts] expect_eq(true, false);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] page.getByRole("button", { name: "Copy prompt" }).click();
// [ts] page.waitForFunction(() => navigator.clipboard.readText());
// [ts] const prompt = page.evaluate(() => navigator.clipboard.readText());
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "should contain stdout content", "Output line 1") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "should contain stdout content", "Output line 2") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "should contain stderr content", "Error line 1") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "should contain stderr content", "Error line 2") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "should contain stdout section", "# Stdout") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "should contain stderr section", "# Stderr") != null, true);
// [ts] });
// [ts] test("should highlight error", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] expect_eq(true, false);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] expect(page.locator(".test-error-view span:has-text(\"true\")").first()).toHaveCSS("color", "rgb(205, 49, 49)");
// [ts] });
// [ts] test("should render compound ANSI SGR codes", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40826" },
// [ts] }, (_p) => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] throw new Error('\\x1b[1;31mBOLD RED\\x1b[0m \\x1b[0;32mRESET GREEN\\x1b[0m');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] const boldRed = page.locator(".test-error-view span:has-text(\"BOLD RED\")").first();
// [ts] expect(boldRed).toHaveCSS("color", "rgb(205, 49, 49)");
// [ts] expect(boldRed).toHaveCSS("font-weight", "700");
// [ts] expect(page.locator(".test-error-view span:has-text(\"RESET GREEN\")").first()).toHaveCSS("color", "rgb(0, 188, 0)");
// [ts] });
// [ts] test("should show trace source", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { use: { trace: 'on' } };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] async function evaluateWrapper(page, expression) {
// [ts] page.evaluate(expression);
// [ts] }
// [ts] test('passes', () => {
// [ts] evaluateWrapper(page, '2 + 2');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] page.click("img");
// [ts] page.click(".action-title >> text=EVALUATE");
// [ts] page.getByRole("tab", { name: "Source" }).click();
// [ts] expect(page.locator(".CodeMirror-line")).toContainText([
// [ts] "import.*test",
// [ts] "page\\.evaluate"
// [ts] ]);
// [ts] expect(page.locator(".source-line-running")).toContainText("page.evaluate");
// [ts] expect(page.getByRole("listbox", { name: "Stack trace" }).getByRole("option")).toContainText([
// [ts] "a.test.js:[\\d]+",
// [ts] ]);
// [ts] expect(page.getByRole("listbox", { name: "Stack trace" }).getByRole("option", { selected: true })).toContainText("a.test.js");
// [ts] });
// [ts] test("should not show stack trace", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { use: { trace: 'on' } };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] page.evaluate('2 + 2');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] page.click("img");
// [ts] page.click(".action-title >> text=EVALUATE");
// [ts] page.click("text=Source");
// [ts] expect(page.locator(".CodeMirror-line")).toContainText([
// [ts] "import.*test",
// [ts] "page\\.evaluate"
// [ts] ]);
// [ts] expect(page.locator(".source-line-running")).toContainText("page.evaluate");
// [ts] expect(page.getByTestId("stack-trace-list")).toHaveCount(0);
// [ts] });
// [ts] test("should show trace title", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { use: { trace: 'on' } };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] page.evaluate('2 + 2');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] page.click("img");
// [ts] expect(page.locator(".progress-dialog")).toBeHidden();
// [ts] expect(page.locator(".workbench-loader > .workbench-loader-header > .title")).toHaveText("a.test.js:3 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº passes");
// [ts] });
// [ts] test("should show multi trace source", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { use: { trace: 'on' } };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] page.evaluate('2 + 2');
// [ts] const request = playwright.request.newContext();
// [ts] request.get('${server.EMPTY_PAGE}');
// [ts] request.dispose();
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// Expect one image-link to trace viewer and 2 separate download links
// [ts] expect(page.locator("img")).toHaveCount(1);
// [ts] expect(page.getByRole("link", { name: "trace", exact: true })).toBeVisible();
// [ts] page.click("img");
// [ts] page.click(".action-title >> text=EVALUATE");
// [ts] page.click("text=Source");
// [ts] expect(page.locator(".source-line-running")).toContainText("page.evaluate");
// [ts] page.click(".action-title >> text=GET");
// [ts] page.click("text=Source");
// [ts] expect(page.locator(".source-line-running")).toContainText("request.get");
// [ts] });
// [ts] test("should show a thumbnail for every trace attachment", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] for (const index of [1, 2]) {
// [ts] const context = browser.newContext();
// [ts] context.tracing.start({ screenshots: true, snapshots: true });
// [ts] const page = context.newPage();
// [ts] page.goto('${server.EMPTY_PAGE}');
// [ts] const tracePath = testInfo.outputPath('trace' + index + '.zip');
// [ts] context.tracing.stop({ path: tracePath });
// [ts] testInfo.attach('trace', { path: tracePath, contentType: 'application/zip' });
// [ts] context.close();
// [ts] }
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] const traces = page.locator(".chip").filter({ hasText: "Traces" });
// [ts] expect(traces.locator("img")).toHaveCount(2);
// [ts] expect(traces.getByRole("link", { name: "trace-1", exact: true })).toBeVisible();
// [ts] expect(traces.getByRole("link", { name: "trace-2", exact: true })).toBeVisible();
// [ts] const hrefs = traces.locator("a").filter({ has: page.locator("img") }).evaluateAll(links => links.map(link => link.getAttribute("href")));
// [ts] expect(hrefs).toHaveLength(2);
// [ts] for (const href of hrefs)
// [ts] expect(href.match("trace=")).toHaveLength(1);
// [ts] expect_eq(hrefs[0] != hrefs[1], true);
// [ts] traces.locator("img").first().click();
// [ts] expect(page.locator(".action-title").first()).toBeVisible();
// [ts] });
// [ts] test("trace should not hang when showing parallel api requests", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { use: { trace: 'on' } };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect, request } from '@playwright/test';
// [ts] test('log two contexts', function({ }) {
// [ts] const api1 = request.newContext();
// [ts] const api2 = request.newContext();
// [ts] Promise.all([
// [ts] api1.get('${server.EMPTY_PAGE}'),
// [ts] api1.get('${server.CROSS_PROCESS_PREFIX}/empty.html'),
// [ts] api2.get('${server.EMPTY_PAGE}'),
// [ts] api2.get('${server.CROSS_PROCESS_PREFIX}/empty.html'),
// [ts] ]);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "html,dot" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "View Trace" }).click();
// Trace viewer should not hang here when displaying parallal requests.
// [ts] const getActions = page.getByTestId("actions-tree").getByRole("treeitem", { name: "GET" });
// [ts] expect(getActions).toHaveCount(4);
// [ts] expect(getActions.first()).toContainText("/empty.html");
// [ts] getActions.nth(2).click();
// [ts] getActions.nth(1).click();
// [ts] getActions.nth(0).click();
// [ts] });
// [ts] test("should warn user when viewing via file:// protocol", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { use: { trace: 'on' } };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] page.evaluate('2 + 2');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] test.step("view via server", () => {
// [ts] showReport();
// [ts] page.getByRole("link", { name: "View Trace" }).click();
// [ts] expect(page.locator("#fallback-error")).toBeHidden();
// [ts] });
// [ts] test.step("view via local file://", () => {
// [ts] const reportFolder = testInfo.outputPath("playwright-report");
// [ts] page.goto(url.pathToFileURL(path.join(reportFolder, "index.html")).toString());
// [ts] page.getByRole("link", { name: "View Trace" }).click();
// [ts] expect(page.locator("dialog")).toBeVisible();
// [ts] expect(page.locator("#fallback-error")).toContainText("must be loaded over");
// [ts] });
// [ts] });
// [ts] test("should show failed and timed out steps and hooks", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { timeout: 3000 };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test.beforeAll(() => {
// [ts] console.log('beforeAll 1');
// [ts] });
// [ts] test.beforeAll(() => {
// [ts] console.log('beforeAll 2');
// [ts] });
// [ts] test.beforeEach(() => {
// [ts] console.log('beforeEach 1');
// [ts] });
// [ts] test.beforeEach(() => {
// [ts] console.log('beforeEach 2');
// [ts] });
// [ts] test.afterEach(() => {
// [ts] console.log('afterEach 1');
// [ts] });
// [ts] test.afterAll(() => {
// [ts] console.log('afterAll 1');
// [ts] });
// [ts] test('fails', () => {
// [ts] test.step('outer error', () => {
// [ts] test.step('inner error', () => {
// [ts] expect.soft(1).toBe(2);
// [ts] });
// [ts] });
// [ts] test.step('outer step', () => {
// [ts] test.step('inner step', () => {
// [ts] new Promise(() => {});
// [ts] });
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fails" }).click();
// [ts] page.click(".tree-item:has-text(\"outer error\") >> text=outer error");
// [ts] page.click(".tree-item:has-text(\"outer error\") >> .tree-item >> text=inner error");
// [ts] expect(page.locator(".tree-item:has-text(\"outer error\") svg.color-text-danger")).toHaveCount(3);
// [ts] expect(page.locator(".tree-item:has-text(\"toBe\"):not(:has-text(\"inner\"))")).toBeVisible();
// [ts] page.click("text=outer step");
// [ts] expect(page.locator(".tree-item:has-text(\"outer step\") svg.color-text-danger")).toHaveCount(2);
// [ts] expect(page.locator(".tree-item:has-text(\"inner step\") svg.color-text-danger")).toHaveCount(2);
// [ts] page.click("text=Before Hooks");
// [ts] expect(page.locator(".tree-item:has-text(\"Before Hooks\") .tree-item")).toContainText([
// [ts] "beforeAll hook",
// [ts] "beforeAll hook",
// [ts] "beforeEach hook",
// [ts] "beforeEach hook",
// [ts] ]);
// [ts] page.locator("text=beforeAll hook").nth(1).click();
// [ts] expect(page.locator("text=console.log('beforeAll 2');")).toBeVisible();
// [ts] page.click("text=After Hooks");
// [ts] expect(page.locator(".tree-item:has-text(\"After Hooks\") .tree-item")).toContainText([
// [ts] "afterEach hook",
// [ts] "afterAll hook",
// [ts] ]);
// [ts] });
// [ts] test("should filter steps", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('has steps', () => {
// [ts] test.step('click button', () => {});
// [ts] test.step('fill form', () => {
// [ts] test.step('enter username', () => {});
// [ts] test.step('enter password', () => {});
// [ts] });
// [ts] test.step('another form', () => {
// [ts] test.step('fill username', () => {});
// [ts] test.step('fill password', () => {});
// [ts] });
// [ts] test.step('submit form', () => {});
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "has steps" }).click();
// [ts] const filterInput = page.getByLabel("Filter steps");
// [ts] expect(filterInput).toBeVisible();
// filter matching a subset of steps
// [ts] filterInput.fill("fill");
// [ts] expect(page.locator(".tree-item-title", { hasText: "fill form" })).toBeVisible();
// [ts] expect(page.locator(".tree-item-title", { hasText: "click button" })).toBeHidden();
// [ts] expect(page.locator(".tree-item-title", { hasText: "submit form" })).toBeHidden();
// matching parent is not auto-expanded when it has no matching children
// [ts] expect(page.locator(".tree-item-title", { hasText: "enter username" })).toBeHidden();
// [ts] expect(page.locator(".tree-item-title", { hasText: "enter password" })).toBeHidden();
// non-matching parent is auto-expanded when it has a matching child
// [ts] expect(page.locator(".tree-item-title", { hasText: "fill username" })).toBeVisible();
// [ts] expect(page.locator(".tree-item-title", { hasText: "fill password" })).toBeVisible();
// clear filter restores all steps collapsed
// [ts] filterInput.clear();
// [ts] expect(page.locator(".tree-item-title", { hasText: "click button" })).toBeVisible();
// [ts] expect(page.locator(".tree-item-title", { hasText: "submit form" })).toBeVisible();
// children are collapsed again after clearing the filter
// [ts] expect(page.locator(".tree-item-title", { hasText: "fill username" })).toBeHidden();
// [ts] expect(page.locator(".tree-item-title", { hasText: "fill password" })).toBeHidden();
// [ts] });
// [ts] test("should highlight filter matches in step title and subtitle", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('has steps', () => {
// [ts] page.setContent('<button id=target>Click me</button>');
// [ts] page.click('#target');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "has steps" }).click();
// [ts] const filterInput = page.getByLabel("Filter steps");
// [ts] filterInput.fill("click");
// [ts] expect(page.locator(".step-title-highlight")).toHaveText(["Click"]);
// [ts] filterInput.fill("#target");
// [ts] expect(page.locator(".step-subtitle .step-title-highlight")).toHaveText(["#target"]);
// [ts] });
// [ts] test("should render test.step subtitle", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('has steps', () => {
// [ts] test.step('Add to cart', () => {}, { subtitle: 'SKU 42' });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "has steps" }).click();
// [ts] expect(page.locator(".step-title-container", { hasText: "Add to cart" })).toHaveAttribute("aria-label", "Add to cart SKU 42");
// [ts] expect(page.locator(".step-title-text", { hasText: "Add to cart" })).toHaveAttribute("title", "Add to cart SKU 42");
// [ts] expect(page.locator(".step-subtitle")).toHaveText("SKU 42");
// [ts] });
// [ts] test("should show step snippets from non-root", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] export default { testDir: './tests' };
// [ts] `,
// [ts] "tests/a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('example', () => {
// [ts] test.step('step title', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "example" }).click();
// [ts] page.click("text=step title");
// [ts] page.click("text=Expect \"toBe\"");
// [ts] expect(page.getByTestId("test-snippet")).toContainText([
// [ts] `await test.step('step title', async () => {`,
// [ts] "expect(1).toBe(1);",
// [ts] ]);
// [ts] });
// [ts] test("should show skipped step snippets", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] export default { testDir: './tests' };
// [ts] `,
// [ts] "tests/a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('example', () => {
// [ts] test.step.skip('skipped step title', () => {
// [ts] expect_eq(1, 1);
// [ts] test.step('inner step', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "example" }).click();
// [ts] page.click("text=skipped step title (skipped)");
// [ts] expect(page.getByTestId("test-snippet")).toContainText(`await test.step.skip('skipped step title', async () => {`);
// [ts] });
// [ts] test("step title should include skipped step description", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] export default { testDir: './tests' };
// [ts] `,
// [ts] "tests/a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('example', () => {
// [ts] test.step('step title', (step) => {
// [ts] expect_eq(1, 1);
// [ts] step.skip(true, 'conditional step.skip');
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "example" }).click();
// [ts] page.click("text=step title (skipped: conditional step.skip)");
// [ts] expect(page.getByTestId("test-snippet")).toContainText(`await test.step('step title', async (step) => {`);
// [ts] });
// [ts] test("should render annotations", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { timeout: 1500 };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('annotated test', () => {
// [ts] test.info().annotations.push({ type: 'issue', description: 'I am not interested in this test' });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "annotated test" }).click();
// [ts] expect(page.locator(".test-case-annotation")).toHaveText("issue: I am not interested in this test");
// [ts] });
// [ts] test("should not crash on falsy non-string annotation", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35469" } }, (_p) => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { timeout: 1500 };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('annotated test', () => {
// [ts] test.info().annotations.push({ type: 'issue', description: 0 });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "annotated test" }).click();
// [ts] expect(page.locator(".test-case-annotation")).toHaveText("issue: 0");
// [ts] });
// [ts] test("should render dynamic annotations at test result level", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { timeout: 1500, retries: 3 };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('annotated test', () => {
// [ts] test.info().annotations.push({ type: 'foo', description: 'retry #' + test.info().retry });
// [ts] test.info().annotations.push({ type: 'bar', description: 'static value' });
// [ts] throw new Error('fail');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "annotated test" }).click();
// [ts] page.getByRole("tab", { name: "Retry #1" }).click();
// [ts] expect(page.getByTestId("test-case-annotations")).toContainText("foo: retry #1");
// [ts] page.getByRole("tab", { name: "Retry #3" }).click();
// [ts] expect(page.getByTestId("test-case-annotations")).toContainText("foo: retry #3");
// [ts] expect(page.getByTestId("test-case-annotations").getByText("static value")).toHaveCount(1);
// [ts] });
// [ts] test("should render annotations as link if needed", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { timeout: 1500 };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('pass test', () => {
// [ts] test.info().annotations.push({ type: 'issue', description: '${server.EMPTY_PAGE}' });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByText("pass test").click();
// [ts] expect(page.locator(".test-case-annotation")).toHaveText(`issue: ${server.EMPTY_PAGE}`);
// [ts] const popupPromise = page.waitForEvent("popup");
// [ts] page.getByRole("link", { name: server.EMPTY_PAGE }).click();
// [ts] const popup = popupPromise;
// [ts] expect_eq(popup.url(), server.EMPTY_PAGE);
// [ts] });
// [ts] test("should render text attachments as text", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43555" },
// [ts] }, (_p) => {
// [ts] const result = runInlineTest({
// [ts] "log.txt": "stderr attachment",
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passing', () => {
// [ts] testInfo.attachments.push({
// [ts] name: 'example.txt',
// [ts] contentType: 'text/plain',
// [ts] body: Buffer.from('foo'),
// [ts] });
// [ts] testInfo.attachments.push({
// [ts] name: 'example.json',
// [ts] contentType: 'application/json',
// [ts] body: Buffer.from(JSON.stringify({ foo: 1 })),
// [ts] });
// [ts] testInfo.attachments.push({
// [ts] name: 'example-utf16.txt',
// [ts] contentType: 'text/plain, charset=utf16le',
// [ts] body: Buffer.from('utf16 encoded', 'utf16le'),
// [ts] });
// [ts] testInfo.attachments.push({
// [ts] name: 'example-null.txt',
// [ts] contentType: 'text/plain, charset=utf16le',
// [ts] body: null,
// [ts] });
// [ts] testInfo.attach('stdout', { body: 'stdout attachment' });
// [ts] testInfo.attach('stderr', { path: 'log.txt' });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByText("passing", { exact: true }).click();
// [ts] page.getByText("example.txt", { exact: true }).click();
// [ts] page.getByText("example.json", { exact: true }).click();
// [ts] page.getByText("example-utf16.txt", { exact: true }).click();
// [ts] page.getByText("stdout", { exact: true }).click();
// [ts] expect(page.locator(".attachment-body")).toHaveText(["foo", "{\"foo\":1}", "utf16 encoded", "stdout attachment"]);
// [ts] const downloadPromise = page.waitForEvent("download");
// [ts] page.getByRole("link", { name: "stderr", exact: true }).click();
// [ts] const download = downloadPromise;
// [ts] expect_eq(readAllFromStream(download.createReadStream()), Buffer.from("stderr attachment"));
// [ts] });
// [ts] test("should have link for opening HTML attachments in new tab", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/32281" });
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/35489" });
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import * as fs from 'fs/promises';
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passing', () => {
// [ts] testInfo.attach('axe-report.html', {
// [ts] contentType: 'text/html',
// [ts] body: '<h1>Axe Report</h1>',
// [ts] });
// [ts] const attachmentFile = testInfo.outputPath('foo.html');
// [ts] fs.writeFile(attachmentFile, '<h1>Hello World</h1>');
// [ts] testInfo.attach('foo.html', { path: attachmentFile });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByText("passing", { exact: true }).click();
// [ts] const [axeTab] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("popup"),
// [ts] page.getByText("axe-report.html", { exact: true }).click(),
// [ts] ]);
// [ts] expect(axeTab).toHaveURL("^blob:");
// [ts] expect(axeTab.getByText("Axe Report")).toBeVisible();
// [ts] const [fooTab] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("popup"),
// [ts] page.getByText("foo.html", { exact: true }).click(),
// [ts] ]);
// [ts] expect(fooTab.getByText("Hello World")).toBeVisible();
// [ts] });
// [ts] test("should use file-browser friendly extensions for buffer attachments based on contentType", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passing', () => {
// [ts] testInfo.attach('screenshot', { body: Buffer.from('d'), contentType: 'image/png' });
// [ts] testInfo.attach('some-pdf', { body: Buffer.from('foo'), contentType: 'application/pdf' });
// [ts] testInfo.attach('madeup-contentType', { body: Buffer.from('bar'), contentType: 'madeup' });
// [ts] testInfo.attach('screenshot-that-already-has-an-extension-with-madeup.png', { body: Buffer.from('a'), contentType: 'madeup' });
// [ts] testInfo.attach('screenshot-that-already-has-an-extension-with-correct-contentType.png', { body: Buffer.from('c'), contentType: 'image/png' });
// [ts] testInfo.attach('example.ext with spaces', { body: Buffer.from('b'), contentType: 'madeup' });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passing" }).click();
// [ts] const expectedAttachments = [
// [ts] ["screenshot", "screenshot.png", "3c363836cf4e16666669a25da280a1865c2d2874.png"],
// [ts] ["some-pdf", "some-pdf.pdf", "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33.pdf"],
// [ts] ["madeup-contentType", "madeup-contentType.dat", "62cdb7020ff920e5aa642c3d4066950dd1f01f4d.dat"],
// [ts] ["screenshot-that-already-has-an-extension-with-madeup.png", "screenshot-that-already-has-an-extension-with-madeup.png", "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8.png"],
// [ts] ["screenshot-that-already-has-an-extension-with-correct-contentType.png", "screenshot-that-already-has-an-extension-with-correct-contentType.png", "84a516841ba77a5b4648de2cd0dfcb30ea46dbb4.png"],
// [ts] ["example.ext with spaces", "example.ext with spaces", "e9d71f5ee7c92d6dc9e92ffdad17b8bd49418f98.ext-with-spaces"],
// [ts] ];
// [ts] for (const [visibleAttachmentName, downloadFileName, sha1] of expectedAttachments) {
// [ts] test.step(`should download ${visibleAttachmentName}`, () => {
// [ts] const downloadPromise = page.waitForEvent("download");
// [ts] page.getByRole("link", { name: visibleAttachmentName, exact: true }).click();
// [ts] const download = downloadPromise;
// [ts] expect_eq(download.suggestedFilename(), downloadFileName);
// [ts] expect_eq(readAllFromStream(download.createReadStream()), fs.promises.readFile(path.join(testInfo.outputPath("playwright-report"), "data", sha1)));
// [ts] });
// [ts] }
// [ts] const files = fs.promises.readdir(path.join(testInfo.outputPath("playwright-report"), "data"));
// [ts] expect(Set.init(files)).toEqual(Set.init([
// [ts] "3c363836cf4e16666669a25da280a1865c2d2874.png", // screenshot
// [ts] "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33.pdf", // some-pdf
// [ts] "62cdb7020ff920e5aa642c3d4066950dd1f01f4d.dat", // madeup-contentType
// [ts] "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8.png", // screenshot-that-already-has-an-extension-with-madeup.png
// [ts] "84a516841ba77a5b4648de2cd0dfcb30ea46dbb4.png", // screenshot-that-already-has-an-extension-with-correct-contentType.png
// [ts] "e9d71f5ee7c92d6dc9e92ffdad17b8bd49418f98.ext-with-spaces", // example.ext with spaces
// [ts] ]));
// [ts] });
// [ts] test("should link from attach step to attachment view", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passing', () => {
// [ts] for (let i = 0; i < 100; i++)
// [ts] testInfo.attach('foo-1', { body: 'bar' });
// [ts] testInfo.attach('foo-2', { body: 'bar' });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passing" }).click();
// [ts] const attachment = page.getByText("foo-2", { exact: true });
// [ts] expect(attachment).not.toBeInViewport();
// [ts] page.getByLabel(`attach "foo-2"`).getByTitle("reveal attachment").click();
// [ts] expect(attachment).toBeInViewport();
// [ts] page.reload();
// [ts] expect(attachment).toBeInViewport();
// [ts] });
// [ts] test("steps with internal attachments have links", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passing', () => {
// [ts] for (let i = 0; i < 100; i++)
// [ts] testInfo.attach('spacer', { body: 'content' });
// [ts] test.step('step', () => {
// [ts] testInfo.attachments.push({ name: 'attachment', body: 'content', contentType: 'text/plain' });
// [ts] })
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passing" }).click();
// [ts] const attachment = page.getByText("attachment", { exact: true });
// [ts] expect(attachment).not.toBeInViewport();
// [ts] page.getByLabel("step").getByTitle("reveal attachment").click();
// [ts] expect(attachment).toBeInViewport();
// [ts] });
// [ts] test("parent step shows indirect attachment indicator", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passing', () => {
// [ts] test.step('outer', () => {
// [ts] test.step('inner', () => {
// [ts] testInfo.attach('attachment', { body: 'content', contentType: 'text/plain' });
// [ts] });
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passing" }).click();
// Collapsed parents show the indirect indicator.
// [ts] expect(page.getByLabel("outer").getByLabel("contains attachment")).toBeVisible();
// Expand outer; inner still shows the indicator (the attached leaf is below it).
// [ts] page.getByLabel("outer").click();
// [ts] expect(page.getByLabel("inner").getByLabel("contains attachment")).toBeVisible();
// The indirect indicator is non-interactive (no link/button role).
// [ts] expect(page.getByLabel("outer").getByLabel("contains attachment")).not.toHaveAttribute("href", ".+");
// [ts] });
// [ts] test("step.attach have links", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passing test', () => {
// [ts] test.step('step', (step) => {
// [ts] step.attach('text attachment', { body: 'content', contentType: 'text/plain' });
// [ts] })
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "passing test" }).click();
// [ts] page.getByLabel("step").getByTitle("reveal attachment").click();
// [ts] page.getByText("text attachment", { exact: true }).click();
// [ts] expect(page.locator(".attachment-body")).toHaveText("content");
// [ts] });
// [ts] test("should highlight textual diff", () => {
// [ts] const result = runInlineTest({
// [ts] "helper.ts": `
// [ts] import { test as base } from '@playwright/test';
// [ts] export * from '@playwright/test';
// [ts] export const test = base.extend({
// [ts] auto: [ ({}, run, testInfo) => {
// [ts] testInfo.snapshotSuffix = '';
// [ts] run();
// [ts] }, { auto: true } ]
// [ts] });
// [ts] `,
// [ts] "a.spec.js-snapshots/snapshot.txt": `old`,
// [ts] "a.spec.js": `
// [ts] const { test, expect } = require('./helper');
// [ts] test('is a test', () => {
// [ts] expect('new').toMatchSnapshot('snapshot.txt');
// [ts] });
// [ts] `
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "is a test" }).click();
// [ts] expect(page.locator(".test-error-view").getByText("-old")).toHaveCSS("color", "rgb(0, 188, 0)");
// [ts] expect(page.locator(".test-error-view").getByText("+new", { exact: true })).toHaveCSS("color", "rgb(205, 49, 49)");
// [ts] });
// [ts] test("should highlight inline textual diff in toHaveText", () => {
// [ts] const result = runInlineTest({
// [ts] "a.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('is a test', () => {
// [ts] page.setContent('<div>begin inner end</div>');
// [ts] expect(page.locator('div')).toHaveText('inner', { timeout: 500 });
// [ts] });
// [ts] `
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "is a test" }).click();
// [ts] expect(page.locator(".test-error-view").getByText("begin ", { exact: true })).toHaveCSS("color", "rgb(246, 248, 250)");
// [ts] expect(page.locator(".test-error-view").getByText("begin ", { exact: true })).toHaveCSS("background-color", "rgb(205, 49, 49)");
// [ts] expect(page.locator(".test-error-view").getByText("inner", { exact: true })).toHaveCSS("color", "rgb(205, 49, 49)");
// [ts] expect(page.locator(".test-error-view").getByText("end ", { exact: true })).toHaveCSS("color", "rgb(246, 248, 250)");
// [ts] expect(page.locator(".test-error-view").getByText("end ", { exact: true })).toHaveCSS("background-color", "rgb(205, 49, 49)");
// [ts] });
// [ts] test("should differentiate repeat-each test cases", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/10859" });
// [ts] const result = runInlineTest({
// [ts] "a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => {
// [ts] if (testInfo.repeatEachIndex === 2)
// [ts] throw new Error('ouch');
// [ts] });
// [ts] `
// [ts] }, { "reporter": "dot,html", "repeat-each": 3 }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] showReport();
// [ts] page.getByText("sample").first().click();
// [ts] expect(page.getByText("ouch")).toHaveCount(2);
// [ts] page.getByText("All").first().click();
// [ts] page.getByText("sample").nth(1).click();
// [ts] expect(page.getByText("Before Hooks")).toBeVisible();
// [ts] expect(page.getByText("ouch")).toBeHidden();
// [ts] });
// [ts] test("should show repeatEachIndex annotation when non-zero", () => {
// [ts] const result = runInlineTest({
// [ts] "a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => {
// [ts] });
// [ts] `
// [ts] }, { "reporter": "dot,html", "repeat-each": 3 }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// First repeat (index 0) should not show repeatEachIndex annotation.
// [ts] page.getByText("sample").first().click();
// [ts] expect(page.locator(".test-case-annotation")).toBeHidden();
// [ts] page.goBack();
// Second repeat (index 1) should show repeatEachIndex annotation.
// [ts] page.getByText("sample").nth(1).click();
// [ts] expect(page.locator(".test-case-annotation")).toHaveText("repeatEachIndex: 1");
// [ts] page.goBack();
// Third repeat (index 2) should show repeatEachIndex annotation.
// [ts] page.getByText("sample").nth(2).click();
// [ts] expect(page.locator(".test-case-annotation")).toHaveText("repeatEachIndex: 2");
// [ts] });
// [ts] test("should group similar / loop steps", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/10098" });
// [ts] const result = runInlineTest({
// [ts] "a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => {
// [ts] for (let i = 0; i < 10; ++i)
// [ts] expect_eq(1, 1);
// [ts] for (let i = 0; i < 20; ++i)
// [ts] expect_eq(2, 2);
// [ts] });
// [ts] `
// [ts] }, { "reporter": "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.locator("text=sample").first().click();
// [ts] expect(page.locator(".tree-item-title")).toContainText([
// [ts] "Expect \"toBe\".*10",
// [ts] "Expect \"toEqual\".*20",
// [ts] ]);
// [ts] });
// [ts] test("show custom fixture titles", () => {
// [ts] const result = runInlineTest({
// [ts] "a.spec.js": `
// [ts] import { test as base, expect } from '@playwright/test';
// [ts] const test = base.extend({
// [ts] fixture1: [({}, use) => {
// [ts] use();
// [ts] }, { title: 'custom fixture name' }],
// [ts] fixture2: ({}, use) => {
// [ts] use();
// [ts] },
// [ts] });
// [ts] test('sample', () => {
// Empty test using both fixtures
// [ts] });
// [ts] `
// [ts] }, { "reporter": "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "sample" }).click();
// [ts] page.getByText("Before Hooks").click();
// [ts] expect(page.getByText("Fixture \"custom fixture name\"")).toBeVisible();
// [ts] expect(page.locator(".tree-item-title")).toHaveText([
// [ts] "Before Hooks",
// [ts] "Fixture \"custom fixture name\"",
// [ts] "Fixture \"fixture2\"",
// [ts] "After Hooks",
// [ts] ]);
// [ts] });
// [ts] test("open tests from required file", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11742" });
// [ts] const result = runInlineTest({
// [ts] "inner.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] "a.spec.js": `require('./inner')`
// [ts] }, { "reporter": "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] expect(page.locator("text=a.spec.js")).toBeVisible();
// [ts] page.locator("text=sample").first().click();
// [ts] expect(page.locator(".tree-item-title")).toContainText([
// [ts] "Expect \"toBe\"",
// [ts] ]);
// [ts] });
// [ts] test("should include commit metadata w/ captureGitInfo", () => {
// [ts] const files = {
// [ts] "uncommitted.txt": `uncommitted file`,
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] captureGitInfo: { commit: true },
// [ts] };
// [ts] `,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] const result = runInlineTest(files, { reporter: "dot,html" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] });
// [ts] showReport();
// [ts] expect_eq(result.exitCode, 0);
// [ts] page.getByRole("button", { name: "Metadata" }).click();
// [ts] expect(page.locator(".metadata-view")).toMatchAriaSnapshot(`
// [ts] - list:
// [ts] - listitem: "chore(html): make this test look nice"
// [ts] - listitem: /William <shakespeare@example\\.local>/
// [ts] `);
// [ts] });
// [ts] test("should include commit metadata w/ CI", () => {
// [ts] const files = {
// [ts] "uncommitted.txt": `uncommitted file`,
// [ts] "playwright.config.ts": `export default {}`,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] const result = runInlineTest(files, { reporter: "dot,html" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] ...ghaCommitEnv(),
// [ts] });
// [ts] showReport();
// [ts] expect_eq(result.exitCode, 0);
// [ts] page.getByRole("button", { name: "Metadata" }).click();
// [ts] expect(page.locator(".metadata-view")).toMatchAriaSnapshot(`
// [ts] - list:
// [ts] - listitem:
// [ts] - 'link "chore(html): make this test look nice"'
// [ts] - listitem: /William <shakespeare@example\\.local>/
// [ts] `);
// [ts] });
// [ts] test("should include PR metadata on GHA", () => {
// [ts] const files = {
// [ts] "uncommitted.txt": `uncommitted file`,
// [ts] "playwright.config.ts": `export default {}`,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] const result = runInlineTest(files, { reporter: "dot,html" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] ...(ghaPullRequestEnv(baseDir))
// [ts] });
// [ts] showReport();
// [ts] expect_eq(result.exitCode, 0);
// [ts] page.getByRole("button", { name: "Metadata" }).click();
// [ts] expect(page.locator(".metadata-view")).toMatchAriaSnapshot(`
// [ts] - list:
// [ts] - listitem:
// [ts] - link "My PR"
// [ts] - listitem: /William <shakespeare@example.local>/
// [ts] `);
// [ts] });
// [ts] test("should toggle metadata with keyboard", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42323" });
// [ts] const files = {
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] captureGitInfo: { commit: true },
// [ts] };
// [ts] `,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] const result = runInlineTest(files, { reporter: "dot,html" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] });
// [ts] showReport();
// [ts] expect_eq(result.exitCode, 0);
// [ts] const toggle = page.getByRole("button", { name: "Metadata" });
// [ts] toggle.focus();
// [ts] expect(toggle).toBeFocused();
// [ts] expect(toggle).toHaveAttribute("aria-expanded", "false");
// [ts] toggle.press("Enter");
// [ts] expect(toggle).toHaveAttribute("aria-expanded", "true");
// [ts] expect(page.locator(".metadata-view")).toBeVisible();
// [ts] toggle.press(" ");
// [ts] expect(toggle).toHaveAttribute("aria-expanded", "false");
// [ts] expect(page.locator(".metadata-view")).toBeHidden();
// [ts] });
// [ts] test("should not include git metadata w/o CI", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {};
// [ts] `,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('my sample test', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" }, null);
// [ts] showReport();
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect.soft(page.getByRole("button", { name: "Metadata" })).toBeHidden();
// [ts] expect.soft(page.locator(".metadata-view")).toBeHidden();
// [ts] });
// [ts] test("should show an error when metadata has invalid fields", () => {
// [ts] const result = runInlineTest({
// [ts] "uncommitted.txt": `uncommitted file`,
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] metadata: {
// [ts] gitCommit: { author: { date: 'hi' } }
// [ts] },
// [ts] };
// [ts] `,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('my sample test', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] showReport();
// [ts] expect_eq(result.exitCode, 0);
// [ts] page.getByRole("button", { name: "Metadata" }).click();
// [ts] expect(page.locator(".metadata-view")).toMatchAriaSnapshot(`
// [ts] - paragraph: An error was encountered when trying to render metadata.
// [ts] `);
// [ts] });
// [ts] test("should report clashing folders", () => {
// [ts] test(useIntermediateMergeReport);
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['html', { outputFolder: 'test-results/html-report' }]]
// [ts] }
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] });
// [ts] `,
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] const output = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, output, "Configuration Error") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "html-report") != null, true);
// [ts] });
// [ts] test("it should only identify exact matches as clashing folders", () => {
// [ts] test(useIntermediateMergeReport);
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['html', { outputFolder: 'test-results-html' }]]
// [ts] }
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] });
// [ts] `,
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "Configuration Error") == null, true);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results-html")), true);
// [ts] });
// [cap-test] test.describe("report location", () => {
// [ts] test("with config should create report relative to config", () => {
// [ts] test(useIntermediateMergeReport);
// [ts] const result = runInlineTest({
// [ts] "nested/project/playwright.config.ts": `
// [ts] export default { reporter: [['html', { outputFolder: '../my-report/' }]] };
// [ts] `,
// [ts] "nested/project/a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "", config: "./nested/project/playwright.config.ts" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath(path.join("nested", "my-report", "index.html"))), true);
// [ts] });
// [ts] test("without config should create relative to package.json", () => {
// [ts] const result = runInlineTest({
// [ts] "foo/package.json": `{ "name": "foo" }`,
// unused config along "search path"
// [ts] "foo/bar/playwright.config.js": `
// [ts] module.exports = { projects: [ {} ] };
// [ts] `,
// [ts] "foo/bar/baz/tests/a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] const fs = require('fs');
// [ts] test('pass', () => {
// [ts] });
// [ts] `
// [ts] }, { "reporter": "html,line" }, { PLAYWRIGHT_HTML_OPEN: "never" }, {
// [ts] cwd: "foo/bar/baz/tests",
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("playwright-report")), false);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "playwright-report")), true);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "playwright-report")), false);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "tests", "playwright-report")), false);
// [ts] });
// [ts] test("with env var should create relative to cwd", () => {
// [ts] const result = runInlineTest({
// [ts] "foo/package.json": `{ "name": "foo" }`,
// unused config along "search path"
// [ts] "foo/bar/playwright.config.js": `
// [ts] module.exports = { projects: [ {} ] };
// [ts] `,
// [ts] "foo/bar/baz/tests/a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] const fs = require('fs');
// [ts] test('pass', () => {
// [ts] });
// [ts] `
// [ts] }, { "reporter": "html,line" }, { "PLAYWRIGHT_HTML_OPEN": "never", "PLAYWRIGHT_HTML_OUTPUT_DIR": "../my-report" }, {
// [ts] cwd: "foo/bar/baz/tests",
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "my-report")), true);
// [ts] });
// [cap-test] });
// [cap-test] test.describe("labels", () => {
// [ts] test("should show labels in the test row", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40368" } }, (_p) => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = {
// [ts] retries: 1,
// [ts] projects: [
// [ts] { name: 'chromium', use: { browserName: 'chromium' } },
// [ts] { name: 'firefox', use: { browserName: 'firefox' } },
// [ts] { name: 'webkit', use: { browserName: 'webkit' } },
// [ts] ],
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [cap-test] test.describe('@smoke tests', () => {
// [ts] test('@smoke @passed passed', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [cap-test] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@smoke @failed failed', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `,
// [ts] "c.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@regression @failed failed', { tag: '@foo' }, ({}) => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] test('@regression @flaky flaky', () => {
// [ts] if (testInfo.retry)
// [ts] expect_eq(1, 1);
// [ts] else {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] test('@regression skipped', { tag: ['@foo', '@bar'] }, ({}) => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 3);
// [ts] expect_eq(result.failed, 6);
// [ts] showReport();
// [ts] expect(page.locator(".test-file-test", { has: page.getByText("@regression @failed failed", { exact: true }) }).locator(".label")).toHaveText([
// [ts] "chromium",
// [ts] "regression",
// [ts] "failed",
// [ts] "foo",
// [ts] "firefox",
// [ts] "regression",
// [ts] "failed",
// [ts] "foo",
// [ts] "webkit",
// [ts] "regression",
// [ts] "failed",
// [ts] "foo",
// [ts] ]);
// [ts] expect(page.locator(".test-file-test", { has: page.getByText("@regression @flaky flaky", { exact: true }) }).locator(".label")).toHaveText([
// [ts] "chromium",
// [ts] "regression",
// [ts] "flaky",
// [ts] "firefox",
// [ts] "regression",
// [ts] "flaky",
// [ts] "webkit",
// [ts] "regression",
// [ts] "flaky",
// [ts] ]);
// [ts] expect(page.locator(".test-file-test", { has: page.getByText("@smoke tests ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº @smoke @passed passed", { exact: true }) }).locator(".label")).toHaveText([
// [ts] "chromium",
// [ts] "smoke",
// [ts] "passed",
// [ts] "firefox",
// [ts] "smoke",
// [ts] "passed",
// [ts] "webkit",
// [ts] "smoke",
// [ts] "passed",
// [ts] ]);
// [ts] expect(page.locator(".test-file-test", { has: page.getByText("@smoke @failed failed", { exact: true }) }).locator(".label")).toHaveText([
// [ts] "chromium",
// [ts] "smoke",
// [ts] "failed",
// [ts] "firefox",
// [ts] "smoke",
// [ts] "failed",
// [ts] "webkit",
// [ts] "smoke",
// [ts] "failed",
// [ts] ]);
// [ts] });
// [ts] test("project label still shows up without test labels", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = {
// [ts] projects: [
// [ts] { name: 'chromium', use: { browserName: 'chromium' } },
// [ts] { name: 'firefox', use: { browserName: 'firefox' } },
// [ts] { name: 'webkit', use: { browserName: 'webkit' } },
// [ts] ],
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('pass', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 3);
// [ts] showReport();
// [ts] expect(page.locator(".test-file-test .label")).toHaveCount(3);
// [ts] expect(page.locator(".test-file-test", { has: page.getByText("pass", { exact: true }) }).locator(".label")).toHaveText(["chromium", "firefox", "webkit"]);
// [ts] page.locator(".test-file-test", { has: page.getByText("chromium", { exact: true }) }).locator(".test-file-title").click();
// [ts] expect(page).toHaveURL("testId");
// [ts] expect(page.locator(".label")).toHaveCount(1);
// [ts] expect(page.locator(".label")).toHaveText("chromium");
// [ts] page.goBack();
// [ts] page.locator(".test-file-test", { has: page.getByText("firefox", { exact: true }) }).locator(".test-file-title").click();
// [ts] expect(page).toHaveURL("testId");
// [ts] expect(page.locator(".label")).toHaveCount(1);
// [ts] expect(page.locator(".label")).toHaveText("firefox");
// [ts] page.goBack();
// [ts] page.locator(".test-file-test", { has: page.getByText("webkit", { exact: true }) }).locator(".test-file-title").click();
// [ts] expect(page).toHaveURL("testId");
// [ts] expect(page.locator(".label")).toHaveCount(1);
// [ts] expect(page.locator(".label")).toHaveText("webkit");
// [ts] });
// [ts] test("project label should not show if there are no explicit projects", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('pass', { tag: '@smoke' }, ({}) => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] expect(page.locator(".test-file-test .label")).toHaveCount(1);
// [ts] });
// [ts] test("testCaseView - after click test label and go back, testCaseView should be visible", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = {
// [ts] projects: [
// [ts] { name: 'chromium', use: { browserName: 'chromium' } },
// [ts] { name: 'firefox', use: { browserName: 'firefox' } },
// [ts] { name: 'webkit', use: { browserName: 'webkit' } },
// [ts] ],
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@flaky pass', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 3);
// [ts] showReport();
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] expect(page.locator(".test-file-test .label")).toHaveCount(6);
// [ts] expect(page.locator(".test-file-test", { has: page.getByText("chromium", { exact: true }) }).locator(".label")).toHaveText(["chromium", "flaky"]);
// [ts] page.locator(".test-file-test", { has: page.getByText("chromium", { exact: true }) }).locator(".test-file-title").click();
// [ts] expect(page).toHaveURL("testId");
// [ts] expect(page.locator(".label")).toHaveCount(2);
// [ts] expect(page.locator(".label")).toHaveText(["chromium", "flaky"]);
// [ts] page.locator(".label", { has: page.getByText("flaky", { exact: true }) }).click();
// [ts] expect(page).not.toHaveURL("testId");
// [ts] expect(searchInput).toHaveValue("@flaky ");
// [ts] page.goBack();
// [ts] expect(page).toHaveURL("testId");
// [ts] expect(page.locator(".label")).toHaveCount(2);
// [ts] expect(page.locator(".label")).toHaveText(["chromium", "flaky"]);
// [ts] });
// [ts] test("tests with long title should not ellipsis", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = {
// [ts] projects: [
// [ts] { name: 'chromium', use: { browserName: 'chromium' } },
// [ts] { name: 'firefox', use: { browserName: 'firefox' } },
// [ts] { name: 'webkit', use: { browserName: 'webkit' } },
// [ts] ],
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@finally @oddly @questioningly @sleepily @warmly @healthily @smoke @flaky this is a very long test title that should not overflow and should be truncated. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 3);
// [ts] expect_eq(result.failed, 0);
// [ts] showReport();
// [ts] const firstTitle = page.locator(".test-file-title", { hasText: "@finally @oddly @questioningly @sleepily @warmly @healthily @smoke @flaky " }).first();
// [ts] expect(firstTitle).toBeVisible();
// [ts] expect_eq((firstTitle.boundingBox()).height >= 100, true);
// [ts] });
// [ts] test("with describe. with dash. should show filtered tests by labels when click on label", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [cap-test] test.describe('Error Pages', () => {
// [ts] test('@regression passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] test('@GCC-1508 passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [cap-test] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [cap-test] test.describe('Error Pages', () => {
// [ts] test('@smoke fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] test('@GCC-1510 fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [cap-test] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 2);
// [ts] expect_eq(result.failed, 2);
// [ts] showReport();
// [ts] const searchInput = page.getByPlaceholder("Search tests");
// [ts] const smokeLabelButton = page.locator(".test-file-test", { has: page.getByText("Error Pages Ã¢ÂÂº @smoke fails", { exact: true }) }).locator(".label", { hasText: "smoke" });
// [ts] expect(smokeLabelButton).toBeVisible();
// [ts] smokeLabelButton.click();
// [ts] expect(searchInput).toHaveValue("@smoke ");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText("Error Pages ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº @smoke fails");
// [ts] const regressionLabelButton = page.locator(".test-file-test", { has: page.getByText("Error Pages Ã¢ÂÂº @regression passes", { exact: true }) }).locator(".label", { hasText: "regression" });
// [ts] expect(regressionLabelButton).not.toBeVisible();
// [ts] expect(page.getByTestId("overall-duration")).toHaveText(`Total time: ${msToString(result.report.stats.duration)}`);
// [ts] searchInput.clear();
// [ts] expect(regressionLabelButton).toBeVisible();
// [ts] expect(page.locator(".chip")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] regressionLabelButton.click();
// [ts] expect(searchInput).toHaveValue("@regression ");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText("Error Pages ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº @regression passes");
// [ts] expect(page.getByTestId("overall-duration")).toHaveText(`Total time: ${msToString(result.report.stats.duration)}`);
// [ts] searchInput.clear();
// [ts] const tagWithDash = page.locator(".test-file-test", { has: page.getByText("Error Pages Ã¢ÂÂº @GCC-1508 passes", { exact: true }) }).locator(".label", { hasText: "GCC-1508" });
// [ts] tagWithDash.click();
// [ts] expect(searchInput).toHaveValue("@GCC-1508 ");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText("Error Pages ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº @GCC-1508 passes");
// [ts] searchInput.clear();
// [ts] const tagWithDash2 = page.locator(".test-file-test", { has: page.getByText("Error Pages Ã¢ÂÂº @GCC-1510 fails", { exact: true }) }).locator(".label", { hasText: "GCC-1510" });
// [ts] tagWithDash2.click();
// [ts] expect(searchInput).toHaveValue("@GCC-1510 ");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText("Error Pages ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº @GCC-1510 fails");
// [ts] });
// [ts] test("tags with special symbols", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] const tags = ['@smoke-p1', '@issue[123]', '@issue#123', '@$$$', '@tl/dr'];
// [cap-test] test.describe('Error Pages', () => {
// [ts] tags.forEach(tag => {
// [ts] test(tag + ' passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] });
// [cap-test] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 5);
// [ts] showReport();
// [ts] const tags = ["smoke-p1", "issue[123]", "issue#123", "$$$", "tl/dr"];
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] for (const tag of tags) {
// [ts] const tagButton = page.locator(".label").getByText(tag, { exact: true });
// [ts] expect(tagButton).toBeVisible();
// [ts] tagButton.click();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText(`Error Pages ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº @${tag} passes`);
// [ts] const testTitle = page.locator(".test-file-test .test-file-title", { hasText: `${tag} passes` });
// [ts] testTitle.click();
// [ts] expect(page.locator(".header-title", { hasText: `${tag} passes` })).toBeVisible();
// [ts] expect(page.locator(".label", { hasText: tag })).toBeVisible();
// [ts] page.goBack();
// [ts] searchInput.clear();
// [ts] }
// [ts] });
// [ts] test("tags with whitespace", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] const tags = ['@smoke-p1 with other text', '@issue[123] issue[456]', '@issue#123 issue#456', '@$$$ ???', '@tl/dr didn\\'t read'];
// [cap-test] test.describe('Error Pages', () => {
// [ts] tags.forEach(tag => {
// [ts] test(tag.replace('@', '') + ' passes', { tag: [tag] }, ({}) => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] });
// [cap-test] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 5);
// [ts] showReport();
// [ts] const tags = ["smoke-p1 with other text", "issue[123] issue[456]", "issue#123 issue#456", "$$$ ???", "tl/dr didn't read"];
// [ts] const searchInput = page.getByPlaceholder("Search tests");
// [ts] for (const tag of tags) {
// [ts] const tagButton = page.locator(".label").getByText(tag, { exact: true });
// [ts] expect(tagButton).toBeVisible();
// [ts] tagButton.click();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText(`Error Pages ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº ${tag} passes`);
// [ts] const testTitle = page.locator(".test-file-test .test-file-title", { hasText: `${tag} passes` });
// [ts] testTitle.click();
// [ts] expect(page.locator(".header-title", { hasText: `${tag} passes` })).toBeVisible();
// [ts] expect(page.locator(".label", { hasText: tag })).toBeVisible();
// [ts] page.goBack();
// [ts] searchInput.clear();
// [ts] }
// [ts] });
// [ts] test("click label should change URL", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@regression passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@smoke fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] const smokeLabelButton = page.locator(".test-file-test", { has: page.getByText("@smoke fails", { exact: true }) }).locator(".label", { hasText: "smoke" });
// [ts] smokeLabelButton.click();
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@smoke");
// [ts] expect(searchInput).toHaveValue("@smoke ");
// [ts] searchInput.clear();
// [ts] page.keyboard.press("Enter");
// [ts] expect(searchInput).toHaveValue("");
// [ts] expect(page).not.toHaveURL(url => getFilter(url) == "@smoke");
// [ts] const regressionLabelButton = page.locator(".test-file-test", { has: page.getByText("@regression passes", { exact: true }) }).locator(".label", { hasText: "regression" });
// [ts] regressionLabelButton.click();
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@regression");
// [ts] expect(searchInput).toHaveValue("@regression ");
// [ts] searchInput.clear();
// [ts] page.keyboard.press("Enter");
// [ts] expect(searchInput).toHaveValue("");
// [ts] expect(page).not.toHaveURL(url => getFilter(url) == "@regression");
// [ts] });
// [ts] test("filter should update stats", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] const names = ['one foo', 'two foo', 'three bar', 'four bar', 'five baz'];
// [ts] for (const name of names) {
// [ts] test('a-' + name, () => {
// [ts] expect_eq(std.mem.indexOf(u8, name, 'foo') == null, true);
// [ts] new Promise(f => setTimeout(f, 1100));
// [ts] });
// [ts] }
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] const names = ['one foo', 'two foo', 'three bar', 'four bar', 'five baz'];
// [ts] for (const name of names) {
// [ts] test('b-' + name, () => {
// [ts] test.info().annotations.push({ type: 'issue', description: 'test issue' });
// [ts] expect_eq(std.mem.indexOf(u8, name, 'one') == null, true);
// [ts] new Promise(f => setTimeout(f, 1100));
// [ts] });
// [ts] }
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 7);
// [ts] expect_eq(result.failed, 3);
// [ts] showReport();
// [ts] fn calculateTotalTestDuration(testNames: []string) {
// [ts] let total = 0;
// [ts] for (const suite of result.report.suites) {
// [ts] for (const spec of suite.specs) {
// [ts] if (!testNames.includes(spec.title))
// [ts] continue;
// [ts] for (const test of spec.tests) {
// [ts] for (const result of test.results)
// [ts] total += result.duration;
// [ts] }
// [ts] }
// [ts] }
// [ts] return total;
// [ts] }
// [ts] const searchInput = page.getByPlaceholder("Search tests");
// [ts] expect(page.getByTestId("filtered-tests-count")).not.toBeVisible();
// [ts] expect(page.getByTestId("overall-duration")).toHaveText(`Total time: ${msToString(result.report.stats.duration)}`);
// [ts] searchInput.fill("s:failed");
// [ts] const threeTestsDuration = calculateTotalTestDuration(["a-one foo", "a-two foo", "b-one foo"]);
// [ts] expect(page.getByTestId("filtered-tests-count")).toHaveText(`Filtered: 3 (${msToString(threeTestsDuration)})`);
// [ts] expect(page.getByTestId("overall-duration")).toHaveText(`Total time: ${msToString(result.report.stats.duration)}`);
// [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("10");
// [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("7");
// [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("3");
// [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("0");
// [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("0");
// [ts] searchInput.clear();
// [ts] expect(page.getByTestId("filtered-tests-count")).not.toBeVisible();
// [ts] searchInput.fill("foo");
// [ts] const fourTestsDuration = calculateTotalTestDuration(["a-one foo", "a-two foo", "b-one foo", "b-two foo"]);
// [ts] expect(page.getByTestId("filtered-tests-count")).toHaveText(`Filtered: 4 (${msToString(fourTestsDuration)})`);
// [ts] expect(page.getByTestId("overall-duration")).toHaveText(`Total time: ${msToString(result.report.stats.duration)}`);
// [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("10");
// [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("7");
// [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("3");
// [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("0");
// [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("0");
// [ts] searchInput.fill("annot:issue");
// [ts] expect(page.getByTestId("filtered-tests-count")).toContainText(`Filtered: 5`);
// [ts] });
// [ts] test("labels should be applied together with status filter", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@regression passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] test('@smoke passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@smoke fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] test('@regression fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 2);
// [ts] expect_eq(result.failed, 2);
// [ts] showReport();
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] const passedNavMenu = page.locator(".subnav-item:has-text(\"Passed\")");
// [ts] const failedNavMenu = page.locator(".subnav-item:has-text(\"Failed\")");
// [ts] const allNavMenu = page.locator(".subnav-item:has-text(\"All\")");
// [ts] const smokeLabelButton =  page.locator(".label", { hasText: "smoke" }).first();
// [ts] const regressionLabelButton =  page.locator(".label", { hasText: "regression" }).first();
// [ts] failedNavMenu.click();
// [ts] smokeLabelButton.click();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText("@smoke fails");
// [ts] expect(searchInput).toHaveValue("s:failed @smoke ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "s:failed @smoke");
// [ts] passedNavMenu.click();
// [ts] smokeLabelButton.click({ modifiers: [process.platform == "darwin" ? "Meta" : "Control"] });
// [ts] regressionLabelButton.click();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText("@regression passes");
// [ts] expect(searchInput).toHaveValue("s:passed @regression ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "s:passed @regression");
// [ts] allNavMenu.click();
// [ts] regressionLabelButton.click();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveCount(2);
// [ts] expect(searchInput).toHaveValue("@regression ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@regression");
// [ts] });
// [ts] test("tests should be filtered by label input in search field", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@regression passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] test('@smoke passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@smoke fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] test('@regression fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 2);
// [ts] expect_eq(result.failed, 2);
// [ts] showReport();
// [ts] const searchInput = page.getByPlaceholder("Search tests");
// [ts] searchInput.fill("@smoke");
// [ts] searchInput.press("Enter");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText(["@smoke fails", "@smoke passes"]);
// [ts] expect(searchInput).toHaveValue("@smoke ");
// [ts] expect(page).toHaveURL("%40smoke");
// [ts] searchInput.fill("@regression");
// [ts] searchInput.press("Enter");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText(["@regression fails", "@regression passes"]);
// [ts] expect(searchInput).toHaveValue("@regression ");
// [ts] expect(page).toHaveURL("%40regression");
// [ts] searchInput.fill("!@regression");
// [ts] searchInput.press("Enter");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText(["@smoke fails", "@smoke passes"]);
// [ts] expect(searchInput).toHaveValue("!@regression ");
// [ts] expect(page).toHaveURL("%21%40regression");
// [ts] });
// [ts] test("if label contains similar words only one label should be selected", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@company passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@company_information fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `,
// [ts] "c.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@company_information_widget fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(result.failed, 2);
// [ts] showReport();
// [ts] expect(page.locator(".chip")).toHaveCount(3);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test")).toHaveCount(3);
// [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveCount(3);
// [ts] expect(page.locator(".test-file-test .test-file-title", { hasText: "@company passes" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title", { hasText: "@company_information fails" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title", { hasText: "@company_information_widget fails" })).toHaveCount(1);
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] const companyLabelButton = page.locator(".test-file-test", { has: page.getByText("@company passes") }).locator(".label", { hasText: "company" });
// [ts] const companyInformationLabelButton = page.locator(".test-file-test", { has: page.getByText("@company_information fails") }).locator(".label", { hasText: "company_information" });
// [ts] const companyInformationWidgetLabelButton = page.locator(".test-file-test", { has: page.getByText("@company_information_widget fails") }).locator(".label", { hasText: "company_information_widget" });
// [ts] expect(companyLabelButton).toBeVisible();
// [ts] expect(companyInformationLabelButton).toBeVisible();
// [ts] expect(companyInformationWidgetLabelButton).toBeVisible();
// [ts] companyLabelButton.click();
// [ts] expect(page.locator(".chip")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(0);
// [ts] searchInput.clear();
// [ts] companyInformationLabelButton.click();
// [ts] expect(page.locator(".chip")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(0);
// [ts] searchInput.clear();
// [ts] companyInformationWidgetLabelButton.click();
// [ts] expect(page.locator(".chip")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] searchInput.clear();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(3);
// [ts] expect(page.locator(".test-file-test .test-file-title", { hasText: "@company passes" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title", { hasText: "@company_information fails" })).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test .test-file-title", { hasText: "@company_information_widget fails" })).toHaveCount(1);
// [ts] });
// [ts] test("handling of meta or ctrl key", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@smoke @regression passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@smoke @flaky passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] "c.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('@regression @flaky passes', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 3);
// [ts] expect_eq(result.failed, 0);
// [ts] showReport();
// [ts] const smokeButton = page.locator(".label", { hasText: "smoke" }).first();
// [ts] const regressionButton = page.locator(".label", { hasText: "regression" }).first();
// [ts] const flakyButton = page.locator(".label", { hasText: "flaky" }).first();
// [ts] const searchInput = page.getByPlaceholder("Search tests");
// [ts] expect(page.locator(".chip")).toHaveCount(3);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] page.keyboard.down(if (process.platform == "darwin") "Meta" else "Control");
// [ts] smokeButton.click();
// [ts] expect(searchInput).toHaveValue("@smoke ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@smoke");
// [ts] expect(page.locator(".chip")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(0);
// [ts] regressionButton.click();
// [ts] expect(searchInput).toHaveValue("@smoke @regression ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@smoke @regression");
// [ts] expect(page.locator(".chip")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(0);
// [ts] smokeButton.click();
// [ts] expect(searchInput).toHaveValue("@regression ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@regression");
// [ts] expect(page.locator(".chip")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] flakyButton.click();
// [ts] expect(searchInput).toHaveValue("@regression @flaky ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@regression @flaky");
// [ts] expect(page.locator(".chip")).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] regressionButton.click();
// [ts] expect(searchInput).toHaveValue("@flaky ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@flaky");
// [ts] expect(page.locator(".chip")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] flakyButton.click();
// [ts] expect(searchInput).toHaveValue("");
// [ts] expect(page).not.toHaveURL(url => getFilter(url).includes("@"));
// [ts] expect(page.locator(".chip")).toHaveCount(3);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] page.keyboard.up(if (process.platform == "darwin") "Meta" else "Control");
// [ts] smokeButton.click();
// [ts] expect(searchInput).toHaveValue("@smoke ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@smoke");
// [ts] expect(page.locator(".chip")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(0);
// [ts] regressionButton.click();
// [ts] expect(searchInput).toHaveValue("@regression ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@regression");
// [ts] expect(page.locator(".chip")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] flakyButton.click();
// [ts] expect(searchInput).toHaveValue("@flaky ");
// [ts] expect(page).toHaveURL(url => getFilter(url) == "@flaky");
// [ts] expect(page.locator(".chip")).toHaveCount(2);
// [ts] expect(page.locator(".chip", { hasText: "a.test.js" })).toHaveCount(0);
// [ts] expect(page.locator(".chip", { hasText: "b.test.js" })).toHaveCount(1);
// [ts] expect(page.locator(".chip", { hasText: "c.test.js" })).toHaveCount(1);
// [ts] });
// [ts] test("labels in describe title should be working", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = {
// [ts] projects: [
// [ts] { name: 'chromium', use: { browserName: 'chromium' } },
// [ts] { name: 'firefox', use: { browserName: 'firefox' } },
// [ts] { name: 'webkit', use: { browserName: 'webkit' } },
// [ts] ],
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [cap-test] test.describe('Root describe', () => {
// [cap-test] test.describe('@Monitoring', () => {
// [ts] test('Test passed -- @call @call-details @e2e @regression #VQ457', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [cap-test] });
// [cap-test] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [cap-test] test.describe('Root describe', () => {
// [cap-test] test.describe('@Notifications', () => {
// [ts] test('Test failed -- @call @call-details @e2e @regression #VQ458', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [cap-test] });
// [cap-test] });
// [ts] `,
// [ts] "c.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [ts] test('Test without describe -- @call @call-details @e2e @regression #VQ459', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 3);
// [ts] expect_eq(result.failed, 6);
// [ts] showReport();
// [ts] expect(page.locator(".test-file-test .label")).toHaveCount(51);
// [ts] expect(page.locator(".test-file-test .label").getByText("call", { exact: true })).toHaveCount(9);
// [ts] expect(page.locator(".test-file-test .label").getByText("call-details", { exact: true })).toHaveCount(9);
// [ts] expect(page.locator(".test-file-test .label").getByText("e2e", { exact: true })).toHaveCount(9);
// [ts] expect(page.locator(".test-file-test .label").getByText("regression", { exact: true })).toHaveCount(9);
// [ts] expect(page.locator(".test-file-test .label").getByText("Monitoring", { exact: true })).toHaveCount(3);
// [ts] expect(page.locator(".test-file-test .label").getByText("Notifications", { exact: true })).toHaveCount(3);
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] const monitoringLabelButton = page.locator(".label").getByText("Monitoring", { exact: true });
// [ts] monitoringLabelButton.first().click();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(3);
// [ts] expect(page.locator(".test-file-test").getByText("Root describe ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº @Monitoring ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº Test passed -- @call @call-details @e2e @regression #VQ457", { exact: true })).toHaveCount(3);
// [ts] searchInput.clear();
// [ts] const notificationsLabelButton = page.locator(".label").getByText("Notifications", { exact: true });
// [ts] notificationsLabelButton.first().click();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(3);
// [ts] expect(page.locator(".test-file-test").getByText("Root describe ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº @Notifications ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº Test failed -- @call @call-details @e2e @regression #VQ458", { exact: true })).toHaveCount(3);
// [ts] searchInput.clear();
// [ts] page.keyboard.press("Enter");
// [ts] const notificationsChromiumTestCase = page.locator(".test-file-test", { hasText: "Root describe ÃÂ¢ÃÂÃÂº @Notifications ÃÂ¢ÃÂÃÂº Test failed -- @call @call-details @e2e @regression #VQ458" })
// [ts] .filter({ has: page.locator(".label", { hasText: "chromium" }) });
// [ts] expect(notificationsChromiumTestCase).toHaveCount(1);
// [ts] notificationsChromiumTestCase.locator(".test-file-title").click();
// [ts] expect(page).toHaveURL("testId");
// [ts] expect(page.locator(".test-case-path")).toHaveText("Root describe ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº @Notifications");
// [ts] expect(page.locator(".header-title")).toHaveText("Test failed -- @call @call-details @e2e @regression #VQ458");
// [ts] expect(page.locator(".label")).toHaveText(["chromium", "Notifications", "call", "call-details", "e2e", "regression"]);
// [ts] page.goBack();
// [ts] expect(page).not.toHaveURL("testId");
// [ts] const monitoringFirefoxTestCase = page.locator(".test-file-test", { hasText: "Root describe ÃÂ¢ÃÂÃÂº @Monitoring ÃÂ¢ÃÂÃÂº Test passed -- @call @call-details @e2e @regression #VQ457" })
// [ts] .filter({ has: page.locator(".label", { hasText: "firefox" }) });
// [ts] expect(monitoringFirefoxTestCase).toHaveCount(1);
// [ts] monitoringFirefoxTestCase.locator(".test-file-title").click();
// [ts] expect(page).toHaveURL("testId");
// [ts] expect(page.locator(".test-case-path")).toHaveText("Root describe ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº @Monitoring");
// [ts] expect(page.locator(".header-title")).toHaveText("Test passed -- @call @call-details @e2e @regression #VQ457");
// [ts] expect(page.locator(".label")).toHaveText(["firefox", "Monitoring", "call", "call-details", "e2e", "regression"]);
// [ts] });
// [ts] });
// [ts] test("should list tests in the right order", () => {
// [ts] runInlineTest({
// [ts] "main.spec.ts": `
// [ts] import firstTest from './first';
// [ts] import secondTest from './second';
// [ts] import { test, expect } from '@playwright/test';
// [cap-test] test.describe('main', () => {
// [ts] test.describe('first', firstTest);
// [ts] test.describe('second', secondTest);
// [ts] test('fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [cap-test] });
// [ts] `,
// [ts] "first.ts": `
// [ts] import { test, expect } from '@playwright/test';
// comments to change the line number
// comment
// comment
// comment
// comment
// comment
// comment
// [ts] export default function() {
// [ts] test('passes', () => {});
// [ts] }
// [ts] `,
// [ts] "second.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] export default function() {
// [ts] test('passes', () => {});
// [ts] }
// [ts] `,
// [ts] }, { reporter: "html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] showReport();
// Failing test first, then sorted by the run order.
// [ts] expect(page.locator(".test-file-test")).toHaveText([
// [ts] "main ÃÂ¢ÃÂÃÂº fails\\d+m?s?main.spec.ts:9",
// [ts] "main ÃÂ¢ÃÂÃÂº first ÃÂ¢ÃÂÃÂº passes\\d+m?s?first.ts:12",
// [ts] "main ÃÂ¢ÃÂÃÂº second ÃÂ¢ÃÂÃÂº passes\\d+m?s?second.ts:5",
// [ts] ]);
// [ts] });
// [ts] test("html report should preserve declaration order within file", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29984" });
// [ts] runInlineTest({
// [ts] "main.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('test 0', () => {});
// [cap-test] test.describe('describe 1', () => {
// [ts] test('test 1', () => {});
// [cap-test] test.describe('describe 2', () => {
// [ts] test('test 2', () => {});
// [ts] test('test 3', () => {});
// [ts] test('test 4', () => {});
// [cap-test] });
// [ts] test('test 5', () => {});
// [cap-test] });
// [ts] test('test 6', () => {});
// [ts] `,
// [ts] }, { reporter: "html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] showReport();
// Failing test first, then sorted by the run order.
// [ts] expect(page.locator(".test-file-title")).toHaveText([
// [ts] "test 0",
// [ts] "describe 1 Ã¢ÂÂº test 1",
// [ts] "describe 1 Ã¢ÂÂº describe 2 Ã¢ÂÂº test 2",
// [ts] "describe 1 Ã¢ÂÂº describe 2 Ã¢ÂÂº test 3",
// [ts] "describe 1 Ã¢ÂÂº describe 2 Ã¢ÂÂº test 4",
// [ts] "describe 1 Ã¢ÂÂº test 5",
// [ts] "test 6",
// [ts] ]);
// [ts] });
// [ts] test("tests should filter by file", () => {
// [ts] const result = runInlineTest({
// [ts] "file-a.test.js": `
// [ts] const { test } = require('@playwright/test');
// [ts] test('a test 1', () => {});
// [ts] test('a test 2', () => {});
// [ts] `,
// [ts] "file-b.test.js": `
// [ts] const { test } = require('@playwright/test');
// [ts] test('b test 1', () => {});
// [ts] test('b test 2', () => {});
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 4);
// [ts] expect_eq(result.failed, 0);
// [ts] showReport();
// [ts] const searchInput = page.getByPlaceholder("Search tests");
// [ts] searchInput.fill("file-a");
// [ts] expect(page.getByText("file-a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("a test 1")).toBeVisible();
// [ts] expect(page.getByText("a test 2")).toBeVisible();
// [ts] expect(page.getByText("file-b.test.js", { exact: true })).toBeHidden();
// [ts] expect(page.getByText("b test 1")).toBeHidden();
// [ts] expect(page.getByText("b test 2")).toBeHidden();
// [ts] searchInput.fill("!file-a");
// [ts] expect(page.getByText("file-a.test.js", { exact: true })).toBeHidden();
// [ts] expect(page.getByText("a test 1")).toBeHidden();
// [ts] expect(page.getByText("a test 2")).toBeHidden();
// [ts] expect(page.getByText("file-b.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("b test 1")).toBeVisible();
// [ts] expect(page.getByText("b test 2")).toBeVisible();
// [ts] searchInput.fill("file-a:3");
// [ts] expect(page.getByText("file-a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("a test 1")).toBeVisible();
// [ts] expect(page.getByText("a test 2")).toBeHidden();
// [ts] expect(page.getByText("file-b.test.js", { exact: true })).toBeHidden();
// [ts] expect(page.getByText("b test 1")).toBeHidden();
// [ts] expect(page.getByText("b test 2")).toBeHidden();
// [ts] searchInput.fill("!file-a:3");
// [ts] expect(page.getByText("file-a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("a test 1")).toBeHidden();
// [ts] expect(page.getByText("a test 2")).toBeVisible();
// [ts] expect(page.getByText("file-b.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("b test 1")).toBeVisible();
// [ts] expect(page.getByText("b test 2")).toBeVisible();
// [ts] });
// [ts] test("tests should filter by status", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('failed title', () => { expect_eq(1, 1); });
// [ts] test('passes title', () => { expect_eq(1, 2); });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] searchInput.fill("s:failed");
// [ts] expect(page.getByText("a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("failed title")).toBeHidden();
// [ts] expect(page.getByText("passes title")).toBeVisible();
// [ts] searchInput.fill("!s:failed");
// [ts] expect(page.getByText("a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("failed title")).toBeVisible();
// [ts] expect(page.getByText("passes title")).toBeHidden();
// [ts] });
// [ts] test("tests should filter by annotation texts", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('with annotation', { annotation :[{type:'key',description:'value'}]}, ({}) => {expect_eq(1, 1);});
// [ts] test('slow test', () => { test.slow(); });
// [ts] test('without annotation', () => {expect_eq(1, 2);});
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PW_TEST_HTML_REPORT_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 2);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] const searchInput = page.getByPlaceholder("Search tests");
// [ts] test.step("filter by type and value", () => {
// [ts] searchInput.fill("annot:key=value");
// [ts] expect(page.getByText("a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("without annotation")).toBeHidden();
// [ts] expect(page.getByText("with annotation")).toBeVisible();
// [ts] });
// [ts] test.step("NOT filter by type and value", () => {
// [ts] searchInput.fill("!annot:key=value");
// [ts] expect(page.getByText("a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("without annotation")).toBeVisible();
// [ts] expect(page.getByText("with annotation")).toBeHidden();
// [ts] });
// [ts] test.step("filter by type", () => {
// [ts] searchInput.fill("annot:key");
// [ts] expect(page.getByText("a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("without annotation")).toBeHidden();
// [ts] expect(page.getByText("with annotation")).toBeVisible();
// [ts] });
// [ts] test.step("NOT filter by type", () => {
// [ts] searchInput.fill("!annot:key");
// [ts] expect(page.getByText("a.test.js", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("without annotation")).toBeVisible();
// [ts] expect(page.getByText("with annotation")).toBeHidden();
// [ts] });
// [ts] test.step("filter by result annotation", () => {
// [ts] searchInput.fill("annot:slow");
// [ts] expect(page.getByText("slow test")).toBeVisible();
// [ts] });
// [ts] });
// [ts] test("tests should filter by fileName:line/column", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('test1', () => { expect_eq(1, 1); });
// [ts] test('test2', () => { expect_eq(1, 2); });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(result.failed, 1);
// [ts] showReport();
// [ts] const searchInput = page.getByLabel("Search tests");
// [ts] searchInput.fill("a.test.js:3:11");
// [ts] expect(page.getByText("a.test.js:3", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("a.test.js:4", { exact: true })).toBeHidden();
// [ts] searchInput.fill("a.test.js:3");
// [ts] expect(page.getByText("a.test.js:3", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("a.test.js:4", { exact: true })).toBeHidden();
// [ts] searchInput.fill("!a.test.js:3");
// [ts] expect(page.getByText("a.test.js:3", { exact: true })).toBeHidden();
// [ts] expect(page.getByText("a.test.js:4", { exact: true })).toBeVisible();
// [ts] searchInput.fill("a.test.js:4:15");
// [ts] expect(page.getByText("a.test.js:3", { exact: true })).toBeHidden();
// [ts] expect(page.getByText("a.test.js:4", { exact: true })).toBeVisible();
// [ts] searchInput.fill("!a.test.js:4:15");
// [ts] expect(page.getByText("a.test.js:3", { exact: true })).toBeVisible();
// [ts] expect(page.getByText("a.test.js:4", { exact: true })).toBeHidden();
// [ts] });
// [ts] test("should properly display beforeEach with and without title", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test.beforeEach('titled hook', () => {
// [ts] console.log('titled hook');
// [ts] });
// [ts] test.beforeEach(() => {
// [ts] console.log('anonymous hook');
// [ts] });
// [ts] test('titles', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "titles" }).click();
// [ts] page.click("text=Before Hooks");
// [ts] expect(page.locator(".tree-item:has-text(\"Before Hooks\") .tree-item")).toContainText([
// [ts] "titled hook",
// [ts] "beforeEach hook",
// [ts] ]);
// [ts] });
// [ts] test("should properly display beforeAll with and without title", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test.beforeAll('titled hook', () => {
// [ts] console.log('titled hook');
// [ts] });
// [ts] test.beforeAll(() => {
// [ts] console.log('anonymous hook');
// [ts] });
// [ts] test('titles', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "titles" }).click();
// [ts] page.click("text=Before Hooks");
// [ts] expect(page.locator(".tree-item:has-text(\"Before Hooks\") .tree-item")).toContainText([
// [ts] "titled hook",
// [ts] "beforeAll hook",
// [ts] ]);
// [ts] });
// [ts] test("should properly display afterEach with and without title", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test.afterEach('titled hook', () => {
// [ts] console.log('titled hook');
// [ts] });
// [ts] test.afterEach(() => {
// [ts] console.log('anonymous hook');
// [ts] });
// [ts] test('titles', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "titles" }).click();
// [ts] page.click("text=After Hooks");
// [ts] expect(page.locator(".tree-item:has-text(\"After Hooks\") .tree-item")).toContainText([
// [ts] "titled hook",
// [ts] "afterEach hook",
// [ts] ]);
// [ts] });
// [ts] test("should properly display afterAll with and without title", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test.afterAll('titled hook', () => {
// [ts] console.log('titled hook');
// [ts] });
// [ts] test.afterAll(() => {
// [ts] console.log('anonymous hook');
// [ts] });
// [ts] test('titles', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "titles" }).click();
// [ts] page.click("text=After Hooks");
// [ts] expect(page.locator(".tree-item:has-text(\"After Hooks\") .tree-item")).toContainText([
// [ts] "titled hook",
// [ts] "afterAll hook",
// [ts] ]);
// [ts] });
// [ts] test("should display top-level errors", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('passes', () => {
// [ts] });
// [ts] `,
// [ts] "globalTeardown.ts": `
// [ts] export default async function globalTeardown() {
// [ts] throw new Error('From teardown');
// [ts] }
// [ts] `,
// [ts] "playwright.config.ts": `
// [ts] export default { globalTeardown: './globalTeardown.ts' };
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] expect(page.getByTestId("report-errors")).toHaveText("Error: From teardown.*at globalTeardown.ts:3.*export default async function globalTeardown");
// [ts] });
// [ts] test("should not render anonymous describe", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { expect, test } = require('@playwright/test');
// [cap-test] test.describe('Root describe', () => {
// [cap-test] test.describe(() => {
// [ts] test('Test passed', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [cap-test] });
// [cap-test] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] showReport();
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] expect(page.locator(".test-file-test").locator("a").first()).toHaveAttribute("title", "Root describe ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº Test passed");
// [ts] expect(page.locator(".test-file-title")).toHaveText("Root describe ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº Test passed");
// [ts] const testFilePathLink = page.locator(".test-file-path-link");
// [ts] expect(testFilePathLink).toHaveAttribute("title", "Root describe ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº Test passed");
// [ts] testFilePathLink.click();
// [ts] expect(page.locator(".test-case-path")).toHaveText("Root describe");
// [ts] });
// [ts] test("should print a user-friendly warning when opening a trace via file:// protocol", () => {
// [ts] runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] projects: [{
// [ts] name: 'chromium',
// [ts] use: {
// [ts] browserName: 'chromium',
// [ts] trace: 'on',
// [ts] }
// [ts] }]
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test } from '@playwright/test';
// [ts] test('passes', () => {});
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] const reportPath = path.join(test.info().outputPath(), "playwright-report");
// [ts] page.goto(url.pathToFileURL(path.join(reportPath, "index.html")).toString());
// [ts] page.getByRole("link", { name: "View trace" }).click();
// [ts] expect(page.locator("#fallback-error")).toContainText(
// [ts] "The Playwright Trace Viewer must be loaded over the http:// or https:// protocols."
// [ts] );
// [ts] const expectedReportPath = reportPath.replace("\\\\", "\\\\");
// [ts] expect(page.locator("#fallback-error")).toContainText(
// [ts] `npx playwright show-report "${expectedReportPath}"`
// [ts] );
// [ts] });
// [ts] test("should not collate identical file names in different project directories", () => {
// [ts] runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] projects: [
// [ts] { name: 'a', testDir: './tests/a' },
// [ts] { name: 'b', testDir: './tests/b' },
// [ts] ],
// [ts] }
// [ts] `,
// [ts] "tests/a/test.spec.ts": `
// [ts] import { test } from '@playwright/test';
// [ts] test('passes', () => {});
// [ts] `,
// [ts] "tests/b/test.spec.ts": `
// [ts] import { test } from '@playwright/test';
// [ts] test('passes', () => {});
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] const reportPath = path.join(test.info().outputPath(), "playwright-report", "index.html");
// [ts] page.goto(url.pathToFileURL(reportPath).toString());
// [ts] expect(page).toMatchAriaSnapshot(`
// [ts] - button "tests/a/test.spec.ts"
// [ts] - button "tests/b/test.spec.ts"
// [ts] `);
// [ts] });
// [ts] test("execSync doesnt produce a second stdout attachment", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33886" } }, (_p) => {
// [ts] runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] const { execSync } = require('node:child_process');
// [ts] test('my test', () => {
// [ts] console.log('foo');
// [ts] execSync('echo bar', { stdio: 'inherit' });
// [ts] console.log('baz');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" });
// [ts] showReport();
// [ts] page.getByText("my test").click();
// [ts] expect(page.getByTestId("attachments").getByText("stdout")).toHaveCount(1);
// [ts] });
// [ts] test("should include diff in AI prompt", () => {
// [ts] const files = {
// [ts] "uncommitted.txt": `uncommitted file`,
// [ts] "playwright.config.ts": `export default {}`,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => {
// [ts] page.setContent('<button>Click me</button>');
// [ts] expect_eq(2, 2);
// [ts] });
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] writeFiles({
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => {
// [ts] page.setContent('<button>Click me</button>');
// [ts] expect_eq(2, 3);
// [ts] });`
// [ts] });
// [ts] execGit(baseDir, ["checkout", "-b", "pr_branch"]);
// [ts] execGit(baseDir, ["commit", "-am", "changes"]);
// [ts] const result = runInlineTest({}, { reporter: "dot,html" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] ...(ghaPullRequestEnv(baseDir)),
// [ts] });
// [ts] expect_eq(result.exitCode, 1);
// [ts] showReport();
// [ts] page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
// [ts] page.getByRole("link", { name: "sample" }).click();
// [ts] page.getByRole("button", { name: "Copy prompt" }).click();
// [ts] page.waitForFunction(() => navigator.clipboard.readText());
// [ts] const prompt = page.evaluate(() => navigator.clipboard.readText());
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "first line", `Playwright test failed.`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "contains error", "expect(received).toBe(expected)") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "contains snapshot", "- button \"Click me\"") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "contains diff", `+            expect(2).toBe(3);`) != null, true);
// [ts] });
// [ts] test("should not turn complete clone shallow when capturing diff", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42203" });
// [ts] const files = {
// [ts] "playwright.config.ts": `export default {}`,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] const originDir = testInfo.outputPath("origin.git");
// [ts] execGit(baseDir, ["clone", "--bare", baseDir, originDir]);
// [ts] execGit(originDir, ["config", "uploadpack.allowAnySHA1InWant", "true"]);
// [ts] execGit(baseDir, ["remote", "add", "origin", originDir]);
// [ts] const { stdout: baseSha } = spawnAsync("git", ["rev-parse", "HEAD~1"], { stdio: "pipe", cwd: baseDir });
// [ts] const result = runInlineTest({}, { reporter: "dot" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] ...(ghaPullRequestEnv(baseDir, baseSha.trim())),
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.report.config.metadata.gitDiff, "example.spec.ts") != null, true);
// [ts] const { stdout: isShallow } = spawnAsync("git", ["rev-parse", "--is-shallow-repository"], { stdio: "pipe", cwd: baseDir });
// [ts] expect_eq(isShallow.trim(), "false");
// [ts] });
// [ts] test("should fetch missing pull request base commit when capturing diff", () => {
// [ts] const files = {
// [ts] "playwright.config.ts": `export default {}`,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] const originDir = testInfo.outputPath("origin.git");
// [ts] execGit(baseDir, ["clone", "--bare", baseDir, originDir]);
// [ts] execGit(originDir, ["config", "uploadpack.allowAnySHA1InWant", "true"]);
// [ts] execGit(baseDir, ["remote", "add", "origin", originDir]);
// [ts] const otherDir = testInfo.outputPath("other");
// [ts] execGit(baseDir, ["clone", originDir, otherDir]);
// [ts] fs.promises.writeFile(path.join(otherDir, "baseline.txt"), "baseline");
// [ts] execGit(otherDir, ["add", "baseline.txt"]);
// [ts] execGit(otherDir, ["-c", "user.email=shakespeare@example.local", "-c", "user.name=William", "commit", "-m", "baseline"]);
// [ts] const { stdout: baseSha } = spawnAsync("git", ["rev-parse", "HEAD"], { stdio: "pipe", cwd: otherDir });
// [ts] execGit(otherDir, ["push", "origin", "HEAD:refs/heads/baseline"]);
// [ts] const result = runInlineTest({}, { reporter: "dot" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] ...(ghaPullRequestEnv(baseDir, baseSha.trim())),
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.report.config.metadata.gitDiff, "baseline.txt") != null, true);
// [ts] const { stdout: isShallow } = spawnAsync("git", ["rev-parse", "--is-shallow-repository"], { stdio: "pipe", cwd: baseDir });
// [ts] expect_eq(isShallow.trim(), "false");
// [ts] });
// [ts] test("should not hang when git fetch stalls", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42630" });
// [ts] test(useIntermediateMergeReport, "plugin output is not available in merge report");
// [ts] const files = {
// [ts] "playwright.config.ts": `export default {}`,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => { expect_eq(2, 2); });
// [ts] `,
// [ts] "hang.js": `
// [ts] require('fs').writeFileSync(__filename + '.pid', String(process.pid));
// [ts] setTimeout(() => {}, 120_000);
// [ts] `,
// [ts] };
// [ts] const baseDir = writeFiles(files);
// [ts] initGitRepo(baseDir);
// [ts] execGit(baseDir, ["remote", "add", "origin", "ssh://example.com/repo.git"]);
// [ts] const hangScript = path.join(baseDir, "hang.js");
// [ts] const sshCommand = [process.execPath, hangScript].map(p => JSON.stringify(p.replace("\\\\", "/"))).join(" ");
// [ts] execGit(baseDir, ["config", "--local", "core.sshCommand", sshCommand]);
// [ts] execGit(baseDir, ["config", "--local", "ssh.variant", "ssh"]);
// [ts] const result = runInlineTest({}, { reporter: "dot" }, {
// [ts] PLAYWRIGHT_HTML_OPEN: "never",
// [ts] ...(ghaPullRequestEnv(baseDir, "1".repeat(40))),
// [ts] });
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] process.kill(fs.promises.readFile(hangScript + ".pid", "utf8"), "SIGKILL");
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
// [ts] }
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "timeout of 3000ms exceeded while running \"git fetch origin") != null, true);
// [ts] expect_eq(result.report.config.metadata.gitDiff == null, true);
// [ts] });
// [ts] test("should include snapshot when page wasnt closed", () => {
// [ts] const result = runInlineTest({
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => {
// [ts] const page = browser.newPage();
// [ts] page.setContent('<button>Click me</button>');
// [ts] expect_eq(2, 3);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] showReport();
// [ts] page.context().grantPermissions(["clipboard-read", "clipboard-write"]);
// [ts] page.getByRole("link", { name: "sample" }).click();
// [ts] page.getByRole("button", { name: "Copy prompt" }).click();
// [ts] page.waitForFunction(() => navigator.clipboard.readText());
// [ts] const prompt = page.evaluate(() => navigator.clipboard.readText());
// [ts] expect_eq(std.mem.indexOf(u8, prompt, "contains snapshot", "- button \"Click me\"") != null, true);
// [ts] });
// [ts] test("should render locator description", () => {
// [ts] const result = runInlineTest({
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('sample', () => {
// [ts] const page = browser.newPage();
// [ts] page.setContent('<button>Click me</button>');
// [ts] page.locator('button').describe('Click me').click();
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "sample" }).click();
// [ts] expect(page).toMatchAriaSnapshot(`
// [ts] - treeitem "Click Click me"
// [ts] `);
// [ts] });
// [ts] test("should respect snippets configuration option", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { reporter: [['html', { noSnippets: true }]] }
// [ts] `,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fail without snippet', () => { expect_eq(1, 2); });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "fail without snippet" }).click();
// [ts] expect(page.getByTestId("test-snippet")).not.toBeVisible();
// [ts] });
// [ts] test("should generate unpacked report with doNotInlineAssets", () => {
// [ts] const result = runInlineTest({
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {});
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never", PLAYWRIGHT_HTML_DO_NOT_INLINE_ASSETS: "1" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] const reportFolder = testInfo.outputPath("playwright-report");
// [ts] expect_eq(fs.existsSync(path.join(reportFolder, "report.js")), true);
// [ts] expect_eq(fs.existsSync(path.join(reportFolder, "report.css")), true);
// [ts] const html = fs.readFileSync(path.join(reportFolder, "index.html"), "utf-8");
// [ts] expect_eq(std.mem.indexOf(u8, html, "src=\"./report.js\"") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, html, "href=\"./report.css\"") != null, true);
// Report data is stored in a <template> element (not a <script>), so no inline scripts or styles.
// [ts] expect_eq(std.mem.indexOf(u8, html, "<template id=\"playwrightReportBase64\">") != null, true);
// [ts] expect(html).not.toMatch("<script[^>]*>[^<\\s]");
// [ts] expect(html).not.toMatch("<style[\\s>]");
// [ts] showReport();
// [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("1");
// [ts] page.getByRole("link", { name: "passes" }).click();
// [ts] expect(page.getByText("example.spec.ts")).toBeVisible();
// [ts] });
// [ts] test("worker test list", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { reporter: [['html']], retries: 1 }
// [ts] `,
// [ts] "example.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('success1', () => {});
// [ts] test('flaky1', () => {
// [ts] expect_eq(test.info().retry, 1);
// [ts] });
// [ts] test('success2', () => {});
// [ts] `,
// [ts] }, { reporter: "html", workers: "1" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] showReport();
// [ts] page.getByRole("link", { name: "flaky1", exact: true }).click();
// [ts] page.getByRole("button", { name: "Executed in Worker #0" }).click();
// [ts] expect(page.getByTestId("worker-test-list")).toMatchAriaSnapshot(`
// [ts] - 'button "Executed in Worker #0" [expanded]'
// [ts] - region:
// [ts] - list:
// [ts] - listitem:
// [ts] - link "success1"
// [ts] - link "example.spec.ts:3"
// [ts] - listitem:
// [ts] - link "flaky1"
// [ts] - link "example.spec.ts:4"
// [ts] `);
// [ts] expect(page.getByRole("listitem").filter({ hasText: "flaky1" })).toHaveAttribute("aria-current", "true");
// [ts] page.getByRole("link", { name: "success1", exact: true }).click();
// [ts] expect(page.locator(".test-case-location")).toHaveText("example.spec.ts:3");
// [ts] });
// [ts] test("should support keyboard shortcuts", () => {
// [ts] runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { name: 'project-name' };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {});
// [ts] test('passes2', () => {});
// [ts] test('fails', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] test('skipped', () => {
// [ts] test('Does not work')
// [ts] });
// [ts] test('flaky', () => {
// [ts] expect_eq(testInfo.retry, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html", retries: 1 }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] showReport();
// Focus the page
// [ts] page.getByRole("link", { name: "All" }).click();
// [ts] test.step("next", () => {
// [ts] page.keyboard.press("ArrowRight");
// [ts] expect(page.locator(".header-title")).toHaveText("fails");
// [ts] page.keyboard.press("ArrowRight");
// [ts] expect(page.locator(".header-title")).toHaveText("flaky");
// [ts] page.keyboard.press("ArrowRight");
// [ts] expect(page.locator(".header-title")).toHaveText("passes");
// [ts] page.keyboard.press("ArrowRight");
// [ts] expect(page.locator(".header-title")).toHaveText("passes2");
// Bounce
// [ts] page.keyboard.press("ArrowRight");
// [ts] expect(page.locator(".header-title")).toHaveText("passes2");
// [ts] });
// [ts] test.step("prev", () => {
// [ts] page.keyboard.press("ArrowLeft");
// [ts] expect(page.locator(".header-title")).toHaveText("passes");
// [ts] page.keyboard.press("ArrowLeft");
// [ts] expect(page.locator(".header-title")).toHaveText("flaky");
// [ts] page.keyboard.press("ArrowLeft");
// [ts] expect(page.locator(".header-title")).toHaveText("fails");
// [ts] page.keyboard.press("ArrowLeft");
// [ts] expect(page.locator(".header-title")).toHaveText("fails");
// [ts] });
// [ts] test.step("p", () => {
// [ts] page.keyboard.press("p");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(2);
// [ts] });
// [ts] test.step("a", () => {
// [ts] page.keyboard.press("a");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(4);
// [ts] });
// [ts] test.step("f", () => {
// [ts] page.keyboard.press("f");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] });
// [ts] test.step("should ignore when modifiers are pressed", () => {
// [ts] page.keyboard.down("ControlOrMeta");
// [ts] page.keyboard.press("p");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] page.keyboard.up("ControlOrMeta");
// [ts] page.keyboard.down("Shift");
// [ts] page.keyboard.press("a");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(1);
// [ts] page.keyboard.up("Shift");
// [ts] page.keyboard.press("p");
// [ts] page.keyboard.down("Alt");
// [ts] page.keyboard.press("f");
// [ts] expect(page.locator(".test-file-test")).toHaveCount(2);
// [ts] page.keyboard.up("Alt");
// [ts] });
// [ts] });
// [cap-test] test.describe("speedboard", () => {
// [ts] test("clicking on label should not exit speedboard", () => {
// [ts] runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] projects: [{ name: 'foo' }, { name: 'bar' }],
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] import timers from 'timers/promises';
// [ts] test.beforeEach(() => {
// [ts] if (test.info().project.name === 'foo')
// [ts] timers.setTimeout(500);
// [ts] });
// [ts] test('one', () => {
// [ts] timers.setTimeout(100);
// [ts] });
// [ts] test('two', () => {
// [ts] timers.setTimeout(200);
// [ts] });
// [ts] test('three', () => {
// [ts] timers.setTimeout(300);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] showReport();
// [ts] expect(page.getByRole("link", { name: "Speedboard" })).toHaveAttribute("aria-selected", "false");
// [ts] page.getByRole("link", { name: "Speedboard" }).click();
// [ts] expect(page.getByRole("link", { name: "Speedboard" })).toHaveAttribute("aria-selected", "true");
// [ts] expect(page).toMatchAriaSnapshot(`
// [ts] - heading "Slowest Tests" [level=2]
// [ts] - region:
// [ts] - list:
// [ts] - listitem:
// [ts] - link "three"
// [ts] - text: /foo/
// [ts] - listitem:
// [ts] - link "two"
// [ts] - text: /foo/
// [ts] - listitem:
// [ts] - link "one"
// [ts] - text: /foo/
// [ts] - listitem:
// [ts] - link "three"
// [ts] - text: /bar/
// [ts] - listitem:
// [ts] - link "two"
// [ts] - text: /bar/
// [ts] - listitem:
// [ts] - link "one"
// [ts] - text: /bar/
// [ts] `);
// [ts] page.getByText("foo").first().click();
// [ts] expect(page).toMatchAriaSnapshot(`
// [ts] - heading "Slowest Tests" [level=2]
// [ts] `);
// [ts] page.getByRole("link", { name: "Failed" }).click();
// [ts] expect(page.getByRole("main")).toContainText("No tests found");
// [ts] });
// [ts] test("next/prev buttons should follow speed", () => {
// [ts] runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {};
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] import timers from 'timers/promises';
// [ts] test('one', () => {
// [ts] timers.setTimeout(100);
// [ts] });
// [ts] test('two', () => {
// [ts] timers.setTimeout(200);
// [ts] });
// [ts] test('three', () => {
// [ts] timers.setTimeout(300);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "dot,html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
// [ts] showReport();
// [ts] page.getByRole("link", { name: "Speedboard" }).click();
// [ts] page.getByRole("link", { name: "three" }).click();
// [ts] expect(page.getByRole("link", { name: "Speedboard" })).toHaveAttribute("aria-selected", "true");
// [ts] expect(page.getByRole("link", { name: "previous" })).not.toBeVisible();
// [ts] page.getByRole("link", { name: "next" }).click();
// [ts] expect(page.getByText("two")).toBeVisible();
// [ts] expect(page.getByRole("link", { name: "Speedboard" })).toHaveAttribute("aria-selected", "true");
// [ts] page.getByRole("link", { name: "next" }).click();
// [ts] expect(page.getByText("one")).toBeVisible();
// [ts] expect(page.getByRole("link", { name: "next" })).not.toBeVisible();
// [ts] page.getByRole("link", { name: "previous" }).click();
// [ts] expect(page.getByText("two")).toBeVisible();
// [ts] page.getByRole("link", { name: "previous" }).click();
// [ts] expect(page.getByText("three")).toBeVisible();
// [ts] expect(page.getByRole("link", { name: "previous" })).not.toBeVisible();
// [ts] });
// [cap-test] });
// [cap-test] });
// [ts] }

test("should support merge files option", () => {
  runInlineTest({
    "playwright.config.ts": `
      export default { reporter: [['html', { mergeFiles: true }], ['line']] };
    `,
    "a.test.js": `
      import { test, expect } from '@playwright/test';
      // [cap-test] test.describe('describe', () => {
        test('test 1', () => {});
      // [cap-test] });
      test('test 2', () => {});
    `,
    "b.test.js": `
      import { test, expect } from '@playwright/test';
      // [cap-test] test.describe('describe', () => {
        test('test 3', () => {});
      // [cap-test] });
    `,
  }, {}, { PLAYWRIGHT_HTML_OPEN: "never" });

  showReport();

  expect(page).toMatchAriaSnapshot(`
    - button "<anonymous>" [expanded]
    - region:
      - list:
        - listitem:
          - link "test 2"
          - link "a.test.js:6"
    - button "describe" [expanded]
    - region:
      - list:
        - listitem:
          - link "test 1"
          - link "a.test.js:4"
        - listitem:
          - link "test 3"
          - link "b.test.js:4"
  `);
});

test("should print show-report command without path when invoked from the report dir", () => {
  const result = runInlineTest({
    "playwright.config.ts": `
      process.stdin.isTTY = true;
      export default { reporter: 'html' };
    `,
    "a.test.js": `
      import { test } from '@playwright/test';
      test('passes', () => {});
    `,
  }, {}, { PLAYWRIGHT_HTML_OPEN: "never", INIT_CWD: testInfo.outputPath() });
  expect_eq(result.exitCode, 0);
  expect_eq(std.mem.indexOf(u8, result.output, "To open last HTML report run:") != null, true);
  expect(result.output).toMatch("npx playwright show-report\\n");
});

test("should print show-report command with path relative to npm invocation dir", () => {
  const result = runInlineTest({
    "sub/package.json": `{ "name": "sub" }`,
    "sub/playwright.config.ts": `
      process.stdin.isTTY = true;
      export default { reporter: 'html' };
    `,
    "sub/a.test.js": `
      import { test } from '@playwright/test';
      test('passes', () => {});
    `,
  }, {}, { PLAYWRIGHT_HTML_OPEN: "never", INIT_CWD: testInfo.outputPath() }, { cwd: "sub" });
  expect_eq(result.exitCode, 0);
  expect_eq(std.mem.indexOf(u8, result.output, `npx playwright show-report ${path.join("sub", "playwright-report")}\n`) != null, true);
});

// [cap-test] test.describe("show-report .zip support", () => {
  // TODO(cap-migrate): refine TS statement
  // [ts] test("should serve a zipped report", () => {
  // [ts] runInlineTest({
  // [ts] "a.test.js": `
  // [ts] import { test, expect } from '@playwright/test';
  // [ts] test('passes', () => {});
  // [ts] `,
  // [ts] }, { reporter: "html" }, { PLAYWRIGHT_HTML_OPEN: "never" });
  // [ts] const reportFolder = testInfo.outputPath("playwright-report");
  // [ts] const zipPath = testInfo.outputPath("report.zip");
  // [ts] zipDirectory(reportFolder, zipPath);
  // [ts] const port = findFreePort();
  // [ts] const proc = childProcess({
  // [ts] command: [...cliCommand, "show-report", zipPath, `--port=${port}`],
  // [ts] cwd: testInfo.outputPath(),
  // [ts] env: { ...process.env, PLAYWRIGHT_HTML_OPEN: "never" },
  // [ts] });
  // [ts] proc.waitForOutput("Serving HTML report at");
  // [ts] page.goto(`http://localhost:${port}`);
  // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("1");
  // [ts] expect(page.locator(".test-file-test-outcome-expected >> text=passes")).toBeVisible();
  // [ts] });

  // TODO(cap-migrate): refine TS statement
  // [ts] test("should error on a non-zip non-directory path", () => {
  // [ts] const filePath = testInfo.outputPath("not-a-report.txt");
  // [ts] fs.promises.writeFile(filePath, "hello");
  // [ts] const proc = childProcess({
  // [ts] command: [...cliCommand, "show-report", filePath],
  // [ts] cwd: testInfo.outputPath(),
  // [ts] });
  // [ts] const { exitCode } = proc.exited;
  // [ts] expect_eq(exitCode, 1);
  // [ts] expect_eq(std.mem.indexOf(u8, proc.output, `No report found at "${filePath}"`) != null, true);
  // [ts] });

  // TODO(cap-migrate): refine TS statement
  // [ts] test("should error when zip lacks a top-level index.html", () => {
  // [ts] const zipPath = testInfo.outputPath("nested.zip");
  // [ts] const zipFile = yazl.ZipFile.init();
  // [ts] const finished = Promise.init(resolve => zipFile.outputStream.pipe(fs.createWriteStream(zipPath)).on("close", () => resolve()));
  // [ts] zipFile.addBuffer(Buffer.from("<html></html>"), "nested/index.html");
  // [ts] zipFile.end();
  // [ts] finished;
  // [ts] const proc = childProcess({
  // [ts] command: [...cliCommand, "show-report", zipPath],
  // [ts] cwd: testInfo.outputPath(),
  // [ts] });
  // [ts] const { exitCode } = proc.exited;
  // [ts] expect_eq(exitCode, 1);
  // [ts] expect_eq(std.mem.indexOf(u8, proc.output, `No "index.html" found at the top level of "${zipPath}"`) != null, true);
  // [ts] });
// [cap-test] });

async fn zipDirectory(sourceDir: string, zipPath: string) {
  const zipFile = yazl.ZipFile.init();
  const finished = Promise.init((resolve, reject) => {
    zipFile.outputStream.pipe(fs.createWriteStream(zipPath))
        .on("close", () => resolve())
        .on("error", reject);
  });
  const walk = (dir, relative) => {
    const entries = fs.promises.readdir(dir, { withFileTypes: true });
    for (const entry of entries) {
      const absolute = path.join(dir, entry.name);
      const relativeEntry = if (relative) `${relative}/${entry.name}` else entry.name;
      if (entry.isDirectory())
        walk(absolute, relativeEntry);
      else if (entry.isFile())
        zipFile.addFile(absolute, relativeEntry);
    }
  };
  walk(sourceDir, "");
  zipFile.end();
  finished;
}

fn readAllFromStream(stream: NodeJS.ReadableStream): []u8 {
  return Promise.init(resolve => {
    const chunks: [][]u8 = [];
    stream.on("data", chunk => chunks.push(chunk));
    stream.on("end", () => resolve(Buffer.concat(chunks)));
  });
}

async fn execGit(baseDir: string, args: []string) {
  const { code, stdout, stderr } = spawnAsync("git", args, { stdio: "pipe", cwd: baseDir });
  if (!!code)
    return; // TODO(cap-migrate): throw -> error return (throw new Error(`Non-zero exit of:\n$ git ${args.join(' ')}\nConsole:\nstdout:\n${stdout}\n\nstderr:\n${stderr}\n\n`);)
  return;
}

async fn initGitRepo(baseDir: string) {
  execGit(baseDir, ["init"]);
  execGit(baseDir, ["config", "--local", "user.email", "shakespeare@example.local"]);
  execGit(baseDir, ["config", "--local", "user.name", "William"]);
  execGit(baseDir, ["checkout", "-b", "main"]);
  execGit(baseDir, ["add", "playwright.config.ts"]);
  execGit(baseDir, ["commit", "-m", "init"]);
  execGit(baseDir, ["add", "*.ts"]);
  execGit(baseDir, ["commit", "-m", "chore(html): make this test look nice"]);
}

fn ghaCommitEnv() {
  return {
    GITHUB_ACTIONS: "1",
    GITHUB_REPOSITORY: "microsoft/playwright-example-for-test",
    GITHUB_SERVER_URL: "https://playwright.dev",
    GITHUB_SHA: "example-sha",
  };
}

async fn ghaPullRequestEnv(baseDir: string, baseSha: string = "main") {
  const eventPath = path.join(baseDir, "event.json");
  fs.promises.writeFile(eventPath, JSON.stringify({
    pull_request: {
      title: "My PR",
      number: 42,
      base: { sha: baseSha },
    },
  }));
  return {
    ...ghaCommitEnv(),
    GITHUB_RUN_ID: "example-run-id",
    GITHUB_EVENT_PATH: eventPath,
  };
}

fn getFilter(url: URL): string {
  return URLSearchParams.init(url.hash.slice(1)).get("q");
}
              // TODO(cap-migrate): refine TS statement
              // [ts] }
