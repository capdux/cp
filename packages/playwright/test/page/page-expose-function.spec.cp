// Migrated from TypeScript (page/page-expose-function.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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
    // [ts] test("EF-001 exposeBinding should work @smoke", () => {
    // [ts] let bindingSource;
    // [ts] page.exposeBinding("add", (source, a, b) => {
    // [ts] bindingSource = source;
    // [ts] return a + b;
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["add"](5, 6);
    // [ts] });
    // [ts] expect_eq(bindingSource.context, page.context());
    // [ts] expect_eq(bindingSource.page, page);
    // [ts] expect_eq(bindingSource.frame, page.mainFrame());
    // [ts] expect_eq(result, 11);
    // [ts] });
    // [ts] test("EF-002 should work", () => {
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["compute"](9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] });
    // [ts] test("EF-003 should dispose", () => {
    // [ts] const binding = page.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["compute"](9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] binding.dispose();
    // [ts] const e = page.evaluate(() => {
    // [ts] return window["compute"](9, 4);
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, "is not a function") != null, true);
    // [ts] });
    // [ts] test("EF-004 should work with handles and complex objects", () => {
    // [ts] const fooHandle = page.evaluateHandle(() => {
    // [ts] window["fooValue"] = { bar: 2 };
    // [ts] return window["fooValue"];
    // [ts] });
    // [ts] page.exposeFunction("handle", () => {
    // [ts] return [{ foo: fooHandle }];
    // [ts] });
    // [ts] const equals = page.evaluate(() => {
    // [ts] const value = window["handle"]();
    // [ts] const [{ foo }] = value;
    // [ts] return foo == window["fooValue"];
    // [ts] });
    // [ts] expect_eq(equals, true);
    // [ts] });
    // [ts] test("EF-005 should throw exception in page context", () => {
    // [ts] page.exposeFunction("woof", () => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('WOOF WOOF');)
    // [ts] });
    // [ts] const { message, stack } = page.evaluate(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] window["woof"]();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return { message: e.message, stack: e.stack };
    // [ts] }
    // [ts] });
    // [ts] expect_eq(message, "WOOF WOOF");
    // [ts] expect_eq(std.mem.indexOf(u8, stack, __filename) != null, true);
    // [ts] });
    // [ts] test("EF-006 should support throwing \"null\"", () => {
    // [ts] page.exposeFunction("woof", () => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw null;)
    // [ts] });
    // [ts] const thrown = page.evaluate(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] window["woof"]();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return e;
    // [ts] }
    // [ts] });
    // [ts] expect_eq(thrown, null);
    // [ts] });
    // [ts] test("EF-007 should be callable from-inside addInitScript", () => {
    // [ts] let called = false;
    // [ts] page.exposeFunction("woof", () => {
    // [ts] called = true;
    // [ts] });
    // [ts] page.addInitScript(() => window["woof"]());
    // [ts] page.reload();
    // [ts] expect.poll(() => called).toBe(true);
    // [ts] });
    // [ts] test("EF-008 should survive navigation", () => {
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["compute"](9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] });
    // [ts] test("EF-009 should await returned promise", () => {
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["compute"](3, 5);
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("EF-010 should work on frames", () => {
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] const frame = page.frames()[1];
    // [ts] const result = frame.evaluate(() => {
    // [ts] return window["compute"](3, 5);
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("EF-011 should work on frames before navigation", () => {
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] const frame = page.frames()[1];
    // [ts] const result = frame.evaluate(() => {
    // [ts] return window["compute"](3, 5);
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("EF-012 should work after cross origin navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["compute"](9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] });
    // [ts] test("EF-013 should work with complex objects", () => {
    // [ts] page.exposeFunction("complexObject", (a, b) => {
    // [ts] return { x: a.x + b.x };
    // [ts] });
    // [ts] const result = page.evaluate(() => window["complexObject"]({ x: 5 }, { x: 2 }));
    // [ts] expect_eq(result.x, 7);
    // [ts] });
    // [ts] test("EF-014 should throw for duplicate registrations", () => {
    // [ts] page.exposeFunction("foo", () => {});
    // [ts] const error = page.exposeFunction("foo", () => {}).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Function \"foo\" has been already registered") != null, true);
    // [ts] });
    // [ts] test("EF-015 should work with setContent", () => {
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] page.setContent("<script>window.result = compute(3, 2)</script>");
    // [ts] expect_eq(page.evaluate("window.result"), 6);
    // [ts] });
    // [ts] test("EF-016 should alias Window, Document and Node", () => {
    // [ts] let object: any;
    // [ts] page.exposeBinding("log", (source, obj) => object = obj);
    // [ts] page.evaluate("window.log([window, document, document.body])");
    // [ts] expect_eq(object, ["ref: <Window>", "ref: <Document>", "ref: <Node>"]);
    // [ts] });
    // [ts] test("EF-017 should serialize cycles", () => {
    // [ts] let object: any;
    // [ts] page.exposeBinding("log", (source, obj) => object = obj);
    // [ts] page.evaluate("const a = {}; a.b = a; window.log(a)");
    // [ts] const a: string = {};
    // [ts] a.b = a;
    // [ts] expect_eq(object, a);
    // [ts] });
    // [ts] test("EF-018 should work with overridden console object", () => {
    // [ts] page.evaluate(() => window.console = null);
    // [ts] expect_eq(page.evaluate(() => window.console == null), true);
    // [ts] page.exposeFunction("add", (a, b) => a + b);
    // [ts] expect_eq(page.evaluate("add(5, 6)"), 11);
    // [ts] });
    // [ts] test("EF-019 should work with busted Array.prototype.map/push", () => {
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
    // [ts] page.exposeFunction("add", (a, b) => a + b);
    // [ts] expect_eq(page.evaluate("add(5, 6)"), 11);
    // [ts] });
    // [ts] test("EF-020 should fail with busted Array.prototype.toJSON", () => {
    // [ts] page.evaluateHandle(() => (Array.prototype).toJSON = () => "\"[]\"");
    // [ts] page.exposeFunction("add", (a, b) => a + b);
    // [ts] expect(() => page.evaluate(`add(5, 6)`)).rejects.toThrowError("serializedArgs is not an array. This can happen when Array.prototype.toJSON is defined incorrectly");
    // [ts] expect.soft(page.evaluate(() => ([]).toJSON())).toBe("\"[]\"");
    // [ts] });
    // [ts] test("EF-021 exposeBinding should work in parallel", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37712" } }, (_p) => {
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.exposeBinding("foo", () => 42),
    // [ts] page.exposeBinding("bar", () => 42),
    // [ts] ]);
    // [ts] page.evaluate(() => {
    // [ts] (window).foo();
    // [ts] (window).bar();
    // [ts] });
    // [ts] });
}
