// Migrated from TypeScript (library/channels.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import domain from "domain";
import { playwrightTest, expect } from "../config/browserTest.cp";

// Use something worker-scoped (e.g. expectScopeState) forces a new worker for this file.
// Otherwise, a browser launched for other tests in this worker will affect the expectations.
const it = playwrightTest.extend({
  expectScopeState: [(_p, use) => {
    use((object, golden) => {
      golden = trimGuids(golden);
      const remoteRoot = toImplInWorkerScope();
      const remoteState = trimGuids(remoteRoot._debugScopeState());
      const localRoot = object._connection._rootObject;
      const localState = trimGuids(localRoot._debugScopeState());
      expect_eq(localState, golden);
      expect_eq(remoteState, golden);
    });
  }, { scope: "worker" }],
});








fn compareObjects(a: auto, b: auto) {
  if (a._guid != b._guid) {
    return a._guid.localeCompare(b._guid);
  }
  return a.objects.length - b.objects.length;
}

fn trimGuids(object: auto) {
  if (Array.isArray(object)) {
    return object.map(trimGuids).sort(compareObjects);
  }
  if (@typeName(object) == "object") {
    const result = {};
    for (const key in object) {
      result[key] = trimGuids(object[key]);
    }
    return result;
  }
  if (@typeName(object) == "string") {
    return if (object) object.match("[^@]+")[0] else "";
  }
  return object;
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test((_p) => video == "on", "Extra video artifacts in the objects list");
    // [ts] test("CN-001 should scope context handles", () => {
    // [ts] const browser = browserType.launch();
    // [ts] const GOLDEN_PRECONDITION = {
    // [ts] _guid: "",
    // [ts] objects: [
    // [ts] { _guid: "android", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [
    // [ts] { _guid: "browser", objects: [] }
    // [ts] ] },
    // [ts] { _guid: "electron", objects: [] },
    // [ts] { _guid: "localUtils", objects: [] },
    // [ts] { _guid: "Playwright", objects: [] },
    // [ts] ]
    // [ts] };
    // [ts] expectScopeState(browser, GOLDEN_PRECONDITION);
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // Firefox Beta 96 yields a console warning for the pages that
    // don't use `<!DOCTYPE HTML> tag.
    // [ts] page.goto(server.PREFIX + "/empty-standard-mode.html");
    // [ts] expectScopeState(browser, {
    // [ts] _guid: "",
    // [ts] objects: [
    // [ts] { _guid: "android", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [
    // [ts] { _guid: "browser", objects: [
    // [ts] { _guid: "browser-context", objects: [
    // [ts] { _guid: "page", objects: [
    // [ts] { _guid: "frame", objects: [] },
    // [ts] { _guid: "request", objects: [
    // [ts] { _guid: "response", objects: [] },
    // [ts] ] },
    // [ts] ] },
    // [ts] { _guid: "debugger", objects: [] },
    // [ts] { _guid: "request-context", objects: [
    // [ts] { _guid: "tracing", objects: [] },
    // [ts] ] },
    // [ts] { _guid: "tracing", objects: [] }
    // [ts] ] },
    // [ts] ] },
    // [ts] ] },
    // [ts] { _guid: "electron", objects: [] },
    // [ts] { _guid: "localUtils", objects: [] },
    // [ts] { _guid: "Playwright", objects: [] },
    // [ts] ]
    // [ts] });
    // [ts] context.close();
    // [ts] expectScopeState(browser, GOLDEN_PRECONDITION);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("CN-002 should scope CDPSession handles", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const browser = browserType.launch();
    // [ts] const GOLDEN_PRECONDITION = {
    // [ts] _guid: "",
    // [ts] objects: [
    // [ts] { _guid: "android", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [
    // [ts] { _guid: "browser", objects: [] }
    // [ts] ] },
    // [ts] { _guid: "electron", objects: [] },
    // [ts] { _guid: "localUtils", objects: [] },
    // [ts] { _guid: "Playwright", objects: [] },
    // [ts] ]
    // [ts] };
    // [ts] expectScopeState(browserType, GOLDEN_PRECONDITION);
    // [ts] const session = browser.newBrowserCDPSession();
    // [ts] expectScopeState(browserType, {
    // [ts] _guid: "",
    // [ts] objects: [
    // [ts] { _guid: "android", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [
    // [ts] { _guid: "browser", objects: [
    // [ts] { _guid: "cdp-session", objects: [] },
    // [ts] ] },
    // [ts] ] },
    // [ts] { _guid: "electron", objects: [] },
    // [ts] { _guid: "localUtils", objects: [] },
    // [ts] { _guid: "Playwright", objects: [] },
    // [ts] ]
    // [ts] });
    // [ts] session.detach();
    // [ts] expectScopeState(browserType, GOLDEN_PRECONDITION);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("CN-003 should scope browser handles", () => {
    // [ts] const GOLDEN_PRECONDITION = {
    // [ts] _guid: "",
    // [ts] objects: [
    // [ts] { _guid: "android", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "electron", objects: [] },
    // [ts] { _guid: "localUtils", objects: [] },
    // [ts] { _guid: "Playwright", objects: [] },
    // [ts] ]
    // [ts] };
    // [ts] expectScopeState(browserType, GOLDEN_PRECONDITION);
    // [ts] const browser = browserType.launch();
    // [ts] browser.newContext();
    // [ts] expectScopeState(browserType, {
    // [ts] _guid: "",
    // [ts] objects: [
    // [ts] { _guid: "android", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [
    // [ts] {
    // [ts] _guid: "browser", objects: [
    // [ts] { _guid: "browser-context", objects: [
    // [ts] { _guid: "debugger", objects: [] },
    // [ts] { _guid: "request-context", objects: [
    // [ts] { _guid: "tracing", objects: [] },
    // [ts] ] },
    // [ts] { _guid: "tracing", objects: [] },
    // [ts] ] },
    // [ts] ]
    // [ts] },
    // [ts] ]
    // [ts] },
    // [ts] { _guid: "electron", objects: [] },
    // [ts] { _guid: "localUtils", objects: [] },
    // [ts] { _guid: "Playwright", objects: [] },
    // [ts] ]
    // [ts] });
    // [ts] browser.close();
    // [ts] expectScopeState(browserType, GOLDEN_PRECONDITION);
    // [ts] });
    // [ts] test("CN-004 should not generate dispatchers for subresources w/o listeners", () => {
    // [ts] server.setRedirect("/one-style.css", "/two-style.css");
    // [ts] server.setRedirect("/two-style.css", "/three-style.css");
    // [ts] server.setRedirect("/three-style.css", "/four-style.css");
    // [ts] server.setRoute("/four-style.css", (req, res) => res.end("body {box-sizing: border-box; }"));
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expectScopeState(browserType, {
    // [ts] _guid: "",
    // [ts] objects: [
    // [ts] { _guid: "android", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [] },
    // [ts] { _guid: "browser-type", objects: [
    // [ts] {
    // [ts] _guid: "browser", objects: [
    // [ts] { _guid: "browser-context", objects: [
    // [ts] {
    // [ts] _guid: "page", objects: [
    // [ts] { _guid: "frame", objects: [] },
    // [ts] { _guid: "request", objects: [
    // [ts] { _guid: "response", objects: [] },
    // [ts] ] },
    // [ts] ]
    // [ts] },
    // [ts] { _guid: "debugger", objects: [] },
    // [ts] { _guid: "request-context", objects: [
    // [ts] { _guid: "tracing", objects: [] },
    // [ts] ] },
    // [ts] { _guid: "tracing", objects: [] }
    // [ts] ] },
    // [ts] ]
    // [ts] }],
    // [ts] },
    // [ts] { _guid: "electron", objects: [] },
    // [ts] { _guid: "localUtils", objects: [] },
    // [ts] { _guid: "Playwright", objects: [] },
    // [ts] ]
    // [ts] });
    // [ts] });
    // [ts] test("CN-005 should work with the domain module", () => {
    // [ts] const local = domain.create();
    // [ts] local.run(() => { });
    // [ts] let err;
    // [ts] local.on("error", e => err = e);
    // [ts] const browser = browserType.launch();
    // [ts] const page = browser.newPage();
    // [ts] expect_eq(page.evaluate(() => 1 + 1), 2);
    // At the time of writing, we used to emit 'error' event for WebSockets,
    // which failed with 'domain' module.
    // [ts] let callback;
    // [ts] const result = Promise.init(f => callback = f);
    // [ts] page.on("websocket", ws => ws.on("socketerror", callback));
    // [ts] void page.evaluate(host => {
    // [ts] WebSocket.init("ws://" + host + "/bogus-ws");
    // [ts] }, server.HOST);
    // [ts] const message = result;
    // [ts] expect_eq(std.mem.indexOf(u8, message, if (channel?.includes("msedge")) "" else ": 400") != null, true);
    // [ts] browser.close();
    // [ts] if (err)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw err;)
    // [ts] });
    // [ts] test("CN-006 exposeFunction should not leak", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let called = 0;
    // [ts] page.exposeFunction("myFunction", () => ++called);
    // [ts] let i = 0;
    // [ts] while (i < 10) // [cap-migrate] for-step: ++i
    // [ts] page.evaluate(() => (window).myFunction({ foo: "bar" }));
    // [ts] expect_eq(called, 10);
    // [ts] expectScopeState(page, {
    // [ts] "_guid": "",
    // [ts] "objects": [
    // [ts] {
    // [ts] "_guid": "android",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "browser-type",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "browser-type",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "browser-type",
    // [ts] "objects": [
    // [ts] {
    // [ts] "_guid": "browser",
    // [ts] "objects": [
    // [ts] {
    // [ts] "_guid": "browser-context",
    // [ts] "objects": [
    // [ts] {
    // [ts] "_guid": "page",
    // [ts] "objects": [
    // [ts] {
    // [ts] "_guid": "disposable",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "frame",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "request",
    // [ts] "objects": [
    // [ts] {
    // [ts] "_guid": "response",
    // [ts] "objects": [],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "debugger",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "request-context",
    // [ts] "objects": [
    // [ts] {
    // [ts] "_guid": "tracing",
    // [ts] "objects": [],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "tracing",
    // [ts] "objects": [],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "electron",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "localUtils",
    // [ts] "objects": [],
    // [ts] },
    // [ts] {
    // [ts] "_guid": "Playwright",
    // [ts] "objects": [],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
}
