// Migrated from TypeScript (page/page-wait-for-selector-2.spec.ts) via cap from -ts

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
import { attachFrame, detachFrame } from "../config/utils.cp";

fn addElement(tag: string) { return document.body.appendChild(document.createElement(tag)); }
































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("WA-044 should survive cross-process navigation", () => {
    // [ts] let boxFound = false;
    // [ts] const waitForSelector = page.waitForSelector(".box").then(() => boxFound = true);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(boxFound, false);
    // [ts] page.reload();
    // [ts] expect_eq(boxFound, false);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] waitForSelector;
    // [ts] expect_eq(boxFound, true);
    // [ts] });
    // [ts] test("WA-045 should wait for visible", () => {
    // [ts] let divFound = false;
    // [ts] const waitForSelector = page.waitForSelector("div").then(() => divFound = true);
    // [ts] page.setContent(`<div style='display: none; visibility: hidden;'>1</div>`);
    // [ts] expect_eq(divFound, false);
    // [ts] page.evaluate(() => document.querySelector("div").style.removeProperty("display"));
    // [ts] expect_eq(divFound, false);
    // [ts] page.evaluate(() => document.querySelector("div").style.removeProperty("visibility"));
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divFound, true);
    // [ts] });
    // [ts] test("WA-046 should not consider visible when zero-sized", () => {
    // [ts] page.setContent(`<div style='width: 0; height: 0;'>1</div>`);
    // [ts] let error = page.waitForSelector("div", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] page.evaluate(() => document.querySelector("div").style.width = "10px");
    // [ts] error = page.waitForSelector("div", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] page.evaluate(() => document.querySelector("div").style.height = "10px");
    // [ts] expect_eq(page.waitForSelector("div", { timeout: 1000 }), true);
    // [ts] });
    // [ts] test("WA-047 should wait for visible recursively", () => {
    // [ts] let divVisible = false;
    // [ts] const waitForSelector = page.waitForSelector("div#inner").then(() => divVisible = true);
    // [ts] page.setContent(`<div style='display: none; visibility: hidden;'><div id="inner">hi</div></div>`);
    // [ts] expect_eq(divVisible, false);
    // [ts] page.evaluate(() => document.querySelector("div").style.removeProperty("display"));
    // [ts] expect_eq(divVisible, false);
    // [ts] page.evaluate(() => document.querySelector("div").style.removeProperty("visibility"));
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divVisible, true);
    // [ts] });
    // [ts] test("WA-048 should consider outside of viewport visible", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] .cover {
    // [ts] position: fixed;
    // [ts] left: 0;
    // [ts] top: 0;
    // [ts] width: 100px;
    // [ts] height: 100px;
    // [ts] background-color: red;
    // [ts] transform: translateX(-200px);
    // [ts] }
    // [ts] </style>
    // [ts] <div class="cover">cover</div>
    // [ts] `);
    // [ts] const cover = page.locator(".cover");
    // [ts] cover.waitFor({ state: "visible" });
    // [ts] expect(cover).toBeVisible();
    // [ts] });
    // [ts] test("WA-049 hidden should wait for hidden", () => {
    // [ts] let divHidden = false;
    // [ts] page.setContent(`<div style='display: block;'>content</div>`);
    // [ts] const waitForSelector = page.waitForSelector("div", { state: "hidden" }).then(() => divHidden = true);
    // [ts] page.waitForSelector("div"); // do a round trip
    // [ts] expect_eq(divHidden, false);
    // [ts] page.evaluate(() => document.querySelector("div").style.setProperty("visibility", "hidden"));
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divHidden, true);
    // [ts] });
    // [ts] test("WA-050 hidden should wait for display: none", () => {
    // [ts] let divHidden = false;
    // [ts] page.setContent(`<div style='display: block;'>content</div>`);
    // [ts] const waitForSelector = page.waitForSelector("div", { state: "hidden" }).then(() => divHidden = true);
    // [ts] page.waitForSelector("div"); // do a round trip
    // [ts] expect_eq(divHidden, false);
    // [ts] page.evaluate(() => document.querySelector("div").style.setProperty("display", "none"));
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divHidden, true);
    // [ts] });
    // [ts] test("WA-051 hidden should wait for removal", () => {
    // [ts] page.setContent(`<div>content</div>`);
    // [ts] let divRemoved = false;
    // [ts] const waitForSelector = page.waitForSelector("div", { state: "hidden" }).then(() => divRemoved = true);
    // [ts] page.waitForSelector("div"); // do a round trip
    // [ts] expect_eq(divRemoved, false);
    // [ts] page.evaluate(() => document.querySelector("div").remove());
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divRemoved, true);
    // [ts] });
    // [ts] test("WA-052 should return null if waiting to hide non-existing element", () => {
    // [ts] const handle = page.waitForSelector("non-existing", { state: "hidden" });
    // [ts] expect_eq(handle, null);
    // [ts] });
    // [ts] test("WA-053 should respect timeout", () => {
    // [ts] let error: Error | null;
    // [ts] page.waitForSelector("div", { timeout: 3000, state: "attached" }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "waiting for locator('div')") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("WA-054 should have an error message specifically for awaiting an element to be hidden", () => {
    // [ts] page.setContent(`<div>content</div>`);
    // [ts] let error: Error | null;
    // [ts] page.waitForSelector("div", { state: "hidden", timeout: 1000 }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "waiting for locator('div') to be hidden") != null, true);
    // [ts] });
    // [ts] test("WA-055 should respond to node attribute mutation", () => {
    // [ts] let divFound = false;
    // [ts] const waitForSelector = page.waitForSelector(".zombo", { state: "attached" }).then(() => divFound = true);
    // [ts] page.setContent(`<div class='notZombo'></div>`);
    // [ts] expect_eq(divFound, false);
    // [ts] page.evaluate(() => document.querySelector("div").className = "zombo");
    // [ts] expect_eq(waitForSelector, true);
    // [ts] });
    // [ts] test("WA-056 should return the element handle", () => {
    // [ts] const waitForSelector = page.waitForSelector(".zombo");
    // [ts] page.setContent(`<div class='zombo'>anything</div>`);
    // [ts] expect_eq(page.evaluate(x => x.textContent, waitForSelector), "anything");
    // [ts] });
    // [ts] test("WA-057 should have correct stack trace for timeout", () => {
    // [ts] let error;
    // [ts] page.waitForSelector(".zombo", { timeout: 10 }).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, "wait-for-selector") != null, true);
    // [ts] });
    // [ts] test("WA-058 should throw for unknown state option", () => {
    // [ts] page.setContent("<section>test</section>");
    // @ts-expect-error state is not an option of waitForSelector
    // [ts] const error = page.waitForSelector("section", { state: "foo" }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "state: expected one of (attached|detached|visible|hidden)") != null, true);
    // [ts] });
    // [ts] test("WA-059 should throw for visibility option", () => {
    // [ts] page.setContent("<section>test</section>");
    // @ts-expect-error visibility is not an option of waitForSelector
    // [ts] const error = page.waitForSelector("section", { visibility: "hidden" }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "options.visibility is not supported, did you mean options.state?") != null, true);
    // [ts] });
    // [ts] test("WA-060 should throw for true state option", () => {
    // [ts] page.setContent("<section>test</section>");
    // @ts-expect-error state is not an option of waitForSelector
    // [ts] const error = page.waitForSelector("section", { state: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "state: expected one of (attached|detached|visible|hidden)") != null, true);
    // [ts] });
    // [ts] test("WA-061 should throw for false state option", () => {
    // [ts] page.setContent("<section>test</section>");
    // @ts-expect-error state is not an option of waitForSelector
    // [ts] const error = page.waitForSelector("section", { state: false }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "state: expected one of (attached|detached|visible|hidden)") != null, true);
    // [ts] });
    // [ts] test("WA-062 should support >> selector syntax", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] const watchdog = frame.waitForSelector("css=div >> css=span", { state: "attached" });
    // [ts] frame.evaluate(addElement, "br");
    // [ts] frame.evaluate(addElement, "div");
    // [ts] frame.evaluate(() => document.querySelector("div").appendChild(document.createElement("span")));
    // [ts] const eHandle = watchdog;
    // [ts] const tagName = eHandle.getProperty("tagName").then(e => e.jsonValue());
    // [ts] expect_eq(tagName, "SPAN");
    // [ts] });
    // [ts] test("WA-063 should wait for detached if already detached", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // [ts] expect_eq(page.waitForSelector("css=div", { state: "detached" }), null);
    // [ts] });
    // [ts] test("WA-064 should wait for detached", () => {
    // [ts] page.setContent("<section id=\"testAttribute\"><div>43543</div></section>");
    // [ts] let done = false;
    // [ts] const waitFor = page.waitForSelector("css=div", { state: "detached" }).then(() => done = true);
    // [ts] expect_eq(done, false);
    // [ts] page.waitForSelector("css=section");
    // [ts] expect_eq(done, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => div.remove());
    // [ts] expect_eq(waitFor, true);
    // [ts] expect_eq(done, true);
    // [ts] });
    // [ts] test("WA-065 should support some fancy xpath", () => {
    // [ts] page.setContent(`<p>red herring</p><p>hello  world  </p>`);
    // [ts] const waitForXPath = page.waitForSelector("//p[normalize-space(.)=\"hello world\"]");
    // [ts] expect_eq(page.evaluate(x => x.textContent, waitForXPath), "hello  world  ");
    // [ts] });
    // [ts] test("WA-066 should respect timeout xpath", () => {
    // [ts] let error: Error | null;
    // [ts] page.waitForSelector("//div", { state: "attached", timeout: 3000 }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "waiting for locator('//div')") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("WA-067 should run in specified frame xpath", () => {
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] const frame2 = page.frames()[2];
    // [ts] const waitForXPathPromise = frame2.waitForSelector("//div", { state: "attached" });
    // [ts] frame1.evaluate(addElement, "div");
    // [ts] frame2.evaluate(addElement, "div");
    // [ts] const eHandle = waitForXPathPromise;
    // [ts] expect_eq(eHandle.ownerFrame(), frame2);
    // [ts] });
    // [ts] test("WA-068 should throw when frame is detached xpath", () => {
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] let waitError: Error | null;
    // [ts] const waitPromise = frame.waitForSelector("//*[@class=\"box\"]").catch(e => waitError = e);
    // [ts] detachFrame(page, "frame1");
    // [ts] waitPromise;
    // [ts] expect_eq(waitError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, waitError.message, "Frame was detached") != null, true);
    // [ts] });
    // [ts] test("WA-069 should return the element handle xpath", () => {
    // [ts] const waitForXPath = page.waitForSelector("//*[@class=\"zombo\"]");
    // [ts] page.setContent(`<div class='zombo'>anything</div>`);
    // [ts] expect_eq(page.evaluate(x => x.textContent, waitForXPath), "anything");
    // [ts] });
    // [ts] test("WA-070 should allow you to select an element with single slash xpath", () => {
    // [ts] page.setContent(`<div>some text</div>`);
    // [ts] const waitForXPath = page.waitForSelector("//html/body/div");
    // [ts] expect_eq(page.evaluate(x => x.textContent, waitForXPath), "some text");
    // [ts] });
    // [ts] test("WA-071 should correctly handle hidden shadow host", () => {
    // [ts] page.setContent(`
    // [ts] <x-host hidden></x-host>
    // [ts] <script>
    // [ts] const host = document.querySelector('x-host');
    // [ts] const root = host.attachShadow({ mode: 'open' });
    // [ts] const style = document.createElement('style');
    // [ts] style.textContent = ':host([hidden]) { display: none; }';
    // [ts] root.appendChild(style);
    // [ts] const child = document.createElement('div');
    // [ts] child.textContent = 'Find me';
    // [ts] root.appendChild(child);
    // [ts] </script>
    // [ts] `);
    // [ts] expect_eq(page.textContent("div"), "Find me");
    // [ts] page.waitForSelector("div", { state: "hidden" });
    // [ts] });
    // [ts] test("WA-072 should work when navigating before node adoption", () => {
    // [ts] test(mode != "default");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<div>Hello</div>`);
    // [ts] let navigatedOnce = false;
    // [ts] const __testHookBeforeAdoptNode = () => {
    // [ts] if (!navigatedOnce) {
    // [ts] navigatedOnce = true;
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] }
    // [ts] };
    // [ts] const div = page.waitForSelector("div", { __testHookBeforeAdoptNode });
    // This text is coming from /one-style.html
    // [ts] expect_eq(div.textContent(), "hello, world!");
    // [ts] });
    // [ts] test("WA-073 should fail when navigating while on handle", () => {
    // [ts] test(mode != "default");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<div>Hello</div>`);
    // [ts] let navigatedOnce = false;
    // [ts] const __testHookBeforeAdoptNode = () => {
    // [ts] if (!navigatedOnce) {
    // [ts] navigatedOnce = true;
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] }
    // [ts] };
    // [ts] const body = page.waitForSelector("body");
    // [ts] const error = body.waitForSelector("div", { __testHookBeforeAdoptNode }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for locator('div') to be visible`) != null, true);
    // [ts] });
    // [ts] test("WA-074 should fail if element handle was detached while waiting", () => {
    // [ts] page.setContent(`<button>hello</button>`);
    // [ts] const button = page.$("button");
    // [ts] const promise = button.waitForSelector("something").catch(e => e);
    // [ts] page.waitForTimeout(100);
    // [ts] page.evaluate(() => document.body.innerText = "");
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not attached to the DOM") != null, true);
    // [ts] });
    // [ts] test("WA-075 should succeed if element handle was detached while waiting for hidden", () => {
    // [ts] page.setContent(`<button>hello</button>`);
    // [ts] const button = page.$("button");
    // [ts] const promise = button.waitForSelector("something", { state: "hidden" });
    // [ts] page.waitForTimeout(1000);
    // [ts] page.evaluate(() => document.body.innerText = "");
    // [ts] promise;
    // [ts] });
    // [ts] test("WA-076 should succeed if element handle was detached while waiting for detached", () => {
    // [ts] page.setContent(`<button>hello</button>`);
    // [ts] const button = page.$("button");
    // [ts] const promise = button.waitForSelector("something", { state: "detached" });
    // [ts] page.waitForTimeout(1000);
    // [ts] page.evaluate(() => document.body.innerText = "");
    // [ts] promise;
    // [ts] });
}
