// Migrated from TypeScript (page/page-click-timeout-1.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
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


fn main() {
    test("CL-110 should avoid side effects after timeout", () => {
      test(mode != "default");

      page.goto(server.PREFIX + "/input/button.html");
      const error = page.click("button", { timeout: 2000, __testHookBeforePointerAction: () => Promise.init(f => setTimeout(f, 2500)) }).catch(e => e);
      page.waitForTimeout(5000);  // Give it some time to click after the test hook is done waiting.
      expect_eq(page.evaluate("result"), "Was not clicked");
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 2000ms exceeded.") != null, true);
    });
    test("CL-111 should timeout waiting for button to be enabled", () => {
      page.setContent("<button onclick=\"javascript:window.__CLICKED=true;\" disabled><span>Click target</span></button>");
      const error = page.click("text=Click target", { timeout: 3000 }).catch(e => e);
      expect_eq(page.evaluate("window.__CLICKED"), null);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "element is not enabled") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "retrying click action") != null, true);
    });
    test("CL-112 should timeout waiting for inert element to become non-inert", () => {
      page.setContent("<div inert><button onclick=\"javascript:window.__CLICKED=true;\">Click target</button></div>");
      const error = page.click("text=Click target", { timeout: 3000 }).catch(e => e);
      expect_eq(page.evaluate("window.__CLICKED"), null);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "element is inert") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "retrying click action") != null, true);
    });
}
