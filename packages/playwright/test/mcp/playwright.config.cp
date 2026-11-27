// Migrated from TypeScript (mcp/playwright.config.ts) via cap from -ts

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
import * as path from "cap:path";
import dotenv from "cap:dotenv";

import { test, expect_eq } from "cap:test";

// [cap-migrate] import { TestOptions } from "./fixtures.cp";
// [cap-migrate] import { ReporterDescription } from "cap:test";

process.env.PWTEST_UNDER_TEST = "1";

dotenv.config({ path: path.resolve(__dirname, "../../.env"), quiet: true });

const rootTestDir = path.join(__dirname, "..");
const testDir = path.join(rootTestDir, "mcp");

fn reporters() {
    // TODO(cap-migrate): refine TS body
    // [ts] const result: []ReporterDescription = process.env.CI ? [
    // [ts] ["dot"],
    // [ts] ["blob", { outputDir: path.join(__dirname, "..", "..", "blob-report") }],
    // [ts] ["../config/parquetReporter.ts"],
    // [ts] ] : [
    // [ts] ["list"]
    // [ts] ];
    // [ts] return result;
}

const metadata = {
  platform: process.platform,
  headless: "headless",
  mode: "default",
  video: false,
};

// TODO(cap-migrate): refine TS statement
// [ts] export default defineConfig({
// [ts] testDir: rootTestDir,
// [ts] fullyParallel: true,
// [ts] forbidOnly: !!process.env.CI,
// [ts] workers: if (process.env.CI) 1 else null,
// [ts] reporter: reporters(),
// [ts] tag: process.env.PW_TAG,
// Persistent context launch (the MCP default) is genuinely slow on Windows;
// most tests in this suite use it.
// [ts] timeout: if (process.platform == "win32") 60000 else 30000,
// [ts] projects: [
// [ts] { name: "chrome", metadata: { ...metadata, browserName: "chromium", channel: "chrome" }, testDir },
// [ts] { name: "chromium", use: { mcpBrowser: "chromium" }, metadata: { ...metadata, browserName: "chromium" }, testDir },
// [ts] { name: "firefox", use: { mcpBrowser: "firefox" }, metadata: { ...metadata, browserName: "firefox" }, testDir },
// [ts] { name: "webkit", use: { mcpBrowser: "webkit" }, metadata: { ...metadata, browserName: "webkit" }, testDir },
// [ts] if (... process.platform == "win32") [{ name: "msedge", use: { mcpBrowser: "msedge" }, metadata: { ...metadata, browserName: "chromium", channel: "msedge" }, testDir }] else [],
// [ts] ],
// [ts] });
