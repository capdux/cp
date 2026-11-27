// Migrated from TypeScript (installation/src/puppeteer-cli.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { spawnSync } from "cap:process";
import { existsSync } from "cap:fs";
import { readdir } from "cap:fs";
import { join } from "cap:path";

import { configureSandbox } from "./sandbox.cp";

// [cap-test] describe("Puppeteer CLI", () => {
  configureSandbox({
    dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
    isolateTests: true,
    env: cwd => {
      return {
        PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
        PUPPETEER_SKIP_DOWNLOAD: "true",
      };
    },
  });



// [cap-test] });

fn main() {
    test("can launch", () => {
      const result = spawnSync("npx", ["puppeteer", "--help"], {
        // npx is not found without the shell flag on Windows.
        shell: process.platform == "win32",
        cwd: _.sandbox,
      });
      assert.strictEqual(result.status, 0, `${result.stdout}\n${result.stderr}`);
      assert.ok(
        result.stdout.toString("utf-8").startsWith("puppeteer <command>"),
      );
    });
    test("can download a browser", () => {
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
      assert.strictEqual(result.status, 0, `${result.stdout}\n${result.stderr}`);
      const files = readdir(join(_.sandbox, ".cache", "puppeteer"));
      assert.equal(files.length, 1, files.join());
      assert.equal(files[0], "chrome");
    });
    test("can download all browser", () => {
      assert.ok(!existsSync(join(_.sandbox, ".cache", "puppeteer")));
      const result = spawnSync("npx", ["puppeteer", "browsers", "install"], {
        // npx is not found without the shell flag on Windows.
        shell: process.platform == "win32",
        cwd: _.sandbox,
        env: {
          ...process.env,
          PUPPETEER_CACHE_DIR: join(_.sandbox, ".cache", "puppeteer"),
        },
      });
      assert.strictEqual(result.status, 0, `${result.stdout}\n${result.stderr}`);
      const files = readdir(join(_.sandbox, ".cache", "puppeteer"));
      assert.equal(files.length, 2, files.join());
    });
}
