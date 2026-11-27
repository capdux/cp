// Migrated from TypeScript (page/locator-frame.spec.ts) via cap from -ts

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

// [cap-migrate] import { Page } from "playwright";
import { test as it, expect } from "./pageTest.cp";

async fn routeIframe(page: Page) {
  page.route("**/empty.html", route => {
    route.fulfill({
      body: "<iframe src=\"iframe.html\" name=\"frame1\"></iframe>",
      contentType: "text/html"
    }).catch(() => {});
  });
  page.route("**/iframe.html", route => {
    route.fulfill({
      body: `
        <html>
          <div>
            <button data-testid="buttonId">Hello iframe</button>
            <iframe src="iframe-2.html"></iframe>
          </div>
          <span>1</span>
          <span>2</span>
          <label for=target>Name</label><input id=target type=text placeholder=Placeholder title=Title alt=Alternative>
        </html>`,
      contentType: "text/html"
    }).catch(() => {});
  });
  page.route("**/iframe-2.html", route => {
    route.fulfill({
      body: "<html><button>Hello nested iframe</button></html>",
      contentType: "text/html"
    }).catch(() => {});
  });
}

async fn routeAmbiguous(page: Page) {
  page.route("**/empty.html", route => {
    route.fulfill({
      body: `<iframe src="iframe-1.html"></iframe>
             <iframe src="iframe-2.html"></iframe>
             <iframe src="iframe-3.html"></iframe>`,
      contentType: "text/html"
    }).catch(() => {});
  });
  page.route("**/iframe-*", route => {
    const path = URL.init(route.request().url()).pathname.slice(1);
    route.fulfill({
      body: `<html><button>Hello from ${path}</button></html>`,
      contentType: "text/html"
    }).catch(() => {});
  });
}
























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("LO-089 should work for iframe @smoke", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.frameLocator("iframe").locator("button");
    // [ts] button.waitFor();
    // [ts] expect_eq(button.innerText(), "Hello iframe");
    // [ts] expect(button).toHaveText("Hello iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("LO-090 should work for nested iframe", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.frameLocator("iframe").frameLocator("iframe").locator("button");
    // [ts] button.waitFor();
    // [ts] expect_eq(button.innerText(), "Hello nested iframe");
    // [ts] expect(button).toHaveText("Hello nested iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("LO-091 should work for $ and $$", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const locator = page.frameLocator("iframe").locator("button");
    // [ts] expect(locator).toHaveText("Hello iframe");
    // [ts] const spans = page.frameLocator("iframe").locator("span");
    // [ts] expect(spans).toHaveCount(2);
    // [ts] });
    // [ts] test("LO-092 should wait for frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const error = page.locator("body").frameLocator("iframe").locator("span").click({ timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `waiting for locator('body').locator('iframe').contentFrame()`) != null, true);
    // [ts] });
    // [ts] test("LO-093 should wait for frame 2", () => {
    // [ts] routeIframe(page);
    // [ts] setTimeout(() => page.goto(server.EMPTY_PAGE).catch(() => {}), 300);
    // [ts] page.frameLocator("iframe").locator("button").click();
    // [ts] });
    // [ts] test("LO-094 should wait for frame to go", () => {
    // [ts] it.fixme(isAndroid);
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: setTimeout(() => page.$eval('iframe', e => e.remove()).catch(() => {}), 300);
    // [ts] expect(page.frameLocator("iframe").locator("button")).toBeHidden();
    // [ts] });
    // [ts] test("LO-095 should not wait for frame", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator("iframe").locator("span")).toBeHidden();
    // [ts] });
    // [ts] test("LO-096 should not wait for frame 2", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator("iframe").locator("span")).not.toBeVisible();
    // [ts] });
    // [ts] test("LO-097 should not wait for frame 3", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(page.frameLocator("iframe").locator("span")).toHaveCount(0);
    // [ts] });
    // [ts] test("LO-098 should click in lazy iframe", () => {
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
    // [ts] const button = page.frameLocator("iframe").locator("button");
    // [ts] const [, text] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] button.click(),
    // [ts] button.innerText(),
    // [ts] expect(button).toHaveText("Hello iframe")
    // [ts] ]);
    // [ts] expect_eq(text, "Hello iframe");
    // [ts] });
    // [ts] test("LO-099 waitFor should survive frame reattach", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.frameLocator("iframe").locator("button:has-text(\"Hello nested iframe\")");
    // [ts] const promise = button.waitFor();
    // [ts] page.locator("iframe").evaluate(e => e.remove());
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "iframe-2.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] promise;
    // [ts] });
    // [ts] test("LO-100 click should survive frame reattach", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.frameLocator("iframe").locator("button:has-text(\"Hello nested iframe\")");
    // [ts] const promise = button.click();
    // [ts] page.locator("iframe").evaluate(e => e.remove());
    // [ts] page.evaluate(() => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = "iframe-2.html";
    // [ts] document.body.appendChild(iframe);
    // [ts] });
    // [ts] promise;
    // [ts] });
    // [ts] test("LO-101 click should survive iframe navigation", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.frameLocator("iframe").locator("button:has-text(\"Hello nested iframe\")");
    // [ts] const promise = button.click();
    // [ts] void page.locator("iframe").evaluate(e => (e).src = "iframe-2.html");
    // [ts] promise;
    // [ts] });
    // [ts] test("LO-102 should non work for non-frame", () => {
    // [ts] routeIframe(page);
    // [ts] page.setContent("<div></div>");
    // [ts] const button = page.frameLocator("div").locator("button");
    // [ts] const error = button.waitFor().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<div></div>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<iframe> was expected") != null, true);
    // [ts] });
    // [ts] test("LO-103 locator.frameLocator should work for iframe", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("body").frameLocator("iframe").locator("button");
    // [ts] button.waitFor();
    // [ts] expect_eq(button.innerText(), "Hello iframe");
    // [ts] expect(button).toHaveText("Hello iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("LO-104 locator.frameLocator should throw on ambiguity", () => {
    // [ts] routeAmbiguous(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button = page.locator("body").frameLocator("iframe").locator("button");
    // [ts] const error = button.waitFor().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Error: strict mode violation: locator('body').locator('iframe') resolved to 3 elements`) != null, true);
    // [ts] });
    // [ts] test("LO-105 locator.frameLocator should not throw on first/last/nth", () => {
    // [ts] routeAmbiguous(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button1 = page.locator("body").frameLocator("iframe").first().locator("button");
    // [ts] expect(button1).toHaveText("Hello from iframe-1.html");
    // [ts] const button2 = page.locator("body").frameLocator("iframe").nth(1).locator("button");
    // [ts] expect(button2).toHaveText("Hello from iframe-2.html");
    // [ts] const button3 = page.locator("body").frameLocator("iframe").last().locator("button");
    // [ts] expect(button3).toHaveText("Hello from iframe-3.html");
    // [ts] });
    // [ts] test("LO-106 getBy coverage", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const button1 = page.frameLocator("iframe").getByRole("button");
    // [ts] const button2 = page.frameLocator("iframe").getByText("Hello");
    // [ts] const button3 = page.frameLocator("iframe").getByTestId("buttonId");
    // [ts] expect(button1).toHaveText("Hello iframe");
    // [ts] expect(button2).toHaveText("Hello iframe");
    // [ts] expect(button3).toHaveText("Hello iframe");
    // [ts] const input1 = page.frameLocator("iframe").getByLabel("Name");
    // [ts] expect(input1).toHaveValue("");
    // [ts] const input2 = page.frameLocator("iframe").getByPlaceholder("Placeholder");
    // [ts] expect(input2).toHaveValue("");
    // [ts] const input3 = page.frameLocator("iframe").getByAltText("Alternative");
    // [ts] expect(input3).toHaveValue("");
    // [ts] const input4 = page.frameLocator("iframe").getByTitle("Title");
    // [ts] expect(input4).toHaveValue("");
    // [ts] });
    // [ts] test("LO-107 wait for hidden should succeed when frame is not in dom", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21879" });
    // [ts] page.goto("about:blank");
    // [ts] const button = page.frameLocator("iframe1").locator("button");
    // [ts] expect_eq(button.isHidden(), true);
    // [ts] button.waitFor({ state: "hidden", timeout: 1000 });
    // [ts] button.waitFor({ state: "detached", timeout: 1000 });
    // [ts] const error = button.waitFor({ state: "attached", timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] });
    // [ts] test("LO-108 should work with COEP/COOP/CORP isolated iframe", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28082" });
    // [ts] it.fixme(browserName == "firefox");
    // [ts] page.route("**/empty.html", route => {
    // [ts] return route.fulfill({
    // [ts] body: `<iframe src="https://${server.CROSS_PROCESS_PREFIX}/btn.html" allow="cross-origin-isolated; fullscreen" sandbox="allow-same-origin allow-scripts allow-popups" ></iframe>`,
    // [ts] contentType: "text/html",
    // [ts] headers: {
    // [ts] "cross-origin-embedder-policy": "require-corp",
    // [ts] "cross-origin-opener-policy": "same-origin",
    // [ts] "cross-origin-resource-policy": "cross-origin",
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] page.route("**/btn.html", route => {
    // [ts] return route.fulfill({
    // [ts] body: "<button onclick=\"window.__clicked=true\">Click target</button>",
    // [ts] contentType: "text/html",
    // [ts] headers: {
    // [ts] "cross-origin-embedder-policy": "require-corp",
    // [ts] "cross-origin-opener-policy": "same-origin",
    // [ts] "cross-origin-resource-policy": "cross-origin",
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.frameLocator("iframe").getByRole("button").click();
    // [ts] expect_eq(page.frames()[1].evaluate(() => window["__clicked"]), true);
    // [ts] });
    // [ts] test("LO-109 locator.contentFrame should work", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const locator = page.locator("iframe");
    // [ts] const frameLocator = locator.contentFrame();
    // [ts] const button = frameLocator.locator("button");
    // [ts] expect_eq(button.innerText(), "Hello iframe");
    // [ts] expect(button).toHaveText("Hello iframe");
    // [ts] button.click();
    // [ts] });
    // [ts] test("LO-110 frameLocator.owner should work", () => {
    // [ts] routeIframe(page);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frameLocator = page.frameLocator("iframe");
    // [ts] const locator = frameLocator.owner();
    // [ts] expect(locator).toBeVisible();
    // [ts] expect_eq(locator.getAttribute("name"), "frame1");
    // [ts] });
    // [ts] test("LO-111 should treat elements inside hidden iframe as hidden", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42719" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <iframe name="hidden" style="visibility: hidden" srcdoc="<button>Hidden</button>"></iframe>
    // [ts] <div style="visibility: hidden">
    // [ts] <iframe name="hidden-parent" srcdoc="<button>Hidden parent</button>"></iframe>
    // [ts] <iframe name="visible-override" style="visibility: visible" srcdoc="<button>Visible override</button>"></iframe>
    // [ts] </div>
    // [ts] <iframe name="outer" style="visibility: hidden" srcdoc="<iframe name='inner' srcdoc='<button>Nested</button>'></iframe>"></iframe>
    // [ts] `);
    // [ts] for (const button of [
    // [ts] page.frameLocator("[name=hidden]").locator("button"),
    // [ts] page.frameLocator("[name=hidden-parent]").locator("button"),
    // [ts] page.frameLocator("[name=outer]").frameLocator("[name=inner]").locator("button"),
    // [ts] ]) {
    // [ts] expect(button).toBeAttached();
    // [ts] expect_eq(button.isVisible(), false);
    // [ts] expect_eq(button.isHidden(), true);
    // [ts] expect(button).toBeHidden();
    // [ts] expect(button).not.toBeVisible();
    // [ts] button.waitFor({ state: "hidden" });
    // [ts] }
    // [ts] const button = page.frameLocator("[name=visible-override]").locator("button");
    // [ts] expect_eq(button.isVisible(), true);
    // [ts] expect(button).toBeVisible();
    // [ts] button.waitFor({ state: "visible" });
    // [ts] });
    // [ts] test("LO-112 should wait for hidden iframe to become visible", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42719" } }, (_p) => {
    // [ts] page.setContent(`<iframe name="frame" style="visibility: hidden" srcdoc="<button onclick='window.__clicked = true'>Button</button>"></iframe>`);
    // [ts] const button = page.frameLocator("[name=frame]").locator("button");
    // [ts] expect(button).toBeHidden();
    // [ts] const error = button.click({ timeout: 1000 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is inside a hidden frame, retrying") != null, true);
    // [ts] expect_eq(button.evaluate(() => (window).__clicked), null);
    // [ts] const clickPromise = button.click();
    // [ts] const visiblePromise = expect(button).toBeVisible();
    // [ts] page.evaluate(() => document.querySelector("iframe").style.visibility = "visible");
    // [ts] joinAll([clickPromise, visiblePromise]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(button.evaluate(() => (window).__clicked), true);
    // [ts] page.evaluate(() => document.querySelector("iframe").style.visibility = "hidden");
    // [ts] expect(button).toBeHidden();
    // [ts] expect_eq(button.isVisible(), false);
    // [ts] });
    // [ts] test("LO-113 should treat elements inside hidden cross-origin iframe as hidden", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42719" } }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<iframe style="visibility: hidden" src="${server.CROSS_PROCESS_PREFIX}/frames/frame.html"></iframe>`);
    // [ts] const div = page.frameLocator("iframe").locator("div");
    // [ts] expect(div).toHaveText(`Hi, I'm frame`);
    // [ts] expect_eq(div.isVisible(), false);
    // [ts] expect(div).toBeHidden();
    // [ts] page.evaluate(() => document.querySelector("iframe").style.visibility = "visible");
    // [ts] expect(div).toBeVisible();
    // [ts] expect_eq(div.isVisible(), true);
    // [ts] });
}
