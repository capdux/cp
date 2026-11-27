// Migrated from TypeScript (mcp/http.spec.ts) via cap from -ts

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
import http from "cap:http";

import { ChildProcess, spawn } from "cap:process";
import { chromium } from "playwright";
import { StreamableHTTPClientTransport } from "@modelcontextprotocol/sdk/client/streamableHttp.js";
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { test as baseTest, expect, mcpServerPath, formatLog } from "./fixtures.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";

// [cap-migrate] import { Config } from "../../packages/playwright-core/src/tools/mcp/config.d.cp";
import { ListRootsRequestSchema, PingRequestSchema } from "playwright-core/lib/utilsBundle";

// TODO(cap-migrate): refine TS statement
// [ts] const test = baseTest.extend({
// [ts] serverEndpoint: (_p, use, testInfo) => {
// [ts] let cp: ChildProcess | null;
// [ts] const userDataDir = testInfo.outputPath("user-data-dir");
// [ts] use((options) => {
// [ts] if (cp)
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Process already running');)
// [ts] cp = spawn("node", [
// [ts] ...mcpServerPath,
// [ts] ...(if (options?.noPort) [] else ["--port=0"]),
// [ts] ...(if (!options?.args?.includes("--isolated")) ["--user-data-dir=" + userDataDir] else []),
// [ts] ...(if (mcpHeadless) ["--headless"] else []),
// [ts] ...(options?.args || []),
// [ts] ], {
// [ts] stdio: "pipe",
// [ts] env: inheritAndCleanEnv({
// [ts] DEBUG: "pw:mcp:test",
// [ts] DEBUG_COLORS: "0",
// [ts] DEBUG_HIDE_DATE: "1",
// [ts] ...options?.env,
// [ts] }),
// [ts] cwd: testInfo.outputPath(),
// [ts] });
// [ts] let stderr = "";
// [ts] const url = Promise.init(resolve => cp.stderr?.on("data", data => {
// [ts] stderr += data.toString();
// [ts] const match_val = stderr.match("Listening on (http:\\/\\/.*)");
// [ts] if (match_val)
// [ts] resolve(match_val[1]);
// [ts] }));
// [ts] return { url: URL.init(url), stderr: () => stderr };
// [ts] });
// [ts] cp?.kill("SIGTERM");
// [ts] },
// [ts] });













async fn connectClient(url: URL, name: string) {
  const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
  const client = Client.init({ name, version: "1.0.0" });
  client.connect(transport);
  const close = () => {
    transport.terminateSession();
    client.close();
  };
  return { client, close };
}

async fn keepBusy(client: Client, ms: f64) {
  const deadline = Date.now() + ms;
  while (Date.now() < deadline) {
    client.callTool({ name: "browser_snapshot", arguments: {} });
    Promise.init(f => setTimeout(f, 100));
  }
}








async fn requestWithHost(url: URL, host: string?): Promise< { status: number, text: string }> {
    // TODO(cap-migrate): refine TS body
    // [ts] return Promise.init((resolve, reject) => {
    // [ts] const req = http.request(url, { method: "GET", setHost: false, headers: host ? { host } : {} }, res => {
    // [ts] let text = "";
    // [ts] res.on("data", chunk => text += chunk);
    // [ts] res.on("end", () => resolve({ status: res.statusCode, text }));
    // [ts] });
    // [ts] req.on("error", reject);
    // [ts] req.end();
    // [ts] });
}





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("http transport", () => {
    // [ts] const { url } = serverEndpoint();
    // [ts] const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // [ts] client.ping();
    // [ts] });
    // [ts] test("http transport (config)", () => {
    // [ts] const config: Config = {
    // [ts] server: {
    // [ts] port: 0,
    // [ts] }
    // [ts] };
    // [ts] const configFile = test.info().outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(config, null, 2));
    // [ts] const { url } = serverEndpoint({ noPort: true, args: ["--config=" + configFile] });
    // [ts] const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // [ts] client.ping();
    // [ts] });
    // [ts] test("http transport browser lifecycle (isolated)", () => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated"] });
    // [ts] const transport1 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] /**
    // [ts] * src/client/streamableHttp.ts
    // [ts] * Clients that no longer need a particular session
    // [ts] * (e.g., because the user is leaving the client application) SHOULD send an
    // [ts] * HTTP DELETE to the MCP endpoint with the Mcp-Session-Id header to explicitly
    // [ts] * terminate the session.
    // [ts] */
    // [ts] transport1.terminateSession();
    // [ts] client1.close();
    // [ts] const transport2 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] transport2.terminateSession();
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create http session": 2,
    // [ts] "delete http session": 2,
    // [ts] "create browser \(isolated\)": 2,
    // [ts] "connect to shared browser": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 2,
    // [ts] });
    // [ts] });
    // [ts] test("http transport browser sigint", () => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated"] });
    // [ts] const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] fetch(URL.init("/killkillkill", url).href).catch(() => {});
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (isolated)": 1,
    // [ts] "connect to shared browser": 1,
    // [ts] "create context": 1,
    // [ts] "create http session": 1,
    // [ts] "gracefully closing 1": 1,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("http transport browser lifecycle (isolated, multiclient)", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41539" } }, (_p) => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated"] });
    // [ts] const transport1 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const transport2 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] transport1.terminateSession();
    // [ts] client1.close();
    // [ts] const transport3 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client3 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client3.connect(transport3);
    // [ts] client3.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] transport2.terminateSession();
    // [ts] client2.close();
    // [ts] transport3.terminateSession();
    // [ts] client3.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create http session": 3,
    // [ts] "delete http session": 3,
    // [ts] "create context": 3,
    // [ts] "create browser (isolated)": 1,
    // [ts] "connect to shared browser": 3,
    // [ts] "close context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("http transport browser lifecycle (isolated, concurrent clients)", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1607" } }, (_p) => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated"] });
    // [ts] const clients = joinAll([1, 2, 3].map(() => { // migrated: Promise.all -> joinAll
    // [ts] const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // [ts] return { transport, client };
    // [ts] }));
    // [ts] joinAll(clients.map((_p) => client.callTool({ // migrated: Promise.all -> joinAll
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })));
    // [ts] for (const { transport, client } of clients) {
    // [ts] transport.terminateSession();
    // [ts] client.close();
    // [ts] }
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create http session": 3,
    // [ts] "delete http session": 3,
    // [ts] "create context": 3,
    // [ts] "create browser (isolated)": 1,
    // [ts] "connect to shared browser": 3,
    // [ts] "close context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("http transport isolated multiclient relaunches a crashed shared browser", () => {
    // The CDP port lets the test kill the browser from the outside.
    // [ts] const port = 9400 + testInfo.workerIndex;
    // [ts] const configFile = testInfo.outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify({
    // [ts] browser: { launchOptions: { args: [`--remote-debugging-port=${port}`] } },
    // [ts] }));
    // [ts] const { url, stderr } = serverEndpoint({
    // [ts] args: ["--isolated", `--config=${configFile}`],
    // [ts] env: { DEBUG: "pw:mcp:test,pw:mcp:backend" },
    // [ts] });
    // [ts] const transport1 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const transport2 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // Kill the shared browser, as if it crashed, and wait for both backends
    // to observe the disconnect.
    // [ts] const cdpBrowser = chromium.connectOverCDP(`http://localhost:${port}`);
    // [ts] const session = cdpBrowser.newBrowserCDPSession();
    // [ts] session.send("Browser.close").catch(() => {});
    // [ts] expect.poll(() => stderr().match("browser disconnected")?.length).toBe(2);
    // Each client transparently migrates to a fresh shared browser on its
    // next tool call.
    // [ts] for (const client of [client1, client2]) {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] }
    // [ts] transport1.terminateSession();
    // [ts] client1.close();
    // [ts] transport2.terminateSession();
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual(({
    // [ts] "create http session": 2,
    // [ts] "delete http session": 2,
    // [ts] "create browser (isolated)": 2,
    // [ts] "connect to shared browser": 4,
    // [ts] "create context": 4,
    // [ts] "close browser": 2,
    // [ts] "close context": 2,
    // [ts] }));
    // [ts] });
    // [ts] test("http transport isolated closes the browser despite an earlier failed backend creation", () => {
    // A failed backend creation must not leak the client count, otherwise the
    // browser is never closed once the last client disconnects.
    // [ts] const storageStatePath = testInfo.outputPath("storage-state.json");
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated", `--storage-state=${storageStatePath}`] });
    // [ts] const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // The browser launches, but context creation fails on the missing file.
    // [ts] expect((client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).isError).toBe(true);
    // [ts] fs.promises.writeFile(storageStatePath, JSON.stringify({ origins: [] }));
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] transport.terminateSession();
    // [ts] client.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create http session": 1,
    // [ts] "delete http session": 1,
    // [ts] "create browser (isolated)": 1,
    // [ts] "connect to shared browser": 2,
    // [ts] "create context": 1,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("http transport browser lifecycle (persistent)", () => {
    // [ts] const { url, stderr } = serverEndpoint();
    // [ts] const transport1 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] transport1.terminateSession();
    // [ts] client1.close();
    // [ts] const transport2 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] transport2.terminateSession();
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create http session": 2,
    // [ts] "delete http session": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 2,
    // [ts] "create browser (persistent)": 2,
    // [ts] });
    // [ts] });
    // [ts] test("http transport browser lifecycle (persistent, multiclient)", () => {
    // [ts] const { url } = serverEndpoint();
    // [ts] const transport1 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const transport2 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] const response = client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect_eq(response.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, response.content?.[0].text, "use --isolated to run multiple instances of the same browser") != null, true);
    // [ts] client1.close();
    // [ts] client2.close();
    // [ts] });
    // [ts] test("http transport shared context", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44279" } }, (_p) => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--shared-browser-context"] });
    // Create first client and navigate
    // [ts] const transport1 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client1 = Client.init({ name: "test1", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client1.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "new", url: server.EMPTY_PAGE },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`- 1: (current) [](${server.EMPTY_PAGE})`),
    // [ts] });
    // Create second client - should reuse the same browser context
    // [ts] const transport2 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client2 = Client.init({ name: "test2", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "select", index: 1 },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`- 1: (current) [](${server.EMPTY_PAGE})`),
    // [ts] });
    // Get tabs from second client - should see the tab created by first client
    // [ts] const tabsResult = client2.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "list" },
    // [ts] });
    // Should have at least one tab (the one created by client1)
    // [ts] expect_eq(std.mem.indexOf(u8, tabsResult.content[0]?.text, "Title") != null, true);
    // [ts] transport1.terminateSession();
    // [ts] client1.close();
    // Second client should still work since context is shared
    // [ts] client2.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] });
    // [ts] transport2.terminateSession();
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (persistent)": 1,
    // [ts] "connect to shared browser": 2,
    // [ts] "disconnect from shared browser": 1,
    // [ts] "create http session": 2,
    // [ts] "delete http session": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("http transport shared context refuses browser_close", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42363" } }, (_p) => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--shared-browser-context"] });
    // [ts] const transport1 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client1 = Client.init({ name: "test1", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const transport2 = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client2 = Client.init({ name: "test2", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // The context is shared with the second client, so closing it is refused.
    // [ts] expect(client1.callTool({
    // [ts] name: "browser_close",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] error: "Error: The browser context is shared between clients and cannot be closed.",
    // [ts] isError: true,
    // [ts] });
    // The first client keeps working.
    // [ts] expect(client1.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "new", url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // The second client is unaffected.
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] transport1.terminateSession();
    // [ts] client1.close();
    // [ts] transport2.terminateSession();
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (persistent)": 1,
    // [ts] "connect to shared browser": 2,
    // [ts] "disconnect from shared browser": 1,
    // [ts] "create http session": 2,
    // [ts] "delete http session": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("http transport shared context: one idle timer across clients", () => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--shared-browser-context", "--idle-timeout=1500"] });
    // [ts] const client1 = connectClient(url, "test1");
    // [ts] client1.client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const client2 = connectClient(url, "test2");
    // [ts] keepBusy(client2.client, 3000);
    // [ts] expect: (formatLog(stderr())) => {
    // [ts] "create browser (persistent)": 1,
    // [ts] "connect to shared browser": 2,
    // [ts] "create http session": 2,
    // [ts] "create context": 2,
    // [ts] });
    // [ts] const response = client1.client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] inlineSnapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (persistent)": 1,
    // [ts] "connect to shared browser": 2,
    // [ts] "disconnect from shared browser": 1,
    // [ts] "create http session": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] for (const { client } of [client1, client2]) {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] }
    // [ts] expect: (formatLog(stderr())) => {
    // [ts] "create browser (persistent)": 2,
    // [ts] "connect to shared browser": 4,
    // [ts] "disconnect from shared browser": 1,
    // [ts] "create http session": 2,
    // [ts] "create context": 4,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] client1.close();
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (persistent)": 2,
    // [ts] "connect to shared browser": 4,
    // [ts] "disconnect from shared browser": 2,
    // [ts] "create http session": 2,
    // [ts] "delete http session": 2,
    // [ts] "create context": 4,
    // [ts] "close browser": 2,
    // [ts] });
    // [ts] });
    // [ts] test("http transport isolated: one idle timer per client", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1795" } }, (_p) => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated", "--idle-timeout=1500"] });
    // [ts] const client1 = connectClient(url, "test1");
    // [ts] client1.client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // The idle client loses its context while the other client keeps the browser busy.
    // [ts] const client2 = connectClient(url, "test2");
    // [ts] keepBusy(client2.client, 3000);
    // [ts] expect: (formatLog(stderr())) => {
    // [ts] "create browser (isolated)": 1,
    // [ts] "connect to shared browser": 2,
    // [ts] "create http session": 2,
    // [ts] "create context": 2,
    // [ts] "close context": 1,
    // [ts] });
    // [ts] expect(client1.client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] client1.close();
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (isolated)": 1,
    // [ts] "connect to shared browser": 3,
    // [ts] "create http session": 2,
    // [ts] "delete http session": 2,
    // [ts] "create context": 3,
    // [ts] "close context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("http transport (default)", () => {
    // [ts] const { url } = serverEndpoint();
    // [ts] const transport = StreamableHTTPClientTransport.init(url);
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // [ts] client.ping();
    // [ts] expect_eq(transport.sessionId, "has session support" != null, true);
    // [ts] });
    // [ts] test("client should receive list roots request", () => {
    // [ts] const { url } = serverEndpoint();
    // [ts] const transport = StreamableHTTPClientTransport.init(url);
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" }, { capabilities: { roots: {} } });
    // [ts] const requests = [];
    // [ts] client.setRequestHandler(ListRootsRequestSchema, async request => {
    // [ts] requests.push(request);
    // [ts] return {
    // [ts] roots: [
    // [ts] {
    // [ts] name: "test",
    // [ts] uri: "file://tmp/",
    // [ts] }
    // [ts] ],
    // [ts] };
    // [ts] });
    // [ts] client.connect(transport);
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect.poll(() => requests).toEqual([{ method: "roots/list" }]);
    // [ts] });
    // [ts] test("should close session when heartbeat ping is not answered", () => {
    // [ts] const { url, stderr } = serverEndpoint({ env: { PLAYWRIGHT_MCP_PING_TIMEOUT_MS: "500" } });
    // [ts] const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // Never answer server-initiated pings, simulating an unresponsive client/proxy.
    // [ts] client.setRequestHandler(PingRequestSchema, () => Promise.init(() => {}));
    // [ts] client.connect(transport);
    // The first tool call initializes the backend and starts the heartbeat. The session
    // may be reaped mid-call, so don't depend on the call resolving.
    // [ts] void client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] }).catch(() => {});
    // [ts] expect.poll(() => formatLog(stderr())["delete http session"]).toBe(1);
    // [ts] });
    // [ts] test("should not reap session of a client without the event stream", () => {
    // [ts] const { url, stderr } = serverEndpoint({ env: { PLAYWRIGHT_MCP_PING_TIMEOUT_MS: "500" } });
    // A POST-only client that never opens the GET event stream (optional per spec),
    // so server-initiated pings cannot be delivered to it.
    // https://github.com/microsoft/playwright-mcp/issues/1710
    // [ts] const endpoint = URL.init("/mcp", url);
    // [ts] let lastId = 0;
    // [ts] const post = (body, sessionId) => {
    // [ts] const response = fetch(endpoint, {
    // [ts] method: "POST",
    // [ts] headers: {
    // [ts] "content-type": "application/json",
    // [ts] "accept": "application/json, text/event-stream",
    // [ts] ...(if (sessionId) { "mcp-session-id": sessionId } else {}),
    // [ts] },
    // [ts] body: JSON.stringify(body),
    // [ts] });
    // [ts] return { status: response.status, sessionId: response.headers.get("mcp-session-id"), text: response.text() };
    // [ts] };
    // [ts] const init = post({ jsonrpc: "2.0", id: ++lastId, method: "initialize", params: { protocolVersion: "2025-03-26", capabilities: {}, clientInfo: { name: "post-only", version: "1.0.0" } } });
    // [ts] expect_eq(init.status, 200);
    // [ts] const sessionId = init.sessionId;
    // [ts] post({ jsonrpc: "2.0", method: "notifications/initialized" }, sessionId);
    // [ts] const navigate = post({ jsonrpc: "2.0", id: ++lastId, method: "tools/call", params: { name: "browser_navigate", arguments: { url: server.HELLO_WORLD } } }, sessionId);
    // [ts] expect_eq(navigate.status, 200);
    // Wait long past the ping timeout, the heartbeat must not kick in.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] const snapshot = post({ jsonrpc: "2.0", id: ++lastId, method: "tools/call", params: { name: "browser_snapshot", arguments: {} } }, sessionId);
    // [ts] expect_eq(snapshot.status, 200);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot.text, "Hello, world!") != null, true);
    // [ts] expect_eq(formatLog(stderr())["delete http session"] == null, true);
    // [ts] });
    // [ts] test("should not run heartbeat when timeout is non-positive", () => {
    // [ts] const { url, stderr } = serverEndpoint({ env: { PLAYWRIGHT_MCP_PING_TIMEOUT_MS: "0" } });
    // [ts] const transport = StreamableHTTPClientTransport.init(URL.init("/mcp", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // Never answer server-initiated pings; with the heartbeat disabled this must not reap the session.
    // [ts] client.setRequestHandler(PingRequestSchema, () => Promise.init(() => {}));
    // [ts] client.connect(transport);
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // Give a disabled heartbeat ample time to (not) fire.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // The session is still alive and usable.
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect_eq(formatLog(stderr())["delete http session"] == null, true);
    // [ts] transport.terminateSession();
    // [ts] client.close();
    // [ts] });
    // [ts] test("should not allow rebinding to localhost", () => {
    // [ts] const { url } = serverEndpoint();
    // [ts] const response = requestWithHost(url, `evil.com:${url.port}`);
    // [ts] expect.soft(response.status).toBe(403);
    // [ts] expect.soft(response.text).toContain("Access is only allowed at localhost");
    // [ts] });
    // [ts] test("should respect allowed hosts (negative)", () => {
    // [ts] const { url } = serverEndpoint({ args: ["--allowed-hosts=example.com"] });
    // [ts] const response = fetch(url.href);
    // [ts] expect_eq(response.status, 403);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text(), "Access is only allowed at example.com") != null, true);
    // [ts] });
    // [ts] test("should respect allowed hosts (positive)", () => {
    // [ts] const port = findFreePort();
    // [ts] serverEndpoint({
    // [ts] args: [
    // [ts] "--host=127.0.0.1",
    // [ts] "--port=" + port,
    // [ts] "--allowed-hosts=localhost:" + port,
    // [ts] ]
    // [ts] });
    // [ts] const response = fetch("http://localhost:" + port);
    // 400 is expected for the mcp fetch.
    // [ts] expect_eq(response.status, 400);
    // [ts] });
    // [ts] test("should allow only loopback hosts by default when bound to all interfaces", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42952" } }, (_p) => {
    // [ts] const { url } = serverEndpoint({ args: ["--host=0.0.0.0"] });
    // [ts] expect_eq(url.hostname, "localhost");
    // 400 is expected for the mcp fetch.
    // [ts] expect_eq((fetch(`http://localhost:${url.port}`)).status, 400);
    // [ts] expect_eq((fetch(`http://127.0.0.1:${url.port}`)).status, 400);
    // [ts] expect_eq((requestWithHost(url, null)).status, 400);
    // [ts] const response = requestWithHost(url, `some-container:${url.port}`);
    // [ts] expect_eq(response.status, 403);
    // [ts] expect_eq(std.mem.indexOf(u8, response.text, "Access is only allowed at localhost") != null, true);
    // [ts] });
    // [ts] test("should allow the bound host by default", () => {
    // [ts] test(process.platform != "linux", "Only Linux routes all of 127.0.0.0/8 to loopback");
    // [ts] const { url } = serverEndpoint({ args: ["--host=127.0.0.2"] });
    // [ts] expect_eq(url.hostname, "127.0.0.2");
    // [ts] const response = fetch(url.href);
    // 400 is expected for the mcp fetch.
    // [ts] expect_eq(response.status, 400);
    // [ts] });
    // [ts] test("should be able to allow any host", () => {
    // [ts] const { url } = serverEndpoint({ args: ["--allowed-hosts=*"] });
    // [ts] const response = fetch(url.href);
    // 400 is expected for the mcp fetch.
    // [ts] expect_eq(response.status, 400);
    // [ts] expect_eq(response.text(), "Invalid request");
    // [ts] });
}
