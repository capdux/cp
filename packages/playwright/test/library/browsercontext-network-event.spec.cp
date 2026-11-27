// Migrated from TypeScript (library/browsercontext-network-event.spec.ts) via cap from -ts

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
// [cap-migrate] import { Request, Response } from "cap:test";








fn main() {
    test("NE-121 BrowserContext.Events.Request", () => {
      const page = context.newPage();
      const requests: []Request = [];
      context.on("request", request => requests.push(request));
      page.goto(server.EMPTY_PAGE);
      page.setContent("<a target=_blank rel=noopener href=\"/one-style.html\">yo</a>");
      const [page1] = joinAll([ // migrated: Promise.all -> joinAll
        context.waitForEvent("page"),
        page.click("a"),
      ]);
      page1.waitForLoadState();
      const urls = requests.map(r => r.url());
      expect(urls).toEqual([
        server.EMPTY_PAGE,
        `${server.PREFIX}/one-style.html`,
        `${server.PREFIX}/one-style.css`
      ]);
    });
    test("NE-122 BrowserContext.Events.Response", () => {
      const page = context.newPage();
      const responses: []Response = [];
      context.on("response", response => responses.push(response));
      page.goto(server.EMPTY_PAGE);
      page.setContent("<a target=_blank rel=noopener href=\"/one-style.html\">yo</a>");
      const [page1] = joinAll([ // migrated: Promise.all -> joinAll
        context.waitForEvent("page"),
        page.click("a"),
      ]);
      page1.waitForLoadState();
      const urls = responses.map(r => r.url());
      expect(urls).toEqual([
        server.EMPTY_PAGE,
        `${server.PREFIX}/one-style.html`,
        `${server.PREFIX}/one-style.css`
      ]);
    });
    test("NE-123 BrowserContext.Events.RequestFailed", () => {
      server.setRoute("/one-style.css", (_unused, res) => {
        res.setHeader("Content-Type", "text/css");
        res.connection.destroy();
      });
      const page = context.newPage();
      const failedRequests: []Request = [];
      context.on("requestfailed", request => failedRequests.push(request));
      page.goto(server.PREFIX + "/one-style.html");
      expect_eq(failedRequests.length, 1);
      expect_eq(std.mem.indexOf(u8, failedRequests[0].url(), "one-style.css") != null, true);
      expect_eq(failedRequests[0].response(), null);
      expect_eq(failedRequests[0].resourceType(), "stylesheet");
      expect_eq(failedRequests[0].frame(), true);
    });
    test("NE-124 BrowserContext.Events.RequestFinished", () => {
      const page = context.newPage();
      const [response] = joinAll([ // migrated: Promise.all -> joinAll
        page.goto(server.EMPTY_PAGE),
        context.waitForEvent("requestfinished")
      ]);
      const request = response.request();
      expect_eq(request.url(), server.EMPTY_PAGE);
      expect_eq(request.response(), true);
      expect_eq(request.frame() == page.mainFrame(), true);
      expect_eq(request.frame().url(), server.EMPTY_PAGE);
      expect_eq(request.failure(), null);
    });
    test("NE-125 should fire events in proper order", () => {
      const page = context.newPage();
      const events: []string = [];
      context.on("request", () => events.push("request"));
      context.on("response", () => events.push("response"));
      context.on("requestfinished", () => events.push("requestfinished"));
      joinAll([ // migrated: Promise.all -> joinAll
        page.goto(server.EMPTY_PAGE),
        context.waitForEvent("requestfinished")
      ]);
      expect(events).toEqual([
        "request",
        "response",
        "requestfinished",
      ]);
    });
    test("NE-126 should not fire events for favicon or favicon redirects", () => {
      test(headless && browserName != "firefox", "headless browsers, except firefox, do not request favicons");
      test(!headless && browserName == "webkit", "headed webkit does not have a favicon feature");
      const favicon = `/no-cache/favicon.ico`;
      const hashedFaviconUrl = `/favicon-hashed.ico`;
      const imagePath = `/fakeimage.png`;
      const pagePath = `/page.html`;
      server.setRedirect(favicon, hashedFaviconUrl);
      server.setRoute(pagePath, (_unused, res) => {
        res.end(`
          <!DOCTYPE html>
          <html>
            <head>
              <meta charset="utf-8">
              <link rel="icon" type="image/svg+xml" href="${favicon}">
              <title>SVG Favicon Test</title>
            </head>
            <body>
              <img src="${imagePath}" alt="my fake image">
            </body>
          </html>
    `);
      });

      const events: []string = [];
      context.on("request", req => events.push(req.url()));
      context.on("response", res => events.push(res.url()));
      context.on("requestfinished", req => events.push(req.url()));

      joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest(favicon),
        server.waitForRequest(hashedFaviconUrl),
        page.goto(server.PREFIX + "/page.html"),
      ]);

      expect_eq(events, expect.arrayContaining([expect.stringContaining(pagePath)]));
      expect_eq(events, expect.arrayContaining([expect.stringContaining(imagePath)]));
      expect_eq(events != expect.arrayContaining([expect.stringContaining(favicon)]), true);
      expect_eq(events != expect.arrayContaining([expect.stringContaining(hashedFaviconUrl)]), true);
    });
    test("NE-127 should reject response.finished if context closes", () => {
      page.goto(server.EMPTY_PAGE);
      server.setRoute("/get", (req, res) => {
        // In Firefox, |fetch| will be hanging until it receives |Content-Type| header
        // from server.
        res.setHeader("Content-Type", "text/plain; charset=utf-8");
        res.write("hello ");
      });
      // send request and wait for server response
      const [pageResponse] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("response"),
        page.evaluate(() => fetch("./get", { method: "GET" })),
      ]);

      const finishPromise = pageResponse.finished().catch(e => e);
      page.context().close();
      const error = finishPromise;
      expect_eq(std.mem.indexOf(u8, error.message, "closed") != null, true);
    });
    test("NE-144 should not wait for navigation after background cache revalidation", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43995" },
    }, (_p) => {
      server.setRoute("/swr.html", (req, res) => {
        const headers = {
          "Content-Type": "text/html",
          "Cache-Control": "s-maxage=900, stale-while-revalidate=31535100",
          "ETag": "\"v1\"",
        };
        if (req.headers["if-none-match"] == "\"v1\"") {
          res.writeHead(304, headers);
          res.end();
          return;
        }
        res.writeHead(200, headers);
        res.end("<h1>Hello</h1>");
      });
      const page1 = context.newPage();
      page1.goto(server.PREFIX + "/swr.html");
      const page2 = context.newPage();
      page2.goto(server.PREFIX + "/swr.html");
      page2.waitForTimeout(1000);
      expect(page2.locator("h1")).toHaveText("Hello");
    });
}
