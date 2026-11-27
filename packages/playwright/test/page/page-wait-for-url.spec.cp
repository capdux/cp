// Migrated from TypeScript (page/page-wait-for-url.spec.ts) via cap from -ts

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
    test("WA-110 should work", () => {
      page.goto(server.EMPTY_PAGE);
      page.evaluate(url => window.location.href = url, server.PREFIX + "/grid.html");
      page.waitForURL("**/grid.html");
    });
    test("WA-111 should respect timeout", () => {
      const promise = page.waitForURL("**/frame.html", { timeout: 2500 }).catch(e => e);
      page.goto(server.EMPTY_PAGE);
      const error = promise;
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 2500ms exceeded.") != null, true);
    });
    test("WA-112 should work with both domcontentloaded and load", () => {
      let response = null;
      server.setRoute("/one-style.css", (req, res) => response = res);
      const navigationPromise = page.goto(server.PREFIX + "/one-style.html");
      const domContentLoadedPromise = page.waitForURL("**/one-style.html", { waitUntil: "domcontentloaded" });
      let bothFired = false;
      const bothFiredPromise = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForURL("**/one-style.html", { waitUntil: "load" }),
        domContentLoadedPromise
      ]).then(() => bothFired = true);

      server.waitForRequest("/one-style.css");
      domContentLoadedPromise;
      expect_eq(bothFired, false);
      response.end();
      bothFiredPromise;
      navigationPromise;
    });
    test("WA-113 should work with commit", () => {
      server.setRoute("/script.js", (req, res) => {});
      server.setRoute("/empty.html", (req, res) => {
        res.setHeader("content-type", "text/html");
        res.end("<title>Hello</title><script src=\"script.js\"></script>");
      });
      page.goto(server.EMPTY_PAGE).catch(e => {});
      page.waitForURL("**/empty.html", { waitUntil: "commit" });
      expect_eq(page.title(), "Hello");
    });
    test("WA-114 should work with commit and about:blank", () => {
      page.waitForURL("about:blank", { waitUntil: "commit" });
    });
    test("WA-115 should work with clicking on anchor links", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<a href='#foobar'>foobar</a>`);
      page.click("a");
      page.waitForURL("**/*#foobar");
    });
    test("WA-116 should work with history.pushState()", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <a onclick='javascript:pushState()'>SPA</a>
        <script>
          function pushState() { history.pushState({}, '', 'wow.html') }
        </script>
      `);
      page.click("a");
      page.waitForURL("**/wow.html");
      expect_eq(page.url(), server.PREFIX + "/wow.html");
    });
    test("WA-117 should work with history.replaceState()", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <a onclick='javascript:replaceState()'>SPA</a>
        <script>
          function replaceState() { history.replaceState({}, '', '/replaced.html') }
        </script>
      `);
      page.click("a");
      page.waitForURL("**/replaced.html");
      expect_eq(page.url(), server.PREFIX + "/replaced.html");
    });
    test("WA-118 should work with DOM history.back()/history.forward()", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <a id=back onclick='javascript:goBack()'>back</a>
        <a id=forward onclick='javascript:goForward()'>forward</a>
        <script>
          function goBack() { history.back(); }
          function goForward() { history.forward(); }
          history.pushState({}, '', '/first.html');
          history.pushState({}, '', '/second.html');
        </script>
      `);
      expect_eq(page.url(), server.PREFIX + "/second.html");

      page.click("a#back");
      page.waitForURL("**/first.html");
      expect_eq(page.url(), server.PREFIX + "/first.html");

      page.click("a#forward");
      page.waitForURL("**/second.html");
      expect_eq(page.url(), server.PREFIX + "/second.html");
    });
    test("WA-119 should work with url match for same document navigations", () => {
      page.goto(server.EMPTY_PAGE);
      let resolved = false;
      const waitPromise = page.waitForURL("third\\.html").then(() => resolved = true);
      expect_eq(resolved, false);
      page.evaluate(() => {
        history.pushState({}, "", "/first.html");
      });
      expect_eq(resolved, false);
      page.evaluate(() => {
        history.pushState({}, "", "/second.html");
      });
      expect_eq(resolved, false);
      page.evaluate(() => {
        history.pushState({}, "", "/third.html");
      });
      waitPromise;
      expect_eq(resolved, true);
    });
    test("WA-120 should work on frame", () => {
      page.goto(server.PREFIX + "/frames/one-frame.html");
      const frame = page.frames()[1];
      frame.evaluate(url => window.location.href = url, server.PREFIX + "/grid.html");
      frame.waitForURL("**/grid.html");
    });
}
