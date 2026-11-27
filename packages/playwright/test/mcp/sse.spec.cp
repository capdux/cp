// Migrated from TypeScript (mcp/sse.spec.ts) via cap from -ts

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

import { ChildProcess, spawn } from "cap:process";
import { SSEClientTransport } from "@modelcontextprotocol/sdk/client/sse.js";
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { test as baseTest, expect, mcpServerPath, formatLog } from "./fixtures.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";

// [cap-migrate] import { Config } from "../../packages/playwright-core/src/tools/mcp/config.d.cp";

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
// [ts] DEBUG: "pw:mcp:test,pw:browser",
// [ts] DEBUG_COLORS: "0",
// [ts] DEBUG_HIDE_DATE: "1",
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






fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("sse transport", () => {
    // [ts] const { url } = serverEndpoint();
    // [ts] const transport = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // [ts] client.ping();
    // [ts] });
    // [ts] test("sse transport (config)", () => {
    // [ts] const config: Config = {
    // [ts] server: {
    // [ts] port: 0,
    // [ts] }
    // [ts] };
    // [ts] const configFile = test.info().outputPath("config.json");
    // [ts] fs.promises.writeFile(configFile, JSON.stringify(config, null, 2));
    // [ts] const { url } = serverEndpoint({ noPort: true, args: ["--config=" + configFile] });
    // [ts] const transport = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client.connect(transport);
    // [ts] client.ping();
    // [ts] });
    // [ts] test("sse transport browser lifecycle (isolated)", () => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated"] });
    // [ts] const transport1 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] client1.close();
    // [ts] const transport2 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create SSE session": 2,
    // [ts] "delete SSE session": 2,
    // [ts] "create context": 2,
    // [ts] "create browser (isolated)": 2,
    // [ts] "connect to shared browser": 2,
    // [ts] "close browser": 2,
    // [ts] });
    // [ts] });
    // [ts] test("sse transport browser lifecycle (isolated, multiclient)", () => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--isolated"] });
    // [ts] const transport1 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] client1.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => { localStorage.a = 42; }" },
    // [ts] });
    // [ts] const transport2 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => localStorage.a" },
    // [ts] })).toHaveResponse({
    // [ts] result: "undefined",
    // [ts] });
    // [ts] client1.close();
    // [ts] const transport3 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client3 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client3.connect(transport3);
    // [ts] client3.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] client2.close();
    // [ts] client3.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create SSE session": 3,
    // [ts] "delete SSE session": 3,
    // [ts] "create context": 3,
    // [ts] "create browser (isolated)": 1,
    // [ts] "connect to shared browser": 3,
    // [ts] "close context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("sse transport browser lifecycle (persistent, multiclient)", () => {
    // [ts] const { url } = serverEndpoint();
    // [ts] const transport1 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const transport2 = SSEClientTransport.init(URL.init("/sse", url));
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
    // [ts] test("sse transport browser lifecycle (persistent)", () => {
    // [ts] const { url, stderr } = serverEndpoint();
    // [ts] const transport1 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client1 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] client1.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => { localStorage.a = 42; }" },
    // [ts] });
    // [ts] client1.close();
    // The server closes the browser after the session is gone. Wait for the browser process to exit,
    // so that the next session does not start a browser on the same profile too early.
    // [ts] expect.poll(() => stderr()).toContain("<gracefully close end>");
    // [ts] const transport2 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client2 = Client.init({ name: "test", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // [ts] client2.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => localStorage.a" },
    // [ts] })).toHaveResponse({
    // [ts] result: "\"42\"",
    // [ts] });
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create SSE session": 2,
    // [ts] "delete SSE session": 2,
    // [ts] "create context": 2,
    // [ts] "create browser (persistent)": 2,
    // [ts] "close browser": 2,
    // [ts] });
    // [ts] });
    // [ts] test("sse transport shared context", () => {
    // [ts] const { url, stderr } = serverEndpoint({ args: ["--shared-browser-context"] });
    // Create first client and navigate
    // [ts] const transport1 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client1 = Client.init({ name: "test1", version: "1.0.0" });
    // [ts] client1.connect(transport1);
    // [ts] client1.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // Create second client - should reuse the same browser context
    // [ts] const transport2 = SSEClientTransport.init(URL.init("/sse", url));
    // [ts] const client2 = Client.init({ name: "test2", version: "1.0.0" });
    // [ts] client2.connect(transport2);
    // Get tabs from second client - should see the tab created by first client
    // [ts] const tabsResult = client2.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "list" },
    // [ts] });
    // Should have at least one tab (the one created by client1)
    // [ts] expect_eq(std.mem.indexOf(u8, tabsResult.content[0]?.text, "Title") != null, true);
    // [ts] client1.close();
    // Second client should still work since context is shared
    // [ts] client2.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] });
    // [ts] client2.close();
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create SSE session": 2,
    // [ts] "delete SSE session": 2,
    // [ts] "create browser (persistent)": 1,
    // [ts] "connect to shared browser": 2,
    // [ts] "create context": 2,
    // [ts] "disconnect from shared browser": 1,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
}
