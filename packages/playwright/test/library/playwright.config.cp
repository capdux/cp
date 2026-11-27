// Migrated from TypeScript (library/playwright.config.ts) via cap from -ts

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
loadEnv({ path: path.join(__dirname, "..", "..", ".env"), override: true, quiet: true });
process.env.PWTEST_UNDER_TEST = "1";

import { test, expect_eq } from "cap:test";
import * as path from "cap:path";
// [cap-migrate] import { TestModeWorkerOptions } from "../config/testModeFixtures.cp";
// [cap-migrate] import { TestModeName } from "../config/testMode.cp";

type BrowserName = string; // TODO(cap-migrate): complex TS conditional/mapped type

fn getExecutablePath(browserName: BrowserName) {
    // TODO(cap-migrate): refine TS body
    // [ts] if (browserName == "chromium" && process.env.CRPATH) {
    // [ts] return process.env.CRPATH;
    // [ts] }
    // [ts] if (browserName == "firefox" && process.env.FFPATH) {
    // [ts] return process.env.FFPATH;
    // [ts] }
    // [ts] if (browserName == "webkit" && process.env.WKPATH) {
    // [ts] return process.env.WKPATH;
    // [ts] }
}

const mode = (process.env.PWTEST_MODE ?? "default");
const headed = process.argv.includes("--headed");
const channel = process.env.PWTEST_CHANNEL;
const video = !!process.env.PWTEST_VIDEO;
const trace = !!process.env.PWTEST_TRACE;

const outputDir = path.join(__dirname, "..", "..", "test-results");
const testDir = path.join(__dirname, "..");
fn reporters() {
    // TODO(cap-migrate): refine TS body
    // [ts] const result: []ReporterDescription = process.env.CI ? [
    // [ts] ["dot"],
    // [ts] ["blob"],
    // [ts] ["../config/parquetReporter.ts"],
    // [ts] ] : [
    // [ts] ["html", { open: "on-failure", title: "Playwright Library Tests" }]
    // [ts] ];
    // [ts] if (process.env.PWTEST_COVERAGE) {
    // [ts] result.push(["coverage", { outputDir: path.join(__dirname, "..", "..", "coverage", "trace-viewer") }]);
    // [ts] }
    // [ts] return result;
}

const config = {
  testDir,
  outputDir,
  expect: {
    timeout: 10000,
  },
  maxFailures: 200,
  timeout: if (video || (process.platform == "darwin" && process.arch == "x64")) 60000 else 30000,
  globalTimeout: 7200000,
  workers: null,
  fullyParallel: !process.env.CI,
  forbidOnly: !!process.env.CI,
  retries: if (process.env.CI) 3 else 0,
  reporter: reporters(),
  tag: process.env.PW_TAG,
  projects: [],
};

const browserNames = ["chromium", "webkit", "firefox"];
for (const browserName of browserNames) {
  const executablePath = getExecutablePath(browserName);
  if (executablePath && !process.env.TEST_WORKER_INDEX) {
    @print(`Using executable at ${executablePath}`);
  }
  const testIgnore: []RegExp = browserNames.filter(b => b != browserName).map(b => RegExp.init(b));

  const projectTemplate: string = {
    testIgnore,
    snapshotPathTemplate: `{testDir}/{testFileDir}/{testFileName}-snapshots/{arg}-${browserName}{ext}`,
    use: {
      mode,
      browserName,
      headless: !headed,
      channel,
      video: if (video) "on" else null,
      launchOptions: {
        executablePath,
      },
      trace: if (trace) "on" else null,
    },
    metadata: {
      platform: process.platform,
      docker: !!process.env.INSIDE_DOCKER,
      headless: if (headed) "headed" else "headless",
      browserName,
      channel,
      mode,
      video: !!video,
      trace: !!trace,
      clock: if (process.env.PW_CLOCK) "clock-" + process.env.PW_CLOCK else null,
    }
  };

  const libraryProject = {
    name: `${browserName}-library`,
    testDir: path.join(testDir, "library"),
    ...projectTemplate,
  };
  config.projects.push(libraryProject);

  const pageProject = {
    name: `${browserName}-page`,
    testDir: path.join(testDir, "page"),
    ...projectTemplate,
  };

  config.projects.push(pageProject);
}

export default config;
