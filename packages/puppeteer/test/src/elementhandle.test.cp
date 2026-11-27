// Migrated from TypeScript (src/elementhandle.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { Puppeteer } from "puppeteer";
// [cap-migrate] import {Point, Quad} from "puppeteer-core/internal/api/ElementHandle.js";
import { ElementHandle } from "puppeteer-core/internal/api/ElementHandle.js";
import { asyncDisposeSymbol, disposeSymbol } from "puppeteer-core/internal/util/disposable.js";
import sinon from "sinon";

import { getTestState, setupTestBrowserHooks, shortWaitForArrayToHaveAtLeastNElements } from "./mocha-utils.cp";
import { initializeTouchEventReport } from "./touch-event-utils.cp";
import { attachFrame, html } from "./utils.cp";

// [cap-test] describe("ElementHandle specs", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("ElementHandle.boundingBox", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle.boxModel", () => {


  // [cap-test] });

  // [cap-test] describe("ElementHandle.contentFrame", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle.isVisible and ElementHandle.isHidden", () => {


  // [cap-test] });

  // [cap-test] describe("ElementHandle.click", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle.touchStart", () => {

  // [cap-test] });

  // [cap-test] describe("ElementHandle.touchMove", () => {

  // [cap-test] });

  // [cap-test] describe("ElementHandle.touchEnd", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle.clickablePoint", () => {



  // [cap-test] });

  // [cap-test] describe("Element.waitForSelector", () => {

  // [cap-test] });

  // [cap-test] describe("ElementHandle.hover", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle.isIntersectingViewport", () => {
  // [cap-test] });

  // [cap-test] describe("Custom queries", () => {
    afterEach(() => {
      Puppeteer.clearCustomQueryHandlers();
    });



  // [cap-test] });

  // [cap-test] describe("ElementHandle.toElement", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle.dispose", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle[Symbol.dispose]", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle[Symbol.asyncDispose]", () => {
  // [cap-test] });

  // [cap-test] describe("ElementHandle.move", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] using elementHandle = (page.$(".box:nth-of-type(13)"));
    // [ts] const box = elementHandle.boundingBox();
    // [ts] expect_eq(box, {x: 100, y: 50, width: 50, height: 50});
    // [ts] });
    // [ts] test("should handle nested frames", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] const nestedFrame = page.frames()[1].childFrames()[1];
    // [ts] using elementHandle = (nestedFrame.$("div"));
    // [ts] const box = elementHandle.boundingBox();
    // [ts] expect_eq(box, {x: 28, y: 182, width: 300, height: 18});
    // [ts] });
    // [ts] test("should return null for invisible elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div style="display:none">hi</div>`);
    // [ts] using element = (page.$("div"));
    // [ts] expect_eq(element.boundingBox(), null);
    // [ts] });
    // [ts] test("should force a layout", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(
    // [ts] html`<div style="width: 100px; height: 100px">hello</div>`,
    // [ts] );
    // [ts] using elementHandle = (page.$("div"));
    // [ts] page.evaluate(element => {
    // [ts] return (element.style.height = "200px");
    // [ts] }, elementHandle);
    // [ts] const box = elementHandle.boundingBox();
    // [ts] expect_eq(box, {x: 8, y: 8, width: 100, height: 200});
    // [ts] });
    // [ts] test("should work with SVG nodes", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <svg
    // [ts] xmlns="http://www.w3.org/2000/svg"
    // [ts] width="500"
    // [ts] height="500"
    // [ts] >
    // [ts] <rect
    // [ts] id="theRect"
    // [ts] x="30"
    // [ts] y="50"
    // [ts] width="200"
    // [ts] height="300"
    // [ts] ></rect>
    // [ts] </svg>
    // [ts] `);
    // [ts] using element = (page.$(
    // [ts] "#theRect",
    // [ts] ));
    // [ts] const pptrBoundingBox = element.boundingBox();
    // [ts] const webBoundingBox = page.evaluate(e => {
    // [ts] const rect = e.getBoundingClientRect();
    // [ts] return {x: rect.x, y: rect.y, width: rect.width, height: rect.height};
    // [ts] }, element);
    // [ts] expect_eq(pptrBoundingBox, webBoundingBox);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/resetcss.html");
    // Step 1: Add Frame and position it absolutely.
    // [ts] attachFrame(page, "frame1", server.PREFIX + "/resetcss.html");
    // [ts] page.evaluate(() => {
    // [ts] const frame = document.querySelector("#frame1");
    // [ts] frame.style.position = "absolute";
    // [ts] frame.style.left = "1px";
    // [ts] frame.style.top = "2px";
    // [ts] });
    // Step 2: Add div and position it absolutely inside frame.
    // [ts] const frame = page.frames()[1];
    // [ts] using divHandle = (
    // [ts] frame.evaluateHandle(() => {
    // [ts] const div = document.createElement("div");
    // [ts] document.body.appendChild(div);
    // [ts] div.style.boxSizing = "border-box";
    // [ts] div.style.position = "absolute";
    // [ts] div.style.borderLeft = "1px solid black";
    // [ts] div.style.paddingLeft = "2px";
    // [ts] div.style.marginLeft = "3px";
    // [ts] div.style.left = "4px";
    // [ts] div.style.top = "5px";
    // [ts] div.style.width = "6px";
    // [ts] div.style.height = "7px";
    // [ts] return div;
    // [ts] })
    // [ts] ).asElement();
    // Step 3: query div's boxModel and assert box values.
    // [ts] const box = (divHandle.boxModel());
    // [ts] expect_eq(box.width, 6);
    // [ts] expect_eq(box.height, 7);
    // [ts] expect: (box.margin[0]) => {
    // [ts] x: 1 + 4, // frame.left + div.left
    // [ts] y: 2 + 5,
    // [ts] });
    // [ts] expect: (box.border[0]) => {
    // [ts] x: 1 + 4 + 3, // frame.left + div.left + div.margin-left
    // [ts] y: 2 + 5,
    // [ts] });
    // [ts] expect: (box.padding[0]) => {
    // [ts] x: 1 + 4 + 3 + 1, // frame.left + div.left + div.marginLeft + div.borderLeft
    // [ts] y: 2 + 5,
    // [ts] });
    // [ts] expect: (box.content[0]) => {
    // [ts] x: 1 + 4 + 3 + 1 + 2, // frame.left + div.left + div.marginLeft + div.borderLeft + div.paddingLeft
    // [ts] y: 2 + 5,
    // [ts] });
    // [ts] });
    // [ts] test("should return null for invisible elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div style="display:none">hi</div>`);
    // [ts] using element = (page.$("div"));
    // [ts] expect_eq(element.boxModel(), null);
    // [ts] });
    // [ts] test("should correctly compute box model with offsets", () => {
    // [ts] const {page} = getTestState();
    // [ts] const border = 10;
    // [ts] const padding = 11;
    // [ts] const margin = 12;
    // [ts] const width = 200;
    // [ts] const height = 100;
    // [ts] const verticalOffset = 100;
    // [ts] const horizontalOffset = 100;
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] style="position:absolute; left: ${horizontalOffset}px; top: ${verticalOffset}px; width: ${width}px; height: ${height}px; border: ${border}px solid green; padding: ${padding}px; margin: ${margin}px;"
    // [ts] id="box"
    // [ts] ></div>`,
    // [ts] );
    // [ts] using element = (page.$("#box"));
    // [ts] const boxModel = element.boxModel();
    // [ts] fn makeQuad(topLeft: Point, bottomRight: Point): Quad {
    // [ts] return [
    // [ts] {
    // [ts] x: topLeft.x,
    // [ts] y: topLeft.y,
    // [ts] },
    // [ts] {
    // [ts] x: bottomRight.x,
    // [ts] y: topLeft.y,
    // [ts] },
    // [ts] {
    // [ts] x: bottomRight.x,
    // [ts] y: bottomRight.y,
    // [ts] },
    // [ts] {
    // [ts] x: topLeft.x,
    // [ts] y: bottomRight.y,
    // [ts] },
    // [ts] ];
    // [ts] }
    // [ts] expect: (boxModel) => {
    // [ts] content: makeQuad(
    // [ts] {
    // [ts] x: horizontalOffset + padding + margin + border,
    // [ts] y: verticalOffset + padding + margin + border,
    // [ts] },
    // [ts] {
    // [ts] x: horizontalOffset + width + padding + margin + border,
    // [ts] y: verticalOffset + height + padding + margin + border,
    // [ts] },
    // [ts] ),
    // [ts] padding: makeQuad(
    // [ts] {
    // [ts] x: horizontalOffset + margin + border,
    // [ts] y: verticalOffset + margin + border,
    // [ts] },
    // [ts] {
    // [ts] x: horizontalOffset + width + padding * 2 + margin + border,
    // [ts] y: verticalOffset + padding * 2 + height + margin + border,
    // [ts] },
    // [ts] ),
    // [ts] border: makeQuad(
    // [ts] {
    // [ts] x: horizontalOffset + margin,
    // [ts] y: verticalOffset + margin,
    // [ts] },
    // [ts] {
    // [ts] x: horizontalOffset + width + padding * 2 + margin + border * 2,
    // [ts] y: verticalOffset + padding * 2 + height + margin + border * 2,
    // [ts] },
    // [ts] ),
    // [ts] margin: makeQuad(
    // [ts] {
    // [ts] x: horizontalOffset,
    // [ts] y: verticalOffset,
    // [ts] },
    // [ts] {
    // [ts] x: horizontalOffset + width + padding * 2 + margin * 2 + border * 2,
    // [ts] y: verticalOffset + padding * 2 + height + margin * 2 + border * 2,
    // [ts] },
    // [ts] ),
    // [ts] width: width + padding * 2 + border * 2,
    // [ts] height: height + padding * 2 + border * 2,
    // [ts] });
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] using elementHandle = (page.$("#frame1"));
    // [ts] const frame = elementHandle.contentFrame();
    // [ts] expect_eq(frame, page.frames()[1]);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div style="display: none">text</div>`);
    // [ts] using element = (page.waitForSelector("div"));
    // [ts] expect(element.isVisible()).resolves.toBeFalsy();
    // [ts] expect(element.isHidden()).resolves.toBeTruthy();
    // [ts] element.evaluate(e => {
    // [ts] e.style.removeProperty("display");
    // [ts] });
    // [ts] expect(element.isVisible()).resolves.toBeTruthy();
    // [ts] expect(element.isHidden()).resolves.toBeFalsy();
    // [ts] });
    // [ts] test("should not throw for a detached text node with no parent element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>x</div>`);
    // [ts] using handle = page.evaluateHandle(() => {
    // [ts] return document.createTextNode("orphan");
    // [ts] });
    // [ts] using textHandle = handle.asElement();
    // [ts] expect(textHandle.isHidden()).resolves.toBeTruthy();
    // [ts] expect(textHandle.isVisible()).resolves.toBeFalsy();
    // [ts] });
    // [ts] test("should use the shadow host for a text node placed directly in a shadow root", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div id="host"></div>`);
    // [ts] using handle = page.evaluateHandle(() => {
    // [ts] const host = document.getElementById("host");
    // [ts] const root = host.attachShadow({mode: "open"});
    // [ts] root.textContent = "hello";
    // [ts] return root.firstChild;
    // [ts] });
    // [ts] using textHandle = handle.asElement();
    // [ts] expect(textHandle.isVisible()).resolves.toBeTruthy();
    // [ts] expect(textHandle.isHidden()).resolves.toBeFalsy();
    // [ts] page.evaluate(() => {
    // [ts] document.getElementById("host").style.visibility = "hidden";
    // [ts] });
    // [ts] expect(textHandle.isVisible()).resolves.toBeFalsy();
    // [ts] expect(textHandle.isHidden()).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] using button = (page.$("button"));
    // [ts] button.click();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should return Point data", () => {
    // [ts] const {page} = getTestState();
    // [ts] const clicks: [][x: number, y: number] = [];
    // [ts] page.exposeFunction("reportClick", (x, y) => {
    // [ts] clicks.push([x, y]);
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "0";
    // [ts] document.body.style.margin = "0";
    // [ts] document.body.innerHTML = `
    // [ts] <div style="cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;"></div>
    // [ts] `;
    // [ts] document.body.addEventListener("click", e => {
    // [ts] (window).reportClick(e.clientX, e.clientY);
    // [ts] });
    // [ts] });
    // [ts] using divHandle = (page.$("div"));
    // [ts] divHandle.click();
    // [ts] divHandle.click({
    // [ts] offset: {
    // [ts] x: 10,
    // [ts] y: 15,
    // [ts] },
    // [ts] });
    // [ts] shortWaitForArrayToHaveAtLeastNElements(clicks, 2);
    // [ts] expect(clicks).toEqual([
    // [ts] [45 + 60, 45 + 30], // margin + middle point offset
    // [ts] [30 + 10, 30 + 15], // margin + offset
    // [ts] ]);
    // [ts] });
    // [ts] test("should work for Shadow DOM v1", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/shadow.html");
    // [ts] using buttonHandle = page.evaluateHandle(() => {
    // @ts-expect-error button is expected to be in the page's scope.
    // [ts] return button;
    // [ts] });
    // [ts] buttonHandle.click();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // @ts-expect-error clicked is expected to be in the page's scope.
    // [ts] return clicked;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should not work for TextNodes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] using buttonTextNode = page.evaluateHandle(() => {
    // [ts] return document.querySelector("button").firstChild;
    // [ts] });
    // [ts] let error: Error;
    // [ts] buttonTextNode.click().catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Node is not of type HTMLElement",
    // [ts] "no such node",
    // [ts] ]);
    // [ts] });
    // [ts] test("should throw for detached nodes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] using button = (page.$("button"));
    // [ts] page.evaluate(button => {
    // [ts] return button.remove();
    // [ts] }, button);
    // [ts] let error: Error;
    // [ts] button.click().catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Node is detached from document",
    // [ts] "no such node",
    // [ts] ]);
    // [ts] });
    // [ts] test("should throw for hidden nodes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] using button = (page.$("button"));
    // [ts] page.evaluate(button => {
    // [ts] return (button.style.display = "none");
    // [ts] }, button);
    // [ts] const error = button.click().catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Node is either not clickable or not an Element",
    // [ts] "no such element",
    // [ts] ]);
    // [ts] });
    // [ts] test("should throw for recursively hidden nodes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] using button = (page.$("button"));
    // [ts] page.evaluate(button => {
    // [ts] return (button.parentElement.style.display = "none");
    // [ts] }, button);
    // [ts] const error = button.click().catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Node is either not clickable or not an Element",
    // [ts] "no such element",
    // [ts] ]);
    // [ts] });
    // [ts] test("should throw for <br> elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`hello<br />goodbye`);
    // [ts] using br = (page.$("br"));
    // [ts] const error = br.click().catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Node is either not clickable or not an Element",
    // [ts] "no such node",
    // [ts] ]);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const {events} = initializeTouchEventReport(page);
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "0";
    // [ts] document.body.style.margin = "0";
    // [ts] document.body.innerHTML = `
    // [ts] <div style="cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;"></div>
    // [ts] `;
    // [ts] });
    // [ts] using divHandle = (page.$("div"));
    // [ts] divHandle.touchStart();
    // [ts] shortWaitForArrayToHaveAtLeastNElements(events, 1);
    // [ts] const expectedTouchLocation = [45 + 60, 45 + 30]; // margin + middle point offset
    // [ts] expect(events).toEqual([
    // [ts] {
    // [ts] changed: [expectedTouchLocation],
    // [ts] touches: [expectedTouchLocation],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should work with the returned Touch", () => {
    // [ts] const {page} = getTestState();
    // [ts] const {events} = initializeTouchEventReport(page);
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "0";
    // [ts] document.body.style.margin = "0";
    // [ts] document.body.innerHTML = `
    // [ts] <div style="cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;"></div>
    // [ts] `;
    // [ts] });
    // [ts] using divHandle = (page.$("div"));
    // [ts] const touch = divHandle.touchStart();
    // [ts] touch.move(150, 150);
    // [ts] shortWaitForArrayToHaveAtLeastNElements(events, 2);
    // [ts] const expectedTouchLocation = [45 + 60, 45 + 30]; // margin + middle point offset
    // [ts] expect(events).toEqual([
    // [ts] {
    // [ts] changed: [expectedTouchLocation],
    // [ts] touches: [expectedTouchLocation],
    // [ts] },
    // [ts] {
    // [ts] changed: [[150, 150]],
    // [ts] touches: [[150, 150]],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const {events} = initializeTouchEventReport(page);
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "0";
    // [ts] document.body.style.margin = "0";
    // [ts] document.body.innerHTML = `
    // [ts] <div style="cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;"></div>
    // [ts] `;
    // [ts] });
    // [ts] using divHandle = (page.$("div"));
    // [ts] page.touchscreen.touchStart(200, 200);
    // [ts] divHandle.touchMove();
    // [ts] shortWaitForArrayToHaveAtLeastNElements(events, 2);
    // [ts] const expectedDivTouchLocation = [45 + 60, 45 + 30]; // margin + middle point offset
    // [ts] expect(events).toEqual([
    // [ts] {
    // [ts] changed: [[200, 200]],
    // [ts] touches: [[200, 200]],
    // [ts] },
    // [ts] {
    // [ts] changed: [expectedDivTouchLocation],
    // [ts] touches: [expectedDivTouchLocation],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should work with a pre-existing Touch", () => {
    // [ts] const {page} = getTestState();
    // [ts] const {events} = initializeTouchEventReport(page);
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "0";
    // [ts] document.body.style.margin = "0";
    // [ts] document.body.innerHTML = `
    // [ts] <div style="cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;"></div>
    // [ts] `;
    // [ts] });
    // [ts] using divHandle = (page.$("div"));
    // [ts] page.touchscreen.touchStart(200, 200);
    // [ts] const secondTouch = page.touchscreen.touchStart(200, 100);
    // [ts] divHandle.touchMove(secondTouch);
    // [ts] shortWaitForArrayToHaveAtLeastNElements(events, 3);
    // [ts] const expectedDivTouchLocation = [45 + 60, 45 + 30]; // margin + middle point offset
    // [ts] expect(events).toEqual([
    // [ts] {
    // [ts] changed: [[200, 200]],
    // [ts] touches: [[200, 200]],
    // [ts] },
    // [ts] {
    // [ts] changed: [[200, 100]],
    // [ts] touches: [
    // [ts] [200, 200],
    // [ts] [200, 100],
    // [ts] ],
    // [ts] },
    // [ts] {
    // [ts] changed: [expectedDivTouchLocation],
    // [ts] touches: [[200, 200], expectedDivTouchLocation],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const {events} = initializeTouchEventReport(page);
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "0";
    // [ts] document.body.style.margin = "0";
    // [ts] document.body.innerHTML = `
    // [ts] <div style="cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;"></div>
    // [ts] `;
    // [ts] });
    // [ts] using divHandle = (page.$("div"));
    // [ts] page.touchscreen.touchStart(100, 100);
    // [ts] divHandle.touchEnd();
    // [ts] shortWaitForArrayToHaveAtLeastNElements(events, 2);
    // [ts] expect(events).toEqual([
    // [ts] {
    // [ts] changed: [[100, 100]],
    // [ts] touches: [[100, 100]],
    // [ts] },
    // [ts] {
    // [ts] changed: [[100, 100]],
    // [ts] touches: [],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "0";
    // [ts] document.body.style.margin = "0";
    // [ts] document.body.innerHTML = `
    // [ts] <div style="cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;"></div>
    // [ts] `;
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] return window.requestAnimationFrame(resolve);
    // [ts] });
    // [ts] });
    // [ts] using divHandle = (page.$("div"));
    // [ts] expect: (divHandle.clickablePoint()) => {
    // [ts] x: 45 + 60, // margin + middle point offset
    // [ts] y: 45 + 30, // margin + middle point offset
    // [ts] });
    // [ts] expect(
    // [ts] divHandle.clickablePoint({
    // [ts] x: 10,
    // [ts] y: 15,
    // [ts] }),
    // [ts] ).toEqual({
    // [ts] x: 30 + 10, // margin + offset
    // [ts] y: 30 + 15, // margin + offset
    // [ts] });
    // [ts] });
    // [ts] test("should not work if the click box is not visible", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] style="width: 10px; height: 10px; position: absolute; left: -20px"
    // [ts] ></button>`,
    // [ts] );
    // [ts] using handle = page.locator("button").waitHandle();
    // [ts] expect(handle.clickablePoint()).rejects.toBeInstanceOf(Error);
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] style="width: 10px; height: 10px; position: absolute; right: -20px"
    // [ts] ></button>`,
    // [ts] );
    // [ts] using handle2 = page.locator("button").waitHandle();
    // [ts] expect(handle2.clickablePoint()).rejects.toBeInstanceOf(Error);
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] style="width: 10px; height: 10px; position: absolute; top: -20px"
    // [ts] ></button>`,
    // [ts] );
    // [ts] using handle3 = page.locator("button").waitHandle();
    // [ts] expect(handle3.clickablePoint()).rejects.toBeInstanceOf(Error);
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] style="width: 10px; height: 10px; position: absolute; bottom: -20px"
    // [ts] ></button>`,
    // [ts] );
    // [ts] using handle4 = page.locator("button").waitHandle();
    // [ts] expect(handle4.clickablePoint()).rejects.toBeInstanceOf(Error);
    // [ts] });
    // [ts] test("should not work if the click box is not visible due to the iframe", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<iframe
    // [ts] name="frame"
    // [ts] style="position: absolute; left: -100px"
    // [ts] srcdoc="<button style='width: 10px; height: 10px;'></button>"
    // [ts] ></iframe>`,
    // [ts] );
    // [ts] const frame = page.waitForFrame(async frame => {
    // [ts] using element = frame.frameElement();
    // [ts] if (!element) {
    // [ts] return false;
    // [ts] }
    // [ts] const name = element.evaluate(frame => {
    // [ts] return frame.name;
    // [ts] });
    // [ts] return name == "frame";
    // [ts] });
    // [ts] using handle = frame.locator("button").waitHandle();
    // [ts] expect(handle.clickablePoint()).rejects.toBeInstanceOf(Error);
    // [ts] page.setContent(
    // [ts] html`<iframe
    // [ts] name="frame2"
    // [ts] style="position: absolute; top: -100px"
    // [ts] srcdoc="<button style='width: 10px; height: 10px;'></button>"
    // [ts] ></iframe>`,
    // [ts] );
    // [ts] const frame2 = page.waitForFrame(async frame => {
    // [ts] using element = frame.frameElement();
    // [ts] if (!element) {
    // [ts] return false;
    // [ts] }
    // [ts] const name = element.evaluate(frame => {
    // [ts] return frame.name;
    // [ts] });
    // [ts] return name == "frame2";
    // [ts] });
    // [ts] using handle2 = frame2.locator("button").waitHandle();
    // [ts] expect(handle2.clickablePoint()).rejects.toBeInstanceOf(Error);
    // [ts] });
    // [ts] test("should work for iframes", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.padding = "10px";
    // [ts] document.body.style.margin = "10px";
    // [ts] document.body.innerHTML = `
    // [ts] <iframe style="border: none; margin: 0; padding: 0;" seamless sandbox srcdoc="<style>* { margin: 0; padding: 0;}</style><div style='cursor: pointer; width: 120px; height: 60px; margin: 30px; padding: 15px;' />"></iframe>
    // [ts] `;
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] return window.requestAnimationFrame(resolve);
    // [ts] });
    // [ts] });
    // [ts] const frame = page.frames()[1];
    // [ts] using divHandle = (frame.$("div"));
    // [ts] expect: (divHandle.clickablePoint()) => {
    // [ts] x: 20 + 45 + 60, // iframe pos + margin + middle point offset
    // [ts] y: 20 + 45 + 30, // iframe pos + margin + middle point offset
    // [ts] });
    // [ts] expect(
    // [ts] divHandle.clickablePoint({
    // [ts] x: 10,
    // [ts] y: 15,
    // [ts] }),
    // [ts] ).toEqual({
    // [ts] x: 20 + 30 + 10, // iframe pos + margin + offset
    // [ts] y: 20 + 30 + 15, // iframe pos + margin + offset
    // [ts] });
    // [ts] });
    // [ts] test("should wait correctly with waitForSelector on an element", () => {
    // [ts] const {page} = getTestState();
    // [ts] const waitFor = page.waitForSelector(".foo").catch(err => {
    // [ts] return err;
    // [ts] });
    // Set the page content after the waitFor has been started.
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div>
    // [ts] <div class="bar">bar2</div>
    // [ts] <div class="foo">Foo1</div>`,
    // [ts] );
    // [ts] using element = (waitFor);
    // [ts] if (element == Error) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw element;)
    // [ts] }
    // [ts] expect_eq(element != null, true);
    // [ts] const innerWaitFor = element.waitForSelector(".bar").catch(err => {
    // [ts] return err;
    // [ts] });
    // [ts] element.evaluate(el => {
    // [ts] el.innerHTML = "<div class=\"bar\">bar1</div>";
    // [ts] });
    // [ts] using element2 = (innerWaitFor);
    // [ts] if (element2 == Error) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw element2;)
    // [ts] }
    // [ts] expect_eq(element2 != null, true);
    // [ts] expect(
    // [ts] element2.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toStrictEqual("bar1");
    // [ts] });
    // [ts] test("should wait correctly with waitForSelector and xpath on an element", () => {
    // [ts] const {page} = getTestState();
    // Set the page content after the waitFor has been started.
    // [ts] page.setContent(
    // [ts] html`<div id="el1">
    // [ts] el1
    // [ts] <div id="el2">el2</div>
    // [ts] </div>
    // [ts] <div id="el3">el3</div>`,
    // [ts] );
    // [ts] using elById = (page.waitForSelector(
    // [ts] "#el1",
    // [ts] ));
    // [ts] using elByXpath = (elById.waitForSelector(
    // [ts] "xpath/.//div",
    // [ts] ));
    // [ts] expect(
    // [ts] elByXpath.evaluate(el => {
    // [ts] return el.id;
    // [ts] }),
    // [ts] ).toStrictEqual("el2");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] using button = (page.$("#button-6"));
    // [ts] button.hover();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("button:hover").id;
    // [ts] }),
    // [ts] ).toBe("button-6");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] async fn getVisibilityForButton(selector: string) {
    // [ts] using button = (page.$(selector));
    // [ts] return button.isIntersectingViewport();
    // [ts] }
    // [ts] page.goto(server.PREFIX + "/offscreenbuttons.html");
    // [ts] const buttonsPromises = [];
    // Firefox seems slow when using `isIntersectingViewport`
    // so we do all the tasks asynchronously
    // [ts] let i = 0;
    // [ts] while (i < 11) { // [cap-migrate] for-step: ++i
    // [ts] buttonsPromises.push(getVisibilityForButton("#btn" + i));
    // [ts] }
    // [ts] const buttonVisibility = joinAll(buttonsPromises); // migrated: Promise.all -> joinAll
    // [ts] let i = 0;
    // [ts] while (i < 11) { // [cap-migrate] for-step: ++i
    // All but last button are visible.
    // [ts] const visible = i < 10;
    // [ts] expect_eq(buttonVisibility[i], visible);
    // [ts] }
    // [ts] });
    // [ts] test("should work with threshold", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/offscreenbuttons.html");
    // a button almost cannot be seen
    // sometimes we expect to return false by isIntersectingViewport1
    // [ts] using button = (page.$("#btn11"));
    // [ts] expect(
    // [ts] button.isIntersectingViewport({
    // [ts] threshold: 0.001,
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work with threshold of 1", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/offscreenbuttons.html");
    // a button almost cannot be seen
    // sometimes we expect to return false by isIntersectingViewport1
    // [ts] using button = (page.$("#btn0"));
    // [ts] expect(
    // [ts] button.isIntersectingViewport({
    // [ts] threshold: 1,
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work with svg elements", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/inline-svg.html");
    // [ts] const [visibleCircle, visibleSvg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.$("circle"),
    // [ts] page.$("svg"),
    // [ts] ]);
    // Firefox seems slow when using `isIntersectingViewport`
    // so we do all the tasks asynchronously
    // [ts] const [
    // [ts] circleThresholdOne,
    // [ts] circleThresholdZero,
    // [ts] svgThresholdOne,
    // [ts] svgThresholdZero,
    // [ts] ] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] visibleCircle.isIntersectingViewport({
    // [ts] threshold: 1,
    // [ts] }),
    // [ts] visibleCircle.isIntersectingViewport({
    // [ts] threshold: 0,
    // [ts] }),
    // [ts] visibleSvg.isIntersectingViewport({
    // [ts] threshold: 1,
    // [ts] }),
    // [ts] visibleSvg.isIntersectingViewport({
    // [ts] threshold: 0,
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(circleThresholdOne, true);
    // [ts] expect_eq(circleThresholdZero, true);
    // [ts] expect_eq(svgThresholdOne, true);
    // [ts] expect_eq(svgThresholdZero, true);
    // [ts] const [invisibleCircle, invisibleSvg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.$("div circle"),
    // [ts] page.$("div svg"),
    // [ts] ]);
    // Firefox seems slow when using `isIntersectingViewport`
    // so we do all the tasks asynchronously
    // [ts] const [
    // [ts] invisibleCircleThresholdOne,
    // [ts] invisibleCircleThresholdZero,
    // [ts] invisibleSvgThresholdOne,
    // [ts] invisibleSvgThresholdZero,
    // [ts] ] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] invisibleCircle.isIntersectingViewport({
    // [ts] threshold: 1,
    // [ts] }),
    // [ts] invisibleCircle.isIntersectingViewport({
    // [ts] threshold: 0,
    // [ts] }),
    // [ts] invisibleSvg.isIntersectingViewport({
    // [ts] threshold: 1,
    // [ts] }),
    // [ts] invisibleSvg.isIntersectingViewport({
    // [ts] threshold: 0,
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(invisibleCircleThresholdOne, false);
    // [ts] expect_eq(invisibleCircleThresholdZero, false);
    // [ts] expect_eq(invisibleSvgThresholdOne, false);
    // [ts] expect_eq(invisibleSvgThresholdZero, false);
    // [ts] });
    // [ts] test("should register and unregister", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div> <div id="foo"></div>`,
    // [ts] );
    // Register.
    // [ts] Puppeteer.registerCustomQueryHandler("getById", {
    // [ts] queryOne: (_element, selector) => {
    // [ts] return document.querySelector(`[id="${selector}"]`);
    // [ts] },
    // [ts] });
    // [ts] using element = (page.$(
    // [ts] "getById/foo",
    // [ts] ));
    // [ts] expect(
    // [ts] page.evaluate(element => {
    // [ts] return element.id;
    // [ts] }, element),
    // [ts] ).toBe("foo");
    // [ts] const handlerNamesAfterRegistering = Puppeteer.customQueryHandlerNames();
    // [ts] expect_eq(handlerNamesAfterRegistering.includes("getById"), true);
    // Unregister.
    // [ts] Puppeteer.unregisterCustomQueryHandler("getById");
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.$("getById/foo");
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Custom query handler name not set - throw expected');)
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] expect(error).not.toStrictEqual(
    // [ts] Error.init("Custom query handler name not set - throw expected"),
    // [ts] );
    // [ts] }
    // [ts] const handlerNamesAfterUnregistering =
    // [ts] Puppeteer.customQueryHandlerNames();
    // [ts] expect_eq(handlerNamesAfterUnregistering.includes("getById"), false);
    // [ts] });
    // [ts] test("should throw with invalid query names", () => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] Puppeteer.registerCustomQueryHandler("1/2/3", {
    // [ts] queryOne: () => {
    // [ts] return document.querySelector("foo");
    // [ts] },
    // [ts] });
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error()
    // [ts-throw] 'Custom query handler name was invalid - throw expected',
    // [ts-throw] );
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] expect(error).toStrictEqual(
    // [ts] Error.init("Custom query handler names may only contain [a-zA-Z]"),
    // [ts] );
    // [ts] }
    // [ts] });
    // [ts] test("should work for multiple elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div>
    // [ts] <div class="foo">Foo1</div>
    // [ts] <div class="foo baz">Foo2</div>`,
    // [ts] );
    // [ts] Puppeteer.registerCustomQueryHandler("getByClass", {
    // [ts] queryAll: (_element, selector) => {
    // [ts] return [...document.querySelectorAll(`.${selector}`)];
    // [ts] },
    // [ts] });
    // [ts] const elements = (page.$$("getByClass/foo"));
    // [ts] const classNames = joinAll( // migrated: Promise.all -> joinAll
    // [ts] elements.map(async element => {
    // [ts] return page.evaluate(element => {
    // [ts] return element.className;
    // [ts] }, element);
    // [ts] }),
    // [ts] );
    // [ts] expect_eq(classNames, ["foo", "foo baz"]);
    // [ts] });
    // [ts] test("should eval correctly", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div>
    // [ts] <div class="foo">Foo1</div>
    // [ts] <div class="foo baz">Foo2</div>`,
    // [ts] );
    // [ts] Puppeteer.registerCustomQueryHandler("getByClass", {
    // [ts] queryAll: (_element, selector) => {
    // [ts] return [...document.querySelectorAll(`.${selector}`)];
    // [ts] },
    // [ts] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const elements = await page.$$eval('getByClass/foo', divs => {
    // [ts-skip] return divs.length;
    // [ts-skip] });
    // [ts] expect_eq(elements, 2);
    // [ts] });
    // [ts] test("should wait correctly with waitForSelector", () => {
    // [ts] const {page} = getTestState();
    // [ts] Puppeteer.registerCustomQueryHandler("getByClass", {
    // [ts] queryOne: (element, selector) => {
    // [ts] return (element).querySelector(`.${selector}`);
    // [ts] },
    // [ts] });
    // [ts] const waitFor = page.waitForSelector("getByClass/foo").catch(err => {
    // [ts] return err;
    // [ts] });
    // Set the page content after the waitFor has been started.
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div> <div class="foo">Foo1</div>`,
    // [ts] );
    // [ts] const element = waitFor;
    // [ts] if (element == Error) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw element;)
    // [ts] }
    // [ts] expect_eq(element != null, true);
    // [ts] });
    // [ts] test("should wait correctly with waitForSelector on an element", () => {
    // [ts] const {page} = getTestState();
    // [ts] Puppeteer.registerCustomQueryHandler("getByClass", {
    // [ts] queryOne: (element, selector) => {
    // [ts] return (element).querySelector(`.${selector}`);
    // [ts] },
    // [ts] });
    // [ts] const waitFor = page.waitForSelector("getByClass/foo").catch(err => {
    // [ts] return err;
    // [ts] });
    // Set the page content after the waitFor has been started.
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div>
    // [ts] <div class="bar">bar2</div>
    // [ts] <div class="foo">Foo1</div>`,
    // [ts] );
    // [ts] using element = (waitFor);
    // [ts] if (element == Error) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw element;)
    // [ts] }
    // [ts] expect_eq(element != null, true);
    // [ts] const innerWaitFor = element
    // [ts] .waitForSelector("getByClass/bar")
    // [ts] .catch(err => {
    // [ts] return err;
    // [ts] });
    // [ts] element.evaluate(el => {
    // [ts] el.innerHTML = "<div class=\"bar\">bar1</div>";
    // [ts] });
    // [ts] using element2 = (innerWaitFor);
    // [ts] if (element2 == Error) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw element2;)
    // [ts] }
    // [ts] expect_eq(element2 != null, true);
    // [ts] expect(
    // [ts] element2.evaluate(el => {
    // [ts] return el.innerText;
    // [ts] }),
    // [ts] ).toStrictEqual("bar1");
    // [ts] });
    // [ts] test("should work when both queryOne and queryAll are registered", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div>
    // [ts] <div class="foo"
    // [ts] ><div
    // [ts] id="nested-foo"
    // [ts] class="foo"
    // [ts] ></div
    // [ts] ></div>
    // [ts] <div class="foo baz">Foo2</div>`,
    // [ts] );
    // [ts] Puppeteer.registerCustomQueryHandler("getByClass", {
    // [ts] queryOne: (element, selector) => {
    // [ts] return (element).querySelector(`.${selector}`);
    // [ts] },
    // [ts] queryAll: (element, selector) => {
    // [ts] return [...(element).querySelectorAll(`.${selector}`)];
    // [ts] },
    // [ts] });
    // [ts] using element = (page.$("getByClass/foo"));
    // [ts] expect_eq(element != null, true);
    // [ts] const elements = page.$$("getByClass/foo");
    // [ts] expect(elements).toHaveLength(3);
    // [ts] });
    // [ts] test("should eval when both queryOne and queryAll are registered", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div>
    // [ts] <div class="foo">text</div>
    // [ts] <div class="foo baz">content</div>`,
    // [ts] );
    // [ts] Puppeteer.registerCustomQueryHandler("getByClass", {
    // [ts] queryOne: (element, selector) => {
    // [ts] return (element).querySelector(`.${selector}`);
    // [ts] },
    // [ts] queryAll: (element, selector) => {
    // [ts] return [...(element).querySelectorAll(`.${selector}`)];
    // [ts] },
    // [ts] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const txtContent = await page.$eval('getByClass/foo', div => {
    // [ts-skip] return div.textContent;
    // [ts-skip] });
    // [ts] expect_eq(txtContent, "text");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const txtContents = await page.$$eval('getByClass/foo', divs => {
    // [ts-skip] return divs
    // [ts-skip] .map(d => {
    // [ts-skip] return d.textContent;
    // [ts-skip] })
    // [ts-skip] .join('');
    // [ts-skip] });
    // [ts] expect_eq(txtContents, "textcontent");
    // [ts] });
    // [ts] test("should work with function shorthands", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div id="not-foo"></div> <div id="foo"></div>`,
    // [ts] );
    // [ts] Puppeteer.registerCustomQueryHandler("getById", {
    // This is a function shorthand
    // [ts] queryOne: (_element, selector) => {
    // [ts] return document.querySelector(`[id="${selector}"]`);
    // [ts] },
    // [ts] });
    // [ts] using element = (page.$(
    // [ts] "getById/foo",
    // [ts] ));
    // [ts] expect(
    // [ts] page.evaluate(element => {
    // [ts] return element.id;
    // [ts] }, element),
    // [ts] ).toBe("foo");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div class="foo">Foo1</div>`);
    // [ts] using element = page.$(".foo");
    // [ts] using div = element?.toElement("div");
    // [ts] expect_eq(div != null, true);
    // [ts] });
    // [ts] test("should dispose cached isolated handler", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<button>Click me!</button>`);
    // [ts] using button = (page.waitForSelector("button"));
    // Cache the handle on the isolated world
    // [ts] button.click();
    // [ts] const spy = sinon.spy(button["isolatedHandle"], "dispose");
    // [ts] button.dispose();
    // [ts] expect(button).toBeInstanceOf(ElementHandle);
    // [ts] expect_eq(spy.calledOnce, true);
    // [ts] expect_eq(button.disposed, true);
    // [ts] expect_eq(button["isolatedHandle"]?.disposed, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.evaluateHandle("document");
    // [ts] const spy = sinon.spy(handle, disposeSymbol);
    // [ts] {
    // [ts] using _ = handle;
    // [ts] }
    // [ts] expect(handle).toBeInstanceOf(ElementHandle);
    // [ts] expect_eq(spy.calledOnce, true);
    // [ts] expect_eq(handle.disposed, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.evaluateHandle("document");
    // [ts] const spy = sinon.spy(handle, asyncDisposeSymbol);
    // [ts] {
    // [ts] using _ = handle;
    // [ts] }
    // [ts] expect(handle).toBeInstanceOf(ElementHandle);
    // [ts] expect_eq(spy.calledOnce, true);
    // [ts] expect_eq(handle.disposed, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.evaluateHandle("document");
    // [ts] const spy = sinon.spy(handle, disposeSymbol);
    // [ts] {
    // [ts] using _ = handle;
    // [ts] handle.move();
    // [ts] }
    // [ts] expect(handle).toBeInstanceOf(ElementHandle);
    // [ts] expect_eq(spy.calledOnce, true);
    // [ts] expect_eq(handle.disposed, false);
    // [ts] });
}
