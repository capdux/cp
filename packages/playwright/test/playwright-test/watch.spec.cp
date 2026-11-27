// Migrated from TypeScript (playwright-test/watch.spec.ts) via cap from -ts

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

import timers from "timers/promises";
import { test, expect } from "./playwright-test-fixtures.cp";

test.describe.configure({ mode: "parallel" });


























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should print dependencies in CJS mode", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44363" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] globalTeardown: './globalTeardown.ts',
    // [ts] });
    // [ts] `,
    // [ts] "helperA.ts": `export function foo() {}`,
    // [ts] "helperB.ts": `import './helperA';`,
    // [ts] "a.test.ts": `
    // [ts] import './helperA';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import './helperB';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { cc } from 'playwright/lib/common';
    // [ts] test('passes', () => {
    // [ts] expect_eq(cc.fileDependenciesForTest(), {});
    // [ts] });
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] import { cc } from 'playwright/lib/common';
    // [ts] export default () => {
    // [ts] console.log('###' + JSON.stringify(cc.fileDependenciesForTest()) + '###');
    // [ts] };
    // [ts] `
    // [ts] }, {});
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] const output = result.output;
    // [ts] const deps = JSON.parse(output.match("###(.*)###")[1]);
    // [ts] expect: (deps) => {
    // [ts] "a.test.ts": ["helperA.ts"],
    // [ts] "b.test.ts": ["helperA.ts", "helperB.ts"],
    // [ts] });
    // [ts] });
    // [ts] test("should print dependencies in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] globalTeardown: './globalTeardown.ts',
    // [ts] });
    // [ts] `,
    // [ts] "helperA.ts": `export function foo() {}`,
    // [ts] "helperB.ts": `import './helperA.js';`,
    // [ts] "a.test.ts": `
    // [ts] import './helperA.js';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import './helperB.js';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] import { cc } from 'playwright/lib/common';
    // [ts] export default () => {
    // [ts] console.log('###' + JSON.stringify(cc.fileDependenciesForTest()) + '###');
    // [ts] };
    // [ts] `
    // [ts] }, {});
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] const output = result.output;
    // [ts] const deps = JSON.parse(output.match("###(.*)###")[1]);
    // [ts] expect: (deps) => {
    // [ts] "a.test.ts": ["helperA.ts"],
    // [ts] "b.test.ts": ["helperA.ts", "helperB.ts"],
    // [ts] });
    // [ts] });
    // [ts] test("should print dependencies in mixed CJS/ESM mode 1", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] globalTeardown: './globalTeardown.ts',
    // [ts] });
    // [ts] `,
    // [ts] "helperA.cjs": `exports.foo = () => {}`,
    // [ts] "helperB.cjs": `require('./helperA');`,
    // [ts] "a.test.ts": `
    // [ts] import './helperA';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.cjs": `
    // [ts] require('./helperB');
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] import { cc } from 'playwright/lib/common';
    // [ts] export default () => {
    // [ts] console.log('###' + JSON.stringify(cc.fileDependenciesForTest()) + '###');
    // [ts] };
    // [ts] `
    // [ts] }, {});
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] const output = result.output;
    // [ts] const deps = JSON.parse(output.match("###(.*)###")[1]);
    // [ts] expect: (deps) => {
    // [ts] "a.test.ts": ["helperA.cjs"],
    // [ts] "b.test.cjs": ["helperA.cjs", "helperB.cjs"],
    // [ts] });
    // [ts] });
    // [ts] test("should print dependencies in mixed CJS/ESM mode 2", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.mts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] globalTeardown: './globalTeardown.ts',
    // [ts] });
    // [ts] `,
    // [ts] "helperA.cjs": `exports.foo = () => {}`,
    // [ts] "helperB.cts": `import './helperA';`,
    // [ts] "a.test.mts": `
    // [ts] import './helperA';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import './helperB';
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "globalTeardown.ts": `
    // [ts] import { cc } from 'playwright/lib/common';
    // [ts] export default () => {
    // [ts] console.log('###' + JSON.stringify(cc.fileDependenciesForTest()) + '###');
    // [ts] };
    // [ts] `
    // [ts] }, {});
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] const output = result.output;
    // [ts] const deps = JSON.parse(output.match("###(.*)###")[1]);
    // [ts] expect: (deps) => {
    // [ts] "a.test.mts": ["helperA.cjs"],
    // [ts] "b.test.ts": ["helperA.cjs", "helperB.cts"],
    // [ts] });
    // [ts] });
    // [ts] test("should not perform initial run", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts") == null, true);
    // [ts] });
    // [ts] test("should quit on Q", () => {
    // [ts] const testProcess = runWatchTest({});
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.write("q");
    // [ts] testProcess.exited;
    // [ts] });
    // [ts] test("should print help on H", () => {
    // [ts] const testProcess = runWatchTest({});
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.write("h");
    // [ts] testProcess.waitForOutput("to quit");
    // [ts] });
    // [ts] test("should run tests on Enter", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test #1");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should not print show-report command of HTML reporter", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({ reporter: 'html' });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "To open last HTML report run:") == null, true);
    // [ts] });
    // [ts] test("should run tests on R", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("r");
    // [ts] testProcess.waitForOutput("npx playwright test (re-running tests) #1");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should run failed tests on F", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test #1");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("b.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº fails");
    // [ts] testProcess.waitForOutput("Error: expect(received).toBe(expected)");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("f");
    // [ts] testProcess.waitForOutput("npx playwright test (running failed tests) #2");
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº fails");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7") == null, true);
    // [ts] });
    // [ts] test("should respect file filter P", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("p");
    // [ts] testProcess.waitForOutput("Input filename pattern (regex)");
    // [ts] testProcess.write("b.test\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test b.test #1");
    // [ts] testProcess.waitForOutput("b.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should respect project filter C", () => {
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({ projects: [{name: 'foo'}, {name: 'bar'}] });
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] };
    // [ts] const testProcess = runWatchTest(files, { project: "foo" });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test --project foo #1");
    // [ts] testProcess.waitForOutput("[foo] ÃÂ¢ÃÂÃÂº a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("c");
    // [ts] testProcess.waitForOutput("Select projects");
    // [ts] testProcess.waitForOutput("foo");
    // [ts] testProcess.waitForOutput("bar");
    // [ts] testProcess.write(" ");
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test --project foo #2");
    // [ts] testProcess.waitForOutput("[foo] ÃÂ¢ÃÂÃÂº a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "[bar] Ã¢ÂÂº a.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles(files); // file change triggers listTests with project filter
    // [ts] testProcess.waitForOutput("[foo] ÃÂ¢ÃÂÃÂº a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.write("c");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.waitForOutput("Select projects");
    // [ts] testProcess.waitForOutput("foo");
    // [ts] testProcess.waitForOutput("bar"); // second selection should still show all
    // [ts] });
    // [ts] test("should respect file filter P and split files", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("p");
    // [ts] testProcess.waitForOutput("Input filename pattern (regex)");
    // [ts] testProcess.write("a.test b.test\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test a.test b.test #1");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("b.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should respect title filter T", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('title 1', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('title 2', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("t");
    // [ts] testProcess.waitForOutput("Input test name pattern (regex)");
    // [ts] testProcess.write("title 2\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test --grep title 2 #1");
    // [ts] testProcess.waitForOutput("b.test.ts:3:7 ÃÂ¢ÃÂÃÂº title 2");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should re-run failed tests on F > R", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test #1");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("b.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº fails");
    // [ts] testProcess.waitForOutput("Error: expect(received).toBe(expected)");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("f");
    // [ts] testProcess.waitForOutput("npx playwright test (running failed tests) #2");
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº fails");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("r");
    // [ts] testProcess.waitForOutput("npx playwright test (re-running tests) #3");
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº fails");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7") == null, true);
    // [ts] });
    // [ts] test("should run on changed files", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "b.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("b.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "c.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] });
    // [ts] test("should run on changed deps", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import './helper';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "helper.ts": `
    // [ts] console.log('old helper');
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "helper.ts": `
    // [ts] console.log('new helper');
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("b.test.ts:4:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] testProcess.waitForOutput("new helper");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should run on changed deps in ESM", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "playwright.config.ts": `export default {};`,
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import './helper.js';
    // [ts] import { test } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "helper.ts": `
    // [ts] console.log('old helper');
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "helper.ts": `
    // [ts] console.log('new helper');
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("b.test.ts:4:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] testProcess.waitForOutput("new helper");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should re-run changed files on R", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "b.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("r");
    // [ts] testProcess.waitForOutput("c.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "a.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "b.test.ts:3:7 Ã¢ÂÂº passes") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should not trigger on changes to non-tests", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "helper.ts": `
    // [ts] console.log('helper');
    // [ts] `,
    // [ts] });
    // [ts] timers.setTimeout(1000);
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "Waiting for file changes.") == null, true);
    // [ts] });
    // [ts] test("should only watch selected projects", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({ projects: [{name: 'foo'}, {name: 'bar'}] });
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] }, null, null, { additionalArgs: ["--project=foo"] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test --project foo");
    // [ts] testProcess.waitForOutput("[foo] ÃÂ¢ÃÂÃÂº a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "[bar]") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("npx playwright test --project foo");
    // [ts] testProcess.waitForOutput("[foo] ÃÂ¢ÃÂÃÂº a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "[bar]") == null, true);
    // [ts] });
    // [ts] test("should watch filtered files", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] }, null, null, { additionalArgs: ["a.test.ts"] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test a.test.ts");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "b.test") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] timers.setTimeout(1000);
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "Waiting for file changes.") == null, true);
    // [ts] });
    // [ts] test("should not watch unfiltered files", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] }, null, null, { additionalArgs: ["a.test.ts"] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("npx playwright test a.test.ts");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "b.test") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.waitForOutput("npx playwright test a.test.ts (files changed)");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] expect_eq(std.mem.indexOf(u8, testProcess.output, "b.test") == null, true);
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] });
    // [ts] test("should run global teardown before exiting", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] globalTeardown: './global-teardown.ts',
    // [ts] };
    // [ts] `,
    // [ts] "global-teardown.ts": `
    // [ts] export default async function() {
    // [ts] console.log('running teardown');
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.write("\x1B");
    // [ts] testProcess.waitForOutput("running teardown");
    // [ts] });
    // [ts] test("should stop testrun on pressing escape", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('stalls', () => {
    // [ts] console.log('test started')
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("Waiting for file changes.");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("test started");
    // [ts] testProcess.write("\x1B");
    // [ts] testProcess.waitForOutput("1 interrupted");
    // [ts] });
    // [ts] test("buffer mode", () => {
    // [ts] const testProcess = runWatchTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes in b', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("b");
    // [ts] testProcess.waitForOutput("Waiting for file changes. Press q to quit");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes again', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("1 test file changed:");
    // [ts] testProcess.waitForOutput("a.test.ts");
    // [ts] testProcess.clearOutput();
    // [ts] writeFiles({
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes in b again', () => {});
    // [ts] `,
    // [ts] });
    // [ts] testProcess.waitForOutput("2 test files changed:");
    // [ts] testProcess.waitForOutput("a.test.ts");
    // [ts] testProcess.waitForOutput("b.test.ts");
    // [ts] testProcess.clearOutput();
    // [ts] testProcess.write("\r\n");
    // [ts] testProcess.waitForOutput("a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] testProcess.waitForOutput("b.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes");
    // [ts] });
}
