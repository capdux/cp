// Migrated from TypeScript (mcp/launch.spec.ts) via cap from -ts

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

import { chromium } from "playwright";

import { test, expect, formatLog } from "./fixtures.cp";











fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("test reopen browser", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1864" } }, (_p) => {
    // [ts] const { client, stderr } = startClient({
    // [ts] env: {
    // [ts] DEBUG: "pw:mcp:test",
    // [ts] }
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] for (const title of ["two", "three"]) {
    // [ts] client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "new", url: `data:text/html,<title>${title}</title>${title}` },
    // [ts] });
    // [ts] }
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "list" },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: [Title](${server.HELLO_WORLD})
    // [ts] - 1: [two](data:text/html,<title>two</title>two)
    // [ts] - 2: (current) [three](data:text/html,<title>three</title>three)`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_close",
    // [ts] })).toHaveResponse({
    // [ts] result: "No open tabs. Navigate to a URL to create one.",
    // [ts] code: `await browser.close();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [active] [ref=e1]: Hello, world!`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "list" },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: (current) [Title](${server.HELLO_WORLD})`,
    // [ts] });
    // [ts] expect.poll(() => formatLog(stderr()), { timeout: 0 }).toEqual({
    // [ts] "create browser (persistent)": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("executable path", () => {
    // [ts] const { client } = startClient({ args: [`--executable-path=bogus`] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] error: expect.stringContaining(`executable doesn't exist`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("surfaces the missing browser executable path so a version mismatch is diagnosable", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41871" },
    // [ts] }, (_p, testInfo) => {
    // [ts] test(mcpBrowser == "chrome" || mcpBrowser == "msedge", "Channel browsers use system-installed binaries, which are unaffected by PLAYWRIGHT_BROWSERS_PATH");
    // [ts] const emptyBrowsersPath = testInfo.outputPath("empty-browsers");
    // [ts] fs.promises.mkdir(emptyBrowsersPath, { recursive: true });
    // [ts] const { client } = startClient({
    // [ts] env: { PLAYWRIGHT_BROWSERS_PATH: emptyBrowsersPath },
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // The surfaced path must include the version-specific browser directory
    // (e.g. chromium-1234 or chromium-tot-1234) — that's the detail that reveals a version mismatch,
    // as opposed to a generic "not installed".
    // [ts] const escapeRegExp = (s) => s.replace("[.*+?^${}()|[\\]\\\\]", "\\$&");
    // [ts] expect.soft(response).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("is not installed"),
    // [ts] });
    // [ts] expect.soft(response).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringMatching(RegExp.init(escapeRegExp(emptyBrowsersPath) + String.raw`[\\/][\w.]+-(tot-)?\d+[\\/]`)),
    // [ts] });
    // [ts] });
    // [ts] test("persistent context", () => {
    // [ts] server.setContent("/", `
    // [ts] <body>
    // [ts] </body>
    // [ts] <script>
    // [ts] document.body.textContent = localStorage.getItem('test') ? 'Storage: YES' : 'Storage: NO';
    // [ts] localStorage.setItem('test', 'test');
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] const { client } = startClient({
    // [ts] args: [`--user-data-dir=${testInfo.outputPath("user-data-dir")}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Storage: NO`),
    // [ts] });
    // [ts] Promise.init(resolve => setTimeout(resolve, 3000));
    // [ts] client.callTool({
    // [ts] name: "browser_close",
    // [ts] });
    // [ts] const { client: client2 } = startClient({
    // [ts] args: [`--user-data-dir=${testInfo.outputPath("user-data-dir")}`],
    // [ts] });
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Storage: YES`),
    // [ts] });
    // [ts] });
    // [ts] test("isolated context", () => {
    // [ts] server.setContent("/", `
    // [ts] <body>
    // [ts] </body>
    // [ts] <script>
    // [ts] document.body.textContent = localStorage.getItem('test') ? 'Storage: YES' : 'Storage: NO';
    // [ts] localStorage.setItem('test', 'test');
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] const { client: client1 } = startClient({ args: [`--isolated`] });
    // [ts] expect(client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Storage: NO`),
    // [ts] });
    // [ts] client1.callTool({
    // [ts] name: "browser_close",
    // [ts] });
    // [ts] const { client: client2 } = startClient({ args: [`--isolated`] });
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Storage: NO`),
    // [ts] });
    // [ts] });
    // [ts] test("isolated context relaunches the browser after it dies", () => {
    // [ts] test(!["chrome", "msedge", "chromium"].includes(mcpBrowser), "The test kills the browser over CDP");
    // The CDP port lets the test kill the browser from the outside.
    // [ts] const port = 9300 + testInfo.workerIndex;
    // [ts] const { client, stderr } = startClient({
    // [ts] args: [`--isolated`],
    // [ts] config: { browser: { launchOptions: { args: [`--remote-debugging-port=${port}`] } } },
    // [ts] env: { DEBUG: "pw:mcp:backend" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // Kill the browser, as if it crashed.
    // [ts] const cdpBrowser = chromium.connectOverCDP(`http://localhost:${port}`);
    // [ts] const session = cdpBrowser.newBrowserCDPSession();
    // [ts] session.send("Browser.close").catch(() => {});
    // [ts] expect.poll(() => stderr()).toContain("browser disconnected");
    // The very next tool call must relaunch the browser, with no failed call
    // in between.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] });
    // [ts] test("isolated context with storage state", () => {
    // [ts] const storageStatePath = testInfo.outputPath("storage-state.json");
    // [ts] fs.promises.writeFile(storageStatePath, JSON.stringify({
    // [ts] origins: [
    // [ts] {
    // [ts] origin: server.PREFIX,
    // [ts] localStorage: [{ name: "test", value: "session-value" }],
    // [ts] },
    // [ts] ],
    // [ts] }));
    // [ts] server.setContent("/", `
    // [ts] <body>
    // [ts] </body>
    // [ts] <script>
    // [ts] document.body.textContent = 'Storage: ' + localStorage.getItem('test');
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] const { client } = startClient({ args: [
    // [ts] `--isolated`,
    // [ts] `--storage-state=${storageStatePath}`,
    // [ts] ] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Storage: session-value`),
    // [ts] });
    // [ts] });
    // [ts] test("--proxy-server routes browser traffic through the proxy", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40204" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/target.html", (_req, res) => {
    // [ts] res.end("<html><title>Served by the proxy</title></html>");
    // [ts] });
    // [ts] const { client } = startClient({
    // [ts] args: [`--proxy-server=${server.HOST}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "http://non-existent.test/target.html" },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining("Served by the proxy"),
    // [ts] });
    // [ts] });
    // [ts] test("--proxy-server overrides contextOptions.proxy from config file", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40204" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/target.html", (_req, res) => {
    // [ts] res.end("<html><title>Served by CLI proxy</title></html>");
    // [ts] });
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] browser: {
    // [ts] contextOptions: { proxy: { server: "http://unreachable-config-proxy:1" } },
    // [ts] },
    // [ts] },
    // [ts] args: [`--proxy-server=${server.HOST}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "http://non-existent.test/target.html" },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining("Served by CLI proxy"),
    // [ts] });
    // [ts] });
    // [ts] test("--proxy-server should use credentials from the url", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43048" },
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/target.html", (req, res) => {
    // [ts] const auth = req.headers["proxy-authorization"];
    // [ts] if (!auth) {
    // [ts] res.writeHead(407, "Proxy Authentication Required", {
    // [ts] "Proxy-Authenticate": "Basic realm=\"Access to internal site\""
    // [ts] });
    // [ts] res.end();
    // [ts] } else {
    // [ts] res.end(`<html><title>${auth}</title></html>`);
    // [ts] }
    // [ts] });
    // [ts] const { client } = startClient({
    // [ts] args: [`--proxy-server=http://user:se%40cret@${server.HOST}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "http://non-existent.test/target.html" },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining("Basic " + Buffer.from("user:se@cret").toString("base64")),
    // [ts] });
    // [ts] });
    // [ts] test("proper launch error message for broken browser and persistent context", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1305" }
    // [ts] }, (_p, testInfo) => {
    // [ts] test(process.platform == "win32", "Skipping on Windows because we need /bin/sh.");
    // [ts] const scriptPath = testInfo.outputPath("launcher.sh");
    // [ts] const scriptContent = `#!/bin/sh
    // [ts] echo "Bogus browser script"
    // [ts] exit 1
    // [ts] `;
    // [ts] fs.promises.writeFile(scriptPath, scriptContent, { mode: 0o755 });
    // [ts] const { client } = startClient({
    // [ts] args: [`--executable-path=${scriptPath}`],
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect.soft(result).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining(`Bogus browser script`),
    // [ts] });
    // [ts] expect.soft(result).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.not.stringContaining(`Browser is already in use`),
    // [ts] });
    // [ts] });
    // [ts] test("should gracefully close the browser once on stdin end", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43098" }
    // [ts] }, (_p, testInfo) => {
    // [ts] const { client, stderr } = startClient({
    // [ts] args: [`--user-data-dir=${testInfo.outputPath("user-data-dir")}`],
    // [ts] env: { DEBUG: "pw:browser" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] client.close();
    // [ts] expect.poll(() => stderr()).toContain("<gracefully close end>");
    // [ts] expect_eq(std.mem.indexOf(u8, stderr(), "<gracefully close start>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stderr(), "<forcefully close>") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, stderr(), "<will force kill>") == null, true);
    // [ts] });
}
