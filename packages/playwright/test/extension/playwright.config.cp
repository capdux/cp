// Migrated from TypeScript (extension/playwright.config.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";

// [cap-migrate] import { TestOptions } from "../mcp/fixtures.cp";

// TODO(cap-migrate): refine TS statement
// [ts] export default defineConfig({
// [ts] testDir: "./",
// [ts] fullyParallel: true,
// [ts] forbidOnly: !!process.env.CI,
// [ts] retries: if (process.env.CI) 2 else 0,
// [ts] reporter: process.env.CI ? [
// [ts] ["list"],
// [ts] ["../config/parquetReporter.ts"],
// [ts] ] : "list",
// [ts] projects: [
// [ts] { name: "chromium", use: { mcpBrowser: "chromium" } },
// [ts] ],
// [ts] });
