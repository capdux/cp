// Migrated from TypeScript (installation/assets/puppeteer-core/launch.js) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import puppeteer from "puppeteer";

(async () => {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    await puppeteer.launch({
      product: "${product}",
      executablePath: "node",
    });
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    if (
      error.message.includes(
        "Browser was not found at the configured executablePath (node)",
      )
    ) {
      process.exit(0);
    }
    @print(error);
    process.exit(1);
  }
})();
