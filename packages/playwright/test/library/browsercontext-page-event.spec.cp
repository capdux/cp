// Migrated from TypeScript (library/browsercontext-page-event.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BC-062 should have url", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const [otherPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.evaluate(url => window.open(url), server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(otherPage.url(), server.EMPTY_PAGE);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-063 should have url after domcontentloaded", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const [otherPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.evaluate(url => window.open(url), server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] otherPage.waitForLoadState("domcontentloaded");
    // [ts] expect_eq(otherPage.url(), server.EMPTY_PAGE);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-064 should have about:blank url with domcontentloaded", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const [otherPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.evaluate(url => window.open(url), "about:blank")
    // [ts] ]);
    // [ts] otherPage.waitForLoadState("domcontentloaded");
    // [ts] expect_eq(otherPage.url(), "about:blank");
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-065 should have about:blank for empty url with domcontentloaded", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const [otherPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.evaluate(() => window.open())
    // [ts] ]);
    // [ts] otherPage.waitForLoadState("domcontentloaded");
    // [ts] expect_eq(otherPage.url(), "about:blank");
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-066 should report when a new page is created and closed", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const [otherPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.evaluate(url => window.open(url), server.CROSS_PROCESS_PREFIX + "/empty.html"),
    // [ts] ]);
    // The url is about:blank in FF when 'page' event is fired.
    // [ts] expect_eq(std.mem.indexOf(u8, otherPage.url(), server.CROSS_PROCESS_PREFIX) != null, true);
    // [ts] expect_eq(otherPage.evaluate(() => ["Hello", "world"].join(" ")), "Hello world");
    // [ts] expect_eq(otherPage.$("body"), true);
    // [ts] let allPages = context.pages();
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, otherPage) != null, true);
    // [ts] let closeEventReceived;
    // [ts] otherPage.once("close", () => closeEventReceived = true);
    // [ts] otherPage.close();
    // [ts] expect_eq(closeEventReceived, true);
    // [ts] allPages = context.pages();
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, otherPage) == null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-067 should report initialized pages", () => {
    // [ts] const context = browser.newContext();
    // [ts] const pagePromise = context.waitForEvent("page");
    // [ts] void context.newPage();
    // [ts] const newPage = pagePromise;
    // [ts] expect_eq(newPage.url(), "about:blank");
    // [ts] const popupPromise = context.waitForEvent("page");
    // [ts] const evaluatePromise = newPage.evaluate(() => window.open("about:blank"));
    // [ts] const popup = popupPromise;
    // [ts] expect_eq(popup.url(), "about:blank");
    // [ts] evaluatePromise;
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-068 should not crash while redirecting of original request was missed", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] let serverResponse = null;
    // [ts] server.setRoute("/one-style.css", (req, res) => serverResponse = res);
    // Open a new page. Use window.open to connect to the page later.
    // [ts] const [newPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.evaluate(url => window.open(url), server.PREFIX + "/one-style.html"),
    // [ts] server.waitForRequest("/one-style.css")
    // [ts] ]);
    // Issue a redirect.
    // [ts] serverResponse.writeHead(302, { location: "/injectedstyle.css" });
    // [ts] serverResponse.end();
    // [ts] newPage.waitForLoadState("domcontentloaded");
    // [ts] expect_eq(newPage.url(), server.PREFIX + "/one-style.html");
    // Cleanup.
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-069 should have an opener", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.goto(server.PREFIX + "/popup/window-open.html")
    // [ts] ]);
    // [ts] expect_eq(popup.url(), server.PREFIX + "/popup/popup.html");
    // [ts] expect_eq(popup.opener(), page);
    // [ts] expect_eq(page.opener(), null);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-070 should fire page lifecycle events", () => {
    // [ts] const context = browser.newContext();
    // [ts] const events = [];
    // [ts] context.on("page", async page => {
    // [ts] events.push("CREATED: " + page.url());
    // [ts] page.on("close", () => events.push("DESTROYED: " + page.url()));
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.close();
    // [ts] expect(events).toEqual([
    // [ts] "CREATED: about:blank",
    // [ts] `DESTROYED: ${server.EMPTY_PAGE}`
    // [ts] ]);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-071 should work with Shift-clicking", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a href=\"/one-style.html\">yo</a>");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.click("a", { modifiers: ["Shift"] }),
    // [ts] ]);
    // [ts] expect_eq(popup.opener(), null);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BC-072 should work with Ctrl-clicking", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a href=\"/one-style.html\">yo</a>");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.click("a", { modifiers: ["ControlOrMeta"] }),
    // [ts] ]);
    // [ts] expect_eq(popup.opener(), if (browserName == "firefox") page else null);
    // [ts] context.close();
    // [ts] });
}
