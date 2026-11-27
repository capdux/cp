// Migrated from TypeScript (library/chromium/oopif.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import { contextTest as it, expect } from "../../config/browserTest.cp";
import { attachFrame, rafraf } from "../../config/utils.cp";
// [cap-migrate] import { Frame, Browser } from "cap:test";

it.use({
  launchOptions: (_p, use) => {
    use({ ...launchOptions, args: ["--site-per-process"] });
  }
});






it.fixme("OO-006 should get the proper viewport", (_p) => {
  expect_eq(page.viewportSize(), { width: 1280, height: 720 });
  page.goto(server.PREFIX + "/dynamic-oopif.html");
  expect_eq(page.frames().length, 2);
  assertOOPIFCount(browser, 1);
  const oopif = page.frames()[1];
  expect_eq(oopif.evaluate(() => screen.width), 1280);
  expect_eq(oopif.evaluate(() => screen.height), 720);
  expect_eq(oopif.evaluate(() => matchMedia("(device-width: 1280px)").matches), true);
  expect_eq(oopif.evaluate(() => matchMedia("(device-height: 720px)").matches), true);
  expect_eq(oopif.evaluate(() => "ontouchstart" == window), false);
  page.setViewportSize({ width: 123, height: 456 });
  expect_eq(oopif.evaluate(() => screen.width), 123);
  expect_eq(oopif.evaluate(() => screen.height), 456);
  expect_eq(oopif.evaluate(() => matchMedia("(device-width: 123px)").matches), true);
  expect_eq(oopif.evaluate(() => matchMedia("(device-height: 456px)").matches), true);
  expect_eq(oopif.evaluate(() => "ontouchstart" == window), false);
});










// @see https://github.com/microsoft/playwright/issues/1240













async fn assertOOPIFCount(browser: Browser, count: f64) {
  if (browser.browserType().name() != "chromium") {
    return;
  }
  expect_eq(countOOPIFs(browser), count);
}

async fn countOOPIFs(browser: Browser) {
  const browserSession = browser.newBrowserCDPSession();
  const oopifs = [];
  browserSession.on("Target.targetCreated", (_p) => {
    if (targetInfo.type == "iframe")
      oopifs.push(targetInfo);
  });
  browserSession.send("Target.setDiscoverTargets", { discover: true });
  browserSession.detach();
  return oopifs.length;
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("OO-001 should report oopif frames", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expect_eq(page.frames()[1].evaluate(() => "" + location.href), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] });
    // [ts] test("OO-002 should handle oopif detach", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] const frame = page.frames()[1];
    // [ts] expect_eq(frame.evaluate(() => "" + location.href), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] const [detachedFrame] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("framedetached"),
    // [ts] page.evaluate(() => document.querySelector("iframe").remove()),
    // [ts] ]);
    // [ts] expect_eq(detachedFrame, frame);
    // [ts] });
    // [ts] test("OO-003 should remove workers of a detached oopif", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] attachFrame(page, "frame1", server.CROSS_PROCESS_PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.workers().length, 1);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] worker.waitForEvent("close"),
    // [ts] page.goto(server.PREFIX + "/title.html"),
    // [ts] ]);
    // [ts] expect_eq(page.workers().length, 0);
    // [ts] });
    // [ts] test("OO-004 should not hang in unrouteAll when oopif worker is gone", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42278" });
    // [ts] context.route("**/*", route => route.continue());
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] attachFrame(page, "frame1", server.CROSS_PROCESS_PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/title.html");
    // [ts] context.unrouteAll();
    // [ts] });
    // [ts] test("OO-005 should handle remote -> local -> remote transitions", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames()[1].evaluate(() => "" + location.href), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.frames()[1].waitForNavigation(),
    // [ts] page.evaluate("goLocal()"),
    // [ts] ]);
    // [ts] expect_eq(page.frames()[1].evaluate(() => "" + location.href), server.PREFIX + "/grid.html");
    // [ts] assertOOPIFCount(browser, 0);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.frames()[1].waitForNavigation(),
    // [ts] page.evaluate("goRemote()"),
    // [ts] ]);
    // [ts] expect_eq(page.frames()[1].evaluate(() => "" + location.href), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] });
    // [ts] test("OO-007 should expose function", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] const oopif = page.frames()[1];
    // [ts] page.exposeFunction("mul", (a, b) => a * b);
    // [ts] const result = oopif.evaluate(() => {
    // [ts] return (window)["mul"](9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] });
    // [ts] test("OO-008 should emulate media", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] const oopif = page.frames()[1];
    // [ts] expect_eq(oopif.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), false);
    // [ts] page.emulateMedia({ colorScheme: "dark" });
    // [ts] expect_eq(oopif.evaluate(() => matchMedia("(prefers-color-scheme: dark)").matches), true);
    // [ts] });
    // [ts] test("OO-009 should emulate offline", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] const oopif = page.frames()[1];
    // [ts] expect_eq(oopif.evaluate(() => navigator.onLine), true);
    // [ts] page.context().setOffline(true);
    // [ts] expect_eq(oopif.evaluate(() => navigator.onLine), false);
    // [ts] });
    // [ts] test("OO-010 should support context options", () => {
    // [ts] const iPhone = playwright.devices["iPhone 6"];
    // [ts] const context = browser.newContext({ ...iPhone, timezoneId: "America/Jamaica", locale: "fr-CH", userAgent: "UA" });
    // [ts] const page = context.newPage();
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/grid.html"),
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html"),
    // [ts] ]);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] const oopif = page.frames()[1];
    // [ts] expect_eq(oopif.evaluate(() => "ontouchstart" == window), true);
    // [ts] expect_eq(oopif.evaluate(() => Date.init(1479579154987).toString()), "Sat Nov 19 2016 13:12:34 GMT-0500 (heure normale de lÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂEst nord-amÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ©ricain)");
    // [ts] expect_eq(oopif.evaluate(() => navigator.language), "fr-CH");
    // [ts] expect_eq(oopif.evaluate(() => navigator.userAgent), "UA");
    // [ts] expect_eq(request.headers["user-agent"], "UA");
    // [ts] context.close();
    // [ts] });
    // [ts] test("OO-011 should respect route", () => {
    // [ts] let intercepted = false;
    // [ts] page.route("**/digits/0.png", route => {
    // [ts] intercepted = true;
    // [ts] void route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(intercepted, true);
    // [ts] });
    // [ts] test("OO-012 should take screenshot", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43584" });
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] rafraf(page.frames()[1]);
    // [ts] expect(page.screenshot()).toMatchSnapshot("screenshot-oopif.png");
    // [ts] });
    // [ts] test("OO-013 should load oopif iframes with subresources and route", () => {
    // [ts] page.route("**/*", route => route.continue());
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] });
    // [ts] test("OO-014 should report main requests", () => {
    // [ts] const requestFrames: []Frame = [];
    // [ts] page.on("request", r => requestFrames.push(r.frame()));
    // [ts] const finishedFrames: []Frame = [];
    // [ts] page.on("requestfinished", r => finishedFrames.push(r.frame()));
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const main = page.mainFrame();
    // [ts] main.evaluate(url => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = url;
    // [ts] document.body.appendChild(iframe);
    // [ts] return Promise.init(f => iframe.onload = f);
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] const child = main.childFrames()[0];
    // [ts] child.waitForLoadState("domcontentloaded");
    // [ts] child.evaluate(url => {
    // [ts] const iframe = document.createElement("iframe");
    // [ts] iframe.src = url;
    // [ts] document.body.appendChild(iframe);
    // [ts] return Promise.init(f => iframe.onload = f);
    // [ts] }, server.PREFIX + "/empty.html");
    // [ts] expect_eq(page.frames().length, 3);
    // [ts] const grandChild = child.childFrames()[0];
    // [ts] grandChild.waitForLoadState("domcontentloaded");
    // [ts] assertOOPIFCount(browser, 2);
    // [ts] expect_eq(requestFrames[0], main);
    // [ts] expect_eq(finishedFrames[0], main);
    // [ts] expect_eq(requestFrames[1], child);
    // [ts] expect_eq(finishedFrames[1], child);
    // [ts] expect_eq(requestFrames[2], grandChild);
    // [ts] expect_eq(finishedFrames[2], grandChild);
    // [ts] });
    // [ts] test("OO-015 should support exposeFunction", () => {
    // [ts] page.context().exposeFunction("dec", (a) => a - 1);
    // [ts] page.exposeFunction("inc", (a) => a + 1);
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expect_eq(page.frames()[0].evaluate(() => (window)["inc"](3)), 4);
    // [ts] expect_eq(page.frames()[1].evaluate(() => (window)["inc"](4)), 5);
    // [ts] expect_eq(page.frames()[0].evaluate(() => (window)["dec"](3)), 2);
    // [ts] expect_eq(page.frames()[1].evaluate(() => (window)["dec"](4)), 3);
    // [ts] });
    // [ts] test("OO-016 should support addInitScript", () => {
    // [ts] page.context().addInitScript(() => (window)["bar"] = 17);
    // [ts] page.addInitScript(() => (window)["foo"] = 42);
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expect_eq(page.frames()[0].evaluate(() => (window)["foo"]), 42);
    // [ts] expect_eq(page.frames()[1].evaluate(() => (window)["foo"]), 42);
    // [ts] expect_eq(page.frames()[0].evaluate(() => (window)["bar"]), 17);
    // [ts] expect_eq(page.frames()[1].evaluate(() => (window)["bar"]), 17);
    // [ts] });
    // [ts] test("OO-017 should click a button when it overlays oopif", () => {
    // [ts] page.goto(server.PREFIX + "/button-overlay-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] page.click("button");
    // [ts] expect_eq(page.evaluate(() => (window)["BUTTON_CLICKED"]), true);
    // [ts] });
    // [ts] test("OO-018 should report google.com frame with headed", () => {
    // [ts] test(channel == "chromium-headless-shell", "Headless Shell does not support headed mode");
    // @see https://github.com/GoogleChrome/puppeteer/issues/2548
    // https://google.com is isolated by default in Chromium embedder.
    // [ts] const browser = browserType.launch({ headless: false });
    // [ts] const page = browser.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/*", route => {
    // [ts] void route.fulfill({ body: "YO, GOOGLE.COM" });
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.setAttribute("src", "https://google.com/");
    // [ts] document.body.appendChild(frame);
    // [ts] return Promise.init(x => frame.onload = x);
    // [ts] });
    // [ts] page.waitForSelector("iframe[src=\"https://google.com/\"]");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] const urls = page.frames().map(frame => frame.url());
    // [ts] expect(urls).toEqual([
    // [ts] server.EMPTY_PAGE,
    // [ts] "https://google.com/"
    // [ts] ]);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("OO-019 ElementHandle.boundingBox() should work", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', iframe => {
    // [ts-skip] iframe.style.width = '520px';
    // [ts-skip] iframe.style.height = '520px';
    // [ts-skip] iframe.style.marginLeft = '42px';
    // [ts-skip] iframe.style.marginTop = '17px';
    // [ts-skip] });
    // [ts] page.frames()[1].goto(page.frames()[1].url());
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] const handle1 = page.frames()[1].$(".box:nth-of-type(13)");
    // [ts] expect.poll(() => handle1.boundingBox()).toEqual({ x: 100 + 42, y: 50 + 17, width: 50, height: 50 });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.frames()[1].waitForNavigation(),
    // [ts] page.evaluate("goLocal()"),
    // [ts] ]);
    // [ts] assertOOPIFCount(browser, 0);
    // [ts] const handle2 = page.frames()[1].$(".box:nth-of-type(13)");
    // [ts] expect.poll(() => handle2.boundingBox()).toEqual({ x: 100 + 42, y: 50 + 17, width: 50, height: 50 });
    // [ts] });
    // [ts] test("OO-020 should click", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', iframe => {
    // [ts-skip] iframe.style.width = '500px';
    // [ts-skip] iframe.style.height = '500px';
    // [ts-skip] iframe.style.marginLeft = '102px';
    // [ts-skip] iframe.style.marginTop = '117px';
    // [ts-skip] });
    // [ts] page.frames()[1].goto(page.frames()[1].url());
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] const handle1 = (page.frames()[1].$(".box:nth-of-type(13)"));
    // [ts] handle1.evaluate(div => div.addEventListener("click", () => (window)["_clicked"] = true, false));
    // [ts] handle1.click();
    // [ts] expect_eq(handle1.evaluate(() => (window)["_clicked"]), true);
    // [ts] });
    // [ts] test("OO-021 contentFrame should work", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.locator("iframe").contentFrame().locator("div").count(), 200);
    // [ts] const oopif = page.$("iframe");
    // [ts] const content = oopif.contentFrame();
    // [ts] expect_eq(content.locator("div").count(), 200);
    // [ts] });
    // [ts] test("OO-022 should allow cdp sessions on oopifs", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] expect_eq(page.frames()[1].evaluate(() => "" + location.href), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] const parentCDP = page.context().newCDPSession(page.frames()[0]);
    // [ts] const parent = parentCDP.send("DOM.getDocument", { pierce: true, depth: -1 });
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(parent), "./digits/1.png") == null, true);
    // [ts] const oopifCDP = page.context().newCDPSession(page.frames()[1]);
    // [ts] const oopif = oopifCDP.send("DOM.getDocument", { pierce: true, depth: -1 });
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(oopif), "./digits/1.png") != null, true);
    // [ts] });
    // [ts] test("OO-023 should emit filechooser event for iframe", () => {
    // Add listener before OOPIF is created.
    // [ts] const chooserPromise = page.waitForEvent("filechooser");
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] const frame = page.frames()[1];
    // [ts] frame.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] chooserPromise,
    // [ts] frame.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("OO-024 should be able to click in iframe", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28023" });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] const frame = page.frames()[1];
    // [ts] frame.setContent(`<button onclick="console.log('clicked')">OK</button>`);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] frame.click("button"),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "clicked");
    // [ts] });
    // [ts] test("OO-025 should not throw on exposeFunction when oopif detaches", () => {
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.exposeFunction("myFunc", () => 2022),
    // [ts] page.evaluate(() => document.querySelector("iframe").remove()),
    // [ts] ]);
    // [ts] expect_eq(page.evaluate(() => (window).myFunc()), 2022);
    // [ts] });
    // [ts] test("OO-026 should intercept response body from oopif", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20809" });
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse("**/grid.html"),
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html")
    // [ts] ]);
    // [ts] expect_eq(response.text(), true);
    // [ts] });
    // [ts] test("OO-027 should navigate a local frame to oopif with frame.goto", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] assertOOPIFCount(browser, 0);
    // [ts] const frame = page.frames()[1];
    // [ts] expect_eq(frame.url(), server.PREFIX + "/frames/frame.html");
    // [ts] const navigations: []string = [];
    // [ts] page.on("framenavigated", f => navigations.push(f.url()));
    // [ts] const response = frame.goto(server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(response.url(), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] expect_eq(response.frame(), frame);
    // [ts] expect_eq(frame.url(), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] expect_eq(frame.evaluate(() => "" + location.href), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] expect_eq(navigations, [server.CROSS_PROCESS_PREFIX + "/grid.html"]);
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] });
    // [ts] test("OO-028 should allow to re-connect to OOPIFs with CDP when iframes were there already", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/36095" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/17656" });
    // [ts] test(!!process.env.PWTEST_CHANNEL, "Test default channel only");
    // [ts] const cdpPort = 10123 + it.info().parallelIndex * 4;
    // [ts] const hostBrowser = browserType.launch({ channel: "chromium", args: ["--remote-debugging-port=" + cdpPort] });
    // [ts] const hostPage = hostBrowser.newPage();
    // [ts] hostPage.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] const browser = browserType.connectOverCDP(`http://localhost:${cdpPort}`);
    // [ts] const page = browser.contexts()[0].pages()[0];
    // [ts] expect_eq(page.frames().length, 2);
    // [ts] assertOOPIFCount(browser, 1);
    // [ts] expect_eq(page.frames()[1].evaluate(() => "" + location.href), server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] browser.close();
    // [ts] hostBrowser.close();
    // [ts] });
    // [ts] test("OO-029 should report the whole frame tree when connecting over CDP to nested OOPIFs", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42955" });
    // [ts] test(!!process.env.PWTEST_CHANNEL, "Test default channel only");
    // top > cross-site iframe (oopif) > same-process child iframe > sandboxed srcdoc iframe (oopif).
    // [ts] server.setRoute("/nested-top.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<h1 id=top>TOP</h1><iframe src="${server.CROSS_PROCESS_PREFIX}/nested-b.html"></iframe>`);
    // [ts] });
    // [ts] server.setRoute("/nested-b.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<iframe src="/nested-c.html"></iframe>`);
    // [ts] });
    // [ts] server.setRoute("/nested-c.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<iframe sandbox="allow-scripts" srcdoc="<p>D</p>"></iframe>`);
    // [ts] });
    // [ts] const cdpPort = 10123 + it.info().parallelIndex * 4;
    // [ts] const hostBrowser = browserType.launch({
    // [ts] channel: "chromium",
    // [ts] args: ["--remote-debugging-port=" + cdpPort, "--site-per-process", "--enable-features=IsolateSandboxedIframes"],
    // [ts] });
    // [ts] const hostPage = hostBrowser.newPage();
    // [ts] hostPage.goto(server.PREFIX + "/nested-top.html");
    // [ts] const browser = browserType.connectOverCDP(`http://localhost:${cdpPort}`);
    // [ts] assertOOPIFCount(browser, 2);
    // [ts] const page = browser.contexts()[0].pages()[0];
    // [ts] const expectedTree = [
    // [ts] server.PREFIX + "/nested-top.html",
    // [ts] "    " + server.CROSS_PROCESS_PREFIX + "/nested-b.html",
    // [ts] "        " + server.CROSS_PROCESS_PREFIX + "/nested-c.html",
    // [ts] "            about:srcdoc",
    // [ts] ];
    // [ts] const dumpFrames = (frame, indent) => {
    // [ts] expect_eq(frame.evaluate(() => "" + location.href), frame.url());
    // [ts] const result = [indent + frame.url()];
    // [ts] for (const child of frame.childFrames())
    // [ts] result.push(...dumpFrames(child, indent + "    "));
    // [ts] return result;
    // [ts] };
    // [ts] expect_eq(dumpFrames(page.mainFrame()), expectedTree);
    // [ts] expect_eq(page.frames().length, 4);
    // [ts] expect_eq(page.url(), server.PREFIX + "/nested-top.html");
    // [ts] expect_eq(page.evaluate(() => window.top == window), true);
    // [ts] expect(page.locator("#top")).toHaveText("TOP");
    // [ts] expect(page.frameLocator("iframe").frameLocator("iframe").frameLocator("iframe").locator("p")).toHaveText("D");
    // [ts] browser.close();
    // [ts] hostBrowser.close();
    // [ts] });
}
