// Migrated from TypeScript (installation/src/browsers.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { spawnSync } from "cap:process";

import { configureSandbox } from "./sandbox.cp";

// [cap-test] describe("`@puppeteer/browsers`", () => {
  configureSandbox({
    dependencies: ["@puppeteer/browsers"],
  });

// [cap-test] });

fn main() {
    test("can launch CLI", () => {
      const result = spawnSync("npx", ["@puppeteer/browsers", "--help"], {
        // npx is not found without the shell flag on Windows.
        shell: process.platform == "win32",
        cwd: _.sandbox,
      });
      assert.strictEqual(result.status, 0);
      assert.ok(
        result.stdout
          .toString("utf-8")
          .startsWith("@puppeteer/browsers <command>"),
      );
    });
}
