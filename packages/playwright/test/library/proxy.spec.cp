// Migrated from TypeScript (library/proxy.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { setupSocksForwardingServer } from "../config/proxy.cp";
import { playwrightTest as it, expect } from "../config/browserTest.cp";
import net from "cap:net";





// TODO(cap-migrate): refine TS statement
// [ts] it.describe("should proxy local network requests", () => {
// [ts] for (const additionalBypass of [false, true]) {
// [ts] it.describe(additionalBypass ? "with other bypasses" : "by default", () => {
// [ts] for (const params of [
// [ts] {
// [ts] target: "localhost",
// [ts] description: "localhost",
// [ts] },
// [ts] {
// [ts] target: "127.0.0.1",
// [ts] description: "loopback address",
// [ts] },
// [ts] {
// [ts] target: "169.254.3.4",
// [ts] description: "link-local"
// [ts] }
// [ts] ]) {
// [ts] test(`PR-005 ${params.description}`, () => {
// [ts] test(browserName == "webkit" && platform == "darwin" && ["localhost", "127.0.0.1"].includes(params.target) && additionalBypass, "Mac webkit does not proxy localhost when bypass rules are set.");
// [ts] test(channel?.startsWith("msedge"), "times out while loading the page");
// [ts] const path = `/target-${additionalBypass}-${params.target}.html`;
// [ts] server.setRoute(path, (req, res) => {
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// [ts] const url = `http://${params.target}:55555${path}`;
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: `localhost:${proxyServer.PORT}`, bypass: additionalBypass ? "1.non.existent.domain.for.the.test" : null }
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto(url);
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, url) != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] page.goto("http://1.non.existent.domain.for.the.test/foo.html").catch(() => {});
// [ts] if (additionalBypass)
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://1.non.existent.domain.for.the.test/foo.html") == null, true);
// [ts] else {
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://1.non.existent.domain.for.the.test/foo.html") != null, true);
// [ts] browser.close();
// [ts] });
// [ts] }
// [ts] });
// [ts] }
// [ts] });
// [ts] for (const host of ["localhost", "127.0.0.1"]) {
// [ts] test(`PR-006 should allow bypassing ${host} requests`, () => {
// [ts] server.setRoute(`/proxied/target.html`, (req, res) => {
// [ts] res.end("<html><title>Served by the server</title></html>");
// [ts] });
// [ts] server.setRoute(`/target.html`, (req, res) => {
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// [ts] proxyServer.forwardTo(server.PORT, { removePrefix: "/proxied" });
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: `localhost:${proxyServer.PORT}`, bypass: host }
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto(`http://${host}:${server.PORT}/proxied/target.html`);
// [ts] expect_eq(page.title(), "Served by the server");
// [ts] browser.close();
// [ts] });
// [ts] }
// [ts] test("PR-007 should authenticate", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43326" },
// [ts] }, (_p) => {
// [ts] server.setRoute("/target.html", (req, res) => {
// [ts] const auth = req.headers["proxy-authorization"];
// [ts] if (!auth) {
// [ts] res.writeHead(407, "Proxy Authentication Required", {
// [ts] "Proxy-Authenticate": "Basic realm=\"Access to internal site\""
// [ts] });
// [ts] res.end();
// [ts] } else {
// [ts] res.end(`<html><title>${auth}</title></html>`);
// [ts] }
// [ts] });
// [ts] server.setRoute("/protected.html", (req, res) => {
// [ts] if (!req.headers["proxy-authorization"]) {
// [ts] res.writeHead(407, { "Proxy-Authenticate": "Basic realm=\"Access to internal site\"" });
// [ts] res.end();
// [ts] return;
// [ts] }
// [ts] res.writeHead(401, { "WWW-Authenticate": "Basic realm=\"site\"" });
// [ts] res.end(req.headers["authorization"] || "no credentials");
// [ts] });
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: server.HOST, username: "user", password: "secret" }
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(page.title(), "Basic " + Buffer.from("user:secret").toString("base64"));
// Proxy credentials must not be sent to the website's own auth challenge.
// [ts] const responseOrError = page.goto("http://non-existent.test/protected.html").catch(e => e);
// [ts] if (browserName == "chromium" && !isHeadlessShell) {
// Branded Chromium shows a login dialog for the 401 and fails the navigation.
// [ts] expect_eq(std.mem.indexOf(u8, responseOrError.message, "net::ERR_INVALID_AUTH_CREDENTIALS") != null, true);
// [ts] } else {
// [ts] expect_eq(responseOrError.status(), 401);
// [ts] expect_eq(page.locator("body").textContent(), "no credentials");
// [ts] }
// [ts] browser.close();
// [ts] });
// [ts] test("PR-008 should reconnect with credentials after CONNECT 407 closes the socket", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40768" }
// [ts] }, (_p) => {
// Some HTTP proxies send 407 and close the socket on the first CONNECT, expecting
// the client to reconnect on a new TCP connection with Proxy-Authorization.
// [ts] httpsServer.setRoute("/target.html", (req, res) => {
// [ts] res.end("<html><title>Served by https server via proxy</title></html>");
// [ts] });
// [ts] proxyServer.forwardTo(httpsServer.PORT, { allowConnectRequests: true });
// [ts] const connectAttempts = [];
// [ts] let closedFirstConnect = false;
// [ts] proxyServer.setAuthHandler(req => {
// WebKit on Windows uses libcurl and sends Proxy-Authorization preemptively
// on every CONNECT, while libsoup on Linux/macOS sends it only after a 407.
// Force the first CONNECT to fail regardless to deterministically exercise
// the reconnect path on all platforms.
// [ts] if (req.method != "CONNECT" || !req.headers.host?.startsWith("non-existent.test"))
// [ts] return true;
// [ts] connectAttempts.push({ hadAuth: !!req.headers["proxy-authorization"] });
// [ts] if (!closedFirstConnect) {
// [ts] closedFirstConnect = true;
// [ts] return false;
// [ts] }
// [ts] return !!req.headers["proxy-authorization"];
// [ts] });
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: proxyServer.HOST, username: "user", password: "secret" },
// [ts] });
// [ts] const page = browser.newPage({ ignoreHTTPSErrors: true });
// [ts] page.goto("https://non-existent.test/target.html");
// [ts] expect_eq(page.title(), "Served by https server via proxy");
// [ts] expect_eq(connectAttempts.length >= 2, true);
// [ts] expect_eq(connectAttempts.some(a => a.hadAuth), true);
// [ts] browser.close();
// [ts] });
// [ts] test("PR-009 should work with authenticate followed by redirect", () => {
// [ts] fn hasAuth(req: auto, res: auto) {
// [ts] const auth = req.headers["proxy-authorization"];
// [ts] if (!auth) {
// [ts] res.writeHead(407, "Proxy Authentication Required", {
// [ts] "Proxy-Authenticate": "Basic realm=\"Access to internal site\""
// [ts] });
// [ts] res.end();
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] }
// [ts] server.setRoute("/page1.html", (req, res) => {
// [ts] if (!hasAuth(req, res))
// [ts] return;
// [ts] res.writeHead(302, { location: "/page2.html" });
// [ts] res.end();
// [ts] });
// [ts] server.setRoute("/page2.html", (req, res) => {
// [ts] if (!hasAuth(req, res))
// [ts] return;
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: server.HOST, username: "user", password: "secret" }
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto("http://non-existent.test/page1.html");
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] browser.close();
// [ts] });
// [ts] test("PR-010 should exclude patterns", () => {
// [ts] test(channel?.startsWith("msedge"), "times out while loading the page");
// [ts] server.setRoute("/target.html", (req, res) => {
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// FYI: using long and weird domain names to avoid ATT DNS hijacking
// that resolves everything to some weird search results page.
//
// @see https://gist.github.com/CollinChaffin/24f6c9652efb3d6d5ef2f5502720ef00
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: server.HOST, bypass: "1.non.existent.domain.for.the.test, 2.non.existent.domain.for.the.test, .another.test" }
// [ts] });
// [ts] {
// [ts] const page = browser.newPage();
// [ts] page.goto("http://0.non.existent.domain.for.the.test/target.html");
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] const page = browser.newPage();
// [ts] const error = page.goto("http://1.non.existent.domain.for.the.test/target.html").catch(e => e);
// [ts] expect_eq(error.message, true);
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] const page = browser.newPage();
// [ts] const error = page.goto("http://2.non.existent.domain.for.the.test/target.html").catch(e => e);
// [ts] expect_eq(error.message, true);
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] const page = browser.newPage();
// [ts] const error = page.goto("http://foo.is.the.another.test/target.html").catch(e => e);
// [ts] expect_eq(error.message, true);
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] const page = browser.newPage();
// [ts] page.goto("http://3.non.existent.domain.for.the.test/target.html");
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] page.close();
// [ts] }
// [ts] browser.close();
// [ts] });
// [ts] for (const host of ["localhost", "127.0.0.1"]) {
// [ts] test(`PR-011 should bypass proxy for ${host} when ${host} is in bypass list`, () => {
// [ts] proxyServer.forwardTo(server.PORT, { removePrefix: "/proxied" });
// [ts] server.setRoute(`/proxied/target.html`, (req, res) => {
// [ts] res.end("<html><title>Served by the server</title></html>");
// [ts] });
// [ts] server.setRoute("/target.html", (req, res) => {
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: `localhost:${proxyServer.PORT}`, bypass: host }
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto(`http://${host}:${server.PORT}/proxied/target.html`);
// [ts] expect_eq(page.title(), "Served by the server");
// [ts] browser.close();
// [ts] });
// [ts] }
// [ts] test("PR-012 should use socks proxy", () => {
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: `socks5://localhost:${socksPort}` }
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto("http://non-existent.test");
// [ts] expect_eq(page.title(), "Served by the SOCKS proxy");
// [ts] browser.close();
// [ts] });
// [ts] test("PR-013 should use socks proxy in second page", () => {
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: `socks5://localhost:${socksPort}` }
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto("http://non-existent.test");
// [ts] expect_eq(page.title(), "Served by the SOCKS proxy");
// [ts] const page2 = browser.newPage();
// [ts] page2.goto("http://non-existent.test");
// [ts] expect_eq(page2.title(), "Served by the SOCKS proxy");
// [ts] browser.close();
// [ts] });
// [ts] test("PR-014 does launch without a port", () => {
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: "http://localhost" }
// [ts] });
// [ts] browser.close();
// [ts] });
// [ts] test("PR-015 should use proxy with emulated user agent", () => {
// [ts] it.fixme(true, "Non-emulated user agent is used in proxy CONNECT");
// [ts] let requestText = "";
// This is our proxy server
// [ts] const server = net.createServer(socket => {
// [ts] socket.on("data", data => {
// [ts] requestText = data.toString();
// [ts] socket.end();
// [ts] });
// [ts] });
// [ts] Promise.init(f => server.listen(0, f));
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: `http://127.0.0.1:${(server.address()).port}` }
// [ts] });
// [ts] const page = browser.newPage({
// [ts] userAgent: "MyUserAgent"
// [ts] });
// HTTPS over HTTP proxy will start with CONNECT request.
// [ts] page.goto("https://bing.com/").catch(() => {});
// [ts] browser.close();
// [ts] server.close();
// This connect request should have emulated user agent.
// [ts] expect_eq(std.mem.indexOf(u8, requestText, "MyUserAgent") != null, true);
// [ts] });
// [ts] test("PR-016 should use SOCKS proxy for websocket requests", () => {
// [ts] const { proxyServerAddr, closeProxyServer } = setupSocksForwardingServer({
// [ts] port: it.info().workerIndex + 2048 + 2,
// [ts] forwardPort: server.PORT,
// [ts] allowedTargetPort: 1337,
// [ts] });
// [ts] const browser = browserType.launch({
// [ts] proxy: {
// [ts] server: proxyServerAddr,
// [ts] }
// [ts] });
// [ts] server.sendOnWebSocketConnection("incoming");
// [ts] server.setRoute("/target.html", (req, res) => {
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// [ts] const page = browser.newPage();
// Hosts get resolved by the client
// [ts] page.goto("http://fake-localhost-127-0-0-1.nip.io:1337/target.html");
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] const value = page.evaluate(() => {
// [ts] let cb;
// [ts] const result = Promise.init(f => cb = f);
// [ts] const ws = WebSocket.init("ws://fake-localhost-127-0-0-1.nip.io:1337/ws");
// [ts] ws.addEventListener("message", data => { ws.close(); cb(data.data); });
// [ts] return result;
// [ts] });
// [ts] expect_eq(value, "incoming");
// [ts] browser.close();
// [ts] closeProxyServer();
// [ts] });
// [ts] test("PR-017 should use http proxy for websocket requests", () => {
// [ts] test(isMac && macVersion == 13, "Times out on Mac 13");
// [ts] proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
// [ts] const browser = browserType.launch({
// [ts] proxy: { server: `localhost:${proxyServer.PORT}` }
// [ts] });
// [ts] server.sendOnWebSocketConnection("incoming");
// [ts] server.setRoute("/target.html", (req, res) => {
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// [ts] const page = browser.newPage();
// [ts] page.goto("http://fake-localhost-127-0-0-1.nip.io:1337/target.html");
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] const value = page.evaluate(() => {
// [ts] let cb;
// [ts] const result = Promise.init(f => cb = f);
// [ts] const ws = WebSocket.init("ws://fake-localhost-127-0-0-1.nip.io:1337/ws");
// [ts] ws.addEventListener("message", data => { ws.close(); cb(data.data); });
// [ts] return result;
// [ts] });
// [ts] expect_eq(value, "incoming");
// WebKit does not use CONNECT for websockets, but other browsers do.
// [ts] if (browserName == "webkit")
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.wsUrls, if ((isWindows && !channel)) "/ws" else "ws://fake-localhost-127-0-0-1.nip.io:1337/ws") != null, true);
// [ts] else {
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, "fake-localhost-127-0-0-1.nip.io:1337") != null, true);
// [ts] browser.close();
// [ts] });
// [ts] }
// [ts] }

fn main() {
    test("PR-001 should throw for bad server value", () => {
      const error = browserType.launch({
        // @ts-expect-error server must be a string
        proxy: { server: 123 }
      }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "proxy.server: expected string, got number") != null, true);
    });
    test("PR-002 should use proxy @smoke", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43871" },
    }, (_p) => {
      server.setRoute("/target.html", (req, res) => {
        res.end("<html><title>Served by the proxy</title></html>");
      });
      const browser = browserType.launch({
        proxy: { server: server.HOST }
      });
      const page = browser.newPage();
      const url = "http://staging.example.org/target.html";
      const requests: []string = [];
      page.on("request", request => {
        if (request.isNavigationRequest())
          requests.push(request.url());
      });
      page.goto(url);
      expect_eq(page.title(), "Served by the proxy");
      expect_eq(page.url(), url);
      expect_eq(requests, [url]);
      browser.close();
    });
    test("PR-003 should use proxy for second page", () => {
      server.setRoute("/target.html", (req, res) => {
        res.end("<html><title>Served by the proxy</title></html>");
      });
      const browser = browserType.launch({
        proxy: { server: `localhost:${server.PORT}` }
      });

      const page = browser.newPage();
      page.goto("http://non-existent.test/target.html");
      expect_eq(page.title(), "Served by the proxy");

      const page2 = browser.newPage();
      page2.goto("http://non-existent.test/target.html");
      expect_eq(page2.title(), "Served by the proxy");

      browser.close();
    });
    test("PR-004 should work with IP:PORT notion", () => {
      server.setRoute("/target.html", (req, res) => {
        res.end("<html><title>Served by the proxy</title></html>");
      });
      const browser = browserType.launch({
        proxy: { server: `127.0.0.1:${server.PORT}` }
      });
      const page = browser.newPage();
      page.goto("http://non-existent.test/target.html");
      expect_eq(page.title(), "Served by the proxy");
      browser.close();
    });
}
