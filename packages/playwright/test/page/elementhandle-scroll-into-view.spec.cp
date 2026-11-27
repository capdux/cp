// Migrated from TypeScript (page/elementhandle-scroll-into-view.spec.ts) via cap from -ts

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



async fn testWaiting(page: auto, after: auto) {
    // TODO(cap-migrate): refine TS body
    // [ts] const div = page.$("div");
    // [ts] let done = false;
    // [ts] const promise = div.scrollIntoViewIfNeeded().then(() => done = true);
    // [ts] page.waitForTimeout(1000);
    // [ts] expect_eq(done, false);
    // [ts] div.evaluate(after);
    // [ts] promise;
    // [ts] expect_eq(done, true);
}







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EH-093 should work @smoke", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] page.goto(server.PREFIX + "/offscreenbuttons.html");
    // [ts] let i = 0;
    // [ts] while (i < 11) { // [cap-migrate] for-step: ++i
    // [ts] const button = page.$("#btn" + i);
    // [ts] const before = button.evaluate(button => {
    // [ts] return button.getBoundingClientRect().right - window.innerWidth;
    // [ts] });
    // [ts] expect_eq(before, 10 * i);
    // [ts] button.scrollIntoViewIfNeeded();
    // [ts] const after = button.evaluate(button => {
    // [ts] return button.getBoundingClientRect().right - window.innerWidth;
    // [ts] });
    // [ts] expect_eq(after <= 0, true);
    // [ts] page.evaluate(() => window.scrollTo(0, 0));
    // [ts] }
    // [ts] });
    // [ts] test("EH-094 should throw for detached element", () => {
    // [ts] page.setContent("<div>Hello</div>");
    // [ts] const div = page.$("div");
    // [ts] div.evaluate(div => div.remove());
    // [ts] const error = div.scrollIntoViewIfNeeded().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not attached to the DOM") != null, true);
    // [ts] });
    // [ts] test("EH-095 should wait for display:none to become visible", () => {
    // [ts] page.setContent("<div style=\"display:none\">Hello</div>");
    // [ts] testWaiting(page, div => div.style.display = "block");
    // [ts] });
    // [ts] test("EH-096 should scroll display:contents into view", () => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 105, "Needs https://chromium-review.googlesource.com/c/chromium/src/+/3758670");
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15034" });
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] html, body { margin: 0; padding: 0; width: 100%; height: 100%; }
    // [ts] ::-webkit-scrollbar { display: none; }
    // [ts] * { scrollbar-width: none; }
    // [ts] </style>
    // [ts] <div id=container style="width:200px;height:200px;overflow:scroll;border:1px solid black;">
    // [ts] <div style="margin-top:500px;background:red;">
    // [ts] <div style="height:50px;width:100px;background:cyan;">
    // [ts] <div id=target style="display:contents">Hello</div>
    // [ts] </div>
    // [ts] <div>
    // [ts] </div>
    // [ts] `);
    // [ts] const div = page.$("#target");
    // [ts] div.scrollIntoViewIfNeeded();
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const scrollTop = await page.$eval('#container', e => e.scrollTop);
    // On Android the value is not exact due to various scale conversions.
    // [ts] expect_eq(Math.abs(scrollTop - 350) < 1, true);
    // [ts] });
    // [ts] test("EH-124 should scroll nested display:contents into view", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] html, body { margin: 0; padding: 0; width: 100%; height: 100%; }
    // [ts] ::-webkit-scrollbar { display: none; }
    // [ts] * { scrollbar-width: none; }
    // [ts] </style>
    // [ts] <div id=container style="width:200px;height:200px;overflow:scroll;border:1px solid black;">
    // [ts] <div style="margin-top:500px;background:red;">
    // [ts] <div id=outer style="display:contents"><div style="display:contents"><div id=target style="height:50px;width:100px;background:cyan;">Hello</div></div><div style="height:2000px;"></div><div>Sibling</div></div>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] const div = page.$("#outer");
    // [ts] div.scrollIntoViewIfNeeded();
    // The first box in the flattened subtree of #outer is #target, two levels down,
    // not the tall sibling. No whitespace text nodes inside #outer: in Firefox they
    // get boxes of their own, which would hide a fallback that only looks one level down.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const targetIsVisible = await page.$eval('#target', target => {
    // [ts-skip] const container = document.getElementById('container').getBoundingClientRect();
    // [ts-skip] const rect = target.getBoundingClientRect();
    // [ts-skip] return rect.top >= container.top && rect.bottom <= container.bottom;
    // [ts-skip] });
    // [ts] expect_eq(targetIsVisible, true);
    // [ts] });
    // [ts] test("EH-097 should work for visibility:hidden element", () => {
    // [ts] page.setContent("<div style=\"visibility:hidden\">Hello</div>");
    // [ts] const div = page.$("div");
    // [ts] div.scrollIntoViewIfNeeded();
    // [ts] });
    // [ts] test("EH-098 should work for zero-sized element", () => {
    // [ts] page.setContent("<div style=\"height:0\">Hello</div>");
    // [ts] const div = page.$("div");
    // [ts] div.scrollIntoViewIfNeeded();
    // [ts] });
    // [ts] test("EH-099 should wait for nested display:none to become visible", () => {
    // [ts] page.setContent("<span style=\"display:none\"><div>Hello</div></span>");
    // [ts] testWaiting(page, div => div.parentElement.style.display = "block");
    // [ts] });
    // [ts] test("EH-100 should wait for element to stop moving", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] @keyframes move {
    // [ts] from { margin-left: 0; }
    // [ts] to { margin-left: 200px; }
    // [ts] }
    // [ts] div.animated {
    // [ts] animation: 2s linear 0s infinite alternate move;
    // [ts] }
    // [ts] </style>
    // [ts] <div class=animated>moving</div>
    // [ts] `);
    // [ts] testWaiting(page, div => div.classList.remove("animated"));
    // [ts] });
    // [ts] test("EH-101 should timeout waiting for visible", () => {
    // [ts] page.setContent("<div style=\"display:none\">Hello</div>");
    // [ts] const div = page.$("div");
    // [ts] const error = div.scrollIntoViewIfNeeded({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is not visible") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "retrying scroll into view action") != null, true);
    // [ts] });
}
