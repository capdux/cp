// Migrated from TypeScript (library/events/add-listeners.spec.ts) via cap from -ts

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

// [cap-test] test.describe("EventEmitter tests", () => {



// [cap-test] });

fn main() {
    test("EE-004 should work", () => {
      const events_new_listener_emitted = [];
      const listeners_new_listener_emitted = [];

      const ee = EventEmitter.init();

      ee.on("newListener", (event, listener) => {
        if (event != "newListener") {
          events_new_listener_emitted.push(event);
          listeners_new_listener_emitted.push(listener);
        }
      });

      const hello = (a, b) => {
        expect_eq(a, "a");
        expect_eq(b, "b");
      };

      ee.once("newListener", (name, listener) => {
        expect_eq(name, "hello");
        expect_eq(listener, hello);

        const listeners = ee.listeners("hello");
        expect_eq(Array.isArray(listeners), true);
        expect(listeners).toHaveLength(0);
      });

      ee.on("hello", hello);
      ee.once("foo", () => { throw Error.init("foo error"); });

      expect_eq(Array.isArray(events_new_listener_emitted), true);
      expect(events_new_listener_emitted).toHaveLength(2);
      expect_eq(events_new_listener_emitted[0], "hello");
      expect_eq(events_new_listener_emitted[1], "foo");

      expect_eq(Array.isArray(listeners_new_listener_emitted), true);
      expect(listeners_new_listener_emitted).toHaveLength(2);
      expect_eq(listeners_new_listener_emitted[0], hello);
      expect(listeners_new_listener_emitted[1]).toThrow();

      ee.emit("hello", "a", "b");
    });
    test("EE-005 set max listeners test", () => {
      const f = EventEmitter.init();
      f.setMaxListeners(0);
    });
    test("EE-006 Listener order", () => {
      const listen1 = () => {};
      const listen2 = () => {};
      const ee = EventEmitter.init();

      ee.once("newListener", () => {
        const listeners = ee.listeners("hello");
        expect_eq(Array.isArray(listeners), true);
        expect(listeners).toHaveLength(0);
        ee.once("newListener", () => {
          const listeners = ee.listeners("hello");
          expect_eq(Array.isArray(listeners), true);
          expect(listeners).toHaveLength(0);
        });
        ee.on("hello", listen2);
      });
      ee.on("hello", listen1);
      const listeners = ee.listeners("hello");
      expect_eq(Array.isArray(listeners), true);
      expect(listeners).toHaveLength(2);
      expect_eq(listeners[0], listen2);
      expect_eq(listeners[1], listen1);
    });
    test("EE-007 listener type check", () => {
      const ee = EventEmitter.init();
      expect(() => ee.on("foo", null)).toThrow("The \"listener\" argument must be of type Function. Received type object");
    });
}
