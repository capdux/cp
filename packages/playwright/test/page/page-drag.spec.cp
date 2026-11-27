// Migrated from TypeScript (page/page-drag.spec.ts) via cap from -ts

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

// [cap-migrate] import { ElementHandle, Page, Route } from "playwright";
import { test as it, expect } from "./pageTest.cp";
import { attachFrame } from "../config/utils.cp";


// TODO(cap-migrate): refine TS statement
// [ts] it.describe("Drag and drop", () => {
// [ts] test("DD-001 should work @smoke", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] page.mouse.up();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
// [ts] });
// [ts] test("DD-002 should send the right events", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] const events = trackEvents(page.$("body"));
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] page.mouse.up();
// [ts] expect(events.jsonValue()).toEqual([
// [ts] "mousemove at 120;86",
// [ts] "mousedown at 120;86",
// [ts] if (browserName == "firefox") "dragstart at 120;86" else "mousemove at 240;350",
// [ts] if (browserName == "firefox") "mousemove at 240;350" else "dragstart at 120;86",
// [ts] "dragenter at 240;350",
// [ts] "dragover at 240;350",
// [ts] "drop at 240;350",
// [ts] "dragend",
// [ts] ]);
// [ts] });
// [ts] test("DD-003 should not send dragover on the first mousemove", () => {
// [ts] it.fixme(browserName != "chromium");
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] const events = trackEvents(page.$("body"));
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] expect(events.jsonValue()).toEqual([
// [ts] "mousemove at 120;86",
// [ts] "mousedown at 120;86",
// [ts] if (browserName == "firefox") "dragstart at 120;86" else "mousemove at 240;350",
// [ts] if (browserName == "firefox") "mousemove at 240;350" else "dragstart at 120;86",
// [ts] "dragenter at 240;350",
// [ts] ]);
// [ts] });
// [ts] test("DD-004 should work inside iframe", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] const frame = attachFrame(page, "myframe", server.PREFIX + "/drag-n-drop.html");
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', iframe => {
// [ts-skip] iframe.style.width = '500px';
// [ts-skip] iframe.style.height = '600px';
// [ts-skip] iframe.style.marginLeft = '80px';
// [ts-skip] iframe.style.marginTop = '60px';
// [ts-skip] });
// [ts] const pageEvents = trackEvents(page.$("body"));
// [ts] const frameEvents = trackEvents(frame.$("body"));
// [ts] frame.hover("#source");
// [ts] page.mouse.down();
// [ts] frame.hover("#target");
// [ts] page.mouse.up();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await frame.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
// [ts] expect(frameEvents.jsonValue()).toEqual([
// [ts] "mousemove at 120;86",
// [ts] "mousedown at 120;86",
// [ts] if (browserName == "firefox") "dragstart at 120;86" else "mousemove at 240;350",
// [ts] if (browserName == "firefox") "mousemove at 240;350" else "dragstart at 120;86",
// [ts] "dragenter at 240;350",
// [ts] "dragover at 240;350",
// [ts] "drop at 240;350",
// [ts] "dragend",
// [ts] ]);
// [ts] expect_eq(pageEvents.jsonValue(), []);
// [ts] });
// [ts] test("DD-005 should cancel on escape", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] const events = trackEvents(page.$("body"));
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] page.keyboard.press("Escape");
// [ts] page.mouse.up();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(false); // found source in target
// [ts] expect(events.jsonValue()).toEqual([
// [ts] "mousemove at 120;86",
// [ts] "mousedown at 120;86",
// [ts] if (browserName == "firefox") "dragstart at 120;86" else "mousemove at 240;350",
// [ts] if (browserName == "firefox") "mousemove at 240;350" else "dragstart at 120;86",
// [ts] "dragenter at 240;350",
// [ts] if (browserName == "chromium") null else "dragover at 240;350",
// [ts] "dragend",
// [ts] "mouseup at 240;350",
// [ts] ].filter(Boolean));
// [ts] });
// [ts] it.describe("iframe", () => {
// [ts] it.fixme(true, "implement dragging with iframes");
// [ts] test("DD-006 should drag into an iframe", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] const frame = attachFrame(page, "oopif", server.PREFIX + "/drag-n-drop.html");
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', iframe => {
// [ts-skip] iframe.style.width = '500px';
// [ts-skip] iframe.style.height = '600px';
// [ts-skip] iframe.style.marginLeft = '500px';
// [ts-skip] iframe.style.marginTop = '60px';
// [ts-skip] });
// [ts] const pageEvents = trackEvents(page.$("body"));
// [ts] const frameEvents = trackEvents(frame.$("body"));
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] frame.hover("#target");
// [ts] page.mouse.up();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await frame.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
// [ts] expect(pageEvents.jsonValue()).toEqual([
// [ts] "mousemove",
// [ts] "mousedown",
// [ts] if (browserName == "firefox") "dragstart" else "mousemove",
// [ts] if (browserName == "firefox") "mousemove" else "dragstart",
// [ts] ]);
// [ts] expect(frameEvents.jsonValue()).toEqual([
// [ts] "dragenter",
// [ts] "dragover",
// [ts] "drop",
// [ts] ]);
// [ts] });
// [ts] test("DD-007 should drag out of an iframe", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] const frame = attachFrame(page, "oopif", server.PREFIX + "/drag-n-drop.html");
// [ts] const pageEvents = trackEvents(page.$("body"));
// [ts] const frameEvents = trackEvents(frame.$("body"));
// [ts] frame.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] page.mouse.up();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
// [ts] expect(frameEvents.jsonValue()).toEqual([
// [ts] "mousemove",
// [ts] "mousedown",
// [ts] "dragstart",
// [ts] "dragend",
// [ts] ]);
// [ts] expect(pageEvents.jsonValue()).toEqual([
// [ts] "dragenter",
// [ts] "dragover",
// [ts] "drop",
// [ts] ]);
// [ts] });
// [ts] });
// [ts] test("DD-008 should respect the drop effect", () => {
// [ts] it.fixme(browserName == "webkit" && isLinux && !headless, "https://github.com/microsoft/playwright/issues/21646");
// [ts] it.fixme(browserName == "chromium" && !isMac && !headless, "https://github.com/microsoft/playwright/issues/21646");
// [ts] it.slow(trace == "on");
// [ts] expect_eq(testIfDropped("copy", "copy"), true);
// [ts] expect_eq(testIfDropped("copy", "move"), false);
// [ts] expect_eq(testIfDropped("all", "link"), true);
// [ts] expect_eq(testIfDropped("all", "none"), false);
// [ts] expect_eq(testIfDropped("copyMove", "copy"), true);
// [ts] expect_eq(testIfDropped("copyLink", "copy"), true);
// [ts] expect_eq(testIfDropped("linkMove", "copy"), false);
// [ts] expect_eq(testIfDropped("copyMove", "link"), false);
// [ts] expect_eq(testIfDropped("copyLink", "link"), true);
// [ts] expect_eq(testIfDropped("linkMove", "link"), true);
// [ts] expect_eq(testIfDropped("copyMove", "move"), true);
// [ts] expect_eq(testIfDropped("copyLink", "move"), false);
// [ts] expect_eq(testIfDropped("linkMove", "move"), true);
// [ts] expect_eq(testIfDropped("uninitialized", "copy"), true);
// [ts] async fn testIfDropped(effectAllowed: string, dropEffect: string) {
// [ts] page.setContent(`
// [ts] <div draggable="true">drag target</div>
// [ts] <drop-target>this is the drop target</drop-target>
// [ts] `);
// [ts] page.evaluate((_p) => {
// [ts] window["dropped"] = false;
// [ts] document.querySelector("div").addEventListener("dragstart", event => {
// [ts] event.dataTransfer.effectAllowed = effectAllowed;
// [ts] event.dataTransfer.setData("text/plain", "drag data");
// [ts] });
// [ts] const dropTarget: HTMLElement = document.querySelector("drop-target");
// [ts] dropTarget.addEventListener("dragover", event => {
// [ts] event.dataTransfer.dropEffect = dropEffect;
// [ts] event.preventDefault();
// [ts] });
// [ts] dropTarget.addEventListener("drop", event => {
// [ts] window["dropped"] = true;
// [ts] });
// [ts] }, { effectAllowed, dropEffect });
// [ts] page.hover("div");
// [ts] page.mouse.down();
// [ts] page.hover("drop-target");
// [ts] page.mouse.up();
// [ts] return page.evaluate("dropped");
// [ts] }
// [ts] });
// [ts] test("DD-028 should report the default drop effect from effectAllowed", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43447" },
// [ts] }, (_p) => {
// [ts] it.fixme(browserName == "webkit" && isLinux && !headless, "https://github.com/microsoft/playwright/issues/21646");
// [ts] page.setContent(`
// [ts] <div draggable="true">drag target</div>
// [ts] <drop-target>this is the drop target</drop-target>
// [ts] `);
// [ts] page.evaluate(() => {
// [ts] window["dropEffect"] = "none";
// [ts] document.querySelector("div").addEventListener("dragstart", event => {
// [ts] event.dataTransfer.effectAllowed = "copyMove";
// [ts] event.dataTransfer.setData("text/plain", "drag data");
// [ts] });
// [ts] document.querySelector("div").addEventListener("dragend", event => {
// [ts] window["dropEffect"] = event.dataTransfer.dropEffect;
// [ts] });
// The target accepts the drag without setting dropEffect.
// [ts] document.querySelector("drop-target").addEventListener("dragover", event => {
// [ts] event.preventDefault();
// [ts] });
// [ts] });
// [ts] page.hover("div");
// [ts] page.mouse.down();
// [ts] page.hover("drop-target");
// [ts] page.mouse.up();
// [ts] expect_eq(page.evaluate("dropEffect"), "move");
// [ts] });
// [ts] test("DD-009 should work if the drag is canceled", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] page.evaluate(() => {
// [ts] document.body.addEventListener("dragstart", event => {
// [ts] event.preventDefault();
// [ts] }, false);
// [ts] });
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] page.mouse.up();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(false);
// [ts] });
// [ts] test("DD-010 should work if the drag event is captured but not canceled", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] page.evaluate(() => {
// [ts] document.body.addEventListener("dragstart", event => {
// [ts] event.stopImmediatePropagation();
// [ts] }, false);
// [ts] });
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] page.mouse.up();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true);
// [ts] });
// [ts] test("DD-011 should be able to drag the mouse in a frame", () => {
// [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
// [ts] const eventsHandle = trackEvents(page.frames()[1].$("html"));
// [ts] page.mouse.move(30, 30);
// [ts] page.mouse.down();
// [ts] page.mouse.move(60, 60);
// [ts] page.mouse.up();
// [ts] expect_eq(eventsHandle.jsonValue(), ["mousemove at 20;20", "mousedown at 20;20", "mousemove at 50;50", "mouseup at 50;50"]);
// [ts] });
// [ts] test("DD-012 should work if a frame is stalled", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] let madeRequest;
// [ts] const routePromise = Promise.init(x => madeRequest = x);
// [ts] page.route("**/empty.html", (route, request) => {
// [ts] madeRequest(route);
// [ts] });
// [ts] attachFrame(page, "frame", server.EMPTY_PAGE).catch(() => {});
// [ts] const route = routePromise;
// [ts] page.hover("#source");
// [ts] page.mouse.down();
// [ts] page.hover("#target");
// [ts] page.mouse.up();
// [ts] route.abort();
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
// [ts] });
// [ts] test("DD-013 should work with the helper method", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] page.dragAndDrop("#source", "#target");
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
// [ts] });
// [ts] [{
// [ts] title: "dragAndDrop",
// [ts] drag: (page, steps) => page.dragAndDrop("#red", "#blue", { steps }),
// [ts] }, {
// [ts] title: "dragTo",
// [ts] drag: (page, steps) => page.locator("#red").dragTo(page.locator("#blue"), { steps }),
// [ts] }].forEach((_p) => {
// [ts] test(`DD-014 should ${title} with tweened mouse movement`, () => {
// [ts] test(!headless, "actual mouse interferes with the exact mousemove events");
// [ts] page.setContent(`
// [ts] <body style="margin: 0; padding: 0;">
// [ts] <div style="width:100px;height:100px;background:red;" id="red"></div>
// [ts] <div style="width:300px;height:100px;background:blue;" id="blue"></div>
// [ts] </body>
// [ts] `);
// [ts] const eventsHandle = page.evaluateHandle(() => {
// [ts] const events = [];
// [ts] document.addEventListener("mousedown", event => {
// [ts] events.push({
// [ts] type: "mousedown",
// [ts] x: event.pageX,
// [ts] y: event.pageY,
// [ts] });
// [ts] });
// [ts] document.addEventListener("mouseup", event => {
// [ts] events.push({
// [ts] type: "mouseup",
// [ts] x: event.pageX,
// [ts] y: event.pageY,
// [ts] });
// [ts] });
// [ts] document.addEventListener("mousemove", event => {
// [ts] events.push({
// [ts] type: "mousemove",
// [ts] x: event.pageX,
// [ts] y: event.pageY,
// [ts] });
// [ts] });
// [ts] return events;
// [ts] });
// [ts] drag(page, 4);
// [ts] expect.poll(() => eventsHandle.jsonValue()).toEqual([
// [ts] { type: "mousemove", x: 50, y: 50 },
// [ts] { type: "mousedown", x: 50, y: 50 },
// [ts] { type: "mousemove", x: 75, y: 75 },
// [ts] { type: "mousemove", x: 100, y: 100 },
// [ts] { type: "mousemove", x: 125, y: 125 },
// [ts] { type: "mousemove", x: 150, y: 150 },
// [ts] { type: "mouseup", x: 150, y: 150 },
// [ts] ]);
// [ts] });
// [ts] });
// [ts] test("DD-015 should allow specifying the position", () => {
// [ts] page.setContent(`
// [ts] <div style="width:100px;height:100px;background:red;" id="red">
// [ts] </div>
// [ts] <div style="width:100px;height:100px;background:blue;" id="blue">
// [ts] </div>
// [ts] `);
// [ts] const eventsHandle = page.evaluateHandle(() => {
// [ts] const events = [];
// [ts] document.getElementById("red").addEventListener("mousedown", event => {
// [ts] events.push({
// [ts] type: "mousedown",
// [ts] x: event.offsetX,
// [ts] y: event.offsetY,
// [ts] });
// [ts] });
// [ts] document.getElementById("blue").addEventListener("mouseup", event => {
// [ts] events.push({
// [ts] type: "mouseup",
// [ts] x: event.offsetX,
// [ts] y: event.offsetY,
// [ts] });
// [ts] });
// [ts] return events;
// [ts] });
// [ts] page.dragAndDrop("#red", "#blue", {
// [ts] sourcePosition: { x: 34, y: 7 },
// [ts] targetPosition: { x: 10, y: 20 },
// [ts] });
// [ts] expect(eventsHandle.jsonValue()).toEqual([
// [ts] { type: "mousedown", x: 34, y: 7 },
// [ts] { type: "mouseup", x: 10, y: 20 },
// [ts] ]);
// [ts] });
// [ts] test("DD-016 should work with locators", () => {
// [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
// [ts] page.locator("#source").dragTo(page.locator("#target"));
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', target => target.contains(document.querySelector('#source')))).toBe(true); // could not find source in target
// [ts] });
// [ts] });



async fn trackEvents(target: ElementHandle) {
    // TODO(cap-migrate): refine TS body
    // [ts] const eventsHandle = target.evaluateHandle(target => {
    // [ts] const events: []string = [];
    // [ts] for (const event of [
    // [ts] "mousedown", "mousemove", "mouseup",
    // [ts] "dragstart", "dragend", "dragover", "dragenter", "dragleave", "dragexit",
    // [ts] "drop"
    // [ts] ]) {
    // [ts] target.addEventListener(event, e => {
    // [ts] const pe = e;
    // Browsers are all over the place with dragend position.
    // [ts] if (event == "dragend")
    // [ts] events.push("dragend");
    // [ts] else {
    // [ts] events.push(`${event} at ${pe.clientX};${pe.clientY}`);
    // [ts] }, false);
    // [ts] }
    // [ts] return events;
    // [ts] });
    // [ts] return eventsHandle;
    // [ts] }
    // [ts] test("DD-019 should handle custom dataTransfer", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/18013" });
    // [ts] it.fixme(browserName == "webkit" && isWindows);
    // [ts] page.setContent(`<button draggable="true">Draggable</button>`);
    // [ts] page.evaluate(() => {
    // [ts] (window).__result = Promise.init(resolve => { document.addEventListener("dragstart", event => { event.dataTransfer.setData("custom-type", "Hello World"); }, false);  document.addEventListener("dragenter", event => { event.preventDefault(); }, false); document.addEventListener("dragover", event => { event.preventDefault(); }, false);  document.addEventListener("drop", event => { event.preventDefault(); resolve({ types: event.dataTransfer.types, data: event.dataTransfer.getData("custom-type"), }); }, false); });
    // [ts] });
    // [ts] page.hover("[draggable=\"true\"]");
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(100, 100);
    // [ts] page.mouse.up();
    // [ts] expect: (page.evaluate(() => (window as any).__result)) => {
    // [ts] types: ["custom-type"],
    // [ts] data: "Hello World",
    // [ts] });
    // [ts] });
    // [ts] test("DD-020 what happens when dragging element is destroyed", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21621" });
    // [ts] page.setContent(`
    // [ts] <button draggable="true">Draggable</button>
    // [ts] <div id=target>drop here</div>
    // [ts] `);
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("#target").addEventListener("dragover", event => {
    // [ts] document.querySelector("button")?.remove();
    // [ts] }, false);
    // [ts] document.querySelector("#target").addEventListener("drop", event => {
    // [ts] document.querySelector("#target").textContent = "dropped";
    // [ts] }, false);
    // [ts] });
    // [ts] page.locator("button").dragTo(page.locator("div"));
    // [ts] expect(page.locator("div")).toHaveText("drop here");
    // [ts] });
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => browserName == "chromium" && browserMajorVersion < 91);
    // [ts] test((_p) => isAndroid, "No drag&drop on Android.");
    // [ts] test((_p) => !headless, "Stray mouse events mess up the tests.");
    // [ts] test("DD-017 should work if not doing a drag", () => {
    // [ts] const eventsHandle = trackEvents(page.$("html"));
    // [ts] page.mouse.move(50, 50);
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(100, 100);
    // [ts] page.mouse.up();
    // [ts] expect_eq(eventsHandle.jsonValue(), ["mousemove at 50;50", "mousedown at 50;50", "mousemove at 100;100", "mouseup at 100;100"]);
    // [ts] });
    // [ts] test("DD-018 should report event.buttons", () => {
    // [ts] const logsHandle = page.evaluateHandle(() => {
    // [ts] const div = document.createElement("div");
    // [ts] document.body.appendChild(div);
    // [ts] div.style.width = "200px";
    // [ts] div.style.height = "200px";
    // [ts] div.style.backgroundColor = "blue";
    // [ts] div.addEventListener("mousedown", onEvent);
    // [ts] div.addEventListener("mousemove", onEvent, { passive: false });
    // [ts] div.addEventListener("mouseup", onEvent);
    // [ts] const logs = [];
    // [ts] fn onEvent(event: auto) {
    // [ts] logs.push({ type: event.type, buttons: event.buttons });
    // [ts] }
    // [ts] Promise.init(window.builtins.requestAnimationFrame);
    // [ts] return logs;
    // [ts] });
    // [ts] page.mouse.move(20, 20);
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(40, 40);
    // [ts] page.mouse.up();
    // [ts] const logs = logsHandle.jsonValue();
    // [ts] expect(logs).toEqual([
    // [ts] { type: "mousemove", buttons: 0 },
    // [ts] { type: "mousedown", buttons: 1 },
    // [ts] { type: "mousemove", buttons: 1 },
    // [ts] { type: "mouseup", buttons: 0 },
    // [ts] ]);
    // [ts] });
}
