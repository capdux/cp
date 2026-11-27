// Migrated from TypeScript (playwright-test/only-changed.spec.ts) via cap from -ts

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

test.slow();








// [cap-test] test.describe("should work the same if being called in subdirectory", () => {

// [cap-test] });



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should detect untracked files", () => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] const result = runInlineTest({
    // [ts] "c.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `
    // [ts] }, { "only-changed": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "c.spec.ts") != null, true);
    // [ts] });
    // [ts] test("should detect changed files", () => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] const result = runInlineTest({
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 3); });
    // [ts] `,
    // [ts] }, { "only-changed": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "b.spec.ts") != null, true);
    // [ts] });
    // [ts] test("should diff based on base commit", () => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] writeFiles({
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 3); });
    // [ts] `,
    // [ts] });
    // [ts] git("commit -a -m update");
    // [ts] const result = runInlineTest({}, { "only-changed": `HEAD~1` });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "b.spec.ts") != null, true);
    // [ts] });
    // [ts] test("should understand dependency structure", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43824" },
    // [ts] }, (_p) => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { answer, question } from './utils';
    // [ts] test('fails', () => { expect_eq(question, answer); });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { answer, question } from './utils';
    // [ts] test('fails', () => { expect_eq(question, answer); });
    // [ts] `,
    // [ts] "c.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "utils.ts": `
    // [ts] export * from './answer';
    // [ts] export * from './question';
    // [ts] `,
    // [ts] "answer.ts": `
    // [ts] export const answer = 42;
    // [ts] `,
    // [ts] "question.ts": `
    // [ts] export const question = "???";
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] writeFiles({
    // [ts] "question.ts": `
    // [ts] export const question = "what is the answer to life the universe and everything";
    // [ts] `,
    // [ts] });
    // [ts] const result = runInlineTest({}, { "only-changed": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 2);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "a.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "b.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "c.spec.ts") == null, true);
    // [ts] writeFiles({
    // [ts] "utils.ts": `
    // [ts] export * from './answer';
    // [ts] export * from './question';
    // [ts] export const ready = import('./leaf.mjs');
    // [ts] `,
    // [ts] "leaf.mjs": `export const value = 'original';`,
    // [ts] });
    // [ts] git("add .");
    // [ts] git("commit -m mixed");
    // [ts] const mixedResult = runInlineTest({
    // [ts] "leaf.mjs": `export const value = 'changed';`,
    // [ts] }, { "only-changed": true, "list": true }, { PLAYWRIGHT_FORCE_ASYNC_LOADER: "1" });
    // [ts] expect_eq(mixedResult.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, mixedResult.output, "a.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, mixedResult.output, "b.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, mixedResult.output, "c.spec.ts") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, mixedResult.output, "Total: 2 tests in 2 files") != null, true);
    // [ts] const packageJson = `{ "type": "module" }`;
    // [ts] writeFiles({ "package.json": packageJson });
    // [ts] git("add .");
    // [ts] git("commit -m esm");
    // [ts] for (const env of [{}, { PLAYWRIGHT_FORCE_ASYNC_LOADER: "1" }]) {
    // [ts] const esmResult = runInlineTest({
    // [ts] "package.json": packageJson,
    // [ts] "question.ts": `export const question = "changed";`,
    // [ts] }, { "only-changed": true, "list": true }, env);
    // [ts] expect_eq(esmResult.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, esmResult.output, "a.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, esmResult.output, "b.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, esmResult.output, "c.spec.ts") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, esmResult.output, "Total: 2 tests in 2 files") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("watch mode is not supported", () => {
    // [ts] const testProcess = runWatchTest({}, { "only-changed": true });
    // [ts] testProcess.exited;
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "--only-changed is not supported in watch mode") != null, true);
    // [ts] });
    // [ts] test("should throw nice error message if git doesnt work", () => {
    // [ts] const result = runInlineTest({}, { "only-changed": `this-commit-does-not-exist` });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "contains our error message", "Cannot detect changed files for --only-changed mode") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "contains command", "git diff this-commit-does-not-exist --name-only") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "contains git command output", "unknown revision or path not in the working tree") != null, true);
    // [ts] });
    // [ts] test("tracked file", () => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] writeFiles({
    // [ts] "tests/c.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -a -m "add test"`);
    // [ts] const result = runInlineTest({
    // [ts] "tests/c.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 3); });
    // [ts] `
    // [ts] }, { "only-changed": true }, {}, { cwd: "tests" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "c.spec.ts") != null, true);
    // [ts] });
    // [ts] test("untracked file", () => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] const result = runInlineTest({
    // [ts] "tests/c.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 3); });
    // [ts] `
    // [ts] }, { "only-changed": true }, {}, { cwd: "tests" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "c.spec.ts") != null, true);
    // [ts] });
    // [ts] test("UI mode is not supported", () => {
    // [ts] const result = runInlineTest({}, { "only-changed": true, "ui": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "--only-changed is not supported in UI mode") != null, true);
    // [ts] });
    // [ts] test("should run project dependencies of changed tests", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/32070",
    // [ts] },
    // [ts] }, (_p) => {
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [
    // [ts] { name: 'setup', testMatch: 'setup.spec.ts', },
    // [ts] { name: 'main', dependencies: ['setup'] },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "setup.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('setup test', () => {
    // [ts] console.log('setup test is executed')
    // [ts] });
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] const result = runInlineTest({
    // [ts] "c.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `
    // [ts] }, { "only-changed": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "setup test is executed") != null, true);
    // [ts] });
    // [ts] test("should work with list mode", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43661" },
    // [ts] }, (_p) => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "cadastro-usuÃÂ¡rio.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('tracked', () => {});
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] const result = runInlineTest({
    // [ts] "cadastro-usuÃÂ¡rio.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('modified', () => {});
    // [ts] `,
    // [ts] "aÃÂ§ÃÂ£o.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('untracked', () => {});
    // [ts] `,
    // [ts] ...(process.platform == "win32" ? {} : {
    // [ts] "quoted \"aÃÂÃÂ§ÃÂÃÂ£o\"\t\n.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('quoted', () => {});
    // [ts] `,
    // [ts] }),
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `
    // [ts] }, { "only-changed": true, "list": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "b.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "a.spec.ts") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "cadastro-usuÃÂ¡rio.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "aÃÂ§ÃÂ£o.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Total: ${if (process.platform == "win32") 3 else 4} tests in ${if (process.platform == "win32") 3 else 4} files`) != null, true);
    // [ts] });
    // [ts] test("exits successfully if there are no changes", () => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] git(`add .`);
    // [ts] git(`commit -m init`);
    // [ts] const result = runInlineTest({}, { "only-changed": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
}
