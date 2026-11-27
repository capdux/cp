// Migrated from TypeScript (page/page-click.spec.ts) via cap from -ts

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
import { attachFrame, detachFrame } from "../config/utils.cp";
// [cap-migrate] import { Page } from "cap:test";

fn giveItAChanceToClick(page: Page) { return rafraf(page, 5); }


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CL-001 should click the button @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-002 should click button inside frameset", () => {
    // [ts] page.goto(server.PREFIX + "/frames/frameset.html");
    // [ts] const frameElement = page.$("frame");
    // [ts] frameElement.evaluate((frame) => frame.src = "/input/button.html");
    // [ts] const frame = frameElement.contentFrame();
    // [ts] frame.click("button");
    // [ts] expect_eq(frame.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-003 should issue clicks in parallel in page and popup", () => {
    // [ts] page.goto(server.PREFIX + "/counter.html");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window.open("/counter.html")),
    // [ts] ]);
    // [ts] const clickPromises = [];
    // [ts] let i = 0;
    // [ts] while (i < 21) { // [cap-migrate] for-step: ++i
    // [ts] if (i % 3 == 0)
    // [ts] clickPromises.push(popup.locator("button").click());
    // [ts] else {
    // [ts] clickPromises.push(page.locator("button").click());
    // [ts] }
    // [ts] joinAll(clickPromises); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(page.evaluate(() => window["count"]), 14);
    // [ts] expect_eq(popup.evaluate(() => window["count"]), 7);
    // [ts] });
    // [ts] test("CL-004 should click svg", () => {
    // [ts] page.setContent(`
    // [ts] <svg height="100" width="100">
    // [ts] <circle onclick="window.__CLICKED=42" cx="50" cy="50" r="40" stroke="black" stroke-width="3" fill="red" />
    // [ts] </svg>
    // [ts] `);
    // [ts] page.click("circle");
    // [ts] expect_eq(page.evaluate("__CLICKED"), 42);
    // [ts] });
    // [ts] test("CL-005 should click the button if window.Node is removed", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => delete window.Node);
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // @see https://github.com/GoogleChrome/puppeteer/issues/4281
    // [ts] test("CL-006 should click on a span with an inline element inside", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] span::before {
    // [ts] content: 'q';
    // [ts] }
    // [ts] </style>
    // [ts] <span onclick='window.CLICKED=42'></span>
    // [ts] `);
    // [ts] page.click("span");
    // [ts] expect_eq(page.evaluate("CLICKED"), 42);
    // [ts] });
    // [ts] test("CL-007 should click the aligned 1x1 div", () => {
    // [ts] page.setContent(`<div style="width: 1px; height: 1px;" onclick="window.__clicked = true"></div>`);
    // [ts] page.click("div");
    // [ts] expect_eq(page.evaluate("window.__clicked"), true);
    // [ts] });
    // [ts] test("CL-008 should click the half-aligned 1x1 div", () => {
    // [ts] page.setContent(`<div style="margin-left: 20.5px; margin-top: 11.5px; width: 1px; height: 1px;" onclick="window.__clicked = true"></div>`);
    // [ts] page.click("div");
    // [ts] expect_eq(page.evaluate("window.__clicked"), true);
    // [ts] });
    // [ts] test("CL-009 should click the unaligned 1x1 div v1", () => {
    // [ts] page.setContent(`<div style="margin-left: 20.23px; margin-top: 11.65px; width: 1px; height: 1px;" onclick="window.__clicked = true"></div>`);
    // [ts] page.click("div");
    // [ts] expect_eq(page.evaluate("window.__clicked"), true);
    // [ts] });
    // [ts] test("CL-010 should click the unaligned 1x1 div v2", () => {
    // [ts] page.setContent(`<div style="margin-left: 20.68px; margin-top: 11.13px; width: 1px; height: 1px;" onclick="window.__clicked = true"></div>`);
    // [ts] page.click("div");
    // [ts] expect_eq(page.evaluate("window.__clicked"), true);
    // [ts] });
    // [ts] test("CL-011 should click the unaligned 1x1 div v3", () => {
    // [ts] page.setContent(`<div style="margin-left: 20.68px; margin-top: 11.52px; width: 1px; height: 1px;" onclick="window.__clicked = true"></div>`);
    // [ts] page.click("div");
    // [ts] expect_eq(page.evaluate("window.__clicked"), true);
    // [ts] });
    // [ts] test("CL-012 should click the unaligned 1x1 div v4", () => {
    // [ts] page.setContent(`<div style="margin-left: 20.15px; margin-top: 11.24px; width: 1px; height: 1px;" onclick="window.__clicked = true"></div>`);
    // [ts] page.click("div");
    // [ts] expect_eq(page.evaluate("window.__clicked"), true);
    // [ts] });
    // [ts] test("CL-013 should click the button after navigation ", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-014 should click the button after a cross origin navigation ", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-015 should click when one of inline box children is outside of viewport", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] i {
    // [ts] position: absolute;
    // [ts] top: -1000px;
    // [ts] }
    // [ts] </style>
    // [ts] <span onclick='window.CLICKED = 42;'><i>woof</i><b>doggo</b></span>
    // [ts] `);
    // [ts] page.click("span");
    // [ts] expect_eq(page.evaluate("CLICKED"), 42);
    // [ts] });
    // [ts] test("CL-016 should select the text by triple clicking", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const text = "This is the text that we are going to try to select. Let's see how it goes.";
    // [ts] page.fill("textarea", text);
    // [ts] page.click("textarea", { clickCount: 3 });
    // [ts] expect(page.evaluate(() => {
    // [ts] const textarea = document.querySelector("textarea");
    // [ts] return textarea.value.substring(textarea.selectionStart, textarea.selectionEnd);
    // [ts] })).toBe(text);
    // [ts] });
    // [ts] test("CL-017 should click offscreen buttons", () => {
    // [ts] page.goto(server.PREFIX + "/offscreenbuttons.html");
    // [ts] const messages = [];
    // [ts] page.on("console", msg => messages.push(msg.text()));
    // [ts] let i = 0;
    // [ts] while (i < 11) { // [cap-migrate] for-step: ++i
    // We might've scrolled to click a button - reset to (0, 0).
    // [ts] page.evaluate(() => window.scrollTo(0, 0));
    // [ts] page.click(`#btn${i}`);
    // [ts] }
    // [ts] expect(messages).toEqual([
    // [ts] "button #0 clicked",
    // [ts] "button #1 clicked",
    // [ts] "button #2 clicked",
    // [ts] "button #3 clicked",
    // [ts] "button #4 clicked",
    // [ts] "button #5 clicked",
    // [ts] "button #6 clicked",
    // [ts] "button #7 clicked",
    // [ts] "button #8 clicked",
    // [ts] "button #9 clicked",
    // [ts] "button #10 clicked"
    // [ts] ]);
    // [ts] });
    // [ts] test("CL-018 should waitFor visible when already visible", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-019 should not wait with force", () => {
    // [ts] let error = null;
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', b => b.style.display = 'none');
    // [ts] page.click("button", { force: true }).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not visible") != null, true);
    // [ts] expect_eq(page.evaluate("result"), "Was not clicked");
    // [ts] });
    // [ts] test("CL-020 should waitFor display:none to be gone", () => {
    // [ts] let done = false;
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', b => b.style.display = 'none');
    // [ts] const clicked = page.click("button", { timeout: 0 }).then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(page.evaluate("result"), "Was not clicked");
    // [ts] expect_eq(done, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', b => b.style.display = 'block');
    // [ts] clicked;
    // [ts] expect_eq(done, true);
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-021 should waitFor visibility:hidden to be gone", () => {
    // [ts] let done = false;
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', b => b.style.visibility = 'hidden');
    // [ts] const clicked = page.click("button", { timeout: 0 }).then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(page.evaluate("result"), "Was not clicked");
    // [ts] expect_eq(done, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', b => b.style.visibility = 'visible');
    // [ts] clicked;
    // [ts] expect_eq(done, true);
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-022 should waitFor visible when parent is hidden", () => {
    // [ts] let done = false;
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', b => b.parentElement.style.display = 'none');
    // [ts] const clicked = page.click("button", { timeout: 0 }).then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(done, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', b => b.parentElement.style.display = 'block');
    // [ts] clicked;
    // [ts] expect_eq(done, true);
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-023 should click wrapped links", () => {
    // [ts] page.goto(server.PREFIX + "/wrappedlink.html");
    // [ts] page.click("a");
    // [ts] expect_eq(page.evaluate("__clicked"), true);
    // [ts] });
    // [ts] test("CL-024 should click on checkbox input and toggle", () => {
    // [ts] page.goto(server.PREFIX + "/input/checkbox.html");
    // [ts] expect_eq(page.evaluate(() => window["result"].check), null);
    // [ts] page.click("input#agree");
    // [ts] expect_eq(page.evaluate(() => window["result"].check), true);
    // [ts] let events: []string = page.evaluate(() => window["result"].events);
    // [ts] if (!headless)
    // [ts] events = events.filter(e => e != "mouseout" && e != "mouseleave");
    // [ts] expect(events).toEqual([
    // [ts] "mouseover",
    // [ts] "mouseenter",
    // [ts] "mousemove",
    // [ts] "mousedown",
    // [ts] "mouseup",
    // [ts] "click",
    // [ts] "input",
    // [ts] "change",
    // [ts] ]);
    // [ts] page.click("input#agree");
    // [ts] expect_eq(page.evaluate(() => window["result"].check), false);
    // [ts] });
    // [ts] test("CL-025 should click on checkbox label and toggle", () => {
    // [ts] page.goto(server.PREFIX + "/input/checkbox.html");
    // [ts] expect_eq(page.evaluate(() => window["result"].check), null);
    // [ts] page.click("label[for=\"agree\"]");
    // [ts] expect_eq(page.evaluate(() => window["result"].check), true);
    // [ts] expect(page.evaluate(() => window["result"].events)).toEqual([
    // [ts] "click",
    // [ts] "input",
    // [ts] "change",
    // [ts] ]);
    // [ts] page.click("label[for=\"agree\"]");
    // [ts] expect_eq(page.evaluate(() => window["result"].check), false);
    // [ts] });
    // [ts] test("CL-026 should scroll and click the button", () => {
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.click("#button-5");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("#button-5").textContent), "clicked");
    // [ts] page.click("#button-80");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("#button-80").textContent), "clicked");
    // [ts] });
    // [ts] test("CL-027 should scroll and click the button with smooth scroll behavior", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/12370" });
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.addStyleTag({ content: "html { scroll-behavior: smooth; }" });
    // [ts] let i = 0;
    // [ts] while (i < 5) { // [cap-migrate] for-step: i++
    // [ts] page.click("#button-80");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("#button-80").textContent), "clicked");
    // [ts] page.click("#button-20");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("#button-20").textContent), "clicked");
    // [ts] }
    // [ts] });
    // [ts] test("CL-028 should double click the button", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] window["double"] = false;
    // [ts] const button = document.querySelector("button");
    // [ts] button.addEventListener("dblclick", event => {
    // [ts] window["double"] = true;
    // [ts] });
    // [ts] });
    // [ts] page.dblclick("button");
    // [ts] expect_eq(page.evaluate("double"), true);
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-029 should click a partially obscured button", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] const button = document.querySelector("button");
    // [ts] button.textContent = "Some really long text that will go offscreen";
    // [ts] button.style.position = "absolute";
    // [ts] button.style.left = "368px";
    // [ts] });
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("CL-030 should click a rotated button", () => {
    // [ts] page.goto(server.PREFIX + "/input/rotatedButton.html");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-031 should fire contextmenu event on right click", () => {
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.click("#button-8", { button: "right" });
    // [ts] expect_eq(page.evaluate(() => document.querySelector("#button-8").textContent), "context menu");
    // [ts] });
    // [ts] test("CL-032 should click links which cause navigation", () => {
    // @see https://github.com/GoogleChrome/puppeteer/issues/206
    // [ts] page.setContent(`<a href="${server.EMPTY_PAGE}">empty.html</a>`);
    // This await should not hang.
    // [ts] page.click("a");
    // [ts] });
    // [ts] test("CL-033 should not wait for navigation after clicking a link answered with 204", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43279" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/204", (req, res) => {
    // [ts] res.statusCode = 204;
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<a href="${server.PREFIX}/204">link</a>`);
    // [ts] page.click("a", { timeout: 5000 });
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("CL-034 should click the button inside an iframe", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<div style=\"width:100px;height:100px\">spacer</div>");
    // [ts] attachFrame(page, "button-test", server.PREFIX + "/input/button.html");
    // [ts] const frame = page.frames()[1];
    // [ts] const button = frame.$("button");
    // [ts] button.click();
    // [ts] expect_eq(frame.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("CL-035 should click the button with fixed position inside an iframe", () => {
    // [ts] it.fixme(browserName == "chromium");
    // @see https://github.com/GoogleChrome/puppeteer/issues/4110
    // @see https://bugs.chromium.org/p/chromium/issues/detail?id=986390
    // @see https://chromium-review.googlesource.com/c/chromium/src/+/1742784
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] page.setContent("<div style=\"width:100px;height:2000px\">spacer</div>");
    // [ts] attachFrame(page, "button-test", server.CROSS_PROCESS_PREFIX + "/input/button.html");
    // [ts] const frame = page.frames()[1];
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await frame.$eval('button', button => button.style.setProperty('position', 'fixed'));
    // [ts] frame.click("button");
    // [ts] expect_eq(frame.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("CL-036 should click the button behind sticky header", () => {
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
    // [ts] background: red;
    // [ts] height: 160px;
    // [ts] }
    // [ts] </style>
    // [ts] <div class=fixed></div>
    // [ts] <ol>
    // [ts] <li>hi1</li><li>hi2</li><li>hi3</li><li>hi4</li><li>hi5</li><li>hi6</li><li>hi7</li><li>hi8</li>
    // [ts] <li id=target onclick="window.__clicked = true">hi9</li>
    // [ts] <li>hi10</li><li>hi11</li><li>hi12</li><li>hi13</li><li id=li14>hi14</li>
    // [ts] </ol>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('#li14', e => e.scrollIntoView());
    // [ts] page.click("#target");
    // [ts] expect_eq(page.evaluate(() => window["__clicked"]), true);
    // [ts] });
    // [ts] test("CL-037 should click the button behind position:absolute header", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36339" },
    // [ts] }, (_p) => {
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
    // [ts] height: 100vh;
    // [ts] overflow: scroll;
    // [ts] padding-top: 160px;
    // [ts] }
    // [ts] body {
    // [ts] position: relative;
    // [ts] }
    // [ts] div.fixed {
    // [ts] position: absolute;
    // [ts] top: 0;
    // [ts] z-index: 1001;
    // [ts] width: 100%;
    // [ts] background: red;
    // [ts] height: 160px;
    // [ts] }
    // [ts] </style>
    // [ts] <ol>
    // [ts] <li>hi1</li><li>hi2</li><li>hi3</li><li>hi4</li><li>hi5</li><li>hi6</li><li>hi7</li><li>hi8</li>
    // [ts] <li id=target onclick="window.__clicked = true">hi9</li>
    // [ts] <li>hi10</li><li>hi11</li><li>hi12</li><li>hi13</li><li id=li14>hi14</li>
    // [ts] </ol>
    // [ts] <div class=fixed>Overlay</div>
    // [ts] `);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('ol', e => {
    // [ts-skip] const target = document.querySelector('#target') as HTMLElement;
    // [ts-skip] e.scrollTo({ top: target.offsetTop, behavior: 'instant' });
    // [ts-skip] });
    // [ts] page.click("#target");
    // [ts] expect_eq(page.evaluate(() => window["__clicked"]), true);
    // [ts] });
    // [ts] test("CL-038 should click the button with px border with offset", () => {
    // [ts] test(isFrozenWebkit, "needs recent webkit");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => button.style.borderWidth = '8px');
    // [ts] page.click("button", { position: { x: 20, y: 10 } });
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // Safari reports border-relative offsetX/offsetY.
    // [ts] expect_eq(page.evaluate("offsetX"), 20);
    // [ts] expect_eq(page.evaluate("offsetY"), 10);
    // [ts] });
    // [ts] test("CL-039 should click the button with em border with offset", () => {
    // [ts] test(isFrozenWebkit, "needs recent webkit");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => button.style.borderWidth = '2em');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => button.style.fontSize = '12px');
    // [ts] page.click("button", { position: { x: 20, y: 10 } });
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // Safari reports border-relative offsetX/offsetY.
    // [ts] expect_eq(page.evaluate("offsetX"), 20);
    // [ts] expect_eq(page.evaluate("offsetY"), 10);
    // [ts] });
    // [ts] test("CL-040 should click a very large button with offset", () => {
    // [ts] it.fixme(isAndroid, "Failed to scroll to a particular point");
    // [ts] test(isFrozenWebkit, "needs recent webkit");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => button.style.borderWidth = '8px');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => button.style.height = button.style.width = '2000px');
    // [ts] page.click("button", { position: { x: 1900, y: 1910 } });
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // Safari reports border-relative offsetX/offsetY.
    // [ts] expect_eq(page.evaluate("offsetX"), 1900);
    // [ts] expect_eq(page.evaluate("offsetY"), 1910);
    // [ts] });
    // [ts] test("CL-041 should click a button in scrolling container with offset", () => {
    // [ts] it.fixme(isAndroid, "Failed to scroll to a particular point");
    // [ts] test(isFrozenWebkit, "needs recent webkit");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
    // [ts-skip] const container = document.createElement('div');
    // [ts-skip] container.style.overflow = 'auto';
    // [ts-skip] container.style.width = '200px';
    // [ts-skip] container.style.height = '200px';
    // [ts-skip] button.parentElement.insertBefore(container, button);
    // [ts-skip] container.appendChild(button);
    // [ts-skip] button.style.height = '2000px';
    // [ts-skip] button.style.width = '2000px';
    // [ts-skip] button.style.borderWidth = '8px';
    // [ts-skip] });
    // [ts] page.click("button", { position: { x: 1900, y: 1910 } });
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // Safari reports border-relative offsetX/offsetY.
    // [ts] expect_eq(page.evaluate("offsetX"), 1900);
    // [ts] expect_eq(page.evaluate("offsetY"), 1910);
    // [ts] });
    // [ts] test("CL-042 should wait for stable position", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
    // [ts-skip] button.style.transition = 'margin 500ms linear 0s';
    // [ts-skip] button.style.marginLeft = '200px';
    // [ts-skip] button.style.borderWidth = '0';
    // [ts-skip] button.style.width = '200px';
    // [ts-skip] button.style.height = '20px';
    // [ts-skip] // Set display to "block" - otherwise Firefox layouts with non-even
    // [ts-skip] // values on Linux.
    // [ts-skip] button.style.display = 'block';
    // [ts-skip] document.body.style.margin = '0';
    // [ts-skip] });
    // rafraf for Firefox to kick in the animation.
    // [ts] rafraf(page);
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] expect_eq(page.evaluate("pageX"), 300);
    // [ts] expect_eq(page.evaluate("pageY"), 10);
    // [ts] });
    // [ts] test("CL-043 should wait for becoming hit target", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
    // [ts-skip] button.style.borderWidth = '0';
    // [ts-skip] button.style.width = '200px';
    // [ts-skip] button.style.height = '20px';
    // [ts-skip] document.body.style.margin = '0';
    // [ts-skip] document.body.style.position = 'relative';
    // [ts-skip] const flyOver = document.createElement('div');
    // [ts-skip] flyOver.className = 'flyover';
    // [ts-skip] flyOver.style.position = 'absolute';
    // [ts-skip] flyOver.style.width = '400px';
    // [ts-skip] flyOver.style.height = '20px';
    // [ts-skip] flyOver.style.left = '-200px';
    // [ts-skip] flyOver.style.top = '0';
    // [ts-skip] flyOver.style.background = 'red';
    // [ts-skip] document.body.appendChild(flyOver);
    // [ts-skip] });
    // [ts] let clicked = false;
    // [ts] const clickPromise = page.click("button").then(() => clicked = true);
    // [ts] expect_eq(clicked, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('.flyover', flyOver => flyOver.style.left = '0');
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(clicked, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('.flyover', flyOver => flyOver.style.left = '200px');
    // [ts] clickPromise;
    // [ts] expect_eq(clicked, true);
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Clicked");
    // [ts] });
    // [ts] test("CL-044 should wait for becoming hit target with trial run", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
    // [ts-skip] button.style.borderWidth = '0';
    // [ts-skip] button.style.width = '200px';
    // [ts-skip] button.style.height = '20px';
    // [ts-skip] document.body.style.margin = '0';
    // [ts-skip] document.body.style.position = 'relative';
    // [ts-skip] const flyOver = document.createElement('div');
    // [ts-skip] flyOver.className = 'flyover';
    // [ts-skip] flyOver.style.position = 'absolute';
    // [ts-skip] flyOver.style.width = '400px';
    // [ts-skip] flyOver.style.height = '20px';
    // [ts-skip] flyOver.style.left = '-200px';
    // [ts-skip] flyOver.style.top = '0';
    // [ts-skip] flyOver.style.background = 'red';
    // [ts-skip] document.body.appendChild(flyOver);
    // [ts-skip] });
    // [ts] let clicked = false;
    // [ts] const clickPromise = page.click("button", { trial: true }).then(() => clicked = true);
    // [ts] expect_eq(clicked, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('.flyover', flyOver => flyOver.style.left = '0');
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(clicked, false);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('.flyover', flyOver => flyOver.style.left = '200px');
    // [ts] clickPromise;
    // [ts] expect_eq(clicked, true);
    // Should not actually click.
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Was not clicked");
    // [ts] });
    // [ts] test("CL-045 trial run should work with short timeout", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => button.disabled = true);
    // [ts] const error = page.click("button", { trial: true, timeout: 2000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "click action (trial run)") != null, true);
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Was not clicked");
    // [ts] });
    // [ts] test("CL-046 trial run should not click", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button", { trial: true });
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Was not clicked");
    // [ts] });
    // [ts] test("CL-047 trial run should not double click", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] window["double"] = false;
    // [ts] const button = document.querySelector("button");
    // [ts] button.addEventListener("dblclick", event => {
    // [ts] window["double"] = true;
    // [ts] });
    // [ts] });
    // [ts] page.dblclick("button", { trial: true });
    // [ts] expect_eq(page.evaluate("double"), false);
    // [ts] expect_eq(page.evaluate("result"), "Was not clicked");
    // [ts] });
    // [ts] test("CL-048 should fail when obscured and not waiting for hit target", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.position = "relative";
    // [ts] const blocker = document.createElement("div");
    // [ts] blocker.style.position = "absolute";
    // [ts] blocker.style.width = "400px";
    // [ts] blocker.style.height = "20px";
    // [ts] blocker.style.left = "0";
    // [ts] blocker.style.top = "0";
    // [ts] document.body.appendChild(blocker);
    // [ts] });
    // [ts] button.click({ force: true });
    // [ts] expect_eq(page.evaluate(() => window["result"]), "Was not clicked");
    // [ts] });
    // [ts] test("CL-049 should wait for button to be enabled", () => {
    // [ts] page.setContent("<button onclick=\"window.__CLICKED=true;\" disabled><span>Click target</span></button>");
    // [ts] let done = false;
    // [ts] const clickPromise = page.click("text=Click target").then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(page.evaluate("window.__CLICKED"), null);
    // [ts] expect_eq(done, false);
    // [ts] page.evaluate(() => document.querySelector("button").removeAttribute("disabled"));
    // [ts] clickPromise;
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-050 should wait for element to become non-inert", () => {
    // [ts] page.setContent("<div inert id=container><button onclick=\"window.__CLICKED=true;\"><span>Click target</span></button></div>");
    // [ts] let done = false;
    // [ts] const clickPromise = page.click("text=Click target").then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(page.evaluate("window.__CLICKED"), null);
    // [ts] expect_eq(done, false);
    // [ts] page.evaluate(() => document.getElementById("container").removeAttribute("inert"));
    // [ts] clickPromise;
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-052 should wait for input to be enabled", () => {
    // [ts] page.setContent("<input onclick=\"window.__CLICKED=true;\" disabled>");
    // [ts] let done = false;
    // [ts] const clickPromise = page.click("input").then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(page.evaluate("window.__CLICKED"), null);
    // [ts] expect_eq(done, false);
    // [ts] page.evaluate(() => document.querySelector("input").removeAttribute("disabled"));
    // [ts] clickPromise;
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-053 should wait for select to be enabled", () => {
    // [ts] page.setContent(`
    // [ts] <select disabled><option selected>Hello</option></select>
    // [ts] <script>
    // [ts] document.querySelector('select').addEventListener('mousedown', event => {
    // [ts] window.__CLICKED=true;
    // [ts] event.preventDefault();
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] let done = false;
    // [ts] const clickPromise = page.click("select").then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(page.evaluate("window.__CLICKED"), null);
    // [ts] expect_eq(done, false);
    // [ts] page.evaluate(() => document.querySelector("select").removeAttribute("disabled"));
    // [ts] clickPromise;
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-054 should click disabled div", () => {
    // [ts] page.setContent("<div onclick=\"window.__CLICKED=true\" disabled>Click target</div>");
    // [ts] page.click("text=Click target");
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-055 should wait for BUTTON to be clickable when it has pointer-events:none", () => {
    // [ts] page.setContent("<button onclick=\"window.__CLICKED=true\" style=\"pointer-events:none\"><span>Click target</span></button>");
    // [ts] let done = false;
    // [ts] const clickPromise = page.click("text=Click target").then(() => done = true);
    // [ts] giveItAChanceToClick(page);
    // [ts] expect_eq(page.evaluate("window.__CLICKED"), null);
    // [ts] expect_eq(done, false);
    // [ts] page.evaluate(() => document.querySelector("button").style.removeProperty("pointer-events"));
    // [ts] clickPromise;
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-056 should wait for LABEL to be clickable when it has pointer-events:none", () => {
    // [ts] page.setContent("<label onclick=\"window.__CLICKED=true\" style=\"pointer-events:none\"><span>Click target</span></label>");
    // [ts] const clickPromise = page.click("text=Click target");
    // Do a few roundtrips to the page.
    // [ts] let i = 0;
    // [ts] while (i < 5) // [cap-migrate] for-step: ++i
    // [ts] expect_eq(page.evaluate("window.__CLICKED"), null);
    // remove `pointer-events: none` css from button.
    // [ts] page.evaluate(() => document.querySelector("label").style.removeProperty("pointer-events"));
    // [ts] clickPromise;
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-057 should update modifiers correctly", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button", { modifiers: ["Shift"] });
    // [ts] expect_eq(page.evaluate("shiftKey"), true);
    // [ts] page.click("button", { modifiers: [] });
    // [ts] expect_eq(page.evaluate("shiftKey"), false);
    // [ts] page.keyboard.down("Shift");
    // [ts] page.click("button", { modifiers: [] });
    // [ts] expect_eq(page.evaluate("shiftKey"), false);
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("shiftKey"), true);
    // [ts] page.keyboard.up("Shift");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("shiftKey"), false);
    // [ts] });
    // [ts] test("CL-058 should click an offscreen element when scroll-behavior is smooth", () => {
    // [ts] page.setContent(`
    // [ts] <div style="border: 1px solid black; height: 500px; overflow: auto; width: 500px; scroll-behavior: smooth">
    // [ts] <button style="margin-top: 2000px" onClick="window.clicked = true">hi</button>
    // [ts] </div>
    // [ts] `);
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("window.clicked"), true);
    // [ts] });
    // [ts] test("CL-059 should report nice error when element is detached and force-clicked", () => {
    // [ts] page.goto(server.PREFIX + "/input/animating-button.html");
    // [ts] page.evaluate("addButton()");
    // [ts] const handle = page.$("button");
    // [ts] page.evaluate("stopButton(true)");
    // [ts] const promise = handle.click({ force: true }).catch(e => e);
    // [ts] const error = promise;
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not attached to the DOM") != null, true);
    // [ts] });
    // [ts] test("CL-060 should fail when element detaches after animation", () => {
    // [ts] page.goto(server.PREFIX + "/input/animating-button.html");
    // [ts] page.evaluate("addButton()");
    // [ts] const handle = page.$("button");
    // [ts] const promise = handle.click().catch(e => e);
    // [ts] page.evaluate("stopButton(true)");
    // [ts] const error = promise;
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not attached to the DOM") != null, true);
    // [ts] });
    // [ts] test("CL-061 should retry when element detaches after animation", () => {
    // [ts] page.goto(server.PREFIX + "/input/animating-button.html");
    // [ts] page.evaluate("addButton()");
    // [ts] let clicked = false;
    // [ts] const promise = page.click("button").then(() => clicked = true);
    // [ts] expect_eq(clicked, false);
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] page.evaluate("stopButton(true)");
    // [ts] page.evaluate("addButton()");
    // [ts] expect_eq(clicked, false);
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] page.evaluate("stopButton(true)");
    // [ts] page.evaluate("addButton()");
    // [ts] expect_eq(clicked, false);
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] page.evaluate("stopButton(false)");
    // [ts] promise;
    // [ts] expect_eq(clicked, true);
    // [ts] expect_eq(page.evaluate("clicked"), true);
    // [ts] });
    // [ts] test("CL-062 should retry when element is animating from outside the viewport", () => {
    // [ts] page.setContent(`<style>
    // [ts] @keyframes move {
    // [ts] from { left: -300px; }
    // [ts] to { left: 0; }
    // [ts] }
    // [ts] button {
    // [ts] position: absolute;
    // [ts] left: -300px;
    // [ts] top: 0;
    // [ts] bottom: 0;
    // [ts] width: 200px;
    // [ts] }
    // [ts] button.animated {
    // [ts] animation: 1s linear 1s move forwards;
    // [ts] }
    // [ts] </style>
    // [ts] <div style="position: relative; width: 300px; height: 300px;">
    // [ts] <button onclick="window.clicked=true"></button>
    // [ts] </div>
    // [ts] `);
    // [ts] const handle = page.$("button");
    // [ts] const promise = handle.click();
    // [ts] handle.evaluate(button => button.className = "animated");
    // [ts] promise;
    // [ts] expect_eq(page.evaluate("clicked"), true);
    // [ts] });
    // [ts] test("CL-063 should fail when element is animating from outside the viewport with force", () => {
    // [ts] page.setContent(`<style>
    // [ts] @keyframes move {
    // [ts] from { left: -300px; }
    // [ts] to { left: 0; }
    // [ts] }
    // [ts] button {
    // [ts] position: absolute;
    // [ts] left: -300px;
    // [ts] top: 0;
    // [ts] bottom: 0;
    // [ts] width: 200px;
    // [ts] }
    // [ts] button.animated {
    // [ts] animation: 1s linear 1s move forwards;
    // [ts] }
    // [ts] </style>
    // [ts] <div style="position: relative; width: 300px; height: 300px;">
    // [ts] <button onclick="window.clicked=true"></button>
    // [ts] </div>
    // [ts] `);
    // [ts] const handle = page.$("button");
    // [ts] const promise = handle.click({ force: true }).catch(e => e);
    // [ts] handle.evaluate(button => button.className = "animated");
    // [ts] const error = promise;
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is outside of the viewport") != null, true);
    // [ts] });
    // [ts] test("CL-064 should dispatch microtasks in order", () => {
    // [ts] page.setContent(`
    // [ts] <button id=button>Click me</button>
    // [ts] <script>
    // [ts] let mutationCount = 0;
    // [ts] const observer = new MutationObserver((mutationsList, observer) => {
    // [ts] for(let mutation of mutationsList)
    // [ts] ++mutationCount;
    // [ts] });
    // [ts] observer.observe(document.body, { attributes: true, childList: true, subtree: true });
    // [ts] button.addEventListener('mousedown', () => {
    // [ts] mutationCount = 0;
    // [ts] document.body.appendChild(document.createElement('div'));
    // [ts] });
    // [ts] button.addEventListener('mouseup', () => {
    // [ts] window['result'] = mutationCount;
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate(() => window["result"]), 1);
    // [ts] });
    // [ts] test("CL-065 should click the button when window.innerWidth is corrupted", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.evaluate(() => Object.defineProperty(window, 'innerWidth', { value: 0 }));
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-066 should click zero-sized input by label", () => {
    // [ts] page.setContent(`
    // [ts] <label>
    // [ts] Click me
    // [ts] <input onclick="window.__clicked=true" style="width:0;height:0;padding:0;margin:0;border:0;">
    // [ts] </label>
    // [ts] `);
    // [ts] page.click("text=Click me");
    // [ts] expect_eq(page.evaluate("window.__clicked"), true);
    // [ts] });
    // [ts] test("CL-067 should not throw protocol error when navigating during the click", () => {
    // [ts] test(mode != "default");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] let firstTime = true;
    // [ts] const __testHookBeforeStable = () => {
    // [ts] if (!firstTime)
    // [ts] return;
    // [ts] firstTime = false;
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] };
    // [ts] page.click("button", { __testHookBeforeStable });
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("CL-068 should retry when navigating during the click", () => {
    // [ts] test(mode != "default");
    // [ts] it.fixme(isAndroid);
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] let firstTime = true;
    // [ts] const __testHookBeforeStable = () => {
    // [ts] if (!firstTime)
    // [ts] return;
    // [ts] firstTime = false;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] };
    // [ts] const error = page.click("button", { __testHookBeforeStable, timeout: 2000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element was detached from the DOM, retrying") != null, true);
    // [ts] });
    // [ts] test("CL-069 should not hang when frame is detached", () => {
    // [ts] test(mode != "default");
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] frame.goto(server.PREFIX + "/input/button.html");
    // Start moving the button.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await frame.$eval('button', button => {
    // [ts-skip] button.style.transition = 'margin 5s linear 0s';
    // [ts-skip] button.style.marginLeft = '200px';
    // [ts-skip] });
    // [ts] let resolveDetachPromise;
    // [ts] const detachPromise = Promise.init(resolve => resolveDetachPromise = resolve);
    // [ts] let firstTime = true;
    // [ts] const __testHookBeforeStable = () => {
    // Detach the frame after "waiting for stable" has started.
    // [ts] if (!firstTime)
    // [ts] return;
    // [ts] firstTime = false;
    // [ts] setTimeout(() => {
    // [ts] detachFrame(page, "frame1");
    // [ts] resolveDetachPromise();
    // [ts] }, 1000);
    // [ts] };
    // [ts] const promise = frame.click("button", { __testHookBeforeStable }).catch(e => e);
    // [ts] detachPromise;
    // [ts] const error = promise;
    // [ts] expect_eq(error, true);
    // [ts] expect(error.message).toMatch("frame got detached|Frame was detached");
    // [ts] });
    // [ts] test("CL-070 should climb dom for inner label with pointer-events:none", () => {
    // [ts] page.setContent("<button onclick=\"window.__CLICKED=true;\"><label style=\"pointer-events:none\">Click target</label></button>");
    // [ts] page.click("text=Click target");
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-071 should climb up to [role=button]", () => {
    // [ts] page.setContent("<div role=button onclick=\"window.__CLICKED=true;\"><div style=\"pointer-events:none\"><span><div>Click target</div></span></div>");
    // [ts] page.click("text=Click target");
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-072 should climb up to a anchor", () => {
    // For Firefox its not allowed to return anything: https://bugzilla.mozilla.org/show_bug.cgi?id=1392046
    // Note the intermediate div - it is necessary, otherwise <a><non-clickable/></a> is not recognized as a clickable link.
    // [ts] page.setContent(`<a href="#" onclick="window.__CLICKED=true" id="outer"><div id="intermediate"><div id="inner" style="pointer-events: none">Inner</div></div></a>`);
    // [ts] page.click("#inner");
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-073 should climb up to a [role=link]", () => {
    // [ts] page.setContent(`<div role=link onclick="window.__CLICKED=true" id="outer"><div id="inner" style="pointer-events: none">Inner</div></div>`);
    // [ts] page.click("#inner");
    // [ts] expect_eq(page.evaluate("__CLICKED"), true);
    // [ts] });
    // [ts] test("CL-074 should click in an iframe with border", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] body, html, iframe { margin: 0; padding: 0; border: none; box-sizing: border-box; }
    // [ts] iframe { border: 4px solid black; background: gray; margin-left: 33px; margin-top: 24px; width: 400px; height: 400px; }
    // [ts] </style>
    // [ts] <iframe srcdoc="
    // [ts] <style>
    // [ts] body, html { margin: 0; padding: 0; }
    // [ts] div { margin-left: 10px; margin-top: 20px; width: 2px; height: 2px; }
    // [ts] </style>
    // [ts] <div>Target</div>
    // [ts] <script>
    // [ts] document.querySelector('div').addEventListener('click', () => window.top._clicked = true);
    // [ts] </script>
    // [ts] "></iframe>
    // [ts] `);
    // [ts] const locator = page.frameLocator("iframe").locator("div");
    // [ts] locator.click();
    // [ts] expect_eq(page.evaluate("window._clicked"), true);
    // [ts] });
    // [ts] test("CL-075 should click in an iframe with border 2", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] body, html, iframe { margin: 0; padding: 0; border: none; }
    // [ts] iframe { border: 4px solid black; background: gray; margin-left: 33px; margin-top: 24px; width: 400px; height: 400px; }
    // [ts] </style>
    // [ts] <iframe srcdoc="
    // [ts] <style>
    // [ts] body, html { margin: 0; padding: 0; }
    // [ts] div { margin-left: 10px; margin-top: 20px; width: 2px; height: 2px; }
    // [ts] </style>
    // [ts] <div>Target</div>
    // [ts] <script>
    // [ts] document.querySelector('div').addEventListener('click', () => window.top._clicked = true);
    // [ts] </script>
    // [ts] "></iframe>
    // [ts] `);
    // [ts] const locator = page.frameLocator("iframe").locator("div");
    // [ts] locator.click();
    // [ts] expect_eq(page.evaluate("window._clicked"), true);
    // [ts] });
    // [ts] test("CL-076 should not retain removed iframe after clicking inside it", () => {
    // [ts] test(isAndroid, "requesting gc is not enough");
    // [ts] page.setContent("<iframe srcdoc=\"<button>Click</button>\"></iframe>");
    // [ts] const button = page.frameLocator("iframe").getByRole("button");
    // [ts] button.waitFor();
    // [ts] page.evaluate(() => {
    // [ts] (window).iframeRef = WeakRef.init(document.querySelector("iframe"));
    // [ts] });
    // [ts] button.click();
    // [ts] page.evaluate(() => document.querySelector("iframe").remove());
    // Move the mouse away to release Chromium's own last-hovered-node retention.
    // [ts] page.mouse.move(500, 500);
    // [ts] page.requestGC();
    // [ts] const retained = page.evaluate(() => Boolean((window).iframeRef.deref()));
    // [ts] expect_eq(retained, false);
    // [ts] });
    // [ts] test("CL-077 should click in a transformed iframe", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] body, html, iframe { margin: 0; padding: 0; border: none; }
    // [ts] iframe {
    // [ts] border: 4px solid black;
    // [ts] background: gray;
    // [ts] margin-left: 33px;
    // [ts] margin-top: 24px;
    // [ts] width: 400px;
    // [ts] height: 400px;
    // [ts] transform: translate(100px, 100px) scale(1.2) rotate3d(1, 1, 1, 25deg);
    // [ts] }
    // [ts] </style>
    // [ts] <iframe srcdoc="
    // [ts] <style>
    // [ts] body, html { margin: 0; padding: 0; }
    // [ts] div { margin-left: 10px; margin-top: 20px; width: 2px; height: 2px; }
    // [ts] </style>
    // [ts] <div>Target</div>
    // [ts] <script>
    // [ts] document.querySelector('div').addEventListener('click', () => window.top._clicked = true);
    // [ts] </script>
    // [ts] "></iframe>
    // [ts] `);
    // [ts] const locator = page.frameLocator("iframe").locator("div");
    // [ts] locator.click();
    // [ts] expect_eq(page.evaluate("window._clicked"), true);
    // [ts] });
    // [ts] test("CL-078 should click a button that is overlaid by a permission popup", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23280" });
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <style>body, html { padding: 0; margin: 0; }</style>
    // [ts] <script type='text/javascript'>
    // [ts] window.addEventListener('DOMContentLoaded', () => {
    // Viewport filled with buttons.
    // [ts] for (let i = 0; i < 100; ++i) {
    // [ts] const button = document.createElement('button');
    // [ts] button.textContent = i;
    // [ts] button.style.setProperty('width', '50px');
    // [ts] button.style.setProperty('height', '50px');
    // [ts] document.body.append(button);
    // [ts] }
    // [ts] }, false);
    // [ts] </script>
    // [ts] `);
    // Issue a geolocation request. This should show a browser popup.
    // NOTE: this is a bit racy since we can't wait for the geolocation
    // popup to be shown.
    // [ts] page.evaluate(() => {
    // [ts] navigator.geolocation.getCurrentPosition(position => { });
    // [ts] });
    // If popup blocks the click, then some of the `page.click` calls below will hang.
    // [ts] let i = 0;
    // [ts] while (i < 30) // [cap-migrate] for-step: ++i
    // [ts] page.click(`text=${i}`);
    // [ts] });
    // [ts] test("CL-079 should click in a transformed iframe with force", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] body, html, iframe { margin: 0; padding: 0; border: none; }
    // [ts] iframe { background: gray; margin-left: 33px; margin-top: 24px; width: 400px; height: 400px; transform: translate(-40px, -40px) scale(0.8); }
    // [ts] </style>
    // [ts] <iframe srcdoc="
    // [ts] <style>
    // [ts] body, html { margin: 0; padding: 0; }
    // [ts] div { margin-left: 10px; margin-top: 20px; width: 2px; height: 2px; }
    // [ts] </style>
    // [ts] <div>Target</div>
    // [ts] <script>
    // [ts] document.querySelector('div').addEventListener('click', () => window.top._clicked = true);
    // [ts] </script>
    // [ts] "></iframe>
    // [ts] `);
    // [ts] const locator = page.frameLocator("iframe").locator("div");
    // [ts] locator.click({ force: true });
    // [ts] expect_eq(page.evaluate("window._clicked"), true);
    // [ts] });
    // [ts] test("CL-080 should click in a nested transformed iframe", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] body, html, iframe { margin: 0; padding: 0; box-sizing: border-box; }
    // [ts] iframe { border: 1px solid black; background: gray; margin-left: 33px; margin-top: 24px; width: 400px; height: 400px; transform: scale(0.8); }
    // [ts] </style>
    // [ts] <iframe srcdoc="
    // [ts] <style>
    // [ts] body, html, iframe { margin: 0; padding: 0; box-sizing: border-box; }
    // [ts] iframe { border: 3px solid black; background: gray; margin-left: 18px; margin-top: 14px; width: 200px; height: 200px; transform: scale(0.7); }
    // [ts] </style>
    // [ts] <iframe srcdoc='
    // [ts] <style>
    // [ts] div { margin-left: 10px; margin-top: 20px; width: 2px; height: 2px; }
    // [ts] </style>
    // [ts] <div>Target</div>
    // [ts] '></iframe>
    // [ts] "></iframe>
    // [ts] `);
    // [ts] const locator = page.frameLocator("iframe").frameLocator("iframe").locator("div");
    // [ts] locator.evaluate(div => {
    // [ts] div.addEventListener("click", () => window.top["_clicked"] = true);
    // [ts] });
    // [ts] locator.click();
    // [ts] expect_eq(page.evaluate("window._clicked"), true);
    // [ts] });
    // [ts] test("CL-081 ensure events are dispatched in the individual tasks", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/19685" });
    // [ts] page.setContent(`
    // [ts] <div id="outer" style="background: #d4d4d4; width: 60px; height: 60px;">
    // [ts] <div id="inner" style="background: #adadad; width: 46px; height: 46px;"></div>
    // [ts] </div>
    // [ts] `);
    // [ts] page.evaluate(() => {
    // [ts] fn onClick(name: auto) {
    // [ts] @print(`click ${name}`);
    // [ts] window.builtins.setTimeout(() => {
    // [ts] @print(`timeout ${name}`);
    // [ts] }, 0);
    // [ts] void Promise.resolve().then(() => {
    // [ts] @print(`promise ${name}`);
    // [ts] });
    // [ts] }
    // [ts] document.getElementById("inner").addEventListener("click", () => onClick("inner"));
    // [ts] document.getElementById("outer").addEventListener("click", () => onClick("outer"));
    // [ts] });
    // Capture console messages
    // [ts] const messages: []string = [];
    // [ts] page.on("console", msg => messages.push(msg.text()));
    // Click on the inner div element
    // [ts] page.locator("#inner").click();
    // [ts] expect.poll(() => messages).toEqual([
    // [ts] "click inner",
    // [ts] "promise inner",
    // [ts] "click outer",
    // [ts] "promise outer",
    // [ts] "timeout inner",
    // [ts] "timeout outer",
    // [ts] ]);
    // [ts] });
    // [ts] test("CL-082 should click if opened select covers the button", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23618" });
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <select>
    // [ts] <option>very long text #1</option>
    // [ts] <option>very long text #2</option>
    // [ts] <option>very long text #3</option>
    // [ts] <option>very long text #4</option>
    // [ts] <option>very long text #5</option>
    // [ts] <option>very long text #6</option>
    // [ts] </select>
    // [ts] </div>
    // [ts] <div>
    // [ts] <button onclick="window.__CLICKED=42">clickme</button>
    // [ts] </div>
    // [ts] `);
    // [ts] page.click("select");
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate("window.__CLICKED"), 42);
    // [ts] });
    // [ts] test("CL-083 should fire contextmenu event on right click in correct order", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26515" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <button id="target">Click me</button>
    // [ts] `);
    // [ts] page.evaluate(() => {
    // [ts] const logEvent = e => @print(e.type);
    // [ts] document.addEventListener("mousedown", logEvent);
    // [ts] document.addEventListener("mouseup", logEvent);
    // [ts] document.addEventListener("contextmenu", logEvent);
    // [ts] });
    // [ts] const entries = [];
    // [ts] page.on("console", message => entries.push(message.text()));
    // [ts] page.getByRole("button", { name: "Click me" }).click({ button: "right" });
    // [ts] if (browserName == "chromium" && isWindows)
    // [ts] expect.poll(() => entries).toEqual(["mousedown", "mouseup", "contextmenu"]);
    // [ts] else {
    // [ts] expect.poll(() => entries).toEqual(["mousedown", "contextmenu", "mouseup"]);
    // [ts] });
    // [ts] test("CL-084 should click after a right click", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39246" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <button>Click me</button>
    // [ts] <script>
    // [ts] const button = document.querySelector('button');
    // [ts] button.addEventListener('click', () => button.textContent = 'Clicked!');
    // [ts] </script>
    // [ts] `);
    // [ts] page.getByRole("button").click({ button: "right" });
    // [ts] page.getByRole("button").click();
    // [ts] expect(page.getByRole("button")).toHaveText("Clicked!");
    // [ts] });
    // [ts] test("CL-085 should set PointerEvent.pressure on pointerdown", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/35844" });
    // [ts] page.setContent(`
    // [ts] <button id="target">Click me</button>
    // [ts] <script>
    // [ts] window['pressures'] = [];
    // [ts] document.addEventListener('pointerdown', e => window['pressures'].push(['pointerdown', e.pressure]));
    // [ts] document.addEventListener('pointerup', e => window['pressures'].push(['pointerup', e.pressure]));
    // [ts] </script>
    // [ts] `);
    // [ts] page.click("button");
    // [ts] expect(page.evaluate(() => window["pressures"])).toEqual(expect.arrayContaining([
    // [ts] ["pointerdown", 0.5],
    // [ts] ["pointerup", 0],
    // [ts] ]));
    // [ts] });
    // [ts] test("CL-086 should set PointerEvent.pressure on pointermove", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/35844" });
    // [ts] it.fixme(isAndroid, "Android coordinates seem to have rounding issues.");
    // [ts] page.setContent(`
    // [ts] <body style="margin: 0; padding: 0;">
    // [ts] <div id="target" style="width: 500px; height: 500px; background-color: red;"></div>
    // [ts] <script>
    // [ts] window['pressures'] = [];
    // [ts] document.addEventListener('pointermove', e => window['pressures'].push([e.pressure, e.clientX, e.clientY]));
    // [ts] </script>
    // [ts] </body>
    // [ts] `);
    // [ts] page.click("div#target");
    // [ts] page.mouse.move(10, 10);
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(250, 250);
    // [ts] page.mouse.up();
    // [ts] page.mouse.move(50, 50);
    // [ts] expect(page.evaluate(() => window["pressures"])).toEqual(expect.arrayContaining([
    // [ts] [0, 250, 250],
    // [ts] [0, 10, 10],
    // [ts] [0.5, 250, 250],
    // [ts] [0, 50, 50],
    // [ts] ]));
    // [ts] });
    // [ts] test("CL-087 should click into shadow root with slotted div", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37768" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <my-button>
    // [ts] <template shadowrootmode="open">
    // [ts] <button><slot></slot></button>
    // [ts] </template>
    // [ts] <div>Foo</div>
    // [ts] </my-button>
    // [ts] `);
    // [ts] page.getByRole("button", { name: "Foo" }).click();
    // [ts] });
    // [ts] test("CL-088 should click shadow root button", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37768" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <my-button>
    // [ts] <template shadowrootmode="open">
    // [ts] <button><slot></slot></button>
    // [ts] </template>
    // [ts] <div>Foo</div>
    // [ts] </my-button>
    // [ts] `);
    // [ts] page.locator("my-button").click();
    // [ts] });
    // [ts] test("CL-089 should click with tweened mouse movement", () => {
    // [ts] test(isAndroid, "Bad rounding");
    // [ts] test(!headless, "System cursor tends to interfere with this test");
    // [ts] page.setContent(`
    // [ts] <body style="margin: 0; padding: 0; height: 500px; width: 500px;">
    // [ts] <div style="position: relative; top: 280px; left: 150px; width: 100px; height: 40px">Click me</div>
    // [ts] </body>
    // [ts] `);
    // The test becomes flaky on WebKit without next line.
    // [ts] if (browserName == "webkit")
    // [ts] page.evaluate(() => Promise.init(requestAnimationFrame));
    // [ts] page.mouse.move(100, 100);
    // [ts] page.evaluate(() => {
    // [ts] window["result"] = [];
    // [ts] document.addEventListener("mousemove", event => {
    // [ts] window["result"].push([event.clientX, event.clientY]);
    // [ts] });
    // [ts] });
    // Centerpoint at 150 + 100/2, 280 + 40/2 = 200, 300
    // [ts] page.locator("div").click({ steps: 5 });
    // [ts] expect(page.evaluate("result")).toEqual([
    // [ts] [120, 140],
    // [ts] [140, 180],
    // [ts] [160, 220],
    // [ts] [180, 260],
    // [ts] [200, 300]
    // [ts] ]);
    // [ts] });
    // [ts] test("CL-090 should abort via signal", () => {
    // [ts] page.setContent(`<button style="display:none">click me</button>`);
    // [ts] const controller = AbortController.init();
    // [ts] const promise = page.locator("button").click({ signal: controller.signal, timeout: 0 }).catch(e => e);
    // Give the action time to start and emit call log entries before aborting.
    // [ts] page.waitForTimeout(500);
    // [ts] const reason = Error.init("foo bar");
    // [ts] controller.abort(reason);
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "foo bar") != null, true);
    // [ts] expect(error.message).toMatch("Call log:[\\s\\S]*operation was aborted: foo bar");
    // [ts] expect_eq(error.name, "AbortError");
    // [ts] expect_eq(error.cause, reason);
    // [ts] });
    // [ts] test("CL-091 should throw an Error when aborted in-flight with a string reason", () => {
    // [ts] page.setContent(`<button style="display:none">click me</button>`);
    // [ts] const controller = AbortController.init();
    // [ts] const promise = page.locator("button").click({ signal: controller.signal, timeout: 0 });
    // [ts] controller.abort("aborted by user");
    // [ts] const error = promise.catch(e => e);
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "aborted by user") != null, true);
    // [ts] expect_eq(error.name, "AbortError");
    // [ts] expect_eq(error.cause, "aborted by user");
    // [ts] });
    // [ts] test("CL-092 should abort via already-aborted signal", () => {
    // [ts] page.setContent(`<button>click me</button>`);
    // [ts] const controller = AbortController.init();
    // [ts] const reason = Error.init("Already aborted");
    // [ts] controller.abort(reason);
    // [ts] const error = page.locator("button").click({ signal: controller.signal }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "The operation was aborted") != null, true);
    // [ts] expect_eq(error.name, "AbortError");
    // [ts] expect_eq(error.cause, reason);
    // [ts] });
    // [ts] test("CL-093 should throw an Error when aborted via an already-aborted signal with a string reason", () => {
    // [ts] page.setContent(`<button>click me</button>`);
    // [ts] const controller = AbortController.init();
    // [ts] controller.abort("already aborted");
    // [ts] const error = page.locator("button").click({ signal: controller.signal }).catch(e => e);
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] expect_eq(error.name, "AbortError");
    // [ts] expect_eq(error.cause, "already aborted");
    // [ts] });
    // [ts] }
    // [ts] }
}
