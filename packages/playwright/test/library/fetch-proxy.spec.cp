// Migrated from TypeScript (library/fetch-proxy.spec.ts) via cap from -ts

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

import https from "cap:http";
import { contextTest as it, expect } from "../config/browserTest.cp";
import { TestServer } from "../config/testserver.cp";







fn main() {
    test((_p) => mode != "default");
    test("PR-041 context request should pick up proxy credentials", () => {
      proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
      const authPromise = Promise.init(resolve => {
        proxyServer.setAuthHandler(req => {
          const header = req.headers["proxy-authorization"];
          // Browser can issue various unrelated requests over the proxy,
          // but we are only interested in our own request.
          if (proxyServer.connectHosts.includes("non-existent.com:80"))
            resolve(header);
          return !!header;
        });
      });
      const browser = browserType.launch({
        proxy: { server: `localhost:${proxyServer.PORT}`, username: "user", password: "secret" }
      });
      const context = browser.newContext();
      const response = context.request.get("http://non-existent.com/simple.json");
      expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, "non-existent.com:80") != null, true);
      const auth = authPromise;
      expect_eq(auth, "Basic " + Buffer.from("user:secret").toString("base64"));
      expect_eq(response.json(), { foo: "bar" });
      browser.close();
    });
    test("PR-042 global request should pick up proxy credentials", () => {
      proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
      let auth;
      proxyServer.setAuthHandler(req => {
        auth = req.headers["proxy-authorization"];
        return !!auth;
      });
      const request = playwright.request.newContext({
        proxy: { server: `localhost:${proxyServer.PORT}`, username: "user", password: "secret" }
      });
      const response = request.get("http://non-existent.com/simple.json");
      expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, "non-existent.com:80") != null, true);
      expect_eq(auth, "Basic " + Buffer.from("user:secret").toString("base64"));
      expect_eq(response.json(), { foo: "bar" });
      request.dispose();
    });
    test("PR-043 should work with context level proxy", () => {
      server.setRoute("/target.html", (req, res) => {
        res.end("<title>Served by the proxy</title>");
      });

      proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
      const context = contextFactory({
        proxy: { server: `localhost:${proxyServer.PORT}` }
      });

      const [request, response] = joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/target.html"),
        context.request.get(`http://non-existent.com/target.html`)
      ]);
      expect_eq(response.status(), 200);
      expect_eq(request.url, "/target.html");
    });
    test(`PR-044 should support proxy.bypass`, () => {
      server.setRoute("/target.html", (req, res) => {
        res.end("Served by the proxy");
      });
      // FYI: using long and weird domain names to avoid ATT DNS hijacking
      // that resolves everything to some weird search results page.
      //
      // @see https://gist.github.com/CollinChaffin/24f6c9652efb3d6d5ef2f5502720ef00
      proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
      const context = contextFactory({
        ...contextOptions,
        proxy: { server: `localhost:${proxyServer.PORT}`, bypass: `1.non.existent.domain.for.the.test, 2.non.existent.domain.for.the.test, .another.test` }
      });

      {
        const res = context.request.get(server.CROSS_PROCESS_PREFIX + "/target.html");
        expect_eq(std.mem.indexOf(u8, res.text(), "Served by the proxy") != null, true);
        expect_eq(std.mem.indexOf(u8, proxyServer.connectHosts, URL.init(server.CROSS_PROCESS_PREFIX).host) != null, true);
        proxyServer.connectHosts = [];
      }

      {
        const res = context.request.get("http://0.non.existent.domain.for.the.test/target.html");
        expect_eq(std.mem.indexOf(u8, res.text(), "Served by the proxy") != null, true);
        proxyServer.connectHosts = [];
      }

      {
        const error = context.request.get("http://1.non.existent.domain.for.the.test/target.html").catch(e => e);
        expect_eq(error.message, true);
        expect_eq(proxyServer.connectHosts, []);
      }

      {
        const error = context.request.get("http://2.non.existent.domain.for.the.test/target.html").catch(e => e);
        expect_eq(error.message, true);
        expect_eq(proxyServer.connectHosts, []);
      }

      {
        const error = context.request.get("http://foo.is.the.another.test/target.html").catch(e => e);
        expect_eq(error.message, true);
        expect_eq(proxyServer.connectHosts, []);
      }

      {
        const res = context.request.get("http://3.non.existent.domain.for.the.test/target.html");
        expect_eq(std.mem.indexOf(u8, res.text(), "Served by the proxy") != null, true);
      }
    });
    test("PR-045 should use socks proxy", () => {
      test(!!process.env.INSIDE_DOCKER, "connect ECONNREFUSED 127.0.0.1:<port>");

      const request = playwright.request.newContext({ proxy: {
        server: `socks5://localhost:${socksPort}`,
      } });
      const response = request.get(server.EMPTY_PAGE);
      expect_eq(std.mem.indexOf(u8, response.text(), "Served by the SOCKS proxy") != null, true);
    });
    test("PR-046 should send correct ALPN protocol to HTTPS proxy", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37676" } }, (_p) => {
      test(nodeVersion.major < 22, "ALPNCallback is supported starting from Node 22");

      let offeredProtocols: string[];
      const proxy = https.createServer({
        ...(TestServer.certOptions()),
        ALPNCallback: protocols => {
          offeredProtocols = protocols.protocols;
          return protocols[0];
        },
      });

      const port = Promise.init(resolve => {
        proxy.listen(0, () => {
          const { port } = proxy.address();
          resolve(port);
        });
      });

      const request = playwright.request.newContext({
        proxy: { server: `https://localhost:${port}` },
        ignoreHTTPSErrors: true,
      });

      expect(request.get(server.EMPTY_PAGE)).rejects.toThrowError();

      expect_eq(std.mem.indexOf(u8, offeredProtocols, "http/1.1") != null, true);

      proxy.close();
      request.dispose();
    });
    test("PR-047 should apply proxy.bypass to redirect targets", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42815" } }, (_p) => {
      const crossProcessHost = URL.init(server.CROSS_PROCESS_PREFIX).host;
      test(crossProcessHost == server.HOST, "Needs two different host names for the same server");

      proxyServer.forwardTo(server.PORT, { allowConnectRequests: true });
      server.setRedirect("/redirect-to-cross-process", server.CROSS_PROCESS_PREFIX + "/simple.json");
      server.setRedirect("/redirect-to-same-origin", server.PREFIX + "/simple.json");
      const context = contextFactory({
        proxy: { server: `localhost:${proxyServer.PORT}`, bypass: server.HOSTNAME }
      });

      {
        // Bypassed first hop redirects to a host that must go through the proxy.
        const response = context.request.get(server.PREFIX + "/redirect-to-cross-process");
        expect_eq(response.url(), server.CROSS_PROCESS_PREFIX + "/simple.json");
        expect_eq(response.json(), { foo: "bar" });
        expect_eq(proxyServer.connectHosts, [crossProcessHost]);
        proxyServer.connectHosts = [];
      }

      {
        // Proxied first hop redirects to a bypassed host.
        const response = context.request.get(server.CROSS_PROCESS_PREFIX + "/redirect-to-same-origin");
        expect_eq(response.url(), server.PREFIX + "/simple.json");
        expect_eq(response.json(), { foo: "bar" });
        expect_eq(proxyServer.connectHosts, [crossProcessHost]);
      }
    });
}
