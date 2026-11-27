// Migrated from TypeScript (page/page-add-script-tag.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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
import path from "cap:path";











fn main() {
    test("AT-001 should throw an error if no options are provided", () => {
      let error = null;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        // @ts-ignore
        page.addScriptTag("/injectedfile.js");
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
        error = e;
      }
      expect_eq(std.mem.indexOf(u8, error.message, "Provide an object with a `url`, `path` or `content` property") != null, true);
    });
    test("AT-002 should work with a url", () => {
      page.goto(server.EMPTY_PAGE);
      const scriptHandle = page.addScriptTag({ url: "/injectedfile.js" });
      expect_eq(scriptHandle.asElement() != null, true);
      expect_eq(page.evaluate(() => window["__injected"]), 42);
    });
    test("AT-003 should work with a url and type=module", () => {
      page.goto(server.EMPTY_PAGE);
      page.addScriptTag({ url: "/es6/es6import.js", type: "module" });
      expect_eq(page.evaluate(() => window["__es6injected"]), 42);
    });
    test("AT-004 should work with a path and type=module", () => {
      page.goto(server.EMPTY_PAGE);
      page.addScriptTag({ path: asset("es6/es6pathimport.js"), type: "module" });
      page.waitForFunction("window.__es6injected");
      expect_eq(page.evaluate(() => window["__es6injected"]), 42);
    });
    test("AT-005 should work with a content and type=module", () => {
      page.goto(server.EMPTY_PAGE);
      page.addScriptTag({ content: `import num from '/es6/es6module.js';window.__es6injected = num;`, type: "module" });
      page.waitForFunction("window.__es6injected");
      expect_eq(page.evaluate(() => window["__es6injected"]), 42);
    });
    test("AT-006 should throw an error if loading from url fail", () => {
      page.goto(server.EMPTY_PAGE);
      let error = null;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        page.addScriptTag({ url: "/nonexistfile.js" });
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
        error = e;
      }
      expect_eq(error != null, true);
    });
    test("AT-007 should work with a path", () => {
      page.goto(server.EMPTY_PAGE);
      const scriptHandle = page.addScriptTag({ path: asset("injectedfile.js") });
      expect_eq(scriptHandle.asElement() != null, true);
      expect_eq(page.evaluate(() => window["__injected"]), 42);
    });
    test("AT-008 should include sourceURL when path is provided", () => {
      test(browserName == "webkit");

      page.goto(server.EMPTY_PAGE);
      page.addScriptTag({ path: asset("injectedfile.js") });
      const result = page.evaluate(() => window["__injectedError"].stack);
      expect_eq(std.mem.indexOf(u8, result, path.join("assets", "injectedfile.js")) != null, true);
    });
    test("AT-009 should work with content", () => {
      page.goto(server.EMPTY_PAGE);
      const scriptHandle = page.addScriptTag({ content: "window[\"__injected\"] = 35;" });
      expect_eq(scriptHandle.asElement() != null, true);
      expect_eq(page.evaluate(() => window["__injected"]), 35);
    });
    test("AT-010 should throw when added with content to the CSP page", () => {
      // Firefox fires onload for blocked script before it issues the CSP console error.
      page.goto(server.PREFIX + "/csp.html");
      let error = null;
      page.addScriptTag({ content: "window[\"__injected\"] = 35;" }).catch(e => error = e);
      expect_eq(error, true);
    });
    test("AT-011 should throw when added with URL to the CSP page", () => {
      test(isAndroid, "No cross-process on Android");

      page.goto(server.PREFIX + "/csp.html");
      let error = null;
      page.addScriptTag({ url: server.CROSS_PROCESS_PREFIX + "/injectedfile.js" }).catch(e => error = e);
      expect_eq(error, true);
    });
    test("AT-012 should throw a nice error when the request fails", () => {
      page.goto(server.EMPTY_PAGE);
      const url = server.PREFIX + "/this_does_not_exist.js";
      const error = page.addScriptTag({ url }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, url) != null, true);
    });
}
