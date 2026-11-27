// Migrated from TypeScript (mcp/cli-session.spec.ts) via cap from -ts

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
import os from "cap:os";
import path from "cap:path";
import { test, expect, daemonFolder } from "./cli-fixtures.cp";
import { killProcessGroup } from "../config/commonFixtures.cp";
import playwright from "../../packages/playwright-core.cp";

fn isProcessAlive(pid: f64): bool {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    process.kill(pid, 0);
    return true;
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    return false;
  }
}













// TODO(cap-migrate): refine TS statement
// [ts] for (const envName of ["XDG_RUNTIME_DIR", "TMPDIR"]) {
// [ts] test(`session stops when socket directory in ${envName} disappears`, () => {
// [ts] test(process.platform == "win32");
// Keep this short because macOS limits Unix socket paths to 103 bytes.
// [ts] const tempDir = fs.promises.mkdtemp(path.join(os.tmpdir(), "p-"));
// [ts] const removedTempDir = tempDir + ".removed";
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] const { daemonPid } = cli("open", server.HELLO_WORLD, {
// [ts] env: {
// [ts] PWTEST_SOCKETS_DIR: "",
// [ts] XDG_RUNTIME_DIR: "",
// [ts] [envName]: tempDir,
// [ts] },
// [ts] });
// [ts] expect_eq(daemonPid != null, true);
// [ts] expect_eq(isProcessAlive(daemonPid), true);
// [ts] fs.promises.rename(tempDir, removedTempDir);
// [ts] expect.poll(() => isProcessAlive(daemonPid)).toBe(false);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] fs.promises.rm(tempDir, { force: true, recursive: true });
// [ts] fs.promises.rm(removedTempDir, { force: true, recursive: true });
// [ts] }
// [ts] });
// [ts] }









// [cap-test] test.describe("browser server", () => {
  test.beforeEach((_p, testInfo) => {
    test(!["chrome", "chromium", "webkit", "firefox"].includes(mcpBrowser));
    process.env.PWTEST_SERVER_REGISTRY = testInfo.outputPath("registry");
  });





// [cap-test] });

const version = "v" + require("../../packages/playwright-core/package.json").version;

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("list", () => {
    // [ts] const { output: emptyOutput } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, emptyOutput, "(no browsers)") != null, true);
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output: listOutput } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, "### Browsers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, "- default:") != null, true);
    // [ts] });
    // [ts] test("list should remove stale registry entries and exit", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/549" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const registryDir = testInfo.outputPath("registry");
    // [ts] fs.promises.mkdir(registryDir, { recursive: true });
    // [ts] for (const guid of ["stale-1", "stale-2"]) {
    // [ts] fs.promises.writeFile(path.join(registryDir, guid), JSON.stringify({
    // [ts] playwrightVersion: "0.0.0",
    // [ts] title: guid,
    // [ts] browser: { guid, browserName: "chromium", launchOptions: {} },
    // [ts] endpoint: testInfo.outputPath(guid + ".sock"),
    // [ts] }));
    // [ts] }
    // [ts] const watchLog = testInfo.outputPath("watch.log");
    // [ts] const preload = testInfo.outputPath("log-registry-watchers.cjs");
    // [ts] fs.promises.writeFile(preload, `
    // [ts] const fs = require('fs');
    // [ts] const watch = fs.watch;
    // [ts] fs.watch = (...args) => {
    // [ts] if (String(args[0]).startsWith(process.env.PWTEST_SERVER_REGISTRY))
    // [ts] fs.appendFileSync(${JSON.stringify(watchLog)}, args[0] + '\\n');
    // [ts] return watch(...args);
    // [ts] };
    // [ts] `);
    // [ts] const { output } = cli("list", { env: { NODE_OPTIONS: `--require="${preload.replaceAll("\\", "/")}"` } });
    // [ts] expect_eq(std.mem.indexOf(u8, output, "(no browsers)") != null, true);
    // [ts] expect_eq(fs.promises.readdir(registryDir), []);
    // A file watcher on the registry can keep the process alive after the stale entries are removed.
    // [ts] expect_eq(fs.existsSync(watchLog), false);
    // [ts] });
    // [ts] test("close", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("close");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Browser 'default' closed`) != null, true);
    // [ts] const { output: listOutput } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, "(no browsers)") != null, true);
    // [ts] });
    // [ts] test("close lets the browser exit on its own", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/475" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const logFile = testInfo.outputPath("daemon.log");
    // [ts] const { daemonPid } = cli("open", server.HELLO_WORLD, { env: { DEBUG: "pw:browser", DEBUG_FILE: logFile } });
    // [ts] cli("close");
    // [ts] expect.poll(() => isProcessAlive(daemonPid)).toBe(false);
    // [ts] const log = fs.promises.readFile(logFile, "utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, log, "<gracefully close start>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, log, "<forcefully close>") == null, true);
    // [ts] });
    // [ts] test("idle timeout shuts the session down", () => {
    // [ts] cli("open", "--idle-timeout=3000", server.HELLO_WORLD);
    // [ts] const { output } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "- default:") != null, true);
    // Firefox on Windows takes several seconds to shut down after the idle timer fires.
    // [ts] expect.poll(() => (cli("list")).output, { timeout: 20_000 }).toContain("(no browsers)");
    // [ts] const { output: afterOutput } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, afterOutput, `The browser 'default' is not open, please run open first`) != null, true);
    // [ts] });
    // [ts] test("close named session", () => {
    // [ts] cli("-s", "mysession", "open", server.HELLO_WORLD);
    // [ts] const { output } = cli("-s", "mysession", "close");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Browser 'mysession' closed`) != null, true);
    // [ts] });
    // [ts] test("close non-running session", () => {
    // [ts] const { output } = cli("-s", "nonexistent", "close");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Browser 'nonexistent' is not open.`) != null, true);
    // [ts] });
    // [ts] test("close-all", () => {
    // [ts] cli("-s", "session1", "open", server.HELLO_WORLD);
    // [ts] cli("-s", "session2", "open", server.HELLO_WORLD);
    // [ts] const { output: listBefore } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listBefore, "session1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listBefore, "session2") != null, true);
    // [ts] cli("close-all");
    // [ts] const { output: listAfter } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listAfter, "session1") == null, true);
    // [ts] });
    // [ts] test("delete-data", () => {
    // [ts] cli("open", server.HELLO_WORLD, "--persistent");
    // [ts] const dataDir = path.resolve(daemonFolder(), "ud-default-" + mcpBrowserNormalized);
    // [ts] expect_eq(fs.existsSync(dataDir), true);
    // [ts] const { output } = cli("delete-data");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Deleted user data for browser 'default'.`) != null, true);
    // [ts] expect_eq(fs.existsSync(dataDir), false);
    // [ts] });
    // [ts] test("delete-data named session", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/499" },
    // [ts] }, (_p) => {
    // [ts] cli("-s", "mysession-test", "open", server.HELLO_WORLD, "--persistent");
    // [ts] cli("-s", "mysession-test", "localstorage-set", "k", "data-of-mysession-test");
    // [ts] cli("-s", "mysession-test", "close");
    // [ts] cli("-s", "mysession", "open", server.HELLO_WORLD, "--persistent");
    // [ts] cli("-s", "mysession", "close");
    // [ts] const dataDir = path.resolve(daemonFolder(), "ud-mysession-" + mcpBrowserNormalized);
    // [ts] expect_eq(fs.existsSync(dataDir), true);
    // [ts] const { output } = cli("-s", "mysession", "delete-data");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Deleted user data for browser 'mysession'.`) != null, true);
    // [ts] expect_eq(fs.existsSync(dataDir), false);
    // [ts] cli("-s", "mysession-test", "open", server.HELLO_WORLD, "--persistent");
    // [ts] const { output: storageOutput } = cli("-s", "mysession-test", "localstorage-get", "k");
    // [ts] expect_eq(std.mem.indexOf(u8, storageOutput, "data-of-mysession-test") != null, true);
    // [ts] });
    // [ts] test("delete-data non-existent session", () => {
    // [ts] const { output } = cli("-s", "nonexistent", "delete-data");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `No user data found for browser 'nonexistent'.`) != null, true);
    // [ts] });
    // [ts] test("session stops when browser exits", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output: listBefore } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listBefore, "default") != null, true);
    // Close the browser - this will cause the daemon to exit so the command may fail
    // [ts] cli("run-code", "() => page.context().browser().close()").catch(() => {});
    // [ts] cli("close");
    // [ts] const { output: listAfter } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listAfter, "(no browsers)") != null, true);
    // [ts] });
    // [ts] test("session reopen with different config", () => {
    // [ts] const config = { browser: { contextOptions: { viewport: { width: 700, height: 500 } } } };
    // [ts] const configPath = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configPath, JSON.stringify(config, null, 2));
    // [ts] {
    // [ts] cli("open", server.HELLO_WORLD, "--config=" + configPath);
    // [ts] const { output } = cli("eval", "window.innerWidth + \"x\" + window.innerHeight");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "700x500") != null, true);
    // [ts] }
    // [ts] {
    // Reopen without config should use default viewport
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("eval", "window.innerWidth + \"x\" + window.innerHeight");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "1280x720") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("workspace isolation - sessions in different workspaces are isolated", () => {
    // Create two separate workspaces with their own daemon dirs
    // [ts] const workspace1 = testInfo.outputPath("workspace1");
    // [ts] const workspace2 = testInfo.outputPath("workspace2");
    // [ts] fs.promises.mkdir(workspace1, { recursive: true });
    // [ts] fs.promises.mkdir(workspace2, { recursive: true });
    // [ts] cli("install", { cwd: workspace1 });
    // [ts] cli("install", { cwd: workspace2 });
    // [ts] expect_eq(fs.existsSync(path.join(workspace1, ".playwright")), true);
    // [ts] expect_eq(fs.existsSync(path.join(workspace2, ".playwright")), true);
    // Open sessions in both workspaces
    // [ts] cli("open", server.HELLO_WORLD, { cwd: workspace1 });
    // [ts] cli("open", server.HELLO_WORLD, { cwd: workspace2 });
    // [ts] const { output: list1 } = cli("list", { cwd: workspace1 });
    // [ts] expect_eq(std.mem.indexOf(u8, list1, "default") != null, true);
    // [ts] const { output: list2 } = cli("list", { cwd: workspace2 });
    // [ts] expect_eq(std.mem.indexOf(u8, list2, " default") != null, true);
    // [ts] cli("close", { cwd: workspace1 });
    // [ts] const { output: list1After } = cli("list", { cwd: workspace1 });
    // [ts] expect_eq(std.mem.indexOf(u8, list1After, "### Browsers") == null, true);
    // [ts] const { output: list2After } = cli("list", { cwd: workspace2 });
    // [ts] expect_eq(std.mem.indexOf(u8, list2After, "default") != null, true);
    // [ts] cli("close", { cwd: workspace2 });
    // [ts] });
    // [ts] test("list --all lists sessions from all workspaces", () => {
    // Create two separate workspaces with their own daemon dirs
    // [ts] const workspace1 = testInfo.outputPath("workspace1");
    // [ts] const workspace2 = testInfo.outputPath("workspace2");
    // [ts] const workspace3 = testInfo.outputPath("workspace3");
    // [ts] fs.promises.mkdir(workspace1, { recursive: true });
    // [ts] fs.promises.mkdir(workspace2, { recursive: true });
    // [ts] fs.promises.mkdir(workspace3, { recursive: true });
    // [ts] cli("install", { cwd: workspace1 });
    // [ts] cli("install", { cwd: workspace2 });
    // [ts] cli("install", { cwd: workspace3 });
    // [ts] cli("-s", "session1", "open", server.HELLO_WORLD, { cwd: workspace1 });
    // [ts] cli("-s", "session2", "open", server.HELLO_WORLD, { cwd: workspace2 });
    // [ts] const session3 = cli("-s", "session3", "open", server.HELLO_WORLD, { cwd: workspace3 });
    // List all sessions from workspace1
    // [ts] const { output: allList } = cli("list", "--all", { cwd: workspace1 });
    // Should include both workspace folders and sessions
    // [ts] expect_eq(std.mem.indexOf(u8, allList, "/:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allList, "session1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allList, ".." + path.sep + "workspace2:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allList, "session2") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allList, ".." + path.sep + "workspace3:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, allList, "session3") != null, true);
    // [ts] const rootDir = test.info().outputPath("daemon");
    // [ts] const dirs = fs.promises.readdir(rootDir);
    // [ts] const getSessionFiles = () => (joinAll(dirs.map(dir => fs.promises.readdir(path.join(rootDir, dir))))).flat(); // migrated: Promise.all -> joinAll
    // [ts] const sessionFilesBefore = getSessionFiles();
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesBefore, "session1.session") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesBefore, "session2.session") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesBefore, "session3.session") != null, true);
    // [ts] cli("-s", "session1", "close", { cwd: workspace1 });
    // [ts] const { output: listTwo } = cli("list", "--all", { cwd: workspace2 });
    // [ts] expect_eq(std.mem.indexOf(u8, listTwo, "workspace1") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listTwo, "session1") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listTwo, "/:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listTwo, "session2") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listTwo, "workspace3") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listTwo, "session3") != null, true);
    // [ts] const sessionFilesAfterClose = getSessionFiles();
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesAfterClose, "session1.session") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesAfterClose, "session2.session") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesAfterClose, "session3.session") != null, true);
    // [ts] killProcessGroup(session3.daemonPid);
    // [ts] const { output: listOne } = cli("list", "--all", { cwd: workspace2 });
    // [ts] expect_eq(std.mem.indexOf(u8, listOne, "workspace1") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOne, "session1") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOne, "/:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOne, "session2") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOne, "workspace3") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOne, "session3") == null, true);
    // [ts] const sessionFilesAfterList = getSessionFiles();
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesAfterList, "session1.session") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesAfterList, "session2.session") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sessionFilesAfterList, "session3.session") == null, true);
    // [ts] });
    // [ts] test("list --all and a failed attach list browser profiles available to attach", () => {
    // [ts] test(process.platform != "linux", "CHROME_CONFIG_HOME changes the browser folders only on Linux");
    // [ts] const configHome = testInfo.outputPath("config");
    // [ts] const userDataDir = path.join(configHome, "google-chrome");
    // [ts] const extensionId = "mmlmfjhmonkocbjadbfplnigmagldckm";
    // [ts] fs.promises.mkdir(path.join(userDataDir, "Default", "Extensions", extensionId), { recursive: true });
    // [ts] fs.promises.mkdir(path.join(userDataDir, "Profile 1"), { recursive: true });
    // [ts] fs.promises.writeFile(path.join(userDataDir, "Local State"), JSON.stringify({
    // [ts] profile: { info_cache: { "Default": { name: "Work" }, "Profile 1": { name: "Profile 2", gaia_name: "Jane Doe", user_name: "jane@example.com" } } },
    // [ts] }));
    // [ts] const env = { CHROME_CONFIG_HOME: configHome, PWTEST_CLI_CHANNEL_SCAN_DISABLED_FOR_TEST: "" };
    // [ts] const listing = `### Browsers available to attach
    // [ts] - chrome:
    // [ts] - data-dir: ${userDataDir}
    // [ts] - attach (remote debugging): enable at chrome://inspect/#remote-debugging
    // [ts] ### Browser profiles available to attach
    // [ts] - chrome - Work:
    // [ts] - profile-dir: ${path.join(userDataDir, 'Default')}
    // [ts] - attach (extension): \`playwright-cli attach --cdp="extension://chrome/Default"\`
    // [ts] - chrome - Jane Doe (jane@example.com):
    // [ts] - profile-dir: ${path.join(userDataDir, 'Profile 1')}
    // [ts] - attach (extension): install at https://chromewebstore.google.com/detail/playwright-extension/${extensionId}`;
    // [ts] const list = cli("list", "--all", { env });
    // [ts] expect_eq(std.mem.indexOf(u8, list.output, listing) != null, true);
    // [ts] const cdp = cli("attach", "--cdp=remote-debugging://chrome", { env });
    // [ts] expect_eq(cdp.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, cdp.error, `DevToolsActivePort file not found at ${path.join(userDataDir, "DevToolsActivePort")}`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, cdp.error, listing) != null, true);
    // [ts] const extension = cli("attach", "--cdp=extension://chrome/Profile 1", { env });
    // [ts] expect_eq(extension.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, extension.error, `Playwright Extension not found in "${path.join(userDataDir, "Profile 1")}"`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, extension.error, listing) != null, true);
    // [ts] });
    // [ts] test("newer client with older daemon is compatible", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { exitCode } = cli("eval", "1+1", { env: { PLAYWRIGHT_CLI_VERSION_FOR_TEST: "9.9.9" } });
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("older client with newer daemon fails with version mismatch error", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output, error, exitCode } = cli("eval", "1+1", { env: { PLAYWRIGHT_CLI_VERSION_FOR_TEST: "0.0.1" } });
    // [ts] expect_eq(exitCode != 0, true);
    // [ts] const fullOutput = output + error;
    // [ts] expect_eq(std.mem.indexOf(u8, fullOutput, "Client is v0.0.1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fullOutput, "playwright-cli open") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fullOutput, "to restart the browser session") != null, true);
    // [ts] });
    // [ts] test("older client with newer daemon - named session includes session name in error", () => {
    // [ts] cli("-s", "mysession", "open", server.HELLO_WORLD);
    // [ts] const { output, error, exitCode } = cli("-s", "mysession", "eval", "1+1", { env: { PLAYWRIGHT_CLI_VERSION_FOR_TEST: "0.0.1" } });
    // [ts] expect_eq(exitCode != 0, true);
    // [ts] const fullOutput = output + error;
    // [ts] expect_eq(std.mem.indexOf(u8, fullOutput, "Client is v0.0.1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fullOutput, `session 'mysession'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fullOutput, "playwright-cli -s=mysession open") != null, true);
    // [ts] });
    // [ts] test("older client with newer daemon - list shows incompatible warning", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("list", { env: { PLAYWRIGHT_CLI_VERSION_FOR_TEST: "0.0.1" } });
    // [ts] expect_eq(std.mem.indexOf(u8, output, "### Browsers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "- default:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[incompatible please re-open]") != null, true);
    // [ts] });
    // [ts] test("list browser servers", () => {
    // [ts] const browserName = mcpBrowser.replace("chrome", "chromium");
    // [ts] using browser = playwright[browserName].launch({ headless: true });
    // [ts] browser.bind("foobar", { workspaceDir: "workspace1" });
    // [ts] const { output } = cli("list", "--all");
    // [ts] expect(output).toBe(`### Browser servers available for attach
    // [ts] workspace1:
    // [ts] - browser "foobar":
    // [ts] - browser: ${/* FIX browser._options */ mcpBrowser.replace('chrome', 'chromium')}
    // [ts] - version: ${version}
    // [ts] - data-dir: <in-memory>
    // [ts] - run \`playwright-cli attach "foobar"\` to attach`);
    // [ts] });
    // [ts] test("attach to browser server", () => {
    // [ts] const browserName = mcpBrowser.replace("chrome", "chromium");
    // [ts] using browser = playwright[browserName].launch({ headless: true });
    // [ts] browser.bind("foobar", { workspaceDir: "workspace1" });
    // [ts] const page = browser.newPage();
    // [ts] page.setContent("<title>My Page</title>");
    // [ts] const { output: openOutput } = cli("attach", "foobar");
    // [ts] expect_eq(std.mem.indexOf(u8, openOutput, "### Session `foobar` created, attached to `foobar`.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, openOutput, "Run commands with: playwright-cli -s=foobar <command>") != null, true);
    // [ts] const { output: listOutput } = cli("list", "--all");
    // [ts] expect(listOutput).toBe(`### Browsers
    // [ts] /:
    // [ts] - foobar:
    // [ts] - status: open
    // [ts] - browser-type: ${mcpBrowser.replace('chrome', 'chromium')} (attached)
    // [ts] ### Browser servers available for attach
    // [ts] workspace1:
    // [ts] - browser "foobar":
    // [ts] - browser: ${mcpBrowser.replace('chrome', 'chromium')}
    // [ts] - version: ${version}
    // [ts] - data-dir: <in-memory>
    // [ts] - run \`playwright-cli attach "foobar"\` to attach`);
    // [ts] });
    // [ts] test("fail to attach to browser server without contexts", () => {
    // [ts] const browserName = mcpBrowser.replace("chrome", "chromium");
    // [ts] using browser = playwright[browserName].launch({ headless: true });
    // [ts] browser.bind("foobar", { workspaceDir: "workspace1" });
    // [ts] const { error } = cli("attach", "foobar");
    // [ts] expect_eq(std.mem.indexOf(u8, error, "Error: unable to connect to a browser that does not have any contexts") != null, true);
    // [ts] });
    // [ts] test("attach with session alias", () => {
    // [ts] const browserName = mcpBrowser.replace("chrome", "chromium");
    // [ts] using browser = playwright[browserName].launch({ headless: true });
    // [ts] browser.bind("foobar", { workspaceDir: "workspace1" });
    // [ts] const page = browser.newPage();
    // [ts] page.setContent("<title>Alias Page</title>");
    // [ts] const { output: openOutput } = cli("attach", "foobar", "--session=mybrowser");
    // [ts] expect_eq(std.mem.indexOf(u8, openOutput, "### Session `mybrowser` created, attached to `foobar`.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, openOutput, "Run commands with: playwright-cli -s=mybrowser <command>") != null, true);
    // [ts] cli("-s", "mybrowser", "close");
    // [ts] });
    // [ts] test("detach from browser server", () => {
    // [ts] const browserName = mcpBrowser.replace("chrome", "chromium");
    // [ts] using browser = playwright[browserName].launch({ headless: true });
    // [ts] browser.newPage();
    // [ts] browser.bind("foobar", { workspaceDir: "workspace1" });
    // [ts] const { output: openOutput } = cli("attach", "foobar");
    // [ts] expect_eq(std.mem.indexOf(u8, openOutput, "Session `foobar` created, attached to `foobar`") != null, true);
    // [ts] cli("-s", "foobar", "close");
    // [ts] const { output: listOutput } = cli("list", "--all");
    // [ts] expect(listOutput).toBe(`### Browser servers available for attach
    // [ts] workspace1:
    // [ts] - browser \"foobar\":
    // [ts] - browser: ${/* FIX browser._options */ mcpBrowser.replace('chrome', 'chromium')}
    // [ts] - version: ${version}
    // [ts] - data-dir: <in-memory>
    // [ts] - run \`playwright-cli attach \"foobar\"\` to attach`);
    // [ts] });
}
