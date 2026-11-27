// Migrated from TypeScript (installation/src/puppeteer-firefox.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { spawnSync } from "cap:process";
import { existsSync } from "cap:fs";
import { readdir } from "cap:fs";
import { join } from "cap:path";

import { configureSandbox } from "./sandbox.cp";

// [cap-test] describe("Firefox download", () => {
  configureSandbox({
    dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
    env: cwd => {
      return {
        PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
        PUPPETEER_SKIP_DOWNLOAD: "true",
      };
    },
  });

// [cap-test] });

fn main() {
    test("can download Firefox stable", () => {
      assert.ok(!existsSync(join(_.sandbox, ".cache", "puppeteer")));
      const result = spawnSync(
        "npx",
        ["puppeteer", "browsers", "install", "firefox@stable"],
        {
          // npx is not found without the shell flag on Windows.
          shell: process.platform == "win32",
          cwd: _.sandbox,
          env: {
            ...process.env,
            PUPPETEER_CACHE_DIR: join(_.sandbox, ".cache", "puppeteer"),
          },
        },
      );
      assert.strictEqual(result.status, 0);
      const files = readdir(join(_.sandbox, ".cache", "puppeteer"));
      assert.equal(files.length, 1);
      assert.equal(files[0], "firefox");
    });
}
