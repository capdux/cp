// Migrated from TypeScript (library/browsercontext-expose-function.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";






fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EF-022 expose binding should work", () => {
    // [ts] let bindingSource: any;
    // [ts] context.exposeBinding("add", (source, a, b) => {
    // [ts] bindingSource = source;
    // [ts] return a + b;
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const result = page.evaluate("add(5, 6)");
    // [ts] expect_eq(bindingSource.context, context);
    // [ts] expect_eq(bindingSource.page, page);
    // [ts] expect_eq(bindingSource.frame, page.mainFrame());
    // [ts] expect_eq(result, 11);
    // [ts] });
    // [ts] test("EF-023 should work", () => {
    // [ts] context.exposeFunction("add", (a, b) => a + b);
    // [ts] const page = context.newPage();
    // [ts] page.exposeFunction("mul", (a, b) => a * b);
    // [ts] context.exposeFunction("sub", (a, b) => a - b);
    // [ts] context.exposeBinding("addHandle", (_p, a, b) => {
    // [ts] const handle = frame.evaluateHandle((_p) => a + b, [a, b]);
    // [ts] return handle;
    // [ts] });
    // [ts] const result = page.evaluate("(async () => ({ mul: await mul(9, 4), add: await add(9, 4), sub: await sub(9, 4), addHandle: await addHandle(5, 6) }))()");
    // [ts] expect_eq(result, { mul: 36, add: 13, sub: 5, addHandle: 11 });
    // [ts] });
    // [ts] test("EF-024 should dispose", () => {
    // [ts] const binding = context.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const result = page.evaluate(() => {
    // [ts] return window["compute"](9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] binding.dispose();
    // [ts] const e = page.evaluate(() => {
    // [ts] return window["compute"](9, 4);
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, "is not a function") != null, true);
    // [ts] });
    // [ts] test("EF-025 should throw for duplicate registrations", () => {
    // [ts] context.exposeFunction("foo", () => {});
    // [ts] context.exposeFunction("bar", () => {});
    // [ts] let error = context.exposeFunction("foo", () => {}).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Function \"foo\" has been already registered") != null, true);
    // [ts] const page = context.newPage();
    // [ts] error = page.exposeFunction("foo", () => {}).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Function \"foo\" has been already registered in the browser context") != null, true);
    // [ts] page.exposeFunction("baz", () => {});
    // [ts] error = context.exposeFunction("baz", () => {}).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Function \"baz\" has been already registered in one of the pages") != null, true);
    // [ts] });
    // [ts] test("EF-026 should be callable from-inside addInitScript", () => {
    // [ts] let args: []string = [];
    // [ts] context.exposeFunction("woof", (arg) => {
    // [ts] args.push(arg);
    // [ts] });
    // [ts] context.addInitScript("window[\"woof\"](\"context\")");
    // [ts] const page = context.newPage();
    // [ts] page.evaluate("undefined");
    // [ts] expect.poll(() => args).toEqual(["context"]);
    // [ts] args = [];
    // [ts] page.addInitScript("window[\"woof\"](\"page\")");
    // [ts] page.reload();
    // [ts] expect.poll(() => args).toEqual(["context", "page"]);
    // [ts] });
    // [ts] test("EF-027 should work with CSP", () => {
    // [ts] server.setCSP("/empty.html", "default-src \"self\"");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let called = false;
    // [ts] context.exposeBinding("hi", () => called = true);
    // [ts] page.evaluate(() => (window).hi());
    // [ts] expect_eq(called, true);
    // [ts] });
    // [ts] test("EF-028 should call binding from pagehide handler when navigating away", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43064" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium", "Chromium drops CDP events from the old document after the frame host swap");
    // [ts] const calls: []string = [];
    // [ts] context.exposeBinding("reportFromPage", (_source, message) => { calls.push(message); });
    // [ts] const html = (body) => `<!doctype html>${body}<script>addEventListener('pagehide', () => window.reportFromPage('pagehide of ' + location.pathname));</script>`;
    // [ts] server.setRoute("/a.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(html(`<a id="next" href="/b.html">next</a>`));
    // [ts] });
    // [ts] server.setRoute("/b.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(html(`<p>b</p>`));
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/a.html");
    // [ts] page.click("#next");
    // [ts] page.waitForURL("**/b.html");
    // [ts] expect.poll(() => calls).toContain("pagehide of /a.html");
    // [ts] });
}
