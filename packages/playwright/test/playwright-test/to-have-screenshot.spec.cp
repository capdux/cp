// Migrated from TypeScript (playwright-test/to-have-screenshot.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as fs from "cap:fs";
import { PNG } from "playwright-core/lib/utilsBundle";
import * as path from "cap:path";
import { pathToFileURL } from "cap:url";
import { test, expect, createImage, createWebpImage, paintBlackPixels } from "./playwright-test-fixtures.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { comparePNGs } from "../config/comparator.cp";

test.describe.configure({ mode: "parallel" });

const IMG_WIDTH = 1280;
const IMG_HEIGHT = 720;
const whiteImage = createImage(IMG_WIDTH, IMG_HEIGHT, 255, 255, 255);
const redImage = createImage(IMG_WIDTH, IMG_HEIGHT, 255, 0, 0);
const greenImage = createImage(IMG_WIDTH, IMG_HEIGHT, 0, 255, 0);
const blueImage = createImage(IMG_WIDTH, IMG_HEIGHT, 0, 0, 255);









// [cap-test] test.describe("expect config animations option", () => {

// [cap-test] });




























































fn playwrightConfig(obj: string) {
  return {
    "playwright.config.js": `;
      module.exports = ${JSON.stringify(obj, null, 2)}
    `,
  };
}


// [cap-test] test.describe("update-snapshots", () => {



// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should fail to screenshot a page with infinite animation", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] animations: "allow",
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] expect(page).toHaveScreenshot({ timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timeout 2000ms exceeded`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Expect "toHaveScreenshot" with timeout 2000ms`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `generating new stable screenshot expectation`) != null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-actual.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-expected.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-previous.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-diff.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.spec.js-snapshots", "is-a-test-1.png")), false);
    // [ts] });
    // [ts] test("should fail like a timeout when aborted", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] const controller = new AbortController();
    // [ts] const promise = expect(page).toHaveScreenshot({ animations: 'allow', timeout: 5000, signal: controller.signal });
    // [ts] page.waitForTimeout(500);
    // [ts] controller.abort(new Error('stop it'));
    // [ts] promise;
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `operation was aborted: stop it`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timeout 5000ms exceeded`) == null, true);
    // [ts] });
    // [ts] test("should fail when already aborted", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] const controller = new AbortController();
    // [ts] controller.abort(new Error('already aborted'));
    // [ts] expect(page).toHaveScreenshot({ signal: controller.signal });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: The assertion was aborted: already aborted`) != null, true);
    // [ts] });
    // [ts] test("should disable animations by default", () => {
    // [ts] const cssTransitionURL = pathToFileURL(path.join(__dirname, "../assets/css-transition.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${cssTransitionURL}');
    // [ts] expect(page).toHaveScreenshot({ timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should not retry missing expectation errors", () => {
    // [ts] const cssTransitionURL = pathToFileURL(path.join(__dirname, "../assets/css-transition.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] retries: 2,
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${cssTransitionURL}');
    // [ts] expect(page).toHaveScreenshot('foo.png', { timeout: 1000 });
    // [ts] expect(page).toHaveScreenshot('bar.png', { timeout: 1000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `retry #`) == null, true);
    // [ts] expect(result.output).toMatch("A snapshot doesn't exist.*foo.*, writing actual.");
    // [ts] expect(result.output).toMatch("A snapshot doesn't exist.*bar.*, writing actual.");
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] test("should not retry serial mode suites with missing expectation errors", () => {
    // [ts] const cssTransitionURL = pathToFileURL(path.join(__dirname, "../assets/css-transition.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] retries: 2,
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test.describe.serial('outer', () => {
    // [ts] test('last', () => {
    // [ts] });
    // [cap-test] test.describe('nested', () => {
    // [ts] test('is a test', () => {
    // [ts] page.goto('${cssTransitionURL}');
    // [ts] expect(page).toHaveScreenshot({ timeout: 1000 });
    // [ts] expect(page).toHaveScreenshot({ timeout: 1000 });
    // [ts] });
    // [ts] test('last', () => {
    // [ts] });
    // [cap-test] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `retry #`) == null, true);
    // [ts] expect(result.output).toMatch("A snapshot doesn't exist.*1.*, writing actual.");
    // [ts] expect(result.output).toMatch("A snapshot doesn't exist.*2.*, writing actual.");
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] test("should respect toHaveScreenshot timeout from config", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] animations: "allow",
    // [ts] timeout: 2000,
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] expect(page).toHaveScreenshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timeout 2000ms exceeded`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Expect "toHaveScreenshot" with timeout 2000ms`) != null, true);
    // [ts] });
    // [ts] test("should let toHaveScreenshot call options override config timeout", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] animations: "allow",
    // [ts] timeout: 30000,
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] expect(page).toHaveScreenshot({ timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timeout 2000ms exceeded`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Expect "toHaveScreenshot" with timeout 2000ms`) != null, true);
    // [ts] });
    // [ts] test("disabled", () => {
    // [ts] const cssTransitionURL = pathToFileURL(path.join(__dirname, "../assets/css-transition.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: { toHaveScreenshot: { animations: "disabled" } },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${cssTransitionURL}');
    // [ts] expect(page).toHaveScreenshot({ timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("allow", () => {
    // [ts] const cssTransitionURL = pathToFileURL(path.join(__dirname, "../assets/css-transition.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: { toHaveScreenshot: { animations: "allow" } },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${cssTransitionURL}');
    // [ts] expect(page).toHaveScreenshot({ timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "is-a-test-1-actual.png") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "is-a-test-1-previous.png") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "is-a-test-1-diff.png") != null, true);
    // [ts] });
    // [ts] test("should fail with proper error when unsupported argument is given", () => {
    // [ts] const cssTransitionURL = pathToFileURL(path.join(__dirname, "../assets/css-transition.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${cssTransitionURL}');
    // [ts] expect(page).toHaveScreenshot({
    // [ts] clip: {
    // [ts] x: 0,
    // [ts] y: 0,
    // [ts] width: 0,
    // [ts] height: 0,
    // [ts] },
    // [ts] timeout: 2000,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Expected options.clip.width to be greater than 0`) != null, true);
    // [ts] });
    // [ts] test("should have scale:css by default", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] const context = browser.newContext({
    // [ts] viewport: { width: ${IMG_WIDTH}, height: ${IMG_HEIGHT} },
    // [ts] deviceScaleFactor: 2,
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] context.close();
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__", "a.spec.js", "snapshot.png");
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshotOutputPath), whiteImage), null);
    // [ts] });
    // [ts] test("should ignore non-documented options in toHaveScreenshot config", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] clip: { x: 0, y: 0, width: 10, height: 10 },
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__", "a.spec.js", "snapshot.png");
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshotOutputPath), whiteImage), null);
    // [ts] });
    // [ts] test("should report toHaveScreenshot step with expectation name in title", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] onStepEnd(test, result, step) {
    // [ts] console.log('%% end [' + step.category + '] ' + step.title);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] ...playwrightConfig({ reporter: "./reporter" }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // Named expectation.
    // [ts] expect(page).toHaveScreenshot('foo.png', { timeout: 2000 });
    // [ts] expect(page).toHaveScreenshot({ name: 'is-a-test-1.png', timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "reporter": "", "workers": 1, "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] `end [pw:api] Launch browser`,
    // [ts] `end [fixture] Fixture "browser"`,
    // [ts] `end [pw:api] Create context`,
    // [ts] `end [fixture] Fixture "context"`,
    // [ts] `end [pw:api] Create page`,
    // [ts] `end [fixture] Fixture "page"`,
    // [ts] `end [hook] Before Hooks`,
    // [ts] `end [expect] Expect "toHaveScreenshot(foo.png)"`,
    // [ts] `end [expect] Expect "toHaveScreenshot(is-a-test-1.png)"`,
    // [ts] `end [fixture] Fixture "page"`,
    // [ts] `end [pw:api] Close context`,
    // [ts] `end [fixture] Fixture "context"`,
    // [ts] `end [hook] After Hooks`,
    // [ts] ]);
    // [ts] });
    // [ts] test("should not fail when racing with navigation", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": createImage(10, 10, 255, 0, 0),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] Promise.all([
    // [ts] page.goto('${infiniteAnimationURL}'),
    // [ts] expect(page).toHaveScreenshot({
    // [ts] name: 'snapshot.png',
    // [ts] animations: "disabled",
    // [ts] clip: { x: 0, y: 0, width: 10, height: 10 },
    // [ts] }),
    // [ts] ]);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should successfully screenshot a page with infinite animation with disableAnimation: true", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] expect(page).toHaveScreenshot({
    // [ts] animations: "disabled",
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("__screenshots__", "a.spec.js", "is-a-test-1.png")), true);
    // [ts] });
    // [ts] test("should support clip option for page", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": createImage(50, 50, 255, 255, 255),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot({
    // [ts] name: 'snapshot.png',
    // [ts] clip: { x: 0, y: 0, width: 50, height: 50, },
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test")), false);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support omitBackground option for locator", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.setProperty('width', '100px');
    // [ts] document.body.style.setProperty('height', '100px');
    // [ts] });
    // [ts] expect(page.locator('body')).toHaveScreenshot({
    // [ts] name: 'snapshot.png',
    // [ts] omitBackground: true,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotPath = testInfo.outputPath("__screenshots__", "a.spec.js", "snapshot.png");
    // [ts] expect_eq(fs.existsSync(snapshotPath), true);
    // [ts] const png = PNG.sync.read(fs.readFileSync(snapshotPath));
    // [ts] expect.soft(png.width, "image width must be 100").toBe(100);
    // [ts] expect.soft(png.height, "image height must be 100").toBe(100);
    // [ts] expect.soft(png.data[0], "image R must be 0").toBe(0);
    // [ts] expect.soft(png.data[1], "image G must be 0").toBe(0);
    // [ts] expect.soft(png.data[2], "image B must be 0").toBe(0);
    // [ts] expect.soft(png.data[3], "image A must be 0").toBe(0);
    // [ts] });
    // [ts] test("should enforce strictness for locator screenshots", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43745" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.setContent('<button>First</button><button>Second</button>');
    // [ts] const buttons = page.getByRole('button');
    // [ts] expect(buttons).toHaveCount(2);
    // [ts] expect(buttons.first()).toHaveScreenshot('button.png');
    // [ts] expect(buttons.screenshot()).rejects.toThrow('strict mode violation');
    // [ts] expect(expect(buttons).toHaveScreenshot('button.png')).rejects.toThrow('strict mode violation');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const regularResult = runInlineTest({});
    // [ts] expect_eq(regularResult.exitCode, 0);
    // [ts] });
    // [ts] test("should fail to screenshot an element with infinite animation", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] projects: [{
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] animations: "allow",
    // [ts] },
    // [ts] },
    // [ts] }],
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] expect(page.locator('body')).toHaveScreenshot({ timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timeout 2000ms exceeded`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Expect "toHaveScreenshot" locator('body') with timeout 2000ms`) != null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-previous.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-actual.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-expected.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-diff.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("__screenshots__", "a.spec.js", "is-a-test-1.png")), false);
    // [ts] });
    // [ts] test("should fail to screenshot an element that keeps moving", () => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] animations: "allow",
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] expect(page.locator('div')).toHaveScreenshot({ timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Timeout 2000ms exceeded`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `element is not stable`) != null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-actual.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-expected.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-diff.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("__screenshots__", "a.spec.js", "is-a-test-1.png")), false);
    // [ts] });
    // [ts] test("should generate default name", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-actual.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("__screenshots__", "a.spec.js", "is-a-test-1.png")), true);
    // [ts] });
    // [ts] test("should compile with different option combinations", () => {
    // [ts] const result = runTSC({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] expect: {
    // [ts] timeout: 10000,
    // [ts] toHaveScreenshot: {
    // [ts] threshold: 0.2,
    // [ts] maxDiffPixels: 10,
    // [ts] maxDiffPixelRatio: 0.2,
    // [ts] animations: "allow",
    // [ts] caret: "hide",
    // [ts] scale: "css",
    // [ts] },
    // [ts] },
    // [ts] });
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot();
    // [ts] expect(page).toHaveScreenshot('img.png');
    // [ts] expect(page).toHaveScreenshot('img.png', { threshold: 0.2, caret: 'initial' });
    // [ts] expect(page.locator('body')).toHaveScreenshot({ threshold: 0.2 });
    // [ts] expect(page).toHaveScreenshot({ maxDiffPixelRatio: 0.2 });
    // [ts] expect(page).toHaveScreenshot({
    // [ts] threshold: 0.2,
    // [ts] maxDiffPixels: 10,
    // [ts] maxDiffPixelRatio: 0.2,
    // [ts] animations: "disabled",
    // [ts] omitBackground: true,
    // [ts] caret: "initial",
    // [ts] scale: "device",
    // [ts] timeout: 1000,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should fail when screenshot is different size", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": createImage(22, 33),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `verifying given screenshot expectation`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `captured a stable screenshot`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Expected an image 22px by 33px, received 1280px by 720px.") != null, true);
    // [ts] });
    // [ts] test("should fail when given unsupported snapshot name", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.jpeg');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Screenshot name "snapshot.jpeg" must have a '.png' or '.webp' extension`) != null, true);
    // [ts] });
    // [ts] test("should pass when webp screenshot matches expectation", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.webp": createWebpImage(IMG_WIDTH, IMG_HEIGHT, 255, 255, 255),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.webp');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should write missing webp expectation", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.webp');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__", "a.spec.js", "snapshot.webp");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const image = utils.decodeWebp(fs.readFileSync(snapshotOutputPath));
    // [ts] expect_eq(image.width, IMG_WIDTH);
    // [ts] expect_eq(image.height, IMG_HEIGHT);
    // [ts] });
    // [ts] test("should fail when webp screenshot is different pixels", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.webp": createWebpImage(IMG_WIDTH, IMG_HEIGHT, 255, 0, 0),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.webp', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: expect(page).toHaveScreenshot(expected)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "are different") != null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "snapshot-actual.webp")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "snapshot-expected.webp")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "snapshot-diff.webp")), true);
    // [ts] });
    // [ts] test("should update webp snapshot with the update-snapshots flag", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.webp": createWebpImage(IMG_WIDTH, IMG_HEIGHT, 255, 0, 0),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.webp');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__", "a.spec.js", "snapshot.webp");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `${snapshotOutputPath} is re-generated, writing actual.`) != null, true);
    // [ts] const image = utils.decodeWebp(fs.readFileSync(snapshotOutputPath));
    // [ts] expect_eq(image.width, IMG_WIDTH);
    // [ts] expect_eq(image.height, IMG_HEIGHT);
    // The red expectation should have been replaced with the actual white page.
    // [ts] expect_eq([...image.data.subarray(0, 4)], [255, 255, 255, 255]);
    // [ts] });
    // [ts] test("should use configured webp type", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] type: "webp",
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/is-a-test-1.webp": createWebpImage(IMG_WIDTH, IMG_HEIGHT, 255, 255, 255),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(test.info().snapshotPath({ kind: 'screenshot' })).toMatch(/is-a-test-1\\.webp$/);
    // Anonymous and explicit .png name both match existing snapshots.
    // [ts] expect(page).toHaveScreenshot();
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // Anonymous locator snapshot is written as webp.
    // [ts] expect(page.locator('body')).toHaveScreenshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] for (const name of ["is-a-test-1.webp", "snapshot.png"])
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${testInfo.outputPath("__screenshots__", "a.spec.js", name)}`) == null, true);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__", "a.spec.js", "is-a-test-2.webp");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(utils.decodeWebp(fs.readFileSync(snapshotOutputPath)).width > 0, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-2-actual.webp")), true);
    // [ts] });
    // [ts] test("should fail on unsupported configured type", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] type: "jpeg",
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `config.expect.toHaveScreenshot.type must be one of "png" or "webp"`) != null, true);
    // [ts] });
    // [ts] test("should fail when given buffer", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from([1])).toHaveScreenshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `toHaveScreenshot can be only used with Page or Locator objects`) != null, true);
    // [ts] });
    // [ts] test("should fail when screenshot is different pixels", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": paintBlackPixels(whiteImage, 12345),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: expect(page).toHaveScreenshot(expected)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "12345 pixels") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Call log") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "ratio 0.02") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Expected:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Received:") != null, true);
    // [ts] });
    // [ts] test("doesn't create comparison artifacts in an output folder for passed negated snapshot matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": blueImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).not.toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const outputText = result.output;
    // [ts] const expectedSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-expected.png");
    // [ts] const actualSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-actual.png");
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, `Expected: ${expectedSnapshotArtifactPath}`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, `Received: ${actualSnapshotArtifactPath}`) == null, true);
    // [ts] expect_eq(fs.existsSync(expectedSnapshotArtifactPath), false);
    // [ts] expect_eq(fs.existsSync(actualSnapshotArtifactPath), false);
    // [ts] });
    // [ts] test("should fail on same snapshots with negate matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).not.toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Screenshot comparison failed:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Expected result should be different from the actual one.") != null, true);
    // [ts] });
    // [ts] test("should not fail if --ignore-snapshots is passed", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": redImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "ignore-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should write missing expectations locally twice and attach them", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] expect(page).toHaveScreenshot('snapshot2.png');
    // [ts] console.log('Here we are!');
    // [ts] });
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('\\n%%' + JSON.stringify(testInfo.attachments));
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshot1OutputPath = testInfo.outputPath("__screenshots__", "a.spec.js", "snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: A snapshot doesn't exist at ${snapshot1OutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshot1OutputPath), whiteImage), null);
    // [ts] const snapshot2OutputPath = testInfo.outputPath("__screenshots__", "a.spec.js", "snapshot2.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: A snapshot doesn't exist at ${snapshot2OutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshot2OutputPath), whiteImage), null);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Here we are!") != null, true);
    // [ts] const stackLines = result.output.split("\n").filter(line => line.includes("    at ")).filter(line => !line.includes("a.spec.js"));
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "a.spec.js:5") != null, true);
    // [ts] expect_eq(stackLines.length, 0);
    // [ts] const attachments = result.outputLines.map(l => JSON.parse(l))[0];
    // [ts] for (const attachment of attachments)
    // [ts] attachment.path = attachment.path.replace("\\\\", "/").replace(".*test-results\\/", "").replace(".*__screenshots__", "__screenshots__");
    // [ts] expect(attachments).toEqual([
    // [ts] {
    // [ts] name: "snapshot-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "__screenshots__/a.spec.js/snapshot.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "a-is-a-test/snapshot-actual.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot2-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "__screenshots__/a.spec.js/snapshot2.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot2-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "a-is-a-test/snapshot2-actual.png"
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should attach missing expectations to right step", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] onStepEnd(test, result, step) {
    // [ts] if (step.attachments.length > 0)
    // [ts] console.log(\`%%[\${step.category}] \${step.title}: \${step.attachments.map(a => a.name).join(", ")}\`);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] ...playwrightConfig({
    // [ts] reporter: [["dot"], ["./reporter"]],
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.outputLines, ["[expect] Expect \"toHaveScreenshot(snapshot.png)\": snapshot-expected.png, snapshot-actual.png"]);
    // [ts] });
    // [ts] test("shouldn't write missing expectations locally for negated matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).not.toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, matchers using ".not" won\'t write them automatically.`) != null, true);
    // [ts] expect_eq(fs.existsSync(snapshotOutputPath), false);
    // [ts] });
    // [ts] test("should update snapshot with the update-snapshots flag", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": blueImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `${snapshotOutputPath} is re-generated, writing actual.`) != null, true);
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshotOutputPath), whiteImage), null);
    // [ts] });
    // [ts] test("should respect config.snapshotPathTemplate and sanitize the name", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35669" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/my-name.png": whiteImage,
    // [ts] "__screenshots__/a.spec.js/my_name/bar.png": whiteImage,
    // [ts] "__screenshots__/a.spec.js/is-a-test-1.png": whiteImage,
    // [ts] "__screenshots__/a.spec.js/is-a-test-2.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const path = require('path');
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] const testDir = test.info().project.testDir;
    // A single name is sanitized to a valid file name.
    // [ts] const expectedPath1 = path.join(testDir, '__screenshots__/a.spec.js/my-name.png');
    // [ts] expect_eq(test.info().snapshotPath('my_name.png'), expectedPath1);
    // [ts] expect(page).toHaveScreenshot('my_name.png');
    // An array is not sanitized - see https://github.com/microsoft/playwright/pull/9156.
    // [ts] const expectedPath2 = path.join(testDir, '__screenshots__/a.spec.js/my_name/bar.png');
    // [ts] expect_eq(test.info().snapshotPath('my_name', 'bar.png'), expectedPath2);
    // [ts] expect(page).toHaveScreenshot(['my_name', 'bar.png']);
    // Auto-generated name is sanitized.
    // [ts] const expectedPath3 = path.join(testDir, '__screenshots__/a.spec.js/is-a-test-1.png');
    // [ts] expect_eq(test.info().snapshotPath('', { kind: 'screenshot' }), expectedPath3);
    // [ts] expect(page).toHaveScreenshot();
    // Auto-generated name is incremented.
    // [ts] const expectedPath4 = path.join(testDir, '__screenshots__/a.spec.js/is-a-test-2.png');
    // [ts] expect_eq(test.info().snapshotPath('', { kind: 'screenshot' }), expectedPath4);
    // [ts] expect(page).toHaveScreenshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should respect config.EXPECT toHaveScreenshot.pathTemplate", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] expect: { toHaveScreenshot: { pathTemplate: "actual-screenshots/{testFilePath}/{arg}{ext}" } },
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": blueImage,
    // [ts] "actual-screenshots/a.spec.js/snapshot.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const path = require('path');
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] const testDir = test.info().project.testDir;
    // [ts] const screenshotPath = path.join(testDir, 'actual-screenshots/a.spec.js/snapshot.png');
    // [ts] expect_eq(test.info().snapshotPath('snapshot.png', { kind: 'screenshot' }), screenshotPath);
    // [ts] const snapshotPath = path.join(testDir, '__screenshots__/a.spec.js/snapshot.png');
    // [ts] expect_eq(test.info().snapshotPath('snapshot.png', { kind: 'snapshot' }), snapshotPath);
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("shouldn't update snapshot with the update-snapshots flag for negated matcher", () => {
    // [ts] const EXPECTED_SNAPSHOT = blueImage;
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).not.toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(fs.readFileSync(snapshotOutputPath).equals(EXPECTED_SNAPSHOT), true);
    // [ts] });
    // [ts] test("should silently write missing expectations locally with the update-snapshots flag", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(comparePNGs(data, whiteImage), null);
    // [ts] });
    // [ts] test("should pass when writing missing expectations with the update-snapshots=missing flag", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "missing" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshotOutputPath), whiteImage), null);
    // [ts] });
    // [ts] test("should fail when writing missing expectations with the update-snapshots=default flag", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "default" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshotOutputPath), whiteImage), null);
    // [ts] });
    // [ts] test("should still fail on a mismatching screenshot with the update-snapshots=missing flag", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": redImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "missing" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(comparePNGs(fs.readFileSync(snapshotOutputPath), redImage), null);
    // [ts] });
    // [ts] test("should not write missing expectations locally with the update-snapshots flag for negated matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).not.toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, matchers using ".not" won\'t write them automatically.`) != null, true);
    // [ts] expect_eq(fs.existsSync(snapshotOutputPath), false);
    // [ts] });
    // [ts] test("should match multiple snapshots", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/red.png": redImage,
    // [ts] "__screenshots__/a.spec.js/green.png": greenImage,
    // [ts] "__screenshots__/a.spec.js/blue.png": blueImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] Promise.all([
    // [ts] page.evaluate(() => document.documentElement.style.setProperty('background', '#f00')),
    // [ts] expect(page).toHaveScreenshot('red.png'),
    // [ts] ]);
    // [ts] Promise.all([
    // [ts] page.evaluate(() => document.documentElement.style.setProperty('background', '#0f0')),
    // [ts] expect(page).toHaveScreenshot('green.png'),
    // [ts] ]);
    // [ts] Promise.all([
    // [ts] page.evaluate(() => document.documentElement.style.setProperty('background', '#00f')),
    // [ts] expect(page).toHaveScreenshot('blue.png'),
    // [ts] ]);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use provided name", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/provided.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('provided.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use provided name via options", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/provided.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot({ name: 'provided.png' });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should respect maxDiffPixels option", () => {
    // [ts] const BAD_PIXELS = 120;
    // [ts] const EXPECTED_SNAPSHOT = paintBlackPixels(whiteImage, BAD_PIXELS);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure default comparison fails").toBe(1);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', {
    // [ts] maxDiffPixels: ${BAD_PIXELS}
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixels option is respected").toBe(0);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] maxDiffPixels: BAD_PIXELS
    // [ts] }
    // [ts] },
    // [ts] projects: [
    // [ts] {
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] },
    // [ts] ],
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixels option in project config is respected").toBe(0);
    // [ts] });
    // [ts] test("should not update screenshot that matches with maxDiffPixels option when -u is passed", () => {
    // [ts] const BAD_PIXELS = 120;
    // [ts] const EXPECTED_SNAPSHOT = paintBlackPixels(whiteImage, BAD_PIXELS);
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { maxDiffPixels: ${BAD_PIXELS} });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `is re-generated, writing actual`) == null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-actual.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-expected.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-previous.png")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-diff.png")), false);
    // [ts] const data = fs.readFileSync(testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png"));
    // [ts] expect_eq(comparePNGs(data, EXPECTED_SNAPSHOT), null);
    // [ts] });
    // [ts] test("should satisfy both maxDiffPixelRatio and maxDiffPixels", () => {
    // [ts] const BAD_RATIO = 0.25;
    // [ts] const BAD_COUNT = Math.floor(IMG_WIDTH * IMG_HEIGHT * BAD_RATIO);
    // [ts] const EXPECTED_SNAPSHOT = paintBlackPixels(whiteImage, BAD_COUNT);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure default comparison fails").toBe(1);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', {
    // [ts] maxDiffPixels: ${Math.floor(BAD_COUNT / 2)},
    // [ts] maxDiffPixelRatio: ${BAD_RATIO},
    // [ts] timeout: 2000,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure it fails when maxDiffPixels < actualBadPixels < maxDiffPixelRatio").toBe(1);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', {
    // [ts] maxDiffPixels: ${BAD_COUNT},
    // [ts] maxDiffPixelRatio: ${BAD_RATIO / 2},
    // [ts] timeout: 2000,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure it fails when maxDiffPixelRatio < actualBadPixels < maxDiffPixels").toBe(1);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', {
    // [ts] maxDiffPixels: ${BAD_COUNT},
    // [ts] maxDiffPixelRatio: ${BAD_RATIO},
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure it passes when actualBadPixels < maxDiffPixelRatio && actualBadPixels < maxDiffPixels").toBe(0);
    // [ts] });
    // [ts] test("should respect maxDiffPixelRatio option", () => {
    // [ts] const BAD_RATIO = 0.25;
    // [ts] const BAD_PIXELS = IMG_WIDTH * IMG_HEIGHT * BAD_RATIO;
    // [ts] const EXPECTED_SNAPSHOT = paintBlackPixels(whiteImage, BAD_PIXELS);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure default comparison fails").toBe(1);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', {
    // [ts] maxDiffPixelRatio: ${BAD_RATIO}
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixelRatio option is respected").toBe(0);
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] maxDiffPixelRatio: BAD_RATIO,
    // [ts] },
    // [ts] },
    // [ts] projects: [{
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }],
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixels option in project config is respected").toBe(0);
    // [ts] });
    // [ts] test("should throw for invalid maxDiffPixels values", () => {
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot({
    // [ts] maxDiffPixels: -1,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode).toBe(1);
    // [ts] });
    // [ts] test("should throw for invalid maxDiffPixelRatio values", () => {
    // [ts] expect((runInlineTest({
    // [ts] ...playwrightConfig({}),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot({
    // [ts] maxDiffPixelRatio: 12,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode).toBe(1);
    // [ts] });
    // [ts] test("should attach expected/actual/diff when sizes are different", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": createImage(2, 2),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('## ' + JSON.stringify(testInfo.attachments));
    // [ts] });
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const outputText = result.output;
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "Expected an image 2px by 2px, received 1280px by 720px.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "4 pixels (ratio 0.01 of all image pixels) are different.") != null, true);
    // [ts] const attachments = outputText.split("\n").filter(l => l.startsWith("## ")).map(l => l.substring(3)).map(l => JSON.parse(l))[0];
    // [ts] for (const attachment of attachments)
    // [ts] attachment.path = attachment.path.replace(testInfo.outputDir, "").substring(1).replace("\\\\", "/");
    // [ts] expect(attachments).toEqual([
    // [ts] {
    // [ts] name: "snapshot-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "__screenshots__/a.spec.js/snapshot.png",
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "test-results/a-is-a-test/snapshot-actual.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot-diff.png",
    // [ts] contentType: "image/png",
    // [ts] path: "test-results/a-is-a-test/snapshot-diff.png"
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should fail with missing expectations and retries", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] retries: 1,
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(comparePNGs(data, whiteImage), null);
    // [ts] });
    // [ts] test("should update expectations with retries", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] retries: 1,
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(comparePNGs(data, whiteImage), null);
    // [ts] });
    // [ts] test("should respect comparator name", () => {
    // [ts] const expected = fs.readFileSync(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-expected.png"));
    // [ts] const actualURL = pathToFileURL(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-actual.png"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": expected,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('should pass', () => {
    // [ts] page.goto('${actualURL}');
    // [ts] expect(page.locator('img')).toHaveScreenshot('snapshot.png', {
    // [ts] threshold: 0,
    // [ts] _comparator: 'ssim-cie94',
    // [ts] });
    // [ts] });
    // [ts] test('should fail', () => {
    // [ts] page.goto('${actualURL}');
    // [ts] expect(page.locator('img')).toHaveScreenshot('snapshot.png', {
    // [ts] threshold: 0,
    // [ts] _comparator: 'pixelmatch',
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].title, "should pass");
    // [ts] expect_eq(result.report.suites[0].specs[0].ok, true);
    // [ts] expect_eq(result.report.suites[0].specs[1].title, "should fail");
    // [ts] expect_eq(result.report.suites[0].specs[1].ok, false);
    // [ts] });
    // [ts] test("should respect comparator in config", () => {
    // [ts] const expected = fs.readFileSync(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-expected.png"));
    // [ts] const actualURL = pathToFileURL(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-actual.png"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] projects: [
    // [ts] {
    // [ts] name: "should-pass",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] _comparator: "ssim-cie94",
    // [ts] }
    // [ts] },
    // [ts] },
    // [ts] {
    // [ts] name: "should-fail",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] _comparator: "pixelmatch",
    // [ts] }
    // [ts] },
    // [ts] },
    // [ts] ],
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": expected,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('test', () => {
    // [ts] page.goto('${actualURL}');
    // [ts] expect(page.locator('img')).toHaveScreenshot('snapshot.png', { threshold: 0, });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].projectName, "should-pass");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].status, "expected");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[1].projectName, "should-fail");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[1].status, "unexpected");
    // [ts] });
    // [ts] test("should throw pretty error if expected PNG file is not a PNG", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": "not a png",
    // [ts] "__screenshots__/a.spec.js/snapshot.jpg": "not a jpg",
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('png', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] test('jpg', () => {
    // [ts] expect(page.screenshot({ type: 'jpeg' })).toMatchSnapshot('snapshot.jpg')
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Could not decode expected image as PNG.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Could not decode expected image as JPEG.") != null, true);
    // [ts] });
    // [ts] test("should throw pretty error if expected PNG file is not a PNG while rebasing", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": "not a png",
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('png', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `,
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Failed to re-generate expected.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Could not decode expected image as PNG.") != null, true);
    // [ts] });
    // [ts] test("should support maskColor option", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": createImage(IMG_WIDTH, IMG_HEIGHT, 0, 255, 0),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('png', () => {
    // [ts] page.setContent('<style> html,body { padding: 0; margin: 0; }</style>');
    // [ts] expect(page).toHaveScreenshot('snapshot.png', {
    // [ts] mask: [page.locator('body')],
    // [ts] maskColor: '#00FF00',
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support stylePath option", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/tests/a.spec.js/snapshot.png": createImage(IMG_WIDTH, IMG_HEIGHT, 0, 255, 0),
    // [ts] "__screenshots__/tests/a.spec.js/png-1.png": createImage(IMG_WIDTH, IMG_HEIGHT, 0, 255, 0),
    // [ts] "screenshot.css": "body { background: #00FF00; }",
    // [ts] "tests/a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('png', () => {
    // [ts] page.setContent('<style> html,body { padding: 0; margin: 0; }</style>');
    // [ts] expect(page).toHaveScreenshot('snapshot.png', {
    // [ts] stylePath: './screenshot.css',
    // [ts] });
    // [ts] expect(page).toHaveScreenshot({
    // [ts] stylePath: './screenshot.css',
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support stylePath option in config", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] stylePath: "./screenshot.css",
    // [ts] },
    // [ts] },
    // [ts] }),
    // [ts] "screenshot.css": "body { background: #00FF00; }",
    // [ts] "__screenshots__/a.spec.js/snapshot.png": createImage(IMG_WIDTH, IMG_HEIGHT, 0, 255, 0),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('png', () => {
    // [ts] page.setContent('<style> html,body { padding: 0; margin: 0; }</style>');
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should trim+sanitize attachment names and paths", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('## ' + JSON.stringify(testInfo.attachments));
    // [ts] });
    // [ts] const title = 'long '.repeat(30) + 'title';
    // [ts] test(title, () => {
    // [ts] expect.soft(page).toHaveScreenshot();
    // [ts] const name = 'long '.repeat(30) + 'name.png';
    // [ts] expect.soft(page).toHaveScreenshot(name);
    // [ts] expect.soft(page).toHaveScreenshot(['dir', name]);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const attachments = result.output.split("\n").filter(l => l.startsWith("## ")).map(l => l.substring(3)).map(l => JSON.parse(l))[0];
    // [ts] for (const attachment of attachments) {
    // [ts] attachment.path = attachment.path.replace(testInfo.outputDir, "").substring(1).replace("\\\\", "/");
    // [ts] attachment.name = attachment.name.replace("\\\\", "/");
    // [ts] }
    // [ts] expect(attachments).toEqual([
    // [ts] {
    // [ts] name: "long-long-long-long-long-l-852e1-long-long-long-long-title-1-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "__screenshots__/a.spec.js/long-long-long-long-long-long-long-long-long-l-852e1-long-long-long-long-long-long-long-long-title-1.png",
    // [ts] },
    // [ts] {
    // [ts] name: "long-long-long-long-long-l-852e1-long-long-long-long-title-1-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "test-results/a-long-long-long-long-long-abd51-g-long-long-long-long-title/long-long-long-long-long-l-852e1-long-long-long-long-title-1-actual.png",
    // [ts] },
    // [ts] {
    // [ts] name: "long-long-long-long-long-l-6bf1e-ong-long-long-long-name-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "__screenshots__/a.spec.js/long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-long-name.png",
    // [ts] },
    // [ts] {
    // [ts] name: "long-long-long-long-long-l-6bf1e-ong-long-long-long-name-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "test-results/a-long-long-long-long-long-abd51-g-long-long-long-long-title/long-long-long-long-long-l-6bf1e-ong-long-long-long-name-actual.png",
    // [ts] },
    // [ts] {
    // [ts] name: "dir/long long long long long long long long long long long long long long long long long long long long long long long long long long long long long long name-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "__screenshots__/a.spec.js/dir/long long long long long long long long long long long long long long long long long long long long long long long long long long long long long long name.png",
    // [ts] },
    // [ts] {
    // [ts] name: "dir/long long long long long long long long long long long long long long long long long long long long long long long long long long long long long long name-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "test-results/a-long-long-long-long-long-abd51-g-long-long-long-long-title/dir/long long long long long long long long long long long long long long long long long long long long long long long long long long long long long long name-actual.png",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should rebase non-matching image", () => {
    // [ts] const BAD_PIXELS = 10;
    // [ts] const EXPECTED_SNAPSHOT = paintBlackPixels(whiteImage, BAD_PIXELS);
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "changed" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const newBaseline = fs.readFileSync(test.info().outputPath("__screenshots__/a.spec.js/snapshot.png"));
    // [ts] expect_eq(comparePNGs(newBaseline, whiteImage), null);
    // [ts] expect_eq(comparePNGs(newBaseline, EXPECTED_SNAPSHOT) != null, true);
    // [ts] });
    // [ts] test("should not update expectations with non-stable screenshot", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44128" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const infiniteAnimationURL = pathToFileURL(path.join(__dirname, "../assets/rotate-z.html"));
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.goto('${infiniteAnimationURL}');
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { animations: 'allow', timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "changed" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Failed to take two consecutive stable screenshots.") != null, true);
    // [ts] const newBaseline = fs.readFileSync(testInfo.outputPath("__screenshots__/a.spec.js/snapshot.png"));
    // [ts] expect_eq(newBaseline.equals(whiteImage), true);
    // [ts] });
    // [ts] test("should not rebase matching image", () => {
    // [ts] const BAD_PIXELS = 10;
    // [ts] const EXPECTED_SNAPSHOT = paintBlackPixels(whiteImage, BAD_PIXELS);
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] maxDiffPixels: BAD_PIXELS
    // [ts] }
    // [ts] }
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "changed" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const newBaseline = fs.readFileSync(test.info().outputPath("__screenshots__/a.spec.js/snapshot.png"));
    // [ts] expect_eq(comparePNGs(newBaseline, EXPECTED_SNAPSHOT), null);
    // [ts] expect_eq(comparePNGs(newBaseline, whiteImage) != null, true);
    // [ts] });
    // [ts] test("should rebase matching image with update-snapshots=all", () => {
    // [ts] const BAD_PIXELS = 10;
    // [ts] const EXPECTED_SNAPSHOT = paintBlackPixels(whiteImage, BAD_PIXELS);
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] expect: {
    // [ts] toHaveScreenshot: {
    // [ts] maxDiffPixels: BAD_PIXELS
    // [ts] }
    // [ts] }
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] expect(page).toHaveScreenshot('snapshot.png', { timeout: 2000 });
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "all" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const newBaseline = fs.readFileSync(test.info().outputPath("__screenshots__/a.spec.js/snapshot.png"));
    // [ts] expect_eq(comparePNGs(newBaseline, whiteImage), null);
    // [ts] expect_eq(comparePNGs(newBaseline, EXPECTED_SNAPSHOT) != null, true);
    // [ts] });
    // [ts] test("should hide overlays during toHaveScreenshot", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...playwrightConfig({
    // [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
    // [ts] }),
    // [ts] "__screenshots__/a.spec.js/snapshot.png": whiteImage,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('is a test', () => {
    // [ts] page.screencast.showOverlay('<div style="position:absolute;inset:0;background:red"></div>');
    // [ts] expect(page).toHaveScreenshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
}
