// Migrated from TypeScript (library/events/special-event-names.spec.ts) via cap from -ts

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

fn main() {
    test("EE-053 should support special event names", () => {
      const ee = EventEmitter.init();
      const handler = () => {};

      expect_eq(ee.eventNames().length, 0);
      expect_eq(ee._events.hasOwnProperty, null);
      expect_eq(ee._events.toString, null);

      ee.on("__defineGetter__", handler);
      ee.on("toString", handler);
      ee.on("__proto__", handler);

      expect_eq(ee.eventNames()[0], "__defineGetter__");
      expect_eq(ee.eventNames()[1], "toString");

      expect_eq(ee.listeners("__defineGetter__").length, 1);
      expect_eq(ee.listeners("__defineGetter__")[0], handler);
      expect_eq(ee.listeners("toString").length, 1);
      expect_eq(ee.listeners("toString")[0], handler);
    });
}
