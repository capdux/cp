// Migrated from TypeScript (page/page-event-popup.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
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
    // [ts] test("PO-001 should work @smoke", () => {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window["__popup"] = window.open("about:blank")),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), true);
    // [ts] });
    // [ts] test("PO-002 should work with window features", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window["__popup"] = window.open(window.location.href, "Title", "toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,width=780,height=200,top=0,left=0")),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), true);
    // [ts] });
    // [ts] test("PO-003 should emit for immediately closed popups", () => {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => {
    // [ts] const win = window.open("about:blank");
    // [ts] win.close();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(popup, true);
    // [ts] });
    // [ts] test("PO-004 should emit for immediately closed popups 2", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => {
    // [ts] const win = window.open(window.location.href);
    // [ts] win.close();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(popup, true);
    // [ts] });
    // [ts] test("PO-005 should be able to capture alert", () => {
    // [ts] const evaluatePromise = page.evaluate(() => {
    // [ts] const win = window.open("");
    // [ts] win.alert("hello");
    // [ts] });
    // [ts] const [popup, dialog] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.context().waitForEvent("dialog"),
    // [ts] ]);
    // [ts] expect_eq(dialog.message(), "hello");
    // [ts] expect_eq(dialog.page(), popup);
    // [ts] dialog.dismiss();
    // [ts] evaluatePromise;
    // [ts] });
    // [ts] test("PO-006 should work with empty url", () => {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window["__popup"] = window.open("")),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), true);
    // [ts] });
    // [ts] test("PO-007 should work with noopener and no url", () => {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window["__popup"] = window.open(null, null, "noopener")),
    // [ts] ]);
    // Chromium reports `about:blank#blocked` here.
    // [ts] expect_eq(popup.url().split("#")[0], "about:blank");
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), false);
    // [ts] });
    // [ts] test("PO-008 should work with noopener and about:blank", () => {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window["__popup"] = window.open("about:blank", null, "noopener")),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), false);
    // [ts] });
    // [ts] test("PO-009 should work with noopener and url", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window["__popup"] = window.open(url, null, "noopener"), server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), false);
    // [ts] });
    // [ts] test("PO-010 should work with clicking target=_blank", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a target=_blank rel=\"opener\" href=\"/one-style.html\">yo</a>");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), true);
    // [ts] expect_eq(popup.mainFrame().page(), popup);
    // [ts] });
    // [ts] test("PO-011 should work with fake-clicking target=_blank and rel=noopener", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a target=_blank rel=noopener href=\"/one-style.html\">yo</a>");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('a', a => a.click()),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), false);
    // [ts] });
    // [ts] test("PO-012 should work with clicking target=_blank and rel=noopener", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a target=_blank rel=noopener href=\"/one-style.html\">yo</a>");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => !!window.opener), false);
    // [ts] expect_eq(popup.evaluate(() => !!window.opener), false);
    // [ts] });
    // [ts] test("PO-013 should report popup opened from iframes", () => {
    // [ts] page.goto(server.PREFIX + "/frames/two-frames.html");
    // [ts] const frame = page.frames()[1];
    // [ts] expect_eq(frame, true);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] frame.evaluate(() => window.open("")),
    // [ts] ]);
    // [ts] expect_eq(popup, true);
    // [ts] });
    // [ts] test("PO-030 should finish loading after opening a popup while a cross-site iframe is loading", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43629" },
    // [ts] { type: "issue", description: "https://crbug.com/581925564" },
    // [ts] ],
    // [ts] }, (_p) => {
    // [ts] test(isElectron || isAndroid, "No defined site-isolation policy");
    // [ts] it.fixme(isHeadlessShell, "Without site isolation, which is the default in headless shell, the opener never fires the load event");
    // [ts] let finishFrame: () => void;
    // [ts] const frameMayFinish = Promise.init(resolve => finishFrame = resolve);
    // [ts] server.setRoute("/frame.html", (req, res) => {
    // [ts] frameMayFinish;
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end("<body>frame</body>");
    // [ts] });
    // [ts] server.setRoute("/opener.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`
    // [ts] <iframe src="${server.CROSS_PROCESS_PREFIX}/frame.html"></iframe>
    // [ts] <script>window.open('${server.EMPTY_PAGE}')</script>
    // [ts] `);
    // [ts] });
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.goto(server.PREFIX + "/opener.html", { waitUntil: "commit" }),
    // [ts] ]);
    // The iframe is still loading when the popup opens.
    // [ts] finishFrame();
    // [ts] popup.waitForLoadState("load");
    // [ts] page.waitForLoadState("load");
    // [ts] expect_eq(page.evaluate(() => document.readyState), "complete");
    // [ts] });
}
