// Migrated from TypeScript (page/page-wait-for-response.spec.ts) via cap from -ts

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







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("WA-129 should work", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/digits/2.png"),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] })
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("WA-130 should respect timeout", () => {
    // [ts] let error = null;
    // [ts] page.waitForEvent("response", { predicate: () => false, timeout: 1 }).catch(e => error = e);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("WA-131 should respect default timeout", () => {
    // [ts] let error = null;
    // [ts] page.setDefaultTimeout(1);
    // [ts] page.waitForResponse(() => false).catch(e => error = e);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // Error stack should point to the api call.
    // [ts] const firstFrame = error.stack.split("\n").find(line => line.startsWith("    at "));
    // [ts] expect_eq(std.mem.indexOf(u8, firstFrame, __filename) != null, true);
    // [ts] });
    // [ts] test("WA-132 should log the url", () => {
    // [ts] const error1 = page.waitForResponse("foo.css", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error1.message, "waiting for response \"foo.css\"") != null, true);
    // [ts] const error2 = page.waitForResponse("foo.css", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error2.message, "waiting for response /foo.css/i") != null, true);
    // [ts] });
    // [ts] test("WA-133 should work with predicate", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response", response => response.url() == server.PREFIX + "/digits/2.png"),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] })
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("WA-134 should work with async predicate", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response1, response2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response", async response => {
    // [ts] const text = response.text();
    // [ts] return text.includes("contents of the file");
    // [ts] }),
    // [ts] page.waitForResponse(async response => {
    // [ts] const text = response.text();
    // [ts] return text.includes("bar");
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/simple.json").then(r => r.json());
    // [ts] void fetch("/file-to-upload.txt").then(r => r.text());
    // [ts] })
    // [ts] ]);
    // [ts] expect_eq(response1.url(), server.PREFIX + "/file-to-upload.txt");
    // [ts] expect_eq(response2.url(), server.PREFIX + "/simple.json");
    // [ts] });
    // [ts] test("WA-135 sync predicate should be only called once", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let counter = 0;
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("response", response => {
    // [ts] ++counter;
    // [ts] return response.url() == server.PREFIX + "/digits/1.png";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] fetch("/digits/1.png");
    // [ts] fetch("/digits/2.png");
    // [ts] fetch("/digits/3.png");
    // [ts] })
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/1.png");
    // [ts] expect_eq(counter, 1);
    // [ts] });
    // [ts] test("WA-136 should work with no timeout", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/digits/2.png", { timeout: 0 }),
    // [ts] page.evaluate(() => window.builtins.setTimeout(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }, 50))
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
}
