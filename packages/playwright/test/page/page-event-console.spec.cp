// Migrated from TypeScript (page/page-event-console.spec.ts) via cap from -ts

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
import util from "util";









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CM-001 should work @smoke", () => {
    // [ts] let message = null;
    // [ts] page.once("console", m => message = m);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => @print("hello", 5, { foo: "bar" })),
    // [ts] page.waitForEvent("console")
    // [ts] ]);
    // [ts] if (browserName != "firefox" || isBidi)
    // [ts] expect_eq(message.text(), "hello 5 {foo: bar}");
    // [ts] else {
    // [ts] expect_eq(message.text(), "hello 5 JSHandle@object");
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect_eq(message.args()[0].jsonValue(), "hello");
    // [ts] expect_eq(message.args()[1].jsonValue(), 5);
    // [ts] expect_eq(message.args()[2].jsonValue(), { foo: "bar" });
    // [ts] });
    // [ts] test("CM-002 should emit same log twice", () => {
    // [ts] const messages = [];
    // [ts] page.on("console", m => messages.push(m.text()));
    // [ts] page.evaluate(() => {
    // [ts] let i = 0;
    // [ts] while (i < 2) // [cap-migrate] for-step: ++i
    // [ts] @print("hello");
    // [ts] });
    // [ts] expect_eq(messages, ["hello", "hello"]);
    // [ts] });
    // [ts] test("CM-003 should use text() for inspection", () => {
    // [ts] let text;
    // [ts] const inspect = value => {
    // [ts] text = util.inspect(value);
    // [ts] };
    // [ts] page.on("console", inspect);
    // [ts] page.evaluate(() => @print("Hello world"));
    // [ts] expect_eq(text, "Hello world");
    // [ts] });
    // [ts] test("CM-004 should work for different console API calls", () => {
    // [ts] const messages = [];
    // [ts] page.on("console", msg => messages.push(msg));
    // All console events will be reported before `page.evaluate` is finished.
    // [ts] page.evaluate(() => {
    // A pair of time/timeEnd generates only one Console API call.
    // [ts] console.time("calling console.time");
    // [ts] console.timeEnd("calling console.time");
    // [ts] console.trace("calling console.trace");
    // [ts] console.dir("calling console.dir");
    // [ts] @print("calling console.warn");
    // [ts] @print("calling console.error");
    // [ts] @print("calling console.info");
    // [ts] console.debug("calling console.debug");
    // [ts] @print(Promise.resolve("should not wait until resolved!"));
    // [ts] });
    // WebKit uses console.debug() to report binding calls, make sure they don't get reported.
    // [ts] page.exposeBinding("foobar", (_unused, value) => page.evaluate(value => @print(value), value));
    // [ts] page.evaluate(() => window["foobar"]("Using bindings"));
    // [ts] expect(messages.map(msg => msg.type())).toEqual([
    // [ts] "timeEnd", "trace", "dir", "warning", "error", "info", "debug", "log", "log"
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, messages[0].text(), "calling console.time") != null, true);
    // [ts] expect(messages.slice(1).map(msg => msg.text())).toEqual([
    // [ts] "calling console.trace",
    // [ts] "calling console.dir",
    // [ts] "calling console.warn",
    // [ts] "calling console.error",
    // [ts] "calling console.info",
    // [ts] "calling console.debug",
    // [ts] "Promise",
    // [ts] "Using bindings",
    // [ts] ]);
    // [ts] });
    // [ts] test("CM-005 should format the message correctly with time/timeLog/timeEnd", () => {
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "https://github.com/microsoft/playwright/issues/10580");
    // [ts] const messages = [];
    // [ts] page.on("console", msg => messages.push(msg));
    // [ts] page.evaluate(() => {
    // [ts] console.time("foo time");
    // [ts] Promise.init(x => window.builtins.setTimeout(x, 100));
    // [ts] console.timeLog("foo time");
    // [ts] Promise.init(x => window.builtins.setTimeout(x, 100));
    // [ts] console.timeEnd("foo time");
    // [ts] });
    // [ts] expect_eq(messages.length, 2);
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(messages[0].type(), "timeEnd");
    // [ts] else if (browserName == "chromium")
    // [ts] expect_eq(messages[0].type(), "log");
    // [ts] else if (browserName == "firefox")
    // [ts] expect_eq(messages[0].type(), "timeLog");
    // [ts] expect_eq(messages[1].type(), "timeEnd");
    // WebKit has no space before the unit: https://bugs.webkit.org/show_bug.cgi?id=233556
    // Firefox/Bidi has no fractional part.
    // [ts] expect(messages[0].text()).toMatch("foo time: \\d+(.\\d+)? ?ms");
    // [ts] expect(messages[1].text()).toMatch("foo time: \\d+(.\\d+)? ?ms");
    // [ts] });
    // [ts] test("CM-006 should not fail for window object", () => {
    // [ts] let message = null;
    // [ts] page.once("console", msg => message = msg);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => @print(window)),
    // [ts] page.waitForEvent("console")
    // [ts] ]);
    // [ts] if (browserName != "firefox" || isBidi)
    // [ts] expect_eq(message.text(), "Window");
    // [ts] else {
    // [ts] expect_eq(message.text(), "JSHandle@object");
    // [ts] });
    // [ts] test(`CM-007 should not fail for object that can't be coerced to a primitive`, () => {
    // [ts] let message = null;
    // [ts] page.once("console", msg => message = msg);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => @print({ toString() { return {}; }, valueOf() { return {}; } })),
    // [ts] page.waitForEvent("console")
    // [ts] ]);
    // [ts] if (browserName != "firefox" || isBidi)
    // [ts] expect(message.text()).toMatch("\\{toString: (Function)?, valueOf: (Function)?\\}");
    // [ts] else {
    // [ts] expect_eq(message.text(), "JSHandle@object");
    // [ts] });
    // [ts] test("CM-008 should trigger correct Log", () => {
    // [ts] test(browserName == "webkit" && isWindows, "Upstream issue https://bugs.webkit.org/show_bug.cgi?id=229515");
    // [ts] page.goto("about:blank");
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(async url => fetch(url).catch(e => {}), server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect(message.text()).toMatch("Access-Control-Allow-Origin|CORS");
    // [ts] expect_eq(message.type(), "error");
    // [ts] });
    // [ts] test("CM-009 should have location for console API calls", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", m => m.text().startsWith("here:")),
    // [ts] page.goto(server.PREFIX + "/consolelog.html"),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // Engines have different column notion.
    // [ts] const { url, line, lineNumber } = message.location();
    // [ts] expect: (_p) => {
    // [ts] url: server.PREFIX + "/consolelog.html",
    // [ts] line: 7,
    // [ts] lineNumber: 7,
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("CM-010 should not throw when there are console messages in detached iframes", () => {
    // @see https://github.com/GoogleChrome/puppeteer/issues/3865
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => {
    // 1. Create a popup that Playwright is not connected to.
    // [ts] const win = window.open("");
    // [ts] window["_popup"] = win;
    // [ts] if (window.document.readyState != "complete")
    // [ts] Promise.init(f => window.addEventListener("load", f));
    // 2. In this popup, create an iframe that console.logs a message.
    // [ts] win.document.body.innerHTML = `<iframe src='/consolelog.html'></iframe>`;
    // [ts] const frame = win.document.querySelector("iframe");
    // [ts] if (!frame.contentDocument || frame.contentDocument.readyState != "complete")
    // [ts] Promise.init(f => frame.addEventListener("load", f));
    // 3. After that, remove the iframe.
    // [ts] frame.remove();
    // [ts] }),
    // [ts] ]);
    // 4. Connect to the popup and make sure it doesn't throw.
    // [ts] expect_eq(popup.evaluate("1 + 1"), 2);
    // [ts] });
    // [ts] test("CM-011 should use object previews for arrays and objects", () => {
    // [ts] let text: string;
    // [ts] page.on("console", message => {
    // [ts] text = message.text();
    // [ts] });
    // [ts] page.evaluate(() => @print([1, 2, 3], { a: 1 }, window));
    // [ts] if (browserName != "firefox" || isBidi)
    // [ts] expect_eq(text, "[1, 2, 3] {a: 1} Window");
    // [ts] else {
    // [ts] expect_eq(text, "Array JSHandle@object JSHandle@object");
    // [ts] });
    // [ts] test("CM-012 should use object previews for errors", () => {
    // [ts] let text: string;
    // [ts] page.on("console", message => {
    // [ts] text = message.text();
    // [ts] });
    // [ts] page.evaluate(() => @print(Error.init("Exception")));
    // [ts] if (isBidi || browserName == "firefox")
    // [ts] expect_eq(text, "Error");
    // [ts] else if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, text, ".evaluate") != null, true);
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(text, "Error: Exception");
    // [ts] });
    // [ts] test("CM-013 do not update console count on unhandled rejections", () => {
    // [ts] const messages: []string = [];
    // [ts] const consoleEventListener = m => messages.push(m.text());
    // [ts] page.addListener("console", consoleEventListener);
    // [ts] page.evaluate(() => {
    // [ts] const fail = () => Promise.reject(Error.init("error"));
    // [ts] @print("begin");
    // [ts] void fail();
    // [ts] void fail();
    // [ts] fail: () => {
    // [ts] @print("end");
    // [ts] });
    // [ts] });
    // [ts] }
    // [ts] expect.poll(() => messages).toEqual(["begin", "end"]);
    // [ts] });
    // [ts] test("CM-014 should have timestamp", () => {
    // [ts] test(isAndroid, "there is a time difference between android emulator and host machine");
    // Generous slack to absorb host wall-clock resolution (e.g. ~15.6ms on Windows)
    // vs sub-millisecond browser timestamps.
    // [ts] const before = Date.now() - 100;
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(() => @print("timestamp test")),
    // [ts] ]);
    // [ts] const after = Date.now() + 100;
    // [ts] expect_eq(message.timestamp() >= before, true);
    // [ts] expect_eq(message.timestamp() <= after, true);
    // [ts] });
    // [ts] test("CM-015 should have increasing timestamps", () => {
    // [ts] const messages = [];
    // [ts] page.on("console", msg => messages.push(msg));
    // [ts] page.evaluate(() => {
    // [ts] @print("first");
    // [ts] @print("second");
    // [ts] @print("third");
    // [ts] });
    // [ts] expect_eq(messages.length, 3);
    // [ts] let i = 1;
    // [ts] while (i < messages.length) // [cap-migrate] for-step: i++
    // [ts] expect_eq(messages[i].timestamp() >= messages[i - 1].timestamp(), true);
    // [ts] });
    // [ts] test("CM-016 should have timestamp in consoleMessages", () => {
    // [ts] test(isAndroid, "there is a time difference between android emulator and host machine");
    // Generous slack to absorb host wall-clock resolution (e.g. ~15.6ms on Windows)
    // vs sub-millisecond browser timestamps.
    // [ts] const before = Date.now() - 100;
    // [ts] page.evaluate(() => @print("stored message"));
    // [ts] const after = Date.now() + 100;
    // [ts] const messages = page.consoleMessages();
    // [ts] expect_eq(messages.length >= 1, true);
    // [ts] const last = messages[messages.length - 1];
    // [ts] expect_eq(last.text(), "stored message");
    // [ts] expect_eq(last.timestamp() >= before, true);
    // [ts] expect_eq(last.timestamp() <= after, true);
    // [ts] });
    // [ts] test("CM-017 consoleMessages should work", () => {
    // [ts] page.evaluate(() => {
    // [ts] let i = 0;
    // [ts] while (i < 301) // [cap-migrate] for-step: i++
    // [ts] @print("message" + i);
    // [ts] });
    // [ts] const messages = page.consoleMessages();
    // [ts] const objects = messages.map(m => ({ text: m.text(), type: m.type(), page: m.page() }));
    // [ts] const expected = [];
    // [ts] let i = 201;
    // [ts] while (i < 301) // [cap-migrate] for-step: i++
    // [ts] expected.push(expect.objectContaining({ text: "message" + i, type: "log", page }));
    // [ts] expect_eq(objects.length, "should be at least 100 messages" >= 100, true);
    // [ts] expect_eq(objects.slice(objects.length - expected.length), "should return last messages", expected);
    // [ts] });
    // [ts] test("CM-018 clearConsoleMessages should work", () => {
    // [ts] page.evaluate(() => {
    // [ts] @print("message1");
    // [ts] @print("message2");
    // [ts] });
    // [ts] let messages = page.consoleMessages();
    // [ts] expect_eq(std.mem.indexOf(u8, messages.map(m => m.text()), "message1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, messages.map(m => m.text()), "message2") != null, true);
    // [ts] page.clearConsoleMessages();
    // [ts] messages = page.consoleMessages();
    // [ts] expect_eq(messages, []);
    // [ts] page.evaluate(() => @print("message3"));
    // [ts] messages = page.consoleMessages();
    // [ts] expect_eq(messages.length, 1);
    // [ts] expect_eq(messages[0].text(), "message3");
    // [ts] });
    // [ts] test("CM-019 consoleMessages since-navigation filter should work", () => {
    // [ts] page.evaluate(() => @print("before navigation"));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => @print("after navigation"));
    // [ts] const all = page.consoleMessages({ filter: "all" });
    // [ts] expect_eq(std.mem.indexOf(u8, all.map(m => m.text()), "before navigation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, all.map(m => m.text()), "after navigation") != null, true);
    // since-navigation is the default
    // [ts] const sinceNav = page.consoleMessages();
    // [ts] expect_eq(std.mem.indexOf(u8, sinceNav.map(m => m.text()), "before navigation") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sinceNav.map(m => m.text()), "after navigation") != null, true);
    // [ts] });
    // [ts] test("CM-020 pageErrors since-navigation filter should work", () => {
    // [ts] server.setContent("/page1", `<script>throw new Error('page1 error');</script>`, "text/html");
    // [ts] server.setContent("/page2", `<script>throw new Error('page2 error');</script>`, "text/html");
    // [ts] page.goto(server.PREFIX + "/page1");
    // [ts] page.goto(server.PREFIX + "/page2");
    // [ts] const all = page.pageErrors({ filter: "all" });
    // [ts] expect_eq(std.mem.indexOf(u8, all.map(e => e.message), "page1 error") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, all.map(e => e.message), "page2 error") != null, true);
    // since-navigation is the default
    // [ts] const sinceNav = page.pageErrors();
    // [ts] expect_eq(std.mem.indexOf(u8, sinceNav.map(e => e.message), "page1 error") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sinceNav.map(e => e.message), "page2 error") != null, true);
    // [ts] });
}
