// Migrated from TypeScript (page/page-dispatchevent.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { test as it, expect } from "./pageTest.cp";
















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("DE-001 should dispatch click event @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.dispatchEvent("button", "click");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("DE-002 should dispatch click event properties", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.dispatchEvent("button", "click");
    // [ts] expect_eq(page.evaluate("bubbles"), true);
    // [ts] expect_eq(page.evaluate("cancelable"), true);
    // [ts] expect_eq(page.evaluate("composed"), true);
    // [ts] });
    // [ts] test("DE-003 should dispatch click svg", () => {
    // [ts] page.setContent(`
    // [ts] <svg height="100" width="100">
    // [ts] <circle onclick="javascript:window.__CLICKED=42" cx="50" cy="50" r="40" stroke="black" stroke-width="3" fill="red" />
    // [ts] </svg>
    // [ts] `);
    // [ts] page.dispatchEvent("circle", "click");
    // [ts] expect_eq(page.evaluate(() => window["__CLICKED"]), 42);
    // [ts] });
    // [ts] test("DE-004 should dispatch click on a span with an inline element inside", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] span::before {
    // [ts] content: 'q';
    // [ts] }
    // [ts] </style>
    // [ts] <span onclick='javascript:window.CLICKED=42'></span>
    // [ts] `);
    // [ts] page.dispatchEvent("span", "click");
    // [ts] expect_eq(page.evaluate(() => window["CLICKED"]), 42);
    // [ts] });
    // [ts] test("DE-005 should dispatch click after navigation ", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.dispatchEvent("button", "click");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.dispatchEvent("button", "click");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("DE-006 should dispatch click after a cross origin navigation ", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.dispatchEvent("button", "click");
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/input/button.html");
    // [ts] page.dispatchEvent("button", "click");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("DE-007 should not fail when element is blocked on hover", () => {
    // [ts] page.setContent(`<style>
    // [ts] container { display: block; position: relative; width: 200px; height: 50px; }
    // [ts] div, button { position: absolute; left: 0; top: 0; bottom: 0; right: 0; }
    // [ts] div { pointer-events: none; }
    // [ts] container:hover div { pointer-events: auto; background: red; }
    // [ts] </style>
    // [ts] <container>
    // [ts] <button onclick="window.clicked=true">Click me</button>
    // [ts] <div></div>
    // [ts] </container>`);
    // [ts] page.dispatchEvent("button", "click");
    // [ts] expect_eq(page.evaluate(() => window["clicked"]), true);
    // [ts] });
    // [ts] test("DE-008 should dispatch click when node is added in shadow dom", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watchdog = page.dispatchEvent("span", "click");
    // [ts] page.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] div.attachShadow({ mode: "open" });
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] page.waitForTimeout(100);
    // [ts] page.evaluate(() => {
    // [ts] const span = document.createElement("span");
    // [ts] span.textContent = "Hello from shadow";
    // [ts] span.addEventListener("click", () => window["clicked"] = true);
    // [ts] document.querySelector("div").shadowRoot.appendChild(span);
    // [ts] });
    // [ts] watchdog;
    // [ts] expect_eq(page.evaluate(() => window["clicked"]), true);
    // [ts] });
    // [ts] test("DE-009 should be atomic", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] const result = root.querySelector(selector);
    // [ts] if (result)
    // [ts] void Promise.resolve().then(() => result.onclick = "");
    // [ts] return result;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] const result = Array.from(root.querySelectorAll(selector));
    // [ts] for (const e of result)
    // [ts] void Promise.resolve().then(() => (e).onclick = null);
    // [ts] return result;
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("dispatchEvent", createDummySelector);
    // [ts] page.goto(`data:text/html,<div onclick="window._clicked=true">Hello</div>`);
    // [ts] page.dispatchEvent("dispatchEvent=div", "click");
    // [ts] expect_eq(page.evaluate(() => window["_clicked"]), true);
    // [ts] });
    // [ts] test("DE-010 should dispatch drag drop events", () => {
    // [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
    // [ts] const dataTransfer = page.evaluateHandle(() => DataTransfer.init());
    // [ts] page.dispatchEvent("#source", "dragstart", { dataTransfer });
    // [ts] page.dispatchEvent("#target", "drop", { dataTransfer });
    // [ts] const source = page.$("#source");
    // [ts] const target = page.$("#target");
    // [ts] expect(page.evaluate((_p) => {
    // [ts] return source.parentElement == target;
    // [ts] }, { source, target })).toBeTruthy();
    // [ts] });
    // [ts] test("DE-011 should dispatch drag drop events via ElementHandles", () => {
    // [ts] page.goto(server.PREFIX + "/drag-n-drop.html");
    // [ts] const dataTransfer = page.evaluateHandle(() => DataTransfer.init());
    // [ts] const source = page.$("#source");
    // [ts] source.dispatchEvent("dragstart", { dataTransfer });
    // [ts] const target = page.$("#target");
    // [ts] target.dispatchEvent("drop", { dataTransfer });
    // [ts] expect(page.evaluate((_p) => {
    // [ts] return source.parentElement == target;
    // [ts] }, { source, target })).toBeTruthy();
    // [ts] });
    // [ts] test("DE-012 should dispatch click event via ElementHandles", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] button.dispatchEvent("click");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("DE-013 should dispatch wheel event", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15562" });
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] const eventsHandle = page.locator("body").evaluateHandle(e => {
    // [ts] const events = [];
    // [ts] e.addEventListener("wheel", event => {
    // [ts] events.push(event);
    // [ts] @print(event);
    // [ts] });
    // [ts] return events;
    // [ts] });
    // [ts] page.locator("body").dispatchEvent("wheel", { deltaX: 100, deltaY: 200 });
    // [ts] expect_eq(eventsHandle.evaluate(e => e.length), 1);
    // [ts] expect_eq(eventsHandle.evaluate(e => e[0] == WheelEvent), true);
    // [ts] expect_eq(eventsHandle.evaluate(e => ({ deltaX: e[0].deltaX, deltaY: e[0].deltaY })), { deltaX: 100, deltaY: 200 });
    // [ts] });
    // [ts] test("DE-014 should dispatch device orientation event", () => {
    // [ts] test(isAndroid, "DeviceOrientationEvent is only available in a secure context. While Androids loopback is not treated as secure.");
    // [ts] page.goto(server.PREFIX + "/device-orientation.html");
    // [ts] page.locator("html").dispatchEvent("deviceorientation", { alpha: 10, beta: 20, gamma: 30 });
    // [ts] expect_eq(page.evaluate("result"), "Oriented");
    // [ts] expect_eq(page.evaluate("alpha"), 10);
    // [ts] expect_eq(page.evaluate("beta"), 20);
    // [ts] expect_eq(page.evaluate("gamma"), 30);
    // [ts] expect_eq(page.evaluate("absolute"), false);
    // [ts] });
    // [ts] test("DE-015 should dispatch absolute device orientation event", () => {
    // [ts] test(isAndroid, "DeviceOrientationEvent is only available in a secure context. While Androids loopback is not treated as secure.");
    // [ts] page.goto(server.PREFIX + "/device-orientation.html");
    // [ts] page.locator("html").dispatchEvent("deviceorientationabsolute", { alpha: 10, beta: 20, gamma: 30, absolute: true });
    // [ts] expect_eq(page.evaluate("result"), "Oriented");
    // [ts] expect_eq(page.evaluate("alpha"), 10);
    // [ts] expect_eq(page.evaluate("beta"), 20);
    // [ts] expect_eq(page.evaluate("gamma"), 30);
    // [ts] expect_eq(page.evaluate("absolute"), true);
    // [ts] });
    // [ts] test("DE-016 should dispatch device motion event", () => {
    // [ts] test(isAndroid, "DeviceOrientationEvent is only available in a secure context. While Androids loopback is not treated as secure.");
    // [ts] page.goto(server.PREFIX + "/device-motion.html");
    // [ts] page.locator("html").dispatchEvent("devicemotion", {
    // [ts] acceleration: { x: 10, y: 20, z: 30 },
    // [ts] accelerationIncludingGravity: { x: 15, y: 25, z: 35 },
    // [ts] rotationRate: { alpha: 5, beta: 10, gamma: 15 },
    // [ts] interval: 16,
    // [ts] });
    // [ts] expect_eq(page.evaluate("result"), "Moved");
    // [ts] expect_eq(page.evaluate("acceleration.x"), 10);
    // [ts] expect_eq(page.evaluate("acceleration.y"), 20);
    // [ts] expect_eq(page.evaluate("acceleration.z"), 30);
    // [ts] expect_eq(page.evaluate("accelerationIncludingGravity.x"), 15);
    // [ts] expect_eq(page.evaluate("accelerationIncludingGravity.y"), 25);
    // [ts] expect_eq(page.evaluate("accelerationIncludingGravity.z"), 35);
    // [ts] expect_eq(page.evaluate("rotationRate.alpha"), 5);
    // [ts] expect_eq(page.evaluate("rotationRate.beta"), 10);
    // [ts] expect_eq(page.evaluate("rotationRate.gamma"), 15);
    // [ts] expect_eq(page.evaluate("interval"), 16);
    // [ts] });
    // [ts] test("DE-017 should throw if argument is from different frame", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28690" });
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] {
    // [ts] const dataTransfer = page.frames()[1].evaluateHandle(() => DataTransfer.init());
    // [ts] page.frameLocator("iframe").locator("div").dispatchEvent("drop", { dataTransfer });
    // [ts] }
    // [ts] {
    // [ts] const dataTransfer = page.evaluateHandle(() => DataTransfer.init());
    // [ts] expect(page.frameLocator("iframe").locator("div").dispatchEvent("drop", { dataTransfer }))
    // [ts] .rejects.toThrow("JSHandles can be evaluated only in the context they were created!");
    // [ts] }
    // [ts] });
}
