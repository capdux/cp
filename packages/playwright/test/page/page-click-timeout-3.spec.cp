// Migrated from TypeScript (page/page-click-timeout-3.spec.ts) via cap from -ts

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
    // [ts] test("CL-116 should fail when element jumps during hit testing", () => {
    // [ts] test(mode != "default");
    // [ts] page.setContent("<button>Click me</button>");
    // [ts] let clicked = false;
    // [ts] const handle = page.$("button");
    // [ts] const __testHookBeforeHitTarget = () => page.evaluate(() => {
    // [ts] const margin = parseInt(document.querySelector("button").style.marginLeft || "0", 10) + 100;
    // [ts] document.querySelector("button").style.marginLeft = margin + "px";
    // [ts] });
    // [ts] const promise = handle.click({ timeout: 5000, __testHookBeforeHitTarget }).then(() => clicked = true).catch(e => e);
    // [ts] const error = promise;
    // [ts] expect_eq(clicked, false);
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 5000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<body>ÃÂ¢ÃÂÃÂ¦</body> intercepts pointer events") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "retrying click action") != null, true);
    // [ts] });
    // [ts] test("CL-117 should timeout waiting for hit target", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.position = "relative";
    // [ts] const blocker = document.createElement("div");
    // [ts] blocker.id = "blocker";
    // [ts] blocker.style.position = "absolute";
    // [ts] blocker.style.width = "400px";
    // [ts] blocker.style.height = "20px";
    // [ts] blocker.style.left = "0";
    // [ts] blocker.style.top = "0";
    // [ts] document.body.appendChild(blocker);
    // [ts] });
    // [ts] const error = button.click({ timeout: 5000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 5000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div id=\"blocker\"></div> intercepts pointer events") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "retrying click action") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "waiting 500ms") != null, true);
    // [ts] });
    // [ts] test("CL-118 should still click when force but hit target is obscured", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.position = "relative";
    // [ts] const blocker = document.createElement("div");
    // [ts] blocker.id = "blocker";
    // [ts] blocker.style.position = "absolute";
    // [ts] blocker.style.width = "400px";
    // [ts] blocker.style.height = "200px";
    // [ts] blocker.style.left = "0";
    // [ts] blocker.style.top = "0";
    // [ts] document.body.appendChild(blocker);
    // [ts] });
    // [ts] button.click({ force: true });
    // [ts] });
    // [ts] test("CL-119 should report wrong hit target subtree", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.position = "relative";
    // [ts] const blocker = document.createElement("div");
    // [ts] blocker.id = "blocker";
    // [ts] blocker.style.position = "absolute";
    // [ts] blocker.style.width = "400px";
    // [ts] blocker.style.height = "20px";
    // [ts] blocker.style.left = "0";
    // [ts] blocker.style.top = "0";
    // [ts] document.body.appendChild(blocker);
    // [ts] const inner = document.createElement("div");
    // [ts] inner.id = "inner";
    // [ts] inner.style.position = "absolute";
    // [ts] inner.style.left = "0";
    // [ts] inner.style.top = "0";
    // [ts] inner.style.right = "0";
    // [ts] inner.style.bottom = "0";
    // [ts] blocker.appendChild(inner);
    // [ts] });
    // [ts] const error = button.click({ timeout: 5000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 5000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div id=\"inner\"></div> from <div id=\"blocker\">ÃÂ¢ÃÂÃÂ¦</div> subtree intercepts pointer events") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "retrying click action") != null, true);
    // [ts] });
}
