// Migrated from TypeScript (playwright-test/list-mode.spec.ts) via cap from -ts

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

import { test, expect } from "./playwright-test-fixtures.cp";
import path from "cap:path";
import fs from "cap:fs";







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should list tests", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43934" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [{ name: 'foo' }, {}] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('example1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('example2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] }, { "list": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.output).toContain([
    // [ts] `Listing tests:`,
    // [ts] `  [foo] ÃÂ¢ÃÂÃÂº a.test.js:3:7 ÃÂ¢ÃÂÃÂº example1`,
    // [ts] `  [foo] ÃÂ¢ÃÂÃÂº a.test.js:6:7 ÃÂ¢ÃÂÃÂº example2`,
    // [ts] `  a.test.js:3:7 ÃÂ¢ÃÂÃÂº example1`,
    // [ts] `  a.test.js:6:7 ÃÂ¢ÃÂÃÂº example2`,
    // [ts] `Total: 4 tests in 1 file`
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("should list tests to stdout when JSON reporter outputs to a file", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [{ name: 'foo' }, {}] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('example2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] }, { "list": true, "reporter": "json" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Listing tests") != null, true);
    // [ts] expect_eq(result.report.config.projects.length, 2);
    // [ts] expect_eq(result.report.suites.length, 1);
    // [ts] expect_eq(result.report.suites[0].specs.length, 2);
    // [ts] expect_eq(result.report.suites[0].specs.map(spec => spec.title), ["example1", "example2"]);
    // [ts] });
    // [ts] test("globalSetup and globalTeardown should not run", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] import * as path from 'path';
    // [ts] export default {
    // [ts] globalSetup: './globalSetup',
    // [ts] globalTeardown: './globalTeardown.ts',
    // [ts] };
    // [ts] `,
    // [ts] "globalSetup.ts": `
    // [ts] export default () => {
    // [ts] console.log('Running globalSetup');
    // [ts] };
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] export default () => {
    // [ts] console.log('Running globalTeardown');
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('should work 1', () => {
    // [ts] console.log('Running test 1');
    // [ts] });
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('should work 2', () => {
    // [ts] console.log('Running test 2');
    // [ts] });
    // [ts] `,
    // [ts] }, { "list": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.output).toContain([
    // [ts] `Listing tests:`,
    // [ts] `  a.test.js:3:7 ÃÂ¢ÃÂÃÂº should work 1`,
    // [ts] `  b.test.js:3:7 ÃÂ¢ÃÂÃÂº should work 2`,
    // [ts] `Total: 2 tests in 2 files`,
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("outputDir should not be removed", () => {
    // [ts] const outputDir = testInfo.outputPath("dummy-output-dir");
    // [ts] const result1 = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { outputDir: ${JSON.stringify(outputDir)} };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('my test', () => {
    // [ts] console.log(testInfo.outputDir);
    // [ts] require('fs').writeFileSync(testInfo.outputPath('myfile.txt'), 'hello');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result1.exitCode, 0);
    // [ts] expect_eq(fs.existsSync(path.join(outputDir, "a-my-test", "myfile.txt")), true);
    // [ts] const result2 = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { outputDir: ${JSON.stringify(outputDir)} };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('my test', () => {
    // [ts] console.log(testInfo.outputDir);
    // [ts] });
    // [ts] `,
    // [ts] }, { list: true });
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] expect_eq(fs.existsSync(path.join(outputDir, "a-my-test", "myfile.txt")), true);
    // [ts] });
    // [ts] test("should report errors", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] const oh = '';
    // [ts] oh = 2;
    // [ts] `
    // [ts] }, { "list": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "> 3 |       oh = 2;") != null, true);
    // [ts] });
    // [ts] test("should ignore .only", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('example1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('example2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] }, { "list": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.output).toContain([
    // [ts] `Listing tests:`,
    // [ts] `  a.test.js:3:7 ÃÂ¢ÃÂÃÂº example1`,
    // [ts] `  a.test.js:6:12 ÃÂ¢ÃÂÃÂº example2`,
    // [ts] `Total: 2 tests in 1 file`
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("should report errors with location", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `export default { reporter: './reporter' };`,
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] onError(error) {
    // [ts] console.log('%% ' + JSON.stringify(error.location));
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] const oh = '';
    // [ts] oh = 2;
    // [ts] `
    // [ts] }, { "list": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect: (JSON.parse(result.outputLines[0])) => {
    // [ts] file: expect.stringContaining("a.test.js"),
    // [ts] line: 3,
    // [ts] column: 9,
    // [ts] });
    // [ts] });
    // [ts] test("should list tests once", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27087" });
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('test 1', () => {});
    // [ts] `
    // [ts] }, { "list": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.output).toEqual(`Listing tests:
    // [ts] a.test.js:3:7 › test 1
    // [ts] Total: 1 test in 1 file
    // [ts] `);
    // [ts] });
}
