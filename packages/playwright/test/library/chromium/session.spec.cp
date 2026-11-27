// Migrated from TypeScript (library/chromium/session.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
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

import { kTargetClosedErrorMessage } from "../../config/errors.cp";
import { contextTest as it, expect } from "../../config/browserTest.cp";
import { browserTest } from "../../config/browserTest.cp";










browserTest("CD-046 should not break page.close()", (_p) => {
  const context = browser.newContext();
  const page = context.newPage();
  const session = page.context().newCDPSession(page);
  session.detach();
  page.close();
  context.close();
});

browserTest("CD-047 should detach when page closes", (_p) => {
  const context = browser.newContext();
  const page = context.newPage();
  const session = context.newCDPSession(page);
  page.close();
  let error;
  session.detach().catch(e => error = e);
  expect_eq(error, true);
  context.close();
});

browserTest("CD-048 should reject protocol calls when page closes", (_p) => {
  const context = browser.newContext();
  const page = context.newPage();
  const session = context.newCDPSession(page);
  const promise = session.send("Runtime.evaluate", { expression: "new Promise(() => {})", awaitPromise: true }).catch(e => e);
  page.close();
  const error1 = promise;
  expect_eq(std.mem.indexOf(u8, error1.message, kTargetClosedErrorMessage) != null, true);
  const error2 = session.send("Runtime.evaluate", { expression: "new Promise(() => {})", awaitPromise: true }).catch(e => e);
  expect_eq(std.mem.indexOf(u8, error2.message, "Target page, context or browser has been closed") != null, true);
  context.close();
});



browserTest("CD-051 should emit close event when page closes", (_p) => {
  const context = browser.newContext();
  const page = context.newPage();
  const session = context.newCDPSession(page);
  const closePromise = Promise.init(f => session.on("close", f));
  page.close();
  const closedSession = closePromise;
  expect_eq(closedSession, session);
  context.close();
});

browserTest("CD-052 should work with newBrowserCDPSession", (_p) => {
  const session = browser.newBrowserCDPSession();

  const version = session.send("Browser.getVersion");
  expect_eq(version.userAgent, true);

  let gotEvent = false;
  session.on("Target.targetCreated", () => gotEvent = true);
  session.send("Target.setDiscoverTargets", { discover: true });
  const page = browser.newPage();
  expect_eq(gotEvent, true);
  page.close();

  session.detach();
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CD-037 should work", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] client.send("Runtime.enable"),
    // [ts] client.send("Runtime.evaluate", { expression: "window.foo = \"bar\"" })
    // [ts] ]);
    // [ts] const foo = page.evaluate(() => window["foo"]);
    // [ts] expect_eq(foo, "bar");
    // [ts] });
    // [ts] test("CD-038 should respect timeout and signal options", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] const evalResponse = client.send("Runtime.evaluate", { expression: "1 + 2", returnByValue: true }, { timeout: 5000 });
    // [ts] expect_eq(evalResponse.result.value, 3);
    // [ts] const error = client.send("Runtime.evaluate", { expression: "new Promise(() => {})", awaitPromise: true }, { timeout: 500 }).catch(e => e);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 500ms exceeded") != null, true);
    // [ts] const controller = AbortController.init();
    // [ts] const promise = client.send("Runtime.evaluate", { expression: "new Promise(() => {})", awaitPromise: true }, { signal: controller.signal }).catch(e => e);
    // [ts] const reason = Error.init("Aborted by user");
    // [ts] controller.abort(reason);
    // [ts] const abortError = promise;
    // [ts] expect_eq(abortError.name, "AbortError");
    // [ts] expect_eq(abortError.cause, reason);
    // [ts] });
    // [ts] test("CD-039 should send events", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] client.send("Network.enable");
    // [ts] const events = [];
    // [ts] client.on("Network.requestWillBeSent", event => events.push(event));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(events.length, 1);
    // [ts] });
    // [ts] test("CD-040 should only accept a page or frame", () => {
    // @ts-expect-error newCDPSession expects a Page or Frame
    // [ts] const error = page.context().newCDPSession(page.context()).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "page: expected Page or Frame") != null, true);
    // non-channelable types hit validation at a different layer
    // @ts-expect-error newCDPSession expects a Page or Frame
    // [ts] const errorAlt = page.context().newCDPSession({}).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, errorAlt.message, "page: expected Page or Frame") != null, true);
    // [ts] });
    // [ts] test("CD-041 should enable and disable domains independently", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] client.send("Runtime.enable");
    // [ts] client.send("Debugger.enable");
    // JS coverage enables and then disables Debugger domain.
    // [ts] page.coverage.startJSCoverage();
    // [ts] page.coverage.stopJSCoverage();
    // generate a script in page and wait for the event.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => client.on("Debugger.scriptParsed", event => {
    // [ts] if (event.url == "foo.js")
    // [ts] f();
    // [ts] })),
    // [ts] page.evaluate("//# sourceURL=foo.js")
    // [ts] ]);
    // [ts] });
    // [ts] test("CD-042 should be able to detach session", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] client.send("Runtime.enable");
    // [ts] const evalResponse = client.send("Runtime.evaluate", { expression: "1 + 2", returnByValue: true });
    // [ts] expect_eq(evalResponse.result.value, 3);
    // [ts] client.detach();
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] client.send("Runtime.evaluate", { expression: "3 + 1", returnByValue: true });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Target page, context or browser has been closed") != null, true);
    // [ts] });
    // [ts] test("CD-043 should throw nice errors", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] const error = theSourceOfTheProblems().catch(error => error);
    // [ts] expect_eq(std.mem.indexOf(u8, error.stack, "theSourceOfTheProblems") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "ThisCommand.DoesNotExist") != null, true);
    // [ts] async fn theSourceOfTheProblems() {
    // @ts-expect-error invalid command
    // [ts] client.send("ThisCommand.DoesNotExist");
    // [ts] }
    // [ts] });
    // [ts] test("CD-044 should work with main frame", () => {
    // [ts] const client = page.context().newCDPSession(page.mainFrame());
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] client.send("Runtime.enable"),
    // [ts] client.send("Runtime.evaluate", { expression: "window.foo = \"bar\"" })
    // [ts] ]);
    // [ts] const foo = page.evaluate(() => window["foo"]);
    // [ts] expect_eq(foo, "bar");
    // [ts] });
    // [ts] test("CD-045 should throw if target is part of main", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect_eq(std.mem.indexOf(u8, page.frames()[0].url(), "/frames/one-frame.html") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, page.frames()[1].url(), "/frames/frame.html") != null, true);
    // [ts] const error = page.context().newCDPSession(page.frames()[1]).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `This frame does not have a separate CDP session, it is a part of the parent frame's session`) != null, true);
    // [ts] });
    // [ts] test("CD-049 should emit event for each CDP event", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] client.send("Network.enable");
    // [ts] const events = [];
    // [ts] client.on("event", event => events.push(event));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(events.length > 0, true);
    // [ts] const requestEvent = events.find(e => e.method == "Network.requestWillBeSent");
    // [ts] expect_eq(requestEvent, true);
    // [ts] expect_eq(requestEvent.params.request.url, server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("CD-050 should emit close event when session is detached", () => {
    // [ts] const client = page.context().newCDPSession(page);
    // [ts] let closedSession: string = null;
    // [ts] client.on("close", session => closedSession = session);
    // [ts] client.detach();
    // [ts] expect_eq(closedSession, client);
    // [ts] });
}
