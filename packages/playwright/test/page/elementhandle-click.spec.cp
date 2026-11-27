// Migrated from TypeScript (page/elementhandle-click.spec.ts) via cap from -ts

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
    // [ts] test("EH-010 should work @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] button.click();
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("EH-011 should work with Node removed", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => delete window["Node"]);
    // [ts] const button = page.$("button");
    // [ts] button.click();
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("EH-012 should work for Shadow DOM v1", () => {
    // [ts] page.goto(server.PREFIX + "/shadow.html");
    // [ts] const buttonHandle = page.evaluateHandle(() => window["button"]);
    // [ts] buttonHandle.click();
    // [ts] expect_eq(page.evaluate("clicked"), true);
    // [ts] });
    // [ts] test("EH-013 should work for TextNodes", () => {
    // [ts] page.setContent(`
    // [ts] <div id='outer' onclick="window['result'] = 'Clicked ' + event.target.id;">
    // [ts] <div id='inner' style="max-width: 50px">Lorem ipsum dolor sit amet consectetur adipiscing elit proin, integer curabitur imperdiet rhoncus cursus tincidunt bibendum, consequat sed magnis laoreet luctus mollis tellus. Nisl parturient mus accumsan feugiat sem laoreet magnis nisi, aptent per sollicitudin gravida orci ac blandit, viverra eros praesent auctor vivamus semper bibendum. Consequat sed habitasse luctus dictumst gravida platea semper phasellus, nascetur ridiculus purus est varius quisque et scelerisque, id vehicula eleifend montes sollicitudin dis velit. Pellentesque ridiculus per natoque et eleifend taciti nunc, laoreet auctor at condimentum imperdiet ante, conubia mi cubilia scelerisque sociosqu sem.</div>
    // [ts] Custom Text.
    // [ts] </div>
    // [ts] `);
    // [ts] const buttonTextNode = page.evaluateHandle(() => document.querySelector("#outer").lastChild);
    // [ts] buttonTextNode.click();
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked outer");
    // [ts] });
    // [ts] test("EH-014 should throw for detached nodes", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] page.evaluate(button => button.remove(), button);
    // [ts] let error = null;
    // [ts] button.click().catch(err => error = err);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not attached to the DOM") != null, true);
    // [ts] });
    // [ts] test("EH-015 should throw for hidden nodes with force", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] page.evaluate(button => button.style.display = "none", button);
    // [ts] const error = button.click({ force: true }).catch(err => err);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not visible") != null, true);
    // [ts] });
    // [ts] test("EH-016 should throw for recursively hidden nodes with force", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] page.evaluate(button => button.parentElement.style.display = "none", button);
    // [ts] const error = button.click({ force: true }).catch(err => err);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not visible") != null, true);
    // [ts] });
    // [ts] test("EH-017 should throw for <br> elements with force", () => {
    // [ts] page.setContent("hello<br>goodbye");
    // [ts] const br = page.$("br");
    // [ts] const error = br.click({ force: true }).catch(err => err);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is outside of the viewport") != null, true);
    // [ts] });
    // [ts] test("EH-018 should double click the button", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] window["double"] = false;
    // [ts] const button = document.querySelector("button");
    // [ts] button.addEventListener("dblclick", event => {
    // [ts] window["double"] = true;
    // [ts] });
    // [ts] });
    // [ts] const button = page.$("button");
    // [ts] button.dblclick();
    // [ts] expect_eq(page.evaluate("double"), true);
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
}
