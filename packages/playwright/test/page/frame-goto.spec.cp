// Migrated from TypeScript (page/frame-goto.spec.ts) via cap from -ts

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
import { attachFrame } from "../config/utils.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("FR-020 should navigate subframes @smoke", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page.frames()[0].url(), "/frames/one-frame.html") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, page.frames()[1].url(), "/frames/frame.html") != null, true);
    // [ts] const response = page.frames()[1].goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.frame(), page.frames()[1]);
    // [ts] });
    // [ts] test("FR-021 should reject when frame detaches", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] server.setRoute("/one-style.css", () => {});
    // [ts] const navigationPromise = page.frames()[1].goto(server.PREFIX + "/one-style.html").catch(e => e);
    // [ts] server.waitForRequest("/one-style.css");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', frame => frame.remove());
    // [ts] const error = navigationPromise;
    // [ts] if (browserName == "chromium")
    // [ts] expect_eq(error.message.includes("net::ERR_ABORTED") || error.message.toLowerCase().includes("frame was detached"), true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message.toLowerCase(), "frame was detached") != null, true);
    // [ts] });
    // [ts] test("FR-022 should continue after client redirect", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "script.js is requested before navigationCommitted arrives");
    // [ts] server.setRoute("/frames/script.js", () => {});
    // [ts] const url = server.PREFIX + "/frames/child-redirect.html";
    // [ts] const error = page.goto(url, { timeout: 5000, waitUntil: "networkidle" }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 5000ms exceeded.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `navigating to "${url}", waiting until "networkidle"`) != null, true);
    // [ts] });
    // [ts] test("FR-023 should return matching responses", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // Attach three frames.
    // [ts] const frames = [
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE),
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE),
    // [ts] attachFrame(page, "frame3", server.EMPTY_PAGE),
    // [ts] ];
    // [ts] const serverResponses = [];
    // [ts] server.setRoute("/0.html", (req, res) => serverResponses.push(res));
    // [ts] server.setRoute("/1.html", (req, res) => serverResponses.push(res));
    // [ts] server.setRoute("/2.html", (req, res) => serverResponses.push(res));
    // [ts] const navigations = [];
    // [ts] let i = 0;
    // [ts] while (i < 3) { // [cap-migrate] for-step: ++i
    // [ts] navigations.push(frames[i].goto(server.PREFIX + "/" + i + ".html"));
    // [ts] server.waitForRequest("/" + i + ".html");
    // [ts] }
    // Respond from server out-of-order.
    // [ts] const serverResponseTexts = ["AAA", "BBB", "CCC"];
    // [ts] for (const i of [1, 2, 0]) {
    // [ts] serverResponses[i].end(serverResponseTexts[i]);
    // [ts] const response = navigations[i];
    // [ts] expect_eq(response.frame(), frames[i]);
    // [ts] expect_eq(response.text(), serverResponseTexts[i]);
    // [ts] }
    // [ts] });
    // [ts] }
}
