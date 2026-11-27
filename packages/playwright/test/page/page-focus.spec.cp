// Migrated from TypeScript (page/page-focus.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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
    test("FO-001 should work @smoke", () => {
      test(browserName == "firefox" && !isBidi);

      page.setContent(`<div id=d1 tabIndex=0></div>`);
      expect_eq(page.evaluate(() => document.activeElement.nodeName), "BODY");
      page.focus("#d1");
      expect_eq(page.evaluate(() => document.activeElement.id), "d1");
    });
    test("FO-002 should emit focus event", () => {
      page.setContent(`<div id=d1 tabIndex=0></div>`);
      let focused = false;
      page.exposeFunction("focusEvent", () => focused = true);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#d1', d1 => d1.addEventListener('focus', window['focusEvent']));
      page.focus("#d1");
      expect_eq(focused, true);
    });
    test("FO-003 should emit blur event", () => {
      page.setContent(`<div id=d1 tabIndex=0>DIV1</div><div id=d2 tabIndex=0>DIV2</div>`);
      page.focus("#d1");
      let focused = false;
      let blurred = false;
      page.exposeFunction("focusEvent", () => focused = true);
      page.exposeFunction("blurEvent", () => blurred = true);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#d1', d1 => d1.addEventListener('blur', window['blurEvent']));
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#d2', d2 => d2.addEventListener('focus', window['focusEvent']));
      page.focus("#d2");
      expect_eq(focused, true);
      expect_eq(blurred, true);
    });
    test("FO-004 should traverse focus", () => {
      page.setContent(`<input id="i1"><input id="i2">`);
      let focused = false;
      page.exposeFunction("focusEvent", () => focused = true);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#i2', i2 => (i2 as HTMLInputElement).addEventListener('focus', window['focusEvent']));

      page.focus("#i1");
      page.keyboard.type("First");
      page.keyboard.press("Tab");
      page.keyboard.type("Last");

      expect_eq(focused, true);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#i1', e => (e as HTMLInputElement).value)).toBe('First');
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#i2', e => (e as HTMLInputElement).value)).toBe('Last');
    });
    test("FO-005 should traverse focus in all directions", () => {
      page.setContent(`<input value="1"><input value="2"><input value="3">`);
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => (document.activeElement).value), "1");
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => (document.activeElement).value), "2");
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => (document.activeElement).value), "3");
      page.keyboard.press("Shift+Tab");
      expect_eq(page.evaluate(() => (document.activeElement).value), "2");
      page.keyboard.press("Shift+Tab");
      expect_eq(page.evaluate(() => (document.activeElement).value), "1");
    });
    test("FO-006 should traverse only form elements", () => {
      test(platform != "darwin" || browserName != "webkit", 
          "Chromium and WebKit both have settings for tab traversing all links, but it is only on by default in WebKit.");

      page.setContent(`
        <input id="input-1">
        <button id="button">button</button>
        <a href id="link">link</a>
        <input id="input-2">
      `);
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input-1");
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input-2");
      page.keyboard.press("Shift+Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input-1");
      page.keyboard.press("Alt+Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "button");
      page.keyboard.press("Alt+Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "link");
      page.keyboard.press("Alt+Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input-2");
      page.keyboard.press("Alt+Shift+Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "link");
      page.keyboard.press("Alt+Shift+Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "button");
      page.keyboard.press("Alt+Shift+Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input-1");
    });
    test("FO-007 clicking checkbox should activate it", () => {
      test(browserName == "webkit", "Safari does not focus on click");

      page.setContent(`<input type=checkbox></input>`);
      page.click("input");
      const nodeName = page.evaluate(() => document.activeElement.nodeName);
      expect_eq(nodeName, "INPUT");
    });
    test("FO-008 tab should cycle between single input and browser", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32339" }
    }, (_p) => {
      test(browserName == "chromium" && !isHeadlessShell, "Chromium keeps input focused.");
      it.fixme(browserName == "firefox");
      page.setContent(`<label for="input1">input1</label>
        <input id="input1">
        <script>
        {
          window.focusEvents = [];
          const input = document.getElementById('input1');
          input.addEventListener('blur', () => focusEvents.push('blur'));
          input.addEventListener('focus', () => focusEvents.push('focus'));
        }
        </script>`);
      expect_eq(page.evaluate(() => document.activeElement.tagName), "BODY");
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input1");
      expect_eq(page.evaluate(() => (window).focusEvents), ["focus"]);
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.tagName), "BODY");
      expect_eq(page.evaluate(() => (window).focusEvents), ["focus", "blur"]);
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input1");
      expect_eq(page.evaluate(() => (window).focusEvents), ["focus", "blur", "focus"]);
    });
    test("FO-009 tab should cycle between document elements and browser", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32339" }
    }, (_p) => {
      test(browserName == "chromium" && !isHeadlessShell, "Chromium keeps last input focused.");
      it.fixme(browserName == "firefox");
      page.setContent(`
        <input id="input1">
        <input id="input2">
        <script>
          window.focusEvents = [];
          {
            const input = document.getElementById('input1');
            input.addEventListener('blur', () => focusEvents.push('blur1'));
            input.addEventListener('focus', () => focusEvents.push('focus1'));
          }
          {
            const input = document.getElementById('input2');
            input.addEventListener('blur', () => focusEvents.push('blur2'));
            input.addEventListener('focus', () => focusEvents.push('focus2'));
          }
        </script>`);
      expect_eq(page.evaluate(() => document.activeElement.tagName), "BODY");
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input1");
      expect_eq(page.evaluate(() => (window).focusEvents), ["focus1"]);
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input2");
      expect_eq(page.evaluate(() => (window).focusEvents), ["focus1", "blur1", "focus2"]);
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.tagName), "BODY");
      expect_eq(page.evaluate(() => (window).focusEvents), ["focus1", "blur1", "focus2", "blur2"]);
      page.keyboard.press("Tab");
      expect_eq(page.evaluate(() => document.activeElement.id), "input1");
      expect_eq(page.evaluate(() => (window).focusEvents), ["focus1", "blur1", "focus2", "blur2", "focus1"]);
    });
    test("FO-010 keeps focus on element when attempting to focus a non-focusable element", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/14254" });

      page.setContent(`
          <div id="focusable" tabindex="0">focusable</div>
          <div id="non-focusable">not focusable</div>
          <script>
            window.eventLog = [];

            const focusable = document.getElementById("focusable");

            focusable.addEventListener('blur', () => window.eventLog.push('blur focusable'));
            focusable.addEventListener('focus', () => window.eventLog.push('focus focusable'));

            const nonFocusable = document.getElementById("non-focusable");
            nonFocusable.addEventListener('blur', () => window.eventLog.push('blur non-focusable'));
            nonFocusable.addEventListener('focus', () => window.eventLog.push('focus non-focusable'));
          </script>
        `);
      page.locator("#focusable").click();
      expect.soft(page.evaluate(() => document.activeElement?.id)).toBe("focusable");
      page.locator("#non-focusable").focus();
      expect.soft(page.evaluate(() => document.activeElement?.id)).toBe("focusable");
      expect.soft(page.evaluate(() => window["eventLog"])).toEqual([
        "focus focusable",
      ]);
    });
    test("FO-011 should move focus from an iframe to its parent", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43280" });
      server.setRoute("/frame.html", (req, res) => {
        res.setHeader("content-type", "text/html");
        res.end(`<input id=inner>`);
      });
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<iframe src="${server.PREFIX}/frame.html"></iframe><input id=outer>`);
      page.frameLocator("iframe").locator("#inner").focus();
      expect_eq(page.frameLocator("iframe").locator("#inner").evaluate(e => e == document.activeElement), true);
      page.evaluate(() => (document.getElementById("outer")).focus());
      expect(page.locator("#outer")).toBeFocused();
      expect_eq(page.frameLocator("iframe").locator("#inner").evaluate(e => e == document.activeElement), false);
    });
    test("FO-012 should tab from an iframe to its parent", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43280" });
      server.setRoute("/frame.html", (req, res) => {
        res.setHeader("content-type", "text/html");
        res.end(`<input id=inner>`);
      });
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<iframe src="${server.PREFIX}/frame.html"></iframe><input id=outer>`);
      page.frameLocator("iframe").locator("#inner").click();
      expect_eq(page.frameLocator("iframe").locator("#inner").evaluate(e => e == document.activeElement), true);
      page.keyboard.press("Tab");
      expect(page.locator("#outer")).toBeFocused();
    });
    test("FO-013 should move focus from the parent into its iframe", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43506" });
      server.setRoute("/frame.html", (req, res) => {
        res.setHeader("content-type", "text/html");
        res.end(`<input id=inner>`);
      });
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<iframe src="${server.PREFIX}/frame.html"></iframe><input id=outer>`);
      page.locator("#outer").focus();
      expect(page.locator("#outer")).toBeFocused();
      // Call native focus() exactly once, locator.focus() would hide the bug by calling it twice.
      page.evaluate(() => document.querySelector("iframe").contentDocument.querySelector("input").focus());
      expect(page.locator("iframe")).toBeFocused();
      expect(page.frameLocator("iframe").locator("#inner")).toBeFocused();
    });
}
