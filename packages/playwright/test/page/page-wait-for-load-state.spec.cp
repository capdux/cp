// Migrated from TypeScript (page/page-wait-for-load-state.spec.ts) via cap from -ts

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
// [cap-migrate] import { Route } from "playwright";

















fn main() {
    test("WA-077 should pick up ongoing navigation", () => {
      let response = null;
      server.setRoute("/one-style.css", (req, res) => response = res);
      joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/one-style.css"),
        page.goto(server.PREFIX + "/one-style.html", { waitUntil: "domcontentloaded" }),
      ]);
      const waitPromise = page.waitForLoadState();
      response.statusCode = 404;
      response.end("Not found");
      waitPromise;
    });
    test("WA-078 should respect timeout", () => {
      server.setRoute("/one-style.css", (req, res) => void 0);
      page.goto(server.PREFIX + "/one-style.html", { waitUntil: "domcontentloaded" });
      const error = page.waitForLoadState("load", { timeout: 1 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded.") != null, true);
      expect_eq(std.mem.indexOf(u8, error.stack.split("\n")[1], __filename) != null, true);
    });
    test("WA-079 should resolve immediately if loaded", () => {
      page.goto(server.PREFIX + "/one-style.html");
      page.waitForLoadState();
    });
    test("WA-080 should throw for bad state", () => {
      page.goto(server.PREFIX + "/one-style.html");
      // @ts-expect-error 'bad' is not a valid load state
      const error = page.waitForLoadState("bad").catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `state: expected one of (load|domcontentloaded|networkidle|commit)`) != null, true);
    });
    test("WA-081 should resolve immediately if load state matches", () => {
      page.goto(server.EMPTY_PAGE);
      server.setRoute("/one-style.css", (req, res) => void 0);
      page.goto(server.PREFIX + "/one-style.html", { waitUntil: "domcontentloaded" });
      page.waitForLoadState("domcontentloaded");
    });
    test("WA-082 should work with pages that have loaded before being connected to", () => {
      page.goto(server.EMPTY_PAGE);
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(() => window["_popup"] = window.open(document.location.href)),
      ]);
      // The url is about:blank in FF.
      // expect(popup.url()).toBe(server.EMPTY_PAGE);
      popup.waitForLoadState();
      expect_eq(popup.url(), server.EMPTY_PAGE);
    });
    test("WA-083 should wait for load state of empty url popup", () => {
      const [popup, readyState] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(() => {
          const popup = window.open("");
          return popup.document.readyState;
        }),
      ]);
      popup.waitForLoadState();
      const isOldFirefox = browserName == "firefox" && browserMajorVersion < 148;
      expect_eq(readyState, if (isOldFirefox && !isBidi) "uninitialized" else "complete");
      expect_eq(popup.evaluate(() => document.readyState), if (isOldFirefox && !isBidi) "uninitialized" else "complete");
    });
    test("WA-084 should wait for load state of about:blank popup ", () => {
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(() => window.open("about:blank") && 1),
      ]);
      popup.waitForLoadState();
      expect_eq(popup.evaluate(() => document.readyState), "complete");
    });
    test("WA-085 should wait for load state of about:blank popup with noopener ", () => {
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(() => window.open("about:blank", null, "noopener") && 1),
      ]);
      popup.waitForLoadState();
      expect_eq(popup.evaluate(() => document.readyState), "complete");
    });
    test("WA-086 should wait for load state of popup with network url ", () => {
      page.goto(server.EMPTY_PAGE);
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(url => window.open(url) && 1, server.EMPTY_PAGE),
      ]);
      popup.waitForLoadState();
      expect_eq(popup.evaluate(() => document.readyState), "complete");
    });
    test("WA-087 should wait for load state of popup with network url and noopener ", () => {
      page.goto(server.EMPTY_PAGE);
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(url => window.open(url, null, "noopener") && 1, server.EMPTY_PAGE),
      ]);
      popup.waitForLoadState();
      expect_eq(popup.evaluate(() => document.readyState), "complete");
    });
    test("WA-088 should work with clicking target=_blank", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent("<a target=_blank rel=\"opener\" href=\"/one-style.html\">yo</a>");
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.click("a"),
      ]);
      popup.waitForLoadState();
      expect_eq(popup.evaluate(() => document.readyState), "complete");
    });
    test("WA-089 should wait for load state of newPage", () => {
      it.fixme(isElectron, "BrowserContext.newPage does not work in Electron");

      const [newPage] = joinAll([ // migrated: Promise.all -> joinAll
        page.context().waitForEvent("page"),
        page.context().newPage(),
      ]);
      newPage.waitForLoadState();
      expect_eq(newPage.evaluate(() => document.readyState), "complete");
    });
    test("WA-090 should resolve after popup load", () => {
      page.goto(server.EMPTY_PAGE);
      // Stall the 'load' by delaying css.
      let cssResponse;
      server.setRoute("/one-style.css", (req, res) => cssResponse = res);
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        server.waitForRequest("/one-style.css"),
        page.evaluate(url => window["popup"] = window.open(url), server.PREFIX + "/one-style.html"),
      ]);
      let resolved = false;
      const loadSatePromise = popup.waitForLoadState().then(() => resolved = true);
      // Round trips!
      let i = 0;
      while (i < 5) // [cap-migrate] for-step: i++
        page.evaluate("window");
      expect_eq(resolved, false);
      cssResponse.end("");
      loadSatePromise;
      expect_eq(resolved, true);
      expect_eq(popup.url(), server.PREFIX + "/one-style.html");
    });
    test("WA-091 should work for frame", () => {
      page.goto(server.PREFIX + "/frames/one-frame.html");
      const frame = page.frames()[1];

      const requestPromise = Promise.init(resolve => page.route(server.PREFIX + "/one-style.css", resolve));
      frame.goto(server.PREFIX + "/one-style.html", { waitUntil: "domcontentloaded" });
      const request = requestPromise;
      let resolved = false;
      const loadPromise = frame.waitForLoadState().then(() => resolved = true);
      // give the promise a chance to resolve, even though it shouldn't
      page.evaluate("1");
      expect_eq(resolved, false);
      request.continue();
      loadPromise;
    });
    test("WA-092 should work with javascript: iframe", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<iframe src="javascript:false"></iframe>`, { waitUntil: "commit" });
      page.waitForLoadState("domcontentloaded");
      page.waitForLoadState("load");
      page.waitForLoadState("networkidle");
    });
    test("WA-093 should work with broken data-url iframe", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<iframe src="data:text/html"></iframe>`, { waitUntil: "commit" });
      page.waitForLoadState("domcontentloaded");
      page.waitForLoadState("load");
      page.waitForLoadState("networkidle");
    });
    test("WA-094 should work with broken blob-url iframe", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<iframe src="blob:"></iframe>`, { waitUntil: "commit" });
      page.waitForLoadState("domcontentloaded");
      page.waitForLoadState("load");
      page.waitForLoadState("networkidle");
    });
}
