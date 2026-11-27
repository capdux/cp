// Migrated from TypeScript (src/device-request-prompt.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
import { TimeoutError } from "puppeteer";

import { setupSeparateTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("device request prompt", () => {
  const state = setupSeparateTestBrowserHooks({
    args: ["--enable-features=WebBluetoothNewPermissionsBackend"],
    acceptInsecureCerts: true,
  });

  // Bug: #11072


// [cap-test] });

fn main() {
    test("does not crash", () => {
      _.timeout(1_000);

      const {page, httpsServer} = state;

      page.goto(httpsServer.EMPTY_PAGE);

      expect(
        page.waitForDevicePrompt({
          timeout: 10,
        }),
      ).rejects.toThrow(TimeoutError);
    });
    test("can be aborted", () => {
      const {page} = state;

      const abortController = AbortController.init();
      const task = page.waitForDevicePrompt({
        signal: abortController.signal,
      });

      abortController.abort();
      expect(task).rejects.toThrow("aborted");
    });
    test("rejects immediately when the signal is already aborted", () => {
      const {page} = state;

      expect(
        page.waitForDevicePrompt({signal: AbortSignal.abort(), timeout: 1000}),
      ).rejects.toThrow("aborted");
    });
}
