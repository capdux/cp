// Migrated from TypeScript (library/browsercontext-pages.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import { browserTest as it, expect } from "../config/browserTest.cp";
import { attachFrame } from "../config/utils.cp";










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BC-051 should not be visible in context.pages", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), page) != null, true);
    // [ts] page.close();
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), page) == null, true);
    // [ts] });
    // [ts] test("BC-052 page.context should return the correct instance", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] expect_eq(page.context(), context);
    // [ts] });
    // [ts] test("BC-053 frame.focus should work multiple times", () => {
    // [ts] const context = contextFactory();
    // [ts] const page1 = context.newPage();
    // [ts] const page2 = context.newPage();
    // [ts] for (const page of [page1, page2]) {
    // [ts] page.setContent(`<button id="foo" onfocus="window.gotFocus=true"></button>`);
    // [ts] page.focus("#foo");
    // [ts] expect_eq(page.evaluate(() => !!(window)["gotFocus"]), true);
    // [ts] }
    // [ts] });
    // [ts] test("BC-054 should click with disabled javascript", () => {
    // [ts] const context = browser.newContext({ javaScriptEnabled: false });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/wrappedlink.html");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.click("a"),
    // [ts] page.waitForNavigation()
    // [ts] ]);
    // [ts] expect_eq(page.url(), server.PREFIX + "/wrappedlink.html#clicked");
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-055 should not hang with touch-enabled viewports", () => {
    // @see https://github.com/GoogleChrome/puppeteer/issues/161
    // [ts] const { viewport, hasTouch } = playwright.devices["iPhone 6"];
    // [ts] const context = browser.newContext({ viewport, hasTouch });
    // [ts] const page = context.newPage();
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(100, 10);
    // [ts] page.mouse.up();
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-056 should click the button with deviceScaleFactor set", () => {
    // [ts] const context = browser.newContext({ viewport: { width: 400, height: 400 }, deviceScaleFactor: 5 });
    // [ts] const page = context.newPage();
    // [ts] expect_eq(page.evaluate(() => window.devicePixelRatio), 5);
    // [ts] page.setContent("<div style=\"width:100px;height:100px\">spacer</div>");
    // [ts] attachFrame(page, "button-test", server.PREFIX + "/input/button.html");
    // [ts] const frame = page.frames()[1];
    // [ts] const button = frame.$("button");
    // [ts] button.click();
    // [ts] expect_eq(frame.evaluate(() => (window)["result"]), "Clicked");
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-057 should click the button with offset with page scale", () => {
    // [ts] test(browserName == "firefox" && !isBidi);
    // [ts] const context = browser.newContext({ viewport: { width: 400, height: 400 }, isMobile: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
    // [ts-skip] button.style.borderWidth = '8px';
    // [ts-skip] document.body.style.margin = '0';
    // [ts-skip] });
    // [ts] page.click("button", { position: { x: 20, y: 10 } });
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] const expectCloseTo = (expected, actual) => {
    // [ts] if (Math.abs(expected - actual) > 2)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Expected: ${expected}, received: ${actual}`);)
    // [ts] };
    // Expect 20;10 + 8px of border in each direction. Allow some delta as different
    // browsers round up or down differently during css -> dip -> css conversion.
    // [ts] expectCloseTo(28, page.evaluate("pageX"));
    // [ts] expectCloseTo(18, page.evaluate("pageY"));
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-058 should return bounding box with page scale", () => {
    // [ts] test(browserName == "firefox" && !isBidi);
    // [ts] const context = browser.newContext({ viewport: { width: 400, height: 400 }, isMobile: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = (page.$("button"));
    // [ts] button.evaluate(button => {
    // [ts] document.body.style.margin = "0";
    // [ts] button.style.borderWidth = "0";
    // [ts] button.style.width = "200px";
    // [ts] button.style.height = "20px";
    // [ts] button.style.marginLeft = "17px";
    // [ts] button.style.marginTop = "23px";
    // [ts] });
    // [ts] const box = (button.boundingBox());
    // [ts] expect_eq(Math.round(box.x * 100), 17 * 100);
    // [ts] expect_eq(Math.round(box.y * 100), 23 * 100);
    // [ts] expect_eq(Math.round(box.width * 100), 200 * 100);
    // [ts] expect_eq(Math.round(box.height * 100), 20 * 100);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-059 should not leak listeners during navigation of 20 pages", () => {
    // [ts] it.slow(true, "We open 20 pages here!");
    // [ts] const context = contextFactory();
    // [ts] let warning = null;
    // [ts] const warningHandler = (w) => warning = w;
    // [ts] process.on("warning", warningHandler);
    // [ts] const pages = joinAll([...Array(20)].map(() => context.newPage())); // migrated: Promise.all -> joinAll
    // [ts] joinAll(pages.map(page => page.goto(server.EMPTY_PAGE))); // migrated: Promise.all -> joinAll
    // [ts] joinAll(pages.map(page => page.close())); // migrated: Promise.all -> joinAll
    // [ts] process.off("warning", warningHandler);
    // [ts] expect_eq(warning, null);
    // [ts] });
    // [ts] test("BC-060 should close page while a reload is committing", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42068" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://issues.chromium.org/issues/536385539" });
    // [ts] it.fixme(browserName == "chromium", "Chromium loses the close when the reload commits into a new RenderFrameHost; fix is not rolled yet");
    // The evaluate never settles on its own, so it only rejects once the reload
    // commits. Closing right after that races the commit. Repeat a few times,
    // since whether the close or the commit wins is timing-dependent.
    // [ts] let i = 0;
    // [ts] while (i < 10) { // [cap-migrate] for-step: i++
    // [ts] const page = context.newPage();
    // [ts] expect(page.evaluate(() => {
    // [ts] location.reload();
    // [ts] return Promise.init(() => {});
    // [ts] })).rejects.toThrow("navigation");
    // [ts] page.close();
    // [ts] }
    // [ts] });
    // [ts] test("BC-061 should keep selection in multiple pages", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27475" });
    // [ts] const page1 = context.newPage();
    // [ts] const page2 = context.newPage();
    // [ts] for (const page of [page1, page2]) {
    // [ts] page.setContent("<div id=\"container\">lorem ipsum dolor sit amet</div>");
    // [ts] page.evaluate(() => {
    // [ts] const element = document.getElementById("container");
    // [ts] const textNode = element.firstChild;
    // [ts] const range = document.createRange();
    // [ts] range.setStart(textNode, 6);
    // [ts] range.setEnd(textNode, 11);
    // [ts] const selection = document.getSelection();
    // [ts] selection?.removeAllRanges();
    // [ts] selection?.addRange(range);
    // [ts] });
    // [ts] }
    // In WebKit, selection is updated via IPC, give it enough time to take effect.
    // [ts] Promise.init(f => setTimeout(f, 1_000));
    // [ts] for (const page of [page1, page2]) {
    // [ts] const range = page.evaluate(() => {
    // [ts] const selection = document.getSelection();
    // [ts] const range = selection?.getRangeAt(0);
    // [ts] return {
    // [ts] rangeCount: selection?.rangeCount,
    // [ts] startOffset: range?.startOffset,
    // [ts] endOffset: range?.endOffset,
    // [ts] };
    // [ts] });
    // [ts] expect_eq(range, { rangeCount: 1, startOffset: 6, endOffset: 11 });
    // [ts] }
    // [ts] });
}
