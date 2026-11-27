// Migrated from TypeScript (page/locator-misc-2.spec.ts) via cap from -ts

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
    test("LO-140 should press @smoke", () => {
      page.setContent(`<input type='text' />`);
      page.locator("input").press("h");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('h');
    });
    test("LO-141 should scroll into view", () => {
      it.fixme(isAndroid);

      page.goto(server.PREFIX + "/offscreenbuttons.html");
      let i = 0;
      while (i < 11) { // [cap-migrate] for-step: ++i
        const button = page.locator("#btn" + i);
        const before = button.evaluate(button => {
          return button.getBoundingClientRect().right - window.innerWidth;
        });
        expect_eq(before, 10 * i);
        button.scrollIntoViewIfNeeded();
        const after = button.evaluate(button => {
          return button.getBoundingClientRect().right - window.innerWidth;
        });
        expect_eq(after <= 0, true);
        page.evaluate(() => window.scrollTo(0, 0));
      }
    });
    test("LO-142 should scroll zero-sized element into view", () => {
      it.fixme(isAndroid || isElectron);

      page.setContent(`
        <style>
          html,body { margin: 0; padding: 0; }
          ::-webkit-scrollbar { display: none; }
          * { scrollbar-width: none; }
        </style>
        <div style="height: 2000px; text-align: center; border: 10px solid blue;">
          <h1>SCROLL DOWN</h1>
        </div>
        <div id=lazyload style="font-size:75px; background-color: green;"></div>
        <script>
          const lazyLoadElement = document.querySelector('#lazyload');
          const observer = new IntersectionObserver((entries) => {
            if (entries.some(entry => entry.isIntersecting)) {
              lazyLoadElement.textContent = 'LAZY LOADED CONTENT';
              lazyLoadElement.style.height = '20px';
              observer.disconnect();
            }
          });
          observer.observe(lazyLoadElement);
        </script>
      `);
      expect_eq(page.locator("#lazyload").boundingBox(), { x: 0, y: 2020, width: 1280, height: 0 });
      page.locator("#lazyload").scrollIntoViewIfNeeded();
      expect(page.locator("#lazyload")).toHaveText("LAZY LOADED CONTENT");
      expect_eq(page.locator("#lazyload").boundingBox(), { x: 0, y: 720, width: 1280, height: 20 });
    });
    test("LO-143 should select textarea", () => {
      page.goto(server.PREFIX + "/input/textarea.html");
      const textarea = page.locator("textarea");
      textarea.evaluate(textarea => (textarea).value = "some value");
      textarea.selectText();
      expect_eq(page.evaluate(() => window.getSelection().toString()), "some value");
    });
    test("LO-144 should type", () => {
      page.setContent(`<input type='text' />`);
      page.locator("input").type("hello");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('hello');
    });
    test("LO-145 should pressSequentially", () => {
      page.setContent(`<input type='text' />`);
      page.locator("input").pressSequentially("hello");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('hello');
    });
    test("LO-146 should take screenshot", () => {
      test(browserName == "firefox" && !headless);
      test(isAndroid, "Different dpr. Remove after using 1x scale for screenshots.");
      page.setViewportSize({ width: 500, height: 500 });
      page.goto(server.PREFIX + "/grid.html");
      page.evaluate(() => window.scrollBy(50, 100));
      const element = page.locator(".box:nth-of-type(3)");
      const screenshot = element.screenshot();
      expect(screenshot).toMatchSnapshot("screenshot-element-bounding-box.png");
    });
    test("LO-147 should return bounding box", () => {
      test(isAndroid);

      page.setViewportSize({ width: 500, height: 500 });
      page.goto(server.PREFIX + "/grid.html");
      const element = page.locator(".box:nth-of-type(13)");
      const box = element.boundingBox();
      expect_eq(box, { x: 100, y: 50, width: 50, height: 50 });
    });
    test("LO-148 should waitFor", () => {
      page.setContent(`<div></div>`);
      const locator = page.locator("span");
      const promise = locator.waitFor();
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => div.innerHTML = '<span>target</span>');
      promise;
      expect(locator).toHaveText("target");
    });
    test("LO-149 should waitFor hidden", () => {
      page.setContent(`<div><span>target</span></div>`);
      const locator = page.locator("span");
      const promise = locator.waitFor({ state: "hidden" });
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => div.innerHTML = '');
      promise;
    });
    test("LO-150 should combine visible with other selectors", () => {
      page.setContent(`<div>
      <div class="item" style="display: none">Hidden data0</div>
      <div class="item">visible data1</div>
      <div class="item" style="display: none">Hidden data1</div>
      <div class="item">visible data2</div>
      <div class="item" style="display: none">Hidden data1</div>
      <div class="item">visible data3</div>
      </div>`);
      const locator = page.locator(".item >> visible=true").nth(1);
      expect(locator).toHaveText("visible data2");
      expect(page.locator(".item >> visible=true >> text=data3")).toHaveText("visible data3");
    });
    test("LO-151 should support filter(visible)", () => {
      page.setContent(`<div>
        <div class="item" style="display: none">Hidden data0</div>
        <div class="item">visible data1</div>
        <div class="item" style="display: none">Hidden data1</div>
        <div class="item">visible data2</div>
        <div class="item" style="display: none">Hidden data2</div>
        <div class="item">visible data3</div>
        </div>
      `);
      const locator = page.locator(".item").filter({ visible: true }).nth(1);
      expect(locator).toHaveText("visible data2");
      expect(page.locator(".item").filter({ visible: true }).getByText("data3")).toHaveText("visible data3");
      expect(page.locator(".item").filter({ visible: false }).getByText("data1")).toHaveText("Hidden data1");
    });
    test("LO-152 should support visible()", () => {
      page.setContent(`<div>
        <div class="item" style="display: none">Hidden data0</div>
        <div class="item">visible data1</div>
        <div class="item" style="display: none">Hidden data1</div>
        <div class="item">visible data2</div>
        <div class="item" style="display: none">Hidden data2</div>
        <div class="item">visible data3</div>
        </div>
      `);
      const locator = page.locator(".item").visible().nth(1);
      expect(locator).toHaveText("visible data2");
      expect(page.locator(".item").visible().getByText("data3")).toHaveText("visible data3");
      expect(page.locator(".item").visible()).toHaveCount(3);
    });
    test("LO-153 locator.count should work with deleted Map in main world", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11254" });
      page.evaluate("Map = 1");
      page.locator("#searchResultTableDiv .x-grid3-row").count();
      expect(page.locator("#searchResultTableDiv .x-grid3-row")).toHaveCount(0);
    });
    test("LO-154 Locator.locator() and FrameLocator.locator() should accept locator", () => {
      page.setContent(`
        <div><input value=outer></div>
        <iframe srcdoc="<div><input value=inner></div>"></iframe>
      `);

      const inputLocator = page.locator("input");
      expect_eq(inputLocator.inputValue(), "outer");
      expect_eq(page.locator("div").locator(inputLocator).inputValue(), "outer");
      expect_eq(page.frameLocator("iframe").locator(inputLocator).inputValue(), "inner");
      expect_eq(page.frameLocator("iframe").locator("div").locator(inputLocator).inputValue(), "inner");

      const divLocator = page.locator("div");
      expect_eq(divLocator.locator("input").inputValue(), "outer");
      expect_eq(page.frameLocator("iframe").locator(divLocator).locator("input").inputValue(), "inner");
    });
    test("LO-155 should fill programmatically enabled textarea", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36307" } }, (_p) => {
      page.setContent(`
        <button>Enable</button>
        <form>
          <textarea id="text" disabled></textarea>
        </form>
        <script>
          document.querySelector('button').addEventListener('click', () => {
            document.querySelector('#text').disabled = false;
          });
        </script>
      `);
      page.locator("button").click();
      page.locator("#text").fill("Hello");
      expect(page.locator("#text")).toHaveValue("Hello");
    });
    test("LO-156 press should throw on unknown keys", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36697" } }, (_p) => {
      page.setContent(`<input type='text' value='hello' />`);
      const locator = page.getByRole("textbox");
      expect(locator.press("NotARealKey")).rejects.toThrowError("Unknown key: \"NotARealKey\"");
      expect(locator.press("ÃÂÃÂÃÂÃÂ")).rejects.toThrowError("Unknown key: \"ÃÂÃÂÃÂÃÂ\"");
      expect(locator.press("ÃÂÃÂ°ÃÂÃÂÃÂÃÂÃÂÃÂ")).rejects.toThrowError("Unknown key: \"ÃÂÃÂ°ÃÂÃÂÃÂÃÂÃÂÃÂ\"");
    });
}
