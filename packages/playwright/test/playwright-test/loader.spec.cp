// Migrated from TypeScript (playwright-test/loader.spec.ts) via cap from -ts

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
import fs from "cap:fs";
import path from "cap:path";
import url from "cap:url";

























fn jsxRuntimeFiles(names: []string) {
    // TODO(cap-migrate): refine TS body
    // [ts] return Object.assign({ "package.json": JSON.stringify({ name: "test-project" }) }, ...names.map(name => ({ [`node_modules/${name}/package.json`]: JSON.stringify({ name, exports: { "./jsx-runtime": "./jsx-runtime.js", "./jsx-dev-runtime": "./jsx-dev-runtime.js", }, }), [`node_modules/${name}/jsx-runtime.js`]: ` exports.jsx = (type, props) => ({ runtime: '${name}', type, props }); exports.jsxs = exports.jsx; exports.Fragment = '${name}-fragment'; `, [`node_modules/${name}/jsx-dev-runtime.js`]: ` exports.jsxDEV = (type, props, key, isStatic, source) => ({ runtime: '${name}-dev', type, props, fileName: source.fileName }); exports.Fragment = '${name}-fragment'; `, })));
}





































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should return the location of a syntax error", () => {
    // [ts] const result = runInlineTest({
    // [ts] "error.spec.js": `
    // [ts] const x = {
    // [ts] foo: 'bar';
    // [ts] };
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "error.spec.js") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `> 3 |         foo: 'bar';\n    |                   ^`) != null, true);
    // [ts] });
    // [ts] test("should return the location of a syntax error with deep stack", () => {
    // [ts] const result = runInlineTest({
    // [ts] "error.ts": `
    // [ts] const x = {
    // [ts] foo: 'bar';
    // [ts] };
    // [ts] `,
    // [ts] "qux.ts": `
    // [ts] import { error } from './error';
    // [ts] export function qux() { error() }
    // [ts] `,
    // [ts] "baz.ts": `
    // [ts] import { qux } from './qux';
    // [ts] export function baz() { qux() }
    // [ts] `,
    // [ts] "bar.ts": `
    // [ts] import { baz } from './baz';
    // [ts] export function bar() { baz() }
    // [ts] `,
    // [ts] "foo.ts": `
    // [ts] import { bar } from './bar';
    // [ts] export function foo() { bar() }
    // [ts] `,
    // [ts] "test.spec.ts": `
    // [ts] import { foo } from './foo';
    // [ts] foo();
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "at error.ts:3") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "qux.ts:2:7") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "baz.ts:2:7") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "bar.ts:2:7") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "foo.ts:2:7") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test.spec.ts:2:7") != null, true);
    // [ts] });
    // [ts] test("should print an improper error", () => {
    // [ts] const result = runInlineTest({
    // [ts] "error.spec.js": `
    // [ts] throw 123;
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "123") != null, true);
    // [ts] });
    // [ts] test("should spread AggregateError sub-errors", () => {
    // [ts] const result = runInlineTest({
    // [ts] "error.spec.js": `
    // [ts] throw new AggregateError([
    // [ts] new AggregateError([new Error('a'), new Error('b')], 'inner'),
    // [ts] new Error('c'),
    // [ts] ], 'outer');
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: a") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: b") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: c") != null, true);
    // [ts] });
    // [ts] test("should print a null error", () => {
    // [ts] const result = runInlineTest({
    // [ts] "error.spec.js": `
    // [ts] throw null;
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "null") != null, true);
    // [ts] });
    // [ts] test("should return the location of a syntax error in typescript", () => {
    // [ts] const result = runInlineTest({
    // [ts] "error.spec.ts": `
    // [ts] const x = {
    // [ts] foo: 'bar';
    // [ts] };
    // [ts] `
    // [ts] }, {}, {
    // [ts] FORCE_COLOR: "0"
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "error.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `'bar';`) != null, true);
    // [ts] });
    // [ts] test("should allow export default form the config file", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { timeout: 1000 };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => {
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 1000ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("should validate configuration object", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { timeout: '1000' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('works', () => {});
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "playwright.config.ts: config.timeout must be a non-negative number") != null, true);
    // [ts] });
    // [ts] test("should match tests well", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('works', () => {});
    // [ts] `,
    // [ts] "hello.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('works', () => {});
    // [ts] `,
    // [ts] "strange.....spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('works', () => {});
    // [ts] `,
    // [ts] "badspec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('bad', () => { throw new Error('badspec.ts')});
    // [ts] `,
    // [ts] "specspec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('bad', () => { throw new Error('specspec.ts')});
    // [ts] `,
    // [ts] "a.testtest.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('bad', () => { throw new Error('a.testtest.ts')});
    // [ts] `,
    // [ts] "b.testspec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('bad', () => { throw new Error('b.testspec.ts')});
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 3);
    // [ts] });
    // [ts] test("should load an mjs file", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] "a.spec.mjs": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(passed, 1);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should allow using import", () => {
    // [ts] const { exitCode } = runInlineTest({
    // [ts] "a.spec.js": `
    // [ts] import fs from 'fs';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should load esm when package.json has type module", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] import * as fs from 'fs';
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "a.esm.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should load mjs config file", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.mjs": `
    // [ts] import * as fs from 'fs';
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should load mts config file", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.mts": `
    // [ts] import * as fs from 'fs';
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should load ts from esm when package.json has type module", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] import * as fs from 'fs';
    // [ts] export default { projects: [{name: 'foo'}] };
    // [ts] `,
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { bar } from './bar.js';
    // [ts] test('check project name', () => {
    // [ts] expect_eq(testInfo.project.name, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] "bar.ts": `
    // [ts] import { foo } from './foo.js';
    // [ts] export const bar = foo;
    // [ts] `,
    // [ts] "foo.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `is an experimental feature`) == null, true);
    // [ts] });
    // [ts] test("should filter stack trace for simple expect", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('should work', () => {
    // [ts] test.expect_eq(1+1, 3);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-test`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-core`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "internal") == null, true);
    // [ts] });
    // [ts] test("should filter stack trace for web-first assertions", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('should work', () => {
    // [ts] expect(page.locator('x-foo'), 'x-foo must be visible').toBeVisible({timeout: 1});
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-test`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-core`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "internal") == null, true);
    // [ts] });
    // [ts] test("should filter out event emitter from stack traces", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const EventEmitter = require('events');
    // [ts] test('should work', () => {
    // [ts] const emitter = new EventEmitter();
    // [ts] emitter.on('event', function handle() { expect_eq(1, 2); });
    // [ts] emitter.emit('event');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const outputWithoutGoodStackFrames = result.output.split("\n").filter(line => !line.includes(testInfo.outputPath())).join("\n");
    // [ts] expect_eq(std.mem.indexOf(u8, outputWithoutGoodStackFrames, "EventEmitter.emit") == null, true);
    // [ts] });
    // [ts] test("should filter out syntax error stack traces", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('should work', () => {
    // syntax error: cannot have await in non-async function
    // [ts] Promise.resolve();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "babel") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "    at ") == null, true);
    // [ts] });
    // [ts] test("should filter stack trace for raw errors", () => {
    // [ts] const result = runInlineTest({
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('should work', () => {
    // [ts] throw new Error('foobar!');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "foobar!") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-test`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-core`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "internal") == null, true);
    // [ts] });
    // [ts] test("should not filter out POM", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] export function foo() {
    // [ts] throw new Error('foo');
    // [ts] }
    // [ts] `,
    // [ts] "expect-test.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const { foo } = require('./helper');
    // [ts] test('should work', () => {
    // [ts] foo();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "foo") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "helper.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "expect-test.spec.ts") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-test`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, path.sep + `playwright-core`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "internal") == null, true);
    // [ts] });
    // [ts] test("should work with cross-imports - 1", () => {
    // [ts] const result = runInlineTest({
    // [ts] "test1.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] new Promise(x => setTimeout(x, 500));
    // [ts] console.log('running TEST-1');
    // [ts] });
    // [ts] `,
    // [ts] "test2.spec.ts": `
    // [ts] import * as _ from './test1.spec';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 2', () => {
    // [ts] new Promise(x => setTimeout(x, 500));
    // [ts] console.log('running TEST-2');
    // [ts] });
    // [ts] `
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "TEST-1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "TEST-2") != null, true);
    // [ts] });
    // [ts] test("should work with cross-imports - 2", () => {
    // [ts] const result = runInlineTest({
    // [ts] "test1.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import * as _ from './test2.spec';
    // [ts] test('test 1', () => {
    // [ts] new Promise(x => setTimeout(x, 500));
    // [ts] console.log('running TEST-1');
    // [ts] });
    // [ts] `,
    // [ts] "test2.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 2', () => {
    // [ts] new Promise(x => setTimeout(x, 500));
    // [ts] console.log('running TEST-2');
    // [ts] });
    // [ts] `
    // [ts] }, { workers: 2, reporter: "list" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "TEST-1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "TEST-2") != null, true);
    // [ts] });
    // [ts] test("should load web server w/o esm loader in esm module", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] webServer: {
    // [ts] command: 'node ws.js',
    // [ts] port: 9876,
    // [ts] timeout: 5000,
    // [ts] },
    // [ts] projects: [{name: 'foo'}]
    // [ts] }`,
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "ws.js": `
    // [ts] console.log('NODE_OPTIONS ' + process.env.NODE_OPTIONS);
    // [ts] setTimeout(() => {}, 100000);
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] `
    // [ts] }, {}, { DEBUG: "pw:webserver" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "NODE_OPTIONS undefined") != null, true);
    // [ts] });
    // [ts] test("should load a jsx/tsx files", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("react"),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const component = () => <div></div>;
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.jsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const component = () => <div></div>;
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(passed, 2);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should load a jsx/tsx files in ESM mode", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("react"),
    // [ts] "package.json": JSON.stringify({
    // [ts] type: "module"
    // [ts] }),
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({ projects: [{name: 'foo'}] });
    // [ts] `,
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const component = () => <div></div>;
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.jsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const component = () => <div></div>;
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(passed, 2);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should use react/jsx-runtime when tsconfig does not configure jsx", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("react"),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect(<div>Hello <span>world</span><>!</></div>).toEqual({
    // [ts] runtime: 'react',
    // [ts] type: 'div',
    // [ts] props: {
    // [ts] children: [
    // [ts] 'Hello ',
    // [ts] { runtime: 'react', type: 'span', props: { children: 'world' } },
    // [ts] { runtime: 'react', type: 'react-fragment', props: { children: '!' } },
    // [ts] ]
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(passed, 1);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should respect jsxImportSource from tsconfig", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("my-jsx"),
    // [ts] "tsconfig.json": JSON.stringify({ compilerOptions: { jsx: "react-jsx", jsxImportSource: "my-jsx" } }),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect(<div><>!</></div>).toEqual({
    // [ts] runtime: 'my-jsx',
    // [ts] type: 'div',
    // [ts] props: { children: { runtime: 'my-jsx', type: 'my-jsx-fragment', props: { children: '!' } } },
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.jsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(<span />, { runtime: 'my-jsx', type: 'span', props: {} });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(passed, 2);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should respect jsxImportSource from tsconfig in ESM mode", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("my-jsx"),
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "playwright.config.ts": `export default {};`,
    // [ts] "tsconfig.json": JSON.stringify({ compilerOptions: { jsx: "react-jsx", jsxImportSource: "my-jsx" } }),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(<div />, { runtime: 'my-jsx', type: 'div', props: {} });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(passed, 1);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should respect classic jsx factory from tsconfig", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] "tsconfig.json": JSON.stringify({ compilerOptions: { jsx: "react", jsxFactory: "h", jsxFragmentFactory: "Frag" } }),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const h = (type, props, ...children) => ({ factory: 'h', type, props, children });
    // [ts] const Frag = 'frag';
    // [ts] test('succeeds', () => {
    // [ts] expect(<div id="a">Hello <>world</></div>).toEqual({
    // [ts] factory: 'h',
    // [ts] type: 'div',
    // [ts] props: { id: 'a' },
    // [ts] children: ['Hello ', { factory: 'h', type: 'frag', props: null, children: ['world'] }],
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(passed, 1);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should respect react-jsxdev from tsconfig", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("my-jsx"),
    // [ts] "tsconfig.json": JSON.stringify({ compilerOptions: { jsx: "react-jsxdev", jsxImportSource: "my-jsx" } }),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(<div />, { runtime: 'my-jsx-dev', type: 'div', props: {}, fileName: expect.stringContaining('a.spec.tsx') });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(passed, 1);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should respect jsx options from tsconfig references and extends", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("my-jsx"),
    // [ts] "tsconfig.json": JSON.stringify({ files: [], references: [{ path: "./tsconfig.node.json" }, { path: "./tsconfig.app.json" }] }),
    // [ts] "tsconfig.node.json": JSON.stringify({ compilerOptions: { strict: true } }),
    // [ts] "tsconfig.app.json": JSON.stringify({ extends: "./tsconfig.base.json", compilerOptions: { jsxImportSource: "my-jsx" } }),
    // [ts] "tsconfig.base.json": JSON.stringify({ compilerOptions: { jsx: "react-jsx", jsxImportSource: "other-jsx" } }),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(<div />, { runtime: 'my-jsx', type: 'div', props: {} });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(passed, 1);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should recompile when tsconfig jsx options change", () => {
    // [ts] const files = {
    // [ts] ...jsxRuntimeFiles("jsx-a", "jsx-b"),
    // [ts] "a.spec.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] console.log('%%' + (<div />).runtime);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result1 = runInlineTest({
    // [ts] ...files,
    // [ts] "tsconfig.json": JSON.stringify({ compilerOptions: { jsx: "react-jsx", jsxImportSource: "jsx-a" } }),
    // [ts] });
    // [ts] expect_eq(result1.exitCode, 0);
    // [ts] expect_eq(result1.outputLines, ["jsx-a"]);
    // [ts] const result2 = runInlineTest({
    // [ts] ...files,
    // [ts] "tsconfig.json": JSON.stringify({ compilerOptions: { jsx: "react-jsx", jsxImportSource: "jsx-b" } }),
    // [ts] });
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] expect_eq(result2.outputLines, ["jsx-b"]);
    // [ts] });
    // [ts] test("should load a jsx/tsx files with fragments", () => {
    // [ts] const { exitCode, passed } = runInlineTest({
    // [ts] ...jsxRuntimeFiles("react"),
    // [ts] "helper.tsx": `
    // [ts] export const component = () => <><div></div></>;
    // [ts] export function add(a: number, b: number) {
    // [ts] return a + b;
    // [ts] }
    // [ts] `,
    // [ts] "helper2.jsx": `
    // [ts] const component = () => <><div></div></>;
    // [ts] function add(a, b) {
    // [ts] return a + b;
    // [ts] }
    // [ts] module.exports = { add, component }
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { add } from './helper';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(add(1, 1), 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.js": `
    // [ts] const { add } = require('./helper2');
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('succeeds', () => {
    // [ts] expect_eq(add(1, 1), 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(passed, 2);
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("should remove type imports from ts", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { Point } from 'helper';
    // [ts] const p: Point = {};
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "node_modules/helper/index.d.ts": `
    // [ts] export type Point = {};
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve directory import to index.js file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve directory import to index.ts file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve directory import to index.tsx file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve directory import to index.mjs file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve directory import to index.jsx file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve file import before directory import in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] "playwright-utils/index.jsx": `
    // [ts] export function gimmeAOne() {
    // intentionally return the wrong thing because this file shouldn't be resolved.
    // [ts] return 2;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve .js import to .ts file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve .js import to .tsx file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should import export assignment from ts", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import number from './utils.js';
    // [ts] test('pass', () => {
    // [ts] expect_eq(number, 1);
    // [ts] });
    // [ts] `,
    // [ts] "utils.ts": `
    // [ts] export = 1;
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve no-extension import to .ts file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve no-extension import to .tsx file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should resolve no-extension import to .jsx file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
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
    // [ts] test("should not resolve .mjs import to .ts file in non-ESM mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils.mjs';
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
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Cannot find module './playwright-utils.mjs'`) != null, true);
    // [ts] });
    // [ts] test("should resolve absolute .js import to .ts file", () => {
    // [ts] const filePath = test.info().outputPath("playwright-utils.js");
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from ${JSON.stringify(filePath)};
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
    // [ts] test("should resolve no-extension import of module into .ts file", () => {
    // [ts] const result = runInlineTest({
    // [ts] "node_modules/playwright-utils/index.js": `
    // [ts] exports.foo = 42;
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { gimmeAOne } from './playwright-utils';
    // [ts] test('pass', () => {
    // [ts] expect_eq(gimmeAOne(), 1);
    // [ts] });
    // [ts] `,
    // [ts] "playwright-utils.ts": `
    // [ts] import { foo } from 'playwright-utils';
    // [ts] export function gimmeAOne() {
    // [ts] return foo - 41;
    // [ts] }
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve extensionless .ts subpath import across a workspace symlink in ESM", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41371" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const baseDir = testInfo.outputPath();
    // [ts] const symlinkType = if (process.platform == "win32") "junction" else "dir";
    // [ts] const link = (target, linkPath) => {
    // [ts] fs.promises.mkdir(path.dirname(linkPath), { recursive: true });
    // [ts] fs.promises.symlink(path.join(baseDir, target), linkPath, symlinkType);
    // [ts] };
    // It is important to symlink so that our belongsToNodeModules() check does not trigger.
    // [ts] link("packages/shared", path.join(baseDir, "packages/core/node_modules/@repro/shared"));
    // [ts] link("packages/core", path.join(baseDir, "apps/e2e/node_modules/@repro/core"));
    // [ts] const result = runInlineTest({
    // Root package.json is required for workspace:* dependencies to work.
    // [ts] "package.json": JSON.stringify({ name: "repro-root", private: true }),
    // [ts] "packages/shared/package.json": JSON.stringify({ name: "@repro/shared", private: true, type: "module" }),
    // [ts] "packages/shared/lib/text.utils.ts": `
    // [ts] export function greet(name: string) {
    // [ts] return 'Hello, ' + name;
    // [ts] }
    // [ts] `,
    // [ts] "packages/core/package.json": JSON.stringify({ name: "@repro/core", private: true, type: "module", dependencies: { "@repro/shared": "workspace:*" } }),
    // [ts] "packages/core/lib/conversations.ts": `
    // [ts] export { greet } from '@repro/shared/lib/text.utils';
    // [ts] `,
    // [ts] "apps/e2e/tests/basic.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { greet } from '@repro/core/lib/conversations';
    // [ts] test('greet returns expected string', () => {
    // [ts] expect_eq(greet('world'), 'Hello, world');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support node imports", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": "export default {}",
    // [ts] "package.json": JSON.stringify({
    // [ts] type: "module"
    // [ts] }),
    // [ts] "test.json": "test data",
    // [ts] "utils.mjs": `
    // [ts] import fs from "node:fs/promises";
    // [ts] export async function utilityModuleThatImportsNodeModule() {
    // [ts] return fs.readFile('test.json', 'utf8');
    // [ts] }
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { utilityModuleThatImportsNodeModule } from './utils.mjs';
    // [ts] test('pass', () => {
    // [ts] expect_eq(utilityModuleThatImportsNodeModule(), 'test data');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should complain when one test file imports another", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { foo } from './b.test';
    // [ts] test('pass1', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] export const foo = 'foo';
    // [ts] test('pass2', () => {
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `test file "a.test.ts" should not import test file "b.test.ts"`) != null, true);
    // [ts] });
    // [ts] test("should support dynamic imports and requires of js, ts from js, ts and cjs", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "helper2.ts": `
    // [ts] export const bar = 'bar';
    // [ts] `,
    // [ts] "helper3.js": `
    // [ts] module.exports.baz = 'baz';
    // [ts] `,
    // [ts] "helper4.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "passthrough.cjs": `
    // [ts] module.exports.load = () => import('./helper2');
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] const { foo } = import('./helper');
    // [ts] expect_eq(foo, 'foo');
    // [ts] const { baz } = import('./helper3');
    // [ts] expect_eq(baz, 'baz');
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { load } from './passthrough.cjs';
    // [ts] test('pass', () => {
    // [ts] const { foo } = import('./helper');
    // [ts] expect_eq(foo, 'foo');
    // [ts] const { bar } = load();
    // [ts] expect_eq(bar, 'bar');
    // [ts] });
    // [ts] `,
    // [ts] "c.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] const { foo } = import('./helper');
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] "d.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] const { foo } = require('./helper4');
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.passed, 4);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should support dynamic imports of esm-only packages", () => {
    // [ts] const result = runInlineTest({
    // [ts] "node_modules/foo-pkg/package.json": `
    // [ts] {
    // [ts] "name": "foo-pkg",
    // [ts] "type": "module",
    // [ts] "exports": { "default": "./index.js" }
    // [ts] }
    // [ts] `,
    // [ts] "node_modules/foo-pkg/index.js": `
    // [ts] export const foo = 'bar';
    // [ts] `,
    // [ts] "package.json": `{ "name": "test-project" }`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] const { foo } = import('foo-pkg');
    // [ts] expect_eq(foo, 'bar');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should allow test.extend.ts and test.ts files", () => {
    // [ts] const result = runInlineTest({
    // [ts] "test.extend.ts": `
    // [ts] export { test, expect } from '@playwright/test';
    // [ts] `,
    // [ts] "test.ts": `
    // [ts] export const helper = 42;
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from './test.extend';
    // [ts] import { helper } from './test';
    // [ts] test('pass1', () => {
    // [ts] expect_eq(helper, 42);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should load css imports as empty modules", () => {
    // [ts] const result = runInlineTest({
    // [ts] "index.css": `
    // [ts] body { color: red; }
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import './index.css';
    // [ts] import foo from './index.css';
    // [ts] import { bar } from './index.css';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should load css imports as empty modules in esm", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43295" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "index.css": `
    // [ts] body { color: red; }
    // [ts] `,
    // Extensions Playwright does not transform are left to the loaders registered in the config.
    // [ts] "playwright.config.ts": `
    // [ts] import { register } from 'node:module';
    // [ts] register('./txt-loader.mjs', import.meta.url);
    // [ts] export default {};
    // [ts] `,
    // [ts] "txt-loader.mjs": `
    // [ts] export async function load(url, context, nextLoad) {
    // [ts] if (!url.endsWith('.txt'))
    // [ts] return nextLoad(url, context);
    // [ts] return { format: 'module', source: 'export default "hello"', shortCircuit: true };
    // [ts] }
    // [ts] `,
    // [ts] "hello.txt": `hello`,
    // [ts] "a.test.ts": `
    // [ts] import './index.css';
    // [ts] import styles from './index.css';
    // [ts] import text from './hello.txt';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(styles, {});
    // [ts] expect_eq(text, 'hello');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should dynamically import re-exported cjs namespace", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35812" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "node_modules/helper/package.json": `
    // [ts] {
    // [ts] "type": "module",
    // [ts] "main": "./index.cjs",
    // [ts] "exports": {
    // [ts] ".": {
    // [ts] "import": "./index.js",
    // [ts] "require": "./index.cjs",
    // [ts] "default": "./index.js"
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "node_modules/helper/index.js": `
    // [ts] import * as mod from './index.cjs'
    // [ts] const { $ } = mod;
    // [ts] export { $ };
    // [ts] `,
    // [ts] "node_modules/helper/modules.cjs": `
    // [ts] "use strict";
    // [ts] function __toCommonJS(props) {
    // [ts] const result = {};
    // [ts] Object.defineProperty(result, '__esModule', { value: true });
    // [ts] for (const prop of Object.getOwnPropertyNames(props))
    // [ts] Object.defineProperty(result, prop, { enumerable: true, get: props[prop] });
    // [ts] return result;
    // [ts] }
    // [ts] function __reExport(to, from) {
    // [ts] for (const prop of Object.getOwnPropertyNames(from)) {
    // [ts] if (prop !== 'default' && !to.hasOwnProperty(prop))
    // [ts] Object.defineProperty(to, prop, { get: () => from[prop], enumerable: true });
    // [ts] }
    // [ts] }
    // [ts] module.exports = { __toCommonJS, __reExport };
    // [ts] `,
    // [ts] "node_modules/helper/index.cjs": `
    // [ts] "use strict";
    // [ts] const { __toCommonJS, __reExport } = require('./modules.cjs');
    // [ts] module.exports = __toCommonJS({});
    // [ts] __reExport(module.exports, require('./inner.cjs'));
    // Annotate the CommonJS export names for ESM import in node:
    // [ts] 0 && (module.exports = { ...require('./inner.cjs') });
    // [ts] `,
    // [ts] "node_modules/helper/inner.cjs": `
    // [ts] "use strict";
    // [ts] const { __toCommonJS, __reExport } = require('./modules.cjs');
    // [ts] const $ = '$ value';
    // [ts] module.exports = __toCommonJS({ $: () => $ });
    // Annotate the CommonJS export names for ESM import in node:
    // [ts] 0 && (module.exports = { $ });
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "type": "commonjs" }
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] const { test, expect } = require('@playwright/test');
    // [ts] test('pass', () => {
    // [ts] const $ = (import('helper')).$;
    // [ts] expect_eq($, '$ value');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should compose with a custom ESM loader before playwright", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39172" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "custom-loader.mjs": `
    // [ts] export async function resolve(specifier, context, nextResolve) {
    // [ts] console.log('%% resolve ' + specifier);
    // [ts] return nextResolve(specifier, context);
    // [ts] }
    // [ts] export async function load(url, context, nextLoad) {
    // [ts] const result = nextLoad(url, context);
    // [ts] console.log('%% load ' + url + ' source=' + result.source);
    // [ts] return result;
    // [ts] }
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] import { register } from 'node:module';
    // [ts] register('./custom-loader.mjs', import.meta.url);
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] projects: [{ name: 'foo' }],
    // [ts] build: { external: ['*'] },
    // [ts] });
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, {}, { PLAYWRIGHT_FORCE_ASYNC_LOADER: "1" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const testFile = url.pathToFileURL(testInfo.outputPath("a.test.js")).toString();
    // [ts] const expectedSequence = [
    // [ts] `resolve ${testFile}.esm.preflight`,
    // no load for preflight, it falls through to the node default loader which cannot resolve it and thus errors out
    // [ts] `resolve ${testFile}`,
    // [ts] `load ${testFile} source=`,
    // [ts] `resolve @playwright/test`,
    // [ts] ];
    // [ts] expect(result.outputLines).toEqual([
    // [ts] ...expectedSequence, // test collection
    // [ts] ...expectedSequence, // worker
    // [ts] ]);
    // [ts] });
    // [ts] test("should compose with a custom ESM loader after playwright", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39172" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "custom-loader.mjs": `
    // [ts] import fs from 'node:fs';
    // [ts] import { fileURLToPath } from 'url';
    // [ts] const outputDir = ${JSON.stringify(url.pathToFileURL(testInfo.outputDir).toString())};
    // [ts] export async function resolve(specifier, context, nextResolve) {
    // [ts] if (specifier.startsWith(outputDir)) {
    // [ts] console.log('%% resolve ' + specifier);
    // [ts] fs.readFileSync(fileURLToPath(specifier)); // throw if file does not exist
    // [ts] }
    // [ts] return nextResolve(specifier, context);
    // [ts] }
    // [ts] export async function load(url, context, nextLoad) {
    // [ts] const result = nextLoad(url, context);
    // [ts] if (url.startsWith(outputDir)) {
    // [ts] console.log('%% load ' + url + ' source=' + result.source);
    // [ts] fs.readFileSync(fileURLToPath(url)); // throw if file does not exist
    // [ts] }
    // [ts] return result;
    // [ts] }
    // [ts] `,
    // [ts] "register-loader.mjs": `
    // [ts] import { register } from 'node:module';
    // [ts] register('./custom-loader.mjs', import.meta.url);
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] projects: [{ name: 'foo' }],
    // [ts] build: { external: ['*'] },
    // [ts] });
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, {}, {
    // [ts] NODE_OPTIONS: `--import ${url.pathToFileURL(testInfo.outputPath("register-loader.mjs")).toString()}`,
    // [ts] PLAYWRIGHT_FORCE_ASYNC_LOADER: "1",
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const outputDir = url.pathToFileURL(testInfo.outputDir).toString();
    // [ts] const expectedSequence = [
    // [ts] `resolve ${outputDir}/playwright.config.ts`,
    // [ts] `resolve ${outputDir}/playwright.config.ts`,
    // [ts] `resolve ${outputDir}/a.test.js`,
    // [ts] `resolve ${outputDir}/a.test.js`,
    // [ts] `load ${outputDir}/a.test.js source=`,
    // [ts] ];
    // [ts] expect(result.outputLines).toEqual([
    // [ts] ...expectedSequence, // test collection
    // [ts] ...expectedSequence, // worker
    // [ts] ]);
    // [ts] });
    // [ts] test("preflight should survive faulty ESM loader ahead of playwright", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39172" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ type: "module" }),
    // [ts] "custom-loader.mjs": `
    // [ts] export async function resolve(specifier, context, nextResolve) {
    // [ts] if (specifier.endsWith('.esm.preflight'))
    // [ts] throw new Error('could not find file, what the heck is preflight?');
    // [ts] return nextResolve(specifier, context);
    // [ts] }
    // [ts] export async function load(url, context, nextLoad) {
    // [ts] return nextLoad(url, context);
    // [ts] }
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] import { register } from 'node:module';
    // [ts] register('./custom-loader.mjs', import.meta.url);
    // [ts] import { defineConfig } from '@playwright/test';
    // [ts] export default defineConfig({
    // [ts] projects: [{ name: 'foo' }],
    // [ts] build: { external: ['*'] },
    // [ts] });
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, {}, { DEBUG: "pw:test", PLAYWRIGHT_FORCE_ASYNC_LOADER: "1" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Failed to load preflight") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "what the heck is preflight") != null, true);
    // [ts] });
}
