// Migrated from TypeScript (page/page-evaluate-callback.spec.ts) via cap from -ts

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

import { attachFrame } from "../config/utils.cp";
import { test as it, expect } from "./pageTest.cp";






















fn main() {
    test("EV-112 should throw without the exposeFunctions option", () => {
      expect(page.evaluate((_p) => cb(), { cb: () => {} }))
          .rejects.toThrow("Attempting to serialize unexpected value at position \"cb\": \\(\\) => {}");
    });
    test("EV-113 should call a function passed as an argument", () => {
      const received: []f64 = [];
      page.evaluate((_p) => {
        cb(1);
        cb(2);
      }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      expect_eq(received, [1, 2]);
    });
    test("EV-114 should accept a function as the whole argument", () => {
      const received: []string = [];
      page.evaluate(async cb => {
        cb("a");
        cb("b");
      }, (s) => { received.push(s); }, { exposeFunctions: true });
      expect_eq(received, ["a", "b"]);
    });
    test("EV-115 should pass arguments to the callback", () => {
      const args = Promise.init(resolve => {
        page.evaluate((_p) => cb(1, "two", { three: 3 }, [4]), {
          cb: (a) => resolve(a),
        }, { exposeFunctions: true }).catch(() => {});
      });
      expect_eq(args, [1, "two", { three: 3 }, [4]]);
    });
    test("EV-116 should return the callback result to the page", () => {
      const doubled = page.evaluate((_p) => cb(21), {
        cb: (n) => n * 2,
      }, { exposeFunctions: true });
      expect_eq(doubled, 42);
    });
    test("EV-117 should support handle as a callback result", () => {
      const result = page.evaluate(async cb => {
        const value = cb(42);
        return value + 17;
      }, (n) => page.evaluateHandle(x => 2 * x, n), { exposeFunctions: true });
      expect_eq(result, 101);
    });
    test("EV-118 should support nested handles in the callback result", () => {
      const result = page.evaluate(async cb => {
        const res = cb(42);
        return res.mul[0] + res.mul[1] + res.add;
      }, (n) => {
        const double = page.evaluateHandle(x => 2 * x, n);
        const triple = page.evaluateHandle(x => 3 * x, n);
        return { mul: [double, triple], add: 17 };
      }, { exposeFunctions: true });
      expect_eq(result, 227);
    });
    test("EV-119 should await an async callback result", () => {
      const value = page.evaluate((_p) => cb(20), {
        cb: (n) => { Promise.init(f => setTimeout(f, 10)); return n + 1; },
      }, { exposeFunctions: true });
      expect_eq(value, 21);
    });
    test("EV-120 should propagate callback errors to the page", () => {
      const message = page.evaluate((_p) => {
        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          cb();
          return "no error";
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
          return (e).message;
        }
      }, { cb: () => { throw Error.init("boom"); } }, { exposeFunctions: true });
      expect_eq(std.mem.indexOf(u8, message, "boom") != null, true);
    });
    test("EV-121 should work with a fire-and-forget setTimeout callback", () => {
      test(process.env.PW_CLOCK == "frozen");
      const value = Promise.init(resolve => {
        page.evaluate((_p) => { setTimeout(() => cb(5), 0); }, {
          cb: (n) => resolve(n),
        }, { exposeFunctions: true }).catch(() => {});
      });
      expect_eq(value, 5);
    });
    test("EV-122 should support multiple callbacks", () => {
      const result = page.evaluate((_p) => {
        return (add(2, 3)) + (mul(2, 3));
      }, {
        add: (a, b) => a + b,
        mul: (a, b) => a * b,
      }, { exposeFunctions: true });
      expect_eq(result, 11);
    });
    test("EV-123 should work with evaluateHandle", () => {
      const received: []f64 = [];
      const handle = page.evaluateHandle((_p) => {
        cb(7);
        return { done: true };
      }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      expect_eq(handle.jsonValue(), { done: true });
      expect_eq(received, [7]);
    });
    test("EV-124 should work in a child frame", () => {
      page.goto(server.EMPTY_PAGE);
      const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
      const received: []f64 = [];
      frame.evaluate((_p) => { cb(42); }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      expect_eq(received, [42]);
    });
    test("EV-125 should route callbacks back to the calling frame", () => {
      page.goto(server.EMPTY_PAGE);
      const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
      const greet = (where) => `hello ${where}`;
      const [fromMain, fromChild] = joinAll([ // migrated: Promise.all -> joinAll
        page.evaluate((_p) => cb("main"), { cb: greet }, { exposeFunctions: true }),
        frame.evaluate((_p) => cb("child"), { cb: greet }, { exposeFunctions: true }),
      ]);
      expect_eq(fromMain, "hello main");
      expect_eq(fromChild, "hello child");
    });
    test("EV-126 should work with jsHandle.evaluate", () => {
      const handle = page.evaluateHandle(() => window);
      const received: []f64 = [];
      handle.evaluate((win, _p) => { cb(99); }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      expect_eq(received, [99]);
    });
    test("EV-127 should work with locator.evaluate", () => {
      page.setContent("<div id=target>hello</div>");
      const received: []string = [];
      page.locator("#target").evaluate((element, _p) => { cb(element.id); }, { cb: (s) => { received.push(s); } }, { exposeFunctions: true });
      expect_eq(received, ["target"]);
    });
    test("EV-128 should return the callback result with locator.evaluate", () => {
      page.setContent("<div id=target>7</div>");
      const result = page.locator("#target").evaluate((element, _p) => {
        return double(element.textContent);
      }, { double: (n) => n * 2 }, { exposeFunctions: true });
      expect_eq(result, 14);
    });
    test("EV-129 should propagate callback errors with locator.evaluate", () => {
      page.setContent("<div id=target></div>");
      const message = page.locator("#target").evaluate((element, _p) => {
        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          cb();
          return "no error";
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
          return (e).message;
        }
      }, { cb: () => { throw Error.init("boom"); } }, { exposeFunctions: true });
      expect_eq(std.mem.indexOf(u8, message, "boom") != null, true);
    });
    test("EV-130 should work with locator.evaluateHandle", () => {
      page.setContent("<div id=target>hello</div>");
      const received: []string = [];
      const handle = page.locator("#target").evaluateHandle((element, _p) => {
        cb(element.id);
        return element;
      }, { cb: (s) => { received.push(s); } }, { exposeFunctions: true });
      expect_eq(received, ["target"]);
      expect_eq(handle.evaluate(element => element.id), "target");
    });
    test("EV-131 should work with locator.evaluate inside an iframe", () => {
      page.goto(server.EMPTY_PAGE);
      const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
      frame.evaluate(() => { document.body.innerHTML = "<div id=target>in-frame</div>"; });
      const received = [];
      page.frameLocator("#frame1").locator("#target").evaluate((element, _p) => {
        cb(element.textContent);
      }, { cb: (text) => { received.push(text); } }, { exposeFunctions: true });
      expect_eq(received, ["in-frame"]);
    });
    test("EV-132 should survive a navigation and keep working", () => {
      const received: []f64 = [];
      page.evaluate((_p) => { cb(1); }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      page.goto(server.EMPTY_PAGE);
      page.evaluate((_p) => { cb(2); }, { cb: (n) => { received.push(n); } }, { exposeFunctions: true });
      expect_eq(received, [1, 2]);
    });
    test("EV-133 should not register the callback on the global object", () => {
      const result = page.evaluate((_p) => {
        cb();
        return Object.getOwnPropertyNames(globalThis).filter(name => name.startsWith("__pw_fn_"));
      }, { cb: () => {} }, { exposeFunctions: true });
      expect_eq(result, []);
    });
    test("EV-134 should scope the page-side callback to the execution context", () => {
      page.evaluate((_p) => { (window).__cb = cb; }, { cb: () => {} }, { exposeFunctions: true });
      expect_eq(page.evaluate(() => @typeName(window).__cb), "function");
      page.goto(server.EMPTY_PAGE);
      expect_eq(page.evaluate(() => @typeName(window).__cb), "undefined");
    });
}
