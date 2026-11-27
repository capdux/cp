// Migrated from TypeScript (page/page-click-timeout-4.spec.ts) via cap from -ts

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

import { test as it, expect, rafraf } from "./pageTest.cp";


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CL-120 should timeout waiting for stable position", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] button.evaluate(button => {
    // [ts] button.style.transition = "margin 5s linear 0s";
    // [ts] button.style.marginLeft = "200px";
    // [ts] });
    // rafraf for Firefox to kick in the animation.
    // [ts] rafraf(page);
    // [ts] const error = button.click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "waiting for element to be visible, enabled and stable") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is not stable") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "retrying click action") != null, true);
    // [ts] });
    // [ts] test("CL-121 should click for the second time after first timeout", () => {
    // [ts] test(mode != "default");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const __testHookBeforePointerAction = () => Promise.init(f => setTimeout(f, 1500));
    // [ts] const error = page.click("button", { timeout: 1000, __testHookBeforePointerAction }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded.") != null, true);
    // [ts] expect_eq(page.evaluate("result"), "Was not clicked");
    // [ts] page.waitForTimeout(2000);
    // [ts] expect_eq(page.evaluate("result"), "Was not clicked");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-122 should fail to click the button behind a large header after scrolling around", () => {
    // [ts] test(isAndroid, "Different viewport size");
    // [ts] page.setViewportSize({ width: 500, height: 240 });
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] * {
    // [ts] padding: 0;
    // [ts] margin: 0;
    // [ts] }
    // [ts] li {
    // [ts] height: 80px;
    // [ts] border: 1px solid black;
    // [ts] }
    // [ts] ol {
    // [ts] padding-top: 160px;
    // [ts] }
    // [ts] div.fixed {
    // [ts] position: fixed;
    // [ts] z-index: 1001;
    // [ts] width: 100%;
    // [ts] background: rgba(255, 0, 0, 0.2);
    // [ts] height: 2000px;
    // [ts] }
    // [ts] </style>
    // [ts] <div class=fixed></div>
    // [ts] <ol>
    // [ts] <li>hi1</li><li>hi2</li><li>hi3</li><li>hi4</li><li>hi5</li><li>hi6</li><li>hi7</li><li>hi8</li>
    // [ts] <li id=target onclick="window.__clicked = true">hi9</li>
    // [ts] <li>hi10</li><li>hi11</li><li>hi12</li><li>hi13</li><li id=li14>hi14</li>
    // [ts] </ol>
    // [ts] <script>
    // [ts] window.scrollTops = [];
    // [ts] window.addEventListener('scroll', () => {
    // [ts] window.scrollTops.push(window.scrollY);
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#li14', e => e.scrollIntoView());
    // [ts] const error = page.click("#target", { timeout: 3500 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `<div class="fixed"></div> intercepts pointer events`) != null, true);
    // [ts] expect_eq(page.evaluate(() => window["__clicked"]), null);
    // [ts] const scrollTops = page.evaluate(() => window["scrollTops"]);
    // [ts] const distinct = Set.init(scrollTops);
    // [ts] expect_eq(distinct.size > 2, true);  // At least three different scroll positions.
    // [ts] });
}
