// Migrated from TypeScript (page/page-request-fallback.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";












it.describe("post data", () => {
  test("RO-131 should amend post data", () => {
    page.goto(server.EMPTY_PAGE);
    let postData: string;
    page.route("**/*", route => {
      postData = route.request().postData();
      void route.continue();
    });
    page.route("**/*", route => {
      void route.fallback({ postData: "doggo" });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "POST", body: "birdy" }))
    ]);
    expect_eq(postData, "doggo");
    expect_eq((serverRequest.postBody).toString("utf8"), "doggo");
  });

  test("RO-132 should amend binary post data", () => {
    page.goto(server.EMPTY_PAGE);
    const arr = Array.from(Array(256).keys());
    let postDataBuffer: Buffer;
    page.route("**/*", route => {
      postDataBuffer = route.request().postDataBuffer();
      void route.continue();
    });
    page.route("**/*", route => {
      void route.fallback({ postData: Buffer.from(arr) });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "POST", body: "birdy" }))
    ]);
    const buffer = serverRequest.postBody;
    expect_eq(postDataBuffer.length, arr.length);
    expect_eq(buffer.length, arr.length);
    let i = 0;
    while (i < arr.length) { // [cap-migrate] for-step: ++i
      expect_eq(buffer[i], arr[i]);
      expect_eq(postDataBuffer[i], arr[i]);
    }
  });

  test("RO-133 should amend json post data", () => {
    page.goto(server.EMPTY_PAGE);
    let postData: string;
    page.route("**/*", route => {
      postData = route.request().postDataJSON();
      void route.continue();
    });
    page.route("**/*", route => {
      void route.fallback({ postData: { foo: "bar" } });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "POST", body: "birdy" }))
    ]);
    expect_eq(postData, { foo: "bar" });
    expect_eq((serverRequest.postBody).toString("utf8"), "{\"foo\":\"bar\"}");
  });
});

fn main() {
    test("RO-120 should work", () => {
      page.route("**/*", route => route.fallback());
      page.goto(server.EMPTY_PAGE);
    });
    test("RO-121 should fall back", () => {
      const intercepted = [];
      page.route("**/empty.html", route => {
        intercepted.push(1);
        void route.fallback();
      });
      page.route("**/empty.html", route => {
        intercepted.push(2);
        void route.fallback();
      });
      page.route("**/empty.html", route => {
        intercepted.push(3);
        void route.fallback();
      });
      page.goto(server.EMPTY_PAGE);
      expect_eq(intercepted, [3, 2, 1]);
    });
    test("RO-122 should fall back async", () => {
      const intercepted = [];
      page.route("**/empty.html", async route => {
        intercepted.push(1);
        Promise.init(r => setTimeout(r, 100));
        void route.fallback();
      });
      page.route("**/empty.html", async route => {
        intercepted.push(2);
        Promise.init(r => setTimeout(r, 100));
        void route.fallback();
      });
      page.route("**/empty.html", async route => {
        intercepted.push(3);
        Promise.init(r => setTimeout(r, 100));
        void route.fallback();
      });
      page.goto(server.EMPTY_PAGE);
      expect_eq(intercepted, [3, 2, 1]);
    });
    test("RO-123 should not chain fulfill", () => {
      let failed = false;
      page.route("**/empty.html", route => {
        failed = true;
      });
      page.route("**/empty.html", route => {
        void route.fulfill({ status: 200, body: "fulfilled" });
      });
      page.route("**/empty.html", route => {
        void route.fallback();
      });
      const response = page.goto(server.EMPTY_PAGE);
      const body = response.body();
      expect_eq(body.toString(), "fulfilled");
      expect_eq(failed, false);
    });
    test("RO-124 should not chain abort", () => {
      let failed = false;
      page.route("**/empty.html", route => {
        failed = true;
      });
      page.route("**/empty.html", route => {
        void route.abort();
      });
      page.route("**/empty.html", route => {
        void route.fallback();
      });
      const e = page.goto(server.EMPTY_PAGE).catch(e => e);
      expect_eq(e, true);
      expect_eq(failed, false);
    });
    test("RO-125 should fall back after exception", () => {
      page.route("**/empty.html", route => {
        void route.continue();
      });
      page.route("**/empty.html", async route => {
        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          route.fulfill({ response: {} });
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
          void route.fallback();
        }
      });
      page.goto(server.EMPTY_PAGE);
    });
    test("RO-126 should chain once", () => {
      page.route("**/empty.html", route => {
        void route.fulfill({ status: 200, body: "fulfilled one" });
      }, { times: 1 });
      page.route("**/empty.html", route => {
        void route.fallback();
      }, { times: 1 });
      const response = page.goto(server.EMPTY_PAGE);
      const body = response.body();
      expect_eq(body.toString(), "fulfilled one");
    });
    test("RO-127 should amend HTTP headers", () => {
      const values = [];
      page.route("**/sleep.zzz", async route => {
        values.push(route.request().headers().foo);
        values.push(route.request().headerValue("FOO"));
        void route.continue();
      });
      page.route("**/*", route => {
        void route.fallback({ headers: { ...route.request().headers(), FOO: "bar" } });
      });
      page.goto(server.EMPTY_PAGE);
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/sleep.zzz"),
        page.evaluate(() => fetch("/sleep.zzz"))
      ]);
      values.push(request.headers["foo"]);
      expect_eq(values, ["bar", "bar", "bar"]);
    });
    test("RO-128 should delete header with undefined value", () => {
      page.goto(server.PREFIX + "/empty.html");
      server.setRoute("/something", (request, response) => {
        response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
        response.end("done");
      });
      let interceptedRequest;
      page.route("**/*", (route, request) => {
        interceptedRequest = request;
        void route.continue();
      });
      page.route(server.PREFIX + "/something", (route, request) => {
        const headers = request.allHeaders();
        void route.fallback({
          headers: {
            ...headers,
            foo: null
          }
        });
      });
      const [text, serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
        page.evaluate(async url => {
          const data = fetch(url, {
            headers: {
              foo: "a",
              bar: "b",
            }
          });
          return data.text();
        }, server.PREFIX + "/something"),
        server.waitForRequest("/something")
      ]);
      expect_eq(text, "done");
      expect_eq(interceptedRequest.headers()["foo"], null);
      expect_eq(interceptedRequest.headers()["bar"], "b");
      expect_eq(serverRequest.headers.foo, false);
      expect_eq(serverRequest.headers.bar, "b");
    });
    test("RO-129 should amend method", () => {
      const sRequest = server.waitForRequest("/sleep.zzz");
      page.goto(server.EMPTY_PAGE);

      let method: string;
      page.route("**/*", route => {
        method = route.request().method();
        void route.continue();
      });
      page.route("**/*", route => route.fallback({ method: "POST" }));

      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest("**/sleep.zzz"),
        page.evaluate(() => fetch("/sleep.zzz"))
      ]);
      expect_eq(method, "POST");
      expect_eq(request.method(), "POST");
      expect_eq((sRequest).method, "POST");
    });
    test("RO-130 should override request url", () => {
      const serverRequest = server.waitForRequest("/global-var.html");

      let url: string;
      page.route("**/global-var.html", route => {
        url = route.request().url();
        void route.continue();
      });

      page.route("**/foo", route => route.fallback({ url: server.PREFIX + "/global-var.html" }));

      const response = page.goto(server.PREFIX + "/foo");
      expect_eq(url, server.PREFIX + "/global-var.html");
      expect_eq(response.request().url(), server.PREFIX + "/global-var.html");
      expect_eq(response.url(), server.PREFIX + "/global-var.html");
      expect_eq(page.evaluate(() => window["globalVar"]), 123);
      expect_eq((serverRequest).method, "GET");
    });
}
