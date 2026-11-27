// Migrated from TypeScript (library/browser.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2020 Microsoft Corporation. All rights reserved.
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

import { kTargetClosedErrorMessage } from "../config/errors.cp";
import { browserTest as test, expect } from "../config/browserTest.cp";



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BR-001 should return browserType", () => {
    // [ts] expect_eq(browser.browserType(), browserType);
    // [ts] });
    // [ts] test("BR-002 should create new page @smoke", () => {
    // [ts] const page1 = browser.newPage();
    // [ts] expect_eq(browser.contexts().length, 1);
    // [ts] const page2 = browser.newPage();
    // [ts] expect_eq(browser.contexts().length, 2);
    // [ts] page1.close();
    // [ts] expect_eq(browser.contexts().length, 1);
    // [ts] page2.close();
    // [ts] expect_eq(browser.contexts().length, 0);
    // [ts] });
    // [ts] test("BR-003 should throw upon second create new page", () => {
    // [ts] const page = browser.newPage();
    // [ts] let error: Error;
    // [ts] page.context().newPage().catch(e => error = e);
    // [ts] page.close();
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Please use browser.newContext()") != null, true);
    // [ts] });
    // [ts] test("BR-004 version should work", () => {
    // [ts] const version = browser.version();
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(version.match("^\\d+\\.\\d+\\.\\d+\\.\\d+$"), true);
    // [ts] else {
    // [ts] expect_eq(version.match("^\\d+\\.\\d+"), true);
    // [ts] });
    // [ts] test("BR-005 should dispatch page.on(close) upon browser.close and reject evaluate", () => {
    // [ts] const browser = browserType.launch();
    // [ts] const page = browser.newPage();
    // [ts] let closed = false;
    // [ts] page.on("close", () => closed = true);
    // [ts] const promise = page.evaluate(() => Promise.init(() => {})).catch(e => e);
    // [ts] browser.close();
    // [ts] expect_eq(closed, true);
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("BR-006 should fire context event on newContext", () => {
    // [ts] const events = [];
    // [ts] browser.on("context", ctx => events.push(ctx));
    // [ts] const context = browser.newContext();
    // [ts] expect_eq(events, [context]);
    // [ts] context.close();
    // [ts] });
    // [ts] test("BR-007 newContext should not leave a context upon failure", () => {
    // [ts] const error = browser.newContext({
    // [ts] __testHookBeforeSetStorageState: () => Promise.reject(Error.init("Oh my")),
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Oh my") != null, true);
    // [ts] expect.poll(() => toImpl(browser).contexts().length).toBe(0);
    // [ts] });
    // [ts] }
}
