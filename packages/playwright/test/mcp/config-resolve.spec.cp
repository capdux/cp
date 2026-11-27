// Migrated from TypeScript (mcp/config-resolve.spec.ts) via cap from -ts

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
import os from "cap:os";
import path from "cap:path";
import { test, expect_eq } from "cap:test";

import { Command } from "commander";

import { test, expect } from "./fixtures.cp";

import { tools } from "../../packages/playwright-core/lib/coreBundle.cp";
// [cap-migrate] import { Config } from "../../packages/playwright-core/src/tools/mcp/config.d.cp";

const { decorateMCPCommand, resolveCLIConfigForCLI, resolveCLIConfigForMCP, isSystemDirectory, outputDir } = tools;

// Parses the command line the same way the mcp server entry point does, without starting the server.
async fn parseCLIOptions(argv: []string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const command = Command.init();
    // [ts] decorateMCPCommand(command);
    // [ts] let options: any;
    // [ts] command.action(o => { options = o; });
    // [ts] command.parseAsync(argv, { from: "user" });
    // [ts] return options;
}

// Empty env to isolate tests from the host environment.
const emptyEnv = {};


// [cap-test] test.describe("browserName and channel", () => {











// [cap-test] });

// ---------------------------------------------------------------------------
// Shared behavior — sandbox
// ---------------------------------------------------------------------------

// [cap-test] test.describe("sandbox", () => {


    // TODO(cap-migrate): refine TS statement
    // [ts] }

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mcpBrowser != "chrome", "Channel-agnostic tests.");
    // [ts] test("no browser option defaults to chromium / chrome", () => {
    // [ts] const config = resolveCLIConfigForMCP({}, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "chromium");
    // [ts] expect_eq(config.browser.launchOptions.channel, "chrome");
    // [ts] });
    // [ts] test("--browser=chrome sets chromium with chrome channel", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "chrome" }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "chromium");
    // [ts] expect_eq(config.browser.launchOptions.channel, "chrome");
    // [ts] });
    // [ts] test("--browser=chromium sets chromium with chrome-for-testing channel", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "chromium" }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "chromium");
    // [ts] expect_eq(config.browser.launchOptions.channel, "chrome-for-testing");
    // [ts] });
    // [ts] test("--browser=firefox sets firefox without channel", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "firefox" }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "firefox");
    // [ts] expect_eq(config.browser.launchOptions.channel == null, true);
    // [ts] });
    // [ts] test("--browser=webkit sets webkit without channel", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "webkit" }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "webkit");
    // [ts] expect_eq(config.browser.launchOptions.channel == null, true);
    // [ts] });
    // [ts] test("--browser=msedge sets chromium with msedge channel", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "msedge" }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "chromium");
    // [ts] expect_eq(config.browser.launchOptions.channel, "msedge");
    // [ts] });
    // [ts] test("config file browserName chromium does not auto-set channel", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ browser: { browserName: "chromium" } }));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "chromium");
    // [ts] expect_eq(config.browser.launchOptions.channel == null, true);
    // [ts] });
    // [ts] test("config file browserName firefox does not set channel", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ browser: { browserName: "firefox" } }));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "firefox");
    // [ts] expect_eq(config.browser.launchOptions.channel == null, true);
    // [ts] });
    // [ts] test("malformed JSON config throws instead of falling back to INI", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41893" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // Trailing comma makes this invalid JSON; it must not be silently parsed as INI.
    // [ts] fs.promises.writeFile(configFile, "{ \"browser\": { \"browserName\": \"firefox\", } }");
    // [ts] expect(resolveCLIConfigForMCP({ config: configFile }, emptyEnv)).rejects.toThrow();
    // [ts] });
    // [ts] test("INI config starting with a section header still parses", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41893" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const configFile = testInfo.outputPath("config.cfg");
    // A leading `[section]` is INI, not JSON — it must not be treated as malformed JSON.
    // [ts] fs.promises.writeFile(configFile, "[browser]\nbrowserName = firefox\n");
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "firefox");
    // [ts] });
    // [ts] test("config file browserName + channel are both preserved", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] browser: { browserName: "chromium", launchOptions: { channel: "msedge" } },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "chromium");
    // [ts] expect_eq(config.browser.launchOptions.channel, "msedge");
    // [ts] });
    // [ts] test("cli --browser overrides config file browserName", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ browser: { browserName: "firefox" } }));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile, browser: "webkit" }, emptyEnv);
    // [ts] expect_eq(config.browser.browserName, "webkit");
    // [ts] });
    // [ts] test("chromium sandbox enabled by default", () => {
    // [ts] const config = resolveCLIConfigForMCP({}, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, true);
    // [ts] });
    // [ts] test("chromium sandbox enabled for chrome channel", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "chrome" }, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, true);
    // [ts] });
    // [ts] test("chromium sandbox for chrome-for-testing channel", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "chromium" }, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.channel, "chrome-for-testing");
    // [ts] if (process.platform == "linux")
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, false);
    // [ts] else {
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, true);
    // [ts] });
    // [ts] test("chromium sandbox disabled for browserName chromium without channel", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42452" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ browser: { browserName: "chromium" } }));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.channel == null, true);
    // [ts] if (process.platform == "linux")
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, false);
    // [ts] else {
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, true);
    // [ts] });
    // [ts] test("sandbox not set for non-chromium browsers", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "firefox" }, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox == null, true);
    // [ts] });
    // [ts] test("explicit --sandbox overrides default", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "chromium", sandbox: true }, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, true);
    // [ts] });
    // [ts] test("explicit --no-sandbox overrides default", () => {
    // [ts] const config = resolveCLIConfigForMCP({ browser: "chrome", sandbox: false }, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, false);
    // [ts] });
    // [ts] test("--sandbox on the command line enables the sandbox", () => {
    // [ts] const config = resolveCLIConfigForMCP(parseCLIOptions(["--browser=chromium", "--sandbox"]), emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.channel, "chrome-for-testing");
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, true);
    // [ts] });
    // [ts] test("--no-sandbox on the command line disables the sandbox", () => {
    // [ts] const config = resolveCLIConfigForMCP(parseCLIOptions(["--browser=chrome", "--no-sandbox"]), emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.chromiumSandbox, false);
    // [ts] });
    // [ts] test("no sandbox flag on the command line leaves the value unset", () => {
    // [ts] const options = parseCLIOptions(["--browser=chrome"]);
    // [ts] expect_eq(options.sandbox == null, true);
    // [ts] });
    // [ts] });
    // ---------------------------------------------------------------------------
    // Shared behavior — timeouts
    // ---------------------------------------------------------------------------
    // [cap-test] test.describe("timeouts", () => {
    // [ts] test("default timeouts", () => {
    // [ts] const config = resolveCLIConfigForMCP({}, emptyEnv);
    // [ts] expect_eq(config.timeouts.action, 5000);
    // [ts] expect_eq(config.timeouts.navigation, 60000);
    // [ts] expect_eq(config.timeouts.expect, 5000);
    // [ts] });
    // [ts] test("config file timeouts override defaults", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ timeouts: { action: 7000 } }));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.timeouts.action, 7000);
    // [ts] expect_eq(config.timeouts.navigation, 60000);
    // [ts] });
    // [cap-test] });
    // ---------------------------------------------------------------------------
    // Shared behavior — viewport
    // ---------------------------------------------------------------------------
    // [cap-test] test.describe("viewport", () => {
    // [ts] test("headless sets default viewport 1280x720", () => {
    // [ts] const config = resolveCLIConfigForMCP({ headless: true }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 1280, height: 720 });
    // [ts] });
    // [ts] test("headed sets viewport to null", () => {
    // [ts] const config = resolveCLIConfigForMCP({ headless: false }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.viewport == null, true);
    // [ts] });
    // [ts] test("explicit viewport is preserved", () => {
    // [ts] const config = resolveCLIConfigForMCP({ headless: true, viewportSize: { width: 800, height: 600 } }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 800, height: 600 });
    // [ts] });
    // [ts] test("config file viewport is preserved", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] browser: { contextOptions: { viewport: { width: 640, height: 480 } } },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile, headless: true }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 640, height: 480 });
    // [ts] });
    // [cap-test] });
    // [ts] test("config file preserves context media preferences", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] colorScheme: "dark",
    // [ts] contrast: "more",
    // [ts] forcedColors: "active",
    // [ts] reducedMotion: "reduce",
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect(config.browser.contextOptions).toMatchObject(fileConfig.browser.contextOptions);
    // [ts] });
    // [cap-test] test.describe("device emulation", () => {
    // [ts] test("--mobile defaults to a Chromium mobile device", () => {
    // [ts] const config = resolveCLIConfigForMCP({ mobile: true }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.isMobile, true);
    // [ts] expect_eq(std.mem.indexOf(u8, config.browser.contextOptions.userAgent, "Pixel 10") != null, true);
    // [ts] });
    // [ts] test("--mobile with a Chromium channel picks a Chromium mobile device", () => {
    // [ts] const config = resolveCLIConfigForMCP({ mobile: true, browser: "msedge" }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.isMobile, true);
    // [ts] expect_eq(std.mem.indexOf(u8, config.browser.contextOptions.userAgent, "Pixel 10") != null, true);
    // [ts] });
    // [ts] test("--mobile with WebKit picks a WebKit mobile device", () => {
    // [ts] const config = resolveCLIConfigForMCP({ mobile: true, browser: "webkit" }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.isMobile, true);
    // [ts] expect_eq(std.mem.indexOf(u8, config.browser.contextOptions.userAgent, "iPhone") != null, true);
    // [ts] });
    // [ts] test("explicit viewport still wins over --mobile", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1834" },
    // [ts] }, () => {
    // [ts] const defaults = resolveCLIConfigForMCP({ mobile: true }, emptyEnv);
    // [ts] const config = resolveCLIConfigForMCP({ mobile: true, viewportSize: { width: 800, height: 600 }, userAgent: "custom-agent" }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.isMobile, true);
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 800, height: 600 });
    // [ts] expect_eq(config.browser.contextOptions.userAgent, "custom-agent");
    // [ts] const nextConfig = resolveCLIConfigForMCP({ mobile: true }, emptyEnv);
    // [ts] expect_eq(nextConfig.browser.contextOptions, defaults.browser.contextOptions);
    // [ts] });
    // [ts] test("--mobile via env var", () => {
    // [ts] const config = resolveCLIConfigForMCP({}, { PLAYWRIGHT_MCP_MOBILE: "1" });
    // [ts] expect_eq(config.browser.contextOptions.isMobile, true);
    // [ts] expect_eq(std.mem.indexOf(u8, config.browser.contextOptions.userAgent, "Pixel 10") != null, true);
    // [ts] });
    // [ts] test("--mobile is rejected with Firefox", () => {
    // [ts] expect(resolveCLIConfigForMCP({ mobile: true, browser: "firefox" }, emptyEnv))
    // [ts] .rejects.toThrow("--mobile is not supported with the Firefox browser.");
    // [ts] });
    // [ts] test("invalid device options are rejected", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1834" },
    // [ts] }, () => {
    // [ts] expect(resolveCLIConfigForMCP({ mobile: true, device: "iPhone 15" }, emptyEnv))
    // [ts] .rejects.toThrow("Cannot use --mobile together with --device");
    // [ts] expect(resolveCLIConfigForMCP({ device: "iPhone 99" }, emptyEnv))
    // [ts] .rejects.toThrow("Unknown device \"iPhone 99\"");
    // [ts] expect(resolveCLIConfigForMCP({ device: "iPhone 99", userAgent: "custom-agent" }, emptyEnv))
    // [ts] .rejects.toThrow("Unknown device \"iPhone 99\"");
    // [ts] });
    // [cap-test] });
    // ---------------------------------------------------------------------------
    // Shared behavior — merge order and config file
    // ---------------------------------------------------------------------------
    // [cap-test] test.describe("merge order", () => {
    // [ts] test("cli overrides config file", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] timeouts: { action: 1000 },
    // [ts] browser: { contextOptions: { viewport: { width: 640, height: 480 } } },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile, timeoutAction: 9999 }, emptyEnv);
    // [ts] expect_eq(config.timeouts.action, 9999);
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 640, height: 480 });
    // [ts] });
    // [ts] test("config file values preserved when cli does not override", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] network: { allowedOrigins: ["https://example.com"] },
    // [ts] browser: { isolated: true },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.network?.allowedOrigins, ["https://example.com"]);
    // [ts] expect_eq(config.browser.isolated, true);
    // [ts] });
    // [ts] test("config file paths resolve against the config dir, cli and env paths against cwd", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1836" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const configDir = testInfo.outputPath("config");
    // [ts] fs.promises.mkdir(configDir);
    // [ts] const configFile = path.join(configDir, "config.json");
    // [ts] const fileConfig: Config = {
    // [ts] outputDir: "artifacts",
    // [ts] browser: {
    // [ts] userDataDir: "profile",
    // [ts] launchOptions: {
    // [ts] executablePath: "browser",
    // [ts] tracesDir: "traces",
    // [ts] downloadsPath: "downloads",
    // [ts] artifactsDir: testInfo.outputPath("absolute-artifacts"),
    // [ts] },
    // [ts] contextOptions: {
    // [ts] storageState: "state.json",
    // [ts] recordHar: { path: "recording.har" },
    // [ts] recordVideo: { dir: "videos" },
    // [ts] clientCertificates: [{ origin: "https://example.com", certPath: "cert.pem", keyPath: "key.pem", pfxPath: "cert.pfx" }],
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.outputDir, path.join(configDir, "artifacts"));
    // [ts] expect: (config.browser) => {
    // [ts] userDataDir: path.join(configDir, "profile"),
    // [ts] launchOptions: {
    // [ts] executablePath: path.join(configDir, "browser"),
    // [ts] tracesDir: path.join(configDir, "traces"),
    // [ts] downloadsPath: path.join(configDir, "downloads"),
    // [ts] artifactsDir: testInfo.outputPath("absolute-artifacts"),
    // [ts] },
    // [ts] contextOptions: {
    // [ts] storageState: path.join(configDir, "state.json"),
    // [ts] recordHar: { path: path.join(configDir, "recording.har") },
    // [ts] recordVideo: { dir: path.join(configDir, "videos") },
    // [ts] clientCertificates: [{
    // [ts] origin: "https://example.com",
    // [ts] certPath: path.join(configDir, "cert.pem"),
    // [ts] keyPath: path.join(configDir, "key.pem"),
    // [ts] pfxPath: path.join(configDir, "cert.pfx"),
    // [ts] }],
    // [ts] },
    // [ts] });
    // [ts] const overridden = resolveCLIConfigForMCP({
    // [ts] config: configFile, outputDir: "cli-output", executablePath: "cli-browser",
    // [ts] }, {
    // [ts] PLAYWRIGHT_MCP_USER_DATA_DIR: "env-profile",
    // [ts] PLAYWRIGHT_MCP_STORAGE_STATE: "env-state.json",
    // [ts] });
    // [ts] expect_eq(overridden.outputDir, path.resolve("cli-output"));
    // [ts] expect_eq(overridden.browser.launchOptions.executablePath, path.resolve("cli-browser"));
    // [ts] expect_eq(overridden.browser.userDataDir, path.resolve("env-profile"));
    // [ts] expect_eq(overridden.browser.contextOptions.storageState, path.resolve("env-state.json"));
    // [ts] });
    // [ts] test("inline storage state in the config file is kept", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const storageState = { cookies: [], origins: [] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ browser: { contextOptions: { storageState } } }));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.storageState, storageState);
    // [ts] });
    // [ts] test("symlinked config file resolves paths next to the real file", () => {
    // [ts] test(process.platform == "win32", "Creating symlinks requires elevated privileges on Windows.");
    // [ts] const realDir = testInfo.outputPath("real");
    // [ts] const linkDir = testInfo.outputPath("link");
    // [ts] fs.promises.mkdir(realDir);
    // [ts] fs.promises.mkdir(linkDir);
    // [ts] fs.promises.writeFile(path.join(realDir, "config.json"), JSON.stringify({ browser: { contextOptions: { storageState: "state.json" } } }));
    // [ts] fs.promises.symlink(path.join(realDir, "config.json"), path.join(linkDir, "config.json"));
    // [ts] const config = resolveCLIConfigForMCP({ config: path.join(linkDir, "config.json") }, emptyEnv);
    // [ts] expect_eq(config.browser.contextOptions.storageState, path.join(realDir, "state.json"));
    // [ts] });
    // [ts] test("env overrides config file", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43038" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ timeouts: { navigation: 1000 }, tabGroup: true }));
    // [ts] const config = resolveCLIConfigForMCP({ ...parseCLIOptions([]), config: configFile }, {
    // [ts] PLAYWRIGHT_MCP_TIMEOUT_NAVIGATION: "45000",
    // [ts] PLAYWRIGHT_MCP_NO_TAB_GROUP: "1",
    // [ts] });
    // [ts] expect_eq(config.timeouts.navigation, 45000);
    // [ts] expect_eq(config.tabGroup, false);
    // [ts] });
    // [ts] test("cli overrides env", () => {
    // [ts] const config = resolveCLIConfigForMCP({ ...parseCLIOptions(["--no-tab-group"]), timeoutAction: 3000, browser: "firefox" }, {
    // [ts] PLAYWRIGHT_MCP_TIMEOUT_ACTION: "9000",
    // [ts] PLAYWRIGHT_MCP_BROWSER: "webkit",
    // [ts] PLAYWRIGHT_MCP_NO_TAB_GROUP: "0",
    // [ts] });
    // [ts] expect_eq(config.timeouts.action, 3000);
    // [ts] expect_eq(config.browser.browserName, "firefox");
    // [ts] expect_eq(config.tabGroup, false);
    // [ts] });
    // [ts] test("file browser.cdpHeaders preserved when env unset", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] browser: {
    // [ts] cdpEndpoint: "ws://example.invalid",
    // [ts] cdpHeaders: { Authorization: "Bearer token-from-file" },
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, emptyEnv);
    // [ts] expect_eq(config.browser.cdpHeaders, { Authorization: "Bearer token-from-file" });
    // [ts] });
    // [ts] test("env browser.cdpHeaders overrides config file and preserves colons in values", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] browser: {
    // [ts] cdpEndpoint: "ws://example.invalid",
    // [ts] cdpHeaders: { Authorization: "Bearer token-from-file" },
    // [ts] },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile }, {
    // [ts] PLAYWRIGHT_MCP_CDP_HEADERS: "X-Forwarded-Proto: value:with:colons",
    // [ts] });
    // [ts] expect_eq(config.browser.cdpHeaders, { "X-Forwarded-Proto": "value:with:colons" });
    // [ts] });
    // [cap-test] });
    // ---------------------------------------------------------------------------
    // Shared behavior — validation
    // ---------------------------------------------------------------------------
    // [cap-test] test.describe("validation", () => {
    // [ts] test("isolated + userDataDir throws", () => {
    // [ts] expect(resolveCLIConfigForMCP({ isolated: true, userDataDir: "/tmp/data" }, emptyEnv))
    // [ts] .rejects.toThrow("Browser userDataDir is not supported in isolated mode.");
    // [ts] });
    // [cap-test] });
    // ---------------------------------------------------------------------------
    // Output directory — fallback for unsuitable cwd, throw on explicit system dir
    // ---------------------------------------------------------------------------
    // [ts] test("outputDir checks Windows directory writability once per session", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44138" },
    // [ts] }, (_p, testInfo) => {
    // [ts] test(process.platform != "win32", "Windows directory permissions require a write probe.");
    // [ts] const cwd = testInfo.outputPath("workspace");
    // [ts] fs.mkdirSync(cwd);
    // [ts] const options = { config: {}, cwd };
    // [ts] const writeFile = mock.method(fs, "writeFileSync");
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(outputDir(options), path.join(cwd, ".playwright-mcp"));
    // [ts] expect_eq(outputDir(options), path.join(cwd, ".playwright-mcp"));
    // [ts] expect_eq(writeFile.mock.callCount(), 1);
    // [ts] expect_eq(outputDir({ config: {}, cwd }), path.join(cwd, ".playwright-mcp"));
    // [ts] expect_eq(writeFile.mock.callCount(), 2);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] writeFile.mock.restore();
    // [ts] }
    // [ts] });
    // [cap-test] test.describe("outputDir", () => {
    // [ts] test(process.platform == "win32", "POSIX-specific cases");
    // [ts] test("falls back to tmpdir when cwd is /", () => {
    // [ts] const result = outputDir({ config: {}, cwd: "/" });
    // [ts] expect_eq(result, path.join(os.tmpdir(), ".playwright-mcp"));
    // [ts] });
    // [ts] test("uses cwd-relative .playwright-mcp for normal cwd", () => {
    // [ts] const cwd = testInfo.outputPath("workspace");
    // [ts] fs.mkdirSync(cwd, { recursive: true });
    // [ts] const result = outputDir({ config: {}, cwd });
    // [ts] expect_eq(result, path.join(cwd, ".playwright-mcp"));
    // [ts] });
    // [ts] test("uses .playwright-cli when skillMode is set", () => {
    // [ts] const cwd = testInfo.outputPath("workspace");
    // [ts] fs.mkdirSync(cwd, { recursive: true });
    // [ts] const result = outputDir({ config: { skillMode: true }, cwd });
    // [ts] expect_eq(result, path.join(cwd, ".playwright-cli"));
    // [ts] });
    // [ts] test("skillMode falls back to tmpdir/.playwright-cli when cwd is /", () => {
    // [ts] const result = outputDir({ config: { skillMode: true }, cwd: "/" });
    // [ts] expect_eq(result, path.join(os.tmpdir(), ".playwright-cli"));
    // [ts] });
    // [ts] test("explicit outputDir wins regardless of cwd", () => {
    // [ts] const explicit = testInfo.outputPath("explicit");
    // [ts] const result = outputDir({ config: { outputDir: explicit }, cwd: "/" });
    // [ts] expect_eq(result, explicit);
    // [ts] });
    // [ts] test("falls back to tmpdir when cwd is not writable", () => {
    // [ts] const cwd = testInfo.outputPath("readonly");
    // [ts] fs.mkdirSync(cwd, { recursive: true });
    // [ts] fs.chmodSync(cwd, 0o500);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const result = outputDir({ config: {}, cwd });
    // [ts] expect_eq(result, path.join(os.tmpdir(), ".playwright-mcp"));
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] fs.chmodSync(cwd, 0o700);
    // [ts] }
    // [ts] });
    // [ts] test("isSystemDirectory detects /", () => {
    // [ts] expect_eq(isSystemDirectory("/"), true);
    // [ts] expect_eq(isSystemDirectory("/tmp"), false);
    // [ts] expect_eq(isSystemDirectory(os.homedir()), false);
    // [ts] });
    // [ts] test("resolveCLIConfigForMCP throws when --output-dir is /", () => {
    // [ts] expect(resolveCLIConfigForMCP({ outputDir: "/" }, emptyEnv))
    // [ts] .rejects.toThrow("--output-dir cannot point to a system directory");
    // [ts] });
    // [cap-test] });
    // ---------------------------------------------------------------------------
    // MCP-specific: headless platform default, timeout CLI options
    // ---------------------------------------------------------------------------
    // [cap-test] test.describe("resolveCLIConfigForMCP", () => {
    // [ts] test("headless defaults based on platform", () => {
    // [ts] const config = resolveCLIConfigForMCP({}, emptyEnv);
    // [ts] expect_eq(config.browser.launchOptions.headless != null, true);
    // [ts] });
    // [ts] test("cli timeout overrides defaults", () => {
    // [ts] const config = resolveCLIConfigForMCP({ timeoutAction: 10000, timeoutNavigation: 30000, idleTimeout: 60000 }, emptyEnv);
    // [ts] expect_eq(config.timeouts.action, 10000);
    // [ts] expect_eq(config.timeouts.navigation, 30000);
    // [ts] expect_eq(config.timeouts.expect, 5000);
    // [ts] expect_eq(config.timeouts.idle, 60000);
    // [ts] });
    // [ts] test("idle timeout is unset by default and comes from the config file or env", () => {
    // [ts] expect_eq((resolveCLIConfigForMCP({}, emptyEnv)).timeouts.idle == null, true);
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ timeouts: { idle: 1000 } }));
    // [ts] expect_eq((resolveCLIConfigForMCP({ config: configFile }, emptyEnv)).timeouts.idle, 1000);
    // [ts] expect_eq((resolveCLIConfigForMCP({ config: configFile }, { ...emptyEnv, PLAYWRIGHT_MCP_IDLE_TIMEOUT: "2000" })).timeouts.idle, 2000);
    // [ts] });
    // [ts] test("cli timeout overrides config file timeout", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ timeouts: { action: 1000 } }));
    // [ts] const config = resolveCLIConfigForMCP({ config: configFile, timeoutAction: 9999 }, emptyEnv);
    // [ts] expect_eq(config.timeouts.action, 9999);
    // [ts] });
    // [cap-test] });
    // ---------------------------------------------------------------------------
    // CLI-specific: headless, isolated, userDataDir, extension, skillMode, snapshot
    // ---------------------------------------------------------------------------
    // [ts] fn resolveCLI(profilesDir: string, sessionName: string, options: string) {
    // [ts] return resolveCLIConfigForCLI(profilesDir, sessionName, options, emptyEnv);
    // [ts] }
    // [cap-test] test.describe("resolveCLIConfigForCLI - headless and viewport", () => {
    // [ts] test("headless by default", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", {});
    // [ts] expect_eq(config.browser.launchOptions.headless, true);
    // [ts] });
    // [ts] test("--headed sets headless=false", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { headed: true });
    // [ts] expect_eq(config.browser.launchOptions.headless, false);
    // [ts] });
    // [ts] test("headless viewport defaults to 1280x720", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", {});
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 1280, height: 720 });
    // [ts] });
    // [ts] test("headed viewport defaults to null", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { headed: true });
    // [ts] expect_eq(config.browser.contextOptions.viewport == null, true);
    // [ts] });
    // [ts] test("config file viewport is preserved", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] browser: { contextOptions: { viewport: { width: 640, height: 480 } } },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { config: configFile });
    // [ts] expect_eq(config.browser.contextOptions.viewport, { width: 640, height: 480 });
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe("resolveCLIConfigForCLI - isolated and userDataDir", () => {
    // [ts] test("defaults to isolated when no profile, persistent, userDataDir, or remoteEndpoint", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", {});
    // [ts] expect_eq(config.browser.isolated, true);
    // [ts] });
    // [ts] test("not isolated when --profile is set", () => {
    // [ts] const profileDir = testInfo.outputPath("my-profile");
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { profile: profileDir });
    // [ts] expect_eq(config.browser.isolated, false);
    // [ts] expect_eq(config.browser.userDataDir, profileDir);
    // [ts] });
    // [ts] test("not isolated when --persistent is set", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { persistent: true });
    // [ts] expect_eq(config.browser.isolated, false);
    // [ts] });
    // [ts] test("not isolated when --attach is set", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { endpoint: "ws://localhost:1234" });
    // [ts] expect_eq(config.browser.isolated, false);
    // [ts] });
    // [ts] test("not isolated when config file sets userDataDir", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const userDataDir = testInfo.outputPath("custom-data");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ browser: { userDataDir } }));
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { config: configFile });
    // [ts] expect_eq(config.browser.isolated, false);
    // [ts] expect_eq(config.browser.userDataDir, userDataDir);
    // [ts] });
    // [ts] test("auto userDataDir uses daemonProfilesDir with session and browser token", () => {
    // [ts] const profilesDir = testInfo.outputPath("profiles");
    // [ts] const config = resolveCLI(profilesDir, "mysession", { persistent: true, browser: "chrome" });
    // [ts] expect_eq(config.browser.userDataDir, path.resolve(profilesDir, "ud-mysession-chrome"));
    // [ts] });
    // [ts] test("auto userDataDir uses browserName when no channel", () => {
    // [ts] const profilesDir = testInfo.outputPath("profiles");
    // [ts] const config = resolveCLI(profilesDir, "default", { persistent: true, browser: "firefox" });
    // [ts] expect_eq(config.browser.userDataDir, path.resolve(profilesDir, "ud-default-firefox"));
    // [ts] });
    // [ts] test("auto userDataDir uses undefined token when no browser specified", () => {
    // [ts] const profilesDir = testInfo.outputPath("profiles");
    // [ts] const config = resolveCLI(profilesDir, "default", { persistent: true });
    // [ts] expect_eq(config.browser.userDataDir, path.resolve(profilesDir, "ud-default-chrome"));
    // [ts] });
    // [ts] test("no auto userDataDir when isolated", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", {});
    // [ts] expect_eq(config.browser.isolated, true);
    // [ts] expect_eq(config.browser.userDataDir == null, true);
    // [ts] });
    // [ts] test("no auto userDataDir when --profile is set", () => {
    // [ts] const profileDir = testInfo.outputPath("my-profile");
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { profile: profileDir });
    // [ts] expect_eq(config.browser.userDataDir, profileDir);
    // [ts] });
    // [ts] test("no auto userDataDir when remoteEndpoint is set", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { attach: "ws://localhost:1234" });
    // [ts] expect_eq(config.browser.userDataDir == null, true);
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe("resolveCLIConfigForCLI - timeouts", () => {
    // [ts] test("config file timeouts override defaults", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ timeouts: { action: 7000 } }));
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { config: configFile });
    // [ts] expect_eq(config.timeouts.action, 7000);
    // [ts] expect_eq(config.timeouts.navigation, 60000);
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe("resolveCLIConfigForCLI - skillMode and snapshotMode", () => {
    // [ts] test("skillMode is always set", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", {});
    // [ts] expect_eq(config.skillMode, true);
    // [ts] });
    // [ts] test("snapshot mode is full", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", {});
    // [ts] expect_eq(config.snapshot?.mode, "full");
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe("resolveCLIConfigForCLI - config file discovery", () => {
    // [ts] test("explicit config file is used", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ timeouts: { action: 3000 } }));
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { config: configFile });
    // [ts] expect_eq(config.timeouts.action, 3000);
    // [ts] expect_eq(config.configFile, configFile);
    // [ts] });
    // [ts] test("cli overrides env", () => {
    // [ts] const config = resolveCLIConfigForCLI(testInfo.outputPath("profiles"), "default", { browser: "firefox" }, {
    // [ts] PLAYWRIGHT_MCP_BROWSER: "webkit",
    // [ts] });
    // [ts] expect_eq(config.browser.browserName, "firefox");
    // [ts] });
    // [ts] test("merge order: config file < daemon overrides", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] browser: { browserName: "firefox" },
    // [ts] timeouts: { action: 1000 },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { config: configFile, browser: "webkit" });
    // [ts] expect_eq(config.browser.browserName, "webkit");
    // [ts] expect_eq(config.timeouts.action, 1000);
    // [ts] });
    // [ts] test("config file values preserved when cli does not override", () => {
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] const fileConfig: Config = {
    // [ts] network: { allowedOrigins: ["https://example.com"] },
    // [ts] };
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(fileConfig));
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { config: configFile });
    // [ts] expect_eq(config.network?.allowedOrigins, ["https://example.com"]);
    // [ts] });
    // [ts] test("config file resolves relative paths against its directory", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1836" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const configDir = testInfo.outputPath("config");
    // [ts] fs.promises.mkdir(configDir);
    // [ts] const configFile = path.join(configDir, "config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({ outputDir: "artifacts", browser: { userDataDir: "profile" } }));
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { config: configFile });
    // [ts] expect_eq(config.outputDir, path.join(configDir, "artifacts"));
    // [ts] expect_eq(config.browser.userDataDir, path.join(configDir, "profile"));
    // [ts] });
    // [ts] test("global config resolves relative paths against its directory", () => {
    // [ts] const fakeHome = testInfo.outputPath("home");
    // [ts] const globalConfigDir = path.join(fakeHome, ".playwright");
    // [ts] fs.promises.mkdir(globalConfigDir, { recursive: true });
    // [ts] fs.promises.writeFile(path.join(globalConfigDir, "cli.config.json"), JSON.stringify({ outputDir: "artifacts", browser: { userDataDir: "profile" } }));
    // [ts] const config = resolveCLIConfigForCLI(testInfo.outputPath("profiles"), "default", {}, { PWTEST_CLI_GLOBAL_CONFIG: fakeHome });
    // [ts] expect_eq(config.outputDir, path.join(globalConfigDir, "artifacts"));
    // [ts] expect_eq(config.browser.userDataDir, path.join(globalConfigDir, "profile"));
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe("resolveCLIConfigForCLI - extension", () => {
    // [ts] test("--extension disables isolated", () => {
    // [ts] const config = resolveCLI(testInfo.outputPath("profiles"), "default", { extension: true });
    // [ts] expect_eq(config.extension, true);
    // [ts] expect_eq(config.browser.isolated, false);
    // [ts] });
    // [cap-test] });
    // [ts] }
}
