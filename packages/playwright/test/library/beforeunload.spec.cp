// Migrated from TypeScript (library/beforeunload.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BU-001 should close browser with beforeunload page", () => {
    // [ts] const browser = browserType.launch();
    // [ts] const page = browser.newPage();
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] page.click("body");
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BU-002 should close browsercontext with beforeunload page", () => {
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] page.click("body");
    // [ts] context.close();
    // [ts] });
    // [ts] test("BU-003 should be able to navigate away from page with beforeunload", () => {
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] page.click("body");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("BU-004 should close page with beforeunload listener", () => {
    // [ts] const newPage = context.newPage();
    // [ts] newPage.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] newPage.click("body");
    // [ts] newPage.close();
    // [ts] });
    // [ts] test("BU-005 should run beforeunload if asked for @smoke", () => {
    // [ts] const newPage = context.newPage();
    // [ts] newPage.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] newPage.click("body");
    // [ts] const [dialog] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] newPage.waitForEvent("dialog"),
    // [ts] newPage.close({ runBeforeUnload: true })
    // [ts] ]);
    // [ts] expect_eq(dialog.type(), "beforeunload");
    // [ts] expect_eq(dialog.defaultValue(), "");
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(dialog.message(), "");
    // [ts] else if (browserName == "webkit")
    // [ts] expect_eq(dialog.message(), "Leave?");
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, dialog.message(), "This page is asking you to confirm that you want to leave") != null, true);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] dialog.accept(),
    // [ts] newPage.waitForEvent("close"),
    // [ts] ]);
    // [ts] });
    // [ts] test("BU-006 should access page after beforeunload", () => {
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] page.click("body");
    // [ts] const [dialog] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("dialog"),
    // [ts] page.close({ runBeforeUnload: true }),
    // [ts] ]);
    // [ts] dialog.dismiss();
    // [ts] page.evaluate(() => document.title);
    // [ts] });
    // [ts] test("BU-007 should not stall on evaluate when dismissing beforeunload", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23141" });
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers fire.
    // [ts] page.click("body");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("dialog").then(dialog => dialog.dismiss()),
    // [ts] page.evaluate(() => {
    // [ts] window.location.reload();
    // [ts] }),
    // [ts] ]);
    // [ts] });
    // [ts] test("BU-008 should not stall on click when dismissing beforeunload", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/33806" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<a href="${server.PREFIX}/frames/one-frame.html">click me</a>`);
    // [ts] page.evaluate(() => {
    // [ts] window.onbeforeunload = () => false;
    // [ts] });
    // [ts] page.on("dialog", async dialog => {
    // [ts] dialog.dismiss();
    // [ts] });
    // [ts] page.getByRole("link").click({ noWaitAfter: true });
    // [ts] page.evaluate(() => {
    // [ts] window.onbeforeunload = null;
    // [ts] });
    // This line should not timeout.
    // [ts] page.getByRole("link").click({ timeout: 5000 });
    // [ts] expect(page).toHaveURL(server.PREFIX + "/frames/one-frame.html");
    // [ts] });
    // [ts] test("BU-009 should support dismissing the dialog multiple times", () => {
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire (in Firefox).
    // [ts] page.click("body");
    // [ts] const [dialog] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("dialog"),
    // [ts] page.close({ runBeforeUnload: true })
    // [ts] ]);
    // [ts] dialog.dismiss();
    // [ts] const [dialog2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("dialog"),
    // [ts] page.close({ runBeforeUnload: true })
    // [ts] ]);
    // [ts] dialog2.dismiss();
    // [ts] });
    // [ts] test("BU-010 should support closing the page after a previous dismiss", () => {
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire (in Firefox).
    // [ts] page.click("body");
    // [ts] const [dialog] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("dialog"),
    // [ts] page.close({ runBeforeUnload: true })
    // [ts] ]);
    // [ts] dialog.dismiss();
    // [ts] page.close();
    // [ts] expect_eq(page.isClosed(), true);
    // [ts] });
    // [ts] test("BU-011 should support closing the page via a subsequent onbeforeunload dialog", () => {
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire (in Firefox).
    // [ts] page.click("body");
    // [ts] const [dialog] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("dialog"),
    // [ts] page.close({ runBeforeUnload: true })
    // [ts] ]);
    // [ts] dialog.accept();
    // [ts] page.waitForEvent("close");
    // [ts] expect_eq(page.isClosed(), true);
    // [ts] });
    // [ts] test("BU-012 does not get stalled by beforeUnload", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/38731" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] page.evaluate(() => {
    // [ts] window.addEventListener("beforeunload", event => {
    // [ts] event.preventDefault();
    // [ts] });
    // [ts] });
    // [ts] page.on("dialog", dialog => dialog.dismiss());
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] page.click("body");
    // [ts] page.route("**/api", route => route.fulfill({ status: 200, body: "ok" }));
    // [ts] page.evaluate(() => fetch(URL.init("/api", window.location.href)));
    // [ts] page.close({ runBeforeUnload: true });
    // [ts] page.evaluate(() => fetch(URL.init("/api", window.location.href)));
    // [ts] });
    // [ts] }
}
