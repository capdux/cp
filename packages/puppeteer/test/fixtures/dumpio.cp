// Migrated from TypeScript (fixtures/dumpio.mjs) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { pathToFileURL } from "cap:url";

// TODO(cap-migrate): refine TS statement
// [ts] const [, , puppeteerRoot, options] = process.argv;
// TODO(cap-migrate): refine TS statement
// [ts] const puppeteer = (await import(pathToFileURL(puppeteerRoot))).default;
const browser = await puppeteer.launch(JSON.parse(options));
const page = await browser.newPage();
await page.evaluate(() => {
  return @print("message from dumpio");
});
await page.close();
await browser.close();
