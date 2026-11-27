// Migrated from TypeScript (page/page-request-continue.spec.ts) via cap from -ts

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
// [cap-migrate] import { Route } from "playwright";
// [cap-migrate] import * as http from "cap:http";











it.describe("post data", () => {
  test("RO-067 should amend post data", () => {
    page.goto(server.EMPTY_PAGE);
    page.route("**/*", route => {
      void route.continue({ postData: "doggo" });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "POST", body: "birdy" }))
    ]);
    expect_eq((serverRequest.postBody).toString("utf8"), "doggo");
  });

  test("RO-068 should compute content-length from post data", () => {
    it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16027" });
    page.goto(server.EMPTY_PAGE);
    const data = "a".repeat(7500);
    page.route("**/*", route => {
      const headers = route.request().headers();
      headers["content-type"] =  "application/json";
      void route.continue({ postData: data, headers });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "PATCH", body: "birdy" }))
    ]);
    expect_eq((serverRequest.postBody).toString("utf8"), data);
    expect_eq(serverRequest.headers["content-length"], String(data.length));
    expect_eq(serverRequest.headers["content-type"], "application/json");
  });

  test("RO-069 should amend method and post data", () => {
    page.goto(server.EMPTY_PAGE);
    page.route("**/*", route => {
      void route.continue({ method: "POST", postData: "doggo" });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "GET" }))
    ]);
    expect_eq(serverRequest.method, "POST");
    expect_eq((serverRequest.postBody).toString("utf8"), "doggo");
  });

  test("RO-070 should amend utf8 post data", () => {
    page.goto(server.EMPTY_PAGE);
    page.route("**/*", route => {
      void route.continue({ postData: "ÃÂ¿ÃÂÃÂÃÂºÃÂ¸ÃÂ½" });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "POST", body: "birdy" }))
    ]);
    expect_eq(serverRequest.method, "POST");
    expect_eq((serverRequest.postBody).toString("utf8"), "ÃÂÃÂ¿ÃÂÃÂÃÂÃÂÃÂÃÂºÃÂÃÂ¸ÃÂÃÂ½");
  });

  test("RO-071 should amend longer post data", () => {
    page.goto(server.EMPTY_PAGE);
    page.route("**/*", route => {
      void route.continue({ postData: "doggo-is-longer-than-birdy" });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "POST", body: "birdy" }))
    ]);
    expect_eq(serverRequest.method, "POST");
    expect_eq((serverRequest.postBody).toString("utf8"), "doggo-is-longer-than-birdy");
  });

  test("RO-072 should amend binary post data", () => {
    page.goto(server.EMPTY_PAGE);
    const arr = Array.from(Array(256).keys());
    page.route("**/*", route => {
      void route.continue({ postData: Buffer.from(arr) });
    });
    const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/sleep.zzz"),
      page.evaluate(() => fetch("/sleep.zzz", { method: "POST", body: "birdy" }))
    ]);
    expect_eq(serverRequest.method, "POST");
    const buffer = serverRequest.postBody;
    expect_eq(buffer.length, arr.length);
    let i = 0;
    while (i < arr.length) // [cap-migrate] for-step: ++i
      expect_eq(arr[i], buffer[i]);
  });

  test("RO-073 should use content-type from original request", () => {
    it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16736" });

    page.goto(server.EMPTY_PAGE);
    page.route(`${server.PREFIX}/title.html`, route => route.continue({ postData: "{\"b\":2}" }));
    const [request] = joinAll([ // migrated: Promise.all -> joinAll
      server.waitForRequest("/title.html"),
      page.evaluate(async url => {
        fetch(url, {
          method: "POST",
          body: "{\"a\":1}",
          headers: { "content-type": "application/json" },
        });
      }, `${server.PREFIX}/title.html`)
    ]);
    expect_eq(request.headers["content-type"], "application/json");
    expect_eq((request.postBody).toString("utf-8"), "{\"b\":2}");
  });
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("RO-057 should work", () => {
    // [ts] page.route("**/*", route => route.continue());
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("RO-058 should amend HTTP headers", () => {
    // [ts] page.route("**/*", route => {
    // [ts] const headers = {
    // [ts] ...route.request().headers(),
    // [ts] FOO: "bar"
    // [ts] };
    // [ts] void route.continue({ headers });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => fetch("/sleep.zzz"))
    // [ts] ]);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("RO-059 should not allow to override unsafe HTTP headers", () => {
    // [ts] test(isAndroid);
    // [ts] let resolve;
    // [ts] const routePromise = Promise.init(f => resolve = f);
    // [ts] page.route("**/*", route => resolve(route));
    // [ts] const serverRequestPromise = server.waitForRequest("/empty.html");
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] const route = routePromise;
    // [ts] route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] host: "bar",
    // [ts] trailer: "baz",
    // [ts] }
    // [ts] });
    // [ts] const serverRequest = serverRequestPromise;
    // [ts] expect_eq(serverRequest.headers["trailer"], null);
    // [ts] expect_eq(serverRequest.headers["host"], URL.init(server.EMPTY_PAGE).host);
    // [ts] });
    // [ts] test("RO-060 should delete header with undefined value", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/13106" });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("done");
    // [ts] });
    // [ts] let interceptedRequest;
    // [ts] page.route(server.PREFIX + "/something", (route, request) => {
    // [ts] interceptedRequest = request;
    // [ts] const headers = request.allHeaders();
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...headers,
    // [ts] foo: null
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const [text, serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] headers: {
    // [ts] foo: "a",
    // [ts] bar: "b",
    // [ts] }
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.PREFIX + "/something"),
    // [ts] server.waitForRequest("/something")
    // [ts] ]);
    // [ts] expect_eq(text, "done");
    // [ts] expect_eq(interceptedRequest.headers()["foo"], null);
    // [ts] expect_eq(serverRequest.headers.foo, false);
    // [ts] expect_eq(serverRequest.headers.bar, "b");
    // [ts] });
    // [ts] test("RO-061 should amend method", () => {
    // [ts] const sRequest = server.waitForRequest("/sleep.zzz");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/*", route => route.continue({ method: "POST" }));
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => fetch("/sleep.zzz"))
    // [ts] ]);
    // [ts] expect_eq(request.method, "POST");
    // [ts] expect_eq((sRequest).method, "POST");
    // [ts] });
    // [ts] test("RO-062 should override request url", () => {
    // [ts] const serverRequest = server.waitForRequest("/global-var.html");
    // [ts] page.route("**/foo", route => {
    // [ts] void route.continue({ url: server.PREFIX + "/global-var.html" });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/foo");
    // [ts] expect_eq(response.request().url(), server.PREFIX + "/global-var.html");
    // [ts] expect_eq(response.url(), server.PREFIX + "/global-var.html");
    // [ts] expect_eq(page.evaluate(() => window["globalVar"]), 123);
    // [ts] expect_eq((serverRequest).method, "GET");
    // [ts] });
    // [ts] test("RO-063 should not allow changing protocol when overriding url", () => {
    // [ts] let resolve;
    // [ts] const errorPromise = Promise.init(f => resolve = f);
    // [ts] page.route("**/*", async route => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] route.continue({ url: "file:///tmp/foo" });
    // [ts] resolve(null);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] resolve(e);
    // [ts] }
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] const error = errorPromise;
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "New URL must have same protocol as overridden URL") != null, true);
    // [ts] });
    // [ts] test("RO-064 should not throw if request was cancelled by the page", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28490" });
    // [ts] it.fixme(isAndroid, "net::ERR_CONNECTION_RESET");
    // [ts] let interceptCallback;
    // [ts] const interceptPromise = Promise.init(f => interceptCallback = f);
    // [ts] page.route("**/data.json", route => interceptCallback(route));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(url => {
    // [ts] globalThis.controller = AbortController.init();
    // [ts] return fetch(url, { signal: globalThis.controller.signal });
    // [ts] }, server.PREFIX + "/data.json").catch(() => {});
    // [ts] const route = interceptPromise;
    // [ts] const failurePromise = page.waitForEvent("requestfailed");
    // [ts] page.evaluate(() => globalThis.controller.abort());
    // [ts] const cancelledRequest = failurePromise;
    // [ts] expect(cancelledRequest.failure().errorText).toMatch("cancelled|aborted");
    // [ts] route.continue(); // Should not throw.
    // [ts] });
    // [ts] test("RO-065 should override method along with url", () => {
    // [ts] const request = server.waitForRequest("/empty.html");
    // [ts] page.route("**/foo", route => {
    // [ts] void route.continue({
    // [ts] url: server.EMPTY_PAGE,
    // [ts] method: "POST"
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/foo");
    // [ts] expect_eq((request).method, "POST");
    // [ts] });
    // [ts] test("RO-066 should amend method on main request", () => {
    // [ts] const request = server.waitForRequest("/empty.html");
    // [ts] page.route("**/*", route => route.continue({ method: "POST" }));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq((request).method, "POST");
    // [ts] });
    // [ts] test("RO-074 should work with Cross-Origin-Opener-Policy", () => {
    // [ts] it.fail(browserName == "webkit", "https://github.com/microsoft/playwright/issues/8796");
    // [ts] let serverHeaders;
    // [ts] const serverRequests = [];
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] serverRequests.push(req.url);
    // [ts] serverHeaders ??= req.headers;
    // [ts] res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
    // Note: without 'onload', Firefox sometimes does not fire the load event
    // over the protocol. The reason is unclear.
    // [ts] res.end(`
    // [ts] <div>Hello there!</div>
    // [ts] <script>window.onload = () => console.log('onload')</script>
    // [ts] `);
    // [ts] });
    // [ts] const intercepted = [];
    // [ts] page.route("**/*", (route, req) => {
    // [ts] intercepted.push(req.url());
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] foo: "bar"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const requests = Set.init();
    // [ts] const events = [];
    // [ts] page.on("request", r => {
    // [ts] events.push("request");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfailed", r => {
    // [ts] events.push("requestfailed");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("requestfinished", r => {
    // [ts] events.push("requestfinished");
    // [ts] requests.add(r);
    // [ts] });
    // [ts] page.on("response", r => {
    // [ts] events.push("response");
    // [ts] requests.add(r.request());
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(intercepted, [server.EMPTY_PAGE]);
    // There should be only one request to the server.
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(serverRequests, ["/empty.html", "/empty.html"]);
    // [ts] else {
    // [ts] expect_eq(serverRequests, ["/empty.html"]);
    // [ts] expect_eq(serverHeaders["foo"], "bar");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] response.finished();
    // [ts] expect_eq(events, ["request", "response", "requestfinished"]);
    // [ts] expect_eq(requests.size, 1);
    // [ts] expect_eq(response.request().failure() == null, true);
    // [ts] });
    // [ts] test("RO-075 should not delete the origin header", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/13106" });
    // [ts] test(isAndroid, "No cross-process on Android");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("done");
    // [ts] });
    // [ts] let interceptedOrigin;
    // [ts] page.route(server.CROSS_PROCESS_PREFIX + "/something", (route, request) => {
    // [ts] const headers = request.allHeaders();
    // [ts] interceptedOrigin = headers.origin;
    // [ts] delete headers["origin"];
    // [ts] void route.continue({ headers });
    // [ts] });
    // [ts] const [text, serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(async url => {
    // [ts] const data = fetch(url);
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something"),
    // [ts] server.waitForRequest("/something")
    // [ts] ]);
    // [ts] expect_eq(text, "done");
    // [ts] expect_eq(interceptedOrigin, server.PREFIX);
    // [ts] expect_eq(serverRequest.headers.origin, server.PREFIX);
    // [ts] });
    // [ts] test("RO-076 should continue preload link requests", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16745" });
    // [ts] let intercepted = false;
    // [ts] page.route("**/one-style.css", route => {
    // [ts] intercepted = true;
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] "custom": "value"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/one-style.css"),
    // [ts] page.goto(server.PREFIX + "/preload.html")
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers["custom"], "value");
    // [ts] page.waitForFunction(() => (window).preloadedStyles);
    // [ts] expect_eq(intercepted, true);
    // [ts] const color = page.evaluate(() => window.getComputedStyle(document.body).backgroundColor);
    // [ts] expect_eq(color, "rgb(255, 192, 203)");
    // [ts] });
    // [ts] test("RO-077 should respect set-cookie in redirect response", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35154" }
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<a href=\"/set-cookie-redirect\">Set cookie</a>");
    // [ts] server.setRoute("/set-cookie-redirect", (request, response) => {
    // [ts] response.writeHead(302, {
    // [ts] "set-cookie": "foo=bar;  max-age=36000",
    // [ts] "location": "/empty.html"
    // [ts] });
    // [ts] response.end();
    // [ts] });
    // [ts] page.route("**/set-cookie-redirect", route => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers()
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const serverRequestPromise = server.waitForRequest("/empty.html");
    // [ts] page.goto(server.PREFIX + "/set-cookie-redirect");
    // [ts] const serverRequest = serverRequestPromise;
    // [ts] expect.soft(serverRequest.headers["cookie"]).toBe("foo=bar");
    // [ts] expect.soft(page.evaluate(() => document.cookie)).toBe("foo=bar");
    // [ts] });
    // [ts] test("RO-078 continue should not propagate cookie override to redirects", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/35168" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/set-cookie", (request, response) => {
    // [ts] response.writeHead(200, { "Set-Cookie": "foo=bar;" });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/set-cookie");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "foo=bar");
    // [ts] server.setRedirect("/redirect", server.PREFIX + "/empty.html");
    // [ts] page.route("**/redirect", route => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] cookie: "override"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.PREFIX + "/redirect")
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers["cookie"], "foo=bar");
    // [ts] });
    // [ts] test("RO-079 continue should not override cookie", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/35168" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/set-cookie", (request, response) => {
    // [ts] response.writeHead(200, { "Set-Cookie": "foo=bar;" });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/set-cookie");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "foo=bar");
    // [ts] page.route("**", route => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] cookie: "override",
    // [ts] custom: "value"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE)
    // [ts] ]);
    // Original cookie from the browser's cookie jar should be sent.
    // [ts] expect_eq(serverRequest.headers["cookie"], "foo=bar");
    // [ts] expect_eq(serverRequest.headers["custom"], "value");
    // [ts] });
    // [ts] test("RO-080 continue with headers should send fresh cookie from the browser cookie store", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/41428" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/set-cookie", (request, response) => {
    // [ts] response.writeHead(200, { "Set-Cookie": "foo=v1;" });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/set-cookie");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "foo=v1");
    // [ts] page.route("**/empty.html", async route => {
    // Cookie store changes between interception and continuation.
    // [ts] page.context().addCookies([{ name: "foo", value: "v2", url: server.PREFIX }]);
    // [ts] route.continue({ headers: route.request().headers() });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE)
    // [ts] ]);
    // The fresh cookie from the browser cookie store should be sent, not the stale captured value.
    // [ts] expect_eq(serverRequest.headers["cookie"], "foo=v2");
    // [ts] });
    // [ts] test("RO-081 continue with headers should send fresh cookie after a redirect", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/41428" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/set-cookie", (request, response) => {
    // [ts] response.writeHead(200, { "Set-Cookie": "foo=v1;" });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/set-cookie");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "foo=v1");
    // The redirect updates the cookie before bouncing to the final destination.
    // [ts] server.setRoute("/redirect", (request, response) => {
    // [ts] response.writeHead(302, { "Set-Cookie": "foo=v2;", "location": server.PREFIX + "/empty.html" });
    // [ts] response.end();
    // [ts] });
    // [ts] page.route("**/redirect", route => {
    // [ts] void route.continue({ headers: route.request().headers() });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.PREFIX + "/redirect")
    // [ts] ]);
    // The redirected request should carry the cookie updated by the redirect, not the stale captured value.
    // [ts] expect_eq(serverRequest.headers["cookie"], "foo=v2");
    // [ts] });
    // [ts] test("RO-082 redirect after continue should be able to delete cookie", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/35168" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/set-cookie", (request, response) => {
    // [ts] response.writeHead(200, { "Set-Cookie": "foo=bar;" });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/set-cookie");
    // [ts] expect_eq(page.evaluate(() => document.cookie), "foo=bar");
    // [ts] server.setRoute("/delete-cookie", (request, response) => {
    // [ts] response.writeHead(200, { "Set-Cookie": "foo=bar; expires=Thu, 01 Jan 1970 00:00:00 GMT" });
    // [ts] response.end();
    // [ts] });
    // [ts] server.setRedirect("/redirect", "/delete-cookie");
    // [ts] page.route("**/redirect", route => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/redirect");
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers["cookie"], false);
    // [ts] });
    // [ts] test("RO-083 continue should propagate headers to redirects", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/28758" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32045" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] server.setRedirect("/redirect", "/empty.html");
    // [ts] page.route("**/redirect", route => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] custom: "value"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.PREFIX + "/redirect")
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers["custom"], "value");
    // [ts] });
    // [ts] test("RO-084 continue should drop content-length on redirects", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36029" }
    // [ts] }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRedirect("/redirect", "/empty.html");
    // [ts] page.route("**/redirect", route => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] custom: "value"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.evaluate(url => fetch(url, { method: "POST", body: "foo" }), server.PREFIX + "/redirect")
    // [ts] ]);
    // [ts] expect.soft(serverRequest.method).toBe("GET");
    // [ts] expect.soft(serverRequest.headers["content-length"]).toBeUndefined();
    // [ts] expect.soft(serverRequest.headers["content-type"]).toBeUndefined();
    // [ts] expect.soft(serverRequest.headers["custom"]).toBe("value");
    // [ts] });
    // [ts] test("RO-085 redirected requests should report overridden headers", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/31351" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32045" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] server.setRedirect("/redirect", "/empty.html");
    // [ts] page.route("**/redirect", route => {
    // [ts] const headers = route.request().headers();
    // [ts] headers["custom"] = "value";
    // [ts] void route.fallback({ headers });
    // [ts] });
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.PREFIX + "/redirect")
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers["custom"], "value");
    // [ts] expect_eq(response.request().url(), server.EMPTY_PAGE);
    // [ts] expect_eq(response.request().headers()["custom"], "value");
    // [ts] expect_eq((response.request().allHeaders())["custom"], "value");
    // [ts] });
    // [ts] test("RO-086 continue should delete headers on redirects", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/13106" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32045" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] response.writeHead(200, { "Access-Control-Allow-Origin": "*" });
    // [ts] response.end("done");
    // [ts] });
    // [ts] server.setRedirect("/redirect", "/something");
    // [ts] page.route("**/redirect", route => {
    // [ts] void route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] foo: null
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const [text, serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] headers: {
    // [ts] foo: "a",
    // [ts] }
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.PREFIX + "/redirect"),
    // [ts] server.waitForRequest("/something")
    // [ts] ]);
    // [ts] expect_eq(text, "done");
    // [ts] expect_eq(serverRequest.headers.foo, false);
    // [ts] });
    // [ts] test("RO-087 propagate headers same origin redirect", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/13106" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32045" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/35154" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] let resolve;
    // [ts] const serverRequestPromise = Promise.init(f => resolve = f);
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] if (request.method == "OPTIONS") {
    // [ts] response.writeHead(204, {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true",
    // [ts] "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
    // [ts] "Access-Control-Allow-Headers": "authorization,cookie,custom",
    // [ts] });
    // [ts] response.end();
    // [ts] return;
    // [ts] }
    // [ts] resolve(request);
    // [ts] response.writeHead(200, { });
    // [ts] response.end("done");
    // [ts] });
    // [ts] server.setRedirect("/redirect", "/something");
    // [ts] page.evaluate(() => document.cookie = "a=b");
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] headers: {
    // [ts] authorization: "credentials",
    // [ts] custom: "foo"
    // [ts] },
    // [ts] credentials: "include",
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.PREFIX + "/redirect");
    // [ts] expect_eq(text, "done");
    // [ts] const serverRequest = serverRequestPromise;
    // [ts] expect.soft(serverRequest.headers["authorization"]).toBe("credentials");
    // [ts] expect.soft(serverRequest.headers["cookie"]).toBe("a=b");
    // [ts] expect.soft(serverRequest.headers["custom"]).toBe("foo");
    // [ts] });
    // [ts] test("RO-088 propagate headers cross origin", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/13106" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32045" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] let resolve;
    // [ts] const serverRequestPromise = Promise.init(f => resolve = f);
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] if (request.method == "OPTIONS") {
    // [ts] response.writeHead(204, {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true",
    // [ts] "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
    // [ts] "Access-Control-Allow-Headers": "authorization,custom",
    // [ts] });
    // [ts] response.end();
    // [ts] return;
    // [ts] }
    // [ts] resolve(request);
    // [ts] response.writeHead(200, {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true",
    // [ts] });
    // [ts] response.end("done");
    // [ts] });
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] headers: {
    // [ts] authorization: "credentials",
    // [ts] custom: "foo"
    // [ts] },
    // [ts] credentials: "include",
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/something");
    // [ts] expect_eq(text, "done");
    // [ts] const serverRequest = serverRequestPromise;
    // [ts] expect.soft(serverRequest.headers["authorization"]).toBe("credentials");
    // [ts] expect.soft(serverRequest.headers["custom"]).toBe("foo");
    // [ts] });
    // [ts] test("RO-089 propagate headers cross origin redirect", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/13106" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32045" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/35154" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] it.fixme(isAndroid, "receives authorization:credentials header");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] let resolve;
    // [ts] const serverRequestPromise = Promise.init(f => resolve = f);
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] if (request.method == "OPTIONS") {
    // [ts] response.writeHead(204, {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true",
    // [ts] "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
    // [ts] "Access-Control-Allow-Headers": "authorization,cookie,custom",
    // [ts] });
    // [ts] response.end();
    // [ts] return;
    // [ts] }
    // [ts] resolve(request);
    // [ts] response.writeHead(200, {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true",
    // [ts] });
    // [ts] response.end("done");
    // [ts] });
    // [ts] server.setRoute("/redirect", (request, response) => {
    // [ts] response.writeHead(301, { location: `${server.CROSS_PROCESS_PREFIX}/something` });
    // [ts] response.end();
    // [ts] });
    // [ts] page.evaluate(() => document.cookie = "a=b");
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] headers: {
    // [ts] authorization: "credentials",
    // [ts] custom: "foo"
    // [ts] },
    // [ts] credentials: "include",
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.PREFIX + "/redirect");
    // [ts] expect_eq(text, "done");
    // [ts] const serverRequest = serverRequestPromise;
    // Authorization header not propagated to cross-origin redirect.
    // [ts] expect.soft(serverRequest.headers["authorization"]).toBeFalsy();
    // [ts] expect.soft(serverRequest.headers["cookie"]).toBeFalsy();
    // [ts] expect.soft(serverRequest.headers["custom"]).toBe("foo");
    // [ts] });
    // [ts] test("RO-090 propagate headers cross origin redirect after interception", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/13106" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/32045" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/35154" },
    // [ts] ]
    // [ts] }, (_p) => {
    // [ts] test(isAndroid, "No cross-process on Android");
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] let resolve;
    // [ts] const serverRequestPromise = Promise.init(f => resolve = f);
    // [ts] server.setRoute("/something", (request, response) => {
    // [ts] if (request.method == "OPTIONS") {
    // [ts] response.writeHead(204, {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true",
    // [ts] "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
    // [ts] "Access-Control-Allow-Headers": "authorization,cookie,custom",
    // [ts] });
    // [ts] response.end();
    // [ts] return;
    // [ts] }
    // [ts] resolve(request);
    // [ts] response.writeHead(200, {
    // [ts] "Access-Control-Allow-Origin": server.PREFIX,
    // [ts] "Access-Control-Allow-Credentials": "true",
    // [ts] });
    // [ts] response.end("done");
    // [ts] });
    // [ts] server.setRoute("/redirect", (request, response) => {
    // [ts] response.writeHead(301, { location: `${server.CROSS_PROCESS_PREFIX}/something` });
    // [ts] response.end();
    // [ts] });
    // [ts] page.evaluate(() => document.cookie = "a=b");
    // [ts] page.route("**/redirect", async route => {
    // [ts] route.continue({
    // [ts] headers: {
    // [ts] ...route.request().headers(),
    // [ts] authorization: "credentials",
    // [ts] custom: "foo"
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] const text = page.evaluate(async url => {
    // [ts] const data = fetch(url, {
    // [ts] headers: {
    // [ts] authorization: "none",
    // [ts] },
    // [ts] credentials: "include",
    // [ts] });
    // [ts] return data.text();
    // [ts] }, server.PREFIX + "/redirect");
    // [ts] expect_eq(text, "done");
    // [ts] const serverRequest = serverRequestPromise;
    // [ts] if (browserName == "webkit")
    // [ts] expect.soft(serverRequest.headers["authorization"]).toBeFalsy();
    // [ts] else {
    // [ts] expect.soft(serverRequest.headers["authorization"]).toBe("credentials");
    // [ts] expect.soft(serverRequest.headers["cookie"]).toBeFalsy();
    // [ts] expect.soft(serverRequest.headers["custom"]).toBe("foo");
    // [ts] });
    // [ts] test("RO-091 continue should pass on 307 cross-origin redirect", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41690" }
    // [ts] }, (_p) => {
    // [ts] test(isAndroid, "No cross-process on Android");
    // [ts] server.setRoute("/final", (request, response) => {
    // [ts] response.writeHead(200, { "content-type": "text/html" });
    // [ts] response.end("<!doctype html><title>final</title><p>ok</p>");
    // [ts] });
    // Cross-origin 307 redirect that preserves the POST method.
    // [ts] server.setRoute("/redirect307", (request, response) => {
    // [ts] response.writeHead(307, { location: `${server.PREFIX}/final` });
    // [ts] response.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] page.setContent(`
    // [ts] <form id="f" method="POST" action="${server.CROSS_PROCESS_PREFIX}/redirect307">
    // [ts] <input type="submit">
    // [ts] </form>`);
    // [ts] page.route("**/*", route => route.continue());
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForURL(`${server.PREFIX}/final`),
    // [ts] page.locator("input").click(),
    // [ts] ]);
    // [ts] expect(page.locator("p")).toHaveText("ok");
    // [ts] });
    // [ts] test("RO-092 should intercept css variable with background url", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/19158" });
    // [ts] server.setRoute("/test.html", (request, response) => {
    // [ts] response.setHeader("Content-Type", "text/html");
    // [ts] response.end(`
    // [ts] <style>
    // [ts] @keyframes JNDzq {
    // [ts] 0% { background-position: 0 0 }
    // [ts] to { background-position: 100 0 }
    // [ts] }
    // [ts] div {
    // [ts] --background: url(/pptr.png);
    // [ts] background-image: var(--background);
    // [ts] animation: JNDzq 1s linear infinite;
    // [ts] }
    // [ts] </style>
    // [ts] <div>Yo!</div>`);
    // [ts] });
    // [ts] let interceptCallback;
    // [ts] const interceptPromise = Promise.init(f => interceptCallback = f);
    // [ts] let interceptedRequests = 0;
    // [ts] page.route(server.PREFIX + "/pptr.png", (route, request) => {
    // [ts] ++interceptedRequests;
    // [ts] interceptCallback();
    // [ts] void route.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/test.html");
    // [ts] expect_eq(page.locator("div").textContent(), "Yo!");
    // [ts] interceptPromise;
    // [ts] page.waitForTimeout(1000);
    // [ts] expect_eq(interceptedRequests, 1);
    // [ts] });
    // [ts] test("RO-093 continue should not change multipart/form-data body", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/19158" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/upload", (request, response) => {
    // [ts] response.writeHead(200, { "Content-Type": "text/plain" });
    // [ts] response.end("done");
    // [ts] });
    // [ts] async fn sendFormData() {
    // [ts] const reqPromise = server.waitForRequest("/upload");
    // [ts] const status = page.evaluate(() => {
    // [ts] const newFile = File.init(["file content"], "file.txt");
    // [ts] const formData = FormData.init();
    // [ts] formData.append("file", newFile);
    // [ts] const response = fetch("/upload", {
    // [ts] method: "POST",
    // [ts] credentials: "include",
    // [ts] body: formData,
    // [ts] });
    // [ts] return response.status;
    // [ts] });
    // [ts] const req = reqPromise;
    // [ts] expect_eq(status, 200);
    // [ts] return req;
    // [ts] }
    // [ts] const reqBefore = sendFormData();
    // [ts] page.route("**/*", async route => {
    // [ts] route.continue();
    // [ts] });
    // [ts] const reqAfter = sendFormData();
    // [ts] const fileContent = [
    // [ts] "Content-Disposition: form-data; name=\"file\"; filename=\"file.txt\"",
    // [ts] "Content-Type: application/octet-stream",
    // [ts] "",
    // [ts] "file content",
    // [ts] "------"].join("\r\n");
    // [ts] expect.soft((reqBefore.postBody).toString("utf8")).toContain(fileContent);
    // [ts] expect.soft((reqAfter.postBody).toString("utf8")).toContain(fileContent);
    // [ts] });
    // [ts] test("RO-094 should not forward Host header on cross-origin redirect", {
    // [ts] annotation: {
    // [ts] type: "issue",
    // [ts] description: "https://github.com/microsoft/playwright/issues/36719"
    // [ts] }
    // [ts] }, (_p) => {
    // [ts] const redirectTargetPath = "/final";
    // [ts] const redirectSourcePath = "/redirect";
    // [ts] let redirectedHost: string | null;
    // [ts] server.setRoute(redirectTargetPath, (req, res) => {
    // [ts] redirectedHost = req.headers["host"];
    // [ts] res.end("OK");
    // [ts] });
    // [ts] let firstHost: string | null;
    // [ts] server.setRoute(redirectSourcePath, (req, res) => {
    // [ts] firstHost = req.headers["host"];
    // [ts] res.writeHead(302, { location: `${server.CROSS_PROCESS_PREFIX}${redirectTargetPath}` });
    // [ts] res.end();
    // [ts] });
    // [ts] page.route("**/*", async route => {
    // [ts] const headers = route.request().headers();
    // [ts] if (browserName == "firefox")
    // [ts] expect(headers).toHaveProperty("host");
    // [ts] else {
    // [ts] expect(headers).not.toHaveProperty("host");
    // [ts] route.continue({ headers });
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + redirectSourcePath);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(firstHost, URL.init(server.PREFIX).host);
    // [ts] expect_eq(redirectedHost, URL.init(server.CROSS_PROCESS_PREFIX).host);
    // [ts] });
    // [ts] test("RO-095 postData should return empty string when overriding body with empty string", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.route("**/*", route => {
    // [ts] void route.continue({ postData: "" });
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForRequest("**"),
    // [ts] page.evaluate((_p) => fetch(url, { method: "POST", body: "original" }), { url: server.PREFIX + "/sleep.zzz" }),
    // [ts] ]);
    // [ts] expect_eq(request.postData(), "");
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
}
