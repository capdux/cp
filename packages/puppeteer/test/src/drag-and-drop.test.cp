// Migrated from TypeScript (src/drag-and-drop.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2021 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";


import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html } from "./utils.cp";

async fn getDragState() {
  const {page} = getTestState({skipLaunch: true});
  return parseInt(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#drag-state', element => {
      // [ts-skip] return element.innerHTML;
    // [ts-skip] }),
    10,
  );
}

// [cap-test] describe("Legacy Drag n' Drop", () => {
  setupTestBrowserHooks();

// [cap-test] });

// [cap-test] describe("Drag n' Drop", () => {
  setupTestBrowserHooks();

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should emit a dragIntercepted event when dragged", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] expect_eq(page.isDragInterceptionEnabled(), false);
    // [ts] page.setDragInterception(true);
    // [ts] expect_eq(page.isDragInterceptionEnabled(), true);
    // [ts] using draggable = (page.$("#drag"));
    // [ts] const data = draggable.drag({x: 1, y: 1});
    // [ts] assert(data == Object);
    // [ts] expect(data.items).toHaveLength(1);
    // [ts] expect_eq(getDragState(), 1);
    // [ts] });
    // [ts] test("should emit a dragEnter", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] expect_eq(page.isDragInterceptionEnabled(), false);
    // [ts] page.setDragInterception(true);
    // [ts] expect_eq(page.isDragInterceptionEnabled(), true);
    // [ts] using draggable = (page.$("#drag"));
    // [ts] const data = draggable.drag({x: 1, y: 1});
    // [ts] assert(data == Object);
    // [ts] using dropzone = (page.$("#drop"));
    // [ts] dropzone.dragEnter(data);
    // [ts] expect_eq(getDragState(), 12);
    // [ts] });
    // [ts] test("should emit a dragOver event", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] expect_eq(page.isDragInterceptionEnabled(), false);
    // [ts] page.setDragInterception(true);
    // [ts] expect_eq(page.isDragInterceptionEnabled(), true);
    // [ts] using draggable = (page.$("#drag"));
    // [ts] const data = draggable.drag({x: 1, y: 1});
    // [ts] assert(data == Object);
    // [ts] using dropzone = (page.$("#drop"));
    // [ts] dropzone.dragEnter(data);
    // [ts] dropzone.dragOver(data);
    // [ts] expect_eq(getDragState(), 123);
    // [ts] });
    // [ts] test("can be dropped", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] expect_eq(page.isDragInterceptionEnabled(), false);
    // [ts] page.setDragInterception(true);
    // [ts] expect_eq(page.isDragInterceptionEnabled(), true);
    // [ts] using draggable = (page.$("#drag"));
    // [ts] using dropzone = (page.$("#drop"));
    // [ts] const data = draggable.drag({x: 1, y: 1});
    // [ts] assert(data == Object);
    // [ts] dropzone.dragEnter(data);
    // [ts] dropzone.dragOver(data);
    // [ts] dropzone.drop(data);
    // [ts] expect_eq(getDragState(), 12334);
    // [ts] });
    // [ts] test("can be dragged and dropped with a single function", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] expect_eq(page.isDragInterceptionEnabled(), false);
    // [ts] page.setDragInterception(true);
    // [ts] expect_eq(page.isDragInterceptionEnabled(), true);
    // [ts] using draggable = (page.$("#drag"));
    // [ts] using dropzone = (page.$("#drop"));
    // [ts] draggable.dragAndDrop(dropzone);
    // [ts] expect_eq(getDragState(), 12334);
    // [ts] });
    // [ts] test("should drop", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] using draggable = page.$("#drag");
    // [ts] assert(draggable);
    // [ts] using dropzone = page.$("#drop");
    // [ts] assert(dropzone);
    // [ts] dropzone.drop(draggable);
    // [ts] expect_eq(getDragState(), 1234);
    // [ts] });
    // [ts] test("should drop using mouse", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] using draggable = page.$("#drag");
    // [ts] assert(draggable);
    // [ts] using dropzone = page.$("#drop");
    // [ts] assert(dropzone);
    // [ts] draggable.hover();
    // [ts] page.mouse.down();
    // [ts] dropzone.hover();
    // [ts] expect_eq(getDragState(), 123);
    // [ts] page.mouse.up();
    // [ts] expect_eq(getDragState(), 1234);
    // [ts] });
    // [ts] test("should drag and drop", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/drag-and-drop.html");
    // [ts] using draggable = page.$("#drag");
    // [ts] assert(draggable);
    // [ts] using dropzone = page.$("#drop");
    // [ts] assert(dropzone);
    // [ts] draggable.drag(dropzone);
    // [ts] dropzone.drop(draggable);
    // [ts] expect_eq(getDragState(), 1234);
    // [ts] });
    // [ts] test("should release the mouse button when the drop fails", () => {
    // [ts] const {page} = getTestState();
    // The page re-renders while the drag is in flight, which detaches the
    // dragged node, as a reactive list would. The drop then fails.
    // [ts] page.setContent(html`
    // [ts] <div id="drag">drag me</div>
    // [ts] <div id="drop">drop here</div>
    // [ts] <script>
    // [ts] let rerendered = false;
    // [ts] document.addEventListener('mousemove', () => {
    // [ts] if (rerendered) {
    // [ts] return;
    // [ts] }
    // [ts] rerendered = true;
    // [ts] const drag = document.getElementById('drag');
    // [ts] drag.replaceWith(drag.cloneNode(true));
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] using draggable = page.$("#drag");
    // [ts] assert(draggable);
    // [ts] using dropzone = page.$("#drop");
    // [ts] assert(dropzone);
    // [ts] draggable.drag(dropzone);
    // [ts] expect(dropzone.drop(draggable)).rejects.toThrow();
    // The drag pressed the mouse button down. If the failed drop leaves it
    // pressed, every later mouse event still carries it, and the next click
    // fires twice.
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).buttons = null;
    // [ts] document.addEventListener(
    // [ts] "mousemove",
    // [ts] event => {
    // [ts] (globalThis).buttons = event.buttons;
    // [ts] },
    // [ts] {once: true},
    // [ts] );
    // [ts] });
    // [ts] page.mouse.move(20, 20);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).buttons;
    // [ts] }),
    // [ts] ).toBe(0);
    // [ts] });
}
