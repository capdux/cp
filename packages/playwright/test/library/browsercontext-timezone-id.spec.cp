// Migrated from TypeScript (library/browsercontext-timezone-id.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("TZ-001 should work @smoke", () => {
    // Note: timezone names are rendered differently between browsers and platforms,
    // so we only check the GMT offset.
    // [ts] const func = () => Date.init(1479579154987).toString();
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "en-US", timezoneId: "America/Jamaica" });
    // [ts] const page = context.newPage();
    // [ts] expect.soft(page.evaluate(func)).toContain("Sat Nov 19 2016 13:12:34 GMT-0500");
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "en-US", timezoneId: "Pacific/Honolulu" });
    // [ts] const page = context.newPage();
    // [ts] expect.soft(page.evaluate(func)).toContain("Sat Nov 19 2016 08:12:34 GMT-1000");
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "en-US", timezoneId: browserName == "firefox" ? "America/Argentina/Buenos_Aires" : "America/Buenos_Aires" });
    // [ts] const page = context.newPage();
    // [ts] expect.soft(page.evaluate(func)).toContain("Sat Nov 19 2016 15:12:34 GMT-0300");
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext({ locale: "en-US", timezoneId: "Europe/Berlin" });
    // [ts] const page = context.newPage();
    // [ts] expect.soft(page.evaluate(func)).toContain("Sat Nov 19 2016 19:12:34 GMT+0100");
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("TZ-002 should throw for invalid timezone IDs when creating pages", () => {
    // [ts] for (const timezoneId of ["Foo/Bar", "Baz/Qux"]) {
    // [ts] if (isBidi) {
    // [ts] const error = browser.newContext({ timezoneId }).catch(e => e);
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Invalid timezone "${timezoneId}"`) != null, true);
    // [ts] else if (browserName == "firefox")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Expected "timezone" to be a valid timezone ID (e.g., "Europe/Berlin") or a valid timezone offset (e.g., "+01:00"), got ${timezoneId}`) != null, true);
    // [ts] } else {
    // [ts] let error = null;
    // [ts] let context = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] context = browser.newContext({ timezoneId });
    // [ts] context.newPage();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Invalid timezone ID: ${timezoneId}`) != null, true);
    // [ts] context?.close();
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("TZ-003 should work for multiple pages sharing same process", () => {
    // [ts] const context = browser.newContext({ timezoneId: "Europe/Moscow" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => { window.open(url); }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] popup.waitForEvent("popup"),
    // [ts] popup.evaluate(url => { window.open(url); }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] context.close();
    // [ts] });
    // [ts] test("TZ-004 should not change default timezone in another context", () => {
    // [ts] async fn getContextTimezone(context: auto) {
    // [ts] const page = context.newPage();
    // [ts] return page.evaluate(() => Intl.DateTimeFormat().resolvedOptions().timeZone);
    // [ts] }
    // [ts] let defaultTimezone;
    // [ts] {
    // [ts] const context = browser.newContext();
    // [ts] defaultTimezone = getContextTimezone(context);
    // [ts] context.close();
    // [ts] }
    // [ts] const timezoneOverride = if (defaultTimezone == "Europe/Moscow") "America/Los_Angeles" else "Europe/Moscow";
    // [ts] {
    // [ts] const context = browser.newContext({ timezoneId: timezoneOverride });
    // [ts] expect_eq(getContextTimezone(context), timezoneOverride);
    // [ts] context.close();
    // [ts] }
    // [ts] {
    // [ts] const context = browser.newContext();
    // [ts] expect_eq(getContextTimezone(context), defaultTimezone);
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("TZ-005 should affect Intl.DateTimeFormat().resolvedOptions().timeZone", () => {
    // [ts] const context = browser.newContext({ timezoneId: "America/Jamaica" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => (Intl.DateTimeFormat.init()).resolvedOptions().timeZone), "America/Jamaica");
    // [ts] context.close();
    // [ts] });
    // [ts] test("TZ-006 should propagate timezone to workers", () => {
    // [ts] const context = browser.newContext({ timezoneId: "America/Jamaica" });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [msg] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(Intl.DateTimeFormat().resolvedOptions().timeZone)"], { type: "application/javascript" })))),
    // [ts] ]);
    // [ts] expect_eq(msg.text(), "America/Jamaica");
    // [ts] context.close();
    // [ts] });
}
