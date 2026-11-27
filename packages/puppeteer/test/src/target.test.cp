// Migrated from TypeScript (src/target.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

// [cap-migrate] import {ServerResponse} from "cap:http";

// [cap-migrate] import {TestServer} from "@pptr/testserver";
import { test, expect_eq } from "cap:test";
import { Target, TimeoutError, UnsupportedOperation } from "puppeteer";
// [cap-migrate] import {Browser} from "puppeteer-core/internal/api/Browser.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
import { TargetCloseError } from "puppeteer-core/internal/common/Errors.js";
// [cap-migrate] import {RemoteException} from "puppeteer-core/internal/common/RemoteException.js";

import { getTestState, launch, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, waitEvent } from "./utils.cp";

// [cap-test] describe("Target", () => {
  setupTestBrowserHooks();




    expect_eq(destroyedTarget, createdTarget);
  // [cap-test] });








  // [cap-test] describe("Target.close", () => {










  // [cap-test] });

  // [cap-test] describe("Browser.waitForTarget", () => {
  // [cap-test] });
// TODO(cap-migrate): refine TS statement
// [ts] });

/**
 * Opens a page that registers a service worker and waits until the service
 * worker is ready, which gives Puppeteer time to attach to it through the page.
 */
async fn openServiceWorkerPage(browser: Browser, server: TestServer): Page {
  const page = browser.newPage();
  page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
  page.evaluate(() => {
    navigator.serviceWorker.ready;
  });
  return page;
}

/**
 * Watches the targets of the browser through a CDP session of the page.
 * `destroyed` resolves when Chrome reports the service worker as destroyed,
 * which it does only after all sessions attached to it detach.
 */
async fn watchServiceWorker(page: Page): Promise< {destroyed: Promise<void>}> {
    // TODO(cap-migrate): refine TS body
    // [ts] const session = page.createCDPSession();
    // [ts] const destroyed = Promise.init(resolve => {
    // [ts] let targetId: string | null;
    // [ts] session.on("Target.targetCreated", event => {
    // [ts] if (event.targetInfo.type == "service_worker") {
    // [ts] targetId = event.targetInfo.targetId;
    // [ts] }
    // [ts] });
    // [ts] session.on("Target.targetDestroyed", event => {
    // [ts] if (event.targetId == targetId) {
    // [ts] resolve();
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] session.send("Target.setDiscoverTargets", {discover: true});
    // [ts] return {destroyed};
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("Target.asPage() should return the same instance", () => {
    // [ts] const {browser} = getTestState();
    // [ts] const page = browser.newPage();
    // [ts] const target = page.target();
    // Test that target.page() and target.asPage() return the same instance
    // [ts] const page1 = target.page();
    // [ts] const page2 = target.asPage();
    // [ts] const page3 = target.asPage();
    // [ts] expect_eq(page1, page);
    // [ts] expect_eq(page2, page);
    // [ts] expect_eq(page3, page);
    // [ts] page.close();
    // [ts] });
    // [ts] test("Browser.targets should return all of the targets", () => {
    // [ts] const {browser} = getTestState();
    // The pages will be the testing page and the original newtab page
    // [ts] const targets = browser.targets();
    // [ts] expect(
    // [ts] targets.some(target => {
    // [ts] return target.type() == "page" && target.url() == "about:blank";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] expect(
    // [ts] targets.some(target => {
    // [ts] return target.type() == "browser";
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("Browser.pages should return all of the pages", () => {
    // [ts] const {page, context} = getTestState();
    // The pages will be the testing page
    // [ts] const allPages = context.pages();
    // [ts] expect(allPages).toHaveLength(1);
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, page) != null, true);
    // [ts] });
    // [ts] test("page should return tab target id", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect_eq(page._tabId.length > 0, true);
    // [ts] });
    // [ts] test("should contain browser target", () => {
    // [ts] const {browser} = getTestState();
    // [ts] const targets = browser.targets();
    // [ts] const browserTarget = targets.find(target => {
    // [ts] return target.type() == "browser";
    // [ts] });
    // [ts] expect_eq(browserTarget, true);
    // [ts] });
    // [ts] test("should be able to use the default page in the browser", () => {
    // [ts] const {page, browser} = getTestState();
    // The pages will be the testing page and the original newtab page
    // [ts] const allPages = browser.pages();
    // [ts] const originalPage = allPages.find(p => {
    // [ts] return p != page;
    // [ts] });
    // [ts] expect(
    // [ts] originalPage.evaluate(() => {
    // [ts] return ["Hello", "world"].join(" ");
    // [ts] }),
    // [ts] ).toBe("Hello world");
    // [ts] expect_eq(originalPage.$("body"), true);
    // [ts] });
    // [ts] test("should be able to use async waitForTarget", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] const [otherPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context
    // [ts] .waitForTarget(
    // [ts] target => {
    // [ts] return target.page().then(page => {
    // [ts] return (
    // [ts] page.url() == server.CROSS_PROCESS_PREFIX + "/empty.html"
    // [ts] );
    // [ts] });
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] )
    // [ts] .then(target => {
    // [ts] return target.page();
    // [ts] }),
    // [ts] page.evaluate((url) => {
    // [ts] return window.open(url);
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/empty.html"),
    // [ts] ]);
    // [ts] expect(otherPage.url()).toEqual(
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] expect_eq(page != otherPage, true);
    // [ts] });
    // [ts] test("should report when a new page is created and closed", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] const [otherPage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context
    // [ts] .waitForTarget(
    // [ts] target => {
    // [ts] return target.url() == server.CROSS_PROCESS_PREFIX + "/empty.html";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] )
    // [ts] .then(target => {
    // [ts] return target.page();
    // [ts] }),
    // [ts] page.evaluate((url) => {
    // [ts] return window.open(url);
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/empty.html"),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, otherPage.url(), server.CROSS_PROCESS_PREFIX) != null, true);
    // [ts] expect(
    // [ts] otherPage.evaluate(() => {
    // [ts] return ["Hello", "world"].join(" ");
    // [ts] }),
    // [ts] ).toBe("Hello world");
    // [ts] expect_eq(otherPage.$("body"), true);
    // [ts] let allPages = context.pages();
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, otherPage) != null, true);
    // [ts] const [closedTarget] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(context, "targetdestroyed"),
    // [ts] otherPage.close(),
    // [ts] ]);
    // [ts] expect_eq(closedTarget.page(), otherPage);
    // [ts] allPages = (joinAll( // migrated: Promise.all -> joinAll
    // [ts] context.targets().map(target => {
    // [ts] return target.page();
    // [ts] }),
    // [ts] ));
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, page) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allPages, otherPage) == null, true);
    // [ts] });
    // [ts] test("should report when a service worker is created and destroyed", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const createdTarget = waitEvent(context, "targetcreated");
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] expect_eq((createdTarget).type(), "service_worker");
    // [ts] expect((createdTarget).url()).toBe(
    // [ts] server.PREFIX + "/serviceworkers/empty/sw.js",
    // [ts] );
    // [ts] const destroyedTarget = waitEvent(context, "targetdestroyed");
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] globalThis>; }
    // [ts] ).registrationPromise.then((registration) => {
    // [ts] return registration.unregister();
    // [ts] });
    // [ts] });
    // [ts] test("should create a worker from a service worker", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] expect(
    // [ts] worker.evaluate(() => {
    // [ts] return self.toString();
    // [ts] }),
    // [ts] ).toBe("[object ServiceWorkerGlobalScope]");
    // [ts] });
    // [ts] test("should emit exceptions on a service worker but not on the page", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const pageExceptions: []RemoteException = [];
    // [ts] page.on("exception", exception => {
    // [ts] pageExceptions.push(exception);
    // [ts] });
    // [ts] const [exception] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, "exception"),
    // [ts] worker.evaluate(() => {
    // [ts] setTimeout(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('service worker error');)
    // [ts] });
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, exception.text(), "service worker error") != null, true);
    // [ts] expect(pageExceptions).toHaveLength(0);
    // [ts] });
    // [ts] test("should close a service worker", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const onceDestroyed = Promise.init(resolve => {
    // [ts] context.once("targetdestroyed", event => {
    // [ts] resolve(event);
    // [ts] });
    // [ts] });
    // [ts] worker.close();
    // [ts] expect_eq(onceDestroyed, target);
    // [ts] });
    // [ts] test("should close a service worker when network rules are set", () => {
    // [ts] const {browser, server, close} = launch({
    // [ts] blocklist: ["*://*:*/empty.html"],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // With network rules, Puppeteer stays attached to service workers, also
    // through the page that registered them.
    // [ts] const page = openServiceWorkerPage(browser, server);
    // [ts] const target = browser.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const targetDestroyed = waitEvent(
    // [ts] browser,
    // [ts] "targetdestroyed",
    // [ts] destroyed => {
    // [ts] return destroyed == target;
    // [ts] },
    // [ts] );
    // [ts] const {destroyed} = watchServiceWorker(page);
    // [ts] worker.close();
    // [ts] expect_eq(targetDestroyed, target);
    // [ts] destroyed;
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should create a worker from a shared worker", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] SharedWorker.init("data:text/javascript,console.log(\"hi\")");
    // [ts] });
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "shared_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] expect(
    // [ts] worker.evaluate(() => {
    // [ts] return self.toString();
    // [ts] }),
    // [ts] ).toBe("[object SharedWorkerGlobalScope]");
    // [ts] });
    // [ts] test("should close a shared worker", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] SharedWorker.init("data:text/javascript,console.log(\"hi2\")");
    // [ts] });
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "shared_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const onceDestroyed = Promise.init(resolve => {
    // [ts] context.once("targetdestroyed", event => {
    // [ts] resolve(event);
    // [ts] });
    // [ts] });
    // [ts] worker.close();
    // [ts] expect_eq(onceDestroyed, target);
    // [ts] });
    // [ts] test("should report when a target url changes", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let changedTarget = waitEvent(context, "targetchanged");
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/");
    // [ts] expect_eq((changedTarget).url(), server.CROSS_PROCESS_PREFIX + "/");
    // [ts] changedTarget = waitEvent(context, "targetchanged");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq((changedTarget).url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should not report uninitialized pages", () => {
    // [ts] const {context} = getTestState();
    // [ts] let targetChanged = false;
    // [ts] const listener = () => {
    // [ts] targetChanged = true;
    // [ts] };
    // [ts] context.on("targetchanged", listener);
    // [ts] const targetPromise = waitEvent(context, "targetcreated");
    // [ts] const newPagePromise = context.newPage();
    // [ts] const target = targetPromise;
    // [ts] expect_eq(target.url(), "about:blank");
    // [ts] const newPage = newPagePromise;
    // [ts] const targetPromise2 = waitEvent(context, "targetcreated");
    // [ts] const evaluatePromise = newPage.evaluate(() => {
    // [ts] return window.open("about:blank");
    // [ts] });
    // [ts] const target2 = targetPromise2;
    // [ts] expect_eq(target2.url(), "about:blank");
    // [ts] evaluatePromise;
    // [ts] newPage.close();
    // [ts] expect_eq(targetChanged, false);
    // [ts] context.off("targetchanged", listener);
    // [ts] });
    // [ts] test("should not crash while redirecting if original request was missed", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] let serverResponse: ServerResponse;
    // [ts] server.setRoute("/one-style.css", (_req, res) => {
    // [ts] return (serverResponse = res);
    // [ts] });
    // Open a new page. Use window.open to connect to the page later.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate((url) => {
    // [ts] return window.open(url);
    // [ts] }, server.PREFIX + "/one-style.html"),
    // [ts] server.waitForRequest("/one-style.css"),
    // [ts] ]);
    // Connect to the opened page.
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.url().includes("one-style.html");
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const newPage = (target.page());
    // [ts] const loadEvent = waitEvent(newPage, "load");
    // Issue a redirect.
    // [ts] serverResponse.writeHead(302, {location: "/injectedstyle.css"});
    // [ts] serverResponse.end();
    // Wait for the new page to load.
    // [ts] loadEvent;
    // Cleanup.
    // [ts] newPage.close();
    // [ts] });
    // [ts] test("should have an opener", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [createdTarget] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(context, "targetcreated"),
    // [ts] page.goto(server.PREFIX + "/popup/window-open.html"),
    // [ts] ]);
    // [ts] expect((createdTarget.page()).url()).toBe(
    // [ts] server.PREFIX + "/popup/popup.html",
    // [ts] );
    // [ts] expect_eq(createdTarget.opener(), page.target());
    // [ts] expect_eq(page.target().opener() == null, true);
    // [ts] });
    // [ts] test("should close the target", () => {
    // [ts] const {context} = getTestState();
    // [ts] const page = context.newPage();
    // [ts] const target = page.target();
    // [ts] const targetDestroyed = waitEvent(
    // [ts] context,
    // [ts] "targetdestroyed",
    // [ts] destroyed => {
    // [ts] return destroyed == target;
    // [ts] },
    // [ts] );
    // [ts] target.close();
    // [ts] expect_eq(std.mem.indexOf(u8, context.targets(), target) == null, true);
    // [ts] expect_eq(targetDestroyed, target);
    // [ts] expect_eq(std.mem.indexOf(u8, context.pages(), page) == null, true);
    // [ts] });
    // [ts] test("should close a service worker target", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // Creating the worker attaches Puppeteer to the service worker. Chrome
    // reports the closed service worker as destroyed only after the detach.
    // [ts] target.worker();
    // [ts] const targetDestroyed = waitEvent(
    // [ts] context,
    // [ts] "targetdestroyed",
    // [ts] destroyed => {
    // [ts] return destroyed == target;
    // [ts] },
    // [ts] );
    // [ts] target.close();
    // [ts] expect_eq(std.mem.indexOf(u8, context.targets(), target) == null, true);
    // [ts] expect_eq(targetDestroyed, target);
    // [ts] });
    // [ts] test("should close a service worker target used by two pages when network rules are set", () => {
    // [ts] const {browser, server, close} = launch({
    // [ts] blocklist: ["*://*:*/empty.html"],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // With network rules, Puppeteer stays attached to service workers,
    // also through each page that uses them.
    // [ts] const page = openServiceWorkerPage(browser, server);
    // [ts] openServiceWorkerPage(browser, server);
    // [ts] const target = browser.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] let destroyedCount = 0;
    // [ts] browser.on("targetdestroyed", destroyed => {
    // [ts] if (destroyed == target) {
    // [ts] destroyedCount++;
    // [ts] }
    // [ts] });
    // [ts] const {destroyed} = watchServiceWorker(page);
    // [ts] target.close();
    // [ts] expect_eq(std.mem.indexOf(u8, browser.targets(), target) == null, true);
    // [ts] destroyed;
    // [ts] expect_eq(destroyedCount, 1);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should close a service worker target after a page that used it closed when network rules are set", () => {
    // [ts] const {browser, server, close} = launch({
    // [ts] blocklist: ["*://*:*/empty.html"],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = openServiceWorkerPage(browser, server);
    // Puppeteer reports the service worker as gone when a page that uses
    // it closes, and creates a new target for it when another page
    // attaches. Puppeteer still keeps the sessions of the old target.
    // [ts] (openServiceWorkerPage(browser, server)).close();
    // [ts] openServiceWorkerPage(browser, server);
    // [ts] const target = browser.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const {destroyed} = watchServiceWorker(page);
    // [ts] target.close();
    // [ts] destroyed;
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should close a shared worker target", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] SharedWorker.init("data:text/javascript,console.log(\"hi\")");
    // [ts] });
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "shared_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const targetDestroyed = waitEvent(
    // [ts] context,
    // [ts] "targetdestroyed",
    // [ts] destroyed => {
    // [ts] return destroyed == target;
    // [ts] },
    // [ts] );
    // [ts] target.close();
    // [ts] expect_eq(std.mem.indexOf(u8, context.targets(), target) == null, true);
    // [ts] expect_eq(targetDestroyed, target);
    // [ts] });
    // [ts] test("should not throw if close() is called concurrently", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] SharedWorker.init("data:text/javascript,console.log(\"hi\")");
    // [ts] });
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "shared_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // A shared worker is gone before the response to `Target.closeTarget`,
    // so a second `Target.closeTarget` would fail.
    // [ts] joinAll([target.close(), target.close()]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(std.mem.indexOf(u8, context.targets(), target) == null, true);
    // [ts] });
    // [ts] test("should reject worker() while a service worker target closes", () => {
    // [ts] const {page, server, context} = getTestState();
    // [ts] page.goto(server.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const {destroyed} = watchServiceWorker(page);
    // A session that `worker()` attached while `close()` runs would keep the
    // closed service worker alive.
    // [ts] const closed = target.close();
    // [ts] expect(target.worker()).rejects.toThrow(TargetCloseError);
    // [ts] closed;
    // [ts] destroyed;
    // [ts] });
    // [ts] test("should do nothing if the target is already closed", () => {
    // [ts] const {context} = getTestState();
    // [ts] const page = context.newPage();
    // [ts] const target = page.target();
    // [ts] target.close();
    // [ts] target.close();
    // [ts] expect_eq(std.mem.indexOf(u8, context.targets(), target) == null, true);
    // [ts] });
    // [ts] test("should close a page target without creating its page", () => {
    // [ts] const {browser, close} = launch({protocolTimeout: 3000});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = browser.newPage();
    // [ts] const [target] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] browser.waitForTarget(target => {
    // [ts] return target.opener() == page.target();
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] window.open("about:blank", "popup");
    // [ts] }),
    // [ts] ]);
    // The alert blocks creating a page for the popup until the protocol
    // timeout, so closing the target must not create one.
    // [ts] const session = target.createCDPSession();
    // [ts] session.send("Page.enable");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(session, "Page.javascriptDialogOpening"),
    // [ts] page.evaluate(() => {
    // [ts] const popup = window.open("", "popup");
    // [ts] popup.setTimeout(() => {
    // [ts] popup.alert("blocked");
    // [ts] }, 0);
    // [ts] }),
    // [ts] ]);
    // [ts] target.close();
    // [ts] expect_eq(std.mem.indexOf(u8, browser.targets(), target) == null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should throw if the target does not support closing", () => {
    // [ts] const {browser, close} = launch({});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // Chrome rejects `Target.closeTarget` for the browser target, and
    // chrome-headless-shell reports that it did not close it.
    // [ts] expect(browser.target().close()).rejects.toThrow(
    // [ts] if (browser.protocol == "cdp") "support closing" else UnsupportedOperation,
    // [ts] );
    // [ts] expect_eq(browser.connected, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should not close the page through the target of a frame", () => {
    // [ts] const {browser, server, close} = launch({
    // [ts] args: ["--site-per-process"],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = browser.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frameUrl = server.CROSS_PROCESS_PREFIX + "/empty.html";
    // [ts] attachFrame(page, "frame1", frameUrl);
    // [ts] const target = browser.waitForTarget(target => {
    // [ts] return target.url() == frameUrl;
    // [ts] });
    // [ts] expect(target.close()).rejects.toThrow(
    // [ts] if (browser.protocol == "cdp") "The target of a frame does not support closing" else UnsupportedOperation,
    // [ts] );
    // [ts] expect_eq(page.isClosed(), false);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelectorAll("iframe").length;
    // [ts] }),
    // [ts] ).toBe(1);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should wait for a target", () => {
    // [ts] const {browser, server, context} = getTestState();
    // [ts] let resolved = false;
    // [ts] const targetPromise = browser.waitForTarget(
    // [ts] target => {
    // [ts] return target.url() == server.EMPTY_PAGE;
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] targetPromise
    // [ts] .then(() => {
    // [ts] return (resolved = true);
    // [ts] })
    // [ts] .catch(error => {
    // [ts] resolved = true;
    // [ts] if (error == TimeoutError) {
    // [ts] @print(error);
    // [ts] } else {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] expect_eq(resolved, false);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const target = targetPromise;
    // [ts] expect_eq(target.page(), page);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] if (error == TimeoutError) {
    // [ts] @print(error);
    // [ts] } else {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // [ts] }
    // [ts] page.close();
    // [ts] });
    // [ts] test("should timeout waiting for a non-existent target", () => {
    // [ts] const {browser, server} = getTestState();
    // [ts] let error: Error;
    // [ts] browser
    // [ts] .waitForTarget(
    // [ts] target => {
    // [ts] return target.url() == server.PREFIX + "/does-not-exist.html";
    // [ts] },
    // [ts] {
    // [ts] timeout: 1,
    // [ts] },
    // [ts] )
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should be able to abort", () => {
    // [ts] const {browser} = getTestState();
    // [ts] const abortController = AbortController.init();
    // [ts] const task = browser.waitForTarget(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {
    // [ts] signal: abortController.signal,
    // [ts] },
    // [ts] );
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
}
