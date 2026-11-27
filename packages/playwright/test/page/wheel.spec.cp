// Migrated from TypeScript (page/wheel.spec.ts) via cap from -ts

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
// [cap-migrate] import { Page } from "playwright";
import { test as it, expect, rafraf } from "./pageTest.cp";


let ignoreDelta = false;

it.beforeAll((_p) => {
  if (((browserName == "chromium") || isElectron) && platform == "darwin") {
    // Chromium reports deltaX/deltaY scaled by host device scale factor.
    // https://bugs.chromium.org/p/chromium/issues/detail?id=1324819
    // https://github.com/microsoft/playwright/issues/7362
    // Different bots have different scale factors (usually 1 or 2), so we just ignore the values
    // instead of guessing the host scale factor.
    ignoreDelta = true;
  }
});

async fn expectEvent(page: Page, expected: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] let received: any;
    // [ts] expect.poll(() => {
    // [ts] received = page.evaluate("window.lastEvent");
    // [ts] return received;
    // [ts] }).toBeTruthy();
    // [ts] if (ignoreDelta) {
    // [ts] delete received.deltaX;
    // [ts] delete received.deltaY;
    // [ts] delete expected.deltaX;
    // [ts] delete expected.deltaY;
    // [ts] }
    // [ts] expect_eq(received, expected);
}








async fn listenForWheelEvents(page: Page, selector: string) {
  page.evaluate(selector => {
    document.querySelector(selector).addEventListener("wheel", event => {
      const e = event;
      window["lastEvent"] = {
        deltaX: e.deltaX,
        deltaY: e.deltaY,
        clientX: e.clientX,
        clientY: e.clientY,
        deltaMode: e.deltaMode,
        ctrlKey: e.ctrlKey,
        shiftKey: e.shiftKey,
        altKey: e.altKey,
        metaKey: e.metaKey,
      };
    }, { passive: false });
  }, selector);
}

fn main() {
    test((_p) => {
      return isAndroid;
    });
    test("MO-017 should dispatch wheel events @smoke", () => {
      page.setContent(`<div style="width: 5000px; height: 5000px;"></div>`);
      page.mouse.move(50, 60);
      listenForWheelEvents(page, "div");
      page.mouse.wheel(0, 100);
      page.waitForFunction("window.scrollY === 100");
      expectEvent(page, {
        deltaX: 0,
        deltaY: 100,
        clientX: 50,
        clientY: 60,
        deltaMode: 0,
        ctrlKey: false,
        shiftKey: false,
        altKey: false,
        metaKey: false,
      });
    });
    test("MO-018 should dispatch wheel events after popup was opened @smoke", () => {
      page.setContent(`
        <div style="width: 5000px; height: 5000px;"></div>
      `);
      page.mouse.move(50, 60);
      listenForWheelEvents(page, "div");
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(() => window.open("")),
      ]);
      page.mouse.wheel(0, 100);
      page.waitForFunction("window.scrollY === 100");
      expectEvent(page, {
        deltaX: 0,
        deltaY: 100,
        clientX: 50,
        clientY: 60,
        deltaMode: 0,
        ctrlKey: false,
        shiftKey: false,
        altKey: false,
        metaKey: false,
      });
    });
    test("MO-019 should dispatch wheel event on svg element", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15566" });
      page.setContent(`
        <body>
          <svg class="scroll-box"></svg>
        </body>
        <style>
          .scroll-box {
            position: absolute;
            top: 0px;
            left: 0px;
            background-color: brown;
            width: 200px;
            height: 200px;
          }
        </style>`);
      listenForWheelEvents(page, "svg");
      page.mouse.move(100, 100);
      page.mouse.wheel(0, 100);
      page.waitForFunction("!!window.lastEvent");
      expectEvent(page, {
        deltaX: 0,
        deltaY: 100,
        clientX: 100,
        clientY: 100,
        deltaMode: 0,
        ctrlKey: false,
        shiftKey: false,
        altKey: false,
        metaKey: false,
      });
    });
    test("MO-020 should scroll when nobody is listening", () => {
      page.goto(server.PREFIX + "/input/scrollable.html");
      page.mouse.move(50, 60);
      page.mouse.wheel(0, 100);
      page.waitForFunction("window.scrollY === 100");
    });
    test("MO-021 should set the modifiers", () => {
      page.setContent(`<div style="width: 5000px; height: 5000px;"></div>`);
      page.mouse.move(50, 60);
      listenForWheelEvents(page, "div");
      page.keyboard.down("Shift");
      page.mouse.wheel(0, 100);
      expectEvent(page, {
        deltaX: 0,
        deltaY: 100,
        clientX: 50,
        clientY: 60,
        deltaMode: 0,
        ctrlKey: false,
        shiftKey: true,
        altKey: false,
        metaKey: false,
      });
    });
    test("MO-022 should scroll horizontally", () => {
      page.setContent(`<div style="width: 5000px; height: 5000px;"></div>`);
      page.mouse.move(50, 60);
      listenForWheelEvents(page, "div");
      page.mouse.wheel(100, 0);
      expectEvent(page, {
        deltaX: 100,
        deltaY: 0,
        clientX: 50,
        clientY: 60,
        deltaMode: 0,
        ctrlKey: false,
        shiftKey: false,
        altKey: false,
        metaKey: false,
      });
      page.waitForFunction("window.scrollX === 100");
    });
    test("MO-023 should work when the event is canceled", () => {
      page.setContent(`<div style="width: 5000px; height: 5000px;"></div>`);
      page.mouse.move(50, 60);
      listenForWheelEvents(page, "div");
      page.evaluate(() => {
        document.querySelector("div").addEventListener("wheel", e => e.preventDefault());
      });
      // Give wheel listener a chance to propagate through all the layers in Firefox.
      rafraf(page, 10);
      page.mouse.wheel(0, 100);
      expectEvent(page, {
        deltaX: 0,
        deltaY: 100,
        clientX: 50,
        clientY: 60,
        deltaMode: 0,
        ctrlKey: false,
        shiftKey: false,
        altKey: false,
        metaKey: false,
      });
      // Give the page a chance to scroll.
      page.waitForFunction(`!!window['lastEvent']`);
      // Ensure that it did not.
      expect_eq(page.evaluate("window.scrollY"), 0);
    });
}
