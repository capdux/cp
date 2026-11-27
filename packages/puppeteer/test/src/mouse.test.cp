// Migrated from TypeScript (src/mouse.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import os from "cap:os";

import { test, expect_eq } from "cap:test";
import { MouseButton } from "puppeteer-core/internal/api/Input.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {KeyInput} from "puppeteer-core/internal/common/USKeyboardLayout.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

struct ClickData {
  type: string = "";
  detail: f64 = 0.0;
  clientX: f64 = 0.0;
  clientY: f64 = 0.0;
  isTrusted: bool = false;
  button: f64 = 0.0;
  buttons: f64 = 0.0;
}

struct Dimensions {
  x: f64 = 0.0;
  y: f64 = 0.0;
  width: f64 = 0.0;
  height: f64 = 0.0;
}

fn dimensions(): Dimensions {
  const rect = document.querySelector("textarea").getBoundingClientRect();
  return {
    x: rect.left,
    y: rect.top,
    width: rect.width,
    height: rect.height,
  };
}

// [cap-test] describe("Mouse", () => {
  setupTestBrowserHooks();

  // @see https://crbug.com/929806
  // Exercises the WebDriver BiDi behavior: BidiMouse keeps no button state and
  // forwards the repeated down() to the browser. CdpMouse deliberately throws
  // instead, so this is expected to fail on CDP (see TestExpectations.json).

  // TODO(cap-migrate): refine TS statement
  // [ts] interface AddMouseDataListenersOptions {
  // [ts] includeMove?: boolean;
  // [ts] }

  // TODO(cap-migrate): refine TS statement
  // [ts] const addMouseDataListeners = (
  // [ts] page: Page,
  // [ts] options: AddMouseDataListenersOptions = {},
  // [ts] ) => {
  // [ts] return page.evaluate((_p) => {
  // [ts] const clicks: []ClickData = [];
  // [ts] const mouseEventListener = (event) => {
  // [ts] clicks.push({
  // [ts] type: event.type,
  // [ts] detail: event.detail,
  // [ts] clientX: event.clientX,
  // [ts] clientY: event.clientY,
  // [ts] isTrusted: event.isTrusted,
  // [ts] button: event.button,
  // [ts] buttons: event.buttons,
  // [ts] });
  // [ts] };
  // [ts] document.addEventListener("mousedown", mouseEventListener);
  // [ts] if (includeMove) {
  // [ts] document.addEventListener("mousemove", mouseEventListener);
  // [ts] }
  // [ts] document.addEventListener("mouseup", mouseEventListener);
  // [ts] document.addEventListener("click", mouseEventListener);
  // [ts] document.addEventListener("auxclick", mouseEventListener);
  // [ts] (window).clicks = clicks;
  // [ts] }, options);
  // [ts] };


  // Exercises the WebDriver BiDi release order: the pressed buttons are
  // released in the reverse order they were pressed and the pointer stays where
  // it is. CdpMouse releases in a fixed order and moves the pointer back to
  // (0, 0), so this is skipped on CDP (see TestExpectations.json).

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should click the document", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).clickPromise = Promise.init(resolve => { document.addEventListener("click", event => { resolve({ type: event.type, detail: event.detail, clientX: event.clientX, clientY: event.clientY, isTrusted: event.isTrusted, button: event.button, }); }); });
    // [ts] });
    // [ts] page.mouse.click(50, 60);
    // [ts] const event = page.evaluate(() => {
    // [ts] return (globalThis).clickPromise;
    // [ts] });
    // [ts] expect_eq(event.type, "click");
    // [ts] expect_eq(event.detail, 1);
    // [ts] expect_eq(event.clientX, 50);
    // [ts] expect_eq(event.clientY, 60);
    // [ts] expect_eq(event.isTrusted, true);
    // [ts] expect_eq(event.button, 0);
    // [ts] });
    // [ts] test("should resize the textarea", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const {x, y, width, height} = page.evaluate(dimensions);
    // [ts] const mouse = page.mouse;
    // [ts] mouse.move(x + width - 4, y + height - 4);
    // [ts] mouse.down();
    // [ts] mouse.move(x + width + 100, y + height + 100);
    // [ts] mouse.up();
    // [ts] const newDimensions = page.evaluate(dimensions);
    // [ts] expect_eq(newDimensions.width, Math.round(width + 104));
    // [ts] expect_eq(newDimensions.height, Math.round(height + 104));
    // [ts] });
    // [ts] test("should select the text with mouse", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const text =
    // [ts] "This is the text that we are going to try to select. Let's see how it goes.";
    // [ts] page.goto(`${server.PREFIX}/input/textarea.html`);
    // [ts] page.evaluate(() => {
    // [ts] const textarea = document.querySelector("textarea");
    // [ts] textarea.style.width = "400px";
    // [ts] textarea.style.height = "150px";
    // [ts] textarea.style.resize = "none";
    // [ts] });
    // [ts] page.focus("textarea");
    // [ts] page.keyboard.type(text);
    // [ts] using handle = page
    // [ts] .locator("textarea")
    // [ts] .filterHandle(async element => {
    // [ts] return element.evaluate((element, text) => {
    // [ts] return element.value == text;
    // [ts] }, text);
    // [ts] })
    // [ts] .waitHandle();
    // [ts] const {x, y, width, height} = page.evaluate(dimensions);
    // [ts] page.mouse.move(x + 2, y + 2);
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(x + width - 5, y + height - 5);
    // [ts] page.mouse.up();
    // [ts] expect(
    // [ts] handle.evaluate(element => {
    // [ts] return element.value.substring(
    // [ts] element.selectionStart,
    // [ts] element.selectionEnd,
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(text);
    // [ts] });
    // [ts] test("should trigger hover state", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.hover("#button-6");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("button:hover").id;
    // [ts] }),
    // [ts] ).toBe("button-6");
    // [ts] page.hover("#button-2");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("button:hover").id;
    // [ts] }),
    // [ts] ).toBe("button-2");
    // [ts] page.hover("#button-91");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("button:hover").id;
    // [ts] }),
    // [ts] ).toBe("button-91");
    // [ts] });
    // [ts] test("should trigger hover state with removed window.Node", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.evaluate(() => {
    // @ts-expect-error Expected.
    // [ts] return delete window.Node;
    // [ts] });
    // [ts] page.hover("#button-6");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("button:hover").id;
    // [ts] }),
    // [ts] ).toBe("button-6");
    // [ts] });
    // [ts] test("should set modifier keys on click", () => {
    // [ts] const {page, server, isFirefox} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("#button-3").addEventListener(
    // [ts] "mousedown",
    // [ts] e => {
    // [ts] return ((globalThis).lastEvent = e);
    // [ts] },
    // [ts] true,
    // [ts] );
    // [ts] });
    // [ts] const modifiers = Map.init([
    // [ts] ["Shift", "shiftKey"],
    // [ts] ["Control", "ctrlKey"],
    // [ts] ["Alt", "altKey"],
    // [ts] ["Meta", "metaKey"],
    // [ts] ]);
    // In Firefox, the Meta modifier only exists on Mac
    // [ts] if (isFirefox && os.platform() != "darwin") {
    // [ts] modifiers.delete("Meta");
    // [ts] }
    // [ts] for (const [modifier, key] of modifiers) {
    // [ts] page.keyboard.down(modifier);
    // [ts] page.click("#button-3");
    // [ts] if (
    // [ts] !(page.evaluate(mod => {
    // [ts] return (globalThis).lastEvent[mod];
    // [ts] }, key))
    // [ts] ) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(key + ' should be true');)
    // [ts] }
    // [ts] page.keyboard.up(modifier);
    // [ts] }
    // [ts] page.click("#button-3");
    // [ts] for (const [modifier, key] of modifiers) {
    // [ts] if (
    // [ts] page.evaluate(mod => {
    // [ts] return (globalThis).lastEvent[mod];
    // [ts] }, key)
    // [ts] ) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(modifiers.get(modifier) + ' should be false');)
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("should send mouse wheel events", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/wheel.html");
    // [ts] using elem = (page.$("div"));
    // [ts] const boundingBoxBefore = (elem.boundingBox());
    // [ts] expect: (boundingBoxBefore) => {
    // [ts] width: 115,
    // [ts] height: 115,
    // [ts] });
    // [ts] page.mouse.move(
    // [ts] boundingBoxBefore.x + boundingBoxBefore.width / 2,
    // [ts] boundingBoxBefore.y + boundingBoxBefore.height / 2,
    // [ts] );
    // [ts] page.mouse.wheel({deltaY: -100});
    // [ts] const boundingBoxAfter = elem.boundingBox();
    // [ts] expect: (boundingBoxAfter) => {
    // [ts] width: 230,
    // [ts] height: 230,
    // [ts] });
    // [ts] });
    // [ts] test("should send mouse wheel events at the position of the last click", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/wheel.html");
    // [ts] using elem = (page.$("div"));
    // [ts] const boundingBoxBefore = (elem.boundingBox());
    // [ts] page.mouse.click(
    // [ts] boundingBoxBefore.x + boundingBoxBefore.width / 2,
    // [ts] boundingBoxBefore.y + boundingBoxBefore.height / 2,
    // [ts] );
    // [ts] page.mouse.wheel({deltaY: -100});
    // [ts] const boundingBoxAfter = elem.boundingBox();
    // [ts] expect: (boundingBoxAfter) => {
    // [ts] width: 230,
    // [ts] height: 230,
    // [ts] });
    // [ts] });
    // [ts] test("should send mouse wheel events at the position of the last of two clicks made in parallel", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const position = page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] window.addEventListener(
    // [ts] "wheel",
    // [ts] event => {
    // [ts] resolve([event.clientX, event.clientY]);
    // [ts] },
    // [ts] {
    // [ts] once: true,
    // [ts] },
    // [ts] );
    // [ts] });
    // [ts] });
    // [ts] joinAll([page.mouse.click(100, 100), page.mouse.click(200, 300)]); // migrated: Promise.all -> joinAll
    // [ts] page.mouse.wheel({deltaY: 100});
    // [ts] expect_eq(position, [200, 300]);
    // [ts] });
    // [ts] test("should send mouse wheel events at the position of a move made in parallel with a click", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).positions = Promise.init(resolve => { const positions: number[][] = []; window.addEventListener("wheel", event => { positions.push([event.clientX, event.clientY]); if (positions.length == 2) { resolve(positions); } }); });
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.mouse.click(100, 100, {delay: 100}),
    // [ts] page.mouse.move(200, 300),
    // [ts] page.mouse.wheel({deltaY: 100}),
    // [ts] ]);
    // [ts] page.mouse.wheel({deltaY: 100});
    // [ts] expect(page.evaluate("positions")).toEqual([
    // [ts] [200, 300],
    // [ts] [200, 300],
    // [ts] ]);
    // [ts] });
    // [ts] test("should send mouse wheel events at the position of a click made in parallel", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).position = Promise.init(resolve => { window.addEventListener( "wheel", event => { resolve([event.clientX, event.clientY]); }, { once: true, }, ); });
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.mouse.click(100, 100),
    // [ts] page.mouse.wheel({deltaY: 100}),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate("position"), [100, 100]);
    // [ts] });
    // [ts] test("should set ctrlKey on the wheel event", () => {
    // [ts] const {page, server, isFirefox} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const ctrlKey = page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] window.addEventListener(
    // [ts] "wheel",
    // [ts] event => {
    // [ts] resolve(event.ctrlKey);
    // [ts] },
    // [ts] {
    // [ts] once: true,
    // [ts] },
    // [ts] );
    // [ts] });
    // [ts] });
    // [ts] page.keyboard.down("Control");
    // [ts] page.mouse.wheel({deltaY: -100});
    // Scroll back to work around
    // https://bugzilla.mozilla.org/show_bug.cgi?id=1901211.
    // [ts] if (isFirefox) {
    // [ts] page.mouse.wheel({deltaY: 100});
    // [ts] }
    // [ts] page.keyboard.up("Control");
    // [ts] expect_eq(ctrlKey, true);
    // [ts] });
    // [ts] test("should tween mouse movement", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.mouse.move(100, 100);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).result = [];
    // [ts] document.addEventListener("mousemove", event => {
    // [ts] (globalThis).result.push([event.clientX, event.clientY]);
    // [ts] });
    // [ts] });
    // [ts] page.mouse.move(200, 300, {steps: 5});
    // [ts] expect(page.evaluate("result")).toEqual([
    // [ts] [120, 140],
    // [ts] [140, 180],
    // [ts] [160, 220],
    // [ts] [180, 260],
    // [ts] [200, 300],
    // [ts] ]);
    // [ts] });
    // [ts] test("should tween mouse movement from the position of the last click", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.mouse.click(100, 100);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).result = [];
    // [ts] document.addEventListener("mousemove", event => {
    // [ts] (globalThis).result.push([event.clientX, event.clientY]);
    // [ts] });
    // [ts] });
    // [ts] page.mouse.move(200, 300, {steps: 5});
    // [ts] expect(page.evaluate("result")).toEqual([
    // [ts] [120, 140],
    // [ts] [140, 180],
    // [ts] [160, 220],
    // [ts] [180, 260],
    // [ts] [200, 300],
    // [ts] ]);
    // [ts] });
    // [ts] test("should tween mouse movement from the position of a click made in parallel", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).result = [];
    // [ts] document.addEventListener("mousemove", event => {
    // [ts] (globalThis).result.push([event.clientX, event.clientY]);
    // [ts] });
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.mouse.click(100, 100),
    // [ts] page.mouse.move(200, 300, {steps: 5}),
    // [ts] ]);
    // [ts] expect(page.evaluate("result")).toEqual([
    // [ts] [100, 100],
    // [ts] [120, 140],
    // [ts] [140, 180],
    // [ts] [160, 220],
    // [ts] [180, 260],
    // [ts] [200, 300],
    // [ts] ]);
    // [ts] });
    // [ts] test("should move after a move made in parallel with a click outside of the viewport", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.mouse.move(100, 100);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.mouse.click(5000, 5000).catch(() => {}),
    // [ts] page.mouse.move(200, 300),
    // [ts] ]);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).result = [];
    // [ts] document.addEventListener("mousemove", event => {
    // [ts] (globalThis).result.push([event.clientX, event.clientY]);
    // [ts] });
    // [ts] });
    // [ts] page.mouse.move(300, 500, {steps: 2});
    // [ts] expect(page.evaluate("result")).toEqual([
    // [ts] [250, 400],
    // [ts] [300, 500],
    // [ts] ]);
    // [ts] });
    // [ts] test("should move after clicks outside of the viewport made in parallel", () => {
    // [ts] const {page, isFirefox} = getTestState();
    // [ts] page.mouse.move(100, 100);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.mouse.click(5000, 5000).catch(() => {}),
    // [ts] page.mouse.click(6000, 6000).catch(() => {}),
    // [ts] ]);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).result = [];
    // [ts] document.addEventListener("mousemove", event => {
    // [ts] (globalThis).result.push([event.clientX, event.clientY]);
    // [ts] });
    // [ts] });
    // [ts] page.mouse.move(200, 300, {steps: 2});
    // [ts] expect(page.evaluate("result")).toEqual(
    // [ts] isFirefox ? [
    // [ts] [150, 200],
    // [ts] [200, 300],
    // [ts] ] : [[200, 300]],
    // [ts] );
    // [ts] });
    // [ts] test("should move from the last position after a move outside of the viewport", () => {
    // [ts] const {page, isFirefox} = getTestState();
    // [ts] page.mouse.move(100, 100);
    // [ts] page.mouse.move(5000, 5000).catch(() => {});
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).result = [];
    // [ts] document.addEventListener("mousemove", event => {
    // [ts] (globalThis).result.push([event.clientX, event.clientY]);
    // [ts] });
    // [ts] });
    // [ts] page.mouse.move(200, 300, {steps: 2});
    // [ts] expect(page.evaluate("result")).toEqual(
    // [ts] isFirefox ? [
    // [ts] [150, 200],
    // [ts] [200, 300],
    // [ts] ] : [[200, 300]],
    // [ts] );
    // [ts] });
    // [ts] test("should work with mobile viewports and cross process navigations", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setViewport({width: 360, height: 640, isMobile: true});
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/mobile.html");
    // [ts] page.evaluate(() => {
    // [ts] document.addEventListener("click", event => {
    // [ts] (globalThis).result = {x: event.clientX, y: event.clientY};
    // [ts] });
    // [ts] });
    // [ts] page.mouse.click(30, 40);
    // [ts] expect_eq(page.evaluate("result"), {x: 30, y: 40});
    // [ts] });
    // [ts] test("should not throw if buttons are pressed twice", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.mouse.down();
    // [ts] page.mouse.down();
    // [ts] });
    // [ts] test("should not throw if clicking in parallel", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] addMouseDataListeners(page);
    // [ts] joinAll([page.mouse.click(0, 5), page.mouse.click(6, 10)]); // migrated: Promise.all -> joinAll
    // [ts] const data = page.evaluate(() => {
    // [ts] return (window).clicks;
    // [ts] });
    // [ts] const commonAttrs = {
    // [ts] isTrusted: true,
    // [ts] detail: 1,
    // [ts] clientY: 5,
    // [ts] clientX: 0,
    // [ts] button: 0,
    // [ts] };
    // [ts] expect: (data.splice(0, 3)) => {
    // [ts] "0": {
    // [ts] type: "mousedown",
    // [ts] buttons: 1,
    // [ts] ...commonAttrs,
    // [ts] },
    // [ts] "1": {
    // [ts] type: "mouseup",
    // [ts] buttons: 0,
    // [ts] ...commonAttrs,
    // [ts] },
    // [ts] "2": {
    // [ts] type: "click",
    // [ts] buttons: 0,
    // [ts] ...commonAttrs,
    // [ts] },
    // [ts] });
    // [ts] Object.assign(commonAttrs, {
    // [ts] clientX: 6,
    // [ts] clientY: 10,
    // [ts] });
    // [ts] expect: (data) => {
    // [ts] "0": {
    // [ts] type: "mousedown",
    // [ts] buttons: 1,
    // [ts] ...commonAttrs,
    // [ts] },
    // [ts] "1": {
    // [ts] type: "mouseup",
    // [ts] buttons: 0,
    // [ts] ...commonAttrs,
    // [ts] },
    // [ts] "2": {
    // [ts] type: "click",
    // [ts] buttons: 0,
    // [ts] ...commonAttrs,
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] test("should reset properly", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.mouse.move(5, 5);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.mouse.down({button: MouseButton.Left}),
    // [ts] page.mouse.down({button: MouseButton.Middle}),
    // [ts] page.mouse.down({button: MouseButton.Right}),
    // [ts] ]);
    // [ts] addMouseDataListeners(page, {includeMove: true});
    // [ts] page.mouse.reset();
    // [ts] const data = page.evaluate(() => {
    // [ts] return (window).clicks;
    // [ts] });
    // [ts] const commonAttrs = {
    // [ts] isTrusted: true,
    // [ts] clientY: 5,
    // [ts] clientX: 5,
    // [ts] };
    // [ts] expect(data.slice(0, 2)).toMatchObject([
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 2,
    // [ts] buttons: 5,
    // [ts] detail: 1,
    // [ts] type: "mouseup",
    // [ts] },
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 2,
    // [ts] buttons: 5,
    // [ts] detail: 1,
    // [ts] type: "auxclick",
    // [ts] },
    // [ts] ]);
    // TODO(crbug/1485040): This should align with the firefox implementation.
    // [ts] if (isChrome) {
    // [ts] expect(data.slice(2)).toMatchObject([
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 1,
    // [ts] buttons: 1,
    // [ts] detail: 0,
    // [ts] type: "mouseup",
    // [ts] },
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 0,
    // [ts] buttons: 0,
    // [ts] detail: 0,
    // [ts] type: "mouseup",
    // [ts] },
    // [ts] ]);
    // [ts] return;
    // [ts] }
    // [ts] expect(data.slice(2)).toMatchObject([
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 1,
    // [ts] buttons: 1,
    // [ts] detail: 1,
    // [ts] type: "mouseup",
    // [ts] },
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 1,
    // [ts] buttons: 1,
    // [ts] detail: 1,
    // [ts] type: "auxclick",
    // [ts] },
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 0,
    // [ts] buttons: 0,
    // [ts] detail: 1,
    // [ts] type: "mouseup",
    // [ts] },
    // [ts] {
    // [ts] ...commonAttrs,
    // [ts] button: 0,
    // [ts] buttons: 0,
    // [ts] detail: 1,
    // [ts] type: "click",
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should evaluate before mouse event", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/input/button.html");
    // [ts] using button = page.waitForSelector("button");
    // [ts] const point = button.clickablePoint();
    // [ts] const result = page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] document
    // [ts] .querySelector("button")
    // [ts] ?.addEventListener("click", resolve, {once: true});
    // [ts] });
    // [ts] });
    // [ts] page.mouse.click(point?.x, point?.y);
    // [ts] result;
    // [ts] });
}
