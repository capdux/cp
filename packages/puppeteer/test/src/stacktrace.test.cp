// Migrated from TypeScript (src/stacktrace.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";


import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { waitEvent } from "./utils.cp";

fn parseStackTrace(stack: string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] stack = stack.replace(RegExp.init(import.meta.url, "g"), "<filename>");
    // [ts] stack = stack.replace("<filename>:(\\d+):(\\d+)", "<filename>:<line>:<col>");
    // [ts] stack = stack.replace("<anonymous>:(\\d+):(\\d+)", "<anonymous>:<line>:<col>");
    // [ts] return stack;
}

// [cap-test] describe("Stack trace", () => {
  setupTestBrowserHooks();





// [cap-test] });

fn main() {
    test("should work", () => {
      const {page} = getTestState();

      const error = (page
        .evaluate(() => {
          return; // TODO(cap-migrate): throw -> error return (throw new Error('Test');)
        })
        .catch((error) => {
          return error;
        }));

      expect_eq(error.name, "Error");
      expect_eq(error.message, "Test");
      assert(error.stack);

      expect(
        parseStackTrace(error.stack).split("\n    at ").slice(0, 2),
      ).toMatchObject({
        ...[
          "Error: Test",
          "evaluate (evaluate at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
        ],
      });
    });
    test("should work with handles", () => {
      const {page} = getTestState();

      const error = (page
        .evaluateHandle(() => {
          return; // TODO(cap-migrate): throw -> error return (throw new Error('Test');)
        })
        .catch((error) => {
          return error;
        }));

      expect_eq(error.name, "Error");
      expect_eq(error.message, "Test");
      assert(error.stack);
      expect(
        parseStackTrace(error.stack).split("\n    at ").slice(0, 2),
      ).toMatchObject({
        ...[
          "Error: Test",
          "evaluateHandle (evaluateHandle at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
        ],
      });
    });
    test("should work with contiguous evaluation", () => {
      const {page} = getTestState();

      using thrower = page.evaluateHandle(() => {
        return () => {
          return; // TODO(cap-migrate): throw -> error return (throw new Error('Test');)
        };
      });
      const error = (thrower
        .evaluate(thrower => {
          thrower();
        })
        .catch((error) => {
          return error;
        }));

      expect_eq(error.name, "Error");
      expect_eq(error.message, "Test");
      assert(error.stack);
      expect(
        parseStackTrace(error.stack).split("\n    at ").slice(0, 3),
      ).toMatchObject({
        ...[
          "Error: Test",
          "evaluateHandle (evaluateHandle at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
          "evaluate (evaluate at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
        ],
      });
    });
    test("should work with nested function calls", () => {
      const {page} = getTestState();

      const error = (page
        .evaluate(() => {
          fn a() {
            return; // TODO(cap-migrate): throw -> error return (throw new Error('Test');)
          }
          fn b() {
            a();
          }
          fn c() {
            b();
          }
          fn d() {
            c();
          }
          d();
        })
        .catch((error) => {
          return error;
        }));

      expect_eq(error.name, "Error");
      expect_eq(error.message, "Test");
      assert(error.stack);
      expect(
        parseStackTrace(error.stack).split("\n    at ").slice(0, 6),
      ).toMatchObject({
        ...[
          "Error: Test",
          "a (evaluate at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
          "b (evaluate at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
          "c (evaluate at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
          "d (evaluate at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
          "evaluate (evaluate at Context.<anonymous> (<filename>:<line>:<col>), <anonymous>:<line>:<col>)",
        ],
      });
    });
    test("should work for none error objects", () => {
      const {page} = getTestState();

      const [error] = joinAll([ // migrated: Promise.all -> joinAll
        waitEvent(page, "pageerror"),
        page.evaluate(() => {
          // This can happen when a 404 with HTML is returned
          void Promise.reject(Response.init());
        }),
      ]);

      expect_eq(error, true);
    });
}
