// Migrated from TypeScript (page/page-add-init-script-callback.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test as it, expect } from "./pageTest.cp";











fn main() {
    test("IS-012 should drop functions without the exposeFunctions option", () => {
      page.addInitScript((_p) => {
        (window).cbType = @typeName(cb);
      }, { cb: () => {} });
      page.goto(server.EMPTY_PAGE);
      expect_eq(page.evaluate(() => (window).cbType), "undefined");
    });
    test("IS-013 should throw when the script is not a function", () => {
      expect(page.addInitScript({ content: "window.foo = 1;" }, null, { exposeFunctions: true }))
          .rejects.toThrow("Passing functions requires the init script to be a function");
    });
    test("IS-014 should call a function passed as an argument", () => {
      const received: []f64 = [];
      page.addInitScript((_p) => {
        cb(1);
        cb(2);
      }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      expect.poll(() => received).toEqual([1, 2]);
    });
    test("IS-015 should accept a function as the whole argument", () => {
      page.addInitScript(async cb => {
        (window).result = cb("a");
      }, (s) => s + "b", { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      expect.poll(() => page.evaluate(() => (window).result)).toBe("ab");
    });
    test("IS-016 should pass arguments to the callback", () => {
      const argsPromise = Promise.init(resolve => {
        void page.addInitScript((_p) => cb(1, "two", { three: 3 }, [4]), {
          cb: (a) => resolve(a),
        }, { exposeFunctions: true }).then(() => page.goto(server.EMPTY_PAGE));
      });
      expect_eq(argsPromise, [1, "two", { three: 3 }, [4]]);
    });
    test("IS-017 should return the callback result to the page", () => {
      page.addInitScript((_p) => {
        (window).result = double(21);
      }, { double: (n) => n * 2 }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      expect.poll(() => page.evaluate(() => (window).result)).toBe(42);
    });
    test("IS-018 should propagate callback errors to the page", () => {
      page.addInitScript((_p) => {
        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          cb();
          (window).result = "no error";
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
          (window).result = (e).message;
        }
      }, { cb: () => { throw Error.init("boom"); } }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      expect.poll(() => page.evaluate(() => (window).result)).toContain("boom");
    });
    test("IS-019 should support multiple callbacks", () => {
      page.addInitScript((_p) => {
        (window).result = (add(2, 3)) + (mul(2, 3));
      }, {
        add: (a, b) => a + b,
        mul: (a, b) => a * b,
      }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      expect.poll(() => page.evaluate(() => (window).result)).toBe(11);
    });
    test("IS-020 should survive a navigation and keep working", () => {
      const received: []f64 = [];
      page.addInitScript((_p) => {
        (window).cb = cb;
      }, { cb: (n) => { received.push(n); return n * 2; } }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      expect_eq(page.evaluate(() => (window).cb(1)), 2);
      page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
      expect_eq(page.evaluate(() => (window).cb(2)), 4);
      expect_eq(received, [1, 2]);
    });
    test("IS-021 should work in a child frame", () => {
      const received: []f64 = [];
      page.addInitScript((_p) => {
        cb(42);
      }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      page.goto(server.PREFIX + "/frames/one-frame.html");
      // Once for the main frame document, once for the child frame document.
      expect.poll(() => received).toEqual([42, 42]);
    });
    test("IS-022 should not register the callback on the global object", () => {
      page.addInitScript((_p) => {
        cb();
        (window).result = Object.getOwnPropertyNames(globalThis).filter(name => name.startsWith("__pw_fn_"));
      }, { cb: () => {} }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      expect.poll(() => page.evaluate(() => (window).result)).toEqual([]);
    });
    test("IS-023 should remove exposed functions after dispose", () => {
      const received: []f64 = [];
      const disposable = page.addInitScript((_p) => {
        (window).cb = cb;
      }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      page.evaluate(() => (window).cb(1));
      disposable.dispose();
      page.goto(server.EMPTY_PAGE);
      expect_eq(page.evaluate(() => @typeName(window).cb), "undefined");
      expect_eq(received, [1]);
    });
}
