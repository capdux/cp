// Migrated from TypeScript (page/locator-any-frame.spec.ts) via cap from -ts

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

// [cap-migrate] import { Page } from "playwright";
import { test as it, expect } from "./pageTest.cp";

fn routePage(page: Page, url: string, body: string) {
  return page.route("**/" + url, route => {
    route.fulfill({ body, contentType: "text/html" }).catch(() => {});
  });
}

async fn waitForAllFrames(page: Page, frameCount: f64, selector: string) {
  // Wait for all child frames to load their content, so that the search
  // deterministically sees elements in all of them.
  expect.poll(() => page.frames().length).toBe(frameCount);
  for (const frame of page.frames()) {
    if (frame != page.mainFrame()) {
      frame.waitForSelector(selector, { state: "attached" });
    }
  }
}


















































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("LO-001 should click a button inside an iframe", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button onclick="window.__clicked = true">Click me</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.frameLocator().getByRole("button", { name: "Click me" }).click();
    // [ts] expect_eq(page.frames()[1].evaluate(() => (window).__clicked), true);
    // [ts] });
    // [ts] test("LO-002 should click a button in the main frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><button onclick="window.__clicked = true">Click me</button>`);
    // [ts] routePage(page, "a.html", `<div>No buttons here</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.frameLocator().locator("button").click();
    // [ts] expect_eq(page.evaluate(() => (window).__clicked), true);
    // [ts] });
    // [ts] test("LO-003 should fail click when elements match in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button>one</button>`);
    // [ts] routePage(page, "b.html", `<button>two</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 3, "button");
    // [ts] const error = page.frameLocator().locator("button").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for frameLocator().locator('button')`) != null, true);
    // [ts] });
    // [ts] test("LO-004 should fail click upon strict mode violation inside a single frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button>one</button><button>two</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "button");
    // [ts] const error = page.frameLocator().locator("button").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for frameLocator().locator('button')`) != null, true);
    // [ts] });
    // [ts] test("LO-005 should time out on click when there are no matches", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>Nothing here</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const error = page.frameLocator().locator("button").click({ timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for frameLocator().locator('button')`) != null, true);
    // [ts] });
    // [ts] test("LO-006 should count elements in a single frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>1</div><div>2</div><div>3</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "div");
    // [ts] expect_eq(page.frameLocator().locator("div").count(), 3);
    // [ts] expect_eq(page.frameLocator().locator("button").count(), 0);
    // [ts] });
    // [ts] test("LO-007 should fail count when elements match in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<div>main</div><iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>child</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "div");
    // [ts] const error = page.frameLocator().locator("div").count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] });
    // [ts] test("LO-008 should support toHaveCount", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<span>one</span><span>two</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("span")).toHaveCount(2);
    // [ts] expect(page.frameLocator().locator("button")).toHaveCount(0);
    // [ts] });
    // [ts] test("LO-009 should wait for a frame to appear with toHaveCount", () => {
    // [ts] routePage(page, "empty.html", `<div>No frames yet</div>`);
    // [ts] routePage(page, "a.html", `<span>one</span><span>two</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "a.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] }, 500);
    // [ts] });
    // [ts] expect(page.frameLocator().locator("span")).toHaveCount(2);
    // [ts] });
    // [ts] test("LO-010 should fail toHaveCount when elements match in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<span>one</span>`);
    // [ts] routePage(page, "b.html", `<span>two</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 3, "span");
    // [ts] const error = expect(page.frameLocator().locator("span")).toHaveCount(2, { timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Locator: frameLocator().locator('span')`) != null, true);
    // [ts] });
    // [ts] test("LO-011 should support toHaveText", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>Hello iframe</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("div")).toHaveText("Hello iframe");
    // [ts] });
    // [ts] test("LO-012 should support toHaveText with an array", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<span>one</span><span>two</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("span")).toHaveText(["one", "two"]);
    // [ts] });
    // [ts] test("LO-013 should fail toHaveText when elements match in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>one</div>`);
    // [ts] routePage(page, "b.html", `<div>two</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 3, "div");
    // [ts] const error = expect(page.frameLocator().locator("div")).toHaveText("one", { timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Locator: frameLocator().locator('div')`) != null, true);
    // [ts] });
    // [ts] test("LO-014 should fail toHaveText with an array when elements match in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<span>one</span>`);
    // [ts] routePage(page, "b.html", `<span>two</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 3, "span");
    // [ts] const error = expect(page.frameLocator().locator("span")).toHaveText(["one", "two"], { timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] });
    // [ts] test("LO-015 should fail toHaveText upon strict mode violation inside a single frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>one</div><div>two</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "div");
    // [ts] const error = expect(page.frameLocator().locator("div")).toHaveText("one", { timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Locator: frameLocator().locator('div')`) != null, true);
    // [ts] });
    // [ts] test("LO-016 should support evaluate", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div data-foo="bar">Hello</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.frameLocator().locator("div").evaluate(e => e.getAttribute("data-foo")), "bar");
    // [ts] });
    // [ts] test("LO-017 should fail evaluate when elements match in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>one</div>`);
    // [ts] routePage(page, "b.html", `<div>two</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 3, "div");
    // [ts] const error = page.frameLocator().locator("div").evaluate(e => e.textContent, null, { timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] });
    // [ts] test("LO-018 should time out on evaluate when there are no matches", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>Nothing here</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const error = page.frameLocator().locator("button").evaluate(e => e.textContent, null, { timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for frameLocator().locator('button')`) != null, true);
    // [ts] });
    // [ts] test("LO-019 should support evaluateAll", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<span>one</span><span>two</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "span");
    // [ts] expect_eq(page.frameLocator().locator("span").evaluateAll(els => els.map(e => e.textContent)), ["one", "two"]);
    // [ts] expect_eq(page.frameLocator().locator("button").evaluateAll(els => els.length), 0);
    // [ts] });
    // [ts] test("LO-020 should fail evaluateAll when elements match in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<span>one</span>`);
    // [ts] routePage(page, "b.html", `<span>two</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 3, "span");
    // [ts] const error = page.frameLocator().locator("span").evaluateAll(els => els.length).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] });
    // [ts] test("LO-021 should support hasText filter", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>foo</div><div>bar</div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("div", { hasText: "bar" })).toHaveText("bar");
    // [ts] });
    // [ts] test("LO-022 should support first/last/nth", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<span>one</span><span>two</span><span>three</span>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "span");
    // [ts] expect(page.frameLocator().locator("span").first()).toHaveText("one");
    // [ts] expect(page.frameLocator().locator("span").last()).toHaveText("three");
    // [ts] expect(page.frameLocator().locator("span").nth(1)).toHaveText("two");
    // [ts] });
    // [ts] test("LO-023 should support nth in the middle of the chain", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div><span>one</span></div><div><span>two</span></div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("div").nth(1).locator("span")).toHaveText("two");
    // [ts] });
    // [ts] test("LO-024 should support composite locators", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div><span>foo</span></div><div><i>bar</i></div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("div", { has: page.locator("span") })).toHaveText("foo");
    // [ts] });
    // [ts] test("LO-025 should support capture", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div id="target"><span>hello</span></div>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("*css=div >> span")).toHaveAttribute("id", "target");
    // [ts] });
    // [ts] test("LO-026 should find a frame inside the scope", () => {
    // [ts] routePage(page, "empty.html", `<section><iframe src="a.html"></iframe></section><button>outside</button>`);
    // [ts] routePage(page, "a.html", `<button>inside</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "button");
    // [ts] const scope = (page.$("section"));
    // [ts] const buttons = scope.$$("internal:control=any-frame >> button");
    // [ts] expect_eq(buttons.length, 1);
    // [ts] expect_eq(buttons[0].textContent(), "inside");
    // [ts] });
    // [ts] test("LO-027 should find a nested frame inside the scope", () => {
    // [ts] routePage(page, "empty.html", `<section><iframe src="a.html"></iframe></section><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<iframe src="c.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<button>outside</button>`);
    // [ts] routePage(page, "c.html", `<button>deep</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => page.frames().length).toBe(4);
    // [ts] for (const frame of page.frames()) {
    // [ts] if (frame.url().includes("b.html") || frame.url().includes("c.html"))
    // [ts] frame.waitForSelector("button", { state: "attached" });
    // [ts] }
    // [ts] const scope = (page.$("section"));
    // [ts] const buttons = scope.$$("internal:control=any-frame >> button");
    // [ts] expect_eq(buttons.length, 1);
    // [ts] expect_eq(buttons[0].textContent(), "deep");
    // [ts] });
    // [ts] test("LO-028 should find a frame inside the scope while another iframe is stalled", () => {
    // [ts] routePage(page, "empty.html", `<section><iframe src="a.html"></iframe><iframe src="stall.html"></iframe></section>`);
    // [ts] routePage(page, "a.html", `<button>inside</button>`);
    // [ts] page.route("**/stall.html", () => {});
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "domcontentloaded" });
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] const scope = (page.$("section"));
    // [ts] const buttons = scope.$$("internal:control=any-frame >> button");
    // [ts] expect_eq(buttons.length, 1);
    // [ts] expect_eq(buttons[0].textContent(), "inside");
    // [ts] });
    // [ts] test("LO-029 should respect the scope without a frame inside the scope", () => {
    // [ts] routePage(page, "empty.html", `<section><button>target</button></section><iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button>in-frame</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "button");
    // [ts] const scope = (page.$("section"));
    // [ts] const buttons = scope.$$("internal:control=any-frame >> button");
    // [ts] expect_eq(buttons.length, 1);
    // [ts] expect_eq(buttons[0].textContent(), "target");
    // [ts] });
    // [ts] test("LO-030 should not match a chain across a frame boundary", () => {
    // [ts] routePage(page, "empty.html", `<section><iframe src="a.html"></iframe></section>`);
    // [ts] routePage(page, "a.html", `<iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<button>deep</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] const deepFrame = page.frames().find(f => f.url().includes("b.html"));
    // [ts] deepFrame.waitForSelector("button", { state: "attached" });
    // "section" lives in the main frame, while the button is two frames below it.
    // [ts] expect(page.frameLocator().locator("section").locator("button")).toHaveCount(0);
    // [ts] expect(page.frameLocator().locator("button")).toHaveText("deep");
    // [ts] });
    // [ts] test("LO-031 should only search frames inside the starting frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><button>main</button>`);
    // [ts] routePage(page, "a.html", `<iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<button>deep</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] const deepFrame = page.frames().find(f => f.url().includes("b.html"));
    // [ts] deepFrame.waitForSelector("button", { state: "attached" });
    // [ts] const middleFrame = page.frames().find(f => f.url().includes("a.html"));
    // [ts] expect(middleFrame.frameLocator().locator("button")).toHaveText("deep");
    // [ts] });
    // [ts] test("LO-032 should enter a frame found in a nested frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><button>main</button>`);
    // [ts] routePage(page, "a.html", `<iframe id="target" src="b.html"></iframe><button>decoy</button>`);
    // [ts] routePage(page, "b.html", `<button>inside</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().frameLocator("#target").locator("button")).toHaveText("inside");
    // [ts] });
    // [ts] test("LO-033 should click inside an entered frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<iframe id="target" src="b.html"></iframe><button>Click me</button>`);
    // [ts] routePage(page, "b.html", `<button onclick="window.__clicked = true">Click me</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.frameLocator().frameLocator("#target").getByRole("button", { name: "Click me" }).click();
    // [ts] const frame = page.frames().find(f => f.url().includes("b.html"));
    // [ts] expect_eq(frame.evaluate(() => (window).__clicked), true);
    // [ts] });
    // [ts] test("LO-034 should not search nested frames after entering a frame", () => {
    // [ts] routePage(page, "empty.html", `<iframe id="target" src="a.html"></iframe><button>main</button>`);
    // [ts] routePage(page, "a.html", `<iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<button>deep</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // The entered frame itself has no button, and we do not look inside its nested frames.
    // [ts] expect(page.frameLocator().frameLocator("#target").locator("button")).toHaveCount(0);
    // [ts] });
    // [ts] test("LO-035 should support two frameLocators", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<iframe id="x" src="b.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<iframe id="y" src="c.html"></iframe><button>decoy</button>`);
    // [ts] routePage(page, "c.html", `<button>bottom</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().frameLocator("#x").frameLocator("#y").locator("button")).toHaveText("bottom");
    // [ts] });
    // [ts] test("LO-036 should support locator before frameLocator", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<section><iframe src="b.html"></iframe></section><iframe src="c.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<button>in-section</button>`);
    // [ts] routePage(page, "c.html", `<button>outside</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("section").frameLocator("iframe").locator("button")).toHaveText("in-section");
    // [ts] });
    // [ts] test("LO-037 should support owner of a frameLocator", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<iframe id="target" src="b.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<button>inside</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.frameLocator().frameLocator("#target").owner().getAttribute("id"), "target");
    // [ts] });
    // [ts] test("LO-038 should wait for the frame to enter to appear", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div>Nothing yet</div>`);
    // [ts] routePage(page, "b.html", `<button>late</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => page.frames().length).toBe(2);
    // [ts] page.frames()[1].evaluate(() => {
    // [ts] window.builtins.setTimeout(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.id = "late";
    // [ts] iframe.src = "b.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] }, 3000);
    // [ts] });
    // [ts] expect(page.frameLocator().frameLocator("#late").locator("button")).toHaveText("late");
    // [ts] });
    // [ts] test("LO-039 should fail when the frame to enter matches in multiple frames", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="b.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<iframe class="inner" src="c.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<iframe class="inner" src="c.html"></iframe>`);
    // [ts] routePage(page, "c.html", `<button>Click me</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => page.frames().length).toBe(5);
    // [ts] for (const frame of page.frames()) {
    // [ts] if (frame.url().includes("c.html"))
    // [ts] frame.waitForSelector("button", { state: "attached" });
    // [ts] }
    // [ts] const error = page.frameLocator().frameLocator(".inner").locator("button").click({ timeout: 3000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] });
    // [ts] test("LO-040 should support contentFrame", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<iframe id="target" src="b.html"></iframe><button>decoy</button>`);
    // [ts] routePage(page, "b.html", `<button>inside</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator().locator("#target").contentFrame().locator("button")).toHaveText("inside");
    // [ts] });
    // [ts] test("LO-041 should render frameLocator() in the locator description", () => {
    // [ts] expect_eq(String(page.frameLocator().frameLocator("#x").locator("button")), `frameLocator().locator('#x').contentFrame().locator('button')`);
    // [ts] expect_eq(String(page.frameLocator().locator("section").frameLocator("iframe").getByText("foo")), `frameLocator().locator('section').locator('iframe').contentFrame().getByText('foo')`);
    // [ts] });
    // [ts] test("LO-042 should not allow frameLocator() inside a composite locator", () => {
    // [ts] routePage(page, "empty.html", `<button>main</button><iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<a href="#">link</a>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "a");
    // [ts] const error = page.locator("button").or(page.frameLocator().locator("a")).count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `frameLocator() is not allowed inside composite locators, while querying "locator('button').or(frameLocator().locator('a'))"`) != null, true);
    // [ts] const error2 = page.locator("button").filter({ has: page.frameLocator().locator("a") }).count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error2.message, `frameLocator() is not allowed inside composite locators`) != null, true);
    // Repeating frameLocator() in the operand is not allowed either, even though the outer locator has it.
    // [ts] const error3 = page.frameLocator().locator("button").or(page.frameLocator().locator("a")).count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error3.message, `frameLocator() is not allowed inside composite locators, while querying "frameLocator().locator('button').or(frameLocator().locator('a'))"`) != null, true);
    // Repeating frameLocator() is not allowed even when the rest of the frame chain matches.
    // [ts] const error4 = page.frameLocator().frameLocator("#f").locator("a").or(page.frameLocator().frameLocator("#f").locator("button")).count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error4.message, `frameLocator() is not allowed inside composite locators`) != null, true);
    // With frameLocator() first, the token applies to the whole locator, so both operands are searched in every frame.
    // [ts] const error5 = page.frameLocator().locator("a").or(page.locator("button")).count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error5.message, `frameLocator() matched elements in multiple frames`) != null, true);
    // [ts] });
    // [ts] test("LO-043 should support a composite locator under frameLocator()", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<div class="classname">first</div><button>second</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "button");
    // [ts] expect(page.frameLocator().locator(".classname").or(page.getByRole("button"))).toHaveText(["first", "second"]);
    // [ts] });
    // [ts] test("LO-044 should support a composite locator under frameLocator() and a frame locator", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<iframe id="f" src="b.html"></iframe>`);
    // [ts] routePage(page, "b.html", `<div class="classname">first</div><button>second</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] expect(page.frameLocator().frameLocator("#f").locator(".classname").or(page.frameLocator("#f").getByRole("button"))).toHaveText(["first", "second"]);
    // [ts] });
    // [ts] test("LO-045 should not allow first/last/nth on frameLocator()", () => {
    // [ts] expect(() => page.frameLocator().first()).toThrow("Selecting the nth frame is not allowed on frameLocator()");
    // [ts] expect(() => page.frameLocator().last()).toThrow("Selecting the nth frame is not allowed on frameLocator()");
    // [ts] expect(() => page.frameLocator().nth(1)).toThrow("Selecting the nth frame is not allowed on frameLocator()");
    // [ts] });
    // [ts] test("LO-046 should not allow owner on frameLocator()", () => {
    // [ts] const error = page.frameLocator().owner().count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Selector cannot be empty after frameLocator()") != null, true);
    // [ts] });
    // [ts] test("LO-047 should resolve aria-ref selectors", () => {
    // [ts] routePage(page, "empty.html", `<button>main</button><iframe src="a.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button>inside</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] waitForAllFrames(page, 2, "button");
    // [ts] const snapshot = page.ariaSnapshot({ mode: "ai" });
    // Refs are unique across frames, so every starting frame resolves them to the same element.
    // [ts] const insideMatch = snapshot.match("button \"inside\" \\[ref=(.*?)\\]");
    // [ts] expect(insideMatch[1]).toMatch("^f\\d+e\\d+$");
    // [ts] expect(page.frameLocator().locator(`aria-ref=${insideMatch[1]}`)).toHaveText("inside");
    // [ts] });
    // [ts] test("LO-048 should click while another iframe is stalled", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="stall.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button onclick="window.__clicked = true">Click me</button>`);
    // [ts] page.route("**/stall.html", () => {});
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "domcontentloaded" });
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] page.frameLocator().locator("button").click();
    // [ts] const frame = page.frames().find(f => f.url().includes("a.html"));
    // [ts] expect_eq(frame.evaluate(() => (window).__clicked), true);
    // [ts] });
    // [ts] test("LO-049 should support toBeVisible while another iframe is stalled", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="stall.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button>Click me</button>`);
    // [ts] page.route("**/stall.html", () => {});
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "domcontentloaded" });
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] expect(page.frameLocator().locator("button")).toBeVisible();
    // [ts] });
    // [ts] test("LO-050 should support toHaveCount while another iframe is stalled", () => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html"></iframe><iframe src="stall.html"></iframe>`);
    // [ts] routePage(page, "a.html", `<button>Click me</button>`);
    // [ts] page.route("**/stall.html", () => {});
    // [ts] page.goto(server.EMPTY_PAGE, { waitUntil: "domcontentloaded" });
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] expect(page.frameLocator().locator("button")).toHaveCount(1);
    // [ts] });
    // [ts] test("LO-051 should treat elements inside hidden iframe as hidden", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42719" } }, (_p) => {
    // [ts] routePage(page, "empty.html", `<iframe src="a.html" style="visibility: hidden"></iframe>`);
    // [ts] routePage(page, "a.html", `<button onclick="window.__clicked = true">Click me</button>`);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.frameLocator().locator("button");
    // [ts] expect(button).toBeAttached();
    // [ts] expect_eq(button.isVisible(), false);
    // [ts] expect(button).toBeHidden();
    // [ts] const error = button.click({ timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is inside a hidden frame, retrying") != null, true);
    // [ts] expect_eq(page.frames()[1].evaluate(() => (window).__clicked), null);
    // [ts] page.evaluate(() => document.querySelector("iframe").style.visibility = "visible");
    // [ts] expect(button).toBeVisible();
    // [ts] button.click();
    // [ts] expect_eq(page.frames()[1].evaluate(() => (window).__clicked), true);
    // [ts] });
}
