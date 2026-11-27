// Migrated from TypeScript (page/elementhandle-misc.spec.ts) via cap from -ts

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
    // [ts] test("EH-045 should hover", () => {
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] const button = page.$("#button-6");
    // [ts] button.hover();
    // [ts] expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-6");
    // [ts] });
    // [ts] test("EH-046 should hover when Node is removed", () => {
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.evaluate(() => delete window["Node"]);
    // [ts] const button = page.$("#button-6");
    // [ts] button.hover();
    // [ts] expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-6");
    // [ts] });
    // [ts] test("EH-047 should fill input", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const handle = page.$("input");
    // [ts] handle.fill("some value");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] });
    // [ts] test("EH-048 should fill input when Node is removed", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.evaluate(() => delete window["Node"]);
    // [ts] const handle = page.$("input");
    // [ts] handle.fill("some value");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "some value");
    // [ts] });
    // [ts] test("EH-049 should check the box", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox'></input>`);
    // [ts] const input = page.$("input");
    // [ts] input.check();
    // [ts] expect_eq(page.evaluate("checkbox.checked"), true);
    // [ts] });
    // [ts] test("EH-050 should check the box using setChecked", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox'></input>`);
    // [ts] const input = page.$("input");
    // [ts] input.setChecked(true);
    // [ts] expect_eq(page.evaluate("checkbox.checked"), true);
    // [ts] input.setChecked(false);
    // [ts] expect_eq(page.evaluate("checkbox.checked"), false);
    // [ts] });
    // [ts] test("EH-051 should uncheck the box", () => {
    // [ts] page.setContent(`<input id='checkbox' type='checkbox' checked></input>`);
    // [ts] const input = page.$("input");
    // [ts] input.uncheck();
    // [ts] expect_eq(page.evaluate("checkbox.checked"), false);
    // [ts] });
    // [ts] test("EH-052 should select single option", () => {
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const select = page.$("select");
    // [ts] select.selectOption("blue");
    // [ts] expect_eq(page.evaluate(() => window["result"].onInput), ["blue"]);
    // [ts] expect_eq(page.evaluate(() => window["result"].onChange), ["blue"]);
    // [ts] });
    // [ts] test("EH-053 should focus a button", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] expect_eq(button.evaluate(button => document.activeElement == button), false);
    // [ts] button.focus();
    // [ts] expect_eq(button.evaluate(button => document.activeElement == button), true);
    // [ts] });
    // [ts] test("EH-054 should allow disposing twice", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29945" });
    // [ts] page.setContent("<section>39</section>");
    // [ts] const element = page.$("section");
    // [ts] expect_eq(element, true);
    // [ts] element.dispose();
    // [ts] element.dispose();
    // [ts] });
}
