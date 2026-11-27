// Migrated from TypeScript (playwright-test/playwright.spec.ts) via cap from -ts

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

import { test, expect, parseTestRunnerOutput } from "./playwright-test-fixtures.cp";
import fs from "cap:fs";
import path from "cap:path";
import { spawnSync } from "cap:process";
import { registry } from "../../packages/playwright-core/lib/coreBundle.cp";

const ffmpeg = registry.registry.findExecutable("ffmpeg").executablePath();

pub class VideoPlayer {
  videoWidth: f64 = 0.0;
  videoHeight: f64 = 0.0;
  fps: f64 = 0.0;

  pub static fn init(fileName: string): VideoPlayer {
      let self_val: VideoPlayer = VideoPlayer{};
    const output = spawnSync(ffmpeg, ["-i", fileName, "-r", "25", `${fileName}-%03d.png`]).stderr.toString();
    const lines = output.split("\n");
    const streamLine = lines.find(l => l.trim().startsWith("Stream #0:0"));
    const resolutionMatch = streamLine.match(", (\\d+)x(\\d+),");
    self_val.videoWidth = parseInt(resolutionMatch[1], 10);
    self_val.videoHeight = parseInt(resolutionMatch[2], 10);
    self_val.fps = parseFloat(streamLine.match(", ([\\d.]+) fps,")[1]);
      return self_val;
  }
}









































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should respect screen option", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { viewport: { width: 500, height: 500 }, screen: { width: 1000, height: 1000 } } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(screen, { width: 1000, height: 1000 });
    // [ts] expect_eq(page.evaluate(() => [window.screen.width, window.screen.height]), [1000, 1000]);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect, devices } from '@playwright/test';
    // [ts] test.use({ ...devices['iPhone 13'] });
    // [ts] test('pass', () => {
    // [ts] expect_eq(screen, { width: 390, height: 844 });
    // [ts] expect_eq(page.evaluate(() => [window.screen.width, window.screen.height]), [390, 844]);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] });
    // [ts] test("should respect viewport option", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { viewport: { width: 800, height: 800 } } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(page.viewportSize(), { width: 800, height: 800 });
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.use({ viewport: { width: 600, height: 600 } });
    // [ts] test('pass', () => {
    // [ts] expect_eq(page.viewportSize(), { width: 600, height: 600 });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] });
    // [ts] test("should run in three browsers with --browser", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { viewport: { width: 800, height: 800 } } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] console.log('\\n%%browser=' + browserName);
    // [ts] });
    // [ts] `,
    // [ts] }, { browser: "all", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // [ts] expect(result.outputLines.sort()).toEqual([
    // [ts] "browser=chromium",
    // [ts] "browser=firefox",
    // [ts] "browser=webkit",
    // [ts] ]);
    // [ts] });
    // [ts] test("should run in one browser with --browser", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { viewport: { width: 800, height: 800 } } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] console.log('\\n%%browser=' + browserName);
    // [ts] });
    // [ts] `,
    // [ts] }, { browser: "webkit", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect(result.outputLines.sort()).toEqual([
    // [ts] "browser=webkit",
    // [ts] ]);
    // [ts] });
    // [ts] test("should complain with projects and --browser", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [ {} ] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { browser: "webkit", workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Cannot use --browser option when configuration file defines projects") != null, true);
    // [ts] });
    // [ts] test("should override any headless option with --headed", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [
    // [ts] { name: 'a', use: { headless: true } }
    // [ts] ] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example', () => {
    // [ts] expect_eq(std.mem.indexOf(u8, page.evaluate(() => navigator.userAgent), 'Headless') == null, true);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, headed: true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should not override use:browserName without projects", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { browserName: 'webkit' } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] console.log('\\n%%browser=' + browserName);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect(result.outputLines.sort()).toEqual([
    // [ts] "browser=webkit",
    // [ts] ]);
    // [ts] });
    // [ts] test("should override use:browserName with --browser", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { browserName: 'webkit' } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] console.log('\\n%%browser=' + browserName);
    // [ts] });
    // [ts] `,
    // [ts] }, { browser: "firefox", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect(result.outputLines.sort()).toEqual([
    // [ts] "browser=firefox",
    // [ts] ]);
    // [ts] });
    // [ts] test("should respect context options in various contexts", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { viewport: { width: 500, height: 500 } } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import fs from 'fs';
    // [ts] import os from 'os';
    // [ts] import path from 'path';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.use({ locale: 'fr-FR' });
    // [ts] let context;
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] context = browser.newContext();
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] context.close();
    // [ts] });
    // [ts] test('shared context', () => {
    // [ts] const page = context.newPage();
    // [ts] expect_eq(page.viewportSize(), { width: 500, height: 500 });
    // [ts] expect_eq(page.evaluate(() => navigator.language), 'fr-FR');
    // [ts] });
    // [ts] test('own context', () => {
    // [ts] const page = browser.newPage();
    // [ts] expect_eq(page.viewportSize(), { width: 500, height: 500 });
    // [ts] expect_eq(page.evaluate(() => navigator.language), 'fr-FR');
    // [ts] page.close();
    // [ts] });
    // [ts] test('default context', () => {
    // [ts] expect_eq(page.viewportSize(), { width: 500, height: 500 });
    // [ts] expect_eq(page.evaluate(() => navigator.language), 'fr-FR');
    // [ts] });
    // [ts] test('persistent context', () => {
    // [ts] const dir = fs.promises.mkdtemp(path.join(os.tmpdir(), 'user-data-dir-'));
    // [ts] const context = playwright[browserName].launchPersistentContext(dir);
    // [ts] const page = context.pages()[0];
    // [ts] expect_eq(page.viewportSize(), { width: 500, height: 500 });
    // [ts] expect_eq(page.evaluate(() => navigator.language), 'fr-FR');
    // [ts] context.close();
    // [ts] fs.rmSync(dir, { recursive: true, force: true, maxRetries: 10 });
    // [ts] });
    // [ts] test('another browser', () => {
    // [ts] const browser = playwright.webkit.launch();
    // [ts] const page = browser.newPage();
    // [ts] expect_eq(page.evaluate(() => navigator.language), 'fr-FR');
    // [ts] browser.close();
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 5);
    // [ts] });
    // [ts] test("should respect headless in launchPersistent", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { headless: false } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import fs from 'fs';
    // [ts] import os from 'os';
    // [ts] import path from 'path';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('persistent context', () => {
    // [ts] const dir = fs.promises.mkdtemp(path.join(os.tmpdir(), 'user-data-dir-'));
    // [ts] const context = playwright[browserName].launchPersistentContext(dir);
    // [ts] const page = context.pages()[0];
    // [ts] expect_eq(std.mem.indexOf(u8, page.evaluate(() => navigator.userAgent), 'Headless') == null, true);
    // [ts] context.close();
    // [ts] fs.rmSync(dir, { recursive: true, force: true, maxRetries: 10 });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should respect headless in modifiers that run before tests", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { headless: false } };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test(({ browser }) => false);
    // [ts] test('should work', () => {
    // [ts] expect_eq(std.mem.indexOf(u8, page.evaluate(() => navigator.userAgent), 'Headless') == null, true);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should call logger from launchOptions config", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const log = [];
    // [ts] test.use({
    // [ts] launchOptions: {
    // [ts] logger: {
    // [ts] log: (name, severity, message) => log.push({name, severity, message}),
    // [ts] isEnabled: (name, severity) => severity !== 'verbose'
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test('should support config logger', () => {
    // [ts] expect_eq(browser.version(), true);
    // [ts] expect_eq(log.length > 0, true);
    // [ts] expect_eq(log.filter(item => item.severity === 'info').length > 0, true);
    // [ts] expect_eq(log.filter(item => item.message.includes('browser.newContext started')).length > 0, true);
    // [ts] expect_eq(log.filter(item => item.message.includes('browser.newContext succeeded')).length > 0, true);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should report error and pending operations on timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('timedout', () => {
    // [ts] page.setContent('<div>Click me</div>');
    // [ts] page.getByText('Missing').click();
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: locator.click: Test timeout of 2000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:5:41") != null, true);
    // [ts] });
    // [ts] test("should report error on timeout with shared page", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] let page;
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] page = browser.newPage();
    // [ts] });
    // [ts] test('passed', () => {
    // [ts] page.setContent('<div>Click me</div>');
    // [ts] });
    // [ts] test('timedout', () => {
    // [ts] page.getByText('Missing').click();
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "waiting for getByText('Missing')") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `11 |         await page.getByText('Missing').click();`) != null, true);
    // [ts] });
    // [ts] test("should report error from beforeAll timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] const page = browser.newPage();
    // [ts] page.setContent('<div>Click me</div>');
    // [ts] Promise.all([
    // [ts] page.getByText('Missing').click(),
    // [ts] page.getByText('More missing').textContent(),
    // [ts] ]);
    // [ts] });
    // [ts] test('ignored', () => {});
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "\"beforeAll\" hook timeout of 2000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "waiting for") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `8 |           page.getByText('More missing').textContent(),`) != null, true);
    // [ts] });
    // [ts] test("should not report waitForEventInfo as pending", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('timedout', () => {
    // [ts] page.setContent('<div>Click me</div>');
    // [ts] page.waitForLoadState('networkidle');
    // [ts] page.click('text=Missing');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 5000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "page.click") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:6:20") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "- page.waitForLoadState") == null, true);
    // [ts] });
    // [ts] test("should throw when using page in beforeAll", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeAll(() => {});
    // [ts] test.beforeAll(({ page }) => {
    // [ts] });
    // [ts] test('ok', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: "context" and "page" fixtures are not supported in "beforeAll"`) != null, true);
    // [ts] });
    // [ts] test("should report click error on sigint", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('timedout', () => {
    // [ts] page.setContent('<div>Click me</div>');
    // [ts] const promise = page.click('text=Missing');
    // [ts] new Promise(f => setTimeout(f, 100));
    // [ts] console.log('\\n%%SEND-SIGINT%%');
    // [ts] promise;
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.interrupted, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `5 |         const promise = page.click('text=Missing');`) != null, true);
    // [ts] });
    // [ts] test("should work with video: retain-on-failure", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { video: 'retain-on-failure' }, name: 'chromium' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] page.waitForTimeout(3000);
    // [ts] test.expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] page.setContent('<div>FAIL</div>');
    // [ts] page.waitForTimeout(3000);
    // [ts] test.expect_eq(1 + 1, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const dirPass = test.info().outputPath("test-results", "a-pass-chromium");
    // [ts] const videoPass = if (fs.existsSync(dirPass)) fs.readdirSync(dirPass).find(file => file.endsWith("webm")) else null;
    // [ts] expect_eq(videoPass, false);
    // [ts] const videoFail = fs.readdirSync(test.info().outputPath("test-results", "a-fail-chromium")).find(file => file.endsWith("webm"));
    // [ts] expect_eq(videoFail, true);
    // [ts] });
    // [ts] test("should work with video: on-first-retry", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { video: 'on-first-retry' }, retries: 1, name: 'chromium' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] page.waitForTimeout(3000);
    // [ts] test.expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] page.setContent('<div>FAIL</div>');
    // [ts] page.waitForTimeout(3000);
    // [ts] test.expect_eq(1 + 1, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const dirPass = test.info().outputPath("test-results", "a-pass-chromium");
    // [ts] expect_eq(fs.existsSync(dirPass), false);
    // [ts] const dirFail = test.info().outputPath("test-results", "a-fail-chromium");
    // [ts] expect_eq(fs.readdirSync(dirFail), ["error-context.md"]);
    // [ts] const dirRetry = test.info().outputPath("test-results", "a-fail-chromium-retry1");
    // [ts] const videoFailRetry = fs.readdirSync(dirRetry).find(file => file.endsWith("webm"));
    // [ts] expect_eq(videoFailRetry, true);
    // [ts] const errorPrompt = expect.objectContaining({ name: "error-context" });
    // [ts] expect_eq(result.report.suites[0].specs[1].tests[0].results[0].attachments, [errorPrompt]);
    // [ts] expect: (result.report.suites[0].specs[1].tests[0].results[1].attachments) => {
    // [ts] name: "video",
    // [ts] contentType: "video/webm",
    // [ts] path: path.join(dirRetry, videoFailRetry),
    // [ts] }, errorPrompt]);
    // [ts] });
    // [ts] test("should work with video: on-all-retries", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { video: 'on-all-retries' }, retries: 2, name: 'chromium' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.setContent('<div>FAIL</div>');
    // [ts] page.waitForTimeout(1000);
    // [ts] test.expect_eq(1 + 1, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const dirFail = test.info().outputPath("test-results", "a-fail-chromium");
    // [ts] expect_eq(fs.readdirSync(dirFail).find(file => file.endsWith("webm")), false);
    // [ts] const dirRetry1 = test.info().outputPath("test-results", "a-fail-chromium-retry1");
    // [ts] expect_eq(fs.readdirSync(dirRetry1).find(file => file.endsWith("webm")), true);
    // [ts] const dirRetry2 = test.info().outputPath("test-results", "a-fail-chromium-retry2");
    // [ts] expect_eq(fs.readdirSync(dirRetry2).find(file => file.endsWith("webm")), true);
    // [ts] });
    // [ts] test("should work with video: retain-on-first-failure", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { video: 'retain-on-first-failure' }, retries: 1, name: 'chromium' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] page.waitForTimeout(1000);
    // [ts] test.expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] page.setContent('<div>FAIL</div>');
    // [ts] page.waitForTimeout(1000);
    // [ts] test.expect_eq(1 + 1, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const dirPass = test.info().outputPath("test-results", "a-pass-chromium");
    // [ts] const videoPass = if (fs.existsSync(dirPass)) fs.readdirSync(dirPass).find(file => file.endsWith("webm")) else null;
    // [ts] expect_eq(videoPass, false);
    // First run failed, so the video is retained.
    // [ts] const dirFail = test.info().outputPath("test-results", "a-fail-chromium");
    // [ts] expect_eq(fs.readdirSync(dirFail).find(file => file.endsWith("webm")), true);
    // No video is captured on retries.
    // [ts] const dirRetry = test.info().outputPath("test-results", "a-fail-chromium-retry1");
    // [ts] expect_eq(fs.readdirSync(dirRetry).find(file => file.endsWith("webm")), false);
    // [ts] });
    // [ts] test("should show a single test overlay when a step starts right after page creation", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { video: { mode: 'on', show: { test: { level: 'step' } } } }, name: 'chromium' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('my test', () => {
    // [ts] test.step('first step', () => {
    // [ts] const lines = page.locator('.x-pw-user-overlay').evaluateAll(els => els.map(el => el.innerText.split('\\n')));
    // [ts] expect_eq(lines, [['a.test.ts', 'my test', 'first step']]);
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should work with video: retain-on-failure-and-retries", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { video: 'retain-on-failure-and-retries' }, retries: 1, name: 'chromium' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('flaky', () => {
    // [ts] page.setContent('<div>FLAKY</div>');
    // [ts] page.waitForTimeout(1000);
    // [ts] test.expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.flaky, 1);
    // First attempt failed, video retained.
    // [ts] const dirFail = test.info().outputPath("test-results", "a-flaky-chromium");
    // [ts] expect_eq(fs.readdirSync(dirFail).find(file => file.endsWith("webm")), true);
    // Retry passed, but all videos are retained once the test was retried.
    // [ts] const dirRetry = test.info().outputPath("test-results", "a-flaky-chromium-retry1");
    // [ts] expect_eq(fs.readdirSync(dirRetry).find(file => file.endsWith("webm")), true);
    // [ts] });
    // [ts] test("should work with video size", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] use: { video: { mode: 'on', size: { width: 220, height: 110 } } },
    // [ts] name: 'chromium',
    // [ts] preserveOutput: 'always',
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] page.waitForTimeout(3000);
    // [ts] test.expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const folder = test.info().outputPath(`test-results/a-pass-chromium/`);
    // [ts] const [file] = fs.readdirSync(folder);
    // [ts] const videoPlayer = VideoPlayer.init(path.join(folder, file));
    // [ts] expect_eq(videoPlayer.videoWidth, 220);
    // [ts] expect_eq(videoPlayer.videoHeight, 110);
    // [ts] });
    // [ts] test("should work with video fps", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] use: { video: { mode: 'on', fps: 60 } },
    // [ts] name: 'chromium',
    // [ts] preserveOutput: 'always',
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] page.waitForTimeout(1000);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const folder = test.info().outputPath(`test-results/a-pass-chromium/`);
    // [ts] const [file] = fs.readdirSync(folder);
    // [ts] const videoPlayer = VideoPlayer.init(path.join(folder, file));
    // [ts] expect_eq(videoPlayer.fps, 60);
    // [ts] });
    // [ts] test("should work with video.path() throwing", () => {
    // When running remotely, video.path() is not available, so we must not use it.
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] use: { video: { mode: 'on' } },
    // [ts] name: 'chromium',
    // [ts] preserveOutput: 'always',
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.video().path = () => { throw new Error('No-no!'); };
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] page.waitForTimeout(3000);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const dir = testInfo.outputPath(`test-results/a-pass-chromium/`);
    // [ts] const video = fs.readdirSync(dir).find(file => file.endsWith("webm"));
    // [ts] expect_eq(video, true);
    // [ts] });
    // [ts] test("should pass fixture defaults to tests", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {};
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(acceptDownloads, true);
    // [ts] expect_eq(actionTimeout, 0);
    // [ts] expect_eq(headless, true);
    // [ts] expect_eq(javaScriptEnabled, true);
    // [ts] expect_eq(navigationTimeout, 0);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should not throw with many fixtures set to undefined", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: {
    // [ts] browserName: undefined,
    // [ts] headless: undefined,
    // [ts] channel: undefined,
    // [ts] launchOptions: undefined,
    // [ts] connectOptions: undefined,
    // [ts] screenshot: undefined,
    // [ts] video: undefined,
    // [ts] trace: undefined,
    // [ts] acceptDownloads: undefined,
    // [ts] bypassCSP: undefined,
    // [ts] colorScheme: undefined,
    // [ts] deviceScaleFactor: undefined,
    // [ts] extraHTTPHeaders: undefined,
    // [ts] geolocation: undefined,
    // [ts] hasTouch: undefined,
    // [ts] httpCredentials: undefined,
    // [ts] ignoreHTTPSErrors: undefined,
    // [ts] isMobile: undefined,
    // [ts] javaScriptEnabled: undefined,
    // [ts] locale: undefined,
    // [ts] offline: undefined,
    // [ts] permissions: undefined,
    // [ts] proxy: undefined,
    // [ts] storageState: undefined,
    // [ts] timezoneId: undefined,
    // [ts] userAgent: undefined,
    // [ts] viewport: undefined,
    // [ts] actionTimeout: undefined,
    // [ts] testIdAttribute: undefined,
    // [ts] navigationTimeout: undefined,
    // [ts] baseURL: undefined,
    // [ts] serviceWorkers: undefined,
    // [ts] contextOptions: undefined,
    // [ts] } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.use({
    // [ts] browserName: undefined,
    // [ts] headless: undefined,
    // [ts] channel: undefined,
    // [ts] launchOptions: undefined,
    // [ts] connectOptions: undefined,
    // [ts] screenshot: undefined,
    // [ts] video: undefined,
    // [ts] trace: undefined,
    // [ts] acceptDownloads: undefined,
    // [ts] bypassCSP: undefined,
    // [ts] colorScheme: undefined,
    // [ts] deviceScaleFactor: undefined,
    // [ts] extraHTTPHeaders: undefined,
    // [ts] geolocation: undefined,
    // [ts] hasTouch: undefined,
    // [ts] httpCredentials: undefined,
    // [ts] ignoreHTTPSErrors: undefined,
    // [ts] isMobile: undefined,
    // [ts] javaScriptEnabled: undefined,
    // [ts] locale: undefined,
    // [ts] offline: undefined,
    // [ts] permissions: undefined,
    // [ts] proxy: undefined,
    // [ts] storageState: undefined,
    // [ts] timezoneId: undefined,
    // [ts] userAgent: undefined,
    // [ts] viewport: undefined,
    // [ts] actionTimeout: undefined,
    // [ts] testIdAttribute: undefined,
    // [ts] navigationTimeout: undefined,
    // [ts] baseURL: undefined,
    // [ts] serviceWorkers: undefined,
    // [ts] contextOptions: undefined,
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] page.setContent('text');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should have strict types for options but allow use(undefined)", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.use({
    // [ts] headless: undefined,
    // [ts] acceptDownloads: undefined,
    // [ts] bypassCSP: undefined,
    // [ts] hasTouch: undefined,
    // [ts] ignoreHTTPSErrors: undefined,
    // [ts] isMobile: undefined,
    // [ts] javaScriptEnabled: undefined,
    // [ts] offline: undefined,
    // [ts] actionTimeout: undefined,
    // [ts] navigationTimeout: undefined,
    // [ts] testIdAttribute: undefined,
    // [ts] });
    // [ts] test('my test', ({
    // [ts] headless, acceptDownloads, bypassCSP,
    // [ts] hasTouch, ignoreHTTPSErrors, isMobile, javaScriptEnabled, offline,
    // [ts] actionTimeout, navigationTimeout, testIdAttribute }) => {
    // [ts] test(headless, 'boolean');
    // [ts] test(acceptDownloads, 'boolean');
    // [ts] test(bypassCSP, 'boolean');
    // [ts] test(hasTouch, 'boolean');
    // [ts] test(ignoreHTTPSErrors, 'boolean');
    // [ts] test(isMobile, 'boolean');
    // [ts] test(javaScriptEnabled, 'boolean');
    // [ts] test(offline, 'boolean');
    // [ts] test(actionTimeout > 0, 'number');
    // [ts] test(navigationTimeout > 0, 'number');
    // [ts] test(testIdAttribute.length > 0, 'string');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should skip on mobile", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [cap-test] test.describe(() => {
    // [ts] test.use({ isMobile: true });
    // [ts] test('test 1', () => {
    // [ts] test(isMobile, 'desktop only!');
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe(() => {
    // [ts] test('test 2', () => {
    // [ts] test(isMobile, 'desktop only!');
    // [ts] });
    // [cap-test] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.skipped, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should use actionTimeout for APIRequestContext", () => {
    // [ts] server.setRoute("/stall", (req, res) => {});
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] use: {
    // [ts] actionTimeout: 1111,
    // [ts] baseURL: '${server.PREFIX}',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('default APIRequestContext fixture', () => {
    // [ts] expect(request.get('/stall')).rejects.toThrow('apiRequestContext.get: Timeout 1111ms exceeded');
    // [ts] });
    // [ts] test('newly created APIRequestContext without options', () => {
    // [ts] const apiRequestContext = playwright.request.newContext();
    // [ts] expect(apiRequestContext.get('/stall')).rejects.toThrow('apiRequestContext.get: Timeout 1111ms exceeded');
    // [ts] });
    // [ts] test('newly created APIRequestContext with options', () => {
    // [ts] const apiRequestContextWithOptions = playwright.request.newContext({ httpCredentials: { username: 'user', password: 'pass' } });
    // [ts] expect(apiRequestContextWithOptions.get('/stall')).rejects.toThrow('apiRequestContext.get: Timeout 1111ms exceeded');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // [ts] });
    // [ts] test("should save trace in two APIRequestContexts", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] timeout: 5000,
    // [ts] use: {
    // [ts] trace: 'on',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, request, BrowserContext, Page, APIRequestContext } from '@playwright/test';
    // [cap-test] test.describe('Example', () => {
    // [ts] let firstContext: APIRequestContext;
    // [ts] let secondContext: APIRequestContext;
    // [ts] let context: BrowserContext;
    // [ts] let page: Page;
    // [ts] test.beforeAll(() => {
    // [ts] firstContext = request.newContext({ baseURL: 'http://example.com' });
    // [ts] secondContext = request.newContext({ baseURL: 'http://example.com' });
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] console.log('afterAll start');
    // [ts] firstContext.dispose();
    // [ts] console.log('afterAll middle');
    // [ts] secondContext.dispose();
    // [ts] console.log('afterAll end');
    // [ts] });
    // [cap-test] test.describe('inner tests', () => {
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] context = browser.newContext();
    // [ts] page = context.newPage();
    // [ts] page.goto('${server.EMPTY_PAGE}');
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] page.close();
    // [ts] context.close();
    // [ts] });
    // [ts] test('test', () => {});
    // [cap-test] });
    // [cap-test] })
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should explain a failure when using a dispose APIRequestContext", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] let context;
    // [ts] test.beforeAll(({ request }) => {
    // [ts] context = request;
    // [ts] });
    // [ts] test('test', () => {
    // [ts] context.fetch('http://example.com');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Recommended fix: use a separate { request } in the test`) != null, true);
    // [ts] });
    // [ts] test("should allow dynamic import in evaluate", () => {
    // [ts] server.setRoute("/foo.js", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "application/javascript" }).end(`
    // [ts] export const foo = 'bar';
    // [ts] `);
    // [ts] });
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.goto("${server.EMPTY_PAGE}");
    // [ts] const result = page.evaluate(() => {
    // [ts] const { foo } = import("${server.PREFIX + '/foo.js'}");
    // [ts] return foo;
    // [ts] });
    // [ts] expect_eq(result, 'bar');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("page.pause() should disable test timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] test.setTimeout(4000);
    // [ts] Promise.race([
    // [ts] page.pause(),
    // [ts] new Promise(f => setTimeout(f, 5000)),
    // [ts] ]);
    // [ts] console.log('success!');
    // [ts] });
    // [ts] `,
    // [ts] }, { headed: true });  // This needs to be headed otherwise entire worker is gone.
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "success!") != null, true);
    // [ts] });
    // [ts] test("window.playwright should be undefined by default", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent('<body></body>');
    // [ts] expect_eq(page.evaluate(() => window.playwright) == null, true);
    // [ts] });
    // [ts] `,
    // [ts] }, {}, {});
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("window.playwright should not override existing property", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent('<script>window.playwright = "foo"</script>');
    // [ts] expect_eq(page.evaluate(() => window.playwright), 'foo');
    // [ts] });
    // [ts] `,
    // [ts] }, {}, { PWDEBUG: "console" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("PWDEBUG=console should opt-in to exposing window.playwright", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent('<body></body>');
    // [ts] expect_eq(page.evaluate(() => window.playwright) != null, true);
    // [ts] });
    // [ts] `,
    // [ts] }, {}, { PWDEBUG: "console" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("init script should not observe playwright internals", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.addInitScript(() => {
    // [ts] window['check'] = () => {
    // [ts] const keys = Reflect.ownKeys(globalThis).map(k => k.toString());
    // [ts] return keys.find(name => name.includes('playwright') || name.includes('_pw')) || 'none';
    // [ts] };
    // [ts] window['found'] = window['check']();
    // [ts] });
    // [ts] page.goto("${server.EMPTY_PAGE}");
    // [ts] expect_eq(page.evaluate(() => window['found']), 'none');
    // [ts] expect_eq(page.evaluate(() => window['check']()), 'none');
    // [ts] });
    // [ts] `,
    // [ts] }, {}, { PWDEBUG: "0" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should pause test timeout while on pause", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] context.debugger.requestPause();
    // [ts] const paused = new Promise(f => context.debugger.once('pausedstatechanged', f));
    // [ts] const contentPromise = page.setContent('<div>hello</div>');
    // [ts] paused;
    // [ts] new Promise(f => setTimeout(f, 5000));
    // [ts] context.debugger.resume();
    // [ts] contentPromise;
    // [ts] });
    // [ts] `,
    // [ts] }, { timeout: 3000 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
}
