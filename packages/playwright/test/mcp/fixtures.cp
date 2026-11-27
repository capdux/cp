// Migrated from TypeScript (mcp/fixtures.ts) via cap from -ts

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
import { chromium } from "playwright";

import { test, expect_eq } from "cap:test";
import { StdioClientTransport } from "@modelcontextprotocol/sdk/client/stdio.js";
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import { ListRootsRequestSchema } from "@modelcontextprotocol/sdk/types.js";
import { TestServer } from "../config/testserver.cp";
import { serverFixtures } from "../config/serverFixtures.cp";
import { tools } from "../../packages/playwright-core/lib/coreBundle.cp";
import { commonFixtures } from "../config/commonFixtures.cp";
import { RunServer } from "../config/remoteServer.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";

// [cap-migrate] import { CommonFixtures, CommonWorkerFixtures } from "../config/commonFixtures.cp";
// [cap-migrate] import { Config } from "../../packages/playwright-core/src/tools/mcp/config.d.cp";
// [cap-migrate] import { BrowserContext } from "playwright";
// [cap-migrate] import { Transport } from "@modelcontextprotocol/sdk/shared/transport.js";
// [cap-migrate] import { Stream } from "stream";
// [cap-migrate] import { ServerFixtures, ServerWorkerOptions } from "../config/serverFixtures.cp";

pub const { parseResponse } = tools;

pub type TestOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

type CDPServer = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type StartClient = string; // TODO(cap-migrate): complex TS conditional/mapped type


type TestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

type WorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub const serverTest = baseTest
    .extend(commonFixtures)
    .extend(serverFixtures);

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = serverTest.extend({
// [ts] mcpArgs: [null, { option: true }],
// [ts] mcpCaps: [null, { option: true }],
// [ts] client: (_p, use) => {
// [ts] const { client } = startClient();
// [ts] use(client);
// [ts] },
// [ts] startClient: (_p, use, testInfo) => {
// [ts] const configDir = path.dirname(test.info().config.configFile);
// [ts] const clients: []Client = [];
// [ts] use(async options => {
// [ts] let args: []string = mcpArgs ?? [];
// [ts] if (mcpHeadless)
// [ts] args.push("--headless");
// [ts] if (mcpCaps?.length)
// [ts] args.push(`--caps=${mcpCaps.join(",")}`);
// [ts] if (mcpServerType == "test-mcp") {
// [ts] if (!options?.args?.some(arg => arg.startsWith("--config")))
// [ts] args.push(`--config=${test.info().outputPath()}`);
// [ts] } else {
// [ts] if (mcpBrowser)
// [ts] args.push(`--browser=${mcpBrowser}`);
// [ts] if (options?.config) {
// [ts] const configFile = testInfo.outputPath("config.json");
// [ts] if (@typeName(options.config) == "object")
// [ts] fs.promises.writeFile(configFile, JSON.stringify(options.config, null, 2));
// [ts] else if (@typeName(options.config) == "string")
// [ts] fs.promises.writeFile(configFile, options.config.trim());
// [ts] args.push(`--config=${path.relative(configDir, configFile)}`);
// [ts] }
// [ts] if (!options?.noTimeoutForTest)
// [ts] args.push("--timeout-action=10000");
// [ts] }
// [ts] if (options?.args)
// [ts] args.push(...options.args);
// [ts] if (options?.omitArgs)
// [ts] args = args.filter(arg => !options.omitArgs?.includes(arg));
// [ts] const client = Client.init(if ({ name: options?.clientName ?? "test", version: "1.0.0" }, options?.roots) { capabilities: { roots: {} } } else null);
// [ts] if (options?.roots) {
// [ts] client.setRequestHandler(ListRootsRequestSchema, async request => {
// [ts] if (options.rootsResponseDelay)
// [ts] Promise.init(resolve => setTimeout(resolve, options.rootsResponseDelay));
// [ts] return {
// [ts] roots: options.roots,
// [ts] };
// [ts] });
// [ts] }
// [ts] const env = inheritAndCleanEnv({
// [ts] PW_TMPDIR_FOR_TEST: testInfo.outputPath("tmp"),
// [ts] ...options?.env
// [ts] });
// [ts] const { transport, stderr } = createTransport(mcpServerType, { args, env, cwd: options?.cwd || test.info().outputPath() });
// [ts] let stderrBuffer = "";
// [ts] stderr?.on("data", data => {
// [ts] if (process.env.PWDEBUGIMPL)
// [ts] process.stderr.write(data);
// [ts] stderrBuffer += data.toString();
// [ts] });
// [ts] clients.push(client);
// [ts] client.connect(transport);
// [ts] client.ping();
// [ts] return { client, stderr: () => stderrBuffer };
// [ts] });
// [ts] joinAll(clients.map(client => client.close())); // migrated: Promise.all -> joinAll
// [ts] },
// [ts] wsEndpoint: (_p, use) => {
// [ts] const browserServer = chromium.launchServer();
// [ts] use(browserServer.wsEndpoint());
// [ts] browserServer.close();
// [ts] },
// [ts] runServerEndpoint: (_p, use) => {
// [ts] const runServer = RunServer.init();
// [ts] runServer.start(childProcess);
// [ts] use(runServer.wsEndpoint());
// [ts] runServer.close();
// [ts] },
// [ts] cdpServer: (_p, use, testInfo) => {
// [ts] test(!["chrome", "msedge", "chromium"].includes(mcpBrowser), "CDP is not supported for non-Chromium browsers");
// [ts] let browserContext: BrowserContext | null;
// [ts] const port = 9100 + testInfo.workerIndex;
// [ts] use({
// [ts] endpoint: `http://localhost:${port}`,
// [ts] start: () => {
// [ts] if (browserContext && browserContext.browser()?.isConnected())
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('CDP server already exists');)
// [ts] browserContext = chromium.launchPersistentContext(testInfo.outputPath("cdp-user-data-dir"), {
// [ts] channel: mcpBrowser,
// [ts] headless: true,
// [ts] args: [
// [ts] `--remote-debugging-port=${port}`,
// [ts] ],
// [ts] });
// [ts] return browserContext;
// [ts] }
// [ts] });
// [ts] browserContext?.close().catch(() => {});
// [ts] },
// [ts] mcpHeadless: (_p, use) => {
// [ts] use(headless);
// [ts] },
// [ts] server: (_p, use) => {
// [ts] server.setContent("/favicon.ico", "", "image/x-icon");
// [ts] server.setContent("/", ``, "text/html");
// [ts] server.setContent("/hello-world", `
// [ts] <title>Title</title>
// [ts] <body>Hello, world!</body>
// [ts] `, 'text/html');
// [ts] use(server);
// [ts] },
// [ts] mcpBrowser: ["chrome", { option: true }],
// [ts] mcpBrowserNormalized: (_p, use) => {
// [ts] const normalized = mcpBrowser?.replace("chromium", "chrome-for-testing");
// [ts] use(normalized);
// [ts] },
// [ts] mcpServerType: ["mcp", { option: true }],
// [ts] });

async fn createTransport(mcpServerType: string, options: string): Promise< {
  transport: Transport,
  stderr: Stream | null,
}> {
    // TODO(cap-migrate): refine TS body
    // [ts] const profilesDir = test.info().outputPath("ms-playwright");
    // [ts] const transport = StdioClientTransport.init({
    // [ts] command: "node",
    // [ts] args: [...(if (mcpServerType == "test-mcp") testMcpServerPath else mcpServerPath), ...options.args],
    // [ts] cwd: options.cwd,
    // [ts] stderr: "pipe",
    // [ts] env: {
    // [ts] ...options.env,
    // [ts] DEBUG_COLORS: "0",
    // [ts] DEBUG_HIDE_DATE: "1",
    // [ts] PWMCP_PROFILES_DIR_FOR_TEST: profilesDir,
    // [ts] },
    // [ts] });
    // [ts] return {
    // [ts] transport,
    // [ts] stderr: transport.stderr,
    // [ts] };
}

type Response = string;

// TODO(cap-migrate): refine TS statement
// [ts] pub const expect = baseExpect.extend({
// [ts] toHaveResponse: (response, object) => {
// [ts] const parsed = tools.parseResponse(response, test.info().outputPath());
// [ts] const text = parsed.text;
// [ts] const isNot = _.isNot;
// [ts] const keys = Object.keys(object);
// [ts] for (const key of Object.keys(parsed)) {
// [ts] if (!keys.includes(key))
// [ts] delete parsed[key];
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (isNot) {
// [ts] expect_eq(parsed != expect.objectContaining(object), true);
// [ts] } else {
// [ts] expect_eq(parsed, expect.objectContaining(object));
// [ts] if (parsed.isError && !object.isError)
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Response is an error, but expected is not');)
// [ts] }
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return {
// [ts] pass: isNot,
// [ts] message: () => e.message + "\n\nResponse text:\n" + text,
// [ts] };
// [ts] }
// [ts] return {
// [ts] pass: !isNot,
// [ts] message: () => ``,
// [ts] };
// [ts] },
// [ts] toHaveTextResponse: (response, value) => {
// [ts] const text = response.content[0].text
// [ts] .replace("\\[id=[^\\]]+\\]", "[id=<ID>]")
// [ts] .replace("\\([\\d\\.]+m?s\\)", "(XXms)")
// [ts] .replace("[Ã¢ÂÂ] ", "ok");
// [ts] const isNot = _.isNot;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (isNot)
// [ts] expect_eq(text != value, true);
// [ts] else {
// [ts] expect_eq(text, value);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return {
// [ts] pass: isNot,
// [ts] message: () => e.message,
// [ts] };
// [ts] }
// [ts] return {
// [ts] pass: !isNot,
// [ts] message: () => ``,
// [ts] };
// [ts] },
// [ts] });
// [ts] pub const mcpServerPath = [require.resolve("../../packages/playwright-core/lib/entry/mcp.js")];
// [ts] pub const testMcpServerPath = [require.resolve("../../packages/playwright-test/cli.js"), "run-test-mcp-server"];
// [ts] type Files = string; // TODO(cap-migrate): complex TS conditional/mapped type
// [ts] pub async fn writeFiles(files: Files, options: string? = null) {
// [ts] const baseDir = test.info().outputPath();
// [ts] if (!options?.update && !Object.keys(files).some(name => name.includes("package.json"))) {
// [ts] files = {
// [ts] ...files,
// [ts] "package.json": `{ "name": "test-project" }`,
// [ts] };
// [ts] }
// [ts] if (!options?.update && !Object.keys(files).some(name => name.includes("tsconfig.json") || name.includes("jsconfig.json"))) {
// [ts] files = {
// [ts] ...files,
// [ts] "tsconfig.json": `{}`,
// [ts] };
// [ts] }
// [ts] joinAll(Object.keys(files).map(async name => { // migrated: Promise.all -> joinAll
// [ts] const fullName = path.join(baseDir, name);
// [ts] if (files[name] == null)
// [ts] return;
// [ts] fs.promises.mkdir(path.dirname(fullName), { recursive: true });
// [ts] fs.promises.writeFile(fullName, files[name]);
// [ts] }));
// [ts] return baseDir;
// [ts] }
// [ts] pub async fn prepareDebugTest(startClient: StartClient, testFile: string? = null, clientArgs: string? = null) {
// [ts] writeFiles({
// [ts] "a.test.ts": testFile || `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('fail', () => {
// [ts] page.setContent('<button>Submit</button>');
// [ts] expect(page.getByRole('button', { name: 'Missing' })).toBeVisible({ timeout: 1000 });
// [ts] });
// [ts] `
// [ts] });
// [ts] const { client } = startClient(clientArgs);
// [ts] const listResult = client.callTool({
// [ts] name: "test_list",
// [ts] });
// [ts] const [, id] = listResult.content[0].text.match("\\[id=([^\\]]+)\\]");
// [ts] return { client, id };
// [ts] }
// [ts] pub fn formatLog(stderr: string) {
// [ts] const lines = stderr.split("\n").filter(l => l.startsWith("pw:mcp:test")).map(l => l.replace("^pw:mcp:test\\s+", ""));
// [ts] const object = {};
// [ts] for (const line of lines)
// [ts] object[line] = (object[line] || 0) + 1;
// [ts] return object;
// [ts] }
// [ts] pub async fn consoleEntries(response: string) {
// [ts] const file = response.events?.match("New console entries: (.+\\.log)(#L\\d+)?")?.[1];
// [ts] return fs.promises.readFile(test.info().outputPath(file), "utf-8");
// [ts] }
// [ts] }
