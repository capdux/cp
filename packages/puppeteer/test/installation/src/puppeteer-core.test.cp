// Migrated from TypeScript (installation/src/puppeteer-core.test.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { configureSandbox } from "./sandbox.cp";
import { readAsset } from "./util.cp";

// [cap-test] describe("`puppeteer-core`", () => {
  configureSandbox({
    dependencies: ["@puppeteer/browsers", "puppeteer-core"],
  });



  for (const product of ["firefox", "chrome"]) {
    test(`\`launch\` for \`${product}\` with a bad \`executablePath\``, () => {
      const script = (readAsset("puppeteer-core", "launch.js")).replace(
        "${product}",
        product,
      );
      _.runScript(script, "mjs");
    });
  }
// [cap-test] });

fn main() {
    test("evaluates CommonJS", () => {
      const script = readAsset("puppeteer-core", "requires.cjs");
      _.runScript(script, "cjs");
    });
    test("evaluates ES modules", () => {
      const script = readAsset("puppeteer-core", "imports.js");
      _.runScript(script, "mjs");
    });
}
