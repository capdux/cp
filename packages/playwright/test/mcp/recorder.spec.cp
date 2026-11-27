// Migrated from TypeScript (mcp/recorder.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";

// [cap-migrate] import { Client } from "@modelcontextprotocol/sdk/client/index.js";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";

test.use({ mcpCaps: ["devtools"] });

async fn navigateToForm(client: Client, server: TestServer) {
    // TODO(cap-migrate): refine TS body
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <button>Submit</button>
    // [ts] <input type="text">
    // [ts] `, 'text/html');
    // [ts] return client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
}








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("record actions between start and stop", () => {
    // [ts] navigateToForm(client, server);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_start_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("Recording started"),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: { element: "textbox", target: "e3", text: "Hello world" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining([
    // [ts] "Recording stopped. Recorded actions:",
    // [ts] "",
    // [ts] "```js",
    // [ts] `await page.getByRole('button', { name: 'Submit' }).click();`,
    // [ts] `await page.getByRole('textbox').fill('Hello world');`,
    // [ts] "```",
    // [ts] ].join("\n")),
    // [ts] });
    // [ts] });
    // [ts] test("record navigation", () => {
    // [ts] navigateToForm(client, server);
    // [ts] server.setContent("/page2", `<title>Page 2</title>`, "text/html");
    // [ts] client.callTool({ name: "browser_start_recording" });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/page2" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`await page.goto('${server.PREFIX}/page2');`),
    // [ts] });
    // [ts] });
    // [ts] test("stop with no actions recorded", () => {
    // [ts] navigateToForm(client, server);
    // [ts] client.callTool({ name: "browser_start_recording" });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("No actions were recorded."),
    // [ts] });
    // [ts] });
    // [ts] test("restarted recording only contains new actions", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <button>Alpha</button>
    // [ts] <button>Beta</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({ name: "browser_start_recording" });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Alpha button", target: "e2" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`await page.getByRole('button', { name: 'Alpha' }).click();`),
    // [ts] });
    // [ts] client.callTool({ name: "browser_start_recording" });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Beta button", target: "e3" },
    // [ts] });
    // Matching the entire block asserts no duplicate or stale actions.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining([
    // [ts] "```js",
    // [ts] `await page.getByRole('button', { name: 'Beta' }).click();`,
    // [ts] "```",
    // [ts] ].join("\n")),
    // [ts] });
    // [ts] });
    // [ts] test("actions performed while not recording are not included", () => {
    // [ts] navigateToForm(client, server);
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] });
    // [ts] client.callTool({ name: "browser_start_recording" });
    // [ts] client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: { element: "textbox", target: "e3", text: "Hello world" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining([
    // [ts] "```js",
    // [ts] `await page.getByRole('textbox').fill('Hello world');`,
    // [ts] "```",
    // [ts] ].join("\n")),
    // [ts] });
    // [ts] });
    // [ts] test("record actions in python", () => {
    // [ts] const { client } = startClient({ args: ["--codegen=python"] });
    // [ts] navigateToForm(client, server);
    // [ts] client.callTool({ name: "browser_start_recording" });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining([
    // [ts] "```python",
    // [ts] `page.get_by_role("button", name="Submit").click()`,
    // [ts] ].join("\n")),
    // [ts] });
    // [ts] });
    // [ts] test("start twice is an error", () => {
    // [ts] navigateToForm(client, server);
    // [ts] client.callTool({ name: "browser_start_recording" });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_start_recording",
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("Recording is already in progress"),
    // [ts] });
    // [ts] });
    // [ts] test("stop without start is an error", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("No recording in progress"),
    // [ts] });
    // [ts] });
    // [ts] test("clients sharing a context only receive their own recordings", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42608" } }, (_p) => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <button>Alpha</button>
    // [ts] <button>Beta</button>
    // [ts] <button>Gamma</button>
    // [ts] `, 'text/html');
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.goto(server.PREFIX);
    // Each client connects to the bound browser with its own connection.
    // [ts] const { client: client1 } = startClient({ args: ["--endpoint=default"] });
    // [ts] const { client: client2 } = startClient({ args: ["--endpoint=default"] });
    // [ts] client1.callTool({ name: "browser_snapshot" });
    // [ts] client2.callTool({ name: "browser_snapshot" });
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("No recording in progress"),
    // [ts] });
    // [ts] client1.callTool({ name: "browser_start_recording" });
    // [ts] client1.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Alpha button", target: "e2" },
    // [ts] });
    // [ts] client2.callTool({ name: "browser_start_recording" });
    // [ts] client1.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Beta button", target: "e3" },
    // [ts] });
    // [ts] expect(client1.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining([
    // [ts] "```js",
    // [ts] `await page.getByRole('button', { name: 'Alpha' }).click();`,
    // [ts] `await page.getByRole('button', { name: 'Beta' }).click();`,
    // [ts] "```",
    // [ts] ].join("\n")),
    // [ts] });
    // Stopping the first client does not stop the second one, which only sees
    // what happened after it started recording.
    // [ts] client2.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Gamma button", target: "e4" },
    // [ts] });
    // [ts] expect(client2.callTool({
    // [ts] name: "browser_stop_recording",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining([
    // [ts] "```js",
    // [ts] `await page.getByRole('button', { name: 'Beta' }).click();`,
    // [ts] `await page.getByRole('button', { name: 'Gamma' }).click();`,
    // [ts] "```",
    // [ts] ].join("\n")),
    // [ts] });
    // [ts] });
}
