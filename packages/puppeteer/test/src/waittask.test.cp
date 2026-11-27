// Migrated from TypeScript (src/waittask.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { TimeoutError, ElementHandle } from "puppeteer";
import { isErrorLike } from "puppeteer-core/internal/util/ErrorLike.js";

import { createTimeout, getTestState, launch, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, detachFrame, html, htmlRaw } from "./utils.cp";

// [cap-test] describe("waittask specs", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Frame.waitForFunction", () => {
    // TODO(cap-migrate): refine TS statement
    // [ts] for (const polling of ["raf", "mutation", 10]) { it(`should not poll after timing out during ${polling} poller creation`, () => { const {page} = getTestState(); page.evaluate(() => { (globalThis).__pollCalls = 0; }); const blocking = page.evaluate(() => { const deadline = performance.now() + 300; while (performance.now() < deadline) { // Keep poller creation queued while the Node-side timeout fires. } }); await expect( page.waitForFunction( () => { ++(globalThis as any).__pollCalls; return false; }, {polling, timeout: 50}, ), ).rejects.toThrow(TimeoutError); await blocking; const calls = await page.evaluate(() => { return (globalThis as any).__pollCalls; }); await page.evaluate(async () => {
    // [ts] document.body.append(document.createElement("div"));
    // [ts] Promise.init(resolve => {
    // [ts] setTimeout(resolve, 100);
    // [ts] });
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__pollCalls;
    // [ts] }),
    // [ts] ).toBe(calls);
    // [ts] expect_eq(calls, 0);
    // [ts] });
    // [ts] }
    // [ts] test("should accept a string", () => {
    // [ts] const {page} = getTestState();
    // [ts] const watchdog = page.waitForFunction("self.__FOO === 1");
    // [ts] page.evaluate(() => {
    // [ts] return ((self).__FOO = 1);
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should work when resolved right before execution context disposal", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluateOnNewDocument(() => {
    // [ts] return ((globalThis).__RELOADED = true);
    // [ts] });
    // [ts] page.waitForFunction(() => {
    // [ts] if (!(globalThis).__RELOADED) { window.location.reload(); return false; }
    // [ts] return true;
    // [ts] });
    // [ts] });
    // [ts] test("should poll on interval", () => {
    // [ts] const {page} = getTestState();
    // [ts] const startTime = Date.now();
    // [ts] const polling = 100;
    // [ts] const watchdog = page.waitForFunction(
    // [ts] () => {
    // [ts] return (globalThis).__FOO == "hit";
    // [ts] },
    // [ts] {polling},
    // [ts] );
    // [ts] page.evaluate(() => {
    // [ts] setTimeout(() => {
    // [ts] (globalThis).__FOO = "hit";
    // [ts] }, 50);
    // [ts] });
    // [ts] watchdog;
    // [ts] expect_eq(Date.now() - startTime >= polling / 2, true);
    // [ts] });
    // [ts] test("should poll on mutation", () => {
    // [ts] const {page} = getTestState();
    // [ts] let success = false;
    // [ts] const watchdog = page
    // [ts] .waitForFunction(
    // [ts] () => {
    // [ts] return (globalThis).__FOO == "hit";
    // [ts] },
    // [ts] {
    // [ts] polling: "mutation",
    // [ts] },
    // [ts] )
    // [ts] .then(() => {
    // [ts] return (success = true);
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__FOO = "hit");
    // [ts] });
    // [ts] expect_eq(success, false);
    // [ts] page.evaluate(() => {
    // [ts] return document.body.appendChild(document.createElement("div"));
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should poll on mutation async", () => {
    // [ts] const {page} = getTestState();
    // [ts] let success = false;
    // [ts] const watchdog = page
    // [ts] .waitForFunction(
    // [ts] () => {
    // [ts] return (globalThis).__FOO == "hit";
    // [ts] },
    // [ts] {
    // [ts] polling: "mutation",
    // [ts] },
    // [ts] )
    // [ts] .then(() => {
    // [ts] return (success = true);
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__FOO = "hit");
    // [ts] });
    // [ts] expect_eq(success, false);
    // [ts] page.evaluate(() => {
    // [ts] return document.body.appendChild(document.createElement("div"));
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should poll on raf", () => {
    // [ts] const {page} = getTestState();
    // [ts] const watchdog = page.waitForFunction(
    // [ts] () => {
    // [ts] return (globalThis).__FOO == "hit";
    // [ts] },
    // [ts] {
    // [ts] polling: "raf",
    // [ts] },
    // [ts] );
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__FOO = "hit");
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should poll on raf async", () => {
    // [ts] const {page} = getTestState();
    // [ts] const watchdog = page.waitForFunction(
    // [ts] () => {
    // [ts] return (globalThis).__FOO == "hit";
    // [ts] },
    // [ts] {
    // [ts] polling: "raf",
    // [ts] },
    // [ts] );
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__FOO = "hit");
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should work with strict CSP policy", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setCSP("/empty.html", "script-src " + server.PREFIX);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error: Error;
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page
    // [ts] .waitForFunction(
    // [ts] () => {
    // [ts] return (globalThis).__FOO == "hit";
    // [ts] },
    // [ts] {
    // [ts] polling: "raf",
    // [ts] },
    // [ts] )
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__FOO = "hit");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(error == null, true);
    // [ts] });
    // [ts] test("should throw negative polling interval", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.waitForFunction(
    // [ts] () => {
    // [ts] return !!document.body;
    // [ts] },
    // [ts] {polling: -10},
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] if (isErrorLike(error_)) {
    // [ts] error = error_;
    // [ts] }
    // [ts] }
    // [ts] expect(error?.message).toContain(
    // [ts] "Cannot poll with non-positive interval",
    // [ts] );
    // [ts] });
    // [ts] test("should return the success value as a JSHandle", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] (
    // [ts] page.waitForFunction(() => {
    // [ts] return 5;
    // [ts] })
    // [ts] ).jsonValue(),
    // [ts] ).toBe(5);
    // [ts] });
    // [ts] test("should return the window as a success value", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.waitForFunction(() => {
    // [ts] return window;
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] });
    // [ts] test("should accept ElementHandle arguments", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div></div>`);
    // [ts] using div = (page.$("div"));
    // [ts] let resolved = false;
    // [ts] const waitForFunction = page
    // [ts] .waitForFunction(
    // [ts] element => {
    // [ts] return element.localName == "div" && !element.parentElement;
    // [ts] },
    // [ts] {},
    // [ts] div,
    // [ts] )
    // [ts] .then(() => {
    // [ts] return (resolved = true);
    // [ts] });
    // [ts] expect_eq(resolved, false);
    // [ts] page.evaluate(element => {
    // [ts] return element.remove();
    // [ts] }, div);
    // [ts] waitForFunction;
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .waitForFunction(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {timeout: 10},
    // [ts] )
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] expect_eq(std.mem.indexOf(u8, error?.message, "Waiting failed: 10ms exceeded") != null, true);
    // [ts] });
    // [ts] test("should respect default timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setDefaultTimeout(1);
    // [ts] let error: Error;
    // [ts] page
    // [ts] .waitForFunction(() => {
    // [ts] return false;
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] expect_eq(std.mem.indexOf(u8, error?.message, "Waiting failed: 1ms exceeded") != null, true);
    // [ts] });
    // [ts] test("should disable timeout when its set to 0", () => {
    // [ts] const {page} = getTestState();
    // [ts] const watchdog = page.waitForFunction(
    // [ts] () => {
    // [ts] (globalThis).__counter =
    // [ts] ((globalThis).__counter || 0) + 1;
    // [ts] return (globalThis).__injected;
    // [ts] },
    // [ts] {timeout: 0, polling: 10},
    // [ts] );
    // [ts] page.waitForFunction(() => {
    // [ts] return (globalThis).__counter > 10;
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__injected = true);
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should survive cross-process navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let fooFound = false;
    // [ts] const waitForFunction = page
    // [ts] .waitForFunction(() => {
    // [ts] return (globalThis).__FOO == 1;
    // [ts] })
    // [ts] .then(() => {
    // [ts] return (fooFound = true);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(fooFound, false);
    // [ts] page.reload();
    // [ts] expect_eq(fooFound, false);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] expect_eq(fooFound, false);
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__FOO = 1);
    // [ts] });
    // [ts] waitForFunction;
    // [ts] expect_eq(fooFound, true);
    // [ts] });
    // [ts] test("should survive navigations", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const watchdog = page.waitForFunction(() => {
    // [ts] return (globalThis).__done;
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.goto(server.PREFIX + "/consolelog.html");
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).__done = true);
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should be cancellable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForFunction(
    // [ts] () => {
    // [ts] return (globalThis).__done;
    // [ts] },
    // [ts] {
    // [ts] signal: abortController.signal,
    // [ts] },
    // [ts] );
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should reject at once when the signal is already aborted", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.waitForFunction(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {signal: AbortSignal.abort()},
    // [ts] ),
    // [ts] ).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should reject with falsy abort reasons", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(
    // [ts] race([ // migrated: Promise.race -> race
    // [ts] page.waitForFunction(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {signal: AbortSignal.abort(null)},
    // [ts] ),
    // [ts] createTimeout(300, "hung"),
    // [ts] ]),
    // [ts] ).rejects.toBe(null);
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForFunction(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {signal: abortController.signal},
    // [ts] );
    // [ts] abortController.abort(0);
    // [ts] expect(
    // [ts] race([task, createTimeout(300, "hung")]), // migrated: Promise.race -> race
    // [ts] ).rejects.toBe(0);
    // [ts] });
    // [ts] test("can start multiple tasks without node warnings", () => {
    // [ts] const {page} = getTestState();
    // [ts] let warning: Error | null;
    // [ts] const warningHandler: NodeJS.WarningListener = w => {
    // [ts] warning = w;
    // [ts] };
    // [ts] process.on("warning", warningHandler);
    // [ts] process.setMaxListeners(1);
    // [ts] const abortController = AbortController.init();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] page.waitForFunction(
    // [ts] () => {
    // [ts] return true;
    // [ts] },
    // [ts] {
    // [ts] signal: abortController.signal,
    // [ts] },
    // [ts] );
    // [ts] }
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] process.setMaxListeners(10);
    // [ts] }
    // [ts] process.off("warning", warningHandler);
    // [ts] expect_eq(warning?.stack, null);
    // [ts] });
    // [ts] });

  // [cap-test] describe("Frame.waitForSelector", () => {
    const addElement = (tag) => {
      return document.body.appendChild(document.createElement(tag));
    };








    const addShadowHost = (tag) => {
      document.body
        .appendChild(document.createElement(tag))
        .attachShadow({mode: "open"});
    };

    const addElementToShadowRoot = (selector, tag) => {
      const element = document.createElement(tag);
      element.textContent = "inside";
      document.querySelector(selector).shadowRoot.appendChild(element);
    };




    // Attaching a shadow root to a node that is already in the DOM does not
    // produce a mutation, so MutationPoller has nothing to react to.
    // See https://github.com/whatwg/dom/issues/1287.






    // [cap-test] describe("xpath", () => {
      const addElement = (tag) => {
        return document.body.appendChild(document.createElement(tag));
      };

    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("protocol timeout", () => {
  // [cap-test] });
// [cap-test] });
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should immediately resolve promise if node exists", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] frame.waitForSelector("*");
    // [ts] frame.evaluate(addElement, "div");
    // [ts] frame.waitForSelector("div");
    // [ts] });
    // [ts] test("should be cancellable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForSelector("wrong", {
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should preserve custom Error and AbortSignal.timeout abort reasons", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const reason = Error.init("custom abort reason");
    // [ts] expect(
    // [ts] page.waitForSelector("wrong", {
    // [ts] signal: AbortSignal.abort(reason),
    // [ts] }),
    // [ts] ).rejects.toBe(reason);
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForSelector("wrong", {
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] setTimeout(() => {
    // [ts] abortController.abort(reason);
    // [ts] }, 10);
    // [ts] expect(task).rejects.toBe(reason);
    // [ts] const timeoutSignal = AbortSignal.timeout(10);
    // [ts] const error = page
    // [ts] .waitForSelector("wrong", {
    // [ts] signal: timeoutSignal,
    // [ts] })
    // [ts] .catch(err => {
    // [ts] return err;
    // [ts] });
    // [ts] expect_eq(error, timeoutSignal.reason);
    // [ts] expect_eq((error).name, "TimeoutError");
    // [ts] });
    // [ts] test("should reject with falsy abort reasons", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(
    // [ts] race([ // migrated: Promise.race -> race
    // [ts] page.waitForSelector("wrong", {
    // [ts] signal: AbortSignal.abort(null),
    // [ts] }),
    // [ts] createTimeout(300, "hung"),
    // [ts] ]),
    // [ts] ).rejects.toBe(null);
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForSelector("wrong", {
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] setTimeout(() => {
    // [ts] abortController.abort(0);
    // [ts] }, 20);
    // [ts] expect(
    // [ts] race([task, createTimeout(300, "hung")]), // migrated: Promise.race -> race
    // [ts] ).rejects.toBe(0);
    // [ts] });
    // [ts] test("should work with removed MutationObserver", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // @ts-expect-error We want to remove it for the test.
    // [ts] delete window.MutationObserver;
    // [ts] });
    // [ts] const [handle] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForSelector(".zombo"),
    // [ts] page.setContent(html`<div class="zombo">anything</div>`),
    // [ts] ]);
    // [ts] expect(
    // [ts] page.evaluate(x => {
    // [ts] return x?.textContent;
    // [ts] }, handle),
    // [ts] ).toBe("anything");
    // [ts] });
    // [ts] test("should resolve promise when node is added", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = page.mainFrame();
    // [ts] const watchdog = frame.waitForSelector("div");
    // [ts] frame.evaluate(addElement, "br");
    // [ts] frame.evaluate(addElement, "div");
    // [ts] using eHandle = (watchdog);
    // [ts] const tagName = (eHandle.getProperty("tagName")).jsonValue();
    // [ts] expect_eq(tagName, "DIV");
    // [ts] });
    // [ts] test("should work when node is added through innerHTML", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watchdog = page.waitForSelector("h3 div");
    // [ts] page.evaluate(addElement, "span");
    // [ts] page.evaluate(() => {
    // [ts] return (document.querySelector("span").innerHTML =
    // [ts] "<h3><div></div></h3>");
    // [ts] });
    // [ts] watchdog;
    // [ts] });
    // [ts] test("should work when node is added in a shadow root", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watcher = page.waitForSelector("div >>> h1");
    // [ts] page.evaluate(addShadowHost, "div");
    // [ts] expect(
    // [ts] race([watcher, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] page.evaluate(addElementToShadowRoot, "div", "h1");
    // [ts] using element = watcher;
    // [ts] expect(
    // [ts] element.evaluate(el => {
    // [ts] return el.textContent;
    // [ts] }),
    // [ts] ).toBe("inside");
    // [ts] });
    // [ts] test("should work when node is added in a shadow root that predates the wait", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(addShadowHost, "div");
    // [ts] const watcher = page.waitForSelector("div >>> h1");
    // [ts] expect(
    // [ts] race([watcher, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] page.evaluate(addElementToShadowRoot, "div", "h1");
    // [ts] using element = watcher;
    // [ts] expect(
    // [ts] element.evaluate(el => {
    // [ts] return el.textContent;
    // [ts] }),
    // [ts] ).toBe("inside");
    // [ts] });
    // [ts] test("should work when node is added in a nested shadow root", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watcher = page.waitForSelector("div >>> h1");
    // [ts] page.evaluate(() => {
    // [ts] const host = document.body.appendChild(document.createElement("div"));
    // [ts] const inner = document.createElement("section");
    // [ts] inner.attachShadow({mode: "open"});
    // [ts] host.attachShadow({mode: "open"}).appendChild(inner);
    // [ts] });
    // [ts] expect(
    // [ts] race([watcher, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] page.evaluate(() => {
    // [ts] const h1 = document.createElement("h1");
    // [ts] h1.textContent = "inside";
    // [ts] document
    // [ts] .querySelector("div")
    // [ts] .shadowRoot.querySelector("section")
    // [ts] .shadowRoot.appendChild(h1);
    // [ts] });
    // [ts] using element = watcher;
    // [ts] expect(
    // [ts] element.evaluate(el => {
    // [ts] return el.textContent;
    // [ts] }),
    // [ts] ).toBe("inside");
    // [ts] });
    // [ts] test("should work when a shadow root is attached to an existing node", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watcher = page.waitForSelector("div >>> h1");
    // [ts] page.evaluate(addElement, "div");
    // [ts] expect(
    // [ts] race([watcher, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] page.evaluate(() => {
    // [ts] const host = document.querySelector("div");
    // [ts] const shadow = host.attachShadow({mode: "open"});
    // [ts] const h1 = document.createElement("h1");
    // [ts] h1.textContent = "inside";
    // [ts] shadow.appendChild(h1);
    // [ts] });
    // [ts] using element = watcher;
    // [ts] expect(
    // [ts] element.evaluate(el => {
    // [ts] return el.textContent;
    // [ts] }),
    // [ts] ).toBe("inside");
    // [ts] });
    // [ts] test("should work for selector with a pseudo class", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const watchdog = page.waitForSelector("input:focus");
    // [ts] expect(
    // [ts] race([watchdog, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] page.setContent(html`<input></input>`);
    // [ts] page.click("input");
    // [ts] watchdog;
    // [ts] });
    // [ts] test("Page.waitForSelector is shortcut for main frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const otherFrame = page.frames()[1];
    // [ts] const watchdog = page.waitForSelector("div");
    // [ts] otherFrame.evaluate(addElement, "div");
    // [ts] page.evaluate(addElement, "div");
    // [ts] using eHandle = watchdog;
    // [ts] expect_eq(eHandle?.frame, page.mainFrame());
    // [ts] });
    // [ts] test("should run in specified frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] const frame2 = page.frames()[2];
    // [ts] const waitForSelectorPromise = frame2.waitForSelector("div");
    // [ts] frame1.evaluate(addElement, "div");
    // [ts] frame2.evaluate(addElement, "div");
    // [ts] using eHandle = waitForSelectorPromise;
    // [ts] expect_eq(eHandle?.frame, frame2);
    // [ts] });
    // [ts] test("should throw when frame is detached", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] let waitError: Error | null;
    // [ts] const waitPromise = frame.waitForSelector(".box").catch(error => {
    // [ts] return (waitError = error);
    // [ts] });
    // [ts] detachFrame(page, "frame1");
    // [ts] waitPromise;
    // [ts] expect_eq(waitError, true);
    // [ts] expect_eq(waitError?.message, "Waiting for selector `.box` failed");
    // [ts] });
    // [ts] test("should survive cross-process navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let boxFound = false;
    // [ts] const waitForSelector = page.waitForSelector(".box").then(() => {
    // [ts] return (boxFound = true);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(boxFound, false);
    // [ts] page.reload();
    // [ts] expect_eq(boxFound, false);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] waitForSelector;
    // [ts] expect_eq(boxFound, true);
    // [ts] });
    // [ts] test("should wait for element to be visible (display)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {visible: true});
    // [ts] page.setContent(html`<div style="display: none">text</div>`);
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] e.style.removeProperty("display");
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be visible (without DOM mutations)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {visible: true});
    // [ts] page.setContent(
    // [ts] html`<style>
    // [ts] div {
    // [ts] display: none;
    // [ts] }
    // [ts] </style>
    // [ts] <div>text</div>`,
    // [ts] );
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect_eq(element, true);
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] page.evaluate(() => {
    // [ts] const extraSheet = CSSStyleSheet.init();
    // [ts] extraSheet.replaceSync("div { display: block; }");
    // [ts] document.adoptedStyleSheets = [
    // [ts] ...document.adoptedStyleSheets,
    // [ts] extraSheet,
    // [ts] ];
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be visible (visibility)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {visible: true});
    // [ts] page.setContent(html`<div style="visibility: hidden">text</div>`);
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] e.style.setProperty("visibility", "collapse");
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] e.style.removeProperty("visibility");
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be visible (bounding box)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {visible: true});
    // [ts] page.setContent(html`<div style="width: 0">text</div>`);
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] e.style.setProperty("height", "0");
    // [ts] e.style.removeProperty("width");
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] e.style.removeProperty("height");
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be visible recursively", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div#inner", {
    // [ts] visible: true,
    // [ts] });
    // [ts] page.setContent(
    // [ts] html`<div style="display: none; visibility: hidden;">
    // [ts] <div id="inner">hi</div>
    // [ts] </div>`,
    // [ts] );
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] return e.style.removeProperty("display");
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] return e.style.removeProperty("visibility");
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be hidden (visibility)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {hidden: true});
    // [ts] page.setContent(html`<div style="display: block;">text</div>`);
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] return e.style.setProperty("visibility", "hidden");
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be hidden (display)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {hidden: true});
    // [ts] page.setContent(html`<div style="display: block;">text</div>`);
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] return e.style.setProperty("display", "none");
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be hidden (bounding box)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {hidden: true});
    // [ts] page.setContent(html`<div>text</div>`);
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeFalsy();
    // [ts] element.evaluate(e => {
    // [ts] e.style.setProperty("height", "0");
    // [ts] });
    // [ts] expect(promise).resolves.toBeTruthy();
    // [ts] });
    // [ts] test("should wait for element to be hidden (removal)", () => {
    // [ts] const {page} = getTestState();
    // [ts] const promise = page.waitForSelector("div", {hidden: true});
    // [ts] page.setContent(html`<div>text</div>`);
    // [ts] using element = page.evaluateHandle(() => {
    // [ts] return document.getElementsByTagName("div")[0];
    // [ts] });
    // [ts] expect(
    // [ts] race([promise, createTimeout(40, true)]), // migrated: Promise.race -> race
    // [ts] ).resolves.toBeTruthy();
    // [ts] element.evaluate(e => {
    // [ts] e.remove();
    // [ts] });
    // [ts] expect(promise).resolves.toBeFalsy();
    // [ts] });
    // [ts] test("should return null if waiting to hide non-existing element", () => {
    // [ts] const {page} = getTestState();
    // [ts] using handle = page.waitForSelector("non-existing", {
    // [ts] hidden: true,
    // [ts] });
    // [ts] expect_eq(handle, null);
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.waitForSelector("div", {timeout: 10}).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] expect_eq(error?.message, "Waiting for selector `div` failed");
    // [ts] });
    // [ts] test("should have an error message specifically for awaiting an element to be hidden", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>text</div>`);
    // [ts] let error: Error;
    // [ts] page
    // [ts] .waitForSelector("div", {hidden: true, timeout: 10})
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(error?.message, "Waiting for selector `div` failed");
    // [ts] });
    // [ts] test("should respond to node attribute mutation", () => {
    // [ts] const {page} = getTestState();
    // [ts] let divFound = false;
    // [ts] const waitForSelector = page.waitForSelector(".zombo").then(() => {
    // [ts] return (divFound = true);
    // [ts] });
    // [ts] page.setContent(html`<div class="notZombo"></div>`);
    // [ts] expect_eq(divFound, false);
    // [ts] page.evaluate(() => {
    // [ts] return (document.querySelector("div").className = "zombo");
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] });
    // [ts] test("should return the element handle", () => {
    // [ts] const {page} = getTestState();
    // [ts] const waitForSelector = page.waitForSelector(".zombo");
    // [ts] page.setContent(html`<div class="zombo">anything</div>`);
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] x => {
    // [ts] return x?.textContent;
    // [ts] },
    // [ts] waitForSelector,
    // [ts] ),
    // [ts] ).toBe("anything");
    // [ts] });
    // [ts] test("should have correct stack trace for timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.waitForSelector(".zombo", {timeout: 10}).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error?.stack, "Waiting for selector `.zombo` failed") != null, true);
    // The extension is ts here as Mocha maps back via sourcemaps.
    // [ts] expect_eq(std.mem.indexOf(u8, error?.stack, "waittask.test.ts") != null, true);
    // [ts] });
    // [ts] test("should support some fancy xpath", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(htmlRaw`<p>red herring</p><p>hello  world  </p>`);
    // [ts] const waitForSelector = page.waitForSelector(
    // [ts] "xpath/.//p[normalize-space(.)=\"hello world\"]",
    // [ts] );
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] x => {
    // [ts] return x?.textContent;
    // [ts] },
    // [ts] waitForSelector,
    // [ts] ),
    // [ts] ).toBe("hello  world  ");
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .waitForSelector("xpath/.//div", {timeout: 10})
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] expect_eq(error?.message, "Waiting for selector `.//div` failed");
    // [ts] });
    // [ts] test("should run in specified frame", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame2", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] const frame2 = page.frames()[2];
    // [ts] const waitForSelector = frame2.waitForSelector("xpath/.//div");
    // [ts] frame1.evaluate(addElement, "div");
    // [ts] frame2.evaluate(addElement, "div");
    // [ts] using eHandle = waitForSelector;
    // [ts] expect_eq(eHandle?.frame, frame2);
    // [ts] });
    // [ts] test("should throw when frame is detached", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] let waitError: Error | null;
    // [ts] const waitPromise = frame
    // [ts] .waitForSelector("xpath/.//*[@class=\"box\"]")
    // [ts] .catch(error => {
    // [ts] return (waitError = error);
    // [ts] });
    // [ts] detachFrame(page, "frame1");
    // [ts] waitPromise;
    // [ts] expect_eq(waitError, true);
    // [ts] expect(waitError?.message).toBe(
    // [ts] "Waiting for selector `.//*[@class=\"box\"]` failed",
    // [ts] );
    // [ts] });
    // [ts] test("hidden should wait for display: none", () => {
    // [ts] const {page} = getTestState();
    // [ts] let divHidden = false;
    // [ts] page.setContent(html`<div style="display: block;">text</div>`);
    // [ts] const waitForSelector = page
    // [ts] .waitForSelector("xpath/.//div", {hidden: true})
    // [ts] .then(() => {
    // [ts] return (divHidden = true);
    // [ts] });
    // [ts] page.waitForSelector("xpath/.//div"); // do a round trip
    // [ts] expect_eq(divHidden, false);
    // [ts] page.evaluate(() => {
    // [ts] return document
    // [ts] .querySelector("div")
    // [ts] ?.style.setProperty("display", "none");
    // [ts] });
    // [ts] expect_eq(waitForSelector, true);
    // [ts] expect_eq(divHidden, true);
    // [ts] });
    // [ts] test("hidden should return null if the element is not found", () => {
    // [ts] const {page} = getTestState();
    // [ts] using waitForSelector = page.waitForSelector("xpath/.//div", {
    // [ts] hidden: true,
    // [ts] });
    // [ts] expect_eq(waitForSelector, null);
    // [ts] });
    // [ts] test("hidden should return an empty element handle if the element is found", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div style="display: none;">text</div>`);
    // [ts] using waitForSelector = page.waitForSelector("xpath/.//div", {
    // [ts] hidden: true,
    // [ts] });
    // [ts] expect(waitForSelector).toBeInstanceOf(ElementHandle);
    // [ts] });
    // [ts] test("should return the element handle", () => {
    // [ts] const {page} = getTestState();
    // [ts] const waitForSelector = page.waitForSelector(
    // [ts] "xpath/.//*[@class=\"zombo\"]",
    // [ts] );
    // [ts] page.setContent(html`<div class="zombo">anything</div>`);
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] x => {
    // [ts] return x?.textContent;
    // [ts] },
    // [ts] waitForSelector,
    // [ts] ),
    // [ts] ).toBe("anything");
    // [ts] });
    // [ts] test("should allow you to select a text node", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>some text</div>`);
    // [ts] using text = page.waitForSelector("xpath/.//div/text()");
    // [ts] expect((text.getProperty("nodeType")).jsonValue()).toBe(
    // [ts] 3 /* Node.TEXT_NODE */,
    // [ts] );
    // [ts] });
    // [ts] test("should allow you to select an element with single slash", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div>some text</div>`);
    // [ts] const waitForSelector = page.waitForSelector("xpath/html/body/div");
    // [ts] expect(
    // [ts] page.evaluate(
    // [ts] x => {
    // [ts] return x?.textContent;
    // [ts] },
    // [ts] waitForSelector,
    // [ts] ),
    // [ts] ).toBe("some text");
    // [ts] });
    // [ts] test("should error if underlying protocol command times out with raf polling", () => {
    // [ts] const {page, close} = launch(
    // [ts] {
    // [ts] protocolTimeout: 5000,
    // [ts] },
    // [ts] {
    // [ts] createContext: true,
    // [ts] createPage: true,
    // [ts] },
    // [ts] );
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] let error: Error;
    // [ts] page
    // [ts] .waitForFunction(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {timeout: 6000},
    // [ts] )
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] expect_eq(error.message, "Waiting failed");
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, "waittask.test.ts") != null, true);
    // [ts] expect(error.cause).toBeInstanceOf(Error);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
}
