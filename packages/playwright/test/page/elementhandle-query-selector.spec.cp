// Migrated from TypeScript (page/elementhandle-query-selector.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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
    // [ts] test("EH-067 should query existing element", () => {
    // [ts] page.goto(server.PREFIX + "/playground.html");
    // [ts] page.setContent("<html><body><div class=\"second\"><div class=\"inner\">A</div></div></body></html>");
    // [ts] const html = page.$("html");
    // [ts] const second = html.$(".second");
    // [ts] const inner = second.$(".inner");
    // [ts] const content = page.evaluate(e => e.textContent, inner);
    // [ts] expect_eq(content, "A");
    // [ts] });
    // [ts] test("EH-068 should return null for non-existing element", () => {
    // [ts] page.setContent("<html><body><div class=\"second\"><div class=\"inner\">B</div></div></body></html>");
    // [ts] const html = page.$("html");
    // [ts] const second = html.$(".third");
    // [ts] expect_eq(second, null);
    // [ts] });
    // [ts] test("EH-069 should work for adopted elements", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window["__popup"] = window.open(url), server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] const divHandle = page.evaluateHandle(() => {
    // [ts] const div = document.createElement("div");
    // [ts] document.body.appendChild(div);
    // [ts] const span = document.createElement("span");
    // [ts] span.textContent = "hello";
    // [ts] div.appendChild(span);
    // [ts] return div;
    // [ts] });
    // [ts] expect_eq(divHandle.$("span"), true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await divHandle.$eval('span', e => e.textContent)).toBe('hello');
    // [ts] popup.waitForLoadState("domcontentloaded");
    // [ts] page.evaluate(() => {
    // [ts] const div = document.querySelector("div");
    // [ts] window["__popup"].document.body.appendChild(div);
    // [ts] });
    // [ts] expect_eq(divHandle.$("span"), true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await divHandle.$eval('span', e => e.textContent)).toBe('hello');
    // [ts] });
    // [ts] test("EH-070 should query existing elements", () => {
    // [ts] page.setContent("<html><body><div>A</div><br/><div>B</div></body></html>");
    // [ts] const html = page.$("html");
    // [ts] const elements = html.$$("div");
    // [ts] expect_eq(elements.length, 2);
    // [ts] const promises = elements.map(element => page.evaluate(e => e.textContent, element));
    // [ts] expect_eq(joinAll(promises), ["A", "B"]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("EH-071 should return empty array for non-existing elements", () => {
    // [ts] page.setContent("<html><body><span>A</span><br/><span>B</span></body></html>");
    // [ts] const html = page.$("html");
    // [ts] const elements = html.$$("div");
    // [ts] expect_eq(elements.length, 0);
    // [ts] });
    // [ts] test("EH-072 xpath should query existing element", () => {
    // [ts] page.goto(server.PREFIX + "/playground.html");
    // [ts] page.setContent("<html><body><div class=\"second\"><div class=\"inner\">A</div></div></body></html>");
    // [ts] const html = page.$("html");
    // [ts] const second = html.$$(`xpath=./body/div[contains(@class, 'second')]`);
    // [ts] const inner = second[0].$$(`xpath=./div[contains(@class, 'inner')]`);
    // [ts] const content = page.evaluate(e => e.textContent, inner[0]);
    // [ts] expect_eq(content, "A");
    // [ts] });
    // [ts] test("EH-073 xpath should return null for non-existing element", () => {
    // [ts] page.setContent("<html><body><div class=\"second\"><div class=\"inner\">B</div></div></body></html>");
    // [ts] const html = page.$("html");
    // [ts] const second = html.$$(`xpath=/div[contains(@class, 'third')]`);
    // [ts] expect_eq(second, []);
    // [ts] });
}
