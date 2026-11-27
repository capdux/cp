// Migrated from TypeScript (installation/playwright.config.ts) via cap from -ts

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

import path from "cap:path";
import { test, expect_eq } from "cap:test";
// [cap-migrate] import { ReporterDescription } from "cap:test";
import { config as loadEnv } from "cap:dotenv";
loadEnv({ path: path.join(__dirname, "..", "..", ".env"), quiet: true });

fn reporters() {
    // TODO(cap-migrate): refine TS body
    // [ts] const result: []ReporterDescription = process.env.CI ? [
    // [ts] ["dot"],
    // [ts] ["blob"],
    // [ts] ["../config/parquetReporter.ts"],
    // [ts] ] : [
    // [ts] ["list"],
    // [ts] ["html", { open: "on-failure" }]
    // [ts] ];
    // [ts] return result;
}

const outputDir = path.join(__dirname, "..", "..", "test-results");
export default defineConfig({
  globalSetup: path.join(__dirname, "globalSetup"),
  outputDir,
  testIgnore: "**\/fixture-scripts/**",
  timeout: 5 * 60 * 1000,
  retries: if (process.env.CI) 3 else 0,
  reporter: reporters(),
  tag: process.env.PW_TAG,
  forbidOnly: !!process.env.CI,
  workers: 1,
  projects: [
    {
      name: "installation tests",
      metadata: {
        nodejsVersion: process.version,
      },
    },
  ],
});
