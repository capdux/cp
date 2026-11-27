// Migrated from TypeScript (playwright-test/test-output-dir.spec.ts) via cap from -ts

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
import * as path from "cap:path";
import { test, expect } from "./playwright-test-fixtures.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work and remove non-failures", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] name: 'chromium',
    // [ts] preserveOutput: 'failures-only',
    // [ts] testDir: 'dir',
    // [ts] };
    // [ts] `,
    // [ts] "dir/my-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] if (testInfo.retry) {
    // [ts] expect_eq(std.mem.indexOf(u8, testInfo.outputDir, 'my-test-test-1-chromium-retry' + testInfo.retry) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, testInfo.outputPath('foo', 'bar'), require('path').join('my-test-test-1-chromium-retry' + testInfo.retry, 'foo', 'bar')) != null, true);
    // [ts] require('fs').writeFileSync(testInfo.outputPath('file.txt'), 'content', 'utf-8');
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, testInfo.outputDir, 'my-test-test-1-chromium') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, testInfo.outputPath(), 'my-test-test-1-chromium') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, testInfo.outputPath('foo', 'bar'), require('path').join('my-test-test-1-chromium', 'foo', 'bar')) != null, true);
    // [ts] require('fs').writeFileSync(testInfo.outputPath('file.txt'), 'content', 'utf-8');
    // [ts] }
    // [ts] expect_eq(require('fs').existsSync(testInfo.outputDir), true);
    // [ts] if (testInfo.retry < 2)
    // [ts] throw new Error('Give me retries');
    // [ts] });
    // [ts] `,
    // [ts] }, { retries: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.results[0].status, "failed");
    // [ts] expect_eq(result.results[0].retry, 0);
    // Should only fail the last retry check.
    // [ts] expect_eq(result.results[0].error.message, "Error: Give me retries");
    // [ts] expect_eq(result.results[1].status, "failed");
    // [ts] expect_eq(result.results[1].retry, 1);
    // Should only fail the last retry check.
    // [ts] expect_eq(result.results[1].error.message, "Error: Give me retries");
    // [ts] expect_eq(result.results[2].status, "passed");
    // [ts] expect_eq(result.results[2].retry, 2);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "my-test-test-1-chromium")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "my-test-test-1-chromium-retry1")), true);
    // Last retry is successful, so output dir should be removed.
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "my-test-test-1-chromium-retry2")), false);
    // [ts] });
    // [ts] test("should include repeat token", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] if (testInfo.repeatEachIndex)
    // [ts] expect_eq(std.mem.indexOf(u8, testInfo.outputPath(''), 'repeat' + testInfo.repeatEachIndex) != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, testInfo.outputPath(''), 'repeat' + testInfo.repeatEachIndex) == null, true);
    // [ts] });
    // [ts] `
    // [ts] }, { "repeat-each": 3 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // [ts] });
    // [ts] test("should default to package.json directory", () => {
    // [ts] const result = runInlineTest({
    // [ts] "foo/package.json": `{ "name": "foo" }`,
    // [ts] "foo/bar/playwright.config.js": `
    // [ts] module.exports = { reporters: [], projects: [ {} ] };
    // [ts] `,
    // [ts] "foo/bar/baz/tests/a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const fs = require('fs');
    // [ts] test('pass', () => {
    // [ts] expect_eq(process.cwd(), __dirname);
    // [ts] fs.writeFileSync(testInfo.outputPath('foo.ts'), 'foobar');
    // [ts] });
    // [ts] `
    // [ts] }, {}, { PW_TEST_REPORTER: "" }, {
    // [ts] cwd: "foo/bar/baz/tests",
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "test-results")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "test-results")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "tests", "test-results")), false);
    // [ts] });
    // [ts] test("should be unique for beforeAll hook from different workers", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeAll(({}, testInfo) => {
    // [ts] console.log('\\n%%' + testInfo.outputDir);
    // [ts] });
    // [ts] test('fails', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] });
    // [ts] `
    // [ts] }, { retries: "1" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] `${testInfo.outputPath("test-results", "a-fails")}`,
    // [ts] `${testInfo.outputPath("test-results", "a-fails-retry1")}`,
    // [ts] `${testInfo.outputPath("test-results", "a-passes")}`,
    // [ts] ]);
    // [ts] });
    // [ts] test("should include the project name", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] export const test = base.extend({
    // [ts] auto: [ ({}, run, testInfo) => {
    // [ts] testInfo.snapshotSuffix = '';
    // [ts] run();
    // [ts] }, { auto: true } ]
    // [ts] });
    // [ts] export const test2 = base.extend({
    // [ts] auto: [ ({}, run, testInfo) => {
    // [ts] testInfo.snapshotSuffix = 'suffix';
    // [ts] run();
    // [ts] }, { auto: true } ]
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [
    // [ts] {},
    // [ts] { name: 'foo' },
    // [ts] { name: 'foo' },
    // [ts] { name: 'Bar space!' },
    // [ts] ] };
    // [ts] `,
    // [ts] "my-test.spec.js": `
    // [ts] const { test, test2 } = require('./helper');
    // [ts] test('test 1', () => {
    // [ts] console.log(testInfo.outputPath('bar.txt').replace(/\\\\/g, '/'));
    // [ts] console.log(testInfo.snapshotPath('bar.txt').replace(/\\\\/g, '/'));
    // [ts] if (testInfo.retry !== 1)
    // [ts] throw new Error('Give me a retry');
    // [ts] });
    // [ts] test2('test 2', ({}, testInfo) => {
    // [ts] console.log(testInfo.outputPath('bar.txt').replace(/\\\\/g, '/'));
    // [ts] console.log(testInfo.snapshotPath('bar.txt').replace(/\\\\/g, '/'));
    // [ts] });
    // [ts] `,
    // [ts] }, { retries: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.results[0].status, "failed");
    // [ts] expect_eq(result.results[1].status, "passed");
    // test1, run with empty
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1-retry1/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar.txt") != null, true);
    // test1, run with foo #1
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1-foo/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-foo.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1-foo-retry1/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-foo.txt") != null, true);
    // test1, run with foo #2
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1-foo1/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-foo.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1-foo1-retry1/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-foo.txt") != null, true);
    // test1, run with bar
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1-Bar-space-/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-Bar-space-.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-1-Bar-space--retry1/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-Bar-space-.txt") != null, true);
    // test2, run with empty
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-2/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-suffix.txt") != null, true);
    // test2, run with foo #1
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-2-foo/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-foo-suffix.txt") != null, true);
    // test2, run with foo #2
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-2-foo1/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-foo-suffix.txt") != null, true);
    // test2, run with bar
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test-results/my-test-test-2-Bar-space-/bar.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/bar-Bar-space--suffix.txt") != null, true);
    // [ts] });
    // [ts] test("should include path option in snapshot", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] export const test = base.extend({
    // [ts] auto: [ ({}, run, testInfo) => {
    // [ts] testInfo.snapshotSuffix = 'suffix';
    // [ts] run();
    // [ts] }, { auto: true } ]
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [
    // [ts] { name: 'foo' },
    // [ts] ] };
    // [ts] `,
    // [ts] "my-test.spec.js": `
    // [ts] const { test } = require('./helper');
    // [ts] test('test with path', () => {
    // [ts] console.log(testInfo.snapshotPath('test', 'path', 'bar.txt').replace(/\\\\/g, '/'));
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.results[0].status, "passed");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "my-test.spec.js-snapshots/test/path/bar-foo-suffix.txt") != null, true);
    // [ts] });
    // [ts] test("should error if outputPath is resolved to outside of parent", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] export const test = base.extend({
    // [ts] auto: [ ({}, run, testInfo) => {
    // [ts] testInfo.snapshotSuffix = 'suffix';
    // [ts] run();
    // [ts] }, { auto: true } ]
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [
    // [ts] { name: 'foo' },
    // [ts] ] };
    // [ts] `,
    // [ts] "my-test.spec.js": `
    // [ts] const { test } = require('./helper');
    // [ts] test('test with parent path', () => {
    // [ts] console.log(testInfo.outputPath('..', 'test', 'path', 'bar-test').replace(/\\\\/g, '/'));
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.results[0].status, "failed");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "The outputPath is not allowed outside of the parent directory. Please fix the defined path.") != null, true);
    // [ts] const badPath = path.join("..", "test", "path", "bar-test");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `outputPath: ${badPath}`) != null, true);
    // [ts] });
    // [ts] test("should remove output dirs for projects run", () => {
    // [ts] const paths: []string = [];
    // [ts] const files: []string = [];
    // [ts] let i = 0;
    // [ts] while (i < 3) { // [cap-migrate] for-step: i++
    // [ts] const p = testInfo.outputPath("path" + i);
    // [ts] fs.promises.mkdir(p, { recursive: true });
    // [ts] const f = path.join(p, "my-file.txt");
    // [ts] fs.promises.writeFile(f, "contents", "utf-8");
    // [ts] paths.push(p);
    // [ts] files.push(f);
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = { projects: [
    // [ts] { outputDir: ${JSON.stringify(paths[0])} },
    // [ts] { outputDir: ${JSON.stringify(paths[2])} },
    // [ts] ] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('my test', () => {});
    // [ts] `
    // [ts] }, { output: "" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(fs.existsSync(files[0]), false);
    // [ts] expect_eq(fs.existsSync(files[1]), true);
    // [ts] expect_eq(fs.existsSync(files[2]), false);
    // [ts] });
    // [ts] test("should remove folders with preserveOutput=never", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { preserveOutput: 'never' };
    // [ts] `,
    // [ts] "dir/my-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] require('fs').writeFileSync(testInfo.outputPath('file.txt'), 'content', 'utf-8');
    // [ts] if (testInfo.retry < 2)
    // [ts] throw new Error('Give me retries');
    // [ts] });
    // [ts] `,
    // [ts] }, { retries: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.results.length, 3);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "dir-my-test-test-1")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "dir-my-test-test-1-retry1")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "dir-my-test-test-1-retry2")), false);
    // [ts] });
    // [ts] test("should preserve failed results", () => {
    // [ts] const result = runInlineTest({
    // [ts] "dir/my-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] require('fs').writeFileSync(testInfo.outputPath('file.txt'), 'content', 'utf-8');
    // [ts] if (testInfo.retry < 2)
    // [ts] throw new Error('Give me retries');
    // [ts] });
    // [ts] `,
    // [ts] }, { "retries": 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.results.length, 3);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "dir-my-test-test-1")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "dir-my-test-test-1-retry1")), true);
    // [ts] });
    // [ts] test("should accept a relative path for outputDir", () => {
    // [ts] const result = runInlineTest({
    // [ts] "my-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.outputDir, ${JSON.stringify(path.join(testInfo.outputDir, './my-output-dir', 'my-test-test'))});
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.js": `
    // [ts] module.exports = { projects: [
    // [ts] { outputDir: './my-output-dir' },
    // [ts] ] };
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should have output dir based on rootDir (cwd)", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] const path = require('path');
    // [ts] module.exports = {
    // [ts] testDir: path.join(__dirname, 'e2e'),
    // [ts] outputDir: 'test-results/',
    // [ts] };`,
    // [ts] "e2e/example.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const fs = require('fs');
    // [ts] test('hello world', () => {
    // [ts] fs.writeFileSync(testInfo.outputPath('foo.txt'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "example-hello-world", "foo.txt")), true);
    // [ts] });
    // [ts] test("should allow nonAscii characters in the output dir", () => {
    // [ts] const result = runInlineTest({
    // [ts] "my-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('こんにちは世界', () => {
    // [ts] console.log('\\n%%' + testInfo.outputDir);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const outputDir = result.outputLines[0];
    // [ts] expect_eq(outputDir, path.join(testInfo.outputDir, "test-results", "my-test-Ã£ÂÂÃ£ÂÂÃ£ÂÂ«Ã£ÂÂ¡Ã£ÂÂ¯Ã¤Â¸ÂÃ§ÂÂ"));
    // [ts] });
    // [ts] test("should allow shorten long output dirs characters in the output dir", () => {
    // [ts] const result = runInlineTest({
    // [ts] "very/deep/and/long/file/name/that/i/want/to/be/trimmed/my-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [cap-test] test.describe('this is a really long description that would be too long for a file path', () => {
    // [ts] test('and this is an even longer test name that just keeps going and going and we should shorten it', () => {
    // [ts] console.log('\\n%%' + testInfo.outputDir);
    // [ts] });
    // [cap-test] });
    // [ts] `,
    // [ts] });
    // [ts] const outputDir = result.outputLines[0];
    // [ts] expect_eq(outputDir, path.join(testInfo.outputDir, "test-results", "very-deep-and-long-file-na-99202-ng-and-we-should-shorten-it"));
    // [ts] });
    // [ts] test("should not mangle double dashes", () => {
    // [ts] const result = runInlineTest({
    // [ts] "my--file.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('my--test', () => {
    // [ts] console.log('\\n%%' + testInfo.outputDir);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const outputDir = result.outputLines[0];
    // [ts] expect_eq(outputDir, path.join(testInfo.outputDir, "test-results", "my--file-my--test"));
    // [ts] });
    // [ts] test("should allow include the describe name the output dir", () => {
    // [ts] const result = runInlineTest({
    // [ts] "my-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [cap-test] test.describe('hello', () => {
    // [ts] test('world', () => {
    // [ts] console.log('\\n%%' + testInfo.outputDir);
    // [ts] });
    // [cap-test] });
    // [ts] `,
    // [ts] });
    // [ts] const outputDir = result.outputLines[0];
    // [ts] expect_eq(outputDir, path.join(testInfo.outputDir, "test-results", "my-test-hello-world"));
    // [ts] });
}
// TODO(cap-migrate): refine TS statement
// [ts] }
