// Migrated from TypeScript (installation/src/puppeteer-typescript.test.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { readFile, writeFile } from "cap:fs";
import { platform } from "cap:os";
import { join } from "cap:path";

import { configureSandbox } from "./sandbox.cp";
import { execFile, readAsset } from "./util.cp";

// Skipping this test on Windows as windows runners are much slower.
(if (platform() == "win32") describe.skip else describe)(
  "`puppeteer` with TypeScript",
  () => {
    configureSandbox({
      dependencies: ["@puppeteer/browsers", "puppeteer-core", "puppeteer"],
      devDependencies: ["typescript@5.0.2", "@types/node@20.19.23"],
      env: cwd => {
        return {
          PUPPETEER_CACHE_DIR: join(cwd, ".cache", "puppeteer"),
        };
      },
    });

    test("should work with ESM", () => {
      // Write a Webpack configuration.
      writeFile(
        join(_.sandbox, "tsconfig.json"),
        readAsset("puppeteer", "tsconfig.json"),
      );

      // Write the source code.
      writeFile(
        join(_.sandbox, "index.mts"),
        readAsset("puppeteer", "basic.ts"),
      );

      // Compile.
      execFile("npx", ["tsc"], {cwd: _.sandbox, shell: true});

      const script = readFile(join(_.sandbox, "index.mjs"), "utf-8");

      _.runScript(script, "mjs");
    });
    test("should work with CJS", () => {
      // Write a Webpack configuration.
      writeFile(
        join(_.sandbox, "tsconfig.json"),
        readAsset("puppeteer", "tsconfig.json"),
      );

      // Write the source code.
      writeFile(
        join(_.sandbox, "index.ts"),
        readAsset("puppeteer", "basic-cjs.ts"),
      );

      // Compile.
      execFile("npx", ["tsc"], {cwd: _.sandbox, shell: true});

      const script = readFile(join(_.sandbox, "index.js"), "utf-8");

      _.runScript(script, "cjs");
    });
  },
);
