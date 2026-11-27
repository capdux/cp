// Migrated from TypeScript (src/queryhandler.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";

import { Puppeteer } from "puppeteer";
// [cap-migrate] import {ElementHandle} from "puppeteer-core/internal/api/ElementHandle.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html } from "./utils.cp";

// [cap-test] describe("Query handler tests", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Pierce selectors", () => {
    async fn setUpPage(): string {
      const state = getTestState();
      state.page.setContent(
        `<script>
         const div = document.createElement('div');
         const shadowRoot = div.attachShadow({mode: 'open'});
         const div1 = document.createElement('div');
         div1.textContent = 'Hello';
         div1.className = 'foo';
         const div2 = document.createElement('div');
         div2.textContent = 'World';
         div2.className = 'foo';
         shadowRoot.appendChild(div1);
         shadowRoot.appendChild(div2);
         document.documentElement.appendChild(div);
         </script>`,
      );
      return state;
    }
  // [cap-test] });

  // [cap-test] describe("Text selectors", () => {
    // [cap-test] describe("in Page", () => {
    // [cap-test] });
    // [cap-test] describe("in ElementHandles", () => {

    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("XPath selectors", () => {
    // [cap-test] describe("in Page", () => {
    // [cap-test] });
    // [cap-test] describe("in ElementHandles", () => {

    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("P selectors", () => {
    beforeEach(() => {
      Puppeteer.clearCustomQueryHandlers();
    });















    const permute = (inputs) => {
      const results: [][]T = [];
      let i = 0;
      while (i < inputs.length) { // [cap-migrate] for-step: ++i
        const permutation = permute(
          inputs.slice(0, i).concat(inputs.slice(i + 1)),
        );
        const value = inputs[i];
        if (permutation.length == 0) {
          results.push([value]);
          continue;
        }
        for (const part of permutation) {
          results.push([value].concat(part));
        }
      }
      return results;
    };



  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should find first element in shadow", () => {
    // [ts] const {page} = setUpPage();
    // [ts] using div = (page.$("pierce/.foo"));
    // [ts] const text = div.evaluate(element => {
    // [ts] return element.textContent;
    // [ts] });
    // [ts] expect_eq(text, "Hello");
    // [ts] });
    // [ts] test("should find all elements in shadow", () => {
    // [ts] const {page} = setUpPage();
    // [ts] const divs = (page.$$("pierce/.foo"));
    // [ts] const text = joinAll( // migrated: Promise.all -> joinAll
    // [ts] divs.map(div => {
    // [ts] return div.evaluate(element => {
    // [ts] return element.textContent;
    // [ts] });
    // [ts] }),
    // [ts] );
    // [ts] expect_eq(text.join(" "), "Hello World");
    // [ts] });
    // [ts] test("should find first child element", () => {
    // [ts] const {page} = setUpPage();
    // [ts] using parentElement = (page.$("html > div"));
    // [ts] using childElement = (parentElement.$(
    // [ts] "pierce/div",
    // [ts] ));
    // [ts] const text = childElement.evaluate(element => {
    // [ts] return element.textContent;
    // [ts] });
    // [ts] expect_eq(text, "Hello");
    // [ts] });
    // [ts] test("should find all child elements", () => {
    // [ts] const {page} = setUpPage();
    // [ts] using parentElement = (page.$("html > div"));
    // [ts] const childElements = (parentElement.$$("pierce/div"));
    // [ts] const text = joinAll( // migrated: Promise.all -> joinAll
    // [ts] childElements.map(div => {
    // [ts] return div.evaluate(element => {
    // [ts] return element.textContent;
    // [ts] });
    // [ts] }),
    // [ts] );
    // [ts] expect_eq(text.join(" "), "Hello World");
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>test</section>`);
    // [ts] expect_eq(page.$("text/test"), true);
    // [ts] expect(page.$$("text/test")).toHaveLength(1);
    // [ts] });
    // [ts] test("should return empty array for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect_eq(page.$("text/test"), false);
    // [ts] expect(page.$$("text/test")).toHaveLength(0);
    // [ts] });
    // [ts] test("should return first element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div id="1">a</div> <div>a</div>`);
    // [ts] using element = page.$("text/a");
    // [ts] expect(
    // [ts] element?.evaluate(e => {
    // [ts] return e.id;
    // [ts] }),
    // [ts] ).toBe("1");
    // [ts] });
    // [ts] test("should return multiple elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>a</div> <div>a</div>`);
    // [ts] const elements = page.$$("text/a");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] });
    // [ts] test("should pierce shadow DOM", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] const shadow = div.attachShadow({mode: "open"});
    // [ts] const diva = document.createElement("div");
    // [ts] shadow.append(diva);
    // [ts] const divb = document.createElement("div");
    // [ts] shadow.append(divb);
    // [ts] diva.innerHTML = "a";
    // [ts] divb.innerHTML = "b";
    // [ts] document.body.append(div);
    // [ts] });
    // [ts] using element = page.$("text/a");
    // [ts] expect(
    // [ts] element?.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }),
    // [ts] ).toBe("a");
    // [ts] });
    // [ts] test("should query deeply nested text", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div>
    // [ts] <div>a</div>
    // [ts] <div>b</div>
    // [ts] </div>`,
    // [ts] );
    // [ts] using element = page.$("text/a");
    // [ts] expect(
    // [ts] element?.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }),
    // [ts] ).toBe("a");
    // [ts] });
    // [ts] test("should query inputs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input value="a" />`);
    // [ts] using element = (page.$(
    // [ts] "text/a",
    // [ts] ));
    // [ts] expect(
    // [ts] element?.evaluate(e => {
    // [ts] return e.value;
    // [ts] }),
    // [ts] ).toBe("a");
    // [ts] });
    // [ts] test("should not query radio", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<radio value="a"></radio>`);
    // [ts] expect_eq(page.$("text/a") == null, true);
    // [ts] });
    // [ts] test("should query text spanning multiple elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div><span>a</span> <span>b</span></div>`);
    // [ts] using element = page.$("text/a b");
    // [ts] expect(
    // [ts] element?.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }),
    // [ts] ).toBe("a b");
    // [ts] });
    // [ts] test("should clear caches", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="target1">text</div>
    // [ts] <input
    // [ts] id="target2"
    // [ts] value="text"
    // [ts] />
    // [ts] <div id="target3">text</div>`,
    // [ts] );
    // [ts] using div = (page.$("#target1"));
    // [ts] using input = (page.$(
    // [ts] "#target2",
    // [ts] ));
    // [ts] div.evaluate(div => {
    // [ts] div.textContent = "text";
    // [ts] });
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval(`text/text`, e => {
    // [ts-skip] return e.id;
    // [ts-skip] }),
    // [ts] ).toBe("target1");
    // [ts] div.evaluate(div => {
    // [ts] div.textContent = "foo";
    // [ts] });
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval(`text/text`, e => {
    // [ts-skip] return e.id;
    // [ts-skip] }),
    // [ts] ).toBe("target2");
    // [ts] input.evaluate(input => {
    // [ts] input.value = "";
    // [ts] });
    // [ts] input.type("foo");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval(`text/text`, e => {
    // [ts-skip] return e.id;
    // [ts-skip] }),
    // [ts] ).toBe("target3");
    // [ts] div.evaluate(div => {
    // [ts] div.textContent = "text";
    // [ts] });
    // [ts] input.evaluate(input => {
    // [ts] input.value = "";
    // [ts] });
    // [ts] input.type("text");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$$eval(`text/text`, es => {
    // [ts-skip] return es.length;
    // [ts-skip] }),
    // [ts] ).toBe(3);
    // [ts] div.evaluate(div => {
    // [ts] div.textContent = "foo";
    // [ts] });
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$$eval(`text/text`, es => {
    // [ts-skip] return es.length;
    // [ts-skip] }),
    // [ts] ).toBe(2);
    // [ts] input.evaluate(input => {
    // [ts] input.value = "";
    // [ts] });
    // [ts] input.type("foo");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$$eval(`text/text`, es => {
    // [ts-skip] return es.length;
    // [ts-skip] }),
    // [ts] ).toBe(1);
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div class="a"><span>a</span></div>`);
    // [ts] using elementHandle = (page.$("div"));
    // [ts] expect_eq(elementHandle.$(`text/a`), true);
    // [ts] expect(elementHandle.$$(`text/a`)).toHaveLength(1);
    // [ts] });
    // [ts] test("should return null for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div class="a"></div>`);
    // [ts] using elementHandle = (page.$("div"));
    // [ts] expect_eq(elementHandle.$(`text/a`), false);
    // [ts] expect(elementHandle.$$(`text/a`)).toHaveLength(0);
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>test</section>`);
    // [ts] expect_eq(page.$("xpath/html/body/section"), true);
    // [ts] expect(page.$$("xpath/html/body/section")).toHaveLength(1);
    // [ts] });
    // [ts] test("should return empty array for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.$("xpath/html/body/non-existing-element"),
    // [ts] ).toBeFalsy();
    // [ts] expect(
    // [ts] page.$$("xpath/html/body/non-existing-element"),
    // [ts] ).toHaveLength(0);
    // [ts] });
    // [ts] test("should return first element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>a</div> <div></div>`);
    // [ts] using element = page.$("xpath/html/body/div");
    // [ts] expect(
    // [ts] element?.evaluate(e => {
    // [ts] return e.textContent == "a";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should return multiple elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div></div> <div></div>`);
    // [ts] const elements = page.$$("xpath/html/body/div");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div class="a">a<span></span></div>`);
    // [ts] using elementHandle = (page.$("div"));
    // [ts] expect_eq(elementHandle.$(`xpath/span`), true);
    // [ts] expect(elementHandle.$$(`xpath/span`)).toHaveLength(1);
    // [ts] });
    // [ts] test("should return null for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div class="a">a</div>`);
    // [ts] using elementHandle = (page.$("div"));
    // [ts] expect_eq(elementHandle.$(`xpath/span`), false);
    // [ts] expect(elementHandle.$$(`xpath/span`)).toHaveLength(0);
    // [ts] });
    // [ts] test("should work with CSS selectors", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("div > button");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] using root = page.$("div");
    // [ts] using button = root.$("& > button");
    // [ts] assert(button, "Could not find element");
    // [ts] expect(
    // [ts] button.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // Should parse more complex CSS selectors. Listing a few problematic
    // cases from bug reports.
    // [ts] for (const selector of [
    // [ts] ".user_row[data-user-id=\"\\38 \"]:not(.deactivated_user)",
    // [ts] `input[value='Search']:not([class='hidden'])`,
    // [ts] `[data-test-id^="test-"]:not([data-test-id^="test-foo"])`,
    // [ts] `& > table`,
    // [ts] ]) {
    // [ts] page.$$(selector);
    // [ts] }
    // [ts] });
    // [ts] test("should work with puppeteer pseudo classes", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("button::-p-text(world)");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should work with deep combinators", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] {
    // [ts] using element = page.$("div >>>> div");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "c";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] {
    // [ts] const elements = page.$$("div >>> div");
    // [ts] assert(elements[1], "Could not find element");
    // [ts] expect(
    // [ts] elements[1]?.evaluate(element => {
    // [ts] return element.id == "d";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] {
    // [ts] const elements = page.$$("#c >>>> div");
    // [ts] assert(elements[0], "Could not find element");
    // [ts] expect(
    // [ts] elements[0]?.evaluate(element => {
    // [ts] return element.id == "d";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] {
    // [ts] const elements = page.$$("#c >>> div");
    // [ts] assert(elements[0], "Could not find element");
    // [ts] expect(
    // [ts] elements[0]?.evaluate(element => {
    // [ts] return element.id == "d";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] });
    // [ts] test("should work with text selectors", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("div ::-p-text(world)");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should query light DOM and slotted text of shadow hosts", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <main><h1>Cart</h1><my-button>Checkout</my-button></main>
    // [ts] <div id="wrap"
    // [ts] ><my-list><li>One</li><li>Two</li></my-list></div
    // [ts] >
    // [ts] `);
    // [ts] page.evaluate(() => {
    // [ts] for (const h of document.querySelectorAll("my-button")) {
    // [ts] h.attachShadow({mode: "open"}).innerHTML =
    // [ts] "<button><slot></slot></button>";
    // [ts] }
    // [ts] for (const h of document.querySelectorAll("my-list")) {
    // [ts] h.attachShadow({mode: "open"}).innerHTML = "<ul><slot></slot></ul>";
    // [ts] }
    // [ts] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const checkout = await page.$$eval('::-p-text(Checkout)', els => {
    // [ts-skip] return els.map(e => {
    // [ts-skip] return e.tagName.toLowerCase();
    // [ts-skip] });
    // [ts-skip] });
    // [ts] expect_eq(checkout, ["my-button"]);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const two = await page.$$eval('::-p-text(Two)', els => {
    // [ts-skip] return els.map(e => {
    // [ts-skip] return e.tagName.toLowerCase();
    // [ts-skip] });
    // [ts-skip] });
    // [ts] expect_eq(two, ["li"]);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const one = await page.$$eval('#wrap ::-p-text(One)', els => {
    // [ts-skip] return els.map(e => {
    // [ts-skip] return e.tagName.toLowerCase();
    // [ts-skip] });
    // [ts-skip] });
    // [ts] expect_eq(one, ["li"]);
    // [ts] });
    // [ts] test("should work ARIA selectors", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("div ::-p-aria(world)");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should work for ARIA selectors in multiple isolated worlds", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.waitForSelector("::-p-aria(world)");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // $ would add ARIA query handler to the main world.
    // [ts] element.$("::-p-aria(world)");
    // [ts] using element2 = page.waitForSelector("::-p-aria(world)");
    // [ts] assert(element2, "Could not find element");
    // [ts] expect(
    // [ts] element2.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should work ARIA selectors with role", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("::-p-aria(world[role=\"button\"])");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should work ARIA selectors with name and role", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("::-p-aria([name=\"world\"][role=\"button\"])");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should work XPath selectors", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("div ::-p-xpath(//button)");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should work with custom selectors", () => {
    // [ts] Puppeteer.registerCustomQueryHandler("div", {
    // [ts] queryOne: () => {
    // [ts] return document.querySelector("div");
    // [ts] },
    // [ts] });
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$("::-p-div");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "a";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should work with custom selectors with args", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] Puppeteer.registerCustomQueryHandler("div", {
    // [ts] queryOne: (_, selector) => {
    // [ts] if (selector == "true") {
    // [ts] return document.querySelector("div");
    // [ts] } else {
    // [ts] return document.querySelector("button");
    // [ts] }
    // [ts] },
    // [ts] });
    // [ts] {
    // [ts] using element = page.$("::-p-div(true)");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "a";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] {
    // [ts] using element = page.$("::-p-div(\"true\")");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "a";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] {
    // [ts] using element = page.$("::-p-div('true')");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "a";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] {
    // [ts] using element = page.$("::-p-div");
    // [ts] assert(element, "Could not find element");
    // [ts] expect(
    // [ts] element.evaluate(element => {
    // [ts] return element.id == "b";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] }
    // [ts] });
    // [ts] test("should work with :hover", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using button = page.$("div ::-p-text(world)");
    // [ts] assert(button, "Could not find element");
    // [ts] button.hover();
    // [ts] using button2 = page.$("div ::-p-text(world):hover");
    // [ts] assert(button2, "Could not find element");
    // [ts] const value = button2.evaluate(span => {
    // [ts] return {textContent: span.textContent, tagName: span.tagName};
    // [ts] });
    // [ts] expect(value).toMatchObject({textContent: "world", tagName: "BUTTON"});
    // [ts] });
    // [ts] test("should work with selector lists", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] const elements = page.$$("div, ::-p-text(world)");
    // [ts] expect(elements).toHaveLength(3);
    // [ts] });
    // [ts] test("should match querySelector* ordering", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] for (const list of permute(["div", "button", "span"])) {
    // [ts] const elements = page.$$(
    // [ts] list
    // [ts] .map(selector => {
    // [ts] return if (selector == "button") "::-p-text(world)" else selector;
    // [ts] })
    // [ts] .join(","),
    // [ts] );
    // [ts] const actual = joinAll( // migrated: Promise.all -> joinAll
    // [ts] elements.map(element => {
    // [ts] return element.evaluate(element => {
    // [ts] return element.id;
    // [ts] });
    // [ts] }),
    // [ts] );
    // [ts] expect_eq(actual.join(), "a,b,f,c");
    // [ts] }
    // [ts] });
    // [ts] test("should not have duplicate elements from selector lists", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] const elements = page.$$("::-p-text(world), button");
    // [ts] expect(elements).toHaveLength(1);
    // [ts] });
    // [ts] test("should handle escapes", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(`${server.PREFIX}/p-selectors.html`);
    // [ts] using element = page.$(
    // [ts] ":scope >>> ::-p-text(My name is Jun \\(pronounced like \"June\"\\))",
    // [ts] );
    // [ts] expect_eq(element, true);
    // [ts] using element2 = page.$(
    // [ts] ":scope >>> ::-p-text(\"My name is Jun (pronounced like \\\"June\\\")\")",
    // [ts] );
    // [ts] expect_eq(element2, true);
    // [ts] using element3 = page.$(
    // [ts] ":scope >>> ::-p-text(My name is Jun \\(pronounced like \"June\"\\)\")",
    // [ts] );
    // [ts] expect_eq(element3, false);
    // [ts] using element4 = page.$(
    // [ts] ":scope >>> ::-p-text(\"My name is Jun \\(pronounced like \"June\"\\))",
    // [ts] );
    // [ts] expect_eq(element4, false);
    // [ts] });
}
