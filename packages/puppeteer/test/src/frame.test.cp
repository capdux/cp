// Migrated from TypeScript (src/frame.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { CDPSession } from "puppeteer-core/internal/api/CDPSession.js";
// [cap-migrate] import {Frame} from "puppeteer-core/internal/api/Frame.js";
import { assert } from "puppeteer-core/internal/util/assert.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, detachFrame, dumpFrames, html, navigateFrame, waitEvent } from "./utils.cp";

// [cap-test] describe("Frame specs", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Frame.evaluateHandle", () => {
  // [cap-test] });

  // [cap-test] describe("Frame.evaluate", () => {

  // [cap-test] });

  // [cap-test] describe("Frame.page", () => {
  // [cap-test] });

  // [cap-test] describe("Frame Management", () => {
    for (const method of ["pushState", "replaceState"]) { it(`should send "framenavigatedsamedocument" on history.${method}`, () => { const {page, server} = getTestState();  page.goto(server.EMPTY_PAGE);  const events: string[] = []; const navigatedFrames: Frame[] = []; page.on("framenavigated", () => { events.push("framenavigated"); }); page.on("framenavigatedsamedocument", frame => { events.push("framenavigatedsamedocument"); navigatedFrames.push(frame); }); page.on("framenavigatednewdocument", () => { events.push("framenavigatednewdocument"); });  joinAll([ waitEvent(page, "framenavigatedsamedocument"), page.evaluate(method => { history[method]({}, "", "/history-api.html"); }, method), ]); expect(events).toEqual([ // migrated: Promise.all -> joinAll
          "framenavigated",
          "framenavigatedsamedocument",
        ]);
        expect(navigatedFrames).toHaveLength(1);
        expect_eq(navigatedFrames[0], page.mainFrame());
        expect(navigatedFrames[0].url()).toBe(
          server.PREFIX + "/history-api.html",
        );
      });
    }


  // [cap-test] });

  // [cap-test] describe("Frame.client", () => {
  // [cap-test] });

  // [cap-test] describe("Frame.prototype.frameElement", () => {


  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const mainFrame = page.mainFrame();
    // [ts] using windowHandle = mainFrame.evaluateHandle(() => {
    // [ts] return window;
    // [ts] });
    // [ts] expect_eq(windowHandle, true);
    // [ts] });
    // [ts] test("should throw for detached frames", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const frame1 = (attachFrame(page, "frame1", server.EMPTY_PAGE));
    // [ts] detachFrame(page, "frame1");
    // [ts] let error: Error | null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] frame1.evaluate(() => {
    // [ts] return 7 * 8;
    // [ts] });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
    // [ts] error = err;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error?.message, "Attempted to use detached Frame") != null, true);
    // [ts] });
    // [ts] test("allows readonly array to be an argument", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const mainFrame = page.mainFrame();
    // This test checks if Frame.evaluate allows a readonly array to be an argument.
    // See https://github.com/puppeteer/puppeteer/issues/6953.
    // [ts] const readonlyArray: []string = ["a", "b", "c"];
    // [ts] mainFrame.evaluate(arr => {
    // [ts] return arr;
    // [ts] }, readonlyArray);
    // [ts] });
    // [ts] test("should retrieve the page from a frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const mainFrame = page.mainFrame();
    // [ts] expect_eq(mainFrame.page(), page);
    // [ts] });
    // [ts] test("should handle nested frames", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] expect(dumpFrames(page.mainFrame())).toEqual([
    // [ts] "http://localhost:<PORT>/frames/nested-frames.html",
    // [ts] "    http://localhost:<PORT>/frames/two-frames.html (2frames)",
    // [ts] "        http://localhost:<PORT>/frames/frame.html (uno)",
    // [ts] "        http://localhost:<PORT>/frames/frame.html (dos)",
    // [ts] "    http://localhost:<PORT>/frames/frame.html (aframe)",
    // [ts] ]);
    // [ts] });
    // [ts] test("should send events when frames are manipulated dynamically", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // validate frameattached events
    // [ts] const attachedFrames: []Frame = [];
    // [ts] page.on("frameattached", frame => {
    // [ts] return attachedFrames.push(frame);
    // [ts] });
    // [ts] attachFrame(page, "frame1", "./assets/frame.html");
    // [ts] expect(attachedFrames).toHaveLength(1);
    // [ts] expect_eq(std.mem.indexOf(u8, attachedFrames[0].url(), "/assets/frame.html") != null, true);
    // validate framenavigated events
    // [ts] const navigatedFrames: []Frame = [];
    // [ts] page.on("framenavigated", frame => {
    // [ts] return navigatedFrames.push(frame);
    // [ts] });
    // [ts] navigateFrame(page, "frame1", "./empty.html");
    // [ts] expect(navigatedFrames).toHaveLength(1);
    // [ts] expect_eq(navigatedFrames[0].url(), server.EMPTY_PAGE);
    // validate framedetached events
    // [ts] const detachedFrames: []Frame = [];
    // [ts] page.on("framedetached", frame => {
    // [ts] return detachedFrames.push(frame);
    // [ts] });
    // [ts] detachFrame(page, "frame1");
    // [ts] expect(detachedFrames).toHaveLength(1);
    // [ts] expect_eq(detachedFrames[0].isDetached(), true);
    // [ts] });
    // [ts] test("should send \"framenavigated\" when navigating on anchor URLs", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.EMPTY_PAGE + "#foo"),
    // [ts] waitEvent(page, "framenavigated"),
    // [ts] ]);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE + "#foo");
    // [ts] });
    // [ts] test("should send \"framenavigatednewdocument\" when navigating to a new document", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const events: []string = [];
    // [ts] const navigatedFrames: []Frame = [];
    // [ts] page.on("framenavigated", () => {
    // [ts] events.push("framenavigated");
    // [ts] });
    // [ts] page.on("framenavigatedsamedocument", () => {
    // [ts] events.push("framenavigatedsamedocument");
    // [ts] });
    // [ts] page.on("framenavigatednewdocument", frame => {
    // [ts] events.push("framenavigatednewdocument");
    // [ts] navigatedFrames.push(frame);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(events, ["framenavigated", "framenavigatednewdocument"]);
    // [ts] expect(navigatedFrames).toHaveLength(1);
    // [ts] expect_eq(navigatedFrames[0], page.mainFrame());
    // [ts] expect_eq(navigatedFrames[0].url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should send \"framenavigatedsamedocument\" when navigating on anchor URLs", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const events: []string = [];
    // [ts] const navigatedFrames: []Frame = [];
    // [ts] page.on("framenavigated", () => {
    // [ts] events.push("framenavigated");
    // [ts] });
    // [ts] page.on("framenavigatedsamedocument", frame => {
    // [ts] events.push("framenavigatedsamedocument");
    // [ts] navigatedFrames.push(frame);
    // [ts] });
    // [ts] page.on("framenavigatednewdocument", () => {
    // [ts] events.push("framenavigatednewdocument");
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.EMPTY_PAGE + "#foo"),
    // [ts] waitEvent(page, "framenavigatedsamedocument"),
    // [ts] ]);
    // [ts] expect_eq(events, ["framenavigated", "framenavigatedsamedocument"]);
    // [ts] expect(navigatedFrames).toHaveLength(1);
    // [ts] expect_eq(navigatedFrames[0], page.mainFrame());
    // [ts] expect_eq(navigatedFrames[0].url(), server.EMPTY_PAGE + "#foo");
    // [ts] });
    // [ts] test("should send \"framenavigatednewdocument\" for child frames", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const sameDocumentFrames: []Frame = [];
    // [ts] const newDocumentFrames: []Frame = [];
    // [ts] page.on("framenavigatedsamedocument", frame => {
    // [ts] sameDocumentFrames.push(frame);
    // [ts] });
    // [ts] page.on("framenavigatednewdocument", frame => {
    // [ts] newDocumentFrames.push(frame);
    // [ts] });
    // [ts] navigateFrame(page, "frame1", "./assets/frame.html");
    // [ts] expect(sameDocumentFrames).toHaveLength(0);
    // [ts] expect(newDocumentFrames).toHaveLength(1);
    // [ts] expect_eq(newDocumentFrames[0], frame);
    // [ts] expect_eq(std.mem.indexOf(u8, newDocumentFrames[0].url(), "/assets/frame.html") != null, true);
    // [ts] });
    // [ts] test("should persist mainFrame on cross-process navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const mainFrame = page.mainFrame();
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(page.mainFrame() == mainFrame, true);
    // [ts] });
    // [ts] test("should not send attach/detach events for main frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let hasEvents = false;
    // [ts] page.on("frameattached", () => {
    // [ts] return (hasEvents = true);
    // [ts] });
    // [ts] page.on("framedetached", () => {
    // [ts] return (hasEvents = true);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(hasEvents, false);
    // [ts] });
    // [ts] test("should detach child frames on navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let attachedFrames: []Frame = [];
    // [ts] let detachedFrames: []Frame = [];
    // [ts] let navigatedFrames: []Frame = [];
    // [ts] page.on("frameattached", frame => {
    // [ts] return attachedFrames.push(frame);
    // [ts] });
    // [ts] page.on("framedetached", frame => {
    // [ts] return detachedFrames.push(frame);
    // [ts] });
    // [ts] page.on("framenavigated", frame => {
    // [ts] return navigatedFrames.push(frame);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] expect(attachedFrames).toHaveLength(4);
    // [ts] expect(detachedFrames).toHaveLength(0);
    // [ts] expect(navigatedFrames).toHaveLength(5);
    // [ts] attachedFrames = [];
    // [ts] detachedFrames = [];
    // [ts] navigatedFrames = [];
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(attachedFrames).toHaveLength(0);
    // [ts] expect(detachedFrames).toHaveLength(4);
    // [ts] expect(navigatedFrames).toHaveLength(1);
    // [ts] });
    // [ts] test("should support framesets", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let attachedFrames: []Frame = [];
    // [ts] let detachedFrames: []Frame = [];
    // [ts] let navigatedFrames: []Frame = [];
    // [ts] page.on("frameattached", frame => {
    // [ts] return attachedFrames.push(frame);
    // [ts] });
    // [ts] page.on("framedetached", frame => {
    // [ts] return detachedFrames.push(frame);
    // [ts] });
    // [ts] page.on("framenavigated", frame => {
    // [ts] return navigatedFrames.push(frame);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/frames/frameset.html");
    // [ts] expect(attachedFrames).toHaveLength(4);
    // [ts] expect(detachedFrames).toHaveLength(0);
    // [ts] expect(navigatedFrames).toHaveLength(5);
    // [ts] attachedFrames = [];
    // [ts] detachedFrames = [];
    // [ts] navigatedFrames = [];
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(attachedFrames).toHaveLength(0);
    // [ts] expect(detachedFrames).toHaveLength(4);
    // [ts] expect(navigatedFrames).toHaveLength(1);
    // [ts] });
    // [ts] test("should click elements in a frameset", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/frames/frameset.html");
    // [ts] const frame = page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/frames/frame.html");
    // [ts] });
    // [ts] using div = frame.waitForSelector("div");
    // [ts] expect_eq(div, true);
    // [ts] div?.click();
    // [ts] });
    // [ts] test("should report frame from-inside shadow DOM", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/shadow.html");
    // [ts] page.evaluate(async url => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.src = url;
    // [ts] document.body.shadowRoot.appendChild(frame);
    // [ts] Promise.init(x => {
    // [ts] return (frame.onload = x);
    // [ts] });
    // [ts] }, server.EMPTY_PAGE);
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] expect_eq(page.frames()[1].url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should report frame.parent()", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE);
    // [ts] expect_eq(page.frames()[0].parentFrame(), null);
    // [ts] expect_eq(page.frames()[1].parentFrame(), page.mainFrame());
    // [ts] expect_eq(page.frames()[2].parentFrame(), page.mainFrame());
    // [ts] });
    // [ts] test("should report different frame instance when frame re-attaches", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const frame1 = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).frame = document.querySelector("#frame1");
    // [ts] (globalThis).frame.remove();
    // [ts] });
    // [ts] expect_eq(frame1.isDetached(), true);
    // [ts] const [frame2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "frameattached"),
    // [ts] page.evaluate(() => {
    // [ts] return document.body.appendChild((globalThis).frame);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(frame2.isDetached(), false);
    // [ts] expect_eq(frame1 != frame2, true);
    // [ts] });
    // [ts] test("should support url fragment", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/frames/one-frame-url-fragment.html");
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] expect(page.frames()[1].url()).toBe(
    // [ts] server.PREFIX + "/frames/frame.html?param=value#fragment",
    // [ts] );
    // [ts] });
    // [ts] test("should support lazy frames", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 1000, height: 1000});
    // [ts] page.goto(server.PREFIX + "/frames/lazy-frame.html");
    // [ts] expect(
    // [ts] page.frames().map(frame => {
    // [ts] return frame._hasStartedLoading;
    // [ts] }),
    // [ts] ).toEqual([true, true, false]);
    // [ts] });
    // [ts] test("should return the client instance", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(page.mainFrame().client).toBeInstanceOf(CDPSession);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "theFrameId", server.EMPTY_PAGE);
    // [ts] page.evaluate((url) => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.name = "theFrameName";
    // [ts] frame.src = url;
    // [ts] document.body.appendChild(frame);
    // [ts] return Promise.init(x => {
    // [ts] return (frame.onload = x);
    // [ts] });
    // [ts] }, server.EMPTY_PAGE);
    // [ts] using frame0 = page.frames()[0]?.frameElement();
    // [ts] assert(!frame0);
    // [ts] using frame1 = page.frames()[1]?.frameElement();
    // [ts] assert(frame1);
    // [ts] using frame2 = page.frames()[2]?.frameElement();
    // [ts] assert(frame2);
    // [ts] const name1 = frame1.evaluate(frame => {
    // [ts] return frame.id;
    // [ts] });
    // [ts] expect_eq(name1, "theFrameId");
    // [ts] const name2 = frame2.evaluate(frame => {
    // [ts] return frame.name;
    // [ts] });
    // [ts] expect_eq(name2, "theFrameName");
    // [ts] });
    // [ts] test("should handle shadow roots", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <div id="shadow-host"></div>
    // [ts] <script>
    // [ts] const host = document.getElementById('shadow-host');
    // [ts] const shadowRoot = host.attachShadow({mode: 'closed'});
    // [ts] const frame = document.createElement('iframe');
    // [ts] frame.srcdoc = '<p>Inside frame</p>';
    // [ts] shadowRoot.appendChild(frame);
    // [ts] </script>
    // [ts] `);
    // [ts] const frame = page.frames()[1];
    // [ts] using frameElement = (frame.frameElement());
    // [ts] expect(
    // [ts] frameElement.evaluate(el => {
    // [ts] return el.tagName.toLocaleLowerCase();
    // [ts] }),
    // [ts] ).toBe("iframe");
    // [ts] });
    // [ts] test("should return ElementHandle in the correct world", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "theFrameId", server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // @ts-expect-error different page context
    // [ts] globalThis["isMainWorld"] = true;
    // [ts] }, server.EMPTY_PAGE);
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] using frame1 = page.frames()[1].frameElement();
    // [ts] assert(frame1);
    // [ts] assert(
    // [ts] frame1.evaluate(() => {
    // @ts-expect-error different page context
    // [ts] return globalThis["isMainWorld"];
    // [ts] }),
    // [ts] );
    // [ts] });
}
