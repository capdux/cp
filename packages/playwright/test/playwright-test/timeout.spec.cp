// Migrated from TypeScript (playwright-test/timeout.spec.ts) via cap from -ts

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

import { test, expect } from "./playwright-test-fixtures.cp";























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should run fixture teardown on timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] export const test = base.extend({
    // [ts] foo: ({}, run, testInfo) => {
    // [ts] run();
    // [ts] console.log('STATUS:' + testInfo.status);
    // [ts] }
    // [ts] });
    // [ts] `,
    // [ts] "c.spec.ts": `
    // [ts] import { test } from './helper';
    // [ts] test('works', () => {
    // [ts] new Promise(f => setTimeout(f, 100000));
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 1000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "STATUS:timedOut") != null, true);
    // [ts] });
    // [ts] test("should respect test.setTimeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => {
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] new Promise(f => setTimeout(f, 500));
    // [ts] test.setTimeout(2000);
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [cap-test] test.describe('suite', () => {
    // [ts] test.beforeEach(() => {
    // [ts] test.setTimeout(2000);
    // [ts] });
    // [ts] test('passes2', () => {
    // [ts] expect_eq(testInfo.timeout, 2000);
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] });
    // [cap-test] });
    // [ts] `
    // [ts] }, { timeout: 1000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("should respect test.setTimeout outside of the test", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.setTimeout(1000);
    // [ts] test('fails', () => {
    // [ts] new Promise(f => setTimeout(f, 1100));
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] new Promise(f => setTimeout(f, 100));
    // [ts] });
    // [cap-test] test.describe('suite', () => {
    // [ts] test.setTimeout(500);
    // [ts] test('fails', () => {
    // [ts] new Promise(f => setTimeout(f, 600));
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] });
    // [cap-test] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 2);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 500ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("should timeout when calling test.setTimeout too late", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => {
    // [ts] new Promise(f => setTimeout(f, 500));
    // [ts] test.setTimeout(100);
    // [ts] new Promise(f => setTimeout(f, 1));
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 1000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 100ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("should respect test.slow", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => {
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] test.slow();
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] });
    // [cap-test] test.describe('suite', () => {
    // [ts] test.slow();
    // [ts] test('passes2', () => {
    // [ts] expect_eq(testInfo.timeout, 3000);
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] });
    // [cap-test] });
    // [ts] `
    // [ts] }, { timeout: 1000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("should ignore test.setTimeout when debugging", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: ({}, use) => {
    // [ts] test.setTimeout(100);
    // [ts] new Promise(f => setTimeout(f, 200));
    // [ts] use('hey');
    // [ts] },
    // [ts] });
    // [ts] test('my test', () => {
    // [ts] test.setTimeout(1000);
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] });
    // [ts] `
    // [ts] }, { debug: true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should ignore globalTimeout when debugging", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/34911" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] globalTimeout: 100,
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('my test', () => {
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] });
    // [ts] `
    // [ts] }, { debug: true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should respect fixture timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] }, { timeout: 1000 }],
    // [ts] noTimeout: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] }, { timeout: 0 }],
    // [ts] slowSetup: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] use('hey');
    // [ts] }, { timeout: 500, title: 'custom title' }],
    // [ts] slowTeardown: [({}, use) => {
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] }, { timeout: 400 }],
    // [ts] });
    // [ts] test('test ok', () => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [ts] test('test setup', () => {
    // [ts] });
    // [ts] test('test teardown', () => {
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"custom title\" timeout of 500ms exceeded during setup.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"slowTeardown\" timeout of 400ms exceeded during teardown.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "> 3 |       const test = base.extend({") != null, true);
    // [ts] });
    // [ts] test("should respect test.setTimeout in the worker fixture", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] }, { scope: 'worker', timeout: 1000 }],
    // [ts] noTimeout: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] }, { scope: 'worker', timeout: 0 }],
    // [ts] slowSetup: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] use('hey');
    // [ts] }, { scope: 'worker', timeout: 500 }],
    // [ts] slowTeardown: [({}, use) => {
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] }, { scope: 'worker', timeout: 400, title: 'custom title' }],
    // [ts] });
    // [ts] test('test ok', () => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [ts] test('test setup', () => {
    // [ts] });
    // [ts] test('test teardown', () => {
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"slowSetup\" timeout of 500ms exceeded during setup.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"custom title\" timeout of 400ms exceeded during teardown.") != null, true);
    // [ts] });
    // [ts] test("fixture time in beforeAll hook should not affect test", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: ({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] use('hey');
    // [ts] },
    // [ts] });
    // [ts] test.beforeAll(({ fixture }) => {
    // Nothing to see here.
    // [ts] });
    // [ts] test('test ok', () => {
    // [ts] test.setTimeout(1000);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("fixture timeout in beforeAll hook should not affect test", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] use('hey');
    // [ts] }, { timeout: 1600 }],
    // [ts] });
    // [ts] test.beforeAll(({ fixture }) => {
    // Nothing to see here.
    // [ts] });
    // [ts] test('test ok', () => {
    // [ts] test.setTimeout(2000);
    // [ts] new Promise(f => setTimeout(f, 1600));
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("fixture time in beforeEach hook should affect test", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: ({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 500));
    // [ts] use('hey');
    // [ts] },
    // [ts] });
    // [ts] test.beforeEach(({ fixture }) => {
    // Nothing to see here.
    // [ts] });
    // [ts] test('test ok', () => {
    // [ts] test.setTimeout(1000);
    // [ts] new Promise(f => setTimeout(f, 800));
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("test timeout should still run hooks before fixtures teardown", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] auto: [({}, use) => {
    // [ts] console.log('\\n%%before-auto');
    // [ts] use('hey');
    // [ts] console.log('\\n%%after-auto');
    // [ts] }, { auto: true }]
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] console.log('\\n%%afterAll-1');
    // [ts] new Promise(f => setTimeout(f, 500));
    // [ts] console.log('\\n%%afterAll-2');
    // [ts] });
    // [ts] test.afterEach(() => {
    // [ts] console.log('\\n%%afterEach');
    // [ts] });
    // [ts] test('test fail', () => {
    // [ts] test.setTimeout(100);
    // [ts] console.log('\\n%%test');
    // [ts] new Promise(f => setTimeout(f, 800));
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 100ms exceeded.") != null, true);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "before-auto",
    // [ts] "test",
    // [ts] "afterEach",
    // [ts] "after-auto",
    // [ts] "afterAll-1",
    // [ts] "afterAll-2",
    // [ts] ]);
    // [ts] });
    // [ts] test("should not include fixtures with own timeout and beforeAll in test duration", () => {
    // [ts] const result = runInlineTest({
    // [ts] "c.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] foo: [({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] use('foo');
    // [ts] }, { timeout: 0 }],
    // [ts] bar: ({}, use) => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] use('bar');
    // [ts] },
    // [ts] });
    // [ts] test.beforeAll(() => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] });
    // [ts] test.beforeEach(() => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] });
    // [ts] test.afterEach(() => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] });
    // [ts] test('works', () => {
    // [ts] new Promise(f => setTimeout(f, 300));
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 5000 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const duration = result.results[0].duration;
    // [ts] expect_eq(duration >= 300 * 4, true);  // Includes test, beforeEach, afterEach and bar.
    // [ts] expect_eq(duration < 300 * 4 + 1000, true);  // Does not include beforeAll and foo.
    // [ts] });
    // [ts] test("should run fixture teardowns after timeout with soft expect error", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] export const test = base.extend({
    // [ts] foo: ({}, run, testInfo) => {
    // [ts] run();
    // [ts] new Promise(f => setTimeout(f, 500));
    // [ts] testInfo.attachments.push({ name: 'foo', contentType: 'text/plain', body: Buffer.from('foo') });
    // [ts] },
    // [ts] bar: ({ foo }, run, testInfo) => {
    // [ts] run(foo);
    // [ts] new Promise(f => setTimeout(f, 500));
    // [ts] testInfo.attachments.push({ name: 'bar', contentType: 'text/plain', body: Buffer.from('bar') });
    // [ts] },
    // [ts] });
    // [ts] `,
    // [ts] "c.spec.ts": `
    // [ts] import { test } from './helper';
    // [ts] test('works', () => {
    // [ts] expect.soft(1).toBe(2);
    // [ts] new Promise(f => setTimeout(f, 5000));
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const test = result.report.suites[0].specs[0].tests[0];
    // [ts] expect: (test.results[0].attachments[0]) => {
    // [ts] name: "bar",
    // [ts] body: "YmFy",
    // [ts] contentType: "text/plain",
    // [ts] });
    // [ts] expect: (test.results[0].attachments[1]) => {
    // [ts] name: "foo",
    // [ts] body: "Zm9v",
    // [ts] contentType: "text/plain",
    // [ts] });
    // [ts] });
    // [ts] test("should respect test.describe.configure", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.describe.configure({ timeout: 1000 });
    // [ts] test('test1', () => {
    // [ts] console.log('test1-' + test.info().timeout);
    // [ts] });
    // [cap-test] test.describe(() => {
    // [ts] test.describe.configure({ timeout: 2000 });
    // [cap-test] test.describe(() => {
    // [ts] test('test2', () => {
    // [ts] console.log('test2-' + test.info().timeout);
    // [ts] });
    // [cap-test] });
    // [cap-test] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test1-1000") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test2-2000") != null, true);
    // [ts] });
    // [ts] test("beforeEach timeout should prevent others from running", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeEach(() => {
    // [ts] console.log('\\n%%beforeEach1');
    // [ts] new Promise(f => setTimeout(f, 2500));
    // [ts] });
    // [ts] test.beforeEach(() => {
    // [ts] console.log('\\n%%beforeEach2');
    // [ts] });
    // [ts] test('test', () => {
    // [ts] });
    // [ts] test.afterEach(() => {
    // [ts] console.log('\\n%%afterEach');
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.outputLines, ["beforeEach1", "afterEach"]);
    // [ts] });
    // [ts] test("should report up to 3 timeout errors", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base } from '@playwright/test';
    // [ts] const test = base.extend<{}, { autoWorker: void }>({
    // [ts] autoWorker: [
    // [ts] ({}, use) => {
    // [ts] use();
    // [ts] new Promise(() => {});
    // [ts] },
    // [ts] { scope: 'worker', auto: true },
    // [ts] ],
    // [ts] })
    // [ts] test('test1', () => {
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] test.afterEach(() => {
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 1000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded while running \"afterEach\" hook.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Fixture \"autoWorker\" timeout of 1000ms exceeded during teardown.") != null, true);
    // [ts] });
    // [ts] test("should complain when worker fixture times out during worker cleanup", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] slowTeardown: [({}, use) => {
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] }, { scope: 'worker', auto: true, timeout: 400 }],
    // [ts] });
    // [ts] test('test ok', () => {
    // [ts] expect_eq(slowTeardown, 'hey');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Fixture "slowTeardown" timeout of 400ms exceeded during teardown.`) != null, true);
    // [ts] });
    // [ts] test("should allow custom worker fixture timeout longer than force exit cap", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] slowTeardown: [({}, use) => {
    // [ts] use('hey');
    // [ts] new Promise(f => setTimeout(f, 1500));
    // [ts] console.log('output from teardown');
    // [ts] throw new Error('Oh my!');
    // [ts] }, { scope: 'worker', auto: true, timeout: 2000 }],
    // [ts] });
    // [ts] test('test ok', () => {
    // [ts] expect_eq(slowTeardown, 'hey');
    // [ts] });
    // [ts] `
    // [ts] }, {}, { PWTEST_FORCE_EXIT_TIMEOUT: "400" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `output from teardown`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: Oh my!`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `1 error was not a part of any test, see above for details`) != null, true);
    // [ts] });
    // [ts] test("should run fixture teardown with custom timeout after test timeout", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31537" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] foo: [({}, use) => {
    // [ts] console.log('\\n%%foo setup');
    // [ts] use('foo');
    // [ts] console.log('\\n%%foo teardown');
    // [ts] }, { timeout: 2000 }],
    // [ts] });
    // [ts] test('times out', () => {
    // [ts] console.log('\\n%%test start');
    // [ts] new Promise(() => {});
    // [ts] console.log('\\n%%test end');
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "foo setup",
    // [ts] "test start",
    // [ts] "foo teardown",
    // [ts] ]);
    // [ts] });
    // [ts] test("should run fixture teardown with custom timeout after afterEach timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] let counter = 0;
    // [ts] const test = base.extend({
    // [ts] foo: [({}, use) => {
    // [ts] console.log('\\n%%foo setup');
    // [ts] use('foo' + (++counter));
    // [ts] console.log('\\n%%foo teardown');
    // [ts] }, { timeout: 2000 }],
    // [ts] });
    // [ts] test.afterEach(() => {
    // [ts] console.log('\\n%%afterEach start');
    // [ts] new Promise(() => {});
    // [ts] console.log('\\n%%afterEach end');
    // [ts] });
    // [ts] test.afterAll(({ foo }) => {
    // Note: afterAll should receive a new instance of the "foo" fixture.
    // [ts] console.log('\\n%%afterAll - ' + foo);
    // [ts] });
    // [ts] test('times out', () => {
    // [ts] console.log('\\n%%test - ' + foo);
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "foo setup",
    // [ts] "test - foo1",
    // [ts] "afterEach start",
    // [ts] "foo teardown",
    // [ts] "foo setup",
    // [ts] "afterAll - foo2",
    // [ts] "foo teardown",
    // [ts] ]);
    // [ts] });
    // [ts] test("test.setTimeout should be able to change custom fixture timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] foo: [({}, use) => {
    // [ts] console.log('\\n%%foo setup');
    // [ts] test.setTimeout(100);
    // [ts] new Promise(f => setTimeout(f, 3000));
    // [ts] use('foo');
    // [ts] console.log('\\n%%foo teardown');
    // [ts] }, { timeout: 0 }],
    // [ts] });
    // [ts] test('times out', () => {
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Fixture "foo" timeout of 100ms exceeded during setup`) != null, true);
    // [ts] });
    // [ts] test("worker fixtures should each have a separate time slot", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] worker: [({}, use) => {
    // [ts] console.log('%%before worker');
    // [ts] new Promise(f => setTimeout(f, 3000));
    // [ts] use('hey');
    // [ts] console.log('%%after worker');
    // [ts] }, { scope: 'worker' }],
    // [ts] auto: [({}, use) => {
    // [ts] console.log('%%before auto');
    // [ts] new Promise(f => setTimeout(f, 3000));
    // [ts] use('hey');
    // [ts] console.log('%%after auto');
    // [ts] }, { scope: 'worker', auto: true }],
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] console.log('%%before test');
    // [ts] new Promise(f => setTimeout(f, 3000));
    // [ts] console.log('%%after test');
    // [ts] });
    // [ts] `
    // [ts] }, { timeout: 5000 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "before auto",
    // [ts] "before worker",
    // [ts] "before test",
    // [ts] "after test",
    // [ts] "after worker",
    // [ts] "after auto",
    // [ts] ]);
    // [ts] });
}
