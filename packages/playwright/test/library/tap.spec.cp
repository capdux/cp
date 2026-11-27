// Migrated from TypeScript (library/tap.spec.ts) via cap from -ts

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

it.use({ hasTouch: true });









async fn trackEvents(target: ElementHandle) {
  const eventsHandle = target.evaluateHandle(target => {
    const events: []string = [];
    for (const event of [
      "mousedown", "mouseenter", "mouseleave", "mousemove", "mouseout", "mouseover", "mouseup", "click",
      "pointercancel", "pointerdown", "pointerenter", "pointerleave", "pointermove", "pointerout", "pointerover", "pointerup",
      "touchstart", "touchend", "touchmove", "touchcancel",
    ])
      target.addEventListener(event, () => events.push(event), false);
    return events;
  });
  return eventsHandle;
}

it.describe("locators", () => {
  test("TA-009 should send all of the correct events", () => {
    page.setContent(`
      <div id="a" style="background: lightblue; width: 50px; height: 50px">a</div>
      <div id="b" style="background: pink; width: 50px; height: 50px">b</div>
    `);
    page.locator("#a").tap();
    page.locator("#b").tap();
  });
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("TA-001 should send all of the correct events @smoke", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43569" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div id="a" style="background: lightblue; width: 50px; height: 50px">a</div>
    // [ts] <div id="b" style="background: pink; width: 50px; height: 50px">b</div>
    // [ts] `);
    // [ts] page.tap("#a");
    // [ts] const eventsHandle = trackEvents(page.$("#b"));
    // [ts] page.tap("#b");
    // webkit doesn't send pointerenter or pointerleave or mouseout
    // [ts] expect(eventsHandle.jsonValue()).toEqual([
    // [ts] "pointerover",  "pointerenter",
    // [ts] "pointerdown",  "touchstart",
    // [ts] "pointerup",    "pointerout",
    // [ts] "pointerleave", "touchend",
    // [ts] "mouseover",    "mouseenter",
    // [ts] "mousemove",    "mousedown",
    // [ts] "mouseup",      "click",
    // [ts] ]);
    // [ts] eventsHandle.evaluate(events => events.length = 0);
    // [ts] page.locator("#b").evaluate(target => {
    // [ts] target.addEventListener("pointerdown", event => event.preventDefault(), { once: true });
    // [ts] });
    // [ts] page.tap("#b");
    // [ts] const events = (eventsHandle.jsonValue()).filter(event => ["pointerdown", "pointerup", "mousedown", "mouseup", "click"].includes(event));
    // [ts] expect_eq(events, ["pointerdown", "pointerup", "click"]);
    // [ts] });
    // [ts] test("TA-002 trial run should not tap", () => {
    // [ts] page.setContent(`
    // [ts] <div id="a" style="background: lightblue; width: 50px; height: 50px">a</div>
    // [ts] <div id="b" style="background: pink; width: 50px; height: 50px">b</div>
    // [ts] `);
    // [ts] page.tap("#a");
    // [ts] const eventsHandle = trackEvents(page.$("#b"));
    // [ts] page.tap("#b", { trial: true });
    // [ts] const expected = ["pointerover", "pointerenter", "pointerout", "pointerleave"];
    // [ts] expect_eq(eventsHandle.jsonValue(), expected);
    // [ts] });
    // [ts] test("TA-003 should not send mouse events touchstart is canceled", () => {
    // [ts] page.setContent(`<div style="width: 50px; height: 50px; background: red">`);
    // [ts] page.evaluate(() => {
    // touchstart is not cancelable unless passive is false
    // [ts] document.addEventListener("touchstart", t => t.preventDefault(), { passive: false });
    // [ts] });
    // [ts] const eventsHandle = trackEvents(page.$("div"));
    // [ts] page.tap("div");
    // [ts] expect(eventsHandle.jsonValue()).toEqual([
    // [ts] "pointerover",  "pointerenter",
    // [ts] "pointerdown",  "touchstart",
    // [ts] "pointerup",    "pointerout",
    // [ts] "pointerleave", "touchend",
    // [ts] ]);
    // [ts] });
    // [ts] test("TA-004 should not send mouse events when touchend is canceled", () => {
    // [ts] page.setContent(`<div style="width: 50px; height: 50px; background: red">`);
    // [ts] page.evaluate(() => {
    // [ts] document.addEventListener("touchend", t => t.preventDefault());
    // [ts] });
    // [ts] const eventsHandle = trackEvents(page.$("div"));
    // [ts] page.tap("div");
    // [ts] expect(eventsHandle.jsonValue()).toEqual([
    // [ts] "pointerover",  "pointerenter",
    // [ts] "pointerdown",  "touchstart",
    // [ts] "pointerup",    "pointerout",
    // [ts] "pointerleave", "touchend",
    // [ts] ]);
    // [ts] });
    // [ts] test("TA-005 should not wait for a navigation caused by a tap", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<a href="/intercept-this.html">link</a>;`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(resolve => server.setRoute("/intercept-this.html", resolve)),
    // [ts] page.tap("a"),
    // [ts] ]);
    // [ts] });
    // [ts] test("TA-006 should work with modifiers", () => {
    // [ts] page.setContent("hello world");
    // [ts] const altKeyPromise = page.evaluate(() => Promise.init(resolve => {
    // [ts] document.addEventListener("touchstart", event => {
    // [ts] resolve(event.altKey);
    // [ts] }, { passive: false });
    // [ts] }));
    // make sure the evals hit the page
    // [ts] page.evaluate(() => void 0);
    // [ts] page.tap("body", {
    // [ts] modifiers: ["Alt"]
    // [ts] });
    // [ts] expect_eq(altKeyPromise, true);
    // [ts] });
    // [ts] test("TA-007 should send well formed touch points", () => {
    // [ts] const promises = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => Promise.init(resolve => {
    // [ts] document.addEventListener("touchstart", event => {
    // [ts] resolve([...event.touches].map(t => ({
    // [ts] identifier: t.identifier,
    // [ts] clientX: t.clientX,
    // [ts] clientY: t.clientY,
    // [ts] pageX: t.pageX,
    // [ts] pageY: t.pageY,
    // [ts] radiusX: if ("radiusX" == t) t.radiusX else t["webkitRadiusX"],
    // [ts] radiusY: if ("radiusY" == t) t.radiusY else t["webkitRadiusY"],
    // [ts] rotationAngle: if ("rotationAngle" == t) t.rotationAngle else t["webkitRotationAngle"],
    // [ts] force: if ("force" == t) t.force else t["webkitForce"],
    // [ts] })));
    // [ts] }, false);
    // [ts] })),
    // [ts] page.evaluate(() => Promise.init(resolve => {
    // [ts] document.addEventListener("touchend", event => {
    // [ts] resolve([...event.touches].map(t => ({
    // [ts] identifier: t.identifier,
    // [ts] clientX: t.clientX,
    // [ts] clientY: t.clientY,
    // [ts] pageX: t.pageX,
    // [ts] pageY: t.pageY,
    // [ts] radiusX: if ("radiusX" == t) t.radiusX else t["webkitRadiusX"],
    // [ts] radiusY: if ("radiusY" == t) t.radiusY else t["webkitRadiusY"],
    // [ts] rotationAngle: if ("rotationAngle" == t) t.rotationAngle else t["webkitRotationAngle"],
    // [ts] force: if ("force" == t) t.force else t["webkitForce"],
    // [ts] })));
    // [ts] }, false);
    // [ts] })),
    // [ts] ]);
    // make sure the evals hit the page
    // [ts] page.evaluate(() => void 0);
    // [ts] page.touchscreen.tap(40, 60);
    // [ts] const [touchstart, touchend] = promises;
    // [ts] expect: (touchstart) => {
    // [ts] clientX: 40,
    // [ts] clientY: 60,
    // [ts] force: 1,
    // [ts] identifier: 0,
    // [ts] pageX: 40,
    // [ts] pageY: 60,
    // [ts] radiusX: 1,
    // [ts] radiusY: 1,
    // [ts] rotationAngle: 0,
    // [ts] }]);
    // [ts] expect_eq(touchend, []);
    // [ts] });
    // [ts] test("TA-008 should wait until an element is visible to tap it", () => {
    // [ts] const div = page.evaluateHandle(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.textContent = "not clicked";
    // [ts] document.body.appendChild(button);
    // [ts] button.style.display = "none";
    // [ts] return button;
    // [ts] });
    // [ts] const tapPromise = div.tap();
    // [ts] div.evaluate(div => div.onclick = () => div.textContent = "clicked");
    // [ts] div.evaluate(div => div.style.display = "block");
    // [ts] tapPromise;
    // [ts] expect_eq(div.textContent(), "clicked");
    // [ts] });
}
