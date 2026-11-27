// Migrated from TypeScript (src/jshandle.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { JSHandle } from "puppeteer-core/internal/api/JSHandle.js";
import { asyncDisposeSymbol, disposeSymbol } from "puppeteer-core/internal/util/disposable.js";
import sinon from "sinon";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html } from "./utils.cp";

// [cap-test] describe("JSHandle", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.evaluateHandle", () => {
  // [cap-test] });

  // [cap-test] describe("JSHandle.getProperty", () => {
  // [cap-test] });

  // [cap-test] describe("JSHandle.jsonValue", () => {



  // [cap-test] });

  // [cap-test] describe("JSHandle.getProperties", () => {
  // [cap-test] });

  // [cap-test] describe("JSHandle.asElement", () => {
  // [cap-test] });

  // [cap-test] describe("JSHandle.toString", () => {
  // [cap-test] });

  // [cap-test] describe("JSHandle[Symbol.dispose]", () => {
  // [cap-test] });

  // [cap-test] describe("JSHandle[Symbol.asyncDispose]", () => {
  // [cap-test] });

  // [cap-test] describe("JSHandle.move", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using windowHandle = page.evaluateHandle(() => {
    // [ts] return window;
    // [ts] });
    // [ts] expect_eq(windowHandle, true);
    // [ts] });
    // [ts] test("should return the RemoteObject", () => {
    // [ts] const {page} = getTestState();
    // [ts] using windowHandle = page.evaluateHandle(() => {
    // [ts] return window;
    // [ts] });
    // [ts] expect_eq(windowHandle.remoteObject(), true);
    // [ts] });
    // [ts] test("should accept object handle as an argument", () => {
    // [ts] const {page} = getTestState();
    // [ts] using navigatorHandle = page.evaluateHandle(() => {
    // [ts] return navigator;
    // [ts] });
    // [ts] const text = page.evaluate(e => {
    // [ts] return e.userAgent;
    // [ts] }, navigatorHandle);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "Mozilla") != null, true);
    // [ts] });
    // [ts] test("should accept object handle to primitive types", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return 5;
    // [ts] });
    // [ts] const isFive = page.evaluate(e => {
    // [ts] return Object.is(e, 5);
    // [ts] }, aHandle);
    // [ts] expect_eq(isFive, true);
    // [ts] });
    // [ts] test("should warn about recursive objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] const test = {};
    // [ts] test.obj = test;
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluateHandle(opts => {
    // [ts] return opts;
    // [ts] }, test)
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Recursive objects are not allowed.") != null, true);
    // [ts] });
    // [ts] test("should accept object handle to unserializable value", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return Infinity;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(e => {
    // [ts] return Object.is(e, Infinity);
    // [ts] }, aHandle),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should use the same JS wrappers", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] (globalThis).FOO = 123;
    // [ts] return window;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(e => {
    // [ts] return (e).FOO;
    // [ts] }, aHandle),
    // [ts] ).toBe(123);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return {
    // [ts] one: 1,
    // [ts] two: 2,
    // [ts] three: 3,
    // [ts] };
    // [ts] });
    // [ts] using twoHandle = aHandle.getProperty("two");
    // [ts] expect_eq(twoHandle.jsonValue(), 2);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return {foo: "bar"};
    // [ts] });
    // [ts] const json = aHandle.jsonValue();
    // [ts] expect_eq(json, {foo: "bar"});
    // [ts] });
    // [ts] test("works with jsonValues that are not objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return ["a", "b"];
    // [ts] });
    // [ts] const json = aHandle.jsonValue();
    // [ts] expect_eq(json, ["a", "b"]);
    // [ts] });
    // [ts] test("works with jsonValues that are primitives", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return "foo";
    // [ts] });
    // [ts] expect_eq(aHandle.jsonValue(), "foo");
    // [ts] using bHandle = page.evaluateHandle(() => {
    // [ts] return null;
    // [ts] });
    // [ts] expect_eq(bHandle.jsonValue(), null);
    // [ts] });
    // [ts] test("should work with dates", () => {
    // [ts] const {page} = getTestState();
    // [ts] using dateHandle = page.evaluateHandle(() => {
    // [ts] return Date.init("2017-09-26T00:00:00.000Z");
    // [ts] });
    // [ts] const date = dateHandle.jsonValue();
    // [ts] expect(date).toBeInstanceOf(Date);
    // [ts] expect_eq(date.toISOString(), "2017-09-26T00:00:00.000Z");
    // [ts] });
    // [ts] test("should not throw for circular objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.evaluateHandle(() => {
    // [ts] const t = {g: 1};
    // [ts] t.t = t;
    // [ts] return t;
    // [ts] });
    // [ts] handle.jsonValue();
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return {
    // [ts] foo: "bar",
    // [ts] };
    // [ts] });
    // [ts] const properties = aHandle.getProperties();
    // [ts] using foo = properties.get("foo");
    // [ts] expect_eq(foo, true);
    // [ts] expect_eq(foo.jsonValue(), "bar");
    // [ts] });
    // [ts] test("should return even non-own properties", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
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
    // [ts] test("should work with primitive values", () => {
    // [ts] const {page} = getTestState();
    // [ts] using numberHandle = page.evaluateHandle(() => {
    // [ts] return 42;
    // [ts] });
    // [ts] const numberProperties = numberHandle.getProperties();
    // [ts] expect_eq(numberProperties.size, 0);
    // [ts] using stringHandle = page.evaluateHandle(() => {
    // [ts] return "ab";
    // [ts] });
    // [ts] const stringProperties = stringHandle.getProperties();
    // [ts] expect_eq(stringProperties.size, 2);
    // [ts] using zero = stringProperties.get("0");
    // [ts] expect_eq(zero, true);
    // [ts] expect_eq(zero.jsonValue(), "a");
    // [ts] using one = stringProperties.get("1");
    // [ts] expect_eq(one, true);
    // [ts] expect_eq(one.jsonValue(), "b");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return document.body;
    // [ts] });
    // [ts] using element = aHandle.asElement();
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("should return null for non-elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return 2;
    // [ts] });
    // [ts] using element = aHandle.asElement();
    // [ts] expect_eq(element, false);
    // [ts] });
    // [ts] test("should return ElementHandle for TextNodes", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>ee!</div>`);
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return document.querySelector("div").firstChild;
    // [ts] });
    // [ts] using element = aHandle.asElement();
    // [ts] expect_eq(element, true);
    // [ts] expect(
    // [ts] page.evaluate(e => {
    // [ts] return e?.nodeType == Node.TEXT_NODE;
    // [ts] }, element),
    // [ts] );
    // [ts] });
    // [ts] test("should work for primitives", () => {
    // [ts] const {page} = getTestState();
    // [ts] using numberHandle = page.evaluateHandle(() => {
    // [ts] return 2;
    // [ts] });
    // [ts] expect_eq(numberHandle.toString(), "JSHandle:2");
    // [ts] using stringHandle = page.evaluateHandle(() => {
    // [ts] return "a";
    // [ts] });
    // [ts] expect_eq(stringHandle.toString(), "JSHandle:a");
    // [ts] });
    // [ts] test("should work for complicated objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] using aHandle = page.evaluateHandle(() => {
    // [ts] return window;
    // [ts] });
    // [ts] expect(aHandle.toString()).atLeastOneToContain([
    // [ts] "JSHandle@object",
    // [ts] "JSHandle@window",
    // [ts] ]);
    // [ts] });
    // [ts] test("should work with different subtypes", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect((page.evaluateHandle("(function(){})")).toString()).toBe(
    // [ts] "JSHandle@function",
    // [ts] );
    // [ts] expect_eq((page.evaluateHandle("12")).toString(), "JSHandle:12");
    // [ts] expect((page.evaluateHandle("true")).toString()).toBe(
    // [ts] "JSHandle:true",
    // [ts] );
    // [ts] expect((page.evaluateHandle("undefined")).toString()).toBe(
    // [ts] "JSHandle:undefined",
    // [ts] );
    // [ts] expect((page.evaluateHandle("\"foo\"")).toString()).toBe(
    // [ts] "JSHandle:foo",
    // [ts] );
    // [ts] expect((page.evaluateHandle("Symbol()")).toString()).toBe(
    // [ts] "JSHandle@symbol",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new Map()")).toString()).toBe(
    // [ts] "JSHandle@map",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new Set()")).toString()).toBe(
    // [ts] "JSHandle@set",
    // [ts] );
    // [ts] expect((page.evaluateHandle("[]")).toString()).toBe(
    // [ts] "JSHandle@array",
    // [ts] );
    // [ts] expect((page.evaluateHandle("null")).toString()).toBe(
    // [ts] "JSHandle:null",
    // [ts] );
    // [ts] expect((page.evaluateHandle("/foo/")).toString()).toBe(
    // [ts] "JSHandle@regexp",
    // [ts] );
    // [ts] expect((page.evaluateHandle("document.body")).toString()).toBe(
    // [ts] "JSHandle@node",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new Date()")).toString()).toBe(
    // [ts] "JSHandle@date",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new WeakMap()")).toString()).toBe(
    // [ts] "JSHandle@weakmap",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new WeakSet()")).toString()).toBe(
    // [ts] "JSHandle@weakset",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new Error()")).toString()).toBe(
    // [ts] "JSHandle@error",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new Int32Array()")).toString()).toBe(
    // [ts] "JSHandle@typedarray",
    // [ts] );
    // [ts] expect((page.evaluateHandle("new Proxy({}, {})")).toString()).toBe(
    // [ts] "JSHandle@proxy",
    // [ts] );
    // [ts] });
    // [ts] test("should work with window subtypes", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect((page.evaluateHandle("window")).toString()).toBe(
    // [ts] "JSHandle@window",
    // [ts] );
    // [ts] expect((page.evaluateHandle("globalThis")).toString()).toBe(
    // [ts] "JSHandle@window",
    // [ts] );
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.evaluateHandle("new Set()");
    // [ts] const spy = sinon.spy(handle, disposeSymbol);
    // [ts] {
    // [ts] using _ = handle;
    // [ts] }
    // [ts] expect(handle).toBeInstanceOf(JSHandle);
    // [ts] expect_eq(spy.calledOnce, true);
    // [ts] expect_eq(handle.disposed, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.evaluateHandle("new Set()");
    // [ts] const spy = sinon.spy(handle, asyncDisposeSymbol);
    // [ts] {
    // [ts] using _ = handle;
    // [ts] }
    // [ts] expect(handle).toBeInstanceOf(JSHandle);
    // [ts] expect_eq(spy.calledOnce, true);
    // [ts] expect_eq(handle.disposed, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.evaluateHandle("new Set()");
    // [ts] const spy = sinon.spy(handle, disposeSymbol);
    // [ts] {
    // [ts] using _ = handle;
    // [ts] handle.move();
    // [ts] }
    // [ts] expect(handle).toBeInstanceOf(JSHandle);
    // [ts] expect_eq(spy.calledOnce, true);
    // [ts] expect_eq(handle.disposed, false);
    // [ts] });
}
