// Migrated from TypeScript (library/browsercontext-proxy.spec.ts) via cap from -ts

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

import { browserTest as it, expect } from "../config/browserTest.cp";

it.beforeEach((_p) => {
  server.setRoute("/target.html", (req, res) => {
    res.end("<html><title>Served by the proxy</title></html>");
  });
});


for (const launchProxy of ["per-context", "http://per-context"]) {
  test(`PR-019 should ignore legacy '${launchProxy}' launch proxy`, () => {
    proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
    const browser = browserType.launch({ proxy: { server: launchProxy } });
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      const context = browser.newContext({ proxy: { server: proxyServer.HOST } });
      const page = context.newPage();
      page.goto("http://non-existent.test/target.html");
      expect_eq(page.title(), "Served by the proxy");
      const response = context.request.get("http://non-existent.test/target.html");
      expect_eq(std.mem.indexOf(u8, response.text(), "Served by the proxy") != null, true);
      expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, "non-existent.test:80") != null, true);

      const directContext = browser.newContext();
      const directPage = directContext.newPage();
      directPage.goto(server.PREFIX + "/target.html");
      expect_eq(directPage.title(), "Served by the proxy");
      const directResponse = directContext.request.get(server.PREFIX + "/target.html");
      expect_eq(directResponse.ok(), true);
    } { // TODO(cap-migrate): replace finally with CP defer
      browser.close();
    }
  });
}





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
// [ts] test(`PR-024 ${params.description}`, () => {
// [ts] test(browserName == "webkit" && platform == "darwin" && ["localhost", "127.0.0.1"].includes(params.target) && additionalBypass, "Mac webkit does not proxy localhost when bypass rules are set");
// [ts] const path = `/target-${additionalBypass}-${params.target}.html`;
// [ts] server.setRoute(path, (req, res) => {
// [ts] res.end("<html><title>Served by the proxy</title></html>");
// [ts] });
// [ts] const url = `http://${params.target}:55555${path}`;
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST, bypass: additionalBypass ? "1.non.existent.domain.for.the.test" : null }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(url);
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, url) != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] page.goto("http://1.non.existent.domain.for.the.test/foo.html").catch(() => {});
// [ts] if (additionalBypass)
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://1.non.existent.domain.for.the.test/foo.html") == null, true);
// [ts] else {
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://1.non.existent.domain.for.the.test/foo.html") != null, true);
// [ts] context.close();
// [ts] });
// [ts] }
// [ts] });
// [ts] }
// [ts] });
// [ts] test("PR-025 should use ipv6 proxy", () => {
// [ts] it.fail(browserName == "firefox" && !isBidi, "page.goto: NS_ERROR_UNKNOWN_HOST");
// [ts] it.fail(channel == "webkit-wsl", "WebKit on WSL does not support IPv6: https://github.com/microsoft/WSL/issues/10803");
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] const context = contextFactory({
// [ts] proxy: { server: `[0:0:0:0:0:0:0:1]:${proxyServer.PORT}` }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-026 should use proxy twice", () => {
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
// [ts] page.goto("http://non-existent-2.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent-2.test/target.html") != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-027 should use proxy for second page", () => {
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] const page2 = context.newPage();
// [ts] proxyServer.requestUrls = [];
// [ts] page2.goto("http://non-existent.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
// [ts] expect_eq(page2.title(), "Served by the proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-028 should use proxy for https urls", () => {
// [ts] httpsServer.setRoute("/target.html", (req, res) => {
// [ts] res.end("<html><title>Served by https server via proxy</title></html>");
// [ts] });
// [ts] proxyServer.forwardTo(httpsServer.PORT, { allowConnectRequests: true });
// [ts] const context = contextFactory({
// [ts] ignoreHTTPSErrors: true,
// [ts] proxy: { server: proxyServer.HOST }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("https://non-existent.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, "non-existent.test:443") != null, true);
// [ts] expect_eq(page.title(), "Served by https server via proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-029 should work with IP:PORT notion", () => {
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] const context = contextFactory({
// [ts] proxy: { server: `127.0.0.1:${proxyServer.PORT}` }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-030 should throw for socks5 authentication", () => {
// [ts] const error = contextFactory({
// [ts] proxy: { server: `socks5://localhost:1234`, username: "user", password: "secret" }
// [ts] }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Browser does not support socks5 proxy authentication") != null, true);
// [ts] });
// [ts] test("PR-031 should throw for socks4 authentication", () => {
// [ts] const error = contextFactory({
// [ts] proxy: { server: `socks4://localhost:1234`, username: "user", password: "secret" }
// [ts] }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Socks4 proxy protocol does not support authentication") != null, true);
// [ts] });
// [ts] test("PR-032 should authenticate", () => {
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] let auth;
// [ts] proxyServer.setAuthHandler(req => {
// [ts] auth = req.headers["proxy-authorization"];
// [ts] return !!auth;
// [ts] });
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST, username: "user", password: "secret" }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
// [ts] expect_eq(auth, "Basic " + Buffer.from("user:secret").toString("base64"));
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-033 should authenticate with empty password", () => {
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] let auth;
// [ts] proxyServer.setAuthHandler(req => {
// [ts] auth = req.headers["proxy-authorization"];
// [ts] return !!auth;
// [ts] });
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST, username: "user", password: "" }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(auth, "Basic " + Buffer.from("user:").toString("base64"));
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-034 should isolate proxy credentials between contexts", () => {
// [ts] proxyServer.forwardTo(server.PORT);
// [ts] let auth;
// [ts] proxyServer.setAuthHandler(req => {
// [ts] auth = req.headers["proxy-authorization"];
// [ts] return !!auth;
// [ts] });
// [ts] {
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST, username: "user1", password: "secret1" }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(auth, "Basic " + Buffer.from("user1:secret1").toString("base64"));
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] context.close();
// [ts] }
// [ts] auth = null;
// [ts] {
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST, username: "user2", password: "secret2" }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test/target.html");
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] expect_eq(auth, "Basic " + Buffer.from("user2:secret2").toString("base64"));
// [ts] context.close();
// [ts] }
// [ts] });
// [ts] test("PR-035 should exclude patterns", () => {
// [ts] proxyServer.forwardTo(server.PORT);
// FYI: using long and weird domain names to avoid ATT DNS hijacking
// that resolves everything to some weird search results page.
//
// @see https://gist.github.com/CollinChaffin/24f6c9652efb3d6d5ef2f5502720ef00
// [ts] const context = contextFactory({
// [ts] proxy: { server: proxyServer.HOST, bypass: "1.non.existent.domain.for.the.test, 2.non.existent.domain.for.the.test, .another.test" }
// [ts] });
// [ts] const nonFaviconUrls = () => {
// [ts] return proxyServer.requestUrls.filter(u => !u.includes("favicon"));
// [ts] };
// [ts] {
// [ts] proxyServer.requestUrls = [];
// [ts] const page = context.newPage();
// [ts] page.goto("http://0.non.existent.domain.for.the.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://0.non.existent.domain.for.the.test/target.html") != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] proxyServer.requestUrls = [];
// [ts] const page = context.newPage();
// [ts] const error = page.goto("http://1.non.existent.domain.for.the.test/target.html").catch(e => e);
// [ts] expect_eq(nonFaviconUrls(), []);
// [ts] expect_eq(error.message, true);
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] proxyServer.requestUrls = [];
// [ts] const page = context.newPage();
// [ts] const error = page.goto("http://2.non.existent.domain.for.the.test/target.html").catch(e => e);
// [ts] expect_eq(nonFaviconUrls(), []);
// [ts] expect_eq(error.message, true);
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] proxyServer.requestUrls = [];
// [ts] const page = context.newPage();
// [ts] const error = page.goto("http://foo.is.the.another.test/target.html").catch(e => e);
// [ts] expect_eq(nonFaviconUrls(), []);
// [ts] expect_eq(error.message, true);
// [ts] page.close();
// [ts] }
// [ts] {
// [ts] proxyServer.requestUrls = [];
// [ts] const page = context.newPage();
// [ts] page.goto("http://3.non.existent.domain.for.the.test/target.html");
// [ts] expect_eq(std.mem.indexOf(u8, nonFaviconUrls(), "http://3.non.existent.domain.for.the.test/target.html") != null, true);
// [ts] expect_eq(page.title(), "Served by the proxy");
// [ts] page.close();
// [ts] }
// [ts] context.close();
// [ts] });
// [ts] test("PR-036 should use socks proxy", () => {
// [ts] const context = contextFactory({
// [ts] proxy: { server: `socks5://localhost:${socksPort}` }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test");
// [ts] expect_eq(page.title(), "Served by the SOCKS proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-037 should use socks proxy in second page", () => {
// [ts] const context = contextFactory({
// [ts] proxy: { server: `socks5://localhost:${socksPort}` }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto("http://non-existent.test");
// [ts] expect_eq(page.title(), "Served by the SOCKS proxy");
// [ts] const page2 = context.newPage();
// [ts] page2.goto("http://non-existent.test");
// [ts] expect_eq(page2.title(), "Served by the SOCKS proxy");
// [ts] context.close();
// [ts] });
// [ts] test("PR-038 does launch without a port", () => {
// [ts] const context = contextFactory({
// [ts] proxy: { server: "http://localhost" }
// [ts] });
// [ts] context.close();
// [ts] });
// [ts] test("PR-039 should isolate proxy credentials between contexts on navigation", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31525" });
// [ts] server.setRoute("/target.html", (req, res) => {
// [ts] const authHeader = req.headers["proxy-authorization"];
// [ts] if (!authHeader) {
// [ts] res.writeHead(407, { "Proxy-Authenticate": "Basic realm=\"proxy\"" });
// [ts] res.end("Proxy authorization required");
// [ts] return;
// [ts] }
// [ts] const [username,] = Buffer.from(authHeader.split(" ")[1], "base64").toString().split(":");
// [ts] res.writeHead(200, { "Content-Type": "text/html" });
// [ts] res.end(`Hello <div data-testid=user>${username}</div>!\n`);
// [ts] });
// [ts] const context1 = contextFactory({
// [ts] proxy: { server: server.PREFIX, username: "user1", password: "secret1" }
// [ts] });
// [ts] const page1 = context1.newPage();
// [ts] page1.goto("http://non-existent.test/target.html");
// [ts] expect(page1.getByTestId("user")).toHaveText("user1");
// [ts] const context2 = contextFactory({
// [ts] proxy: { server: server.PREFIX, username: "user2", password: "secret2" }
// [ts] });
// [ts] const page2 = context2.newPage();
// [ts] page2.goto("http://non-existent.test/target.html");
// [ts] expect(page2.getByTestId("user")).toHaveText("user2");
// [ts] page1.goto("http://non-existent.test/target.html");
// [ts] expect(page1.getByTestId("user")).toHaveText("user1");
// [ts] });
// [ts] }

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("PR-018 should work when passing the proxy only on the context level", () => {
    // [ts] proxyServer.forwardTo(server.PORT);
    // [ts] let browser;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] browser = browserType.launch({
    // [ts] proxy: null,
    // [ts] });
    // [ts] const context = browser.newContext({
    // [ts] proxy: { server: proxyServer.HOST }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto("http://non-existent.test/target.html");
    // [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
    // [ts] expect_eq(page.title(), "Served by the proxy");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.close();
    // [ts] }
    // [ts] });
    // [ts] test("PR-020 should throw for bad server value", () => {
    // [ts] const error = contextFactory({
    // @ts-expect-error server must be a string
    // [ts] proxy: { server: 123 }
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "proxy.server: expected string, got number") != null, true);
    // [ts] });
    // [ts] test("PR-021 should use proxy", () => {
    // [ts] proxyServer.forwardTo(server.PORT);
    // [ts] const context = contextFactory({
    // [ts] proxy: { server: proxyServer.HOST }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto("http://non-existent.test/target.html");
    // [ts] expect_eq(std.mem.indexOf(u8, proxyServer.requestUrls, "http://non-existent.test/target.html") != null, true);
    // [ts] expect_eq(page.title(), "Served by the proxy");
    // [ts] context.close();
    // [ts] });
    // [ts] test("PR-022 should send secure cookies to subdomain.localhost", () => {
    // [ts] proxyServer.forwardTo(server.PORT);
    // [ts] const context = contextFactory({
    // [ts] proxy: { server: proxyServer.HOST },
    // [ts] });
    // [ts] server.setRoute("/set-cookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [`non-secure=1; HttpOnly`, `secure=1; HttpOnly; Secure`]);
    // [ts] res.end();
    // [ts] });
    // [ts] server.setRoute("/read-cookie.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", `text/html`);
    // [ts] res.end(`<div>Cookie: ${req.headers.cookie.split(";").map(c => c.trim()).sort().join("; ")}</div>`);
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(`http://subdomain.localhost/set-cookie.html`);
    // [ts] const cookies = context.cookies("http://subdomain.localhost");
    // [ts] expect(cookies.map((_p) => ({ name, domain }))).toEqual([
    // [ts] {
    // [ts] name: "non-secure",
    // [ts] domain: "subdomain.localhost",
    // [ts] },
    // [ts] ...((browserName == "webkit" && (!isWindows || channel == "webkit-wsl")) ? [] : [{
    // [ts] name: "secure",
    // [ts] domain: "subdomain.localhost",
    // [ts] }]),
    // [ts] ]);
    // [ts] page.goto(`http://subdomain.localhost/read-cookie.html`);
    // [ts] expect(page.locator("div")).toHaveText(if (browserName == "webkit") "Cookie: non-secure=1" else "Cookie: non-secure=1; secure=1");
    // [ts] context.close();
    // [ts] });
    // [ts] test("PR-023 should set cookie for top-level domain", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/18362" }
    // [ts] }, (_p) => {
    // [ts] it.fixme(browserName == "webkit" && (isLinux || channel == "webkit-wsl"));
    // [ts] proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
    // [ts] const context = contextFactory({
    // [ts] proxy: { server: proxyServer.HOST }
    // [ts] });
    // [ts] server.setRoute("/empty.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", `name=val; Domain=codes; Path=/;`);
    // [ts] res.end();
    // [ts] });
    // [ts] context.request.get("http://codes/empty.html");
    // [ts] const [cookie] = context.cookies();
    // [ts] expect_eq(cookie, true);
    // [ts] expect_eq(cookie.name, "name");
    // [ts] expect_eq(cookie.value, "val");
    // [ts] context.close();
    // [ts] });
}
