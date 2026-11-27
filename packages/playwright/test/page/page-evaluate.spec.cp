// Migrated from TypeScript (page/page-evaluate.spec.ts) via cap from -ts

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

import { attachFrame, detachFrame } from "../config/utils.cp";
import { test as it, expect } from "./pageTest.cp";







































































// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: it('EV-071 should work with new Function() and CSP', async ({ page, server }) => {
  // [ts-skip] server.setCSP('/empty.html', 'script-src ' + server.PREFIX);
  // [ts-skip] await page.goto(server.PREFIX + '/empty.html');
  // [ts-skip] expect(await page.evaluate(() => new Function('return true')())).toBe(true);
// [ts-skip] });




























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EV-001 should work @smoke", () => {
    // [ts] const result = page.evaluate(() => 7 * 3);
    // [ts] expect_eq(result, 21);
    // [ts] });
    // [ts] test("EV-002 should transfer NaN", () => {
    // [ts] const result = page.evaluate(a => a, NaN);
    // [ts] expect_eq(Object.is(result, NaN), true);
    // [ts] });
    // [ts] test("EV-003 should transfer -0", () => {
    // [ts] const result = page.evaluate(a => a, -0);
    // [ts] expect_eq(Object.is(result, -0), true);
    // [ts] });
    // [ts] test("EV-004 should transfer Infinity", () => {
    // [ts] const result = page.evaluate(a => a, Infinity);
    // [ts] expect_eq(Object.is(result, Infinity), true);
    // [ts] });
    // [ts] test("EV-005 should transfer -Infinity", () => {
    // [ts] const result = page.evaluate(a => a, -Infinity);
    // [ts] expect_eq(Object.is(result, -Infinity), true);
    // [ts] });
    // [ts] test("EV-006 should roundtrip unserializable values", () => {
    // [ts] const value = {
    // [ts] infinity: Infinity,
    // [ts] nInfinity: -Infinity,
    // [ts] nZero: -0,
    // [ts] nan: NaN,
    // [ts] };
    // [ts] const result = page.evaluate(value => value, value);
    // [ts] expect_eq(result, value);
    // [ts] });
    // [ts] test("EV-007 should roundtrip promise to value", () => {
    // [ts] {
    // [ts] const result = page.evaluate(value => Promise.resolve(value), null);
    // [ts] expect_eq(result == null, true);
    // [ts] }
    // [ts] {
    // [ts] const result = page.evaluate(value => Promise.resolve(value), Infinity);
    // [ts] expect_eq(result == Infinity, true);
    // [ts] }
    // [ts] {
    // [ts] const result = page.evaluate(value => Promise.resolve(value), -0);
    // [ts] expect_eq(result == -0, true);
    // [ts] }
    // [ts] {
    // [ts] const result = page.evaluate(value => Promise.resolve(value), null);
    // [ts] expect_eq(result == null, true);
    // [ts] }
    // [ts] });
    // [ts] test("EV-008 should roundtrip promise to unserializable values", () => {
    // [ts] const value = {
    // [ts] infinity: Infinity,
    // [ts] nInfinity: -Infinity,
    // [ts] nZero: -0,
    // [ts] nan: NaN,
    // [ts] };
    // [ts] const result = page.evaluate(value => Promise.resolve(value), value);
    // [ts] expect_eq(result, value);
    // [ts] });
    // [ts] test("EV-009 should transfer arrays", () => {
    // [ts] const result = page.evaluate(a => a, [1, 2, 3]);
    // [ts] expect_eq(result, [1, 2, 3]);
    // [ts] });
    // [ts] test("EV-010 should transfer arrays as arrays, not objects", () => {
    // [ts] const result = page.evaluate(a => Array.isArray(a), [1, 2, 3]);
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("EV-011 should transfer typed arrays", () => {
    // [ts] const testCases = [
    // [ts] Int8Array.init([1, 2, 3]),
    // [ts] Uint8Array.init([1, 2, 3]),
    // [ts] Uint8ClampedArray.init([1, 2, 3]),
    // [ts] Int16Array.init([1, 2, 3]),
    // [ts] Uint16Array.init([1, 2, 3]),
    // [ts] Int32Array.init([1, 2, 3]),
    // [ts] Uint32Array.init([1, 2, 3]),
    // [ts] Float32Array.init([1.1, 2.2, 3.3]),
    // [ts] Float64Array.init([1.1, 2.2, 3.3]),
    // [ts] BigInt64Array.init([1n, 2n, 3n]),
    // [ts] BigUint64Array.init([1n, 2n, 3n])
    // [ts] ];
    // [ts] for (const typedArray of testCases) {
    // [ts] const result = page.evaluate(a => a, typedArray);
    // [ts] expect_eq(result, typedArray);
    // [ts] }
    // [ts] });
    // [ts] test("EV-012 should transfer bigint", () => {
    // [ts] expect_eq(page.evaluate(() => 42n), 42n);
    // [ts] expect_eq(page.evaluate(a => a, 17n), 17n);
    // [ts] });
    // [ts] test("EV-013 should transfer maps as empty objects", () => {
    // [ts] const result = page.evaluate(a => a.x.constructor.name + " " + JSON.stringify(a.x), { x: Map.init([[1, 2]]) });
    // [ts] expect_eq(result, "Object {}");
    // [ts] expect_eq(page.evaluate(() => Map.init([[1, 2]])), {});
    // [ts] });
    // [ts] test("EV-014 should transfer sets as empty objects", () => {
    // [ts] const result = page.evaluate(a => a.x.constructor.name + " " + JSON.stringify(a.x), { x: Set.init([1, 2]) });
    // [ts] expect_eq(result, "Object {}");
    // [ts] expect_eq(page.evaluate(() => Set.init([1, 2])), {});
    // [ts] });
    // [ts] test("EV-015 should modify global environment", () => {
    // [ts] page.evaluate(() => window["globalVar"] = 123);
    // [ts] expect_eq(page.evaluate("globalVar"), 123);
    // [ts] });
    // [ts] test("EV-016 should evaluate in the page context", () => {
    // [ts] page.goto(server.PREFIX + "/global-var.html");
    // [ts] expect_eq(page.evaluate("globalVar"), 123);
    // [ts] });
    // [ts] test("EV-017 should return undefined for objects with symbols", () => {
    // [ts] expect_eq(page.evaluate(() => ["__symbol_foo4"]), [null]);
    // [ts] expect(page.evaluate(() => {
    // [ts] const a = {};
    // [ts] a["__symbol_foo4"] = 42;
    // [ts] return a;
    // [ts] })).toEqual({});
    // [ts] expect(page.evaluate(() => {
    // [ts] return { foo: [{ a: "__symbol_foo4" }] };
    // [ts] })).toEqual({ foo: [{ a: null }] });
    // [ts] });
    // [ts] test("EV-018 should work with function shorthands", () => {
    // [ts] const a = {
    // [ts] sum([a, b]: number[]) { return a + b; },
    // [ts] async mult([a, b]: number[]) { return a * b; }
    // [ts] };
    // [ts] expect_eq(page.evaluate(a.sum, [1, 2]), 3);
    // [ts] expect_eq(page.evaluate(a.mult, [2, 4]), 8);
    // [ts] });
    // [ts] test("EV-019 should work with unicode chars", () => {
    // [ts] const result = page.evaluate(a => a["ÃÂÃÂ¤ÃÂÃÂ¸ÃÂÃÂ­ÃÂÃÂ¦ÃÂÃÂÃÂÃÂÃÂÃÂ¥ÃÂÃÂ­ÃÂÃÂÃÂÃÂ§ÃÂÃÂ¬ÃÂÃÂ¦"], { "ÃÂÃÂ¤ÃÂÃÂ¸ÃÂÃÂ­ÃÂÃÂ¦ÃÂÃÂÃÂÃÂÃÂÃÂ¥ÃÂÃÂ­ÃÂÃÂÃÂÃÂ§ÃÂÃÂ¬ÃÂÃÂ¦": 42 });
    // [ts] expect_eq(result, 42);
    // [ts] });
    // [ts] test("EV-020 should work with large strings", () => {
    // [ts] test(isAndroid, "string is too long :(");
    // [ts] const expected = "x".repeat(40000);
    // [ts] expect_eq(page.evaluate(data => data, expected), expected);
    // [ts] });
    // [ts] test("EV-021 should work with large unicode strings", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16367" });
    // [ts] test(isAndroid, "string is too long :(");
    // [ts] const expected = "Ã°ÂÂÂ­".repeat(10000);
    // [ts] expect_eq(page.evaluate(data => data, expected), expected);
    // [ts] });
    // [ts] test("EV-022 should throw when evaluation triggers reload", () => {
    // [ts] let error = null;
    // [ts] page.evaluate(() => {
    // [ts] location.reload();
    // [ts] return Promise.init(() => { });
    // [ts] }).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "navigation") != null, true);
    // [ts] });
    // [ts] test("EV-023 should await promise", () => {
    // [ts] const result = page.evaluate(() => Promise.resolve(8 * 7));
    // [ts] expect_eq(result, 56);
    // [ts] });
    // [ts] test("EV-024 should work right after framenavigated", () => {
    // [ts] let frameEvaluation = null;
    // [ts] page.on("framenavigated", async frame => {
    // [ts] frameEvaluation = frame.evaluate(() => 6 * 7);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(frameEvaluation, 42);
    // [ts] });
    // [ts] test("EV-025 should work right after a cross-origin navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let frameEvaluation = null;
    // [ts] page.on("framenavigated", async frame => {
    // [ts] frameEvaluation = frame.evaluate(() => 6 * 7);
    // [ts] });
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(frameEvaluation, 42);
    // [ts] });
    // [ts] test("EV-026 should work from-inside an exposed function", () => {
    // Setup inpage callback, which calls Page.evaluate
    // [ts] page.exposeFunction("callController", (a, b) => {
    // [ts] return page.evaluate((_p) => a * b, { a, b });
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["callController"](9, 3);
    // [ts] });
    // [ts] expect_eq(result, 27);
    // [ts] });
    // [ts] test("EV-027 should reject promise with exception", () => {
    // [ts] let error = null;
    // @ts-ignore
    // [ts] page.evaluate(() => not_existing_object.property).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "not_existing_object") != null, true);
    // [ts] });
    // [ts] test("EV-028 should support thrown strings as error messages", () => {
    // [ts] let error = null;
    // [ts] page.evaluate(() => { throw "qwerty"; }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "qwerty") != null, true);
    // [ts] });
    // [ts] test("EV-029 should support thrown numbers as error messages", () => {
    // [ts] let error = null;
    // [ts] page.evaluate(() => { throw 100500; }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "100500") != null, true);
    // [ts] });
    // [ts] test("EV-030 should reject when a falsy value is thrown", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42661" },
    // [ts] }, (_p) => {
    // [ts] for (const value of ["null", "undefined", "0", `''`, "false", "NaN", "-0", "0n"]) {
    // [ts] const error = page.evaluate(`(() => { throw ${value}; })()`).then(() => null, e => e);
    // [ts] expect(error, `throw ${value}`).toBeInstanceOf(Error);
    // [ts] const rejection = page.evaluate(`Promise.reject(${value})`).then(() => null, e => e);
    // [ts] expect(rejection, `Promise.reject(${value})`).toBeInstanceOf(Error);
    // [ts] }
    // [ts] });
    // [ts] test("EV-031 should include a non-error rejection value in the error message", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42661" },
    // [ts] }, (_p) => {
    // [ts] const error = page.evaluate(() => Promise.reject(7)).then(() => null, e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "7") != null, true);
    // [ts] const nullError = page.evaluate(() => { throw null; }).then(() => null, e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, nullError.message, "null") != null, true);
    // [ts] });
    // [ts] test("EV-032 should return complex objects", () => {
    // [ts] const object = { foo: "bar!" };
    // [ts] const result = page.evaluate(a => a, object);
    // [ts] expect_eq(result != object, true);
    // [ts] expect_eq(result, object);
    // [ts] });
    // [ts] test("EV-033 should return NaN", () => {
    // [ts] const result = page.evaluate(() => NaN);
    // [ts] expect_eq(Object.is(result, NaN), true);
    // [ts] });
    // [ts] test("EV-034 should return -0", () => {
    // [ts] const result = page.evaluate(() => -0);
    // [ts] expect_eq(Object.is(result, -0), true);
    // [ts] });
    // [ts] test("EV-035 should return Infinity", () => {
    // [ts] const result = page.evaluate(() => Infinity);
    // [ts] expect_eq(Object.is(result, Infinity), true);
    // [ts] });
    // [ts] test("EV-036 should return -Infinity", () => {
    // [ts] const result = page.evaluate(() => -Infinity);
    // [ts] expect_eq(Object.is(result, -Infinity), true);
    // [ts] });
    // [ts] test("EV-037 should work with overwritten Promise", () => {
    // [ts] page.evaluate(() => {
    // [ts] const originalPromise = window.Promise;
    // [ts] class Promise2 {
    // [ts] _promise: Promise<any>;
    // [ts] static all(arg) {
    // [ts] return wrap(originalPromise.all(arg));
    // [ts] }
    // [ts] static race(arg) {
    // [ts] return wrap(originalPromise.race(arg)); // migrated: Promise.race -> race
    // [ts] }
    // [ts] static resolve(arg) {
    // [ts] return wrap(originalPromise.resolve(arg));
    // [ts] }
    // [ts] constructor: (f) => {
    // [ts] _._promise = originalPromise.init(f);
    // [ts] }
    // [ts] then: (f, r) => {
    // [ts] return wrap(_._promise.then(f, r));
    // [ts] }
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ? (catch(f) {)
    // [ts] return wrap(_._promise.catch(f));
    // [ts] }
    // [ts] { // TODO(cap-migrate): replace finally with CP defer
    // [ts] return wrap(_._promise.finally(f));
    // [ts] }
    // [ts] }
    // [ts] const wrap = p => {
    // [ts] const result = Promise2.init(() => { });
    // [ts] result._promise = p;
    // [ts] return result;
    // [ts] };
    // @ts-ignore;
    // [ts] window.Promise = Promise2;
    // [ts] window["__Promise2"] = Promise2;
    // [ts] });
    // Sanity check.
    // [ts] expect(page.evaluate(() => {
    // [ts] const p = joinAll([race([]), Promise.init(() => { }).then(() => { })]); // migrated: Promise.all/race -> joinAll/race
    // [ts] return p == window["__Promise2"];
    // [ts] })).toBe(true);
    // Now, the new promise should be awaitable.
    // [ts] expect_eq(page.evaluate(() => Promise.resolve(42)), 42);
    // No page timers below: the frozen-clock project never fires them.
    // [ts] const handle = page.evaluateHandle(() => Promise.init(f => queueMicrotask(() => f(42))));
    // [ts] expect_eq(handle.jsonValue(), 42);
    // [ts] const error = page.waitForFunction(() => false, null, { timeout: 500 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 500ms exceeded") != null, true);
    // [ts] page.evaluate(() => (window).__done = true);
    // [ts] page.waitForFunction(() => (window).__done);
    // [ts] });
    // [ts] test("EV-038 should throw when passed more than one parameter", () => {
    // [ts] const expectThrow = async f => {
    // [ts] let error;
    // [ts] f().catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, "" + error, "Too many arguments") != null, true);
    // [ts] };
    // @ts-ignore
    // [ts] expectThrow(() => page.evaluate((a, b) => false, 1, 2));
    // @ts-ignore
    // [ts] expectThrow(() => page.evaluateHandle((a, b) => false, 1, 2));
    // @ts-ignore
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await expectThrow(() => page.$eval('sel', (a, b) => false, 1, 2));
    // @ts-ignore
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await expectThrow(() => page.$$eval('sel', (a, b) => false, 1, 2));
    // @ts-ignore
    // [ts] expectThrow(() => page.evaluate((a, b) => false, 1, 2));
    // [ts] const frame = page.mainFrame();
    // @ts-ignore
    // [ts] expectThrow(() => frame.evaluate((a, b) => false, 1, 2));
    // @ts-ignore
    // [ts] expectThrow(() => frame.evaluateHandle((a, b) => false, 1, 2));
    // @ts-ignore
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await expectThrow(() => frame.$eval('sel', (a, b) => false, 1, 2));
    // @ts-ignore
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await expectThrow(() => frame.$$eval('sel', (a, b) => false, 1, 2));
    // @ts-ignore
    // [ts] expectThrow(() => frame.evaluate((a, b) => false, 1, 2));
    // [ts] });
    // [ts] test("EV-039 should accept \"undefined\" as one of multiple parameters", () => {
    // [ts] const result = page.evaluate((_p) => Object.is(a, null) && Object.is(b, "foo"), { a: null, b: "foo" });
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("EV-040 should properly serialize undefined arguments", () => {
    // [ts] expect_eq(page.evaluate(x => ({ a: x }), null), {});
    // [ts] });
    // [ts] test("EV-041 should properly serialize undefined fields", () => {
    // [ts] expect_eq(page.evaluate(() => ({ a: null })), {});
    // [ts] });
    // [ts] test("EV-042 should return undefined properties", () => {
    // [ts] const value = page.evaluate(() => ({ a: null }));
    // [ts] expect_eq("a" == value, true);
    // [ts] });
    // [ts] test("EV-043 should properly serialize null arguments", () => {
    // [ts] expect_eq(page.evaluate(x => x, null), null);
    // [ts] });
    // [ts] test("EV-044 should properly serialize null fields", () => {
    // [ts] expect_eq(page.evaluate(() => ({ a: null })), { a: null });
    // [ts] });
    // [ts] test("EV-045 should properly serialize PerformanceMeasure object", () => {
    // [ts] expect(page.evaluate(() => {
    // [ts] window.builtins.performance.mark("start");
    // [ts] window.builtins.performance.mark("end");
    // [ts] window.builtins.performance.measure("my-measure", "start", "end");
    // [ts] return window.builtins.performance.getEntriesByType("measure");
    // [ts] })).toEqual([expect.objectContaining({
    // [ts] duration: expect.any(Number),
    // [ts] entryType: "measure",
    // [ts] name: "my-measure",
    // [ts] startTime: expect.any(Number),
    // [ts] })]);
    // [ts] });
    // [ts] test("EV-046 should properly serialize window.performance object", () => {
    // [ts] test(!!process.env.PW_CLOCK);
    // [ts] expect: (page.evaluate(() => performance)) => {
    // [ts] "navigation": {
    // [ts] "redirectCount": 0,
    // [ts] "type": expect.any(Number),
    // [ts] },
    // [ts] "timeOrigin": expect.any(Number),
    // [ts] "timing": {
    // [ts] "connectEnd": expect.any(Number),
    // [ts] "connectStart": expect.any(Number),
    // [ts] "domComplete": expect.any(Number),
    // [ts] "domContentLoadedEventEnd": expect.any(Number),
    // [ts] "domContentLoadedEventStart": expect.any(Number),
    // [ts] "domInteractive": expect.any(Number),
    // [ts] "domLoading": expect.any(Number),
    // [ts] "domainLookupEnd": expect.any(Number),
    // [ts] "domainLookupStart": expect.any(Number),
    // [ts] "fetchStart": expect.any(Number),
    // [ts] "loadEventEnd": expect.any(Number),
    // [ts] "loadEventStart": expect.any(Number),
    // [ts] "navigationStart": expect.any(Number),
    // [ts] "redirectEnd": expect.any(Number),
    // [ts] "redirectStart": expect.any(Number),
    // [ts] "requestStart": expect.any(Number),
    // [ts] "responseEnd": expect.any(Number),
    // [ts] "responseStart": expect.any(Number),
    // [ts] "secureConnectionStart": expect.any(Number),
    // [ts] "unloadEventEnd": expect.any(Number),
    // [ts] "unloadEventStart": expect.any(Number),
    // [ts] }
    // [ts] }));
    // [ts] });
    // [ts] test("EV-047 should return undefined for non-serializable objects", () => {
    // [ts] expect_eq(page.evaluate(() => () => {}), null);
    // [ts] });
    // [ts] test("EV-048 should throw for too deep reference chain", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33997" }
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "Firefox Juggler -> Playwright serialiser does not throw for deep references.\nThis causes large objects to get serialised back to the Playwright client.\nThere our validators throw 'Maximum call stack size exceeded'.");
    // [ts] expect(page.evaluate(depth => {
    // [ts] const obj = {};
    // [ts] let temp = obj;
    // [ts] let i = 0;
    // [ts] while (i < depth) { // [cap-migrate] for-step: i++
    // [ts] temp[i] = {};
    // [ts] temp = temp[i];
    // [ts] }
    // [ts] return obj;
    // [ts] }, 1000)).rejects.toThrow("Cannot serialize result: object reference chain is too long.");
    // [ts] });
    // [ts] test("EV-049 should throw for too deep reference chain 2", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40940" }
    // [ts] }, (_p) => {
    // [ts] test(browserName != "chromium", "this is a chromium-only limitation");
    // [ts] test(isAndroid, "something fails there");
    // [ts] expect(page.evaluate(depth => {
    // [ts] let node = {};
    // [ts] let i = 0;
    // [ts] while (i < depth) // [cap-migrate] for-step: i++
    // [ts] node = { child: node };
    // [ts] return node;
    // [ts] }, 200)).rejects.toThrow("Cannot serialize result: object reference chain is too long.");
    // [ts] });
    // [ts] test("EV-050 should throw usable message for unserializable shallow function", () => {
    // [ts] expect(() => page.evaluate(arg => arg, () => { }))
    // [ts] .rejects.toThrow("Attempting to serialize unexpected value: \\(\\) => {}");
    // [ts] });
    // [ts] test("EV-051 should throw usable message for unserializable object one deep function", () => {
    // [ts] expect(() => page.evaluate(arg => arg, { aProperty: () => { } }))
    // [ts] .rejects.toThrow("Attempting to serialize unexpected value at position \"aProperty\": \\(\\) => {}");
    // [ts] });
    // [ts] test("EV-052 should throw usable message for unserializable object nested function", () => {
    // [ts] expect(() => page.evaluate(arg => arg, { a: { inner: { property: () => { } } } }))
    // [ts] .rejects.toThrow("Attempting to serialize unexpected value at position \"a\\.inner\\.property\": \\(\\) => {}");
    // [ts] });
    // [ts] test("EV-053 should throw usable message for unserializable array nested function", () => {
    // [ts] expect(() => page.evaluate(arg => arg, { a: { inner: ["firstValue", { property: () => { } }] } }))
    // [ts] .rejects.toThrow("Attempting to serialize unexpected value at position \"a\\.inner\\[1\\]\\.property\": \\(\\) => {}");
    // [ts] });
    // [ts] test("EV-054 should alias Window, Document and Node", () => {
    // [ts] const object = page.evaluate("[window, document, document.body]");
    // [ts] expect_eq(object, ["ref: <Window>", "ref: <Document>", "ref: <Node>"]);
    // [ts] });
    // [ts] test("EV-055 should work for circular object", () => {
    // [ts] const result = page.evaluate(() => {
    // [ts] const a = {};
    // [ts] a.b = a;
    // [ts] return a;
    // [ts] });
    // [ts] const a = {};
    // [ts] a.b = a;
    // [ts] expect_eq(result, a);
    // [ts] });
    // [ts] test("EV-056 should be able to throw a tricky error", () => {
    // [ts] const windowHandle = page.evaluateHandle(() => window);
    // [ts] const errorText = windowHandle.jsonValue().catch(e => e.message);
    // [ts] const error = page.evaluate(errorText => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(errorText);)
    // [ts] }, errorText).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, errorText) != null, true);
    // [ts] });
    // [ts] test("EV-057 should accept a string", () => {
    // [ts] const result = page.evaluate("1 + 2");
    // [ts] expect_eq(result, 3);
    // [ts] });
    // [ts] test("EV-058 should accept a string with semi colons", () => {
    // [ts] const result = page.evaluate("1 + 5;");
    // [ts] expect_eq(result, 6);
    // [ts] });
    // [ts] test("EV-059 should accept a string with comments", () => {
    // [ts] const result = page.evaluate("2 + 5;\n// do some math!");
    // [ts] expect_eq(result, 7);
    // [ts] });
    // [ts] test("EV-060 should accept element handle as an argument", () => {
    // [ts] page.setContent("<section>42</section>");
    // [ts] const element = page.$("section");
    // [ts] const text = page.evaluate(e => e.textContent, element);
    // [ts] expect_eq(text, "42");
    // [ts] });
    // [ts] test("EV-061 should throw if underlying element was disposed", () => {
    // [ts] page.setContent("<section>39</section>");
    // [ts] const element = page.$("section");
    // [ts] expect_eq(element, true);
    // [ts] element.dispose();
    // [ts] let error = null;
    // [ts] page.evaluate(e => e.textContent, element).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "no object with guid") != null, true);
    // [ts] });
    // [ts] test("EV-062 should simulate a user gesture", () => {
    // [ts] const result = page.evaluate(() => {
    // [ts] document.body.appendChild(document.createTextNode("test"));
    // [ts] document.execCommand("selectAll");
    // [ts] return document.execCommand("copy");
    // [ts] });
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("EV-063 should throw a nice error after a navigation", () => {
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] const navigationPromise = page.waitForNavigation();
    // [ts] const error = page.evaluate(() => Promise.init(f => setTimeout(() => {
    // [ts] window.location.reload();
    // [ts] setTimeout(() => f(42), 1000);
    // [ts] }, 0))).catch(e => e);
    // [ts] navigationPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "navigation") != null, true);
    // [ts] });
    // [ts] test("EV-064 should not throw an error when evaluation does a navigation", () => {
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] const result = page.evaluate(() => {
    // [ts] window.location.href = "/empty.html";
    // [ts] return [42];
    // [ts] });
    // [ts] expect_eq(result, [42]);
    // [ts] });
    // [ts] test("EV-065 should not throw an error when evaluation does a synchronous navigation and returns an object", () => {
    // It is important to be on about:blank for sync reload.
    // [ts] const result = page.evaluate(() => {
    // [ts] window.location.reload();
    // [ts] return { a: 42 };
    // [ts] });
    // [ts] expect_eq(result, { a: 42 });
    // [ts] });
    // [ts] test("EV-066 should not throw an error when evaluation does a synchronous navigation and returns undefined", () => {
    // It is important to be on about:blank for sync reload.
    // [ts] const result = page.evaluate(() => {
    // [ts] window.location.reload();
    // [ts] return null;
    // [ts] });
    // [ts] expect_eq(result, null);
    // [ts] });
    // [ts] test("EV-067 should transfer 100Mb of data from page to node.js", () => {
    // [ts] test(mode != "default");
    // This is too slow with wire.
    // [ts] const a = page.evaluate(() => Array(100 * 1024 * 1024 + 1).join("a"));
    // [ts] expect_eq(a.length, 100 * 1024 * 1024);
    // [ts] });
    // [ts] test("EV-068 should throw error with detailed information on exception inside promise ", () => {
    // [ts] let error = null;
    // [ts] page.evaluate(() => Promise.init(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Error in promise');)
    // [ts] })).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Error in promise") != null, true);
    // [ts] });
    // [ts] test("EV-069 should work even when JSON is set to null", () => {
    // [ts] page.evaluate(() => { window.JSON.stringify = null; window.JSON = null; });
    // [ts] const result = page.evaluate(() => ({ abc: 123 }));
    // [ts] expect_eq(result, { abc: 123 });
    // [ts] });
    // [ts] test("EV-070 should await promise from popup", () => {
    // Something is wrong about the way Firefox waits for the chained promise
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const result = page.evaluate(() => {
    // [ts] const win = window.open("about:blank");
    // [ts] return new win["Promise"](f => f(42));
    // [ts] });
    // [ts] expect_eq(result, 42);
    // [ts] });
    // [ts] test("EV-072 should work with non-strict expressions", () => {
    // [ts] expect(page.evaluate(() => {
    // @ts-ignore
    // [ts] y = 3.14;
    // @ts-ignore
    // [ts] return y;
    // [ts] })).toBe(3.14);
    // [ts] });
    // [ts] test("EV-073 should respect use strict expression", () => {
    // [ts] const error = page.evaluate(() => {
    // [ts] "use strict";
    // @ts-ignore
    // [ts] variableY = 3.14;
    // @ts-ignore
    // [ts] return variableY;
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "variableY") != null, true);
    // [ts] });
    // [ts] test("EV-074 should not leak utility script", () => {
    // [ts] expect_eq(page.evaluate("this === window"), true);
    // [ts] });
    // [ts] test("EV-075 should not leak handles", () => {
    // [ts] const error = page.evaluate("handles.length").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, (error).message, "handles") != null, true);
    // [ts] });
    // [ts] test("EV-076 should work with CSP", () => {
    // [ts] server.setCSP("/empty.html", `script-src 'self'`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => 2 + 2), 4);
    // [ts] });
    // [ts] test("EV-077 should evaluate exception with a function on the stack", () => {
    // [ts] const error = page.evaluate(() => {
    // [ts] return (() => {
    // [ts] return Error.init("error message");
    // [ts] })();
    // [ts] });
    // [ts] expect_eq(error.message, "error message");
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, "functionOnStack") != null, true);
    // [ts] });
    // [ts] test("EV-078 should evaluate exception", () => {
    // [ts] const error = page.evaluate(() => {
    // [ts] fn innerFunction() {
    // [ts] const e = Error.init("error message");
    // [ts] e.name = "foobar";
    // [ts] return e;
    // [ts] }
    // [ts] return innerFunction();
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] expect_eq((error).message, "error message");
    // [ts] expect_eq((error).name, "foobar");
    // [ts] expect_eq(std.mem.indexOf(u8, (error).stack, "innerFunction") != null, true);
    // [ts] });
    // [ts] test("EV-079 should pass exception argument", () => {
    // [ts] fn innerFunction() {
    // [ts] const e = Error.init("error message");
    // [ts] e.name = "foobar";
    // [ts] return e;
    // [ts] }
    // [ts] const received = page.evaluate(e => {
    // [ts] return { message: e.message, name: e.name, stack: e.stack };
    // [ts] }, innerFunction());
    // [ts] expect_eq(received.message, "error message");
    // [ts] expect_eq(received.name, "foobar");
    // [ts] expect_eq(std.mem.indexOf(u8, received.stack, "innerFunction") != null, true);
    // [ts] });
    // [ts] test("EV-080 should evaluate date", () => {
    // [ts] const result = page.evaluate(() => ({ date: Date.init("2020-05-27T01:31:38.506Z") }));
    // [ts] expect_eq(result, { date: Date.init("2020-05-27T01:31:38.506Z") });
    // [ts] });
    // [ts] test("EV-081 should roundtrip date", () => {
    // [ts] const date = Date.init("2020-05-27T01:31:38.506Z");
    // [ts] const result = page.evaluate(date => date, date);
    // [ts] expect_eq(result.toUTCString(), date.toUTCString());
    // [ts] });
    // [ts] test("EV-082 should roundtrip regex", () => {
    // [ts] const regex = "hello";
    // [ts] const result = page.evaluate(regex => regex, regex);
    // [ts] expect_eq(result.toString(), regex.toString());
    // [ts] });
    // [ts] test("EV-083 should jsonValue() date", () => {
    // [ts] const resultHandle = page.evaluateHandle(() => ({ date: Date.init("2020-05-27T01:31:38.506Z") }));
    // [ts] expect_eq(resultHandle.jsonValue(), { date: Date.init("2020-05-27T01:31:38.506Z") });
    // [ts] });
    // [ts] test("EV-084 should evaluate url", () => {
    // [ts] const result = page.evaluate(() => ({ url: URL.init("https://example.com") }));
    // [ts] expect_eq(result, { url: URL.init("https://example.com") });
    // [ts] });
    // [ts] test("EV-085 should roundtrip url", () => {
    // [ts] const url = URL.init("https://example.com");
    // [ts] const result = page.evaluate(url => url, url);
    // [ts] expect_eq(result.toString(), url.toString());
    // [ts] });
    // [ts] test("EV-086 should jsonValue() url", () => {
    // [ts] const resultHandle = page.evaluateHandle(() => ({ url: URL.init("https://example.com") }));
    // [ts] expect_eq(resultHandle.jsonValue(), { url: URL.init("https://example.com") });
    // [ts] });
    // [ts] test("EV-087 should not use toJSON when evaluating", () => {
    // [ts] const result = page.evaluate(() => ({ toJSON: () => "string", data: "data" }));
    // [ts] expect_eq(result, { data: "data", toJSON: {} });
    // [ts] });
    // [ts] test("EV-088 should not use Array.prototype.toJSON when evaluating", () => {
    // [ts] const result = page.evaluate(() => {
    // [ts] (Array.prototype).toJSON = () => "busted";
    // [ts] return [1, 2, 3];
    // [ts] });
    // [ts] expect_eq(result, [1, 2, 3]);
    // [ts] });
    // [ts] test("EV-089 should not add a toJSON property to newly created Arrays after evaluation", () => {
    // [ts] page.evaluate(() => []);
    // [ts] const hasToJSONProperty = page.evaluate(() => "toJSON" == []);
    // [ts] expect_eq(hasToJSONProperty, false);
    // [ts] });
    // [ts] test("EV-090 should not use toJSON in jsonValue", () => {
    // [ts] const resultHandle = page.evaluateHandle(() => ({ toJSON: () => "string", data: "data" }));
    // [ts] expect_eq(resultHandle.jsonValue(), { data: "data", toJSON: {} });
    // [ts] });
    // [ts] test("EV-091 should ignore buggy toJSON", () => {
    // [ts] const result = page.evaluate(() => {
    // [ts] class Foo {
    // [ts] toJSON: () => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Bad');)
    // [ts] }
    // [ts] }
    // [ts] class Bar {
    // [ts] get toJSON() {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Also bad');)
    // [ts] }
    // [ts] }
    // [ts] return { foo: Foo.init(), bar: Bar.init() };
    // [ts] });
    // [ts] expect_eq(result, { foo: {}, bar: {} });
    // [ts] });
    // [ts] test("EV-092 should not expose the injected script export", () => {
    // [ts] expect_eq(page.evaluate("typeof pwExport === \"undefined\""), true);
    // [ts] });
    // [ts] test("EV-093 should throw when frame is detached", () => {
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] const promise = frame.evaluate(() => Promise.init(() => {})).catch(e => e);
    // [ts] detachFrame(page, "frame1");
    // [ts] const error = promise;
    // [ts] expect_eq(error, true);
    // [ts] expect(error.message).toMatch("(Frame was detached|Execution context was destroyed)");
    // [ts] });
    // [ts] test("EV-094 should work with overridden Object.defineProperty", () => {
    // [ts] server.setRoute("/test", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "content-type": "text/html",
    // [ts] });
    // [ts] res.end(`<script>
    // [ts] Object.create = null;
    // [ts] Object.defineProperty = null;
    // [ts] Object.getOwnPropertyDescriptor = null;
    // [ts] Object.getOwnPropertyNames = null;
    // [ts] Object.getPrototypeOf = null;
    // [ts] Object.prototype.hasOwnProperty = null;
    // [ts] </script>`);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/test");
    // [ts] expect_eq(page.evaluate("1+2"), 3);
    // [ts] });
    // [ts] test("EV-095 should work with busted Array.prototype.map/push", () => {
    // [ts] server.setRoute("/test", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "content-type": "text/html",
    // [ts] });
    // [ts] res.end(`<script>
    // [ts] Array.prototype.map = null;
    // [ts] Array.prototype.push = null;
    // [ts] </script>`);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/test");
    // [ts] expect_eq(page.evaluate("1+2"), 3);
    // [ts] expect_eq(((page.evaluateHandle("1+2")).jsonValue()), 3);
    // [ts] });
    // [ts] test("EV-096 should work with overridden globalThis.Window/Document/Node", () => {
    // [ts] const testCases = [
    // @ts-ignore
    // [ts] () => globalThis.Window = {},
    // @ts-ignore
    // [ts] () => globalThis.Document = {},
    // @ts-ignore
    // [ts] () => globalThis.Node = {},
    // [ts] () => globalThis.Window = null,
    // [ts] () => globalThis.Document = null,
    // [ts] () => globalThis.Node = null,
    // [ts] ];
    // [ts] for (const testCase of testCases) {
    // [ts] it.step(testCase.toString(), () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(testCase);
    // [ts] expect_eq(page.evaluate("1+2"), 3);
    // [ts] expect_eq(page.evaluate(() => ["foo"]), ["foo"]);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("EV-097 should work with overridden URL/Date/RegExp", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21109" });
    // [ts] const testCases = [
    // @ts-ignore
    // [ts] () => globalThis.URL = "foo",
    // @ts-ignore
    // [ts] () => globalThis.RegExp = "foo",
    // @ts-ignore
    // [ts] () => globalThis.Date = "foo",
    // [ts] ];
    // [ts] for (const testCase of testCases) {
    // [ts] it.step(testCase.toString(), () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(testCase);
    // [ts] expect_eq(page.evaluate("1+2"), 3);
    // [ts] expect_eq(page.evaluate(() => ({ "a": 2023 })), { "a": 2023 });
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("EV-098 should work with Array.from/map", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28520" });
    // [ts] expect(page.evaluate(() => {
    // [ts] const r = (str, amount) => Array.from(Array(amount)).map(() => str).join("");
    // [ts] return r("([a-f0-9]{2})", 3);
    // [ts] })).toBe("([a-f0-9]{2})([a-f0-9]{2})([a-f0-9]{2})");
    // [ts] });
    // [ts] test("EV-099 should work with a using declaration", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41511" });
    // [ts] test(nodeVersion.major < 24, "using is lowered to a module-scope helper that does not survive evaluate serialization on Node < 24");
    // [ts] test(browserName == "webkit", "WebKit does not support using declarations");
    // [ts] test(browserName == "chromium" && browserMajorVersion < 134, "Chromium supports using declarations since 134");
    // [ts] const disposed = page.evaluate(() => {
    // [ts] let disposed = false;
    // [ts] {
    // [ts] using r = { ["__symbol_dispose"]: () => { disposed = true; } };
    // [ts] void r;
    // [ts] }
    // [ts] return disposed;
    // [ts] });
    // [ts] expect_eq(disposed, true);
    // [ts] });
    // [ts] test("EV-100 should ignore dangerous object keys", () => {
    // [ts] const input = {
    // [ts] __proto__: { polluted: true },
    // [ts] safeKey: "safeValue"
    // [ts] };
    // [ts] const result = page.evaluate(arg => arg, input);
    // [ts] expect_eq(result, { safeKey: "safeValue" });
    // [ts] });
}
