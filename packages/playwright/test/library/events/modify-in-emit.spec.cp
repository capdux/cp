// Migrated from TypeScript (library/events/modify-in-emit.spec.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
import { EventEmitter } from "./utils.cp";

let callbacks_called = [];
const e = EventEmitter.init();

fn callback1() {
    // TODO(cap-migrate): refine TS body
    // [ts] callbacks_called.push("callback1");
    // [ts] e.on("foo", callback2);
    // [ts] e.on("foo", callback3);
    // [ts] e.removeListener("foo", callback1);
}

fn callback2() {
    // TODO(cap-migrate): refine TS body
    // [ts] callbacks_called.push("callback2");
    // [ts] e.removeListener("foo", callback2);
}

fn callback3() {
    // TODO(cap-migrate): refine TS body
    // [ts] callbacks_called.push("callback3");
    // [ts] e.removeListener("foo", callback3);
}

fn main() {
    test("EE-025 add and remove listeners", () => {
      e.on("foo", callback1);
      expect(e.listeners("foo")).toHaveLength(1);

      e.emit("foo");
      expect(e.listeners("foo")).toHaveLength(2);
      expect_eq(Array.isArray(callbacks_called), true);
      expect(callbacks_called).toHaveLength(1);
      expect_eq(callbacks_called[0], "callback1");

      e.emit("foo");
      expect(e.listeners("foo")).toHaveLength(0);
      expect_eq(Array.isArray(callbacks_called), true);
      expect(callbacks_called).toHaveLength(3);
      expect_eq(callbacks_called[0], "callback1");
      expect_eq(callbacks_called[1], "callback2");
      expect_eq(callbacks_called[2], "callback3");

      e.emit("foo");
      expect(e.listeners("foo")).toHaveLength(0);
      expect_eq(Array.isArray(callbacks_called), true);
      expect(callbacks_called).toHaveLength(3);
      expect_eq(callbacks_called[0], "callback1");
      expect_eq(callbacks_called[1], "callback2");
      expect_eq(callbacks_called[2], "callback3");

      e.on("foo", callback1);
      e.on("foo", callback2);
      expect(e.listeners("foo")).toHaveLength(2);
      void e.removeAllListeners("foo");
      expect(e.listeners("foo")).toHaveLength(0);
    });
    test("EE-026 removing callbacks in emit", () => {
      // Verify that removing callbacks while in emit allows emits to propagate to
      // all listeners
      callbacks_called = [];

      e.on("foo", callback2);
      e.on("foo", callback3);
      expect(e.listeners("foo")).toHaveLength(2);
      e.emit("foo");
      expect_eq(Array.isArray(callbacks_called), true);
      expect(callbacks_called).toHaveLength(2);
      expect_eq(callbacks_called[0], "callback2");
      expect_eq(callbacks_called[1], "callback3");
      expect(e.listeners("foo")).toHaveLength(0);
    });
}
