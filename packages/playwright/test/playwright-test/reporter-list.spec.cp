// Migrated from TypeScript (playwright-test/reporter-list.spec.ts) via cap from -ts

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

import { test, expect, stripAnsi } from "./playwright-test-fixtures.cp";

const DOES_NOT_SUPPORT_UTF8_IN_TERMINAL = process.platform == "win32" && process.env.TERM_PROGRAM != "vscode" && !process.env.WT_SESSION;
const POSITIVE_STATUS_MARK = if (DOES_NOT_SUPPORT_UTF8_IN_TERMINAL) "ok" else "Ã¢ÂÂ ";
const NEGATIVE_STATUS_MARK = if (DOES_NOT_SUPPORT_UTF8_IN_TERMINAL) "x " else "Ã¢ÂÂ ";

// TODO(cap-migrate): refine TS statement
// [ts] for (const useIntermediateMergeReport of [false, true]) { test.describe(`${if (useIntermediateMergeReport) "merged" else "created"}`, () => { test.use({ useIntermediateMergeReport });  test("render each test with project name", (_p) => { const result = runInlineTest({ "playwright.config.ts": ` export default { projects: [ { name: 'foo' }, { name: 'bar' }, ] }; `, "a.test.ts": ` const { test, expect } = require('@playwright/test'); test('fails', async ({}) => { expect(1).toBe(0); }); test('passes', async ({}) => { expect(0).toBe(0); }); test.skip('skipped', async () => { }); `, }, { reporter: "list", workers: "1" }); const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, `${NEGATIVE_STATUS_MARK} 1 [foo] ÃÂ¢ÃÂÃÂº a.test.ts:3:11 ÃÂ¢ÃÂÃÂº fails`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, `${POSITIVE_STATUS_MARK} 2 [foo] ÃÂ¢ÃÂÃÂº a.test.ts:6:11 ÃÂ¢ÃÂÃÂº passes`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, `-  3 [foo] ÃÂ¢ÃÂÃÂº a.test.ts:9:16 ÃÂ¢ÃÂÃÂº skipped`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, `${NEGATIVE_STATUS_MARK} 4 [bar] ÃÂ¢ÃÂÃÂº a.test.ts:3:11 ÃÂ¢ÃÂÃÂº fails`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, `${POSITIVE_STATUS_MARK} 5 [bar] ÃÂ¢ÃÂÃÂº a.test.ts:6:11 ÃÂ¢ÃÂÃÂº passes`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, `-  6 [bar] ÃÂ¢ÃÂÃÂº a.test.ts:9:16 ÃÂ¢ÃÂÃÂº skipped`) != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("render steps", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] test.step('outer 1.0', () => {
// [ts] test.step('inner 1.1', () => {});
// [ts] test.step('inner 1.2', () => {});
// [ts] });
// [ts] test.step('outer 2.0', () => {
// [ts] test.step('inner 2.1', () => {});
// [ts] test.step('inner 2.2', () => {});
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_LIST_PRINT_STEPS: "1", PLAYWRIGHT_FORCE_TTY: "80" });
// [ts] const text = result.output;
// [ts] const lines = text.split("\n").filter(l => l.match("^#.* :")).map(l => l.replace("[.\\d]+m?s", "Xms"));
// [ts] lines.pop(); // Remove last item that contains [v] and time in ms.
// [ts] expect(lines).toEqual([
// [ts] "#0 :      1 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes",
// [ts] "#1 :      1.1 passes ÃÂ¢ÃÂÃÂº outer 1.0",
// [ts] "#2 :      1.2 passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.1",
// [ts] "#2 :      1.2 passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.1 (Xms)",
// [ts] "#3 :      1.3 passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.2",
// [ts] "#3 :      1.3 passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.2 (Xms)",
// [ts] "#1 :      1.1 passes ÃÂ¢ÃÂÃÂº outer 1.0 (Xms)",
// [ts] "#4 :      1.4 passes ÃÂ¢ÃÂÃÂº outer 2.0",
// [ts] "#5 :      1.5 passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.1",
// [ts] "#5 :      1.5 passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.1 (Xms)",
// [ts] "#6 :      1.6 passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.2",
// [ts] "#6 :      1.6 passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.2 (Xms)",
// [ts] "#4 :      1.4 passes ÃÂ¢ÃÂÃÂº outer 2.0 (Xms)",
// [ts] ]);
// [ts] });
// [ts] test("render steps inline", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] test.step('outer 1.0', () => {
// [ts] test.step('inner 1.1', () => {});
// [ts] test.step('inner 1.2', () => {});
// [ts] });
// [ts] test.step('outer 2.0', () => {
// [ts] test.step('inner 2.1', () => {});
// [ts] test.step('inner 2.2', () => {});
// [ts] });
// [ts] });`,
// [ts] }, { reporter: "list" }, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_FORCE_TTY: "80" });
// [ts] const text = result.output;
// [ts] const lines = text.split("\n").filter(l => l.match("^#.* :")).map(l => l.replace("[.\\d]+m?s", "Xms"));
// [ts] lines.pop(); // Remove last item that contains [v] and time in ms.
// [ts] expect(lines).toEqual([
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.1",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.2",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.1",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.2",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0",
// [ts] "#0 :      1 a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes",
// [ts] ]);
// [ts] });
// [ts] test("render steps in non-TTY mode", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] test.step('outer 1.0', () => {
// [ts] test.step('inner 1.1', () => {});
// [ts] test.step('inner 1.2', () => {});
// [ts] });
// [ts] test.step('outer 2.0', () => {
// [ts] test.step('inner 2.1', () => {});
// [ts] test.step('inner 2.2', () => {});
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_LIST_PRINT_STEPS: "1" });
// [ts] const text = result.output;
// [ts] const lines = text.split("\n").filter(l => l.match("^#.* :")).map(l => l.replace("[.\\d]+m?s", "Xms"));
// [ts] expect(lines).toEqual([
// [ts] "#0 :      1.1 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.1 (Xms)",
// [ts] "#1 :      1.2 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0 ÃÂ¢ÃÂÃÂº inner 1.2 (Xms)",
// [ts] "#2 :      1.3 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 1.0 (Xms)",
// [ts] "#3 :      1.4 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.1 (Xms)",
// [ts] "#4 :      1.5 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0 ÃÂ¢ÃÂÃÂº inner 2.2 (Xms)",
// [ts] "#5 :      1.6 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes ÃÂ¢ÃÂÃÂº outer 2.0 (Xms)",
// [ts] `#6 :   ${POSITIVE_STATUS_MARK} 1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº passes (Xms)`,
// [ts] ]);
// [ts] });
// [ts] test("very long console line should not mess terminal", () => {
// [ts] const TTY_WIDTH = 80;
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] console.log('a'.repeat(80) + 'b'.repeat(20));
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { PLAYWRIGHT_FORCE_TTY: TTY_WIDTH + "" });
// [ts] const renderedText = simpleAnsiRenderer(result.rawOutput, TTY_WIDTH);
// [ts] if (process.platform == "win32")
// [ts] expect_eq(std.mem.indexOf(u8, renderedText, "  ok 1 a.test.ts:3:11 Ã¢ÂÂº passes") != null, true);
// [ts] else {
// [ts] expect_eq(std.mem.indexOf(u8, renderedText, "  Ã¢ÂÂ  1 a.test.ts:3:11 Ã¢ÂÂº passes") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, renderedText, "     1 a.test.ts:3:11 Ã¢ÂÂº passes") == null, true);
// [ts] expect_eq(std.mem.indexOf(u8, renderedText, "a".repeat(80) + "\n" + "b".repeat(20)) != null, true);
// [ts] });
// [ts] test("render retries", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('flaky', () => {
// [ts] expect_eq(testInfo.retry, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list", retries: "1" }, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_FORCE_TTY: "80" });
// [ts] const text = result.output;
// [ts] const lines = text.split("\n").filter(l => l.startsWith("#0 :") || l.startsWith("#1 :")).map(l => l.replace("\\d+(\\.\\d+)?m?s", "XXms"));
// [ts] expect(lines).toEqual([
// [ts] `#0 :      1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky`,
// [ts] `#0 :   ${NEGATIVE_STATUS_MARK} 1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky (XXms)`,
// [ts] `#1 :      2 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky (retry #1)`,
// [ts] `#1 :   ${POSITIVE_STATUS_MARK} 2 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky (retry #1) (XXms)`,
// [ts] ]);
// [ts] });
// [ts] test("should truncate long test names", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { projects: [
// [ts] { name: 'foo' },
// [ts] ] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('failure in very long name', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] test('passes', () => {
// [ts] });
// [ts] test('passes 2 long name', () => {
// [ts] });
// [ts] test('skipped very long name', () => {
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list", retries: 0 }, { PLAYWRIGHT_FORCE_TTY: "50" });
// [ts] expect_eq(result.exitCode, 1);
// [ts] const lines = result.rawOutput.split("\n").map(line => line.split("\x1B[22m\x1B[1E")).flat().map(line => stripAnsi(line)).filter(line => line.trim()).slice(1, 9);
// [ts] expect_eq(lines.every(line => line.length <= 50), true);
// [ts] expect_eq(lines[0], `     1 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ¦a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failure in very long name`);
// [ts] expect_eq(std.mem.indexOf(u8, lines[1], `${NEGATIVE_STATUS_MARK} 1 ÃÂ¢ÃÂÃÂ¦`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, lines[1], `:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failure in very long name (`) != null, true);
// [ts] expect_eq(lines[1].length, 50);
// [ts] expect_eq(lines[2], `     2 [foo] ÃÂ¢ÃÂÃÂº a.test.ts:6:11 ÃÂ¢ÃÂÃÂº passes`);
// [ts] expect_eq(std.mem.indexOf(u8, lines[3], `${POSITIVE_STATUS_MARK} 2 [foo] ÃÂ¢ÃÂÃÂº a.test.ts:6:11 ÃÂ¢ÃÂÃÂº passes (`) != null, true);
// [ts] expect_eq(lines[4], `     3 [foo] ÃÂ¢ÃÂÃÂº a.test.ts:8:11 ÃÂ¢ÃÂÃÂº passes 2 long name`);
// [ts] expect_eq(std.mem.indexOf(u8, lines[5], `${POSITIVE_STATUS_MARK} 3 ÃÂ¢ÃÂÃÂ¦`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, lines[5], `test.ts:8:11 ÃÂ¢ÃÂÃÂº passes 2 long name (`) != null, true);
// [ts] expect_eq(lines[5].length, 50);
// [ts] expect_eq(lines[6], `     4 ÃÂ¢ÃÂÃÂ¦ÃÂ¢ÃÂÃÂº a.test.ts:10:16 ÃÂ¢ÃÂÃÂº skipped very long name`);
// [ts] expect_eq(lines[7], `  -  4 ÃÂ¢ÃÂÃÂ¦ÃÂ¢ÃÂÃÂº a.test.ts:10:16 ÃÂ¢ÃÂÃÂº skipped very long name`);
// [ts] });
// [ts] test("render failed test steps", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] test.step('outer 1.0', () => {
// [ts] test.step('inner 1.1', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "1) a.test.ts:3:11 Ã¢ÂÂº passes Ã¢ÂÂº outer 1.0 Ã¢ÂÂº inner 1.1 Ã¢ÂÂÃ¢ÂÂ") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("print failures inline with option", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['list', { printFailuresInline: true }]],
// [ts] workers: 1,
// [ts] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails early', () => {
// [ts] expect_eq(1, 2);
// [ts] });
// [ts] test('runs later', () => {
// [ts] });
// [ts] `,
// [ts] });
// [ts] const text = result.output;
// [ts] const failureHeader = "1) a.test.ts:3:11 Ã¢ÂÂº fails early";
// [ts] const laterTestStatus = `${POSITIVE_STATUS_MARK} 2 a.test.ts:6:11 ÃÂ¢ÃÂÃÂº runs later`;
// [ts] const failureIndex = text.indexOf(failureHeader);
// [ts] const laterTestIndex = text.indexOf(laterTestStatus);
// [ts] expect_eq(failureIndex > -1, true);
// [ts] expect_eq(laterTestIndex > failureIndex, true);
// [ts] expect_eq(text.indexOf("Error: expect(received).toBe(expected)", failureIndex) > failureIndex, true);
// [ts] expect_eq(text.indexOf(failureHeader, failureIndex + 1), -1);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("print failures inline for a non-retriable error", () => {
// A missing snapshot fails the test but is not retried. The reporter must
// still print the failure inline, even though the retry budget is not exhausted.
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['list', { printFailuresInline: true }]],
// [ts] retries: 1,
// [ts] workers: 1,
// [ts] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('missing snapshot', () => {
// [ts] expect('actual').toMatchSnapshot('foo.txt');
// [ts] });
// [ts] `,
// [ts] });
// [ts] const text = result.output;
// [ts] const failureHeader = "1) a.test.ts:3:11 Ã¢ÂÂº missing snapshot";
// [ts] const failureIndex = text.indexOf(failureHeader);
// [ts] expect_eq(failureIndex, "failure should be printed inline" != -1, true);
// [ts] expect_eq(text.indexOf(`A snapshot doesn't exist`, failureIndex) > failureIndex, true);
// It must not be retried, so there is exactly one failure block.
// [ts] expect_eq(text.indexOf(failureHeader, failureIndex + 1), -1);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("print failures inline with retries", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['list', { printFailuresInline: true }]],
// [ts] retries: 2,
// [ts] workers: 1,
// [ts] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fails', () => {
// [ts] throw new Error('attempt ' + testInfo.retry);
// [ts] });
// [ts] `,
// [ts] });
// [ts] const text = result.output;
// [ts] const failureHeader = "1) a.test.ts:3:11 Ã¢ÂÂº fails";
// Each attempt is printed once, under the same failure index.
// [ts] expect_eq(text.split(failureHeader).length - 1, 3);
// [ts] expect_eq(std.mem.indexOf(u8, text, "2) a.test.ts:3:11 Ã¢ÂÂº fails") == null, true);
// [ts] for (const attempt of ["attempt 0", "attempt 1", "attempt 2"])
// [ts] expect_eq(text.split("Error: " + attempt).length - 1, 1);
// [ts] expect_eq(text.split("Retry #1").length - 1, 1);
// [ts] expect_eq(text.split("Retry #2").length - 1, 1);
// Retry sections come after the corresponding retry attempt.
// [ts] expect_eq(text.indexOf("Retry #1") > text.indexOf("Error: attempt 0"), true);
// [ts] expect_eq(text.indexOf("Retry #2") > text.indexOf("Error: attempt 1"), true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("print failures inline for a flaky test", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [['list', { printFailuresInline: true }]],
// [ts] retries: 1,
// [ts] workers: 1,
// [ts] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('flaky', () => {
// [ts] if (testInfo.retry === 0)
// [ts] throw new Error('attempt ' + testInfo.retry);
// [ts] });
// [ts] `,
// [ts] });
// [ts] const text = result.output;
// [ts] const failureHeader = "1) a.test.ts:3:11 Ã¢ÂÂº flaky";
// [ts] expect_eq(text.split(failureHeader).length - 1, 1);
// [ts] expect_eq(text.split("Error: attempt 0").length - 1, 1);
// [ts] expect_eq(std.mem.indexOf(u8, text, "Retry #1") == null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, "1 flaky") != null, true);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("print stdio", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] new Promise(resolve => process.stdout.write('line1', () => resolve()));
// [ts] new Promise(resolve => process.stdout.write('line2\\n', () => resolve()));
// [ts] new Promise(resolve => process.stderr.write(Buffer.from(''), () => resolve()));
// [ts] });
// [ts] test('passes 2', () => {
// [ts] new Promise(resolve => process.stdout.write('partial', () => resolve()));
// [ts] });
// [ts] test('passes 3', () => {
// [ts] new Promise(resolve => process.stdout.write('full\\n', () => resolve()));
// [ts] });
// [ts] test('passes 4', () => {
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_FORCE_TTY: "80" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 4);
// [ts] const expected = [
// [ts] "#0 :      1 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº passes",
// [ts] "line1line2",
// [ts] `#0 :   ${POSITIVE_STATUS_MARK} 1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº passes`,
// [ts] "",
// [ts] "#3 :      2 a.test.ts:9:11 ÃÂ¢ÃÂÃÂº passes 2",
// [ts] `partial#3 :   ${POSITIVE_STATUS_MARK} 2 a.test.ts:9:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº passes 2`,
// [ts] "",
// [ts] "#5 :      3 a.test.ts:13:11 ÃÂ¢ÃÂÃÂº passes 3",
// [ts] "full",
// [ts] `#5 :   ${POSITIVE_STATUS_MARK} 3 a.test.ts:13:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº passes 3`,
// [ts] "#7 :      4 a.test.ts:17:11 ÃÂ¢ÃÂÃÂº passes 4",
// [ts] `#7 :   ${POSITIVE_STATUS_MARK} 4 a.test.ts:17:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº passes 4`,
// [ts] ];
// [ts] const lines = result.output.split("\n");
// [ts] const firstIndex = lines.indexOf(expected[0]);
// [ts] expect_eq(firstIndex, "first line should be there" != -1, true);
// [ts] let i = 0;
// [ts] while (i < expected.length) // [cap-migrate] for-step: ++i
// [ts] expect_eq(std.mem.indexOf(u8, lines[firstIndex + i], expected[i]) != null, true);
// [ts] });
// [ts] test("should update test status row only when TTY has not scrolled", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('A', () => {
// [ts] for (let i = 0; i < 20; ++i) {
// [ts] console.log('line ' + i);
// [ts] }
// [ts] });
// [ts] test('B', () => {
// Go past end of the screen
// [ts] for (let i = 20; i < 60; ++i) {
// [ts] console.log('line ' + i);
// [ts] }
// Should create new line
// [ts] test.step('First step', () => {
// [ts] console.log('step 1');
// [ts] });
// [ts] for (let i = 60; i < 80; ++i) {
// [ts] console.log('line ' + i);
// [ts] }
// Should update the new (not original) line
// [ts] test.step('Second step', () => {
// [ts] console.log('step 2');
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_FORCE_TTY: "80" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 2);
// [ts] const expected = [
// [ts] "#0 :      1 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº A",
// [ts] ];
// [ts] let i = 0;
// [ts] while (i < 20) // [cap-migrate] for-step: ++i
// [ts] expected.push(`line ${i}`);
// Update to initial test status row
// [ts] expected.push(`#0 :   ${POSITIVE_STATUS_MARK} 1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº A`);
// [ts] expected.push(`#21 :      2 a.test.ts:9:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº B`);
// [ts] let i = 20;
// [ts] while (i < 60) // [cap-migrate] for-step: ++i
// [ts] expected.push(`line ${i}`);
// [ts] expected.push(`#62 :      2 a.test.ts:9:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº B ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº First step`);
// [ts] expected.push(`step 1`);
// [ts] expected.push(`#62 :      2 a.test.ts:9:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº B`);
// [ts] let i = 60;
// [ts] while (i < 80) // [cap-migrate] for-step: ++i
// [ts] expected.push(`line ${i}`);
// [ts] expected.push(`#62 :      2 a.test.ts:9:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº B ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº Second step`);
// [ts] expected.push(`step 2`);
// [ts] expected.push(`#62 :      2 a.test.ts:9:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº B`);
// [ts] expected.push(`#62 :   ${POSITIVE_STATUS_MARK} 2 a.test.ts:9:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº B`);
// [ts] const lines = result.output.split("\n");
// [ts] const firstIndex = lines.indexOf(expected[0]);
// [ts] expect_eq(firstIndex, "first line should be there" != -1, true);
// [ts] let i = 0;
// [ts] while (i < expected.length) // [cap-migrate] for-step: ++i
// [ts] expect_eq(std.mem.indexOf(u8, lines[firstIndex + i], expected[i]) != null, true);
// [ts] });
// [ts] test("should update test status row only within configured TTY height", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('A', () => {
// No scroll
// [ts] for (let i = 0; i < 60; ++i) {
// [ts] console.log('line ' + i);
// [ts] }
// Update original line
// [ts] test.step('First step', () => {
// [ts] console.log('step 1');
// [ts] });
// [ts] for (let i = 60; i < 120; ++i) {
// [ts] console.log('line ' + i);
// [ts] }
// Should create new line
// [ts] test.step('Second step', () => {
// [ts] console.log('step 2');
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_FORCE_TTY: "80x80" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] const expected = [
// [ts] "#0 :      1 a.test.ts:3:11 ÃÂ¢ÃÂÃÂº A",
// [ts] ];
// [ts] let i = 0;
// [ts] while (i < 60) // [cap-migrate] for-step: ++i
// [ts] expected.push(`line ${i}`);
// Update to initial test status row
// [ts] expected.push(`#0 :      1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº A ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº First step`);
// [ts] expected.push(`step 1`);
// [ts] expected.push(`#0 :      1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº A`);
// [ts] let i = 60;
// [ts] while (i < 120) // [cap-migrate] for-step: ++i
// [ts] expected.push(`line ${i}`);
// [ts] expected.push(`#122 :      1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº A ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº Second step`);
// [ts] expected.push(`step 2`);
// [ts] expected.push(`#122 :      1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº A`);
// [ts] expected.push(`#122 :   ${POSITIVE_STATUS_MARK} 1 a.test.ts:3:11 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº A`);
// [ts] const lines = result.output.split("\n");
// [ts] const firstIndex = lines.indexOf(expected[0]);
// [ts] expect_eq(firstIndex, "first line should be there" != -1, true);
// [ts] let i = 0;
// [ts] while (i < expected.length) // [cap-migrate] for-step: ++i
// [ts] expect_eq(std.mem.indexOf(u8, lines[firstIndex + i], expected[i]) != null, true);
// [ts] });
// [ts] });
// [ts] }
// [ts] test("strips ansi from test stdout when FORCE_COLOR=0", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] console.log('\\u001b[31mRED\\u001b[39mWHITE');
// [ts] process.stderr.write('GREEN\\u001b[32mDOT\\u001b[39mEND\\n');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { FORCE_COLOR: "0", PLAYWRIGHT_FORCE_TTY: "80" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, "REDWHITE") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, "GREENDOTEND") != null, true);
// [ts] });
// [ts] test("strips ansi from test stdout when NO_COLOR is set", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] console.log('\\u001b[31mRED\\u001b[39mWHITE');
// [ts] process.stderr.write('GREEN\\u001b[32mDOT\\u001b[39mEND\\n');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "list" }, { NO_COLOR: "1", FORCE_COLOR: "", PLAYWRIGHT_FORCE_TTY: "80" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, "REDWHITE") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, "GREENDOTEND") != null, true);
// [ts] });
// [ts] fn simpleAnsiRenderer(text: auto, ttyWidth: auto) {
// [ts] let lineNumber = 0;
// [ts] let columnNumber = 0;
// [ts] const screenLines: [][]string = [];
// [ts] const ensureScreenSize = () => {
// [ts] if (lineNumber < 0) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Bad terminal navigation!');)
// [ts] }
// [ts] while (lineNumber >= screenLines.length) {
// [ts] screenLines.push(Array.init(ttyWidth).fill(""));
// [ts] }
// [ts] };
// [ts] const print = ch => {
// [ts] ensureScreenSize();
// [ts] if (ch == "\n") {
// [ts] }
// [ts] columnNumber = 0;
// [ts] ++lineNumber;
// [ts] } else {
// [ts] screenLines[lineNumber][columnNumber++] = ch;
// [ts] if (columnNumber == ttyWidth) {
// [ts] columnNumber = 0;
// [ts] ++lineNumber;
// [ts] }
// [ts] }
// [ts] ensureScreenSize();
// [ts] };
// [ts] let index = 0;
// [ts] const ansiCodes = [...text.matchAll("\\u001B\\[(\\d*)(.)")];
// [ts] for (const ansiCode of ansiCodes) {
// [ts] const [matchText, codeValue, codeType] = ansiCode;
// [ts] const code = (codeValue + codeType).toUpperCase();
// [ts] while (index < ansiCode.index) {
// [ts] print(text[index++]);
// [ts] }
// [ts] if (codeType.toUpperCase() == "E") {
// Go X lines down
// [ts] lineNumber += codeValue;
// [ts] ensureScreenSize();
// [ts] } else if (codeType.toUpperCase() == "A") {
// Go X lines up
// [ts] lineNumber -= codeValue;
// [ts] ensureScreenSize();
// [ts] } else if (code == "2K") {
// Erase full line
// [ts] ensureScreenSize();
// [ts] screenLines[lineNumber] = Array.init(ttyWidth).fill("");
// [ts] } else if (code == "0G") {
// Go to start
// [ts] columnNumber = 0;
// [ts] } else {
// Unsupported ANSI code (e.g. all colors).
// [ts] }
// [ts] index += matchText.length;
// [ts] }
// [ts] while (index < text.length) {
// [ts] print(text[index++]);
// [ts] }
// [ts] return screenLines.map(line => line.join("")).join("\n");
// [ts] }
