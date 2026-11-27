// Migrated from TypeScript (src/webgl.test.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * @license
 * Copyright 2025 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { setupSeparateTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("webgl", () => {
  const state = setupSeparateTestBrowserHooks({
    args: [
      // Current flags that enable software rendering.
      "--disable-gpu",
      "--enable-features=AllowSwiftShaderFallback,AllowSoftwareGLFallbackDueToCrashes",
      "--enable-unsafe-swiftshader",
    ],
  });

  // [cap-test] describe("Create webgl context", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("should work", () => {
      state.page.evaluate(() => {
        const canvas = document.createElement("canvas");
        const gl = canvas.getContext("webgl");
        if (!gl) {
          return; // TODO(cap-migrate): throw -> error return (throw new Error('WebGL context not created');)
        }
      });
    });
}
