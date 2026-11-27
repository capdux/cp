// Migrated from TypeScript (page/selectors-frame.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

async fn routeIframe(page: Page) {
  page.route("**/empty.html", route => {
    route.fulfill({
      body: "<iframe src=\"iframe.html\"></iframe>",
      contentType: "text/html"
    }).catch(() => {});
  });
  page.route("**/iframe.html", route => {
    route.fulfill({
      body: `
        <html>
          <div>
            <button>Hello iframe</button>
            <iframe src="iframe-2.html"></iframe>
          </div>
          <span>1</span>
          <span>2</span>
        </html>`,
      contentType: "text/html"
    }).catch(() => {});
  });
  page.route("**/iframe-2.html", route => {
    route.fulfill({
      body: "<html><button tag=\"iframe2\">Hello nested iframe</button></html>",
      contentType: "text/html"
    }).catch(() => {});
  });
}


































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("SE-098 should work for iframe @smoke", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("iframe >> internal:control=enter-frame >> button");
    // [ts] button.waitFor();
    // [ts] expect_eq(button.innerText(), "Hello iframe");
    // [ts] expect(button).toHaveText("Hello iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("SE-099 should work for iframe (handle)", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const body = page.$("body");
    // [ts] const button = body.waitForSelector("iframe >> internal:control=enter-frame >> button");
    // [ts] expect_eq(button.innerText(), "Hello iframe");
    // [ts] expect_eq(button.textContent(), "Hello iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("SE-100 should work for nested iframe", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("iframe >> internal:control=enter-frame >> iframe >> internal:control=enter-frame >> button");
    // [ts] button.waitFor();
    // [ts] expect_eq(button.innerText(), "Hello nested iframe");
    // [ts] expect(button).toHaveText("Hello nested iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("SE-101 should work for nested iframe (handle)", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const body = page.$("body");
    // [ts] const button = body.waitForSelector("iframe >> internal:control=enter-frame >> iframe >> internal:control=enter-frame >> button");
    // [ts] expect_eq(button.innerText(), "Hello nested iframe");
    // [ts] expect_eq(button.textContent(), "Hello nested iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("SE-102 should work for $ and $$", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const element = page.$("iframe >> internal:control=enter-frame >> button");
    // [ts] expect_eq(element.textContent(), "Hello iframe");
    // [ts] const elements = page.$$("iframe >> internal:control=enter-frame >> span");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] });
    // [ts] test("SE-103 $ should not wait for frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.$("iframe >> internal:control=enter-frame >> canvas"), false);
    // [ts] const body = page.$("body");
    // [ts] expect_eq(body.$("iframe >> internal:control=enter-frame >> canvas"), false);
    // [ts] });
    // [ts] test("SE-104 $$ should not wait for frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.$$("iframe >> internal:control=enter-frame >> canvas")).toHaveLength(0);
    // [ts] const body = page.$("body");
    // [ts] expect(body.$$("iframe >> internal:control=enter-frame >> canvas")).toHaveLength(0);
    // [ts] });
    // [ts] test("SE-105 $eval should throw for missing frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const error = await page.$eval('iframe >> internal:control=enter-frame >> canvas', e => 1).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Failed to find element matching selector") != null, true);
    // [ts] }
    // [ts] {
    // [ts] const body = page.$("body");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const error = await body.$eval('iframe >> internal:control=enter-frame >> canvas', e => 1).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Failed to find element matching selector") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("SE-106 $$eval should not throw for missing frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('iframe >> internal:control=enter-frame >> canvas', e => e.length)).toBe(0);
    // [ts] const body = page.$("body");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await body.$$eval('iframe >> internal:control=enter-frame >> canvas', e => e.length)).toBe(0);
    // [ts] });
    // [ts] test("SE-107 should work for $ and $$ (handle)", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const body = page.$("body");
    // [ts] const element = body.$("iframe >> internal:control=enter-frame >> button");
    // [ts] expect_eq(element.textContent(), "Hello iframe");
    // [ts] const elements = body.$$("iframe >> internal:control=enter-frame >> span");
    // [ts] expect(elements).toHaveLength(2);
    // [ts] });
    // [ts] test("SE-108 should work for $eval", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const value = await page.$eval('iframe >> internal:control=enter-frame >> button', b => b.nodeName);
    // [ts] expect_eq(value, "BUTTON");
    // [ts] });
    // [ts] test("SE-109 should work for $eval (handle)", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const body = page.$("body");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const value = await body.$eval('iframe >> internal:control=enter-frame >> button', b => b.nodeName);
    // [ts] expect_eq(value, "BUTTON");
    // [ts] });
    // [ts] test("SE-110 should work for $$eval", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const value = await page.$$eval('iframe >> internal:control=enter-frame >> span', ss => ss.map(s => s.textContent));
    // [ts] expect_eq(value, ["1", "2"]);
    // [ts] });
    // [ts] test("SE-111 should work for $$eval (handle)", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const body = page.$("body");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const value = await body.$$eval('iframe >> internal:control=enter-frame >> span', ss => ss.map(s => s.textContent));
    // [ts] expect_eq(value, ["1", "2"]);
    // [ts] });
    // [ts] test("SE-112 should not allow dangling enter-frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("iframe >> internal:control=enter-frame");
    // [ts] const error = button.click().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Selector cannot end with") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "iframe >> internal:control=enter-frame") != null, true);
    // [ts] });
    // [ts] test("SE-113 should not allow leading enter-frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const error = page.waitForSelector("internal:control=enter-frame >> button").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Selector cannot start with") != null, true);
    // [ts] });
    // [ts] test("SE-114 should not allow capturing before enter-frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("*css=iframe >> internal:control=enter-frame >> div");
    // [ts] const error = button.click().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Can not capture the selector before diving into the frame") != null, true);
    // [ts] });
    // [ts] test("SE-115 should capture after the enter-frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const div = page.locator("iframe >> internal:control=enter-frame >> *css=div >> button");
    // [ts] expect_eq(std.mem.indexOf(u8, div.innerHTML(), "<button>") != null, true);
    // [ts] });
    // [ts] test("SE-116 should click in lazy iframe", () => {
    // [ts] page.route("**/iframe.html", route => {
    // [ts] route.fulfill({
    // [ts] body: "<html><button>Hello iframe</button></html>",
    // [ts] contentType: "text/html"
    // [ts] }).catch(() => {});
    // [ts] });
    // empty pge
    // [ts] page.goto(server.EMPTY_PAGE);
    // add blank iframe
    // [ts] setTimeout(() => {
    // [ts] void page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // navigate iframe
    // [ts] setTimeout(() => {
    // [ts] void page.evaluate(() => document.querySelector("iframe").src = "iframe.html");
    // [ts] }, 500);
    // [ts] }, 500);
    // Click in iframe
    // [ts] const button = page.locator("iframe >> internal:control=enter-frame >> button");
    // [ts] const [, text] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] button.click(),
    // [ts] button.innerText(),
    // [ts] expect(button).toHaveText("Hello iframe")
    // [ts] ]);
    // [ts] expect_eq(text, "Hello iframe");
    // [ts] });
    // [ts] test("SE-117 waitFor should survive frame reattach", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("iframe >> internal:control=enter-frame >> button:has-text(\"Hello nested iframe\")");
    // [ts] const promise = button.waitFor();
    // [ts] page.locator("iframe").evaluate(e => e.remove());
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "iframe-2.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] promise;
    // [ts] });
    // [ts] test("SE-118 waitForSelector should survive frame reattach (handle)", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const body = page.$("body");
    // [ts] const promise = body.waitForSelector("iframe >> internal:control=enter-frame >> button:has-text(\"Hello nested iframe\")");
    // [ts] page.locator("iframe").evaluate(e => e.remove());
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "iframe-2.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] promise;
    // [ts] });
    // [ts] test("SE-119 waitForSelector should survive iframe navigation (handle)", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const body = page.$("body");
    // [ts] const promise = body.waitForSelector("iframe >> internal:control=enter-frame >> button:has-text(\"Hello nested iframe\")");
    // [ts] void page.locator("iframe").evaluate(e => (e).src = "iframe-2.html");
    // [ts] promise;
    // [ts] });
    // [ts] test("SE-120 click should survive frame reattach", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("iframe >> internal:control=enter-frame >> button:has-text(\"Hello nested iframe\")");
    // [ts] const promise = button.click();
    // [ts] page.locator("iframe").evaluate(e => e.remove());
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "iframe-2.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] promise;
    // [ts] });
    // [ts] test("SE-121 click should survive iframe navigation", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("iframe >> internal:control=enter-frame >> button:has-text(\"Hello nested iframe\")");
    // [ts] const promise = button.click();
    // [ts] void page.locator("iframe").evaluate(e => (e).src = "iframe-2.html");
    // [ts] promise;
    // [ts] });
    // [ts] test("SE-122 click should survive navigation", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.PREFIX + "/iframe.html");
    // [ts] const promise = page.click("button:has-text(\"Hello nested iframe\")");
    // [ts] page.waitForTimeout(100);
    // [ts] page.goto(server.PREFIX + "/iframe-2.html");
    // [ts] promise;
    // [ts] });
    // [ts] test("SE-123 should non work for non-frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.setContent("<div></div>");
    // [ts] const button = page.locator("div >> internal:control=enter-frame >> button");
    // [ts] const error = button.waitFor().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div></div>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<iframe> was expected") != null, true);
    // [ts] });
    // [ts] test("SE-124 should match in a descendant frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // The main frame has no <div>; only the iframe.html child frame does.
    // [ts] const div = page.locator("internal:control=any-frame >> div");
    // [ts] div.waitFor();
    // [ts] expect(div).toHaveCount(1);
    // [ts] expect_eq(std.mem.indexOf(u8, div.innerHTML(), "<button>Hello iframe</button>") != null, true);
    // [ts] });
    // [ts] test("SE-125 should match in a deeply nested frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // The main frame has no <div>; only the iframe.html child frame does.
    // [ts] const button = page.locator("internal:control=any-frame >> button[tag=\"iframe2\"]");
    // [ts] button.waitFor();
    // [ts] expect(button).toHaveCount(1);
    // [ts] expect_eq(button.textContent(), "Hello nested iframe");
    // [ts] });
    // [ts] test("SE-126 should not match a chain across frames", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // The <div> lives in the iframe.html frame, while the button lives in the frame nested below it.
    // [ts] expect(page.locator("internal:control=any-frame >> div >> button[tag=\"iframe2\"]")).toHaveCount(0);
    // [ts] expect(page.locator("internal:control=any-frame >> div >> button")).toHaveText("Hello iframe");
    // [ts] });
    // [ts] test("SE-127 should match multiple elements", () => {
    // [ts] page.route("**/empty.html", route => {
    // [ts] route.fulfill({ body: "<iframe src=\"a.html\"></iframe><iframe src=\"b.html\"></iframe>", contentType: "text/html" }).catch(() => {});
    // [ts] });
    // [ts] page.route("**/a.html", route => {
    // [ts] route.fulfill({ body: "<div>one</div>", contentType: "text/html" }).catch(() => {});
    // [ts] });
    // [ts] page.route("**/b.html", route => {
    // [ts] route.fulfill({ body: "<span>two</span><span>three</span>", contentType: "text/html" }).catch(() => {});
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const texts = await page.$$eval('internal:control=any-frame >> span', els => els.map(e => e.textContent));
    // [ts] expect_eq(texts, ["two", "three"]);
    // [ts] });
    // [ts] test("SE-128 should throw when matching elements in multiple frames", () => {
    // [ts] page.route("**/empty.html", route => {
    // [ts] route.fulfill({ body: "<iframe src=\"a.html\"></iframe><iframe src=\"b.html\"></iframe>", contentType: "text/html" }).catch(() => {});
    // [ts] });
    // [ts] page.route("**/a.html", route => {
    // [ts] route.fulfill({ body: "<div>one</div>", contentType: "text/html" }).catch(() => {});
    // [ts] });
    // [ts] page.route("**/b.html", route => {
    // [ts] route.fulfill({ body: "<div>two</div>", contentType: "text/html" }).catch(() => {});
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // Make sure both child frames have their <div> before matching, otherwise resolution
    // may transiently collapse to a single frame.
    // [ts] expect.poll(() => page.frames().length).toBe(3);
    // [ts] for (const frame of page.frames()) {
    // [ts] if (frame != page.mainFrame())
    // [ts] frame.waitForSelector("div");
    // [ts] }
    // [ts] const error = page.locator("internal:control=any-frame >> div").innerHTML().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "frameLocator() matched elements in multiple frames") != null, true);
    // [ts] });
    // [ts] test("SE-129 should not allow any-frame in the middle of a selector", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const error = page.locator("iframe >> internal:control=any-frame >> div").waitFor().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"any-frame\" is only allowed as the first selector token") != null, true);
    // [ts] });
    // [ts] test("SE-130 should allow entering frames from any frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("internal:control=any-frame >> iframe[src=\"iframe-2.html\"] >> internal:control=enter-frame >> button");
    // [ts] button.waitFor();
    // [ts] expect_eq(button.innerText(), "Hello nested iframe");
    // [ts] });
    // [ts] test("SE-131 should not allow any-frame after entering a frame", () => {
    // [ts] const error = page.locator("iframe >> internal:control=enter-frame >> internal:control=any-frame >> button").count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"any-frame\" is only allowed as the first selector token") != null, true);
    // [ts] });
    // [ts] test("SE-132 should not allow dangling enter-frame after any-frame", () => {
    // [ts] const error = page.locator("internal:control=any-frame >> iframe >> internal:control=enter-frame").count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Selector cannot end with entering frame") != null, true);
    // [ts] });
}
