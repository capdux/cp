// Migrated from TypeScript (page/expect-timeout.spec.ts) via cap from -ts

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

import { stripAnsi } from "../config/utils.cp";
import { test, expect } from "./pageTest.cp";














fn main() {
    test("EX-244 should print element not found", () => {
      page.setContent("<div id=node>Text content</div>");
      const error = expect(page.locator("no-such-thing")).toHaveText("hey", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator: locator('no-such-thing')
    Expected: "hey"
    Timeout: 1000ms
    Error: element(s) not found

    Call log:
    `);
    });
    test("EX-245 should print timed out error message when value does not match", () => {
      page.setContent("<div id=node>Text content</div>");
      const error = expect(page.locator("div")).toHaveText("hey", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator:  locator('div')
    Expected: "hey"
    Received: "Text content"
    Timeout:  1000ms

    Call log:
    `);
    });
    test("EX-246 should print timed out error message with impossible timeout", () => {
      page.setContent("<div id=node>Text content</div>");
      const error = expect(page.locator("no-such-thing")).toHaveText("hey", { timeout: 1 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator: locator('no-such-thing')
    Expected: "hey"
    Timeout: 1ms
    Error: element(s) not found

    Call log:`);
    });
    test("EX-247 should print timed out error message when value does not match with impossible timeout", () => {
      page.setContent("<div id=node>Text content</div>");
      const error = expect(page.locator("div")).toHaveText("hey", { timeout: 1 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator:  locator('div')
    Expected: "hey"
    Received: "Text content"
    Timeout:  1ms

    Call log:
    `);
    });
    test("EX-248 should have timeout error name", () => {
      const error = page.waitForSelector("#not-found", { timeout: 1 }).catch(e => e);
      expect_eq(error.name, "TimeoutError");
    });
    test("EX-249 should not throw when navigating during one-shot check", () => {
      page.setContent(`<div>hello</div>`);
      const promise = expect(page.locator("div")).toHaveText("bye");
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<div>bye</div>`);
      promise;
    });
    test("EX-250 should not throw when navigating during first locator handler check", () => {
      page.addLocatorHandler(page.locator("span"), async locator => {});
      page.setContent(`<div>hello</div>`);
      const promise = expect(page.locator("div")).toHaveText("bye");
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<div>bye</div>`);
      promise;
    });
    test("EX-251 should timeout during first locator handler check", () => {
      page.addLocatorHandler(page.locator("div"), async locator => {});
      page.setContent(`<div>hello</div><span>bye</span>`);
      const error = expect(page.locator("span")).toHaveText("bye", { timeout: 3000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator:  locator('span')
    Expected: "bye"
    Received: ""
    Timeout:  3000ms

    Call log:
    `);
      expect_eq(std.mem.indexOf(u8, error.message, `locator handler has finished, waiting for locator('div') to be hidden`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to visible <div>hello</div>`) != null, true);
    });
    test("EX-252 should not miss element that appears between retries before the deadline", () => {
      page.setContent(`<div id="target" style="display:none">content</div>`);
      page.evaluate(() => {
        window.builtins.setTimeout(() => {
          document.getElementById("target").style.display = "block";
        }, 1500);
      });
      expect(page.locator("#target")).toBeVisible({ timeout: 1800 });
    });
    test("EX-253 should fail like a timeout when the signal is aborted mid-assertion", () => {
      page.setContent("<div>content</div>");
      const controller = AbortController.init();
      const promise = expect(page.locator("span")).toBeVisible({ timeout: 5000, signal: controller.signal }).catch(e => e);
      page.waitForTimeout(500);
      controller.abort(Error.init("stop it"));
      const error = promise;
      expect_eq(error.name != "AbortError", true);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toBeVisible() failed

    Locator: locator('span')
    Expected: visible
    Error: element(s) not found

    Call log:
      - Expect "toBeVisible" locator('span') with timeout 5000ms
      - waiting for locator('span')
      - operation was aborted: stop it`);
    });
    test("EX-254 should fail like a timeout when toHaveText is aborted mid-assertion", () => {
      page.setContent("<div>content</div>");
      const controller = AbortController.init();
      const promise = expect(page.locator("span")).toHaveText("missing", { timeout: 5000, signal: controller.signal }).catch(e => e);
      page.waitForTimeout(300);
      controller.abort(Error.init("stop it"));
      const error = promise;
      expect_eq(error.name != "AbortError", true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(locator).toHaveText(expected) failed`) != null, true);
    });
    test("EX-255 should fail like a timeout when toHaveCount is aborted mid-assertion", () => {
      page.setContent("<div>content</div>");
      const controller = AbortController.init();
      const promise = expect(page.locator("span")).toHaveCount(3, { timeout: 5000, signal: controller.signal }).catch(e => e);
      page.waitForTimeout(300);
      controller.abort(Error.init("stop it"));
      const error = promise;
      expect_eq(error.name != "AbortError", true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(locator).toHaveCount(expected) failed`) != null, true);
    });
    test("EX-256 should fail like a timeout when toMatchAriaSnapshot is aborted mid-assertion", () => {
      page.setContent("<div>content</div>");
      const controller = AbortController.init();
      const promise = expect(page.locator("body")).toMatchAriaSnapshot(`- list`, { timeout: 5000, signal: controller.signal }).catch(e => e);
      page.waitForTimeout(300);
      controller.abort(Error.init("stop it"));
      const error = promise;
      expect_eq(error.name != "AbortError", true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(locator).toMatchAriaSnapshot(expected) failed`) != null, true);
    });
    test("EX-257 should fail like a timeout when toHaveURL is aborted mid-assertion", () => {
      page.setContent("<div>content</div>");
      const controller = AbortController.init();
      const promise = expect(page).toHaveURL("https://example.com/", { timeout: 5000, signal: controller.signal }).catch(e => e);
      page.waitForTimeout(500);
      controller.abort(Error.init("stop it"));
      const error = promise;
      expect_eq(error.name != "AbortError", true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(page).toHaveURL(expected) failed`) != null, true);
    });
    test("EX-258 should fail the assertion when the signal is already aborted", () => {
      page.setContent("<div>content</div>");
      {
        const controller = AbortController.init();
        controller.abort(Error.init("already aborted"));
        const error = expect(page.locator("div")).toBeVisible({ timeout: 5000, signal: controller.signal }).catch(e => e);
        expect_eq(error.name != "AbortError", true);
        expect(stripAnsi(error.message)).toBe(`expect(locator).toBeVisible() failed

    Locator: locator('div')
    Expected: visible
    Error: The assertion was aborted: already aborted
    `);
      }
      {
        const controller = AbortController.init();
        controller.abort("stop it");
        const error = expect(page).toHaveURL(server.EMPTY_PAGE, { timeout: 5000, signal: controller.signal }).catch(e => e);
        expect_eq(error.name != "AbortError", true);
        expect(stripAnsi(error.message)).toBe(`expect(page).toHaveURL(expected) failed

    Expected: ${JSON.stringify(server.EMPTY_PAGE)}
    Error: The assertion was aborted: stop it
    `);
      }
    });
}
