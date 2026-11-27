// Migrated from TypeScript (page/elementhandle-select-text.spec.ts) via cap from -ts

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




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EH-102 should select textarea", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.evaluate(textarea => textarea.value = "some value");
    // [ts] textarea.selectText();
    // [ts] expect_eq(page.evaluate(() => window.getSelection().toString()), "some value");
    // [ts] });
    // [ts] test("EH-103 should select input", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const input = page.$("input");
    // [ts] input.evaluate(input => input.value = "some value");
    // [ts] input.selectText();
    // [ts] expect_eq(page.evaluate(() => window.getSelection().toString()), "some value");
    // [ts] });
    // [ts] test("EH-104 should select plain div", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const div = page.$("div.plain");
    // [ts] div.selectText();
    // [ts] expect_eq(page.evaluate(() => window.getSelection().toString()), "Plain div");
    // [ts] });
    // [ts] test("EH-105 should timeout waiting for invisible element", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.evaluate(e => e.style.display = "none");
    // [ts] const error = textarea.selectText({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is not visible") != null, true);
    // [ts] });
    // [ts] test("EH-106 should wait for visible", () => {
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] const textarea = page.$("textarea");
    // [ts] textarea.evaluate(textarea => textarea.value = "some value");
    // [ts] textarea.evaluate(e => e.style.display = "none");
    // [ts] let done = false;
    // [ts] const promise = textarea.selectText({ timeout: 3000 }).then(() => done = true);
    // [ts] page.waitForTimeout(1000);
    // [ts] expect_eq(done, false);
    // [ts] textarea.evaluate(e => e.style.display = "block");
    // [ts] promise;
    // [ts] });
}
