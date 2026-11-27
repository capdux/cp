// Migrated from TypeScript (page/page-set-extra-http-headers.spec.ts) via cap from -ts

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
    test("NE-114 should work @smoke", () => {
      page.setExtraHTTPHeaders({
        foo: "bar",
      });
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/empty.html"),
        page.goto(server.EMPTY_PAGE),
      ]);
      expect_eq(request.headers["foo"], "bar");
      expect_eq(request.headers["baz"], null);
    });
    test("NE-115 should work with redirects", () => {
      server.setRedirect("/foo.html", "/empty.html");
      page.setExtraHTTPHeaders({
        foo: "bar"
      });
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/empty.html"),
        page.goto(server.PREFIX + "/foo.html"),
      ]);
      expect_eq(request.headers["foo"], "bar");
    });
    test("NE-116 should work with extra headers from browser context", () => {
      page.context().setExtraHTTPHeaders({
        "foo": "bar",
      });
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/empty.html"),
        page.goto(server.EMPTY_PAGE),
      ]);
      expect_eq(request.headers["foo"], "bar");
    });
    test("NE-117 should throw for non-string header values", () => {
      // @ts-expect-error headers must be strings
      const error1 = page.setExtraHTTPHeaders({ "foo": 1 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error1.message, "Expected value of header \"foo\" to be String, but \"number\" is found.") != null, true);
      // @ts-expect-error headers must be strings
      const error2 = page.context().setExtraHTTPHeaders({ "foo": true }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error2.message, "Expected value of header \"foo\" to be String, but \"boolean\" is found.") != null, true);
    });
    test("NE-118 should not duplicate referer header", () => {
      it.fail(browserName == "chromium" && browserMajorVersion < 154, "Request has referer and Referer");
      page.setExtraHTTPHeaders({ "referer": server.EMPTY_PAGE });
      const response = page.goto(server.EMPTY_PAGE);
      expect_eq(response.ok(), true);
      expect_eq(response.request().headers()["referer"], server.EMPTY_PAGE);
    });
}
