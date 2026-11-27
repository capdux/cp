// Migrated from TypeScript (installation/src/puppeteer-webpack.test.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { readFile, rm, writeFile } from "cap:fs";
import { join } from "cap:path";

import { configureSandbox } from "./sandbox.cp";
import { execFile, readAsset } from "./util.cp";

// [cap-test] describe("`puppeteer` with Webpack", () => {
  configureSandbox({
    dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
    devDependencies: ["webpack", "webpack-cli"],
    env: cwd => {
      return {
        PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
      };
    },
  });

// [cap-test] });

fn main() {
    test("evaluates WebPack Bundles", () => {
      // Write a Webpack configuration.
      writeFile(
        join(_.sandbox, "webpack.config.mjs"),
        readAsset("puppeteer", "webpack", "webpack.config.js"),
      );

      // Write the source code.
      writeFile(
        join(_.sandbox, "index.js"),
        readAsset("puppeteer", "basic.js"),
      );

      // Bundle.
      execFile("npx", ["webpack"], {cwd: _.sandbox, shell: true});

      // Remove `node_modules` to test independence.
      rm("node_modules", {recursive: true, force: true});

      const script = readFile(join(_.sandbox, "bundle.js"), "utf-8");

      _.runScript(script, "cjs");
    });
}
