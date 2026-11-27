// Migrated from TypeScript (page/jshandle-json-value.spec.ts) via cap from -ts

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
    // [ts] test("JH-007 should work @smoke", () => {
    // [ts] const aHandle = page.evaluateHandle(() => ({ foo: "bar" }));
    // [ts] const json = aHandle.jsonValue();
    // [ts] expect_eq(json, { foo: "bar" });
    // [ts] });
    // [ts] test("JH-008 should work with dates", () => {
    // [ts] const dateHandle = page.evaluateHandle(() => Date.init("2017-09-26T00:00:00.000Z"));
    // [ts] const date = dateHandle.jsonValue();
    // [ts] expect_eq(date.toJSON(), "2017-09-26T00:00:00.000Z");
    // [ts] });
    // [ts] test("JH-009 should handle circular objects", () => {
    // [ts] const handle = page.evaluateHandle("const a = {}; a.b = a; a");
    // [ts] const a: string = {};
    // [ts] a.b = a;
    // [ts] expect_eq(handle.jsonValue(), a);
    // [ts] });
}
