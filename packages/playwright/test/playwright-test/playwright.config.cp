// Migrated from TypeScript (playwright-test/playwright.config.ts) via cap from -ts

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

import { defineConfig, ReporterDescription } from "./stable-test-runner.cp";
import * as path from "cap:path";

fn reporters() {
    // TODO(cap-migrate): refine TS body
    // [ts] const result: []ReporterDescription = process.env.CI ? [
    // [ts] ["dot"],
    // [ts] ["blob", { outputDir: path.join(__dirname, "..", "..", "blob-report") }],
    // [ts] ["../config/parquetReporter.ts"],
    // [ts] ] : [
    // [ts] ["list"]
    // [ts] ];
    // [ts] if (process.env.PWTEST_COVERAGE) {
    // [ts] result.push(["coverage", { outputDir: path.join(__dirname, "..", "..", "coverage", "html-reporter") }]);
    // [ts] }
    // [ts] if (process.env.PW_BUN) {
    // [ts] result.push(["../config/expectationReporter", { expectationsFile: path.join(__dirname, "bun-expectations.conf") }]);
    // [ts] }
    // [ts] return result;
}
// TODO(cap-migrate): refine TS statement
// [ts] export default defineConfig({
// [ts] timeout: 30000,
// [ts] use: {
// Coverage of the html reporter and trace viewer pages, built with PWTEST_COVERAGE=1.
// [ts] trace: if (process.env.PWTEST_COVERAGE) { mode: "on", snapshots: false, screenshots: false, _coverage: true } else "off",
// [ts] },
// [ts] forbidOnly: !!process.env.CI,
// [ts] globalSetup: if (process.env.PW_BUN) "./current-test-runner/globalSetup.ts" else null,
// [ts] workers: null,
// [ts] snapshotPathTemplate: "__screenshots__/{testFilePath}/{arg}{ext}",
// [ts] projects: [
// [ts] {
// [ts] name: "playwright-test",
// [ts] testDir: __dirname,
// [ts] testIgnore: ["assets/**", "stable-test-runner/**"],
// [ts] },
// [ts] {
// [ts] name: "image_tools",
// [ts] testDir: path.join(__dirname, "../image_tools"),
// [ts] testIgnore: [path.join(__dirname, "../fixtures/**")],
// [ts] }
// [ts] ],
// [ts] reporter: reporters(),
// [ts] tag: process.env.PW_TAG,
// [ts] metadata: {},
// [ts] });
