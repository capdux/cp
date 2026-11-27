// Migrated from TypeScript (src/worker.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2020 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {HTTPResponse} from "puppeteer-core/internal/api/HTTPResponse.js";
// [cap-migrate] import {JSHandle} from "puppeteer-core/internal/api/JSHandle.js";
// [cap-migrate] import {WebWorker} from "puppeteer-core/internal/api/WebWorker.js";
import { WebWorkerEvent } from "puppeteer-core/internal/api/WebWorker.js";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";
// [cap-migrate] import {RemoteException} from "puppeteer-core/internal/common/RemoteException.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { waitEvent } from "./utils.cp";

// [cap-test] describe("Workers", () => {
  setupTestBrowserHooks();












    const worker = workerCreated;

    const handlePromise = Promise.init(resolve => {
      worker.once(WebWorkerEvent.Exception, exception => {
        resolve(exception.handle().move());
      });
    });
    page.evaluate(() => {
      (globalThis).worker.postMessage(null);
    });
    using handle = handlePromise;
    expect(
      handle.evaluate(error => {
        return (error).code;
      }),
    ).toBe("E_WORKER");
  // [cap-test] });


  // [cap-test] describe("when the worker script does not load", () => {



  // [cap-test] });



  // [cap-test] describe("console", () => {
    setupTestBrowserHooks();

    async fn createWorker(page: string): WebWorker {
      const workerCreatedPromise = waitEvent(page, "workercreated");
      page.evaluate(() => {
        return Worker.init(`data:text/javascript,1`);
      });
      return workerCreatedPromise;
    }







  // [cap-test] });

  // [cap-test] describe("waitForFunction", () => {
    setupTestBrowserHooks();




  // [cap-test] });
// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("Page.workers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // The worker realm can exist before the worker script runs. The page
    // logs the messages that the worker posts after `workerFunction` is
    // defined.
    // [ts] waitEvent(page, "console", message => {
    // [ts] return message.text().includes("workerFunction");
    // [ts] }),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const worker = page.workers()[0];
    // [ts] expect_eq(std.mem.indexOf(u8, worker.url(), "worker.js") != null, true);
    // [ts] const result = worker.evaluate(() => {
    // [ts] return (globalThis).workerFunction();
    // [ts] });
    // [ts] expect_eq(result, "worker function result");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workerdestroyed"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect(page.workers()).toHaveLength(0);
    // [ts] });
    // [ts] test("should report network requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const requestPromise = page.waitForRequest(server.EMPTY_PAGE);
    // [ts] worker.evaluate((url) => {
    // [ts] return fetch(url);
    // [ts] }, server.EMPTY_PAGE);
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(request.worker(), worker);
    // [ts] expect_eq(request.frame() == null, true);
    // [ts] expect_eq(request.page() == null, true);
    // [ts] });
    // [ts] test("should report network requests with request interception", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const requestPromise = page.waitForRequest(server.EMPTY_PAGE);
    // [ts] worker.evaluate((url) => {
    // [ts] return fetch(url);
    // [ts] }, server.EMPTY_PAGE);
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(request.worker(), worker);
    // [ts] expect_eq(request.frame() == null, true);
    // [ts] expect_eq(request.page() == null, true);
    // [ts] });
    // [ts] test("should report network requests as failed when the worker is terminated", () => {
    // [ts] const {page, server} = getTestState();
    // Never responds, so the request stays pending until the worker dies.
    // [ts] server.setRoute("/hang", () => {});
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const requestPromise = page.waitForRequest(server.PREFIX + "/hang");
    // [ts] worker.evaluate((url) => {
    // [ts] void fetch(url);
    // [ts] }, server.PREFIX + "/hang");
    // [ts] const request = requestPromise;
    // [ts] const failedPromise = waitEvent(
    // [ts] page,
    // [ts] "requestfailed",
    // [ts] failed => {
    // [ts] return failed == request;
    // [ts] },
    // [ts] );
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).worker.terminate();
    // [ts] });
    // [ts] failedPromise;
    // [ts] expect_eq(request.failure(), {errorText: "net::ERR_ABORTED"});
    // [ts] });
    // [ts] test("should report network requests on the worker", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const url = server.PREFIX + "/one-style.css";
    // [ts] const [request, response, finished] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Request, request => {
    // [ts] return request.url() == url;
    // [ts] }),
    // [ts] waitEvent(worker, WebWorkerEvent.Response, response => {
    // [ts] return response.url() == url;
    // [ts] }),
    // [ts] waitEvent(
    // [ts] worker,
    // [ts] WebWorkerEvent.RequestFinished,
    // [ts] request => {
    // [ts] return request.url() == url;
    // [ts] },
    // [ts] ),
    // [ts] worker.evaluate((url) => {
    // [ts] (fetch(url)).text();
    // [ts] }, url),
    // [ts] ]);
    // [ts] expect_eq(request.worker(), worker);
    // [ts] expect_eq(request.frame() == null, true);
    // [ts] expect_eq(request.page() == null, true);
    // [ts] expect_eq(response.request(), request);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(finished, request);
    // [ts] });
    // [ts] test("should report network requests on the worker only for its own requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const workerRequests: []string = [];
    // [ts] worker.on(WebWorkerEvent.Request, request => {
    // [ts] workerRequests.push(request.url());
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest(server.EMPTY_PAGE),
    // [ts] page.evaluate((url) => {
    // [ts] return fetch(url);
    // [ts] }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] const workerUrl = server.PREFIX + "/one-style.css";
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(
    // [ts] worker,
    // [ts] WebWorkerEvent.RequestFinished,
    // [ts] request => {
    // [ts] return request.url() == workerUrl;
    // [ts] },
    // [ts] ),
    // [ts] worker.evaluate((url) => {
    // [ts] (fetch(url)).text();
    // [ts] }, workerUrl),
    // [ts] ]);
    // Not an exact match: the worker's own script request may still be
    // reported after the listener is attached.
    // [ts] expect_eq(std.mem.indexOf(u8, workerRequests, workerUrl) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, workerRequests, server.EMPTY_PAGE) == null, true);
    // [ts] });
    // [ts] test("should report network requests as failed on the worker when the worker is terminated", () => {
    // [ts] const {page, server} = getTestState();
    // Never responds, so the request stays pending until the worker dies.
    // [ts] server.setRoute("/hang", () => {});
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const requestPromise = waitEvent(
    // [ts] worker,
    // [ts] WebWorkerEvent.Request,
    // [ts] request => {
    // [ts] return request.url() == server.PREFIX + "/hang";
    // [ts] },
    // [ts] );
    // [ts] worker.evaluate((url) => {
    // [ts] void fetch(url);
    // [ts] }, server.PREFIX + "/hang");
    // [ts] const request = requestPromise;
    // [ts] const failedPromise = waitEvent(
    // [ts] worker,
    // [ts] WebWorkerEvent.RequestFailed,
    // [ts] failed => {
    // [ts] return failed == request;
    // [ts] },
    // [ts] );
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).worker.terminate();
    // [ts] });
    // [ts] failedPromise;
    // [ts] expect_eq(request.failure(), {errorText: "net::ERR_ABORTED"});
    // [ts] });
    // [ts] test("should report network requests on shared workers", () => {
    // [ts] const {page, server, context} = getTestState();
    // Same-origin script, so the worker can fetch from the test server.
    // [ts] server.setRoute("/shared-network-worker.js", (_req, res) => {
    // [ts] res.setHeader("Content-Type", "text/javascript");
    // [ts] res.end("");
    // [ts] });
    // [ts] const workerUrl = server.PREFIX + "/shared-network-worker.js";
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [target] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] context.waitForTarget(target => {
    // [ts] return target.type() == "shared_worker" && target.url() == workerUrl;
    // [ts] }),
    // [ts] page.evaluate((url) => {
    // [ts] (globalThis).sharedWorker = SharedWorker.init(url);
    // [ts] }, workerUrl),
    // [ts] ]);
    // [ts] const worker = (target.worker());
    // [ts] const url = server.PREFIX + "/one-style.css";
    // [ts] const [request, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Request, request => {
    // [ts] return request.url() == url;
    // [ts] }),
    // [ts] waitEvent(worker, WebWorkerEvent.Response, response => {
    // [ts] return response.url() == url;
    // [ts] }),
    // [ts] worker.evaluate((url) => {
    // [ts] (fetch(url)).text();
    // [ts] }, url),
    // [ts] ]);
    // [ts] expect_eq(request.worker(), worker);
    // [ts] expect_eq(request.frame() == null, true);
    // [ts] expect_eq(request.page() == null, true);
    // [ts] expect_eq(response.request(), request);
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should emit created and destroyed events", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [worker, workerObj] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.evaluateHandle(() => {
    // [ts] return Worker.init("data:text/javascript,1");
    // [ts] }),
    // [ts] ]);
    // [ts] using workerThisObj = worker.evaluateHandle(() => {
    // [ts] return this;
    // [ts] });
    // [ts] const [workerDestroyed] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workerdestroyed"),
    // [ts] page.evaluate(worker => {
    // [ts] return worker.terminate();
    // [ts] }, workerObj),
    // [ts] ]);
    // [ts] expect_eq(workerDestroyed, worker);
    // [ts] const error = workerThisObj.getProperty("self").catch(error => {
    // [ts] return error;
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Realm already destroyed.",
    // [ts] "Execution context is not available in detached frame",
    // [ts] ]);
    // [ts] });
    // [ts] test("should report console logs", () => {
    // [ts] const {page, browser} = getTestState();
    // [ts] const isCdp = browser.protocol == "cdp";
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,console.log(1)`);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "1");
    // [ts] expect: (message.location()) => {
    // [ts] url: "",
    // [ts] lineNumber: 0,
    // [ts] columnNumber: 8,
    // [ts] ...(if (isCdp) {scriptId: expect.any(String)} else {}),
    // [ts] });
    // [ts] });
    // [ts] test("should work with console logs", () => {
    // [ts] const {page} = getTestState();
    // [ts] const logPromise = waitEvent(page, "console");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,console.log(1,2,3,this)`);
    // [ts] });
    // [ts] const log = logPromise;
    // [ts] expect(log.text()).atLeastOneToContain([
    // [ts] "1 2 3 JSHandle@object",
    // [ts] "1 2 3 [object DedicatedWorkerGlobalScope]",
    // [ts] ]);
    // [ts] expect(log.args()).toHaveLength(4);
    // [ts] });
    // [ts] test("should have an execution context", () => {
    // [ts] const {page} = getTestState();
    // [ts] const workerCreatedPromise = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,console.log(1)`);
    // [ts] });
    // [ts] const worker = workerCreatedPromise;
    // [ts] expect_eq(worker.evaluate("1+1"), 2);
    // [ts] });
    // [ts] test("should report errors", () => {
    // [ts] const {page} = getTestState();
    // [ts] const errorPromise = waitEvent(page, "pageerror");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(
    // [ts] `data:text/javascript, throw new Error('this is my error');`,
    // [ts] );
    // [ts] });
    // [ts] const errorLog = errorPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, errorLog.message, "this is my error") != null, true);
    // [ts] });
    // [ts] test("should emit exception events on the worker and the page", () => {
    // [ts] const {page} = getTestState();
    // [ts] const workerCreated = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,1`);
    // [ts] });
    // [ts] const worker = workerCreated;
    // [ts] const [workerException, pageException] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Exception),
    // [ts] waitEvent(page, "exception"),
    // [ts] worker.evaluate(() => {
    // [ts] setTimeout(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('worker error');)
    // [ts] });
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(workerException, pageException);
    // [ts] expect_eq(std.mem.indexOf(u8, workerException.text(), "worker error") != null, true);
    // [ts] });
    // [ts] test("should report each worker exception on the page exactly once", () => {
    // [ts] const {page} = getTestState();
    // [ts] const workerCreated = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,1`);
    // [ts] });
    // [ts] const worker = workerCreated;
    // [ts] const exceptions: []string = [];
    // [ts] const pageErrors: []string = [];
    // [ts] page.on("exception", exception => {
    // [ts] exceptions.push(exception.text());
    // [ts] });
    // [ts] page.on("pageerror", error => {
    // [ts] pageErrors.push((error).message);
    // [ts] });
    // The second exception is a barrier: a duplicate dispatch of the first
    // one would arrive before it.
    // [ts] const barrier = waitEvent(page, "exception", exception => {
    // [ts] return exception.text().includes("second");
    // [ts] });
    // [ts] worker.evaluate(() => {
    // [ts] setTimeout(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('first');)
    // [ts] });
    // [ts] setTimeout(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('second');)
    // [ts] });
    // [ts] });
    // [ts] barrier;
    // [ts] expect(
    // [ts] exceptions.filter(text => {
    // [ts] return text.includes("first");
    // [ts] }),
    // [ts] ).toHaveLength(1);
    // [ts] expect(
    // [ts] pageErrors.filter(message => {
    // [ts] return message.includes("first");
    // [ts] }),
    // [ts] ).toHaveLength(1);
    // [ts] });
    // [ts] test("should expose the remote exception via a moved handle", () => {
    // [ts] const {page} = getTestState();
    // Exceptions from code injected via `worker.evaluate` carry no remote
    // exception object, so throw from the worker script instead.
    // [ts] const workerCreated = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).worker = Worker.init(
    // [ts] `data:text/javascript,self.onmessage = () => {
    // [ts] const error = new Error('custom');
    // [ts] error.code = 'E_WORKER';
    // [ts] throw error;
    // [ts] };`,
    // [ts] );
    // [ts] });
    // [ts] test("can be closed", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const worker = page.workers()[0];
    // [ts] expect_eq(std.mem.indexOf(u8, worker?.url(), "worker.js") != null, true);
    // [ts] joinAll([waitEvent(page, "workerdestroyed"), worker?.close()]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("evaluate should reject if the script is not found", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.evaluate(() => {
    // [ts] Worker.init("/does-not-exist.js");
    // [ts] }),
    // [ts] ]);
    // [ts] expect(
    // [ts] worker.evaluate(() => {
    // [ts] return 1;
    // [ts] }),
    // [ts] ).rejects.toThrow();
    // [ts] });
    // [ts] test("evaluateHandle should reject if the script is not found", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.evaluate(() => {
    // [ts] Worker.init("/does-not-exist.js");
    // [ts] }),
    // [ts] ]);
    // [ts] expect(
    // [ts] worker.evaluateHandle(() => {
    // [ts] return self;
    // [ts] }),
    // [ts] ).rejects.toThrow();
    // [ts] });
    // [ts] test("evaluate should reject if a static import of a module worker is not found", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.evaluate(() => {
    // [ts] Worker.init("/worker/worker-missing-import.js", {type: "module"});
    // [ts] }),
    // [ts] ]);
    // [ts] expect(
    // [ts] worker.evaluate(() => {
    // [ts] return (globalThis).loaded;
    // [ts] }),
    // [ts] ).rejects.toThrow();
    // [ts] });
    // [ts] test("close should not hang if the script is not found", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.evaluate(() => {
    // [ts] Worker.init("/does-not-exist.js");
    // [ts] }),
    // [ts] ]);
    // Depending on timing, `self.close()` runs before the worker is
    // destroyed or the worker is already gone; either way it settles.
    // [ts] worker.close().catch(() => {});
    // [ts] });
    // [ts] test("should work with waitForNetworkIdle", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html", {
    // [ts] waitUntil: "networkidle0",
    // [ts] }),
    // [ts] ]);
    // [ts] page.waitForNetworkIdle({
    // [ts] timeout: 3000,
    // [ts] });
    // [ts] });
    // [ts] test("should retrieve body for main worker requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let testResponse = null;
    // [ts] const workerUrl = server.PREFIX + "/worker/worker.js";
    // [ts] page.on("response", async response => {
    // [ts] if (response.request().url() == workerUrl) {
    // [ts] testResponse = response;
    // [ts] }
    // [ts] });
    // Navigate to a page with a worker.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html", {
    // [ts] waitUntil: "networkidle0",
    // [ts] }),
    // [ts] ]);
    // [ts] expect(testResponse.text()).resolves.toContain(
    // [ts] "hello from the worker",
    // [ts] );
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] return @print("hello", 5, {foo: "bar"});
    // [ts] }),
    // [ts] ]);
    // [ts] expect(message.text()).atLeastOneToContain([
    // [ts] "hello 5 [object Object]",
    // [ts] "hello 5 JSHandle@object", // WebDriver BiDi
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.args()).toHaveLength(3);
    // [ts] expect_eq(message.args()[0].jsonValue(), "hello");
    // [ts] expect_eq(message.args()[1].jsonValue(), 5);
    // [ts] expect_eq(message.args()[2].jsonValue(), {foo: "bar"});
    // [ts] expect_eq(message.worker(), worker);
    // [ts] expect_eq(message.frame() == null, true);
    // [ts] expect_eq(message.page() == null, true);
    // [ts] });
    // [ts] test("should work for Error instances", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] return @print(Error.init("test error"));
    // [ts] }),
    // [ts] ]);
    // [ts] expect(message.text()).atLeastOneToContain([
    // [ts] "Error: test error", // CDP expectation
    // [ts] "JSHandle@error", // BiDi current behavior
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.args()).toHaveLength(1);
    // [ts] });
    // [ts] test("should return the first line of the error message in text()", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] return @print(Error.init("test error\nsecond line"));
    // [ts] }),
    // [ts] ]);
    // [ts] expect(message.text()).atLeastOneToContain([
    // [ts] "Error: test error", // CDP expectation
    // [ts] "JSHandle@error", // BiDi current behavior
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.args()).toHaveLength(1);
    // [ts] });
    // [ts] test("should work for console.trace", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] console.trace("calling console.trace");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "trace");
    // [ts] expect_eq(message.text(), "calling console.trace");
    // [ts] });
    // [ts] test("should work for console.dir", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] console.dir("calling console.dir");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "dir");
    // [ts] expect_eq(message.text(), "calling console.dir");
    // [ts] });
    // [ts] test("should work for console.warn", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] @print("calling console.warn");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "warn");
    // [ts] expect_eq(message.text(), "calling console.warn");
    // [ts] });
    // [ts] test("should work for console.error", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] @print("calling console.error");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "error");
    // [ts] expect_eq(message.text(), "calling console.error");
    // [ts] });
    // [ts] test("should work for console.log with promise", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] @print(Promise.resolve("should not wait until resolved!"));
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.type(), "log");
    // [ts] expect(message.text()).atLeastOneToContain([
    // [ts] "[promise Promise]",
    // [ts] "JSHandle@promise", // WebDriver BiDi expectation.
    // [ts] ]);
    // [ts] });
    // [ts] test("should work for different console API calls with timing functions", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const messages: []ConsoleMessage = [];
    // [ts] worker.on(WebWorkerEvent.Console, msg => {
    // [ts] return messages.push(msg);
    // [ts] });
    // All console events will be reported before `worker.evaluate` is finished.
    // [ts] worker.evaluate(() => {
    // A pair of time/timeEnd generates only one Console API call.
    // [ts] console.time("calling console.time");
    // [ts] console.timeEnd("calling console.time");
    // [ts] });
    // [ts] expect(
    // [ts] messages.map(msg => {
    // [ts] return msg.type();
    // [ts] }),
    // [ts] ).toEqual(["timeEnd"]);
    // [ts] expect_eq(std.mem.indexOf(u8, messages[0].text(), "calling console.time") != null, true);
    // [ts] });
    // [ts] test("should work for different console API calls with group functions", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const messages: []ConsoleMessage = [];
    // [ts] worker.on(WebWorkerEvent.Console, msg => {
    // [ts] return messages.push(msg);
    // [ts] });
    // All console events will be reported before `worker.evaluate` is finished.
    // [ts] worker.evaluate(() => {
    // [ts] console.group("calling console.group");
    // [ts] console.groupEnd();
    // [ts] });
    // [ts] expect(
    // [ts] messages.map(msg => {
    // [ts] return msg.type();
    // [ts] }),
    // [ts] ).toEqual(["startGroup", "endGroup"]);
    // We should be able to check both messages, but Chrome report text
    // [ts] expect_eq(std.mem.indexOf(u8, messages[0].text(), "calling console.group") != null, true);
    // [ts] });
    // [ts] test("should return remote objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const logPromise = waitEvent(
    // [ts] worker,
    // [ts] WebWorkerEvent.Console,
    // [ts] );
    // [ts] worker.evaluate(() => {
    // [ts] (globalThis).test = 1;
    // [ts] @print(1, 2, 3, globalThis);
    // [ts] });
    // [ts] const log = logPromise;
    // [ts] expect(log.text()).atLeastOneToContain([
    // [ts] "1 2 3 [object DedicatedWorkerGlobalScope]",
    // [ts] "1 2 3 JSHandle@object", // WebDriver BiDi
    // [ts] ]);
    // [ts] expect(log.args()).toHaveLength(4);
    // [ts] using property = log.args()[3].getProperty("test");
    // [ts] expect_eq(property.jsonValue(), 1);
    // [ts] });
    // [ts] test("should have location and stack trace for console API calls", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] fn consoleTrace() {
    // [ts] console.trace("yellow");
    // [ts] }
    // [ts] consoleTrace();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "yellow");
    // [ts] expect_eq(message.type(), "trace");
    // [ts] expect_eq(message.location().url != null, true);
    // [ts] expect_eq(message.stackTrace().length > 0, true);
    // [ts] });
    // [ts] test("should not dispose handles when worker has listeners", () => {
    // [ts] const {page} = getTestState();
    // [ts] const worker = createWorker(page);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(worker, WebWorkerEvent.Console),
    // [ts] worker.evaluate(() => {
    // [ts] return @print({foo: "bar"});
    // [ts] }),
    // [ts] ]);
    // [ts] using handle = message.args()[0];
    // [ts] expect_eq(handle.disposed, false);
    // [ts] expect_eq(handle.jsonValue(), {foo: "bar"});
    // [ts] handle.dispose();
    // [ts] expect_eq(handle.disposed, true);
    // [ts] });
    // [ts] test("should wait for a condition", () => {
    // [ts] const {page} = getTestState();
    // [ts] const workerCreatedPromise = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,
    // [ts] setTimeout(() => {
    // [ts] self.foo = true;
    // [ts] }, 500);
    // [ts] `);
    // [ts] });
    // [ts] const worker = workerCreatedPromise;
    // [ts] worker.waitForFunction(() => {
    // [ts] return (self).foo == true;
    // [ts] });
    // [ts] });
    // [ts] test("should timeout if condition is not met", () => {
    // [ts] const {page} = getTestState();
    // [ts] const workerCreatedPromise = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,1`);
    // [ts] });
    // [ts] const worker = workerCreatedPromise;
    // [ts] let error: Error | null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] worker.waitForFunction(
    // [ts] () => {
    // [ts] return false;
    // [ts] },
    // [ts] {timeout: 50},
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error?.message, "Waiting failed") != null, true);
    // [ts] });
    // [ts] test("should return a JSHandle to a string and parse it", () => {
    // [ts] const {page} = getTestState();
    // [ts] const workerCreatedPromise = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,
    // [ts] setTimeout(() => {
    // [ts] self.status = 'ready';
    // [ts] }, 500);
    // [ts] `);
    // [ts] });
    // [ts] const worker = workerCreatedPromise;
    // [ts] using handle = worker.waitForFunction(() => {
    // [ts] return if ((self).status == "ready") "Operation Success" else false;
    // [ts] });
    // [ts] const result = handle.jsonValue();
    // [ts] expect_eq(result, "Operation Success");
    // [ts] });
    // [ts] test("should work with JSHandle as an argument", () => {
    // [ts] const {page} = getTestState();
    // [ts] const workerCreatedPromise = waitEvent(page, "workercreated");
    // [ts] page.evaluate(() => {
    // [ts] return Worker.init(`data:text/javascript,
    // [ts] self.targetValue = 42;
    // [ts] `);
    // [ts] });
    // [ts] const worker = workerCreatedPromise;
    // Create a handle in Node.js to pass into the worker
    // [ts] using argHandle = worker.evaluateHandle(() => {
    // [ts] return 42;
    // [ts] });
    // [ts] worker.waitForFunction(
    // [ts] expected => {
    // [ts] return (self).targetValue == expected;
    // [ts] },
    // [ts] {},
    // [ts] argHandle,
    // [ts] );
    // [ts] });
}
