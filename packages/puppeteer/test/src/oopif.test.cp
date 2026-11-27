// Migrated from TypeScript (src/oopif.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {CDPSession} from "puppeteer-core/internal/api/CDPSession.js";
import { CDPSessionEvent } from "puppeteer-core/internal/api/CDPSession.js";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {WebWorker} from "puppeteer-core/internal/api/WebWorker.js";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";

import { setupSeparateTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, detachFrame, dumpFrames, navigateFrame, waitEvent } from "./utils.cp";

// [cap-test] describe("OOPIF", () => {
  // We start a new browser instance for this test because we need the
  // --site-per-process flag.
  const state = setupSeparateTestBrowserHooks(
    {
      args: ["--site-per-process"],
    },
    {createContext: true},
  );






















  // [cap-test] describe("waitForFrame", () => {
  // [cap-test] });






// [cap-test] });

async fn iframes(page: Page) {
  const iframes = joinAll( // migrated: Promise.all -> joinAll
    page.frames().map(async frame => {
      return frame.frameElement();
    }),
  );
  return iframes.filter(frame => {
    return frame != null;
  });
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should treat OOP iframes and normal iframes the same", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/empty.html");
    // [ts] });
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame2",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] framePromise;
    // [ts] expect(page.mainFrame().childFrames()).toHaveLength(2);
    // [ts] });
    // [ts] test("should track navigations within OOP iframes", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const frame = framePromise;
    // [ts] expect_eq(std.mem.indexOf(u8, frame.url(), "/empty.html") != null, true);
    // [ts] navigateFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/assets/frame.html",
    // [ts] );
    // [ts] expect_eq(std.mem.indexOf(u8, frame.url(), "/assets/frame.html") != null, true);
    // [ts] });
    // [ts] test("should support OOP iframes becoming normal iframes again", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] framePromise;
    // [ts] navigateFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] navigateFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] });
    // [ts] test("should evaluate in OOP iframes that become normal iframes again", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = framePromise;
    // [ts] const crossProcessUrl = server.CROSS_PROCESS_PREFIX + "/empty.html";
    // [ts] navigateFrame(page, "frame1", crossProcessUrl);
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return location.href;
    // [ts] }),
    // [ts] ).toBe(crossProcessUrl);
    // [ts] navigateFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect_eq(frame.url(), server.EMPTY_PAGE);
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return location.href;
    // [ts] }),
    // [ts] ).toBe(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should evaluate in OOP iframes that navigate themselves to become normal iframes again", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = framePromise;
    // [ts] navigateFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame.waitForNavigation(),
    // [ts] frame.evaluate(url => {
    // [ts] setTimeout(() => {
    // [ts] location.assign(url);
    // [ts] }, 0);
    // [ts] }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(frame.url(), server.EMPTY_PAGE);
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return location.href;
    // [ts] }),
    // [ts] ).toBe(server.EMPTY_PAGE);
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] });
    // [ts] test("should support frames within OOP frames", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame1Promise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] const frame2Promise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 2;
    // [ts] });
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/frames/one-frame.html",
    // [ts] );
    // [ts] const [frame1, frame2] = joinAll([frame1Promise, frame2Promise]); // migrated: Promise.all -> joinAll
    // [ts] expect(
    // [ts] frame1.evaluate(() => {
    // [ts] return document.location.href;
    // [ts] }),
    // [ts] ).toMatch("one-frame\\.html$");
    // [ts] expect(
    // [ts] frame2.evaluate(() => {
    // [ts] return document.location.href;
    // [ts] }),
    // [ts] ).toMatch("frames\\/frame\\.html$");
    // [ts] });
    // [ts] test("should recover cross-origin frames on reconnect", () => {
    // [ts] const {server, page, puppeteer, browser} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame1Promise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] const frame2Promise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 2;
    // [ts] });
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/frames/one-frame.html",
    // [ts] );
    // [ts] joinAll([frame1Promise, frame2Promise]); // migrated: Promise.all -> joinAll
    // [ts] const dump1 = dumpFrames(page.mainFrame());
    // [ts] using browserTwo = puppeteer.connect({
    // [ts] browserWSEndpoint: browser.wsEndpoint(),
    // [ts] protocol: browser.protocol,
    // [ts] });
    // [ts] const pages = browserTwo.pages();
    // [ts] const emptyPages = pages.filter(page => {
    // [ts] return page.url() == server.EMPTY_PAGE;
    // [ts] });
    // [ts] expect_eq(emptyPages.length, 1);
    // [ts] const dump2 = dumpFrames(emptyPages[0].mainFrame());
    // [ts] expect_eq(dump1, dump2);
    // [ts] });
    // [ts] test("should support OOP iframes getting detached", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] framePromise;
    // [ts] navigateFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] detachFrame(page, "frame1");
    // [ts] expect(page.frames()).toHaveLength(1);
    // [ts] });
    // [ts] test("should support wait for navigation for transitions from local to OOPIF", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = framePromise;
    // [ts] const nav = frame.waitForNavigation();
    // [ts] navigateFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] nav;
    // [ts] detachFrame(page, "frame1");
    // [ts] expect(page.frames()).toHaveLength(1);
    // [ts] });
    // [ts] test("should keep track of a frames OOP state", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const frame = framePromise;
    // [ts] expect_eq(std.mem.indexOf(u8, frame.url(), "/empty.html") != null, true);
    // [ts] navigateFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect_eq(frame.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should support evaluating in oop iframes", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const frame = framePromise;
    // [ts] frame.evaluate(() => {
    // [ts] (window)._test = "Test 123!";
    // [ts] });
    // [ts] const result = frame.evaluate(() => {
    // [ts] return (window)._test;
    // [ts] });
    // [ts] expect_eq(result, "Test 123!");
    // [ts] });
    // [ts] test("should provide access to elements", () => {
    // [ts] const {server, isHeadless, headless, page} = state;
    // [ts] if (!isHeadless || headless == "true") {
    // TODO: this test is partially blocked on crbug.com/1334119. Enable test once
    // the upstream is fixed.
    // TLDR: when we dispatch events to the frame the compositor might
    // not be up-to-date yet resulting in a misclick (the iframe element
    // becomes the event target instead of the content inside the iframe).
    // The solution is to use InsertVisualCallback on the backend but that causes
    // another issue that events cannot be dispatched to inactive tabs as the
    // visual callback is never invoked.
    // The old headless mode does not have this issue since it operates with
    // special scheduling settings that keep even inactive tabs updating.
    // [ts] return;
    // [ts] }
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const frame = framePromise;
    // [ts] frame.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.id = "test-button";
    // [ts] button.innerText = "click";
    // [ts] button.onclick = () => {
    // [ts] button.id = "clicked";
    // [ts] };
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.border = "150px solid black";
    // [ts] document.body.style.margin = "250px";
    // [ts] document.body.style.padding = "50px";
    // [ts] });
    // [ts] frame.waitForSelector("#test-button", {visible: true});
    // The page layout change moves the OOP iframe. The browser routes input
    // with hit-test data from the last compositor frame. Wait for two
    // animation frames in the main frame so that the browser has hit-test
    // data for the new iframe position. Otherwise, the click can go to the
    // <iframe> element in the main frame instead of the button.
    // [ts] page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] requestAnimationFrame(() => {
    // [ts] requestAnimationFrame(resolve);
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] frame.click("#test-button");
    // [ts] frame.waitForSelector("#clicked");
    // [ts] });
    // [ts] test("should report oopif frames", () => {
    // [ts] const {server, page} = state;
    // [ts] const frame = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] frame;
    // [ts] expect(iframes(page)).toHaveLength(1);
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] });
    // [ts] test("should wait for inner OOPIFs", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(`http://domain1.test:${server.PORT}/main-frame.html`);
    // [ts] const frame2 = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("inner-frame2.html");
    // [ts] });
    // [ts] expect(iframes(page)).toHaveLength(2);
    // [ts] expect(page.frames()).toHaveLength(3);
    // [ts] expect(
    // [ts] frame2.evaluate(() => {
    // [ts] return document.querySelectorAll("button").length;
    // [ts] }),
    // [ts] ).toStrictEqual(1);
    // [ts] });
    // [ts] test("should load oopif iframes with subresources and request interception", () => {
    // [ts] const {server, page} = state;
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] const requestPromise = page.waitForRequest(request => {
    // [ts] return request.url().includes("requestFromOOPIF");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] const frame = framePromise;
    // [ts] const request = requestPromise;
    // [ts] expect(iframes(page)).toHaveLength(1);
    // [ts] expect_eq(request.frame(), frame);
    // [ts] });
    // [ts] test("should support frames within OOP iframes", () => {
    // [ts] const {server, page} = state;
    // [ts] const oopIframePromise = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] const oopIframe = oopIframePromise;
    // [ts] attachFrame(
    // [ts] oopIframe,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const frame1 = oopIframe.childFrames()[0];
    // [ts] expect(frame1.url()).toMatch("empty.html$");
    // [ts] navigateFrame(
    // [ts] oopIframe,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/oopif.html",
    // [ts] );
    // [ts] expect(frame1.url()).toMatch("oopif.html$");
    // [ts] frame1.goto(
    // [ts] server.CROSS_PROCESS_PREFIX + "/oopif.html#navigate-within-document",
    // [ts] {waitUntil: "load"},
    // [ts] );
    // [ts] expect(frame1.url()).toMatch("oopif.html#navigate-within-document$");
    // [ts] detachFrame(oopIframe, "frame1");
    // [ts] expect(oopIframe.childFrames()).toHaveLength(0);
    // [ts] });
    // [ts] test("clickablePoint, boundingBox, boxModel should work for elements inside OOPIFs", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const framePromise = page.waitForFrame(frame => {
    // [ts] return page.frames().indexOf(frame) == 1;
    // [ts] });
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const frame = framePromise;
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.border = "50px solid black";
    // [ts] document.body.style.margin = "50px";
    // [ts] document.body.style.padding = "50px";
    // [ts] });
    // [ts] frame.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.id = "test-button";
    // [ts] button.innerText = "click";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] using button = (frame.waitForSelector("#test-button", {
    // [ts] visible: true,
    // [ts] }));
    // [ts] const result = button.clickablePoint();
    // [ts] expect_eq(result.x > 150, true); // padding + margin + border left
    // [ts] expect_eq(result.y > 150, true); // padding + margin + border top
    // [ts] const resultBoxModel = (button.boxModel());
    // [ts] for (const quad of [
    // [ts] resultBoxModel.content,
    // [ts] resultBoxModel.border,
    // [ts] resultBoxModel.margin,
    // [ts] resultBoxModel.padding,
    // [ts] ]) {
    // [ts] for (const part of quad) {
    // [ts] expect_eq(part.x > 150, true); // padding + margin + border left
    // [ts] expect_eq(part.y > 150, true); // padding + margin + border top
    // [ts] }
    // [ts] }
    // [ts] const resultBoundingBox = (button.boundingBox());
    // [ts] expect_eq(resultBoundingBox.x > 150, true); // padding + margin + border left
    // [ts] expect_eq(resultBoundingBox.y > 150, true); // padding + margin + border top
    // [ts] });
    // [ts] test("should detect existing OOPIFs when Puppeteer connects to an existing page", () => {
    // [ts] const {server, puppeteer, browser, page} = state;
    // [ts] const frame = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] frame;
    // [ts] expect(iframes(page)).toHaveLength(1);
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] using browser1 = puppeteer.connect({
    // [ts] browserWSEndpoint: browser.wsEndpoint(),
    // [ts] });
    // [ts] const target = browser1.waitForTarget(target => {
    // [ts] return target.url().endsWith("dynamic-oopif.html");
    // [ts] });
    // [ts] target.page();
    // [ts] browser1.disconnect();
    // [ts] });
    // [ts] test("should support lazy OOP frames", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.PREFIX + "/lazy-oopif-frame.html");
    // [ts] page.setViewport({width: 1000, height: 1000});
    // [ts] expect(
    // [ts] page.frames().map(frame => {
    // [ts] return frame._hasStartedLoading;
    // [ts] }),
    // [ts] ).toEqual([true, true, false]);
    // [ts] });
    // [ts] test("should exposeFunction on a page with a PDF viewer", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/pdf-viewer.html", {
    // [ts] waitUntil: "networkidle2",
    // [ts] });
    // [ts] page.exposeFunction("test", () => {
    // [ts] @print("test");
    // [ts] });
    // [ts] });
    // [ts] test("should exposeFunction when an OOP iframe goes away mid-call", () => {
    // [ts] const {page, server} = state;
    // [ts] const frameCount = 8;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let i = 0;
    // [ts] while (i < frameCount) { // [cap-migrate] for-step: i++
    // [ts] attachFrame(
    // [ts] page,
    // [ts] `oopif${i}`,
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] }
    // [ts] expect(page.frames()).toHaveLength(frameCount + 1);
    // Start the teardown first so it lands while the per-frame calls run.
    // [ts] const detached = page.evaluate(() => {
    // [ts] for (const frame of document.querySelectorAll("iframe")) {
    // [ts] frame.remove();
    // [ts] }
    // [ts] });
    // [ts] const exposed = page.exposeFunction("doubleIt", (value) => {
    // [ts] return value * 2;
    // [ts] });
    // [ts] exposed;
    // [ts] detached;
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (window).doubleIt(21);
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should evaluate on a page with a PDF viewer", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/pdf-viewer.html", {
    // [ts] waitUntil: "networkidle2",
    // [ts] });
    // [ts] expect(
    // [ts] joinAll( // migrated: Promise.all -> joinAll
    // [ts] page.frames().map(async frame => {
    // [ts] return frame.evaluate(() => {
    // [ts] return window.location.pathname;
    // [ts] });
    // [ts] }),
    // [ts] ),
    // [ts] ).toEqual([
    // [ts] "/pdf-viewer.html",
    // [ts] "/sample.pdf",
    // [ts] "/index.html",
    // [ts] "/sample.pdf",
    // [ts] ]);
    // [ts] });
    // [ts] test("should support evaluateOnNewDocument", () => {
    // [ts] const {page, server} = state;
    // [ts] page.evaluateOnNewDocument(() => {
    // [ts] (window).evaluateOnNewDocument = true;
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] for (const frame of page.frames()) {
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return (window).evaluateOnNewDocument;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] }
    // [ts] });
    // [ts] test("should support removing evaluateOnNewDocument scripts", () => {
    // [ts] const {page, server} = state;
    // [ts] const {identifier} = page.evaluateOnNewDocument(() => {
    // [ts] (window).evaluateOnNewDocument = true;
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] for (const frame of page.frames()) {
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return (window).evaluateOnNewDocument;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] }
    // [ts] page.removeScriptToEvaluateOnNewDocument(identifier);
    // [ts] page.reload();
    // [ts] page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] });
    // [ts] test("should support exposeFunction", () => {
    // [ts] const {page, server} = state;
    // [ts] let count = 0;
    // [ts] page.exposeFunction("plusOne", () => {
    // [ts] count++;
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] for (const frame of page.frames()) {
    // [ts] frame.evaluate(() => {
    // @ts-expect-error different context
    // [ts] return window.plusOne();
    // [ts] });
    // [ts] }
    // [ts] expect_eq(count, 2);
    // [ts] });
    // [ts] test("should support removing exposed function", () => {
    // [ts] const {page, server} = state;
    // [ts] page.exposeFunction("plusOne", () => {});
    // [ts] const frame = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/oopif.html");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/dynamic-oopif.html");
    // [ts] frame;
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] page.removeExposedFunction("plusOne");
    // [ts] for (const frame of page.frames()) {
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // @ts-expect-error different context
    // [ts] return !!window["plusOne"];
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] }
    // [ts] });
    // [ts] test("should resolve immediately if the frame already exists", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame2",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/empty.html");
    // [ts] });
    // [ts] });
    // [ts] test("should report google.com frame", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", r => {
    // [ts] return r.respond({body: "YO, GOOGLE.COM"});
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.setAttribute("src", "https://google.com/");
    // [ts] document.body.appendChild(frame);
    // [ts] return Promise.init(x => {
    // [ts] return (frame.onload = x);
    // [ts] });
    // [ts] });
    // [ts] page.waitForSelector("iframe[src=\"https://google.com/\"]");
    // [ts] const urls = page
    // [ts] .frames()
    // [ts] .map(frame => {
    // [ts] return frame.url();
    // [ts] })
    // [ts] .sort();
    // [ts] expect_eq(urls, [server.EMPTY_PAGE, "https://google.com/"]);
    // [ts] });
    // [ts] test("should expose events within OOPIFs", () => {
    // [ts] const {server, page} = state;
    // Setup our session listeners to observe OOPIF activity.
    // [ts] const session = page.createCDPSession();
    // [ts] const networkEvents: []string = [];
    // [ts] const otherSessions: []CDPSession = [];
    // [ts] session.send("Target.setAutoAttach", {
    // [ts] autoAttach: true,
    // [ts] flatten: true,
    // [ts] waitForDebuggerOnStart: true,
    // [ts] });
    // [ts] session.on(CDPSessionEvent.SessionAttached, async session => {
    // [ts] otherSessions.push(session);
    // [ts] session.on("Network.requestWillBeSent", params => {
    // [ts] return networkEvents.push(params.request.url);
    // [ts] });
    // [ts] session.send("Network.enable");
    // [ts] session.send("Runtime.runIfWaitingForDebugger");
    // [ts] });
    // Navigate to the empty page and add an OOPIF iframe with at least one request.
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(
    // [ts] (frameUrl) => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.setAttribute("src", frameUrl);
    // [ts] document.body.appendChild(frame);
    // [ts] return Promise.init((x, y) => {
    // [ts] frame.onload = x;
    // [ts] frame.onerror = y;
    // [ts] });
    // [ts] },
    // [ts] server.PREFIX.replace("localhost", "domain1.test") + "/one-style.html",
    // [ts] );
    // [ts] page.waitForSelector("iframe");
    // Ensure we found the iframe session.
    // [ts] expect(otherSessions).toHaveLength(1);
    // Resume the iframe and trigger another request.
    // [ts] const iframeSession = otherSessions[0];
    // [ts] iframeSession.send("Runtime.evaluate", {
    // [ts] expression: `fetch('/fetch')`,
    // [ts] awaitPromise: true,
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, networkEvents, `http://domain1.test:${server.PORT}/fetch`) != null, true);
    // [ts] });
    // [ts] test("should retrieve body for OOPIF document requests", () => {
    // [ts] const {server, page} = state;
    // [ts] const frameUrl =
    // [ts] server.PREFIX.replace("localhost", "domain1.test") +
    // [ts] "/oopif-response.html";
    // [ts] expect.assertions(1);
    // [ts] let testResponse = null;
    // [ts] page.on("response", async response => {
    // [ts] if (response.request().url() == frameUrl) {
    // [ts] testResponse = response;
    // [ts] }
    // [ts] });
    // Navigate to the empty page and add an OOPIF iframe.
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate((frameUrl) => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.setAttribute("src", frameUrl);
    // [ts] document.body.appendChild(frame);
    // [ts] return Promise.init((x, y) => {
    // [ts] frame.onload = x;
    // [ts] frame.onerror = y;
    // [ts] });
    // [ts] }, frameUrl);
    // [ts] page.waitForSelector("iframe");
    // [ts] expect(testResponse.text()).resolves.toMatch("I'm an OOPIF");
    // [ts] });
    // [ts] test("should report console messages from OOP iframes to the browser and context", () => {
    // [ts] const {server, page, context, browser} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const [browserMessage, contextMessage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "console", message => {
    // [ts] return message.text() == "from-oopif";
    // [ts] }),
    // [ts] waitEvent(context, "console", message => {
    // [ts] return message.text() == "from-oopif";
    // [ts] }),
    // [ts] frame.evaluate(() => {
    // [ts] @print("from-oopif");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(browserMessage, contextMessage);
    // [ts] expect_eq(browserMessage.frame(), frame);
    // [ts] expect_eq(browserMessage.page(), page);
    // [ts] });
    // [ts] test("should report requests from OOP iframes to the browser and context", () => {
    // [ts] const {server, page, context, browser} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const url = server.CROSS_PROCESS_PREFIX + "/one-style.css";
    // [ts] const [browserRequest, contextRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(browser, "request", request => {
    // [ts] return request.url() == url;
    // [ts] }),
    // [ts] waitEvent(context, "request", request => {
    // [ts] return request.url() == url;
    // [ts] }),
    // [ts] frame.evaluate((url) => {
    // [ts] fetch(url, {cache: "no-store"});
    // [ts] }, url),
    // [ts] ]);
    // [ts] expect_eq(browserRequest, contextRequest);
    // [ts] expect_eq(browserRequest.frame(), frame);
    // [ts] expect_eq(browserRequest.page(), page);
    // [ts] });
    // [ts] test("should continue intercepted requests from workers in OOPIFs", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const workerPromise = waitEvent(page, "workercreated");
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/worker/worker.html",
    // [ts] );
    // [ts] const worker = workerPromise;
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] requests.push(request);
    // [ts] void request.continue();
    // [ts] });
    // Requests from a worker in an OOPIF are paused on the OOPIF session, so
    // continuing them on any other session leaves the fetch hanging.
    // [ts] const url = server.CROSS_PROCESS_PREFIX + "/empty.html";
    // [ts] const result = worker.evaluate((url) => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url, {
    // [ts] cache: "no-store",
    // [ts] signal: AbortSignal.timeout(5000),
    // [ts] });
    // [ts] return "ok";
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return "error";
    // [ts] }
    // [ts] }, url);
    // [ts] expect_eq(result, "ok");
    // [ts] const request = requests.find(request => {
    // [ts] return request.url() == url;
    // [ts] });
    // [ts] expect_eq(request?.worker(), worker);
    // [ts] });
}
