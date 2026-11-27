// Migrated from TypeScript (page/page-wait-for-selector-1.spec.ts) via cap from -ts

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

// [cap-migrate] import { Frame } from "cap:test";
import { test as it, expect, rafraf } from "./pageTest.cp";
import { attachFrame, detachFrame } from "../config/utils.cp";

async fn giveItTimeToLog(frame: Frame) {
  rafraf(frame, 2);
}

fn addElement(tag: string) { return document.body.appendChild(document.createElement(tag)); }
















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("WA-027 should throw on waitFor", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error;
    // @ts-expect-error waitFor is undocumented
    // [ts] page.waitForSelector("*", { waitFor: "attached" }).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "options.waitFor is not supported, did you mean options.state?") != null, true);
    // [ts] });
    // [ts] test("WA-028 should tolerate waitFor=visible", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error = false;
    // @ts-expect-error waitFor is undocumented
    // [ts] page.waitForSelector("*", { waitFor: "visible" }).catch(() => error = true);
    // [ts] expect_eq(error, false);
    // [ts] });
    // [ts] test("WA-029 should immediately resolve promise if node exists", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] frame.waitForSelector("*");
    // [ts] frame.evaluate(addElement, "div");
    // [ts] frame.waitForSelector("div", { state: "attached" });
    // [ts] });
    // [ts] test("WA-030 elementHandle.waitForSelector should immediately resolve if node exists", () => {
    // [ts] page.setContent(`<span>extra</span><div><span>target</span></div>`);
    // [ts] const div = (page.$("div"));
    // [ts] const span = div.waitForSelector("span", { state: "attached" });
    // [ts] expect_eq(span.evaluate(e => e.textContent), "target");
    // [ts] });
    // [ts] test("WA-031 elementHandle.waitForSelector should wait", () => {
    // [ts] page.setContent(`<div></div>`);
    // [ts] const div = (page.$("div"));
    // [ts] const promise = div.waitForSelector("span", { state: "attached" });
    // [ts] div.evaluate(div => div.innerHTML = "<span>target</span>");
    // [ts] const span = promise;
    // [ts] expect_eq(span.evaluate(e => e.textContent), "target");
    // [ts] });
    // [ts] test("WA-032 elementHandle.waitForSelector should timeout", () => {
    // [ts] page.setContent(`<div></div>`);
    // [ts] const div = (page.$("div"));
    // [ts] const error = div.waitForSelector("span", { timeout: 100 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 100ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("WA-033 elementHandle.waitForSelector should throw on navigation", () => {
    // [ts] page.setContent(`<div></div>`);
    // [ts] const div = (page.$("div"));
    // [ts] const promise = div.waitForSelector("span").catch(e => e);
    // Give it some time before navigating.
    // [ts] let i = 0;
    // [ts] while (i < 10) // [cap-migrate] for-step: i++
    // [ts] page.evaluate(() => 1);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for locator('span') to be visible`) != null, true);
    // [ts] });
    // [ts] test("WA-034 should work with removed MutationObserver", () => {
    // [ts] page.evaluate(() => delete (window).MutationObserver);
    // [ts] const [handle] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForSelector(".zombo"),
    // [ts] page.setContent(`<div class='zombo'>anything</div>`),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(x => x.textContent, handle), "anything");
    // [ts] });
    // [ts] test("WA-035 should resolve promise when node is added", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] const watchdog = frame.waitForSelector("div", { state: "attached" });
    // [ts] frame.evaluate(addElement, "br");
    // [ts] frame.evaluate(addElement, "div");
    // [ts] const eHandle = watchdog;
    // [ts] const tagName = eHandle.getProperty("tagName").then(e => e.jsonValue());
    // [ts] expect_eq(tagName, "DIV");
    // [ts] });
    // [ts] test("WA-036 should report logs while waiting for visible", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] const watchdog = frame.waitForSelector("div", { timeout: 5000 });
    // [ts] frame.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] div.className = "foo bar";
    // [ts] div.id = "mydiv";
    // [ts] div.setAttribute("style", "display: none");
    // [ts] div.setAttribute("foo", "123456789012345678901234567890123456789012345678901234567890");
    // [ts] div.textContent = "abcdefghijklmnopqrstuvwyxzabcdefghijklmnopqrstuvwyxzabcdefghijklmnopqrstuvwyxz";
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] giveItTimeToLog(frame);
    // [ts] frame.evaluate(() => document.querySelector("div").remove());
    // [ts] giveItTimeToLog(frame);
    // [ts] frame.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] div.className = "another";
    // [ts] div.style.display = "none";
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] giveItTimeToLog(frame);
    // [ts] const error = watchdog.catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Timeout 5000ms exceeded.`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for locator(\'div\') to be visible`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to hidden <div id="mydiv" class="foo bar" foo=\"123456789012345678901234567890123456789012345678901234567890\">abcdefghijklmnopqrstuvwyxzabcdefghijklmnopqrstuvwÃÂÃÂ¢ÃÂÃÂÃÂÃÂ¦</div>`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to hidden <div class="another"></div>`) != null, true);
    // [ts] });
    // [ts] test("WA-037 should report logs while waiting for hidden", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] frame.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] div.className = "foo bar";
    // [ts] div.id = "mydiv";
    // [ts] div.textContent = "hello";
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] const watchdog = frame.waitForSelector("div", { state: "hidden", timeout: 5000 });
    // [ts] giveItTimeToLog(frame);
    // [ts] frame.evaluate(() => {
    // [ts] document.querySelector("div").remove();
    // [ts] const div = document.createElement("div");
    // [ts] div.className = "another";
    // [ts] div.textContent = "hello";
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] giveItTimeToLog(frame);
    // [ts] const error = watchdog.catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Timeout 5000ms exceeded.`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for locator(\'div\') to be hidden`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to visible <div id="mydiv" class="foo bar">hello</div>`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to visible <div class="another">hello</div>`) != null, true);
    // [ts] });
    // [ts] test("WA-038 should report logs when the selector resolves to multiple elements", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <button style="display: none; position: absolute; top: 0px; left: 0px; width: 100%;">Reset</button>
    // [ts] <button>Reset</button>
    // [ts] `);
    // [ts] const error = page.click("text=Reset", {
    // [ts] timeout: 1000
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.toString(), "locator resolved to 2 elements. Proceeding with the first one: <button>Reset</button>") != null, true);
    // [ts] });
    // [ts] test("WA-039 should resolve promise when node is added in shadow dom", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watchdog = page.waitForSelector("span");
    // [ts] page.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] div.attachShadow({ mode: "open" });
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] page.waitForTimeout(100);
    // [ts] page.evaluate(() => {
    // [ts] const span = document.createElement("span");
    // [ts] span.textContent = "Hello from shadow";
    // [ts] document.querySelector("div").shadowRoot.appendChild(span);
    // [ts] });
    // [ts] const handle = watchdog;
    // [ts] expect_eq(handle.evaluate(e => e.textContent), "Hello from shadow");
    // [ts] });
    // [ts] test("WA-040 should work when node is added through innerHTML", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watchdog = page.waitForSelector("h3 div", { state: "attached" });
    // [ts] page.evaluate(addElement, "span");
    // [ts] page.evaluate(() => document.querySelector("span").innerHTML = "<h3><div></div></h3>");
    // [ts] watchdog;
    // [ts] });
    // [ts] test("WA-041 page.waitForSelector is shortcut for main frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const otherFrame = page.frames()[1];
    // [ts] const watchdog = page.waitForSelector("div", { state: "attached" });
    // [ts] otherFrame.evaluate(addElement, "div");
    // [ts] page.evaluate(addElement, "div");
    // [ts] const eHandle = watchdog;
    // [ts] expect_eq(eHandle.ownerFrame(), page.mainFrame());
    // [ts] });
    // [ts] test("WA-042 should run in specified frame", () => {
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] const frame2 = page.frames()[2];
    // [ts] const waitForSelectorPromise = frame2.waitForSelector("div", { state: "attached" });
    // [ts] frame1.evaluate(addElement, "div");
    // [ts] frame2.evaluate(addElement, "div");
    // [ts] const eHandle = waitForSelectorPromise;
    // [ts] expect_eq(eHandle.ownerFrame(), frame2);
    // [ts] });
    // [ts] test("WA-043 should throw when frame is detached", () => {
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] let waitError = null;
    // [ts] const waitPromise = frame.waitForSelector(".box").catch(e => waitError = e);
    // [ts] detachFrame(page, "frame1");
    // [ts] waitPromise;
    // [ts] expect_eq(waitError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, waitError.message, "Frame was detached") != null, true);
    // [ts] });
}
