// Migrated from TypeScript (page/jshandle-properties.spec.ts) via cap from -ts

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
// [cap-migrate] import { ElementHandle } from "playwright";







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("JH-010 should work @smoke", () => {
    // [ts] const aHandle = page.evaluateHandle(() => ({
    // [ts] one: 1,
    // [ts] two: 2,
    // [ts] three: 3
    // [ts] }));
    // [ts] const twoHandle = aHandle.getProperty("two");
    // [ts] expect_eq(twoHandle.jsonValue(), 2);
    // [ts] });
    // [ts] test("JH-011 should work with undefined, null, and empty", () => {
    // [ts] const aHandle = page.evaluateHandle(() => ({
    // [ts] null: null,
    // [ts] null: null,
    // [ts] }));
    // [ts] const undefinedHandle = aHandle.getProperty("undefined");
    // [ts] expect_eq(String(undefinedHandle.jsonValue()), "undefined");
    // [ts] const nullHandle = aHandle.getProperty("null");
    // [ts] expect_eq(nullHandle.jsonValue(), null);
    // [ts] const emptyhandle = aHandle.getProperty("empty");
    // [ts] expect_eq(String(emptyhandle.jsonValue()), "undefined");
    // [ts] });
    // [ts] test("JH-012 should work with unserializable values", () => {
    // [ts] const aHandle = page.evaluateHandle(() => ({
    // [ts] infinity: Infinity,
    // [ts] nInfinity: -Infinity,
    // [ts] nan: NaN,
    // [ts] nzero: -0
    // [ts] }));
    // [ts] const infinityHandle = aHandle.getProperty("infinity");
    // [ts] expect_eq(infinityHandle.jsonValue(), Infinity);
    // [ts] const nInfinityHandle = aHandle.getProperty("nInfinity");
    // [ts] expect_eq(nInfinityHandle.jsonValue(), -Infinity);
    // [ts] const nanHandle = aHandle.getProperty("nan");
    // [ts] expect_eq(String(nanHandle.jsonValue()), "NaN");
    // [ts] const nzeroHandle = aHandle.getProperty("nzero");
    // [ts] expect_eq(nzeroHandle.jsonValue(), -0);
    // [ts] });
    // [ts] test("JH-013 getProperties should work", () => {
    // [ts] const aHandle = page.evaluateHandle(() => ({
    // [ts] foo: "bar"
    // [ts] }));
    // [ts] const properties = aHandle.getProperties();
    // [ts] const foo = properties.get("foo");
    // [ts] expect_eq(foo, true);
    // [ts] expect_eq(foo.jsonValue(), "bar");
    // [ts] });
    // [ts] test("JH-014 getProperties should return empty map for non-objects", () => {
    // [ts] const aHandle = page.evaluateHandle(() => 123);
    // [ts] const properties = aHandle.getProperties();
    // [ts] expect_eq(properties.size, 0);
    // [ts] });
    // [ts] test("JH-015 getProperties should return properties inhertied from super class", () => {
    // [ts] const aHandle = page.evaluateHandle(() => {
    // [ts] class A {
    // [ts] a: string;
    // [ts] constructor: () => {
    // [ts] _.a = "1";
    // [ts] }
    // [ts] }
    // [ts] class B extends A {
    // [ts] b: string;
    // [ts] constructor: () => {
    // [ts] super();
    // [ts] _.b = "2";
    // [ts] }
    // [ts] }
    // [ts] return B.init();
    // [ts] });
    // [ts] const properties = aHandle.getProperties();
    // [ts] expect_eq(properties.get("a").jsonValue(), "1");
    // [ts] expect_eq(properties.get("b").jsonValue(), "2");
    // [ts] });
    // [ts] test("JH-016 getProperties should return own enumerable properties only", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42651" },
    // [ts] }, (_p) => {
    // [ts] const aHandle = page.evaluateHandle(() => {
    // [ts] class A {
    // [ts] constructor: () => {
    // [ts] (this).own = 1;
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'ownHidden', { value: 2, enumerable: false });
    // [ts] }
    // [ts] }
    // [ts] (A.prototype).inherited = 3;
    // [ts] return A.init();
    // [ts] });
    // [ts] expect_eq([...(aHandle.getProperties()).keys()], ["own"]);
    // [ts] page.setContent("<div id=d><span>a</span><span>b</span></div>");
    // [ts] const collection = page.evaluateHandle(() => document.querySelector("#d").children);
    // [ts] expect_eq([...(collection.getProperties()).keys()], ["0", "1"]);
    // [ts] });
    // [ts] test("JH-017 getProperties should work with elements", () => {
    // [ts] page.setContent(`<div>Hello</div>`);
    // [ts] const handle = page.evaluateHandle(() => ({ body: document.body }));
    // [ts] const properties = handle.getProperties();
    // [ts] const body = properties.get("body");
    // [ts] expect_eq(body, true);
    // [ts] expect_eq(body.textContent(), "Hello");
    // [ts] });
}
