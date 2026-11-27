// Migrated from TypeScript (library/client-certificates.spec.ts) via cap from -ts

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
import tls from "tls";
// [cap-migrate] import https from "cap:http";
import zlib from "cap:zlib";
// [cap-migrate] import http2 from "http2";
// [cap-migrate] import http from "cap:http";
import { expect, playwrightTest as base } from "../config/browserTest.cp";
// [cap-migrate] import net from "cap:net";
// [cap-migrate] import { BrowserContextOptions } from "../../packages/playwright-test.cp";
import { setupSocksForwardingServer } from "../config/proxy.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
const { createHttpsServer, createHttp2Server } = utils;

type TestOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] const test = base.extend({
// [ts] startCCServer: (_p, use) => {
// [ts] process.env.PWTEST_UNSUPPORTED_CUSTOM_CA = asset("client-certificates/server/server_cert.pem");
// [ts] let server: http.Server | http2.Http2SecureServer | null;
// [ts] use(async options => {
// [ts] server = (if (options?.http2) createHttp2Server else createHttpsServer)({
// [ts] key: fs.readFileSync(asset("client-certificates/server/server_key.pem")),
// [ts] cert: fs.readFileSync(asset("client-certificates/server/server_cert.pem")),
// [ts] ca: [
// [ts] fs.readFileSync(asset("client-certificates/server/server_cert.pem")),
// [ts] ],
// [ts] requestCert: true,
// [ts] rejectUnauthorized: options?.rejectUnauthorized ?? false,
// [ts] allowHTTP1: options?.enableHTTP1FallbackWhenUsingHttp2,
// [ts] }, (req, res) => {
// [ts] const tlsSocket = req.socket;
// [ts] const parts = [];
// [ts] parts.push({ key: "alpn-protocol", value: tlsSocket.alpnProtocol });
// [ts] parts.push({ key: "servername", value: tlsSocket.servername });
// [ts] const cert = tlsSocket.getPeerCertificate();
// [ts] if (tlsSocket.authorized) {
// [ts] res.writeHead(200, { "Content-Type": "text/html" });
// [ts] parts.push({ key: "message", value: `Hello ${cert.subject.CN}, your certificate was issued by ${cert.issuer.CN}!` });
// [ts] } else if (cert.subject) {
// [ts] res.writeHead(403, { "Content-Type": "text/html" });
// [ts] parts.push({ key: "message", value: `Sorry ${cert.subject.CN}, certificates from ${cert.issuer.CN} are not welcome here.` });
// [ts] } else {
// [ts] res.writeHead(401, { "Content-Type": "text/html" });
// [ts] parts.push({ key: "message", value: `Sorry, but you need to provide a client certificate to continue.` });
// [ts] }
// [ts] res.end(parts.map((_p) => `<div data-testid="${key}">${value}</div>`).join(""));
// [ts] });
// [ts] Promise.init(f => server.listen(0, "127.0.0.1", () => f()));
// [ts] const host = if (options?.useFakeLocalhost) "local.playwright" else "127.0.0.1";
// [ts] return `https://${host}:${(server.address()).port}/`;
// [ts] });
// [ts] if (server)
// [ts] Promise.init(resolve => server.close(() => resolve()));
// [ts] },
// [ts] });

const kDummyFileName = __filename;
const kValidationSubTests: [][BrowserContextOptions, string] = [
  [{ clientCertificates: [{ origin: "test" }] }, "None of cert, key, passphrase or pfx is specified"],
  [{
    clientCertificates: [{
      origin: "test",
      certPath: kDummyFileName,
      keyPath: kDummyFileName,
      pfxPath: kDummyFileName,
      passphrase: kDummyFileName,
    }]
  }, "pfx is specified together with cert, key or passphrase"],
];

// [cap-test] test.describe("fetch", () => {











// [cap-test] });


// [cap-test] test.describe("browser", () => {














            // TODO(cap-migrate): refine TS statement
            // [ts] }

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CC-001 validate input", () => {
    // [ts] for (const [contextOptions, expected] of kValidationSubTests)
    // [ts] expect(playwright.request.newContext(contextOptions)).rejects.toThrow(expected);
    // [ts] });
    // [ts] test("CC-002 should fail with no client certificates provided", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const request = playwright.request.newContext({ ignoreHTTPSErrors: true });
    // [ts] const response = request.get(serverURL);
    // [ts] expect_eq(response.status(), 401);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Sorry, but you need to provide a client certificate to continue.") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-003 should keep supporting http", () => {
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(server.PREFIX).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const response = request.get(server.PREFIX + "/one-style.html");
    // [ts] expect_eq(response.url(), server.PREFIX + "/one-style.html");
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "<div>hello, world!</div>") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-004 should throw with untrusted client certs", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/self-signed/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/self-signed/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const response = request.get(serverURL);
    // [ts] expect_eq(response.url(), serverURL);
    // [ts] expect_eq(response.status(), 403);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Sorry Bob, certificates from Bob are not welcome here.") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-005 pass with trusted client certificates", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const response = request.get(serverURL);
    // [ts] expect_eq(response.url(), serverURL);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Hello Alice, your certificate was issued by localhost!") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-006 should not leak client certificate to cross-origin redirect target", () => {
    // [ts] const targetURL = startCCServer();
    // Standalone HTTPS server (not cert-required) that 302-redirects to the cert-required server.
    // [ts] const redirectServer = createHttpsServer({
    // [ts] key: fs.readFileSync(asset("client-certificates/server/server_key.pem")),
    // [ts] cert: fs.readFileSync(asset("client-certificates/server/server_cert.pem")),
    // [ts] }, (req, res) => {
    // [ts] res.writeHead(302, { Location: targetURL });
    // [ts] res.end();
    // [ts] });
    // [ts] Promise.init(f => redirectServer.listen(0, "127.0.0.1", () => f()));
    // [ts] const redirectAddr = redirectServer.address();
    // Use 'localhost' for the redirect origin so it differs from the target's '127.0.0.1' origin
    // even though both resolve to the loopback interface and share the same trusted CA.
    // [ts] const redirectURL = `https://localhost:${redirectAddr.port}/redir`;
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // Cert is scoped to the redirect *origin*, not the target — it must not follow the redirect.
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(redirectURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const response = request.get(redirectURL);
    // [ts] expect_eq(response.url(), targetURL);
    // [ts] expect_eq(response.status(), 401);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "you need to provide a client certificate") != null, true);
    // [ts] request.dispose();
    // [ts] Promise.init(f => redirectServer.close(() => f()));
    // [ts] });
    // [ts] test("CC-007 pass with trusted client certificates in pfx format", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] pfxPath: asset("client-certificates/client/trusted/cert.pfx"),
    // [ts] passphrase: "secure"
    // [ts] }],
    // [ts] });
    // [ts] const response = request.get(serverURL);
    // [ts] expect_eq(response.url(), serverURL);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Hello Alice, your certificate was issued by localhost!") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-008 pass with trusted client certificates and when a http proxy is used", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] proxyServer.forwardTo(parseInt(URL.init(serverURL).port, 10), { allowConnectRequests: true });
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] proxy: { server: `localhost:${proxyServer.PORT}` }
    // [ts] });
    // [ts] expect_eq(proxyServer.connectHosts, []);
    // [ts] const response = request.get(serverURL);
    // [ts] expect_eq(proxyServer.connectHosts, [URL.init(serverURL).host]);
    // [ts] expect_eq(response.url(), serverURL);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Hello Alice, your certificate was issued by localhost!") != null, true);
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-009 pass with trusted client certificates and when a socks proxy is used", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const serverPort = parseInt(URL.init(serverURL).port, 10);
    // [ts] const { proxyServerAddr, closeProxyServer, connectHosts } = setupSocksForwardingServer({
    // [ts] port: test.info().workerIndex + 2048 + 2,
    // [ts] forwardPort: serverPort,
    // [ts] allowedTargetPort: serverPort,
    // [ts] });
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] proxy: { server: proxyServerAddr }
    // [ts] });
    // [ts] expect_eq(connectHosts, []);
    // [ts] const response = request.get(serverURL);
    // [ts] expect_eq(connectHosts, [URL.init(serverURL).host]);
    // [ts] expect_eq(response.url(), serverURL);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Hello Alice, your certificate was issued by localhost!") != null, true);
    // [ts] request.dispose();
    // [ts] closeProxyServer();
    // [ts] });
    // [ts] test("CC-010 should throw a http error if the pfx passphrase is incorect", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] pfxPath: asset("client-certificates/client/trusted/cert.pfx"),
    // [ts] passphrase: "this-password-is-incorrect"
    // [ts] }],
    // [ts] });
    // [ts] expect(request.get(serverURL)).rejects.toThrow("mac verify failure");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-011 should fail with matching certificates in legacy pfx format", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] pfxPath: asset("client-certificates/client/trusted/cert-legacy.pfx"),
    // [ts] passphrase: "secure"
    // [ts] }],
    // [ts] });
    // [ts] expect(request.get(serverURL)).rejects.toThrow("Unsupported TLS certificate");
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-012 should work in the browser with request interception", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const request = playwright.request.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const page = browser.newPage({ ignoreHTTPSErrors: true });
    // [ts] page.route("**/*", async route => {
    // [ts] const response = request.fetch(route.request());
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] request.dispose();
    // [ts] });
    // [ts] test("CC-013 validate input", () => {
    // [ts] for (const [contextOptions, expected] of kValidationSubTests)
    // [ts] expect(browser.newContext(contextOptions)).rejects.toThrow(expected);
    // [ts] });
    // [ts] test("CC-014 should keep supporting http", () => {
    // [ts] const page = browser.newPage({
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(server.PREFIX).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] expect(page.getByText("hello, world!")).toBeVisible();
    // [ts] expect(page.locator("body")).toHaveCSS("background-color", "rgb(255, 192, 203)");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-015 should pass through to non-matching origin with self-signed cert", () => {
    // [ts] httpsServer.setRoute("/hello.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end("<html><body><div data-testid=\"message\">hello</div></body></html>");
    // [ts] });
    // [ts] const page = browser.newPage({
    // [ts] clientCertificates: [{
    // [ts] origin: "https://not-matching.com",
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(httpsServer.PREFIX + "/hello.html");
    // [ts] expect(page.getByTestId("message")).toHaveText("hello");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-016 should not intercept TLS for origins without a client certificate", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41106" },
    // [ts] }, (_p) => {
    // If the proxy intercepted this origin, the browser would see its self-signed cert (CN=localhost)
    // instead of the real server cert (CN=playwright-test).
    // [ts] const page = browser.newPage({
    // [ts] clientCertificates: [{
    // [ts] origin: "https://not-matching.com",
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const response = page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(response.ok(), true);
    // [ts] const securityDetails = response.securityDetails();
    // [ts] expect_eq(std.mem.indexOf(u8, securityDetails.subjectName, "playwright-test") != null, true);
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-017 should fail with no client certificates", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: "https://not-matching.com",
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Sorry, but you need to provide a client certificate to continue.");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-018 should fail with self-signed client certificates", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/self-signed/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/self-signed/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Sorry Bob, certificates from Bob are not welcome here.");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-019 should complete browser TLS handshake when server rejects client certificate after it", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43092" },
    // [ts] }, (_p) => {
    // [ts] const serverURL = startCCServer({ rejectUnauthorized: true, useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/self-signed/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/self-signed/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const result = page.goto(serverURL).catch(e => e);
    // [ts] if (result == Error) {
    // The server closed the connection with FIN after the TLS handshake.
    // [ts] expect(result.message).toMatch("net::ERR_EMPTY_RESPONSE|NS_ERROR_NET_RESET|Connection terminated unexpectedly|The network connection was lost|Failure when receiving data from the peer");
    // [ts] } else {
    // The server reset the connection, which the proxy reports as a client certificate error.
    // [ts] expect_eq(result.status(), 503);
    // [ts] expect(page.getByText("Playwright client-certificate error")).toBeVisible();
    // [ts] }
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-020 should pass with matching certificates", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-021 should pass with matching certificates when passing as content", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] cert: fs.promises.readFile(asset("client-certificates/client/trusted/cert.pem")),
    // [ts] key: fs.promises.readFile(asset("client-certificates/client/trusted/key.pem")),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-022 should pass with matching certificates and when a http proxy is used", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] proxyServer.forwardTo(parseInt(URL.init(serverURL).port, 10), { allowConnectRequests: true });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] proxy: { server: `localhost:${proxyServer.PORT}` }
    // [ts] });
    // [ts] expect_eq(proxyServer.connectHosts, []);
    // [ts] page.goto(serverURL);
    // [ts] const host = if (browserName == "webkit" && isMac) "localhost" else "127.0.0.1";
    // [ts] expect_eq([...Set.init(proxyServer.connectHosts)], [`${host}:${URL.init(serverURL).port}`]);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-023 should pass with matching certificates and when a http proxy is used from env", () => {
    // [ts] test(mode != "default", "Out of process transport does not allow us to set env vars dynamically");
    // [ts] process.env.HTTPS_PROXY = `http://localhost:${proxyServer.PORT}`;
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] proxyServer.forwardTo(parseInt(URL.init(serverURL).port, 10), { allowConnectRequests: true });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] expect_eq(proxyServer.connectHosts, []);
    // [ts] page.goto(serverURL);
    // [ts] const host = if (browserName == "webkit" && isMac) "localhost" else "127.0.0.1";
    // [ts] expect_eq([...Set.init(proxyServer.connectHosts)], [`${host}:${URL.init(serverURL).port}`]);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] delete process.env.HTTPS_PROXY;
    // [ts] });
    // [ts] test("CC-024 should pass with matching certificates and when a http proxy is used from config but env is there", () => {
    // [ts] process.env.HTTPS_PROXY = `http://this-should-not-taken-into-account:4242`;
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] proxyServer.forwardTo(parseInt(URL.init(serverURL).port, 10), { allowConnectRequests: true });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] proxy: { server: `localhost:${proxyServer.PORT}` }
    // [ts] });
    // [ts] expect_eq(proxyServer.connectHosts, []);
    // [ts] page.goto(serverURL);
    // [ts] const host = if (browserName == "webkit" && isMac) "localhost" else "127.0.0.1";
    // [ts] expect_eq([...Set.init(proxyServer.connectHosts)], [`${host}:${URL.init(serverURL).port}`]);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] delete process.env.HTTPS_PROXY;
    // [ts] });
    // [ts] test("CC-025 should respect launch proxy and proxy bypass", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42806" } }, (_p) => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const { origin, hostname, port } = URL.init(serverURL);
    // [ts] proxyServer.forwardTo(parseInt(port, 10), { allowConnectRequests: true });
    // [ts] const proxy = { server: `localhost:${proxyServer.PORT}` };
    // [ts] const serverHost = `${if (browserName == "webkit" && isMac) "localhost" else "127.0.0.1"}:${port}`;
    // [ts] const clientCertificates = [{
    // [ts] origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }];
    // [ts] const browser = browserType.launch({ proxy });
    // [ts] {
    // [ts] const page = browser.newPage({ ignoreHTTPSErrors: true, clientCertificates });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // Headed Chromium also sends its own background requests through the launch proxy.
    // [ts] expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, serverHost) != null, true);
    // [ts] page.close();
    // [ts] }
    // [ts] proxyServer.connectHosts = [];
    // [ts] {
    // [ts] const page = browser.newPage({ ignoreHTTPSErrors: true, clientCertificates, proxy: { ...proxy, bypass: hostname } });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, serverHost) == null, true);
    // [ts] page.close();
    // [ts] }
    // [ts] browser.close();
    // [ts] });
    // [ts] test("CC-026 should pass with matching certificates and when a socks proxy is used", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const serverPort = parseInt(URL.init(serverURL).port, 10);
    // [ts] const { proxyServerAddr, closeProxyServer, connectHosts } = setupSocksForwardingServer({
    // [ts] port: test.info().workerIndex + 2048 + 2,
    // [ts] forwardPort: serverPort,
    // [ts] allowedTargetPort: serverPort,
    // [ts] });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] proxy: { server: proxyServerAddr }
    // [ts] });
    // [ts] expect_eq(connectHosts, []);
    // [ts] page.goto(serverURL);
    // [ts] const host = if (browserName == "webkit" && isMac) "localhost" else "127.0.0.1";
    // [ts] expect_eq(connectHosts, [`${host}:${serverPort}`]);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] closeProxyServer();
    // [ts] });
    // [ts] test("CC-027 should not hang on tls errors during TLS 1.2 handshake", () => {
    // [ts] for (const tlsVersion of ["TLSv1.3", "TLSv1.2"]) { test.step(`TLS version: ${tlsVersion}`, () => { const server = tls.createServer({ key: fs.readFileSync(asset("client-certificates/server/server_key.pem")), cert: fs.readFileSync(asset("client-certificates/server/server_cert.pem")), ca: [ fs.readFileSync(asset("client-certificates/server/server_cert.pem")), ], requestCert: true, rejectUnauthorized: true, minVersion: tlsVersion, maxVersion: tlsVersion, SNICallback: (servername, cb) => { // Always reject the connection by passing an error cb(new Error('Connection rejected'), null); } }, () => { // Do nothing }); const serverURL = await new Promise<string>(resolve => { server.listen(0, 'localhost', () => { const host = browserName === 'webkit' && platform === 'darwin' ? 'local.playwright' : 'localhost'; resolve(`https://${host}:${(server.address() as net.AddressInfo).port}/`); }); }); const page = await browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/self-signed/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/self-signed/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByText("Playwright client-certificate error: Client network socket disconnected before secure TLS connection was established")).toBeVisible();
    // [ts] page.close();
    // [ts] Promise.init(resolve => server.close(() => resolve()));
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("CC-028 should pass with matching certificates in pfx format", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] pfxPath: asset("client-certificates/client/trusted/cert.pfx"),
    // [ts] passphrase: "secure"
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-029 should handle TLS renegotiation with client certificates", () => {
    // [ts] const server: https.Server = createHttpsServer({
    // [ts] key: fs.readFileSync(asset("client-certificates/server/server_key.pem")),
    // [ts] cert: fs.readFileSync(asset("client-certificates/server/server_cert.pem")),
    // [ts] ca: [fs.readFileSync(asset("client-certificates/server/server_cert.pem"))],
    // [ts] requestCert: false, // Initially don't request client cert
    // [ts] rejectUnauthorized: false,
    // TLSv1.3 does not support renegotiation
    // [ts] minVersion: "TLSv1.2",
    // [ts] maxVersion: "TLSv1.2",
    // [ts] });
    // [ts] server.on("request", (req, res) => {
    // [ts] if (!req.socket)
    // [ts] return;
    // [ts] const renegotiate = () => Promise.init((resolve, reject) => {
    // [ts] (req.socket).renegotiate({ requestCert: true, rejectUnauthorized: false }, err => { if (err) reject(err); else resolve(); });
    // [ts] });
    // [ts] if (req.url == "/") {
    // [ts] res.writeHead(200, { "Content-Type": "text/html", "connection": "close" });
    // [ts] res.end();
    // [ts] } else if (req.url == "/from-fetch-api") {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/plain",
    // [ts] "Transfer-Encoding": "chunked",
    // A renegotiated connection must not be reused: a second renegotiation on
    // the same socket trips Node's tls.CLIENT_RENEG_LIMIT and destroys it.
    // [ts] "Connection": "close"
    // [ts] });
    // [ts] res.flushHeaders();
    // [ts] Promise.init(resolve => req.once("data", data => {
    // [ts] res.write(`server received: ${data.toString()}\n`);
    // [ts] resolve();
    // [ts] }));
    // [ts] renegotiate();
    // [ts] let i = 0;
    // [ts] while (i < 4) { // [cap-migrate] for-step: i++
    // [ts] res.write(`${i}-from-server\n`);
    // Best-effort to trigger a new chunk
    // [ts] Promise.init(resolve => setTimeout(resolve, 100));
    // [ts] }
    // [ts] res.end("server closed the connection");
    // [ts] } else if (req.url == "/style.css") {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/css",
    // [ts] "Content-Encoding": "gzip",
    // [ts] "Transfer-Encoding": "chunked",
    // [ts] "Connection": "close"
    // [ts] });
    // [ts] res.flushHeaders();
    // [ts] renegotiate();
    // [ts] const stylesheet = `
    // [ts] button {
    // [ts] background-color: red;
    // [ts] }
    // [ts] `;
    // [ts] const stylesheetBuffer = Promise.init((resolve, reject) => {
    // [ts] zlib.gzip(stylesheet, (err, buffer) => {
    // [ts] if (err)
    // [ts] reject(err);
    // [ts] else {
    // [ts] resolve(buffer);
    // [ts] });
    // [ts] });
    // [ts] let i = 0;
    // [ts] while (i < stylesheetBuffer.length) { // [cap-migrate] for-step: i += 100
    // [ts] res.write(stylesheetBuffer.slice(i, i + 100));
    // Best-effort to trigger a new chunk
    // [ts] Promise.init(resolve => setTimeout(resolve, 20));
    // [ts] }
    // [ts] res.end();
    // [ts] } else {
    // [ts] res.writeHead(404);
    // [ts] res.end();
    // [ts] }
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(0, "localhost", resolve));
    // [ts] const port = (server.address()).port;
    // [ts] const host = if (browserName == "webkit" && platform == "darwin") "local.playwright" else "localhost";
    // [ts] const serverUrl = `https://${host}:${port}`;
    // [ts] const context = browser.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: serverUrl,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] test.step("fetch API", () => {
    // [ts] page.goto(serverUrl);
    // [ts] const response = page.evaluate(() => {
    // [ts] const response = fetch("/from-fetch-api", {
    // [ts] method: "POST",
    // [ts] body: "client-request-payload"
    // [ts] });
    // [ts] return response.text();
    // [ts] });
    // [ts] expect(response).toBe([
    // [ts] "server received: client-request-payload",
    // [ts] "0-from-server",
    // [ts] "1-from-server",
    // [ts] "2-from-server",
    // [ts] "3-from-server",
    // [ts] "server closed the connection"
    // [ts] ].join("\n"));
    // [ts] });
    // [ts] test.step("Gzip encoded CSS Stylesheet", () => {
    // [ts] page.goto(serverUrl);
    // The <link> would throw with net::ERR_INVALID_CHUNKED_ENCODING
    // [ts] page.setContent(`
    // [ts] <button>Click me</button>
    // [ts] <link rel="stylesheet" href="/style.css">
    // [ts] `);
    // [ts] expect(page.locator("button")).toHaveCSS("background-color", /* red */"rgb(255, 0, 0)");
    // [ts] });
    // [ts] context.close();
    // [ts] Promise.init(resolve => server.close(() => resolve()));
    // [ts] });
    // [ts] test("CC-030 should pass with matching certificates in pfx format when passing as content", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] pfx: fs.promises.readFile(asset("client-certificates/client/trusted/cert.pfx")),
    // [ts] passphrase: "secure"
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-031 should fail with matching certificates in legacy pfx format", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] expect(browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] pfxPath: asset("client-certificates/client/trusted/cert-legacy.pfx"),
    // [ts] passphrase: "secure"
    // [ts] }],
    // [ts] })).rejects.toThrow("Unsupported TLS certificate");
    // [ts] });
    // [ts] test("CC-032 should throw a http error if the pfx passphrase is incorect", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] expect(browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] pfxPath: asset("client-certificates/client/trusted/cert.pfx"),
    // [ts] passphrase: "this-password-is-incorrect"
    // [ts] }],
    // [ts] })).rejects.toThrow("Failed to load client certificate: mac verify failure");
    // [ts] });
    // [ts] test("CC-033 should pass with matching certificates on context APIRequestContext instance", () => {
    // [ts] const serverURL = startCCServer();
    // [ts] const baseOptions = {
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] };
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] ...baseOptions,
    // [ts] }, {
    // [ts] origin: URL.init(serverURL).origin.replace("localhost", "127.0.0.1"),
    // [ts] ...baseOptions,
    // [ts] }],
    // [ts] });
    // [ts] for (const url of [serverURL, serverURL.replace("localhost", "127.0.0.1")]) {
    // [ts] const response = page.request.get(url);
    // [ts] expect_eq(response.status(), 200);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Hello Alice, your certificate was issued by localhost!") != null, true);
    // [ts] }
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-034 should pass with matching certificates and trailing slash", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: serverURL,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByText("Hello Alice, your certificate was issued by localhost!")).toBeVisible();
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-035 should have ignoreHTTPSErrors=false by default", () => {
    // [ts] const targetURL = if (browserName == "webkit" && platform == "darwin") httpsServer.EMPTY_PAGE.replace("localhost", "local.playwright") else httpsServer.EMPTY_PAGE;
    // [ts] const page = browser.newPage({
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(targetURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(targetURL);
    // [ts] expect(page.getByText("Playwright client-certificate error: self-signed certificate")).toBeVisible();
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-036 support http2", () => {
    // [ts] test(browserName == "webkit" && isMac, "WebKit on macOS doesn\n proxy localhost");
    // [ts] const serverURL = startCCServer({ http2: true });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] {
    // [ts] page.goto(serverURL.replace("127.0.0.1", "local.playwright"));
    // [ts] expect(page.getByTestId("message")).toHaveText("Sorry, but you need to provide a client certificate to continue.");
    // [ts] expect(page.getByTestId("alpn-protocol")).toHaveText("h2");
    // [ts] expect(page.getByTestId("servername")).toHaveText("local.playwright");
    // [ts] }
    // [ts] {
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] expect(page.getByTestId("alpn-protocol")).toHaveText("h2");
    // [ts] }
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-037 support http2 if the browser only supports http1.1", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const serverURL = startCCServer({ http2: true, enableHTTP1FallbackWhenUsingHttp2: true });
    // [ts] const browser = browserType.launch({ args: ["--disable-http2"] });
    // [ts] const page = browser.newPage({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] {
    // [ts] page.goto(serverURL.replace("127.0.0.1", "local.playwright"));
    // [ts] expect(page.getByTestId("message")).toHaveText("Sorry, but you need to provide a client certificate to continue.");
    // [ts] expect(page.getByTestId("alpn-protocol")).toHaveText("http/1.1");
    // [ts] }
    // [ts] {
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] expect(page.getByTestId("alpn-protocol")).toHaveText("http/1.1");
    // [ts] }
    // [ts] browser.close();
    // [ts] });
    // [ts] test("CC-038 should return target connection errors when using http2", () => {
    // [ts] test(browserName == "webkit" && isMac, "WebKit on macOS does not proxy localhost");
    // [ts] test(nodeVersion.major < 20, "http2.performServerHandshake is not supported in older Node.js versions");
    // [ts] const serverURL = startCCServer({ http2: true });
    // [ts] const page = browser.newPage({
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByText("Playwright client-certificate error: self-signed certificate")).toBeVisible();
    // [ts] page.close();
    // [ts] });
    // [ts] test("CC-039 should handle rejected certificate in handshake with HTTP/2", () => {
    // [ts] const server: http2.Http2SecureServer = createHttp2Server({
    // [ts] key: fs.readFileSync(asset("client-certificates/server/server_key.pem")),
    // [ts] cert: fs.readFileSync(asset("client-certificates/server/server_cert.pem")),
    // [ts] ca: [fs.readFileSync(asset("client-certificates/server/server_cert.pem"))],
    // [ts] requestCert: true,
    // [ts] }, (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end("Hello world");
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(0, "localhost", resolve));
    // [ts] const port = (server.address()).port;
    // [ts] const serverUrl = "https://" + (if (browserName == "webkit" && platform == "darwin") "local.playwright" else "localhost") + ":" + port;
    // [ts] const context = browser.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: "https://just-there-that-the-client-certificates-proxy-server-is-getting-launched.com",
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] const page = context.newPage();
    // This was triggering an unhandled error before.
    // [ts] page.goto(serverUrl).catch(() => {});
    // [ts] context.close();
    // [ts] Promise.init(resolve => server.close(() => resolve()));
    // [ts] });
    // [cap-test] test.describe("persistentContext", () => {
    // [ts] test("CC-040 validate input", () => {
    // [ts] test.slow();
    // [ts] for (const [contextOptions, expected] of kValidationSubTests)
    // [ts] expect(launchPersistent(contextOptions)).rejects.toThrow(expected);
    // [ts] });
    // [ts] test("CC-041 should pass with matching certificates", () => {
    // [ts] const serverURL = startCCServer({ useFakeLocalhost: browserName == "webkit" && isMac });
    // [ts] const { page } = launchPersistent({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] clientCertificates: [{
    // [ts] origin: URL.init(serverURL).origin,
    // [ts] certPath: asset("client-certificates/client/trusted/cert.pem"),
    // [ts] keyPath: asset("client-certificates/client/trusted/key.pem"),
    // [ts] }],
    // [ts] });
    // [ts] page.goto(serverURL);
    // [ts] expect(page.getByTestId("message")).toHaveText("Hello Alice, your certificate was issued by localhost!");
    // [ts] });
    // [cap-test] });
    // [ts] });
    // [ts] } // [cap-migrate] auto-closed brace
}
