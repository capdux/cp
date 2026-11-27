// Migrated from TypeScript (page/page-basic.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";













fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("PA-001 should fire load when expected", () => {
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto("about:blank"),
    // [ts] page.waitForEvent("load"),
    // [ts] ]);
    // [ts] });
    // [ts] test("PA-002 async stacks should work", () => {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] req.socket.end();
    // [ts] });
    // [ts] let error = null;
    // [ts] page.goto(server.EMPTY_PAGE).catch(e => error = e);
    // [ts] expect_eq(error != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, __filename) != null, true);
    // [ts] });
    // [ts] test("PA-003 should provide access to the opener page", () => {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window.open("about:blank")),
    // [ts] ]);
    // [ts] const opener = popup.opener();
    // [ts] expect_eq(opener, page);
    // [ts] });
    // [ts] test("PA-004 should fire domcontentloaded when expected", () => {
    // [ts] const navigatedPromise = page.goto("about:blank");
    // [ts] page.waitForEvent("domcontentloaded");
    // [ts] navigatedPromise;
    // [ts] });
    // [ts] test("PA-005 should pass self as argument to domcontentloaded event", () => {
    // [ts] const [eventArg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => page.on("domcontentloaded", f)),
    // [ts] page.goto("about:blank")
    // [ts] ]);
    // [ts] expect_eq(eventArg, page);
    // [ts] });
    // [ts] test("PA-006 should pass self as argument to load event", () => {
    // [ts] const [eventArg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => page.on("load", f)),
    // [ts] page.goto("about:blank")
    // [ts] ]);
    // [ts] expect_eq(eventArg, page);
    // [ts] });
    // [ts] test("PA-007 page.url should work", () => {
    // [ts] expect_eq(page.url(), "about:blank");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("PA-008 page.url should include hashes", () => {
    // [ts] page.goto(server.EMPTY_PAGE + "#hash");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE + "#hash");
    // [ts] page.evaluate(() => {
    // [ts] window.location.hash = "dynamic";
    // [ts] });
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE + "#dynamic");
    // [ts] });
    // [ts] test("PA-009 page.title should return the page title", () => {
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] expect_eq(page.title(), "Woof-Woof");
    // [ts] });
    // [ts] test("PA-010 page.title should not throw during navigation", () => {
    // [ts] page.setContent("<title>hello</title>");
    // [ts] const promise = page.goto(server.PREFIX + "/title.html");
    // [ts] const [titleOrError] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.title().catch(e => e),
    // [ts] promise,
    // [ts] ]);
    // [ts] expect_eq(@typeName(titleOrError), "string");
    // [ts] expect(titleOrError).toMatch("^(hello|Loading http.*title.html||Woof-Woof)$");
    // [ts] expect(page).toHaveTitle("Woof-Woof");
    // [ts] });
    // [ts] test("PA-011 page.close should work with window.close", () => {
    // [ts] const newPagePromise = page.waitForEvent("popup");
    // [ts] page.evaluate(() => window["newPage"] = window.open("about:blank"));
    // [ts] const newPage = newPagePromise;
    // [ts] const closedPromise = Promise.init(x => newPage.on("close", x));
    // [ts] page.evaluate(() => window["newPage"].close());
    // [ts] closedPromise;
    // [ts] });
    // [ts] test("PA-012 page.frame should respect name", () => {
    // [ts] test(isBidi, "page.frame({ name }) is racy with BiDi");
    // [ts] page.setContent(`<iframe name=target></iframe>`);
    // [ts] expect_eq(page.frame({ name: "bogus" }), null);
    // [ts] const frame = page.frame({ name: "target" });
    // [ts] expect_eq(frame, true);
    // [ts] expect_eq(frame == page.mainFrame().childFrames()[0], true);
    // [ts] });
    // [ts] test("PA-013 page.frame should respect url", () => {
    // [ts] page.setContent(`<iframe src="${server.EMPTY_PAGE}"></iframe>`);
    // [ts] expect_eq(page.frame({ url: "bogus" }), null);
    // [ts] expect_eq(page.frame({ url: "empty" }).url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("PA-014 should have sane user agent", () => {
    // [ts] test(isAndroid);
    // [ts] test(isElectron);
    // [ts] const userAgent = page.evaluate(() => navigator.userAgent);
    // [ts] const [
    // [ts] part1,
    // [ts] /* part2 */,
    // [ts] part3,
    // [ts] part4,
    // [ts] part5,
    // [ts] ] = userAgent.split("[()]").map(part => part.trim());
    // First part is always "Mozilla/5.0"
    // [ts] expect_eq(part1, "Mozilla/5.0");
    // Second part in parenthesis is platform - ignore it.
    // Third part for Firefox is the last one and encodes engine and browser versions.
    // [ts] if (browserName == "firefox") {
    // [ts] const [engine, browser] = part3.split(" ");
    // [ts] expect_eq(engine.startsWith("Gecko"), true);
    // [ts] expect_eq(browser.startsWith("Firefox"), true);
    // [ts] expect_eq(part4, null);
    // [ts] expect_eq(part5, null);
    // [ts] return;
    // [ts] }
    // For both options.CHROMIUM and options.WEBKIT, third part is the AppleWebKit version.
    // [ts] expect_eq(part3.startsWith("AppleWebKit/"), true);
    // [ts] expect_eq(part4, "KHTML, like Gecko");
    // 5th part encodes real browser name and engine version.
    // [ts] const tokens = part5.split(" ");
    // [ts] const engine = tokens[0];
    // [ts] const browser = tokens.find(t => t.startsWith("Safari/"));
    // [ts] expect_eq(browser?.startsWith("Safari/"), true);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(engine.includes("Chrome/"), true);
    // [ts] else {
    // [ts] expect_eq(engine.startsWith("Version/"), true);
    // [ts] });
    // [ts] test("PA-015 page.press should work", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.press("textarea", "a");
    // [ts] expect_eq(page.evaluate(() => document.querySelector("textarea").value), "a");
    // [ts] });
    // [ts] test("PA-016 page.press should work for Enter", () => {
    // [ts] page.setContent(`<input onkeypress="console.log('press')"></input>`);
    // [ts] const messages = [];
    // [ts] page.on("console", message => messages.push(message));
    // [ts] page.press("input", "Enter");
    // [ts] expect_eq(messages[0].text(), "press");
    // [ts] });
    // [ts] test("PA-017 frame.press should work", () => {
    // [ts] page.setContent(`<iframe name=inner src="${server.PREFIX}/input/textarea.html"></iframe>`);
    // [ts] const frame = page.frame("inner");
    // [ts] frame.press("textarea", "a");
    // [ts] expect_eq(frame.evaluate(() => document.querySelector("textarea").value), "a");
    // [ts] });
    // [ts] test("PA-018 has navigator.webdriver set to true", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] expect_eq(page.evaluate(() => navigator.webdriver), true);
    // [ts] });
    // [ts] test("PA-019 should iterate over page properties", () => {
    // [ts] const props = [];
    // [ts] for (const prop in page) {
    // [ts] if (page[prop] && @typeName(page[prop]) == "object")
    // [ts] props.push(page[prop]["__symbol_iterator"]);
    // [ts] }
    // [ts] });
    // [ts] test("PA-057 should prefix the error message with the api name", () => {
    // [ts] const error = page.goto(server.PREFIX + "/empty.html", { timeout: 1 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "page.goto: Timeout 1ms exceeded.") != null, true);
    // [ts] });
    // [ts] }
}
