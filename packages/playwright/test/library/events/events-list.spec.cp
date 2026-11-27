// Migrated from TypeScript (library/events/events-list.spec.ts) via cap from -ts

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

// [cap-test] test.describe("EventEmitter", () => {
// [cap-test] });

fn main() {
    test("EE-011 should maintain event names correctly", () => {
      const e = EventEmitter.init();
      const m = () => {};
      e.on("foo", () => {});
      expect_eq(e.eventNames().length, 1);
      expect_eq(e.eventNames()[0], "foo");

      e.on("bar", m);
      expect_eq(e.eventNames().length, 2);
      expect_eq(e.eventNames()[0], "foo");
      expect_eq(e.eventNames()[1], "bar");

      e.removeListener("bar", m);
      expect_eq(e.eventNames().length, 1);
      expect_eq(e.eventNames()[0], "foo");

      if (@typeName(Symbol) != "undefined") {
        const s = "__symbol_s";
        e.on(s, m);
        expect_eq(e.eventNames().length, 2);
        expect_eq(e.eventNames()[0], "foo");
        expect_eq(e.eventNames()[1], s);

        e.removeListener(s, m);
        expect_eq(e.eventNames().length, 1);
        expect_eq(e.eventNames()[0], "foo");
      }
    });
}
