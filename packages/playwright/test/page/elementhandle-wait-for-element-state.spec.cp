// Migrated from TypeScript (page/elementhandle-wait-for-element-state.spec.ts) via cap from -ts

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

// [cap-migrate] import { Page } from "cap:test";
import { test as it, expect, rafraf } from "./pageTest.cp";

fn giveItAChanceToResolve(page: Page) { return rafraf(page, 5); }











fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EH-112 should wait for visible", () => {
    // [ts] page.setContent(`<div style='display:none'>content</div>`);
    // [ts] const div = page.$("div");
    // [ts] let done = false;
    // [ts] const promise = div.waitForElementState("visible").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] div.evaluate(div => div.style.display = "block");
    // [ts] promise;
    // [ts] });
    // [ts] test("EH-113 should wait for already visible", () => {
    // [ts] page.setContent(`<div>content</div>`);
    // [ts] const div = page.$("div");
    // [ts] div.waitForElementState("visible");
    // [ts] });
    // [ts] test("EH-114 should timeout waiting for visible", () => {
    // [ts] page.setContent(`<div style='display:none'>content</div>`);
    // [ts] const div = page.$("div");
    // [ts] const error = div.waitForElementState("visible", { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] });
    // [ts] test("EH-115 should throw waiting for visible when detached", () => {
    // [ts] page.setContent(`<div style='display:none'>content</div>`);
    // [ts] const div = page.$("div");
    // [ts] const promise = div.waitForElementState("visible").catch(e => e);
    // [ts] div.evaluate(div => div.remove());
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not attached to the DOM") != null, true);
    // [ts] });
    // [ts] test("EH-116 should wait for hidden", () => {
    // [ts] page.setContent(`<div>content</div>`);
    // [ts] const div = page.$("div");
    // [ts] let done = false;
    // [ts] const promise = div.waitForElementState("hidden").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] div.evaluate(div => div.style.display = "none");
    // [ts] promise;
    // [ts] });
    // [ts] test("EH-117 should wait for already hidden", () => {
    // [ts] page.setContent(`<div></div>`);
    // [ts] const div = page.$("div");
    // [ts] div.waitForElementState("hidden");
    // [ts] });
    // [ts] test("EH-118 should wait for hidden when detached", () => {
    // [ts] page.setContent(`<div>content</div>`);
    // [ts] const div = page.$("div");
    // [ts] let done = false;
    // [ts] const promise = div.waitForElementState("hidden").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] div.evaluate(div => div.remove());
    // [ts] promise;
    // [ts] });
    // [ts] test("EH-119 should throw waiting for enabled when detached", () => {
    // [ts] page.setContent(`<button disabled>Target</button>`);
    // [ts] const button = page.$("button");
    // [ts] const promise = button.waitForElementState("enabled").catch(e => e);
    // [ts] button.evaluate(button => button.remove());
    // [ts] const error = promise;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not attached to the DOM") != null, true);
    // [ts] });
    // [ts] test("EH-120 should wait for aria enabled button", () => {
    // [ts] page.setContent("<button aria-disabled=true><span>Target</span></button>");
    // [ts] const span = page.$("text=Target");
    // [ts] let done = false;
    // [ts] const promise = span.waitForElementState("enabled").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] span.evaluate(span => span.parentElement.setAttribute("aria-disabled", "false"));
    // [ts] promise;
    // [ts] });
    // [ts] test("EH-121 should wait for button with an aria-disabled parent", () => {
    // [ts] page.setContent("<div role=\"group\" aria-disabled=true><button><span>Target</span></button></div>");
    // [ts] const span = page.$("text=Target");
    // [ts] let done = false;
    // [ts] const promise = span.waitForElementState("enabled").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] span.evaluate(span => span.parentElement.parentElement.setAttribute("aria-disabled", "false"));
    // [ts] promise;
    // [ts] });
    // [ts] test("EH-122 should wait for stable position", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43632" } }, (_p) => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const button = page.$("button");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
    // [ts-skip] button.style.transition = 'margin 10000ms linear 0s';
    // [ts-skip] button.style.marginLeft = '20000px';
    // [ts-skip] });
    // rafraf for Firefox to kick in the animation.
    // [ts] rafraf(page);
    // [ts] let done = false;
    // [ts] const promise = button.waitForElementState("stable").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] button.evaluate(button => button.style.transition = "");
    // [ts] promise;
    // [ts] });
    // [ts] test("EH-123 should wait for editable input", () => {
    // [ts] page.setContent("<input readonly>");
    // [ts] const input = page.$("input");
    // [ts] let done = false;
    // [ts] const promise = input.waitForElementState("editable").then(() => done = true);
    // [ts] giveItAChanceToResolve(page);
    // [ts] expect_eq(done, false);
    // [ts] input.evaluate(input => input.readOnly = false);
    // [ts] promise;
    // [ts] });
}
