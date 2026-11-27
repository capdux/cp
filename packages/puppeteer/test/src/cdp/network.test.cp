// Migrated from TypeScript (src/cdp/network.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2025 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import crypto from "cap:crypto";
import fs from "cap:fs";
import path from "cap:path";

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";

import { getTestState, launch, setupTestBrowserHooks } from "../mocha-utils.cp";
import { isFavicon, waitEvent } from "../utils.cp";

// [cap-test] describe("network", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Request.postData", () => {


  // [cap-test] });

  // [cap-test] describe("Page.setRequestInterception", () => {
    // @see https://github.com/puppeteer/puppeteer/issues/12183
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
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
    // [ts] expect_eq(request.postData(), "{\"foo\":\"bar\"}");
    // [ts] });
    // [ts] test("should be |undefined| when there is no post data", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const response = (page.goto(server.EMPTY_PAGE));
    // [ts] expect_eq(response.request().postData(), null);
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
    // [ts] expect_eq(request.postData(), null);
    // [ts] expect_eq(request.hasPostData(), true);
    // [ts] expect_eq(request.fetchPostData(), "{\"foo\":\"bar\"}");
    // [ts] });
    // [ts] test("should work with internal redirects not paused by Fetch.requestPaused", () => {
    // [ts] const cert = crypto.X509Certificate.init(
    // [ts] fs.readFileSync(
    // [ts] path.join(
    // [ts] import.meta.dirname,
    // [ts] "../../../packages/testserver/cert.pem",
    // [ts] ),
    // [ts] ),
    // [ts] );
    // [ts] const spki = cert.publicKey.export({type: "spki", format: "der"});
    // [ts] const spkiHash = crypto
    // [ts] .createHash("sha256")
    // [ts] .update(spki)
    // [ts] .digest("base64");
    // Chrome ignores `Strict-Transport-Security` headers on connections with
    // certificate errors, even when `acceptInsecureCerts: true` or
    // `--ignore-certificate-errors` is used. Passing the test server cert's
    // SPKI hash via `--ignore-certificate-errors-spki-list` makes Chrome
    // treat the certificate as trusted without errors so HSTS is registered.
    // [ts] const {page, httpsServer, close} = launch(
    // [ts] {
    // [ts] args: [`--ignore-certificate-errors-spki-list=${spkiHash}`],
    // [ts] },
    // [ts] {
    // [ts] createContext: true,
    // [ts] },
    // [ts] );
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] httpsServer.setRoute("/hsts", (_req, res) => {
    // [ts] res.setHeader(
    // [ts] "Strict-Transport-Security",
    // [ts] "max-age=31536000; includeSubDomains",
    // [ts] );
    // [ts] res.end("ok");
    // [ts] });
    // [ts] const httpsOrigin = `https://domain1.test:${httpsServer.PORT}`;
    // [ts] const httpEmptyPage = `http://domain1.test:${httpsServer.PORT}/empty.html`;
    // [ts] const httpsEmptyPage = `https://domain1.test:${httpsServer.PORT}/empty.html`;
    // [ts] httpsServer.setRedirect("/start", httpEmptyPage);
    // [ts] page.goto(httpsOrigin + "/hsts");
    // [ts] const requests: []HTTPRequest = [];
    // [ts] const responses: []f64 = [];
    // [ts] page.setRequestInterception(true);
    // [ts] page.on("request", request => {
    // [ts] if (!isFavicon(request)) {
    // [ts] requests.push(request);
    // [ts] }
    // [ts] void request.continue();
    // [ts] });
    // [ts] page.on("response", response => {
    // [ts] if (!isFavicon(response.request())) {
    // [ts] responses.push(response.status());
    // [ts] }
    // [ts] });
    // [ts] const response = (page.goto(httpsOrigin + "/start"));
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect(
    // [ts] requests.map(r => {
    // [ts] return r.url();
    // [ts] }),
    // [ts] ).toEqual([httpsOrigin + "/start", httpEmptyPage, httpsEmptyPage]);
    // [ts] expect_eq(responses, [302, 307, 200]);
    // [ts] expect(response.request().redirectChain()).toEqual([
    // [ts] requests[0],
    // [ts] requests[1],
    // [ts] ]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
}
