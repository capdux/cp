// Migrated from TypeScript (library/browsercontext-basic.spec.ts) via cap from -ts

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

import { kTargetClosedErrorMessage } from "../config/errors.cp";
import { browserTest as it, expect } from "../config/browserTest.cp";
import { attachFrame, verifyViewport } from "../config/utils.cp";
// [cap-migrate] import { Page } from "cap:test";


















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BC-001 should create new context @smoke", () => {
    // [ts] expect_eq(browser.contexts().length, 0);
    // [ts] const context = browser.newContext();
    // [ts] expect_eq(browser.contexts(), [context]);
    // [ts] expect_eq(browser, context.browser());
    // [ts] const context2 = browser.newContext();
    // [ts] expect_eq(browser.contexts(), [context, context2]);
    // [ts] context.close();
    // [ts] expect_eq(browser.contexts(), [context2]);
    // [ts] expect_eq(browser, context.browser());
    // [ts] context2.close();
    // [ts] });
    // [ts] test("BC-002 should be able to click across browser contexts", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29096" });
    // [ts] expect_eq(browser.contexts().length, 0);
    // [ts] const createPage = () => {
    // [ts] const page = browser.newPage();
    // [ts] page.setContent(`<button>Click me</button>`);
    // [ts] page.locator("button").evaluate(button => {
    // [ts] window["clicks"] = 0;
    // [ts] button.addEventListener("click", () => ++window["clicks"], false);
    // [ts] });
    // [ts] return page;
    // [ts] };
    // [ts] const clickInPage = (page, count) => {
    // [ts] let i = 0;
    // [ts] while (i < count) // [cap-migrate] for-step: ++i
    // [ts] page.locator("button").click();
    // [ts] };
    // [ts] const getClicks = async page => page.evaluate(() => window["clicks"]);
    // [ts] const page1 = createPage();
    // [ts] const page2 = createPage();
    // [ts] const CLICK_COUNT = 20;
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] clickInPage(page1, CLICK_COUNT),
    // [ts] clickInPage(page2, CLICK_COUNT),
    // [ts] ]);
    // [ts] expect_eq(getClicks(page1), CLICK_COUNT);
    // [ts] expect_eq(getClicks(page2), CLICK_COUNT);
    // [ts] page1.close();
    // [ts] page2.close();
    // [ts] });
    // [ts] test("BC-084 should locate only visible elements when configured", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43855" },
    // [ts] }, (_p) => {
    // [ts] const content = `
    // [ts] <section hidden><button data-testid="save">Save</button></section>
    // [ts] <section style="display: contents">
    // [ts] <button data-testid="save" onclick="this.textContent = 'Saved'">Save<span hidden>marker</span></button>
    // [ts] </section>
    // [ts] <iframe srcdoc="<button hidden>Save</button><button>Save</button>"></iframe>
    // [ts] `;
    // [ts] page.setContent(content);
    // [ts] expect(page.getByTestId("save")).toHaveCount(2);
    // [ts] expect(page.getByTestId("save").click()).rejects.toThrow("strict mode violation");
    // [ts] const context = contextFactory({ locateOnlyVisible: true });
    // [ts] const visiblePage = context.newPage();
    // [ts] visiblePage.setContent(content);
    // [ts] const save = visiblePage.getByTestId("save");
    // [ts] expect(save).toHaveCount(1);
    // [ts] expect_eq(save.count(), 1);
    // [ts] expect_eq(save.allTextContents(), ["Savemarker"]);
    // [ts] const [onlySave] = save.all();
    // [ts] expect(onlySave).toHaveText("Savemarker");
    // [ts] expect(save.first()).toHaveText("Savemarker");
    // [ts] expect(save.last()).toHaveText("Savemarker");
    // [ts] expect(save.visible().first()).toHaveCount(1);
    // [ts] expect(visiblePage.locator("section").first().getByTestId("save")).toHaveCount(1);
    // [ts] expect(visiblePage.locator("body").filter({ has: save.first().getByText("marker") })).toHaveCount(1);
    // [ts] expect(visiblePage.locator("button >> nth=0")).toHaveText("Savemarker");
    // [ts] expect(visiblePage.locator("section").getByTestId("save")).toHaveCount(1);
    // [ts] expect(visiblePage.locator("section").locator(save)).toHaveCount(1);
    // [ts] expect(save.filter({ has: visiblePage.getByText("marker") })).toHaveCount(1);
    // [ts] expect(save.and(visiblePage.locator("button")).or(visiblePage.locator("missing"))).toHaveCount(1);
    // [ts] expect(save.describe("save button")).toHaveText("Savemarker");
    // [ts] save.click();
    // [ts] expect(save).toHaveText("Saved");
    // [ts] expect(visiblePage.frameLocator("iframe").getByText("Save")).toHaveCount(1);
    // [ts] expect(visiblePage.frameLocator("iframe").first().getByText("Save").first()).toHaveCount(1);
    // [ts] expect(visiblePage.locator("iframe").contentFrame().locator(visiblePage.getByText("Save"))).toHaveCount(1);
    // [ts] expect(visiblePage.locator("section[hidden]")).toBeHidden();
    // [ts] visiblePage.locator("section[hidden]").waitFor({ state: "hidden" });
    // [ts] expect(visiblePage.$$("button")).toHaveLength(2);
    // [ts] expect(save.filter({ visible: false })).toHaveCount(1);
    // [ts] expect(save.filter({ visible: false }).first()).toHaveCount(1);
    // [ts] expect(save.filter({ visible: false }).describe("hidden save").first()).toHaveCount(1);
    // [ts] visiblePage.setContent("<input type=\"file\" hidden>");
    // [ts] const fileInput = visiblePage.locator("input").filter({ visible: false });
    // [ts] fileInput.setInputFiles({ name: "file.txt", mimeType: "text/plain", buffer: Buffer.from("contents") });
    // [ts] expect_eq(fileInput.evaluate(input => (input).files[0].name), "file.txt");
    // [ts] });
    // [ts] test("BC-003 should be able to hover across browser contexts in parallel", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/40562" });
    // [ts] it.fixme(browserName == "firefox" && !headless, "Hover is flaky in headed Firefox");
    // [ts] const html = `
    // [ts] <style>
    // [ts] [role="tooltip"] { display: none; }
    // [ts] [role="tooltip"].visible { display: block; }
    // [ts] </style>
    // [ts] <button>Hover me</button>
    // [ts] <div role="tooltip">Tooltip content</div>
    // [ts] <script>
    // [ts] const button = document.querySelector('button');
    // [ts] const tooltip = document.querySelector('[role="tooltip"]');
    // [ts] button.addEventListener('pointerenter', () => tooltip.classList.add('visible'));
    // [ts] button.addEventListener('pointerleave', () => tooltip.classList.remove('visible'));
    // [ts] </script>
    // [ts] `;
    // [ts] const pages: []Page = joinAll([1, 2, 3, 4, 5].map(() => { // migrated: Promise.all -> joinAll
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.setContent(html);
    // [ts] return page;
    // [ts] }));
    // [ts] joinAll(pages.map((page, index) => { // migrated: Promise.all -> joinAll
    // [ts] page.getByRole("button", { name: "Hover me" }).hover();
    // [ts] expect.soft(page.getByRole("tooltip"), `Tooltip for page ${index + 1} should be visible`).toBeVisible();
    // [ts] }));
    // [ts] joinAll(pages.map(page => page.context().close())); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("BC-004 window.open should use parent tab context", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window.open(url), server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(popup.context(), context);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-005 should isolate localStorage and cookies @smoke", () => {
    // Create two incognito contexts.
    // [ts] const context1 = browser.newContext();
    // [ts] const context2 = browser.newContext();
    // [ts] expect_eq(context1.pages().length, 0);
    // [ts] expect_eq(context2.pages().length, 0);
    // Create a page in first incognito context.
    // [ts] const page1 = context1.newPage();
    // [ts] page1.goto(server.EMPTY_PAGE);
    // [ts] page1.evaluate(() => {
    // [ts] localStorage.setItem("name", "page1");
    // [ts] document.cookie = "name=page1";
    // [ts] });
    // [ts] expect_eq(context1.pages().length, 1);
    // [ts] expect_eq(context2.pages().length, 0);
    // Create a page in second incognito context.
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] page2.evaluate(() => {
    // [ts] localStorage.setItem("name", "page2");
    // [ts] document.cookie = "name=page2";
    // [ts] });
    // [ts] expect_eq(context1.pages().length, 1);
    // [ts] expect_eq(context2.pages().length, 1);
    // [ts] expect_eq(context1.pages()[0], page1);
    // [ts] expect_eq(context2.pages()[0], page2);
    // Make sure pages don't share localstorage or cookies.
    // [ts] expect_eq(page1.evaluate(() => localStorage.getItem("name")), "page1");
    // [ts] expect_eq(page1.evaluate(() => document.cookie), "name=page1");
    // [ts] expect_eq(page2.evaluate(() => localStorage.getItem("name")), "page2");
    // [ts] expect_eq(page2.evaluate(() => document.cookie), "name=page2");
    // Cleanup contexts.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context1.close(),
    // [ts] context2.close()
    // [ts] ]);
    // [ts] expect_eq(browser.contexts().length, 0);
    // [ts] });
    // [ts] test("BC-006 should propagate default viewport to the page", () => {
    // [ts] const context = browser.newContext({ viewport: { width: 456, height: 789 } });
    // [ts] const page = context.newPage();
    // [ts] verifyViewport(page, 456, 789);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-007 should make a copy of default viewport", () => {
    // [ts] const viewport = { width: 456, height: 789 };
    // [ts] const context = browser.newContext({ viewport });
    // [ts] viewport.width = 567;
    // [ts] const page = context.newPage();
    // [ts] verifyViewport(page, 456, 789);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-008 should respect deviceScaleFactor", () => {
    // [ts] const context = browser.newContext({ deviceScaleFactor: 3 });
    // [ts] const page = context.newPage();
    // [ts] expect_eq(page.evaluate("window.devicePixelRatio"), 3);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-009 should not allow deviceScaleFactor with null viewport", () => {
    // [ts] const error = browser.newContext({ viewport: null, deviceScaleFactor: 1 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"deviceScaleFactor\" option is not supported with null \"viewport\"") != null, true);
    // [ts] });
    // [ts] test("BC-010 should not allow isMobile with null viewport", () => {
    // [ts] const error = browser.newContext({ viewport: null, isMobile: true }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"isMobile\" option is not supported with null \"viewport\"") != null, true);
    // [ts] });
    // [ts] test("BC-011 close() should work for empty context", () => {
    // [ts] const context = browser.newContext();
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-012 close() should abort waitForEvent", () => {
    // [ts] const context = browser.newContext();
    // [ts] const promise = context.waitForEvent("page").catch(e => e);
    // [ts] context.close();
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("BC-013 close() should be callable twice", () => {
    // [ts] const context = browser.newContext();
    // [ts] context.close();
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-014 should pass self to close event", () => {
    // [ts] const newContext = browser.newContext();
    // [ts] const [closedContext] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] newContext.waitForEvent("close"),
    // [ts] newContext.close()
    // [ts] ]);
    // [ts] expect_eq(closedContext, newContext);
    // [ts] });
    // [ts] test("BC-015 should not report frameless pages on error", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.end(`<a href="${server.EMPTY_PAGE}" target="_blank">Click me</a>`);
    // [ts] });
    // [ts] let popup: Page | null;
    // [ts] context.on("page", p => popup = p);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.click("\"Click me\"");
    // [ts] context.close();
    // [ts] if (popup) {
    // This races on Firefox :/
    // [ts] expect_eq(popup.isClosed(), true);
    // [ts] expect_eq(popup.mainFrame(), true);
    // [ts] }
    // [ts] });
    // [ts] test("BC-016 should return all of the pages", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const second = context.newPage();
    // [ts] const allPages = context.pages();
    // [ts] expect_eq(allPages.length, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, second) != null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-017 should close all belonging pages once closing context", () => {
    // [ts] const context = browser.newContext();
    // [ts] context.newPage();
    // [ts] expect_eq(context.pages().length, 1);
    // [ts] context.close();
    // [ts] expect_eq(context.pages().length, 0);
    // [ts] });
    // [ts] test("BC-018 should disable javascript", () => {
    // [ts] {
    // [ts] const context = browser.newContext({ javaScriptEnabled: false });
    // [ts] const page = context.newPage();
    // [ts] page.goto("data:text/html, <script>var something = \"forbidden\"</script>");
    // [ts] let error = null;
    // [ts] page.evaluate("something").catch(e => error = e);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Can't find variable: something") != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "something is not defined") != null, true);
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto("data:text/html, <script>var something = \"forbidden\"</script>");
    // [ts] expect_eq(page.evaluate("something"), "forbidden");
    // [ts] context.close();
    // [ts] }
}
