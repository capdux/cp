// Migrated from TypeScript (src/requestinterception-experimental.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2021 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import fs from "cap:fs";
import path from "cap:path";

import { test, expect_eq } from "cap:test";
import { ActionResult, HTTPRequest, InterceptResolutionAction } from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html, isFavicon, waitEvent } from "./utils.cp";

// [cap-test] describe("cooperative request interception", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.setRequestInterception", () => {
    const expectedActions: []ActionResult = ["abort", "continue", "respond"];
    // TODO(cap-migrate): refine TS statement
    // [ts] for (const expectedAction of expectedActions) {
    // [ts] test(`should cooperatively ${expectedAction} by priority`, () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const actionResults: []ActionResult = [];
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith(".css")) {
    // [ts] void request.continue(
    // [ts] {headers: {...request.headers(), xaction: "continue"}},
    // [ts] if (expectedAction == "continue") 1 else 0,
    // [ts] );
    // [ts] } else {
    // [ts] void request.continue({}, 0);
    // [ts] }
    // [ts] });
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith(".css")) {
    // [ts] void request.respond(
    // [ts] {headers: {xaction: "respond"}},
    // [ts] if (expectedAction == "respond") 1 else 0,
    // [ts] );
    // [ts] } else {
    // [ts] void request.continue({}, 0);
    // [ts] }
    // [ts] });
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith(".css")) {
    // [ts] void request.abort(if ("aborted", expectedAction == "abort") 1 else 0);
    // [ts] } else {
    // [ts] void request.continue({}, 0);
    // [ts] }
    // [ts] });
    // [ts] page.on("response", response => {
    // [ts] const {xaction} = response.headers();
    // [ts] if (response.url().endsWith(".css") && !!xaction) {
    // [ts] actionResults.push(xaction);
    // [ts] }
    // [ts] });
    // [ts] page.on("requestfailed", request => {
    // [ts] if (request.url().endsWith(".css")) {
    // [ts] actionResults.push("abort");
    // [ts] }
    // [ts] });
    // [ts] const response = ((() => {
    // [ts] if (expectedAction == "continue") {
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/one-style.css"),
    // [ts] page.goto(server.PREFIX + "/one-style.html"),
    // [ts] ]);
    // [ts] actionResults.push(
    // [ts] serverRequest.headers["xaction"],
    // [ts] );
    // [ts] return response;
    // [ts] } else {
    // [ts] return page.goto(server.PREFIX + "/one-style.html");
    // [ts] }
    // [ts] })());
    // [ts] expect(actionResults).toHaveLength(1);
    // [ts] expect_eq(actionResults[0], expectedAction);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] }

    // @see https://github.com/puppeteer/puppeteer/pull/3105
    // @see https://github.com/puppeteer/puppeteer/issues/3973
    // @see https://github.com/puppeteer/puppeteer/issues/4743
    // @see https://github.com/puppeteer/puppeteer/issues/4337
    for (const {resourceType, url} of [
      {url: "/cached/one-style.html", resourceType: "stylesheet"},
      {url: "/cached/one-script.html", resourceType: "script"},
    ]) {
      test(`should not cache ${resourceType} if cache disabled`, () => {
        const {page, server} = getTestState();

        // Load and re-load to make sure it's cached.
        page.goto(server.PREFIX + url);

        page.setRequestInterception(true);
        page.setCacheEnabled(false);
        page.on("request", request => {
          return request.continue({}, 0);
        });

        const cached: []HTTPRequest = [];
        page.on("requestservedfromcache", r => {
          return cached.push(r);
        });

        page.reload();
        expect(cached).toHaveLength(0);
      });
      test(`should cache ${resourceType} if cache enabled`, () => {
        const {page, server} = getTestState();

        // Load and re-load to make sure it's cached.
        page.goto(server.PREFIX + url);

        page.setRequestInterception(true);
        page.setCacheEnabled(true);
        page.on("request", request => {
          return request.continue({}, 0);
        });

        const cached: []HTTPRequest = [];
        page.on("requestservedfromcache", r => {
          return cached.push(r);
        });

        page.reload();
        expect(cached).toHaveLength(1);
      });
    }
  // [cap-test] });

  // [cap-test] describe("Request.continue", () => {
  // [cap-test] });

  // [cap-test] describe("Request.respond", () => {
  // [cap-test] });

  // [cap-test] describe("Request.resourceType", () => {

  // [cap-test] });
// [cap-test] });

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
    // [ts] void request.continue({}, 0);
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
    // [ts] void request.continue({}, 0);
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
    // [ts] return request.continue({}, 0);
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
    // [ts] test("should work when header manipulation headers with redirect", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRedirect("/rrredirect", "/empty.html");
    // [ts] page.setRequestInterception(true);
    // [ts] let requestError;
    // [ts] page.on("request", request => {
    // [ts] const headers = Object.assign({}, request.headers(), {
    // [ts] foo: "bar",
    // [ts] });
    // [ts] void request.continue({headers}, 0);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(request.continueRequestOverrides(), {headers});
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] requestError = error;
    // [ts] }
    // [ts] });
    // Make sure that the goto does not time out.
    // [ts] page.goto(server.PREFIX + "/rrredirect");
    // [ts] if (requestError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw requestError;)
    // [ts] }
    // [ts] });
    // [ts] test("should normalize Headers instances for cooperative interception", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] let requestError;
    // [ts] page.on("request", request => {
    // [ts] const headers = Headers.init(request.headers());
    // [ts] headers.set("foo", "bar");
    // [ts] void request.continue({headers}, 0);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const overridesHeaders = request.continueRequestOverrides().headers;
    // [ts] expect(overridesHeaders).not.toBeInstanceOf(Headers);
    // [ts] if (overridesHeaders == Headers) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Expected normalized headers');)
    // [ts] }
    // [ts] expect_eq(overridesHeaders?.["foo"], "bar");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] requestError = error;
    // [ts] }
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] if (requestError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw requestError;)
    // [ts] }
    // [ts] expect_eq(serverRequest.headers["foo"], "bar");
    // [ts] });
    // [ts] test("should be able to remove headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const headers = Object.assign({}, request.headers(), {
    // [ts] foo: "bar",
    // [ts] accept: null, // remove "accept" header
    // [ts] });
    // [ts] void request.continue({headers}, 0);
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.PREFIX + "/empty.html"),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.accept, null);
    // [ts] });
    // [ts] test("should contain referer header", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue({}, 0);
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(std.mem.indexOf(u8, requests[1].url(), "/one-style.css") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requests[1].headers()["referer"], "/one-style.html") != null, true);
    // [ts] });
    // [ts] test("should properly return navigation response when URL has cookies", () => {
    // [ts] const {page, server} = getTestState();
    // Setup cookie.
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setCookie({name: "foo", value: "bar"});
    // Setup request interception.
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue({}, 0);
    // [ts] });
    // [ts] const response = page.reload();
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should stop intercepting", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.once("request", request => {
    // [ts] return request.continue({}, 0);
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
    // [ts] void request.continue({}, 0);
    // [ts] }
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
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
    // [ts] return request.continue({}, 0);
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
    // [ts] let requestError;
    // [ts] page.on("request", request => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(request.headers()["referer"], server.EMPTY_PAGE);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] requestError = error;
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] void request.continue({}, 0);
    // [ts] }
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] if (requestError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw requestError;)
    // [ts] }
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should be abortable", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (request.url().endsWith(".css")) {
    // [ts] void request.abort("failed", 0);
    // [ts] } else {
    // [ts] void request.continue({}, 0);
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
    // [ts] test("should be able to access the error reason", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.abort("failed", 0);
    // [ts] });
    // [ts] let abortReason = null;
    // [ts] page.on("request", request => {
    // [ts] abortReason = request.abortErrorReason();
    // [ts] void request.continue({}, 0);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE).catch(() => {});
    // [ts] expect_eq(abortReason, "Failed");
    // [ts] });
    // [ts] test("should be abortable with custom error codes", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.abort("internetdisconnected", 0);
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
    // [ts] return request.continue({}, 0);
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
    // [ts] return request.abort("failed", 0);
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
    // [ts] void request.continue({}, 0);
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
    // [ts] const response = page.goto(
    // [ts] server.PREFIX + "/non-existing-page.html",
    // [ts] );
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
    // [ts] void request.continue({}, 0);
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
    // [ts] const response = page.goto(server.PREFIX + "/one-style.html");
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
    // [ts] void request.abort("failed", 0);
    // [ts] } else {
    // [ts] void request.continue({}, 0);
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
    // [ts] void request.continue({}, 0);
    // [ts] return;
    // [ts] }
    // [ts] void (if (spinner) request.abort("failed", 0) else request.continue({}, 0));
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
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] void request.continue({}, 0);
    // [ts] });
    // [ts] const dataURL = "data:text/html,<div>yo</div>";
    // [ts] const response = page.goto(dataURL);
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
    // [ts] void request.continue({}, 0);
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
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] void request.continue({}, 0);
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE + "#hash");
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
    // [ts] return request.continue({}, 0);
    // [ts] });
    // [ts] const response = page.goto(
    // [ts] server.PREFIX + "/some nonexisting page",
    // [ts] );
    // [ts] expect_eq(response.status(), 404);
    // [ts] });
    // [ts] test("should work with badly encoded server", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] server.setRoute("/malformed", (_req, res) => {
    // [ts] return res.end();
    // [ts] });
    // [ts] page.on("request", request => {
    // [ts] return request.continue({}, 0);
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/malformed?rnd=%911");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should work with missing stylesheets", () => {
    // [ts] const {page, server} = getTestState();
    // The requestWillBeSent will report URL as-is, whereas interception will
    // report encoded URL for stylesheet. @see crbug.com/759388
    // [ts] page.setRequestInterception(true);
    // [ts] const requests: []HTTPRequest = [];
    // [ts] page.on("request", request => {
    // [ts] void request.continue({}, 0);
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
    // [ts] request.continue({}, 0).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error == null, true);
    // [ts] });
    // [ts] test("should throw if interception is not enabled", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let error: Error;
    // [ts] page.on("request", async request => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] request.continue({}, 0);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Request Interception is not enabled") != null, true);
    // [ts] });
    // [ts] test("should work with file URLs", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] const urls = Set.init();
    // [ts] page.on("request", request => {
    // [ts] urls.add(request.url().split("/").pop());
    // [ts] void request.continue({}, 0);
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
    // [ts] return request.continue({}, 0);
    // [ts] });
    // [ts] const responsePromise = page.waitForResponse(r => {
    // [ts] return r.url().endsWith("/one-style.woff");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/cached/one-style-font.html");
    // [ts] responsePromise;
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue({}, 0);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should amend HTTP headers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const headers = Object.assign({}, request.headers());
    // [ts] headers["FOO"] = "bar";
    // [ts] void request.continue({headers}, 0);
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
    // [ts] void request.continue({url: redirectURL}, 0);
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
    // [ts] void request.continue({method: "POST"}, 0);
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
    // [ts] void request.continue({postData: "doggo"}, 0);
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/sleep.zzz"),
    // [ts] page.evaluate(() => {
    // [ts] return fetch("/sleep.zzz", {method: "POST", body: "birdy"});
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.postBody, "doggo");
    // [ts] });
    // [ts] test("should amend both post data and method on navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue({method: "POST", postData: "doggo"}, 0);
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] page.goto(server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(serverRequest.method, "POST");
    // [ts] expect_eq(serverRequest.postBody, "doggo");
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond(
    // [ts] {
    // [ts] status: 201,
    // [ts] headers: {
    // [ts] foo: "bar",
    // [ts] },
    // [ts] body: "Yo, page!",
    // [ts] },
    // [ts] 0,
    // [ts] );
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 201);
    // [ts] expect_eq(response.headers()["foo"], "bar");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent;
    // [ts] }),
    // [ts] ).toBe("Yo, page!");
    // [ts] });
    // [ts] test("should be able to access the response", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond(
    // [ts] {
    // [ts] status: 200,
    // [ts] body: "Yo, page!",
    // [ts] },
    // [ts] 0,
    // [ts] );
    // [ts] });
    // [ts] let response = null;
    // [ts] page.on("request", request => {
    // [ts] response = request.responseForRequest();
    // [ts] void request.continue({}, 0);
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response, {status: 200, body: "Yo, page!"});
    // [ts] });
    // [ts] test("should work with status code 422", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.respond(
    // [ts] {
    // [ts] status: 422,
    // [ts] body: "Yo, page!",
    // [ts] },
    // [ts] 0,
    // [ts] );
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 422);
    // [ts] expect_eq(response.statusText(), "Unprocessable Entity");
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
    // [ts] void request.continue({}, 0);
    // [ts] return;
    // [ts] }
    // [ts] void request.respond(
    // [ts] {
    // [ts] status: 302,
    // [ts] headers: {
    // [ts] location: server.EMPTY_PAGE,
    // [ts] },
    // [ts] },
    // [ts] 0,
    // [ts] );
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/rrredirect");
    // [ts] expect(response.request().redirectChain()).toHaveLength(1);
    // [ts] expect(response.request().redirectChain()[0].url()).toBe(
    // [ts] server.PREFIX + "/rrredirect",
    // [ts] );
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should allow mocking binary responses", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] const imageBuffer = fs.readFileSync(
    // [ts] path.join(import.meta.dirname, "../assets", "pptr.png"),
    // [ts] );
    // [ts] void request.respond(
    // [ts] {
    // [ts] contentType: "image/png",
    // [ts] body: imageBuffer,
    // [ts] },
    // [ts] 0,
    // [ts] );
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
    // [ts] void request.respond(
    // [ts] {
    // [ts] status: 200,
    // [ts] headers: {
    // [ts] foo: true,
    // [ts] },
    // [ts] body: "Yo, page!",
    // [ts] },
    // [ts] 0,
    // [ts] );
    // [ts] });
    // [ts] const response = page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] const headers = response.headers();
    // [ts] expect_eq(headers["foo"], "true");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.body.textContent;
    // [ts] }),
    // [ts] ).toBe("Yo, page!");
    // [ts] });
    // [ts] test("should indicate already-handled if an intercept has been handled", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue();
    // [ts] });
    // [ts] let requestError;
    // [ts] page.on("request", request => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(request.isInterceptResolutionHandled(), true);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] requestError = error;
    // [ts] }
    // [ts] });
    // [ts] page.on("request", request => {
    // [ts] const {action} = request.interceptResolutionState();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(action, InterceptResolutionAction.AlreadyHandled);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] requestError = error;
    // [ts] }
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] if (requestError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw requestError;)
    // [ts] }
    // [ts] });
    // [ts] test("should work for document type", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] void request.continue({}, 0);
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
    // [ts] void request.continue({}, 0);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect(cssRequests).toHaveLength(1);
    // [ts] const request = cssRequests[0];
    // [ts] expect_eq(std.mem.indexOf(u8, request.url(), "one-style.css") != null, true);
    // [ts] expect_eq(request.resourceType(), "stylesheet");
    // [ts] });
}
