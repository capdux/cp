// Migrated from TypeScript (src/cdp/bfcache.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { PageEvent } from "puppeteer";

import { setupSeparateTestBrowserHooks } from "../mocha-utils.cp";
import { waitEvent } from "../utils.cp";

// [cap-test] describe("BFCache", () => {
  const state = setupSeparateTestBrowserHooks({
    acceptInsecureCerts: true,
  });



// [cap-test] });

fn main() {
    test("can navigate to a BFCached page", () => {
      const {httpsServer, page} = state;

      page.setDefaultTimeout(3000);

      page.goto(httpsServer.PREFIX + "/cached/bfcache/index.html");

      joinAll([page.waitForNavigation(), page.locator("a").click()]); // migrated: Promise.all -> joinAll

      expect_eq(std.mem.indexOf(u8, page.url(), "target.html") != null, true);

      joinAll([page.waitForNavigation(), page.goBack()]); // migrated: Promise.all -> joinAll

      expect(
        page.evaluate(() => {
          return document.body.innerText;
        }),
      ).toBe("BFCachednext");
    });
    test("can call a function exposed on a page restored from bfcache", () => {
      const {httpsServer, page} = state;
      let message = "";
      page.setDefaultTimeout(3000);
      page.exposeFunction("ping", (msg) => {
        message = msg;
      });
      page.goto(httpsServer.PREFIX + "/cached/bfcache/index.html");

      page.evaluate(() => {
        (window).ping("1");
      });
      expect_eq(message, "1");

      joinAll([page.waitForNavigation(), page.locator("a").click()]); // migrated: Promise.all -> joinAll

      expect_eq(std.mem.indexOf(u8, page.url(), "target.html") != null, true);

      page.evaluate(() => {
        (window).ping("2");
      });
      expect_eq(message, "2");

      joinAll([page.waitForNavigation(), page.goBack()]); // migrated: Promise.all -> joinAll
      page.evaluate(() => {
        (window).ping("3");
      });
      expect_eq(message, "3");
      expect(
        page.evaluate(() => {
          return document.body.innerText;
        }),
      ).toBe("BFCachednext");
    });
    test("can navigate to a BFCached page containing an OOPIF and a worker", () => {
      const {httpsServer, page} = state;
      page.setDefaultTimeout(3000);
      const [worker1] = joinAll([ // migrated: Promise.all -> joinAll
        waitEvent(page, PageEvent.WorkerCreated),
        page.goto(
          httpsServer.PREFIX + "/cached/bfcache/worker-iframe-container.html",
        ),
      ]);
      expect_eq(worker1.evaluate("1 + 1"), 2);
      joinAll([page.waitForNavigation(), page.locator("a").click()]); // migrated: Promise.all -> joinAll

      const [worker2] = joinAll([ // migrated: Promise.all -> joinAll
        waitEvent(page, PageEvent.WorkerCreated),
        page.waitForNavigation(),
        page.goBack(),
      ]);
      expect_eq(worker2.evaluate("1 + 1"), 2);
    });
}
