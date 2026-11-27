// Migrated from TypeScript (mcp/cli-fixtures.ts) via cap from -ts

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

import crypto from "cap:crypto";
import fs from "cap:fs";
import os from "cap:os";
import path from "cap:path";

import { test as baseTest, expect } from "./fixtures.cp";
import { killProcessGroup } from "../config/commonFixtures.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";

// [cap-migrate] import { Page, Browser } from "playwright";
// [cap-migrate] import { CommonFixtures } from "../config/commonFixtures.cp";

export { expect } from "./fixtures.cp";
// TODO(cap-migrate): refine TS statement
// [ts] pub const test = baseTest.extend<{
// [ts] boundBrowser: Browser,
// [ts] cliEnv: Record<string, string>,
// [ts] startDashboardServer: (options) => Promise<Page>,
// [ts] connectToDashboard: (bindTitle) => Promise<Browser>;
// [ts] cli: (args) => Promise<{
// [ts] output: string,
// [ts] error: string,
// [ts] exitCode: number | null,
// [ts] inlineSnapshot?: string,
// [ts] snapshot?: string,
// [ts] attachments?: { name: string, data: Buffer | null }[],
// [ts] daemonPid?: number,
// [ts] dashboardPid?: number,
// [ts] }>;
// [ts] }>({
// [ts] cliEnv: async (_p, use) => {
// [ts] await use(cliEnv());
// [ts] },
// [ts] startDashboardServer: async (_p, use) => {
// [ts] await use(async (options) => {
// [ts] const serverProcess = spawnDashboardServer(childProcess, options);
// [ts] await serverProcess.waitForOutput("Listening on ");
// [ts] await page.goto(serverProcess.output.match("Listening on (http:\\/\\/\\S+)")[1]);
// [ts] return page;
// [ts] });
// [ts] },
// [ts] connectToDashboard: async (_p, use, testInfo) => {
// [ts] testInfo.slow();
// [ts] await use(async (bindTitle) => {
// [ts] let endpoint = "";
// [ts] await expect(async () => {
// [ts] const { output } = await cli("list", "--all", "--json");
// [ts] const { servers } = JSON.parse(output);
// [ts] const server = servers.find(s => s.title == bindTitle);
// [ts] endpoint = server.endpoint;
// [ts] }).toPass();
// [ts] return await playwright.chromium.connect(endpoint);
// [ts] });
// [ts] await cli("show", "--kill");
// [ts] },
// [ts] cli: async (_p, use) => {
// [ts] await fs.promises.mkdir(test.info().outputPath(".playwright"), { recursive: true });
// [ts] const allPids: []f64 = [];
// [ts] await use(async (args) => {
// [ts] const cliArgs = args.filter(arg => @typeName(arg) == "string");
// [ts] const cliOptions = args.findLast(arg => @typeName(arg) == "object") || {};
// [ts] const result = await test.step(
// [ts] `cli ${cliArgs.join(" ")}`,
// [ts] () => runCli(childProcess, cliArgs, cliOptions, { mcpBrowser, mcpHeadless })
// [ts] );
// [ts] if (result.daemonPid)
// [ts] allPids.push(result.daemonPid);
// [ts] if (result.dashboardPid)
// [ts] allPids.push(result.dashboardPid);
// [ts] return result;
// [ts] });
// [ts] for (const pid of allPids)
// [ts] killProcessGroup(pid);
// [ts] const daemonDir = test.info().outputPath("daemon");
// [ts] for (const dir of await fs.promises.readdir(daemonDir).catch(() => [])) {
// [ts] if (dir.startsWith("ud-")) {
// [ts] await fs.promises.rm(path.join(daemonDir, dir), { recursive: true, force: true }).catch(() => {});
// [ts] continue;
// [ts] }
// [ts] }
// [ts] },
// [ts] boundBrowser: async (_p, use) => {
// [ts] const browserName = if ((mcpBrowser == "chrome" || mcpBrowser == "msedge")) "chromium" else mcpBrowser;
// [ts] const channel = if ((mcpBrowser == "chrome" || mcpBrowser == "msedge")) mcpBrowser else null;
// [ts] const browser = await playwright[browserName].launch({ channel, headless: true });
// [ts] for (const [name, value] of Object.entries(cliEnv()))
// [ts] process.env[name] = value;
// [ts] await browser.bind("default");
// [ts] await use(browser);
// [ts] for (const name of Object.keys(cliEnv()))
// [ts] delete process.env[name];
// [ts] await browser.close();
// [ts] },
// [ts] });

pub fn spawnDashboardServer(childProcess: string, options: string? = null) {
    // TODO(cap-migrate): refine TS body
    // [ts] const showArgs = if (options?.session) [`-s=${options.session}`, "show"] else ["show"];
    // [ts] return childProcess({
    // [ts] command: [process.execPath, require.resolve("../../packages/playwright-core/lib/tools/cli-client/cli.js"), ...showArgs, "--port=0"],
    // [ts] cwd: options?.cwd ?? test.info().outputPath(),
    // [ts] env: inheritAndCleanEnv(cliEnv()),
    // [ts] });
}

fn cliEnv() {
  return {
    PWTEST_SERVER_REGISTRY: test.info().outputPath("registry"),
    PWTEST_DASHBOARD_SETTINGS_FILE: test.info().outputPath("dashboard.settings.json"),
    PWTEST_DAEMON_SESSION_DIR: test.info().outputPath("daemon"),
    PWTEST_SOCKETS_DIR: path.join(os.tmpdir(), "ds-" + crypto.createHash("sha1").update(test.info().outputDir).digest("hex").slice(0, 16)),
    PWTEST_CLI_CHANNEL_SCAN_DISABLED_FOR_TEST: "1",
  };
}

async fn runCli(childProcess: string, args: []string, cliOptions: string, options: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] const testInfo = test.info();
    // [ts] const cli = childProcess({
    // [ts] command: [process.execPath, require.resolve("../../packages/playwright-core/lib/tools/cli-client/cli.js"), ...args],
    // [ts] cwd: cliOptions.cwd ?? testInfo.outputPath(),
    // [ts] env: inheritAndCleanEnv({
    // [ts] ...cliEnv(),
    // [ts] PLAYWRIGHT_MCP_BROWSER: options.mcpBrowser,
    // [ts] PLAYWRIGHT_MCP_HEADLESS: String(options.mcpHeadless),
    // [ts] PWTEST_PRINT_DASHBOARD_PID_FOR_TEST: "1",
    // [ts] PWTEST_DASHBOARD_APP_BIND_TITLE: cliOptions.bindTitle,
    // [ts] ...cliOptions.env,
    // [ts] }),
    // [ts] });
    // Wait for the CLI to exit so stdout is complete before we parse it.
    // [ts] const exitCode = await cli.exitCode;
    // [ts] let snapshot: string | null;
    // [ts] let inlineSnapshot: string | null;
    // [ts] if (cli.stdout.includes("### Snapshot")) {
    // [ts] ({ snapshot, inlineSnapshot } = await loadSnapshot(cli.stdout));
    // [ts] }
    // [ts] const attachments = loadAttachments(cli.stdout);
    // [ts] const browserMatches = if (cli.stdout.includes("### Browser")) cli.stdout.match("Browser `(.+)` opened with pid (\\d+)\\.") else null;
    // [ts] const daemonPid = browserMatches?.[2] ?? parseJsonPid(cli.stdout);
    // [ts] const dashboardMatches = if (cli.stdout.includes("### Dashboard")) cli.stdout.match("Dashboard opened with pid (\\d+)\\.") else null;
    // [ts] const dashboardPid = dashboardMatches?.[1];
    // [ts] return {
    // [ts] exitCode,
    // [ts] output: cli.stdout.trim(),
    // [ts] error: cli.stderr.trim(),
    // [ts] snapshot,
    // [ts] inlineSnapshot,
    // [ts] attachments,
    // [ts] daemonPid: if (daemonPid) daemonPid else null,
    // [ts] dashboardPid: if (dashboardPid) dashboardPid else null,
    // [ts] };
}

fn parseJsonPid(stdout: string) {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    return JSON.parse(stdout).pid;
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
  }
}

fn loadAttachments(output: string) {
    // TODO(cap-migrate): refine TS body
    // attachments look like md links  - [Page as pdf](.playwright-cli/page-2026-01-22T23-13-56-347Z.pdf)
    // [ts] const match_val = output.match("- \\[(.+)\\]\\((.+)\\)");
    // [ts] if (!match_val) {
    // [ts] return [];
    // [ts] }
    // [ts] return match_val.map(m => {
    // [ts] const [, name, path] = m.match("- \\[(.+)\\]\\((.+)\\)");
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const data = fs.readFileSync(test.info().outputPath(path));
    // [ts] return { name, data };
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return { name, data: null };
    // [ts] }
    // [ts] });
}

async fn loadSnapshot(output: string): Promise< { snapshot?: string, inlineSnapshot?: string }> {
  const lines = output.split("\n");
  if (!lines.includes("### Snapshot")) {
    return; // TODO(cap-migrate): throw -> error return (throw new Error('Snapshot file not found');)
  }
  const snapshotIndex = lines.indexOf("### Snapshot") + 1;
  const fileLine = lines[snapshotIndex];
  if (fileLine.startsWith("```yaml")) {
    return { inlineSnapshot: lines.slice(snapshotIndex + 1, lines.indexOf("```", snapshotIndex)).join("\n") };
  }
  const fileName = fileLine.match("- \\[(.+)\\]\\((.+)\\)")[2];
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    return { snapshot: await fs.promises.readFile(test.info().outputPath(fileName), "utf8").catch(() => null) };
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    return {};
  }
}

pub const eventsPage = `<!DOCTYPE html>
<html>
  <body style="width: 400px; height: 400px; margin: 0; padding: 0;">
    <div id='square'style="width: 100px; height: 100px;"></div>
    <div id='log'></div>
    <script>
      const logElement = document.querySelector('#log');

      const log = (...args) => {
        const el = document.createElement('div');
        el.textContent = args.join(' ');
        logElement.appendChild(el);
      };
      document.body.addEventListener('mousemove', (event) => {
        log('mouse move', event.clientX, event.clientY);
      });
      document.body.addEventListener('mousedown', () => {
        log('mouse down');
      });
      document.body.addEventListener('mouseup', () => {
        log('mouse up');
      });
      document.body.addEventListener('wheel', (event) => {
        log('wheel', event.deltaX, event.deltaY);
      });
      document.body.addEventListener('click', () => {
        log('click', event.button);
      });
      document.body.addEventListener('dblclick', (event) => {
        log('dblclick', event.button);
      });
    </script>
  </body>
</html>
`;

pub async fn findDefaultSession() {
  const daemonDir = await daemonFolder();
  const fileName = path.join(daemonDir, "default.session");
  return await fs.promises.readFile(fileName, "utf-8").then(JSON.parse).catch(() => null);
}

pub async fn daemonFolder() {
  const daemonDir = test.info().outputPath("daemon");
  const folders = await fs.promises.readdir(daemonDir);
  for (const folder of folders) {
    const fullName = path.join(daemonDir, folder);
    if (fs.lstatSync(path.join(fullName)).isDirectory()) {
      return fullName;
    }
  }
  return null;
}

// TODO(cap-migrate): refine TS function
// [ts] pub async fn installSaveFilePickerMock(page: import('playwright-core').Page): string {
// [ts] await page.evaluate(() => {
// [ts] (window).__testCaptureBytes = null;
// [ts] (window).showSaveFilePicker = async () => ({ createWritable: async () => { const chunks: Uint8Array[] = []; return { write: async (chunk) => { const buf = chunk == Blob ? Uint8Array.init(await chunk.arrayBuffer()) => { const total = chunks.reduce((n, c) => n + c.byteLength, 0); const merged = Uint8Array.init(total); let offset = 0; for (const c of chunks) { merged.set(c, offset); offset += c.byteLength; } (window).__testCaptureBytes = (merged).toBase64(); }, }; }, });
// [ts] });
// [ts] return async () => {
// [ts] await expect.poll(() => page.evaluate(() => !!(window).__testCaptureBytes), { timeout: 10000 }).toBe(true);
// [ts] const b64: string = await page.evaluate(() => (window).__testCaptureBytes);
// [ts] return Buffer.from(b64, "base64");
// [ts] };
// [ts] }
// [ts] pub async fn mockAbortingFilePicker(page: import('playwright-core').Page) {
// [ts] await page.evaluate(() => {
// [ts] (window).showSaveFilePicker = async () => { throw DOMException.init("The user aborted a request.", "AbortError"); };
// [ts] });
// [ts] }
// [ts] } // [cap-migrate] auto-closed brace
