// Migrated from TypeScript (page/page-add-locator-handler.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";
import { test, expect } from "./pageTest.cp";

const { ManualPromise } = iso;












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("LH-001 should work", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let beforeCount = 0;
    // [ts] let afterCount = 0;
    // [ts] const originalLocator = page.getByText("This interstitial covers the button");
    // [ts] page.addLocatorHandler(originalLocator, async locatorArgument => {
    // [ts] expect_eq(locatorArgument, originalLocator);
    // [ts] ++beforeCount;
    // [ts] page.locator("#close").click();
    // [ts] ++afterCount;
    // [ts] });
    // [ts] for (const args of [
    // [ts] ["mouseover", 1],
    // [ts] ["mouseover", 1, "capture"],
    // [ts] ["mouseover", 2],
    // [ts] ["mouseover", 2, "capture"],
    // [ts] ["pointerover", 1],
    // [ts] ["pointerover", 1, "capture"],
    // [ts] ["none", 1],
    // [ts] ["remove", 1],
    // [ts] ["hide", 1],
    // [ts] ]) {
    // [ts] test.step(`${args[0]}${if (args[2] == "capture") " with capture" else ""} ${args[1]} times`, () => {
    // [ts] page.locator("#aside").hover();
    // [ts] beforeCount = 0;
    // [ts] afterCount = 0;
    // [ts] page.evaluate(args => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial(...args);
    // [ts] }, args);
    // [ts] expect_eq(beforeCount, 0);
    // [ts] expect_eq(afterCount, 0);
    // [ts] page.locator("#target").click();
    // [ts] expect_eq(beforeCount, args[1]);
    // [ts] expect_eq(afterCount, args[1]);
    // [ts] expect_eq(page.evaluate("window.clicked"), 1);
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("LH-002 should work with a custom check", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] page.addLocatorHandler(page.locator("body"), () => {
    // [ts] if (page.getByText("This interstitial covers the button").isVisible())
    // [ts] page.locator("#close").click();
    // [ts] }, { noWaitAfter: true });
    // [ts] for (const args of [
    // [ts] ["mouseover", 2],
    // [ts] ["none", 1],
    // [ts] ["remove", 1],
    // [ts] ["hide", 1],
    // [ts] ]) {
    // [ts] test.step(`${args[0]}${if (args[2] == "capture") " with capture" else ""} ${args[1]} times`, () => {
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(args => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial(...args);
    // [ts] }, args);
    // [ts] page.locator("#target").click();
    // [ts] expect_eq(page.evaluate("window.clicked"), 1);
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("LH-003 should work with locator.hover()", () => {
    // [ts] test(!headless, "Stray hovers in headed mode");
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] page.addLocatorHandler(page.getByText("This interstitial covers the button"), () => {
    // [ts] page.locator("#close").click();
    // [ts] });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).setupAnnoyingInterstitial("pointerover", 1, "capture");
    // [ts] });
    // [ts] page.locator("#target").hover();
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('#target', e => window.getComputedStyle(e).backgroundColor)).toBe('rgb(255, 255, 0)');
    // [ts] });
    // [ts] test("LH-004 should not work with force:true", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] page.addLocatorHandler(page.getByText("This interstitial covers the button"), () => {
    // [ts] page.locator("#close").click();
    // [ts] });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).setupAnnoyingInterstitial("none", 1);
    // [ts] });
    // [ts] page.locator("#target").click({ force: true, timeout: 2000 });
    // [ts] expect_eq(page.locator("#interstitial").isVisible(), true);
    // [ts] expect_eq(page.evaluate("window.clicked"), null);
    // [ts] });
    // [ts] test("LH-005 should throw when handler times out", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43607" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] const releaseHandler = ManualPromise.init();
    // [ts] const handlerFinished = ManualPromise.init();
    // [ts] page.addLocatorHandler(page.getByText("This interstitial covers the button"), () => {
    // [ts] ++called;
    // Deliberately timeout.
    // [ts] releaseHandler;
    // [ts] handlerFinished.resolve(page.locator("#close").click({ timeout: 3000 }).catch(e => e));
    // [ts] });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("mouseover", 1);
    // [ts] });
    // [ts] const error = page.locator("#target").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
    // [ts] const error2 = page.locator("#target").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error2.message, "Timeout 3000ms exceeded") != null, true);
    // Should not enter the same handler while it is still running.
    // [ts] expect_eq(called, 1);
    // [ts] releaseHandler.resolve();
    // [ts] expect_eq(handlerFinished == null, true);
    // [ts] });
    // [ts] test("LH-006 should work with toBeVisible", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.getByText("This interstitial covers the button"), () => {
    // [ts] ++called;
    // [ts] page.locator("#close").click();
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("remove", 1);
    // [ts] });
    // [ts] expect(page.locator("#target")).toBeVisible();
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // [ts] expect_eq(called, 1);
    // [ts] });
    // [ts] test("LH-007 should work with locator.waitFor", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.getByText("This interstitial covers the button"), () => {
    // [ts] ++called;
    // [ts] page.locator("#close").click();
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("remove", 1);
    // [ts] });
    // [ts] page.locator("#target").waitFor();
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // [ts] expect_eq(called, 1);
    // [ts] });
    // [ts] test("LH-008 should work with toHaveScreenshot", () => {
    // [ts] test.fixme(isAndroid, "Screenshots are cut off on Android");
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] page.evaluate(() => {
    // [ts] const overlay = document.createElement("div");
    // [ts] document.body.append(overlay);
    // [ts] overlay.style.position = "absolute";
    // [ts] overlay.style.left = "0";
    // [ts] overlay.style.right = "0";
    // [ts] overlay.style.top = "0";
    // [ts] overlay.style.bottom = "0";
    // [ts] overlay.style.backgroundColor = "red";
    // [ts] const closeButton = document.createElement("button");
    // [ts] overlay.appendChild(closeButton);
    // [ts] closeButton.textContent = "close";
    // [ts] closeButton.addEventListener("click", () => overlay.remove());
    // [ts] });
    // [ts] page.addLocatorHandler(page.getByRole("button", { name: "close" }), () => {
    // [ts] page.getByRole("button", { name: "close" }).click();
    // [ts] });
    // [ts] expect(page).toHaveScreenshot("screenshot-grid.png");
    // [ts] });
    // [ts] test("LH-009 should work when owner frame detaches", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "data:text/html,<body>hello from iframe</body>";
    // [ts] document.body.append(iframe);
    // [ts] const target = document.createElement("button");
    // [ts] target.textContent = "Click me";
    // [ts] target.id = "target";
    // [ts] target.addEventListener("click", () => (window)._clicked = true);
    // [ts] document.body.appendChild(target);
    // [ts] const closeButton = document.createElement("button");
    // [ts] closeButton.textContent = "close";
    // [ts] closeButton.id = "close";
    // [ts] closeButton.addEventListener("click", () => iframe.remove());
    // [ts] document.body.appendChild(closeButton);
    // [ts] });
    // [ts] page.addLocatorHandler(page.frameLocator("iframe").locator("body"), () => {
    // [ts] page.locator("#close").click();
    // [ts] });
    // [ts] page.locator("#target").click();
    // [ts] expect_eq(page.$("iframe"), null);
    // [ts] expect_eq(page.evaluate("window._clicked"), true);
    // [ts] });
    // [ts] test("LH-010 should work with times: option", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.locator("body"), () => {
    // [ts] ++called;
    // [ts] }, { noWaitAfter: true, times: 2 });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("mouseover", 4);
    // [ts] });
    // [ts] const error = page.locator("#target").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(called, 2);
    // [ts] expect_eq(page.evaluate("window.clicked"), 0);
    // [ts] expect(page.locator("#interstitial")).toBeVisible();
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `<div>This interstitial covers the button</div> from <div class="visible" id="interstitial">ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ¦</div> subtree intercepts pointer events`) != null, true);
    // [ts] });
    // [ts] test("LH-011 should wait for hidden by default", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.getByRole("button", { name: "close" }), async button => {
    // [ts] called++;
    // [ts] button.click();
    // [ts] });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("timeout", 1);
    // [ts] });
    // [ts] page.locator("#target").click();
    // [ts] expect_eq(page.evaluate("window.clicked"), 1);
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // [ts] expect_eq(called, 1);
    // [ts] });
    // [ts] test("LH-012 should wait for hidden by default 2", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.getByRole("button", { name: "close" }), async button => {
    // [ts] called++;
    // [ts] });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("hide", 1);
    // [ts] });
    // [ts] const error = page.locator("#target").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(page.evaluate("window.clicked"), 0);
    // [ts] expect_eq(page.locator("#interstitial").isVisible(), true);
    // [ts] expect_eq(called, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `locator handler has finished, waiting for getByRole('button', { name: 'close' }) to be hidden`) != null, true);
    // [ts] });
    // [ts] test("LH-013 should work with noWaitAfter", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.getByRole("button", { name: "close" }), async button => {
    // [ts] called++;
    // [ts] if (called == 1)
    // [ts] button.click();
    // [ts] else {
    // [ts] page.locator("#interstitial").waitFor({ state: "hidden" });
    // [ts] }, { noWaitAfter: true });
    // [ts] page.locator("#aside").hover();
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("timeout", 1);
    // [ts] });
    // [ts] page.locator("#target").click();
    // [ts] expect_eq(page.evaluate("window.clicked"), 1);
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // [ts] expect_eq(called, 2);
    // [ts] });
    // [ts] test("LH-014 should removeLocatorHandler", () => {
    // [ts] page.goto(server.PREFIX + "/input/handle-locator.html");
    // [ts] let called = 0;
    // [ts] page.addLocatorHandler(page.getByRole("button", { name: "close" }), async locator => {
    // [ts] ++called;
    // [ts] locator.click();
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("hide", 1);
    // [ts] });
    // [ts] page.locator("#target").click();
    // [ts] expect_eq(called, 1);
    // [ts] expect_eq(page.evaluate("window.clicked"), 1);
    // [ts] expect(page.locator("#interstitial")).not.toBeVisible();
    // [ts] page.evaluate(() => {
    // [ts] (window).clicked = 0;
    // [ts] (window).setupAnnoyingInterstitial("hide", 1);
    // [ts] });
    // [ts] page.removeLocatorHandler(page.getByRole("button", { name: "close" }));
    // [ts] const error = page.locator("#target").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(called, 1);
    // [ts] expect_eq(page.evaluate("window.clicked"), 0);
    // [ts] expect(page.locator("#interstitial")).toBeVisible();
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
    // [ts] });
    // [ts] }
}
