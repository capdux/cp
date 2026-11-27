// Migrated from TypeScript (page/page-evaluate-handle.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { test as it, expect } from "./pageTest.cp";










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EV-101 should work", () => {
    // [ts] const windowHandle = page.evaluateHandle(() => window);
    // [ts] expect_eq(windowHandle, true);
    // [ts] });
    // [ts] test("EV-102 should accept object handle as an argument", () => {
    // [ts] const navigatorHandle = page.evaluateHandle(() => navigator);
    // [ts] const text = page.evaluate(e => e.userAgent, navigatorHandle);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "Mozilla") != null, true);
    // [ts] });
    // [ts] test("EV-103 should accept object handle to primitive types", () => {
    // [ts] const aHandle = page.evaluateHandle(() => 5);
    // [ts] const isFive = page.evaluate(e => Object.is(e, 5), aHandle);
    // [ts] expect_eq(isFive, true);
    // [ts] });
    // [ts] test("EV-104 should accept nested handle", () => {
    // [ts] const foo = page.evaluateHandle(() => ({ x: 1, y: "foo" }));
    // [ts] const result = page.evaluate((_p) => {
    // [ts] return foo;
    // [ts] }, { foo });
    // [ts] expect_eq(result, { x: 1, y: "foo" });
    // [ts] });
    // [ts] test("EV-105 should accept nested window handle", () => {
    // [ts] const foo = page.evaluateHandle(() => window);
    // [ts] const result = page.evaluate((_p) => {
    // [ts] return foo == window;
    // [ts] }, { foo });
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("EV-106 should accept multiple nested handles", () => {
    // [ts] const foo = page.evaluateHandle(() => ({ x: 1, y: "foo" }));
    // [ts] const bar = page.evaluateHandle(() => 5);
    // [ts] const baz = page.evaluateHandle(() => (["baz"]));
    // [ts] const result = page.evaluate(x => {
    // [ts] return JSON.stringify(x);
    // [ts] }, { a1: { foo }, a2: { bar, arr: [{ baz }] } });
    // [ts] expect: (JSON.parse(result)) => {
    // [ts] a1: { foo: { x: 1, y: "foo" } },
    // [ts] a2: { bar: 5, arr: [{ baz: ["baz"] }] }
    // [ts] });
    // [ts] });
    // [ts] test("EV-107 should accept same handle multiple times", () => {
    // [ts] const foo = page.evaluateHandle(() => 1);
    // [ts] expect_eq(page.evaluate(x => x, { foo, bar: [foo], baz: { foo } }), { foo: 1, bar: [1], baz: { foo: 1 } });
    // [ts] });
    // [ts] test("EV-108 should accept same nested object multiple times", () => {
    // [ts] const foo = { x: 1 };
    // [ts] expect_eq(page.evaluate(x => x, { foo, bar: [foo], baz: { foo } }), { foo: { x: 1 }, bar: [{ x: 1 }], baz: { foo: { x: 1 } } });
    // [ts] });
    // [ts] test("EV-109 should accept object handle to unserializable value", () => {
    // [ts] const aHandle = page.evaluateHandle(() => Infinity);
    // [ts] expect_eq(page.evaluate(e => Object.is(e, Infinity), aHandle), true);
    // [ts] });
    // [ts] test("EV-110 should pass configurable args", () => {
    // [ts] const result = page.evaluate(arg => {
    // [ts] if (arg.foo != 42)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Not a 42');)
    // [ts] arg.foo = 17;
    // [ts] if (arg.foo != 17)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Not 17');)
    // [ts] delete arg.foo;
    // [ts] if (arg.foo == 17)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Still 17');)
    // [ts] return arg;
    // [ts] }, { foo: 42 });
    // [ts] expect_eq(result, {});
    // [ts] });
    // [ts] test("EV-111 should work with primitives", () => {
    // [ts] const aHandle = page.evaluateHandle(() => {
    // [ts] window["FOO"] = 123;
    // [ts] return window;
    // [ts] });
    // [ts] expect_eq(page.evaluate(e => e["FOO"], aHandle), 123);
    // [ts] });
}
