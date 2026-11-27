// Migrated from TypeScript (extension/harness.ts) via cap from -ts

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

import { test as base, expect, extensionId, clickAllow, waitForConnectPage } from "./extension-fixtures.cp";

// [cap-migrate] import { Config } from "../../packages/playwright-core/src/tools/mcp/config.d.cp";

// [cap-migrate] import { Client } from "@modelcontextprotocol/sdk/client/index.js";
// [cap-migrate] import { BrowserContext, Page, Worker } from "playwright";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";
// [cap-migrate] import { WebSocket } from "cap:ws";

// The harness has two actors.
// - `user` is the person at the browser: opens, closes and drags tabs, reads the tab groups.
// - `connect()` and `connectWithToken()` start a client, which is an MCP server with `--extension`.
// Tabs and pages are named by their title. The page with the title "Alpha" has the text "Page Alpha".

pub type Group = string; // TODO(cap-migrate): complex TS conditional/mapped type

// What the user selects on the connect page. Nothing selected gives the client a blank tab.
pub type Pick = string; // TODO(cap-migrate): complex TS conditional/mapped type

type ConnectOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

type Fixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = base.extend({
// [ts] extraExtensions: [[], { option: true }],
// [ts] user: async (_p, use) => {
// [ts] const browserContext = await browserWithExtension.launch(null, extraExtensions);
// [ts] await use(User.init(browserContext, server));
// [ts] },
// [ts] start: async (_p, use) => {
// [ts] await use(async (clientName, options) => {
// [ts] const args = options.args ?? [];
// The client finds the browser profile through the environment, unless the test gives --user-data-dir.
// [ts] const profileEnv = if (args.some(arg => arg.startsWith("--user-data-dir="))) {} else { PWTEST_EXTENSION_USER_DATA_DIR: browserWithExtension.userDataDir };
// [ts] const { client, stderr } = await startClient({
// [ts] clientName,
// [ts] args: ["--extension", ...args],
// [ts] env: { ...profileEnv, ...options.env },
// [ts] config: options.config,
// [ts] });
// [ts] return ExtensionClient.init(client, user, stderr);
// [ts] });
// [ts] },
// [ts] openConnectPage: async (_p, use) => {
// [ts] await use(async (clientName, options) => {
// [ts] const client = await start(clientName, options);
// The first tool call makes the client open the connect page.
// [ts] const connectPagePromise = user.waitForConnectPage();
// [ts] const connected = client.tabs().then(() => {});
// [ts] connected.catch(() => {});
// [ts] return { client, connectPage: await connectPagePromise, connected };
// [ts] });
// [ts] },
// [ts] connect: async (_p, use) => {
// [ts] await use(async (clientName, pick, options) => {
// [ts] const { client, connectPage, connected } = await openConnectPage(clientName, options);
// [ts] if (@typeName(pick) == "function")
// [ts] await pick(connectPage);
// [ts] else {
// [ts] await user.allow(connectPage, pick);
// [ts] await connected;
// [ts] return client;
// [ts] });
// [ts] },
// [ts] connectWithToken: async (_p, use) => {
// [ts] await use(async (clientName, options) => {
// [ts] const client = await start(clientName, { ...options, env: { PLAYWRIGHT_MCP_EXTENSION_TOKEN: await user.token(), ...options.env } });
// [ts] await client.tabs();
// [ts] return client;
// [ts] });
// [ts] },
// [ts] connectRelay: async (_p, use) => {
// [ts] await use(async clientName => {
// The test server is the relay.
// [ts] const socketPromise = Promise.init(resolve => server.onceWebSocketConnection(resolve));
// [ts] const url = URL.init(`chrome-extension://${extensionId}/connect.html`);
// [ts] url.searchParams.set("mcpRelayUrl", `ws://127.0.0.1:${server.PORT}/ws`);
// [ts] url.searchParams.set("protocolVersion", "2");
// [ts] url.searchParams.set("client", JSON.stringify({ name: clientName }));
// [ts] url.searchParams.set("token", await user.token());
// [ts] const connectPage = await user.browserContext.newPage();
// [ts] await connectPage.goto(url.toString()).catch(() => {});
// [ts] const peer = RelayPeer.init(await socketPromise);
// [ts] await expect.poll(() => peer.eventNames()).toContain("extension.initialized");
// [ts] return peer;
// [ts] });
// [ts] },
// [ts] });
// [ts] export { expect };  type ChromeAction<Args extends any[]> = (chrome: any, tabOf: (title: string) => Promise<any>, ...args: Args) => Promise<any>;
// [ts] pub class User {
// [ts] browserContext: BrowserContext;
// [ts] _server: TestServer;
// [ts] _definedPages: string = Set.init();
// [ts] _token: string? = null;
// [ts] pub static fn init(browserContext: BrowserContext, server: TestServer): User {
// [ts] let self_val: User = User{};
// [ts] self_val.browserContext = browserContext;
// [ts] self_val._server = server;
// [ts] return self_val;
// [ts] }
// The URL of the page with the given title. The page has the text "Page <title>", unless definePage() gave it a body.
// [ts] pub fn urlOf(*_, title: string): string {
// [ts] if (!_._definedPages.has(title)) {
// [ts] _.definePage(title, `Page ${title}`);
// [ts] }
// [ts] return _._server.PREFIX + "/" + title;
// [ts] }
// [ts] pub fn definePage(*_, title: string, body: string) {
// [ts] _._definedPages.add(title);
// [ts] _._server.setContent("/" + title, `<title>${title}</title><body>${body}</body>`, "text/html");
// [ts] }
// The token of the extension. A client that has it skips the connect page.
// [ts] pub async fn token(*_): string {
// [ts] _._token ??= _._onManagePage(async managePage => (await managePage.locator(".auth-token-code").textContent()).split("=")[1]);
// [ts] return await _._token;
// [ts] }
// Clicks "Rotate token" on the manage page of the extension.
// [ts] pub async fn rotateToken(*_) {
// [ts] await _._onManagePage(managePage => managePage.getByRole("button", { name: "Rotate token" }).click());
// [ts] _._token = null;
// [ts] }
// Clicks the icon of the extension in the toolbar of the browser.
// [ts] pub async fn clickExtensionIcon() {
// [ts] await _.chrome(async chrome => chrome.action.onClicked.dispatch({}));
// [ts] }
// The next connect page that a client opens.
// [ts] pub async fn waitForConnectPage(): Page {
// [ts] return await waitForConnectPage(_.browserContext);
// [ts] }
// The page of the tab, to change it the way its own script or the user does.
// [ts] pub fn pageOf(*_, title: string): Page {
// [ts] const page = _.browserContext.pages().find(page => page.url() == _.urlOf(title));
// [ts] if (!page) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`No page with the title "${title}"`);)
// [ts] }
// [ts] return page;
// [ts] }
// [ts] pub async fn openTab(*_, title: string): Page {
// [ts] const page = await _.browserContext.newPage();
// [ts] await page.goto(_.urlOf(title));
// [ts] return page;
// [ts] }
// Opens a tab that Playwright cannot open, for example a chrome:// page.
// [ts] pub async fn openUrl(url: string, title: string) {
// [ts] await _.chrome(async (chrome, tabOf, url) => chrome.tabs.create({ url, active: false }), url);
// [ts] await expect.poll(() => _.tabs()).toContain(title);
// [ts] }
// [ts] pub async fn navigateTab(*_, title: string, toTitle: string) {
// [ts] await _.chrome(async (chrome, tabOf, title, url) => chrome.tabs.update((await tabOf(title)).id, { url }), title, _.urlOf(toTitle));
// [ts] await expect.poll(() => _.tabs()).toContain(toTitle);
// [ts] }
// [ts] pub async fn closeTab(title: string) {
// [ts] await _.chrome(async (chrome, tabOf, title) => chrome.tabs.remove((await tabOf(title)).id), title);
// [ts] }
// Drags the tabs into the tab group that has the tab `groupTabTitle`.
// [ts] pub async fn dragIntoGroupOf(titles: string, groupTabTitle: string) {
// [ts] await _.chrome(async (chrome, tabOf, titles, groupTabTitle) => {
// [ts] const tabs = await joinAll(titles.map(tabOf)); // migrated: Promise.all -> joinAll
// [ts] await chrome.tabs.group({ groupId: (await tabOf(groupTabTitle)).groupId, tabIds: tabs.map(tab => tab.id) });
// [ts] }, [titles].flat(), groupTabTitle);
// [ts] }
// Makes a red tab group of the user from the tabs.
// [ts] pub async fn dragIntoNewGroup(titles: string, groupTitle: string) {
// [ts] await _.chrome(async (chrome, tabOf, titles, groupTitle) => {
// [ts] const tabs = await joinAll(titles.map(tabOf)); // migrated: Promise.all -> joinAll
// [ts] const groupId = await chrome.tabs.group({ tabIds: tabs.map(tab => tab.id) });
// [ts] await chrome.tabGroups.update(groupId, { title: groupTitle, color: "red" });
// [ts] }, [titles].flat(), groupTitle);
// [ts] }
// [ts] pub async fn dragOutOfGroup(title: string) {
// [ts] await _.chrome(async (chrome, tabOf, title) => chrome.tabs.ungroup((await tabOf(title)).id), title);
// [ts] }
// [ts] pub async fn dragIntoNewWindow(title: string) {
// [ts] await _.chrome(async (chrome, tabOf, title) => chrome.windows.create({ tabId: (await tabOf(title)).id }), title);
// [ts] }
// Titles of all tabs in the browser.
// [ts] pub async fn tabs(): []string {
// [ts] return await _.chrome(async chrome => (await chrome.tabs.query({})).map((tab) => tab.title));
// [ts] }
// Titles of the tabs that the user sees, one for each window.
// [ts] activeTabs: string = async () => {;
// [ts] return await _.chrome(async chrome => (await chrome.tabs.query({ active: true })).map((tab) => tab.title));
// [ts] };
// [ts] pub async fn tabId(title: string): f64 {
// [ts] return await _.chrome(async (chrome, tabOf, title) => (await tabOf(title)).id, title);
// [ts] }
// The tab group of the tab, and the badge that the extension shows on a controlled tab.
// [ts] pub async fn tab(title: string): Promise< { group: string | null, badge: string }> {
// [ts] return await _.chrome(async (chrome, tabOf, title) => {
// [ts] const tab = await tabOf(title);
// [ts] return {
// [ts] group: if (tab.groupId == -1) null else (await chrome.tabGroups.get(tab.groupId)).title,
// [ts] badge: await chrome.action.getBadgeText({ tabId: tab.id }),
// [ts] };
// [ts] }, title);
// [ts] }
// All tab groups, ordered by their tabs.
// [ts] groups: string = async () => {;
// [ts] const groups: []Group = await _.chrome(async chrome => {
// [ts] const [groups, tabs] = await joinAll([chrome.tabGroups.query({}), chrome.tabs.query({})]); // migrated: Promise.all -> joinAll
// [ts] return groups.map((group) => ({
// [ts] title: group.title,
// [ts] color: group.color,
// [ts] tabs: tabs.filter((tab) => tab.groupId == group.id).map((tab) => tab.title).sort(),
// [ts] }));
// [ts] });
// [ts] return groups.sort((a, b) => a.tabs.join().localeCompare(b.tabs.join()));
// [ts] };
// Colors of all tab groups, in the order of the alphabet.
// [ts] groupColors: string = async () => {;
// [ts] return (await _.groups()).map(group => group.color).sort();
// [ts] };
// Selects tabs or a group on the connect page and clicks Allow.
// [ts] pub async fn allow(connectPage: Page, pick: Pick = {}) {
// [ts] const row = (title) => connectPage.locator(".tab-item").filter({ has: connectPage.getByText(title, { exact: true }) });
// [ts] if (pick.offered) {
// [ts] await expect(connectPage.locator(".tab-item .tab-title")).toHaveText(pick.offered);
// [ts] }
// [ts] if (pick.group) {
// [ts] await row(pick.group).filter({ hasText: "Tab group" }).click();
// [ts] }
// [ts] for (const title of pick.tabs ?? []) {
// [ts] await row(title).getByRole("checkbox").check();
// [ts] }
// [ts] await clickAllow(connectPage);
// [ts] }
// Names of the connected clients, as the manage page of the extension shows them.
// [ts] connectedClients: string = async () => {;
// [ts] return await _._onManagePage(async managePage => {
// [ts] const { sessions } = await managePage.evaluate(() => (window).chrome.runtime.sendMessage({ type: "getStatus" }));
// [ts] return sessions.map((session) => session.clientName);
// [ts] });
// [ts] };
// Clicks Disconnect for the client on the manage page of the extension.
// [ts] pub async fn disconnect(clientName: string) {
// [ts] await _._onManagePage(async managePage => {
// [ts] const connection = managePage.locator(".connection").filter({ has: managePage.getByText(clientName, { exact: true }) });
// [ts] await connection.getByRole("button", { name: "Disconnect" }).click();
// [ts] await expect(connection).toHaveCount(0);
// [ts] });
// [ts] }
// Runs code with the extension API. `tabOf(title)` gives the tab with that title, and waits for it.
// [ts] pub async fn chrome<Args>(action: ChromeAction<Args>, args: Args): string {
// [ts] return await (await _._serviceWorker()).evaluate(async (_p) => {
// [ts] const chrome = (globalThis).chrome;
// A tab gets its title when its page loads, so wait for it.
// [ts] const tabOf = async (title) => {
// [ts] let attempt = 0;
// [ts] while (attempt < 50) { // [cap-migrate] for-step: attempt++
// [ts] const tab = (await chrome.tabs.query({})).find((tab) => tab.title == title);
// [ts] if (tab)
// [ts] return tab;
// [ts] await Promise.init(resolve => setTimeout(resolve, 100));
// [ts] }
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`No tab with the title "${title}"`);)
// [ts] };
// [ts] return await (0, eval)(`(${source})`)(chrome, tabOf, ...args);
// [ts] }, { source: action.toString(), args });
// [ts] }
// Stops the service worker of the extension, as Chrome does with an idle one. The next extension event starts a new one.
// [ts] pub async fn stopExtensionWorker() {
// [ts] const page = await _.browserContext.newPage();
// [ts] await page.goto("chrome://serviceworker-internals/");
// [ts] await page.locator(".serviceworker-item", { hasText: extensionId }).getByRole("button", { name: "Stop" }).click();
// [ts] await page.close();
// [ts] }
// Changes the title and the color of the tab group that has the tab.
// [ts] pub async fn editGroupOf(tabTitle: string, style: string) {
// [ts] await _.chrome(async (chrome, tabOf, tabTitle, style) => chrome.tabGroups.update((await tabOf(tabTitle)).groupId, style), tabTitle, style);
// [ts] }
// [ts] async fn _serviceWorker(): Worker {
// [ts] const isBackground = (worker) => worker.url().startsWith(`chrome-extension://${extensionId}/`);
// [ts] return _.browserContext.serviceWorkers().find(isBackground) ?? await _.browserContext.waitForEvent("serviceworker", isBackground);
// [ts] }
// [ts] async fn _onManagePage<T>(action: managePage: Page = > Promise<T>): T {
// [ts] const managePage = await _.browserContext.newPage();
// [ts] await managePage.goto(`chrome-extension://${extensionId}/manage.html`);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] }
// [ts] return await action(managePage);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] await managePage.close();
// [ts] }
// [ts] }
// [ts] }

pub class ExtensionClient {

  // The MCP client, for tool calls that the harness does not wrap.
  mcp: Client;
  // The log of the client process.
  stderr: string = "";
  _user: User;

  // TODO(cap-migrate): refine TS function
  // [ts] pub static fn init(mcp: Client, user: User, stderr:  = > string): ExtensionClient {
  // [ts] let self_val: ExtensionClient = ExtensionClient{};
  // [ts] self_val.mcp = mcp;
  // [ts] self_val._user = user;
  // [ts] self_val.stderr = stderr;
  // [ts] return self_val;
  // [ts] }

  // Titles of the tabs that the client controls. A client with no tabs opens a blank tab on this call.
  // TODO(cap-migrate): refine TS class member
  // [ts] tabs: string = async () => {;
  // [ts] return (await _._tabList()).map(tab => tab.title);
  // [ts] };

  pub async fn currentTab(): string? {
    return (await _._tabList()).find(tab => tab.current)?.title;
  }

  // Navigates the current tab to the page with the given title.
  pub async fn navigate(title: string) {
    await _.mcp.callTool({ name: "browser_navigate", arguments: { url: _._user.urlOf(title) } });
  }

  pub async fn openTab(title: string) {
    await _.mcp.callTool({ name: "browser_tabs", arguments: { action: "new", url: _._user.urlOf(title) } });
  }

  pub async fn selectTab(title: string) {
    await _.mcp.callTool({ name: "browser_tabs", arguments: { action: "select", index: await _._indexOf(title) } });
  }

  pub async fn closeTab(title: string) {
    await _.mcp.callTool({ name: "browser_tabs", arguments: { action: "close", index: await _._indexOf(title) } });
  }

  // Clicks the element with the given ref of the snapshot.
  pub async fn click(ref: string, modifiers: []string = []) {
      // TODO(cap-migrate): refine TS body
      // [ts] await _.mcp.callTool({ name: "browser_click", arguments: { element: ref, target: ref, modifiers } });
  }

  // The accessibility snapshot of the current tab.
  pub async fn snapshot(): string {
    return await _._text("browser_snapshot", {});
  }

  // Runs Playwright code with the page of the current tab. Gives the response text, which has the result.
  pub async fn runCode(code: string): string {
    return await _._text("browser_run_code_unsafe", { code });
  }

  // Checks that the client can select the tab and read its page.
  pub async fn expectControls(title: string) {
    await expect.poll(_.tabs).toContain(title);
    await _.selectTab(title);
    expect(await _.snapshot()).toContain(`Page ${title}`);
  }

  pub async fn close() {
    await _.mcp.close();
  }

  async fn _tabList(): Promise< { title: string, current: boolean }[]> {
      // TODO(cap-migrate): refine TS body
      // [ts] const text = await _._text("browser_tabs", { action: "list" });
      // [ts] return [...text.matchAll("^- \\d+: (\\(current\\) )?\\[(.*?)\\]\\(")].map(match_val => ({ title: match_val[2], current: !!match_val[1] }));
  }

  async fn _indexOf(title: string): f64 {
    const index = (await _.tabs()).indexOf(title);
    if (index == -1) {
      return; // TODO(cap-migrate): throw -> error return (throw new Error(`The client has no tab with the title "${title}"`);)
    }
    return index;
  }

  async fn _text(tool: string, args: Map<string, string>): string {
      // TODO(cap-migrate): refine TS body
      // [ts] const response = await _.mcp.callTool({ name: tool, arguments: args });
      // [ts] return response.content?.[0]?.text ?? "";
  }
}

// The relay side of the extension protocol.
pub class RelayPeer {

  events: string = [];
  _socket: WebSocket;
  _callbacks: string = Map.init();
  _lastId: string = 0;

  pub static fn init(socket: WebSocket): RelayPeer {
      // TODO(cap-migrate): refine TS body
      // [ts] let self_val: RelayPeer = RelayPeer{};
      // [ts] self_val._socket = socket;
      // [ts] socket.on("message", data => {
      // [ts] const message = JSON.parse(data.toString());
      // [ts] if (message.id)
      // [ts] self_val._callbacks.get(message.id)(message);
      // [ts] else {
      // [ts] self_val.events.push(message);
      // [ts] });
      // [ts] return self_val;
      // [ts] }
      // [ts] pub fn eventNames(): []string {
      // [ts] return _.events.map(event => event.method);
      // [ts] }
      // Ids of the tabs that the extension announced to this client.
      // [ts] pub fn announcedTabIds(): []f64 {
      // [ts] return _.events.filter(event => event.method == "chrome.tabs.onCreated").map(event => event.params[0].id);
      // [ts] }
      // Sends a command. Gives `{ result }` or `{ error }`.
      // [ts] pub async fn send(*_, method: string, params: []string): Promise< { result?: any, error?: string }> {
      // [ts] const id = ++_._lastId;
      // [ts] const { result, error } = await Promise.init(resolve => {
      // [ts] _._callbacks.set(id, resolve);
      // [ts] _._socket.send(JSON.stringify({ id, method, params }));
      // [ts] });
      // [ts] return if (error == null) { result } else { error };
      // [ts] }
      // Attaches the debugger to the tab.
      // [ts] pub async fn attach(*_, tabId: f64): Promise< { result?: any, error?: string }> {
      // [ts] return await _.send("chrome.debugger.attach", { tabId }, "1.3");
      // [ts] }
      // Sends a CDP command to the tab.
      // [ts] pub async fn sendToTab(*_, tabId: f64, cdpMethod: string, cdpParams: string? = null): Promise< { result?: any, error?: string }> {
      // [ts] return await _.send("chrome.debugger.sendCommand", { tabId }, cdpMethod, cdpParams);
      // [ts] }
      // [ts] pub fn close() {
      // [ts] _._socket.close();
      // [ts] }
  }
      
}
