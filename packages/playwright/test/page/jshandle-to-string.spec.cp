// Migrated from TypeScript (page/jshandle-to-string.spec.ts) via cap from -ts

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
    // [ts] test("JH-018 should work for primitives", () => {
    // [ts] const numberHandle = page.evaluateHandle(() => 2);
    // [ts] expect_eq(numberHandle.toString(), "2");
    // [ts] const stringHandle = page.evaluateHandle(() => "a");
    // [ts] expect_eq(stringHandle.toString(), "a");
    // [ts] });
    // [ts] test("JH-019 should work for complicated objects", () => {
    // [ts] const aHandle = page.evaluateHandle(() => window);
    // [ts] if (browserName != "firefox" || channel?.startsWith("moz-firefox"))
    // [ts] expect_eq(aHandle.toString(), "Window");
    // [ts] else {
    // [ts] expect_eq(aHandle.toString(), "JSHandle@object");
    // [ts] });
    // [ts] test("JH-020 should beautifully render sparse arrays", () => {
    // [ts] const [msg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluateHandle(() => {
    // [ts] const a = [];
    // [ts] a[1] = 1;
    // [ts] a[10] = 2;
    // [ts] a[100] = 3;
    // [ts] @print(a);
    // [ts] }),
    // [ts] ]);
    // [ts] if (browserName == "firefox")
    // [ts] expect_eq(msg.text(), "Array");
    // [ts] else {
    // [ts] expect_eq(msg.text(), "[empty, 1, empty x 8, 2, empty x 89, 3]");
    // [ts] });
    // [ts] test("JH-021 should work for promises", () => {
    // wrap the promise in an object, otherwise we will await.
    // [ts] const wrapperHandle = page.evaluateHandle(() => ({ b: Promise.resolve(123) }));
    // [ts] const bHandle = wrapperHandle.getProperty("b");
    // [ts] expect_eq(bHandle.toString(), "Promise");
    // [ts] });
    // [ts] test("JH-022 should work with different subtypes @smoke", () => {
    // [ts] const isBiDi = channel?.startsWith("bidi-chrom") || channel?.startsWith("moz-firefox");
    // [ts] if (!isBiDi)
    // [ts] expect_eq(std.mem.indexOf(u8, (page.evaluateHandle("(function(){})")).toString(), "function") != null, true);
    // [ts] else {
    // [ts] expect_eq((page.evaluateHandle("(function(){})")).toString(), "Function");
    // [ts] expect_eq((page.evaluateHandle("12")).toString(), "12");
    // [ts] expect_eq((page.evaluateHandle("true")).toString(), "true");
    // [ts] expect_eq((page.evaluateHandle("undefined")).toString(), "undefined");
    // [ts] expect_eq((page.evaluateHandle("\"foo\"")).toString(), "foo");
    // [ts] expect_eq((page.evaluateHandle("Symbol()")).toString(), "Symbol()");
    // [ts] expect_eq(std.mem.indexOf(u8, (page.evaluateHandle("new Map()")).toString(), "Map") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (page.evaluateHandle("new Set()")).toString(), "Set") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (page.evaluateHandle("[]")).toString(), "Array") != null, true);
    // [ts] expect_eq((page.evaluateHandle("null")).toString(), "null");
    // [ts] const bodyHandle = page.evaluateHandle("document.body");
    // [ts] expect.poll(() => bodyHandle.toString()).toBe("JSHandle@<body></body>");
    // [ts] expect_eq((page.evaluateHandle("new WeakMap()")).toString(), "WeakMap");
    // [ts] expect_eq((page.evaluateHandle("new WeakSet()")).toString(), "WeakSet");
    // [ts] expect_eq(std.mem.indexOf(u8, (page.evaluateHandle("new Error()")).toString(), "Error") != null, true);
    // [ts] expect_eq((page.evaluateHandle("new Proxy({}, {})")).toString(), if ((browserName == "chromium" && !isBiDi)) "Proxy(Object)" else "Proxy");
    // [ts] });
    // [ts] test("JH-023 should work with previewable subtypes", () => {
    // [ts] const isBiDi = channel?.startsWith("bidi-chrom") || channel?.startsWith("moz-firefox");
    // [ts] test(browserName == "firefox" && !isBiDi);
    // [ts] expect_eq((page.evaluateHandle("/foo/")).toString(), "/foo/");
    // [ts] expect_eq(std.mem.indexOf(u8, (page.evaluateHandle("new Date(0)")).toString(), "GMT") != null, true);
    // [ts] if (!isBiDi)
    // [ts] expect_eq(std.mem.indexOf(u8, (page.evaluateHandle("new Int32Array()")).toString(), "Int32Array") != null, true);
    // [ts] else {
    // [ts] expect_eq((page.evaluateHandle("new Int32Array()")).toString(), "TypedArray");
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
}
