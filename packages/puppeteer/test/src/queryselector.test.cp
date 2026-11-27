// Migrated from TypeScript (src/queryselector.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
import { Puppeteer } from "puppeteer";
// [cap-migrate] import {CustomQueryHandler} from "puppeteer-core/internal/common/CustomQueryHandler.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html } from "./utils.cp";

// [cap-test] describe("querySelector", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.$eval", () => {
  // [cap-test] });

  // The tests for $$eval are repeated later in this file in the test group 'QueryAll'.
  // This is done to also test a query handler where QueryAll returns an Element[]
  // as opposed to NodeListOf<Element>.
  // [cap-test] describe("Page.$$eval", () => {
  // [cap-test] });

  // [cap-test] describe("Page.$", () => {
  // [cap-test] });

  // [cap-test] describe("Page.$$", () => {



    // [cap-test] describe("xpath", () => {
    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("ElementHandle.$", () => {

  // [cap-test] });
  // [cap-test] describe("ElementHandle.$eval", () => {


  // [cap-test] });
  // [cap-test] describe("ElementHandle.$$eval", () => {


  // [cap-test] });

  // [cap-test] describe("ElementHandle.$$", () => {


    // [cap-test] describe("xpath", () => {

    // [cap-test] });
  // [cap-test] });
  // This is the same tests for `$$eval` and `$$` as above, but with a queryAll
  // handler that returns an array instead of a list of nodes.
  // [cap-test] describe("QueryAll", () => {
    // TODO(cap-migrate): refine TS statement
    // [ts] const handler: CustomQueryHandler = {
    // [ts] queryAll: (element, selector) => {
    // [ts] return [...(element).querySelectorAll(selector)];
    // [ts] },
    // [ts] };
    before(() => {
      Puppeteer.registerCustomQueryHandler("allArray", handler);
    });


  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section id="testAttribute">43543</section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('section', e => {
    // [ts-skip] return e.id;
    // [ts-skip] });
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("should accept arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>hello</section>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const text = await page.$eval(
    // [ts] "section",
    // [ts] (e, suffix) => {
    // [ts] return e.textContent + suffix;
    // [ts] },
    // [ts] " world!",
    // [ts] );
    // [ts] expect_eq(text, "hello world!");
    // [ts] });
    // [ts] test("should accept ElementHandles as arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>hello</section><div> world</div>`);
    // [ts] using divHandle = (page.$("div"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const text = await page.$eval(
    // [ts] "section",
    // [ts] (e, div) => {
    // [ts] return e.textContent + div.textContent;
    // [ts] },
    // [ts] divHandle,
    // [ts] );
    // [ts] expect_eq(text, "hello world");
    // [ts] });
    // [ts] test("should throw error if no element is found", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: .$eval('section', e => {
    // [ts-skip] return e.id;
    // [ts-skip] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toContain(
    // [ts] "failed to find element matching selector \"section\"",
    // [ts] );
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div>hello</div>
    // [ts] <div>beautiful</div>
    // [ts] <div>world!</div>`,
    // [ts] );
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCount = await page.$$eval('div', divs => {
    // [ts-skip] return divs.length;
    // [ts-skip] });
    // [ts] expect_eq(divsCount, 3);
    // [ts] });
    // [ts] test("should accept extra arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div>hello</div>
    // [ts] <div>beautiful</div>
    // [ts] <div>world!</div>`,
    // [ts] );
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCountPlus5 = await page.$$eval(
    // [ts] "div",
    // [ts] (divs, two, three) => {
    // [ts] return divs.length + (two) + (three);
    // [ts] },
    // [ts] 2,
    // [ts] 3,
    // [ts] );
    // [ts] expect_eq(divsCountPlus5, 8);
    // [ts] });
    // [ts] test("should accept ElementHandles as arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<section>2</section>
    // [ts] <section>2</section>
    // [ts] <section>1</section>
    // [ts] <div>3</div>`,
    // [ts] );
    // [ts] using divHandle = (page.$("div"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const sum = await page.$$eval(
    // [ts] "section",
    // [ts] (sections, div) => {
    // [ts] return (
    // [ts] sections.reduce((acc, section) => {
    // [ts] return acc + Number(section.textContent);
    // [ts] }, 0) + Number(div.textContent)
    // [ts] );
    // [ts] },
    // [ts] divHandle,
    // [ts] );
    // [ts] expect_eq(sum, 8);
    // [ts] });
    // [ts] test("should handle many elements", () => {
    // [ts] _.timeout(25_000);
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(
    // [ts] `
    // [ts] for (var i = 0; i <= 1000; i++) {
    // [ts] const section = document.createElement('section');
    // [ts] section.textContent = i;
    // [ts] document.body.appendChild(section);
    // [ts] }
    // [ts] `,
    // [ts] );
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const sum = await page.$$eval('section', sections => {
    // [ts-skip] return sections.reduce((acc, section) => {
    // [ts-skip] return acc + Number(section.textContent);
    // [ts-skip] }, 0);
    // [ts-skip] });
    // [ts] expect_eq(sum, 500500);
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>test</section>`);
    // [ts] using element = (page.$("section"));
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("should return null for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] using element = (page.$("non-existing-element"));
    // [ts] expect_eq(element, null);
    // [ts] });
    // [ts] test("should query existing elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div>A</div>
    // [ts] <br />
    // [ts] <div>B</div>`,
    // [ts] );
    // [ts] const elements = page.$$("div");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] const promises = elements.map(element => {
    // [ts] return page.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }, element);
    // [ts] });
    // [ts] expect_eq(joinAll(promises), ["A", "B"]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("should query existing elements without isolation", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div>A</div>
    // [ts] <br />
    // [ts] <div>B</div>`,
    // [ts] );
    // [ts] const elements = page.$$("div", {
    // [ts] isolate: false,
    // [ts] });
    // [ts] expect(elements).toHaveLength(2);
    // [ts] const promises = elements.map(element => {
    // [ts] return page.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }, element);
    // [ts] });
    // [ts] expect_eq(joinAll(promises), ["A", "B"]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("should return empty array if nothing is found", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const elements = page.$$("div");
    // [ts] expect(elements).toHaveLength(0);
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>test</section>`);
    // [ts] const elements = page.$$("xpath/html/body/section");
    // [ts] expect_eq(elements[0], true);
    // [ts] expect(elements).toHaveLength(1);
    // [ts] });
    // [ts] test("should return empty array for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] const element = page.$$("xpath/html/body/non-existing-element");
    // [ts] expect_eq(element, []);
    // [ts] });
    // [ts] test("should return multiple elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div></div> <div></div>`);
    // [ts] const elements = page.$$("xpath/html/body/div");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/playground.html");
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div class="second"><div class="inner">A</div></div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] using second = (htmlEl.$(".second"));
    // [ts] using inner = second.$(".inner");
    // [ts] const content = page.evaluate(e => {
    // [ts] return e?.textContent;
    // [ts] }, inner);
    // [ts] expect_eq(content, "A");
    // [ts] });
    // [ts] test("should return null for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div class="second"><div class="inner">B</div></div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] using second = htmlEl.$(".third");
    // [ts] expect_eq(second, null);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div class="tweet">
    // [ts] <div class="like">100</div>
    // [ts] <div class="retweets">10</div>
    // [ts] </div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using tweet = (page.$(".tweet"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await tweet.$eval('.like', node => {
    // [ts-skip] return (node as HTMLElement).innerText;
    // [ts-skip] });
    // [ts] expect_eq(content, "100");
    // [ts] });
    // [ts] test("should retrieve content from subtree", () => {
    // [ts] const {page} = getTestState();
    // [ts] const htmlContent =
    // [ts] "<div class=\"a\">not-a-child-div</div><div id=\"myId\"><div class=\"a\">a-child-div</div></div>";
    // [ts] page.setContent(html`${htmlContent}`);
    // [ts] using elementHandle = (page.$("#myId"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await elementHandle.$eval('.a', node => {
    // [ts-skip] return (node as HTMLElement).innerText;
    // [ts-skip] });
    // [ts] expect_eq(content, "a-child-div");
    // [ts] });
    // [ts] test("should throw in case of missing selector", () => {
    // [ts] const {page} = getTestState();
    // [ts] const htmlContent =
    // [ts] "<div class=\"a\">not-a-child-div</div><div id=\"myId\"></div>";
    // [ts] page.setContent(html`${htmlContent}`);
    // [ts] using elementHandle = (page.$("#myId"));
    // [ts] const errorMessage = elementHandle
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: .$eval('.a', node => {
    // [ts-skip] return (node as HTMLElement).innerText;
    // [ts-skip] })
    // [ts] .catch(error => {
    // [ts] return error.message;
    // [ts] });
    // [ts] expect(errorMessage).toBe(
    // [ts] `Error: failed to find element matching selector ".a"`,
    // [ts] );
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div class="tweet">
    // [ts] <div class="like">100</div>
    // [ts] <div class="like">10</div>
    // [ts] </div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using tweet = (page.$(".tweet"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await tweet.$$eval('.like', nodes => {
    // [ts-skip] return (nodes as HTMLElement[]).map(n => {
    // [ts-skip] return n.innerText;
    // [ts-skip] });
    // [ts-skip] });
    // [ts] expect_eq(content, ["100", "10"]);
    // [ts] });
    // [ts] test("should retrieve content from subtree", () => {
    // [ts] const {page} = getTestState();
    // [ts] const htmlContent =
    // [ts] "<div class=\"a\">not-a-child-div</div><div id=\"myId\"><div class=\"a\">a1-child-div</div><div class=\"a\">a2-child-div</div></div>";
    // [ts] page.setContent(html`${htmlContent}`);
    // [ts] using elementHandle = (page.$("#myId"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const content = await elementHandle.$$eval('.a', nodes => {
    // [ts-skip] return (nodes as HTMLElement[]).map(n => {
    // [ts-skip] return n.innerText;
    // [ts-skip] });
    // [ts-skip] });
    // [ts] expect_eq(content, ["a1-child-div", "a2-child-div"]);
    // [ts] });
    // [ts] test("should not throw in case of missing selector", () => {
    // [ts] const {page} = getTestState();
    // [ts] const htmlContent =
    // [ts] "<div class=\"a\">not-a-child-div</div><div id=\"myId\"></div>";
    // [ts] page.setContent(html`${htmlContent}`);
    // [ts] using elementHandle = (page.$("#myId"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const nodesLength = await elementHandle.$$eval('.a', nodes => {
    // [ts-skip] return nodes.length;
    // [ts-skip] });
    // [ts] expect_eq(nodesLength, 0);
    // [ts] });
    // [ts] test("should query existing elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div>A</div>
    // [ts] <br />
    // [ts] <div>B</div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] const elements = htmlEl.$$("div");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] const promises = elements.map(element => {
    // [ts] return page.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }, element);
    // [ts] });
    // [ts] expect_eq(joinAll(promises), ["A", "B"]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("should return empty array for non-existing elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <span>A</span><br /><span>B</span>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] const elements = htmlEl.$$("div");
    // [ts] expect(elements).toHaveLength(0);
    // [ts] });
    // [ts] test("should query existing element", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/playground.html");
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div class="second"><div class="inner">A</div></div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] const second = htmlEl.$$(
    // [ts] `xpath/./body/div[contains(@class, 'second')]`,
    // [ts] );
    // [ts] const inner = second[0].$$(
    // [ts] `xpath/./div[contains(@class, 'inner')]`,
    // [ts] );
    // [ts] const content = page.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }, inner[0]);
    // [ts] expect_eq(content, "A");
    // [ts] });
    // [ts] test("should return null for non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div class="second"><div class="inner">B</div></div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] const second = htmlEl.$$(`xpath/div[contains(@class, 'third')]`);
    // [ts] expect_eq(second, []);
    // [ts] });
    // [ts] test("should have registered handler", () => {
    // [ts] expect(
    // [ts] Puppeteer.customQueryHandlerNames().includes("allArray"),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("$$ should query existing elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <div>A</div>
    // [ts] <br />
    // [ts] <div>B</div>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] const elements = htmlEl.$$("allArray/div");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] const promises = elements.map(element => {
    // [ts] return page.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }, element);
    // [ts] });
    // [ts] expect_eq(joinAll(promises), ["A", "B"]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("$$ should return empty array for non-existing elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<html>
    // [ts] <body>
    // [ts] <span>A</span><br /><span>B</span>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] using htmlEl = (page.$("html"));
    // [ts] const elements = htmlEl.$$("allArray/div");
    // [ts] expect(elements).toHaveLength(0);
    // [ts] });
    // [ts] test("$$eval should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div>hello</div>
    // [ts] <div>beautiful</div>
    // [ts] <div>world!</div>`,
    // [ts] );
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCount = await page.$$eval('allArray/div', divs => {
    // [ts-skip] return divs.length;
    // [ts-skip] });
    // [ts] expect_eq(divsCount, 3);
    // [ts] });
    // [ts] test("$$eval should accept extra arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div>hello</div>
    // [ts] <div>beautiful</div>
    // [ts] <div>world!</div>`,
    // [ts] );
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCountPlus5 = await page.$$eval(
    // [ts] "allArray/div",
    // [ts] (divs, two, three) => {
    // [ts] return divs.length + (two) + (three);
    // [ts] },
    // [ts] 2,
    // [ts] 3,
    // [ts] );
    // [ts] expect_eq(divsCountPlus5, 8);
    // [ts] });
    // [ts] test("$$eval should accept ElementHandles as arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<section>2</section>
    // [ts] <section>2</section>
    // [ts] <section>1</section>
    // [ts] <div>3</div>`,
    // [ts] );
    // [ts] using divHandle = (page.$("div"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const sum = await page.$$eval(
    // [ts] "allArray/section",
    // [ts] (sections, div) => {
    // [ts] return (
    // [ts] sections.reduce((acc, section) => {
    // [ts] return acc + Number(section.textContent);
    // [ts] }, 0) + Number(div.textContent)
    // [ts] );
    // [ts] },
    // [ts] divHandle,
    // [ts] );
    // [ts] expect_eq(sum, 8);
    // [ts] });
    // [ts] test("$$eval should handle many elements", () => {
    // [ts] _.timeout(25_000);
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(
    // [ts] `
    // [ts] for (var i = 0; i <= 1000; i++) {
    // [ts] const section = document.createElement('section');
    // [ts] section.textContent = i;
    // [ts] document.body.appendChild(section);
    // [ts] }
    // [ts] `,
    // [ts] );
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const sum = await page.$$eval('allArray/section', sections => {
    // [ts-skip] return sections.reduce((acc, section) => {
    // [ts-skip] return acc + Number(section.textContent);
    // [ts-skip] }, 0);
    // [ts-skip] });
    // [ts] expect_eq(sum, 500500);
    // [ts] });
}
