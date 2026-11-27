// Migrated from TypeScript (playwright-test/reporter-line.spec.ts) via cap from -ts

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

import path from "cap:path";
import { test, expect } from "./playwright-test-fixtures.cp";

// TODO(cap-migrate): refine TS statement
// [ts] for (const useIntermediateMergeReport of [false, true]) { test.describe(`${if (useIntermediateMergeReport) "merged" else "created"}`, () => { test.use({ useIntermediateMergeReport });  test("render unexpected after retry", (_p) => { const result = runInlineTest({ "a.test.js": ` const { test, expect } = require('@playwright/test'); test('one', async ({}) => { expect(1).toBe(0); }); `, }, { retries: 3, reporter: "line" }); const text = result.output; expect_eq(std.mem.indexOf(u8, text, "[1/1] a.test.js:3:11 ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº one") != null, true); expect_eq(std.mem.indexOf(u8, text, "[2/1] (retries) a.test.js:3:11 ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº one (retry #1)") != null, true); expect_eq(std.mem.indexOf(u8, text, "[3/1] (retries) a.test.js:3:11 ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº one (retry #2)") != null, true); expect_eq(std.mem.indexOf(u8, text, "[4/1] (retries) a.test.js:3:11 ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº one (retry #3)") != null, true); expect_eq(std.mem.indexOf(u8, text, "1 failed") != null, true); expect_eq(std.mem.indexOf(u8, text, "1) a.test") != null, true); expect_eq(std.mem.indexOf(u8, text, "2) a.test") == null, true); expect_eq(std.mem.indexOf(u8, text, "Retry #1 ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ") != null, true); expect_eq(std.mem.indexOf(u8, text, "Retry #2 ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ") != null, true); expect_eq(std.mem.indexOf(u8, text, "Retry #3 ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ") != null, true); expect_eq(result.exitCode, 1); });
// [ts] test("should not label passing tests as retries", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('failing', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] test('passing', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { retries: 1, reporter: "line" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "[1/2] a.test.js:3:11 Ã¢ÂÂº failing") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, "a.test.js:3:11 ÃÂ¢ÃÂÃÂº failing (retry #1)") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, "[3/2] a.test.js:6:11 Ã¢ÂÂº passing") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, "(retries) a.test.js:6:11 Ã¢ÂÂº passing") == null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("render flaky", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(testInfo.retry, 3);
// [ts] });
// [ts] `,
// [ts] }, { retries: 3, reporter: "line" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "1 flaky") != null, true);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should print flaky failures", () => {
// [ts] const result = runInlineTest({
// [ts] "a.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('foobar', () => {
// [ts] expect_eq(testInfo.retry, 1);
// [ts] });
// [ts] `
// [ts] }, { retries: "1", reporter: "line" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.flaky, 1);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "expect(testInfo.retry).toBe(1)") != null, true);
// [ts] });
// [ts] test("should work on CI", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('one', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "line" }, { CI: "1" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "[1/1] a.test.js:3:11 Ã¢ÂÂº one") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, "1 failed") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, text, "1) a.test") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should print output", () => {
// [ts] const result = runInlineTest({
// [ts] "a.spec.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('foobar', () => {
// [ts] process.stdout.write('one');
// [ts] process.stdout.write('two');
// [ts] console.log('full-line');
// [ts] });
// [ts] `
// [ts] }, { reporter: "line" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect(result.output).toContain([
// [ts] "a.spec.ts:3:11 Ã¢ÂÂº foobar",
// [ts] "one",
// [ts] "",
// [ts] "two",
// [ts] "",
// [ts] "full-line",
// [ts] ].join("\n"));
// [ts] });
// [ts] test("should trim multiline step titles to first line", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31266" }
// [ts] }, (_p) => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] test.step(\`outer
// [ts] 1.0\`, async () => {
// [ts] test.step(\`inner
// [ts] 1.1\`, async () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "line" }, { PLAYWRIGHT_FORCE_TTY: "1" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "[1/1] a.test.ts:3:11 Ã¢ÂÂº passes Ã¢ÂÂº outer Ã¢ÂÂº inner") != null, true);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] }
// [ts] }
// [ts] fn main() {
// [ts] test("should render failed test steps", () => {
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
// [ts] }, { reporter: "line" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "1) a.test.ts:3:11 Ã¢ÂÂº passes Ã¢ÂÂº outer 1.0 Ã¢ÂÂº inner 1.1 Ã¢ÂÂÃ¢ÂÂ") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should not render more than one failed test steps in header", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] test.step('outer 1.0', () => {
// [ts] test.step('inner 1.1', () => {
// [ts] expect.soft(1).toBe(2);
// [ts] });
// [ts] test.step('inner 1.2', () => {
// [ts] expect.soft(1).toBe(2);
// [ts] });
// [ts] });
// [ts] });
// [ts] `,
// [ts] }, { reporter: "line" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "1) a.test.ts:3:11 Ã¢ÂÂº passes Ã¢ÂÂº outer 1.0 Ã¢ÂÂÃ¢ÂÂ") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should not render more than one failed test steps in header (2)", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] test.step('outer 1.0', () => {
// [ts] test.step('inner 1.1', () => {
// [ts] expect.soft(1).toBe(2);
// [ts] });
// [ts] });
// [ts] expect.soft(1).toBe(2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "line" });
// [ts] const text = result.output;
// [ts] expect_eq(std.mem.indexOf(u8, text, "1) a.test.ts:3:11 Ã¢ÂÂº passes Ã¢ÂÂÃ¢ÂÂ") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should show error context with relative path", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] const { test, expect } = require('@playwright/test');
// [ts] test('one', () => {
// [ts] page.setContent('<div>hello</div>');
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "line" });
// [ts] const text = result.output;
// [ts] if (useIntermediateMergeReport)
// [ts] expect_eq(std.mem.indexOf(u8, text, `Error Context: ${path.join("blob-report", "resources")}`) != null, true);
// [ts] else {
// [ts] expect_eq(std.mem.indexOf(u8, text, `Error Context: ${path.join("test-results", "a-one", "error-context.md")}`) != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should not include global tag in the title", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { tag: '@global' };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('passes', () => {
// [ts] });
// [ts] `,
// [ts] }, { reporter: "line" });
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "[1/1] a.test.ts:3:11 Ã¢ÂÂº passes\n") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "@global") == null, true);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] });
// [ts] }
