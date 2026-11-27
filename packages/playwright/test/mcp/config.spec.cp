// Migrated from TypeScript (mcp/config.spec.ts) via cap from -ts

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

import { test, expect, parseResponse } from "./fixtures.cp";
// [cap-migrate] import { Config } from "../../packages/playwright-core/src/tools/mcp/config.d.cp";





// [cap-test] test.describe(() => {
  test.use({ mcpBrowser: "" });
// [cap-test] });



// [cap-test] test.describe("chromiumSandbox", () => {


// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("config user data dir", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1836" },
    // [ts] }, (_p, testInfo) => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <body>Hello, world!</body>
    // [ts] `, 'text/html');
    // Relative paths in the config file resolve against the config directory,
    // not against the server cwd.
    // [ts] const configDir = testInfo.outputPath("config");
    // [ts] fs.promises.mkdir(configDir);
    // [ts] const config: Config = {
    // [ts] outputDir: "artifacts",
    // [ts] browser: {
    // [ts] userDataDir: "user-data-dir",
    // [ts] },
    // [ts] };
    // [ts] const configPath = path.join(configDir, "config.json");
    // [ts] fs.promises.writeFile(configPath, JSON.stringify(config, null, 2));
    // [ts] const { client } = startClient({ args: ["--config", configPath] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] expect_eq((client.callTool({ name: "browser_take_screenshot" })).isError, false);
    // [ts] const artifacts = fs.promises.readdir(path.join(configDir, "artifacts"));
    // [ts] expect_eq(artifacts, expect.arrayContaining([expect.stringMatching("\\.yml$"), expect.stringMatching("\\.png$")]));
    // [ts] const files = fs.promises.readdir(path.join(configDir, "user-data-dir"));
    // [ts] expect_eq(files.length > 0, true);
    // [ts] });
    // [ts] test("config storage state", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1836" },
    // [ts] }, (_p, testInfo) => {
    // [ts] server.setContent("/", `
    // [ts] <body></body>
    // [ts] <script>document.body.textContent = 'Storage: ' + localStorage.getItem('test');</script>
    // [ts] `, 'text/html');
    // Storage state only applies to isolated contexts. A relative path in the
    // config file resolves against the config directory, not against the server cwd.
    // [ts] const configDir = testInfo.outputPath("config");
    // [ts] fs.promises.mkdir(configDir);
    // [ts] fs.promises.writeFile(path.join(configDir, "state.json"), JSON.stringify({
    // [ts] origins: [{ origin: server.PREFIX, localStorage: [{ name: "test", value: "config-value" }] }],
    // [ts] }));
    // [ts] const config: Config = {
    // [ts] browser: {
    // [ts] isolated: true,
    // [ts] contextOptions: { storageState: "state.json" },
    // [ts] },
    // [ts] };
    // [ts] const configPath = path.join(configDir, "config.json");
    // [ts] fs.promises.writeFile(configPath, JSON.stringify(config, null, 2));
    // [ts] const { client } = startClient({ args: ["--config", configPath] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining("Storage: config-value"),
    // [ts] });
    // [ts] });
    // [ts] test("config with UTF-8 BOM", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <body>Hello, world!</body>
    // [ts] `, 'text/html');
    // [ts] const config: Config = {
    // [ts] browser: {
    // [ts] userDataDir: testInfo.outputPath("user-data-dir"),
    // [ts] },
    // [ts] };
    // [ts] const configPath = testInfo.outputPath("config.json");
    // Write config with UTF-8 BOM prefix, as some Windows editors (Notepad, PowerShell) do.
    // [ts] fs.promises.writeFile(configPath, "\uFEFF" + JSON.stringify(config, null, 2));
    // [ts] const { client } = startClient({ args: ["--config", configPath] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] const files = fs.promises.readdir(config.browser.userDataDir);
    // [ts] expect_eq(files.length > 0, true);
    // [ts] });
    // [ts] test("executable path", () => {
    // [ts] const { client } = startClient({ args: ["--executable-path", testInfo.outputPath("missing-executable")] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringMatching("Failed to launch.*missing-executable"),
    // [ts] });
    // [ts] });
    // [ts] test("browserName", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/458" } }, (_p, testInfo) => {
    // [ts] const config: Config = {
    // [ts] browser: {
    // [ts] browserName: "firefox",
    // [ts] },
    // [ts] };
    // [ts] const configPath = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configPath, JSON.stringify(config, null, 2));
    // [ts] const { client } = startClient({ args: ["--config", configPath] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "data:text/html,<script>document.title = navigator.userAgent</script>" },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`Firefox`),
    // [ts] });
    // [ts] });
    // [ts] test("config ignoreDefaultArgs merged with persistent mode defaults", () => {
    // [ts] test(!["chrome", "chromium", "msedge"].includes(mcpBrowser), "chrome://version is Chromium-specific");
    // [ts] const config: Config = {
    // [ts] browser: {
    // [ts] userDataDir: testInfo.outputPath("user-data-dir"),
    // [ts] launchOptions: {
    // [ts] ignoreDefaultArgs: ["--password-store=basic"],
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] const { client } = startClient({ config });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "chrome://version" },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.getElementById(\"command_line\").innerText" },
    // [ts] });
    // [ts] const commandLine = result.content[0].text;
    // User-specified arg should be removed.
    // [ts] expect_eq(std.mem.indexOf(u8, commandLine, "--password-store=basic") == null, true);
    // Persistent mode's built-in --disable-extensions should also be removed.
    // [ts] expect_eq(std.mem.indexOf(u8, commandLine, "--disable-extensions") == null, true);
    // Other default args should still be present.
    // [ts] expect_eq(std.mem.indexOf(u8, commandLine, "--use-mock-keychain") != null, true);
    // [ts] });
    // [ts] test("browser_get_config returns merged config from file, env and cli", () => {
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 800, height: 600 },
    // [ts] },
    // [ts] },
    // [ts] capabilities: ["config"],
    // [ts] timeouts: {
    // [ts] action: 10000,
    // [ts] navigation: 30000,
    // [ts] },
    // [ts] },
    // [ts] args: ["--isolated"],
    // [ts] env: {
    // [ts] PLAYWRIGHT_MCP_TIMEOUT_NAVIGATION: "45000",
    // [ts] },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_get_config",
    // [ts] });
    // [ts] expect_eq(result.isError, false);
    // [ts] const parsed = parseResponse(result);
    // [ts] const config = JSON.parse(parsed.result);
    // From config file.
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 800, height: 600 });
    // [ts] expect_eq(config.timeouts.action, 10000);
    // Env var overrides file value.
    // [ts] expect_eq(config.timeouts.navigation, 45000);
    // From CLI arg (--isolated).
    // [ts] expect_eq(config.browser.isolated, true);
    // [ts] });
    // [ts] test((_p) => mcpBrowser != "chrome", "Channel-agnostic tests.");
    // [ts] test("config file value is respected", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1716" } }, (_p) => {
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] capabilities: ["config"],
    // [ts] browser: { launchOptions: { chromiumSandbox: true } },
    // [ts] },
    // [ts] });
    // [ts] const config = JSON.parse(parseResponse(client.callTool({ name: "browser_get_config" })).result);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, true);
    // [ts] });
    // [ts] test("--no-sandbox overrides config file value", () => {
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] capabilities: ["config"],
    // [ts] browser: { launchOptions: { chromiumSandbox: true } },
    // [ts] },
    // [ts] args: ["--no-sandbox"],
    // [ts] });
    // [ts] const config = JSON.parse(parseResponse(client.callTool({ name: "browser_get_config" })).result);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, false);
    // [ts] });
}
