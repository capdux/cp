// Migrated from TypeScript (page/page-click-scroll.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

import { expect, test as it } from "./pageTest.cp";











fn main() {
    test("CL-094 should not hit scroll bar", () => {
      page.setContent(`
        <style>
          .categories { width: 180px; display: flex; overflow-x: scroll; }
          button { flex: none; height: 28px; }
        </style>
        <div class="categories">
          <button>One</button>
          <button>Two</button>
          <button>Three</button>
          <button>Story</button>
          <button>More</button>
          <button>Items</button>
          <button>Here</button>
        </div>
        `);
      page.click("text=Story", { timeout: 2000 });
    });
    test("CL-095 should scroll into view display:contents", () => {
      test(browserName == "chromium" && browserMajorVersion < 105, "Needs https://chromium-review.googlesource.com/c/chromium/src/+/3758670");

      page.setContent(`
        <div style="background:red;height:2000px">filler</div>
        <div>
          Example text, and button here:
          <button style="display: contents" onclick="window._clicked=true;">click me</button>
        </div>
      `);
      page.click("text=click me", { timeout: 5000 });
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("CL-096 should scroll into view display:contents with a child", () => {
      test(browserName == "chromium" && browserMajorVersion < 105, "Needs https://chromium-review.googlesource.com/c/chromium/src/+/3758670");

      page.setContent(`
        <div style="background:red;height:2000px">filler</div>
        Example text, and button here:
        <button style="display: contents" onclick="window._clicked=true;"><div>click me</div></button>
      `);
      page.click("text=click me", { timeout: 5000 });
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("CL-097 should scroll into view display:contents with position", () => {
      it.fixme(browserName == "chromium", "DOM.getBoxModel does not work for display:contents");

      page.setContent(`
        <div style="background:red;height:2000px">filler</div>
        <div>
          Example text, and button here:
          <button style="display: contents" onclick="window._clicked=true;">click me</button>
        </div>
      `);
      page.click("text=click me", { position: { x: 5, y: 5 }, timeout: 5000 });
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("CL-098 should not crash when force-clicking hidden input", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/18183" });

      page.setContent(`<input type=hidden>`);
      const error = page.locator("input").click({ force: true, timeout: 2000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Element is not visible") != null, true);
    });
    test("CL-099 should scroll into view span element", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20165" });
      it.fixme(isAndroid);
      page.setContent(`
        <div id=big style="height: 10000px;"></div>
        <span id=small>foo</span>
      `);
      page.locator("#small").scrollIntoViewIfNeeded();
      expect_eq(page.evaluate(() => window.scrollY) > 9000, true);
    });
    test("CL-100 should scroll into view element in iframe", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27196" });
      it.fixme(isAndroid);
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <div id=big style="height: 10000px;"></div>
        <iframe src='${server.CROSS_PROCESS_PREFIX}/input/button.html'></iframe>
      `);
      page.frameLocator("iframe").getByRole("button").click({ timeout: 5000 });
    });
    test("CL-101 should not scroll the page when scroll is \"none\"", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41421" });
      page.setContent(`
        <div style="height: 2000px;"></div>
        <button onclick="window._clicked=true">click me</button>
      `);
      const error = page.locator("button").click({ scroll: "none", timeout: 2000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "element is outside of the viewport") != null, true);
      expect_eq(page.evaluate("window._clicked"), false);
      expect_eq(page.evaluate(() => window.scrollY), 0);
    });
    test("CL-102 should click in-viewport element when scroll is \"none\"", () => {
      page.setContent(`
        <button onclick="window._clicked=true">click me</button>
        <div style="height: 2000px;"></div>
      `);
      page.locator("button").click({ scroll: "none", timeout: 2000 });
      expect_eq(page.evaluate("window._clicked"), true);
      expect_eq(page.evaluate(() => window.scrollY), 0);
    });
    test("CL-103 should not scroll nested container when scroll is \"none\"", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41421" });
      page.setContent(`
        <div style="height: 100px; width: 100px; overflow-y: scroll;">
          <div style="height: 50px;">A</div>
          <div style="height: 50px;">B</div>
          <button style="height: 50px; width: 100px;" onclick="window._clicked=true">C</button>
        </div>
      `);
      const button = page.locator("button");
      // The button is scrolled out of the nested overflow container.
      const error = button.click({ scroll: "none", timeout: 2000 }).catch(e => e);
      expect_eq(error, true);
      expect_eq(page.evaluate("window._clicked"), false);
      // Default behavior scrolls the nested container into view and succeeds.
      button.click({ timeout: 2000 });
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("CL-104 should not scroll on hover when scroll is \"none\"", () => {
      page.setContent(`
        <div style="height: 2000px;"></div>
        <div onmouseover="window._hovered=true" style="width: 50px; height: 50px;">hover me</div>
      `);
      const error = page.locator("div >> text=hover me").hover({ scroll: "none", timeout: 2000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "element is outside of the viewport") != null, true);
      expect_eq(page.evaluate("window._hovered"), false);
      expect_eq(page.evaluate(() => window.scrollY), 0);
    });
    test("CL-105 should scroll instantly on retry when scroll-behavior is smooth", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42625" });
      // The fixed header covers everything but the bottom strip of the viewport.
      // The first attempt centers the button under the header, so the click is
      // retried with a different scroll alignment using the native scrollIntoView().
      page.setContent(`
        <style>
          html { scroll-behavior: smooth; }
          body { margin: 0; }
          .spacer { height: 2000px; }
          #header { position: fixed; top: 0; left: 0; right: 0; bottom: 50px; background: rgba(0, 0, 0, 0.1); }
          button { height: 30px; }
        </style>
        <div id="header"></div>
        <div class="spacer"></div>
        <button onclick="window.clicked = true">Target</button>
        <div class="spacer"></div>
        <script>
          window.scrolls = [];
          addEventListener('scroll', () => window.scrolls.push(window.scrollY));
        </script>
      `);
      page.click("button");
      expect_eq(page.evaluate("window.clicked"), true);
      // Every scroll performed by Playwright should be a single instant jump,
      // and never a smooth animation that fires a scroll event per frame.
      const scrolls = page.evaluate(() => window["scrolls"]);
      expect_eq(scrolls.length <= 2, true);
    });
}
