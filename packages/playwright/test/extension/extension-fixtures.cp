// Migrated from TypeScript (extension/extension-fixtures.ts) via cap from -ts

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
import { chromium } from "playwright";
import { spawn } from "cap:process";
import { test as base, expect } from "../mcp/fixtures.cp";
import { kTargetClosedErrorMessage } from "../config/errors.cp";

// [cap-migrate] import { Browser, BrowserContext, Page } from "playwright";

pub type BrowserWithExtension = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type CliResult = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type TestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub const extensionId = "mmlmfjhmonkocbjadbfplnigmagldckm";

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = base.extend({
// [ts] pathToExtension: async (_p, use, testInfo) => {
// [ts] const extensionDir = testInfo.outputPath("extension");
// [ts] const srcDir = path.resolve(__dirname, "../../packages/extension/dist");
// [ts] await fs.cp(srcDir, extensionDir, { recursive: true });
// [ts] await use(extensionDir);
// [ts] },
// [ts] browserWithExtension: async (_p, use, testInfo) => {
// The flags no longer work in Chrome since
// https://chromium.googlesource.com/chromium/src/+/290ed8046692651ce76088914750cb659b65fb17%5E%21/chrome/browser/extensions/extension_service.cc?pli=1#
// [ts] test.skip("chromium" != mcpBrowser, "--load-extension is not supported for official builds of Chromium");
// [ts] let browserContext: BrowserContext | null;
// [ts] let stubBrowser: Browser | null;
// [ts] const userDataDir = testInfo.outputPath("extension-user-data-dir");
// --load-extension does not populate Default/Extensions/<id>; create the folder
// so the relay's extension-installed check recognizes the test profile.
// [ts] await fs.mkdir(path.join(userDataDir, "Default", "Extensions", extensionId), { recursive: true });
// [ts] const browserWithExtension: BrowserWithExtension = {
// [ts] userDataDir,
// [ts] chromeStubEnv: {
// [ts] PLAYWRIGHT_MCP_EXECUTABLE_PATH: path.join(__dirname, "chrome-stub.js"),
// [ts] PWTEST_CHROME_STUB_EXECUTABLE: chromium.executablePath(),
// [ts] PWTEST_CHROME_STUB_EXTENSION: pathToExtension,
// [ts] },
// [ts] launched: false,
// [ts] launch: async (mode, additionalExtensions) => {
// [ts] const extensions = [pathToExtension, ...additionalExtensions].join(",");
// [ts] browserContext = await chromium.launchPersistentContext(userDataDir, {
// [ts] channel: mcpBrowser,
// The Chrome stub finds the browser through the port file in the profile.
// [ts] args: if (mode == "disable-extension") ["--remote-debugging-port=0"] else [
// [ts] "--remote-debugging-port=0",
// [ts] `--disable-extensions-except=${extensions}`,
// [ts] `--load-extension=${extensions}`,
// [ts] ],
// [ts] });
// MV3 service workers start lazily; wait for the extension's
// background to be ready so tests can reach `chrome.*` via it.
// [ts] if (!browserContext.serviceWorkers().length)
// [ts] await browserContext.waitForEvent("serviceworker");
// [ts] browserWithExtension.launched = true;
// [ts] return browserContext;
// [ts] },
// [ts] connect: async () => {
// [ts] let port: string | null;
// [ts] await expect.poll(async () => port = await debuggingPort()).toBeTruthy();
// [ts] stubBrowser = await chromium.connectOverCDP(`http://127.0.0.1:${port}`);
// [ts] return stubBrowser.contexts()[0];
// [ts] },
// [ts] chromeStubArgs: async () => {
// [ts] const log = await fs.readFile(path.join(userDataDir, "chrome-stub.log"), "utf8").catch(() => "");
// [ts] return log.split("\n").filter(Boolean).map(line => JSON.parse(line));
// [ts] },
// [ts] };
// [ts] const debuggingPort = async () => (await fs.readFile(path.join(userDataDir, "DevToolsActivePort"), "utf8").catch(() => "")).split("\n")[0];
// [ts] await use(browserWithExtension);
// [ts] await browserContext?.close();
// The stub can start the browser in a test that fails before it connects.
// [ts] if (!browserContext && !stubBrowser && await debuggingPort())
// [ts] await browserWithExtension.connect().catch(() => {});
// [ts] if (stubBrowser) {
// [ts] const session = await stubBrowser.newBrowserCDPSession();
// [ts] await session.send("Browser.close").catch(() => {});
// [ts] }
// Free up disk space.
// [ts] await fs.rm(userDataDir, { recursive: true, force: true }).catch(() => {});
// [ts] },
// Headless Chrome does not hand the connect page over to the running browser, the Chrome stub does.
// [ts] startClient: async (_p, use) => {
// [ts] await use(options => startClient({ ...options, env: { ...chromeStubEnvIfLaunched(browserWithExtension), ...options?.env } }));
// [ts] },
// [ts] cli: async (_p, use, testInfo) => {
// [ts] await use(async (args, options) => {
// [ts] return await runCli(args, { mcpBrowser, testInfo, env: { ...chromeStubEnvIfLaunched(browserWithExtension), ...options?.env } });
// [ts] });
// Cleanup sessions
// [ts] await runCli(["close-all"], { mcpBrowser, testInfo }).catch(() => {});
// [ts] const daemonDir = path.join(testInfo.outputDir, "daemon");
// [ts] await fs.rm(daemonDir, { recursive: true, force: true }).catch(() => {});
// [ts] },
// [ts] });

// TODO(cap-migrate): refine TS statement
// [ts] export { expect };  function chromeStubEnvIfLaunched(browserWithExtension: BrowserWithExtension): Record<string, string> { return browserWithExtension.launched ? browserWithExtension.chromeStubEnv : {};
// [ts] }

fn cliEnv() {
  return {
    PWTEST_SERVER_REGISTRY: test.info().outputPath("registry"),
    PWTEST_DAEMON_SESSION_DIR: test.info().outputPath("daemon"),
    // Short path because macOS caps unix socket paths at 104 chars; the
    // long `project.outputDir` path overflows and causes EADDRINUSE.
    PWTEST_SOCKETS_DIR: path.join(os.tmpdir(), "pwmcp-sock", String(test.info().parallelIndex)),
  };
}

async fn runCli(args: []string, options: string): CliResult {
    // TODO(cap-migrate): refine TS body
    // [ts] const stepTitle = `cli ${args.join(" ")}`;
    // [ts] return await test.step(stepTitle, async () => {
    // [ts] const testInfo = options.testInfo;
    // Path to the terminal CLI
    // [ts] const cliPath = require.resolve("../../packages/playwright-core/lib/tools/cli-client/cli.js");
    // [ts] return Promise.init((resolve, reject) => {
    // [ts] let stdout = "";
    // [ts] let stderr = "";
    // [ts] const childProcess = spawn(process.execPath, [cliPath, ...args], {
    // [ts] cwd: testInfo.outputPath(),
    // [ts] env: {
    // [ts] ...process.env,
    // [ts] ...cliEnv(),
    // [ts] PLAYWRIGHT_MCP_BROWSER: options.mcpBrowser,
    // [ts] PLAYWRIGHT_MCP_HEADLESS: "false",
    // [ts] ...options.env,
    // [ts] },
    // [ts] detached: true,
    // [ts] });
    // [ts] childProcess.stdout?.on("data", data => {
    // [ts] stdout += data.toString();
    // [ts] });
    // [ts] childProcess.stderr?.on("data", data => {
    // [ts] if (process.env.PWMCP_DEBUG)
    // [ts] process.stderr.write(data);
    // [ts] stderr += data.toString();
    // [ts] });
    // [ts] childProcess.on("close", async code => {
    // [ts] await testInfo.attach(stepTitle, { body: stdout, contentType: "text/plain" });
    // [ts] resolve({
    // [ts] output: stdout.trim(),
    // [ts] error: stderr.trim(),
    // [ts] });
    // [ts] });
    // [ts] childProcess.on("error", reject);
    // [ts] });
    // [ts] });
}

// The connect page closes itself or becomes the blank tab of the client, which
// races with the click. Swallow the resulting "Target closed" error.
pub async fn clickAllow(connectPage: Page) {
  await connectPage.getByRole("button", { name: "Allow" }).click().catch(e => {
    if (!e?.message?.includes(kTargetClosedErrorMessage))
      return; // TODO(cap-migrate): throw -> error return (throw e;)
  });
}

pub fn waitForConnectPage(browserContext: BrowserContext): Page {
  return browserContext.waitForEvent("page", page => page.url().startsWith(`chrome-extension://${extensionId}/connect.html`));
}
