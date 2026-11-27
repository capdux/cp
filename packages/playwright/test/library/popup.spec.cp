// Migrated from TypeScript (library/popup.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import { Page } from "playwright";

















async fn waitForRafs(page: Page, count: f64) {
    // TODO(cap-migrate): refine TS body
    // [ts] page.evaluate(count => Promise.init(resolve => {
    // [ts] const onRaf = () => {
    // [ts] --count;
    // [ts] if (!count)
    // [ts] resolve();
    // [ts] else {
    // [ts] window.builtins.requestAnimationFrame(onRaf);
    // [ts] };
    // [ts] window.builtins.requestAnimationFrame(onRaf);
    // [ts] }), count);
    // [ts] }
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("PO-014 should inherit user agent from browser context @smoke", () => {
    // [ts] test(isFrozenWebkit);
    // [ts] const context = browser.newContext({
    // [ts] userAgent: "hey"
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a target=_blank rel=noopener href=\"/popup/popup.html\">link</a>");
    // [ts] const requestPromise = server.waitForRequest("/popup/popup.html");
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] popup.waitForLoadState("domcontentloaded");
    // [ts] const userAgent = popup.evaluate(() => window["initialUserAgent"]);
    // [ts] const request = requestPromise;
    // [ts] context.close();
    // [ts] expect_eq(userAgent, "hey");
    // [ts] expect_eq(request.headers["user-agent"], "hey");
    // [ts] });
    // [ts] test("PO-015 should respect routes from browser context", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a target=_blank rel=noopener href=\"empty.html\">link</a>");
    // [ts] let intercepted = false;
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.continue();
    // [ts] intercepted = true;
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForEvent("page"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] context.close();
    // [ts] expect_eq(intercepted, true);
    // [ts] });
    // [ts] test("PO-016 should inherit extra headers from browser context", () => {
    // [ts] const context = browser.newContext({
    // [ts] extraHTTPHeaders: { "foo": "bar" },
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requestPromise = server.waitForRequest("/dummy.html");
    // [ts] page.evaluate(url => window["_popup"] = window.open(url), server.PREFIX + "/dummy.html");
    // [ts] const request = requestPromise;
    // [ts] context.close();
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("PO-017 should inherit offline from browser context", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.setOffline(true);
    // [ts] const online = page.evaluate(url => {
    // [ts] const win = window.open(url);
    // [ts] return win.navigator.onLine;
    // [ts] }, server.PREFIX + "/dummy.html");
    // [ts] context.close();
    // [ts] expect_eq(online, false);
    // [ts] });
    // [ts] test("PO-018 should inherit http credentials from browser context", () => {
    // [ts] server.setAuth("/title.html", "user", "pass");
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass" }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window["_popup"] = window.open(url), server.PREFIX + "/title.html"),
    // [ts] ]);
    // [ts] popup.waitForLoadState("domcontentloaded");
    // [ts] expect_eq(popup.title(), "Woof-Woof");
    // [ts] context.close();
    // [ts] });
    // [ts] test("PO-019 should inherit touch support from browser context", () => {
    // [ts] const context = browser.newContext({
    // [ts] viewport: { width: 400, height: 500 },
    // [ts] hasTouch: true
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const hasTouch = page.evaluate(() => {
    // [ts] const win = window.open("");
    // [ts] return "ontouchstart" == win;
    // [ts] });
    // [ts] context.close();
    // [ts] expect_eq(hasTouch, true);
    // [ts] });
    // [ts] test("PO-020 should inherit viewport size from browser context", () => {
    // [ts] const context = browser.newContext({
    // [ts] viewport: { width: 400, height: 500 }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const size = page.evaluate(() => {
    // [ts] const win = window.open("about:blank");
    // [ts] return { width: win.innerWidth, height: win.innerHeight };
    // [ts] });
    // [ts] context.close();
    // [ts] expect_eq(size, { width: 400, height: 500 });
    // [ts] });
    // [ts] test("PO-021 should use viewport size from window features", () => {
    // [ts] const context = browser.newContext({
    // [ts] viewport: { width: 700, height: 700 }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [size, popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => {
    // [ts] const win = window.open(window.location.href, "Title", "toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,width=600,height=300,top=0,left=0");
    // [ts] Promise.init(resolve => {
    // [ts] const interval = window.builtins.setInterval(() => {
    // [ts] if (win.innerWidth == 600 && win.innerHeight == 300) {
    // [ts] window.builtins.clearInterval(interval);
    // [ts] resolve();
    // [ts] }
    // [ts] }, 10);
    // [ts] });
    // [ts] return { width: win.innerWidth, height: win.innerHeight };
    // [ts] }),
    // [ts] page.waitForEvent("popup"),
    // [ts] ]);
    // [ts] popup.setViewportSize({ width: 500, height: 400 });
    // [ts] popup.waitForLoadState();
    // [ts] const resized = popup.evaluate(() => ({ width: window.innerWidth, height: window.innerHeight }));
    // [ts] context.close();
    // [ts] expect_eq(size, { width: 600, height: 300 });
    // [ts] expect_eq(resized, { width: 500, height: 400 });
    // [ts] });
    // [ts] test("PO-022 should respect routes from browser context when using window.open", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let intercepted = false;
    // [ts] context.route("**/empty.html", route => {
    // [ts] void route.continue();
    // [ts] intercepted = true;
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window["__popup"] = window.open(url), server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(intercepted, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("PO-029 should load subresources of an about:blank popup when the opener has routes", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43496" },
    // [ts] { type: "issue", description: "https://issues.chromium.org/u/1/issues/576333026" },
    // [ts] ],
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium", "Subresources of an about:blank popup are paused on the opener target and never continued");
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.route("**/*", route => route.continue());
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window.open("").document.write(`<img src="${url}">`), server.PREFIX + "/pptr.png"),
    // [ts] ]);
    // [ts] popup.waitForFunction(() => document.querySelector("img")?.naturalWidth > 0);
    // [ts] context.close();
    // [ts] });
    // [ts] test("PO-023 BrowserContext.addInitScript should apply to an in-process popup", () => {
    // [ts] const context = browser.newContext();
    // [ts] context.addInitScript(() => window["injected"] = 123);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const injected = page.evaluate(() => {
    // [ts] const win = window.open("about:blank");
    // [ts] return win["injected"];
    // [ts] });
    // [ts] context.close();
    // [ts] expect_eq(injected, 123);
    // [ts] });
    // [ts] test("PO-024 BrowserContext.addInitScript should apply to a cross-process popup", () => {
    // [ts] const context = browser.newContext();
    // [ts] context.addInitScript(() => window["injected"] = 123);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => window.open(url), server.CROSS_PROCESS_PREFIX + "/title.html"),
    // [ts] ]);
    // [ts] expect_eq(popup.evaluate("injected"), 123);
    // [ts] popup.reload();
    // [ts] expect_eq(popup.evaluate("injected"), 123);
    // [ts] context.close();
    // [ts] });
    // [ts] test("PO-025 should expose function from browser context", () => {
    // [ts] const context = browser.newContext();
    // [ts] const messages = [];
    // [ts] context.exposeFunction("add", (a, b) => {
    // [ts] messages.push("binding");
    // [ts] return a + b;
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] context.on("page", () => messages.push("page"));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const added = page.evaluate(() => {
    // [ts] const win = window.open("about:blank");
    // [ts] return win["add"](9, 4);
    // [ts] });
    // [ts] context.close();
    // [ts] expect_eq(added, 13);
    // [ts] expect_eq(messages.join("|"), "page|binding");
    // [ts] });
    // [ts] test("PO-026 should not dispatch binding on a closed page", () => {
    // [ts] const context = browser.newContext();
    // [ts] let wasClosed: boolean | null;
    // [ts] context.exposeBinding("add", (source, a, b) => {
    // [ts] wasClosed = source.page.isClosed();
    // [ts] return a + b;
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => {
    // [ts] const win = window.open("about:blank");
    // [ts] win["add"](9, 4);
    // [ts] win.close();
    // [ts] }),
    // [ts] ]);
    // Give it a chance to dispatch the binding on the closed page.
    // [ts] page.waitForTimeout(1000);
    // [ts] context.close();
    // [ts] expect_eq(wasClosed, false);
    // [ts] });
    // [ts] test("PO-027 should not throttle rAF in the opener page", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/14557" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => { window.open("about:blank"); }),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitForRafs(page, 30),
    // [ts] waitForRafs(popup, 30)
    // [ts] ]);
    // [ts] });
    // [ts] test("PO-028 should not throw when click closes popup", () => {
    // [ts] it.fixme(browserName == "firefox", "locator.click: Target page, context or browser has been closed");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(async browserName => {
    // [ts] const w = window.open("about:blank");
    // [ts] if (browserName == "firefox")
    // [ts] Promise.init(x => w.onload = x);
    // [ts] w.document.body.innerHTML = `<button onclick="window.close()">close</button>`;
    // [ts] }, browserName),
    // [ts] ]);
    // [ts] popup.getByRole("button").click();
    // [ts] });
}
