// Migrated from TypeScript (page/queryselector.spec.ts) via cap from -ts

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
    // [ts] test("QS-001 should throw for non-string selector", () => {
    // [ts] const error = page.$(null).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "selector: expected string, got object") != null, true);
    // [ts] });
    // [ts] test("QS-002 should query existing element with css selector @smoke", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("css=section");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-003 should query existing element with text selector", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("text=\"test\"");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-004 should query existing element with xpath selector", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("xpath=\"html\"/section");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-005 should return null for non-existing element", () => {
    // [ts] const element = page.$("non-existing-element");
    // [ts] expect_eq(element, null);
    // [ts] });
    // [ts] test("QS-006 should auto-detect xpath selector", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("//html/body/section");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-007 should auto-detect xpath selector with starting parenthesis", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("(//section)[1]");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-008 should auto-detect xpath selector starting with ..", () => {
    // [ts] page.setContent("<div><section>test</section><span></span></div>");
    // [ts] const span = page.$("\"test\" >> ../span");
    // [ts] expect_eq(span.evaluate(e => e.nodeName), "SPAN");
    // [ts] const div = page.$("\"test\" >> ..");
    // [ts] expect_eq(div.evaluate(e => e.nodeName), "DIV");
    // [ts] });
    // [ts] test("QS-009 should auto-detect text selector", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("\"test\"");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-010 should auto-detect css selector", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const element = page.$("section");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-011 should support >> syntax", () => {
    // [ts] page.setContent("<section><div>test</div></section>");
    // [ts] const element = page.$("css=section >> css=div");
    // [ts] expect_eq(element, true);
    // [ts] });
    // [ts] test("QS-012 should query existing elements", () => {
    // [ts] page.setContent("<div>A</div><br/><div>B</div>");
    // [ts] const elements = page.$$("div");
    // [ts] expect_eq(elements.length, 2);
    // [ts] const promises = elements.map(element => page.evaluate(e => e.textContent, element));
    // [ts] expect_eq(joinAll(promises), ["A", "B"]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("QS-013 should return empty array if nothing is found", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const elements = page.$$("div");
    // [ts] expect_eq(elements.length, 0);
    // [ts] });
    // [ts] test("QS-014 xpath should query existing element", () => {
    // [ts] page.setContent("<section>test</section>");
    // [ts] const elements = page.$$("xpath=\"html\"/section");
    // [ts] expect_eq(elements[0], true);
    // [ts] expect_eq(elements.length, 1);
    // [ts] });
    // [ts] test("QS-015 xpath should return empty array for non-existing element", () => {
    // [ts] const element = page.$$("//html/body/non-existing-element");
    // [ts] expect_eq(element, []);
    // [ts] });
    // [ts] test("QS-016 xpath should return multiple elements", () => {
    // [ts] page.setContent("<div></div><div></div>");
    // [ts] const elements = page.$$("xpath=\"html\"/div");
    // [ts] expect_eq(elements.length, 2);
    // [ts] });
    // [ts] test("QS-017 $$ should work with bogus Array.from", () => {
    // [ts] page.setContent("<div>hello</div><div></div>");
    // [ts] const div1 = page.evaluateHandle(() => {
    // [ts] Array.from = () => [];
    // [ts] return document.querySelector("div");
    // [ts] });
    // [ts] const elements = page.$$("div");
    // [ts] expect_eq(elements.length, 2);
    // Check that element handle is functional and belongs to the main world.
    // [ts] expect_eq(elements[0].evaluate((div, div1) => div == div1, div1), true);
    // [ts] });
}
