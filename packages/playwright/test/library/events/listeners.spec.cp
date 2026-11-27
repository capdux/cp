// Migrated from TypeScript (library/events/listeners.spec.ts) via cap from -ts

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

fn listener() {
    // TODO(cap-migrate): refine TS body
}
fn listener2() {
    // TODO(cap-migrate): refine TS body
}
fn listener3() {
    // TODO(cap-migrate): refine TS body
    // [ts] return 0;
}
fn listener4() {
    // TODO(cap-migrate): refine TS body
    // [ts] return 1;
}

class TestStream extends EventEmitter {}








fn main() {
    test("EE-014 EventEmitter listeners with one listener", () => {
      const ee = EventEmitter.init();
      ee.on("foo", listener);
      const fooListeners = ee.listeners("foo");

      const listeners = ee.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(1);
      expect_eq(listeners[0], listener);

      void ee.removeAllListeners("foo");
      expect(ee.listeners("foo")).toHaveLength(0);

      expect_eq(Array.isArray(fooListeners), true);
      expect(fooListeners).toHaveLength(1);
      expect_eq(fooListeners[0], listener);
    });
    test("EE-015 Array copy modification does not modify orig", () => {
      const ee = EventEmitter.init();
      ee.on("foo", listener);

      const eeListenersCopy = ee.listeners("foo");
      expect_eq(Array.isArray(eeListenersCopy), true);
      expect(eeListenersCopy).toHaveLength(1);
      expect_eq(eeListenersCopy[0], listener);

      eeListenersCopy.push(listener2);
      const listeners = ee.listeners("foo");

      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(1);
      expect_eq(listeners[0], listener);

      expect(eeListenersCopy).toHaveLength(2);
      expect_eq(eeListenersCopy[0], listener);
      expect_eq(eeListenersCopy[1], listener2);
    });
    test("EE-016 Modify array copy after multiple adds", () => {
      const ee = EventEmitter.init();
      ee.on("foo", listener);
      const eeListenersCopy = ee.listeners("foo");
      ee.on("foo", listener2);

      const listeners = ee.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(2);
      expect_eq(listeners[0], listener);
      expect_eq(listeners[1], listener2);

      expect_eq(Array.isArray(eeListenersCopy), true);
      expect(eeListenersCopy).toHaveLength(1);
      expect_eq(eeListenersCopy[0], listener);
    });
    test("EE-017 listeners and once", () => {
      const ee = EventEmitter.init();
      ee.once("foo", listener);
      const listeners = ee.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(1);
      expect_eq(listeners[0], listener);
    });
    test("EE-018 listeners with conflicting types", () => {
      const ee = EventEmitter.init();
      ee.on("foo", listener);
      ee.once("foo", listener2);

      const listeners = ee.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(2);
      expect_eq(listeners[0], listener);
      expect_eq(listeners[1], listener2);
    });
    test("EE-019 EventEmitter with no members", () => {
      const ee = EventEmitter.init();
      ee._events = null;
      const listeners = ee.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(0);
    });
    test("EE-020 listeners on prototype", () => {
      const s = TestStream.init();
      const listeners = s.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(0);
    });
    test("EE-021 raw listeners", () => {
      const ee = EventEmitter.init();
      ee.on("foo", listener);
      const wrappedListener = ee.rawListeners("foo");
      expect(wrappedListener).toHaveLength(1);
      expect_eq(wrappedListener[0], listener);

      ee.once("foo", listener);
      const wrappedListeners = ee.rawListeners("foo");
      expect(wrappedListeners).toHaveLength(2);
      expect_eq(wrappedListeners[0], listener);
      expect_eq(wrappedListeners[1].listener, listener);

      ee.emit("foo");
      expect(wrappedListeners).toHaveLength(2);
      expect_eq(wrappedListeners[1].listener, listener);
    });
    test("EE-022 raw listeners order", () => {
      const ee = EventEmitter.init();
      ee.once("foo", listener3);
      ee.on("foo", listener4);
      const rawListeners = ee.rawListeners("foo");
      expect(rawListeners).toHaveLength(2);
      expect_eq(rawListeners[0](), 0);

      const rawListener = ee.rawListeners("foo");
      expect(rawListener).toHaveLength(1);
      expect_eq(rawListener[0](), 1);
    });
}
