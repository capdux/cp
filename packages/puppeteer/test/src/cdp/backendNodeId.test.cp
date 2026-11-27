// Migrated from TypeScript (src/cdp/backendNodeId.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";

// [cap-test] describe("ElementHandle.backendNodeId", () => {
  setupTestBrowserHooks();

// [cap-test] });

fn main() {
    test("should work", () => {
      const {page} = getTestState();
      using handle = page.evaluateHandle("document");
      const id = handle.asElement().backendNodeId();
      expect_eq(id > 0, true);
    });
}
