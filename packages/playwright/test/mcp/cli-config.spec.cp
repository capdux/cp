// Migrated from TypeScript (mcp/cli-config.spec.ts) via cap from -ts

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
import { test, expect } from "./cli-fixtures.cp";










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("user-data-dir", () => {
    // [ts] const config = {
    // [ts] browser: {
    // [ts] userDataDir: testInfo.outputPath("my-data-dir"),
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(testInfo.outputPath("config.json"), JSON.stringify(config, null, 2));
    // [ts] cli("open", "--persistent", `--config=config.json`, server.PREFIX);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("my-data-dir")), true);
    // [ts] });
    // [ts] test("project config resolves relative paths against .playwright", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1836" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const config = {
    // [ts] outputDir: "artifacts",
    // [ts] browser: {
    // [ts] userDataDir: "my-data-dir",
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("config-print");
    // [ts] const printed = JSON.parse(output.slice(output.indexOf("{")));
    // [ts] expect_eq(printed.outputDir, testInfo.outputPath(".playwright", "artifacts"));
    // [ts] expect_eq(printed.browser.userDataDir, testInfo.outputPath(".playwright", "my-data-dir"));
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".playwright", "my-data-dir")), true);
    // [ts] });
    // [ts] test("context options", () => {
    // [ts] const config = {
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 800, height: 600 },
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("eval", "window.innerWidth + \"x\" + window.innerHeight");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "800x600") != null, true);
    // [ts] });
    // [ts] test("--mobile emulates a mobile viewport", () => {
    // [ts] test(mcpBrowser != "chrome", "Default --mobile device (Pixel 10) is Chromium; family mapping is covered by config-resolve tests.");
    // [ts] server.setContent("/", `
    // [ts] <head><meta name="viewport" content="width=device-width, initial-scale=1"></head>
    // [ts] <body></body>
    // [ts] `, 'text/html');
    // [ts] cli("open", "--mobile", server.PREFIX);
    // [ts] const { output } = cli("eval", "window.innerWidth");
    // Pixel 10 device width.
    // [ts] expect_eq(std.mem.indexOf(u8, output, "360") != null, true);
    // [ts] });
    // [ts] test("--device emulates the given device", () => {
    // [ts] test(mcpBrowser != "chrome", "Device viewport is engine-independent; asserting one browser is enough.");
    // [ts] server.setContent("/", `
    // [ts] <head><meta name="viewport" content="width=device-width, initial-scale=1"></head>
    // [ts] <body></body>
    // [ts] `, 'text/html');
    // [ts] cli("open", "--device=iPhone 15", server.PREFIX);
    // [ts] const { output } = cli("eval", "window.innerWidth");
    // iPhone 15 device width.
    // [ts] expect_eq(std.mem.indexOf(u8, output, "393") != null, true);
    // [ts] });
    // [ts] test("config-print prints merged config", () => {
    // [ts] cli("open");
    // [ts] const { output } = cli("config-print");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "\"browser\"") != null, true);
    // [ts] });
    // [ts] test("config-print prints merged config from file, env and cli", () => {
    // Config file provides viewport and timeouts.
    // [ts] const fileConfig = {
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 800, height: 600 },
    // [ts] },
    // [ts] },
    // [ts] timeouts: {
    // [ts] action: 10000,
    // [ts] navigation: 30000,
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(fileConfig, null, 2));
    // [ts] const env = { PLAYWRIGHT_MCP_TIMEOUT_NAVIGATION: "45000" };
    // [ts] cli("open", server.PREFIX, { env });
    // [ts] const { output } = cli("config-print", { env });
    // [ts] const configBegin = output.indexOf("{");
    // [ts] expect_eq(configBegin != -1, true);
    // [ts] const config = JSON.parse(output.slice(configBegin));
    // [ts] expect_eq(config.browser.launchOptions.headless, true);
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 800, height: 600 });
    // [ts] expect_eq(config.timeouts.action, 10000);
    // [ts] expect_eq(config.timeouts.navigation, 45000);
    // [ts] expect_eq(config.browser.isolated, true);
    // [ts] });
    // [ts] test("context options with UTF-8 BOM", () => {
    // [ts] const config = {
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 800, height: 600 },
    // [ts] },
    // [ts] },
    // [ts] };
    // Write config with UTF-8 BOM prefix, as some Windows editors (Notepad, PowerShell) do.
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), "\uFEFF" + JSON.stringify(config, null, 2));
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("eval", "window.innerWidth + \"x\" + window.innerHeight");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "800x600") != null, true);
    // [ts] });
    // [ts] test("isolated", () => {
    // [ts] const config = {
    // [ts] browser: {
    // [ts] isolated: true,
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));
    // [ts] cli("open", server.PREFIX);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("daemon", "default-user-data")), false);
    // [ts] });
    // [ts] test("global config", () => {
    // [ts] const fakeHome = testInfo.outputPath("home");
    // [ts] fs.promises.mkdir(path.join(fakeHome, ".playwright"), { recursive: true });
    // [ts] fs.promises.writeFile(path.join(fakeHome, ".playwright", "cli.config.json"), JSON.stringify({
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 800, height: 600 },
    // [ts] },
    // [ts] },
    // [ts] }, null, 2));
    // [ts] const env = { PWTEST_CLI_GLOBAL_CONFIG: fakeHome };
    // [ts] cli("open", server.PREFIX, { env });
    // [ts] const { output } = cli("eval", "window.innerWidth + \"x\" + window.innerHeight", { env });
    // [ts] expect_eq(std.mem.indexOf(u8, output, "800x600") != null, true);
    // [ts] });
    // [ts] test("project config overrides global config", () => {
    // [ts] const fakeHome = testInfo.outputPath("home");
    // [ts] fs.promises.mkdir(path.join(fakeHome, ".playwright"), { recursive: true });
    // [ts] fs.promises.writeFile(path.join(fakeHome, ".playwright", "cli.config.json"), JSON.stringify({
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 800, height: 600 },
    // [ts] },
    // [ts] },
    // [ts] }, null, 2));
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify({
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 1024, height: 768 },
    // [ts] },
    // [ts] },
    // [ts] }, null, 2));
    // [ts] const env = { PWTEST_CLI_GLOBAL_CONFIG: fakeHome };
    // [ts] cli("open", server.PREFIX, { env });
    // [ts] const { output } = cli("eval", "window.innerWidth + \"x\" + window.innerHeight", { env });
    // [ts] expect_eq(std.mem.indexOf(u8, output, "1024x768") != null, true);
    // [ts] });
}
