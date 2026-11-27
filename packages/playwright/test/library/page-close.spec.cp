// Migrated from TypeScript (library/page-close.spec.ts) via cap from -ts

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

import { stripAnsi } from "../config/utils.cp";
import { browserTest as test, expect } from "../config/browserTest.cp";
import { kTargetClosedErrorMessage } from "../config/errors.cp";












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("PA-039 should close page with active dialog", () => {
    // [ts] page.evaluate("\"trigger builtins.setTimeout\"");
    // [ts] page.setContent(`<button onclick="builtins.setTimeout(() => alert(1))">alert</button>`);
    // [ts] void page.click("button").catch(() => {});
    // [ts] page.waitForEvent("dialog");
    // [ts] page.close();
    // [ts] });
    // [ts] test("PA-040 expect should not print timed out error message when page closes", () => {
    // [ts] page.setContent("<div id=node>Text content</div>");
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] expect(page.locator("div")).toHaveText("hey", { timeout: 100000 }).catch(e => e),
    // [ts] page.close(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(locator).toHaveText(expected)`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Timed out") == null, true);
    // [ts] });
    // [ts] test("PA-041 addLocatorHandler should throw when page closes", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] page.addLocatorHandler(page.getByText("This interstitial covers the button"), () => {
    // [ts] page.close({ reason: "custom reason" });
    // [ts] });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("mouseover", 1);
    // [ts] });
    // [ts] const error = page.locator("#target").click().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "custom reason") != null, true);
    // [ts] });
    // [ts] test("PA-042 should reject all promises when page is closed", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43647" } }, (_p) => {
    // [ts] server.setRoute("/hang", () => {});
    // [ts] const requestPromise = server.waitForRequest("/hang");
    // [ts] page.setContent(`<iframe src="${server.PREFIX}/hang"></iframe>`, { waitUntil: "domcontentloaded" });
    // [ts] requestPromise;
    // [ts] const frameErrorPromise = page.frames()[1].evaluate(() => Promise.init(() => {})).catch(e => e);
    // [ts] let error = null;
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => Promise.init(r => {})).catch(e => error = e),
    // [ts] page.close(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, (frameErrorPromise).message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("PA-043 should set the page close state", () => {
    // [ts] expect_eq(page.isClosed(), false);
    // [ts] page.close();
    // [ts] expect_eq(page.isClosed(), true);
    // [ts] });
    // [ts] test("PA-044 should pass page to close event", () => {
    // [ts] const [closedPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("close"),
    // [ts] page.close()
    // [ts] ]);
    // [ts] expect_eq(closedPage, page);
    // [ts] });
    // [ts] test("PA-045 should terminate network waiters", () => {
    // [ts] const results = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest(server.EMPTY_PAGE).catch(e => e),
    // [ts] page.waitForResponse(server.EMPTY_PAGE).catch(e => e),
    // [ts] page.close()
    // [ts] ]);
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] const message = results[i].message;
    // [ts] expect_eq(std.mem.indexOf(u8, message, kTargetClosedErrorMessage) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, message, "Timeout") == null, true);
    // [ts] }
    // [ts] });
    // [ts] test("PA-046 should be callable twice", () => {
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.close(),
    // [ts] page.close(),
    // [ts] ]);
    // [ts] page.close();
    // [ts] });
    // [ts] test("PA-047 should return null if parent page has been closed", () => {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window.open("about:blank")),
    // [ts] ]);
    // [ts] page.close();
    // [ts] const opener = popup.opener();
    // [ts] expect_eq(opener, null);
    // [ts] });
    // [ts] test("PA-048 should fail with error upon disconnect", () => {
    // [ts] let error;
    // [ts] const waitForPromise = page.waitForEvent("download").catch(e => error = e);
    // [ts] page.close();
    // [ts] waitForPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("PA-049 page.close should work with window.close", () => {
    // [ts] const closedPromise = Promise.init(x => page.on("close", x));
    // [ts] page.close();
    // [ts] closedPromise;
    // [ts] });
    // [ts] test("PA-050 should not throw UnhandledPromiseRejection when page closes", () => {
    // [ts] test.fixme(browserName == "firefox" && isWindows, "makes the next test to always timeout");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.close(),
    // [ts] page.mouse.click(1, 2),
    // [ts] ]).catch(e => {});
    // [ts] });
    // [ts] test("PA-051 interrupt request.response() and request.allHeaders() on page.close", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27227" });
    // [ts] server.setRoute("/one-style.css", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/css");
    // [ts] });
    // [ts] const reqPromise = page.waitForRequest("**/one-style.css");
    // [ts] page.goto(server.PREFIX + "/one-style.html", { waitUntil: "domcontentloaded" });
    // [ts] const req = reqPromise;
    // [ts] const respPromise = req.response().catch(e => e);
    // [ts] const headersPromise = req.allHeaders().catch(e => e);
    // [ts] page.close();
    // [ts] expect_eq(std.mem.indexOf(u8, (respPromise).message, kTargetClosedErrorMessage) != null, true);
    // All headers are the same as "provisional" headers in Firefox.
    // [ts] if (browserName == "firefox")
    // [ts] expect_eq((headersPromise)["user-agent"], true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, (headersPromise).message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("PA-052 should not treat navigations as new popups", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a target=_blank rel=noopener href=\"/one-style.html\">yo</a>");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] let badSecondPopup = false;
    // [ts] page.on("popup", () => badSecondPopup = true);
    // [ts] popup.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] page.close();
    // [ts] expect_eq(badSecondPopup, false);
    // [ts] });
    // [ts] test("PA-053 should not result in unhandled rejection", () => {
    // [ts] const closedPromise = page.waitForEvent("close");
    // [ts] page.exposeFunction("foo", () => {
    // [ts] page.close();
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => (window).foo(), 0);
    // [ts] return null;
    // [ts] });
    // [ts] closedPromise;
    // Make a round-trip to be sure we did not throw immediately after closing.
    // [ts] expect(page.evaluate("1 + 1").catch(e => e)).toBeInstanceOf(Error);
    // [ts] });
    // [ts] test("PA-054 should reject response.finished if page closes", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/get", (req, res) => {
    // In Firefox, |fetch| will be hanging until it receives |Content-Type| header
    // from server.
    // [ts] res.setHeader("Content-Type", "text/plain; charset=utf-8");
    // [ts] res.write("hello ");
    // [ts] });
    // send request and wait for server response
    // [ts] const [pageResponse] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response"),
    // [ts] page.evaluate(() => fetch("./get", { method: "GET" })),
    // [ts] ]);
    // [ts] const finishPromise = pageResponse.finished().catch(e => e);
    // [ts] page.close();
    // [ts] const error = finishPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "closed") != null, true);
    // [ts] });
    // [ts] test("PA-055 should not throw when continuing while page is closing", () => {
    // [ts] let done;
    // [ts] page.route("**/*", async route => {
    // [ts] done = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] void route.continue(),
    // [ts] page.close(),
    // [ts] ]);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] done;
    // [ts] });
    // [ts] test("PA-056 should not throw when continuing after page is closed", () => {
    // [ts] let done;
    // [ts] page.route("**/*", async route => {
    // [ts] page.close();
    // [ts] done = route.continue();
    // [ts] });
    // [ts] const error = page.goto(server.EMPTY_PAGE).catch(e => e);
    // [ts] done;
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] });
    // [ts] }
}
