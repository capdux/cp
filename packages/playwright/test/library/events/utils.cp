// Migrated from TypeScript (library/events/utils.ts) via cap from -ts

// Copyright Joyent, Inc. and other Node contributors.
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
import { clientEventEmitter } from "../../../packages/playwright-core/lib/coreBundle.cp";

pub fn mustNotCall(msg: string? = null) {
    // TODO(cap-migrate): refine TS body
    // [ts] return () => {
    // [ts] expect_eq(false, msg || "function should not have been called", true);
    // [ts] };
}

let count;

pub fn beforeEach() {
    // TODO(cap-migrate): refine TS body
    // [ts] count = 0;
}

pub fn afterEach() {
    // TODO(cap-migrate): refine TS body
    // [ts] expect_eq(count, 0);
}

pub fn mustCall(fn: string? = null, exact: f64? = null) {
    // TODO(cap-migrate): refine TS body
    // [ts] count += exact || 1;
    // [ts] return (args) => {
    // [ts] fn?.(...args);
    // [ts] --count;
    // [ts] };
}

/** as any breaks long TS resolution chain that makes tests unhappy */
pub class EventEmitter {
  pub static fn init(): EventEmitter {
      let self_val: EventEmitter = EventEmitter{};
    // [cap-migrate] super();
      return self_val;
  }
}
