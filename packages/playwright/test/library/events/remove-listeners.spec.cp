// Migrated from TypeScript (library/events/remove-listeners.spec.ts) via cap from -ts

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

fn listener1() {
    // TODO(cap-migrate): refine TS body
}
fn listener2() {
    // TODO(cap-migrate): refine TS body
}

test.beforeEach(() => common.beforeEach());
test.afterEach(() => common.afterEach());








// Verify that the removed listener must be a function

fn main() {
    test("EE-042 First test", () => {
      const ee = EventEmitter.init();
      ee.on("hello", listener1);
      ee.on("removeListener", common.mustCall((name, cb) => {
        expect_eq(name, "hello");
        expect_eq(cb, listener1);
      }));
      ee.removeListener("hello", listener1);
      const listeners = ee.listeners("hello");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(0);
    });
    test("EE-043 Second test", () => {
      const ee = EventEmitter.init();
      ee.on("hello", listener1);
      ee.on("removeListener", common.mustNotCall());
      ee.removeListener("hello", listener2);

      const listeners = ee.listeners("hello");
      expect_eq(Array.isArray(listeners), true);
      expect_eq(listeners.length, 1);
      expect_eq(listeners[0], listener1);
    });
    test("EE-044 Third test", () => {
      const ee = EventEmitter.init();
      ee.on("hello", listener1);
      ee.on("hello", listener2);

      let listeners;
      ee.once("removeListener", common.mustCall((name, cb) => {
        expect_eq(name, "hello");
        expect_eq(cb, listener1);
        listeners = ee.listeners("hello");
        expect_eq(Array.isArray(listeners), true);
        expect_eq(listeners.length, 1);
        expect_eq(listeners[0], listener2);
      }));
      ee.removeListener("hello", listener1);
      listeners = ee.listeners("hello");
      expect_eq(Array.isArray(listeners), true);
      expect_eq(listeners.length, 1);
      expect_eq(listeners[0], listener2);
      ee.once("removeListener", common.mustCall((name, cb) => {
        expect_eq(name, "hello");
        expect_eq(cb, listener2);
        listeners = ee.listeners("hello");
        expect_eq(Array.isArray(listeners), true);
        expect_eq(listeners.length, 0);
      }));
      ee.removeListener("hello", listener2);
      listeners = ee.listeners("hello");
      expect_eq(Array.isArray(listeners), true);
      expect_eq(listeners.length, 0);
    });
    test("EE-045 Fourth test", () => {
      const ee = EventEmitter.init();

      const remove1 = () => {
        return; // TODO(cap-migrate): throw -> error return (throw new Error('remove1 should not have been called');)
      };

      const remove2 = () => {
        return; // TODO(cap-migrate): throw -> error return (throw new Error('remove2 should not have been called');)
      };

      ee.on("removeListener", common.mustCall((name, cb) => {
        if (cb != remove1)
          return;
        ee.removeListener("quux", remove2);
        ee.emit("quux");
      }, 2));
      ee.on("quux", remove1);
      ee.on("quux", remove2);
      ee.removeListener("quux", remove1);
    });
    test("EE-046 Fifth test", () => {
      const ee = EventEmitter.init();
      const listener3 = common.mustCall(() => {
        ee.removeListener("hello", listener4);
      }, 2);
      const listener4 = common.mustCall();

      ee.on("hello", listener3);
      ee.on("hello", listener4);

      // listener4 will still be called although it is removed by listener 3.
      ee.emit("hello");
      // This is so because the interal listener array at time of emit
      // was [listener3,listener4]

      // Interal listener array [listener3]
      ee.emit("hello");
    });
    test("EE-047 Sixth test", () => {
      const ee = EventEmitter.init();

      ee.once("hello", listener1);
      ee.on("removeListener", common.mustCall((eventName, listener) => {
        expect_eq(eventName, "hello");
        expect_eq(listener, listener1);
      }));
      ee.emit("hello");
    });
    test("EE-048 Seventh test", () => {
      const ee = EventEmitter.init();

      expect_eq(ee, ee.removeListener("foo", () => {}));
    });
    test("EE-049 Eighth test", () => {
      expect(() => {
        const ee = EventEmitter.init();

        ee.removeListener("foo", null);
      }).toThrow("^The \"listener\" argument must be of type Function\\. Received type object$");
    });
    test("EE-050 Ninth test", () => {
      const ee = EventEmitter.init();
      const listener = () => {};
      ee._events = null;
      const e = ee.removeListener("foo", listener);
      expect_eq(e, ee);
    });
    test("EE-051 Tenth test", () => {
      const ee = EventEmitter.init();

      ee.on("foo", listener1);
      ee.on("foo", listener2);
      let listeners = ee.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect_eq(listeners.length, 2);
      expect_eq(listeners[0], listener1);
      expect_eq(listeners[1], listener2);

      ee.removeListener("foo", listener1);
      expect_eq(ee._events.foo, listener2);

      ee.on("foo", listener1);
      listeners = ee.listeners("foo");
      expect_eq(Array.isArray(listeners), true);
      expect_eq(listeners.length, 2);
      expect_eq(listeners[0], listener2);
      expect_eq(listeners[1], listener1);

      ee.removeListener("foo", listener1);
      expect_eq(ee._events.foo, listener2);
    });
}
