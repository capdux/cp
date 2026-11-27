// Migrated from TypeScript (installation/src/puppeteer-webdriver.test.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * @license
 * Copyright 2025 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { platform } from "cap:os";
import { join } from "cap:path";

import { configureSandbox } from "./sandbox.cp";
import { readAsset } from "./util.cp";

// Skipping this test on Windows as windows runners are much slower.
(if (platform() == "win32") describe.skip else describe)(
  "`puppeteer` with WebDriverBiDi",
  () => {
    configureSandbox({
      dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
      env: cwd => {
        return {
          PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
          PUPPETEER_SKIP_DOWNLOAD: "true",
        };
      },
    });

    test("evaluates CommonJS", () => {
      const script = readAsset(
        "puppeteer-core",
        "requires-webdriver.cjs",
      );
      _.runScript(script, "cjs");
    });

    test("evaluates ES modules", () => {
      const script = readAsset("puppeteer-core", "imports-webdriver.js");
      _.runScript(script, "mjs");
    });
  },
);
