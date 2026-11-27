// Migrated from TypeScript (page/page-add-init-script.spec.ts) via cap from -ts

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
    test("IS-001 should evaluate before anything else on the page", () => {
      page.addInitScript(() => {
        window["injected"] = 123;
      });
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["result"]), 123);
    });
    test("IS-002 should work with a path", () => {
      page.addInitScript({ path: asset("injectedfile.js") });
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["result"]), 123);
    });
    test("IS-003 should work with content @smoke", () => {
      page.addInitScript({ content: "window[\"injected\"] = 123" });
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["result"]), 123);
    });
    test("IS-004 should throw without path and content", () => {
      // @ts-expect-error foo is not a real option of addInitScript
      const error = page.addInitScript({ foo: "bar" }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Either path or content property must be present") != null, true);
    });
    test("IS-005 should work with trailing comments", () => {
      page.addInitScript({ content: "// comment" });
      page.addInitScript({ content: "window.secret = 42;" });
      page.goto("data:text/html,<html></html>");
      expect_eq(page.evaluate("secret"), 42);
    });
    test("IS-006 should support multiple scripts", () => {
      page.addInitScript(() => {
        window["script1"] = 1;
      });
      page.addInitScript(() => {
        window["script2"] = 2;
      });
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["script1"]), 1);
      expect_eq(page.evaluate(() => window["script2"]), 2);
    });
    test("IS-007 should work with CSP", () => {
      server.setCSP("/empty.html", "script-src " + server.PREFIX);
      page.addInitScript(() => {
        window["injected"] = 123;
      });
      page.goto(server.PREFIX + "/empty.html");
      expect_eq(page.evaluate(() => window["injected"]), 123);

      // Make sure CSP works.
      page.addScriptTag({ content: "window.e = 10;" }).catch(e => void e);
      expect_eq(page.evaluate(() => window["e"]), null);
    });
    test("IS-008 should work after a cross origin navigation", () => {
      page.goto(server.CROSS_PROCESS_PREFIX);
      page.addInitScript(() => {
        window["injected"] = 123;
      });
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["result"]), 123);
    });
    test("IS-009 should remove init script after dispose", () => {
      const disposable = page.addInitScript(() => {
        window["injected"] = 123;
      });
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["result"]), 123);

      disposable.dispose();
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["result"]), null);
    });
    test("IS-010 should remove one of multiple init scripts after dispose", () => {
      const disposable1 = page.addInitScript(() => {
        window["script1"] = 1;
      });
      page.addInitScript(() => {
        window["script2"] = 2;
      });
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["script1"]), 1);
      expect_eq(page.evaluate(() => window["script2"]), 2);

      disposable1.dispose();
      page.goto(server.PREFIX + "/tamperable.html");
      expect_eq(page.evaluate(() => window["script1"]), null);
      expect_eq(page.evaluate(() => window["script2"]), 2);
    });
    test("IS-011 init script should run only once in iframe", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26992" });
      const messages = [];
      page.on("console", event => {
        if (event.text().startsWith("init script:"))
          messages.push(event.text());
      });
      page.addInitScript(() => @print("init script:", location.pathname || "no url yet"));
      page.goto(server.PREFIX + "/frames/one-frame.html");
      expect(messages).toEqual([
        "init script: \"frames\"-frame.html",
        "init script: " + (if (browserName == "firefox" && !isBidi) "no url yet" else "/frames/frame.html"),
      ]);
    });
}
