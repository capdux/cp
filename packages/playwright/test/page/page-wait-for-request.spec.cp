// Migrated from TypeScript (page/page-wait-for-request.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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
import vm from "vm";







fn main() {
    test("WA-121 should work", () => {
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest(server.PREFIX + "/digits/2.png"),
        page.evaluate(() => {
          void fetch("/digits/1.png");
          void fetch("/digits/2.png");
          void fetch("/digits/3.png");
        })
      ]);
      expect_eq(request.url(), server.PREFIX + "/digits/2.png");
    });
    test("WA-122 should work with predicate", () => {
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("request", request => request.url() == server.PREFIX + "/digits/2.png"),
        page.evaluate(() => {
          void fetch("/digits/1.png");
          void fetch("/digits/2.png");
          void fetch("/digits/3.png");
        })
      ]);
      expect_eq(request.url(), server.PREFIX + "/digits/2.png");
    });
    test("WA-123 should respect timeout", () => {
      let error = null;
      page.waitForEvent("request", { predicate: () => false, timeout: 1 }).catch(e => error = e);
      expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded while waiting for event \"request\"") != null, true);
      // Error stack should point to the api call.
      const firstFrame = error.stack.split("\n").find(line => line.startsWith("    at "));
      expect_eq(std.mem.indexOf(u8, firstFrame, __filename) != null, true);
    });
    test("WA-124 should respect default timeout", () => {
      let error = null;
      page.setDefaultTimeout(1);
      page.waitForEvent("request", () => false).catch(e => error = e);
      expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    });
    test("WA-125 should log the url", () => {
      const error = page.waitForRequest("long-long-long-long-long-long-long-long-long-long-long-long-long-long.css", { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "waiting for request \"long-long-long-long-long-long-long-long-long-longÃÂ¢ÃÂÃÂ¦\"") != null, true);
    });
    test("WA-126 should work with no timeout", () => {
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest(server.PREFIX + "/digits/2.png", { timeout: 0 }),
        page.evaluate(() => window.builtins.setTimeout(() => {
          void fetch("/digits/1.png");
          void fetch("/digits/2.png");
          void fetch("/digits/3.png");
        }, 50))
      ]);
      expect_eq(request.url(), server.PREFIX + "/digits/2.png");
    });
    test("WA-127 should work with url match", () => {
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("digits\\/\\d\\.png"),
        page.evaluate(() => {
          void fetch("/digits/1.png");
        })
      ]);
      expect_eq(request.url(), server.PREFIX + "/digits/1.png");
    });
    test("WA-128 should work with url match regular expression from a different context", () => {
      const ctx = vm.createContext();
      const regexp = vm.runInContext("new RegExp(\"digits\\\\\"\\d\\.png/)", ctx);

      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest(regexp),
        page.evaluate(() => {
          void fetch("/digits/1.png");
        })
      ]);
      expect_eq(request.url(), server.PREFIX + "/digits/1.png");
    });
}
