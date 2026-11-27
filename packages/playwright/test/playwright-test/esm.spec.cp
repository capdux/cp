// Migrated from TypeScript (playwright-test/esm.spec.ts) via cap from -ts

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






  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });






















  expect_eq(result.exitCode, 0);
  expect_eq(result.passed, 1);
  expect(result.outputLines).toEqual([
    "global-setup import level",
    "execSync: hello from hello.js",
    "spawnSync: hello from hello.js",
    "global-setup export level",
    "execSync: hello from hello.js",
    "spawnSync: hello from hello.js",
    "fork: hello from hellofork.js",
    "inside test file",
    "execSync: hello from hello.js",
    "spawnSync: hello from hello.js",
    "inside test file",
    "execSync: hello from hello.js",
    "spawnSync: hello from hello.js",
    "inside test",
    "execSync: hello from hello.js",
    "spawnSync: hello from hello.js",
    "fork: hello from hellofork.js",
    "global-teardown import level",
    "execSync: hello from hello.js",
    "spawnSync: hello from hello.js",
    "global-teardown export level",
    "execSync: hello from hello.js",
    "spawnSync: hello from hello.js",
    "fork: hello from hellofork.js",
  ]);
// TODO(cap-migrate): refine TS statement
// [ts] });



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should load nested as esm when package.json has type module", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] import * as fs from 'fs';
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "nested/folder/a.esm.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should support import attributes", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] import packageJSON from './package.json' with { type: 'json' };
    // [ts] console.log('imported value (config): ' + packageJSON.foo);
    // [ts] export default { };
    // [ts] `,
    // [ts] "package.json": JSON.stringify({ type: "module", foo: "bar" }),
    // [ts] "a.test.ts": `
    // [ts] import config from './package.json' with { type: 'json' };
    // [ts] import configFooFromUtils from './utils.js'
    // [ts] console.log('imported value (test): ' + config.foo);
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] expect_eq(configFooFromUtils.foo, 'bar');
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "utils.js": `
    // [ts] import config from './package.json' with { type: 'json' };
    // [ts] export default config;
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.stdout, "imported value (config): bar") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.stdout, "imported value (test): bar") != null, true);
    // [ts] });
    // [ts] test("should import esm from ts when package.json has type module", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] import * as fs from 'fs';
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "a.test.ts": `
    // [ts] import { foo } from './b.ts';
    // [ts] import { bar } from './c.js';
    // [ts] import { qux } from './d.js';
    // Make sure to import a type-only Locator below to check that our babel strips it out.
    // [ts] import { test, expect, Locator } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] const locator: Locator = null!;
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] expect_eq(bar, 'bar');
    // [ts] expect_eq(qux, 'qux');
    // [ts] });
    // [ts] `,
    // [ts] "b.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "c.ts": `
    // [ts] export const bar: string = 'bar';
    // [ts] `,
    // [ts] "d.js": `
    // [ts] export const qux = 'qux';
    // [ts] `,
    // [ts] }, {});
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should propagate subprocess exit code in experimental mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('failing test', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, {});
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] test("should respect path resolver in experimental mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "playwright.config.ts": `
    // Make sure that config can use the path mapping.
    // [ts] import { foo } from 'util/b.js';
    // [ts] export default {
    // [ts] projects: [{ name: foo }],
    // [ts] };
    // [ts] `,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "target": "ES2019",
    // [ts] "module": "commonjs",
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "util/*": ["./foo/bar/util/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.test.ts": `
    // [ts] import { foo } from 'util/b.js';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] }, {});
    // [ts] test("should use source maps", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15202" });
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `
    // [ts] }, { reporter: "list" });
    // [ts] const output = result.output;
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[foo] Ã¢ÂÂº a.test.ts:4:7 Ã¢ÂÂº check project name") != null, true);
    // [ts] });
    // [ts] test("should use source maps when importing a file throws an error", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29418" });
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `
    // [ts] export default {};
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] throw new Error('Oh my!');
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect(result.output).toContain(`Error: Oh my!
    // [ts] at a.test.ts:4
    // [ts] 2 |       import { test, expect } from '@playwright/test';
    // [ts] 3 |
    // [ts] > 4 |       throw new Error('Oh my!');
    // [ts] |             ^
    // [ts] `);
    // [ts] });
    // [ts] test("should use source maps when importing a file throws an error in legacy mode", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35706" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `
    // [ts] export default {};
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] throw new Error('Oh my!');
    // [ts] `
    // [ts] }, {}, { "PLAYWRIGHT_WAIT_FOR_SOURCE_MAPS": "1" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect(result.output).toContain(`Error: Oh my!
    // [ts] at a.test.ts:4
    // [ts] 2 |       import { test, expect } from '@playwright/test';
    // [ts] 3 |
    // [ts] > 4 |       throw new Error('Oh my!');
    // [ts] |             ^
    // [ts] `);
    // [ts] });
    // [ts] test("should show the codeframe in errors", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(1, 2);
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] test('foobar', () => {
    // [ts] const error = new Error('my-message');
    // [ts] error.name = 'FooBarError';
    // [ts] throw error;
    // [ts] });
    // [ts] `
    // [ts] }, { reporter: "list" }, {
    // [ts] FORCE_COLOR: "0",
    // [ts] });
    // [ts] const output = result.output;
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 2);
    // [ts] expect(output, "error carrotÃÂ¢ÃÂÃÂvia source mapsÃÂ¢ÃÂÃÂis positioned appropriately").toContain(
    // [ts] [
    // [ts] `    > 5 |         expect(1).toBe(2);`,
    // [ts] `        |                   ^`
    // [ts] ].join("\n"));
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "FooBarError: my-message") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "at a.test.ts") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `   9 |       test('foobar', async ({}) => {`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `> 10 |         const error = new Error('my-message');`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "     |                       ^") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "  11 |         error.name = 'FooBarError';") != null, true);
    // [ts] });
    // [ts] test("should filter by line", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15200" });
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "foo/x.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => { expect_eq(1, 2); });
    // [ts] test('two', () => { expect_eq(1, 2); });
    // [ts] test('three', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "foo/y.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] }, null, null, { additionalArgs: ["x.spec.ts:4"] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect(result.output).toMatch("x\\.spec\\.ts.*two");
    // [ts] });
    // [ts] test("should resolve directory import to index.js file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils/index.js": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve directory import to index.ts file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils/index.ts": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve directory import to index.tsx file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils/index.tsx": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve directory import to index.mjs file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils/index.mjs": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve directory import to index.jsx file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils/index.jsx": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve file import before directory import in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.js": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] "playwright-utils/index.js": `
    // [ts] export function gimmeAOne() {
    // intentionally return the wrong thing because this file shouldn't be resolved.
    // [ts] return 2;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve .js import to .ts file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils.js';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.ts": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve .js import to .tsx file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils.js';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.tsx": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve .js import to .jsx file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils.js';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.jsx": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve no-extension import to .ts file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.ts": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve no-extension import to .tsx file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.tsx": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve no-extension import to .jsx file in ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.jsx": `
    // [ts] export function gimmeAOne() {
    // [ts] return 1;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should load cjs config and test in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.cjs": `
    // [ts] const fs = require('fs');
    // [ts] module.exports = { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.cjs": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] });
    // [ts] test("should allow ESM when config is cjs", () => {
    // [ts] test(nodeVersion.major < 18, "ESM loader is enabled conditionally with older API");
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.cjs": `
    // [ts] const fs = require('fs');
    // [ts] module.exports = { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should load mts without config", () => {
    // [ts] test(nodeVersion.major < 18, "ESM loader is enabled conditionally with older API");
    // [ts] const result = runInlineTest({
    // [ts] "a.test.mts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(true, true);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should load type module without config", () => {
    // [ts] test(nodeVersion.major < 18, "ESM loader is enabled conditionally with older API");
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "helper.js": `
    // [ts] const foo = 42;
    // [ts] export default foo;
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import foo from './helper.js';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should be able to use use execSync with a Node.js file inside a spec", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/24516" });
    // [ts] const result = runInlineTest({
    // [ts] "global-setup.ts": `
    // [ts] import { execSync, spawnSync, fork } from 'child_process';
    // [ts] console.log('%%global-setup import level');
    // [ts] console.log('%%execSync: ' + execSync('node hello.js').toString());
    // [ts] console.log('%%spawnSync: ' + spawnSync('node', ['hello.js']).stdout.toString());
    // [ts] export default () => {
    // [ts] console.log('%%global-setup export level');
    // [ts] console.log('%%execSync: ' + execSync('node hello.js').toString());
    // [ts] console.log('%%spawnSync: ' + spawnSync('node', ['hello.js']).stdout.toString());
    // [ts] const child = fork('hellofork.js');
    // [ts] child.on('message', (m) => console.log('%%fork: ' + m));
    // [ts] new Promise((resolve) => child.on('exit', (code) => resolve(code)));
    // [ts] }
    // [ts] `,
    // [ts] "global-teardown.ts": `
    // [ts] import { execSync, spawnSync, fork } from 'child_process';
    // [ts] console.log('%%global-teardown import level');
    // [ts] console.log('%%execSync: ' + execSync('node hello.js').toString());
    // [ts] console.log('%%spawnSync: ' + spawnSync('node', ['hello.js']).stdout.toString());
    // [ts] export default () => {
    // [ts] console.log('%%global-teardown export level');
    // [ts] console.log('%%execSync: ' + execSync('node hello.js').toString());
    // [ts] console.log('%%spawnSync: ' + spawnSync('node', ['hello.js']).stdout.toString());
    // [ts] const child = fork('hellofork.js');
    // [ts] child.on('message', (m) => console.log('%%fork: ' + m));
    // [ts] new Promise((resolve) => child.on('exit', (code) => resolve(code)));
    // [ts] }
    // [ts] `,
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "playwright.config.ts": `export default {
    // [ts] projects: [{name: 'foo'}],
    // [ts] globalSetup: './global-setup.ts',
    // [ts] globalTeardown: './global-teardown.ts',
    // [ts] };`,
    // [ts] "hello.js": `console.log('hello from hello.js');`,
    // [ts] "hellofork.js": `process.send('hello from hellofork.js');`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { execSync, spawnSync, fork } from 'child_process';
    // [ts] console.log('%%inside test file');
    // [ts] console.log('%%execSync: ' + execSync('node hello.js').toString());
    // [ts] console.log('%%spawnSync: ' + spawnSync('node', ['hello.js']).stdout.toString());
    // [ts] test('check project name', () => {
    // [ts] console.log('%%inside test');
    // [ts] console.log('%%execSync: ' + execSync('node hello.js').toString());
    // [ts] console.log('%%spawnSync: ' + spawnSync('node', ['hello.js']).stdout.toString());
    // [ts] const child = fork('hellofork.js');
    // [ts] child.on('message', (m) => console.log('%%fork: ' + m));
    // [ts] new Promise((resolve) => child.on('exit', (code) => resolve(code)));
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] test("should be able to use mergeTests/mergeExpect", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.mjs": `
    // [ts] import { test as base, expect as baseExpect, mergeTests, mergeExpects } from '@playwright/test';
    // [ts] const test = mergeTests(
    // [ts] base.extend({
    // [ts] myFixture1: '1',
    // [ts] }),
    // [ts] base.extend({
    // [ts] myFixture2: '2',
    // [ts] }),
    // [ts] );
    // [ts] const expect = mergeExpects(
    // [ts] baseExpect.extend({
    // [ts] async toBeFoo1(page, x) {
    // [ts] return { pass: true, message: () => '' };
    // [ts] }
    // [ts] }),
    // [ts] baseExpect.extend({
    // [ts] async toBeFoo2(page, x) {
    // [ts] return { pass: true, message: () => '' };
    // [ts] }
    // [ts] }),
    // [ts] );
    // [ts] test('merged', () => {
    // [ts] console.log('%%myFixture1: ' + myFixture1);
    // [ts] console.log('%%myFixture2: ' + myFixture2);
    // [ts] expect(1).toBeFoo1();
    // [ts] expect(1).toBeFoo2();
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines, "myFixture1: 1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines, "myFixture2: 2") != null, true);
    // [ts] });
    // [ts] test("should exit after merge-reports", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28699" });
    // [ts] const result = runInlineTest({
    // [ts] "merge.config.ts": `
    // [ts] export default { reporter: 'line' };
    // [ts] `,
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "nested/folder/a.esm.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] `
    // [ts] }, null, null, { additionalArgs: ["--reporter", "blob"] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const { exitCode } = mergeReports(test.info().outputPath("blob-report"), null, { additionalArgs: ["-c", "merge.config.ts"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should load esm -> cjs -> cjs require tree without stalling", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35706" },
    // [ts] }, (_p) => {
    // [ts] test.setTimeout(10000);
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "a.esm.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import * as root from './nested/root_require.js';
    // [ts] test('check project name', () => {
    // [ts] console.log('root', root);
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] "nested/package.json": JSON.stringify({ type: "commonjs" }),
    // [ts] "nested/root_require.js": `
    // [ts] const nested = require('./nested_require.js');
    // [ts] console.log(nested);
    // [ts] `,
    // [ts] "nested/nested_require.js": `
    // [ts] console.log('nested require');
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should have source maps when throwing during nested module import", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35706" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { foo } from './import1.ts';
    // [ts] test('dummy', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] "import1.ts": `
    // [ts] export { foo } from './import2.ts';
    // [ts] `,
    // [ts] "import2.ts": `
    // [ts] type Foo = { bar: string };
    // [ts] const fs = require('fs');
    // [ts] throw new Error('Oh no!');
    // [ts] export const foo: Foo = { bar: 'foo' };
    // [ts] console.log(fs.existsSync('does-not-exist.txt'));
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `require is not defined in ES module scope`) != null, true);
    // [ts] expect(result.output).toContain([
    // [ts] `  2 |       type Foo = { bar: string };`,
    // [ts] `> 3 |       const fs = require('fs');`,
    // [ts] `    |                  ^`,
    // [ts] ].join("\n"));
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `at import2.ts:3`) != null, true);
    // [ts] });
}
