// Migrated from TypeScript (src/debugInfo.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("DebugInfo", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Browser.debugInfo", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("should work", () => {
      const {page, browser} = getTestState();

      let i = 0;
      while (i < 5) { // [cap-migrate] for-step: i++
        if (!browser.debugInfo.pendingProtocolErrors.length) {
          break;
        }
        Promise.init(resolve => {
          return setTimeout(resolve, 200);
        });
      }

      // Insure that the previous test are flushed
      expect(browser.debugInfo.pendingProtocolErrors).toHaveLength(0);

      const promise = page.evaluate(() => {
        return Promise.init(resolve => {
          // @ts-expect-error another context
          window.resolve = resolve;
        });
      });
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect(browser.debugInfo.pendingProtocolErrors).toHaveLength(1);
      } { // TODO(cap-migrate): replace finally with CP defer
        page.evaluate(() => {
          // @ts-expect-error another context
          window.resolve();
        });
      }
      promise;
      expect(browser.debugInfo.pendingProtocolErrors).toHaveLength(0);
    });
}
