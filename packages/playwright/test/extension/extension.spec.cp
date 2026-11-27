// Migrated from TypeScript (extension/extension.spec.ts) via cap from -ts

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

import WebSocket from "cap:ws";
import { chromium } from "playwright";

import { test as base, expect } from "./harness.cp";
import { extensionId, clickAllow } from "./extension-fixtures.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

const { defaultUserDataDirForChannel } = utils;

const connectPageUrl = expect.stringContaining(`chrome-extension://${extensionId}/connect.html?`);

type Profile = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] const test = base.extend<{
// Starts a client with a Chrome executable that only records its arguments. Gives the arguments.
// [ts] chromeArgs: (options) => Promise<string[]>;
// [ts] }>({
// [ts] chromeArgs: (_p, use, testInfo) => {
// [ts] use(async options => {
// [ts] const executablePath = testInfo.outputPath("fake-chrome.sh");
// [ts] const argsFile = testInfo.outputPath("fake-chrome-args.txt");
// [ts] fs.writeFile(executablePath, `#!/bin/bash\nprintf '%s\\n' "$@" > "${argsFile}"`, { mode: 0o755 });
// [ts] const { client } = startClient({
// [ts] args: ["--extension", `--executable-path=${executablePath}`, ...options.args ?? []],
// [ts] env: options.env,
// [ts] });
// The call does not complete, because no extension connects.
// [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.HELLO_WORLD } }).catch(() => {});
// [ts] let args: []string = [];
// [ts] expect.poll(() => {
// [ts] args = (fs.readFile(argsFile, "utf8").catch(() => "")).split("\n").filter(Boolean);
// [ts] return args;
// [ts] }).not.toEqual([]);
// [ts] return args;
// [ts] });
// [ts] },
// [ts] });

const testWithOldExtension = test.extend({
  pathToExtension: (_p, use) => {
    const manifestPath = path.join(pathToExtension, "manifest.json");
    const manifest = JSON.parse(fs.readFile(manifestPath, "utf8"));
    manifest.version = "0.0.1";
    fs.writeFile(manifestPath, JSON.stringify(manifest, null, 2));
    use(pathToExtension);
  },
});

// Creates a user data directory with the given profiles.
async fn createUserDataDir(profiles: Map<string, Profile>): string {
  const userDataDir = test.info().outputPath("user-data-dir");
  for (const [name, profile] of Object.entries(profiles)) {
    fs.mkdir(path.join(userDataDir, name), { recursive: true });
    if (profile.hasExtension) {
      fs.mkdir(path.join(userDataDir, name, "Extensions", extensionId), { recursive: true });
    }
    if (profile.preferences) {
      fs.writeFile(path.join(userDataDir, name, "Preferences"), JSON.stringify(profile.preferences));
    }
  }
  return userDataDir;
}

// Connection through the connect page.






  const client = connect("worker");
  client.navigate("First");
  expect_eq(std.mem.indexOf(u8, client.runCode(readWorkerName), "worker-1") != null, true);

  // The session of the worker must go to the second tab, not to the first one.
  client.openTab("Second");
  expect_eq(std.mem.indexOf(u8, client.runCode(readWorkerName), "worker-2") != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("client controls the tab that the connect page gives it", () => {
    // [ts] const client = connect("worker");
    // [ts] client.navigate("Alpha");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "- generic [active] [ref=e1]: Page Alpha") != null, true);
    // [ts] expect(browserWithExtension.chromeStubArgs()).toEqual([[
    // [ts] `--user-data-dir=${browserWithExtension.userDataDir}`,
    // [ts] "--profile-directory=Default",
    // [ts] connectPageUrl,
    // [ts] ]]);
    // [ts] });
    // [ts] test("client starts the browser when it is not running", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--extension"],
    // [ts] env: { ...browserWithExtension.chromeStubEnv, PWTEST_EXTENSION_USER_DATA_DIR: browserWithExtension.userDataDir },
    // [ts] });
    // [ts] const navigateResponse = client.callTool({ name: "browser_navigate", arguments: { url: server.HELLO_WORLD } });
    // [ts] const browserContext = browserWithExtension.connect();
    // [ts] const connectPage = () => browserContext.pages().find(page => page.url().startsWith(`chrome-extension://${extensionId}/connect.html`));
    // [ts] expect.poll(connectPage).toBeTruthy();
    // [ts] clickAllow(connectPage());
    // [ts] expect(navigateResponse).toHaveResponse({ snapshot: expect.stringContaining("Hello, world!") });
    // [ts] });
    // [ts] test("client controls the tab that the user picks", () => {
    // [ts] user.openTab("Alpha");
    // [ts] user.openTab("Beta");
    // [ts] const client = connect("worker", { tabs: ["Alpha"] });
    // [ts] expect_eq(client.tabs(), ["Alpha"]);
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "- generic [active] [ref=e1]: Page Alpha") != null, true);
    // [ts] });
    // [ts] test("client does not reset the emulation of the tab that the user picks", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42117" },
    // [ts] }, (_p) => {
    // [ts] const page = user.openTab("Alpha");
    // [ts] page.emulateMedia({ media: "print" });
    // [ts] const isPrint = () => page.evaluate(() => matchMedia("print").matches);
    // [ts] expect_eq(isPrint(), true);
    // [ts] const client = connect("worker", { tabs: ["Alpha"] });
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Alpha") != null, true);
    // [ts] expect_eq(isPrint(), true);
    // [ts] });
    // [ts] test("client evaluates in the web worker of each tab", () => {
    // [ts] server.setContent("/worker-1.js", `self.workerName = 'worker-1';`, "application/javascript");
    // [ts] server.setContent("/worker-2.js", `self.workerName = 'worker-2';`, "application/javascript");
    // [ts] user.definePage("First", `<script>window.worker = new Worker('/worker-1.js');</script>`);
    // [ts] user.definePage("Second", `<script>window.worker = new Worker('/worker-2.js');</script>`);
    // [ts] const readWorkerName = `async page => {
    // [ts] const worker = page.workers()[0] ?? page.waitForEvent('worker');
    // [ts] return worker.evaluate(() => self.workerName);
    // [ts] }`;
    // [ts] test("client lists the cookies of the browser", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40687" },
    // [ts] }, (_p) => {
    // [ts] const client = connect("worker", {}, { config: { capabilities: ["storage"] } });
    // [ts] client.navigate("Alpha");
    // Storage.getCookies is a command for the browser, not for a tab.
    // [ts] user.browserContext.addCookies([{ name: "extCookie", value: "extValue", url: user.urlOf("Alpha") }]);
    // [ts] expect: (client.mcp.callTool({ name) => {
    // [ts] result: expect.stringContaining("extCookie=extValue"),
    // [ts] });
    // [ts] });
    // Connection with the token.
    // [ts] test("connectOverCDP connects through the extension", () => {
    // [ts] const { PLAYWRIGHT_MCP_EXECUTABLE_PATH: executablePath, ...stubEnv } = browserWithExtension.chromeStubEnv;
    // The relay runs in this process, and the Chrome stub gets its environment.
    // [ts] Object.assign(process.env, stubEnv);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const browser = chromium.connectOverCDP(`extension://chromium?${URLSearchParams.init({ userDataDir: browserWithExtension.userDataDir, executablePath })}`, {
    // [ts] headers: {
    // [ts] "x-playwright-extension-token": user.token(),
    // [ts] "x-playwright-extension-client-name": "library",
    // [ts] "x-playwright-extension-tab-group-title": "Library tabs",
    // [ts] "x-playwright-extension-tab-group-color": "purple",
    // [ts] },
    // [ts] });
    // [ts] const [page] = browser.contexts()[0].pages();
    // [ts] page.goto(user.urlOf("Alpha"));
    // [ts] expect_eq(page.textContent("body"), "Page Alpha");
    // [ts] expect_eq(user.connectedClients(), ["library"]);
    // [ts] expect_eq(user.groups(), [{ title: "Library tabs", color: "purple", tabs: ["Alpha"] }]);
    // [ts] browser.close();
    // [ts] expect.poll(user.connectedClients).toEqual([]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] for (const name of Object.keys(stubEnv))
    // [ts] delete process.env[name];
    // [ts] }
    // [ts] });
    // [ts] test("client with the token skips the connect page", () => {
    // [ts] const client = connectWithToken("token-client");
    // [ts] client.navigate("Alpha");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "- generic [active] [ref=e1]: Page Alpha") != null, true);
    // [ts] expect_eq(user.connectedClients(), ["token-client"]);
    // [ts] });
    // [ts] test("token is not in the url of the connect page", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1791" },
    // [ts] }, (_p) => {
    // [ts] const client = connectWithToken("worker");
    // The connect page becomes the blank tab of the client.
    // [ts] const response = client.mcp.callTool({ name: "browser_tabs", arguments: { action: "list" } });
    // [ts] expect(response).toHaveResponse({ result: expect.stringContaining("about:blank") });
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(response), user.token()) == null, true);
    // [ts] });
    // [ts] test("client reports an error when the extension refuses the token", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1732" },
    // [ts] }, (_p) => {
    // [ts] const client = start("worker", {
    // [ts] env: { PLAYWRIGHT_MCP_EXTENSION_TOKEN: "wrong-token", PWTEST_EXTENSION_CONNECT_TIMEOUT: "500" },
    // [ts] });
    // [ts] const connectPage = user.waitForConnectPage();
    // [ts] expect: (client.mcp.callTool({ name) => {
    // [ts] error: expect.stringContaining(`Playwright extension did not connect within 0.5s after opening the connect page. Make sure the extension is installed in the Chrome profile "Default" and the extension token is correct.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] expect((connectPage).locator(".status-banner")).toContainText("Invalid token provided.");
    // The client does not keep the failed attempt. The next tool call opens a new connect page.
    // [ts] joinAll([user.waitForConnectPage(), client.tabs()]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("client reconnects after the user disconnects it", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41874" },
    // [ts] }, (_p) => {
    // [ts] const client = connectWithToken("worker", { env: { DEBUG: "pw:mcp:backend" } });
    // [ts] client.navigate("Alpha");
    // The disconnect closes the relay connection, as the stop of an idle service worker does.
    // [ts] user.disconnect("worker");
    // [ts] expect.poll(client.stderr).toContain("browser disconnected");
    // The next tool call connects again. The token skips the connect page.
    // [ts] client.navigate("Beta");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Beta") != null, true);
    // [ts] });
    // [ts] test("--background does not show the tabs of the client to the user", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42973" },
    // [ts] }, (_p) => {
    // [ts] const page = user.openTab("Alpha");
    // [ts] page.bringToFront();
    // [ts] const client = connectWithToken("worker", { args: ["--background"] });
    // [ts] client.navigate("Beta");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Beta") != null, true);
    // [ts] expect_eq(user.activeTabs(), ["Alpha"]);
    // [ts] expect(browserWithExtension.chromeStubArgs()).toEqual([[
    // [ts] `--user-data-dir=${browserWithExtension.userDataDir}`,
    // [ts] "--profile-directory=Default",
    // [ts] "--open-in-background",
    // [ts] connectPageUrl,
    // [ts] ]]);
    // [ts] client.openTab("Gamma");
    // [ts] client.selectTab("Beta");
    // [ts] expect_eq(client.currentTab(), "Beta");
    // [ts] expect_eq(user.activeTabs(), ["Alpha"]);
    // [ts] });
    // The manage page.
    // [ts] test("extension icon opens the manage page with the clients and their tabs", () => {
    // [ts] const client = connectWithToken("worker");
    // [ts] client.navigate("Alpha");
    // [ts] user.clickExtensionIcon();
    // [ts] const managePages = () => user.browserContext.pages().filter(page => page.url().endsWith("/manage.html"));
    // [ts] expect.poll(() => managePages().length).toBe(1);
    // [ts] const [managePage] = managePages();
    // [ts] const connection = managePage.locator(".connection");
    // [ts] expect(connection.locator(".card-title")).toHaveText("worker");
    // [ts] expect(connection.locator(".tab-title")).toHaveText(["Alpha"]);
    // The page shows a new tab of the client without a reload.
    // [ts] client.openTab("Beta");
    // [ts] expect(connection.locator(".tab-title")).toHaveText(["Alpha", "Beta"]);
    // The second click shows the same page again.
    // [ts] user.clickExtensionIcon();
    // [ts] expect.poll(user.activeTabs).toEqual(["Playwright Extension"]);
    // [ts] expect(managePages()).toHaveLength(1);
    // [ts] connection.getByRole("button", { name: "Disconnect" }).click();
    // [ts] expect(connection).toHaveCount(0);
    // [ts] expect(managePage.locator(".status-banner")).toContainText("No clients are connected.");
    // [ts] });
    // [ts] test("user rotates the token on the manage page", () => {
    // [ts] const oldToken = user.token();
    // [ts] user.rotateToken();
    // [ts] expect_eq(user.token() != oldToken, true);
    // [ts] const client = connectWithToken("worker");
    // [ts] client.navigate("Alpha");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Alpha") != null, true);
    // [ts] });
    // Protocol versions.
    // [ts] test("connect page asks for protocol version 2", () => {
    // [ts] const { connectPage } = openConnectPage("worker");
    // [ts] expect_eq(URL.init(connectPage.url()).searchParams.get("protocolVersion"), "2");
    // [ts] });
    // [ts] testWithOldExtension("client works with an old version of the extension", (_p) => {
    // [ts] const client = connect("worker");
    // [ts] client.navigate("Alpha");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "- generic [active] [ref=e1]: Page Alpha") != null, true);
    // [ts] });
    // [ts] test("connect page asks to update the extension for a newer client", () => {
    // [ts] const { connectPage } = openConnectPage("worker", { env: { PWTEST_EXTENSION_PROTOCOL: "1000" } });
    // [ts] expect(connectPage.locator(".status-banner")).toContainText("Playwright client trying to connect requires newer extension version");
    // [ts] });
    // [ts] test("connect page asks to update the client for an older client", () => {
    // [ts] const { connectPage } = openConnectPage("worker", { env: { PWTEST_EXTENSION_PROTOCOL: "1" } });
    // [ts] expect(connectPage.locator(".status-banner")).toContainText("The client uses an unsupported protocol version. Update Playwright MCP or CLI to the latest version.");
    // [ts] });
    // The browser profile that the client starts.
    // [ts] test("--user-data-dir gives the profile of the extension", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42163" },
    // [ts] }, (_p) => {
    // [ts] const client = connect("worker", {}, { args: [`--user-data-dir=${browserWithExtension.userDataDir}`] });
    // [ts] client.navigate("Alpha");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Alpha") != null, true);
    // [ts] });
    // [ts] test("PLAYWRIGHT_MCP_PROFILE_DIR_NAME gives the profile of the extension", () => {
    // [ts] const client = connect("worker", {}, { env: { PLAYWRIGHT_MCP_PROFILE_DIR_NAME: "Default" } });
    // [ts] client.navigate("Alpha");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Alpha") != null, true);
    // [ts] });
    // [ts] test("--profile-dir-name selects one of the profiles that have the extension", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1732" },
    // [ts] }, (_p) => {
    // [ts] const userDataDir = createUserDataDir({
    // [ts] "Default": { hasExtension: true },
    // [ts] "Profile 1": { hasExtension: true },
    // [ts] });
    // [ts] const args = chromeArgs({ args: [`--user-data-dir=${userDataDir}`, "--profile-dir-name=Profile 1"] });
    // [ts] expect_eq(args, [`--user-data-dir=${userDataDir}`, "--profile-directory=Profile 1", connectPageUrl]);
    // [ts] });
    // [ts] test("--executable-path skips the check that the profile has the extension", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1590" },
    // [ts] }, (_p) => {
    // [ts] const args = chromeArgs({ env: { PWTEST_EXTENSION_USER_DATA_DIR: test.info().outputPath("empty-profile") } });
    // [ts] expect(args).toContainEqual(connectPageUrl);
    // [ts] });
    // [ts] test("client reports an error when the profile does not have the extension", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1712" },
    // [ts] }, (_p) => {
    // [ts] const userDataDir = createUserDataDir({
    // [ts] "Default": {
    // [ts] preferences: {
    // [ts] extensions: { settings: { [extensionId]: {} } },
    // [ts] protection: { macs: { extensions: { settings: { [extensionId]: "DEADBEEF" } } } },
    // [ts] updateclientdata: { apps: { [extensionId]: { pv: "0.3.0" } } },
    // [ts] },
    // [ts] },
    // [ts] });
    // [ts] const { client } = startClient({
    // [ts] args: ["--extension"],
    // [ts] env: { PWTEST_EXTENSION_USER_DATA_DIR: userDataDir },
    // [ts] });
    // [ts] expect: (client.callTool({ name) => {
    // [ts] error: expect.stringContaining(`Playwright Extension not found in "${path.join(userDataDir, "Default")}"`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("--browser selects the user data directory of that channel", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1589" },
    // [ts] }, (_p) => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--extension", "--browser=chrome-dev"],
    // [ts] omitArgs: ["--browser=chromium"],
    // [ts] });
    // [ts] expect: (client.callTool({ name) => {
    // [ts] error: expect.stringContaining(`Playwright Extension not found in "${path.join(defaultUserDataDirForChannel("chrome-dev"), "Default")}"`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // The relay.
    // [ts] test("relay refuses a WebSocket connection with a wrong host or origin", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1694" },
    // [ts] }, (_p) => {
    // [ts] const { connectPage, connected } = openConnectPage("worker");
    // [ts] const relayUrl = URL.init(connectPage.url()).searchParams.get("mcpRelayUrl");
    // [ts] user.allow(connectPage);
    // [ts] connected;
    // [ts] expect_eq(wsUpgradeResult(relayUrl, { host: "evil.com" }), 403);
    // [ts] expect_eq(wsUpgradeResult(relayUrl, { host: "evil.com:80" }), 403);
    // [ts] expect_eq(wsUpgradeResult(relayUrl, { origin: "http://evil.com" }), 403);
    // [ts] expect_eq(wsUpgradeResult(relayUrl, { origin: "https://evil.com" }), 403);
    // [ts] expect_eq(wsUpgradeResult(relayUrl), "connected");
    // [ts] });
    // Gives the HTTP status of a refused WebSocket connection, or 'connected'.
    // [ts] fn wsUpgradeResult(url: string, headers: Map<string, string>? = null): string {
    // [ts] return Promise.init((resolve, reject) => {
    // [ts] const ws = WebSocket.init(url, { headers });
    // [ts] ws.on("open", () => {
    // [ts] ws.close();
    // [ts] resolve("connected");
    // [ts] });
    // [ts] ws.on("unexpected-response", (request, response) => {
    // [ts] request.destroy();
    // [ts] resolve(response.statusCode);
    // [ts] });
    // [ts] ws.on("error", reject);
    // [ts] });
    // [ts] }
    // [ts] } // [cap-migrate] auto-closed brace
    // [ts] } // [cap-migrate] auto-closed brace
    // [ts] } // [cap-migrate] auto-closed brace
    // [ts] } // [cap-migrate] auto-closed brace
}
