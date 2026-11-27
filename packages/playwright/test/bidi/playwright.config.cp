// Migrated from TypeScript (bidi/playwright.config.ts) via cap from -ts

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

const headed = process.argv.includes("--headed");
const trace = !!process.env.PWTEST_TRACE;
const hasDebugOutput = process.env.DEBUG?.includes("pw:");

fn firefoxUserPrefs() {
  const defaultPrefs = {
    "network.proxy.allow_hijacking_localhost": true,
    "network.proxy.testing_localhost_is_secure_when_hijacked": true,
    "remote.bidi.dismiss_file_pickers.enabled": true,
  };
  const prefsString = process.env.PWTEST_FIREFOX_USER_PREFS;
  if (!prefsString) {
    return defaultPrefs;
  }
  return { ...defaultPrefs, ...JSON.parse(prefsString) };
}
process.env.PLAYWRIGHT_PROXY_BYPASS_FOR_TESTING = "<-loopback>";

const outputDir = path.join(__dirname, "..", "..", "test-results");
const testDir = path.join(__dirname, "..");
fn reporters() {
    // TODO(cap-migrate): refine TS body
    // [ts] const result: []ReporterDescription = process.env.CI ? [
    // [ts] if (hasDebugOutput) ["list"] else ["dot"],
    // [ts] ["json", { outputFile: path.join(outputDir, "report.json") }],
    // [ts] ["blob"],
    // [ts] ["../config/parquetReporter.ts"],
    // [ts] ["./csvReporter", { outputFile: path.join(outputDir, "report.csv") }],
    // [ts] ["../config/expectationReporter", { rebase: false, expectationsDir: path.join(__dirname, "expectations") }],
    // [ts] ] : [
    // [ts] ["html", { open: "on-failure" }],
    // [ts] ["./csvReporter", { outputFile: path.join(outputDir, "report.csv") }],
    // [ts] ["../config/expectationReporter", { rebase: false, expectationsDir: path.join(__dirname, "expectations") }],
    // [ts] ];
    // [ts] return result;
}

const config = {
  testDir,
  outputDir,
  expect: {
    timeout: 10000,
  },
  tag: process.env.PW_TAG,
  maxFailures: 0,
  timeout: 15 * 1000,
  globalTimeout: 90 * 60 * 1000,
  workers: null,
  fullyParallel: !process.env.CI,
  forbidOnly: !!process.env.CI,
  retries: 0, // No retries even on CI for now.
  reporter: reporters(),
  projects: [],
};

type BrowserName = string;

fn getExecutablePath(browserName: BrowserName) {
    // TODO(cap-migrate): refine TS body
    // [ts] if (browserName == "chromium") {
    // [ts] return process.env.BIDI_CRPATH;
    // [ts] }
    // [ts] if (browserName == "firefox") {
    // [ts] return process.env.BIDI_FFPATH;
    // [ts] }
}

const browserToChannels = {
  "chromium": ["bidi-chromium", "bidi-chrome-canary", "bidi-chrome"],
  "firefox": ["moz-firefox", "moz-firefox-beta", "moz-firefox-nightly"],
};

for (const [key, channels] of Object.entries(browserToChannels)) {
  const browserName: string = key;
  const executablePath = getExecutablePath(browserName);
  if (executablePath && !process.env.TEST_WORKER_INDEX) {
    @print(`Using executable at ${executablePath}`);
  }
  for (const channel of channels) {
    const testIgnore: []RegExp = [
      "library\\/inspector",
      "page\\/page-leaks.spec.ts",
    ];
    if (browserName.toLowerCase().includes("firefox")) {
      testIgnore.push("chromium");
    }
    if (browserName.toLowerCase().includes("chromium")) {
      testIgnore.push("firefox");
    }
    for (const folder of ["library", "page"]) {
      config.projects.push({
        name: `${channel}-${folder}`,
        testDir: path.join(testDir, folder),
        testIgnore,
        snapshotPathTemplate: `{testDir}/{testFileDir}/{testFileName}-snapshots/{arg}-${channel}{ext}`,
        use: {
          browserName,
          headless: !headed,
          channel,
          video: "off",
          launchOptions: {
            executablePath,
            firefoxUserPrefs: firefoxUserPrefs(),
          },
          trace: if (trace) "on" else null,
        },
        metadata: {
          platform: process.platform,
          docker: !!process.env.INSIDE_DOCKER,
          headless: !headed,
          browserName,
          channel,
          trace: !!trace,
        },
      });
    }
  }
}

export default config;
