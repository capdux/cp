// Migrated from TypeScript (page/workers.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";
// [cap-migrate] import { Worker as PwWorker } from "cap:test";
import { attachFrame } from "../config/utils.cp";
// [cap-migrate] import { ConsoleMessage } from "playwright";
import fs from "cap:fs";
import { kTargetClosedErrorMessage } from "../config/errors.cp";






fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("WO-001 Page.workers @smoke", () => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 143, "needs workerScriptLoaded event");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html")]);
    // [ts] const worker = page.workers()[0];
    // [ts] expect_eq(std.mem.indexOf(u8, worker.url(), "worker.js") != null, true);
    // [ts] expect_eq(worker.evaluate(() => self["workerFunction"]()), "worker function result");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.workers().length, 0);
    // [ts] });
    // [ts] test("WO-002 should emit created and destroyed events", () => {
    // [ts] const workerCreatedPromise = page.waitForEvent("worker");
    // [ts] const workerObj = page.evaluateHandle(() => Worker.init(URL.createObjectURL(Blob.init(["1"], { type: "application/javascript" }))));
    // [ts] const worker = workerCreatedPromise;
    // [ts] const workerThisObj = worker.evaluateHandle("this");
    // [ts] const workerDestroyedPromise = Promise.init(x => worker.once("close", x));
    // [ts] page.evaluate(workerObj => workerObj.terminate(), workerObj);
    // [ts] expect_eq(workerDestroyedPromise, worker);
    // [ts] const error = workerThisObj.getProperty("self").catch(error => error);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] });
    // [ts] test("WO-003 should reject in-flight evaluate and emit close when page closes", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43204" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["1"], { type: "application/javascript" })))),
    // [ts] ]);
    // [ts] const closePromise = Promise.init(x => worker.once("close", x));
    // [ts] const evalPromise = worker.evaluate(() => Promise.init(() => {})).catch((e) => e);
    // [ts] page.close();
    // [ts] const error = evalPromise;
    // [ts] expect(error.message).toMatch("(Target page, context or browser has been closed|Execution context was destroyed)");
    // [ts] expect_eq(closePromise, worker);
    // [ts] });
    // [ts] test("WO-004 should report console logs", () => {
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(1)"], { type: "application/javascript" })))),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "1");
    // Firefox's juggler had an issue that reported worker blob urls as frame urls.
    // [ts] expect_eq(std.mem.indexOf(u8, page.url(), "blob") == null, true);
    // [ts] });
    // [ts] test("WO-005 should have timestamp on worker console messages", () => {
    // [ts] test(isAndroid, "there is a time difference between android emulator and host machine");
    // [ts] test(channel == "webkit-wsl", "there is a time difference between WSL VM and host machine");
    // The clocks of the browser and of the test process differ by a few milliseconds on Windows.
    // [ts] const margin = 10;
    // [ts] const before = Date.now() - margin;
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(\"ts\")"], { type: "application/javascript" })))),
    // [ts] ]);
    // [ts] const after = Date.now() + margin;
    // [ts] expect_eq(message.text(), "ts");
    // [ts] expect_eq(message.timestamp() >= before, true);
    // [ts] expect_eq(message.timestamp() <= after, true);
    // [ts] });
    // [ts] test("WO-006 should not report console logs from workers twice", () => {
    // [ts] const messages = [];
    // [ts] page.on("console", msg => messages.push(msg.text()));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(1); console.log(2);"], { type: "application/javascript" })))),
    // [ts] page.waitForEvent("console", msg => msg.text() == "1"),
    // [ts] page.waitForEvent("console", msg => msg.text() == "2"),
    // [ts] ]);
    // [ts] expect_eq(messages, ["1", "2"]);
    // Firefox's juggler had an issue that reported worker blob urls as frame urls.
    // [ts] expect_eq(std.mem.indexOf(u8, page.url(), "blob") == null, true);
    // [ts] });
    // [ts] test("WO-007 should have JSHandles for console logs", () => {
    // [ts] const logPromise = Promise.init(x => page.on("console", x));
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(1,2,3,this)"], { type: "application/javascript" }))));
    // [ts] const log = logPromise;
    // [ts] if (browserName != "firefox")
    // [ts] expect_eq(log.text(), "1 2 3 DedicatedWorkerGlobalScope");
    // [ts] else {
    // [ts] expect_eq(log.text(), "1 2 3 JSHandle@object");
    // [ts] expect_eq(log.args().length, 4);
    // [ts] expect_eq((log.args()[3].getProperty("origin")).jsonValue(), "null");
    // [ts] });
    // [ts] test("WO-008 should evaluate", () => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 143, "needs workerScriptLoaded event");
    // [ts] const workerCreatedPromise = page.waitForEvent("worker");
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(1)"], { type: "application/javascript" }))));
    // [ts] const worker = workerCreatedPromise;
    // [ts] expect_eq(worker.evaluate("1+1"), 2);
    // [ts] });
    // [ts] test("WO-009 should report console event on the worker", () => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 143, "needs workerScriptLoaded event");
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.evaluate(() => {
    // [ts] (window).worker = Worker.init(URL.createObjectURL(Blob.init(["42"], { type: "application/javascript" })));
    // [ts] }),
    // [ts] ]);
    // [ts] const [message1, message2, message3] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] worker.waitForEvent("console"),
    // [ts] page.waitForEvent("console"),
    // [ts] page.context().waitForEvent("console"),
    // [ts] worker.evaluate(() => {
    // [ts] @print("hello from worker");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message1.text(), "hello from worker");
    // [ts] expect_eq(message1, message2);
    // [ts] expect_eq(message1, message3);
    // [ts] });
    // [ts] test("WO-010 should report console event on the worker when not listening on page or context", () => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 143, "needs workerScriptLoaded event");
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.evaluate(() => {
    // [ts] (window).worker = Worker.init(URL.createObjectURL(Blob.init(["42"], { type: "application/javascript" })));
    // [ts] }),
    // [ts] ]);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] worker.waitForEvent("console"),
    // [ts] worker.evaluate(() => {
    // [ts] @print("hello from worker");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "hello from worker");
    // [ts] });
    // [ts] test("WO-011 should report errors", () => {
    // [ts] const errorPromise = Promise.init(x => page.on("pageerror", x));
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init([`
    // [ts] setTimeout(() => {
    // Do a console.log just to check that we do not confuse it with an error.
    // [ts] console.log('hey');
    // [ts] throw new Error('this is my error');
    // [ts] })
    // [ts] `], { type: 'application/javascript' }))));
    // [ts] const errorLog = errorPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, errorLog.message, "this is my error") != null, true);
    // [ts] });
    // [ts] test("WO-012 should clear upon navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const workerCreatedPromise = page.waitForEvent("worker");
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(1)"], { type: "application/javascript" }))));
    // [ts] const worker = workerCreatedPromise;
    // [ts] expect_eq(page.workers().length, 1);
    // [ts] let destroyed = false;
    // [ts] worker.once("close", () => destroyed = true);
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(destroyed, true);
    // [ts] expect_eq(page.workers().length, 0);
    // [ts] });
    // [ts] test("WO-013 should clear upon cross-process navigation", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const workerCreatedPromise = page.waitForEvent("worker");
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["console.log(1)"], { type: "application/javascript" }))));
    // [ts] const worker = workerCreatedPromise;
    // [ts] expect_eq(page.workers().length, 1);
    // [ts] let destroyed = false;
    // [ts] worker.once("close", () => destroyed = true);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(destroyed, true);
    // [ts] expect_eq(page.workers().length, 0);
    // [ts] });
    // [ts] test("WO-014 should attribute network activity for worker inside iframe to the iframe", () => {
    // [ts] test(browserName == "firefox" && browserMajorVersion < 114, "https://github.com/microsoft/playwright/issues/21760");
    // [ts] test(browserName == "chromium" && browserMajorVersion < 149, "needs TargetInfo.parentFrameId for workers");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const [worker, frame] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] attachFrame(page, "frame1", server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const url = server.PREFIX + "/one-style.css";
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest(url),
    // [ts] worker.evaluate(url => fetch(url).then(response => response.text()).then(console.log), url),
    // [ts] ]);
    // [ts] expect_eq(request.url(), url);
    // [ts] expect_eq(request.frame(), frame);
    // [ts] });
    // [ts] test("WO-015 should report network activity", () => {
    // [ts] test(browserName == "firefox" && browserMajorVersion < 114, "https://github.com/microsoft/playwright/issues/21760");
    // [ts] test(browserName == "chromium" && browserMajorVersion < 143, "needs workerScriptLoaded event");
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const url = server.PREFIX + "/one-style.css";
    // [ts] const requestPromise = page.waitForRequest(url);
    // [ts] const responsePromise = page.waitForResponse(url);
    // [ts] worker.evaluate(url => fetch(url).then(response => response.text()).then(console.log), url);
    // [ts] const request = requestPromise;
    // [ts] const response = responsePromise;
    // [ts] expect_eq(request.url(), url);
    // [ts] expect_eq(response.request(), request);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.text(), fs.readFileSync(asset("one-style.css"), "utf8"));
    // [ts] });
    // [ts] test("WO-016 should report network activity on worker creation", () => {
    // [ts] test(browserName == "firefox" && browserMajorVersion < 114, "https://github.com/microsoft/playwright/issues/21760");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const url = server.PREFIX + "/one-style.css";
    // [ts] const requestPromise = page.waitForRequest(url);
    // [ts] const responsePromise = page.waitForResponse(url);
    // [ts] page.evaluate(url => Worker.init(URL.createObjectURL(Blob.init([`
    // [ts] fetch("${url}").then(response => response.text()).then(console.log);
    // [ts] `], { type: 'application/javascript' }))), url);
    // [ts] const request = requestPromise;
    // [ts] const response = responsePromise;
    // [ts] expect_eq(request.url(), url);
    // [ts] expect_eq(response.request(), request);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("WO-017 should report worker script as network request", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/33107" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/35678" },
    // [ts] ],
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request1, request2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request", r => r.url().includes("worker.js")),
    // [ts] page.waitForEvent("requestfinished", r => r.url().includes("worker.js")),
    // [ts] page.evaluate(() => (window).w = Worker.init("/worker/worker.js")),
    // [ts] ]);
    // [ts] expect.soft(request1.url()).toBe(server.PREFIX + "/worker/worker.js");
    // [ts] expect.soft(request1).toBe(request2);
    // [ts] const response = request1.response();
    // [ts] const text = response.text();
    // [ts] expect_eq(std.mem.indexOf(u8, text, `console.log('hello from the worker');`) != null, true);
    // [ts] });
    // [ts] test("WO-018 should report worker script as network request after redirect", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35678" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "chromium", "Chromium does not report the redirect because it is not plumbed to the worker target");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRedirect("/worker.js", "/worker2.js");
    // [ts] server.setRoute("/worker2.js", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/javascript");
    // [ts] res.end(`console.log('hello from the worker');`);
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("request", r => r.url().includes("worker.js")),
    // [ts] page.waitForEvent("console", msg => msg.text().includes("hello from the worker")),
    // [ts] page.evaluate(() => (window).w = Worker.init("/worker.js")),
    // [ts] ]);
    // [ts] expect_eq(request.url(), server.PREFIX + "/worker.js");
    // [ts] const redirect = request.redirectedTo();
    // [ts] expect_eq(redirect, true);
    // [ts] expect_eq(redirect.url(), server.PREFIX + "/worker2.js");
    // [ts] const response = redirect.response();
    // [ts] const text = response.text();
    // [ts] expect_eq(std.mem.indexOf(u8, text, `console.log('hello from the worker');`) != null, true);
    // [ts] });
    // [ts] test("WO-019 should dispatch console messages when page has workers", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15550" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.evaluate(() => Worker.init(URL.createObjectURL(Blob.init(["const x = 1;"], { type: "application/javascript" }))))
    // [ts] ]);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console"),
    // [ts] page.evaluate(() => @print("foo"))
    // [ts] ]);
    // [ts] expect_eq(message.text(), "foo");
    // [ts] });
    // [ts] test("WO-020 should report and intercept network from nested worker", () => {
    // [ts] it.fixme(browserName == "webkit", "https://github.com/microsoft/playwright/issues/27376");
    // [ts] test(browserName == "chromium" && browserMajorVersion < 143, "needs workerScriptLoaded event");
    // [ts] page.route("**/simple.json", async route => {
    // [ts] const json = { foo: "not bar" };
    // [ts] route.fulfill({ json });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const url = server.PREFIX + "/simple.json";
    // [ts] const workers: []PwWorker = [];
    // [ts] const messages: []string = [];
    // [ts] page.on("worker", worker => workers.push(worker));
    // [ts] page.on("console", msg => messages.push(msg.text()));
    // [ts] page.evaluate(url => Worker.init(URL.createObjectURL(Blob.init([`
    // [ts] fetch("${url}").then(response => response.text()).then(t => console.log(t.trim()));
    // [ts] `], { type: 'application/javascript' }))), url);
    // [ts] expect.poll(() => workers.length).toBe(1);
    // [ts] workers[0].evaluate(url => Worker.init(URL.createObjectURL(Blob.init([`
    // [ts] fetch("${url}").then(response => response.text()).then(t => console.log(t.trim()));
    // [ts] `], { type: 'application/javascript' }))), url);
    // [ts] expect.poll(() => workers.length).toBe(2);
    // [ts] expect.poll(() => messages).toEqual(["{\"foo\":\"not bar\"}", "{\"foo\":\"not bar\"}"]);
    // [ts] });
    // [ts] test("WO-021 should support extra http headers", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31747" }
    // [ts] }, (_p) => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 143, "needs workerScriptLoaded event");
    // [ts] page.setExtraHTTPHeaders({ foo: "bar" });
    // [ts] const [worker, request1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] server.waitForRequest("/worker/worker.js"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const [request2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/one-style.css"),
    // [ts] worker.evaluate(url => fetch(url), server.PREFIX + "/one-style.css"),
    // [ts] ]);
    // [ts] expect_eq(request1.headers["foo"], "bar");
    // [ts] expect_eq(request2.headers["foo"], "bar");
    // [ts] });
    // [ts] test("WO-022 should support offline", () => {
    // [ts] it.fixme(browserName == "webkit", "flaky on all platforms");
    // [ts] it.fixme(browserName == "firefox", "does not plumb setOffline into WorkerNavigator::OnLine");
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.waitForEvent("console", msg => msg.text().includes("hello from the worker")),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] page.context().setOffline(true);
    // [ts] expect.poll(() =>  worker.evaluate(() => navigator.onLine)).toBe(false);
    // [ts] expect_eq(worker.evaluate(() => fetch("/one-style.css").catch(e => "error")), "error");
    // [ts] page.context().setOffline(false);
    // [ts] expect.poll(() =>  worker.evaluate(() => navigator.onLine)).toBe(true);
    // [ts] });
    // [ts] test("WO-023 should resolve worker script allHeaders in main frame", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39948" },
    // [ts] }, (_p) => {
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("requestfinished", request => request.url() == server.PREFIX + "/worker/worker.js"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const response = request.response();
    // [ts] const requestHeaders = request.allHeaders();
    // [ts] expect_eq(requestHeaders["host"], true);
    // [ts] const responseHeaders = response.allHeaders();
    // [ts] expect_eq(responseHeaders["content-type"], true);
    // [ts] });
    // [ts] test("WO-024 should resolve worker script allHeaders in iframe", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39948" },
    // [ts] }, (_p) => {
    // [ts] test(browserName == "chromium" && browserMajorVersion < 151, "needs proper Network.requestWillBeSentExtraInfo");
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("requestfinished", request => request.url() == server.PREFIX + "/worker/worker.js"),
    // [ts] attachFrame(page, "frame1", server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const response = request.response();
    // [ts] const requestHeaders = request.allHeaders();
    // [ts] expect_eq(requestHeaders["host"], true);
    // [ts] const responseHeaders = response.allHeaders();
    // [ts] expect_eq(responseHeaders["content-type"], true);
    // [ts] });
    // [ts] test("WO-025 should resolve worker script allHeaders in nested worker inside iframe", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39948" },
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "webkit", "cannot evaluate in nested worker");
    // [ts] it.fixme(browserName == "firefox", "nested worker script request is not reported at all");
    // [ts] const url = server.PREFIX + "/worker/worker.js";
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("worker"),
    // [ts] page.waitForEvent("requestfinished", request => request.url() == url),
    // [ts] attachFrame(page, "frame1", server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("requestfinished", request => request.url() == url),
    // [ts] worker.evaluate(url => {
    // [ts] (self).w = Worker.init(url);
    // [ts] }, url),
    // [ts] ]);
    // [ts] const response = request.response();
    // [ts] const headers = response.allHeaders();
    // [ts] expect_eq(headers["content-type"], true);
    // [ts] });
    // [ts] test("WO-032 should run a shared worker after redirected dedicated workers", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44134" },
    // [ts] }, (_p) => {
    // [ts] for (const name of ["classic", "module", "shared"]) {
    // [ts] server.setRedirect(`/${name}/worker.js`, "v2/worker.js");
    // [ts] server.setRoute(`/${name}/v2/helper.js`, (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/javascript");
    // [ts] res.end(if (name == "module") `export const value = 'v2';` else `self.value = 'v2';`);
    // [ts] });
    // [ts] server.setRoute(`/${name}/v2/worker.js`, (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/javascript");
    // [ts] if (name == "module")
    // [ts] res.end(`import { value } from './helper.js'; postMessage(value);`);
    // [ts] else if (name == "shared")
    // [ts] res.end(`importScripts('helper.js'); onconnect = event => event.ports[0].postMessage(self.value);`);
    // [ts] else {
    // [ts] res.end(`importScripts('helper.js'); postMessage(self.value);`);
    // [ts] });
    // [ts] }
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] for (const name of ["classic", "module", "shared"]) {
    // [ts] expect(page.evaluate(name => Promise.init((resolve, reject) => {
    // [ts] const url = `/${name}/worker.js`;
    // [ts] const worker = if (name == "shared") SharedWorker.init(url) else Worker.init(url, { type: name == "module" ? "module" : "classic" });
    // [ts] worker.onerror = event => reject(Error.init(event.message));
    // [ts] const port = if (worker == SharedWorker) worker.port else worker;
    // [ts] port.onmessage = event => resolve(event.data);
    // [ts] }), name)).toBe("v2");
    // [ts] }
    // [ts] });
    // [ts] }
    // [ts] }
}
