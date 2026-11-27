// Migrated from TypeScript (page/page-event-pageerror.spec.ts) via cap from -ts

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
// [cap-migrate] import { ServerResponse } from "cap:http";












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("ER-001 should fire", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43450" },
    // [ts] }, (_p) => {
    // [ts] const url = server.PREFIX + "/error.html";
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.goto(url),
    // [ts] ]);
    // [ts] expect_eq(error.name, "Error");
    // [ts] expect_eq(error.message, "Fancy error!");
    // [ts] if (browserName == "chromium" || (browserName == "firefox" && isBidi)) {
    // [ts] expect(error.stack).toBe(`Error: Fancy error!
    // [ts] at c (myscript.js:14:11)
    // [ts] at b (myscript.js:10:5)
    // [ts] at a (myscript.js:6:5)
    // [ts] at myscript.js:3:1`);
    // [ts] } else if (browserName == "webkit") {
    // [ts] expect(error.stack).toBe(`Error: Fancy error!
    // [ts] at c (${url}:14:36)
    // [ts] at b (${url}:10:6)
    // [ts] at a (${url}:6:6)
    // [ts] at global code (${url}:3:2)`);
    // [ts] } else if (browserName == "firefox") {
    // [ts] expect(error.stack).toBe(`Error: Fancy error!
    // [ts] at c (myscript.js:14:11)
    // [ts] at b (myscript.js:10:5)
    // [ts] at a (myscript.js:6:5)
    // [ts] at  (myscript.js:3:1)`);
    // [ts] }
    // The same script from another origin: Chromium reports it without an exception object,
    // so the stack is built from the CDP call frames, which are zero-based.
    // [ts] const crossOriginUrl = server.CROSS_PROCESS_PREFIX + "/error.js";
    // [ts] server.setRoute("/error.js", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/javascript");
    // Same lines as error.html, so the stack has the same locations.
    // [ts] res.end(`
    // [ts] console.error('Not a JS error');
    // [ts] a();
    // [ts] function a() {
    // [ts] b();
    // [ts] }
    // [ts] function b() {
    // [ts] c();
    // [ts] }
    // [ts] function c() {
    // [ts] throw new Error('Fancy error!');
    // [ts] }
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/cross-origin-error.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<script src="${crossOriginUrl}"></script>`);
    // [ts] });
    // [ts] const [crossOriginError] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.goto(server.PREFIX + "/cross-origin-error.html"),
    // [ts] ]);
    // [ts] expect_eq(crossOriginError.message, "Fancy error!");
    // [ts] if (browserName == "chromium" && browserMajorVersion >= 155) {
    // [ts] expect(crossOriginError.stack).toBe(`Uncaught Error: Fancy error!
    // [ts] at c (${crossOriginUrl}:14:11)
    // [ts] at b (${crossOriginUrl}:10:5)
    // [ts] at a (${crossOriginUrl}:6:5)
    // [ts] at <anonymous> (${crossOriginUrl}:3:1)`);
    // [ts] } else if (browserName == "webkit") {
    // [ts] expect(crossOriginError.stack).toBe(`Error: Fancy error!
    // [ts] at c (${crossOriginUrl}:14:36)
    // [ts] at b (${crossOriginUrl}:10:6)
    // [ts] at a (${crossOriginUrl}:6:6)
    // [ts] at global code (${crossOriginUrl}:3:2)`);
    // [ts] } else if (browserName == "firefox" && !isBidi) {
    // [ts] expect(crossOriginError.stack).toBe(`Error: Fancy error!
    // [ts] at c (${crossOriginUrl}:14:11)
    // [ts] at b (${crossOriginUrl}:10:5)
    // [ts] at a (${crossOriginUrl}:6:5)
    // [ts] at  (${crossOriginUrl}:3:1)`);
    // [ts] }
    // [ts] });
    // [ts] test("ER-002 should not receive console message for pageError", () => {
    // [ts] const messages = [];
    // [ts] page.on("console", e => messages.push(e));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.goto(server.PREFIX + "/error.html"),
    // [ts] ]);
    // [ts] expect_eq(messages.length, 1);
    // [ts] });
    // [ts] test("ER-003 should contain sourceURL", () => {
    // [ts] it.fail(browserName == "webkit");
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.goto(server.PREFIX + "/error.html"),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, "myscript.js") != null, true);
    // [ts] });
    // [ts] test("ER-004 should contain the Error.name property", () => {
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => {
    // [ts] const error = Error.init("my-message");
    // [ts] error.name = "my-name";
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }, 0);
    // [ts] })
    // [ts] ]);
    // [ts] expect_eq(error.name, "my-name");
    // [ts] expect_eq(error.message, "my-message");
    // [ts] });
    // [ts] test("ER-005 should support an empty Error.name property", () => {
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => {
    // [ts] const error = Error.init("my-message");
    // [ts] error.name = "";
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }, 0);
    // [ts] })
    // [ts] ]);
    // [ts] expect_eq(error.name, "");
    // [ts] expect_eq(error.message, "my-message");
    // [ts] });
    // [ts] test("ER-006 should handle odd values", () => {
    // [ts] const cases = [
    // [ts] [null, "null"],
    // [ts] [null, "undefined"],
    // [ts] [0, "0"],
    // [ts] ["", ""],
    // [ts] ];
    // [ts] for (const [value, message] of cases) {
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.evaluate(value => {
    // [ts] window.builtins.setTimeout(() => { throw value; }, 0);
    // [ts] }, value),
    // [ts] ]);
    // [ts] expect_eq(error.message, message);
    // [ts] }
    // [ts] });
    // [ts] test("ER-007 should handle object", () => {
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => { throw {}; }, 0);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(error.message, if (browserName == "chromium") "Object" else "[object Object]");
    // [ts] });
    // [ts] test("ER-008 should handle window", () => {
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => { throw window; }, 0);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(error.message, if (browserName == "chromium") "Window" else "[object Window]");
    // [ts] });
    // [ts] test("ER-009 should remove a listener of a non-existing event handler", () => {
    // [ts] page.removeListener("pageerror", () => {});
    // [ts] });
    // [ts] test("ER-010 should emit error from unhandled rejects", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/14165" });
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.setContent(`
    // [ts] <script>
    // [ts] Promise.reject(new Error('sad :('));
    // [ts] </script>
    // [ts] `),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "sad :(") != null, true);
    // [ts] });
    // [ts] test("ER-011 pageErrors should work", () => {
    // [ts] page.evaluate(() => {
    // [ts] let i = 0;
    // [ts] while (i < 301) // [cap-migrate] for-step: i++
    // [ts] window.builtins.setTimeout(() => { throw Error.init("error" + i); }, 0);
    // [ts] Promise.init(f => window.builtins.setTimeout(f, 2000));
    // [ts] });
    // [ts] const errors = page.pageErrors();
    // [ts] const messages = errors.map(e => e.message);
    // [ts] const expected = [];
    // [ts] let i = 201;
    // [ts] while (i < 301) // [cap-migrate] for-step: i++
    // [ts] expected.push("error" + i);
    // [ts] expect_eq(messages.length, "should be at least 100 errors" >= 100, true);
    // [ts] expect_eq(messages.slice(messages.length - expected.length), "should return last errors", expected);
    // [ts] });
    // [ts] test("ER-012 clearPageErrors should work", () => {
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => { throw Error.init("error1"); }, 0);
    // [ts] window.builtins.setTimeout(() => { throw Error.init("error2"); }, 0);
    // [ts] });
    // [ts] page.waitForTimeout(1000);
    // [ts] let errors = page.pageErrors();
    // [ts] expect_eq(std.mem.indexOf(u8, errors.map(e => e.message), "error1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, errors.map(e => e.message), "error2") != null, true);
    // [ts] page.clearPageErrors();
    // [ts] errors = page.pageErrors();
    // [ts] expect_eq(errors, []);
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => { throw Error.init("error3"); }, 0);
    // [ts] });
    // [ts] page.waitForTimeout(1000);
    // [ts] errors = page.pageErrors();
    // [ts] expect_eq(errors.length, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, errors[0].message, "error3") != null, true);
    // [ts] });
    // [ts] test("ER-013 should fire illegal character error", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/38388" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/error.html", (req, res) => {
    // [ts] res.end(`
    // [ts] <!doctype html>
    // [ts] <html lang="en">
    // [ts] <head>
    // [ts] <meta charset="UTF-8" />
    // [ts] <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    // [ts] <title>vite-project</title>
    // [ts] </head>
    // [ts] <body>
    // [ts] <div id="app"></div>
    // [ts] <script >
    // [ts] let a=10；//   !!!note: ； not ;
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] });
    // [ts] const url = server.PREFIX + "/error.html";
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.goto(url),
    // [ts] ]);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Invalid or unexpected token") != null, true);
    // [ts] else if (browserName == "webkit" && isWindows && channel != "webkit-wsl")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "No identifiers allowed directly after numeric literal") != null, true);
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Invalid character") != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "illegal character") != null, true);
    // [ts] });
    // [ts] test("ER-022 should not fire for a caught fetch rejected by a pending navigation", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44015" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const errors: []string = [];
    // [ts] page.on("pageerror", error => errors.push(error.message));
    // [ts] const waitResponse = Promise.init(resolve => server.setRoute("/wait", (req, res) => resolve(res)));
    // [ts] const replacementResponse = Promise.init(resolve => server.setRoute("/replacement", (req, res) => resolve(res)));
    // [ts] const fetchDone = page.waitForEvent("console", message => message.text() == "fetch done");
    // Chromium does not run evaluate during a pending navigation, so the page starts the second fetch.
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/wait").catch(() => {}).then(() => fetch("/empty.html")).catch(() => {}).then(() => @print("fetch done"));
    // [ts] });
    // [ts] const navigation = page.goto(server.PREFIX + "/replacement");
    // [ts] const response = replacementResponse;
    // WebKit rejects the second fetch, because the document is about to go away.
    // [ts] (waitResponse).end();
    // [ts] fetchDone;
    // [ts] response.end("replacement");
    // [ts] navigation;
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("pageerror"),
    // [ts] page.evaluate(() => window.builtins.setTimeout(() => { throw Error.init("genuine error"); }, 0)),
    // [ts] ]);
    // [ts] expect_eq(error.message, "genuine error");
    // [ts] expect_eq(errors, ["genuine error"]);
    // [ts] });
    // [ts] }
}
