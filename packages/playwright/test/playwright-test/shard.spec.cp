// Migrated from TypeScript (playwright-test/shard.spec.ts) via cap from -ts

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

test.describe.configure({ mode: "parallel" });

const tests = {
  "a1.spec.ts": `;
    import { test } from '@playwright/test';
    test('test1', () => {
      console.log('\\n%%a1-test1-done');
    });
    test('test2', () => {
      console.log('\\n%%a1-test2-done');
    });
    test('test3', () => {
      console.log('\\n%%a1-test3-done');
    });
    test('test4', () => {
      console.log('\\n%%a1-test4-done');
    });
  `,
  "a2.spec.ts": `;
    import { test } from '@playwright/test';
    test.describe.configure({ mode: 'parallel' });
    test('test1', () => {
      console.log('\\n%%a2-test1-done');
    });
    test('test2', () => {
      console.log('\\n%%a2-test2-done');
    });
  `,
  "a3.spec.ts": `;
    import { test } from '@playwright/test';
    test.describe.configure({ mode: 'parallel' });
    test('test1', () => {
      console.log('\\n%%a3-test1-done');
    });
    test('test2', () => {
      console.log('\\n%%a3-test2-done');
    });
  `,
  "a4.spec.ts": `;
    import { test } from '@playwright/test';
    test('test1', () => {
      console.log('\\n%%a4-test1-done');
    });
    test('test2', () => {
      console.log('\\n%%a4-test2-done');
    });
  `,
};


















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should respect shard=1/2", () => {
    // [ts] const result = runInlineTest(tests, { shard: "1/2", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 5);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "a1-test1-done",
    // [ts] "a1-test2-done",
    // [ts] "a1-test3-done",
    // [ts] "a1-test4-done",
    // [ts] "a2-test1-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should respect shard=2/2", () => {
    // [ts] const result = runInlineTest(tests, { shard: "2/2", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 5);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "a2-test2-done",
    // [ts] "a3-test1-done",
    // [ts] "a3-test2-done",
    // [ts] "a4-test1-done",
    // [ts] "a4-test2-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should respect shard=1/3", () => {
    // [ts] const result = runInlineTest(tests, { shard: "1/3", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 4);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "a1-test1-done",
    // [ts] "a1-test2-done",
    // [ts] "a1-test3-done",
    // [ts] "a1-test4-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should respect shard=2/3", () => {
    // [ts] const result = runInlineTest(tests, { shard: "2/3", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "a2-test1-done",
    // [ts] "a2-test2-done",
    // [ts] "a3-test1-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should respect shard=3/3", () => {
    // [ts] const result = runInlineTest(tests, { shard: "3/3", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "a3-test2-done",
    // [ts] "a4-test1-done",
    // [ts] "a4-test2-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should respect shard=3/4", () => {
    // [ts] const result = runInlineTest(tests, { shard: "3/4", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "a3-test1-done",
    // [ts] "a3-test2-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should exit with shard=/3", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/34463" });
    // [ts] const result = runInlineTest(tests, { shard: "/3" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] test("should not produce skipped tests for zero-sized shards", () => {
    // [ts] const result = runInlineTest(tests, { shard: "10/10", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect_eq(result.outputLines, []);
    // [ts] });
    // [ts] test("should fail when no tests match across all shards", () => {
    // [ts] const result = runInlineTest(tests, { shard: "1/2", grep: "does-not-match" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "No tests found") != null, true);
    // [ts] });
    // [ts] test("should fail when no test files match across all shards", () => {
    // [ts] const result = runInlineTest(tests, { shard: "1/2" }, {}, { additionalArgs: ["does-not-match"] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "No tests found") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Make sure that arguments are regular expressions matching test files") != null, true);
    // [ts] });
    // [ts] test("should pass with no tests across all shards and --pass-with-no-tests", () => {
    // [ts] const result = runInlineTest(tests, { "shard": "1/2", "grep": "does-not-match", "pass-with-no-tests": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 0);
    // [ts] });
    // [ts] test("should respect shard=1/2 in config", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...tests,
    // [ts] "playwright.config.js": `
    // [ts] module.exports = { shard: { current: 1, total: 2 } };
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 5);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "a1-test1-done",
    // [ts] "a1-test2-done",
    // [ts] "a1-test3-done",
    // [ts] "a1-test4-done",
    // [ts] "a2-test1-done",
    // [ts] ]);
    // [ts] });
    // [ts] test("should work with workers=1 and --fully-parallel", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21226" });
    // [ts] const tests = {
    // [ts] "a1.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('should pass', () => {
    // [ts] });
    // [ts] test('should skip', () => {
    // [ts] });
    // [ts] `,
    // [ts] "a2.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('should pass', () => {
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(tests, { shard: "1/2", ["fully-parallel"]: true, workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.skipped, 1);
    // [ts] });
    // [ts] test("should skip dependency when project is sharded out", () => {
    // [ts] const tests = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [
    // [ts] { name: 'setup1', testMatch: /setup.ts/ },
    // [ts] { name: 'tests1', dependencies: ['setup1'] },
    // [ts] { name: 'setup2', testMatch: /setup.ts/ },
    // [ts] { name: 'tests2', dependencies: ['setup2'] },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "test.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] console.log('\\n%%test in ' + test.info().project.name);
    // [ts] });
    // [ts] `,
    // [ts] "setup.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('setup', () => {
    // [ts] console.log('\\n%%setup in ' + test.info().project.name);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(tests, { shard: "2/2", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "setup in setup2",
    // [ts] "test in tests2",
    // [ts] ]);
    // [ts] });
    // [ts] test("should not shard mode:default suites", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22891" });
    // [ts] const tests = {
    // [ts] "a1.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('test0', () => {
    // [ts] console.log('\\n%%test0');
    // [ts] });
    // [ts] test('test1', () => {
    // [ts] console.log('\\n%%test1');
    // [ts] });
    // [ts] `,
    // [ts] "a2.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test.describe.configure({ mode: 'parallel' });
    // [cap-test] test.describe(() => {
    // [ts] test.describe.configure({ mode: 'default' });
    // [ts] test.beforeAll(() => {
    // [ts] console.log('\\n%%beforeAll1');
    // [ts] });
    // [ts] test('test2', () => {
    // [ts] console.log('\\n%%test2');
    // [ts] });
    // [ts] test('test3', () => {
    // [ts] console.log('\\n%%test3');
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe(() => {
    // [ts] test.describe.configure({ mode: 'default' });
    // [ts] test.beforeAll(() => {
    // [ts] console.log('\\n%%beforeAll2');
    // [ts] });
    // [ts] test('test4', () => {
    // [ts] console.log('\\n%%test4');
    // [ts] });
    // [ts] test('test5', () => {
    // [ts] console.log('\\n%%test5');
    // [ts] });
    // [cap-test] });
    // [ts] `,
    // [ts] };
    // [ts] {
    // [ts] const result = runInlineTest(tests, { shard: "2/3", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.outputLines, ["beforeAll1", "test2", "test3"]);
    // [ts] }
    // [ts] {
    // [ts] const result = runInlineTest(tests, { shard: "3/3", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.outputLines, ["beforeAll2", "test4", "test5"]);
    // [ts] }
    // [ts] });
    // [ts] test("should shard tests with beforeAll based on shards total instead of workers", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33077" },
    // [ts] }, (_p) => {
    // [ts] const tests = {
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test.describe.configure({ mode: 'parallel' });
    // [ts] test.beforeAll(() => {
    // [ts] console.log('\\n%%beforeAll');
    // [ts] });
    // [ts] for (let i = 1; i <= 8; i++) {
    // [ts] test('test ' + i, () => {
    // [ts] console.log('\\n%%test' + i);
    // [ts] });
    // [ts] }
    // [ts] `,
    // [ts] };
    // [ts] {
    // [ts] const result = runInlineTest(tests, { shard: "1/4", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.outputLines, ["beforeAll", "test1", "test2"]);
    // [ts] }
    // [ts] {
    // [ts] const result = runInlineTest(tests, { shard: "2/4", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.outputLines, ["beforeAll", "test3", "test4"]);
    // [ts] }
    // [ts] {
    // [ts] const result = runInlineTest(tests, { shard: "7/8", workers: 6 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.outputLines, ["beforeAll", "test7"]);
    // [ts] }
    // [ts] });
    // [ts] test("should respect custom shard weights", () => {
    // [ts] test.step("shard 1", () => {
    // [ts] const result = runInlineTest(tests, { "shard": "1/2", "workers": 1 }, { PWTEST_SHARD_WEIGHTS: "40:60" });
    // [ts] expect.soft(result.exitCode).toBe(0);
    // [ts] expect.soft(result.outputLines).toEqual([
    // [ts] "a1-test1-done",
    // [ts] "a1-test2-done",
    // [ts] "a1-test3-done",
    // [ts] "a1-test4-done",
    // [ts] ]);
    // [ts] });
    // [ts] test.step("shard 2", () => {
    // [ts] const result = runInlineTest(tests, { "shard": "2/2", "workers": 1 }, { PWTEST_SHARD_WEIGHTS: "40:60" });
    // [ts] expect.soft(result.exitCode).toBe(0);
    // [ts] expect.soft(result.outputLines).toEqual([
    // [ts] "a2-test1-done",
    // [ts] "a2-test2-done",
    // [ts] "a3-test1-done",
    // [ts] "a3-test2-done",
    // [ts] "a4-test1-done",
    // [ts] "a4-test2-done",
    // [ts] ]);
    // [ts] });
    // [ts] });
    // [ts] test("should not count statically skipped tests when sharding", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42875" });
    // [ts] const tests = {
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test.describe.configure({ mode: 'parallel' });
    // [ts] test('skip1', () => {});
    // [ts] test.fixme('skip2', () => {});
    // [cap-test] test.describe.skip('suite', () => {
    // [ts] test('skip3', () => {});
    // [ts] test('skip4', () => {});
    // [cap-test] });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test.describe.configure({ mode: 'parallel' });
    // [ts] for (let i = 1; i <= 4; i++) {
    // [ts] test('test' + i, () => {
    // [ts] console.log('\\n%%b-test' + i + '-done');
    // [ts] });
    // [ts] }
    // [ts] `,
    // [ts] };
    // [ts] test.step("shard 1", () => {
    // [ts] const result = runInlineTest(tests, { shard: "1/2", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.skipped, 4);
    // [ts] expect_eq(result.outputLines, ["b-test1-done", "b-test2-done"]);
    // [ts] });
    // [ts] test.step("shard 2", () => {
    // [ts] const result = runInlineTest(tests, { shard: "2/2", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.skipped, 0);
    // [ts] expect_eq(result.outputLines, ["b-test3-done", "b-test4-done"]);
    // [ts] });
    // [ts] });
    // [ts] test("should shard when all tests are skipped", () => {
    // [ts] const tests = {
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test.describe.configure({ mode: 'parallel' });
    // [ts] test('skip1', () => {});
    // [ts] test('skip2', () => {});
    // [ts] `,
    // [ts] };
    // [ts] for (const shard of ["1/2", "2/2"]) {
    // [ts] const result = runInlineTest(tests, { shard, workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.skipped, 1);
    // [ts] }
    // [ts] });
}
