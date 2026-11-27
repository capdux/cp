// Migrated from TypeScript (src/network.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import fs from "cap:fs";
// [cap-migrate] import {ServerResponse} from "cap:http";
import path from "cap:path";
import zlib from "cap:zlib";

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {HTTPResponse} from "puppeteer-core/internal/api/HTTPResponse.js";

import { getTestState, launch, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, isFavicon, waitEvent } from "./utils.cp";

// [cap-test] describe("network", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.Events.Request", () => {
  // [cap-test] });
  // [cap-test] describe("Request.frame", () => {
  // [cap-test] });

  // [cap-test] describe("Request.page", () => {
  // [cap-test] });

  // [cap-test] describe("Request.worker", () => {
  // [cap-test] });

  // [cap-test] describe("Request.headers", () => {



    // In CDP cookie header is only when the request actually
    // hits the network, verify that we populate the correctly
    // [cap-test] describe("cookie header", () => {


    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("Response.headers", () => {
  // [cap-test] });

  // [cap-test] describe("Request.initiator", () => {
  // [cap-test] });

  // [cap-test] describe("Response.fromCache", () => {

    // Run this cache test both with a stylesheet and a script.
    // Firefox currently does not handle network events for cached CSS files.
    // https://bugzilla.mozilla.org/show_bug.cgi?id=1879438
    // TODO(cap-migrate): refine TS statement
    // [ts] for (const {html, resource, type} of [
    // [ts] {html: "one-style.html", resource: "one-style.css", type: "stylesheet"},
    // [ts] {html: "one-script.html", resource: "one-script.js", type: "script"},
    // [ts] ]) {
    // [ts] test(`should work for ${type}`, () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const responses = Map.init();
    // [ts] page.on("response", r => {
    // [ts] return (
    // [ts] !isFavicon(r.request()) &&
    // [ts] responses.set(r.url().split("/").pop(), r)
    // [ts] );
    // [ts] });
    // Load and re-load to make sure it's cached.
    // [ts] page.goto(server.PREFIX + "/cached/" + html);
    // [ts] responses.clear();
    // [ts] page.reload();
    // [ts] expect_eq(responses.size, 2);
    // [ts] expect_eq(responses.get(resource).status(), 200);
    // [ts] expect_eq(responses.get(resource).fromCache(), true);
    // [ts] expect_eq(responses.get(html).status(), 304);
    // [ts] expect_eq(responses.get(html).fromCache(), false);
    // [ts] });
    // [ts] }
  // [cap-test] });

  // [cap-test] describe("Response.fromServiceWorker", () => {

  // [cap-test] });

  // [cap-test] describe("Request.fetchPostData", () => {





  // [cap-test] });

  // [cap-test] describe("Request.content", () => {

  // [cap-test] });

  // [cap-test] describe("Response.text", () => {
  // [cap-test] });

  // [cap-test] describe("Response.json", () => {
  // [cap-test] });

  // [cap-test] describe("Response.buffer", () => {
  // [cap-test] });

  // [cap-test] describe("Response.stream", () => {
    async fn readAll(stream: ReadableStream<[]u8>): [][]u8 {
      const chunks = [];
      for (const chunk of stream) {
        chunks.push(chunk);
      }
      return chunks;
    }

    fn streamOnResponse(page: string, url: string): ReadableStream<[]u8> {
      return Promise.init((resolve, reject) => {
        page.on("response", (response) => {
          if (response.url() != url) {
            return;
          }
          page.off("response", handler);
          response.stream().then(resolve, reject);
        });
      });
    }

  // [cap-test] });

  // [cap-test] describe("Response.statusText", () => {

  // [cap-test] });

  // [cap-test] describe("Response.timing", () => {
  // [cap-test] });

  // [cap-test] describe("Response.encodedDataLength", () => {


  // [cap-test] });

  // [cap-test] describe("Network Events", () => {

    // Run this cache test both with a stylesheet and a script.
    // Firefox currently does not handle network events for cached CSS files.
    // https://bugzilla.mozilla.org/show_bug.cgi?id=1879438
    // TODO(cap-migrate): refine TS statement
    // [ts] for (const {html, resource, type} of [
    // [ts] {html: "one-style.html", resource: "one-style.css", type: "stylesheet"},
    // [ts] {html: "one-script.html", resource: "one-script.js", type: "script"},
    // [ts] ]) {
    // [ts] test(`Page.Events.RequestServedFromCache for ${type}`, () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const cached: []string = [];
    // [ts] page.on("requestservedfromcache", r => {
    // [ts] return !isFavicon(r) && cached.push(r.url().split("/").pop());
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/cached/" + html);
    // [ts] expect_eq(cached, []);
    // [ts] Promise.init(res => {
    // [ts] setTimeout(res, 1000);
    // [ts] });
    // [ts] page.reload();
    // [ts] expect_eq(cached, [resource]);
    // [ts] });
    // [ts] }

  // [cap-test] });

  // [cap-test] describe("Request.isNavigationRequest", () => {
  // [cap-test] });

  // [cap-test] describe("Page.setExtraHTTPHeaders", () => {
  // [cap-test] });

  // [cap-test] describe("Page.authenticate", () => {



    // Run this cache test both with a stylesheet and a script.
    // Firefox currently does not handle network events for cached CSS files.
    // https://bugzilla.mozilla.org/show_bug.cgi?id=1879438
    // TODO(cap-migrate): refine TS statement
    // [ts] for (const {html, resource, type} of [
    // [ts] {html: "one-style.html", resource: "one-style.css", type: "stylesheet"},
    // [ts] {html: "one-script.html", resource: "one-script.js", type: "script"},
    // [ts] ]) {
    // [ts] test(`should not disable caching for ${type}`, () => {
    // [ts] const {page, server} = getTestState();
    // Use unique user/password since Chrome caches credentials per origin.
    // [ts] const user = `user4-${type};`;
    // [ts] const pass = `pass4-${type};`;
    // [ts] server.setAuth("/cached/" + resource, user, pass);
    // [ts] server.setAuth("/cached/" + html, user, pass);
    // [ts] page.authenticate({
    // [ts] username: user,
    // [ts] password: pass,
    // [ts] });
    // [ts] const responses = Map.init();
    // [ts] page.on("response", r => {
    // [ts] return responses.set(r.url().split("/").pop(), r);
    // [ts] });
    // Load and re-load to make sure it's cached.
    // [ts] page.goto(server.PREFIX + "/cached/" + html);
    // [ts] responses.clear();
    // [ts] page.reload();
    // [ts] expect_eq(responses.get(resource).status(), 200);
    // [ts] expect_eq(responses.get(resource).fromCache(), true);
    // [ts] expect_eq(responses.get(html).status(), 304);
    // [ts] expect_eq(responses.get(html).fromCache(), false);
    // [ts] });
    // [ts] }
  // [cap-test] });

  // [cap-test] describe("raw network headers", () => {


  // [cap-test] });

  // [cap-test] describe("Page.setBypassServiceWorker", () => {
  // [cap-test] });

  // [cap-test] describe("Request.resourceType", () => {

  // [cap-test] });

  // [cap-test] describe("Response.remoteAddress", () => {

  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should fire for navigation requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(requests).toHaveLength(1);
    // [ts] });
    // [ts] test("should fire for iframes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect(requests).toHaveLength(2);
    // [ts] });
    // [ts] test("should fire for fetches", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/empty.html");
    // [ts] });
    // [ts] expect(requests).toHaveLength(2);
    // [ts] });
    // [ts] test("should work for main frame navigation request", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(requests).toHaveLength(1);
    // [ts] expect_eq(requests[0].frame(), page.mainFrame());
    // [ts] });
    // [ts] test("should work for subframe navigation request", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect(requests).toHaveLength(1);
    // [ts] expect_eq(requests[0].frame(), page.frames()[1]);
    // [ts] });
    // [ts] test("should work for fetch requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/digits/1.png");
    // [ts] });
    // [ts] requests = requests.filter(request => {
    // [ts] return !request.url().includes("favicon");
    // [ts] });
    // [ts] expect(requests).toHaveLength(1);
    // [ts] expect_eq(requests[0].frame(), page.mainFrame());
    // [ts] });
    // [ts] test("should work for main frame navigation request", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requestPromise = page.waitForRequest(server.EMPTY_PAGE);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.page(), page);
    // [ts] });
    // [ts] test("should work for subframe navigation request", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requestPromise = page.waitForRequest(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.frame(), frame);
    // [ts] expect_eq(request.page(), page);
    // [ts] });
    // [ts] test("should work for fetch requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const requestPromise = page.waitForRequest(
    // [ts] server.PREFIX + "/digits/1.png",
    // [ts] );
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/digits/1.png");
    // [ts] });
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.page(), page);
    // [ts] });
    // [ts] test("should return null for page navigation requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requestPromise = page.waitForRequest(server.EMPTY_PAGE);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const request = requestPromise;
    // [ts] expect_eq(request.worker() == null, true);
    // [ts] });
    // [ts] test("should define Browser in user agent header", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] const userAgent = response.request().headers()["user-agent"];
    // [ts] if (isChrome) {
    // [ts] expect_eq(std.mem.indexOf(u8, userAgent, "Chrome") != null, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, userAgent, "Firefox") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should report all headers sent to the server on response.request()", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(response.request().headers(), serverRequest.headers);
    // [ts] });
    // [ts] test("should report all headers sent to the server on requestfailed", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/fail.html", req => {
    // [ts] req.socket.destroy();
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] const [serverRequest, failedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/fail.html"),
    // [ts] waitEvent(page, "requestfailed", req => {
    // [ts] return !isFavicon(req);
    // [ts] }),
    // [ts] page.goto(server.PREFIX + "/fail.html").catch(() => {}),
    // [ts] ]);
    // [ts] expect_eq(failedRequest.headers(), serverRequest.headers);
    // [ts] });
    // [ts] test("should show Cookie header", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/title.html"));
    // [ts] const cookie = response.request().headers()["cookie"];
    // [ts] expect_eq(std.mem.indexOf(u8, cookie, "username=John Doe") != null, true);
    // [ts] });
    // [ts] test("should show Cookie header for redirect", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRedirect("/foo.html", "/title.html");
    // [ts] page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/foo.html"));
    // [ts] const cookie1 = response.request().redirectChain()[0].headers()[
    // [ts] "cookie"
    // [ts] ];
    // [ts] expect_eq(std.mem.indexOf(u8, cookie1, "username=John Doe") != null, true);
    // [ts] const cookie2 = response.request().headers()["cookie"];
    // [ts] expect_eq(std.mem.indexOf(u8, cookie2, "username=John Doe") != null, true);
    // [ts] });
    // [ts] test("should show Cookie header for fetch request", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] });
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "response", req => {
    // [ts] return !isFavicon(req);
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] fetch("/title.html");
    // [ts] }),
    // [ts] ]);
    // [ts] const cookie = response.request().headers()["cookie"];
    // [ts] expect_eq(std.mem.indexOf(u8, cookie, "username=John Doe") != null, true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/empty.html", (_req, res) => {
    // [ts] res.setHeader("foo", "bar");
    // [ts] res.end();
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.headers()["foo"], "bar");
    // [ts] });
    // [ts] test("should return the initiator", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const initiators = Map.init();
    // [ts] page.on("request", request => {
    // [ts] return initiators.set(
    // [ts] request.url().split("/").pop(),
    // [ts] request.initiator(),
    // [ts] );
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/initiator.html");
    // [ts] expect_eq(initiators.get("initiator.html").type, "other");
    // [ts] expect_eq(initiators.get("initiator.js").type, "parser");
    // [ts] expect(initiators.get("initiator.js").url).toBe(
    // [ts] server.PREFIX + "/initiator.html",
    // [ts] );
    // [ts] expect_eq(initiators.get("frame.html").type, "parser");
    // [ts] expect(initiators.get("frame.html").url).toBe(
    // [ts] server.PREFIX + "/initiator.html",
    // [ts] );
    // [ts] expect_eq(initiators.get("script.js").type, "parser");
    // [ts] expect(initiators.get("script.js").url).toBe(
    // [ts] server.PREFIX + "/frames/frame.html",
    // [ts] );
    // [ts] expect_eq(initiators.get("style.css").type, "parser");
    // [ts] expect(initiators.get("style.css").url).toBe(
    // [ts] server.PREFIX + "/frames/frame.html",
    // [ts] );
    // [ts] expect_eq(initiators.get("initiator.js").type, "parser");
    // [ts] expect_eq(initiators.get("injectedfile.js").type, "script");
    // [ts] expect(initiators.get("injectedfile.js").stack.callFrames[0].url).toBe(
    // [ts] server.PREFIX + "/initiator.js",
    // [ts] );
    // [ts] expect_eq(initiators.get("injectedstyle.css").type, "script");
    // [ts] expect(initiators.get("injectedstyle.css").stack.callFrames[0].url).toBe(
    // [ts] server.PREFIX + "/initiator.js",
    // [ts] );
    // [ts] expect(initiators.get("initiator.js").url).toBe(
    // [ts] server.PREFIX + "/initiator.html",
    // [ts] );
    // [ts] });
    // [ts] test("should return |false| for non-cached content", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.fromCache(), false);
    // [ts] });
    // [ts] test("should return |false| for non-service-worker content", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.fromServiceWorker(), false);
    // [ts] });
    // [ts] test("Response.fromServiceWorker", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const responses = Map.init();
    // [ts] page.on("response", r => {
    // [ts] return !isFavicon(r) && responses.set(r.url().split("/").pop(), r);
    // [ts] });
    // Load and re-load to make sure serviceworker is installed and running.
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html", {
    // [ts] waitUntil: "networkidle2",
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).activationPromise;
    // [ts] });
    // [ts] page.reload();
    // [ts] expect_eq(responses.size, 2);
    // [ts] expect_eq(responses.get("sw.html").status(), 200);
    // [ts] expect_eq(responses.get("sw.html").fromServiceWorker(), true);
    // [ts] expect_eq(responses.get("style.css").status(), 200);
    // [ts] expect_eq(responses.get("style.css").fromServiceWorker(), true);
    // [ts] });
    // [ts] test("should work for intercepted form submissions", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] let postData: string | null;
    // [ts] page.on("request", async request => {
    // [ts] if (!isFavicon(request) && request.method() == "POST") {
    // [ts] postData = request.fetchPostData();
    // [ts] }
    // [ts] request.continue();
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNavigation(),
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.method = "POST";
    // [ts] form.action = "/post";
    // [ts] const input = document.createElement("input");
    // [ts] input.name = "foo";
    // [ts] input.value = "bar";
    // [ts] form.append(input);
    // [ts] document.body.append(form);
    // [ts] form.submit();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(postData, "foo=bar");
    // [ts] });
    // [ts] test("should work for intercepted form submissions in cooperative mode", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] let postData: string | null;
    // [ts] page.on("request", async request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] request.continue({}, 0);
    // [ts] }
    // [ts] });
    // [ts] page.on("request", async request => {
    // [ts] if (request.method() == "POST") {
    // [ts] postData = request.fetchPostData();
    // [ts] }
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNavigation(),
    // [ts] page.evaluate(() => {
    // [ts] const form = document.createElement("form");
    // [ts] form.method = "POST";
    // [ts] form.action = "/post";
    // [ts] const input = document.createElement("input");
    // [ts] input.name = "foo";
    // [ts] input.value = "bar";
    // [ts] form.append(input);
    // [ts] document.body.append(form);
    // [ts] form.submit();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(postData, "foo=bar");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "request", r => {
    // [ts] return !isFavicon(r);
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("./post", {
    // [ts] method: "POST",
    // [ts] body: JSON.stringify({foo: "bar"}),
    // [ts] });
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.hasPostData(), true);
    // [ts] expect_eq(request.fetchPostData(), "{\"foo\":\"bar\"}");
    // [ts] });
    // [ts] test("should be |undefined| when there is no post data", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.request().hasPostData(), false);
    // [ts] expect_eq(response.request().fetchPostData(), null);
    // [ts] });
    // [ts] test("should work with blobs", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "request", r => {
    // [ts] return !isFavicon(r);
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("./post", {
    // [ts] method: "POST",
    // [ts] body: Blob.init([JSON.stringify({foo: "bar"})], {
    // [ts] type: "application/json",
    // [ts] }),
    // [ts] });
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.hasPostData(), true);
    // [ts] expect_eq(request.fetchPostData(), "{\"foo\":\"bar\"}");
    // [ts] });
    // [ts] test("should work with binary post data", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "request", r => {
    // [ts] return !isFavicon(r);
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("./post", {
    // [ts] method: "POST",
    // [ts] body: Uint8Array.init([104, 101, 108, 108, 111, 0, 255]),
    // [ts] });
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.hasPostData(), true);
    // [ts] expect_eq(request.fetchPostData(), "hello\x00\uFFFD");
    // [ts] });
    // [ts] test("should work with binary post data", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/post", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "request", r => {
    // [ts] return !isFavicon(r);
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("./post", {
    // [ts] method: "POST",
    // [ts] body: Uint8Array.init([0, 255, 128, 10, 200, 1]),
    // [ts] });
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(request.hasPostData(), true);
    // [ts] expect(request.content()).toEqual(
    // [ts] Uint8Array.init([0, 255, 128, 10, 200, 1]),
    // [ts] );
    // [ts] });
    // [ts] test("should be |undefined| when there is no post data", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.request().hasPostData(), false);
    // [ts] expect_eq(response.request().content(), null);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.PREFIX + "/simple.json"));
    // [ts] const responseText = (response.text()).trimEnd();
    // [ts] expect_eq(responseText, "{\"foo\": \"bar\"}");
    // [ts] });
    // [ts] test("should return uncompressed text", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.enableGzip("/simple.json");
    // [ts] const response = (page.goto(server.PREFIX + "/simple.json"));
    // [ts] expect_eq(response.headers()["content-encoding"], "gzip");
    // [ts] const responseText = (response.text()).trimEnd();
    // [ts] expect_eq(responseText, "{\"foo\": \"bar\"}");
    // [ts] });
    // [ts] test("should throw when requesting body of redirected response", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const response = (page.goto(server.PREFIX + "/foo.html"));
    // [ts] const redirectChain = response.request().redirectChain();
    // [ts] expect(redirectChain).toHaveLength(1);
    // [ts] const redirected = redirectChain[0].response();
    // [ts] expect_eq(redirected.status(), 302);
    // [ts] let error: Error;
    // [ts] redirected.text().catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toContain(
    // [ts] "Response body is unavailable for redirect responses",
    // [ts] );
    // [ts] });
    // [ts] test("should wait until response completes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // Setup server to trap request.
    // [ts] let serverResponse: ServerResponse;
    // [ts] server.setRoute("/get", (_req, res) => {
    // [ts] serverResponse = res;
    // In Firefox, |fetch| will be hanging until it receives |Content-Type| header
    // from server.
    // [ts] res.setHeader("Content-Type", "text/plain; charset=utf-8");
    // [ts] res.write("hello ");
    // [ts] });
    // Setup page to trap response.
    // [ts] let requestFinished = false;
    // [ts] page.on("requestfinished", r => {
    // [ts] return (requestFinished = requestFinished || r.url().includes("/get"));
    // [ts] });
    // send request and wait for server response
    // [ts] const [pageResponse] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForResponse(r => {
    // [ts] return !isFavicon(r.request());
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("./get", {method: "GET"});
    // [ts] }),
    // [ts] server.waitForRequest("/get"),
    // [ts] ]);
    // [ts] expect_eq(serverResponse, true);
    // [ts] expect_eq(pageResponse, true);
    // [ts] expect_eq(pageResponse.status(), 200);
    // [ts] expect_eq(requestFinished, false);
    // [ts] const responseText = pageResponse.text();
    // Write part of the response and wait for it to be flushed.
    // [ts] Promise.init(x => {
    // [ts] return serverResponse.write("wor", x);
    // [ts] });
    // Finish response.
    // [ts] Promise.init(x => {
    // [ts] serverResponse.end("ld!", () => {
    // [ts] return x();
    // [ts] });
    // [ts] });
    // [ts] expect_eq(responseText, "hello world!");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.PREFIX + "/simple.json"));
    // [ts] expect_eq(response.json(), {foo: "bar"});
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.PREFIX + "/pptr.png"));
    // [ts] const imageBuffer = fs.readFileSync(
    // [ts] path.join(import.meta.dirname, "../assets", "pptr.png"),
    // [ts] );
    // [ts] const responseBuffer = response.buffer();
    // [ts] expect_eq(Buffer.from(responseBuffer).equals(imageBuffer), true);
    // [ts] });
    // [ts] test("should work with compression", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.enableGzip("/pptr.png");
    // [ts] const response = (page.goto(server.PREFIX + "/pptr.png"));
    // [ts] const imageBuffer = fs.readFileSync(
    // [ts] path.join(import.meta.dirname, "../assets", "pptr.png"),
    // [ts] );
    // [ts] const responseBuffer = response.buffer();
    // [ts] expect_eq(Buffer.from(responseBuffer).equals(imageBuffer), true);
    // [ts] });
    // [ts] test("should throw if the response does not have a body", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] server.setRoute("/test.html", (_req, res) => {
    // [ts] res.setHeader("Access-Control-Allow-Origin", "*");
    // [ts] res.setHeader("Access-Control-Allow-Headers", "x-ping");
    // [ts] res.end("Hello World");
    // [ts] });
    // [ts] const url = server.CROSS_PROCESS_PREFIX + "/test.html";
    // [ts] const responsePromise = waitEvent(
    // [ts] page,
    // [ts] "response",
    // [ts] response => {
    // Get the preflight response.
    // [ts] return (
    // [ts] response.request().method() == "OPTIONS" && response.url() == url
    // [ts] );
    // [ts] },
    // [ts] );
    // Trigger a request with a preflight.
    // [ts] page.evaluate(async src => {
    // [ts] const response = fetch(src, {
    // [ts] method: "POST",
    // [ts] headers: {"x-ping": "pong"},
    // [ts] });
    // [ts] return response;
    // [ts] }, url);
    // [ts] const response = responsePromise;
    // [ts] expect(response.buffer()).rejects.toThrow(
    // [ts] "Could not load response body for this request. This might happen if the request is a preflight request.",
    // [ts] );
    // [ts] });
    // [ts] test("should stream the body while it is loading", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/stream", (_req, res) => {
    // [ts] res.writeHead(200, {"content-type": "text/plain"});
    // [ts] let i = 0;
    // [ts] const interval = setInterval(() => {
    // [ts] res.write(`chunk${i}\n`);
    // [ts] if (++i == 4) {
    // [ts] clearInterval(interval);
    // [ts] res.end();
    // [ts] }
    // [ts] }, 100);
    // [ts] });
    // [ts] const streamPromise = streamOnResponse(page, server.PREFIX + "/stream");
    // [ts] const [chunks] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] streamPromise.then(readAll),
    // [ts] page.evaluate(() => {
    // [ts] (fetch("/stream")).text();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(chunks.length > 1, true);
    // [ts] expect(Buffer.concat(chunks).toString()).toBe(
    // [ts] "chunk0\nchunk1\nchunk2\nchunk3\n",
    // [ts] );
    // [ts] });
    // [ts] test("should return the whole body if loading has finished", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.PREFIX + "/simple.json"));
    // [ts] const chunks = readAll(response.stream());
    // [ts] expect_eq(Buffer.concat(chunks).toString(), response.text());
    // [ts] });
    // [ts] test("should error the stream if loading fails", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRoute("/stream", (_req, res) => {
    // [ts] res.writeHead(200, {"content-type": "text/plain"});
    // [ts] res.write("chunk0\n");
    // [ts] setTimeout(() => {
    // [ts] res.socket?.destroy();
    // [ts] }, 100);
    // [ts] });
    // [ts] const streamPromise = streamOnResponse(page, server.PREFIX + "/stream");
    // [ts] const [error] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] streamPromise.then(readAll).catch(error => {
    // [ts] return error;
    // [ts] }),
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] (fetch("/stream")).text();
    // [ts] })
    // [ts] .catch(() => {}),
    // [ts] ]);
    // [ts] expect(error).toBeInstanceOf(Error);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Loading failed") != null, true);
    // [ts] });
    // [ts] test("should stop listening when the stream is cancelled", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let serverResponse: ServerResponse;
    // [ts] server.setRoute("/stream", (_req, res) => {
    // [ts] res.writeHead(200, {"content-type": "text/plain"});
    // [ts] res.write("chunk0\n");
    // [ts] serverResponse = res;
    // [ts] });
    // [ts] const responsePromise = waitEvent(page, "response", r => {
    // [ts] return r.url() == server.PREFIX + "/stream";
    // [ts] });
    // [ts] const fetchPromise = page.evaluate(() => {
    // [ts] (fetch("/stream")).text();
    // [ts] });
    // [ts] const response = responsePromise;
    // [ts] const client = response.request().client;
    // [ts] const listeners = client.listenerCount("Network.dataReceived");
    // [ts] const stream = response.stream();
    // [ts] expect_eq(client.listenerCount("Network.dataReceived"), listeners + 1);
    // [ts] stream.cancel();
    // [ts] expect_eq(client.listenerCount("Network.dataReceived"), listeners);
    // [ts] serverResponse.end("chunk1\n");
    // [ts] fetchPromise;
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/cool", (_req, res) => {
    // [ts] res.writeHead(200, "cool!");
    // [ts] res.end();
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/cool"));
    // [ts] expect_eq(response.statusText(), "cool!");
    // [ts] });
    // [ts] test("handles missing status text", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/nostatus", (_req, res) => {
    // [ts] res.writeHead(200, "");
    // [ts] res.end();
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/nostatus"));
    // [ts] expect_eq(response.statusText(), "");
    // [ts] });
    // [ts] test("returns timing information", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const responses: []HTTPResponse = [];
    // [ts] page.on("response", response => {
    // [ts] if (isFavicon(response)) {
    // [ts] return;
    // [ts] }
    // [ts] return responses.push(response);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(responses).toHaveLength(1);
    // [ts] expect_eq(responses[0].timing().receiveHeadersEnd > 0, true);
    // [ts] });
    // [ts] test("should return the encoded transfer size", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const raw = Array.from({length: 1000}, (_unused, i) => {
    // [ts] return `line-number-${i}-some-repeated-text`;
    // [ts] }).join("\n");
    // [ts] const compressed = zlib.gzipSync(raw);
    // [ts] server.setRoute("/compressible", (_req, res) => {
    // [ts] res.setHeader("Content-Type", "text/plain");
    // [ts] res.setHeader("Content-Encoding", "gzip");
    // [ts] res.end(compressed);
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/compressible"));
    // [ts] const responseBuffer = response.buffer();
    // [ts] expect_eq(responseBuffer.length, raw.length);
    // [ts] expect(response.encodedDataLength()).toBeGreaterThanOrEqual(
    // [ts] compressed.length,
    // [ts] );
    // [ts] expect_eq(response.encodedDataLength() < raw.length, true);
    // [ts] });
    // [ts] test("should return 0 for cached responses", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/cached-encoded-length", (_req, res) => {
    // [ts] res.setHeader("Cache-Control", "public, max-age=3600");
    // [ts] res.setHeader("Content-Type", "text/plain");
    // [ts] res.end("cached-content-".repeat(100));
    // [ts] });
    // [ts] const firstResponse = (page.goto(
    // [ts] server.PREFIX + "/cached-encoded-length",
    // [ts] ));
    // [ts] firstResponse.buffer();
    // [ts] expect_eq(firstResponse.fromCache(), false);
    // [ts] expect_eq(firstResponse.encodedDataLength() > 1500, true);
    // [ts] const cachedRequestFinished = waitEvent(
    // [ts] page,
    // [ts] "requestfinished",
    // [ts] request => {
    // [ts] return request.url() == server.PREFIX + "/cached-encoded-length";
    // [ts] },
    // [ts] );
    // [ts] const cachedResponse = (page.goto(
    // [ts] server.PREFIX + "/cached-encoded-length",
    // [ts] ));
    // [ts] cachedRequestFinished;
    // [ts] expect_eq(cachedResponse.fromCache(), true);
    // [ts] expect_eq(cachedResponse.encodedDataLength(), 0);
    // [ts] });
    // [ts] test("should not return a negative value for data: URLs", () => {
    // [ts] const {page} = getTestState();
    // Chromium reports -1 for data: URLs when the response is received.
    // Browsers differ in the final value, so only check that the unknown
    // size is not exposed.
    // [ts] let responseLengthOnEvent: number | null;
    // [ts] page.on("response", response => {
    // [ts] if (response.url() == "data:text/plain,hello") {
    // [ts] responseLengthOnEvent = response.encodedDataLength();
    // [ts] }
    // [ts] });
    // [ts] const response = (page.goto("data:text/plain,hello"));
    // [ts] expect_eq(responseLengthOnEvent >= 0, true);
    // [ts] expect_eq(response.encodedDataLength() >= 0, true);
    // [ts] });
    // [ts] test("Page.Events.Request", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] waitEvent(page, "response", response => {
    // [ts] return response.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] ]);
    // [ts] expect(requests).toHaveLength(1);
    // [ts] const request = requests[0];
    // [ts] expect_eq(request.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(request.method(), "GET");
    // [ts] expect_eq(request.response(), true);
    // [ts] expect_eq(request.frame() == page.mainFrame(), true);
    // [ts] expect_eq(request.frame().url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("Page.Events.Response", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const responses: []HTTPResponse = [];
    // [ts] page.on("response", response => {
    // [ts] return !isFavicon(response) && responses.push(response);
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] waitEvent(page, "response", response => {
    // [ts] return response.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] ]);
    // [ts] expect(responses).toHaveLength(1);
    // [ts] const response = responses[0];
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.request(), true);
    // [ts] });
    // [ts] test("Page.Events.RequestFailed", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith("css")) {
    // [ts] void request.abort();
    // [ts] } else {
    // [ts] void request.continue();
    // [ts] }
    // [ts] });
    // [ts] const failedRequests: []HTTPRequest = [];
    // [ts] page.on("requestfailed", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] failedRequests.push(request);
    // [ts] }
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.PREFIX + "/one-style.html"),
    // [ts] waitEvent(page, "requestfailed", request => {
    // [ts] return !isFavicon(request);
    // [ts] }),
    // [ts] ]);
    // [ts] expect(failedRequests).toHaveLength(1);
    // [ts] const failedRequest = failedRequests[0];
    // [ts] expect_eq(std.mem.indexOf(u8, failedRequest.url(), "one-style.css") != null, true);
    // [ts] expect_eq(failedRequest.response(), null);
    // [ts] expect_eq(failedRequest.frame(), true);
    // [ts] if (isChrome) {
    // [ts] expect_eq(failedRequest.failure().errorText, "net::ERR_FAILED");
    // [ts] } else {
    // [ts] expect_eq(failedRequest.failure().errorText, "NS_ERROR_ABORT");
    // [ts] }
    // [ts] });
    // [ts] test("Page.Events.RequestFailed for blocked requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setCSP("/empty.html", "default-src 'none'");
    // [ts] const failedRequests: []HTTPRequest = [];
    // [ts] page.on("requestfailed", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] failedRequests.push(request);
    // [ts] }
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page
    // [ts] .addScriptTag({url: server.PREFIX + "/one-script.js"})
    // [ts] .catch(() => {});
    // [ts] expect(failedRequests).toHaveLength(1);
    // [ts] expect_eq(failedRequests[0].failure()?.errorText, "csp");
    // [ts] });
    // [ts] test("Page.Events.RequestFinished", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("requestfinished", request => {
    // [ts] return !isFavicon(request) && requests.push(request);
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] waitEvent(page, "requestfinished", request => {
    // [ts] return request.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] ]);
    // [ts] expect(requests).toHaveLength(1);
    // [ts] const request = requests[0];
    // [ts] expect_eq(request.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(request.response(), true);
    // [ts] expect_eq(request.frame() == page.mainFrame(), true);
    // [ts] expect_eq(request.frame().url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should fire events in proper order", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const events: []string = [];
    // [ts] page.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push("request");
    // [ts] }
    // [ts] });
    // [ts] page.on("response", response => {
    // [ts] if (!isFavicon(response)) {
    // [ts] events.push("response");
    // [ts] }
    // [ts] });
    // [ts] page.on("requestfinished", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push("requestfinished");
    // [ts] }
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] waitEvent(page, "requestfinished", request => {
    // [ts] return request.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(events, ["request", "response", "requestfinished"]);
    // [ts] });
    // [ts] test("should support redirects", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const events: []string = [];
    // [ts] page.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push(`${request.method()} ${request.url()}`);
    // [ts] }
    // [ts] });
    // [ts] page.on("response", response => {
    // [ts] if (!isFavicon(response)) {
    // [ts] events.push(`${response.status()} ${response.url()}`);
    // [ts] }
    // [ts] });
    // [ts] page.on("requestfinished", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push(`DONE ${request.url()}`);
    // [ts] }
    // [ts] });
    // [ts] page.on("requestfailed", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push(`FAIL ${request.url()}`);
    // [ts] }
    // [ts] });
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const FOO_URL = server.PREFIX + "/foo.html";
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(FOO_URL),
    // [ts] waitEvent(page, "requestfinished", request => {
    // [ts] return request.url() == server.EMPTY_PAGE;
    // [ts] }),
    // [ts] ]);
    // [ts] expect(events).toEqual([
    // [ts] `GET ${FOO_URL}`,
    // [ts] `302 ${FOO_URL}`,
    // [ts] `DONE ${FOO_URL}`,
    // [ts] `GET ${server.EMPTY_PAGE}`,
    // [ts] `200 ${server.EMPTY_PAGE}`,
    // [ts] `DONE ${server.EMPTY_PAGE}`,
    // [ts] ]);
    // Check redirect chain
    // [ts] const redirectChain = response.request().redirectChain();
    // [ts] expect(redirectChain).toHaveLength(1);
    // [ts] expect_eq(std.mem.indexOf(u8, redirectChain[0].url(), "/foo.html") != null, true);
    // [ts] });
    // [ts] test("should treat a redirect status without a Location header as a final response", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const events: []string = [];
    // [ts] page.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push(`${request.method()} ${request.url()}`);
    // [ts] }
    // [ts] });
    // [ts] page.on("response", response => {
    // [ts] if (!isFavicon(response)) {
    // [ts] events.push(`${response.status()} ${response.url()}`);
    // [ts] }
    // [ts] });
    // [ts] page.on("requestfinished", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push(`DONE ${request.url()}`);
    // [ts] }
    // [ts] });
    // [ts] page.on("requestfailed", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] events.push(`FAIL ${request.url()}`);
    // [ts] }
    // [ts] });
    // [ts] server.setRoute("/no-location.html", (_req, res) => {
    // [ts] res.statusCode = 302;
    // [ts] res.end("final response");
    // [ts] });
    // [ts] const url = server.PREFIX + "/no-location.html";
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(url),
    // [ts] waitEvent(page, "requestfinished", request => {
    // [ts] return request.url() == url;
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(events, [`GET ${url}`, `302 ${url}`, `DONE ${url}`]);
    // [ts] expect_eq(response.status(), 302);
    // [ts] expect(response.request().redirectChain()).toHaveLength(0);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests = Map.init();
    // [ts] page.on("request", request => {
    // [ts] return requests.set(request.url().split("/").pop(), request);
    // [ts] });
    // [ts] server.setRedirect("/rrredirect", "/frames/one-frame.html");
    // [ts] page.goto(server.PREFIX + "/rrredirect");
    // [ts] expect_eq(requests.get("rrredirect").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("one-frame.html").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("frame.html").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("script.js").isNavigationRequest(), false);
    // [ts] expect_eq(requests.get("style.css").isNavigationRequest(), false);
    // [ts] });
    // [ts] test("should work with request interception", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const requests = Map.init();
    // [ts] page.on("request", request => {
    // [ts] requests.set(request.url().split("/").pop(), request);
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] server.setRedirect("/rrredirect", "/frames/one-frame.html");
    // [ts] page.goto(server.PREFIX + "/rrredirect");
    // [ts] expect_eq(requests.get("rrredirect").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("one-frame.html").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("frame.html").isNavigationRequest(), true);
    // [ts] expect_eq(requests.get("script.js").isNavigationRequest(), false);
    // [ts] expect_eq(requests.get("style.css").isNavigationRequest(), false);
    // [ts] });
    // [ts] test("should work when navigating to image", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "request"),
    // [ts] page.goto(server.PREFIX + "/pptr.png"),
    // [ts] ]);
    // [ts] expect_eq(request.isNavigationRequest(), true);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setExtraHTTPHeaders({
    // [ts] foo: "bar",
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("should work with a Headers instance", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setExtraHTTPHeaders(Headers.init({foo: "bar"}));
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("should throw for non-string header values", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error purposeful bad input
    // [ts] page.setExtraHTTPHeaders({foo: 1});
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect(error.message).toBe(
    // [ts] "Expected value of header \"foo\" to be String, but \"number\" is found.",
    // [ts] );
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] page.authenticate({
    // [ts] username: "user",
    // [ts] password: "pass",
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should work with interception", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", async req => {
    // [ts] req.continue();
    // [ts] });
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] page.authenticate({
    // [ts] username: "user",
    // [ts] password: "pass",
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should error if authentication is required but not enabled", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] let response;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 401);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // In headful, an error is thrown instead of 401.
    // [ts] if (
    // [ts] !(error).message?.includes(
    // [ts] "net::ERR_INVALID_AUTH_CREDENTIALS",
    // [ts] )
    // [ts] ) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // [ts] }
    // [ts] page.authenticate({
    // [ts] username: "user",
    // [ts] password: "pass",
    // [ts] });
    // [ts] response = (page.reload());
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should fail if wrong credentials", () => {
    // [ts] const {page, server} = getTestState();
    // Use unique user/password since Chrome caches credentials per origin.
    // [ts] server.setAuth("/empty.html", "user2", "pass2");
    // [ts] page.authenticate({
    // [ts] username: "foo",
    // [ts] password: "bar",
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 401);
    // [ts] });
    // [ts] test("should allow disable authentication", () => {
    // [ts] const {page, server} = getTestState();
    // Use unique user/password since Chrome caches credentials per origin.
    // [ts] server.setAuth("/empty.html", "user3", "pass3");
    // [ts] page.authenticate({
    // [ts] username: "user3",
    // [ts] password: "pass3",
    // [ts] });
    // [ts] let response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 200);
    // [ts] page.authenticate(null);
    // Navigate to a different origin to bust Chrome's credential caching.
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] response = (page.goto(
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] ));
    // [ts] expect_eq(response.status(), 401);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // In headful, an error is thrown instead of 401.
    // [ts] if (
    // [ts] !(error).message?.includes(
    // [ts] "net::ERR_INVALID_AUTH_CREDENTIALS",
    // [ts] )
    // [ts] ) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("Same-origin set-cookie navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const setCookieString = "foo=bar";
    // [ts] server.setRoute("/empty.html", (_req, res) => {
    // [ts] res.setHeader("set-cookie", setCookieString);
    // [ts] res.end("hello world");
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.headers()["set-cookie"], setCookieString);
    // [ts] });
    // [ts] test("Same-origin set-cookie subresource", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const setCookieString = "foo=bar";
    // [ts] server.setRoute("/foo", (_req, res) => {
    // [ts] res.setHeader("set-cookie", setCookieString);
    // [ts] res.end("hello world");
    // [ts] });
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "response", res => {
    // [ts] return res.url() == server.PREFIX + "/foo";
    // [ts] }),
    // [ts] page.evaluate(() => {
    // [ts] return Promise.init((resolve, reject) => {
    // [ts] const xhr = XMLHttpRequest.init();
    // [ts] xhr.onload = () => {
    // [ts] resolve();
    // [ts] };
    // [ts] xhr.onerror = () => {
    // [ts] reject(Error.init("XHR failed"));
    // [ts] };
    // [ts] xhr.open("GET", "/foo");
    // [ts] xhr.send();
    // [ts] });
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(response.headers()["set-cookie"], setCookieString);
    // [ts] });
    // [ts] test("Cross-origin set-cookie", () => {
    // [ts] const {page, httpsServer, close} = launch(
    // [ts] {
    // [ts] acceptInsecureCerts: true,
    // [ts] },
    // [ts] {
    // [ts] createContext: true,
    // [ts] },
    // [ts] );
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.goto(httpsServer.PREFIX + "/empty.html");
    // [ts] const setCookieString = "hello=world";
    // [ts] httpsServer.setRoute("/setcookie.html", (_req, res) => {
    // [ts] res.setHeader("Access-Control-Allow-Origin", "*");
    // [ts] res.setHeader("set-cookie", setCookieString);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(httpsServer.PREFIX + "/setcookie.html");
    // [ts] const url = httpsServer.CROSS_PROCESS_PREFIX + "/setcookie.html";
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "response", response => {
    // [ts] return response.url() == url;
    // [ts] }),
    // [ts] page.evaluate(src => {
    // [ts] const xhr = XMLHttpRequest.init();
    // [ts] xhr.open("GET", src);
    // [ts] xhr.send();
    // [ts] }, url),
    // [ts] ]);
    // [ts] expect_eq(response.headers()["set-cookie"], setCookieString);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("bypass for network", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const responses = Map.init();
    // [ts] page.on("response", r => {
    // [ts] return !isFavicon(r) && responses.set(r.url().split("/").pop(), r);
    // [ts] });
    // Load and re-load to make sure serviceworker is installed and running.
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html", {
    // [ts] waitUntil: "networkidle2",
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).activationPromise;
    // [ts] });
    // [ts] page.reload({
    // [ts] waitUntil: "networkidle2",
    // [ts] });
    // [ts] expect_eq(page.isServiceWorkerBypassed(), false);
    // [ts] expect_eq(responses.size, 2);
    // [ts] expect_eq(responses.get("sw.html").status(), 200);
    // [ts] expect_eq(responses.get("sw.html").fromServiceWorker(), true);
    // [ts] expect_eq(responses.get("style.css").status(), 200);
    // [ts] expect_eq(responses.get("style.css").fromServiceWorker(), true);
    // [ts] page.setBypassServiceWorker(true);
    // [ts] page.reload({
    // [ts] waitUntil: "networkidle2",
    // [ts] });
    // [ts] expect_eq(page.isServiceWorkerBypassed(), true);
    // [ts] expect_eq(responses.get("sw.html").status(), 200);
    // [ts] expect_eq(responses.get("sw.html").fromServiceWorker(), false);
    // [ts] expect_eq(responses.get("style.css").status(), 200);
    // [ts] expect_eq(responses.get("style.css").fromServiceWorker(), false);
    // [ts] });
    // [ts] test("should work for document type", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const request = response.request();
    // [ts] expect_eq(request.resourceType(), "document");
    // [ts] });
    // [ts] test("should work for stylesheets", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const cssRequests: []HTTPRequest = [];
    // [ts] const promise = Promise.init(resolve => {
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith("css")) {
    // [ts] cssRequests.push(request);
    // [ts] resolve();
    // [ts] }
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] promise;
    // [ts] expect(cssRequests).toHaveLength(1);
    // [ts] const request = cssRequests[0];
    // [ts] expect_eq(std.mem.indexOf(u8, request.url(), "one-style.css") != null, true);
    // [ts] expect_eq(request.resourceType(), "stylesheet");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] const remoteAddress = response.remoteAddress();
    // Either IPv6 or IPv4, depending on environment.
    // [ts] expect(
    // [ts] remoteAddress.ip.includes("::1") || remoteAddress.ip == "127.0.0.1",
    // [ts] ).toBe(true);
    // [ts] expect_eq(remoteAddress.port, server.PORT);
    // [ts] });
    // [ts] test("should support redirects", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRedirect("/foo.html", "/empty.html");
    // [ts] const FOO_URL = server.PREFIX + "/foo.html";
    // [ts] const response = (page.goto(FOO_URL));
    // Check redirect chain
    // [ts] const redirectChain = response.request().redirectChain();
    // [ts] expect(redirectChain).toHaveLength(1);
    // [ts] expect_eq(std.mem.indexOf(u8, redirectChain[0].url(), "/foo.html") != null, true);
    // [ts] expect(redirectChain[0].response().remoteAddress().port).toBe(
    // [ts] server.PORT,
    // [ts] );
    // [ts] });
}
