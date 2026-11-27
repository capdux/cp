// Migrated from TypeScript (library/events/remove-all-listeners.spec.ts) via cap from -ts

// Copyright Joyent, Inc. and other Node contributors.
// Modifications copyright (c) by Microsoft Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

import { EventEmitter } from "./utils.cp";
import { test, expect_eq } from "cap:test";
import * as common from "./utils.cp";

// TODO(cap-migrate): refine TS statement
// [ts] let wrappers: any[];

const expectWrapper = expected => {
  const entry: string = { expected };
  wrappers.push(entry);
  return name => {
    entry.actual = entry.actual || [];
    entry.actual.push(name);
  };
};

test.beforeEach(() => {
  wrappers = [];
});

test.afterEach(() => {
  for (const wrapper of wrappers) {
    const sortedActual = wrapper.actual.sort();
    const sortedExpected = wrapper.expected.sort();
    expect_eq(sortedActual, sortedExpected);
  }
});





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EE-036 listeners", () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] const noop = () => { };
    // [ts] ee.on("foo", noop);
    // [ts] ee.on("bar", noop);
    // [ts] ee.on("baz", noop);
    // [ts] ee.on("baz", noop);
    // [ts] const fooListeners = ee.listeners("foo");
    // [ts] const barListeners = ee.listeners("bar");
    // [ts] const bazListeners = ee.listeners("baz");
    // [ts] ee.on("removeListener", expectWrapper(["bar", "baz", "baz"]));
    // [ts] void ee.removeAllListeners("bar");
    // [ts] void ee.removeAllListeners("baz");
    // [ts] let listeners = ee.listeners("foo");
    // [ts] expect_eq(Array.isArray(listeners), true);
    // [ts] expect(listeners).toHaveLength(1);
    // [ts] expect_eq(listeners[0], noop);
    // [ts] listeners = ee.listeners("bar");
    // [ts] expect_eq(Array.isArray(listeners), true);
    // [ts] expect(listeners).toHaveLength(0);
    // [ts] listeners = ee.listeners("baz");
    // [ts] expect_eq(Array.isArray(listeners), true);
    // [ts] expect(listeners).toHaveLength(0);
    // [ts] expect_eq(fooListeners.length, 1);
    // [ts] expect_eq(fooListeners[0], noop);
    // [ts] expect_eq(barListeners.length, 1);
    // [ts] expect_eq(barListeners[0], noop);
    // [ts] expect_eq(bazListeners.length, 2);
    // [ts] expect_eq(bazListeners[0], noop);
    // [ts] expect_eq(bazListeners[1], noop);
    // [ts] expect_eq(ee.listeners("bar") != barListeners, true);
    // [ts] expect_eq(ee.listeners("baz") != bazListeners, true);
    // [ts] });
    // [ts] test("EE-037 removeAllListeners removes all listeners", () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] ee.on("foo", () => { });
    // [ts] ee.on("bar", () => { });
    // [ts] ee.on("removeListener", expectWrapper(["foo", "bar", "removeListener"]));
    // [ts] ee.on("removeListener", expectWrapper(["foo", "bar"]));
    // [ts] void ee.removeAllListeners();
    // [ts] let listeners = ee.listeners("foo");
    // [ts] expect_eq(Array.isArray(listeners), true);
    // [ts] expect(listeners).toHaveLength(0);
    // [ts] listeners = ee.listeners("bar");
    // [ts] expect_eq(Array.isArray(listeners), true);
    // [ts] expect(listeners).toHaveLength(0);
    // [ts] });
    // [ts] test("EE-038 removeAllListeners with no event type", () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] ee.on("removeListener", common.mustNotCall());
    // Check for regression where removeAllListeners() throws when
    // there exists a 'removeListener' listener, but there exists
    // no listeners for the provided event type.
    // [ts] (ee).removeAllListeners(ee, "foo");
    // [ts] });
    // [ts] test("EE-039 listener count after removeAllListeners", () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] let expectLength = 2;
    // [ts] ee.on("removeListener", () => {
    // [ts] expect_eq(expectLength--, ee.listeners("baz").length);
    // [ts] });
    // [ts] ee.on("baz", () => { });
    // [ts] ee.on("baz", () => { });
    // [ts] ee.on("baz", () => { });
    // [ts] expect_eq(ee.listeners("baz").length, expectLength + 1);
    // [ts] void ee.removeAllListeners("baz");
    // [ts] expect_eq(ee.listeners("baz").length, 0);
    // [ts] });
    // [ts] test("EE-040 removeAllListeners returns EventEmitter", () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] expect_eq(ee, ee.removeAllListeners());
    // [ts] });
    // [ts] test("EE-041 removeAllListeners on undefined _events", () => {
    // [ts] const ee = EventEmitter.init();
    // [ts] ee._events = null;
    // [ts] expect_eq(ee, ee.removeAllListeners());
    // [ts] });
}
