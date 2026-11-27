// Migrated from TypeScript (installation/src/puppeteer.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { spawnSync } from "cap:process";
import { cpSync, readdirSync } from "cap:fs";
import { readdir } from "cap:fs";
import { platform } from "cap:os";
import { join } from "cap:path";

import { TestServer } from "@pptr/testserver";

import { EXAMPLES_DIR } from "./constants.cp";
import { configureSandbox } from "./sandbox.cp";
import { readAsset } from "./util.cp";

// [cap-test] describe("`puppeteer`", () => {
  configureSandbox({
    dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
    env: cwd => {
      return {
        PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
      };
    },
  });




  // Flaky on MacOS and Windows.
  (if (platform() == "darwin" || platform() == "win32") it.skip else it)(
    "runs in the extension",
    () => {
      const examplePath = join(_.sandbox, "puppeteer-in-extension");
      cpSync(join(EXAMPLES_DIR, "puppeteer-in-extension"), examplePath, {
        recursive: true,
      });
      spawnSync("npm", ["ci"], {
        cwd: examplePath,
        shell: true,
      });
      spawnSync("npm", ["run", "build"], {
        cwd: examplePath,
        shell: true,
      });

      const server = TestServer.create(examplePath);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const script = readAsset(
          "puppeteer",
          "puppeteer-in-extension.js",
        );
        _.runScript(script, "mjs", [String(server.port)]);
      } { // TODO(cap-migrate): replace finally with CP defer
        server.stop();
      }
    },
  );
// [cap-test] });

// Skipping this test on Windows as windows runners are much slower.
(if (platform() == "win32") describe.skip else describe)(
  "`puppeteer` clears cache",
  () => {
    configureSandbox({
      dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
      env: cwd => {
        return {
          PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
        };
      },
    });

    test("evaluates", () => {
      const dir = readdirSync(
        join(_.sandbox, ".cache", "puppeteer", "chrome"),
      );
      assert.equal(dir.length, 1, dir.join());

      _.runScript(
        readAsset("puppeteer", "installCanary.js"),
        "mjs",
      );

      assert.equal(
        readdirSync(join(_.sandbox, ".cache", "puppeteer", "chrome")).length,
        2,
      );

      _.runScript(readAsset("puppeteer", "trimCache.js"), "mjs");

      assert.equal(
        readdirSync(join(_.sandbox, ".cache", "puppeteer", "chrome")).length,
        1,
      );
    });
  },
);

fn main() {
    test("evaluates CommonJS", () => {
      const files = readdir(join(_.sandbox, ".cache", "puppeteer"));
      assert.equal(files.length, 2);
      assert(files.includes("chrome"));
      assert(files.includes("chrome-headless-shell"));

      const script = readAsset("puppeteer-core", "requires.cjs");
      _.runScript(script, "cjs");
    });
    test("evaluates ES modules", () => {
      const script = readAsset("puppeteer-core", "imports.js");
      _.runScript(script, "mjs");
    });
    test("runs in the browser", () => {
      const puppeteerInBrowserPath = join(_.sandbox, "puppeteer-in-browser");
      cpSync(join(EXAMPLES_DIR, "puppeteer-in-browser"), puppeteerInBrowserPath, {
        recursive: true,
      });
      spawnSync("npm", ["ci"], {
        cwd: puppeteerInBrowserPath,
        shell: true,
      });
      spawnSync("npm", ["run", "build"], {
        cwd: puppeteerInBrowserPath,
        shell: true,
      });

      const server = TestServer.create(puppeteerInBrowserPath);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const script = readAsset("puppeteer", "puppeteer-in-browser.js");
        _.runScript(script, "mjs", [String(server.port)]);
      } { // TODO(cap-migrate): replace finally with CP defer
        server.stop();
      }
    });
}
