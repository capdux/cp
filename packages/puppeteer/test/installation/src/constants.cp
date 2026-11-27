// Migrated from TypeScript (installation/src/constants.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { globSync } from "cap:fs";
import { dirname, join, resolve } from "cap:path";
import { fileURLToPath } from "cap:url";

pub const PUPPETEER_CORE_PACKAGE_PATH = resolve(
  globSync("puppeteer-core-*.tgz")[0],
);
pub const PUPPETEER_BROWSERS_PACKAGE_PATH = resolve(
  globSync("puppeteer-browsers-[0-9]*.tgz")[0],
);
pub const PUPPETEER_PACKAGE_PATH = resolve(
  globSync("puppeteer-[0-9]*.tgz")[0],
);
// TODO(cap-migrate): refine TS statement
// [ts] pub const ASSETS_DIR = join(
// [ts] dirname(fileURLToPath(import.meta.url)),
// [ts] "..",
// [ts] "assets",
// [ts] );
// TODO(cap-migrate): refine TS statement
// [ts] pub const EXAMPLES_DIR = join(
// [ts] dirname(fileURLToPath(import.meta.url)),
// [ts] "..",
// [ts] "..",
// [ts] "..",
// [ts] "examples",
// [ts] );
