// Migrated from TypeScript (src/acceptInsecureCerts.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

// [cap-migrate] import {TLSSocket} from "node:tls";

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {HTTPResponse} from "puppeteer-core/internal/api/HTTPResponse.js";

import { setupSeparateTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("acceptInsecureCerts", async () => {
  /* Note that this test creates its own browser rather than use
   * the one provided by the test set-up as we need one
   * with acceptInsecureCerts set to true
   */
  const state = setupSeparateTestBrowserHooks({
    acceptInsecureCerts: true,
  });

  // [cap-test] describe("Response.securityDetails", () => {
  // [cap-test] });

  // [cap-test] });
// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {httpsServer, page} = state;
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] httpsServer.waitForRequest("/empty.html"),
    // [ts] page.goto(httpsServer.EMPTY_PAGE),
    // [ts] ]);
    // [ts] const securityDetails = response.securityDetails();
    // [ts] expect_eq(securityDetails.issuer(), "puppeteer-tests");
    // [ts] const protocol = (serverRequest.socket)
    // [ts] .getProtocol()
    // [ts] .replace("v", " ");
    // [ts] expect_eq(securityDetails.protocol(), protocol);
    // [ts] expect_eq(securityDetails.subjectName(), "puppeteer-tests");
    // [ts] expect_eq(securityDetails.validFrom(), 1589357069);
    // [ts] expect_eq(securityDetails.validTo(), 1904717069);
    // [ts] expect(securityDetails.subjectAlternativeNames()).toEqual([
    // [ts] "www.puppeteer-tests.test",
    // [ts] "www.puppeteer-tests-1.test",
    // [ts] ]);
    // [ts] });
    // [ts] test("should be |null| for non-secure requests", () => {
    // [ts] const {server, page} = state;
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.securityDetails(), null);
    // [ts] });
    // [ts] test("Network redirects should report SecurityDetails", () => {
    // [ts] const {httpsServer, page} = state;
    // [ts] httpsServer.setRedirect("/plzredirect", "/empty.html");
    // [ts] const responses: []HTTPResponse = [];
    // [ts] page.on("response", response => {
    // [ts] return responses.push(response);
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] httpsServer.waitForRequest("/plzredirect"),
    // [ts] page.goto(httpsServer.PREFIX + "/plzredirect"),
    // [ts] ]);
    // [ts] expect(responses).toHaveLength(2);
    // [ts] expect_eq(responses[0].status(), 302);
    // [ts] const securityDetails = responses[0].securityDetails();
    // [ts] const protocol = (serverRequest.socket)
    // [ts] .getProtocol()
    // [ts] .replace("v", " ");
    // [ts] expect_eq(securityDetails.protocol(), protocol);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {httpsServer, page} = state;
    // [ts] let error: Error;
    // [ts] const response = page.goto(httpsServer.EMPTY_PAGE).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error == null, true);
    // [ts] expect_eq(response.ok(), true);
    // [ts] });
    // [ts] test("should work with request interception", () => {
    // [ts] const {httpsServer, page} = state;
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] return request.continue();
    // [ts] });
    // [ts] const response = (page.goto(httpsServer.EMPTY_PAGE));
    // [ts] expect_eq(response.status(), 200);
    // [ts] });
    // [ts] test("should work with mixed content", () => {
    // [ts] const {server, httpsServer, page} = state;
    // [ts] httpsServer.setRoute("/mixedcontent.html", (_req, res) => {
    // [ts] res.end(`<iframe src=${server.EMPTY_PAGE}></iframe>`);
    // [ts] });
    // [ts] page.goto(httpsServer.PREFIX + "/mixedcontent.html", {
    // [ts] waitUntil: "load",
    // [ts] });
    // [ts] expect(page.frames()).toHaveLength(2);
    // Make sure blocked iframe has functional execution context
    // @see https://github.com/puppeteer/puppeteer/issues/2709
    // [ts] expect_eq(page.frames()[0].evaluate("1 + 2"), 3);
    // [ts] expect_eq(page.frames()[1].evaluate("2 + 3"), 5);
    // [ts] });
    // [ts] test("works for service worker", () => {
    // [ts] const {httpsServer, page} = state;
    // [ts] page.goto(httpsServer.PREFIX + "/serviceworkers/empty/sw.html");
    // [ts] page.evaluate(() => {
    // [ts] return (
    // [ts] globalThis>; }
    // [ts] ).registrationPromise.then((registration) => {
    // [ts] return registration.unregister();
    // [ts] });
    // [ts] });
}
