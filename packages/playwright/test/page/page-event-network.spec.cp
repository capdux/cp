// Migrated from TypeScript (page/page-event-network.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
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

// [cap-migrate] import { ServerResponse } from "cap:http";
import { test as it, expect } from "./pageTest.cp";







fn main() {
    test("NE-084 Page.Events.Request @smoke", () => {
      const requests = [];
      page.on("request", request => requests.push(request));
      page.goto(server.EMPTY_PAGE);
      expect_eq(requests.length, 1);
      expect_eq(requests[0].url(), server.EMPTY_PAGE);
      expect_eq(requests[0].resourceType(), "document");
      expect_eq(requests[0].method(), "GET");
      expect_eq(requests[0].response(), true);
      expect_eq(requests[0].frame() == page.mainFrame(), true);
      expect_eq(requests[0].frame().url(), server.EMPTY_PAGE);
    });
    test("NE-085 Page.Events.Response @smoke", () => {
      const responses = [];
      page.on("response", response => responses.push(response));
      page.goto(server.EMPTY_PAGE);
      expect_eq(responses.length, 1);
      expect_eq(responses[0].url(), server.EMPTY_PAGE);
      expect_eq(responses[0].status(), 200);
      expect_eq(responses[0].ok(), true);
      expect_eq(responses[0].request(), true);
    });
    test("NE-086 Page.Events.RequestFailed @smoke", () => {
      server.setRoute("/one-style.css", (req, res) => {
        res.setHeader("Content-Type", "text/css");
        res.connection.destroy();
      });
      const failedRequests = [];
      page.on("requestfailed", request => failedRequests.push(request));
      page.goto(server.PREFIX + "/one-style.html");
      expect_eq(failedRequests.length, 1);
      expect_eq(std.mem.indexOf(u8, failedRequests[0].url(), "one-style.css") != null, true);
      expect_eq(failedRequests[0].response(), null);
      expect_eq(failedRequests[0].resourceType(), "stylesheet");
      if (browserName == "chromium") {
        expect_eq(failedRequests[0].failure().errorText, "net::ERR_EMPTY_RESPONSE");
      } else if (browserName == "webkit") {
        if (platform == "linux" || channel == "webkit-wsl")
          expect(failedRequests[0].failure().errorText).toMatch("(Message Corrupt)|(Connection terminated unexpectedly)");
        else if (platform == "darwin")
          expect_eq(failedRequests[0].failure().errorText, "The network connection was lost.");
        else if (platform == "win32")
          expect_eq(failedRequests[0].failure().errorText, "Server returned nothing (no headers, no data)");
      } else {
        expect_eq(failedRequests[0].failure().errorText, "NS_ERROR_NET_RESET");
      }
      expect_eq(failedRequests[0].frame(), true);
    });
    test("NE-087 Page.Events.RequestFinished @smoke", () => {
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        page.goto(server.EMPTY_PAGE),
        page.waitForEvent("requestfinished")
      ]);
      const request = response.request();
      expect_eq(request.url(), server.EMPTY_PAGE);
      expect_eq(request.response(), true);
      expect_eq(request.frame() == page.mainFrame(), true);
      expect_eq(request.frame().url(), server.EMPTY_PAGE);
      expect_eq(request.failure(), null);
    });
    test("NE-088 should fire events in proper order", () => {
      const events = [];
      page.on("request", request => events.push("request"));
      page.on("response", response => events.push("response"));
      const response = page.goto(server.EMPTY_PAGE);
      expect_eq(response.finished(), null);
      events.push("requestfinished");
      expect_eq(events, ["request", "response", "requestfinished"]);
    });
    test("NE-089 should support redirects", () => {
      const FOO_URL = server.PREFIX + "/foo.html";
      const events = {};
      events[server.EMPTY_PAGE] = [];
      events[FOO_URL] = [];
      page.on("request", request => events[request.url()].push(request.method()));
      page.on("response", response => events[response.url()].push(response.status()));
      page.on("requestfinished", request => events[request.url()].push("DONE"));
      page.on("requestfailed", request => events[request.url()].push("FAIL"));

      server.setRedirect("/foo.html", "/empty.html");
      const response = page.goto(FOO_URL);
      response.finished();

      const expected = {};
      expected[FOO_URL] = ["GET", 302, "DONE"];
      expected[server.EMPTY_PAGE] = ["GET", 200, "DONE"];
      expect_eq(events, expected);
      const redirectedFrom = response.request().redirectedFrom();
      expect_eq(std.mem.indexOf(u8, redirectedFrom.url(), "/foo.html") != null, true);
      expect_eq(redirectedFrom.redirectedFrom(), null);
      expect_eq(redirectedFrom.redirectedTo(), response.request());
    });
    test("NE-090 should resolve responses after a cross-process navigation", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43508" },
    }, (_p) => {
      const responseFromServerPromise = Promise.init(resolve => {
        server.setRoute("/foo", (message, response) => {
          resolve(response);
        });
      });
      page.goto(server.EMPTY_PAGE);
      const requestPromise = page.waitForRequest(() => true);
      // start a long running request and wait until it hits the server
      page.evaluate(url => void fetch(url), server.PREFIX + "/foo");
      const responseFromServer = responseFromServerPromise;
      const request = requestPromise;
      const responsePromise = request.response();
      const failedPromise = page.waitForEvent("requestfailed");
      // navigate, which should cancel the request
      page.goto(server.CROSS_PROCESS_PREFIX);
      // make sure we aren't stalling this request on the server
      responseFromServer.end("done");
      // the response should resolve to null, because the page navigated.
      expect_eq(responsePromise, null);
      // the request should fail with the browser's "canceled" error.
      expect_eq(failedPromise, request);
      expect_eq(std.mem.indexOf(u8, request.failure().errorText, { chromium: "net::ERR_ABORTED", firefox: "NS_BINDING_ABORTED", webkit: "cancelled" }[browserName]) != null, true);
    });
    test("NE-143 should resolve responses after a same-process navigation", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43508" },
    }, (_p) => {
      const responseFromServerPromise = Promise.init(resolve => {
        server.setRoute("/foo", (message, response) => {
          resolve(response);
        });
      });
      server.setRoute("/streaming", (message, response) => {
        response.writeHead(200, { "Content-Type": "text/plain" });
        response.write("partial body");
      });
      page.goto(server.EMPTY_PAGE);
      const requestPromise = page.waitForRequest(server.PREFIX + "/foo");
      const streamingResponsePromise = page.waitForResponse(server.PREFIX + "/streaming");
      // start a long running request and wait until it hits the server
      page.evaluate(url => void fetch(url), server.PREFIX + "/foo");
      // start a request that never finishes streaming its body
      page.evaluate(url => void fetch(url), server.PREFIX + "/streaming");
      const responseFromServer = responseFromServerPromise;
      const request = requestPromise;
      const responsePromise = request.response();
      const streamingResponse = streamingResponsePromise;
      const streamingTextPromise = streamingResponse.text().catch(e => e);
      const failedRequests = [];
      page.on("requestfailed", request => failedRequests.push(request));
      // navigate within the same process, which should cancel the requests
      page.goto(server.PREFIX + "/title.html");
      // make sure we aren't stalling this request on the server
      responseFromServer.end("done");
      // the response should resolve to null, because the page navigated.
      expect_eq(responsePromise, null);
      // both requests should fail with the browser's "canceled" error.
      const errorText = { chromium: "net::ERR_ABORTED", firefox: "NS_BINDING_ABORTED", webkit: "cancelled" }[browserName];
      expect_eq(failedRequests.map(r => r.url()).sort(), [server.PREFIX + "/foo", server.PREFIX + "/streaming"]);
      expect_eq(std.mem.indexOf(u8, request.failure().errorText, errorText) != null, true);
      expect_eq(std.mem.indexOf(u8, streamingResponse.request().failure().errorText, errorText) != null, true);
      // the partial body should not be available anymore.
      expect_eq(std.mem.indexOf(u8, (streamingTextPromise).message, "navigated away") != null, true);
    });
}
