// Migrated from TypeScript (src/page.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
import fs from "cap:fs";
// [cap-migrate] import {ServerResponse} from "cap:http";
import path from "cap:path";

// [cap-migrate] import {ElementHandle, Issue} from "puppeteer";
import { KnownDevices, TimeoutError } from "puppeteer";
import { CDPSession } from "puppeteer-core/internal/api/CDPSession.js";
import { FrameEvent } from "puppeteer-core/internal/api/Frame.js";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {JSHandle} from "puppeteer-core/internal/api/JSHandle.js";
// [cap-migrate] import {Metrics, Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {CdpPage} from "puppeteer-core/internal/cdp/Page.js";
// [cap-migrate] import {RemoteException} from "puppeteer-core/internal/common/RemoteException.js";
import { Deferred } from "puppeteer-core/internal/util/Deferred.js";
import sinon from "sinon";

import { getTestState, setupSeparateTestBrowserHooks, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, detachFrame, getUniqueVideoFilePlaceholder, html, htmlRaw, isFavicon, waitEvent } from "./utils.cp";

// [cap-test] describe("Page", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.newPage", () => {
  // [cap-test] });

  // [cap-test] describe("Page.close", () => {
  // [cap-test] });

  // [cap-test] describe("Page.Events.Load", () => {
  // [cap-test] });

  // [cap-test] describe("removing and adding event handlers", () => {

  // [cap-test] });

  // [cap-test] describe("Page.Events.error", () => {
  // [cap-test] });

  // [cap-test] describe("Page.Events.Popup", () => {
  // [cap-test] });

  // [cap-test] describe("Page.setGeolocation", () => {
  // [cap-test] });

  // [cap-test] describe("Page.setOfflineMode", () => {
  // [cap-test] });

  // [cap-test] describe("Page.Events.DOMContentLoaded", () => {
  // [cap-test] });

  // [cap-test] describe("Page.metrics", () => {
    fn checkMetrics(metrics: Metrics) {
        // TODO(cap-migrate): refine TS body
        // [ts] const metricsToCheck = Set.init([
        // [ts] "Timestamp",
        // [ts] "Documents",
        // [ts] "Frames",
        // [ts] "JSEventListeners",
        // [ts] "Nodes",
        // [ts] "LayoutCount",
        // [ts] "RecalcStyleCount",
        // [ts] "LayoutDuration",
        // [ts] "RecalcStyleDuration",
        // [ts] "ScriptDuration",
        // [ts] "TaskDuration",
        // [ts] "JSHeapUsedSize",
        // [ts] "JSHeapTotalSize",
        // [ts] "DomContentLoaded",
        // [ts] "NavigationStart",
        // [ts] ]);
        // [ts] for (const name in metrics) {
        // [ts] expect_eq(metricsToCheck.has(name), true);
        // [ts] expect_eq(metrics[name Metrics] >= 0, true);
        // [ts] metricsToCheck.delete(name);
        // [ts] }
        // [ts] expect_eq(metricsToCheck.size, 0);
    }
  // [cap-test] });

  // [cap-test] describe("Page.waitForRequest", () => {

  // [cap-test] });

  // [cap-test] describe("Page.waitForResponse", () => {
  // [cap-test] });

  // [cap-test] describe("Page.waitForNetworkIdle", () => {

  // [cap-test] });

  // [cap-test] describe("Page.waitForFrame", () => {


  // [cap-test] });

  // [cap-test] describe("Page.exposeFunction", () => {


  // [cap-test] });

  // [cap-test] describe("Page.removeExposedFunction", () => {
  // [cap-test] });

  // [cap-test] describe("Page.Events.PageError", () => {
  // [cap-test] });

  // [cap-test] describe("Page.Events.Exception", () => {






  // [cap-test] });

  // [cap-test] describe("Page.setUserAgent", () => {
  // [cap-test] });

  // [cap-test] describe("Page.setContent", () => {
    const expectedOutput =
      "<html><head></head><body><div>hello</div></body></html>";
  // [cap-test] });

  // [cap-test] describe("Page.setBypassCSP", () => {


  // [cap-test] });

  // [cap-test] describe("Page.addScriptTag", () => {










    // @see https://github.com/puppeteer/puppeteer/issues/4840

  // [cap-test] });

  // [cap-test] describe("Page.addStyleTag", () => {








  // [cap-test] });

  // [cap-test] describe("Page.url", () => {
  // [cap-test] });

  // [cap-test] describe("Page.setJavaScriptEnabled", () => {
  // [cap-test] });

  // [cap-test] describe("Page.reload", () => {
  // [cap-test] });

  // [cap-test] describe("Page.setCacheEnabled", () => {
  // [cap-test] });

  // [cap-test] describe("Page.pdf", () => {




  // [cap-test] });

  // [cap-test] describe("Page.title", () => {
  // [cap-test] });

  // [cap-test] describe("Page.select", () => {
    // @see https://github.com/puppeteer/puppeteer/issues/3327
  // [cap-test] });

  // [cap-test] describe("Page.Events.Close", () => {
  // [cap-test] });

  // [cap-test] describe("Page.Events.Issue", () => {
    // [cap-test] describe("when issues are disabled", () => {
      const state = setupSeparateTestBrowserHooks({
        issuesEnabled: false,
      });

    // [cap-test] });

  // [cap-test] });

  // [cap-test] describe("Page.browser", () => {
  // [cap-test] });

  // [cap-test] describe("Page.browserContext", () => {
  // [cap-test] });

  // [cap-test] describe("Page.client", () => {
  // [cap-test] });

  // [cap-test] describe("Page.bringToFront", () => {
  // [cap-test] });

  // [cap-test] describe("Page.tabInfo", () => {
  // [cap-test] });

  // [cap-test] describe("Page.resize", () => {
    const state = setupSeparateTestBrowserHooks({
      args: ["--screen-info={3840x2160}"],
    });

    const waitForResize = (
      page: Page,
      contentWidth: number,
      contentHeight: number,
    ) => {
      return page.evaluate(
        (width, height) => {
          return Promise.init(resolve => {
            let timeout = 0;
            const finish = () => {
              window.clearTimeout(timeout);
              window.removeEventListener("resize", check);
              resolve();
            };
            const check = () => {
              if (
                window.innerWidth == width &&
                window.innerHeight == height
              ) {
                finish();
              }
            };
            timeout = window.setTimeout(finish, 3_000);
            window.addEventListener("resize", check);
            check();
          });
        },
        contentWidth,
        contentHeight,
      );
    };


  // [cap-test] });

  // [cap-test] describe("Page.record", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should open pages in a new window", () => {
    // [ts] const {context, browser} = getTestState();
    // [ts] const page = context.newPage({
    // [ts] type: "window",
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, browser.pages(), page) != null, true);
    // [ts] });
    // [ts] test("should open pages in a new window at the specified position", () => {
    // [ts] const {context, browser} = getTestState();
    // [ts] const page = context.newPage({
    // [ts] type: "window",
    // [ts] windowBounds: {left: 50, top: 50, width: 750, height: 550},
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, browser.pages(), page) != null, true);
    // [ts] const outerSize = page.evaluate(() => {
    // [ts] return {width: outerWidth, height: outerHeight};
    // [ts] });
    // [ts] expect_eq(outerSize.width, 750);
    // [ts] expect_eq(outerSize.height, 550);
    // [ts] });
    // [ts] test("should open pages in a new window in maximized state", () => {
    // [ts] const {context, browser} = getTestState();
    // [ts] const page = context.newPage({
    // [ts] type: "window",
    // [ts] windowBounds: {windowState: "maximized"},
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, browser.pages(), page) != null, true);
    // [ts] const outerSize = page.evaluate(() => {
    // [ts] return {width: outerWidth, height: outerHeight};
    // [ts] });
    // Should match default headless screen size 800x600.
    // [ts] expect_eq(outerSize.width, 800);
    // [ts] expect_eq(outerSize.height, 600);
    // [ts] });
    // [ts] test("should create a background page", () => {
    // [ts] const {context} = getTestState();
    // [ts] const page = context.newPage({
    // [ts] background: true,
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.visibilityState;
    // [ts] }),
    // [ts] ).toBe("hidden");
    // [ts] });
    // [ts] test("should reject all promises when page is closed", () => {
    // [ts] const {context} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] let error: Error;
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] newPage
    // [ts] .evaluate(() => {
    // [ts] return Promise.init(() => {});
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] }),
    // [ts] newPage.close(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Protocol error") != null, true);
    // [ts] });
    // [ts] test("should not be visible in browser.pages", () => {
    // [ts] const {browser, context} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] expect_eq(std.mem.indexOf(u8, browser.pages(), newPage) != null, true);
    // [ts] newPage.close();
    // [ts] expect_eq(std.mem.indexOf(u8, browser.pages(), newPage) == null, true);
    // [ts] });
    // [ts] test("should close child iframes", () => {
    // [ts] const {context, server} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] newPage.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(newPage.frames().length, 2);
    // [ts] newPage.close();
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), newPage) == null, true);
    // [ts] });
    // [ts] test("should run beforeunload if asked for", () => {
    // [ts] const {context, server, isChrome} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] newPage.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] newPage.click("body");
    // [ts] const pageClosingPromise = newPage.close({runBeforeUnload: true});
    // [ts] const dialog = waitEvent(newPage, "dialog");
    // [ts] expect_eq(dialog.type(), "beforeunload");
    // [ts] expect_eq(dialog.defaultValue(), "");
    // [ts] if (isChrome) {
    // [ts] expect_eq(dialog.message(), "");
    // [ts] } else {
    // [ts] expect_eq(dialog.message(), true);
    // [ts] }
    // [ts] dialog.accept();
    // [ts] pageClosingPromise;
    // [ts] });
    // [ts] test("should *not* run beforeunload by default", () => {
    // [ts] const {context, server} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] newPage.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] newPage.click("body");
    // [ts] newPage.close();
    // [ts] });
    // [ts] test("should set the page close state", () => {
    // [ts] const {context} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] expect_eq(newPage.isClosed(), false);
    // [ts] newPage.close();
    // [ts] expect_eq(newPage.isClosed(), true);
    // [ts] });
    // [ts] test("should terminate network waiters", () => {
    // [ts] const {context, server} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] const results = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] newPage.waitForRequest(server.EMPTY_PAGE).catch(error => {
    // [ts] return error;
    // [ts] }),
    // [ts] newPage.waitForResponse(server.EMPTY_PAGE).catch(error => {
    // [ts] return error;
    // [ts] }),
    // [ts] newPage.close(),
    // [ts] ]);
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] const message = results[i].message;
    // [ts] expect(message).atLeastOneToContain([
    // [ts] "Target closed",
    // [ts] "Page closed!",
    // [ts] "Frame detached",
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, message, "Timeout") == null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should fire when expected", () => {
    // [ts] const {page} = getTestState();
    // [ts] joinAll([waitEvent(page, "load"), page.goto("about:blank")]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("should correctly fire event handlers as they are added and then removed", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const handler = sinon.spy();
    // [ts] const onResponse = (response) => {
    // Ignore default favicon requests.
    // [ts] if (!isFavicon(response)) {
    // [ts] handler();
    // [ts] }
    // [ts] };
    // [ts] page.on("response", onResponse);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(handler.callCount, 1);
    // [ts] page.off("response", onResponse);
    // [ts] page.goto(server.EMPTY_PAGE);
    // Still one because we removed the handler.
    // [ts] expect_eq(handler.callCount, 1);
    // [ts] page.on("response", onResponse);
    // [ts] page.goto(server.EMPTY_PAGE);
    // Two now because we added the handler back.
    // [ts] expect_eq(handler.callCount, 2);
    // [ts] });
    // [ts] test("should correctly added and removed request events", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const handler = sinon.spy();
    // [ts] const onResponse = (response) => {
    // Ignore default favicon requests.
    // [ts] if (!isFavicon(response)) {
    // [ts] handler();
    // [ts] }
    // [ts] };
    // [ts] page.on("request", onResponse);
    // [ts] page.on("request", onResponse);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(handler.callCount, 2);
    // [ts] page.off("request", onResponse);
    // [ts] page.goto(server.EMPTY_PAGE);
    // Still one because we removed the handler.
    // [ts] expect_eq(handler.callCount, 3);
    // [ts] page.off("request", onResponse);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(handler.callCount, 3);
    // [ts] page.on("request", onResponse);
    // [ts] page.goto(server.EMPTY_PAGE);
    // Two now because we added the handler back.
    // [ts] expect_eq(handler.callCount, 4);
    // [ts] });
    // [ts] test("should throw when page crashes", () => {
    // [ts] const {page, isChrome} = getTestState();
    // [ts] let navigate: Promise<unknown>;
    // [ts] if (isChrome) {
    // [ts] navigate = page.goto("chrome://crash").catch(() => {});
    // [ts] } else {
    // [ts] navigate = page.goto("about:crashcontent").catch(() => {});
    // [ts] }
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(
    // [ts] page,
    // [ts] "error",
    // [ts] () => {
    // [ts] return true;
    // [ts] },
    // [ts] 10_000,
    // [ts] ),
    // [ts] navigate,
    // [ts] ]);
    // [ts] expect_eq(error.message, "Page crashed!");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "popup"),
    // [ts] page.evaluate(() => {
    // [ts] return window.open("about:blank");
    // [ts] }),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] popup.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work with noopener", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "popup"),
    // [ts] page.evaluate(() => {
    // [ts] return window.open("about:blank", null, "noopener");
    // [ts] }),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] popup.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work with clicking target=_blank and without rel=opener", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(
    // [ts] html`<a
    // [ts] target="_blank"
    // [ts] href="/one-style.html"
    // [ts] >yo</a
    // [ts] >`,
    // [ts] );
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "popup"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] popup.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work with clicking target=_blank and with rel=opener", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(
    // [ts] html`<a
    // [ts] target="_blank"
    // [ts] rel="opener"
    // [ts] href="/one-style.html"
    // [ts] >yo</a
    // [ts] >`,
    // [ts] );
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "popup"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] popup.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should work with fake-clicking target=_blank and rel=noopener", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(
    // [ts] html`<a
    // [ts] target="_blank"
    // [ts] rel="noopener"
    // [ts] href="/one-style.html"
    // [ts] >yo</a
    // [ts] >`,
    // [ts] );
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "popup"),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('a', a => {
    // [ts-skip] return a.click();
    // [ts-skip] }),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] popup.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work with clicking target=_blank and rel=noopener", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(
    // [ts] html`<a
    // [ts] target="_blank"
    // [ts] rel="noopener"
    // [ts] href="/one-style.html"
    // [ts] >yo</a
    // [ts] >`,
    // [ts] );
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "popup"),
    // [ts] page.click("a"),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] expect(
    // [ts] popup.evaluate(() => {
    // [ts] return !!window.opener;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] context.overridePermissions(server.PREFIX, ["geolocation"]);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setGeolocation({longitude: 10, latitude: 10});
    // [ts] const geolocation = page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] return navigator.geolocation.getCurrentPosition(position => {
    // [ts] resolve({
    // [ts] latitude: position.coords.latitude,
    // [ts] longitude: position.coords.longitude,
    // [ts] accuracy: position.coords.accuracy,
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] expect: (geolocation) => {
    // [ts] latitude: 10,
    // [ts] longitude: 10,
    // [ts] accuracy: 0,
    // [ts] });
    // [ts] });
    // [ts] test("should remove the override when called with null", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] context.overridePermissions(server.PREFIX, ["geolocation"]);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setGeolocation({longitude: 10, latitude: 10});
    // [ts] page.setGeolocation(null);
    // [ts] const geolocation = page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] return navigator.geolocation.getCurrentPosition(
    // [ts] position => {
    // [ts] resolve({
    // [ts] latitude: position.coords.latitude,
    // [ts] longitude: position.coords.longitude,
    // [ts] });
    // [ts] },
    // [ts] () => {
    // [ts] resolve("no position");
    // [ts] },
    // [ts] {timeout: 1000},
    // [ts] );
    // [ts] });
    // [ts] });
    // [ts] expect: (geolocation) => {
    // [ts] latitude: 10,
    // [ts] longitude: 10,
    // [ts] });
    // [ts] });
    // [ts] test("should throw when invalid longitude", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.setGeolocation({longitude: 200, latitude: 10});
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Invalid longitude \"200\"") != null, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setOfflineMode(true);
    // [ts] let error: Error;
    // [ts] page.goto(server.EMPTY_PAGE).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] page.setOfflineMode(false);
    // [ts] const response = (page.reload());
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should emulate navigator.onLine", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.navigator.onLine;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] page.setOfflineMode(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.navigator.onLine;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] page.setOfflineMode(false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.navigator.onLine;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should fire when expected", () => {
    // [ts] const {page} = getTestState();
    // [ts] const navigate = page.goto("about:blank");
    // [ts] joinAll([waitEvent(page, "domcontentloaded"), navigate]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("should get metrics from a page", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.goto("about:blank");
    // [ts] const metrics = page.metrics();
    // [ts] checkMetrics(metrics);
    // [ts] });
    // [ts] test("metrics event fired on console.timeStamp", () => {
    // [ts] const {page} = getTestState();
    // [ts] const metricsPromise = waitEvent<{metrics: Metrics; title: string}>(
    // [ts] page,
    // [ts] "metrics",
    // [ts] );
    // [ts] page.evaluate(() => {
    // [ts] return console.timeStamp("test42");
    // [ts] });
    // [ts] const metrics = metricsPromise;
    // [ts] expect_eq(metrics.title, "test42");
    // [ts] checkMetrics(metrics.metrics);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest(server.PREFIX + "/digits/2.png"),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should work with predicate", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest(request => {
    // [ts] return request.url() == server.PREFIX + "/digits/2.png";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should work with async predicate", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest(async request => {
    // [ts] return request.url() == server.PREFIX + "/digits/2.png";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .waitForRequest(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {timeout: 1},
    // [ts] )
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should respect default timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.setDefaultTimeout(1);
    // [ts] page
    // [ts] .waitForRequest(() => {
    // [ts] return false;
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should work with no timeout", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest(server.PREFIX + "/digits/2.png", {timeout: 0}),
    // [ts] page.evaluate(() => {
    // [ts] return setTimeout(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }, 50);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should be cancellable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const abortController = AbortController.init();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const task = page.waitForRequest(server.PREFIX + "/abortme", {
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/digits/2.png"),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .waitForResponse(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {timeout: 1},
    // [ts] )
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should respect default timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.setDefaultTimeout(1);
    // [ts] page
    // [ts] .waitForResponse(() => {
    // [ts] return false;
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should work with predicate", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(response => {
    // [ts] return response.url() == server.PREFIX + "/digits/2.png";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should work with async predicate", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(async response => {
    // [ts] return response.url() == server.PREFIX + "/digits/2.png";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should work with no timeout", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(server.PREFIX + "/digits/2.png", {timeout: 0}),
    // [ts] page.evaluate(() => {
    // [ts] return setTimeout(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }, 50);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(response.url(), server.PREFIX + "/digits/2.png");
    // [ts] });
    // [ts] test("should be cancellable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForResponse(server.PREFIX + "/abortme", {
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let res;
    // [ts] const [t1, t2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNetworkIdle().then(r => {
    // [ts] res = r;
    // [ts] return Date.now();
    // [ts] }),
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] joinAll([fetch("/digits/1.png"), fetch("/digits/2.png")]); // migrated: Promise.all -> joinAll
    // [ts] Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 200);
    // [ts] });
    // [ts] fetch("/digits/3.png");
    // [ts] Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 200);
    // [ts] });
    // [ts] fetch("/digits/4.png");
    // [ts] })
    // [ts] .then(() => {
    // [ts] return Date.now();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(res, null);
    // [ts] expect_eq(t1 > t2, true);
    // [ts] expect_eq(t1 - t2 >= 400, true);
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.waitForNetworkIdle({timeout: 1}).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should respect idleTime", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [t1, t2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNetworkIdle({idleTime: 10}).then(() => {
    // [ts] return Date.now();
    // [ts] }),
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] return (() => {
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] fetch("/digits/1.png"),
    // [ts] fetch("/digits/2.png"),
    // [ts] ]);
    // [ts] Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 250);
    // [ts] });
    // [ts] })();
    // [ts] })
    // [ts] .then(() => {
    // [ts] return Date.now();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(t2 > t1, true);
    // [ts] });
    // [ts] test("should work with no timeout", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [result] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNetworkIdle({timeout: 0}),
    // [ts] page.evaluate(() => {
    // [ts] return setTimeout(() => {
    // [ts] void fetch("/digits/1.png");
    // [ts] void fetch("/digits/2.png");
    // [ts] void fetch("/digits/3.png");
    // [ts] }, 50);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(result, null);
    // [ts] });
    // [ts] test("should work with aborted requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/abort-request.html");
    // [ts] using element = page.$(`#abort`);
    // [ts] element.click();
    // [ts] let error = false;
    // [ts] page.waitForNetworkIdle().catch(() => {
    // [ts] return (error = true);
    // [ts] });
    // [ts] expect_eq(error, false);
    // [ts] });
    // [ts] test("should work with delayed response", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let response: ServerResponse;
    // [ts] server.setRoute("/fetch-request-b.js", (_req, res) => {
    // [ts] response = res;
    // [ts] });
    // [ts] const t0 = Date.now();
    // [ts] const [t1, t2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNetworkIdle({idleTime: 100}).then(() => {
    // [ts] return Date.now();
    // [ts] }),
    // [ts] Promise.init(res => {
    // [ts] setTimeout(() => {
    // [ts] response.end();
    // [ts] res(Date.now());
    // [ts] }, 300);
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] fetch("/fetch-request-b.js");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(t1 > t2, true);
    // request finished + idle time.
    // [ts] expect_eq(t1 - t0 >= 400, true);
    // request finished + idle time - request finished.
    // [ts] expect_eq(t1 - t2 >= 100, true);
    // [ts] });
    // [ts] test("should be cancelable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForNetworkIdle({
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] const promise = page.evaluate(() => {
    // [ts] joinAll([fetch("/digits/1.png")]); // migrated: Promise.all -> joinAll
    // [ts] fetch("/digits/2.png");
    // [ts] });
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] promise;
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [waitedFrame] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFrame(frame => {
    // [ts] return frame.url().endsWith("/title.html");
    // [ts] }),
    // [ts] attachFrame(page, "frame2", server.PREFIX + "/title.html"),
    // [ts] ]);
    // [ts] expect_eq(waitedFrame.parentFrame(), page.mainFrame());
    // [ts] });
    // [ts] test("should work with a URL predicate", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [waitedFrame] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFrame(server.PREFIX + "/title.html"),
    // [ts] attachFrame(page, "frame2", server.PREFIX + "/title.html"),
    // [ts] ]);
    // [ts] expect_eq(waitedFrame.parentFrame(), page.mainFrame());
    // [ts] });
    // [ts] test("should be cancellable", () => {
    // [ts] const {server, page} = getTestState();
    // [ts] const abortController = AbortController.init();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const task = page.waitForFrame(
    // [ts] frame => {
    // [ts] return frame.url().endsWith("/title.html");
    // [ts] },
    // [ts] {
    // [ts] signal: abortController.signal,
    // [ts] },
    // [ts] );
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).compute(9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] });
    // [ts] test("should throw exception in page context", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction("woof", () => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('WOOF WOOF');)
    // [ts] });
    // [ts] const {message, stack} = page.evaluate(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] return (
    // [ts] globalThis
    // [ts] ).woof();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] return {
    // [ts] message: (error).message,
    // [ts] stack: (error).stack,
    // [ts] };
    // [ts] }
    // [ts] });
    // [ts] expect_eq(message, "WOOF WOOF");
    // [ts] expect_eq(std.mem.indexOf(u8, stack, "page.test.ts") != null, true);
    // [ts] });
    // [ts] test("should support throwing \"null\"", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction("woof", () => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw null;)
    // [ts] });
    // [ts] const thrown = page.evaluate(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] (globalThis).woof();
    // [ts] return;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] return error;
    // [ts] }
    // [ts] });
    // [ts] expect_eq(thrown, null);
    // [ts] });
    // [ts] test("should reject when the return value cannot be serialized", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction("woof", () => {
    // [ts] return {value: BigInt(42)};
    // [ts] });
    // [ts] const message = page.evaluate(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] (globalThis).woof();
    // [ts] return;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] return (error).message;
    // [ts] }
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, message, "BigInt") != null, true);
    // [ts] });
    // [ts] test("should be callable from-inside evaluateOnNewDocument", () => {
    // [ts] const {page} = getTestState();
    // [ts] const called = Deferred.init();
    // [ts] page.exposeFunction("woof", () => {
    // [ts] called.resolve();
    // [ts] });
    // [ts] page.evaluateOnNewDocument(() => {
    // [ts] return (globalThis).woof();
    // [ts] });
    // [ts] page.reload();
    // [ts] called.valueOrThrow();
    // [ts] });
    // [ts] test("should survive navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).compute(9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] });
    // [ts] test("should await returned promise", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).compute(3, 5);
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("should await returned if called from function", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] const result = (globalThis).compute(3, 5);
    // [ts] return result;
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("should work on frames", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] const frame = page.frames()[1];
    // [ts] const result = frame.evaluate(() => {
    // [ts] return (globalThis).compute(3, 5);
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("should work with loading frames", () => {
    // Tries to reproduce the scenario from
    // https://github.com/puppeteer/puppeteer/issues/8106
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] let saveRequest: (value) => void;
    // [ts] const iframeRequest = Promise.init(resolve => {
    // [ts] saveRequest = resolve;
    // [ts] });
    // [ts] page.on("request", async req => {
    // [ts] if (req.url().endsWith("/frames/frame.html")) {
    // [ts] saveRequest(req);
    // [ts] } else {
    // [ts] req.continue();
    // [ts] }
    // [ts] });
    // [ts] let error: Error | null;
    // [ts] const navPromise = page
    // [ts] .goto(server.PREFIX + "/frames/one-frame.html", {
    // [ts] waitUntil: "networkidle0",
    // [ts] })
    // [ts] .catch(err => {
    // [ts] error = err;
    // [ts] });
    // [ts] const req = iframeRequest;
    // Expose function while the frame is being loaded. Loading process is
    // controlled by interception.
    // [ts] const exposePromise = page.exposeFunction(
    // [ts] "compute",
    // [ts] (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] },
    // [ts] );
    // [ts] joinAll([req.continue(), exposePromise]); // migrated: Promise.all -> joinAll
    // [ts] navPromise;
    // [ts] expect_eq(error == null, true);
    // [ts] const frame = page.frames()[1];
    // [ts] const result = frame.evaluate(() => {
    // [ts] return (globalThis).compute(3, 5);
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("should work on frames before navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] const frame = page.frames()[1];
    // [ts] const result = frame.evaluate(() => {
    // [ts] return (globalThis).compute(3, 5);
    // [ts] });
    // [ts] expect_eq(result, 15);
    // [ts] });
    // [ts] test("should not throw when frames detach", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return Promise.resolve(a * b);
    // [ts] });
    // [ts] detachFrame(page, "frame1");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).compute(3, 5);
    // [ts] }),
    // [ts] ).resolves.toEqual(15);
    // [ts] });
    // [ts] test("should work with complex objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction(
    // [ts] "complexObject",
    // [ts] (a, b) => {
    // [ts] return {x: a.x + b.x};
    // [ts] },
    // [ts] );
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).complexObject({x: 5}, {x: 2});
    // [ts] });
    // [ts] expect_eq(result.x, 7);
    // [ts] });
    // [ts] test("should fallback to default export when passed a module object", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const moduleObject = {
    // [ts] default: (a, b) => {
    // [ts] return a * b;
    // [ts] },
    // [ts] };
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.exposeFunction("compute", moduleObject);
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).compute(9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] });
    // [ts] test("should be called once", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] let calls = 0;
    // [ts] page.exposeFunction("call", () => {
    // [ts] calls++;
    // [ts] });
    // [ts] const frame = page.frames()[1];
    // [ts] frame.evaluate(() => {
    // [ts] return (globalThis).call();
    // [ts] });
    // [ts] expect_eq(calls, 1);
    // [ts] });
    // [ts] test("should work after deleting exposed function from globalThis", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction(
    // [ts] "compute",
    // [ts] (a, b, element) => {
    // [ts] if (a < 0) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('negative input');)
    // [ts] }
    // [ts] const tag = element ? element.evaluate(el => {
    // [ts] return el.nodeName;
    // [ts] }) : "";
    // [ts] return `${a * b}:${tag}`;
    // [ts] },
    // [ts] );
    // [ts] const result = page.evaluate(() => {
    // [ts] const compute = (globalThis).compute;
    // [ts] delete (globalThis).compute;
    // [ts] const value = compute(9, 4, document.body);
    // [ts] let errorMessage = "";
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] compute(-1, 4);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] errorMessage = (error).message;
    // [ts] }
    // [ts] return {
    // [ts] existsOnWindow: "compute" == globalThis,
    // [ts] value,
    // [ts] errorMessage,
    // [ts] };
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] existsOnWindow: false,
    // [ts] value: "36:BODY",
    // [ts] errorMessage: "negative input",
    // [ts] });
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.exposeFunction("compute", (a, b) => {
    // [ts] return a * b;
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).compute(9, 4);
    // [ts] });
    // [ts] expect_eq(result, 36);
    // [ts] page.removeExposedFunction("compute");
    // [ts] const error = page
    // [ts] .evaluate(() => {
    // [ts] return (globalThis).compute(9, 4);
    // [ts] })
    // [ts] .then(() => {
    // [ts] return null;
    // [ts] })
    // [ts] .catch(error => {
    // [ts] return error;
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("should fire", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "pageerror", err => {
    // [ts] return err.message.includes("Fancy");
    // [ts] }),
    // [ts] page.goto(server.PREFIX + "/error.html"),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Fancy") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack?.split("\n").at(-1), "error.html:3:1") != null, true);
    // [ts] });
    // [ts] test("should fire for all value types", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "pageerror"),
    // [ts] page.goto(server.PREFIX + "/error-primitive.html"),
    // [ts] ]);
    // [ts] expect_eq(error, null);
    // [ts] });
    // [ts] test("should fire", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const isCdp = browser.protocol == "cdp";
    // [ts] const [exception] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "exception"),
    // [ts] page.goto(server.PREFIX + "/error.html"),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, exception.text(), "Fancy error!") != null, true);
    // [ts] expect_eq(exception.name(), if (isCdp) "Error" else null);
    // [ts] const stackTrace = exception.stackTrace();
    // [ts] expect(
    // [ts] stackTrace.slice(0, 3).map(frame => {
    // [ts] return frame.functionName;
    // [ts] }),
    // [ts] ).toEqual(["c", "b", "a"]);
    // [ts] expect_eq(exception.location(), stackTrace[0]);
    // [ts] expect_eq(std.mem.indexOf(u8, exception.location().url, "error.html") != null, true);
    // 0-based, `throw new Error('Fancy error!')` is on line 14.
    // [ts] expect_eq(exception.location().lineNumber, 13);
    // [ts] });
    // [ts] test("should fire together with pageerror", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const [error, exception] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "pageerror"),
    // [ts] waitEvent(page, "exception"),
    // [ts] page.goto(server.PREFIX + "/error.html"),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Fancy error!") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, exception.text(), "Fancy error!") != null, true);
    // [ts] });
    // [ts] test("should fire for primitive values", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const isCdp = browser.protocol == "cdp";
    // [ts] const [exception] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "exception"),
    // [ts] page.goto(server.PREFIX + "/error-primitive.html"),
    // [ts] ]);
    // [ts] expect_eq(exception.name(), if (isCdp) "" else null);
    // [ts] expect_eq(exception.handle() == null, true);
    // [ts] });
    // [ts] test("should report the name and text as rendered by the browser", () => {
    // [ts] const {page} = getTestState();
    // [ts] const cases = [
    // [ts] {
    // [ts] code: `const error = new Error('x'); error.name = 'AbortError'; throw error;`,
    // [ts] name: "AbortError",
    // [ts] text: "AbortError: x",
    // [ts] },
    // [ts] {
    // [ts] code: `throw new DOMException('bad', 'AbortError');`,
    // [ts] name: "AbortError",
    // [ts] text: "AbortError: bad",
    // [ts] },
    // [ts] {
    // [ts] code: `class ValidationError extends Error {} throw new ValidationError('v');`,
    // [ts] name: "ValidationError",
    // [ts] text: "ValidationError: v",
    // [ts] },
    // [ts] {code: `throw new Error();`, name: "Error", text: "Error"},
    // [ts] {
    // [ts] code: `throw new Error('line1\\nline2');`,
    // [ts] name: "Error",
    // [ts] text: "Error: line1\nline2",
    // [ts] },
    // [ts] {code: `throw {code: 1};`, name: "", text: "Object"},
    // [ts] {code: `throw Symbol('s');`, name: "", text: "Symbol(s)"},
    // [ts] ];
    // [ts] for (const {code, name, text} of cases) {
    // [ts] const [exception] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "exception"),
    // [ts] page.setContent(
    // [ts] html`<script>
    // [ts] ${code};
    // [ts] </script>`,
    // [ts] ),
    // [ts] ]);
    // [ts] expect: (_p) => {
    // [ts] name,
    // [ts] text,
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("should fall back to the exception location for syntax errors", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const isCdp = browser.protocol == "cdp";
    // Load the invalid script from a file rather than via `setContent`:
    // `setContent` parses the HTML inside a Puppeteer-injected script, and
    // WebDriver BiDi then reports that injected code as the stack.
    // [ts] const [exception] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "exception"),
    // [ts] page.goto(server.PREFIX + "/syntax-error.html"),
    // [ts] ]);
    // [ts] expect_eq(exception.stackTrace(), []);
    // [ts] if (!isCdp) {
    // `log.JavascriptLogEntry` has no top-level coordinates, so there is
    // nothing to fall back to.
    // [ts] expect_eq(exception.location(), {});
    // [ts] return;
    // [ts] }
    // [ts] expect_eq(exception.name(), "SyntaxError");
    // [ts] expect_eq(exception.text(), "SyntaxError: Unexpected token '='");
    // With an empty stack, `location()` falls back to the top-level
    // coordinates of the exception.
    // [ts] expect: (exception.location()) => {
    // [ts] url: server.PREFIX + "/syntax-error.js",
    // [ts] lineNumber: 2,
    // [ts] columnNumber: expect.any(Number),
    // [ts] });
    // [ts] });
    // [ts] test("should expose the remote exception via a moved handle", () => {
    // [ts] const {page} = getTestState();
    // [ts] const handlePromise = Promise.init(resolve => {
    // [ts] page.once("exception", exception => {
    // [ts] resolve(exception.handle().move());
    // [ts] });
    // [ts] });
    // Exceptions from code injected via `page.evaluate` carry no remote
    // exception object, so throw from a page script instead.
    // [ts] page.setContent(
    // [ts] html`<script>
    // [ts] const error = new Error('custom');
    // [ts] error.code = 'E_CUSTOM';
    // [ts] throw error;
    // [ts] </script>`,
    // [ts] );
    // [ts] using handle = handlePromise;
    // [ts] expect_eq(handle.disposed, false);
    // [ts] expect(
    // [ts] handle.evaluate(error => {
    // [ts] return (error).code;
    // [ts] }),
    // [ts] ).toBe("E_CUSTOM");
    // [ts] });
    // [ts] test("should dispose the handle after dispatch if not moved", () => {
    // [ts] const {page} = getTestState();
    // [ts] const handlePromise = Promise.init(resolve => {
    // [ts] page.once("exception", exception => {
    // [ts] resolve(exception.handle());
    // [ts] });
    // [ts] });
    // [ts] page.setContent(
    // [ts] html`<script>
    // [ts] throw new Error('not moved');
    // [ts] </script>`,
    // [ts] );
    // eslint-disable-next-line max-len -- The comment is long.
    // eslint-disable-next-line @puppeteer/use-using -- Not owned by the test; Puppeteer disposes of it.
    // [ts] const handle = handlePromise;
    // [ts] expect_eq(handle != null, true);
    // [ts] expect_eq(handle.disposed, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] }),
    // [ts] ).toContain("Mozilla");
    // [ts] page.setUserAgent("foobar");
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "foobar");
    // [ts] });
    // [ts] test("should work with options parameter", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] }),
    // [ts] ).toContain("Mozilla");
    // [ts] page.setUserAgent({userAgent: "foobar"});
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "foobar");
    // [ts] });
    // [ts] test("should work with platform option", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.platform;
    // [ts] }),
    // [ts] ).not.toBe("MockPlatform");
    // [ts] page.setUserAgent({
    // [ts] userAgent: "foobar",
    // [ts] platform: "MockPlatform",
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.platform;
    // [ts] }),
    // [ts] ).toBe("MockPlatform");
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "foobar");
    // [ts] });
    // [ts] test("should work with platform option without userAgent", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const originalUserAgent = page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.platform;
    // [ts] }),
    // [ts] ).not.toBe("MockPlatform");
    // [ts] page.setUserAgent({
    // [ts] platform: "MockPlatform",
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.platform;
    // [ts] }),
    // [ts] ).toBe("MockPlatform");
    // User agent should remain the same
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] }),
    // [ts] ).toBe(originalUserAgent);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], originalUserAgent);
    // [ts] });
    // [ts] test("should work for subframes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] }),
    // [ts] ).toContain("Mozilla");
    // [ts] page.setUserAgent("foobar");
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], "foobar");
    // [ts] });
    // [ts] test("should emulate device user-agent", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/mobile.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] }),
    // [ts] ).not.toContain("iPhone");
    // [ts] page.setUserAgent(KnownDevices["iPhone 6"].userAgent);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] }),
    // [ts] ).toContain("iPhone");
    // [ts] });
    // [ts] test("should work with additional userAgentMetdata", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setUserAgent("MockBrowser", {
    // [ts] architecture: "Mock1",
    // [ts] mobile: false,
    // [ts] model: "Mockbook",
    // [ts] platform: "MockOS",
    // [ts] platformVersion: "3.1",
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // @ts-expect-error: userAgentData not yet in TypeScript DOM API
    // [ts] return navigator.userAgentData.mobile;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] const uaData = page.evaluate(() => {
    // @ts-expect-error: userAgentData not yet in TypeScript DOM API
    // [ts] return navigator.userAgentData.getHighEntropyValues([
    // [ts] "architecture",
    // [ts] "model",
    // [ts] "platform",
    // [ts] "platformVersion",
    // [ts] ]);
    // [ts] });
    // [ts] expect_eq(uaData["architecture"], "Mock1");
    // [ts] expect_eq(uaData["model"], "Mockbook");
    // [ts] expect_eq(uaData["platform"], "MockOS");
    // [ts] expect_eq(uaData["platformVersion"], "3.1");
    // [ts] expect_eq(request.headers["user-agent"], "MockBrowser");
    // [ts] });
    // [ts] test("should restore original", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const userAgent = page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] });
    // [ts] page.setUserAgent("foobar");
    // [ts] const [requestWithOverride] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(requestWithOverride.headers["user-agent"], "foobar");
    // [ts] page.setUserAgent("");
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["user-agent"], userAgent);
    // [ts] const userAgentRestored = page.evaluate(() => {
    // [ts] return navigator.userAgent;
    // [ts] });
    // [ts] expect_eq(userAgentRestored, userAgent);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(htmlRaw`<div>hello</div>`);
    // [ts] const result = page.content();
    // [ts] expect_eq(result, expectedOutput);
    // [ts] });
    // [ts] test("should work with doctype", () => {
    // [ts] const {page} = getTestState();
    // [ts] const doctype = "<!DOCTYPE html>";
    // [ts] page.setContent(htmlRaw`${doctype}<div>hello</div>`);
    // [ts] const result = page.content();
    // [ts] expect_eq(result, `${doctype}${expectedOutput}`);
    // [ts] });
    // [ts] test("should work with HTML 4 doctype", () => {
    // [ts] const {page} = getTestState();
    // [ts] const doctype =
    // [ts] "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" " +
    // [ts] "\"http://www.w3.org/TR/html4/strict.dtd\">";
    // [ts] page.setContent(htmlRaw`${doctype}<div>hello</div>`);
    // [ts] const result = page.content();
    // [ts] expect_eq(result, `${doctype}${expectedOutput}`);
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const imgPath = "/img.png";
    // stall for image
    // [ts] server.setRoute(imgPath, () => {});
    // [ts] let error: Error;
    // [ts] page
    // [ts] .setContent(html`<img src="${server.PREFIX + imgPath}"></img>`, {
    // [ts] timeout: 1,
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should remove its listeners when it times out", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const imgPath = "/img.png";
    // [ts] server.setRoute(imgPath, () => {});
    // [ts] const frame = page.mainFrame();
    // [ts] const listeners = frame.listenerCount(FrameEvent.FrameDetached);
    // [ts] expect(
    // [ts] page.setContent(html`<img src="${server.PREFIX + imgPath}"></img>`, {
    // [ts] timeout: 1,
    // [ts] }),
    // [ts] ).rejects.toBeInstanceOf(TimeoutError);
    // [ts] expect_eq(frame.listenerCount(FrameEvent.FrameDetached), listeners);
    // [ts] });
    // [ts] test("should be cancellable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const imgPath = "/img.png";
    // stall for image
    // [ts] server.setRoute(imgPath, () => {});
    // [ts] const abortController = AbortController.init();
    // [ts] const content = page.setContent(
    // [ts] html`<img src="${server.PREFIX + imgPath}"></img>`,
    // [ts] {signal: abortController.signal},
    // [ts] );
    // [ts] server.waitForRequest(imgPath);
    // [ts] abortController.abort();
    // [ts] expect(content).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should not set the content when the signal is already aborted", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(htmlRaw`<div>before</div>`);
    // [ts] expect(
    // [ts] page.setContent(htmlRaw`<div>after</div>`, {
    // [ts] signal: AbortSignal.abort(),
    // [ts] }),
    // [ts] ).rejects.toThrow("aborted");
    // [ts] expect_eq(std.mem.indexOf(u8, page.content(), "<div>before</div>") != null, true);
    // [ts] });
    // [ts] test("should respect default navigation timeout", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setDefaultNavigationTimeout(1);
    // [ts] const imgPath = "/img.png";
    // stall for image
    // [ts] server.setRoute(imgPath, () => {});
    // [ts] let error: Error;
    // [ts] page
    // [ts] .setContent(html`<img src="${server.PREFIX + imgPath}"></img>`)
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should await resources to load", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const imgPath = "/img.png";
    // [ts] let imgResponse: ServerResponse;
    // [ts] server.setRoute(imgPath, (_req, res) => {
    // [ts] return (imgResponse = res);
    // [ts] });
    // [ts] let loaded = false;
    // [ts] const contentPromise = page
    // [ts] .setContent(html`<img src="${server.PREFIX + imgPath}"></img>`)
    // [ts] .then(() => {
    // [ts] return (loaded = true);
    // [ts] });
    // [ts] server.waitForRequest(imgPath);
    // [ts] expect_eq(loaded, false);
    // [ts] imgResponse.end();
    // [ts] contentPromise;
    // [ts] });
    // [ts] test("should work fast enough", () => {
    // [ts] const {page} = getTestState();
    // [ts] let i = 0;
    // [ts] while (i < 20) { // [cap-migrate] for-step: ++i
    // [ts] page.setContent(html`<div>yo</div>`);
    // [ts] }
    // [ts] });
    // [ts] test("should work with tricky content", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`${"<div>hello world</div>" + "\x7F"}`);
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
    // [ts-skip] return div.textContent;
    // [ts-skip] }),
    // [ts] ).toBe("hello world");
    // [ts] });
    // [ts] test("should work with accents", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>aberraciÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ³n</div>`);
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
    // [ts-skip] return div.textContent;
    // [ts-skip] }),
    // [ts] ).toBe("aberraciÃÂ³n");
    // [ts] });
    // [ts] test("should work with emojis", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>ÃÂÃÂÃÂÃÂ°ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¥</div>`);
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
    // [ts-skip] return div.textContent;
    // [ts-skip] }),
    // [ts] ).toBe("Ã°ÂÂÂ¥");
    // [ts] });
    // [ts] test("should work with newline", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(htmlRaw`<div>\n</div>`);
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', div => {
    // [ts-skip] return div.textContent;
    // [ts-skip] }),
    // [ts] ).toBe("\n");
    // [ts] });
    // [ts] test("should work with comments outside HTML tag", () => {
    // [ts] const {page} = getTestState();
    // [ts] const comment = "<!-- Comment -->";
    // [ts] page.setContent(htmlRaw`${comment}<div>hello</div>`);
    // [ts] const result = page.content();
    // [ts] expect_eq(result, `${comment}${expectedOutput}`);
    // [ts] });
    // [ts] test("should not run a cross-origin script through document.write", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const warnings: []string = [];
    // [ts] page.on("console", message => {
    // [ts] if (message.type() == "warn") {
    // [ts] warnings.push(message.text());
    // [ts] }
    // [ts] });
    // [ts] page.setContent(
    // [ts] html`<script src="${server.CROSS_PROCESS_PREFIX + "/injectedfile.js"}"></script>`,
    // [ts] {waitUntil: "load"},
    // [ts] );
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] expect(
    // [ts] warnings.filter(warning => {
    // [ts] return warning.includes("document.write");
    // [ts] }),
    // [ts] ).toHaveLength(0);
    // [ts] });
    // [ts] test("should bypass CSP meta tag", () => {
    // [ts] const {page, server} = getTestState();
    // Make sure CSP prohibits addScriptTag.
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] page
    // [ts] .addScriptTag({content: "window.__injected = 42;"})
    // [ts] .catch(error => {
    // [ts] return void error;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(null);
    // By-pass CSP and try one more time.
    // [ts] page.setBypassCSP(true);
    // [ts] page.reload();
    // [ts] page.addScriptTag({content: "window.__injected = 42;"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should bypass CSP header", () => {
    // [ts] const {page, server} = getTestState();
    // Make sure CSP prohibits addScriptTag.
    // [ts] server.setCSP("/empty.html", "default-src \"self\"");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page
    // [ts] .addScriptTag({content: "window.__injected = 42;"})
    // [ts] .catch(error => {
    // [ts] return void error;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(null);
    // By-pass CSP and try one more time.
    // [ts] page.setBypassCSP(true);
    // [ts] page.reload();
    // [ts] page.addScriptTag({content: "window.__injected = 42;"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should bypass after cross-process navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setBypassCSP(true);
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] page.addScriptTag({content: "window.__injected = 42;"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/csp.html");
    // [ts] page.addScriptTag({content: "window.__injected = 42;"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should bypass CSP in iframes as well", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] {
    // Make sure CSP prohibits addScriptTag in an iframe.
    // [ts] const frame = (attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.PREFIX + "/csp.html",
    // [ts] ));
    // [ts] frame
    // [ts] .addScriptTag({content: "window.__injected = 42;"})
    // [ts] .catch(error => {
    // [ts] return void error;
    // [ts] });
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(null);
    // [ts] }
    // By-pass CSP and try one more time.
    // [ts] page.setBypassCSP(true);
    // [ts] page.reload();
    // [ts] {
    // [ts] const frame = (attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.PREFIX + "/csp.html",
    // [ts] ));
    // [ts] frame
    // [ts] .addScriptTag({content: "window.__injected = 42;"})
    // [ts] .catch(error => {
    // [ts] return void error;
    // [ts] });
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] }
    // [ts] });
    // [ts] test("should throw an error if no options are provided", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error purposefully passing bad options
    // [ts] page.addScriptTag("/injectedfile.js");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect(error.message).toBe(
    // [ts] "Exactly one of `url`, `path`, or `content` must be specified.",
    // [ts] );
    // [ts] });
    // [ts] test("should work with a url", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] using scriptHandle = page.addScriptTag({url: "/injectedfile.js"});
    // [ts] expect_eq(scriptHandle.asElement() != null, true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should work with a url and type=module", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.addScriptTag({url: "/es6/es6import.js", type: "module"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (window).__es6injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should work with a path and type=module", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.addScriptTag({
    // [ts] path: path.join(import.meta.dirname, "../assets/es6/es6pathimport.js"),
    // [ts] type: "module",
    // [ts] });
    // [ts] page.waitForFunction(() => {
    // [ts] return (window).__es6injected;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (window).__es6injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should work with a content and type=module", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.addScriptTag({
    // [ts] content: `import num from '/es6/es6module.js';window.__es6injected = num;`,
    // [ts] type: "module",
    // [ts] });
    // [ts] page.waitForFunction(() => {
    // [ts] return (window).__es6injected;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (window).__es6injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should throw an error if loading from url fail", () => {
    // [ts] const {page, server, isFirefox} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.addScriptTag({url: "/nonexistfile.js"});
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] if (isFirefox) {
    // [ts] expect_eq(error.message, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Could not load script") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should work with a path", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] using scriptHandle = page.addScriptTag({
    // [ts] path: path.join(import.meta.dirname, "../assets/injectedfile.js"),
    // [ts] });
    // [ts] expect_eq(scriptHandle.asElement() != null, true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should include sourcemap when path is provided", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.addScriptTag({
    // [ts] path: path.join(import.meta.dirname, "../assets/injectedfile.js"),
    // [ts] });
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).__injectedError.stack;
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, result, path.join("assets", "injectedfile.js")) != null, true);
    // [ts] });
    // [ts] test("should work with content", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] using scriptHandle = page.addScriptTag({
    // [ts] content: "window.__injected = 35;",
    // [ts] });
    // [ts] expect_eq(scriptHandle.asElement() != null, true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__injected;
    // [ts] }),
    // [ts] ).toBe(35);
    // [ts] });
    // [ts] test("should add id when provided", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.addScriptTag({content: "window.__injected = 1;", id: "one"});
    // [ts] page.addScriptTag({url: "/injectedfile.js", id: "two"});
    // [ts] expect_eq(page.$("#one") != null, true);
    // [ts] expect_eq(page.$("#two") != null, true);
    // [ts] });
    // [ts] test("should throw when added with content to the CSP page", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] let error: Error;
    // [ts] page
    // [ts] .addScriptTag({content: "window.__injected = 35;"})
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("should throw when added with URL to the CSP page", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] let error: Error;
    // [ts] page
    // [ts] .addScriptTag({url: server.CROSS_PROCESS_PREFIX + "/injectedfile.js"})
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("should throw an error if no options are provided", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error purposefully passing bad input
    // [ts] page.addStyleTag("/injectedstyle.css");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect(error.message).toBe(
    // [ts] "Exactly one of `url`, `path`, or `content` must be specified.",
    // [ts] );
    // [ts] });
    // [ts] test("should work with a url", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] using styleHandle = page.addStyleTag({url: "/injectedstyle.css"});
    // [ts] expect_eq(styleHandle.asElement() != null, true);
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] `window.getComputedStyle(document.querySelector('body')).getPropertyValue('background-color')`,
    // [ts] ),
    // [ts] ).toBe("rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("should throw an error if loading from url fail", () => {
    // [ts] const {page, server, isFirefox} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.addStyleTag({url: "/nonexistfile.js"});
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] if (isFirefox) {
    // [ts] expect_eq(error.message, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Could not load style") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should work with a path", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] using styleHandle = page.addStyleTag({
    // [ts] path: path.join(import.meta.dirname, "../assets/injectedstyle.css"),
    // [ts] });
    // [ts] expect_eq(styleHandle.asElement() != null, true);
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] `window.getComputedStyle(document.querySelector('body')).getPropertyValue('background-color')`,
    // [ts] ),
    // [ts] ).toBe("rgb(255, 0, 0)");
    // [ts] });
    // [ts] test("should work with a path when the options are reused", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const options = {
    // [ts] path: path.join(import.meta.dirname, "../assets/injectedstyle.css"),
    // [ts] };
    // [ts] page.addStyleTag(options);
    // [ts] using styleHandle = page.addStyleTag(options);
    // [ts] expect_eq(styleHandle.asElement() != null, true);
    // [ts] expect_eq(Object.keys(options), ["path"]);
    // [ts] });
    // [ts] test("should include sourcemap when path is provided", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.addStyleTag({
    // [ts] path: path.join(import.meta.dirname, "../assets/injectedstyle.css"),
    // [ts] });
    // [ts] using styleHandle = (page.$("style"));
    // [ts] const styleContent = page.evaluate(style => {
    // [ts] return style.innerHTML;
    // [ts] }, styleHandle);
    // [ts] expect_eq(std.mem.indexOf(u8, styleContent, path.join("assets", "injectedstyle.css")) != null, true);
    // [ts] });
    // [ts] test("should work with content", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] using styleHandle = page.addStyleTag({
    // [ts] content: "body { background-color: green; }",
    // [ts] });
    // [ts] expect_eq(styleHandle.asElement() != null, true);
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] `window.getComputedStyle(document.querySelector('body')).getPropertyValue('background-color')`,
    // [ts] ),
    // [ts] ).toBe("rgb(0, 128, 0)");
    // [ts] });
    // [ts] test("should throw when added with content to the CSP page", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] let error: Error;
    // [ts] page
    // [ts] .addStyleTag({content: "body { background-color: green; }"})
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("should throw when added with URL to the CSP page", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] let error: Error;
    // [ts] page
    // [ts] .addStyleTag({
    // [ts] url: server.CROSS_PROCESS_PREFIX + "/injectedstyle.css",
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] expect_eq(page.url(), "about:blank");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setJavaScriptEnabled(false);
    // [ts] expect_eq(page.isJavaScriptEnabled(), false);
    // [ts] page.goto(
    // [ts] "data:text/html, <script>var something = \"forbidden\"</script>",
    // [ts] );
    // [ts] let error: Error;
    // [ts] page.evaluate("something").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "something is not defined") != null, true);
    // [ts] page.setJavaScriptEnabled(true);
    // [ts] expect_eq(page.isJavaScriptEnabled(), true);
    // [ts] page.goto(
    // [ts] "data:text/html, <script>var something = \"forbidden\"</script>",
    // [ts] );
    // [ts] expect_eq(page.evaluate("something"), "forbidden");
    // [ts] });
    // [ts] test("setInterval should pause", () => {
    // [ts] const {page} = getTestState();
    // Set up an interval that increments a counter every 0ms. This will queue up tasks
    // to run as fast as possible.
    // [ts] page.evaluate(() => {
    // [ts] return setInterval(() => {
    // [ts] return ((globalThis).intervalCounter =
    // [ts] ((globalThis).intervalCounter ?? 0) + 1);
    // [ts] }, 0);
    // [ts] });
    // Disable JavaScript execution on the page. This should pause timers.
    // [ts] page.setJavaScriptEnabled(false);
    // Capture the current value of the counter after JS is disabled.
    // [ts] const intervalCounter = page.evaluate(() => {
    // [ts] return (globalThis).intervalCounter;
    // [ts] });
    // Wait for 100 ms. This gives the event loop with the task a
    // chance to run if it were not paused, which would have incremented the counter.
    // [ts] Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 100);
    // [ts] });
    // Verify that the counter has not changed, confirming that setInterval was paused
    // when JavaScript was disabled.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).intervalCounter;
    // [ts] }),
    // [ts] ).toBe(intervalCounter);
    // Re-enable JavaScript execution.
    // [ts] page.setJavaScriptEnabled(true);
    // Wait for another task. It should be long enough to avoid flakiness, as the
    // original `setInterval` will be throttled after several invocations.
    // [ts] page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 100);
    // [ts] });
    // [ts] });
    // Verify that the counter increased. This confirms that timers resumed when
    // JavaScript is re-enabled.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).intervalCounter;
    // [ts] }),
    // [ts] ).toBeGreaterThan(intervalCounter);
    // [ts] });
    // [ts] test("setTimeout should stop", () => {
    // [ts] const {page} = getTestState();
    // Set up a recursive setTimeout chain. The `task` function increments a counter and
    // immediately schedules itself to run again.
    // [ts] page.evaluate(() => {
    // [ts] const task = () => {
    // [ts] (globalThis).timeoutCounter =
    // [ts] ((globalThis).timeoutCounter ?? 0) + 1;
    // [ts] setTimeout(task, 0);
    // [ts] };
    // [ts] task();
    // [ts] });
    // Disable JavaScript, which should pause the timeout chain.
    // [ts] page.setJavaScriptEnabled(false);
    // Capture the counter's value after the timeout chain is paused.
    // [ts] const timeoutCounter = page.evaluate(() => {
    // [ts] return (globalThis).timeoutCounter;
    // [ts] });
    // Wait for 100 ms. This gives the event loop with the task a
    // chance to run if it were not paused, which would have incremented the counter.
    // [ts] Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 100);
    // [ts] });
    // Verify the counter has not changed, confirming that setTimeout was paused.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).timeoutCounter;
    // [ts] }),
    // [ts] ).toBe(timeoutCounter);
    // Re-enable JavaScript.
    // [ts] page.setJavaScriptEnabled(true);
    // Wait for another task. It should be long enough to avoid flakiness, as the
    // original `setInterval` will be throttled after several invocations.
    // [ts] page.evaluate(() => {
    // [ts] return Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 100);
    // [ts] });
    // [ts] });
    // Verify the counter still has not changed, confirming that `setTimeout` do not
    // resume upon re-enabling JavaScript.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).timeoutCounter;
    // [ts] }),
    // [ts] ).toBe(timeoutCounter);
    // [ts] });
    // [ts] test("then should not pause", () => {
    // [ts] const {page} = getTestState();
    // Disable JavaScript execution on the page.
    // [ts] page.setJavaScriptEnabled(false);
    // Assert the microtasks continue to work even when page scripts are disabled.
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return Promise.resolve().then(() => {
    // [ts] return 42;
    // [ts] });
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should enable or disable the cache based on reload params", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/cached/one-style.html");
    // [ts] const [cachedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/cached/one-style.html"),
    // [ts] page.reload(),
    // [ts] ]);
    // Rely on "if-modified-since" caching in our test server.
    // [ts] expect_eq(cachedRequest.headers["if-modified-since"] != null, true);
    // [ts] const [nonCachedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/cached/one-style.html"),
    // [ts] page.reload({
    // [ts] ignoreCache: true,
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(nonCachedRequest.headers["if-modified-since"], null);
    // [ts] });
    // [ts] test("should enable or disable the cache based on the state passed", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/cached/one-style.html");
    // [ts] const [cachedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/cached/one-style.html"),
    // [ts] page.reload(),
    // [ts] ]);
    // Rely on "if-modified-since" caching in our test server.
    // [ts] expect_eq(cachedRequest.headers["if-modified-since"] != null, true);
    // [ts] page.setCacheEnabled(false);
    // [ts] const [nonCachedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/cached/one-style.html"),
    // [ts] page.reload(),
    // [ts] ]);
    // [ts] expect_eq(nonCachedRequest.headers["if-modified-since"], null);
    // [ts] });
    // [ts] test("should enable the cache when called without arguments", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setCacheEnabled(false);
    // [ts] page.setCacheEnabled();
    // [ts] page.goto(server.PREFIX + "/cached/one-style.html");
    // [ts] const [cachedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/cached/one-style.html"),
    // [ts] page.reload(),
    // [ts] ]);
    // [ts] expect_eq(cachedRequest.headers["if-modified-since"] != null, true);
    // [ts] });
    // [ts] test("should stay disabled when toggling request interception on/off", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setCacheEnabled(false);
    // [ts] page.setRequestInterception(true);
    // [ts] page.setRequestInterception(false);
    // [ts] page.goto(server.PREFIX + "/cached/one-style.html");
    // [ts] const [nonCachedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/cached/one-style.html"),
    // [ts] page.reload(),
    // [ts] ]);
    // [ts] expect_eq(nonCachedRequest.headers["if-modified-since"], null);
    // [ts] });
    // [ts] test("can print to PDF and save to file", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const outputFile = import.meta.dirname + "/../assets/output.pdf";
    // [ts] page.goto(server.PREFIX + "/pdf.html");
    // [ts] page.pdf({path: outputFile});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(fs.readFileSync(outputFile).byteLength > 0, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] fs.unlinkSync(outputFile);
    // [ts] }
    // [ts] });
    // [ts] test("can print to PDF and stream the result", () => {
    // [ts] const {page} = getTestState();
    // [ts] const stream = page.createPDFStream();
    // [ts] let size = 0;
    // [ts] const reader = stream.getReader();
    // [ts] while (true) {
    // [ts] const {done, value} = reader.read();
    // [ts] if (done) {
    // [ts] break;
    // [ts] }
    // [ts] size += value.length;
    // [ts] }
    // [ts] expect_eq(size > 0, true);
    // [ts] });
    // [ts] test("should accept page ranges without spaces", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <div style="break-after: page">1</div>
    // [ts] <div style="break-after: page">2</div>
    // [ts] <div>3</div>
    // [ts] `);
    // [ts] const pdf = page.pdf({pageRanges: "1-2,3"});
    // [ts] expect_eq(pdf.byteLength > 0, true);
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/pdf.html");
    // [ts] const error = page.pdf({timeout: 1}).catch(err => {
    // [ts] return err;
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should not wait for fonts if waitForFonts is false", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // The font request never gets a response, so `document.fonts.ready`
    // does not resolve.
    // [ts] server.setRoute("/never-loading-font.woff2", () => {});
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/never-loading-font.woff2"),
    // [ts] page.evaluate(url => {
    // [ts] const font = FontFace.init("never-loading", `url(${url})`);
    // [ts] document.fonts.add(font);
    // [ts] void font.load();
    // [ts] }, server.PREFIX + "/never-loading-font.woff2"),
    // [ts] ]);
    // [ts] const pdf = page.pdf({waitForFonts: false, timeout: 5_000});
    // [ts] expect_eq(pdf.byteLength > 0, true);
    // [ts] });
    // [ts] test("should return the page title", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] expect_eq(page.title(), "Woof-Woof");
    // [ts] });
    // [ts] test("should select single option", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.select("select", "blue");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onInput;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onChange;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] });
    // [ts] test("should select only first option", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.select("select", "blue", "green", "red");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onInput;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onChange;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] });
    // [ts] test("should not throw when select causes navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', select => {
    // [ts-skip] return select.addEventListener('input', () => {
    // [ts-skip] return ((window as any).location = '/empty.html');
    // [ts-skip] });
    // [ts-skip] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.select("select", "blue"),
    // [ts] page.waitForNavigation(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, page.url(), "empty.html") != null, true);
    // [ts] });
    // [ts] test("should select multiple options", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).makeMultiple();
    // [ts] });
    // [ts] page.select("select", "blue", "green", "red");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onInput;
    // [ts] }),
    // [ts] ).toEqual(["blue", "green", "red"]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onChange;
    // [ts] }),
    // [ts] ).toEqual(["blue", "green", "red"]);
    // [ts] });
    // [ts] test("should respect event bubbling", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.select("select", "blue");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onBubblingInput;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onBubblingChange;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] });
    // [ts] test("should throw when element is not a <select>", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let error: Error;
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.select("body", "").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Element is not a <select> element.") != null, true);
    // [ts] });
    // [ts] test("should return [] on no matched values", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const result = page.select("select", "42", "abc");
    // [ts] expect_eq(result, []);
    // [ts] });
    // [ts] test("should return an array of matched values", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).makeMultiple();
    // [ts] });
    // [ts] const result = page.select("select", "blue", "black", "magenta");
    // [ts] expect(
    // [ts] result.reduce((accumulator, current) => {
    // [ts] return ["blue", "black", "magenta"].includes(current) && accumulator;
    // [ts] }, true),
    // [ts] ).toEqual(true);
    // [ts] });
    // [ts] test("should return an array of one element when multiple is not set", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const result = page.select(
    // [ts] "select",
    // [ts] "42",
    // [ts] "blue",
    // [ts] "black",
    // [ts] "magenta",
    // [ts] );
    // [ts] expect(result).toHaveLength(1);
    // [ts] });
    // [ts] test("should return [] on no values", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] const result = page.select("select");
    // [ts] expect_eq(result, []);
    // [ts] });
    // [ts] test("should deselect all options when passed no values for a multiple select", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).makeMultiple();
    // [ts] });
    // [ts] page.select("select", "blue", "black", "magenta");
    // [ts] page.select("select");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', select => {
    // [ts-skip] return Array.from(select.options).every(option => {
    // [ts-skip] return !option.selected;
    // [ts-skip] });
    // [ts-skip] }),
    // [ts] ).toEqual(true);
    // [ts] });
    // [ts] test("should deselect all options when passed no values for a select without multiple", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.select("select", "blue", "black", "magenta");
    // [ts] page.select("select");
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('select', select => {
    // [ts-skip] return Array.from(select.options).filter(option => {
    // [ts-skip] return option.selected;
    // [ts-skip] })[0]!.value;
    // [ts-skip] }),
    // [ts] ).toEqual("");
    // [ts] });
    // [ts] test("should throw if passed in non-strings", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<select>
    // [ts] <option value="12"></option>
    // [ts] </select>`,
    // [ts] );
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error purposefully passing bad input
    // [ts] page.select("select", 12);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Values must be strings") != null, true);
    // [ts] });
    // [ts] test("should work when re-defining top-level Event class", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/select.html");
    // [ts] page.evaluate(() => {
    // @ts-expect-error Expected.
    // [ts] return (window.Event = null);
    // [ts] });
    // [ts] page.select("select", "blue");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onInput;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.onChange;
    // [ts] }),
    // [ts] ).toEqual(["blue"]);
    // [ts] });
    // [ts] test("should work with window.close", () => {
    // [ts] const {page, context} = getTestState();
    // [ts] const newPagePromise = Promise.init(fulfill => {
    // [ts] return context.once("targetcreated", target => {
    // [ts] return fulfill(target.page());
    // [ts] });
    // [ts] });
    // [ts] assert(page);
    // [ts] page.evaluate(() => {
    // [ts] return ((window)["newPage"] = window.open("about:blank"));
    // [ts] });
    // [ts] const newPage = newPagePromise;
    // [ts] assert(newPage);
    // [ts] const closedPromise = waitEvent(newPage, "close");
    // [ts] page.evaluate(() => {
    // [ts] return (window)["newPage"].close();
    // [ts] });
    // [ts] closedPromise;
    // [ts] });
    // [ts] test("should work with page.close", () => {
    // [ts] const {context} = getTestState();
    // [ts] const newPage = context.newPage();
    // [ts] const closedPromise = waitEvent(newPage, "close");
    // [ts] newPage.close();
    // [ts] closedPromise;
    // [ts] });
    // [ts] test("should be able to connect and disable issues", () => {
    // [ts] const {page, server} = state;
    // [ts] let issueEmitted = false;
    // [ts] page.on("issue", () => {
    // [ts] issueEmitted = true;
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] expect_eq(issueEmitted, false);
    // [ts] });
    // [ts] test("should emit issue event when CSP violation occurs", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/csp.html");
    // [ts] const issuePromise = waitEvent(page, "issue");
    // [ts] page.addScriptTag({content: "console.log(\"CSP test\")"});
    // [ts] const issue = issuePromise;
    // [ts] expect_eq(issue, true);
    // [ts] expect_eq(issue.code, "ContentSecurityPolicyIssue");
    // [ts] });
    // [ts] test("should emit issue event from cross-origin iframe", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const issuePromise = waitEvent(page, "issue");
    // [ts] const crossOriginUrl = server.CROSS_PROCESS_PREFIX + "/csp.html";
    // [ts] page.setContent(html`<iframe src="${crossOriginUrl}"></iframe>`);
    // [ts] const frame = page.waitForFrame(crossOriginUrl);
    // [ts] expect_eq(frame, true);
    // [ts] frame.addScriptTag({
    // [ts] content: "console.log(\"CSP test in iframe\")",
    // [ts] });
    // [ts] const issue = issuePromise;
    // [ts] expect_eq(issue, true);
    // [ts] expect_eq(issue.code, "ContentSecurityPolicyIssue");
    // [ts] });
    // [ts] test("should return the correct browser instance", () => {
    // [ts] const {page, browser} = getTestState();
    // [ts] expect_eq(page.browser(), browser);
    // [ts] });
    // [ts] test("should return the correct browser context instance", () => {
    // [ts] const {page, context} = getTestState();
    // [ts] expect_eq(page.browserContext(), context);
    // [ts] });
    // [ts] test("should return the client instance", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect((page)._client()).toBeInstanceOf(CDPSession);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {context} = getTestState();
    // [ts] const page1 = context.newPage();
    // [ts] const page2 = context.newPage();
    // [ts] const waitForVisibilityState = async (
    // [ts] targetPage: Page,
    // [ts] expectedState: DocumentVisibilityState,
    // [ts] ) => {
    // [ts] return targetPage.evaluate(async expected => {
    // [ts] if (document.visibilityState == expected) {
    // [ts] return document.visibilityState;
    // [ts] }
    // [ts] Promise.init(resolve => {
    // [ts] const timeout = window.setTimeout(() => {
    // [ts] document.removeEventListener(
    // [ts] "visibilitychange",
    // [ts] onVisibilityChange,
    // [ts] );
    // [ts] resolve();
    // [ts] }, 3_000);
    // [ts] const onVisibilityChange = () => {
    // [ts] if (document.visibilityState == expected) {
    // [ts] window.clearTimeout(timeout);
    // [ts] document.removeEventListener(
    // [ts] "visibilitychange",
    // [ts] onVisibilityChange,
    // [ts] );
    // [ts] resolve();
    // [ts] }
    // [ts] };
    // [ts] document.addEventListener("visibilitychange", onVisibilityChange);
    // [ts] });
    // [ts] return document.visibilityState;
    // [ts] }, expectedState);
    // [ts] };
    // [ts] page1.bringToFront();
    // [ts] expect_eq(waitForVisibilityState(page1, "visible"), "visible");
    // [ts] expect_eq(waitForVisibilityState(page2, "hidden"), "hidden");
    // [ts] page2.bringToFront();
    // [ts] expect_eq(waitForVisibilityState(page1, "hidden"), "hidden");
    // [ts] expect_eq(waitForVisibilityState(page2, "visible"), "visible");
    // [ts] page1.close();
    // [ts] page2.close();
    // [ts] });
    // [ts] test("should return the tab index and active state", () => {
    // [ts] const {context} = getTestState();
    // [ts] const page1 = context.newPage();
    // [ts] const page2 = context.newPage();
    // [ts] page1.bringToFront();
    // [ts] const tabInfo1 = page1.tabInfo();
    // [ts] const tabInfo2 = page2.tabInfo();
    // [ts] expect(tabInfo1).toMatchObject({active: true, pinned: false});
    // [ts] expect(tabInfo2).toMatchObject({active: false, pinned: false});
    // [ts] expect_eq(tabInfo2.index, tabInfo1.index + 1);
    // [ts] expect_eq(tabInfo1.groupId == null, true);
    // [ts] expect_eq(tabInfo2.groupId == null, true);
    // [ts] page2.bringToFront();
    // [ts] expect(page1.tabInfo()).toMatchObject({active: false});
    // [ts] expect(page2.tabInfo()).toMatchObject({active: true});
    // [ts] page1.close();
    // [ts] page2.close();
    // [ts] });
    // [ts] test("should resize the browser window to fit page content", () => {
    // [ts] const {context} = state;
    // [ts] const page = context.newPage();
    // Default view port restricts window to 800x600, so remove it.
    // [ts] page.setViewport(null);
    // [ts] const contentWidth = 500;
    // [ts] const contentHeight = 400;
    // [ts] const resized = waitForResize(page, contentWidth, contentHeight);
    // [ts] page.resize({contentWidth, contentHeight});
    // [ts] resized;
    // [ts] const innerSize = page.evaluate(() => {
    // [ts] return {width: window.innerWidth, height: window.innerHeight};
    // [ts] });
    // [ts] expect_eq(innerSize.width, contentWidth);
    // [ts] expect_eq(innerSize.height, contentHeight);
    // [ts] });
    // [ts] test("should resize the browser window to fit page content when fullscreen", () => {
    // [ts] const {browser, context} = state;
    // [ts] const page = context.newPage();
    // Default view port restricts window to 800x600, so remove it.
    // [ts] page.setViewport(null);
    // [ts] const windowId = page.windowId();
    // [ts] browser.setWindowBounds(windowId, {windowState: "fullscreen"});
    // [ts] const windowState = browser.getWindowBounds(windowId);
    // [ts] assert.strictEqual(windowState.windowState, "fullscreen");
    // [ts] browser.setWindowBounds(windowId, {windowState: "normal"});
    // [ts] browser.setWindowBounds(windowId, {windowState: "normal"});
    // [ts] const contentWidth = 500;
    // [ts] const contentHeight = 400;
    // [ts] const resized = waitForResize(page, contentWidth, contentHeight);
    // [ts] page.resize({contentWidth, contentHeight});
    // [ts] resized;
    // [ts] const innerSize = page.evaluate(() => {
    // [ts] return {
    // [ts] width: window.innerWidth,
    // [ts] height: window.innerHeight,
    // [ts] };
    // [ts] });
    // [ts] expect_eq(innerSize.width, contentWidth);
    // [ts] expect_eq(innerSize.height, contentHeight);
    // [ts] });
    // [ts] test("should record page", () => {
    // [ts] using file = getUniqueVideoFilePlaceholder();
    // [ts] const {page} = getTestState();
    // [ts] const recording = page.record({
    // [ts] path: file.filename,
    // [ts] });
    // [ts] page.goto("data:text/html,<input>");
    // [ts] using input = page.locator("input").waitHandle();
    // [ts] input.type("ab", {delay: 100});
    // [ts] recording.stop();
    // [ts] expect_eq(fs.statSync(file.filename).size > 0, true);
    // [ts] });
}
