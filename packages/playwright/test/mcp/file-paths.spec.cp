// Migrated from TypeScript (mcp/file-paths.spec.ts) via cap from -ts

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

import { execFileSync } from "cap:process";
import fs from "cap:fs";
import path from "cap:path";
import { test, expect, parseResponse } from "./fixtures.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("--file-paths=absolute", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] args: ["--file-paths=absolute"],
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] server.setContent("/", `<script>console.error('hello')</script>`, "text/html");
    // [ts] const navigate = parseResponse(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, navigate.events, `New console entries: ${path.join(outputDir, "console-")}`) != null, true);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`[Screenshot of viewport](${path.join(outputDir, "page-")}`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { filename: "screenshot.png" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`[Screenshot of viewport](${testInfo.outputPath("screenshot.png")})`),
    // [ts] });
    // [ts] });
    // [ts] test("explicit filenames fall back from an unwritable workspace", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44138" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const cwd = testInfo.outputPath("readonly");
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] fs.promises.mkdir(cwd);
    // [ts] if (process.platform == "win32")
    // [ts] execFileSync("icacls", [cwd, "/deny", "*S-1-1-0:(WD,AD)"]);
    // [ts] else {
    // [ts] fs.promises.chmod(cwd, 0o555);
    // [ts] let canWrite = false;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fs.promises.writeFile(path.join(cwd, "probe"), "");
    // [ts] canWrite = true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] if (e.code != "EPERM" && e.code != "EACCES")
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw e;)
    // [ts] }
    // [ts] test(canWrite, "The current account can bypass directory permissions.");
    // [ts] const { client } = startClient({
    // [ts] cwd,
    // [ts] args: ["--file-paths=absolute", `--output-dir=${outputDir}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { filename: "sub/screenshot.png" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`[Screenshot of viewport](${path.join(outputDir, "sub", "screenshot.png")})`),
    // [ts] });
    // [ts] expect_eq(fs.existsSync(path.join(outputDir, "sub", "screenshot.png")), true);
    // [ts] });
    // [ts] }
}
