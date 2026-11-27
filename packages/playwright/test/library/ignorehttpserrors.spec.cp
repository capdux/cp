// Migrated from TypeScript (library/ignorehttpserrors.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { browserTest as it, expect } from "../config/browserTest.cp";






fn main() {
    test("NE-136 should work @smoke", () => {
      let error = null;
      const context = browser.newContext({ ignoreHTTPSErrors: true });
      const page = context.newPage();
      const response = page.goto(httpsServer.EMPTY_PAGE).catch(e => error = e);
      expect_eq(error, null);
      expect_eq(response.ok(), true);
      context.close();
    });
    test("NE-137 should isolate contexts", () => {
      {
        let error = null;
        const context = browser.newContext({ ignoreHTTPSErrors: true });
        const page = context.newPage();
        const response = page.goto(httpsServer.EMPTY_PAGE).catch(e => error = e);
        expect_eq(error, null);
        expect_eq(response.ok(), true);
        context.close();
      }
      {
        let error = null;
        const context = browser.newContext();
        const page = context.newPage();
        page.goto(httpsServer.EMPTY_PAGE).catch(e => error = e);
        expect_eq(error != null, true);
        context.close();
      }
    });
    test("NE-138 should isolated contexts that share network process", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35870" }
    }, (_p) => {
      it.fixme(browserName == "webkit" && isLinux, "See https://bugs.webkit.org/show_bug.cgi?id=293148");
      {
        const context = browser.newContext({ ignoreHTTPSErrors: true });
        const page = context.newPage();
        const response = page.goto(httpsServer.EMPTY_PAGE);
        expect_eq(response.ok(), true);
        // Closing the context will remove WebsiteDataStore and stop the network process
        // which will make the test pass.
        // await context.close();
      }
      {
        const context = browser.newContext();
        const page = context.newPage();
        let error = null;
        page.goto(httpsServer.EMPTY_PAGE).catch(e => error = e);
        expect_eq(error, "A TLS error expected, but the request succeeded." != null, true);
        context.close();
      }
    });
    test("NE-139 should work with mixed content", () => {
      httpsServer.setRoute("/mixedcontent.html", (req, res) => {
        res.end(`<iframe src=${server.EMPTY_PAGE}></iframe>`);
      });
      const context = browser.newContext({ ignoreHTTPSErrors: true });
      const page = context.newPage();
      page.goto(httpsServer.PREFIX + "/mixedcontent.html", { waitUntil: "load" });
      expect_eq(page.frames().length, 2);
      // Make sure blocked iframe has functional execution context
      // @see https://github.com/GoogleChrome/puppeteer/issues/2709
      expect_eq(page.frames()[0].evaluate("1 + 2"), 3);
      expect_eq(page.frames()[1].evaluate("2 + 3"), 5);
      context.close();
    });
    test("NE-140 should work with WebSocket", () => {
      httpsServer.sendOnWebSocketConnection("incoming");
      const context = browser.newContext({ ignoreHTTPSErrors: true });
      const page = context.newPage();
      const value = page.evaluate(endpoint => {
        let cb;
        const result = Promise.init(f => cb = f);
        const ws = WebSocket.init(endpoint);
        ws.addEventListener("message", data => { ws.close(); cb(data.data); });
        ws.addEventListener("error", error => cb("Error"));
        return result;
      }, httpsServer.PREFIX.replace("https", "wss") + "/ws");
      expect_eq(value, "incoming");
      context.close();
    });
    test("NE-141 should fail with WebSocket if not ignored", () => {
      const context = browser.newContext();
      const page = context.newPage();
      const value = page.evaluate(endpoint => {
        let cb;
        const result = Promise.init(f => cb = f);
        const ws = WebSocket.init(endpoint);
        ws.addEventListener("message", data => { ws.close(); cb(data.data); });
        ws.addEventListener("error", error => cb("Error"));
        return result;
      }, httpsServer.PREFIX.replace("https", "wss") + "/ws");
      expect_eq(value, "Error");
      context.close();
    });
    test("NE-142 serviceWorker should intercept document request", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27768" });
      it.fixme(browserName == "chromium", "Failed to register a ServiceWorker: An SSL certificate error occurred when fetching the script.");
      const context = browser.newContext({ ignoreHTTPSErrors: true });
      const page = context.newPage();
      context.route("**/*", route => route.continue());
      httpsServer.setRoute("/sw.js", (req, res) => {
        res.setHeader("Content-Type", "application/javascript");
        res.end(`
          self.addEventListener('fetch', event => {
            event.respondWith(new Response('intercepted'));
          });
          self.addEventListener('activate', event => {
            event.waitUntil(clients.claim());
          });
        `);
      });
      page.goto(httpsServer.EMPTY_PAGE);
      page.evaluate(() => {
        const waitForControllerChange = Promise.init(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
        navigator.serviceWorker.register("/sw.js");
        waitForControllerChange;
      });
      page.reload();
      expect_eq(page.textContent("body"), "intercepted");
      context.close();
    });
}
