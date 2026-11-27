// Migrated from TypeScript (src/requestinterception.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import fs from "cap:fs";
import path from "cap:path";

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html, isFavicon, waitEvent } from "./utils.cp";

// [cap-test] describe("request interception", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.setRequestInterception", () => {
    // @see https://github.com/puppeteer/puppeteer/pull/3105
    // @see https://github.com/puppeteer/puppeteer/issues/3973
    // @see https://github.com/puppeteer/puppeteer/issues/4743
    // @see https://github.com/puppeteer/puppeteer/issues/4337
    // @see https://github.com/puppeteer/puppeteer/issues/15575

    // TODO(cap-migrate): refine TS statement
    // [ts] for (const {resourceType, url, cachedResourceUrl} of [
    // [ts] {
    // [ts] url: "/cached/one-style.html",
    // [ts] cachedResourceUrl: "/cached/one-style.css",
    // [ts] resourceType: "stylesheet",
    // [ts] },
    // [ts] {
    // [ts] url: "/cached/one-script.html",
    // [ts] cachedResourceUrl: "/cached/one-script.js",
    // [ts] resourceType: "script",
    // [ts] },
    // [ts] ]) {
    // [ts] test(`should not cache ${resourceType} if cache disabled`, () => {
    // [ts] const {page, server} = getTestState();
    // Load and re-load to make sure it's cached.
    // [ts] page.goto(server.PREFIX + url);
    // [ts] page.setRequestInterception(true);
    // [ts] page.setCacheEnabled(false);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const cached: []HTTPRequest = [];
    // [ts] page.on("requestservedfromcache", r => {
    // [ts] return cached.push(r);
    // [ts] });
    // [ts] page.reload();
    // [ts] expect(cached).toHaveLength(0);
    // [ts] });
    // [ts] test(`should cache ${resourceType} if cache enabled`, () => {
    // [ts] const {page, server} = getTestState();
    // Load and re-load to make sure it's cached.
    // [ts] page.goto(server.PREFIX + url);
    // [ts] page.setRequestInterception(true);
    // [ts] page.setCacheEnabled(true);
    // [ts] let error: Error | null;
    // [ts] page.on("request", async request => {
    // [ts] request.continue().catch(_error => {
    // [ts] error = _error;
    // [ts] });
    // [ts] });
    // [ts] const cached: []HTTPRequest = [];
    // [ts] page.on("requestservedfromcache", r => {
    // [ts] if (isFavicon(r)) {
    // [ts] return;
    // [ts] }
    // [ts] return cached.push(r);
    // [ts] });
    // [ts] page.reload();
    // [ts] expect_eq(error == null, true);
    // [ts] expect(cached).toHaveLength(1);
    // [ts] expect_eq(cached[0].url(), server.PREFIX + cachedResourceUrl);
    // [ts] });
    // [ts] }
    // @see https://crbug.com/1196004

      expect_eq(settled, 20);
    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("Request.continue", () => {
  // [cap-test] });

  // [cap-test] describe("Request.respond", () => {



  // [cap-test] });

  // [cap-test] describe("Request.resourceType", () => {

  // [cap-test] });
// TODO(cap-migrate): refine TS statement
// [ts] });

fn pathToFileURL(path: string): string {
  let pathName = path.replace("\\\\", "/");
  // Windows drive letter must be prefixed with a slash.
  if (!pathName.startsWith("/")) {
    pathName = "/" + pathName;
  }
  return "file://" + pathName;
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should intercept", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] let requestError;
    // [ts] page.on("request", request => {
    // [ts] if (isFavicon(request)) {
    // [ts] void request.continue();
    // [ts] return;
    // [ts] }
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(request, true);
    // [ts] expect_eq(std.mem.indexOf(u8, request.url(), "empty.html") != null, true);
    // [ts] expect_eq(request.headers()["user-agent"], true);
    // [ts] expect_eq(request.method(), "GET");
    // [ts] expect_eq(request.isNavigationRequest(), true);
    // [ts] expect_eq(request.frame() == page.mainFrame(), true);
    // [ts] expect_eq(request.frame().url(), "about:blank");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] requestError = error;
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] void request.continue();
    // [ts] }
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] if (requestError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw requestError;)
    // [ts] }
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should work when POST is redirected with 302", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRedirect("/rredirect", "/empty.html");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] page.setContent(html`
    // [ts] <form
    // [ts] action="/rredirect"
    // [ts] method="post"
    // [ts] >
    // [ts] <input
    // [ts] type="hidden"
    // [ts] id="foo"
    // [ts] name="foo"
    // [ts] value="FOOBAR"
    // [ts] />
    // [ts] </form>
    // [ts] `);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('form', form => {
    // [ts-skip] return form.submit();
    // [ts-skip] }),
    // [ts] page.waitForNavigation(),
    // [ts] ]);
    // [ts] });
    // [ts] test("should work with keep alive redirects", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/rredirect", (_req, res) => {
    // [ts] res.writeHead(302, {location: "/target"});
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/target", (_req, res) => {
    // [ts] res.end("Hello World");
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] const redirectRequest = page.waitForRequest(
    // [ts] request => {
    // [ts] return request.url().endsWith("/rredirect");
    // [ts] },
    // [ts] {
    // [ts] timeout: 1000,
    // [ts] },
    // [ts] );
    // [ts] const targetResponse = page.waitForResponse(
    // [ts] response => {
    // [ts] return response.request().url().endsWith("/target");
    // [ts] },
    // [ts] {
    // [ts] timeout: 1000,
    // [ts] },
    // [ts] );
    // [ts] page.evaluate(async url => {
    // [ts] void fetch(url, {
    // [ts] method: "POST",
    // [ts] body: JSON.stringify({test: "test"}),
    // [ts] mode: "no-cors",
    // [ts] keepalive: true,
    // [ts] }).then(async res => {
    // [ts] @print(res.text());
    // [ts] });
    // [ts] }, server.PREFIX + "/rredirect");
    // [ts] redirectRequest;
    // [ts] targetResponse;
    // [ts] });
    // [ts] test("should work when header manipulation headers with redirect", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRedirect("/rrredirect", "/empty.html");
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const headers = Object.assign({}, request.headers(), {
    // [ts] foo: "bar",
    // [ts] });
    // [ts] void request.continue({headers});
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.PREFIX + "/rrredirect"),
    // [ts] ]);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("should be able to remove headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const headers = Object.assign({}, request.headers(), {
    // [ts] foo: "bar",
    // [ts] origin: null, // remove "origin" header
    // [ts] });
    // [ts] void request.continue({headers});
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.PREFIX + "/empty.html"),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.origin, null);
    // [ts] });
    // [ts] test("should contain referer header", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, requests[1].url(), "/one-style.css") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requests[1].headers()["referer"], "/one-style.html") != null, true);
    // [ts] });
    // [ts] test("should not allow mutating request headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] const headers = request.headers();
    // [ts] headers["test"] = "test";
    // [ts] void request.continue({headers: request.headers()});
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(std.mem.indexOf(u8, Object.keys(requests[0].headers()), "test") == null, true);
    // [ts] });
    // [ts] test("should work with requests without networkId", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] const cdp = page.createCDPSession();
    // [ts] cdp.send("DOM.enable");
    // [ts] const urls: []string = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] if (isFavicon(request)) {
    // [ts] return;
    // [ts] }
    // [ts] urls.push(request.url());
    // [ts] });
    // This causes network requests without networkId.
    // [ts] cdp.send("CSS.enable");
    // [ts] expect_eq(urls, [server.EMPTY_PAGE]);
    // [ts] });
    // [ts] test("should properly return navigation response when URL has cookies", () => {
    // [ts] const {page, server} = getTestState();
    // Setup cookie.
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({name: "foo", value: "bar"});
    // Setup request interception.
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const response = (page.reload());
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should stop intercepting", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.once("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(false);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should show custom HTTP headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setExtraHTTPHeaders({
    // [ts] foo: "bar",
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] let requestError;
    // [ts] page.on("request", request => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(request.headers()["foo"], "bar");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] requestError = error;
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] void request.continue();
    // [ts] }
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] if (requestError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw requestError;)
    // [ts] }
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should work with redirect inside sync XHR", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] server.setRedirect("/logo.png", "/pptr.png");
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const status = page.evaluate(() => {
    // [ts] const request = XMLHttpRequest.init();
    // [ts] request.open("GET", "/logo.png", false); // `false` makes the request synchronous
    // [ts] request.send(null);
    // [ts] return request.status;
    // [ts] });
    // [ts] expect_eq(status, 200);
    // [ts] });
    // [ts] test("should work with custom referer headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setExtraHTTPHeaders({referer: server.EMPTY_PAGE});
    // [ts] page.setRequestInterception(true);
    // [ts] let request: HTTPRequest;
    // [ts] page.on("request", req => {
    // [ts] request = req;
    // [ts] void request.continue();
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(request.headers()["referer"], server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should be abortable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith(".css")) {
    // [ts] void request.abort();
    // [ts] } else {
    // [ts] void request.continue();
    // [ts] }
    // [ts] });
    // [ts] let failedRequests = 0;
    // [ts] page.on("requestfailed", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] ++failedRequests;
    // [ts] }
    // [ts] });
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.PREFIX + "/one-style.html"),
    // [ts] waitEvent(page, "requestfailed", request => {
    // [ts] return !isFavicon(request);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.request().failure(), null);
    // [ts] expect_eq(failedRequests, 1);
    // [ts] });
    // [ts] test("should be abortable with custom error codes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.abort("internetdisconnected");
    // [ts] });
    // [ts] const [failedRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "requestfailed"),
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {}),
    // [ts] ]);
    // [ts] expect_eq(failedRequest, true);
    // [ts] expect(failedRequest.failure().errorText).toBe(
    // [ts] "net::ERR_INTERNET_DISCONNECTED",
    // [ts] );
    // [ts] });
    // [ts] test("should send referer", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setExtraHTTPHeaders({
    // [ts] referer: "http://google.com/",
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/grid.html"),
    // [ts] page.goto(server.PREFIX + "/grid.html"),
    // [ts] ]);
    // [ts] expect_eq(request.headers["referer"], "http://google.com/");
    // [ts] });
    // [ts] test("should fail navigation when aborting main resource", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.abort();
    // [ts] });
    // [ts] let error: Error;
    // [ts] page.goto(server.EMPTY_PAGE).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] if (isChrome) {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "net::ERR_FAILED") != null, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_ERROR_ABORT") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should work with redirects", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] server.setRedirect(
    // [ts] "/non-existing-page.html",
    // [ts] "/non-existing-page-2.html",
    // [ts] );
    // [ts] server.setRedirect(
    // [ts] "/non-existing-page-2.html",
    // [ts] "/non-existing-page-3.html",
    // [ts] );
    // [ts] server.setRedirect(
    // [ts] "/non-existing-page-3.html",
    // [ts] "/non-existing-page-4.html",
    // [ts] );
    // [ts] server.setRedirect("/non-existing-page-4.html", "/empty.html");
    // [ts] const response = (page.goto(
    // [ts] server.PREFIX + "/non-existing-page.html",
    // [ts] ));
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.url(), "empty.html") != null, true);
    // [ts] expect(requests).toHaveLength(5);
    // Check redirect chain
    // [ts] const redirectChain = response.request().redirectChain();
    // [ts] expect(redirectChain).toHaveLength(4);
    // [ts] expect_eq(std.mem.indexOf(u8, redirectChain[0].url(), "/non-existing-page.html") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, redirectChain[2].url(), "/non-existing-page-3.html") != null, true);
    // [ts] let i = 0;
    // [ts] while (i < redirectChain.length) { // [cap-migrate] for-step: ++i
    // [ts] const request = redirectChain[i];
    // [ts] expect_eq(request.isNavigationRequest(), true);
    // [ts] expect_eq(request.redirectChain().indexOf(request), i);
    // [ts] }
    // [ts] });
    // [ts] test("should work with redirects for subresources", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] server.setRedirect("/one-style.css", "/two-style.css");
    // [ts] server.setRedirect("/two-style.css", "/three-style.css");
    // [ts] server.setRedirect("/three-style.css", "/four-style.css");
    // [ts] server.setRoute("/four-style.css", (_req, res) => {
    // [ts] return res.end("body {box-sizing: border-box; }");
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/one-style.html"));
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.url(), "one-style.html") != null, true);
    // [ts] expect(requests).toHaveLength(5);
    // Check redirect chain
    // [ts] const redirectChain = requests[1].redirectChain();
    // [ts] expect(redirectChain).toHaveLength(3);
    // [ts] expect_eq(std.mem.indexOf(u8, redirectChain[0].url(), "/one-style.css") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, redirectChain[2].url(), "/three-style.css") != null, true);
    // [ts] });
    // [ts] test("should be able to abort redirects", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] server.setRedirect("/non-existing.json", "/non-existing-2.json");
    // [ts] server.setRedirect("/non-existing-2.json", "/simple.html");
    // [ts] page.on("request", request => {
    // [ts] if (request.url().includes("non-existing-2")) {
    // [ts] void request.abort();
    // [ts] } else {
    // [ts] void request.continue();
    // [ts] }
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const result = page.evaluate(() => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] return fetch("/non-existing.json");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] return (error).message;
    // [ts] }
    // [ts] });
    // [ts] if (isChrome) {
    // [ts] expect_eq(std.mem.indexOf(u8, result, "Failed to fetch") != null, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, result, "NetworkError") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should work with equal requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let responseCount = 1;
    // [ts] server.setRoute("/zzz", (_req, res) => {
    // [ts] return res.end(responseCount++ * 11 + "");
    // [ts] });
    // [ts] page.setRequestInterception(true);
    // [ts] let spinner = false;
    // Cancel 2nd request.
    // [ts] page.on("request", request => {
    // [ts] if (isFavicon(request)) {
    // [ts] void request.continue();
    // [ts] return;
    // [ts] }
    // [ts] void (if (spinner) request.abort() else request.continue());
    // [ts] spinner = !spinner;
    // [ts] });
    // [ts] const results = page.evaluate(() => {
    // [ts] return joinAll([ // migrated: Promise.all -> joinAll
    // [ts] fetch("/zzz")
    // [ts] .then(response => {
    // [ts] return response.text();
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return "FAILED";
    // [ts] }),
    // [ts] fetch("/zzz")
    // [ts] .then(response => {
    // [ts] return response.text();
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return "FAILED";
    // [ts] }),
    // [ts] fetch("/zzz")
    // [ts] .then(response => {
    // [ts] return response.text();
    // [ts] })
    // [ts] .catch(() => {
    // [ts] return "FAILED";
    // [ts] }),
    // [ts] ]);
    // [ts] });
    // [ts] expect_eq(results, ["11", "FAILED", "22"]);
    // [ts] });
    // [ts] test("should navigate to dataURL and fire dataURL requests", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] const dataURL = "data:text/html,<div>yo</div>";
    // [ts] const response = (page.goto(dataURL));
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect(requests).toHaveLength(1);
    // [ts] expect_eq(requests[0].url(), dataURL);
    // [ts] });
    // [ts] test("should be able to fetch dataURL and fire dataURL requests", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] const dataURL = "data:text/html,<div>yo</div>";
    // [ts] const text = page.evaluate(url => {
    // [ts] return fetch(url).then(r => {
    // [ts] return r.text();
    // [ts] });
    // [ts] }, dataURL);
    // [ts] expect_eq(text, "<div>yo</div>");
    // [ts] expect(requests).toHaveLength(1);
    // [ts] expect_eq(requests[0].url(), dataURL);
    // [ts] });
    // [ts] test("should navigate to URL with hash and fire requests without hash", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE + "#hash"));
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE + "#hash");
    // [ts] expect(requests).toHaveLength(1);
    // [ts] expect_eq(requests[0].url(), server.EMPTY_PAGE + "#hash");
    // [ts] });
    // [ts] test("should work with encoded server", () => {
    // [ts] const {page, server} = getTestState();
    // The requestWillBeSent will report encoded URL, whereas interception will
    // report URL as-is. @see crbug.com/759388
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const response = (page.goto(
    // [ts] server.PREFIX + "/some nonexisting page",
    // [ts] ));
    // [ts] expect_eq(response.status(), 404);
    // [ts] });
    // [ts] test("should work with badly encoded server", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] server.setRoute("/malformed", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const response = (page.goto(
    // [ts] server.PREFIX + "/malformed?rnd=%911",
    // [ts] ));
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should work with missing stylesheets", () => {
    // [ts] const {page, server} = getTestState();
    // The requestWillBeSent will report URL as-is, whereas interception will
    // report encoded URL for stylesheet. @see crbug.com/759388
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/style-404.html"));
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect(requests).toHaveLength(2);
    // [ts] expect_eq(requests[1].response().status(), 404);
    // [ts] });
    // [ts] test("should not throw \"Invalid Interception Id\" if the request was cancelled", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setContent(html`<iframe></iframe>`);
    // [ts] page.setRequestInterception(true);
    // [ts] let request: HTTPRequest;
    // [ts] page.on("request", async r => {
    // [ts] return (request = r);
    // [ts] });
    // [ts] void (
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval(
    // [ts] "iframe",
    // [ts] (frame, url) => {
    // [ts] return (frame.src = url);
    // [ts] },
    // [ts] server.EMPTY_PAGE,
    // [ts] ),
    // Wait for request interception.
    // [ts] waitEvent(page, "request")
    // [ts] );
    // Delete frame to cause request to be canceled.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', frame => {
    // [ts-skip] return frame.remove();
    // [ts-skip] });
    // [ts] let error: Error;
    // [ts] request.continue().catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error == null, true);
    // [ts] });
    // [ts] test("should emit requestfailed for requests aborted right after they start", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const pending = Set.init();
    // [ts] page.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] pending.add(request);
    // [ts] }
    // [ts] void request.continue().catch(() => {});
    // [ts] });
    // [ts] page.on("requestfinished", request => {
    // [ts] pending.delete(request);
    // [ts] });
    // [ts] page.on("requestfailed", request => {
    // [ts] pending.delete(request);
    // [ts] });
    // Chrome sometimes delivers Network.loadingFailed before
    // Fetch.requestPaused for such requests. The order is racy, so the
    // scenario is repeated to make it likely to show up at least once.
    // The fetch is cross-origin (about:blank to the test server); with a
    // same-origin fetch the abort usually wins before the request is paused.
    // [ts] let i = 0;
    // [ts] while (i < 50) { // [cap-migrate] for-step: i++
    // [ts] page.setContent(html`<div>${i}</div>`);
    // [ts] page.evaluate(
    // [ts] (url, abortSync) => {
    // [ts] const controller = AbortController.init();
    // [ts] const promise = fetch(url, {signal: controller.signal});
    // [ts] if (abortSync) {
    // [ts] controller.abort();
    // [ts] } else {
    // [ts] setTimeout(() => {
    // [ts] controller.abort();
    // [ts] }, 0);
    // [ts] }
    // [ts] promise.catch(() => {});
    // [ts] },
    // [ts] `${server.PREFIX}/abort-${i}`,
    // [ts] i % 2 == 0,
    // [ts] );
    // [ts] }
    // Give the remaining terminal events a chance to arrive.
    // [ts] page
    // [ts] .waitForNetworkIdle({idleTime: 100, timeout: 2000})
    // [ts] .catch(() => {});
    // [ts] expect(
    // [ts] [...pending].map(request => {
    // [ts] return request.url();
    // [ts] }),
    // [ts] ).toEqual([]);
    // [ts] });
    // [ts] test("should throw if interception is not enabled", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let error: Error;
    // [ts] page.on("request", async request => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] request.continue();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(error != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Request Interception is not enabled") != null, true);
    // [ts] });
    // [ts] test("should work with file URLs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const urls = Set.init();
    // [ts] page.on("request", request => {
    // [ts] urls.add(request.url().split("/").pop());
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.goto(
    // [ts] pathToFileURL(
    // [ts] path.join(import.meta.dirname, "../assets", "one-style.html"),
    // [ts] ),
    // [ts] );
    // [ts] expect_eq(urls.size, 2);
    // [ts] expect_eq(urls.has("one-style.html"), true);
    // [ts] expect_eq(urls.has("one-style.css"), true);
    // [ts] });
    // [ts] test("should load fonts if cache enabled", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.setCacheEnabled(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const responsePromise = page.waitForResponse(r => {
    // [ts] return r.url().endsWith("/one-style.woff");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/cached/one-style-font.html");
    // [ts] responsePromise;
    // [ts] });
    // [ts] test("should emit a single request for a font Chrome restarts", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.setCacheEnabled(true);
    // [ts] const isFont = (request) => {
    // [ts] return request.url().endsWith(".ttf");
    // [ts] };
    // [ts] const events: []string = [];
    // [ts] page.on("request", request => {
    // [ts] if (isFont(request)) {
    // [ts] events.push("request");
    // [ts] }
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.on("requestfinished", request => {
    // [ts] if (isFont(request)) {
    // [ts] events.push("requestfinished");
    // [ts] }
    // [ts] });
    // [ts] page.on("requestfailed", request => {
    // [ts] if (isFont(request)) {
    // [ts] events.push("requestfailed");
    // [ts] }
    // [ts] });
    // [ts] const fontLoaded = waitEvent(page, "requestfinished", isFont);
    // [ts] page.goto(server.PREFIX + "/font-face.html");
    // [ts] fontLoaded;
    // [ts] expect_eq(events, ["request", "requestfinished"]);
    // [ts] });
    // [ts] test("should work with worker", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] page.setRequestInterception(true);
    // [ts] });
    // [ts] test("should intercept parallel fetches from a dedicated worker", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (request.url().includes("abort")) {
    // [ts] void request.abort();
    // [ts] } else {
    // [ts] void request.continue();
    // [ts] }
    // [ts] });
    // [ts] const settled = page.evaluate(() => {
    // [ts] const source = `onmessage = async ({data: url}) => {
    // [ts] const results = Promise.allSettled(
    // [ts] Array.from({length: 20}, (_, i) => {
    // [ts] return fetch(url + (i % 2 ? '?abort=' : '?continue=') + i);
    // [ts] }),
    // [ts] );
    // [ts] postMessage(results.length);
    // [ts] };`;
    // [ts] const worker = Worker.init(
    // [ts] URL.createObjectURL(Blob.init([source], {type: "text/javascript"})),
    // [ts] );
    // [ts] return Promise.init(resolve => {
    // [ts] worker.onmessage = event => {
    // [ts] resolve(event.data);
    // [ts] };
    // [ts] worker.postMessage(location.href);
    // [ts] });
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should amend HTTP headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const headers = Object.assign({}, request.headers());
    // [ts] headers["FOO"] = "bar";
    // [ts] void request.continue({headers});
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/sleep.zzz");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("should amend HTTP headers given as a Headers instance", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const headers = Headers.init(request.headers());
    // [ts] headers.set("foo", "bar");
    // [ts] void request.continue({
    // [ts] headers,
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/sleep.zzz");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request.headers["foo"], "bar");
    // [ts] });
    // [ts] test("should redirect in a way non-observable to page", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const redirectURL = if (request.url().includes("/empty.html")) server.PREFIX + "/consolelog.html" else null;
    // [ts] void request.continue({url: redirectURL});
    // [ts] });
    // [ts] const [consoleMessage] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(consoleMessage.text(), "yellow");
    // [ts] });
    // [ts] test("should amend method", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue({method: "POST"});
    // [ts] });
    // [ts] const [request] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/sleep.zzz");
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(request.method, "POST");
    // [ts] });
    // [ts] test("should amend post data", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue({postData: "Ã°ÂÂÂ¶"});
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/sleep.zzz", {method: "POST", body: "Ã°ÂÂÂ¦"});
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.postBody, "ÃÂ°ÃÂÃÂÃÂ¶");
    // [ts] });
    // [ts] test("should amend post data to an empty string", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue({postData: ""});
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/sleep.zzz", {method: "POST", body: "Ã°ÂÂÂ¦"});
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.postBody, "");
    // [ts] });
    // [ts] test("should amend both post data and method on navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue({method: "POST", postData: "Ã°ÂÂÂ¶"});
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.method, "POST");
    // [ts] expect_eq(serverRequest.postBody, "ÃÂ°ÃÂÃÂÃÂ¶");
    // [ts] });
    // [ts] test("should fail if the header value is invalid", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let error: Error;
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", async request => {
    // [ts] request
    // [ts] .continue({
    // [ts] headers: {
    // [ts] "X-Invalid-Header": "a\nb",
    // [ts] },
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] error = error_;
    // [ts] });
    // [ts] request.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect(error.message).toMatch(
    // [ts] "Invalid header|Expected \"header\"|invalid argument",
    // [ts] );
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 201,
    // [ts] headers: {
    // [ts] foo: "bar",
    // [ts] },
    // [ts] body: "Yo, page!",
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.headers()["foo"], "bar");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent;
    // [ts] }),
    // [ts] ).toBe("Yo, page!");
    // [ts] });
    // [ts] test("should work with headers given as a Headers instance", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 201,
    // [ts] headers: Headers.init({
    // [ts] foo: "bar",
    // [ts] }),
    // [ts] body: "Yo, page!",
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.headers()["foo"], "bar");
    // [ts] });
    // [ts] test("should work with status code 422", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 422,
    // [ts] body: "Yo, page!",
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 422);
    // [ts] expect_eq(response.statusText(), "Unprocessable Entity");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent;
    // [ts] }),
    // [ts] ).toBe("Yo, page!");
    // [ts] });
    // [ts] test("should work with a status code that has no standard reason phrase", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 419,
    // [ts] body: "Yo, page!",
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 419);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent;
    // [ts] }),
    // [ts] ).toBe("Yo, page!");
    // [ts] });
    // [ts] test("should redirect", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (!request.url().includes("rrredirect")) {
    // [ts] void request.continue();
    // [ts] return;
    // [ts] }
    // [ts] void request.respond({
    // [ts] status: 302,
    // [ts] headers: {
    // [ts] location: server.EMPTY_PAGE,
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/rrredirect"));
    // [ts] expect(response.request().redirectChain()).toHaveLength(1);
    // [ts] expect(response.request().redirectChain()[0].url()).toBe(
    // [ts] server.PREFIX + "/rrredirect",
    // [ts] );
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should allow mocking multiple headers with same key", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] foo: "bar",
    // [ts] arr: ["1", "2"],
    // [ts] "set-cookie": ["first=1", "second=2"],
    // [ts] },
    // [ts] body: "Hello Ã°ÂÂÂ",
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] const cookies = page.cookies();
    // [ts] const firstCookie = cookies.find(cookie => {
    // [ts] return cookie.name == "first";
    // [ts] });
    // [ts] const secondCookie = cookies.find(cookie => {
    // [ts] return cookie.name == "second";
    // [ts] });
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.headers()["foo"], "bar");
    // [ts] expect_eq(response.headers()["arr"], "1, 2");
    // request.respond() will not trigger Network.responseReceivedExtraInfo
    // fail to get 'set-cookie' header from response
    // [ts] expect_eq(firstCookie?.value, "1");
    // [ts] expect_eq(secondCookie?.value, "2");
    // [ts] });
    // [ts] test("should allow mocking binary responses", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const imageBuffer = fs.readFileSync(
    // [ts] path.join(import.meta.dirname, "../assets", "pptr.png"),
    // [ts] );
    // [ts] void request.respond({
    // [ts] contentType: "image/png",
    // [ts] body: imageBuffer,
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(PREFIX => {
    // [ts] const img = document.createElement("img");
    // [ts] img.src = PREFIX + "/does-not-exist.png";
    // [ts] document.body.appendChild(img);
    // [ts] return Promise.init(fulfill => {
    // [ts] return (img.onload = fulfill);
    // [ts] });
    // [ts] }, server.PREFIX);
    // [ts] using img = (page.$("img"));
    // [ts] expect(img.screenshot()).toBeGolden("mock-binary-response.png");
    // [ts] });
    // [ts] test("should stringify intercepted request response headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] foo: true,
    // [ts] },
    // [ts] body: "Yo, page!",
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 200);
    // [ts] const headers = response.headers();
    // [ts] expect_eq(headers["foo"], "true");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent;
    // [ts] }),
    // [ts] ).toBe("Yo, page!");
    // [ts] });
    // [ts] test("should fail if the header value is invalid", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let error: Error;
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", async request => {
    // [ts] request
    // [ts] .respond({
    // [ts] headers: {
    // [ts] "X-Invalid-Header": "a\nb",
    // [ts] },
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] error = error_;
    // [ts] });
    // [ts] request.respond({
    // [ts] status: 200,
    // [ts] body: "Hello Ã°ÂÂÂ",
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect(error.message).toMatch(
    // [ts] "Invalid header|Expected \"header\"|invalid argument",
    // [ts] );
    // [ts] });
    // [ts] test("should report correct content-length header with string", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 200,
    // [ts] body: "Correct length Ã°ÂÂÂ?",
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] const headers = response.headers();
    // [ts] expect_eq(headers["content-length"], "20");
    // [ts] });
    // [ts] test("should report correct content-length header with buffer", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 200,
    // [ts] body: Buffer.from("Correct length ÃÂ°ÃÂÃÂÃÂ?"),
    // [ts] });
    // [ts] });
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] const headers = response.headers();
    // [ts] expect_eq(headers["content-length"], "20");
    // [ts] });
    // [ts] test("should report correct encoding from page when content-type is set", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond({
    // [ts] status: 200,
    // [ts] body: Buffer.from("Correct length ÃÂ°ÃÂÃÂÃÂ?"),
    // [ts] headers: {
    // [ts] "Content-Type": "text/plain; charset=utf-8",
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const content = page.evaluate(() => {
    // [ts] return document.documentElement.innerText;
    // [ts] });
    // [ts] expect_eq(content, "Correct length Ã°ÂÂÂ?");
    // [ts] });
    // [ts] test("should work for document type", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] const request = response.request();
    // [ts] expect_eq(request.resourceType(), "document");
    // [ts] });
    // [ts] test("should work for stylesheets", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const cssRequests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith("css")) {
    // [ts] cssRequests.push(request);
    // [ts] }
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect(cssRequests).toHaveLength(1);
    // [ts] const request = cssRequests[0];
    // [ts] expect_eq(std.mem.indexOf(u8, request.url(), "one-style.css") != null, true);
    // [ts] expect_eq(request.resourceType(), "stylesheet");
    // [ts] });
}
