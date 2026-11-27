// Migrated from TypeScript (mcp/tabs.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";

// [cap-migrate] import { Client } from "@modelcontextprotocol/sdk/client/index.js";

async fn createTab(client: Client, title: string, body: string) {
  client.callTool({
    name: "browser_tabs",
    arguments: {
      action: "new",
    },
  });
  return client.callTool({
    name: "browser_navigate",
    arguments: {
      url: `data:text/html,<title>${title}</title><body>${body}</body>`,
    },
  });
}








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("list initial tabs", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "list",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: (current) [](about:blank)`,
    // [ts] });
    // [ts] });
    // [ts] test("list first tab", () => {
    // [ts] createTab(client, "Tab one", "Body one");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "list",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: [](about:blank)
    // [ts] - 1: (current) [Tab one](data:text/html,<title>Tab one</title><body>Body one</body>)`,
    // [ts] });
    // [ts] });
    // [ts] test("create new tab", () => {
    // [ts] expect: (createTab(client, 'Tab one', 'Body one')) => {
    // [ts] tabs: `- 0: [](about:blank)
    // [ts] - 1: (current) [Tab one](data:text/html,<title>Tab one</title><body>Body one</body>)`,
    // [ts] snapshot: `- generic [active] [ref=e1]: Body one`,
    // [ts] });
    // [ts] expect: (createTab(client, 'Tab two', 'Body two')) => {
    // [ts] tabs: `- 0: [](about:blank)
    // [ts] - 1: [Tab one](data:text/html,<title>Tab one</title><body>Body one</body>)
    // [ts] - 2: (current) [Tab two](data:text/html,<title>Tab two</title><body>Body two</body>)`,
    // [ts] snapshot: `- generic [active] [ref=e1]: Body two`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining("Page URL: data:text/html,<title>Tab two</title><body>Body two</body>"),
    // [ts] });
    // [ts] });
    // [ts] test("create new tab with url", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "new",
    // [ts] url: `data:text/html,<title>Tab one</title><body>Body one</body>`,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: [](about:blank)
    // [ts] - 1: (current) [Tab one](data:text/html,<title>Tab one</title><body>Body one</body>)`,
    // [ts] });
    // [ts] });
    // [ts] test("select tab", () => {
    // [ts] createTab(client, "Tab one", "Body one");
    // [ts] createTab(client, "Tab two", "Body two");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "select",
    // [ts] index: 1,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: [](about:blank)
    // [ts] - 1: (current) [Tab one](data:text/html,<title>Tab one</title><body>Body one</body>)
    // [ts] - 2: [Tab two](data:text/html,<title>Tab two</title><body>Body two</body>)`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "select",
    // [ts] index: 0,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: (current) [](about:blank)
    // [ts] - 1: [Tab one](data:text/html,<title>Tab one</title><body>Body one</body>)
    // [ts] - 2: [Tab two](data:text/html,<title>Tab two</title><body>Body two</body>)`,
    // [ts] });
    // [ts] });
    // [ts] test("close tab", () => {
    // [ts] createTab(client, "Tab one", "Body one");
    // [ts] createTab(client, "Tab two", "Body two");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "close",
    // [ts] index: 2,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: [](about:blank)
    // [ts] - 1: (current) [Tab one](data:text/html,<title>Tab one</title><body>Body one</body>)`,
    // [ts] });
    // [ts] });
    // [ts] test("reuse first tab when navigating", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const pages = browserContext.pages();
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect_eq(pages.length, 1);
    // [ts] expect_eq(pages[0].title(), "Title");
    // [ts] });
    // [ts] test("create new tab in isolated context", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "new",
    // [ts] url: server.HELLO_WORLD,
    // [ts] isolatedContext: "alice",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: [](about:blank)
    // [ts] - 1: (current) [Title](${server.HELLO_WORLD}) [isolatedContext: alice]`,
    // [ts] page: expect.stringContaining("- Isolated context: alice"),
    // [ts] });
    // [ts] });
    // [ts] test("tabs in the same isolated context share storage", () => {
    // [ts] const newTab = (isolatedContext) => client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "new", url: server.EMPTY_PAGE, isolatedContext },
    // [ts] });
    // [ts] const selectTab = (index) => client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "select", index },
    // [ts] });
    // [ts] const setCookie = (value) => client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: `() => document.cookie = 'user=${value}'` },
    // [ts] });
    // [ts] const getCookie = () => client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.cookie" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] setCookie("default");
    // [ts] newTab("alice");
    // [ts] setCookie("alice");
    // [ts] newTab("bob");
    // [ts] setCookie("bob");
    // [ts] newTab("alice");
    // [ts] expect(getCookie()).toHaveResponse({ result: `"user=alice"` });
    // [ts] selectTab(0);
    // [ts] expect(getCookie()).toHaveResponse({ result: `"user=default"` });
    // [ts] selectTab(2);
    // [ts] expect(getCookie()).toHaveResponse({ result: `"user=bob"` });
    // [ts] newTab("default");
    // [ts] expect(getCookie()).toHaveResponse({ result: `"user=default"` });
    // [ts] });
}
