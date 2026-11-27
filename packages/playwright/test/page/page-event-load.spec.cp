// Migrated from TypeScript (page/page-event-load.spec.ts) via cap from -ts

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
    // [ts] test("NA-087 should fire once", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15086" });
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "Firefox sometimes double fires.");
    // [ts] let count = 0;
    // [ts] page.on("load", () => count++);
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] page.waitForTimeout(1000);
    // [ts] expect_eq(count, 1);
    // [ts] });
    // [ts] test("NA-088 should fire once with iframe navigation", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15086" });
    // [ts] it.fixme(browserName == "firefox" && !isBidi, "Firefox sometimes double fires.");
    // [ts] let requestCount = 0;
    // [ts] server.setRoute("/tracker", (_unused, res) => {
    // [ts] res.write(`request count: ${++requestCount}`);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/home", (_unused, res) => {
    // [ts] res.write(`
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <head>
    // [ts] </head>
    // [ts] <body>
    // [ts] <script>
    // [ts] window.eventLog = [];
    // [ts] window.addEventListener('load', () => {
    // [ts] window.eventLog.push('load');
    // [ts] });
    // [ts] </script>
    // [ts] <form id="trackerForm" action="/tracker" method="post" target="tracker">
    // [ts] <input type="submit">
    // [ts] </form>
    // [ts] <iframe name="tracker" src="/tracker">
    // [ts] </body>
    // [ts] </html>
    // [ts] `);
    // [ts] res.end();
    // [ts] });
    // [ts] let count = 0;
    // [ts] page.on("load", () => count++);
    // [ts] page.goto(server.PREFIX + "/home");
    // [ts] const trackingFrame = page.frameLocator("[name=tracker]");
    // [ts] expect(trackingFrame.locator(":scope")).toContainText("request count: 1");
    // [ts] const loadFired = race([ // migrated: Promise.race -> race
    // [ts] page.waitForEvent("load").then(() => "loadfired"),
    // [ts] page.waitForTimeout(1000).then(() => "timeout"),
    // [ts] ]);
    // [ts] page.locator("input[type=submit]").click();
    // [ts] expect.soft(loadFired).resolves.toBe("timeout");
    // [ts] expect.soft(count).toBe(1);
    // [ts] expect_eq(page.evaluate(() => window["eventLog"]), ["load"]);
    // [ts] });
}
