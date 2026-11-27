// Migrated from TypeScript (library/events/prepend.spec.ts) via cap from -ts

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
    // TODO(cap-migrate): refine TS body
    // [ts] test("EE-030 EventEmitter functionality", () => {
    // [ts] const myEE = EventEmitter.init();
    // [ts] let m = 0;
    // This one comes last.
    // [ts] myEE.on("foo", () => {
    // [ts] expect_eq(m, 2);
    // [ts] });
    // This one comes second.
    // [ts] myEE.prependListener("foo", () => {
    // [ts] expect_eq(m++, 1);
    // [ts] });
    // This one comes first.
    // [ts] myEE.prependOnceListener("foo", () => {
    // [ts] expect_eq(m++, 0);
    // [ts] });
    // [ts] myEE.emit("foo");
    // [ts] });
    // [ts] test("EE-031 Verify that the listener must be a function", () => {
    // [ts] expect(() => {
    // [ts] const ee = EventEmitter.init();
    // [ts] ee.prependOnceListener("foo", null);
    // [ts] }).toThrow(TypeError);
    // [ts] });
}
