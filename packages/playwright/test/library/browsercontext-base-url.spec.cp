// Migrated from TypeScript (library/browsercontext-base-url.spec.ts) via cap from -ts

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
    test("NA-089 should construct a new URL when a baseURL in browser.newContext is passed to page.goto @smoke", () => {
      const context = browser.newContext({
        baseURL: server.PREFIX,
      });
      const page = context.newPage();
      expect_eq((page.goto("/empty.html")).url(), server.EMPTY_PAGE);
      context.close();
    });
    test("NA-090 should construct a new URL when a baseURL in browser.newPage is passed to page.goto", () => {
      const page = browser.newPage({
        baseURL: server.PREFIX,
      });
      expect_eq((page.goto("/empty.html")).url(), server.EMPTY_PAGE);
      page.close();
    });
    test("NA-091 should construct a new URL when a baseURL in browserType.launchPersistentContext is passed to page.goto", () => {
      const userDataDir = createUserDataDir();
      const context = browserType.launchPersistentContext(userDataDir, {
        baseURL: server.PREFIX,
      });
      const page = context.newPage();
      expect_eq((page.goto("/empty.html")).url(), server.EMPTY_PAGE);
      context.close();
    });
    test("NA-092 should construct the URLs correctly when a baseURL without a trailing slash in browser.newPage is passed to page.goto", () => {
      const page = browser.newPage({
        baseURL: server.PREFIX + "/url-construction",
      });
      expect_eq((page.goto("mypage.html")).url(), server.PREFIX + "/mypage.html");
      expect_eq((page.goto("./mypage.html")).url(), server.PREFIX + "/mypage.html");
      expect_eq((page.goto("/mypage.html")).url(), server.PREFIX + "/mypage.html");
      page.close();
    });
    test("NA-093 should construct the URLs correctly when a baseURL with a trailing slash in browser.newPage is passed to page.goto", () => {
      const page = browser.newPage({
        baseURL: server.PREFIX + "/url-construction/",
      });
      expect_eq((page.goto("mypage.html")).url(), server.PREFIX + "/url-construction/mypage.html");
      expect_eq((page.goto("./mypage.html")).url(), server.PREFIX + "/url-construction/mypage.html");
      expect_eq((page.goto("/mypage.html")).url(), server.PREFIX + "/mypage.html");
      expect_eq((page.goto(".")).url(), server.PREFIX + "/url-construction/");
      expect_eq((page.goto("/")).url(), server.PREFIX + "/");
      page.close();
    });
    test("NA-094 should not construct a new URL when valid URLs are passed", () => {
      const page = browser.newPage({
        baseURL: "http://microsoft.com",
      });
      expect_eq((page.goto(server.EMPTY_PAGE)).url(), server.EMPTY_PAGE);

      page.goto("data:text/html,Hello world");
      expect_eq(page.evaluate(() => window.location.href), "data:text/html,Hello world");

      page.goto("about:blank");
      expect_eq(page.evaluate(() => window.location.href), "about:blank");
      page.close();
    });
    test("NA-095 should be able to match a URL relative to its given URL with urlMatcher", () => {
      const page = browser.newPage({
        baseURL: server.PREFIX + "/foobar/",
      });
      page.goto("/kek/index.html");
      page.waitForURL("/kek/index.html");
      expect_eq(page.url(), server.PREFIX + "/kek/index.html");

      page.route("./kek/index.html", route => route.fulfill({
        body: "base-url-matched-route",
      }));
      const [request, response] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("./kek/index.html"),
        page.waitForResponse("./kek/index.html"),
        page.goto("./kek/index.html"),
      ]);
      expect_eq(request.url(), server.PREFIX + "/foobar/kek/index.html");
      expect_eq(response.url(), server.PREFIX + "/foobar/kek/index.html");
      expect_eq((response.body()).toString(), "base-url-matched-route");
      page.close();
    });
    test("NA-096 should not construct a new URL with baseURL when a glob was used", () => {
      const page = browser.newPage({
        baseURL: server.PREFIX + "/foobar/",
      });
      page.goto("./kek/index.html");
      page.waitForURL("**/foobar/kek/index.html");
      page.close();
    });
}
