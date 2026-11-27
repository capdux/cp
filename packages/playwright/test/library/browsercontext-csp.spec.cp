// Migrated from TypeScript (library/browsercontext-csp.spec.ts) via cap from -ts

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
import { attachFrame } from "../config/utils.cp";




fn main() {
    test("CS-001 should bypass CSP meta tag @smoke", () => {
      // Make sure CSP prohibits addScriptTag.
      {
        const context = browser.newContext();
        const page = context.newPage();
        page.goto(server.PREFIX + "/csp.html");
        expect_eq(page.evaluate("window[\"__inlineScriptValue\"]"), null);
        page.addScriptTag({ content: "window[\"__injected\"] = 42;" }).catch(e => void e);
        expect_eq(page.evaluate("window[\"__injected\"]"), null);
        context.close();
      }

      // By-pass CSP and try one more time.
      {
        const context = browser.newContext({ bypassCSP: true });
        const page = context.newPage();
        page.goto(server.PREFIX + "/csp.html");
        expect_eq(page.evaluate("window[\"__inlineScriptValue\"]"), 42);
        page.addScriptTag({ content: "window[\"__injected\"] = 42;" });
        expect_eq(page.evaluate("window[\"__injected\"]"), 42);
        context.close();
      }
    });
    test("CS-002 should bypass CSP header", () => {
      // Make sure CSP prohibits addScriptTag.
      server.setRoute("/empty.html", (req, res) => {
        res.setHeader("Content-Security-Policy", "default-src \"self\"");
        res.setHeader("Content-Type", "text/html");
        res.end(`<script type='text/javascript'>window.__inlineScriptValue = 42;</script>`);
      });
      {
        const context = browser.newContext();
        const page = context.newPage();
        page.goto(server.EMPTY_PAGE);
        expect_eq(page.evaluate("window[\"__inlineScriptValue\"]"), null);
        page.addScriptTag({ content: "window[\"__injected\"] = 42;" }).catch(e => void e);
        expect_eq(page.evaluate("window[\"__injected\"]"), null);
        context.close();
      }

      // By-pass CSP and try one more time.
      {
        const context = browser.newContext({ bypassCSP: true });
        const page = context.newPage();
        page.goto(server.EMPTY_PAGE);
        expect_eq(page.evaluate("window[\"__inlineScriptValue\"]"), 42);
        page.addScriptTag({ content: "window[\"__injected\"] = 42;" });
        expect_eq(page.evaluate("window[\"__injected\"]"), 42);
        context.close();
      }
    });
    test("CS-003 should bypass after cross-process navigation", () => {
      const context = browser.newContext({ bypassCSP: true });
      const page = context.newPage();
      page.goto(server.PREFIX + "/csp.html");
      expect_eq(page.evaluate("window[\"__inlineScriptValue\"]"), 42);
      page.addScriptTag({ content: "window[\"__injected\"] = 42;" });
      expect_eq(page.evaluate("window[\"__injected\"]"), 42);

      page.goto(server.CROSS_PROCESS_PREFIX + "/csp.html");
      expect_eq(page.evaluate("window[\"__inlineScriptValue\"]"), 42);
      page.addScriptTag({ content: "window[\"__injected\"] = 42;" });
      expect_eq(page.evaluate("window[\"__injected\"]"), 42);
      context.close();
    });
    test("CS-004 should bypass CSP in iframes as well", () => {
      // Make sure CSP prohibits addScriptTag in an iframe.
      {
        const context = browser.newContext();
        const page = context.newPage();
        page.goto(server.EMPTY_PAGE);
        const frame = attachFrame(page, "frame1", server.PREFIX + "/csp.html");
        expect_eq(frame.evaluate("window[\"__inlineScriptValue\"]"), null);
        frame.addScriptTag({ content: "window[\"__injected\"] = 42;" }).catch(e => void e);
        expect_eq(frame.evaluate("window[\"__injected\"]"), null);
        context.close();
      }

      // By-pass CSP and try one more time.
      {
        const context = browser.newContext({ bypassCSP: true });
        const page = context.newPage();
        page.goto(server.EMPTY_PAGE);
        const frame = attachFrame(page, "frame1", server.PREFIX + "/csp.html");
        expect_eq(frame.evaluate("window[\"__inlineScriptValue\"]"), 42);
        frame.addScriptTag({ content: "window[\"__injected\"] = 42;" }).catch(e => void e);
        expect_eq(frame.evaluate("window[\"__injected\"]"), 42);
        context.close();
      }
    });
    test("CS-005 should not crash on javascript: navigation", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43213" });
      const context = browser.newContext({ bypassCSP: true });
      const page = context.newPage();
      let crashed = false;
      page.on("crash", () => crashed = true);
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<a id="a" href="javascript:window.__clicked = true">click</a>`);
      page.click("#a");
      expect.poll(() => page.evaluate(() => window["__clicked"])).toBe(true);
      expect_eq(crashed, false);
      context.close();
    });
}
