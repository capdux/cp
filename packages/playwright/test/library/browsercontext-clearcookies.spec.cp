// Migrated from TypeScript (library/browsercontext-clearcookies.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";









fn main() {
    test("CO-042 should clear cookies", () => {
      page.goto(server.EMPTY_PAGE);
      context.addCookies([{
        url: server.EMPTY_PAGE,
        name: "cookie1",
        value: "1"
      }]);
      expect_eq(page.evaluate("document.cookie"), "cookie1=1");
      context.clearCookies();
      expect_eq(context.cookies(), []);
      page.reload();
      expect_eq(page.evaluate("document.cookie"), "");
    });
    test("CO-043 should isolate cookies when clearing", () => {
      const anotherContext = browser.newContext();
      context.addCookies([{ url: server.EMPTY_PAGE, name: "page1cookie", value: "page1value" }]);
      anotherContext.addCookies([{ url: server.EMPTY_PAGE, name: "page2cookie", value: "page2value" }]);

      expect_eq((context.cookies()).length, 1);
      expect_eq((anotherContext.cookies()).length, 1);

      context.clearCookies();
      expect_eq((context.cookies()).length, 0);
      expect_eq((anotherContext.cookies()).length, 1);

      anotherContext.clearCookies();
      expect_eq((context.cookies()).length, 0);
      expect_eq((anotherContext.cookies()).length, 0);
      anotherContext.close();
    });
    test("CO-044 should remove cookies by name", () => {
      context.addCookies([{
        name: "cookie1",
        value: "1",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      },
      {
        name: "cookie2",
        value: "2",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      }
      ]);
      page.goto(server.PREFIX);
      expect_eq(page.evaluate("document.cookie"), "cookie1=1; cookie2=2");
      context.clearCookies({ name: "cookie1" });
      expect_eq(page.evaluate("document.cookie"), "cookie2=2");
    });
    test("CO-045 should remove cookies by name regex", () => {
      context.addCookies([{
        name: "cookie1",
        value: "1",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      },
      {
        name: "cookie2",
        value: "2",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      }
      ]);
      page.goto(server.PREFIX);
      expect_eq(page.evaluate("document.cookie"), "cookie1=1; cookie2=2");
      context.clearCookies({ name: "coo.*1" });
      expect_eq(page.evaluate("document.cookie"), "cookie2=2");
    });
    test("CO-046 should remove cookies by domain", () => {
      context.addCookies([{
        name: "cookie1",
        value: "1",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      },
      {
        name: "cookie2",
        value: "2",
        domain: URL.init(server.CROSS_PROCESS_PREFIX).hostname,
        path: "/",
      }
      ]);
      page.goto(server.PREFIX);
      expect_eq(page.evaluate("document.cookie"), "cookie1=1");
      page.goto(server.CROSS_PROCESS_PREFIX);
      expect_eq(page.evaluate("document.cookie"), "cookie2=2");
      context.clearCookies({ domain: URL.init(server.CROSS_PROCESS_PREFIX).hostname });
      expect_eq(page.evaluate("document.cookie"), "");
      page.goto(server.PREFIX);
      expect_eq(page.evaluate("document.cookie"), "cookie1=1");
    });
    test("CO-047 should remove cookies by path", () => {
      context.addCookies([{
        name: "cookie1",
        value: "1",
        domain: URL.init(server.PREFIX).hostname,
        path: "/api/v1",
      },
      {
        name: "cookie2",
        value: "2",
        domain: URL.init(server.PREFIX).hostname,
        path: "/api/v2",
      },
      {
        name: "cookie3",
        value: "3",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      }
      ]);
      page.goto(server.PREFIX + "/api/v1");
      expect_eq(page.evaluate("document.cookie"), "cookie1=1; cookie3=3");
      context.clearCookies({ path: "/api/v1" });
      expect_eq(page.evaluate("document.cookie"), "cookie3=3");
      page.goto(server.PREFIX + "/api/v2");
      expect_eq(page.evaluate("document.cookie"), "cookie2=2; cookie3=3");
      page.goto(server.PREFIX + "/");
      expect_eq(page.evaluate("document.cookie"), "cookie3=3");
    });
    test("CO-048 should remove cookies by name and domain", () => {
      context.addCookies([{
        name: "cookie1",
        value: "1",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      },
      {
        name: "cookie1",
        value: "1",
        domain: URL.init(server.CROSS_PROCESS_PREFIX).hostname,
        path: "/",
      }
      ]);
      page.goto(server.PREFIX);
      expect_eq(page.evaluate("document.cookie"), "cookie1=1");
      context.clearCookies({ name: "cookie1", domain: URL.init(server.PREFIX).hostname });
      expect_eq(page.evaluate("document.cookie"), "");
      page.goto(server.CROSS_PROCESS_PREFIX);
      expect_eq(page.evaluate("document.cookie"), "cookie1=1");
    });
    test("CO-049 should not transiently delete non-matching cookies when filtering", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40953" },
    }, (_p) => {
      test(browserName == "webkit" && isWindows, "cookieStore change events not supported on WebKit/Windows (curl backend lacks cookie change notifications)");
      test(isFrozenWebkit);

      context.addCookies([{
        name: "keep_me",
        value: "1",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      },
      {
        name: "delete_me",
        value: "2",
        domain: URL.init(server.PREFIX).hostname,
        path: "/",
      }
      ]);
      page.goto(server.PREFIX);

      const eventsHandle = page.evaluateHandle(() => {
        const events: []string = [];
        cookieStore.addEventListener("change", event => {
          for (const changed of event.changed)
            events.push(`changed ${changed.name}`);
          for (const deleted of event.deleted)
            events.push(`deleted ${deleted.name}`);
        });
        return events;
      });

      context.clearCookies({ name: "delete_me" });
      expect.poll(() => eventsHandle.jsonValue()).toEqual(["deleted delete_me"]);
      expect_eq(page.evaluate("document.cookie"), "keep_me=1");
    });
    test("CO-050 should remove partitioned cookies by name", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40953" },
    }, (_p) => {
      test(browserName != "chromium", "Partitioned cookies (CHIPS) are Chromium-specific");
      const context = browser.newContext({ ignoreHTTPSErrors: true });
      context.addCookies([
        {
          name: "delete_me",
          value: "1",
          domain: httpsServer.HOSTNAME,
          path: "/",
          secure: true,
          sameSite: "None",
          partitionKey: `https://${httpsServer.HOSTNAME}`,
        },
        {
          name: "keep_me",
          value: "2",
          domain: httpsServer.HOSTNAME,
          path: "/",
          secure: true,
          sameSite: "None",
          partitionKey: `https://${httpsServer.HOSTNAME}`,
        },
      ]);
      const before = context.cookies();
      expect(before).toEqual([
        expect.objectContaining({ name: "delete_me", partitionKey: `https://${httpsServer.HOSTNAME}` }),
        expect.objectContaining({ name: "keep_me", partitionKey: `https://${httpsServer.HOSTNAME}` }),
      ]);

      context.clearCookies({ name: "delete_me" });
      expect(context.cookies()).toEqual([
        expect.objectContaining({ name: "keep_me", partitionKey: `https://${httpsServer.HOSTNAME}` }),
      ]);
      context.close();
    });
    test("CO-051 should remove nameless cookies by empty name", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43112" },
    }, (_p) => {
      context.addCookies([
        { name: "", value: "nameless", domain: "example.com", path: "/" },
        { name: "session", value: "secret", domain: "example.com", path: "/" },
      ]);
      context.clearCookies({ name: "" });
      expect_eq((context.cookies()).map(cookie => cookie.name), ["session"]);
    });
}
