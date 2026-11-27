// Migrated from TypeScript (library/browsercontext-set-extra-http-headers.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";

fn main() {
    test("NE-119 should override extra headers from browser context", () => {
      const context = browser.newContext({
        extraHTTPHeaders: { "fOo": "bAr", "baR": "foO" },
      });
      const page = context.newPage();
      page.setExtraHTTPHeaders({
        "Foo": "Bar"
      });
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/empty.html"),
        page.goto(server.EMPTY_PAGE),
      ]);
      context.close();
      expect_eq(request.headers["foo"], "Bar");
      expect_eq(request.headers["bar"], "foO");
    });
    test("NE-120 should throw for non-string header values", () => {
      const error3 = browser.newContext({ extraHTTPHeaders: { "foo": null } }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error3.message, "Expected value of header \"foo\" to be String, but \"object\" is found.") != null, true);
    });
}
