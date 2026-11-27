// Migrated from TypeScript (page/page-history.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";
import url from "cap:url";















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("NA-071 page.goBack should work @smoke", () => {
    // [ts] expect_eq(page.goBack(), null);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] let response = page.goBack();
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(std.mem.indexOf(u8, response.url(), server.EMPTY_PAGE) != null, true);
    // [ts] response = page.goForward();
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(std.mem.indexOf(u8, response.url(), "/grid.html") != null, true);
    // [ts] response = page.goForward();
    // [ts] expect_eq(response, null);
    // [ts] });
    // [ts] test("NA-072 page.goBack should work with HistoryAPI", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] history.pushState({}, "", "/first.html");
    // [ts] history.pushState({}, "", "/second.html");
    // [ts] });
    // [ts] expect_eq(page.url(), server.PREFIX + "/second.html");
    // [ts] page.goBack();
    // [ts] expect_eq(page.url(), server.PREFIX + "/first.html");
    // [ts] page.goBack();
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] page.goForward();
    // [ts] expect_eq(page.url(), server.PREFIX + "/first.html");
    // [ts] });
    // [ts] test("NA-073 page.goBack should work for file urls", () => {
    // [ts] test(isAndroid, "No files on Android");
    // [ts] test(channel == "webkit-wsl");
    // [ts] const url1 = url.pathToFileURL(asset("consolelog.html")).href;
    // [ts] const url2 = server.PREFIX + "/consolelog.html";
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", message => message.text() == "here:" + url1),
    // [ts] page.goto(url1),
    // [ts] ]);
    // [ts] page.setContent(`<a href='${url2}'>url2</a>`);
    // [ts] expect_eq(page.url().toLowerCase(), url1.toLowerCase());
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", message => message.text() == "here:" + url2),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect_eq(page.url(), url2);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", message => message.text() == "here:" + url1),
    // [ts] page.goBack(),
    // [ts] ]);
    // [ts] expect_eq(page.url().toLowerCase(), url1.toLowerCase());
    // Should be able to evaluate in the new context, and
    // not reach for the old cross-process one.
    // [ts] expect_eq(page.evaluate(() => window.scrollX), 0);
    // Should be able to screenshot.
    // [ts] page.screenshot();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", message => message.text() == "here:" + url2),
    // [ts] page.goForward(),
    // [ts] ]);
    // [ts] expect_eq(page.url(), url2);
    // [ts] expect_eq(page.evaluate(() => window.scrollX), 0);
    // [ts] page.screenshot();
    // [ts] });
    // [ts] test("NA-074 goBack/goForward should work with bfcache-able pages", () => {
    // [ts] page.goto(server.PREFIX + "/cached/bfcached.html");
    // [ts] page.setContent(`<a href=${JSON.stringify(server.PREFIX + "/cached/bfcached.html?foo")}>click me</a>`);
    // [ts] page.click("a");
    // [ts] let response = page.goBack();
    // [ts] expect_eq(response.url(), server.PREFIX + "/cached/bfcached.html");
    // BFCache should be disabled.
    // [ts] expect_eq(page.evaluate("window.didShow"), { persisted: false });
    // [ts] response = page.goForward();
    // [ts] expect_eq(response.url(), server.PREFIX + "/cached/bfcached.html?foo");
    // [ts] });
    // [ts] test("NA-075 page.reload should work", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => window["_foo"] = 10);
    // [ts] page.reload();
    // [ts] expect_eq(page.evaluate(() => window["_foo"]), null);
    // [ts] });
    // [ts] test("NA-076 page.reload should work with data url", () => {
    // [ts] page.goto("data:text/html,hello");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "hello") != null, true);
    // [ts] expect_eq(page.reload(), null);
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "hello") != null, true);
    // [ts] });
    // [ts] test("NA-077 page.reload during renderer-initiated navigation", () => {
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] page.setContent(`<form method='POST' action='/post'>Form is here<input type='submit'></form>`);
    // [ts] server.setRoute("/post", (req, res) => {});
    // [ts] let callback;
    // [ts] const reloadFailedPromise = Promise.init(f => callback = f);
    // [ts] page.once("request", () => {
    // [ts] page.reload().catch(e => {});
    // [ts] callback();
    // [ts] });
    // [ts] const clickPromise = page.click("input[type=submit]").catch(e => {});
    // [ts] reloadFailedPromise;
    // [ts] clickPromise;
    // Form submit should be canceled, and reload should eventually arrive
    // to the original one-style.html.
    // [ts] page.waitForSelector("text=hello");
    // [ts] });
    // [ts] test("NA-078 page.reload should not resolve with same-document navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // 1. Make sure execution contexts are ready for fast evaluate.
    // [ts] page.evaluate("1");
    // 2. Stall the reload request.
    // [ts] let response;
    // [ts] server.setRoute("/empty.html", (req, res) => { response = res; });
    // [ts] const requestPromise = server.waitForRequest("/empty.html");
    // 3. Trigger push state that could resolve the reload.
    // [ts] page.evaluate(() => {
    // [ts] window.history.pushState({}, "");
    // [ts] }).catch(() => {});
    // 4. Trigger the reload, it should not resolve.
    // [ts] const reloadPromise = page.reload();
    // 5. Trigger push state again, for the good measure :)
    // [ts] page.evaluate(() => {
    // [ts] window.history.pushState({}, "");
    // [ts] }).catch(() => {});
    // 5. Serve the request, it should resolve the reload.
    // [ts] requestPromise;
    // [ts] response.end("hello");
    // 6. Check the reload response.
    // [ts] const gotResponse = reloadPromise;
    // [ts] expect_eq(gotResponse.text(), "hello");
    // [ts] });
    // [ts] test("NA-079 page.reload should work with same origin redirect", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16147" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRedirect("/empty.html", server.PREFIX + "/title.html");
    // [ts] page.reload();
    // [ts] expect(page).toHaveURL(server.PREFIX + "/title.html");
    // [ts] });
    // [ts] test("NA-080 page.reload should work with cross-origin redirect", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16147" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRedirect("/empty.html", server.CROSS_PROCESS_PREFIX + "/title.html");
    // [ts] page.reload();
    // [ts] expect(page).toHaveURL(server.CROSS_PROCESS_PREFIX + "/title.html");
    // [ts] });
    // [ts] test("NA-081 page.reload should work on a page with a hash", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21145" });
    // [ts] page.goto(server.EMPTY_PAGE + "#hash");
    // [ts] page.reload();
    // [ts] expect(page).toHaveURL(server.EMPTY_PAGE + "#hash");
    // [ts] });
    // [ts] test("NA-082 page.reload should work on a page with a hash at the end", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21430" });
    // [ts] page.goto(server.EMPTY_PAGE + "#");
    // [ts] page.reload();
    // [ts] expect(page).toHaveURL(server.EMPTY_PAGE + "#");
    // [ts] });
    // [ts] test("NA-083 page.goBack during renderer-initiated navigation", () => {
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<form method='POST' action='/post'>Form is here<input type='submit'></form>`);
    // [ts] server.setRoute("/post", (req, res) => {});
    // [ts] let callback;
    // [ts] const reloadFailedPromise = Promise.init(f => callback = f);
    // [ts] page.once("request", () => {
    // [ts] page.goBack().catch(e => {});
    // [ts] callback();
    // [ts] });
    // [ts] const clickPromise = page.click("input[type=submit]").catch(e => {});
    // [ts] reloadFailedPromise;
    // [ts] clickPromise;
    // Form submit should be canceled, and goBack should eventually arrive
    // to the original one-style.html.
    // [ts] page.waitForSelector("text=hello");
    // [ts] });
    // [ts] test("NA-084 page.goForward during renderer-initiated navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] page.goBack();
    // [ts] page.setContent(`<form method='POST' action='/post'>Form is here<input type='submit'></form>`);
    // [ts] server.setRoute("/post", (req, res) => {});
    // [ts] let callback;
    // [ts] const reloadFailedPromise = Promise.init(f => callback = f);
    // [ts] page.once("request", () => {
    // [ts] page.goForward().catch(e => {});
    // [ts] callback();
    // [ts] });
    // [ts] const clickPromise = page.click("input[type=submit]").catch(e => {});
    // [ts] reloadFailedPromise;
    // [ts] clickPromise;
    // Form submit should be canceled, and goForward should eventually arrive
    // to the original one-style.html.
    // [ts] page.waitForSelector("text=hello");
    // [ts] });
    // [ts] test("NA-085 regression test for issue 20791", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20791" });
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] server.setRoute("/iframe.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html; charset=utf-8" });
    // iframe access parent frame to log a value from it.
    // [ts] res.end(`
    // [ts] <!doctype html>
    // [ts] <script type="text/javascript">
    // [ts] console.log(window.parent.foo);
    // [ts] </script>
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/main.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html; charset=utf-8" });
    // [ts] res.end(`
    // [ts] <!doctype html>
    // [ts] <iframe id=myframe src="about:blank"></iframe>
    // [ts] <script type="text/javascript">
    // [ts] setTimeout(() => window.foo = 'foo', 0);
    // [ts] setTimeout(() => myframe.contentDocument.location.href = '${server.PREFIX}/iframe.html', 0);
    // [ts] </script>
    // [ts] `);
    // [ts] });
    // [ts] const messages = [];
    // [ts] page.on("console", msg => messages.push(msg.text()));
    // [ts] page.goto(server.PREFIX + "/main.html");
    // [ts] expect.poll(() => messages).toEqual(["foo"]);
    // [ts] page.reload();
    // [ts] expect.poll(() => messages).toEqual(["foo", "foo"]);
    // [ts] });
    // [ts] test("NA-086 should reload proper page", () => {
    // [ts] let mainRequest = 0;
    // [ts] let popupRequest = 0;
    // [ts] server.setRoute("/main.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html; charset=utf-8" });
    // [ts] res.end(`<!doctype html><h1>main: ${++mainRequest}</h1>`);
    // [ts] });
    // [ts] server.setRoute("/popup.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html; charset=utf-8" });
    // [ts] res.end(`<!doctype html><h1>popup: ${++popupRequest}</h1>`);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/main.html");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window.open("/popup.html")),
    // [ts] ]);
    // [ts] expect(page.locator("h1")).toHaveText("main: 1");
    // [ts] expect(popup.locator("h1")).toHaveText("popup: 1");
    // [ts] page.reload();
    // [ts] expect(page.locator("h1")).toHaveText("main: 2");
    // [ts] expect(popup.locator("h1")).toHaveText("popup: 1");
    // [ts] popup.reload();
    // [ts] expect(page.locator("h1")).toHaveText("main: 2");
    // [ts] expect(popup.locator("h1")).toHaveText("popup: 2");
    // [ts] });
}
