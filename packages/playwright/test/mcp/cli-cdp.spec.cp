// Migrated from TypeScript (mcp/cli-cdp.spec.ts) via cap from -ts

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
import { test, expect, daemonFolder } from "./cli-fixtures.cp";

test.describe.configure({
  retries: 1,
});












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("connect by channel name error", () => {
    // [ts] const { error } = cli("attach", "--cdp=chrome-dev");
    // [ts] expect_eq(std.mem.indexOf(u8, error, "Could not connect to chrome-dev") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error, "chrome://inspect/#remote-debugging") != null, true);
    // [ts] });
    // [ts] test("attach --cdp=<channel> defaults session name to the channel", () => {
    // Connection fails, but the daemon writes its err log under the resolved session name.
    // [ts] cli("attach", "--cdp=chrome-dev");
    // [ts] const folder = daemonFolder();
    // [ts] expect_eq(folder, true);
    // [ts] const files = fs.promises.readdir(folder);
    // [ts] expect_eq(std.mem.indexOf(u8, files, "chrome-dev.err") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, files, "default.err") == null, true);
    // [ts] });
    // [ts] test("explicit --session wins over --cdp=<channel>", () => {
    // [ts] cli("attach", "--cdp=chrome-dev", "-s=explicit");
    // [ts] const folder = daemonFolder();
    // [ts] expect_eq(folder, true);
    // [ts] const files = fs.promises.readdir(folder);
    // [ts] expect_eq(std.mem.indexOf(u8, files, "explicit.err") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, files, "chrome-dev.err") == null, true);
    // [ts] });
    // [ts] test("attach via cdp URL keeps the default session", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [page] = browserContext.pages();
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] cli("attach", `--cdp=${cdpServer.endpoint}`);
    // [ts] const { output: listOutput } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, "- default:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, "(attached)") != null, true);
    // [ts] });
    // [ts] test("attach via cdp URL honors PLAYWRIGHT_CLI_SESSION as session name", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/414" } }, (_p) => {
    // [ts] cdpServer.start();
    // [ts] const { exitCode } = cli("attach", `--cdp=${cdpServer.endpoint}`, { env: { PLAYWRIGHT_CLI_SESSION: "myname" } });
    // [ts] expect_eq(exitCode, 0);
    // [ts] });
    // [ts] test("attach rejects combining --cdp, --endpoint, or --extension", () => {
    // [ts] const { error, exitCode } = cli("attach", "--cdp=chrome-dev", "--endpoint=\"tmp\"");
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, error, "only one of [name], --cdp, --endpoint, or --extension can be specified") != null, true);
    // [ts] });
    // [ts] test("detach tears down an attached session", () => {
    // [ts] cdpServer.start();
    // [ts] cli("attach", `--cdp=${cdpServer.endpoint}`, "-s=mine");
    // [ts] const { output: listBefore } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listBefore, "- mine:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listBefore, "(attached)") != null, true);
    // [ts] const { output: detachOutput } = cli("-s=mine", "detach");
    // [ts] expect_eq(std.mem.indexOf(u8, detachOutput, `Browser 'mine' detached`) != null, true);
    // [ts] const { output: listAfter } = cli("list");
    // [ts] expect_eq(std.mem.indexOf(u8, listAfter, "- mine:") == null, true);
    // [ts] });
    // [ts] test("detach rejects sessions opened via open", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { error, exitCode } = cli("detach");
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, error, `session 'default' was not attached`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error, "close") != null, true);
    // [ts] });
    // [ts] test("detach on unknown session reports not attached", () => {
    // [ts] const { output } = cli("-s=nobody", "detach");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Browser 'nobody' is not attached.`) != null, true);
    // [ts] });
    // [ts] test("cdp server", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [page] = browserContext.pages();
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] const configPath = test.info().outputPath("config.ini");
    // [ts] fs.promises.writeFile(configPath, `
    // [ts] browser.isolated=false
    // [ts] `);
    // [ts] cli("attach", `--cdp=${cdpServer.endpoint}`, `--config=${configPath}`);
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, `- generic [active] [ref=e1]: Hello, world!`) != null, true);
    // [ts] });
    // [ts] test("attach via cdp", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [page] = browserContext.pages();
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] cli("attach", `--cdp=${cdpServer.endpoint}`);
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, `- generic [active] [ref=e1]: Hello, world!`) != null, true);
    // [ts] });
    // [ts] test("tracing-start-stop over cdp", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [page] = browserContext.pages();
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] cli("attach", `--cdp=${cdpServer.endpoint}`);
    // [ts] const { output } = cli("tracing-start");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Trace recording started") != null, true);
    // [ts] cli("eval", "() => fetch(\"/hello-world\")");
    // [ts] const { output: tracingStopOutput } = cli("tracing-stop");
    // [ts] expect_eq(std.mem.indexOf(u8, tracingStopOutput, "Trace recording stopped") != null, true);
    // [ts] const [, timestamp] = tracingStopOutput.match("trace-(\\d+)\\.trace");
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".playwright-cli", "traces", "resources")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".playwright-cli", "traces", `trace-${timestamp}.trace`)), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".playwright-cli", "traces", `trace-${timestamp}.network`)), true);
    // [ts] });
    // [ts] test("attach via cdp with a crashed page", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41714" },
    // [ts] }, (_p) => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [page] = browserContext.pages();
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] const crashedPage = browserContext.newPage();
    // [ts] crashedPage.goto(server.PREFIX + "/title.html");
    // [ts] crashedPage.goto("chrome://crash").catch(() => {});
    // [ts] crashedPage.waitForEvent("crash");
    // Attaching should not hang on the crashed page, and should not list it.
    // [ts] const { output: attachOutput, exitCode } = cli("attach", `--cdp=${cdpServer.endpoint}`);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect(attachOutput).toContain(`### Page
    // [ts] - Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`);
    // [ts] expect_eq(std.mem.indexOf(u8, attachOutput, "### Open tabs") == null, true);
    // [ts] const { output: listOutput } = cli("tab-list");
    // [ts] expect(listOutput).toBe(`### Result
    // [ts] - 0: (current) [Title](${server.HELLO_WORLD})`);
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, `- generic [active] [ref=e1]: Hello, world!`) != null, true);
    // [ts] });
}
