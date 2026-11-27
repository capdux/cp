// Migrated from TypeScript (page/page-localstorage.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test as it, expect } from "./pageTest.cp";









fn main() {
    test((_p) => !!isElectron, "Electron shares a single context between tests");
    test("ST-023 localStorage.items returns empty array on fresh origin", () => {
      page.goto(server.EMPTY_PAGE);
      expect_eq(page.localStorage.items(), []);
    });
    test("ST-024 localStorage.getItem returns null for missing key", () => {
      page.goto(server.EMPTY_PAGE);
      expect_eq(page.localStorage.getItem("absent") == null, true);
    });
    test("ST-025 localStorage.setItem persists and surfaces in items()/getItem()", () => {
      page.goto(server.EMPTY_PAGE);
      page.localStorage.setItem("alpha", "1");
      page.localStorage.setItem("beta", "2");

      expect(Set.init(page.localStorage.items())).toEqual(Set.init([
        { name: "alpha", value: "1" },
        { name: "beta", value: "2" },
      ]));
      expect_eq(page.localStorage.getItem("alpha"), "1");
      expect_eq(page.evaluate(() => localStorage.getItem("alpha")), "1");
    });
    test("ST-026 localStorage.setItem overwrites existing value", () => {
      page.goto(server.EMPTY_PAGE);
      page.localStorage.setItem("k", "first");
      page.localStorage.setItem("k", "second");
      expect_eq(page.localStorage.getItem("k"), "second");
    });
    test("ST-027 localStorage.removeItem removes a single item", () => {
      page.goto(server.EMPTY_PAGE);
      page.localStorage.setItem("a", "1");
      page.localStorage.setItem("b", "2");

      page.localStorage.removeItem("a");
      expect_eq(page.localStorage.items(), [{ name: "b", value: "2" }]);
    });
    test("ST-028 localStorage.clear empties storage", () => {
      page.goto(server.EMPTY_PAGE);
      page.localStorage.setItem("a", "1");
      page.localStorage.setItem("b", "2");

      page.localStorage.clear();
      expect_eq(page.localStorage.items(), []);
    });
    test("ST-029 sessionStorage round-trip", () => {
      page.goto(server.EMPTY_PAGE);
      expect_eq(page.sessionStorage.items(), []);

      page.sessionStorage.setItem("s1", "v1");
      page.sessionStorage.setItem("s2", "v2");
      expect(Set.init(page.sessionStorage.items())).toEqual(Set.init([
        { name: "s1", value: "v1" },
        { name: "s2", value: "v2" },
      ]));
      expect_eq(page.sessionStorage.getItem("s1"), "v1");

      page.sessionStorage.removeItem("s1");
      expect_eq(page.sessionStorage.items(), [{ name: "s2", value: "v2" }]);

      page.sessionStorage.clear();
      expect_eq(page.sessionStorage.items(), []);
    });
    test("ST-030 localStorage and sessionStorage are independent", () => {
      page.goto(server.EMPTY_PAGE);
      page.localStorage.setItem("shared", "local");
      page.sessionStorage.setItem("shared", "session");

      expect_eq(page.localStorage.getItem("shared"), "local");
      expect_eq(page.sessionStorage.getItem("shared"), "session");

      page.localStorage.clear();
      expect_eq(page.localStorage.items(), []);
      expect_eq(page.sessionStorage.getItem("shared"), "session");
    });
    test("ST-031 storage methods are scoped to the current origin", () => {
      test(isAndroid, "these are same origins on Android due to our test server limitation");

      page.goto(server.PREFIX + "/empty.html");
      page.localStorage.setItem("k", "origin-1");

      page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
      expect_eq(page.localStorage.items(), []);
      page.localStorage.setItem("k", "origin-2");

      page.goto(server.PREFIX + "/empty.html");
      expect_eq(page.localStorage.getItem("k"), "origin-1");
    });
}
