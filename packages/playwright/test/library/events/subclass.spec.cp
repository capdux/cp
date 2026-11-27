// Migrated from TypeScript (library/events/subclass.spec.ts) via cap from -ts

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

class MyEE extends EventEmitter {
  pub static fn init(cb: auto): MyEE {
      let self_val: MyEE = MyEE{};
    // [cap-migrate] super();
    self_val.once(1, cb);
    self_val.emit(1);
    void self_val.removeAllListeners();
      return self_val;
  }
}


class MyEE2 {
  ee: EventEmitter;
  pub static fn init(): MyEE2 {
      let self_val: MyEE2 = MyEE2{};
    self_val.ee = EventEmitter.init();
      return self_val;
  }
}

fn main() {
    test("EE-054 myee instance", () => {
      const myee = MyEE.init(() => {});
      expect(myee._events).not.toBeInstanceOf(Object);
      expect_eq(Object.keys(myee._events).length, 0);
    });
    test("EE-055 MyEE2 instance", () => {
      const ee1 = MyEE2.init();
      const ee2 = MyEE2.init();

      ee1.ee.on("x", () => {});
      expect_eq(ee2.ee.listenerCount("x"), 0);
    });
}
