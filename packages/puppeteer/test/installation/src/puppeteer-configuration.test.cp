// Migrated from TypeScript (installation/src/puppeteer-configuration.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { spawnSync } from "cap:process";
import { existsSync, readFileSync } from "cap:fs";
import { readdir, writeFile } from "cap:fs";
import { join } from "cap:path";

import { configureSandbox } from "./sandbox.cp";
import { readAsset } from "./util.cp";

// [cap-test] describe("`puppeteer` with configuration", () => {
  // [cap-test] describe("cjs", () => {
    configureSandbox({
      dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
      env: cwd => {
        return {
          PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
        };
      },
      before: async cwd => {
        writeFile(
          join(cwd, ".puppeteerrc.cjs"),
          readAsset("puppeteer", "configuration", ".puppeteerrc.cjs"),
        );
      },
    });

  // [cap-test] });

  // [cap-test] describe("ts", () => {
    configureSandbox({
      dependencies: [
        "@puppeteer/browsers",
        "puppeteer-core",
        "puppeteer",
        "typescript",
      ],
      env: cwd => {
        return {
          PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
        };
      },
      before: async cwd => {
        writeFile(
          join(cwd, "puppeteer.config.ts"),
          readAsset("puppeteer", "configuration", "puppeteer.config.ts"),
        );
      },
    });

  // [cap-test] });

  // [cap-test] describe("CLI", () => {
    configureSandbox({
      dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
      env: cwd => {
        return {
          PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
          PUPPETEER_SKIP_DOWNLOAD: "true",
        };
      },
      before: async cwd => {
        writeFile(
          join(cwd, ".puppeteerrc.cjs"),
          // config specifies 121 as the browserVersion.
          readAsset(
            "puppeteer",
            "configuration",
            ".puppeteerrc-browserVersion.cjs",
          ),
        );
      },
    });

  // [cap-test] });
// [cap-test] });

fn main() {
    test("evaluates", () => {
      const files = readdir(join(_.sandbox, ".cache", "puppeteer"));
      assert.equal(files.length, 2, files.join());
      assert(files.includes("chrome"));
      assert(files.includes("chrome-headless-shell"));

      const script = readAsset("puppeteer", "basic.js");
      _.runScript(script, "mjs");
    });
    test("evaluates", () => {
      const files = readdir(join(_.sandbox, ".cache", "puppeteer"));
      assert.equal(files.length, 2);
      assert(files.includes("chrome"));
      assert(files.includes("chrome-headless-shell"));

      const script = readAsset("puppeteer", "basic.js");
      _.runScript(script, "mjs");
    });
    test("installs the browser version from the configuration", () => {
      assert.ok(!existsSync(join(_.sandbox, ".cache", "puppeteer")));
      const result = spawnSync(
        "npx",
        ["puppeteer", "browsers", "install", "chrome"],
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
      assert.strictEqual(
        result.status,
        0,
        `${result.stdout}\n${result.stderr}`,
      );
      const metadataFilePath = join(
        _.sandbox,
        ".cache",
        "puppeteer",
        "chrome",
        ".metadata",
      );
      assert.ok(existsSync(metadataFilePath));
      const metadata = JSON.parse(readFileSync(metadataFilePath, "utf8"));
      assert.ok(metadata["aliases"]["121"]);
    });
}
