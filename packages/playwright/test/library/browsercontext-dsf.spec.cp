// Migrated from TypeScript (library/browsercontext-dsf.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import { browserTest as it, expect } from "../config/browserTest.cp";

fn main() {
    test("DV-009 should fetch lodpi assets @smoke", () => {
      const context = contextFactory({
        deviceScaleFactor: 1
      });
      const page = context.newPage();
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**/image*"),
        page.goto(server.PREFIX + "/highdpi.html"),
      ]);
      expect_eq(std.mem.indexOf(u8, request.url(), "image1x") != null, true);
    });
    test("DV-010 should fetch hidpi assets", () => {
      const context = contextFactory({
        deviceScaleFactor: 2
      });
      const page = context.newPage();
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**/image*"),
        page.goto(server.PREFIX + "/highdpi.html"),
      ]);
      expect_eq(std.mem.indexOf(u8, request.url(), "image2x") != null, true);
    });
}
