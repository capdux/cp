// Migrated from TypeScript (playwright-test/reporter-json.spec.ts) via cap from -ts

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

import * as path from "cap:path";
import * as fs from "cap:fs";
import { test, expect, stripAnsi } from "./playwright-test-fixtures.cp";











// [cap-test] test.describe("report location", () => {



// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should support spec.ok and stats", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math works!', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('math fails!', () => {
    // [ts] expect_eq(1 + 1, 3);
    // [ts] });
    // [ts] test('math skipped', () => {
    // [ts] });
    // [ts] `
    // [ts] }, { });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].ok, true);
    // [ts] expect_eq(result.report.suites[0].specs[1].ok, false);
    // [ts] expect_eq(result.report.suites[0].specs[2].ok, true);
    // [ts] expect_eq(result.report.stats.expected, 1);
    // [ts] expect_eq(result.report.stats.unexpected, 1);
    // [ts] expect_eq(result.report.stats.flaky, 0);
    // [ts] expect_eq(result.report.stats.skipped, 1);
    // [ts] });
    // [ts] test("should not report skipped due to sharding", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {
    // [ts] });
    // [ts] test('two', () => {
    // [ts] test();
    // [ts] });
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('three', () => {
    // [ts] });
    // [ts] test('four', () => {
    // [ts] test();
    // [ts] });
    // [ts] test('five', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { shard: "1/3", reporter: "json" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.report.suites.length, 1);
    // [ts] expect_eq(result.report.suites[0].specs.length, 2);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].status, "expected");
    // [ts] expect_eq(result.report.suites[0].specs[1].tests[0].status, "skipped");
    // [ts] });
    // [ts] test("should report projects and stats", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 2,
    // [ts] projects: [
    // [ts] {
    // [ts] timeout: 5000,
    // [ts] name: 'p1',
    // [ts] metadata: { foo: 'bar' },
    // [ts] },
    // [ts] {
    // [ts] timeout: 8000,
    // [ts] name: 'p2',
    // [ts] metadata: { bar: 42 },
    // [ts] }
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math works!', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] }, { });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const projects = result.report.config.projects;
    // [ts] const testDir = testInfo.outputDir.split(path.sep).join(path.posix.sep);
    // [ts] expect_eq(projects[0].name, "p1");
    // [ts] expect_eq(projects[0].retries, 2);
    // [ts] expect_eq(projects[0].timeout, 5000);
    // [ts] expect_eq(projects[0].metadata, { foo: "bar" });
    // [ts] expect_eq(projects[0].testDir, testDir);
    // [ts] expect_eq(projects[1].name, "p2");
    // [ts] expect_eq(projects[1].retries, 2);
    // [ts] expect_eq(projects[1].timeout, 8000);
    // [ts] expect_eq(projects[1].metadata, { bar: 42 });
    // [ts] expect_eq(projects[1].testDir, testDir);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].projectName, "p1");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[1].projectName, "p2");
    // [ts] expect(Date.init(result.report.stats.startTime).valueOf()).not.toBeNaN();
    // [ts] expect_eq(result.report.stats.duration, expect.any(Number));
    // [ts] });
    // [ts] test("should merge specs from all projects when testDir is not cwd", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] testDir: 'tests',
    // [ts] projects: [{ name: 'p1' }, { name: 'p2' }],
    // [ts] };
    // [ts] `,
    // [ts] "tests/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math works!', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [cap-test] test.describe('suite', () => {
    // [ts] test('only passes in p1', () => {
    // [ts] expect_eq(testInfo.project.name, 'p1');
    // [ts] });
    // [cap-test] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const fileSuite = result.report.suites[0];
    // [ts] expect_eq(fileSuite.specs.length, 1);
    // [ts] expect_eq(fileSuite.specs[0].ok, true);
    // [ts] expect_eq(fileSuite.specs[0].tests.map(t => t.projectName), ["p1", "p2"]);
    // [ts] expect_eq(fileSuite.suites.length, 1);
    // [ts] expect_eq(fileSuite.suites[0].specs.length, 1);
    // [ts] expect_eq(fileSuite.suites[0].specs[0].ok, false);
    // [ts] expect_eq(fileSuite.suites[0].specs[0].tests.map(t => t.projectName), ["p1", "p2"]);
    // [ts] });
    // [ts] test("should show steps", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math works!', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] test.step('math works in a step', () => {
    // [ts] expect_eq(2 + 2, 4);
    // [ts] test.step('nested step', () => {
    // [ts] expect_eq(2 + 2, 4);
    // [ts] test.step('deeply nested step', () => {
    // [ts] expect_eq(2 + 2, 4);
    // [ts] });
    // [ts] })
    // [ts] })
    // [ts] test.step('failing step', () => {
    // [ts] expect_eq(2 + 2, 5);
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.report.suites.length, 1);
    // [ts] expect_eq(result.report.suites[0].specs.length, 1);
    // [ts] const testResult = result.report.suites[0].specs[0].tests[0].results[0];
    // [ts] const steps = testResult.steps;
    // [ts] expect_eq(steps[0].title, "math works in a step");
    // [ts] expect_eq(steps[0].steps[0].title, "nested step");
    // [ts] expect_eq(steps[0].steps[0].steps[0].title, "deeply nested step");
    // [ts] expect_eq(steps[0].steps[0].steps[0].steps == null, true);
    // [ts] expect_eq(steps[1].error != null, true);
    // [ts] expect(testResult.errors).toHaveLength(1);
    // [ts] const snippet = stripAnsi(testResult.errors[0].message);
    // [ts] expect_eq(std.mem.indexOf(u8, snippet, "failing step") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snippet, "expect(2 + 2).toBe(5)") != null, true);
    // [ts] });
    // [ts] test("should display tags separately from title", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math works! @USR-MATH-001 @USR-MATH-002', { tag: '@foo' }, ({}) => {
    // [ts] test.info().annotations.push({ type: 'issue', description: 'issue-link' });
    // [ts] test.info().annotations.push({ type: 'novalue' });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.report.suites[0].specs[0].tags, ["USR-MATH-001", "USR-MATH-002", "foo"]);
    // [ts] });
    // [ts] test("should have relative always-posix paths", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('math works!', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.report.config.rootDir.indexOf(path.win32.sep), -1);
    // [ts] expect_eq(result.report.suites[0].specs[0].file, "a.test.js");
    // [ts] expect_eq(result.report.suites[0].specs[0].line, 3);
    // [ts] expect_eq(result.report.suites[0].specs[0].column, 7);
    // [ts] });
    // [ts] test("should have error position in results", ({
    // [ts] runInlineTest,
    // [ts] }) => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('math works!', () => {
    // [ts] expect_eq(1 + 1, 3);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].file, "a.test.js");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].results[0].errorLocation.line, 4);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].results[0].errorLocation.column, 23);
    // [ts] });
    // [ts] test("should add dot in addition to file json with CI", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: [['json', { outputFile: 'a.json' }]] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "" }, { CI: "1" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "ÃÂ·") != null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.json")), true);
    // [ts] });
    // [ts] test("should add line in addition to file json without CI", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: [['json', { outputFile: 'a.json' }]] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('one', () => {
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "" }, { PW_TEST_DEBUG_REPORTERS: "1" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "[1/1] a.test.js:3:7 Ã¢ÂÂº one") != null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.json")), true);
    // [ts] });
    // [ts] test("should have starting time in results", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math works!', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] },   { reporter: "json" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const startTime = result.report.suites[0].specs[0].tests[0].results[0].startTime;
    // [ts] expect_eq(Date.init(startTime).getTime() > Date.init("1/1/2000").getTime(), true);
    // [ts] });
    // [ts] test("with config should create report relative to config", () => {
    // [ts] const result = runInlineTest({
    // [ts] "nested/project/playwright.config.ts": `
    // [ts] export default { reporter: [['json', { outputFile: '../my-report/a.json' }]] };
    // [ts] `,
    // [ts] "nested/project/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "", config: "./nested/project/playwright.config.ts" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(path.join("nested", "my-report", "a.json"))), true);
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
    // [ts] }, { "reporter": "json" }, { "PLAYWRIGHT_JSON_OUTPUT_NAME": "../my-report.json" }, {
    // [ts] cwd: "foo/bar/baz/tests",
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "my-report.json")), true);
    // [ts] });
    // [ts] test("support PLAYWRIGHT_JSON_OUTPUT_FILE", () => {
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
    // [ts] }, { "reporter": "json" }, { "PLAYWRIGHT_JSON_OUTPUT_FILE": "../my-report.json" }, {
    // [ts] cwd: "foo/bar/baz/tests",
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "my-report.json")), true);
    // [ts] });
    // [ts] test("support PLAYWRIGHT_JSON_OUTPUT_DIR and PLAYWRIGHT_JSON_OUTPUT_NAME", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = { projects: [ {} ] };
    // [ts] `,
    // [ts] "tests/a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const fs = require('fs');
    // [ts] test('pass', () => {
    // [ts] });
    // [ts] `
    // [ts] }, { "reporter": "json" }, { "PLAYWRIGHT_JSON_OUTPUT_DIR": "foo/bar", "PLAYWRIGHT_JSON_OUTPUT_NAME": "baz/my-report.json" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "my-report.json")), true);
    // [ts] });
    // [ts] test("should report parallelIndex", () => {
    // [ts] const result = runInlineTest({
    // [ts] "tests/a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const fs = require('fs');
    // [ts] test.describe.configure({ mode: 'parallel' });
    // [ts] test('test 1 passes!', () => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [ts] test('test 2 fails!', () => {
    // [ts] expect_eq(1 + 1, 3);
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [ts] test('test 3 passes!', () => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [ts] `
    // [ts] }, { "workers": "2", "reporter": "json" });
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].results[0].workerIndex, 0);
    // [ts] expect_eq(result.report.suites[0].specs[1].tests[0].results[0].workerIndex, 1);
    // [ts] expect_eq(result.report.suites[0].specs[2].tests[0].results[0].workerIndex, 2);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].results[0].parallelIndex, 0);
    // [ts] expect_eq(result.report.suites[0].specs[1].tests[0].results[0].parallelIndex, 1);
    // [ts] expect_eq(result.report.suites[0].specs[2].tests[0].results[0].parallelIndex, 1);
    // [ts] });
}
