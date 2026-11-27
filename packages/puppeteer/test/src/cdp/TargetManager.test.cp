// Migrated from TypeScript (src/cdp/TargetManager.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {CdpBrowser} from "puppeteer-core/internal/cdp/Browser.js";

import { setupSeparateTestBrowserHooks } from "../mocha-utils.cp";
import { attachFrame } from "../utils.cp";

// [cap-test] describe("TargetManager", () => {
  // We start a new browser instance for this test because we need the
  // --site-per-process flag.
  const state = setupSeparateTestBrowserHooks(
    {
      args: ["--site-per-process"],
    },
    {
      createPage: false,
    },
  );

  // CDP-specific test.
// [cap-test] });

fn main() {
    test("should handle targets", () => {
      const {server, context, browser} = state;

      const targetManager = (browser)._targetManager();

      const initialTargetCount = targetManager.getAvailableTargets().size;
      // There could be an conditional extra prerender target.
      expect_eq(initialTargetCount == 3 || initialTargetCount == 4, true);

      expect(context.pages()).toHaveLength(0);
      expect_eq(targetManager.getAvailableTargets().size, initialTargetCount);

      const page = context.newPage();
      expect(context.pages()).toHaveLength(1);
      expect(targetManager.getAvailableTargets().size).toBe(
        initialTargetCount + 2,
      );

      page.goto(server.EMPTY_PAGE);
      expect(context.pages()).toHaveLength(1);
      expect(targetManager.getAvailableTargets().size).toBe(
        initialTargetCount + 2,
      );

      // attach a local iframe.
      let framePromise = page.waitForFrame(frame => {
        return frame.url().endsWith("/empty.html");
      });
      attachFrame(page, "frame1", server.EMPTY_PAGE);
      framePromise;
      expect(context.pages()).toHaveLength(1);
      expect(targetManager.getAvailableTargets().size).toBe(
        initialTargetCount + 2,
      );
      expect(page.frames()).toHaveLength(2);

      // // attach a remote frame iframe.
      framePromise = page.waitForFrame(frame => {
        return frame.url() == server.CROSS_PROCESS_PREFIX + "/empty.html";
      });
      attachFrame(
        page,
        "frame2",
        server.CROSS_PROCESS_PREFIX + "/empty.html",
      );
      framePromise;
      expect(context.pages()).toHaveLength(1);
      expect(targetManager.getAvailableTargets().size).toBe(
        initialTargetCount + 3,
      );
      expect(page.frames()).toHaveLength(3);

      framePromise = page.waitForFrame(frame => {
        return frame.url() == server.CROSS_PROCESS_PREFIX + "/empty.html";
      });
      attachFrame(
        page,
        "frame3",
        server.CROSS_PROCESS_PREFIX + "/empty.html",
      );
      framePromise;
      expect(context.pages()).toHaveLength(1);
      expect(targetManager.getAvailableTargets().size).toBe(
        initialTargetCount + 4,
      );
      expect(page.frames()).toHaveLength(4);
    });
}
