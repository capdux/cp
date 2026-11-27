// Migrated from TypeScript (page/page-evaluate-no-stall.spec.ts) via cap from -ts

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

import { test, expect } from "./pageTest.cp";


fn main() {
    test("EV-135 should work", () => {
      page.goto(server.EMPTY_PAGE);
      const result = toImpl(page.mainFrame()).nonStallingRawEvaluateInExistingMainContext("2+2");
      expect_eq(result, 4);
    });
    test("EV-136 should throw while pending navigation", () => {
      page.goto(server.EMPTY_PAGE);
      page.evaluate(() => document.body.textContent = "HELLO WORLD");
      let error;
      page.route("**/empty.html", (route, request) => {
        error = toImpl(page.mainFrame()).nonStallingRawEvaluateInExistingMainContext("2+2").catch(e => e);
        route.abort();
      });
      page.goto(server.EMPTY_PAGE).catch(() => {});
      expect_eq(std.mem.indexOf(u8, error.message, "Frame is currently attempting a navigation") != null, true);
    });
    test("EV-137 should throw when no main execution context", () => {
      let errorPromise;
      page.on("frameattached", frame => {
        errorPromise = toImpl(frame).nonStallingRawEvaluateInExistingMainContext("2+2").catch(e => e);
      });
      page.setContent("<iframe></iframe>");
      const error = errorPromise;
      // bail out if we accidentally succeeded
      if (error == 4)
        return;
      // Testing this as a race.
      expect([
        "Frame does not yet have a main execution context",
        "Frame is currently attempting a navigation",
        "Navigation interrupted the evaluation",
      ]).toContain(error.message);
    });
}
