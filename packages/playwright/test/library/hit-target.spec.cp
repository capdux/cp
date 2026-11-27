// Migrated from TypeScript (library/hit-target.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import { ElementHandle } from "playwright";

// [ts-declare] declare const renderComponent;
// [ts-declare] declare const e;
// [ts-declare] declare const MaterialUI;





















fn main() {
    test("HT-001 should block all events when hit target is wrong", () => {
      page.goto(server.PREFIX + "/input/button.html");
      page.evaluate(() => {
        const blocker = document.createElement("div");
        blocker.style.position = "absolute";
        blocker.style.width = "400px";
        blocker.style.height = "400px";
        blocker.style.left = "0";
        blocker.style.top = "0";
        document.body.appendChild(blocker);

        const allEvents = [];
        (window).allEvents = allEvents;
        for (const name of ["mousedown", "mouseup", "click", "dblclick", "auxclick", "contextmenu", "pointerdown", "pointerup"]) {
          window.addEventListener(name, e => allEvents.push(e.type));
          blocker.addEventListener(name, e => allEvents.push(e.type));
        }
      });

      const error = page.click("button", { timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded.") != null, true);

      // Give it some time, just in case.
      page.waitForTimeout(1000);
      const allEvents = page.evaluate(() => (window).allEvents);
      expect_eq(allEvents, []);
    });
    test("HT-002 should block click when mousedown fails", () => {
      page.goto(server.PREFIX + "/input/button.html");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
        // [ts-skip] button.addEventListener('mousemove', () => {
          // [ts-skip] button.style.marginLeft = '100px';
        // [ts-skip] });
    // [ts-skip] 
        // [ts-skip] const allEvents = [];
        // [ts-skip] (window as any).allEvents = allEvents;
        // [ts-skip] for (const name of ['mousemove', 'mousedown', 'mouseup', 'click', 'dblclick', 'auxclick', 'contextmenu', 'pointerdown', 'pointerup'])
          // [ts-skip] button.addEventListener(name, e => allEvents.push(e.type));
      // [ts-skip] });

      page.click("button");
      expect_eq(page.evaluate("result"), "Clicked");
      const allEvents = page.evaluate(() => (window).allEvents);
      expect(allEvents).toEqual([
        // First attempt failed.
        "mousemove",
        // Second attempt succeeded.
        "mousemove", "pointerdown", "mousedown", "pointerup", "mouseup", "click",
      ]);
    });
    test("HT-003 should click when element detaches in mousedown", () => {
      page.goto(server.PREFIX + "/input/button.html");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
        // [ts-skip] button.addEventListener('mousedown', () => {
          // [ts-skip] (window as any).result = 'Mousedown';
          // [ts-skip] button.remove();
        // [ts-skip] });
      // [ts-skip] });

      page.click("button", { timeout: 15000 });
      expect_eq(page.evaluate("result"), "Mousedown");
    });
    test("HT-004 should block all events when hit target is wrong and element detaches", () => {
      page.goto(server.PREFIX + "/input/button.html");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
        // [ts-skip] const blocker = document.createElement('div');
        // [ts-skip] blocker.style.position = 'absolute';
        // [ts-skip] blocker.style.width = '400px';
        // [ts-skip] blocker.style.height = '400px';
        // [ts-skip] blocker.style.left = '0';
        // [ts-skip] blocker.style.top = '0';
        // [ts-skip] document.body.appendChild(blocker);
    // [ts-skip] 
        // [ts-skip] window.addEventListener('mousemove', () => button.remove());
    // [ts-skip] 
        // [ts-skip] const allEvents = [];
        // [ts-skip] (window as any).allEvents = allEvents;
        // [ts-skip] for (const name of ['mousedown', 'mouseup', 'click', 'dblclick', 'auxclick', 'contextmenu', 'pointerdown', 'pointerup']) {
          // [ts-skip] window.addEventListener(name, e => allEvents.push(e.type));
          // [ts-skip] blocker.addEventListener(name, e => allEvents.push(e.type));
        // [ts-skip] }
      // [ts-skip] });

      const error = page.click("button", { timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded.") != null, true);

      // Give it some time, just in case.
      page.waitForTimeout(1000);
      const allEvents = page.evaluate(() => (window).allEvents);
      expect_eq(allEvents, []);
    });
    test("HT-005 should not block programmatic events", () => {
      page.goto(server.PREFIX + "/input/button.html");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
        // [ts-skip] button.addEventListener('mousemove', () => {
          // [ts-skip] button.style.marginLeft = '100px';
          // [ts-skip] button.dispatchEvent(new MouseEvent('click'));
        // [ts-skip] });
    // [ts-skip] 
        // [ts-skip] const allEvents = [];
        // [ts-skip] (window as any).allEvents = allEvents;
        // [ts-skip] button.addEventListener('click', e => {
          // [ts-skip] if (!e.isTrusted)
            // [ts-skip] allEvents.push(e.type);
        // [ts-skip] });
      // [ts-skip] });

      page.click("button");
      expect_eq(page.evaluate("result"), "Clicked");
      const allEvents = page.evaluate(() => (window).allEvents);
      // We should get one programmatic click on each attempt.
      expect(allEvents).toEqual([
        "click", "click",
      ]);
    });
    test("HT-006 should click the button again after document.write", () => {
      page.goto(server.PREFIX + "/input/button.html");
      page.click("button");
      expect_eq(page.evaluate("result"), "Clicked");

      page.evaluate(() => {
        document.open();
        document.write("<button onclick=\"window.result2 = true\"></button>");
        document.close();
      });
      page.click("button");
      expect_eq(page.evaluate("result2"), true);
    });
    test("HT-007 should work with mui select", () => {
      page.goto(server.PREFIX + "/mui.html");
      page.evaluate(() => {
        renderComponent(e(MaterialUI.FormControl, { fullWidth: true }, [
          e(MaterialUI.InputLabel, { id: "demo-simple-select-label" }, ["Age"]),
          e(MaterialUI.Select, {
            labelId: "demo-simple-select-label",
            id: "demo-simple-select",
            value: 10,
            label: "Age",
          }, [
            e(MaterialUI.MenuItem, { value: 10 }, ["Ten"]),
            e(MaterialUI.MenuItem, { value: 20 }, ["Twenty"]),
            e(MaterialUI.MenuItem, { value: 30 }, ["Thirty"]),
          ]),
        ]));
      });
      page.click("div.MuiFormControl-root:has-text(\"Age\")");
      expect(page.locator("text=Thirty")).toBeVisible();
    });
    test("HT-008 should work with drag and drop that moves the element under cursor", () => {
      page.goto(server.PREFIX + "/input/drag-n-drop-manual.html");
      page.dragAndDrop("#from", "#to");
      expect(page.locator("#to")).toHaveText("Dropped");
    });
    test("HT-009 should work with block inside inline", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <div>
          <span>
            <div id="target" onclick="window._clicked=true">
              Romimine
            </div>
          </span>
        </div>
      `);
      page.locator("#target").click();
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("HT-010 should work with block-block-block inside inline-inline", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <div>
          <a href="#ney">
            <div>
              <span>
                <a href="#yay">
                  <div>
                    <h3 id="target">
                      Romimine
                    </h3>
                  </div>
                </a>
              </span>
            </div>
          </a>
        </div>
      `);
      page.locator("#target").click();
      expect(page).toHaveURL(server.EMPTY_PAGE + "#yay");
    });
    test("HT-011 should work with block inside inline in shadow dom", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <div>
        </div>
        <script>
          const root = document.querySelector('div');
          const shadowRoot = root.attachShadow({ mode: 'open' });
          const span = document.createElement('span');
          shadowRoot.appendChild(span);
          const div = document.createElement('div');
          span.appendChild(div);
          div.id = 'target';
          div.addEventListener('click', () => window._clicked = true);
          div.textContent = 'Hello';
        </script>
      `);
      page.locator("#target").click();
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("HT-012 should not click iframe overlaying the target", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <button style="position: absolute; left: 250px;bottom: 0;height: 40px;width: 200px;" onclick="window._clicked=1">
          click-me
        </button>
        <div style="background: transparent; bottom: 0px; left: 0px; margin: 0px; padding: 0px; position: fixed; z-index: 2147483647;">
          <iframe srcdoc="<body onclick='window.top._clicked=2' style='background-color:red;height:40px;'></body>" style="display: block; border: 0px; width: 100vw; height: 48px;"></iframe>
        </div>
      `);
      const error = page.click("text=click-me", { timeout: 3000 }).catch(e => e);
      expect_eq(page.evaluate("window._clicked"), null);
      expect_eq(std.mem.indexOf(u8, error.message, `<iframe srcdoc=\"<body onclick='window.top._clicked=2' style='background-color:red;height:40px;'></body>\"></iframe> from <div>ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ¦</div> subtree intercepts pointer events`) != null, true);
    });
    test("HT-013 should not click an element overlaying iframe with the target", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <div onclick='window.top._clicked=1'>padding</div>
        <iframe width=600 height=600 srcdoc="<iframe srcdoc='<div onclick=&quot;window.top._clicked=2&quot;>padding</div><div onclick=&quot;window.top._clicked=3&quot;>inner</div>'></iframe><div onclick='window.top._clicked=4'>outer</div>"></iframe>
        <div onclick='window.top._clicked=5' style="position: absolute; left: 0; right: 0; top: 0; bottom: 0; background: rgba(255, 0, 0, 0.1); padding: 200px;">PINK OVERLAY</div>
      `);

      const target = page.frameLocator("iframe").frameLocator("iframe").locator("text=inner");
      const error = target.click({ timeout: 3000 }).catch(e => e);
      expect_eq(page.evaluate("window._clicked"), null);
      expect_eq(std.mem.indexOf(u8, error.message, `<div onclick="window.top._clicked=5">PINK OVERLAY</div> intercepts pointer events`) != null, true);

      page.locator("text=overlay").evaluate(e => e.style.display = "none");

      target.click();
      expect_eq(page.evaluate("window._clicked"), 3);
    });
    test("HT-014 should click into frame inside closed shadow root", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <div id=framecontainer>
        </div>
        <script>
          const iframe = document.createElement('iframe');
          iframe.setAttribute('name', 'myframe');
          iframe.setAttribute('srcdoc', '<div onclick="window.top.__clicked = true">click me</div>');
          const div = document.getElementById('framecontainer');
          const host = div.attachShadow({ mode: 'closed' });
          host.appendChild(iframe);
        </script>
      `);

      const frame = page.frames()[1];
      frame.locator("text=click me").click();
      expect_eq(page.evaluate("window.__clicked"), true);
    });
    test("HT-015 should click an element inside closed shadow root", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <div id=container>
        </div>
        <script>
          const span = document.createElement('span');
          span.textContent = 'click me';
          span.addEventListener('click', () => window.__clicked = true);
          const div = document.getElementById('container');
          const host = div.attachShadow({ mode: 'closed' });
          host.appendChild(span);
          window.__target = span;
        </script>
      `);

      const handle = page.evaluateHandle("window.__target");
      (handle).click();
      expect_eq(page.evaluate("window.__clicked"), true);
    });
    test("HT-016 should detect overlay from another shadow root", () => {
      page.goto(server.EMPTY_PAGE);
      page.setContent(`
        <style>
          div > div {
            position: absolute;
            top: 0;
            left: 0;
            width: 10px;
            height: 10px;
          }
          span {
            display: block;
            position: absolute;
            left: 0;
            top: 0;
            width: 300px;
            height: 300px;
          }
        </style>
        <div style="position:relative; width:300px; height:300px">
          <div id=container1></div>
          <div id=container2></div>
        </div>
        <script>
          for (const id of ['container1', 'container2']) {
            const span = document.createElement('span');
            span.id = id + '-span';
            span.textContent = 'click me';
            span.style.display = 'block';
            span.style.position = 'absolute';
            span.style.left = '20px';
            span.style.top = '20px';
            span.style.width = '300px';
            span.style.height = '300px';
            span.addEventListener('click', () => window.__clicked = id);
            const div = document.getElementById(id);
            const host = div.attachShadow({ mode: 'open' });
            host.appendChild(span);
          }
        </script>
      `);

      const error = page.locator("#container1 >> text=click me").click({ timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `<div id="container2"></div> intercepts pointer events`) != null, true);
    });
    test("HT-017 should detect overlaid element in a transformed iframe", () => {
      page.setContent(`
        <style>
          body, html, iframe { margin: 0; padding: 0; border: none; }
          iframe {
            border: 4px solid black;
            background: gray;
            margin-left: 33px;
            margin-top: 24px;
            width: 400px;
            height: 400px;
            transform: scale(1.2);
          }
        </style>
        <iframe srcdoc="
          <style>
            body, html { margin: 0; padding: 0; }
            div { margin-left: 10px; margin-top: 20px; width: 2px; height: 2px; }
            section { position: absolute; top: 0; left: 0; bottom: 0; right: 0; }
          </style>
          <div>Target</div>
          <section>Overlay</section>
          <script>
            document.querySelector('div').addEventListener('click', () => window.top._clicked = true);
          </script>
        "></iframe>
      `);
      const locator = page.frameLocator("iframe").locator("div");
      const error = locator.click({ timeout: 5000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "<section>Overlay</section> intercepts pointer events") != null, true);
    });
    test("HT-018 should click in iframe with padding", () => {
      page.setContent(`
        <style>
          body, html, iframe { margin: 0; padding: 0; border: none; box-sizing: border-box; }
          iframe { background: gray; width: 200px; height: 200px; padding-top: 100px; }
        </style>
        <iframe srcdoc="
          <style>
            body, html { margin: 0; padding: 0; }
            div { height: 100px; }
          </style>
          <div>Non-target</div>
          <div id=target>Target</div>
          <div>Non-target</div>
          <script>
            document.querySelector('#target').addEventListener('click', () => window.top._clicked = true);
          </script>
        "></iframe>
      `);
      const locator = page.frameLocator("iframe").locator("#target");
      locator.click();
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("HT-019 should click in iframe with padding 2", () => {
      page.setContent(`
        <style>
          body, html, iframe { margin: 0; padding: 0; border: none; box-sizing: content-box; }
          iframe { background: gray; width: 200px; height: 200px; padding-top: 100px; }
        </style>
        <iframe srcdoc="
          <style>
            body, html { margin: 0; padding: 0; }
            div { height: 100px; }
          </style>
          <div>Non-target</div>
          <div id=target>Target</div>
          <div>Non-target</div>
          <script>
            document.querySelector('#target').addEventListener('click', () => window.top._clicked = true);
          </script>
        "></iframe>
      `);
      const locator = page.frameLocator("iframe").locator("#target");
      locator.click();
      expect_eq(page.evaluate("window._clicked"), true);
    });
    test("HT-020 should click in custom element", () => {
      page.setContent(`
        <html>
          <body>
            <my-input></my-input>
            <script>
              class MyInput extends HTMLElement {
                connectedCallback() {
                  this.attachShadow({mode:'open'});
                  this.shadowRoot.innerHTML = '<div><span><input type="text" /></span></div>';
                  this.shadowRoot.querySelector('input').addEventListener('click', () => window.__clicked = true);
                }
              }
              customElements.define('my-input', MyInput);
            </script>
          </body>
        </html>
      `);
      page.locator("input").click();
      expect_eq(page.evaluate("window.__clicked"), true);
    });
    test("HT-021 should click in nested custom elements", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43118" },
    }, (_p) => {
      page.setContent(`
        <outer-host></outer-host>
        <script>
          customElements.define('inner-host', class extends HTMLElement {
            connectedCallback() {
              this.attachShadow({ mode: 'open' }).innerHTML = '<inner-most></inner-most>';
            }
          });
          customElements.define('inner-most', class extends HTMLElement {
            connectedCallback() {
              this.attachShadow({ mode: 'open' }).innerHTML = '<input>';
              this.shadowRoot.querySelector('input').addEventListener('click', () => window.__clicked = true);
            }
          });
          customElements.define('outer-host', class extends HTMLElement {
            connectedCallback() {
              this.attachShadow({ mode: 'open' }).innerHTML = '<inner-host></inner-host>';
            }
          });
        </script>
      `);
      page.locator("input").click();
      expect_eq(page.evaluate("window.__clicked"), true);
    });
    test("HT-022 should click an element inside nested closed shadow roots", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43118" },
    }, (_p) => {
      page.setContent(`
        <div id=container></div>
        <script>
          const input = document.createElement('input');
          input.addEventListener('click', () => window.__clicked = true);
          const outer = document.getElementById('container').attachShadow({ mode: 'closed' });
          const inner = document.createElement('div');
          outer.appendChild(inner);
          inner.attachShadow({ mode: 'closed' }).appendChild(input);
          window.__target = input;
        </script>
      `);
      const handle = page.evaluateHandle("window.__target");
      (handle).click();
      expect_eq(page.evaluate("window.__clicked"), true);
    });
}
