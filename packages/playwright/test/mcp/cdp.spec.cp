// Migrated from TypeScript (mcp/cdp.spec.ts) via cap from -ts

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

import { spawnSync } from "cap:process";
import { test, expect, mcpServerPath } from "./fixtures.cp";

test.describe.configure({
  retries: 1,
});









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("cdp server", () => {
    // [ts] cdpServer.start();
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [active] [ref=e1]: Hello, world!`),
    // [ts] });
    // [ts] });
    // [ts] test("cdp server reuse tab", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] const [page] = browserContext.pages();
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Hello, world!",
    // [ts] target: "f0",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] error: `Error: "f0" does not match any elements.`,
    // [ts] isError: true,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] page: `- Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`,
    // [ts] inlineSnapshot: `- generic [active] [ref=e1]: Hello, world!`,
    // [ts] });
    // [ts] });
    // [ts] test("cdp connection uses noDefaults", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41661" });
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [page] = browserContext.pages();
    // [ts] page.emulateMedia({ media: "print" });
    // [ts] expect_eq(page.evaluate(() => matchMedia("print").matches), true);
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] client.callTool({ name: "browser_snapshot" });
    // [ts] expect_eq(page.evaluate(() => matchMedia("print").matches), true);
    // [ts] });
    // [ts] test("cdp connection does not wait for a busy page", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44052" },
    // [ts] }, (_p) => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [busyPage] = browserContext.pages();
    // [ts] busyPage.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] const page = browserContext.newPage();
    // [ts] page.goto(server.HELLO_WORLD);
    // [ts] const busyStarted = busyPage.waitForEvent("console", message => message.text() == "busy");
    // [ts] busyPage.evaluate(() => {
    // [ts] @print("busy");
    // [ts] for (;;) {}
    // [ts] }).catch(() => {});
    // [ts] busyStarted;
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] page: `- Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`,
    // [ts] inlineSnapshot: `- generic [active] [ref=e1]: Hello, world!`,
    // [ts] });
    // [ts] });
    // [ts] test("should throw connection error and allow re-connecting", () => {
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <body>Hello, world!</body>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`connect ECONNREFUSED`),
    // [ts] isError: true,
    // [ts] });
    // [ts] cdpServer.start();
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [active] [ref=e1]: Hello, world!`),
    // [ts] });
    // [ts] });
    // [ts] test("auto-recover when remote browser disconnects mid-session", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // Simulate the remote browser dying mid-session (e.g. CDP endpoint session timeout).
    // [ts] browserContext.close();
    // The next call hits the dead backend; it must error and let the MCP server discard
    // the backend so the next call can transparently establish a fresh connection.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] });
    // Bring the CDP endpoint back. The next call should reconnect transparently —
    // no manual browser_close needed (regression test for playwright-mcp#1588).
    // [ts] cdpServer.start();
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] });
    // [ts] test("transparently reconnects when the remote browser is restarted", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const { client, stderr } = startClient({
    // [ts] args: [`--cdp-endpoint=${cdpServer.endpoint}`],
    // [ts] env: { DEBUG: "pw:mcp:backend" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] browserContext.close();
    // [ts] expect.poll(() => stderr()).toContain("browser disconnected");
    // [ts] cdpServer.start();
    // The very next tool call must reconnect, with no failed call in between.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] });
    // [ts] test("does not support --device", () => {
    // [ts] const result = spawnSync("node", [
    // [ts] ...mcpServerPath, "--device=Pixel 5", "--cdp-endpoint=http://localhost:1234",
    // [ts] ]);
    // [ts] expect_eq(result.error == null, true);
    // [ts] expect_eq(result.status, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.stderr.toString(), "Device emulation is not supported with cdpEndpoint.") != null, true);
    // [ts] });
    // [ts] test("cdp server with headers", () => {
    // [ts] let authHeader = "";
    // [ts] server.setRoute("/json/version/", (req, res) => {
    // [ts] authHeader = req.headers["authorization"];
    // [ts] res.end();
    // [ts] });
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${server.PREFIX}`, "--cdp-header", "Authorization: Bearer 1234567890"] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] });
    // [ts] expect_eq(authHeader, "Bearer 1234567890");
    // [ts] });
    // [ts] test("cdp server with empty and complex headers", () => {
    // [ts] let customHeader = "";
    // [ts] let emptyHeader = "";
    // [ts] server.setRoute("/json/version/", (req, res) => {
    // [ts] customHeader = req.headers["x-forwarded-proto"];
    // [ts] emptyHeader = req.headers["x-empty"];
    // [ts] res.end();
    // [ts] });
    // [ts] const { client } = startClient({
    // [ts] args: [
    // [ts] `--cdp-endpoint=${server.PREFIX}`,
    // [ts] "--cdp-header", "X-Forwarded-Proto: value:with:colons",
    // [ts] "--cdp-header", "X-Empty"
    // [ts] ]
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] });
    // [ts] expect_eq(customHeader, "value:with:colons");
    // [ts] expect_eq(emptyHeader, "");
    // [ts] });
}
