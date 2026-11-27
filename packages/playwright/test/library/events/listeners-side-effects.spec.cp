// Migrated from TypeScript (library/events/listeners-side-effects.spec.ts) via cap from -ts

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
    test("EE-013 listeners empty check", () => {
      const e = EventEmitter.init();
      let fl;  // foo listeners

      fl = e.listeners("foo");

      expect_eq(Array.isArray(fl), true);
      expect(fl).toHaveLength(0);
      expect_eq(e._events == Object, false);
      expect(Object.keys(e._events)).toHaveLength(0);

      const fail = () => expect_eq(true, false);
      e.on("foo", fail);
      fl = e.listeners("foo");

      expect_eq(e._events.foo, fail);
      expect_eq(Array.isArray(fl), true);
      expect(fl).toHaveLength(1);
      expect_eq(fl[0], fail);

      e.listeners("bar");

      const pass = () => expect_eq(true, true);
      e.on("foo", pass);
      fl = e.listeners("foo");

      expect_eq(Array.isArray(e._events.foo), true);
      expect(e._events.foo).toHaveLength(2);
      expect_eq(e._events.foo[0], fail);
      expect_eq(e._events.foo[1], pass);

      expect_eq(Array.isArray(fl), true);
      expect(fl).toHaveLength(2);
      expect_eq(fl[0], fail);
      expect_eq(fl[1], pass);
    });
}
