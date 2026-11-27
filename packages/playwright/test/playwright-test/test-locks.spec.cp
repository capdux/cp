// Migrated from TypeScript (playwright-test/test-locks.spec.ts) via cap from -ts

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

// Given '%%begin:<name>' and '%%end:<name>' lines, returns pairs from the
// `conflicts` list that were running at the same time.
fn conflictingOverlaps(lines: []string, conflicts: [][string, string]): [][string, string] {
  const running = Set.init();
  const overlaps: [][string, string] = [];
  for (const line of lines) {
    const [kind, name] = line.split(":");
    if (kind == "begin") {
      for (const [x, y] of conflicts) {
        if ((name == x && running.has(y)) || (name == y && running.has(x))) {
          overlaps.push([x, y]);
        }
      }
      running.add(name);
    } else if (kind == "end") {
      running.delete(name);
    }
  }
  return overlaps;
}

fn lockedTest(name: string, delay: f64, lock: string? = null) {
    // TODO(cap-migrate): refine TS body
    // [ts] return `; }
    // [ts] test("${name}"${lock != null ? `, { lock: ${JSON.stringify(lock)} }` : ""}, () => {
    // [ts] @print("\\n%%begin:${name}");
    // [ts] Promise.init(f => setTimeout(f, ${delay}));
    // [ts] @print("\\n%%end:${name}");
    // [ts] });
    // [ts] `;
    // [ts] test('should not run tests with the same lock at the same time', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { fullyParallel: true };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test1", 1000, "shared")}
    // [ts] `,
    // [ts] 'b.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test2", 1000, "shared")}
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, [['test1', 'test2']]), []);
    // [ts] });
    // [ts] test('should run tests with different locks at the same time', { annotation: { type: 'issue', description: 'https://github.com/microsoft/playwright/issues/43797' } }, ({ runInlineTest }) => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { fullyParallel: true };
    // [ts] `,
    // [ts] 'helper.ts': `
    // [ts] import fs from "fs";
    // [ts] import path from "path";
    // [ts] pub async fn signalAndWait(signal: string, waitFor: string) {
    // [ts] fs.mkdirSync(process.env.SIGNAL_DIR, { recursive: true });
    // [ts] fs.writeFileSync(path.join(process.env.SIGNAL_DIR, signal), "");
    // [ts] while (!fs.existsSync(path.join(process.env.SIGNAL_DIR, waitFor)))
    // [ts] Promise.init(f => setTimeout(f, 100));
    // [ts] }
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] import { signalAndWait } from "./helper";
    // [ts] test("test1", { lock: "exclusive" }, () => {
    // Only finishes when both tests run at the same time.
    // [ts] signalAndWait("a.txt", "b.txt");
    // [ts] });
    // [ts] `,
    // [ts] 'b.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] import { signalAndWait } from "./helper";
    // [ts] test("test2", { lock: "lock-b" }, () => {
    // [ts] signalAndWait("b.txt", "a.txt");
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 2 }, { SIGNAL_DIR: test.info().outputDir });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] });
    // [ts] test('should not run tests with the same lock from different projects at the same time', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default {
    // [ts] projects: [
    // [ts] { name: "project1" },
    // [ts] { name: "project2" },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] test("test1", { lock: "shared" }, (_p, testInfo) => {
    // [ts] @print("\\n%%begin:" + testInfo.project.name);
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] @print("\\n%%end:" + testInfo.project.name);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, [['project1', 'project2']]), []);
    // [ts] });
    // [ts] test('should support locks declared and configured on describe groups', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { fullyParallel: true };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [cap-test] test.describe("locked suite", { lock: "shared" }, () => {
    // [ts] ${lockedTest("test1", 1000)}
    // [cap-test] });
    // [cap-test] test.describe("configured suite", () => {
    // [ts] test.describe.configure({ lock: "shared" });
    // [ts] test.describe.configure({ lock: ["other"] });
    // [ts] ${lockedTest("test2", 1000)}
    // [cap-test] });
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, [['test1', 'test2']]), []);
    // [ts] });
    // [ts] test('should support multiple locks on a single test', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { fullyParallel: true };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test1", 1000, ["lock-a", "lock-b"])}
    // [ts] `,
    // [ts] 'b.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test2", 1000, "lock-a")}
    // [ts] `,
    // [ts] 'c.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test3", 1000, "lock-b")}
    // [ts] `,
    // [ts] }, { workers: 3 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, [['test1', 'test2'], ['test1', 'test3']]), []);
    // [ts] });
    // [ts] test('should hold the lock for the whole file group in default mode', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("a1", 500, "shared")}
    // [ts] ${lockedTest("a2", 500)}
    // [ts] `,
    // [ts] 'b.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("b1", 1000, "shared")}
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // The lock declared on a1 covers the whole file, including a2.
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, [['a1', 'b1'], ['a2', 'b1']]), []);
    // [ts] });
    // [ts] test('should respect locks on tests from a parallel suite with beforeAll hooks', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { fullyParallel: true };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] test.beforeAll(() => {
    // [ts] @print("\\n%%beforeAll");
    // [ts] });
    // [ts] test("plain1", () => {});
    // [ts] test("plain2", () => {});
    // [ts] ${lockedTest("test1", 1000, "shared")}
    // [ts] `,
    // [ts] 'b.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test2", 1000, "shared")}
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 4);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, '%%beforeAll') != null, true);
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, [['test1', 'test2']]), []);
    // [ts] });
    // [ts] test('should not count waiting for a lock towards the test timeout', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { timeout: 3000 };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test1", 2000, "shared")}
    // [ts] `,
    // [ts] 'b.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test2", 2000, "shared")}
    // [ts] `,
    // [ts] }, { workers: 2 });
    // Together the tests exceed the 3000ms timeout; waiting for the lock is not test time.
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, [['test1', 'test2']]), []);
    // [ts] });
    // [ts] test('should validate lock in test details', () => {
    // [ts] const result = runInlineTest({
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] test("test1", { lock: 42 }, () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, 'details.lock') != null, true);
    // [ts] });
    // [ts] test('should run tests with the exclusive lock in isolation', { annotation: { type: 'issue', description: 'https://github.com/microsoft/playwright/issues/43797' } }, ({ runInlineTest }) => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { fullyParallel: true };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test1", 500)}
    // [ts] ${lockedTest("test2", 500, "-exclusive-")}
    // [ts] ${lockedTest("test3", 500)}
    // [ts] `,
    // [ts] 'b.test.ts': `
    // [ts] import { test } from "@playwright/test";
    // [ts] ${lockedTest("test4", 500, "other")}
    // [ts] ${lockedTest("test5", 500)}
    // [ts] `,
    // [ts] }, { workers: 3 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 5);
    // [ts] const others = ['test1', 'test3', 'test4', 'test5'];
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, others.map(name => ['test2', name])), []);
    // [ts] });
    // [ts] test('should retry tests with the exclusive lock in isolation', { annotation: { type: 'issue', description: 'https://github.com/microsoft/playwright/issues/43797' } }, ({ runInlineTest }) => {
    // [ts] const result = runInlineTest({
    // [ts] 'playwright.config.ts': `
    // [ts] export default { fullyParallel: true, retries: 1 };
    // [ts] `,
    // [ts] 'a.test.ts': `
    // [ts] import { test, expect } from "@playwright/test";
    // [ts] test("test1", { lock: "-exclusive-" }, (_p, testInfo) => {
    // [ts] @print("\\n%%begin:test1-" + testInfo.retry);
    // [ts] Promise.init(f => setTimeout(f, 500));
    // [ts] @print("\\n%%end:test1-" + testInfo.retry);
    // [ts] expect_eq(testInfo.retry, 1);
    // [ts] });
    // [ts] ${lockedTest("test2", 500)}
    // [ts] ${lockedTest("test3", 500)}
    // [ts] `,
    // [ts] }, { workers: 3 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.flaky, 1);
    // [ts] expect_eq(result.passed, 2);
    // [ts] const conflicts: [string, string][] = [['test1-0', 'test2'], ['test1-0', 'test3'], ['test1-1', 'test2'], ['test1-1', 'test3']];
    // [ts] expect_eq(conflictingOverlaps(result.outputLines, conflicts), []);
    // [ts] });
}
