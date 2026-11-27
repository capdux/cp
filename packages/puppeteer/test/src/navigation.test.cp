// Migrated from TypeScript (src/navigation.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { getEventListeners } from "node:events";
// [cap-migrate] import {ServerResponse} from "cap:http";

import { test, expect_eq } from "cap:test";
import { Frame, TimeoutError } from "puppeteer";
import { FrameEvent } from "puppeteer-core/internal/api/Frame.js";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {HTTPResponse} from "puppeteer-core/internal/api/HTTPResponse.js";
import { Deferred } from "puppeteer-core/internal/util/Deferred.js";
import sinon from "sinon";

import { getTestState, setupSeparateTestBrowserHooks, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, html, isFavicon, waitEvent } from "./utils.cp";

// [cap-test] describe("navigation", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.goto", () => {

    const EXPECTED_SSL_CERT_MESSAGE_REGEX =
      "net::ERR_CERT_INVALID|net::ERR_CERT_AUTHORITY_INVALID|MOZILLA_PKIX_ERROR_SELF_SIGNED_CERT|SSL_ERROR_UNKNOWN";



  // [cap-test] });

  // [cap-test] describe("Page.waitForNavigation", () => {
  // [cap-test] });

  // [cap-test] describe("Page.goBack", () => {
  // [cap-test] });

  // [cap-test] describe("Frame.goto", () => {
  // [cap-test] });

  // [cap-test] describe("Frame.waitForNavigation", () => {
  // [cap-test] });

  // [cap-test] describe("Page.reload", () => {
  // [cap-test] });
// [cap-test] });

// [cap-test] describe("with network events disabled", () => {
  const state = setupSeparateTestBrowserHooks({
    networkEnabled: false,
  });

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should work with anchor navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] page.goto(server.EMPTY_PAGE + "#foo");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE + "#foo");
    // [ts] page.goto(server.EMPTY_PAGE + "#bar");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE + "#bar");
    // [ts] });
    // [ts] test("should work with redirects", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRedirect("/redirect/1.html", "/redirect/2.html");
    // [ts] server.setRedirect("/redirect/2.html", "/empty.html");
    // [ts] page.goto(server.PREFIX + "/redirect/1.html");
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should navigate to about:blank", () => {
    // [ts] const {page} = getTestState();
    // [ts] const response = page.goto("about:blank");
    // [ts] expect_eq(response, null);
    // [ts] });
    // [ts] test("should return response when page changes its URL after load", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = page.goto(server.PREFIX + "/historyapi.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should return response when page replaces its state during load", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = page.goto(
    // [ts] server.PREFIX + "/historyapi-replaceState.html",
    // [ts] {
    // [ts] waitUntil: "networkidle2",
    // [ts] },
    // [ts] );
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(page.url(), server.PREFIX + "/historyapi-replaceState.html");
    // [ts] });
    // [ts] test("should work with subframes return 204", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/frames/frame.html", (_req, res) => {
    // [ts] res.statusCode = 204;
    // [ts] res.end();
    // [ts] });
    // [ts] let error: Error;
    // [ts] page
    // [ts] .goto(server.PREFIX + "/frames/one-frame.html")
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error == null, true);
    // [ts] });
    // [ts] test("should fail when server returns 204", () => {
    // [ts] const {page, server, isChrome} = getTestState();
    // [ts] server.setRoute("/empty.html", (_req, res) => {
    // [ts] res.statusCode = 204;
    // [ts] res.end();
    // [ts] });
    // [ts] let error: Error;
    // [ts] page.goto(server.EMPTY_PAGE).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] if (isChrome) {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "net::ERR_ABORTED") != null, true);
    // [ts] } else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "NS_BINDING_ABORTED") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should navigate to empty page with domcontentloaded", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = page.goto(server.EMPTY_PAGE, {
    // [ts] waitUntil: "domcontentloaded",
    // [ts] });
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should navigate successfully after encountering network error", () => {
    // [ts] const {page, server} = getTestState();
    // Destroy the connection to simulate a network error. This will open
    // about:neterror on Firefox.
    // [ts] server.setRoute("/network-error", req => {
    // [ts] req.socket.destroy();
    // [ts] });
    // [ts] let error: Error;
    // [ts] page.goto(server.PREFIX + "/network-error").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] const response = page.goto(server.PREFIX + "/grid.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should work when page calls history API in beforeunload", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] window.addEventListener(
    // [ts] "beforeunload",
    // [ts] () => {
    // [ts] return history.replaceState(null, "initial", window.location.href);
    // [ts] },
    // [ts] false,
    // [ts] );
    // [ts] });
    // [ts] const response = page.goto(server.PREFIX + "/grid.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should work when reload causes history API in beforeunload", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).beforeReload = true;
    // [ts] window.addEventListener("beforeunload", () => {
    // [ts] history.replaceState({scroll: 0}, "", window.location.href);
    // [ts] });
    // [ts] });
    // [ts] const response = page.reload();
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).beforeReload;
    // [ts] }),
    // [ts] ).toBe(null);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return history.state;
    // [ts] }),
    // [ts] ).toEqual({scroll: 0});
    // [ts] });
    // [ts] test("should navigate to empty page with networkidle0", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = page.goto(server.EMPTY_PAGE, {
    // [ts] waitUntil: "networkidle0",
    // [ts] });
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should navigate to page with iframe and networkidle0", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = page.goto(
    // [ts] server.PREFIX + "/frames/one-frame.html",
    // [ts] {
    // [ts] waitUntil: "networkidle0",
    // [ts] },
    // [ts] );
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should navigate to empty page with networkidle2", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = page.goto(server.EMPTY_PAGE, {
    // [ts] waitUntil: "networkidle2",
    // [ts] });
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should fail when navigating to bad url", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.goto("asdfasdf").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Cannot navigate to invalid URL", // Firefox WebDriver BiDi.
    // [ts] "invalid argument", // Others.
    // [ts] ]);
    // [ts] });
    // [ts] test("should fail when navigating to bad SSL", () => {
    // [ts] const {page, httpsServer} = getTestState();
    // Make sure that network events do not emit 'undefined'.
    // @see https://crbug.com/750469
    // [ts] const requests: []string = [];
    // [ts] page.on("request", () => {
    // [ts] return requests.push("request");
    // [ts] });
    // [ts] page.on("requestfinished", () => {
    // [ts] return requests.push("requestfinished");
    // [ts] });
    // [ts] page.on("requestfailed", () => {
    // [ts] return requests.push("requestfailed");
    // [ts] });
    // [ts] let error: Error;
    // [ts] page.goto(httpsServer.EMPTY_PAGE).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toMatch(EXPECTED_SSL_CERT_MESSAGE_REGEX);
    // [ts] expect(requests).toHaveLength(2);
    // [ts] expect_eq(requests[0], "request");
    // [ts] expect_eq(requests[1], "requestfailed");
    // [ts] });
    // [ts] test("should fail when navigating to bad SSL after redirects", () => {
    // [ts] const {page, server, httpsServer} = getTestState();
    // [ts] server.setRedirect("/redirect/1.html", "/redirect/2.html");
    // [ts] server.setRedirect("/redirect/2.html", "/empty.html");
    // [ts] let error: Error;
    // [ts] page.goto(httpsServer.PREFIX + "/redirect/1.html").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toMatch(EXPECTED_SSL_CERT_MESSAGE_REGEX);
    // [ts] });
    // [ts] test("should fail when main resources failed to load", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .goto("http://localhost:44123/non-existing-url")
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toMatch(
    // [ts] "net::ERR_CONNECTION_REFUSED|NS_ERROR_CONNECTION_REFUSED",
    // [ts] );
    // [ts] });
    // [ts] test("should fail when exceeding maximum navigation timeout", () => {
    // [ts] const {page, server} = getTestState();
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] let error: Error;
    // [ts] page
    // [ts] .goto(server.PREFIX + "/empty.html", {timeout: 1})
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Navigation timeout of 1 ms exceeded") != null, true);
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should remove its listeners when the navigation times out", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] const frame = page.mainFrame();
    // [ts] const listeners = frame.listenerCount(FrameEvent.FrameDetached);
    // [ts] expect(
    // [ts] page.goto(server.PREFIX + "/empty.html", {timeout: 1}),
    // [ts] ).rejects.toBeInstanceOf(TimeoutError);
    // [ts] expect_eq(frame.listenerCount(FrameEvent.FrameDetached), listeners);
    // [ts] });
    // [ts] test("should be cancellable", () => {
    // [ts] const {page, server} = getTestState();
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] const abortController = AbortController.init();
    // [ts] const navigation = page.goto(server.PREFIX + "/empty.html", {
    // [ts] signal: abortController.signal,
    // [ts] });
    // [ts] server.waitForRequest("/empty.html");
    // [ts] abortController.abort();
    // [ts] expect(navigation).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should not navigate when the signal is already aborted", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let requested = false;
    // [ts] server.setRoute("/title.html", (_req, res) => {
    // [ts] requested = true;
    // [ts] res.end();
    // [ts] });
    // [ts] expect(
    // [ts] page.goto(server.PREFIX + "/title.html", {
    // [ts] signal: AbortSignal.abort(),
    // [ts] }),
    // [ts] ).rejects.toThrow("aborted");
    // [ts] Promise.init(resolve => {
    // [ts] return setTimeout(resolve, 200);
    // [ts] });
    // [ts] expect_eq(requested, false);
    // [ts] expect_eq(page.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should settle when the page navigates again while loading", () => {
    // [ts] const {page, server} = getTestState();
    // The document loads, then navigates again from its own load handler.
    // [ts] server.setRoute("/navigate-on-load.html", (_unused, res) => {
    // [ts] res.end(
    // [ts] `<html><body onload="location.href='/hang.html'">ok</body></html>`,
    // [ts] );
    // [ts] });
    // Hang for the request the page navigates to.
    // [ts] server.setRoute("/hang.html", () => {});
    // [ts] const loaded = Promise.init(resolve => {
    // [ts] page.once("load", () => {
    // [ts] return resolve();
    // [ts] });
    // [ts] });
    // [ts] const hangRequested = server.waitForRequest("/hang.html");
    // [ts] const clock = sinon.useFakeTimers({
    // [ts] shouldClearNativeTimers: true,
    // [ts] shouldAdvanceTime: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // Resolving and timing out are both fine; never settling is not.
    // [ts] let settled = false;
    // [ts] const goto = page
    // [ts] .goto(server.PREFIX + "/navigate-on-load.html", {timeout: 10000})
    // [ts] .catch(() => {})
    // [ts] .then(() => {
    // [ts] settled = true;
    // [ts] });
    // Only once the first document is done and the second navigation is in
    // flight is the deadline the thing goto is waiting on.
    // [ts] joinAll([loaded, hangRequested]); // migrated: Promise.all -> joinAll
    // [ts] clock.tickAsync(10000);
    // [ts] race([goto, clock.tickAsync(5000)]); // migrated: Promise.race -> race
    // [ts] expect_eq(settled, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clock.restore();
    // [ts] }
    // [ts] });
    // [ts] test("should fail when exceeding default maximum navigation timeout", () => {
    // [ts] const {page, server} = getTestState();
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] let error: Error;
    // [ts] page.setDefaultNavigationTimeout(1);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Navigation timeout of 1 ms exceeded") != null, true);
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should fail when exceeding default maximum timeout", () => {
    // [ts] const {page, server} = getTestState();
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] let error: Error;
    // [ts] page.setDefaultTimeout(1);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Navigation timeout of 1 ms exceeded") != null, true);
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should prioritize default navigation timeout over default timeout", () => {
    // [ts] const {page, server} = getTestState();
    // Hang for request to the empty.html
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] let error: Error;
    // [ts] page.setDefaultTimeout(0);
    // [ts] page.setDefaultNavigationTimeout(1);
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Navigation timeout of 1 ms exceeded") != null, true);
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should prioritize default navigation timeout over default timeout with networkidle0", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setDefaultTimeout(1);
    // [ts] page.setDefaultNavigationTimeout(0);
    // [ts] const response = (page.goto(server.EMPTY_PAGE, {
    // [ts] waitUntil: "networkidle0",
    // [ts] }));
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should disable timeout when its set to 0", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let error: Error;
    // [ts] let loaded = false;
    // [ts] page.once("load", () => {
    // [ts] loaded = true;
    // [ts] });
    // [ts] page
    // [ts] .goto(server.PREFIX + "/grid.html", {timeout: 0, waitUntil: ["load"]})
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error == null, true);
    // [ts] expect_eq(loaded, true);
    // [ts] });
    // [ts] test("should work when navigating to valid url", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should work when navigating to a URL with a client redirect", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(
    // [ts] server.PREFIX + "/client-redirect.html",
    // [ts] ));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.url(), server.PREFIX + "/client-redirect.html");
    // [ts] });
    // [ts] test("should work when a page redirects on DOMContentLoaded", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(
    // [ts] server.PREFIX + "/client-redirect-DOMContentLoaded.html",
    // [ts] ));
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should work when navigating to data url", () => {
    // [ts] const {page} = getTestState();
    // [ts] const response = (page.goto("data:text/html,hello"));
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should work when navigating to 404", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.PREFIX + "/not-found"));
    // [ts] expect_eq(response.ok(), false);
    // [ts] expect_eq(response.status(), 404);
    // [ts] });
    // [ts] test("should not throw an error for a 404 response with an empty body", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/404-error", (_unused, res) => {
    // [ts] res.statusCode = 404;
    // [ts] setTimeout(() => {
    // [ts] res.end();
    // [ts] }, 50);
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/404-error"));
    // [ts] expect_eq(response.ok(), false);
    // [ts] expect_eq(response.status(), 404);
    // [ts] });
    // [ts] test("should not throw an error for a 500 response with an empty body", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRoute("/500-error", (_unused, res) => {
    // [ts] res.statusCode = 500;
    // [ts] setTimeout(() => {
    // [ts] res.end();
    // [ts] }, 50);
    // [ts] });
    // [ts] const response = (page.goto(server.PREFIX + "/500-error"));
    // [ts] expect_eq(response.ok(), false);
    // [ts] expect_eq(response.status(), 500);
    // [ts] });
    // [ts] test("should return last response in redirect chain", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setRedirect("/redirect/1.html", "/redirect/2.html");
    // [ts] server.setRedirect("/redirect/2.html", "/redirect/3.html");
    // [ts] server.setRedirect("/redirect/3.html", server.EMPTY_PAGE);
    // [ts] const response = (page.goto(server.PREFIX + "/redirect/1.html"));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("should wait for network idle to succeed navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let responses: []ServerResponse = [];
    // Hold on to a bunch of requests without answering.
    // [ts] server.setRoute("/fetch-request-a.js", (_req, res) => {
    // [ts] return responses.push(res);
    // [ts] });
    // [ts] server.setRoute("/fetch-request-b.js", (_req, res) => {
    // [ts] return responses.push(res);
    // [ts] });
    // [ts] server.setRoute("/fetch-request-c.js", (_req, res) => {
    // [ts] return responses.push(res);
    // [ts] });
    // [ts] server.setRoute("/fetch-request-d.js", (_req, res) => {
    // [ts] return responses.push(res);
    // [ts] });
    // [ts] const initialFetchResourcesRequested = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/fetch-request-a.js"),
    // [ts] server.waitForRequest("/fetch-request-b.js"),
    // [ts] server.waitForRequest("/fetch-request-c.js"),
    // [ts] ]).catch(() => {
    // Ignore Error that arise from test server during hooks
    // [ts] });
    // [ts] const secondFetchResourceRequested = server
    // [ts] .waitForRequest("/fetch-request-d.js")
    // [ts] .catch(() => {
    // Ignore Error that arise from test server during hooks
    // [ts] });
    // Track when the navigation gets completed.
    // [ts] let navigationFinished = false;
    // [ts] let navigationError: Error | null;
    // Navigate to a page which loads immediately and then does a bunch of
    // requests via javascript's fetch method.
    // [ts] const navigationPromise = page
    // [ts] .goto(server.PREFIX + "/networkidle.html", {
    // [ts] waitUntil: "networkidle0",
    // [ts] })
    // [ts] .then(response => {
    // [ts] navigationFinished = true;
    // [ts] return response;
    // [ts] })
    // [ts] .catch(error => {
    // [ts] navigationError = error;
    // [ts] return null;
    // [ts] });
    // [ts] let afterNavigationError: Error | null;
    // [ts] const afterNavigationPromise = (() => {
    // Wait for the page's 'load' event.
    // [ts] waitEvent(page, "load");
    // [ts] expect_eq(navigationFinished, false);
    // Wait for the initial three resources to be requested.
    // [ts] initialFetchResourcesRequested;
    // Expect navigation still to be not finished.
    // [ts] expect_eq(navigationFinished, false);
    // Respond to initial requests.
    // [ts] for (const response of responses) {
    // [ts] response.statusCode = 404;
    // [ts] response.end(`File not found`);
    // [ts] }
    // Reset responses array
    // [ts] responses = [];
    // Wait for the second round to be requested.
    // [ts] secondFetchResourceRequested;
    // Expect navigation still to be not finished.
    // [ts] expect_eq(navigationFinished, false);
    // Respond to requests.
    // [ts] for (const response of responses) {
    // [ts] response.statusCode = 404;
    // [ts] response.end(`File not found`);
    // [ts] }
    // [ts] })().catch(error => {
    // [ts] afterNavigationError = error;
    // [ts] });
    // [ts] race([navigationPromise, afterNavigationPromise]); // migrated: Promise.race -> race
    // [ts] if (navigationError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw navigationError;)
    // [ts] }
    // [ts] joinAll([navigationPromise, afterNavigationPromise]); // migrated: Promise.all -> joinAll
    // [ts] if (afterNavigationError) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw afterNavigationError;)
    // [ts] }
    // Expect navigation to succeed.
    // [ts] expect_eq(navigationFinished, true);
    // [ts] expect_eq((navigationPromise)?.ok(), true);
}
