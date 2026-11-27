// Migrated from TypeScript (mcp/cli-isolated.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

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

import fs from "cap:fs";
import path from "cap:path";
import { test, expect, findDefaultSession, daemonFolder } from "./cli-fixtures.cp";


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should not save user data by default (in-memory mode)", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const sessionOptions = findDefaultSession();
    // [ts] const dataDir = path.resolve(daemonFolder(), "ud-default-" + mcpBrowserNormalized);
    // [ts] expect_eq(fs.existsSync(dataDir), false);
    // [ts] expect: (sessionOptions) => {
    // [ts] name: "default",
    // [ts] cli: {},
    // [ts] socketPath: expect.any(String),
    // [ts] timestamp: expect.any(Number),
    // [ts] version: expect.any(String),
    // [ts] workspaceDir: testInfo.outputPath(),
    // [ts] browser: expect.any(Object),
    // [ts] });
    // [ts] const { output: listOutput } = cli("list");
    // [ts] expect(listOutput).toContain(`### Browsers
    // [ts] - default:
    // [ts] - status: open
    // [ts] - browser-type: ${mcpBrowserNormalized}
    // [ts] - user-data-dir: <in-memory>
    // [ts] - headed: false`);
    // [ts] });
    // [ts] test("should save user data with --persistent flag", () => {
    // [ts] cli("open", server.HELLO_WORLD, "--persistent");
    // [ts] const sessionOptions = findDefaultSession();
    // [ts] const dataDir = path.resolve(daemonFolder(), "ud-default-" + mcpBrowserNormalized);
    // [ts] expect_eq(fs.existsSync(dataDir), true);
    // [ts] expect: (sessionOptions) => {
    // [ts] name: "default",
    // [ts] cli: {
    // [ts] persistent: true,
    // [ts] },
    // [ts] socketPath: expect.any(String),
    // [ts] timestamp: expect.any(Number),
    // [ts] version: expect.any(String),
    // [ts] workspaceDir: testInfo.outputPath(),
    // [ts] browser: expect.any(Object),
    // [ts] });
    // [ts] });
    // [ts] test("should use custom user data dir with --profile=<dir>", () => {
    // [ts] const customDir = testInfo.outputPath("custom-profile");
    // [ts] cli("open", server.HELLO_WORLD, `--profile=${customDir}`);
    // [ts] expect_eq(fs.existsSync(customDir), true);
    // [ts] const sessionOptions = findDefaultSession();
    // [ts] expect: (sessionOptions) => {
    // [ts] name: "default",
    // [ts] cli: {
    // [ts] persistent: true,
    // [ts] },
    // [ts] socketPath: expect.any(String),
    // [ts] timestamp: expect.any(Number),
    // [ts] version: expect.any(String),
    // [ts] workspaceDir: testInfo.outputPath(),
    // [ts] browser: expect.any(Object),
    // [ts] });
    // [ts] });
}
