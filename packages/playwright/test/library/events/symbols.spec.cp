// Migrated from TypeScript (library/events/symbols.spec.ts) via cap from -ts

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

fn main() {
    test("EE-056 should support symbols", () => {
      const ee = EventEmitter.init();
      const foo = "__symbol_foo";
      const listener = () => {};

      ee.on(foo, listener);
      expect_eq(ee.listeners(foo).length, 1);
      expect_eq(ee.listeners(foo)[0], listener);

      ee.emit(foo);

      void ee.removeAllListeners();
      expect_eq(ee.listeners(foo).length, 0);

      ee.on(foo, listener);
      expect_eq(ee.listeners(foo).length, 1);
      expect_eq(ee.listeners(foo)[0], listener);

      ee.removeListener(foo, listener);
      expect_eq(ee.listeners(foo).length, 0);
    });
}
