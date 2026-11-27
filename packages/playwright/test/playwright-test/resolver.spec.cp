// Migrated from TypeScript (playwright-test/resolver.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import path from "cap:path";

import { test, expect } from "./playwright-test-fixtures.cp";


    // TODO(cap-migrate): refine TS statement
    // [ts] "dir/inner.spec.ts": `
    // This import should pick up <root>/dir/tsconfig
    // [ts] import { foo } from 'parent-util/b';
    // This import should pick up <root>/tsconfig through the helper
    // [ts] import { foo as foo2 } from '../helper';
    // [ts] import { test, expect_eq } from "cap:test";
    // [ts] `,
    // [ts] });

  expect_eq(result.passed, 4);
  expect_eq(result.exitCode, 0);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.passed, 2);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.exitCode, 0);
  expect_eq(result.passed, 1);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.exitCode, 0);
  expect_eq(result.passed, 1);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.exitCode, 0);
  expect_eq(result.passed, 1);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.passed, 7);
  expect_eq(result.exitCode, 0);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.exitCode, 0);
  expect_eq(result.passed, 1);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.exitCode, 0);
  expect_eq(result.passed, 1);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
  expect_eq(std.mem.indexOf(u8, result.output, `Cannot`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(std.mem.indexOf(u8, result.output, "Cannot find module 'util/b'") != null, true);
  expect_eq(result.exitCode, 1);
// TODO(cap-migrate): refine TS statement
// [ts] });


  expect_eq(result.passed, 2);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "tsconfig.base2.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": "dir",
    // [ts] },
    // [ts] }`,
    // [ts] "tsconfig.base.json": `{
    // [ts] "compilerOptions": {
    // [ts] "paths": {
    // [ts] "util/*": ["./foo/bar/util/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.test.js": `
    // This js file is affected by tsconfig because allowJs is inherited.
    // Next line resolve to the final baseUrl ("dir") + relative path mapping ("./foo/bar/util/*").
    // [ts] const { foo } = require('util/file');
    // [ts] `,
    // [ts] "dir/foo/bar/util/file.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });

  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "a.test.ts": `
    // This resolves relative to the base tsconfig that defined path mapping,
    // because there is no baseUrl in the final tsconfig.
    // [ts] const { foo } = require('~/file');
    // [ts] `,
    // [ts] "mapped/file.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });

  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "a.test.ts": `
    // [ts] import { foo } from 'util/file';
    // [ts] `,
    // [ts] "mapped/file.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });

  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "a.test.ts": `
    // [ts] import { foo } from 'util/file';
    // [ts] `,
    // [ts] "mapped/file.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });
  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "a.test.ts": `
    // [ts] import { foo } from 'util/file';
    // [ts] `,
    // [ts] "mapped/file.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });

  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });




    // TODO(cap-migrate): refine TS statement
    // [ts] "a.test.ts": `
    // [ts] `,
    // [ts] });

  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { foo } from 'util/b';
    // [ts] `,
    // [ts] });

  expect_eq(result.exitCode, 0);
  expect_eq(result.passed, 1);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "mapped-from-root/foo.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "tests42/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["../should-be-ignored/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tests42/a.test.ts": `
    // [ts] import { foo } from '~/foo';
    // [ts] `,
    // [ts] "should-be-ignored/foo.ts": `
    // [ts] export const foo = 43;
    // [ts] `,
    // [ts] }, { tsconfig: "tsconfig.special.json" });

  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "mapped-from-tests/foo.ts": `
    // [ts] export const testFoo = 42;
    // [ts] `,
    // [ts] "tests/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["../should-be-ignored/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tests/a.test.ts": `
    // [ts] import { testFoo } from '~/foo';
    // [ts] import { configFoo } from '../playwright.config';
    // [ts] `,
    // [ts] "should-be-ignored/foo.ts": `
    // [ts] export const testFoo = 43;
    // [ts] export const configFoo = 18;
    // [ts] `,
    // [ts] });

  expect_eq(result.passed, 1);
  expect_eq(result.exitCode, 0);
  expect_eq(std.mem.indexOf(u8, result.output, `Could not`) == null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });

// [cap-test] test.describe("directory imports", () => {









// [cap-test] });

// [cap-test] test.describe("package.json imports", () => {
  // TODO(cap-migrate): refine TS statement
  // [ts] for (const { type, condition } of [{ type: "module", condition: "esm" }, { type: "commonjs", condition: "cjs" }]) {
  // [ts] test(`should try targets until one exists in ${type}`, {
  // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43966" },
  // [ts] }, (_p) => {
  // [ts] const files = {
  // [ts] "package.json": JSON.stringify({
  // [ts] name: "example-project",
  // [ts] type,
  // [ts] imports: {
  // [ts] "#/*": ["./src/*", "./src/*.ts", "./src/*/index.ts"],
  // [ts] "#lib/*": "./src/lib/*",
  // [ts] "#condition": {
  // [ts] import: ["./src/missing.ts", "./src/esm.ts"],
  // [ts] require: ["./src/missing.ts", "./src/cjs.ts"],
  // [ts] },
  // [ts] "#custom": { custom: "./src/custom.ts", default: "./src/missing.ts" },
  // [ts] },
  // [ts] }),
  // [ts] "tsconfig.json": JSON.stringify({
  // [ts] compilerOptions: { moduleResolution: "bundler", module: "preserve", customConditions: ["custom"], noEmit: true, allowJs: true, types: ["node"] },
  // [ts] }),
  // [ts] "playwright.config.ts": `export default {};`,
  // [ts] "src/utils/env.ts": `export const env: string = 'env';`,
  // [ts] "src/widgets/index.ts": `export const widgets: string = 'widgets';`,
  // [ts] "src/lib/foo.ts": `export const foo: string = 'foo';`,
  // [ts] "src/esm.ts": `export const condition: string = 'esm';`,
  // [ts] "src/cjs.ts": `export const condition: string = 'cjs';`,
  // [ts] "src/custom.ts": `export const custom: string = 'custom';`,
  // [ts] "src/helper.js": `export { env as envFromJs } from '#/utils/env';`,
  // [ts] "example.spec.ts": `
  // [ts] import { test, expect } from '@playwright/test';
  // [ts] import { env } from '#/utils/env';
  // [ts] import { widgets } from '#/widgets';
  // [ts] import { foo } from '#lib/foo.js';
  // [ts] import { condition } from '#condition';
  // [ts] import { custom } from '#custom';
  // [ts] import { envFromJs } from './src/helper.js';
  // [ts] test('test', () => {
  // [ts] expect_eq([env, widgets, foo, condition, custom, envFromJs], ['env', 'widgets', 'foo', '${condition}', 'custom', 'env']);
  // [ts] });
  // [ts] `,
  // [ts] };
  // [ts] for (const env of [{}, { PLAYWRIGHT_FORCE_ASYNC_LOADER: "1" }]) {
  // [ts] const result = runInlineTest(files, {}, { NODE_OPTIONS: "--conditions=custom", ...env });
  // [ts] expect_eq(result.exitCode, 0);
  // [ts] expect_eq(result.passed, 1);
  // [ts] }
  // [ts] const tscResult = runTSC(files);
  // [ts] expect_eq(tscResult.exitCode, 0);
  // [ts] });
  // [ts] }

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should print tsconfig parsing error", () => {
    // [ts] const files = {
    // [ts] "a.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] "foo": "bar"
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Failed to load tsconfig file at`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `tsconfig.json`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `JSON5: invalid character ':' at 2:12`) != null, true);
    // [ts] });
    // [ts] test("should respect path resolver", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11656" });
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [{name: 'foo'}],
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
    // [ts] "util2/*": ["./foo/bar/util/*"],
    // [ts] "util3": ["./does-not-exist", "./foo/bar/util/b"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.test.ts": `
    // [ts] import { foo } from 'util/b';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { foo } from 'util2/b';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "c.test.ts": `
    // [ts] import { foo } from 'util3';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "helper.ts": `
    // [ts] export { foo } from 'util3';
    // [ts] `,
    // [ts] "dir/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "target": "ES2019",
    // [ts] "module": "commonjs",
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "parent-util/*": ["../foo/bar/util/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] expect_eq(testInfo.project.name, foo2);
    // [ts] });
    // [ts] test("should respect baseurl", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [{name: 'foo'}],
    // [ts] };
    // [ts] `,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "target": "ES2019",
    // [ts] "module": "commonjs",
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "baseUrl": "./foo",
    // [ts] "paths": {
    // [ts] "util/*": ["./bar/util/*"],
    // [ts] "util2": ["./bar/util/b"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.test.ts": `
    // [ts] import { foo } from 'util/b';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { foo } from 'util2';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] });
    // [ts] test("should respect baseurl w/o paths", () => {
    // [ts] const result = runInlineTest({
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "dir2/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "target": "ES2019",
    // [ts] "module": "commonjs",
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "baseUrl": "..",
    // [ts] },
    // [ts] }`,
    // [ts] "dir2/inner.spec.ts": `
    // This import should pick up ../foo/bar/util/b due to baseUrl.
    // [ts] import { foo } from 'foo/bar/util/b';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] test("should fallback to *:* when baseurl and paths are specified", () => {
    // [ts] const result = runInlineTest({
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "shared/x.ts": `
    // [ts] export const x = 43;
    // [ts] `,
    // [ts] "dir2/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "target": "ES2019",
    // [ts] "module": "commonjs",
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "baseUrl": "..",
    // [ts] "paths": {
    // [ts] "shared/*": ["./shared/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "dir2/inner.spec.ts": `
    // This import should pick up ../foo/bar/util/b due to baseUrl and *:* fallback.
    // [ts] import { foo } from 'foo/bar/util/b';
    // This import should pick up ../shared/x due to baseUrl+paths.
    // [ts] import { x } from 'shared/x';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] expect_eq(x, 43);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] test("should use the location of the tsconfig as the paths root when no baseUrl is specified", () => {
    // [ts] const result = runInlineTest({
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "dir2/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "target": "ES2019",
    // [ts] "module": "commonjs",
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "paths": {"foo/*": ["../foo/*"]},
    // [ts] },
    // [ts] }`,
    // [ts] "dir2/inner.spec.ts": `
    // This import should pick up ../foo/bar/util/b due to paths.
    // [ts] import { foo } from 'foo/bar/util/b';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] test("should respect complex path resolver", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [{name: 'foo'}],
    // [ts] };
    // [ts] `,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "target": "ES2019",
    // [ts] "module": "commonjs",
    // [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "prefix-*": ["./prefix-*/bar"],
    // [ts] "prefix-*-suffix": ["./prefix-*-suffix/bar"],
    // [ts] "*-suffix": ["./*-suffix/bar"],
    // [ts] "no-star": ["./no-star-foo"],
    // [ts] "longest-*": ["./this-is-not-the-longest-prefix"],
    // [ts] "longest-pre*": ["./this-is-the-longest-prefix"],
    // [ts] "*bar": ["./*bar"],
    // [ts] "*[bar]": ["*foo"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.spec.ts": `
    // [ts] import { foo } from 'prefix-matchedstar';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "prefix-matchedstar/bar/index.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { foo } from 'prefix-matchedstar-suffix';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "prefix-matchedstar-suffix/bar.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "c.spec.ts": `
    // [ts] import { foo } from 'matchedstar-suffix';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "matchedstar-suffix/bar.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "d.spec.ts": `
    // [ts] import { foo } from 'no-star';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "./no-star-foo.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "e.spec.ts": `
    // [ts] import { foo } from 'longest-prefix';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "./this-is-the-longest-prefix.ts": `
    // this module should be resolved as it matches by a longer prefix
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "./this-is-not-the-longest-prefix.ts": `
    // This module should't be resolved as it matches by a shorter prefix
    // [ts] export const bar: string = 'bar';
    // [ts] `,
    // [ts] "f.spec.ts": `
    // [ts] import { foo } from 'barfoobar';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "barfoobar.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] "g.spec.ts": `
    // [ts] import { foo } from 'foo/[bar]';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "foo/foo.ts": `
    // [ts] export const foo: string = 'foo';
    // [ts] `,
    // [ts] });
    // [ts] test("should not use baseurl for relative imports", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15891" });
    // [ts] const result = runInlineTest({
    // [ts] "frontend/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": "src",
    // [ts] },
    // [ts] }`,
    // [ts] "frontend/playwright/utils.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "frontend/playwright/tests/forms_cms_standard.spec.ts": `
    // This relative import should not use baseUrl
    // [ts] import { foo } from '../utils';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] test("should not use baseurl for relative imports when dir with same name exists", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15891" });
    // [ts] const result = runInlineTest({
    // [ts] "frontend/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": "src",
    // [ts] },
    // [ts] }`,
    // [ts] "frontend/src/utils/foo.js": `
    // [ts] export const foo = -1;
    // [ts] `,
    // [ts] "frontend/src/index.js": `
    // [ts] export const index = -1;
    // [ts] `,
    // [ts] "frontend/src/.bar.js": `
    // [ts] export const bar = 42;
    // [ts] `,
    // [ts] "frontend/playwright/tests/utils.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "frontend/playwright/tests/index.js": `
    // [ts] export const index = 42;
    // [ts] `,
    // [ts] "frontend/playwright/tests/forms_cms_standard.spec.ts": `
    // These relative imports should not use baseUrl
    // [ts] import { foo } from './utils';
    // [ts] import { index } from '.';
    // This absolute import should use baseUrl
    // [ts] import { bar } from '.bar';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] expect_eq(index, 42);
    // [ts] expect_eq(bar, 42);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] test("should respect path resolver for JS files when allowJs", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}], };`,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "allowJs": true,
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "util/*": ["./foo/bar/util/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.test.js": `
    // [ts] const { foo } = require('util/b');
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });
    // [ts] test("should not respect path resolver for JS files w/o allowJS", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}], };`,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "util/*": ["./foo/bar/util/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.test.js": `
    // [ts] const { foo } = require('util/b');
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });
    // [ts] test("should respect path resolver for JS and TS files from jsconfig.json", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}], };`,
    // [ts] "jsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "util/*": ["./foo/bar/util/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "a.test.js": `
    // [ts] const { foo } = require('util/b');
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { foo } from 'util/b';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] expect_eq(testInfo.project.name, foo);
    // [ts] });
    // [ts] `,
    // [ts] "foo/bar/util/b.ts": `
    // [ts] export const foo = 'foo';
    // [ts] `,
    // [ts] });
    // [ts] test("should support extends in tsconfig.json", () => {
    // [ts] const result = runInlineTest({
    // [ts] "tsconfig.json": `{
    // [ts] "extends": ["./tsconfig.base1.json", "./tsconfig.base2.json"],
    // [ts] }`,
    // [ts] "tsconfig.base1.json": `{
    // [ts] "extends": "./tsconfig.base.json",
    // [ts] "compilerOptions": {
    // [ts] "allowJs": true,
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] test("should resolve paths relative to the originating config when extending and no baseUrl", () => {
    // [ts] const result = runInlineTest({
    // [ts] "tsconfig.json": `{
    // [ts] "extends": ["./dir/tsconfig.base.json"],
    // [ts] }`,
    // [ts] "dir/tsconfig.base.json": `{
    // [ts] "compilerOptions": {
    // [ts] "paths": {
    // [ts] "~/*": ["../mapped/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] test("should resolve extends from an explicit node_modules subpath", () => {
    // The @tsconfig/* base packages are commonly referenced by an explicit subpath,
    // e.g. "extends": "@tsconfig/node18/tsconfig.json". Playwright resolves such a
    // value through the config's node_modules when it is not found relative to the
    // config directory. Removing that fallback would break these configs.
    // [ts] const result = runInlineTest({
    // [ts] "node_modules/@my/tsconfig-base/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "paths": {
    // [ts] "util/*": ["./mapped/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tsconfig.json": `{
    // [ts] "extends": "@my/tsconfig-base/tsconfig.json",
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] test("should resolve extends from a bare package name", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43234" });
    // The @tsconfig/* base packages are documented as "extends": "@tsconfig/node20".
    // tsc resolves a bare package name to the tsconfig.json at the package root.
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ name: "test-project" }),
    // [ts] "node_modules/@tsconfig/base/package.json": JSON.stringify({ name: "@tsconfig/base", version: "1.0.0" }),
    // [ts] "node_modules/@tsconfig/base/tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "paths": {
    // [ts] "util/*": ["./mapped/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tsconfig.json": `{
    // [ts] "extends": "@tsconfig/base",
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] test("should resolve extends from the \"tsconfig\" field in package.json", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43234" });
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ name: "test-project" }),
    // [ts] "node_modules/@my/tsconfig-base/package.json": JSON.stringify({ name: "@my/tsconfig-base", version: "1.0.0", tsconfig: "./base.json" }),
    // [ts] "node_modules/@my/tsconfig-base/base.json": `{
    // [ts] "compilerOptions": {
    // [ts] "paths": {
    // [ts] "util/*": ["./mapped/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tsconfig.json": `{
    // [ts] "extends": "@my/tsconfig-base",
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] test("should ignore extends bare specifier resolvable only via node_modules walk-up", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41989" });
    // [ts] const baseDir = testInfo.outputPath();
    // [ts] const symlinkType = if (process.platform == "win32") "junction" else "dir";
    // npm symlinks "file:" dependencies into node_modules. The realpath escapes node_modules,
    // so the dependency goes through per-file tsconfig discovery.
    // [ts] fs.promises.mkdir(path.join(baseDir, "node_modules"), { recursive: true });
    // [ts] fs.promises.symlink(path.join(baseDir, "dep-src"), path.join(baseDir, "node_modules", "dep"), symlinkType);
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ name: "repro", private: true, dependencies: { "dep": "file:./dep-src", "foo": "1.0.0" } }),
    // "foo" is hoisted to the root node_modules and is absent from dep's own node_modules.
    // tsc resolves the "extends" below by walking up node_modules from dep-src.
    // [ts] "node_modules/foo/package.json": JSON.stringify({ name: "foo", version: "1.0.0", main: "index.js" }),
    // [ts] "node_modules/foo/index.js": `module.exports = 'foo';`,
    // [ts] "node_modules/foo/tsconfig.json": `{ "compilerOptions": {} }`,
    // [ts] "dep-src/package.json": JSON.stringify({ name: "dep", version: "1.0.0", main: "index.js" }),
    // [ts] "dep-src/tsconfig.json": `{ "extends": "foo/tsconfig.json" }`,
    // [ts] "dep-src/index.js": `
    // [ts] require('foo');
    // [ts] module.exports = 'dep';
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const dep = require('dep');
    // [ts] test('test', () => {
    // [ts] expect_eq(dep, 'dep');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should ignore extends subpath resolvable only via package.json exports", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41989" });
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ name: "test-project" }),
    // The "./next" subpath only exists in the exports map, there is no "next.json" file.
    // [ts] "node_modules/@repo/tsconfig/package.json": JSON.stringify({ name: "@repo/tsconfig", version: "1.0.0", exports: { "./next": "./tsconfig.next.json" } }),
    // [ts] "node_modules/@repo/tsconfig/tsconfig.next.json": `{ "compilerOptions": {} }`,
    // [ts] "tsconfig.json": `{ "extends": "@repo/tsconfig/next" }`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should ignore extends bare package name resolvable only via package.json exports", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41989" });
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({ name: "test-project" }),
    // [ts] "node_modules/typescript-config-silverwind/package.json": JSON.stringify({ name: "typescript-config-silverwind", version: "1.0.0", exports: "./tsconfig.json" }),
    // [ts] "node_modules/typescript-config-silverwind/tsconfig.json": `{ "compilerOptions": {} }`,
    // [ts] "tsconfig.json": `{ "extends": "typescript-config-silverwind" }`,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should ignore tsconfig references pointing to a directory", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41998" });
    // [ts] const result = runInlineTest({
    // tsc resolves a directory-form reference to "./sub/tsconfig.json",
    // as written by "tsc --build" and "nx sync".
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": { "composite": true },
    // [ts] "references": [{ "path": "./sub" }],
    // [ts] }`,
    // [ts] "sub/tsconfig.json": `{
    // [ts] "compilerOptions": { "composite": true },
    // [ts] }`,
    // [ts] test('test', () => {});
    // [ts] test("should respect tsconfig project references", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29256" });
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `export default { projects: [{name: 'foo'}], };`,
    // [ts] "tsconfig.json": `{
    // [ts] "files": [],
    // [ts] "references": [
    // [ts] { "path": "./tsconfig.app.json" },
    // [ts] { "path": "./tsconfig.test.json" }
    // [ts] ]
    // [ts] }`,
    // [ts] "tsconfig.test.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "util/*": ["./foo/bar/util/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 'foo');
    // [ts] });
    // [ts] test("should respect --tsconfig option", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] import { foo } from '~/foo';
    // [ts] export default {
    // [ts] testDir: './tests' + foo,
    // [ts] };
    // [ts] `,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["./does-not-exist/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "tsconfig.special.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["./mapped-from-root/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(foo, 42);
    // [ts] });
    // [ts] test("should respect config.tsconfig option", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export { configFoo } from '~/foo';
    // [ts] export default {
    // [ts] testDir: './tests',
    // [ts] tsconfig: './tsconfig.tests.json',
    // [ts] };
    // [ts] `,
    // [ts] "tsconfig.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["./mapped-from-config/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] "mapped-from-config/foo.ts": `
    // [ts] export const configFoo = 17;
    // [ts] `,
    // [ts] "tsconfig.tests.json": `{
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "~/*": ["./mapped-from-tests/*"],
    // [ts] },
    // [ts] },
    // [ts] }`,
    // [ts] test('test', () => {
    // [ts] expect_eq(testFoo, 42);
    // [ts] expect_eq(configFoo, 17);
    // [ts] });
    // [ts] test("should resolve index.js without path mapping in CJS", () => {
    // [ts] const files = {
    // [ts] "foo-pkg/index.js": `
    // [ts] exports.foo = 'bar';
    // [ts] `,
    // [ts] "foo-pkg/index.d.ts": `
    // [ts] export const foo: 'bar';
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { foo } from './foo-pkg';
    // [ts] test('pass', () => {
    // [ts] const bar: 'bar' = foo;
    // [ts] expect_eq(bar, 'bar');
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve index.js without path mapping in ESM", () => {
    // [ts] const files = {
    // [ts] "foo-pkg/index.js": `
    // [ts] export const foo = 'bar';
    // [ts] `,
    // [ts] "foo-pkg/index.d.ts": `
    // [ts] export const foo: 'bar';
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "type": "module" }
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { foo } from './foo-pkg';
    // [ts] test('pass', () => {
    // [ts] const bar: 'bar' = foo;
    // [ts] expect_eq(bar, 'bar');
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve index.js after path mapping in CJS", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31811" });
    // [ts] const files = {
    // [ts] "@acme/lib/index.js": `
    // [ts] exports.greet = () => 2;
    // [ts] `,
    // [ts] "@acme/lib/index.d.ts": `
    // [ts] export const greet: () => number;
    // [ts] `,
    // [ts] "tests/hello.test.ts": `
    // [ts] import { greet } from '@acme/lib';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('hello', () => {
    // [ts] const foo: number = greet();
    // [ts] expect_eq(foo, 2);
    // [ts] });
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "paths": {
    // [ts] "@acme/*": ["./@acme/*"]
    // [ts] },
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"]
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should resolve index.js after path mapping in ESM", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31811" });
    // [ts] const files = {
    // [ts] "@acme/lib/index.js": `
    // [ts] export const greet = () => 2;
    // [ts] `,
    // [ts] "@acme/lib/index.d.ts": `
    // [ts] export const greet: () => number;
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "type": "module" }
    // [ts] `,
    // [ts] "tests/hello.test.ts": `
    // [ts] import { greet } from '@acme/lib';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('hello', () => {
    // [ts] const foo: number = greet();
    // [ts] expect_eq(foo, 2);
    // [ts] });
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "paths": {
    // [ts] "@acme/*": ["./@acme/*"]
    // [ts] },
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"]
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should respect package.json#main after path mapping in CJS", () => {
    // [ts] const files = {
    // [ts] "app/pkg/main.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "app/pkg/package.json": `
    // [ts] { "main": "main.ts" }
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "name": "example-project" }
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {};
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "app/*": ["app/*"]
    // [ts] },
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"],
    // [ts] "ignoreDeprecations": "6.0"
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "example.spec.ts": `
    // [ts] import { foo } from 'app/pkg';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] const bar: number = foo;
    // [ts] expect_eq(bar, 42);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `find module`) == null, true);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should respect package.json#main after path mapping in ESM", () => {
    // [ts] const files = {
    // [ts] "app/pkg/main.ts": `
    // [ts] export const foo = 42;
    // [ts] `,
    // [ts] "app/pkg/package.json": `
    // [ts] { "main": "main.ts", "type": "module" }
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "name": "example-project", "type": "module" }
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {};
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "app/*": ["app/*"]
    // [ts] },
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"],
    // [ts] "ignoreDeprecations": "6.0"
    // [ts] },
    // [ts] }
    // [ts] `,
    // [ts] "example.spec.ts": `
    // [ts] import { foo } from 'app/pkg';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] const bar: number = foo;
    // [ts] expect_eq(bar, 42);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should respect package.json#exports without path mapping in CJS", () => {
    // [ts] const files = {
    // [ts] "node_modules/foo-pkg/package.json": `
    // [ts] { "name": "foo-pkg", "exports": { ".": "./foo.js" } }
    // [ts] `,
    // [ts] "node_modules/foo-pkg/foo.js": `
    // [ts] exports.foo = 'bar';
    // [ts] `,
    // [ts] "node_modules/foo-pkg/foo.d.ts": `
    // [ts] export const foo: 'bar';
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "name": "test-project" }
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { foo } from 'foo-pkg';
    // [ts] test('pass', () => {
    // [ts] const bar: 'bar' = foo;
    // [ts] expect_eq(bar, 'bar');
    // [ts] });
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"]
    // [ts] },
    // [ts] }
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should respect package.json#exports without path mapping in ESM", () => {
    // [ts] const files = {
    // [ts] "node_modules/foo-pkg/package.json": `
    // [ts] { "name": "foo-pkg", "type": "module", "exports": { "default": "./foo.js" } }
    // [ts] `,
    // [ts] "node_modules/foo-pkg/foo.js": `
    // [ts] export const foo = 'bar';
    // [ts] `,
    // [ts] "node_modules/foo-pkg/foo.d.ts": `
    // [ts] export const foo: 'bar';
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "name": "test-project", "type": "module" }
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { foo } from 'foo-pkg';
    // [ts] test('pass', () => {
    // [ts] const bar: 'bar' = foo;
    // [ts] expect_eq(bar, 'bar');
    // [ts] });
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"]
    // [ts] },
    // [ts] }
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should not respect package.json#exports after type mapping in CJS", () => {
    // [ts] const files = {
    // [ts] "app/pkg/main.ts": `
    // [ts] export const filename: 'main.ts' = 'main.ts';
    // [ts] `,
    // [ts] "app/pkg/index.js": `
    // [ts] export const filename = 'index.js';
    // [ts] `,
    // [ts] "app/pkg/index.d.ts": `
    // [ts] export const filename: 'index.js';
    // [ts] `,
    // [ts] "app/pkg/package.json": `
    // [ts] { "exports": { ".": "./main.ts" } }
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "name": "example-project" }
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {};
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "app/*": ["app/*"]
    // [ts] },
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"],
    // [ts] "ignoreDeprecations": "6.0"
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "example.spec.ts": `
    // [ts] import { filename } from 'app/pkg';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] const foo: 'index.js' = filename;
    // [ts] expect_eq(foo, 'index.js');
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should not respect package.json#exports after type mapping in ESM", () => {
    // [ts] const files = {
    // [ts] "app/pkg/main.ts": `
    // [ts] export const filename: 'main.ts' = 'main.ts';
    // [ts] `,
    // [ts] "app/pkg/index.js": `
    // [ts] export const filename = 'index.js';
    // [ts] `,
    // [ts] "app/pkg/index.d.ts": `
    // [ts] export const filename: 'index.js';
    // [ts] `,
    // [ts] "app/pkg/package.json": `
    // [ts] { "exports": { ".": "./main.ts" }, "type": "module" }
    // [ts] `,
    // [ts] "package.json": `
    // [ts] { "name": "example-project", "type": "module" }
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {};
    // [ts] `,
    // [ts] "tsconfig.json": `
    // [ts] {
    // [ts] "compilerOptions": {
    // [ts] "baseUrl": ".",
    // [ts] "paths": {
    // [ts] "app/*": ["app/*"]
    // [ts] },
    // [ts] "moduleResolution": "bundler",
    // [ts] "module": "preserve",
    // [ts] "noEmit": true,
    // [ts] "noImplicitAny": true,
    // [ts] "types": ["node"],
    // [ts] "ignoreDeprecations": "6.0"
    // [ts] }
    // [ts] }
    // [ts] `,
    // [ts] "example.spec.ts": `
    // [ts] import { filename } from 'app/pkg';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] const foo: 'index.js' = filename;
    // [ts] expect_eq(foo, 'index.js');
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] const result = runInlineTest(files);
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tscResult = runTSC(files);
    // [ts] expect_eq(tscResult.exitCode, 0);
    // [ts] });
    // [ts] test("should keep the target that Node.js picks", () => {
    // [ts] const result = runInlineTest({
    // [ts] "package.json": JSON.stringify({
    // [ts] name: "example-project",
    // [ts] type: "module",
    // [ts] imports: {
    // [ts] "#package": ["./src/missing.ts", "dep"],
    // [ts] "#optional": ["optional-dep", "./src/local.ts"],
    // [ts] "#both": "./src/both.js",
    // [ts] "#first-null": [null, "./src/local.ts"],
    // [ts] "#/*": "./src/*.ts",
    // [ts] "#/utils/*": "./lib/*.ts",
    // [ts] "#paths": "./src/local.ts",
    // [ts] },
    // [ts] }),
    // [ts] "tsconfig.json": JSON.stringify({
    // [ts] compilerOptions: { paths: { "#paths": ["./src/from-paths.ts"] } },
    // [ts] }),
    // [ts] "playwright.config.ts": `export default {};`,
    // [ts] "node_modules/dep/package.json": JSON.stringify({ name: "dep", type: "module", exports: "./index.js" }),
    // [ts] "node_modules/dep/index.js": `export const value = 'dep';`,
    // [ts] "src/local.ts": `export const value: string = 'local';`,
    // [ts] "src/both.js": `export const value = 'both.js';`,
    // [ts] "src/both.ts": `export const value: string = 'both.ts';`,
    // [ts] "src/from-paths.ts": `export const value: string = 'from-paths';`,
    // [ts] "src/utils/foo.ts": `export const value: string = 'src/utils/foo';`,
    // [ts] "lib/foo.ts": `export const value: string = 'lib/foo';`,
    // [ts] "example.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { value as pkg } from '#package';
    // [ts] import { value as optional } from '#optional';
    // [ts] import { value as both } from '#both';
    // [ts] import { value as firstNull } from '#first-null';
    // [ts] import { value as longestPattern } from '#/utils/foo';
    // [ts] import { value as fromPaths } from '#paths';
    // [ts] test('test', () => {
    // [ts] expect_eq([pkg, optional, both, firstNull, longestPattern, fromPaths], ['dep', 'local', 'both.js', 'local', 'lib/foo', 'from-paths']);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
}
