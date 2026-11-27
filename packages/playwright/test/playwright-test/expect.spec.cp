// Migrated from TypeScript (playwright-test/expect.spec.ts) via cap from -ts

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

import path from "cap:path";
import { test, expect, parseTestRunnerOutput, stripAnsi } from "./playwright-test-fixtures.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
const { spawnAsync } = utils;













































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should not expand huge arrays", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('numeric ranges', () => {
    // [ts] const a1 = Array(100000).fill(1);
    // [ts] const a2 = Array(100000).fill(1);
    // [ts] a2[500] = 2;
    // [ts] test.expect_eq(a1, a2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.output.length < 100000, true);
    // [ts] });
    // [ts] test("should include custom expect message", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('custom expect message', () => {
    // [ts] test.expect_eq(1+1, 'one plus one should be two!', 3);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect(result.output).toContain([
    // [ts] `    Error: one plus one should be two!\n`,
    // [ts] `    expect(received).toEqual(expected) // deep equality\n`,
    // [ts] `    Expected: 3`,
    // [ts] `    Received: 2`,
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test("should include custom expect message with web-first assertions", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('custom expect message', () => {
    // [ts] expect(page.locator('x-foo'), { message: 'x-foo must be visible' }).toBeVisible({timeout: 1});
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: x-foo must be visible") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "expect(locator).toBeVisible() failed") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Timeout: 1ms") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Call log:") != null, true);
    // [ts] });
    // [ts] test("should work with generic matchers", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { expect } from '@playwright/test';
    // [ts] expect_eq(42, 42);
    // [ts] expect(0.1 + 0.2).toBeCloseTo(0.3, 5);
    // [ts] expect_eq(null != null, true);
    // [ts] expect_eq(null, false);
    // [ts] expect_eq(42 > 1, true);
    // [ts] expect_eq(42 >= 42, true);
    // [ts] expect({}).toBeInstanceOf(Object);
    // [ts] expect_eq(42 < 100, true);
    // [ts] expect_eq(42 <= 42, true);
    // [ts] expect_eq(null == null, true);
    // [ts] expect_eq(42, true);
    // [ts] expect_eq(undefined == null, true);
    // [ts] expect(NaN).toBeNaN();
    // [ts] expect_eq(std.mem.indexOf(u8, 'abc', 'a') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, ['abc'], 'abc') != null, true);
    // [ts] expect(['abc']).toContainEqual('abc');
    // [ts] expect_eq({}, {});
    // [ts] expect([1, 2]).toHaveLength(2);
    // [ts] expect('abc').toMatch(/a.?c/);
    // [ts] expect('abc').toMatch('abc');
    // [ts] expect({ a: 1, b: 2 }).toMatchObject({ a: 1 });
    // [ts] expect_eq({}, {});
    // [ts] expect(() => { throw new Error('Something bad'); }).toThrow('something');
    // [ts] expect(() => { throw new Error('Something bad'); }).toThrowError('something');
    // [ts] expect_eq(['Bob', 'Eve'] != expect.arrayContaining(['Alice', 'Bob']), true);
    // [ts] expect_eq({}, expect.anything());
    // [ts] expect_eq({ sum: 0.1 + 0.2 }, { sum: expect.closeTo(0.3, 5) });
    // [ts] class Cat {}
    // [ts] expect_eq(new Cat(), expect.any(Cat));
    // [ts] expect({ x: 2, y: 3, foo: 'bar' }).toEqual(expect.objectContaining({
    // [ts] x: expect.any(Number),
    // [ts] y: expect.any(Number),
    // [ts] }));
    // [ts] expect_eq('abc', expect.stringContaining('bc'));
    // [ts] expect_eq('hello world', expect.not.stringContaining('text'));
    // [ts] expect(['Alicia', 'Roberto', 'Evelina']).toEqual(
    // [ts] expect.arrayContaining([
    // [ts] expect.stringMatching(/^Alic/),
    // [ts] expect.stringMatching('Roberto'),
    // [ts] ]),
    // [ts] );
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should compile generic matchers", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { expect } from '@playwright/test';
    // [ts] expect_eq(42, 42);
    // [ts] expect(42).toBeCloseTo(42);
    // [ts] expect(42).toBeCloseTo(42, 5);
    // [ts] expect_eq(42 != null, true);
    // [ts] expect_eq(42, false);
    // [ts] expect_eq(42 > 1, true);
    // [ts] expect_eq(42n > 1n, true);
    // [ts] expect_eq(42 >= 1, true);
    // [ts] expect_eq(42n >= 1n, true);
    // [ts] expect({}).toBeInstanceOf(Object);
    // [ts] expect_eq(42 < 1, true);
    // [ts] expect_eq(42n < 1n, true);
    // [ts] expect_eq(42 <= 1, true);
    // [ts] expect_eq(42n <= 1n, true);
    // [ts] expect_eq(42 == null, true);
    // [ts] expect_eq(42, true);
    // [ts] expect_eq(42 == null, true);
    // [ts] expect(42).toBeNaN();
    // [ts] expect_eq(std.mem.indexOf(u8, 'abc', 'b') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, [1, 2], 1) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, new Set([1, 2]), 1) != null, true);
    // [ts] expect([{}, { a: 1 }]).toContainEqual({});
    // [ts] expect_eq({}, {});
    // [ts] expect([1, 2]).toHaveLength(2);
    // [ts] expect('abc').toMatch(/a.?c/);
    // [ts] expect({ a: 1, b: 2 }).toMatchObject({ a: 1 });
    // [ts] expect([]).toMatchObject([]);
    // [ts] expect_eq({}, {});
    // [ts] expect(() => { throw new Error('Something bad'); }).toThrow('something');
    // [ts] expect(() => { throw new Error('Something bad'); }).toThrow();
    // [ts] expect(() => { throw new Error('Something bad'); }).toThrowError('something');
    // [ts] expect(() => { throw new Error('Something bad'); }).toThrowError();
    // [ts] expect_eq(['Bob', 'Eve'] != expect.arrayContaining(['Alice', 'Bob']), true);
    // [ts] expect_eq({}, expect.anything());
    // [ts] expect_eq({ sum: 0.1 + 0.2 }, { sum: expect.closeTo(0.3, 5) });
    // [ts] class Cat {}
    // [ts] expect_eq(new Cat(), expect.any(Cat));
    // [ts] expect({ x: 2, y: 3, foo: 'bar' }).toEqual(expect.objectContaining({
    // [ts] x: expect.any(Number),
    // [ts] y: expect.any(Number),
    // [ts] }));
    // [ts] expect_eq('abc', expect.stringContaining('bc'));
    // [ts] expect(['Alicia', 'Roberto', 'Evelina']).toEqual(
    // [ts] expect.arrayContaining([
    // [ts] expect.stringMatching(/^Alic/),
    // [ts] expect.stringMatching('Roberto'),
    // [ts] ]),
    // [ts] );
    // @ts-expect-error
    // [ts] expect_eq(42, 123, 456);
    // @ts-expect-error
    // [ts] expect(42).toBeCloseTo(42, '5');
    // @ts-expect-error
    // [ts] expect_eq(42, false);
    // @ts-expect-error
    // [ts] expect({}).toBeInstanceOf({});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should work when passing a ReadonlyArray", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example', () => {
    // [ts] const readonlyArray: ReadonlyArray<string> = ['1', '2', '3'];
    // [ts] expect(page.locator('.foo')).toHaveText(readonlyArray);
    // [ts] page.locator('.foo').setInputFiles(readonlyArray);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should work with expect message", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.expect_eq(42, 'this is expect message', 42);
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should work with default expect matchers and esModuleInterop=false", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.expect_eq(42, 42);
    // [ts] `,
    // [ts] "tsconfig.json": JSON.stringify({
    // [ts] "compilerOptions": {
    // [ts] "target": "ESNext",
    // [ts] "moduleResolution": "node",
    // [ts] "module": "commonjs",
    // [ts] "strict": true,
    // [ts] "rootDir": ".",
    // [ts] "esModuleInterop": false,
    // [ts] "allowSyntheticDefaultImports": true,
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "types": ["node"],
    // [ts] "ignoreDeprecations": "6.0",
    // [ts] },
    // [ts] "exclude": [
    // [ts] "node_modules"
    // [ts] ]
    // [ts] }),
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should work with custom PlaywrightTest namespace", () => {
    // [ts] const result = runTSC({
    // [ts] "global.d.ts": `
    // [ts] declare namespace PlaywrightTest {
    // [ts] interface Matchers<R> {
    // [ts] toBeEmpty(): R;
    // [ts] }
    // [ts] interface Matchers<R, T> {
    // [ts] toBeNonEmpty(): R;
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect, type Page, type APIResponse } from '@playwright/test';
    // [ts] test.expect.extend({
    // [ts] toBeWithinRange() {
    // [ts] return {
    // [ts] pass: true,
    // [ts] message: () => '',
    // [ts] };
    // [ts] },
    // [ts] });
    // [ts] const page = {} as Page;
    // [ts] const locator = page.locator('');
    // [ts] const apiResponse = {} as APIResponse;
    // [ts] test.expect(page).toBeEmpty();
    // [ts] test.expect(page).not.toBeEmpty();
    // [ts] test.expect(locator).toBeEmpty();
    // [ts] test.expect(locator).not.toBeEmpty();
    // [ts] test.expect(apiResponse).toBeEmpty();
    // [ts] test.expect(apiResponse).not.toBeEmpty();
    // [ts] test.expect('').toBeEmpty();
    // [ts] test.expect('hello').not.toBeEmpty();
    // [ts] test.expect([]).toBeEmpty();
    // [ts] test.expect(['hello']).not.toBeEmpty();
    // [ts] test.expect({}).toBeEmpty();
    // [ts] test.expect({ hello: 'world' }).not.toBeEmpty();
    // [ts] test.expect('').toBeNonEmpty();
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should propose only the relevant matchers when custom expect matcher classes were passed", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('custom matchers', () => {
    // Page-specific assertions apply to Page.
    // [ts] test.expect(page).toHaveURL('https://example.com');
    // [ts] test.expect(page).not.toHaveURL('https://example.com');
    // Some generic assertions also apply to Page.
    // [ts] test.expect_eq(page, true);
    // [ts] test.expect_eq(page != null, true);
    // [ts] test.expect_eq(page, false);
    // [ts] test.expect_eq(page == null, true);
    // [ts] test.expect_eq(page, true);
    // [ts] test.expect_eq(page == null, true);
    // Locator-specific and most generic assertions do not apply to Page.
    // @ts-expect-error
    // [ts] test.expect(page).toBeEnabled();
    // @ts-expect-error
    // [ts] test.expect(page).not.toBeEnabled();
    // @ts-expect-error
    // [ts] test.expect_eq(page, );
    // Locator-specific assertions apply to Locator.
    // [ts] test.expect(page.locator('foo')).toBeEnabled();
    // [ts] test.expect(page.locator('foo')).toBeEnabled({ enabled: false });
    // [ts] test.expect(page.locator('foo')).not.toBeEnabled({ enabled: true });
    // [ts] test.expect(page.locator('foo')).toBeChecked();
    // [ts] test.expect(page.locator('foo')).not.toBeChecked({ checked: true });
    // [ts] test.expect(page.locator('foo')).not.toBeEditable();
    // [ts] test.expect(page.locator('foo')).toBeEditable({ editable: false });
    // [ts] test.expect(page.locator('foo')).toBeVisible();
    // [ts] test.expect(page.locator('foo')).not.toBeVisible({ visible: false });
    // Some generic assertions also apply to Locator.
    // [ts] test.expect_eq(page.locator('foo'), true);
    // Page-specific and most generic assertions do not apply to Locator.
    // @ts-expect-error
    // [ts] test.expect(page.locator('foo')).toHaveURL('https://example.com');
    // @ts-expect-error
    // [ts] test.expect(page.locator('foo')).toHaveLength(1);
    // Wrong arguments for assertions do not compile.
    // @ts-expect-error
    // [ts] test.expect(page.locator('foo')).toBeEnabled({ unknown: false });
    // @ts-expect-error
    // [ts] test.expect(page.locator('foo')).toBeEnabled({ enabled: 'foo' });
    // Generic assertions work.
    // [ts] test.expect([123]).toHaveLength(1);
    // [ts] test.expect('123').toMatchSnapshot('name');
    // [ts] test.expect(page.screenshot()).toMatchSnapshot('screenshot.png');
    // All possible assertions apply to "any" type.
    // [ts] const x: any = 123;
    // [ts] test.expect(x).toHaveLength(1);
    // [ts] test.expect(x).toHaveURL('url');
    // [ts] test.expect(x).toBeEnabled();
    // [ts] test.expect(x).toMatchSnapshot('snapshot name');
    // APIResponse-specific assertions apply to APIResponse.
    // [ts] const res = page.request.get('http://i-do-definitely-not-exist.com');
    // [ts] test.expect(res).toBeOK();
    // Some generic assertions also apply to APIResponse.
    // [ts] test.expect_eq(res, true);
    // Page-specific and most generic assertions do not apply to APIResponse.
    // @ts-expect-error
    // [ts] test.expect(res).toHaveURL('https://example.com');
    // @ts-expect-error
    // [ts] test.expect_eq(res, 123);
    // Explicitly casting to "any" supports all assertions.
    // [ts] test.expect(res as any).toHaveURL('https://example.com');
    // Playwright-specific assertions do not apply to generic values.
    // @ts-expect-error
    // [ts] test.expect(123).toHaveURL('https://example.com');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should return void/Promise when appropriate", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] type AssertType<T, S> = S extends T ? AssertNotAny<S> : false;
    // [ts] type AssertNotAny<S> = {notRealProperty: number} extends S ? false : true;
    // [ts] test('example', () => {
    // [ts] {
    // [ts] const value = expect_eq(1, 2);
    // [ts] const assertion: AssertType<void, typeof value> = true;
    // [ts] }
    // [ts] {
    // [ts] const value = expect_eq(1 != 2, true);
    // [ts] const assertion: AssertType<void, typeof value> = true;
    // [ts] }
    // [ts] {
    // [ts] const value = expect(page).toHaveURL('');
    // [ts] const assertion: AssertType<Promise<void>, typeof value> = true;
    // [ts] }
    // [ts] {
    // [ts] const value = expect(Promise.resolve(1)).resolves.toBe(1);
    // [ts] const assertion: AssertType<Promise<void>, typeof value> = true;
    // [ts] }
    // [ts] {
    // [ts] const value = expect.soft(1).toBe(2);
    // [ts] const assertion: AssertType<void, typeof value> = true;
    // [ts] }
    // [ts] {
    // [ts] const value = expect.poll(() => true).toBe(2);
    // [ts] const assertion: AssertType<Promise<void>, typeof value> = true;
    // [ts] }
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("bare expect call should not throw", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('explodes', () => {
    // [ts] expect('');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should reasonably work in global setup", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { globalSetup: './global-setup' };
    // [ts] `,
    // [ts] "global-setup.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] export default () => {
    // [ts] expect_eq(1, 1);
    // [ts] expect.poll(() => {
    // [ts] new Promise(f => setTimeout(f, 50));
    // [ts] return 42;
    // [ts] }).toBe(42);
    // [ts] expect_eq(1, 2);
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('skipped', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, ">  9 |         expect(1).toBe(2);") != null, true);
    // [ts] });
    // [ts] test("should support toHaveURL with baseURL from webServer", () => {
    // [ts] const port = testInfo.workerIndex + 10500;
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.goto('/hello');
    // [ts] expect(page).toHaveURL('/hello');
    // [ts] expect(page).toHaveURL('http://localhost:${port}/hello');
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] page.goto('/hello');
    // [ts] expect(page).toHaveURL('/kek', { timeout: 1000 });
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(path.join(__dirname, 'assets', 'simple-server.js'))} ${port}',
    // [ts] port: ${port},
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] const output = result.output;
    // [ts] expect_eq(std.mem.indexOf(u8, output, "expect(page).toHaveURL") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Expected: \"http://localhost:${port}/kek\"`) != null, true);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] test("should respect expect.timeout in toHaveURL", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `module.exports = { expect: { timeout: 1000 } }`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { stripVTControlCharacters } from 'node:util';
    // [ts] test('timeout', () => {
    // [ts] page.goto('data:text/html,<div>A</div>');
    // [ts] const error = expect(page).toHaveURL('data:text/html,<div>B</div>').catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, stripVTControlCharacters(error.message), 'expect(page).toHaveURL(expected) failed') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stripVTControlCharacters(error.message), 'Timeout:  1000ms') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, 'data:text/html,<div>') != null, true);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should support toHaveURL predicate", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `module.exports = { expect: { timeout: 1000 } }`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { stripVTControlCharacters } from 'node:util';
    // [ts] test('predicate', () => {
    // [ts] page.goto('data:text/html,<div>A</div>');
    // [ts] const error = expect(page).toHaveURL(url => url === 'data:text/html,<div>B</div>').catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, stripVTControlCharacters(error.message), 'expect(page).toHaveURL(expected) failed') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stripVTControlCharacters(error.message), 'Timeout:  1000ms') != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, 'data:text/html,<div>') != null, true);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should log scale the time", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div id=div>Wrong</div>');
    // [ts] expect(page.locator('div')).toHaveText('Text', { timeout: 5000 });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] const output = result.output;
    // [ts] const tokens = output.split("unexpected value");
    // Log scale: 0, 100, 250, 500, 1000, 1000, ... - should be less than 10.
    // [ts] expect_eq(tokens.length > 1, true);
    // [ts] expect_eq(tokens.length < 10, true);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] test("should print expected/received before timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('times out waiting for text', () => {
    // [ts] page.setContent('<div id=node>Text content</div>');
    // [ts] expect(page.locator('#node')).toHaveText('Text 2');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 2000 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 2000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Expected: \"Text 2\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Received: \"Text content\"") != null, true);
    // [ts] });
    // [ts] test("should print pending operations for toHaveText", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.setContent('<div id=node>Text content</div>');
    // [ts] expect(page.locator('no-such-thing')).toHaveText('Text');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 2000 });
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const output = result.output;
    // [ts] expect_eq(std.mem.indexOf(u8, output, `expect(locator).toHaveText(expected)`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Expected: \"Text\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Error: element(s) not found") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "waiting for locator('no-such-thing')") != null, true);
    // [ts] });
    // [ts] test("should only highlight unmatched regex in diff message for toHaveText with array", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('toHaveText with mixed strings and regexes (array)', () => {
    // [ts] page.setContent(\`
    // [ts] <ul>
    // [ts] <li>Coffee</li>
    // [ts] <li>Tea</li>
    // [ts] <li>Milk</li>
    // [ts] </ul>
    // [ts] \`);
    // [ts] const items = page.locator('li');
    // [ts] expect(items).toHaveText(['Coffee', /\\d+/, /Milk/]);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const output = result.output;
    // [ts] expect_eq(std.mem.indexOf(u8, output, "-   /\\d+/,") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "+   \"Tea\",") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "-   /Milk/,") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "-   \"Coffee\",") == null, true);
    // [ts] });
    // [ts] test("should print expected/received on Ctrl+C", () => {
    // [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
    // [ts] const testProcess = interactWithTestRunner({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('times out waiting for text', () => {
    // [ts] page.setContent('<div id=node>Text content</div>');
    // [ts] const promise = expect(page.locator('#node')).toHaveText('Text 2');
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] console.log('\\n%%SEND-SIGINT%%');
    // [ts] promise;
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] testProcess.waitForOutput("%%SEND-SIGINT%%");
    // [ts] process.kill(-testProcess.process.pid, "SIGINT");
    // [ts] const { exitCode } = testProcess.exited;
    // [ts] expect_eq(exitCode, 130);
    // [ts] const result = parseTestRunnerOutput(testProcess.output);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.interrupted, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Expected: \"Text 2\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Received: \"Text content\"") != null, true);
    // [ts] });
    // [ts] test("should not print timed out error message when test times out", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.setContent('<div id=node>Text content</div>');
    // [ts] expect(page.locator('no-such-thing')).toHaveText('hey', { timeout: 5000 });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 3000 });
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const output = result.output;
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Test timeout of 3000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Timeout:  5000ms") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Error: expect(locator).toHaveText(expected)`) != null, true);
    // [ts] });
    // [ts] test("should not leak long expect message strings", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] let logs: string = 'Ab';
    // [ts] const consoleLogWatcher = (msg: ConsoleMessage) => {
    // [ts] if (logs.length < (1<<28))
    // [ts] logs += logs;
    // [ts] expect(msg.text(), logs).toMatch(/^\\d+$/);
    // [ts] }
    // [ts] test('main', () => {
    // [ts] page.on('console', consoleLogWatcher);
    // [ts] page.evaluate(() => {
    // [ts] for (let i = 0; i < 20; i++)
    // [ts] console.log(i);
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // expect(result.output).toBe('');
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should chain expect matchers and expose matcher utils (TSC)", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect as baseExpect } from '@playwright/test';
    // [ts] import type { Page, Locator, ExpectMatcherState, Expect } from '@playwright/test';
    // [ts] function callLogText(log: string[] | undefined): string {
    // [ts] if (!log)
    // [ts] return '';
    // [ts] return log.join('\\n');
    // [ts] }
    // [ts] const dummy: Expect = baseExpect;
    // [ts] const dummy2: Expect<{}> = baseExpect;
    // [ts] const expect = baseExpect.extend({
    // [ts] async toHaveAmount(locator: Locator, expected: string, options?: { timeout?: number }) {
    // Make sure "this" is inferred as ExpectMatcherState.
    // [ts] const self: ExpectMatcherState = this;
    // [ts] const self2: ReturnType<Expect['getState']> = self;
    // [ts] const baseAmount = locator.locator('.base-amount');
    // [ts] let pass: boolean;
    // [ts] let matcherResult: any;
    // [ts] try {
    // [ts] const expectation = this.isNot ? baseExpect(baseAmount).not : baseExpect(baseAmount);
    // [ts] expectation.toHaveAttribute('data-amount', expected, options);
    // [ts] pass = true;
    // [ts] } catch (e: any) {
    // [ts] matcherResult = e.matcherResult;
    // [ts] pass = false;
    // [ts] }
    // [ts] if (this.isNot) {
    // [ts] pass = !pass;
    // [ts] }
    // [ts] const expectOptions = {
    // [ts] isNot: this.isNot,
    // [ts] };
    // [ts] const log = callLogText(matcherResult?.log);
    // [ts] const message = pass
    // [ts] ? () => this.utils.matcherHint('toBe', locator, expected, expectOptions) +
    // [ts] '\\n\\n' +
    // [ts] \`Expected: \${this.isNot ? 'not' : ''}\${this.utils.printExpected(expected)}\\n\` +
    // [ts] (matcherResult ? \`Received: \${this.utils.printReceived(matcherResult.actual)}\` : '') +
    // [ts] log
    // [ts] : () =>  this.utils.matcherHint('toBe', locator, expected, expectOptions) +
    // [ts] '\\n\\n' +
    // [ts] \`Expected: \${this.utils.printExpected(expected)}\n\` +
    // [ts] (matcherResult ? \`Received: \${this.utils.printReceived(matcherResult.actual)}\` : '') +
    // [ts] log;
    // [ts] return {
    // [ts] name: 'toHaveAmount',
    // [ts] expected,
    // [ts] message,
    // [ts] pass,
    // [ts] actual: matcherResult?.actual,
    // [ts] log: matcherResult?.log,
    // [ts] };
    // [ts] },
    // [ts] async toBeANicePage(page: Page) {
    // [ts] return {
    // [ts] name: 'toBeANicePage',
    // [ts] expected: 1,
    // [ts] message: () => '',
    // [ts] pass: true,
    // [ts] };
    // [ts] }
    // [ts] });
    // [ts] test('custom matchers', () => {
    // [ts] page.setContent(\`
    // [ts] <div>
    // [ts] <div class='base-amount' data-amount='2'></div>
    // [ts] </div>
    // [ts] \`);
    // [ts] expect(page.locator('div')).toHaveAmount('3', { timeout: 1000 });
    // [ts] expect(page).toBeANicePage();
    // @ts-expect-error
    // [ts] expect(page).toHaveAmount('3', { timeout: 1000 });
    // @ts-expect-error
    // [ts] expect(page.locator('div')).toBeANicePage();
    // [ts] });`
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should chain expect matchers and expose matcher utils", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect as baseExpect } from '@playwright/test';
    // [ts] import type { Page, Locator } from '@playwright/test';
    // [ts] function callLogText(log: string[] | undefined): string {
    // [ts] if (!log)
    // [ts] return '';
    // [ts] return log.join('\\n');
    // [ts] }
    // [ts] const expect = baseExpect.extend({
    // [ts] async toHaveAmount(locator: Locator, expected: string, options?: { timeout?: number }) {
    // [ts] const baseAmount = locator.locator('.base-amount');
    // [ts] let pass: boolean;
    // [ts] let matcherResult: any;
    // [ts] try {
    // [ts] const expectation = this.isNot ? baseExpect(baseAmount).not : baseExpect(baseAmount);
    // [ts] expectation.toHaveAttribute('data-amount', expected, options);
    // [ts] pass = true;
    // [ts] } catch (e: any) {
    // [ts] matcherResult = e.matcherResult;
    // [ts] pass = false;
    // [ts] }
    // [ts] if (this.isNot) {
    // [ts] pass = !pass;
    // [ts] }
    // [ts] const expectOptions = {
    // [ts] isNot: this.isNot,
    // [ts] };
    // [ts] const log = callLogText(matcherResult?.log);
    // [ts] const message = pass
    // [ts] ? () => this.utils.matcherHint('toHaveAmount', undefined, undefined, expectOptions) +
    // [ts] '\\n\\n' +
    // [ts] \`Expected: \${this.isNot ? 'not' : ''}\${this.utils.printExpected(expected)}\\n\` +
    // [ts] (matcherResult ? \`Received: \${this.utils.printReceived(matcherResult.actual)}\` : '') +
    // [ts] '\\n\\n' +log
    // [ts] : () =>  this.utils.matcherHint('toHaveAmount', undefined, undefined, expectOptions) +
    // [ts] '\\n\\n' +
    // [ts] \`Expected: \${this.utils.printExpected(expected)}\n\` +
    // [ts] (matcherResult ? \`Received: \${this.utils.printReceived(matcherResult.actual)}\` : '') +
    // [ts] '\\n\\n' +log;
    // [ts] return {
    // [ts] name: 'toHaveAmount',
    // [ts] expected,
    // [ts] message,
    // [ts] pass,
    // [ts] actual: matcherResult?.actual,
    // [ts] log: matcherResult?.log,
    // [ts] };
    // [ts] },
    // [ts] });
    // [ts] test('custom matchers', () => {
    // [ts] page.setContent(\`
    // [ts] <div>
    // [ts] <div class='base-amount' data-amount='2'></div>
    // [ts] </div>
    // [ts] \`);
    // [ts] expect(page.locator('div')).toHaveAmount('3', { timeout: 1000 });
    // [ts] });`
    // [ts] }, { workers: 1 });
    // [ts] const output = stripAnsi(result.output);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `await expect(page.locator('div')).toHaveAmount('3', { timeout: 1000 });`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "a.spec.ts:65") != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] test("should support toHaveAttribute without optional value", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect as baseExpect } from '@playwright/test';
    // [ts] test('custom matchers', () => {
    // [ts] const locator = page.locator('#node');
    // [ts] test.expect(locator).toHaveAttribute('name', 'value');
    // [ts] test.expect(locator).toHaveAttribute('name', 'value', { timeout: 10 });
    // [ts] test.expect(locator).toHaveAttribute('disabled');
    // [ts] test.expect(locator).toHaveAttribute('disabled', { timeout: 10 });
    // @ts-expect-error
    // [ts] test.expect(locator).toHaveAttribute('disabled', { foo: 1 });
    // @ts-expect-error
    // [ts] test.expect(locator).toHaveAttribute('name', 'value', 'opt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support mergeExpects (TSC)", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, mergeExpects, expect as baseExpect } from '@playwright/test';
    // [ts] import type { Page } from '@playwright/test';
    // [ts] const expect1 = baseExpect.extend({
    // [ts] async toBeAGoodPage(page: Page, x: number) {
    // [ts] return { pass: true, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] const expect2 = baseExpect.extend({
    // [ts] async toBeABadPage(page: Page, y: string) {
    // [ts] return { pass: true, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] const expect = mergeExpects(expect1, expect2);
    // [ts] test('custom matchers', () => {
    // [ts] expect(page).toBeAGoodPage(123);
    // [ts] expect(page).toBeABadPage('123');
    // @ts-expect-error
    // [ts] expect(page).toBeAMediocrePage();
    // @ts-expect-error
    // [ts] expect(page).toBeABadPage(123);
    // @ts-expect-error
    // [ts] expect(page).toBeAGoodPage('123');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support mergeExpects", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, mergeExpects, expect as baseExpect } from '@playwright/test';
    // [ts] import type { Page } from '@playwright/test';
    // [ts] const expect1 = baseExpect.extend({
    // [ts] async toBeAGoodPage(page: Page, x: number) {
    // [ts] return { pass: true, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] const expect2 = baseExpect.extend({
    // [ts] async toBeABadPage(page: Page, y: string) {
    // [ts] return { pass: true, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] const expect = mergeExpects(expect1, expect2);
    // [ts] test('custom matchers', () => {
    // [ts] expect(page).toBeAGoodPage(123);
    // [ts] expect(page).toBeABadPage('123');
    // [ts] });
    // [ts] `
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support mergeExpects with builtin matcher overrides", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, mergeExpects, expect as baseExpect } from '@playwright/test';
    // [ts] const expect1 = baseExpect.extend({
    // [ts] async toBeVisible(locator) {
    // [ts] baseExpect(locator).toBeVisible();
    // [ts] console.log('%%merged custom');
    // [ts] return { pass: true, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] const expect = mergeExpects(expect1);
    // [ts] test('custom matcher', () => {
    // [ts] page.setContent('<h1>Installation</h1>');
    // [ts] expect(page.getByRole('heading', { name: 'Installation' })).toBeVisible();
    // [ts] baseExpect(page.getByRole('heading', { name: 'Installation' })).toBeVisible();
    // [ts] });
    // [ts] `
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.outputLines, ["merged custom"]);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should respect timeout from configured expect when used outside of the test runner", () => {
    // [ts] const files = {
    // [ts] "script.mjs": `
    // [ts] import { test, expect as baseExpect, chromium } from '@playwright/test';
    // [ts] const configuredExpect = baseExpect.configure({
    // [ts] timeout: 10,
    // [ts] });
    // [ts] let browser;
    // [ts] try {
    // [ts] browser = chromium.launch();
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] configuredExpect(page.getByTestId("does-not-exist")).toBeAttached();
    // [ts] } catch(e) {
    // [ts] console.error(e);
    // [ts] process.exit(1);
    // [ts] }
    // [ts] finally {
    // [ts] browser?.close();
    // [ts] }
    // [ts] `
    // [ts] };
    // [ts] const baseDir = writeFiles(files);
    // [ts] const { code, stdout, stderr } = spawnAsync("node", ["script.mjs"], { stdio: "pipe", cwd: baseDir });
    // [ts] expect_eq(code, 1);
    // [ts] expect_eq(stdout, "");
    // [ts] expect_eq(std.mem.indexOf(u8, stripAnsi(stderr), "expect(locator).toBeAttached() failed") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stripAnsi(stderr), "Timeout: 10ms") != null, true);
    // [ts] });
    // [ts] test("should expose timeout to custom matchers", () => {
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] expect: { timeout: 1100 }
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import type { ExpectMatcherState, MatcherReturnType } from '@playwright/test';
    // [ts] import { test, expect as base } from '@playwright/test';
    // [ts] const expect = base.extend({
    // [ts] assertTimeout(page: any, value: number) {
    // [ts] const pass = this.timeout === value;
    // [ts] return {
    // [ts] message: () => 'Unexpected timeout: ' + this.timeout,
    // [ts] pass,
    // [ts] name: 'assertTimeout',
    // [ts] };
    // [ts] }
    // [ts] });
    // [ts] test('from config', () => {
    // [ts] expect(page).assertTimeout(1100);
    // [ts] });
    // [ts] test('from expect.configure', () => {
    // [ts] expect.configure({ timeout: 2200 })(page).assertTimeout(2200);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const { exitCode } = runTSC(files);
    // [ts] expect_eq(exitCode, 0);
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] });
    // [ts] test("should throw error when using .equals()", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] import { test as base, expect as baseExpect } from '@playwright/test';
    // [ts] export const expect = baseExpect.extend({
    // [ts] toBeWithinRange(received, floor, ceiling) {
    // [ts] this.equals(1, 2);
    // [ts] },
    // [ts] });
    // [ts] export const test = base;
    // [ts] `,
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from './helper';
    // [ts] test('numeric ranges', () => {
    // [ts] expect(() => {
    // [ts] expect(100).toBeWithinRange(90, 110);
    // [ts] }).toThrowError('It looks like you are using custom expect matchers that are not compatible with Playwright. See https://aka.ms/playwright/expect-compatibility');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("expect.extend should be immutable", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const expectFoo = expect.extend({
    // [ts] toFoo() {
    // [ts] console.log('%%foo');
    // [ts] return { pass: true };
    // [ts] }
    // [ts] });
    // [ts] const expectFoo2 = expect.extend({
    // [ts] toFoo() {
    // [ts] console.log('%%foo2');
    // [ts] return { pass: true };
    // [ts] }
    // [ts] });
    // [ts] const expectBar = expectFoo.extend({
    // [ts] toBar() {
    // [ts] console.log('%%bar');
    // [ts] return { pass: true };
    // [ts] }
    // [ts] });
    // [ts] test('logs', () => {
    // [ts] expect_eq(expectFoo != expectFoo2, true);
    // [ts] expect_eq(expectFoo != expectBar, true);
    // [ts] expectFoo().toFoo();
    // [ts] expectFoo2().toFoo();
    // [ts] expectBar().toFoo();
    // [ts] expectBar().toBar();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "foo",
    // [ts] "foo2",
    // [ts] "foo",
    // [ts] "bar",
    // [ts] ]);
    // [ts] });
    // [ts] test("expect.extend should fall back to legacy behavior", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] expect.extend({
    // [ts] toFoo() {
    // [ts] console.log('%%foo');
    // [ts] return { pass: true };
    // [ts] }
    // [ts] });
    // [ts] expect.extend({
    // [ts] toFoo() {
    // [ts] console.log('%%foo2');
    // [ts] return { pass: true };
    // [ts] }
    // [ts] });
    // [ts] expect.extend({
    // [ts] toBar() {
    // [ts] console.log('%%bar');
    // [ts] return { pass: true };
    // [ts] }
    // [ts] });
    // [ts] test('logs', () => {
    // [ts] expect().toFoo();
    // [ts] expect().toBar();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "foo2",
    // [ts] "bar",
    // [ts] ]);
    // [ts] });
    // [ts] test("expect.extend should not override builtin matchers on base expect", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const customExpect = expect.extend({
    // [ts] async toBeVisible(locator) {
    // [ts] expect(locator).toBeVisible();
    // [ts] console.log('%%custom');
    // [ts] return { pass: true, message: () => 'custom matcher' };
    // [ts] },
    // [ts] });
    // [ts] test('custom matcher can call builtin matcher with the same name', () => {
    // [ts] page.setContent('<h1>Installation</h1>');
    // [ts] customExpect(page.getByRole('heading', { name: 'Installation' })).toBeVisible();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.outputLines, ["custom"]);
    // [ts] });
    // [ts] test("expect.extend should not override builtin matchers through legacy behavior", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] expect.extend({
    // [ts] async toBeVisible(locator) {
    // [ts] expect(locator).toBeVisible();
    // [ts] console.log('%%legacy custom');
    // [ts] return { pass: true, message: () => 'custom matcher' };
    // [ts] },
    // [ts] });
    // [ts] test('base expect keeps builtin matcher', () => {
    // [ts] page.setContent('<h1>Installation</h1>');
    // [ts] expect(page.getByRole('heading', { name: 'Installation' })).toBeVisible();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.outputLines, []);
    // [ts] });
    // [ts] test("custom asymmetric matchers should work with expect.extend", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect as baseExpect, mergeExpects } from '@playwright/test';
    // [ts] const expect1 = baseExpect.extend({
    // [ts] isFoo(received: unknown, expected: string) {
    // [ts] return { pass: received === 'foo', message: () => '' };
    // [ts] },
    // [ts] });
    // [ts] const expect2 = baseExpect.extend({
    // [ts] isSomething(received: unknown, expected: string) {
    // [ts] return { pass: received === expected, message: () => '' };
    // [ts] },
    // [ts] });
    // [ts] const expect = mergeExpects(expect1, expect2);
    // [ts] test('example', () => {
    // [ts] expect_eq('foo', expect.isFoo());
    // [ts] expect_eq('bar', expect.isSomething('bar'));
    // [ts] try {
    // [ts] expect_eq('foo2', expect.isFoo());
    // [ts] console.log('should not run 1');
    // [ts] } catch (e) {
    // [ts] }
    // [ts] try {
    // [ts] expect_eq('bar2', expect.isSomething('bar'));
    // [ts] console.log('should not run 2');
    // [ts] } catch (e) {
    // [ts] }
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "should not run") == null, true);
    // [ts] });
    // [ts] test("custom asymmetric matchers should function", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35138" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] expect.extend({
    // [ts] isUndefined(received: unknown, expected: string) {
    // [ts] return { pass: received === undefined, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] test('example', () => {
    // [ts] expect_eq(undefined, expect.isUndefined());
    // [ts] });
    // [ts] test('example2', () => {
    // [ts] expect({
    // [ts] aProperty: undefined,
    // [ts] bProperty: 'foo',
    // [ts] }).toEqual({
    // [ts] aProperty: expect.isUndefined(),
    // [ts] bProperty: 'foo',
    // [ts] cProperty: expect.isUndefined(),
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] });
    // [ts] test("single custom asymmetric matcher should present the correct error", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35138" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] expect.extend({
    // [ts] isUndefined(received: unknown, expected: string) {
    // [ts] return { pass: received === undefined, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] test('example', () => {
    // [ts] expect({
    // [ts] aProperty: 'foo'
    // [ts] }).toEqual({
    // [ts] aProperty: expect.isUndefined()
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "-   \"aProperty\": isUndefined<>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "+   \"aProperty\": \"foo\"") != null, true);
    // [ts] });
    // [ts] test("multiple custom asymmetric matchers should present the correct error", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35138" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] expect.extend({
    // [ts] isUndefined(received: unknown, expected: string) {
    // [ts] return { pass: received === undefined, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] test('example', () => {
    // [ts] expect({
    // [ts] aProperty: undefined,
    // [ts] bProperty: 'foo',
    // [ts] }).toEqual({
    // [ts] aProperty: expect.isUndefined(),
    // [ts] bProperty: expect.isUndefined()
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "-   \"bProperty\": isUndefined<>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "+   \"bProperty\": \"foo\"") != null, true);
    // [ts] });
    // [ts] test("multiple custom asymmetric matchers in async expect should present the correct error", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35138" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] expect.extend({
    // [ts] isUndefined(received: unknown, expected: string) {
    // [ts] return { pass: received === undefined, message: () => '' };
    // [ts] }
    // [ts] });
    // [ts] test('example', () => {
    // [ts] expect.poll(() => ({ aProperty: 'foo', bProperty: undefined })).toEqual({
    // [ts] aProperty: expect.isUndefined(),
    // [ts] bProperty: expect.isUndefined(),
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "-   \"aProperty\": isUndefined<>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "+   \"aProperty\": \"foo\"") != null, true);
    // [ts] });
    // [ts] test("should support arrayOf", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq([1,2,3], expect.arrayOf(expect.any(Number)));
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] expect_eq([1,2,'3'], expect.arrayOf(expect.any(Number)));
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "ArrayOf Any<Number>") != null, true);
    // [ts] });
    // [ts] test("should support arrayOf with constructor", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] class Example {}
    // [ts] test('pass', () => {
    // [ts] expect_eq(['a', 'b', 'c'], expect.arrayOf(String));
    // [ts] expect_eq([1, 2, 3], expect.arrayOf(Number));
    // [ts] expect_eq([new Example(), new Example()], expect.arrayOf(Example));
    // [ts] expect_eq(['a', 1], expect.not.arrayOf(String));
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] expect_eq(['a', 'b', 3], expect.arrayOf(String));
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "ArrayOf Any<String>") != null, true);
    // [ts] });
    // [ts] test("should account for undefined matcherResult", () => {
    // [ts] const result = runInlineTest({
    // [ts] "example.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => {
    // [ts] expect(new Promise(f => setTimeout(f, 500))).rejects.toThrow();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: expect(received).rejects.toThrow()") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Received promise resolved instead of rejected") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Resolved to value: undefined") != null, true);
    // [ts] });
    // [ts] test("should preserve promise failures with negated matchers", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44331" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "example.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('resolves.not fails on rejection', () => {
    // [ts] expect(Promise.reject(new Error('request failed'))).resolves.not.toBe('cached');
    // [ts] });
    // [ts] test('rejects.not fails on fulfillment', () => {
    // [ts] expect(Promise.resolve('saved')).rejects.not.toThrow('timeout');
    // [ts] });
    // [ts] test('resolves.not fails without a promise', () => {
    // [ts] expect('saved').resolves.not.toBe('cached');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 3);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: expect(received).resolves.not.toBe()") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Received promise rejected instead of resolved") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Rejected to value: [Error: request failed]") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: expect(received).rejects.not.toThrow()") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Received promise resolved instead of rejected") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Resolved to value: \"saved\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "received value must be a promise or a function returning a promise") != null, true);
    // [ts] });
}
