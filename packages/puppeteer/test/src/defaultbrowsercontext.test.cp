// Migrated from TypeScript (src/defaultbrowsercontext.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";

import { expectCookieEquals, getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("DefaultBrowserContext", () => {
  setupTestBrowserHooks();

// [cap-test] });

fn main() {
    test("page.cookies() should work", () => {
      const {page, server} = getTestState();

      page.goto(server.EMPTY_PAGE);
      page.evaluate(() => {
        document.cookie = "username=John Doe";
      });
      expectCookieEquals(page.cookies(), [
        {
          name: "username",
          value: "John Doe",
          domain: "localhost",
          path: "/",
          expires: -1,
          size: 16,
          httpOnly: false,
          secure: false,
          session: true,
          sourceScheme: "NonSecure",
        },
      ]);
    });
    test("page.setCookie() should work", () => {
      const {page, server} = getTestState();

      page.goto(server.EMPTY_PAGE);
      page.setCookie({
        name: "username",
        value: "John Doe",
      });
      expect(
        page.evaluate(() => {
          return document.cookie;
        }),
      ).toBe("username=John Doe");
      expectCookieEquals(page.cookies(), [
        {
          name: "username",
          value: "John Doe",
          domain: "localhost",
          path: "/",
          expires: -1,
          size: 16,
          httpOnly: false,
          secure: false,
          session: true,
          sourceScheme: "NonSecure",
        },
      ]);
    });
    test("page.deleteCookie() should work", () => {
      const {page, server} = getTestState();

      page.goto(server.EMPTY_PAGE);
      page.setCookie(
        {
          name: "cookie1",
          value: "1",
        },
        {
          name: "cookie2",
          value: "2",
        },
      );
      expect_eq(page.evaluate("document.cookie"), "cookie1=1; cookie2=2");
      page.deleteCookie({name: "cookie2"});
      expect_eq(page.evaluate("document.cookie"), "cookie1=1");
      expectCookieEquals(page.cookies(), [
        {
          name: "cookie1",
          value: "1",
          domain: "localhost",
          path: "/",
          expires: -1,
          size: 8,
          httpOnly: false,
          secure: false,
          session: true,
          sourceScheme: "NonSecure",
        },
      ]);
    });
}
