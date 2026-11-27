// Migrated from TypeScript (playwright-test/shuffle.spec.ts) via cap from -ts

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

const count = 10;

fn testFile(name: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] return `;
    // [ts] import { test } from '@playwright/test';
    // [ts] ${Array.from({ length: count }, (_, i) => `test('test${i}', () => console.log('\\n%%${name}-test${i}'));`).join('\n')}
    // [ts] `;
}

const fileNames = ["a", "b", "c", "d", "e", "f", "g", "h"];
const files = Object.fromEntries(fileNames.map(name => [`${name}.spec.ts`, testFile(name)]));
fn testsInFile(name: string) { return Array.from({ length: count }, (_unused, i) => `${name}-test${i}`); }
const naturalOrder = fileNames.flatMap(testsInFile);



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should shuffle files and keep tests within a file in order", () => {
    // [ts] const result = runInlineTest(files, { workers: 1, shuffle: "42" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, naturalOrder.length);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "shuffle seed 42") != null, true);
    // [ts] expect_eq(result.outputLines != naturalOrder, true);
    // [ts] const fileOrder = [...Set.init(result.outputLines.map(line => line.split("-")[0]))];
    // [ts] expect_eq(result.outputLines, fileOrder.flatMap(testsInFile));
    // [ts] const result2 = runInlineTest(files, { workers: 1, shuffle: "42" });
    // [ts] expect_eq(result2.outputLines, result.outputLines);
    // [ts] const result3 = runInlineTest(files, { workers: 1, shuffle: "43" });
    // [ts] expect_eq(result3.outputLines != result.outputLines, true);
    // [ts] });
    // [ts] test("should shuffle individual tests in fully parallel mode", () => {
    // [ts] const result = runInlineTest(files, { "workers": 1, "shuffle": "42", "fully-parallel": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, naturalOrder.length);
    // [ts] const aTests = result.outputLines.filter(line => line.startsWith("a-"));
    // [ts] expect_eq(aTests != testsInFile("a"), true);
    // [ts] expect_eq([...aTests].sort(), [...testsInFile("a")].sort());
    // [ts] });
    // [ts] test("should pick and print a random seed", () => {
    // [ts] const result = runInlineTest(files, { workers: 1, shuffle: true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const seed = result.output.match("shuffle seed (\\d+)")[1];
    // [ts] const result2 = runInlineTest(files, { workers: 1, shuffle: seed });
    // [ts] expect_eq(result2.outputLines, result.outputLines);
    // [ts] });
    // [ts] test("should keep serial suites together in parallel mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.describe.configure({ mode: 'parallel' });
    // [ts] ${Array.from({ length: count }, (_, i) => `test('test${i}', () => console.log('\\n%%test${i}'));`).join('\n')}
    // [ts] test.describe.serial('serial', () => {
    // [ts] let counter = 0;
    // [ts] ${Array.from({ length: count }, (_, i) => `test('serial${i}', () => { expect(counter++).toBe(${i}); console.log('\\n%%serial${i}'); });`).join('\n')}
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, shuffle: "7" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2 * count);
    // [ts] const lines = result.outputLines;
    // [ts] const serialStart = lines.indexOf("serial0");
    // [ts] expect_eq(lines.slice(serialStart, serialStart + count), Array.from({ length: count }, (_unused, i) => `serial${i}`));
    // [ts] expect_eq(lines.filter(line => line.startsWith("test")) != Array.from({ length: count }, (_unused, i) => `test${i}`), true);
    // [ts] });
}
