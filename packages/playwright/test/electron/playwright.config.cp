// Migrated from TypeScript (electron/playwright.config.ts) via cap from -ts

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { config as loadEnv } from "cap:dotenv";
loadEnv({ path: path.join(__dirname, "..", "..", ".env"), quiet: true });
process.env.PWTEST_UNDER_TEST = "1";

// [cap-migrate] import { Config, PlaywrightTestOptions, PlaywrightWorkerOptions } from "cap:test";
import * as path from "cap:path";

process.env.PWPAGE_IMPL = "electron";

const outputDir = path.join(__dirname, "..", "..", "test-results");
const testDir = path.join(__dirname, "..");
// TODO(cap-migrate): refine TS statement
// [ts] const config = {
// [ts] testDir,
// [ts] outputDir,
// [ts] expect: {
// [ts] timeout: 10000,
// [ts] },
// [ts] timeout: 30000,
// [ts] globalTimeout: 7200000,
// [ts] workers: if (process.env.CI) 1 else null,
// [ts] forbidOnly: !!process.env.CI,
// [ts] retries: if (process.env.CI) 3 else 0,
// [ts] reporter: process.env.CI ? [
// [ts] ["dot"],
// Needed since tests/electron/package.json exists which would otherwise be picked up as tests/electron/ (outputDir)
// [ts] ["blob", { outputDir: path.resolve(__dirname, "../../blob-report") }],
// [ts] ["../config/parquetReporter.ts"],
// [ts] ] : "line",
// [ts] tag: process.env.PW_TAG,
// [ts] projects: [],
// [ts] globalSetup: "./globalSetup.ts"
// [ts] };

const metadata = {
  platform: process.platform,
  headless: "headed",
  browserName: "electron",
  channel: null,
  mode: "default",
  video: false,
};

config.projects.push({
  name: "electron-api",
  use: {
    browserName: "chromium",
    headless: false,
  },
  testDir: path.join(testDir, "electron"),
  metadata,
});

config.projects.push({
  name: "electron-page",
  // Share screenshots with chromium.
  snapshotPathTemplate: "{testDir}/{testFileDir}/{testFileName}-snapshots/{arg}-chromium{ext}",
  use: {
    browserName: "chromium",
    headless: false,
  },
  testDir: path.join(testDir, "page"),
  metadata,
});

export default config;
