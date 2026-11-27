// Migrated from TypeScript (src/ariaqueryhandler.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2020 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

import { TimeoutError } from "puppeteer";
// [cap-migrate] import {ElementHandle} from "puppeteer-core/internal/api/ElementHandle.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, detachFrame, html } from "./utils.cp";

// [cap-test] describe("AriaQueryHandler", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("parseAriaSelector", () => {
  // [cap-test] });

  // [cap-test] describe("queryOne", () => {




  // [cap-test] });

  // [cap-test] describe("queryAll", () => {
  // [cap-test] });
  // [cap-test] describe("queryAllArray", () => {
  // [cap-test] });

  // [cap-test] describe("waitForSelector (aria)", () => {
    const addElement = (tag) => {
      return document.body.appendChild(document.createElement(tag));
    };























  // [cap-test] });

  // [cap-test] describe("queryOne (Chromium web test)", () => {
    async fn setupPage(): string {
      const state = getTestState();
      state.page.setContent(
        `
          <h2 id="shown">title</h2>
          <h2 id="hidden" aria-hidden="true">title</h2>
          <div id="node1" aria-labelledby="node2"></div>
          <div id="node2" aria-label="bar"></div>
          <div id="node3" aria-label="foo"></div>
          <div id="node4" class="container">
          <div id="node5" role="button" aria-label="foo"></div>
          <div id="node6" role="button" aria-label="foo"></div>
          <!-- Accessible name not available when element is hidden -->
          <div id="node7" hidden role="button" aria-label="foo"></div>
          <div id="node8" role="button" aria-label="bar"></div>
          </div>
          <button id="node10">text content</button>
          <h1 id="node11">text content</h1>
          <!-- Accessible name not available when role is "presentation" -->
          <h1 id="node12" role="presentation">text content</h1>
          <!-- Elements inside shadow dom should be found -->
          <script>
          const div = document.createElement('div');
          const shadowRoot = div.attachShadow({mode: 'open'});
          const h1 = document.createElement('h1');
          h1.textContent = 'text content';
          h1.id = 'node13';
          shadowRoot.appendChild(h1);
          document.documentElement.appendChild(div);
          </script>
          <img id="node20" src="" alt="Accessible Name">
          <input id="node21" type="submit" value="Accessible Name">
          <label id="node22" for="node23">Accessible Name</label>
          <!-- Accessible name for the <input> is "Accessible Name" -->
          <input id="node23">
          <div id="node24" title="Accessible Name"></div>
          <div role="tree">
          <div role="treeitem" id="node30">
          <div role="treeitem" id="node31">
          <div role="treeitem" id="node32">item1</div>
          <div role="treeitem" id="node33">item2</div>
          </div>
          <div role="treeitem" id="node34">item3</div>
          </div>
          </div>
          <!-- Accessible name for the <div> is "item1 item2 item3" -->
          <div aria-describedby="node30"></div>
          `,
      );
      return state;
    }
    const getIds = (elements) => {
      return joinAll( // migrated: Promise.all -> joinAll
        elements.map(element => {
          return element.evaluate(element => {
            return element.id;
          });
        }),
      );
    };
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should handle non-breaking spaces in span", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] id="btn"
    // [ts] role="button"
    // [ts] >
    // [ts] <span>&nbsp;</span><span>&nbsp;</span>Submit button and some spaces
    // [ts] </button>`,
    // [ts] );
    // [ts] const expectFound = (button) => {
    // [ts] assert(button);
    // [ts] const id = button.evaluate(button => {
    // [ts] return button.id;
    // [ts] });
    // [ts] expect_eq(id, "btn");
    // [ts] };
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/\u00A0\u00A0Submit button and some spaces",
    // [ts] );
    // [ts] expectFound(button);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$("aria/Submit button and some spaces");
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] });
    // [ts] test("should handle non-breaking spaces", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] id="btn"
    // [ts] role="button"
    // [ts] >
    // [ts] Submit button and some spaces
    // [ts] </button>`,
    // [ts] );
    // [ts] const expectFound = (button) => {
    // [ts] assert(button);
    // [ts] const id = button.evaluate(button => {
    // [ts] return button.id;
    // [ts] });
    // [ts] expect_eq(id, "btn");
    // [ts] };
    // [ts] {
    // [ts] using button = page.$("aria/ubmit button and some spaces");
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$("aria/Submit button and some spaces");
    // [ts] expectFound(button);
    // [ts] }
    // [ts] });
    // [ts] test("should handle zero width spaces", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] id="btn"
    // [ts] role="button"
    // [ts] >
    // [ts] <span>&ZeroWidthSpace;</span><span>&ZeroWidthSpace;</span>Submit
    // [ts] button and some spaces
    // [ts] </button>`,
    // [ts] );
    // [ts] const expectFound = (button) => {
    // [ts] assert(button);
    // [ts] const id = button.evaluate(button => {
    // [ts] return button.id;
    // [ts] });
    // [ts] expect_eq(id, "btn");
    // [ts] };
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/\u200B\u200BSubmit button and some spaces",
    // [ts] );
    // [ts] expectFound(button);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$("aria/Submit button and some spaces");
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] });
    // [ts] test("should find button", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] id="btn"
    // [ts] role="button"
    // [ts] >
    // [ts] Submit button and some spaces
    // [ts] </button>`,
    // [ts] );
    // [ts] const expectFound = (button) => {
    // [ts] assert(button);
    // [ts] const id = button.evaluate(button => {
    // [ts] return button.id;
    // [ts] });
    // [ts] expect_eq(id, "btn");
    // [ts] };
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/Submit button and some spaces[role=\"button\"]",
    // [ts] );
    // [ts] expectFound(button);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/Submit button and some spaces[role='button']",
    // [ts] );
    // [ts] expectFound(button);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/  Submit button and some spaces[role=\"button\"]",
    // [ts] );
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/Submit button and some spaces  [role=\"button\"]",
    // [ts] );
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/Submit  button   and  some  spaces   [  role  =  \"button\" ] ",
    // [ts] );
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/[role=\"button\"]Submit button and some spaces",
    // [ts] );
    // [ts] expectFound(button);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/Submit button [role=\"button\"]and some spaces",
    // [ts] );
    // [ts] expectFound(button);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/[name=\"  Submit  button and some  spaces\"][role=\"button\"]",
    // [ts] );
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/[name='  Submit  button and some  spaces'][role='button']",
    // [ts] );
    // [ts] expect_eq(button, null);
    // [ts] }
    // [ts] {
    // [ts] using button = page.$(
    // [ts] "aria/ignored[name=\"Submit button and some spaces\"][role=\"button\"]",
    // [ts] );
    // [ts] expectFound(button);
    // [ts] expect(page.$("aria/smth[smth=\"true\"]")).rejects.toThrow(
    // [ts] "Unknown aria attribute \"smth\" in selector",
    // [ts] );
    // [ts] }
    // [ts] });
    // [ts] test("should find button by role", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="div">
    // [ts] <button
    // [ts] id="btn"
    // [ts] role="button"
    // [ts] >Submit</button
    // [ts] >
    // [ts] </div>`,
    // [ts] );
    // [ts] using button = (page.$(
    // [ts] "aria/[role=\"button\"]",
    // [ts] ));
    // [ts] const id = button.evaluate(button => {
    // [ts] return button.id;
    // [ts] });
    // [ts] expect_eq(id, "btn");
    // [ts] });
    // [ts] test("should find button by name and role", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="div">
    // [ts] <button
    // [ts] id="btn"
    // [ts] role="button"
    // [ts] >Submit</button
    // [ts] >
    // [ts] </div>`,
    // [ts] );
    // [ts] using button = (page.$(
    // [ts] "aria/Submit[role=\"button\"]",
    // [ts] ));
    // [ts] const id = button.evaluate(button => {
    // [ts] return button.id;
    // [ts] });
    // [ts] expect_eq(id, "btn");
    // [ts] });
    // [ts] test("should find first matching element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu1"
    // [ts] aria-label="menu div"
    // [ts] ></div>
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu2"
    // [ts] aria-label="menu div"
    // [ts] ></div>
    // [ts] `);
    // [ts] using div = (page.$(
    // [ts] "aria/menu div",
    // [ts] ));
    // [ts] const id = div.evaluate(div => {
    // [ts] return div.id;
    // [ts] });
    // [ts] expect_eq(id, "mnu1");
    // [ts] });
    // [ts] test("should find by name", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu1"
    // [ts] aria-label="menu-label1"
    // [ts] >menu div</div
    // [ts] >
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu2"
    // [ts] aria-label="menu-label2"
    // [ts] >menu div</div
    // [ts] >
    // [ts] `);
    // [ts] using menu = (page.$(
    // [ts] "aria/menu-label1",
    // [ts] ));
    // [ts] const id = menu.evaluate(div => {
    // [ts] return div.id;
    // [ts] });
    // [ts] expect_eq(id, "mnu1");
    // [ts] });
    // [ts] test("should find 2nd element by name", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu1"
    // [ts] aria-label="menu-label1"
    // [ts] >menu div</div
    // [ts] >
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu2"
    // [ts] aria-label="menu-label2"
    // [ts] >menu div</div
    // [ts] >
    // [ts] `);
    // [ts] using menu = (page.$(
    // [ts] "aria/menu-label2",
    // [ts] ));
    // [ts] const id = menu.evaluate(div => {
    // [ts] return div.id;
    // [ts] });
    // [ts] expect_eq(id, "mnu2");
    // [ts] });
    // [ts] test("should find menu by name", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu1"
    // [ts] aria-label="menu div"
    // [ts] ></div>
    // [ts] <div
    // [ts] role="menu"
    // [ts] id="mnu2"
    // [ts] aria-label="menu div"
    // [ts] ></div>
    // [ts] `);
    // [ts] const divs = (page.$$("aria/menu div"));
    // [ts] const ids = joinAll( // migrated: Promise.all -> joinAll
    // [ts] divs.map(n => {
    // [ts] return n.evaluate(div => {
    // [ts] return div.id;
    // [ts] });
    // [ts] }),
    // [ts] );
    // [ts] expect_eq(ids.join(", "), "mnu1, mnu2");
    // [ts] });
    // [ts] test("$$eval should handle many elements", () => {
    // [ts] _.timeout(40_000);
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html``);
    // [ts] page.evaluate(
    // [ts] `
    // [ts] for (var i = 0; i <= 10000; i++) {
    // [ts] const button = document.createElement('button');
    // [ts] button.textContent = i;
    // [ts] document.body.appendChild(button);
    // [ts] }
    // [ts] `,
    // [ts] );
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const sum = await page.$$eval('aria/[role="button"]', buttons => {
    // [ts-skip] return buttons.reduce((acc, button) => {
    // [ts-skip] return acc + Number(button.textContent);
    // [ts-skip] }, 0);
    // [ts-skip] });
    // [ts] expect_eq(sum, 50005000);
    // [ts] });
    // [ts] test("should immediately resolve promise if node exists", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(addElement, "button");
    // [ts] page.waitForSelector("aria/[role=\"button\"]");
    // [ts] });
    // [ts] test("should work for ElementHandle.waitForSelector", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] return (document.body.innerHTML = `<div><button>test</button></div>`);
    // [ts] });
    // [ts] using element = (page.$("div"));
    // [ts] element.waitForSelector("aria/test");
    // [ts] });
    // [ts] test("should persist query handler bindings across reloads", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(addElement, "button");
    // [ts] page.waitForSelector("aria/[role=\"button\"]");
    // [ts] page.reload();
    // [ts] page.evaluate(addElement, "button");
    // [ts] page.waitForSelector("aria/[role=\"button\"]");
    // [ts] });
    // [ts] test("should persist query handler bindings across navigations", () => {
    // [ts] const {page, server} = getTestState();
    // Reset page but make sure that execution context ids start with 1.
    // [ts] page.goto("data:text/html,");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(addElement, "button");
    // [ts] page.waitForSelector("aria/[role=\"button\"]");
    // Reset page but again make sure that execution context ids start with 1.
    // [ts] page.goto("data:text/html,");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(addElement, "button");
    // [ts] page.waitForSelector("aria/[role=\"button\"]");
    // [ts] });
    // [ts] test("should work independently of `exposeFunction`", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.exposeFunction("ariaQuerySelector", (a, b) => {
    // [ts] return a + b;
    // [ts] });
    // [ts] page.evaluate(addElement, "button");
    // [ts] page.waitForSelector("aria/[role=\"button\"]");
    // [ts] const result = page.evaluate("globalThis.ariaQuerySelector(2,8)");
    // [ts] expect_eq(result, 10);
    // [ts] });
    // [ts] test("should work with removed MutationObserver", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // @ts-expect-error This is the point of the test.
    // [ts] return delete window.MutationObserver;
    // [ts] });
    // [ts] const [handle] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForSelector("aria/anything"),
    // [ts] page.setContent(html`<h1>anything</h1>`),
    // [ts] ]);
    // [ts] assert(handle);
    // [ts] expect(
    // [ts] page.evaluate(x => {
    // [ts] return x.textContent;
    // [ts] }, handle),
    // [ts] ).toBe("anything");
    // [ts] });
    // [ts] test("should resolve promise when node is added", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] const watchdog = frame.waitForSelector("aria/[role=\"heading\"]");
    // [ts] frame.evaluate(addElement, "br");
    // [ts] frame.evaluate(addElement, "h1");
    // [ts] using elementHandle = (watchdog);
    // [ts] const tagName = (
    // [ts] elementHandle.getProperty("tagName")
    // [ts] ).jsonValue();
    // [ts] expect_eq(tagName, "H1");
    // [ts] });
    // [ts] test("should work when node is added through innerHTML", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watchdog = page.waitForSelector("aria/name");
    // [ts] page.evaluate(addElement, "span");
    // [ts] page.evaluate(() => {
    // [ts] return (document.querySelector("span").innerHTML =
    // [ts] "<h3><div aria-label=\"name\"></div></h3>");
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("Page.waitForSelector is shortcut for main frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const otherFrame = page.frames()[1];
    // [ts] const watchdog = page.waitForSelector("aria/[role=\"button\"]");
    // [ts] otherFrame.evaluate(addElement, "button");
    // [ts] page.evaluate(addElement, "button");
    // [ts] using elementHandle = watchdog;
    // [ts] expect_eq(elementHandle.frame, page.mainFrame());
    // [ts] });
    // [ts] test("should run in specified frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] const frame2 = page.frames()[2];
    // [ts] const waitForSelectorPromise = frame2.waitForSelector(
    // [ts] "aria/[role=\"button\"]",
    // [ts] );
    // [ts] frame1.evaluate(addElement, "button");
    // [ts] frame2.evaluate(addElement, "button");
    // [ts] using elementHandle = waitForSelectorPromise;
    // [ts] expect_eq(elementHandle.frame, frame2);
    // [ts] });
    // [ts] test("should throw when frame is detached", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] let waitError: Error;
    // [ts] const waitPromise = frame
    // [ts] .waitForSelector("aria/does-not-exist")
    // [ts] .catch(error => {
    // [ts] return (waitError = error);
    // [ts] });
    // [ts] detachFrame(page, "frame1");
    // [ts] waitPromise;
    // [ts] expect_eq(waitError, true);
    // [ts] expect(waitError.message).toBe(
    // [ts] "Waiting for selector `does-not-exist` failed",
    // [ts] );
    // [ts] });
    // [ts] test("should survive cross-process navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let imgFound = false;
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("aria/[role=\"image\"]")
    // [ts] .then(() => {
    // [ts] return (imgFound = true);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(imgFound, false);
    // [ts] page.reload();
    // [ts] expect_eq(imgFound, false);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] waitForSelector;
    // [ts] expect_eq(imgFound, true);
    // [ts] });
    // [ts] test("should wait for visible", () => {
    // [ts] const {page} = getTestState();
    // [ts] let divFound = false;
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("aria/name", {visible: true})
    // [ts] .then(() => {
    // [ts] return (divFound = true);
    // [ts] });
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] aria-label="name"
    // [ts] style="display: none; visibility: hidden;"
    // [ts] >
    // [ts] 1
    // [ts] </div>`,
    // [ts] );
    // [ts] expect_eq(divFound, false);
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("div").style.removeProperty("display");
    // [ts] });
    // [ts] expect_eq(divFound, false);
    // [ts] page.evaluate(() => {
    // [ts] return document
    // [ts] .querySelector("div")
    // [ts] .style.removeProperty("visibility");
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divFound, true);
    // [ts] });
    // [ts] test("should wait for visible recursively", () => {
    // [ts] const {page} = getTestState();
    // [ts] let divVisible = false;
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("aria/inner", {visible: true})
    // [ts] .then(() => {
    // [ts] return (divVisible = true);
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return (divVisible = false);
    // [ts] });
    // [ts] page.setContent(
    // [ts] html`<div style="display: none; visibility: hidden;">
    // [ts] <div aria-label="inner">hi</div>
    // [ts] </div>`,
    // [ts] );
    // [ts] expect_eq(divVisible, false);
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("div").style.removeProperty("display");
    // [ts] });
    // [ts] expect_eq(divVisible, false);
    // [ts] page.evaluate(() => {
    // [ts] return document
    // [ts] .querySelector("div")
    // [ts] .style.removeProperty("visibility");
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divVisible, true);
    // [ts] });
    // [ts] test("hidden should wait for visibility: hidden", () => {
    // [ts] const {page} = getTestState();
    // [ts] let divHidden = false;
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="button"
    // [ts] style="display: block;"
    // [ts] >text</div
    // [ts] >`,
    // [ts] );
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("aria/[role=\"button\"]", {hidden: true})
    // [ts] .then(() => {
    // [ts] return (divHidden = true);
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return (divHidden = false);
    // [ts] });
    // [ts] page.waitForSelector("aria/[role=\"button\"]"); // do a round trip
    // [ts] expect_eq(divHidden, false);
    // [ts] page.evaluate(() => {
    // [ts] return document
    // [ts] .querySelector("div")
    // [ts] .style.setProperty("visibility", "hidden");
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divHidden, true);
    // [ts] });
    // [ts] test("hidden should wait for display: none", () => {
    // [ts] const {page} = getTestState();
    // [ts] let divHidden = false;
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="main"
    // [ts] style="display: block;"
    // [ts] >text</div
    // [ts] >`,
    // [ts] );
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("aria/[role=\"main\"]", {hidden: true})
    // [ts] .then(() => {
    // [ts] return (divHidden = true);
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return (divHidden = false);
    // [ts] });
    // [ts] page.waitForSelector("aria/[role=\"main\"]"); // do a round trip
    // [ts] expect_eq(divHidden, false);
    // [ts] page.evaluate(() => {
    // [ts] return document
    // [ts] .querySelector("div")
    // [ts] .style.setProperty("display", "none");
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divHidden, true);
    // [ts] });
    // [ts] test("hidden should wait for removal", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div role="main">text</div>`);
    // [ts] let divRemoved = false;
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("aria/[role=\"main\"]", {hidden: true})
    // [ts] .then(() => {
    // [ts] return (divRemoved = true);
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return (divRemoved = false);
    // [ts] });
    // [ts] page.waitForSelector("aria/[role=\"main\"]"); // do a round trip
    // [ts] expect_eq(divRemoved, false);
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("div").remove();
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divRemoved, true);
    // [ts] });
    // [ts] test("should return null if waiting to hide non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.waitForSelector("aria/non-existing", {
    // [ts] hidden: true,
    // [ts] });
    // [ts] expect_eq(handle, null);
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] const error = page
    // [ts] .waitForSelector("aria/[role=\"button\"]", {
    // [ts] timeout: 10,
    // [ts] })
    // [ts] .catch(error => {
    // [ts] return error;
    // [ts] });
    // [ts] expect(error.message).toContain(
    // [ts] "Waiting for selector `[role=\"button\"]` failed",
    // [ts] );
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should have an error message specifically for awaiting an element to be hidden", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div role="main">text</div>`);
    // [ts] const promise = page.waitForSelector("aria/[role=\"main\"]", {
    // [ts] hidden: true,
    // [ts] timeout: 10,
    // [ts] });
    // [ts] expect(promise).rejects.toMatchObject({
    // [ts] message: "Waiting for selector `[role=\"main\"]` failed",
    // [ts] });
    // [ts] });
    // [ts] test("should respond to node attribute mutation", () => {
    // [ts] const {page} = getTestState();
    // [ts] let divFound = false;
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("aria/zombo")
    // [ts] .then(() => {
    // [ts] return (divFound = true);
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return (divFound = false);
    // [ts] });
    // [ts] page.setContent(html`<div aria-label="notZombo"></div>`);
    // [ts] expect_eq(divFound, false);
    // [ts] page.evaluate(() => {
    // [ts] return document
    // [ts] .querySelector("div")
    // [ts] .setAttribute("aria-label", "zombo");
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] });
    // [ts] test("should return the element handle", () => {
    // [ts] const {page} = getTestState();
    // [ts] const waitForSelector = page.waitForSelector("aria/zombo").catch(err => {
    // [ts] return err;
    // [ts] });
    // [ts] page.setContent(html`<div aria-label="zombo">anything</div>`);
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] x => {
    // [ts] return x?.textContent;
    // [ts] },
    // [ts] waitForSelector,
    // [ts] ),
    // [ts] ).toBe("anything");
    // [ts] });
    // [ts] test("should have correct stack trace for timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.waitForSelector("aria/zombo", {timeout: 10}).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, "Waiting for selector `zombo` failed") != null, true);
    // [ts] });
    // [ts] test("should find by name \"foo\"", () => {
    // [ts] const {page} = setupPage();
    // [ts] const found = page.$$("aria/foo");
    // [ts] const ids = getIds(found);
    // [ts] expect_eq(ids, ["node3", "node5", "node6"]);
    // [ts] });
    // [ts] test("should find by name \"bar\"", () => {
    // [ts] const {page} = setupPage();
    // [ts] const found = page.$$("aria/bar");
    // [ts] const ids = getIds(found);
    // [ts] expect_eq(ids, ["node1", "node2", "node8"]);
    // [ts] });
    // [ts] test("should find treeitem by name", () => {
    // [ts] const {page} = setupPage();
    // [ts] const found = page.$$("aria/item1 item2 item3");
    // [ts] const ids = getIds(found);
    // [ts] expect_eq(ids, ["node30"]);
    // [ts] });
    // [ts] test("should find by role \"button\"", () => {
    // [ts] const {page} = setupPage();
    // [ts] const found = (page.$$("aria/[role=\"button\"]"));
    // [ts] const ids = getIds(found);
    // [ts] expect_eq(ids, ["node5", "node6", "node8", "node10", "node21"]);
    // [ts] });
    // [ts] test("should find by role \"heading\"", () => {
    // [ts] const {page} = setupPage();
    // [ts] const found = page.$$("aria/[role=\"heading\"]");
    // [ts] const ids = getIds(found);
    // [ts] expect_eq(ids, ["shown", "node11", "node13"]);
    // [ts] });
    // [ts] test("should find both ignored and unignored", () => {
    // [ts] const {page} = setupPage();
    // [ts] const found = page.$$("aria/title");
    // [ts] const ids = getIds(found);
    // [ts] expect_eq(ids, ["shown"]);
    // [ts] });
}
