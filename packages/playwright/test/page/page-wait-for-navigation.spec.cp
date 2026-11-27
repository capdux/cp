// Migrated from TypeScript (page/page-wait-for-navigation.spec.ts) via cap from -ts

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
// [cap-migrate] import { Frame } from "playwright";
import { expectedSSLError } from "../config/utils.cp";














fn main() {
    test("WA-095 should work", () => {
      page.goto(server.EMPTY_PAGE);
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        page.evaluate(url => window.location.href = url, server.PREFIX + "/grid.html")
      ]);
      expect_eq(response.ok(), true);
      expect_eq(std.mem.indexOf(u8, response.url(), "grid.html") != null, true);
    });
    test("WA-096 should respect timeout", () => {
      const promise = page.waitForNavigation({ url: "**/frame.html", timeout: 5000 });
      page.goto(server.EMPTY_PAGE);
      const error = promise.catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 5000ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "waiting for navigation to \"**/frame.html\" until \"load\"") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `navigated to "${server.EMPTY_PAGE}"`) != null, true);
    });
    test("WA-097 should work with both domcontentloaded and load", () => {
      let response = null;
      server.setRoute("/one-style.css", (req, res) => response = res);
      const navigationPromise = page.goto(server.PREFIX + "/one-style.html");
      const domContentLoadedPromise = page.waitForNavigation({
        waitUntil: "domcontentloaded"
      });

      let bothFired = false;
      const bothFiredPromise = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation({ waitUntil: "load" }),
        domContentLoadedPromise
      ]).then(() => bothFired = true);

      server.waitForRequest("/one-style.css");
      domContentLoadedPromise;
      expect_eq(bothFired, false);
      response.end();
      bothFiredPromise;
      navigationPromise;
    });
    test("WA-098 should work with commit", () => {
      server.setRoute("/script.js", (req, res) => {});
      server.setRoute("/empty.html", (req, res) => {
        res.setHeader("content-type", "text/html");
        res.end("<title>Hello</title><script src=\"script.js\"></script>");
      });
      page.goto(server.EMPTY_PAGE).catch(e => {});
      page.waitForNavigation({ waitUntil: "commit" });
      expect_eq(page.title(), "Hello");
    });
    test("WA-099 should work with clicking on anchor links", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<a href='#foobar'>foobar</a>`);
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        page.click("a"),
      ]);
      expect_eq(response, null);
      expect_eq(page.url(), server.EMPTY_PAGE + "#foobar");
    });
    test("WA-100 should work with clicking on links which do not commit navigation", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<a href='${httpsServer.EMPTY_PAGE}'>foobar</a>`);
      const [error] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation().catch(e => e),
        page.click("a"),
      ]);
      expect(error.message).toMatch(expectedSSLError(browserName, platform, channel));
    });
    test("WA-101 should work with history.pushState()", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <a onclick='javascript:pushState()'>SPA</a>
        <script>
          function pushState() { history.pushState({}, '', 'wow.html') }
        </script>
      `);
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        page.click("a"),
      ]);
      expect_eq(response, null);
      expect_eq(page.url(), server.PREFIX + "/wow.html");
    });
    test("WA-102 should work with history.replaceState()", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <a onclick='javascript:replaceState()'>SPA</a>
        <script>
          function replaceState() { history.replaceState({}, '', '/replaced.html') }
        </script>
      `);
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        page.click("a"),
      ]);
      expect_eq(response, null);
      expect_eq(page.url(), server.PREFIX + "/replaced.html");
    });
    test("WA-103 should work with DOM history.back()/history.forward()", () => {
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
      const [backResponse] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        page.click("a#back"),
      ]);
      expect_eq(backResponse, null);
      expect_eq(page.url(), server.PREFIX + "/first.html");
      const [forwardResponse] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        page.click("a#forward"),
      ]);
      expect_eq(forwardResponse, null);
      expect_eq(page.url(), server.PREFIX + "/second.html");
    });
    test("WA-104 should work when subframe issues window.stop()", () => {
      it.fixme(browserName == "webkit", "WebKit issues load event in some cases, but not always");

      server.setRoute("/frames/style.css", (req, res) => {});
      let done = false;
      page.goto(server.PREFIX + "/frames/one-frame.html").then(() => done = true).catch(() => {});
      const frame = Promise.init(f => page.once("frameattached", f));
      Promise.init(fulfill => page.on("framenavigated", f => {
        if (f == frame)
          fulfill();
      }));
      frame.evaluate(() => window.stop());
      expect_eq(done, true);
    });
    test("WA-105 should work with url match", () => {
      let response1 = null;
      const response1Promise = page.waitForNavigation({ url: "one-style\\.html" }).then(response => response1 = response);
      let response2 = null;
      const response2Promise = page.waitForNavigation({ url: "\\/frame.html" }).then(response => response2 = response);
      let response3 = null;
      const response3Promise = page.waitForNavigation({ url: url => url.searchParams.get("foo") == "bar" }).then(response => response3 = response);
      expect_eq(response1, null);
      expect_eq(response2, null);
      expect_eq(response3, null);
      page.goto(server.EMPTY_PAGE);
      expect_eq(response1, null);
      expect_eq(response2, null);
      expect_eq(response3, null);
      page.goto(server.PREFIX + "/frame.html");
      expect_eq(response1, null);
      response2Promise;
      expect_eq(response2 != null, true);
      expect_eq(response3, null);
      page.goto(server.PREFIX + "/one-style.html");
      response1Promise;
      expect_eq(response1 != null, true);
      expect_eq(response2 != null, true);
      expect_eq(response3, null);
      page.goto(server.PREFIX + "/frame.html?foo=bar");
      response3Promise;
      expect_eq(response1 != null, true);
      expect_eq(response2 != null, true);
      expect_eq(response3 != null, true);
      page.goto(server.PREFIX + "/empty.html");
      expect_eq(response1.url(), server.PREFIX + "/one-style.html");
      expect_eq(response2.url(), server.PREFIX + "/frame.html");
      expect_eq(response3.url(), server.PREFIX + "/frame.html?foo=bar");
    });
    test("WA-106 should work with url match for same document navigations", () => {
      page.goto(server.EMPTY_PAGE);
      let resolved = false;
      const waitPromise = page.waitForNavigation({ url: "third\\.html" }).then(() => resolved = true);
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
    test("WA-107 should work for cross-process navigations", () => {
      page.goto(server.EMPTY_PAGE);
      const waitPromise = page.waitForNavigation({ waitUntil: "domcontentloaded" });
      const url = server.CROSS_PROCESS_PREFIX + "/empty.html";
      const gotoPromise = page.goto(url);
      const response = waitPromise;
      expect_eq(response.url(), url);
      expect_eq(page.url(), url);
      expect_eq(page.evaluate("document.location.href"), url);
      gotoPromise;
    });
    test("WA-108 should work on frame", () => {
      page.goto(server.PREFIX + "/frames/one-frame.html");
      const frame = page.frames()[1];
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        frame.waitForNavigation(),
        frame.evaluate(url => window.location.href = url, server.PREFIX + "/grid.html")
      ]);
      expect_eq(response.ok(), true);
      expect_eq(std.mem.indexOf(u8, response.url(), "grid.html") != null, true);
      expect_eq(response.frame(), frame);
      expect_eq(std.mem.indexOf(u8, page.url(), "/frames/one-frame.html") != null, true);
    });
    test("WA-109 should fail when frame detaches", () => {
      page.goto(server.PREFIX + "/frames/one-frame.html");
      const frame = page.frames()[1];
      server.setRoute("/empty.html", () => {});
      server.setRoute("/one-style.css", () => {});
      const [error] = joinAll([ // migrated: Promise.all -> joinAll
        frame.waitForNavigation().catch(e => e),
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('iframe', frame => { frame.contentWindow.location.href = '/one-style.html'; }),
        // Make sure policy checks pass and navigation actually begins before removing the frame to avoid other errors
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: server.waitForRequest('/one-style.css').then(() => page.$eval('iframe', frame => window.builtins.setTimeout(() => frame.remove(), 0)))
      ]);
      expect_eq(std.mem.indexOf(u8, error.message, "waiting for navigation until \"load\"") != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "frame was detached") != null, true);
    });
}
