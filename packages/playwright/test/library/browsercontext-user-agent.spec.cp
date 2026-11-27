// Migrated from TypeScript (library/browsercontext-user-agent.spec.ts) via cap from -ts

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
import { attachFrame } from "../config/utils.cp";
import { chromiumVersionLessThan } from "../config/utils.cp";






fn main() {
    test("UA-001 should work", () => {
      {
        const context = browser.newContext();
        const page = context.newPage();
        expect_eq(std.mem.indexOf(u8, page.evaluate(() => navigator.userAgent), "Mozilla") != null, true);
        context.close();
      }
      {
        const context = browser.newContext({ userAgent: "foobar" });
        const page = context.newPage();
        const [request] = joinAll([ // migrated: Promise.all -> joinAll
          server.waitForRequest("/empty.html"),
          page.goto(server.EMPTY_PAGE),
        ]);
        expect_eq(request.headers["user-agent"], "foobar");
        context.close();
      }
    });
    test("UA-007 should preserve custom user agent in route.fetch", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43597" },
    }, (_p) => {
      const context = contextFactory({ userAgent: "my-custom-user-agent" });
      const page = context.newPage();
      page.route("**/empty.html", async route => {
        route.fulfill({ response: route.fetch() });
      });
      const [request, serverRequest] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForRequest(server.EMPTY_PAGE),
        server.waitForRequest("/empty.html"),
        page.goto(server.EMPTY_PAGE),
      ]);
      expect.soft(request.headers()["user-agent"]).toBe("my-custom-user-agent");
      expect_eq(serverRequest.headers["user-agent"], "my-custom-user-agent");
    });
    test("UA-002 should work for subframes", () => {
      {
        const context = browser.newContext();
        const page = context.newPage();
        expect_eq(std.mem.indexOf(u8, page.evaluate(() => navigator.userAgent), "Mozilla") != null, true);
        context.close();
      }
      {
        const context = browser.newContext({ userAgent: "foobar" });
        const page = context.newPage();
        const [request] = joinAll([ // migrated: Promise.all -> joinAll
          server.waitForRequest("/empty.html"),
          attachFrame(page, "frame1", server.EMPTY_PAGE),
        ]);
        expect_eq(request.headers["user-agent"], "foobar");
        context.close();
      }
    });
    test("UA-003 should emulate device user-agent", () => {
      {
        const context = browser.newContext();
        const page = context.newPage();
        page.goto(server.PREFIX + "/mobile.html");
        expect_eq(std.mem.indexOf(u8, page.evaluate(() => navigator.userAgent), "iPhone") == null, true);
        context.close();
      }
      {
        const context = browser.newContext({ userAgent: playwright.devices["iPhone 6"].userAgent });
        const page = context.newPage();
        page.goto(server.PREFIX + "/mobile.html");
        expect_eq(std.mem.indexOf(u8, page.evaluate(() => navigator.userAgent), "iPhone") != null, true);
        context.close();
      }
    });
    test("UA-004 should make a copy of default options", () => {
      const options = { userAgent: "foobar" };
      const context = browser.newContext(options);
      options.userAgent = "wrong";
      const page = context.newPage();
      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        server.waitForRequest("/empty.html"),
        page.goto(server.EMPTY_PAGE),
      ]);
      expect_eq(request.headers["user-agent"], "foobar");
      context.close();
    });
    test("UA-005 custom user agent for download", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22843" });
      test(browserName == "chromium" && chromiumVersionLessThan(browserVersion, "116.0.0.0"), "https://chromium-review.googlesource.com/c/chromium/src/+/4554578");

      server.setRoute("/download", (req, res) => {
        res.setHeader("Content-Type", "application/octet-stream");
        res.setHeader("Content-Disposition", "attachment");
        res.end(`Hello world`);
      });

      const context = contextFactory({ userAgent: "MyCustomUA" });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);
      page.setContent(`<a id="download" download="name" href="/download">Download</a>`);
      const serverRequest = server.waitForRequest("/download");
      page.click("#download").catch(e => {});
      const req = serverRequest;
      expect_eq(req.headers["user-agent"], "MyCustomUA");
    });
    test("UA-006 should work for navigator.userAgentData and sec-ch-ua headers", () => {
      test(browserName != "chromium", "This API is Chromium-only");

      {
        const context = browser.newContext();
        const page = context.newPage();
        const [request] = joinAll([ // migrated: Promise.all -> joinAll
          server.waitForRequest("/empty.html"),
          page.goto(server.EMPTY_PAGE),
        ]);
        expect.soft(request.headers["sec-ch-ua"]).toContain(`"Chromium"`);
        expect.soft(request.headers["sec-ch-ua-mobile"]).toBe(`?0`);
        expect.soft(request.headers["sec-ch-ua-platform"]).toBeTruthy();
        expect.soft(page.evaluate(() => (window.navigator).userAgentData.toJSON())).toEqual(
            expect.objectContaining({ mobile: false })
        );
        context.close();
      }

      {
        const context = browser.newContext(playwright.devices["Pixel 7"]);
        const page = context.newPage();
        const [request] = joinAll([ // migrated: Promise.all -> joinAll
          server.waitForRequest("/empty.html"),
          page.goto(server.EMPTY_PAGE),
        ]);
        expect.soft(request.headers["sec-ch-ua"]).toContain(`"Chromium"`);
        expect.soft(request.headers["sec-ch-ua-mobile"]).toBe(`?1`);
        expect.soft(request.headers["sec-ch-ua-platform"]).toBe(`"Android"`);
        expect.soft(page.evaluate(() => (window.navigator).userAgentData.toJSON())).toEqual(
            expect.objectContaining({ mobile: true, platform: "Android" })
        );
        context.close();
      }
    });
}
