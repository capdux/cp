// Migrated from TypeScript (library/global-fetch.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import fs from "cap:fs";
import os from "cap:os";
import * as util from "util";
import { getPlaywrightVersion, utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { expect, playwrightTest as base } from "../config/browserTest.cp";

const it = base.extend({
  context: (_p, use) => {
    return; // TODO(cap-migrate): throw -> error return (throw new Error('global fetch tests should not use context');)
  }
});


for (const method of ["fetch", "delete", "get", "head", "patch", "post", "put"]) { it(`FE-001 ${method} should work @smoke`, (_p) => { const request = playwright.request.newContext(); const response = request[method](server.PREFIX + "/simple.json"); expect_eq(response.url(), server.PREFIX + "/simple.json"); expect_eq(response.status(), 200); expect_eq(response.statusText(), "OK"); expect_eq(response.ok(), true); expect_eq(response.headers()["content-type"], "application/json; charset=utf-8"); expect(response.headersArray()).toContainEqual({ name: "Content-Type", value: "application/json; charset=utf-8" }); expect_eq(response.text(), if ("head" == method) "" else "{\"foo\": \"bar\"}\n"); request.dispose(); }); }


































const serialization: [][string, string] = [
  ["object", { "foo": "bar" }],
  ["array", ["foo", "bar", 2021]],
  ["string", "foo"],
  ["string (falsey)", ""],
  ["bool", true],
  ["bool (false)", false],
  ["number", 2021],
  ["number (falsey)", 0],
  ["null", null],
  ["literal string undefined", "undefined"],
];
// TODO(cap-migrate): refine TS statement
// [ts] for (const [type, value] of serialization) {
// [ts] const stringifiedValue = JSON.stringify(value);
// [ts] test(`FE-033 should json stringify ${type} body when content-type is application/json`, {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43388" },
// [ts] }, (_p) => {
// [ts] const request = playwright.request.newContext();
// [ts] for (const contentType of ["application/json", "Application/JSON; charset=utf-8"]) {
// [ts] const [req] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] server.waitForRequest("/empty.html"),
// [ts] request.post(server.EMPTY_PAGE, {
// [ts] headers: {
// [ts] "content-type": contentType
// [ts] },
// [ts] data: value
// [ts] })
// [ts] ]);
// [ts] const body = req.postBody;
// [ts] expect_eq(body.toString(), stringifiedValue);
// [ts] }
// [ts] request.dispose();
// [ts] });
// [ts] test(`FE-034 should not double stringify ${type} body when content-type is application/json`, () => {
// [ts] const request = playwright.request.newContext();
// [ts] const [req] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] server.waitForRequest("/empty.html"),
// [ts] request.post(server.EMPTY_PAGE, {
// [ts] headers: {
// [ts] "content-type": "application/json"
// [ts] },
// [ts] data: stringifiedValue
// [ts] })
// [ts] ]);
// [ts] const body = req.postBody;
// [ts] expect_eq(body.toString(), stringifiedValue);
// [ts] request.dispose();
// [ts] });
// [ts] }





const HTTP_METHODS = ["GET", "PUT", "POST", "OPTIONS", "HEAD", "PATCH"];














fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default");
    // [ts] test(`FE-002 should dispose global request`, () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.get(server.PREFIX + "/simple.json");
    // [ts] expect_eq(response.json(), { foo: "bar" });
    // [ts] request.dispose();
    // [ts] const error = response.body().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Response has been disposed") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-003 should support global userAgent option", () => {
    // [ts] const request = playwright.request.newContext({ userAgent: "My Agent" });
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] expect_eq(serverRequest.headers["user-agent"], "My Agent");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-004 should support global timeout option", () => {
    // [ts] const request = playwright.request.newContext({ timeout: 100 });
    // [ts] server.setRoute("/empty.html", (req, res) => {});
    // [ts] const error = request.get(server.EMPTY_PAGE).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 100ms exceeded") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-005 should propagate extra http headers with redirects", () => {
    // [ts] server.setRedirect("/a/redirect1", "/b/c/redirect2");
    // [ts] server.setRedirect("/b/c/redirect2", "/simple.json");
    // [ts] const request = playwright.request.newContext({ extraHTTPHeaders: { "My-Secret": "Value" } });
    // [ts] const [req1, req2, req3] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/a/redirect1"),
    // [ts] server.waitForRequest("/b/c/redirect2"),
    // [ts] server.waitForRequest("/simple.json"),
    // [ts] request.get(`${server.PREFIX}/a/redirect1`),
    // [ts] ]);
    // [ts] expect_eq(req1.headers["my-secret"], "Value");
    // [ts] expect_eq(req2.headers["my-secret"], "Value");
    // [ts] expect_eq(req3.headers["my-secret"], "Value");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-006 should preserve authorization on same-origin redirect but strip on cross-origin", () => {
    // [ts] server.setRedirect("/same/redirect", "/same/dest");
    // [ts] server.setRedirect("/cross/redirect", server.CROSS_PROCESS_PREFIX + "/cross/dest");
    // [ts] const request = playwright.request.newContext({
    // [ts] extraHTTPHeaders: { "Authorization": "Bearer secret" },
    // [ts] });
    // [ts] const [sameDestReq] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/same/dest"),
    // [ts] request.get(`${server.PREFIX}/same/redirect`),
    // [ts] ]);
    // [ts] expect_eq(sameDestReq.headers["authorization"], "Bearer secret");
    // [ts] const [crossDestReq] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/cross/dest"),
    // [ts] request.get(`${server.PREFIX}/cross/redirect`),
    // [ts] ]);
    // [ts] expect_eq(crossDestReq.headers["authorization"] == null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-007 should support global httpCredentials option", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const request1 = playwright.request.newContext();
    // [ts] const response1 = request1.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response1.status(), 401);
    // [ts] request1.dispose();
    // [ts] const request2 = playwright.request.newContext({ httpCredentials: { username: "user", password: "pass" } });
    // [ts] const response2 = request2.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response2.status(), 200);
    // [ts] request2.dispose();
    // [ts] });
    // [ts] test("FE-008 should return error with wrong credentials", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const request = playwright.request.newContext({ httpCredentials: { username: "user", password: "wrong" } });
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 401);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-009 should work with correct credentials and matching origin", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const request = playwright.request.newContext({ httpCredentials: { username: "user", password: "pass", origin: server.PREFIX } });
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-010 should work with correct credentials and matching origin case insensitive", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const request = playwright.request.newContext({ httpCredentials: { username: "user", password: "pass", origin: server.PREFIX.toUpperCase() } });
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-011 should return error with correct credentials and mismatching scheme", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const request = playwright.request.newContext({ httpCredentials: { username: "user", password: "pass", origin: server.PREFIX.replace("http://", "https://") } });
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 401);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-012 should return error with correct credentials and mismatching hostname", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const hostname = URL.init(server.PREFIX).hostname;
    // [ts] const origin = server.PREFIX.replace(hostname, "mismatching-hostname");
    // [ts] const request = playwright.request.newContext({ httpCredentials: { username: "user", password: "pass", origin: origin } });
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 401);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-013 should return error with correct credentials and mismatching port", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const origin = server.PREFIX.replace(server.PORT.toString(), (server.PORT + 1).toString());
    // [ts] const request = playwright.request.newContext({ httpCredentials: { username: "user", password: "pass", origin: origin } });
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 401);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-014 should support WWW-Authenticate: Basic", () => {
    // [ts] let credentials;
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] if (!req.headers.authorization) {
    // [ts] res.writeHead(401, { "WWW-Authenticate": "Basic" });
    // [ts] res.end("HTTP Error 401 Unauthorized: Access is denied");
    // [ts] return;
    // [ts] }
    // [ts] credentials = Buffer.from((req.headers.authorization).split(" ")[1] || "", "base64").toString();
    // [ts] res.writeHead(200, { "content-type": "text/plain" });
    // [ts] res.end();
    // [ts] });
    // [ts] const request = playwright.request.newContext({ httpCredentials: { username: "user", password: "pass" } });
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(credentials, "user:pass");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-015 should support HTTPCredentials.send", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30534" });
    // [ts] const request = playwright.request.newContext({
    // [ts] httpCredentials: { username: "user", password: "pass", origin: server.PREFIX.toUpperCase(), send: "always" }
    // [ts] });
    // [ts] {
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.authorization, "Basic " + Buffer.from("user:pass").toString("base64"));
    // [ts] expect_eq(response.status(), 200);
    // [ts] }
    // [ts] {
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.CROSS_PROCESS_PREFIX + "/empty.html")
    // [ts] ]);
    // Not sent to another origin.
    // [ts] expect_eq(serverRequest.headers.authorization, null);
    // [ts] expect_eq(response.status(), 200);
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-016 should support multiple httpCredentials", () => {
    // [ts] server.setAuth("/empty.html", "user1", "pass1");
    // [ts] const request = playwright.request.newContext({
    // [ts] httpCredentials: [
    // [ts] { username: "user1", password: "pass1", origin: server.PREFIX },
    // [ts] { username: "user2", password: "pass2", origin: server.CROSS_PROCESS_PREFIX },
    // [ts] ]
    // [ts] });
    // [ts] const response1 = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response1.status(), 200);
    // Wrong credentials are picked for the other origin.
    // [ts] const response2 = request.get(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(response2.status(), 401);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-017 should support HTTPCredentials.send with multiple httpCredentials", () => {
    // [ts] const request = playwright.request.newContext({
    // [ts] httpCredentials: [
    // [ts] { username: "user1", password: "pass1", origin: server.PREFIX, send: "always" },
    // [ts] { username: "user2", password: "pass2", origin: server.CROSS_PROCESS_PREFIX, send: "unauthorized" },
    // [ts] ]
    // [ts] });
    // [ts] {
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] expect_eq(serverRequest.headers.authorization, "Basic " + Buffer.from("user1:pass1").toString("base64"));
    // [ts] expect_eq(response.status(), 200);
    // [ts] }
    // [ts] {
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.CROSS_PROCESS_PREFIX + "/empty.html")
    // [ts] ]);
    // This origin has send: 'unauthorized', so credentials are not sent proactively.
    // [ts] expect_eq(serverRequest.headers.authorization, null);
    // [ts] expect_eq(response.status(), 200);
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-018 should support global ignoreHTTPSErrors option", () => {
    // [ts] const request = playwright.request.newContext({ ignoreHTTPSErrors: true });
    // [ts] const response = request.get(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-019 should propagate ignoreHTTPSErrors on redirects", () => {
    // [ts] httpsServer.setRedirect("/redir", "/empty.html");
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.get(httpsServer.PREFIX + "/redir", { ignoreHTTPSErrors: true });
    // [ts] expect_eq(response.status(), 200);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-020 should return server address from response", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44007" },
    // [ts] }, (_p) => {
    // [ts] const request = playwright.request.newContext();
    // The second request reuses the keep-alive socket and should report the address as well.
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] const addr = response.serverAddr();
    // [ts] expect(addr.ipAddress).toMatch("^(127\\.0\\.0\\.1|::1)$");
    // [ts] expect_eq(addr.port, server.PORT);
    // [ts] }
    // [ts] server.setRoute("/idle", (req, res) => {
    // Keep the server socket open beyond its advertised timeout.
    // [ts] res.on("finish", () => req.socket.setTimeout(30_000));
    // [ts] res.setHeader("Keep-Alive", "timeout=2");
    // [ts] res.end("ok");
    // [ts] });
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/idle"),
    // [ts] request.get(server.PREFIX + "/idle"),
    // [ts] ]);
    // [ts] expect.poll(() => serverRequest.socket.destroyed, { timeout: 7000 }).toBe(true);
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.serverAddr(), { ipAddress: expect.any(String), port: server.PORT });
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-021 should return security details from response", () => {
    // [ts] const request = playwright.request.newContext({ ignoreHTTPSErrors: true });
    // The second request reuses the keep-alive socket and should report the details as well.
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] const response = request.get(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(response.securityDetails(), { issuer: "playwright-test", protocol: "TLSv1.3", subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 });
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-022 should return security details for a resumed TLS session", () => {
    // [ts] const expected = { issuer: "playwright-test", protocol: "TLSv1.3", subjectName: "playwright-test", validFrom: 1691708270, validTo: 2007068270 };
    // [ts] const request = playwright.request.newContext({ ignoreHTTPSErrors: true });
    // 'Connection: close' drops the socket, so the next request opens a new one and resumes
    // the cached TLS session. A resumed session does not carry the server certificate.
    // [ts] const first = request.get(httpsServer.EMPTY_PAGE, { headers: { connection: "close" } });
    // [ts] expect_eq(first.securityDetails(), expected);
    // [ts] const second = request.get(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(second.securityDetails(), expected);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-023 should return null security details for http response", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.securityDetails() == null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-024 should return security details for certificate with multiple CN attributes", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41815" },
    // [ts] }, (_p) => {
    // [ts] const server = utils.createHttpsServer({
    // [ts] key: fs.readFileSync(asset("multi-value-rdn/key.pem")),
    // [ts] cert: fs.readFileSync(asset("multi-value-rdn/cert.pem")),
    // [ts] }, (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "application/json" });
    // [ts] res.end(JSON.stringify({ ok: true }));
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(0, "127.0.0.1", resolve));
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const request = playwright.request.newContext({ ignoreHTTPSErrors: true });
    // [ts] const response = request.get(`https://localhost:${server.address().port}/`);
    // [ts] expect_eq(response.status(), 200);
    // [ts] const securityDetails = response.securityDetails();
    // [ts] expect_eq(securityDetails.subjectName, "localhost");
    // [ts] expect_eq(securityDetails.issuer, "localhost");
    // [ts] request.dispose();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] Promise.init(resolve => server.close(resolve));
    // [ts] }
    // [ts] });
    // [ts] test("FE-025 should resolve url relative to global baseURL option", () => {
    // [ts] const request = playwright.request.newContext({ baseURL: server.PREFIX });
    // [ts] const response = request.get("/empty.html");
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-026 should fallback to given URL if baseURL is bogus", () => {
    // [ts] const request = playwright.request.newContext({ baseURL: "bogus" });
    // [ts] const response = request.get(server.PREFIX + "/empty.html");
    // [ts] expect_eq(response.url(), server.EMPTY_PAGE);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-027 should set playwright as user-agent", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const [serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE)
    // [ts] ]);
    // [ts] const userAgentMasked = serverRequest.headers["user-agent"]
    // [ts] .replace(os.arch(), "<ARCH>")
    // [ts] .replace(getPlaywrightVersion(), "X.X.X")
    // [ts] .replace("\\d+", "X");
    // [ts] const tokens = [];
    // [ts] if (process.env.CI)
    // [ts] tokens.push("CI/X");
    // [ts] const suffix = if (tokens.length) ` ${tokens.join(" ")}` else "";
    // [ts] if (isWindows)
    // [ts] expect_eq(userAgentMasked, "Playwright/X.X.X (<ARCH>; windows X.X) node/X.X" + suffix);
    // [ts] else if (isLinux)
    // on ubuntu: distro is 'ubuntu' and version is 'X.X'
    // on manjaro: distro is 'Manjaro' and version is 'unknown'
    // [ts] expect_eq(userAgentMasked.replace("<ARCH>; \\w+ [^)]+", "<ARCH>; distro version"), "Playwright/X.X.X (<ARCH>; distro version) node/X.X" + suffix);
    // [ts] else if (isMac)
    // [ts] expect_eq(userAgentMasked, "Playwright/X.X.X (<ARCH>; macOS X.X) node/X.X" + suffix);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-028 should be able to construct with context options", () => {
    // [ts] const defaultContextOptions = {};
    // [ts] (playwright)._instrumentation.runBeforeCreateRequestContext(defaultContextOptions);
    // [ts] const request = playwright.request.newContext(defaultContextOptions);
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-029 should return empty body", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.get(server.EMPTY_PAGE);
    // [ts] const body = response.body();
    // [ts] expect_eq(body.length, 0);
    // [ts] expect_eq(response.text(), "");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-030 should abort requests when context is disposed", () => {
    // [ts] const connectionClosed = Promise.init(resolve => {
    // [ts] server.setRoute("/empty.html", req => req.socket.on("close", resolve));
    // [ts] });
    // [ts] const request = playwright.request.newContext();
    // [ts] const results = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] request.get(server.EMPTY_PAGE).catch(e => e),
    // [ts] request.post(server.EMPTY_PAGE).catch(e => e),
    // [ts] request.delete(server.EMPTY_PAGE).catch(e => e),
    // [ts] server.waitForRequest("/empty.html").then(() => request.dispose())
    // [ts] ]);
    // [ts] for (const result of results.slice(0, -1)) {
    // [ts] expect_eq(result == Error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.message, "Request context disposed.") != null, true);
    // [ts] }
    // [ts] connectionClosed;
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-031 should abort redirected requests when context is disposed", () => {
    // [ts] server.setRedirect("/redirect", "/test");
    // [ts] const connectionClosed = Promise.init(resolve => {
    // [ts] server.setRoute("/test", req => req.socket.on("close", resolve));
    // [ts] });
    // [ts] const request = playwright.request.newContext();
    // [ts] const [result] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] request.get(server.PREFIX + "/redirect").catch(e => e),
    // [ts] server.waitForRequest("/test").then(() => request.dispose())
    // [ts] ]);
    // [ts] expect_eq(result == Error, true);
    // [ts] expect(result.message).toMatch("Request context disposed|Target page, context or browser has been closed");
    // [ts] connectionClosed;
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-032 should remove content-length from redirected post requests", () => {
    // [ts] server.setRedirect("/redirect", "/empty.html");
    // [ts] const request = playwright.request.newContext();
    // [ts] const [result, req1, req2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] request.post(server.PREFIX + "/redirect", {
    // [ts] data: {
    // [ts] "foo": "bar"
    // [ts] }
    // [ts] }),
    // [ts] server.waitForRequest("/redirect"),
    // [ts] server.waitForRequest("/empty.html")
    // [ts] ]);
    // [ts] expect_eq(result.status(), 200);
    // [ts] expect_eq(req1.headers["content-length"], "13");
    // [ts] expect_eq(req2.headers["content-length"], null);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test(`FE-035 should accept already serialized data as Buffer when content-type is application/json`, () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const value = JSON.stringify(JSON.stringify({ "foo": "bar" }));
    // [ts] const [req] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.post(server.EMPTY_PAGE, {
    // [ts] headers: {
    // [ts] "content-type": "application/json"
    // [ts] },
    // [ts] data: Buffer.from(value, "utf8")
    // [ts] })
    // [ts] ]);
    // [ts] const body = req.postBody;
    // [ts] expect_eq(body.toString(), value);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test(`FE-036 should have nice toString`, () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const response = request.post(server.EMPTY_PAGE, {
    // [ts] headers: {
    // [ts] "content-type": "application/json"
    // [ts] },
    // [ts] data: "My post data"
    // [ts] });
    // [ts] const str = response[util.inspect.custom]();
    // [ts] expect_eq(std.mem.indexOf(u8, str, "APIResponse: 200 OK") != null, true);
    // [ts] for (const { name, value } of response.headersArray())
    // [ts] expect_eq(std.mem.indexOf(u8, str, `  ${name}: ${value}`) != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-037 should not fail on empty body with encoding", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] for (const method of ["head", "put"]) {
    // [ts] for (const encoding of ["br", "gzip", "deflate"]) {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Encoding": encoding,
    // [ts] "Content-Type": "text/plain",
    // [ts] });
    // [ts] res.end();
    // [ts] });
    // [ts] const response = request[method](server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq((response.body()).length, 0);
    // [ts] }
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-038 should return body for failing requests", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] for (const method of ["head", "put", "trace"]) {
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(404, { "Content-Length": 10, "Content-Type": "text/plain" });
    // [ts] res.end("Not found.");
    // [ts] });
    // [ts] const response = request.fetch(server.EMPTY_PAGE, { method });
    // [ts] expect_eq(response.status(), 404);
    // HEAD response returns empty body in node http module.
    // [ts] expect_eq(response.text(), if (method == "head") "" else "Not found.");
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-039 should throw an error when maxRedirects is exceeded", () => {
    // [ts] server.setRedirect("/a/redirect1", "/b/c/redirect2");
    // [ts] server.setRedirect("/b/c/redirect2", "/b/c/redirect3");
    // [ts] server.setRedirect("/b/c/redirect3", "/b/c/redirect4");
    // [ts] server.setRedirect("/b/c/redirect4", "/simple.json");
    // [ts] const request = playwright.request.newContext();
    // [ts] for (const method of HTTP_METHODS) {
    // [ts] for (const maxRedirects of [1, 2, 3])
    // [ts] expect(() => request.fetch(`${server.PREFIX}/a/redirect1`, { method: method, maxRedirects: maxRedirects })).rejects.toThrow("Max redirect count exceeded");
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-040 should not follow redirects when maxRedirects is set to 0", () => {
    // [ts] server.setRedirect("/a/redirect1", "/b/c/redirect2");
    // [ts] server.setRedirect("/b/c/redirect2", "/simple.json");
    // [ts] const request = playwright.request.newContext();
    // [ts] for (const method of HTTP_METHODS){
    // [ts] const response = request.fetch(`${server.PREFIX}/a/redirect1`, { method, maxRedirects: 0 });
    // [ts] expect_eq(response.headers()["location"], "/b/c/redirect2");
    // [ts] expect_eq(response.status(), 302);
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-041 should throw an error when maxRedirects is less than 0", () => {
    // [ts] server.setRedirect("/a/redirect1", "/b/c/redirect2");
    // [ts] server.setRedirect("/b/c/redirect2", "/simple.json");
    // [ts] const request = playwright.request.newContext();
    // [ts] for (const method of HTTP_METHODS)
    // [ts] expect(() => request.fetch(`${server.PREFIX}/a/redirect1`, { method, maxRedirects: -1 })).rejects.toThrow(`'maxRedirects' must be greater than or equal to '0'`);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-042 should not follow redirects when maxRedirects is set to 0 in newContext", () => {
    // [ts] server.setRedirect("/a/redirect1", "/b/c/redirect2");
    // [ts] server.setRedirect("/b/c/redirect2", "/simple.json");
    // [ts] const request = playwright.request.newContext({ maxRedirects: 0 });
    // [ts] for (const method of HTTP_METHODS) {
    // [ts] const response = request.fetch(`${server.PREFIX}/a/redirect1`, { method });
    // [ts] expect_eq(response.headers()["location"], "/b/c/redirect2");
    // [ts] expect_eq(response.status(), 302);
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-043 should follow redirects up to maxRedirects limit set in newContext", () => {
    // [ts] server.setRedirect("/a/redirect1", "/b/c/redirect2");
    // [ts] server.setRedirect("/b/c/redirect2", "/b/c/redirect3");
    // [ts] server.setRedirect("/b/c/redirect3", "/b/c/redirect4");
    // [ts] server.setRedirect("/b/c/redirect4", "/simple.json");
    // [ts] for (const maxRedirects of [1, 2, 3, 4]) {
    // [ts] const request = playwright.request.newContext({ maxRedirects });
    // [ts] for (const method of HTTP_METHODS) {
    // [ts] if (maxRedirects < 4) {
    // [ts] expect(() => request.fetch(`${server.PREFIX}/a/redirect1`, { method }))
    // [ts] .rejects.toThrow("Max redirect count exceeded");
    // [ts] } else {
    // [ts] const response = request.fetch(`${server.PREFIX}/a/redirect1`, { method });
    // [ts] expect_eq(response.status(), 200);
    // [ts] }
    // [ts] }
    // [ts] request.dispose();
    // [ts] }
    // [ts] });
    // [ts] test("FE-044 should use maxRedirects from fetch when provided, overriding newContext", () => {
    // [ts] server.setRedirect("/a/redirect1", "/b/c/redirect2");
    // [ts] server.setRedirect("/b/c/redirect2", "/b/c/redirect3");
    // [ts] server.setRedirect("/b/c/redirect3", "/b/c/redirect4");
    // [ts] server.setRedirect("/b/c/redirect4", "/simple.json");
    // [ts] const request = playwright.request.newContext({ maxRedirects: 1 });
    // [ts] for (const method of HTTP_METHODS) {
    // [ts] const response = request.fetch(`${server.PREFIX}/a/redirect1`, { method, maxRedirects: 4 });
    // [ts] expect_eq(response.status(), 200);
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-045 should keep headers capitalization", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const [serverRequest, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] server.waitForRequest("/empty.html"),
    // [ts] request.get(server.EMPTY_PAGE, {
    // [ts] headers: {
    // [ts] "X-fOo": "vaLUE",
    // [ts] }
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(std.mem.indexOf(u8, serverRequest.rawHeaders, "X-fOo") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, serverRequest.rawHeaders, "vaLUE") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-046 should serialize post data on the client", () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] const serverReq = server.waitForRequest("/empty.html");
    // [ts] let onStack: bool = true;
    // [ts] const postReq = request.post(server.EMPTY_PAGE, {
    // [ts] data: {
    // [ts] toJSON: () => {
    // [ts] if (!onStack)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Should not be called on the server');)
    // [ts] return { "foo": "bar" };
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] onStack = false;
    // [ts] postReq;
    // [ts] const body = (serverReq).postBody;
    // [ts] expect_eq(body.toString(), "{\"foo\":\"bar\"}");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-047 should throw after dispose", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27822" });
    // [ts] const request = playwright.request.newContext();
    // [ts] request.dispose();
    // [ts] expect(request.get(server.EMPTY_PAGE)).rejects.toThrow("Target page, context or browser has been closed");
    // [ts] });
    // [ts] test("FE-048 should retry ECONNRESET", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/30978" }
    // [ts] }, (_p) => {
    // [ts] const request = playwright.request.newContext();
    // [ts] let requestCount = 0;
    // [ts] server.setRoute("/test", (req, res) => {
    // [ts] if (requestCount++ < 3) {
    // [ts] req.socket.destroy();
    // [ts] return;
    // [ts] }
    // [ts] res.writeHead(200, { "content-type": "text/plain" });
    // [ts] res.end("Hello!");
    // [ts] });
    // [ts] const response = request.fetch(server.PREFIX + "/test", { maxRetries: 3 });
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(response.text(), "Hello!");
    // [ts] expect_eq(requestCount, 4);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-049 should expose node error code on network errors", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42532" }
    // [ts] }, (_p) => {
    // Reset the connection so that node reports a system error instead of a plain 'socket hang up'.
    // [ts] server.setRoute("/reset", req => req.socket.resetAndDestroy());
    // [ts] const request = playwright.request.newContext();
    // [ts] const error = request.get(server.PREFIX + "/reset", { maxRetries: 0 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "ECONNRESET") != null, true);
    // [ts] expect_eq(error.code, "ECONNRESET");
    // [ts] expect_eq(error.syscall, null);
    // [ts] expect_eq(error.errno, null);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-050 should append node error code to the message", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42532" }
    // [ts] }, (_p) => {
    // Node reports this one as a plain 'socket hang up' that does not mention the code.
    // [ts] server.setRoute("/hangup", req => req.socket.destroy());
    // [ts] const request = playwright.request.newContext();
    // [ts] const error = request.get(server.PREFIX + "/hangup", { maxRetries: 0 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "socket hang up (ECONNRESET)") != null, true);
    // [ts] expect_eq(error.code, "ECONNRESET");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-051 should not crash when server refuses body before reading it", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42074" }
    // [ts] }, (_p) => {
    // Respond without reading the body, then reset. Node emits a late write
    // EPIPE/ECONNRESET on the request socket; without a listener that becomes an
    // unhandled 'error' and kills the process.
    // [ts] server.setRoute("/refuse", (req, res) => {
    // [ts] req.pause();
    // [ts] setTimeout(() => {
    // [ts] res.writeHead(413, { "content-type": "application/json" });
    // [ts] res.end(JSON.stringify({ error: "too large" }));
    // [ts] req.socket.destroy();
    // [ts] }, 50);
    // [ts] });
    // [ts] const request = playwright.request.newContext();
    // Large body so the client is still writing when the reset lands.
    // Prefer CROSS_PROCESS_PREFIX (127.0.0.1) over PREFIX (localhost/::1) for a stable race.
    // [ts] const result = request.post(server.CROSS_PROCESS_PREFIX + "/refuse", {
    // [ts] data: Buffer.alloc(20 * 1024 * 1024, 0x78),
    // [ts] headers: { "content-type": "text/plain" },
    // [ts] maxRetries: 0,
    // [ts] }).catch(e => e);
    // [ts] if (result == Error) {
    // [ts] expect(result.message).toMatch("apiRequestContext\\.post|ECONNRESET|EPIPE|ECONNABORTED|socket");
    // [ts] } else {
    // [ts] expect_eq(result.status(), 413);
    // [ts] expect_eq(result.text(), JSON.stringify({ error: "too large" }));
    // [ts] }
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-052 should throw when failOnStatusCode is set to true inside APIRequest context options", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/34204" });
    // [ts] const request = playwright.request.newContext({ failOnStatusCode: true });
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(404, { "Content-Length": 10, "Content-Type": "text/plain" });
    // [ts] res.end("Not found.");
    // [ts] });
    // [ts] const error = request.fetch(server.EMPTY_PAGE).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "404 Not Found") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("FE-053 should not throw when failOnStatusCode is set to false inside APIRequest context options", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/34204" });
    // [ts] const request = playwright.request.newContext({ failOnStatusCode: false });
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(404, { "Content-Length": 10, "Content-Type": "text/plain" });
    // [ts] res.end("Not found.");
    // [ts] });
    // [ts] const response = request.fetch(server.EMPTY_PAGE);
    // [ts] expect_eq(response.status(), 404);
    // [ts] request.dispose();
    // [ts] });
}
