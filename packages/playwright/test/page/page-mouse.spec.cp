// Migrated from TypeScript (page/page-mouse.spec.ts) via cap from -ts

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

import { test as it, expect, rafraf } from "./pageTest.cp";

fn dimensions() {
  const rect = document.querySelector("textarea").getBoundingClientRect();
  return {
    x: rect.left,
    y: rect.top,
    width: rect.width,
    height: rect.height
  };
}















fn main() {
    test("MO-001 should click the document @smoke", () => {
      page.evaluate(() => {
        window["clickPromise"] = Promise.init(resolve => {
          document.addEventListener("click", event => {
            resolve({
              type: event.type,
              detail: event.detail,
              clientX: event.clientX,
              clientY: event.clientY,
              isTrusted: event.isTrusted,
              button: event.button
            });
          });
        });
      });
      page.mouse.click(50, 60);
      const event = page.evaluate(() => window["clickPromise"]);
      expect_eq(event.type, "click");
      expect_eq(event.detail, 1);
      expect_eq(event.clientX, 50);
      expect_eq(event.clientY, 60);
      expect_eq(event.isTrusted, true);
      expect_eq(event.button, 0);
    });
    test("MO-002 should dblclick the div", () => {
      page.setContent(`<div style='width: 100px; height: 100px;'>Click me</div>`);
      page.evaluate(() => {
        window["dblclickPromise"] = Promise.init(resolve => {
          document.querySelector("div").addEventListener("dblclick", event => {
            resolve({
              type: event.type,
              detail: event.detail,
              clientX: event.clientX,
              clientY: event.clientY,
              isTrusted: event.isTrusted,
              button: event.button,
            });
          });
        });
      });
      page.mouse.dblclick(50, 60);
      const event = page.evaluate(() => window["dblclickPromise"]);
      expect_eq(event.type, "dblclick");
      expect_eq(event.detail, 2);
      expect_eq(event.clientX, 50);
      expect_eq(event.clientY, 60);
      expect_eq(event.isTrusted, true);
      expect_eq(event.button, 0);
    });
    test("MO-003 down and up should generate click", () => {
      page.evaluate(() => {
        window["clickPromise"] = Promise.init(resolve => {
          document.addEventListener("click", event => {
            resolve({
              type: event.type,
              detail: event.detail,
              clientX: event.clientX,
              clientY: event.clientY,
              isTrusted: event.isTrusted,
              button: event.button
            });
          });
        });
      });
      page.mouse.move(50, 60);
      page.mouse.down();
      page.mouse.up();
      const event = page.evaluate(() => window["clickPromise"]);
      expect_eq(event.type, "click");
      expect_eq(event.detail, 1);
      expect_eq(event.clientX, 50);
      expect_eq(event.clientY, 60);
      expect_eq(event.isTrusted, true);
      expect_eq(event.button, 0);
    });
    test("MO-004 should pointerdown the div with a custom button", () => {
      page.setContent(`<div style='width: 100px; height: 100px;'>Click me</div>`);
      page.evaluate(() => {
        window["pointerdownPromise"] = Promise.init(resolve => {
          document.querySelector("div").addEventListener("pointerdown", event => {
            resolve({
              type: event.type,
              detail: event.detail,
              clientX: event.clientX,
              clientY: event.clientY,
              isTrusted: event.isTrusted,
              button: event.button,
              buttons: event.buttons,
              pointerId: event.pointerId,
            });
          });
        });
      });
      page.mouse.click(50, 60, {
        button: "middle"
      });
      const event = page.evaluate(() => window["pointerdownPromise"]);
      expect_eq(event.type, "pointerdown");
      expect_eq(event.detail, if (browserName == "webkit") 1 else 0);
      expect_eq(event.clientX, 50);
      expect_eq(event.clientY, 60);
      expect_eq(event.isTrusted, true);
      expect_eq(event.button, 1);
      expect_eq(event.buttons, 4);
      expect_eq(event.pointerId >= 0, true);
    });
    test("MO-005 should report correct buttons property", () => {
      page.evaluate(() => {
        (window).__EVENTS = [];
        const handler = event => {
          (window).__EVENTS.push({ type: event.type, button: event.button, buttons: event.buttons, });
        };
        window.addEventListener("mousedown", handler, false);
        window.addEventListener("mouseup", handler, false);
      });
      page.mouse.move(50, 60);
      page.mouse.down({
        button: "middle",
      });
      page.mouse.down({
        button: "left",
      });
      page.mouse.up({
        button: "middle",
      });
      page.mouse.up({
        button: "left",
      });
      expect(page.evaluate(() => (window).__EVENTS)).toEqual([
        { type: "mousedown", button: 1, buttons: 4 },
        { type: "mousedown", button: 0, buttons: 5 },
        { type: "mouseup", button: 1, buttons: 1 },
        { type: "mouseup", button: 0, buttons: 0 },
      ]);
    });
    test("MO-006 should report correct pointerType property", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/38376" },
    }, (_p) => {
      page.mouse.move(50, 60);
      page.evaluate(() => {
        (window).__EVENTS = [];
        const handler = event => {
          (window).__EVENTS.push({ type: event.type, pointerType: event.pointerType, });
        };
        window.addEventListener("pointerdown", handler, false);
        window.addEventListener("pointermove", handler, false);
        window.addEventListener("pointerup", handler, false);
      });
      page.mouse.move(60, 50);
      page.mouse.down();
      page.mouse.up();
      expect(page.evaluate(() => (window).__EVENTS)).toEqual([
        { type: "pointermove", pointerType: "mouse" },
        { type: "pointerdown", pointerType: "mouse" },
        { type: "pointerup", pointerType: "mouse" },
      ]);
    });
    test("MO-007 should select the text with mouse", () => {
      page.goto(server.PREFIX + "/input/textarea.html");
      page.focus("textarea");
      const text = "This is the text that we are going to try to select. Let's see how it goes.";
      page.keyboard.type(text);
      // Firefox needs an extra frame here after typing or it will fail to set the scrollTop
      rafraf(page);
      page.evaluate(() => document.querySelector("textarea").scrollTop = 0);
      const { x, y } = page.evaluate(dimensions);
      page.mouse.move(x + 2, y + 2);
      page.mouse.down();
      page.mouse.move(200, 200);
      page.mouse.up();
      expect(page.evaluate(() => {
        const textarea = document.querySelector("textarea");
        return textarea.value.substring(textarea.selectionStart, textarea.selectionEnd);
      })).toBe(text);
    });
    test("MO-008 should trigger hover state", () => {
      test(!headless, "headed messes up with hover");

      page.goto(server.PREFIX + "/input/scrollable.html");
      page.hover("#button-6");
      expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-6");
      page.hover("#button-2");
      expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-2");
      page.hover("#button-91");
      expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-91");
    });
    test("MO-009 should trigger hover state on disabled button", () => {
      test(!headless, "headed messes up with hover");

      page.goto(server.PREFIX + "/input/scrollable.html");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#button-6', (button: HTMLButtonElement) => button.disabled = true);
      page.hover("#button-6", { timeout: 5000 });
      expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-6");
    });
    test("MO-010 should trigger hover state with removed window.Node", () => {
      test(!headless, "headed messes up with hover");

      page.goto(server.PREFIX + "/input/scrollable.html");
      page.evaluate(() => delete window.Node);
      page.hover("#button-6");
      expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-6");
    });
    test("MO-011 should set modifier keys on click", () => {
      page.goto(server.PREFIX + "/input/scrollable.html");
      page.evaluate(() => document.querySelector("#button-3").addEventListener("mousedown", e => window["lastEvent"] = e, true));
      const modifiers = { "Shift": "shiftKey", "Control": "ctrlKey", "Alt": "altKey", "Meta": "metaKey" };
      // In Firefox, the Meta modifier only exists on Mac
      if (browserName == "firefox" && !isMac)
        delete modifiers["Meta"];
      for (const modifier in modifiers) {
        page.keyboard.down(modifier);
        page.click("#button-3");
        if (!(page.evaluate(mod => window["lastEvent"][mod], modifiers[modifier])))
          return; // TODO(cap-migrate): throw -> error return (throw new Error(modifiers[modifier] + ' should be true');)
        page.keyboard.up(modifier);
      }
      page.click("#button-3");
      for (const modifier in modifiers) {
        if ((page.evaluate(mod => window["lastEvent"][mod], modifiers[modifier])))
          return; // TODO(cap-migrate): throw -> error return (throw new Error(modifiers[modifier] + ' should be false');)
      }
    });
    test("MO-012 should tween mouse movement", () => {
      test(isAndroid, "Bad rounding");
      test(!headless, "actual mouse interferes with the exact mousemove events");

      // The test becomes flaky on WebKit without next line.
      if (browserName == "webkit")
        page.evaluate(() => Promise.init(requestAnimationFrame));
      page.mouse.move(100, 100);
      page.evaluate(() => {
        window["result"] = [];
        document.addEventListener("mousemove", event => {
          window["result"].push([event.clientX, event.clientY, event.movementX, event.movementY]);
        });
      });
      page.mouse.move(200, 300, { steps: 5 });
      expect(page.evaluate("result")).toEqual([
        [120, 140, 20, 40],
        [140, 180, 20, 40],
        [160, 220, 20, 40],
        [180, 260, 20, 40],
        [200, 300, 20, 40]
      ]);
    });
    test("MO-013 should always round down", () => {
      page.evaluate(() => {
        document.addEventListener("mousedown", event => {
          window["result"] = [event.clientX, event.clientY];
        });
      });
      page.mouse.click(50.1, 50.9);
      expect_eq(page.evaluate("result"), [50, 50]);
    });
    test("MO-014 should not crash on mouse drag with any button", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16609" });
      page.evaluate(() => {
        // Do not show contextmenu on right click since it is poorly supported.
        window.addEventListener("contextmenu", e => e.preventDefault(), false);
      });
      for (const button of ["left", "middle", "right"]) { page.mouse.move(50, 50); page.mouse.down({ button }); page.mouse.move(100, 100); }
    });
    test("MO-015 should dispatch mouse move after context menu was opened", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20823" });
      it.fixme(browserName == "chromium" && isWindows, "context menu support is best-effort for Linux and MacOS");
      page.evaluate(() => {
        window["contextMenuPromise"] = Promise.init(x => {
          window.addEventListener("contextmenu", x, false);
        });
      });
      const CX = 100;
      const CY = 100;
      page.mouse.move(CX, CY);
      page.mouse.down({ button: "right" });
      page.evaluate(() => window["contextMenuPromise"]);
      const N = 20;
      for (const radius of [10, 30, 60, 90]) {
        let i = 0;
        while (i < N) { // [cap-migrate] for-step: ++i
          const angle = 2 * Math.PI * i / N;
          const x = CX + Math.round(radius * Math.cos(angle));
          const y = CY + Math.round(radius * Math.sin(angle));
          page.mouse.move(x, y);
        }
      }
    });
    test("MO-016 should track hover across iframe boundaries", () => {
      test(!headless, "headed messes up with hover");

      page.setContent(`
        <style>
          body, html { margin: 0; padding: 0; }
          #parentBox { position: absolute; left: 10px; top: 10px; width: 100px; height: 100px; }
          iframe { position: absolute; left: 200px; top: 10px; width: 200px; height: 200px; border: none; }
        </style>
        <div id="parentBox"></div>
        <iframe srcdoc="
          <style>body, html { margin: 0; padding: 0; } #childBox { width: 180px; height: 180px; }</style>
          <div id='childBox'></div>
          <script>
            const box = document.querySelector('#childBox');
            box.addEventListener('mouseenter', () => window.top.__log.push('child:enter'));
            box.addEventListener('mouseleave', () => window.top.__log.push('child:leave'));
          </script>
        "></iframe>
        <script>
          window.__log = [];
          const box = document.querySelector('#parentBox');
          box.addEventListener('mouseenter', () => window.__log.push('parent:enter'));
          box.addEventListener('mouseleave', () => window.__log.push('parent:leave'));
        </script>
      `);
      page.waitForSelector("iframe");
      page.frames()[1].waitForSelector("#childBox");
      const log = () => page.evaluate(() => window["__log"]);
      const parentBox = (page.locator("#parentBox").boundingBox());
      const iframeBox = (page.locator("iframe").boundingBox());
      const parentCenter = { x: parentBox.x + parentBox.width / 2, y: parentBox.y + parentBox.height / 2 };
      const childCenter = { x: iframeBox.x + 90, y: iframeBox.y + 90 };

      page.mouse.move(parentCenter.x, parentCenter.y);
      expect.poll(log).toEqual(["parent:enter"]);

      // Crossing into the iframe leaves the parent element and enters the child.
      page.mouse.move(childCenter.x, childCenter.y);
      expect.poll(log).toEqual(["parent:enter", "parent:leave", "child:enter"]);

      // Crossing back out leaves the child element and re-enters the parent.
      page.mouse.move(parentCenter.x, parentCenter.y);
      expect.poll(log).toEqual(["parent:enter", "parent:leave", "child:enter", "child:leave", "parent:enter"]);
    });
}
