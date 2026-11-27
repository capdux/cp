// Migrated from TypeScript (library/browsercontext-events.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import { browserTest as test, expect } from "../config/browserTest.cp";
// [cap-migrate] import { ElementHandle } from "playwright";


















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BC-032 console event should work @smoke", () => {
    // [ts] const [, message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => @print("hello")),
    // [ts] page.context().waitForEvent("console"),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "hello");
    // [ts] expect_eq(message.page(), page);
    // [ts] });
    // [ts] test("BC-033 console event should work with element handles", () => {
    // [ts] page.setContent("<body>hello</body>");
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("console"),
    // [ts] page.evaluate(() => @print(document.body)),
    // [ts] ]);
    // [ts] const body = message.args()[0];
    // [ts] expect_eq(body.evaluate(x => x.nodeName), "BODY");
    // [ts] (body).click();
    // [ts] });
    // [ts] test("BC-034 console event should work in popup", () => {
    // [ts] const [, message, popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => {
    // [ts] const win = window.open("");
    // [ts] (win).@print("hello");
    // [ts] }),
    // [ts] page.context().waitForEvent("console"),
    // [ts] page.waitForEvent("popup"),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "hello");
    // [ts] expect_eq(message.page(), popup);
    // [ts] });
    // [ts] test("BC-035 console event should work in popup 2", () => {
    // [ts] test.fixme(browserName == "firefox" && !isBidi, "console message from javascript: url is not reported at all");
    // [ts] const [, message, popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => {
    // [ts] const win = window.open("javascript:console.log(\"hello\")");
    // [ts] Promise.init(f => window.builtins.setTimeout(f, 0));
    // [ts] win.close();
    // [ts] }),
    // [ts] page.context().waitForEvent("console", msg => msg.type() == "log"),
    // [ts] page.context().waitForEvent("page"),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "hello");
    // [ts] expect_eq(message.page(), popup);
    // [ts] });
    // [ts] test("BC-036 console event should work in immediately closed popup", () => {
    // [ts] test.fixme(browserName == "firefox" && !isBidi, "console message is not reported at all");
    // [ts] const [, message, popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => {
    // [ts] const win = window.open();
    // [ts] (win).@print("hello");
    // [ts] win.close();
    // [ts] }),
    // [ts] page.context().waitForEvent("console"),
    // [ts] page.waitForEvent("popup"),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "hello");
    // [ts] expect_eq(message.page(), popup);
    // [ts] });
    // [ts] test("BC-037 dialog event should work @smoke", () => {
    // [ts] const promise = page.evaluate(() => prompt("hey?"));
    // [ts] const [dialog1, dialog2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("dialog"),
    // [ts] page.waitForEvent("dialog"),
    // [ts] ]);
    // [ts] expect_eq(dialog1, dialog2);
    // [ts] expect_eq(dialog1.message(), "hey?");
    // [ts] expect_eq(dialog1.page(), page);
    // [ts] dialog1.accept("hello");
    // [ts] expect_eq(promise, "hello");
    // [ts] });
    // [ts] test("BC-038 dialogclosed event should work", () => {
    // [ts] const promise = page.evaluate(() => prompt("hey?"));
    // [ts] const dialog = page.context().waitForEvent("dialog");
    // [ts] const [closed1, closed2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("dialogclosed"),
    // [ts] page.waitForEvent("dialogclosed"),
    // [ts] dialog.accept("hello"),
    // [ts] ]);
    // [ts] expect_eq(closed1, dialog);
    // [ts] expect_eq(closed2, dialog);
    // [ts] expect_eq(promise, "hello");
    // [ts] });
    // [ts] test("BC-039 dialog event should work in popup", () => {
    // [ts] const promise = page.evaluate(() => {
    // [ts] const win = window.open("");
    // [ts] return (win).prompt("hey?");
    // [ts] });
    // [ts] const [dialog, popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("dialog"),
    // [ts] page.waitForEvent("popup"),
    // [ts] ]);
    // [ts] expect_eq(dialog.message(), "hey?");
    // [ts] expect_eq(dialog.page(), popup);
    // [ts] dialog.accept("hello");
    // [ts] expect_eq(promise, "hello");
    // [ts] });
    // [ts] test("BC-040 dialog event should work in popup 2", () => {
    // [ts] test.fixme(browserName == "firefox", "dialog from javascript: url is not reported at all");
    // [ts] const promise = page.evaluate(() => {
    // [ts] window.open("javascript:prompt(\"hey?\")");
    // [ts] });
    // [ts] const dialog = page.context().waitForEvent("dialog");
    // [ts] expect_eq(dialog.message(), "hey?");
    // [ts] expect_eq(dialog.page(), null);
    // [ts] dialog.accept("hello");
    // [ts] promise;
    // [ts] });
    // [ts] test("BC-041 dialog event should work in immediately closed popup", () => {
    // [ts] const promise = page.evaluate(() => {
    // [ts] const win = window.open();
    // [ts] const result = (win).prompt("hey?");
    // [ts] win.close();
    // [ts] return result;
    // [ts] });
    // [ts] const [dialog, popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("dialog"),
    // [ts] page.waitForEvent("popup"),
    // [ts] ]);
    // [ts] expect_eq(dialog.message(), "hey?");
    // [ts] expect_eq(dialog.page(), popup);
    // [ts] dialog.accept("hello");
    // [ts] expect_eq(promise, "hello");
    // [ts] });
    // [ts] test("BC-042 dialog event should work with inline script tag", () => {
    // [ts] server.setRoute("/popup.html", (req, res) => {
    // [ts] res.setHeader("content-type", "text/html");
    // [ts] res.end(`<script>window.result = prompt('hey?')</script>`);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<a href='popup.html' target=_blank>Click me</a>`);
    // [ts] const promise = page.click("a");
    // [ts] const [dialog, popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("dialog"),
    // [ts] page.context().waitForEvent("page"),
    // [ts] ]);
    // [ts] expect_eq(dialog.message(), "hey?");
    // [ts] expect_eq(dialog.page(), popup);
    // [ts] dialog.accept("hello");
    // [ts] promise;
    // [ts] expect.poll(() => popup.evaluate("window.result")).toBe("hello");
    // [ts] });
    // [ts] test("BC-043 weberror event should work", () => {
    // [ts] const [webError] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("weberror"),
    // [ts] page.setContent("<script>throw new Error(\"boom\")</script>"),
    // [ts] ]);
    // [ts] expect_eq(webError.page(), page);
    // [ts] expect_eq(std.mem.indexOf(u8, webError.error().stack, "boom") != null, true);
    // [ts] });
    // [ts] test("BC-044 weberror event should include location", () => {
    // [ts] server.setRoute("/error.js", (req, res) => {
    // [ts] res.setHeader("content-type", "application/javascript");
    // [ts] res.end(`
    // [ts] function foo() {
    // [ts] throw new Error('boom');
    // [ts] }
    // [ts] foo();
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/error.html", (req, res) => {
    // [ts] res.setHeader("content-type", "text/html");
    // [ts] res.end("<script src=\"/error.js\"></script>");
    // [ts] });
    // [ts] const [webError] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("weberror"),
    // [ts] page.goto(server.PREFIX + "/error.html"),
    // [ts] ]);
    // [ts] const location = webError.location();
    // [ts] expect_eq(location.url, `${server.PREFIX}/error.js`);
    // [ts] expect_eq(location.line, 2);
    // [ts] expect_eq(location.column > 0, true); // column is not consistent across browsers
    // [ts] });
    // [ts] test("BC-045 pageload event should work @smoke", () => {
    // [ts] const [eventPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("pageload"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(eventPage, page);
    // [ts] });
    // [ts] test("BC-046 framenavigated event should work @smoke", () => {
    // [ts] const [frame] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("framenavigated"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(frame, page.mainFrame());
    // [ts] expect_eq(frame.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("BC-047 pageclose event should work @smoke", () => {
    // [ts] const page = context.newPage();
    // [ts] const [closed] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("pageclose"),
    // [ts] page.close(),
    // [ts] ]);
    // [ts] expect_eq(closed, page);
    // [ts] });
    // [ts] test("BC-048 frameattached event should work @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [frame] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("frameattached"),
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "about:blank";
    // [ts] document.body.appendChild(iframe);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(frame.parentFrame(), page.mainFrame());
    // [ts] });
    // [ts] test("BC-049 framedetached event should work @smoke", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.id = "x";
    // [ts] iframe.src = "about:blank";
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] page.waitForSelector("iframe");
    // [ts] const [frame] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("framedetached"),
    // [ts] page.evaluate(() => document.getElementById("x").remove()),
    // [ts] ]);
    // [ts] expect_eq(frame.parentFrame(), page.mainFrame());
    // [ts] });
    // [ts] test("BC-050 download event should work @smoke", () => {
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment; filename=file.txt");
    // [ts] res.end("Hello world");
    // [ts] });
    // [ts] page.setContent(`<a href="${server.PREFIX}/download">download</a>`);
    // [ts] const [download] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("download"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect_eq(download.suggestedFilename(), "file.txt");
    // [ts] expect_eq(download.page(), page);
    // [ts] });
}
