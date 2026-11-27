// Migrated from TypeScript (src/console.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html, waitEvent } from "./utils.cp";

// [cap-test] describe("console", () => {
  setupTestBrowserHooks();




  // @see https://github.com/puppeteer/puppeteer/issues/3865


// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return @print("hello", 5, {foo: "bar"});
    // [ts] }),
    // [ts] ]);
    // [ts] expect(message.text()).atLeastOneToContain([
    // [ts] "hello 5 [object Object]",
    // [ts] "hello 5 JSHandle@object", // WebDriver BiDi
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.args()).toHaveLength(3);
    // [ts] expect_eq(message.args()[0].jsonValue(), "hello");
    // [ts] expect_eq(message.args()[1].jsonValue(), 5);
    // [ts] expect_eq(message.args()[2].jsonValue(), {foo: "bar"});
    // [ts] });
    // [ts] test("should include null and undefined in text()", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return @print("hello", null, null);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "hello null undefined");
    // [ts] expect(message.args()).toHaveLength(3);
    // [ts] });
    // [ts] test("should work for Error instances", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return @print(Error.init("test error"));
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "Error: test error");
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.args()).toHaveLength(1);
    // [ts] });
    // [ts] test("should return the first line of the error message in text()", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return @print(Error.init("test error\nsecond line"));
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "Error: test error");
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.args()).toHaveLength(1);
    // [ts] });
    // [ts] test("should work on script call right after navigation", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.goto(
    // Firefox prints warn if <!DOCTYPE html> is not present
    // [ts] `data:text/html,<!DOCTYPE html><script>console.log('SOME_LOG_MESSAGE');</script>`,
    // [ts] ),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "SOME_LOG_MESSAGE");
    // [ts] });
    // [ts] test("should work for console.trace", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] console.trace("calling console.trace");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "trace");
    // [ts] expect_eq(message.text(), "calling console.trace");
    // [ts] });
    // [ts] test("should work for console.dir", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] console.dir("calling console.dir");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "dir");
    // [ts] expect_eq(message.text(), "calling console.dir");
    // [ts] });
    // [ts] test("should work for console.warn", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] @print("calling console.warn");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "warn");
    // [ts] expect_eq(message.text(), "calling console.warn");
    // [ts] });
    // [ts] test("should work for console.error", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] @print("calling console.error");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "error");
    // [ts] expect_eq(message.text(), "calling console.error");
    // [ts] });
    // [ts] test("should work for console.log with promise", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] @print(Promise.resolve("should not wait until resolved!"));
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.text()).atLeastOneToContain([
    // [ts] "[promise Promise]",
    // [ts] "JSHandle@promise", // WebDriver BiDi expectation.
    // [ts] ]);
    // [ts] });
    // [ts] test("should work for different console API calls with timing functions", () => {
    // [ts] const {page} = getTestState();
    // [ts] const messages: []string = [];
    // [ts] page.on("console", msg => {
    // [ts] return messages.push(msg);
    // [ts] });
    // All console events will be reported before `page.evaluate` is finished.
    // [ts] page.evaluate(() => {
    // A pair of time/timeEnd generates only one Console API call.
    // [ts] console.time("calling console.time");
    // [ts] console.timeEnd("calling console.time");
    // [ts] });
    // [ts] expect(
    // [ts] messages.map(msg => {
    // [ts] return msg.type();
    // [ts] }),
    // [ts] ).toEqual(["timeEnd"]);
    // [ts] expect_eq(std.mem.indexOf(u8, messages[0].text(), "calling console.time") != null, true);
    // [ts] });
    // [ts] test("should work for different console API calls with group functions", () => {
    // [ts] const {page} = getTestState();
    // [ts] const messages: []ConsoleMessage = [];
    // [ts] page.on("console", msg => {
    // [ts] return messages.push(msg);
    // [ts] });
    // All console events will be reported before `page.evaluate` is finished.
    // [ts] page.evaluate(() => {
    // [ts] console.group("calling console.group");
    // [ts] console.groupEnd();
    // [ts] });
    // [ts] expect(
    // [ts] messages.map(msg => {
    // [ts] return msg.type();
    // [ts] }),
    // [ts] ).toEqual(["startGroup", "endGroup"]);
    // We should be able to check both messages, but Chrome report text
    // [ts] expect_eq(std.mem.indexOf(u8, messages[0].text(), "calling console.group") != null, true);
    // [ts] });
    // [ts] test("should not fail for window object", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return @print(window);
    // [ts] }),
    // [ts] ]);
    // [ts] expect(message.text()).atLeastOneToContain([
    // [ts] "[object Window]",
    // [ts] "JSHandle@window", // WebDriver BiDi
    // [ts] ]);
    // [ts] });
    // [ts] test("should return remote objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] const logPromise = waitEvent(page, "console");
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).test = 1;
    // [ts] @print(1, 2, 3, globalThis);
    // [ts] });
    // [ts] const log = logPromise;
    // [ts] expect(log.text()).atLeastOneToContain([
    // [ts] "1 2 3 [object Window]",
    // [ts] "1 2 3 JSHandle@window", // WebDriver BiDi
    // [ts] ]);
    // [ts] expect(log.args()).toHaveLength(4);
    // [ts] using property = log.args()[3].getProperty("test");
    // [ts] expect_eq(property.jsonValue(), 1);
    // [ts] });
    // [ts] test("should not dispose handles when page has listeners", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return @print({foo: "bar"});
    // [ts] }),
    // [ts] ]);
    // [ts] using handle = message.args()[0];
    // [ts] expect_eq(handle.disposed, false);
    // [ts] expect_eq(handle.jsonValue(), {foo: "bar"});
    // [ts] handle.dispose();
    // [ts] expect_eq(handle.disposed, true);
    // [ts] });
    // [ts] test("should trigger correct Log", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] page.goto(`http://domain1.test:${server.PORT}/empty.html`);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(async url => {
    // [ts] return fetch(url).catch(() => {});
    // [ts] }, `http://domain2.test:${server.PORT}/empty.html`),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, message.text(), "Access-Control-Allow-Origin") != null, true);
    // [ts] if (isChrome) {
    // [ts] expect_eq(message.type(), "error");
    // [ts] } else {
    // [ts] expect_eq(message.type(), "warn");
    // [ts] }
    // [ts] });
    // [ts] test("should have location when fetch fails", () => {
    // [ts] const {page, server} = getTestState();
    // The point of this test is to make sure that we report console messages from
    // Log domain: https://vanilla.aslushnikov.com/?Log.entryAdded
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.setContent(
    // [ts] html`<script>
    // [ts] fetch('http://wat');
    // [ts] </script>`,
    // [ts] ),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, message.text(), `ERR_NAME_NOT_RESOLVED`) != null, true);
    // [ts] expect_eq(message.type(), "error");
    // [ts] expect: (message.location()) => {
    // [ts] url: "http://wat/",
    // [ts] lineNumber: null,
    // [ts] });
    // [ts] });
    // [ts] test("should have location and stack trace for console API calls", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const isCdp = browser.protocol == "cdp";
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.goto(server.PREFIX + "/consoletrace.html"),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "yellow");
    // [ts] expect_eq(message.type(), "trace");
    // [ts] expect: (message.location()) => {
    // [ts] url: server.PREFIX + "/consoletrace.html",
    // [ts] lineNumber: 8,
    // [ts] columnNumber: 16,
    // [ts] ...(if (isCdp) {scriptId: expect.any(String)} else {}),
    // [ts] });
    // [ts] expect(message.stackTrace()).toEqual([
    // [ts] {
    // [ts] url: server.PREFIX + "/consoletrace.html",
    // [ts] lineNumber: 8,
    // [ts] columnNumber: 16,
    // [ts] ...(if (isCdp) {scriptId: expect.any(String)} else {}),
    // [ts] },
    // [ts] {
    // [ts] url: server.PREFIX + "/consoletrace.html",
    // [ts] lineNumber: 11,
    // [ts] columnNumber: 8,
    // [ts] ...(if (isCdp) {scriptId: expect.any(String)} else {}),
    // [ts] },
    // [ts] {
    // [ts] url: server.PREFIX + "/consoletrace.html",
    // [ts] lineNumber: 13,
    // [ts] columnNumber: 6,
    // [ts] ...(if (isCdp) {scriptId: expect.any(String)} else {}),
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should have scriptId in stack trace for eval console API calls", () => {
    // [ts] const {page, browser} = getTestState();
    // [ts] const isCdp = browser.protocol == "cdp";
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: eval(
    // [ts] "function dynamicFoo() { console.trace('from eval'); }\ndynamicFoo();",
    // [ts] );
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "from eval");
    // [ts] expect_eq(message.type(), "trace");
    // [ts] if (isCdp) {
    // [ts] expect_eq(message.location().scriptId, expect.any(String));
    // [ts] expect_eq(message.stackTrace()[0]?.scriptId, expect.any(String));
    // [ts] }
    // [ts] });
    // [ts] test("should not throw when there are console messages in detached iframes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // 1. Create a popup that Puppeteer is not connected to.
    // [ts] const win = window.open(
    // [ts] window.location.href,
    // [ts] "Title",
    // [ts] "toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,width=780,height=200,top=0,left=0",
    // [ts] );
    // [ts] Promise.init(x => {
    // [ts] return (win.onload = x);
    // [ts] });
    // 2. In this popup, create an iframe that console.logs a message.
    // [ts] win.document.body.innerHTML = `<iframe src='/consolelog.html'></iframe>`;
    // [ts] const frame = win.document.querySelector("iframe");
    // [ts] Promise.init(x => {
    // [ts] return (frame.onload = x);
    // [ts] });
    // 3. After that, remove the iframe.
    // [ts] frame.remove();
    // [ts] });
    // 4. The target will always be the last one.
    // [ts] const popupTarget = page.browserContext().targets().at(-1);
    // 5. Connect to the popup and make sure it doesn't throw and is not the same page.
    // [ts] expect_eq(popupTarget.page() != page, true);
    // [ts] });
    // [ts] test("should report the originating frame and page for main frame logs", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return @print("hello from main frame");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.frame(), page.mainFrame());
    // [ts] expect_eq(message.page(), page);
    // [ts] expect_eq(message.worker() == null, true);
    // [ts] });
    // [ts] test("should report the originating frame and page for iframe logs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<iframe></iframe>`);
    // [ts] using iframeElement = (page.$("iframe"));
    // [ts] const iframe = iframeElement.contentFrame();
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] iframe.evaluate(() => {
    // [ts] return @print("hello from iframe");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.frame(), iframe);
    // [ts] expect_eq(message.frame() != page.mainFrame(), true);
    // [ts] expect_eq(message.page(), page);
    // [ts] expect_eq(message.worker() == null, true);
    // [ts] });
}
