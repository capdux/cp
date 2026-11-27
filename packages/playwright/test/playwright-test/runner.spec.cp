// Migrated from TypeScript (playwright-test/runner.spec.ts) via cap from -ts

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
import { test, expect, parseTestRunnerOutput, countTimes } from "./playwright-test-fixtures.cp";































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("it should not allow multiple tests with the same name per suite", () => {
    // [ts] const result = runInlineTest({
    // [ts] "tests/example.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [cap-test] test.describe('suite', () => {
    // [ts] test('i-am-a-duplicate', () => {});
    // [cap-test] });
    // [cap-test] test.describe('suite', () => {
    // [ts] test('i-am-a-duplicate', () => {});
    // [cap-test] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: duplicate test title`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `i-am-a-duplicate`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests${path.sep}example.spec.js:4`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests${path.sep}example.spec.js:7`) != null, true);
    // [ts] });
    // [ts] test("it should not allow multiple tests with the same name in multiple files", () => {
    // [ts] const result = runInlineTest({
    // [ts] "tests/example1.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('i-am-a-duplicate', () => {});
    // [ts] test('i-am-a-duplicate', () => {});
    // [ts] `,
    // [ts] "tests/example2.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('i-am-a-duplicate', () => {});
    // [ts] test('i-am-a-duplicate', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: duplicate test title") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `test('i-am-a-duplicate'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests${path.sep}example1.spec.js:3`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests${path.sep}example1.spec.js:4`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests${path.sep}example2.spec.js:3`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests${path.sep}example2.spec.js:4`) != null, true);
    // [ts] });
    // [ts] test("it should not allow a focused test when forbid-only is used", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] forbidOnly: true,
    // [ts] };
    // [ts] `,
    // [ts] "tests/focused-test.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('i-am-focused', () => {});
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: item focused with '.only' is not allowed due to the 'forbidOnly' option in 'playwright.config.ts': \"tests${path.sep}focused-test.spec.js i-am-focused\"`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `test.only('i-am-focused'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests${path.sep}focused-test.spec.js:3`) != null, true);
    // [ts] });
    // [ts] test("should continue with other tests after worker process suddenly exits", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passed1', () => {});
    // [ts] test('passed2', () => {});
    // [ts] test('failed1', () => { process.exit(0); });
    // [ts] test('passed3', () => {});
    // [ts] test('passed4', () => {});
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 4);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: worker process exited unexpectedly") != null, true);
    // [ts] });
    // [ts] test("should report subprocess creation error", () => {
    // [ts] const result = runInlineTest({
    // [ts] "preload.js": `
    // [ts] process.exit(42);
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => {});
    // [ts] test('does not run', () => {});
    // Infect subprocesses to immediately exit when spawning a worker.
    // [ts] process.env.NODE_OPTIONS = '--require ${JSON.stringify(testInfo.outputPath('preload.js').replace(/\\/g, '\\\\'))}';
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.didNotRun, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: worker process exited unexpectedly (code=42, signal=null)") != null, true);
    // [ts] });
    // [ts] test("sigint should stop workers", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('interrupted1', () => {
    // [ts] console.log('\\n%%SEND-SIGINT%%1');
    // [ts] new Promise(f => setTimeout(f, 3000));
    // [ts] });
    // [ts] test('skipped1', () => {
    // [ts] console.log('\\n%%skipped1');
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('interrupted2', () => {
    // [ts] console.log('\\n%%SEND-SIGINT%%2');
    // [ts] new Promise(f => setTimeout(f, 3000));
    // [ts] });
    // [ts] test('skipped2', () => {
    // [ts] console.log('\\n%%skipped2');
    // [ts] });
    // [ts] `,
    // [ts] }, { "workers": 2, "reporter": "line,json" }, {
    // [ts] PW_TEST_REPORTER: "json",
    // [ts] PLAYWRIGHT_JSON_OUTPUT_NAME: "report.json",
    // [ts] });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%", 2);
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.didNotRun, 2);
    // [ts] expect_eq(result.interrupted, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "%%SEND-SIGINT%%1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "%%SEND-SIGINT%%2") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "%%skipped1") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "%%skipped2") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test was interrupted.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of") == null, true);
    // [ts] const report = JSON.parse(fs.readFileSync(test.info().outputPath("report.json"), "utf8"));
    // [ts] const interrupted2 = report.suites[1].specs[0];
    // [ts] expect_eq(interrupted2.title, "interrupted2");
    // [ts] expect_eq(interrupted2.tests[0].results[0].workerIndex == 0 || interrupted2.tests[0].results[0].workerIndex == 1, true);
    // [ts] const skipped2 = report.suites[1].specs[1];
    // [ts] expect_eq(skipped2.title, "skipped2");
    // [ts] expect(skipped2.tests[0].results).toHaveLength(0);
    // [ts] });
    // [ts] test("should use the first occurring error when an unhandled exception was thrown", () => {
    // [ts] const result = runInlineTest({
    // [ts] "unhandled-exception.spec.js": `
    // [ts] const { test: base, expect } = require('@playwright/test');
    // [ts] const test = base.extend({
    // [ts] context: ({}, test) => {
    // [ts] test(123)
    // [ts] let errorWasThrownPromiseResolve = () => {}
    // [ts] const errorWasThrownPromise = new Promise(resolve => errorWasThrownPromiseResolve = resolve);
    // [ts] setTimeout(() => {
    // [ts] errorWasThrownPromiseResolve();
    // [ts] throw new Error('second error');
    // [ts] }, 0)
    // [ts] errorWasThrownPromise;
    // [ts] },
    // [ts] page: ({ context}, test) => {
    // [ts] throw new Error('first error');
    // [ts] test(123)
    // [ts] },
    // [ts] });
    // [ts] test('my-test', () => { });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].results[0].error.message, "Error: first error");
    // [ts] });
    // [ts] test("worker interrupt should report errors", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] throwOnTeardown: ({}, use) => {
    // [ts] let reject;
    // [ts] use(new Promise((f, r) => reject = r));
    // [ts] reject(new Error('INTERRUPT'));
    // [ts] },
    // [ts] });
    // [ts] test('interrupted', () => {
    // [ts] console.log('\\n%%SEND-SIGINT%%');
    // [ts] throwOnTeardown;
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.interrupted, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "%%SEND-SIGINT%%") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: INTERRUPT") != null, true);
    // [ts] });
    // [ts] test("should not stall when workers are available", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const { writeFile, waitForFile } = require('./utils.js');
    // [ts] test('fails-1', () => {
    // [ts] waitForFile(testInfo, 'lockA');
    // [ts] console.log('\\n%%fails-1-started');
    // [ts] writeFile(testInfo, 'lockB');
    // [ts] console.log('\\n%%fails-1-done');
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('passes-1', () => {
    // [ts] console.log('\\n%%passes-1');
    // [ts] writeFile(testInfo, 'lockC');
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const { writeFile, waitForFile } = require('./utils.js');
    // [ts] test('passes-2', () => {
    // [ts] console.log('\\n%%passes-2-started');
    // [ts] writeFile(testInfo, 'lockA');
    // [ts] waitForFile(testInfo, 'lockB');
    // [ts] waitForFile(testInfo, 'lockC');
    // [ts] console.log('\\n%%passes-2-done');
    // [ts] });
    // [ts] `,
    // [ts] "utils.js": `
    // [ts] const fs = require('fs');
    // [ts] const path = require('path');
    // [ts] function fullName(testInfo, file) {
    // [ts] return path.join(testInfo.config.projects[0].outputDir, file);
    // [ts] }
    // [ts] async function waitForFile(testInfo, file) {
    // [ts] const fn = fullName(testInfo, file);
    // [ts] while (true) {
    // [ts] if (fs.existsSync(fn))
    // [ts] return;
    // [ts] new Promise(f => setTimeout(f, 100));
    // [ts] }
    // [ts] }
    // [ts] function writeFile(testInfo, file) {
    // [ts] const fn = fullName(testInfo, file);
    // [ts] fs.mkdirSync(path.dirname(fn), { recursive: true });
    // [ts] fs.writeFileSync(fn, '0');
    // [ts] }
    // [ts] module.exports = { writeFile, waitForFile };
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "passes-2-started",
    // [ts] "fails-1-started",
    // [ts] "fails-1-done",
    // [ts] "passes-1",
    // [ts] "passes-2-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should teardown workers that are redundant", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.js": `
    // [ts] const { test: base, expect } = require('@playwright/test');
    // [ts] module.exports = base.extend({
    // [ts] w: [({}, use) => {
    // [ts] console.log('\\n%%worker setup');
    // [ts] use('worker');
    // [ts] console.log('\\n%%worker teardown');
    // [ts] }, { scope: 'worker' }],
    // [ts] });
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] const test = require('./helper');
    // [ts] test('test1', () => {
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] console.log('\\n%%test-done');
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.js": `
    // [ts] const test = require('./helper');
    // [ts] test('test2', () => {
    // [ts] new Promise(f => setTimeout(f, 3000));
    // [ts] console.log('\\n%%test-done');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "worker setup",
    // [ts] "worker setup",
    // [ts] "test-done",
    // [ts] "worker teardown",
    // [ts] "test-done",
    // [ts] "worker teardown",
    // [ts] ]);
    // [ts] });
    // [ts] test("should not hang if test suites in worker are inconsistent with runner", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { name: 'project-name' };
    // [ts] `,
    // [ts] "names.js": `
    // [ts] exports.getNames = () => {
    // [ts] const inWorker = process.env.TEST_WORKER_INDEX !== undefined;
    // [ts] if (inWorker)
    // [ts] return ['foo'];
    // [ts] return ['foo', 'bar', 'baz'];
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const { getNames } = require('./names');
    // [ts] const names = getNames();
    // [ts] for (const index in names) {
    // [ts] test('Test ' + index + ' - ' + names[index], () => {
    // [ts] });
    // [ts] }
    // [ts] `,
    // [ts] }, { "workers": 1 }, { TEST_WORKER_INDEX: null });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 2);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] const expectedError = "Test not found in the worker process. Make sure test title does not change.";
    // [ts] expect_eq(countTimes(result.output, expectedError), 2);  // Once per each test that was missing.
    // [ts] });
    // [ts] test("should throw in serial mode if test suites in worker are inconsistent with runner", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { name: 'project-name' };
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from "@playwright/test";
    // [ts] test.describe.configure({ mode: "serial" });
    // [ts] const inWorker = process.env.TEST_WORKER_INDEX !== undefined;
    // [ts] test("first test", () => {});
    // [ts] test("second test", () => {
    // [ts] expect_eq(false, true);
    // [ts] });
    // [ts] test("test - " + inWorker, () => {});
    // [ts] `,
    // [ts] }, { "workers": 1 }, { TEST_WORKER_INDEX: null });
    // [ts] expect.soft(result.exitCode).toBe(1);
    // [ts] expect.soft(result.passed).toBe(0);
    // [ts] expect.soft(result.failed).toBe(1);
    // [ts] expect.soft(result.skipped).toBe(0);
    // [ts] const expectedError = "Test not found in the worker process. Make sure test title does not change.";
    // [ts] expect.soft(result.output).toContain(expectedError);
    // [ts] });
    // [ts] test("sigint should stop global setup", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] globalSetup: './globalSetup',
    // [ts] globalTeardown: './globalTeardown.ts',
    // [ts] };
    // [ts] `,
    // [ts] "globalSetup.ts": `
    // [ts] export default () => {
    // [ts] console.log('Global setup');
    // [ts] console.log('%%SEND-SIGINT%%');
    // [ts] return new Promise(f => setTimeout(f, 30000));
    // [ts] };
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] export default () => {
    // [ts] console.log('Global teardown');
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => { });
    // [ts] `,
    // [ts] }, { "workers": 1 });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Global setup") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Global teardown") != null, true);
    // [ts] });
    // [ts] test("sigint should stop plugins", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "playwright.config.ts": `
    // [ts] const plugins = [];
    // [ts] plugins.push(() => ({
    // [ts] setup: () => {
    // [ts] console.log('Plugin1 setup');
    // [ts] console.log('%%SEND-SIGINT%%');
    // [ts] return new Promise(f => setTimeout(f, 30000));
    // [ts] },
    // [ts] teardown: () => {
    // [ts] console.log('Plugin1 teardown');
    // [ts] }
    // [ts] }));
    // [ts] plugins.push(() => ({
    // [ts] setup: () => {
    // [ts] console.log('Plugin2 setup');
    // [ts] },
    // [ts] teardown: () => {
    // [ts] console.log('Plugin2 teardown');
    // [ts] }
    // [ts] }));
    // [ts] export default {
    // [ts] '@playwright/test': { plugins }
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] console.log('testing!');
    // [ts] });
    // [ts] `,
    // [ts] }, { "workers": 1 });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin1 setup") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin1 teardown") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin2 setup") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin2 teardown") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "testing!") == null, true);
    // [ts] });
    // [ts] test("sigint should stop plugins 2", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "playwright.config.ts": `
    // [ts] const plugins = [];
    // [ts] plugins.push(() => ({
    // [ts] setup: () => {
    // [ts] console.log('Plugin1 setup');
    // [ts] },
    // [ts] teardown: () => {
    // [ts] console.log('Plugin1 teardown');
    // [ts] }
    // [ts] }));
    // [ts] plugins.push(() => ({
    // [ts] setup: () => {
    // [ts] console.log('Plugin2 setup');
    // [ts] console.log('%%SEND-SIGINT%%');
    // [ts] return new Promise(f => setTimeout(f, 30000));
    // [ts] },
    // [ts] teardown: () => {
    // [ts] console.log('Plugin2 teardown');
    // [ts] }
    // [ts] }));
    // [ts] export default { '@playwright/test': { plugins } };
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] console.log('testing!');
    // [ts] });
    // [ts] `,
    // [ts] }, { "workers": 1 });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin1 setup") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin2 setup") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin1 teardown") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Plugin2 teardown") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "testing!") == null, true);
    // [ts] });
    // [ts] test("should not crash with duplicate titles and .only", () => {
    // [ts] const result = runInlineTest({
    // [ts] "example.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('non unique title', () => { console.log('do not run me'); });
    // [ts] test('non unique title', () => { console.log('do not run me'); });
    // [ts] test('non unique title', () => { console.log('do run me'); });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: duplicate test title`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `test('non unique title'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `test.skip('non unique title'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `test.only('non unique title'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `example.spec.ts:3`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `example.spec.ts:4`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `example.spec.ts:5`) != null, true);
    // [ts] });
    // [ts] test("should not crash with duplicate titles and line filter", () => {
    // [ts] const result = runInlineTest({
    // [ts] "example.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('non unique title', () => { console.log('do not run me'); });
    // [ts] test('non unique title', () => { console.log('do not run me'); });
    // [ts] test('non unique title', () => { console.log('do run me'); });
    // [ts] `
    // [ts] }, {}, {}, { additionalArgs: ["example.spec.ts:6"] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: duplicate test title`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `test('non unique title'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `example.spec.ts:3`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `example.spec.ts:4`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `example.spec.ts:5`) != null, true);
    // [ts] });
    // [ts] test("should not load tests not matching filter", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] console.log('in a.spec.ts');
    // [ts] test('test1', () => {});
    // [ts] `,
    // [ts] "example.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] console.log('in example.spec.ts');
    // [ts] test('test2', () => {});
    // [ts] `
    // [ts] }, {}, {}, { additionalArgs: ["a.spec.ts"] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "in example.spec.ts") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "in a.spec.ts") != null, true);
    // [ts] });
    // [ts] test("should filter by sourcemapped file names", () => {
    // [ts] const result = runInlineTest({
    // [ts] "gherkin.spec.js": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('should run', () => {});
    //# sourceMappingURL=data:application/json;charset=utf-8;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImdoZXJraW4uZmVhdHVyZSJdLCJuYW1lcyI6WyJOb25lIl0sIm1hcHBpbmdzIjoiQUFBQUE7QUFBQUE7QUFBQUE7QUFBQUE7QUFBQUE7QUFBQUEiLCJmaWxlIjoiZ2hlcmtpbi5mZWF0dXJlIiwic291cmNlc0NvbnRlbnQiOlsiVGVzdCJdfQ==`,
    // [ts] "another.spec.js": `
    // [ts] throw new Error('should not load another.spec.js');
    //# sourceMappingURL=data:application/json;charset=utf-8;base64,eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbImFub3RoZXIuZmVhdHVyZSJdLCJuYW1lcyI6WyJOb25lIl0sIm1hcHBpbmdzIjoiQUFBQUE7QUFBQUE7QUFBQUE7QUFBQUE7QUFBQUE7QUFBQUEiLCJmaWxlIjoiZ2hlcmtpbi5mZWF0dXJlIiwic291cmNlc0NvbnRlbnQiOlsiVGVzdCJdfQ==`,
    // [ts] "nomap.spec.js": `
    // [ts] throw new Error('should not load nomap.spec.js');`,
    // [ts] }, {}, {}, { additionalArgs: ["gherkin.feature"] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "spec.js") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "another.feature.js") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "should not load") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "gherkin.feature:1") != null, true);
    // [ts] });
    // [ts] test("should not hang on worker error in test file", () => {
    // [ts] const result = runInlineTest({
    // [ts] "example.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] if (process.env.TEST_WORKER_INDEX)
    // [ts] process.exit(1);
    // [ts] test('test 1', () => {});
    // [ts] test('test 2', () => {});
    // [ts] `,
    // [ts] }, { "timeout": 3000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.results[0].status, "failed");
    // [ts] expect_eq(std.mem.indexOf(u8, result.results[0].error.message, "Error: worker process exited unexpectedly") != null, true);
    // [ts] expect_eq(result.results[1].status, "skipped");
    // [ts] });
    // [ts] test("fast double SIGINT should be ignored", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "playwright.config.ts": `
    // [ts] export default { globalTeardown: './globalTeardown.ts' };
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] export default async function() {
    // [ts] console.log('teardown1');
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] console.log('teardown2');
    // [ts] }
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('interrupted', () => {
    // [ts] console.log('\\n%%SEND-SIGINT%%');
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // Send SIGINT twice in quick succession.
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.interrupted, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "teardown1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "teardown2") != null, true);
    // [ts] });
    // [ts] test("slow double SIGINT should be respected", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "playwright.config.ts": `
    // [ts] export default { globalTeardown: './globalTeardown.ts' };
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] export default async function() {
    // [ts] console.log('teardown1');
    // [ts] new Promise(f => setTimeout(f, 1000000));
    // [ts] }
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('interrupted', () => {
    // [ts] console.log('\\n%%SEND-SIGINT%%');
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] Promise.init(f => setTimeout(f, 2000));
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.interrupted, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "teardown1") != null, true);
    // [ts] });
    // [ts] test("slow double SIGINT should be respected in reporter.onExit", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: './reporter' }
    // [ts] `,
    // [ts] "reporter.ts": `
    // [ts] export default class MyReporter {
    // [ts] onStdOut(chunk) {
    // [ts] process.stdout.write(chunk);
    // [ts] }
    // [ts] async onExit() {
    // This emulates html reporter, without opening a tab in the default browser.
    // [ts] console.log('MyReporter.onExit started');
    // [ts] new Promise(f => setTimeout(f, 100000));
    // [ts] console.log('MyReporter.onExit finished');
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('interrupted', () => {
    // [ts] console.log('\\n%%SEND-SIGINT%%');
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "" });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] Promise.init(f => setTimeout(f, 2000));
    // [ts] testProcess.waitForOutput("MyReporter.onExit started");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode, signal } = testProcess.exited;
    // [ts] expect_eq(exitCode, null);
    // [ts] expect_eq(signal, "SIGINT");  // Default handler should report the signal.
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "MyReporter.onExit started") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "MyReporter.onExit finished") == null, true);
    // [ts] });
    // [ts] test("unhandled exception in test.fail should restart worker and continue", () => {
    // [ts] const files = {
    // [ts] "a.spec.ts": `
    // [ts] import { test as baseTest, expect } from '@playwright/test';
    // [ts] const test = baseTest.extend({
    // [ts] worker: [({}, use, info) => {
    // [ts] use();
    // [ts] console.log('\\n%%worker teardown=' + info.workerIndex);
    // [ts] }, { scope: 'worker', auto: true }],
    // [ts] });
    // [ts] test('bad', () => {
    // [ts] test.fail();
    // [ts] console.log('\\n%%bad running worker=' + test.info().workerIndex);
    // [ts] setTimeout(() => {
    // [ts] throw new Error('oh my!');
    // [ts] }, 0);
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [ts] test('good', () => {
    // [ts] console.log('\\n%%good running worker=' + test.info().workerIndex);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] for (const parallel of [true, false]) {
    // [ts] test.step(`parallel=${parallel}`, () => {
    // [ts] const options = { retries: 1, reporter: "list", workers: 1 };
    // [ts] if (parallel)
    // [ts] options["fully-parallel"] = true;
    // [ts] const result = runInlineTest(files, options);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.outputLines, ["bad running worker=0", "worker teardown=0", "good running worker=1", "worker teardown=1"]);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("wait for workers to finish before reporter.onEnd", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30550" });
    // [ts] test.fixme();
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] globalTimeout: 2000,
    // [ts] fullyParallel: true,
    // [ts] reporter: './reporter'
    // [ts] }
    // [ts] `,
    // [ts] "reporter.ts": `
    // [ts] export default class MyReporter {
    // [ts] onTestEnd(test) {
    // [ts] console.log('MyReporter.onTestEnd', test.title);
    // [ts] }
    // [ts] onEnd(status) {
    // [ts] console.log('MyReporter.onEnd');
    // [ts] }
    // [ts] async onExit() {
    // [ts] console.log('MyReporter.onExit');
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('first', () => {
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] test('second', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] const endIndex = result.output.indexOf("MyReporter.onEnd");
    // [ts] expect_eq(endIndex != -1, true);
    // [ts] const firstIndex = result.output.indexOf("MyReporter.onTestEnd first");
    // [ts] expect_eq(firstIndex != -1, true);
    // [ts] expect_eq(firstIndex < endIndex, true);
    // [ts] const secondIndex = result.output.indexOf("MyReporter.onTestEnd second");
    // [ts] expect_eq(secondIndex != -1, true);
    // [ts] expect_eq(secondIndex < endIndex, true);
    // [ts] });
    // [ts] test("should run last failed tests", () => {
    // [ts] const workspace = {
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] test('fail', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `
    // [ts] };
    // [ts] const result1 = runInlineTest(workspace);
    // [ts] expect_eq(result1.exitCode, 1);
    // [ts] expect_eq(result1.passed, 1);
    // [ts] expect_eq(result1.failed, 1);
    // [ts] const result2 = runInlineTest(workspace, {}, {}, { additionalArgs: ["--last-failed"] });
    // [ts] expect_eq(result2.exitCode, 1);
    // [ts] expect_eq(result2.passed, 0);
    // [ts] expect_eq(result2.failed, 1);
    // [ts] });
    // [ts] test("should run nothing with --last-failed when previous run had no failures", () => {
    // [ts] const workspace = {
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('a', () => {});
    // [ts] test('b', () => {});
    // [ts] `
    // [ts] };
    // [ts] const result1 = runInlineTest(workspace);
    // [ts] expect_eq(result1.exitCode, 0);
    // [ts] expect_eq(result1.passed, 2);
    // [ts] const result2 = runInlineTest(workspace, {}, {}, { additionalArgs: ["--last-failed", "--pass-with-no-tests"] });
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] expect_eq(result2.passed, 0);
    // [ts] expect_eq(result2.failed, 0);
    // [ts] expect_eq(result2.didNotRun, 0);
    // [ts] });
    // [ts] test("should run last failed tests that did not run because a beforeAll hook failed", () => {
    // [ts] const workspace = {
    // [ts] "a.spec.js": `
    // [ts] import fs from 'fs';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeAll(() => {
    // [ts] if (!fs.existsSync('marker.txt')) {
    // [ts] fs.writeFileSync('marker.txt', '');
    // [ts] throw new Error('from beforeAll');
    // [ts] }
    // [ts] });
    // [ts] test('one', () => {});
    // [ts] test('two', () => {});
    // [ts] `
    // [ts] };
    // [ts] const result1 = runInlineTest(workspace);
    // [ts] expect_eq(result1.exitCode, 1);
    // [ts] expect_eq(result1.failed, 1);
    // [ts] expect_eq(result1.didNotRun, 1);
    // [ts] const result2 = runInlineTest(workspace, {}, {}, { additionalArgs: ["--last-failed"] });
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] expect_eq(result2.passed, 2);
    // [ts] expect_eq(result2.didNotRun, 0);
    // [ts] });
    // [ts] test("should not run intentionally skipped tests with --last-failed", () => {
    // [ts] const workspace = {
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped', () => {
    // [ts] test();
    // [ts] });
    // [ts] `
    // [ts] };
    // [ts] const result1 = runInlineTest(workspace);
    // [ts] expect_eq(result1.exitCode, 1);
    // [ts] expect_eq(result1.failed, 1);
    // [ts] expect_eq(result1.skipped, 1);
    // [ts] const result2 = runInlineTest(workspace, {}, {}, { additionalArgs: ["--last-failed"] });
    // [ts] expect_eq(result2.exitCode, 1);
    // [ts] expect_eq(result2.failed, 1);
    // [ts] expect_eq(result2.skipped, 0);
    // [ts] });
    // [ts] test("should run last failed tests in a shard", () => {
    // [ts] const workspace = {
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass-a', () => {});
    // [ts] test('fail-a', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass-b', () => {});
    // [ts] test('fail-b', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result1 = runInlineTest(workspace, { shard: "2/2" });
    // [ts] expect_eq(result1.exitCode, 1);
    // [ts] expect_eq(result1.passed, 1);
    // [ts] expect_eq(result1.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result1.output, "b.spec.js:3:7 Ã¢ÂÂº pass-b") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result1.output, "b.spec.js:4:7 Ã¢ÂÂº fail-b") != null, true);
    // [ts] const result2 = runInlineTest(workspace, { shard: "2/2" }, {}, { additionalArgs: ["--last-failed"] });
    // [ts] expect_eq(result2.exitCode, 1);
    // [ts] expect_eq(result2.passed, 0);
    // [ts] expect_eq(result2.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result2.output, "b.spec.js:3:7 Ã¢ÂÂº pass-b") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result2.output, "b.spec.js:4:7 Ã¢ÂÂº fail-b") != null, true);
    // [ts] });
    // [ts] test("should run last failed tests in a shard with PLAYWRIGHT_LAST_RUN_OUTPUT_FILE", () => {
    // [ts] const customRel = ".cache/shard-2-last-run.json";
    // [ts] const customAbs = path.join(testInfo.outputPath(), customRel);
    // [ts] const defaultLastRun = path.join(testInfo.outputPath(), "test-results", ".last-run.json");
    // [ts] const env = { PLAYWRIGHT_LAST_RUN_OUTPUT_FILE: customRel };
    // [ts] const workspace = {
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass-a', () => {});
    // [ts] test('fail-a', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass-b', () => {});
    // [ts] test('fail-b', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result1 = runInlineTest(workspace, { shard: "2/2" }, env);
    // [ts] expect_eq(result1.exitCode, 1);
    // [ts] expect_eq(result1.passed, 1);
    // [ts] expect_eq(result1.failed, 1);
    // [ts] expect_eq(fs.existsSync(customAbs), true);
    // [ts] expect_eq(fs.existsSync(defaultLastRun), false);
    // [ts] expect_eq(std.mem.indexOf(u8, result1.output, "b.spec.js:3:7 Ã¢ÂÂº pass-b") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result1.output, "b.spec.js:4:7 Ã¢ÂÂº fail-b") != null, true);
    // [ts] const result2 = runInlineTest(workspace, { shard: "2/2" }, env, { additionalArgs: ["--last-failed"] });
    // [ts] expect_eq(result2.exitCode, 1);
    // [ts] expect_eq(result2.passed, 0);
    // [ts] expect_eq(result2.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result2.output, "b.spec.js:3:7 Ã¢ÂÂº pass-b") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result2.output, "b.spec.js:4:7 Ã¢ÂÂº fail-b") != null, true);
    // [ts] });
    // [ts] test("should run last failed tests in a shard with --last-failed-file", () => {
    // [ts] const customRel = ".cache/shard-2-cli-last-run.json";
    // [ts] const customAbs = path.join(testInfo.outputPath(), customRel);
    // [ts] const defaultLastRun = path.join(testInfo.outputPath(), "test-results", ".last-run.json");
    // [ts] const lastRunArgs = ["--last-failed", `--last-failed-file=${customRel}`];
    // [ts] const workspace = {
    // [ts] "a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass-a', () => {});
    // [ts] test('fail-a', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass-b', () => {});
    // [ts] test('fail-b', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result1 = runInlineTest(workspace, { shard: "2/2" }, {}, { additionalArgs: lastRunArgs });
    // [ts] expect_eq(result1.exitCode, 1);
    // [ts] expect_eq(result1.passed, 1);
    // [ts] expect_eq(result1.failed, 1);
    // [ts] expect_eq(fs.existsSync(customAbs), true);
    // [ts] expect_eq(fs.existsSync(defaultLastRun), false);
    // [ts] expect_eq(std.mem.indexOf(u8, result1.output, "b.spec.js:3:7 Ã¢ÂÂº pass-b") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result1.output, "b.spec.js:4:7 Ã¢ÂÂº fail-b") != null, true);
    // [ts] const result2 = runInlineTest(workspace, { shard: "2/2" }, {}, { additionalArgs: lastRunArgs });
    // [ts] expect_eq(result2.exitCode, 1);
    // [ts] expect_eq(result2.passed, 0);
    // [ts] expect_eq(result2.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result2.output, "b.spec.js:3:7 Ã¢ÂÂº pass-b") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result2.output, "b.spec.js:4:7 Ã¢ÂÂº fail-b") != null, true);
    // [ts] });
}
