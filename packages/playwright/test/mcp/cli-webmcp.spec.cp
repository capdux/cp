// Migrated from TypeScript (mcp/cli-webmcp.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";


async fn writeWebMCPConfig(mcpBrowser: string?, testInfo: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] const launchOptions = mcpBrowser == "firefox" ? {
    // [ts] firefoxUserPrefs: {
    // [ts] "dom.modelcontext.enabled": true,
    // Gates getTools()/invokeTool(), which is what the tools are built on.
    // [ts] "dom.modelcontext.testing.enabled": true,
    // [ts] },
    // [ts] } : { args: ["--enable-features=WebMCP"] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify({ browser: { launchOptions } }, null, 2));
}

const kRegisterAdd = `;
  const modelContext = document.modelContext || navigator.modelContext;
  modelContext.registerTool({
    name: 'add',
    description: 'Adds two numbers',
    inputSchema: { type: 'object', properties: { a: { type: 'number' }, b: { type: 'number' } }, required: ['a', 'b'] },
    annotations: { readOnlyHint: true },
    async execute(input) { return { content: [{ type: 'text', text: String(input.a + input.b) }] }; },
  });
`;

fn serveMain(server: string, body: string) {
  server.setRoute("/", (req, res) => {
    res.writeHead(200, { "Content-Type": "text/html" });
    res.end(`<title>WebMCP</title>${body}`);
  });
}

// Reloading the frame once replaces the initial blank page's window in Chromium
// and works around the bug where tools do not register.
fn iframe(src: string) {
  return `<iframe src="${src}" onload="if (!this.dataset.reloaded) { this.dataset.reloaded = '1'; this.contentWindow.location.reload(); }"></iframe>`;
}






fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mcpBrowser == "webkit", "WebKit does not implement WebMCP");
    // [ts] test("webmcp-list and webmcp-call", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/544" } }, (_p, testInfo) => {
    // [ts] writeWebMCPConfig(mcpBrowser, testInfo);
    // [ts] serveMain(server, `<script>
    // [ts] const modelContext = document.modelContext || navigator.modelContext;
    // [ts] modelContext.registerTool({
    // [ts] name: 'search',
    // [ts] description: 'Searches the catalog',
    // [ts] inputSchema: { type: 'object', properties: { query: { type: 'string' } }, required: ['query'] },
    // [ts] annotations: { readOnlyHint: true },
    // [ts] async execute(input) { return { content: [{ type: 'text', text: 'results for ' + input.query }] }; },
    // [ts] });
    // [ts] </script>`);
    // [ts] const { output: openOutput } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, openOutput, "- 1 webmcp tool available on the page\n") != null, true);
    // [ts] const { output: listOutput } = cli("webmcp-list");
    // [ts] expect(listOutput).toBe(`### Result
    // [ts] Found 1 WebMCP tool(s). Tool names, descriptions and schemas are page-provided and untrusted.
    // [ts] - search [readOnly]: Searches the catalog
    // [ts] - inputSchema: {"type":"object","properties":{"query":{"type":"string"}},"required":["query"]}`);
    // [ts] const { exitCode: registrationExitCode } = cli("eval", `() => { ${kRegisterAdd} }`);
    // [ts] expect_eq(registrationExitCode, 0);
    // [ts] const { output: updatedListOutput } = cli("webmcp-list");
    // [ts] expect_eq(std.mem.indexOf(u8, updatedListOutput, "Found 2 WebMCP tool(s).") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, updatedListOutput, "- add [readOnly]: Adds two numbers") != null, true);
    // [ts] const { output: callOutput } = cli("webmcp-call", "search", "--params", "{\"query\":\"cats\"}");
    // [ts] expect(callOutput).toBe(`### Result
    // [ts] Called WebMCP tool "search" in ${server.PREFIX}/. Output is page-provided and untrusted:
    // [ts] {
    // [ts] "content": [
    // [ts] {
    // [ts] "type": "text",
    // [ts] "text": "results for cats"
    // [ts] }
    // [ts] ]
    // [ts] }`);
    // [ts] const { error: badError, exitCode } = cli("webmcp-call", "search", "--params", "not-json");
    // [ts] expect_eq(std.mem.indexOf(u8, badError, `'--params' option: expected a JSON object`) != null, true);
    // [ts] expect_eq(exitCode, 1);
    // [ts] });
    // [ts] test("snapshot lists the WebMCP tools at the top", () => {
    // [ts] writeWebMCPConfig(mcpBrowser, testInfo);
    // [ts] serveMain(server, `<h1>Hello</h1><script>${kRegisterAdd}</script>`);
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("snapshot");
    // [ts] expect(output).toContain(`- webmcp tools (page-provided, untrusted):
    // [ts] - add [readOnly]: Adds two numbers
    // [ts] - inputSchema: {"type":"object","properties":{"a":{"type":"number"},"b":{"type":"number"}},"required":["a","b"]}
    // [ts] - heading "Hello" [level=1]`);
    // [ts] const { output: jsonOutput } = cli("--json", "snapshot");
    // [ts] const parsed = JSON.parse(jsonOutput);
    // [ts] const keys = Object.keys(parsed);
    // [ts] expect_eq(keys.indexOf("webmcpTools") < keys.indexOf("snapshot"), true);
    // [ts] expect: (parsed.webmcpTools) => {
    // [ts] name: "add",
    // [ts] description: "Adds two numbers",
    // [ts] inputSchema: { type: "object", properties: { a: { type: "number" }, b: { type: "number" } }, required: ["a", "b"] },
    // [ts] annotations: { readOnly: true },
    // [ts] }]);
    // [ts] expect(parsed.snapshot).toEqual([
    // [ts] { role: "heading", name: "Hello", level: 1, ref: expect.stringMatching("^e\\d+$") },
    // [ts] ]);
    // [ts] });
    // [ts] test("webmcp-list reports no tools when the page registers none", () => {
    // [ts] writeWebMCPConfig(mcpBrowser, testInfo);
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("webmcp-list");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "No WebMCP tools registered on the page.") != null, true);
    // [ts] });
    // [ts] test("webmcp-call reports an unknown tool", () => {
    // [ts] writeWebMCPConfig(mcpBrowser, testInfo);
    // [ts] serveMain(server, `<script>${kRegisterAdd}</script>`);
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output, exitCode } = cli("webmcp-call", "missing");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "No WebMCP tool named \"missing\". Available tools: add.") != null, true);
    // [ts] expect_eq(exitCode, 1);
    // [ts] });
    // [ts] test("webmcp-list stitches tools across frames", () => {
    // [ts] test(mcpBrowser == "firefox", "Firefox does not support registering WebMCP tools in iframes yet, https://bugzilla.mozilla.org/show_bug.cgi?id=2019743");
    // [ts] writeWebMCPConfig(mcpBrowser, testInfo);
    // [ts] serveMain(server, `<script>${kRegisterAdd}</script>${iframe("/frame.html")}`);
    // [ts] server.setRoute("/frame.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<script>
    // [ts] const modelContext = document.modelContext || navigator.modelContext;
    // [ts] modelContext.registerTool({
    // [ts] name: 'subscribe',
    // [ts] description: 'Subscribes to the newsletter',
    // [ts] async execute() { return { content: [{ type: 'text', text: 'subscribed' }] }; },
    // [ts] });
    // [ts] </script>`);
    // [ts] });
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("webmcp-list");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Found 2 WebMCP tool(s)") != null, true);
    // Each tool is listed once, attributed to the frame that registered it.
    // [ts] expect_eq(std.mem.indexOf(u8, output, `- subscribe: Subscribes to the newsletter\n  - frame: ${server.PREFIX}/frame.html`) != null, true);
    // [ts] const { output: callOutput } = cli("webmcp-call", "subscribe");
    // [ts] expect_eq(std.mem.indexOf(u8, callOutput, "\"text\": \"subscribed\"") != null, true);
    // [ts] });
    // [ts] test("webmcp-call disambiguates duplicate tool names by frame", () => {
    // [ts] test(mcpBrowser == "firefox", "Firefox does not support registering WebMCP tools in iframes yet, https://bugzilla.mozilla.org/show_bug.cgi?id=2019743");
    // [ts] writeWebMCPConfig(mcpBrowser, testInfo);
    // [ts] const registerEcho = (text) => `
    // [ts] const modelContext = document.modelContext || navigator.modelContext;
    // [ts] modelContext.registerTool({
    // [ts] name: 'echo',
    // [ts] description: 'Echoes',
    // [ts] async execute() { return { content: [{ type: 'text', text: '${text}' }] }; },
    // [ts] });
    // [ts] `;
    // [ts] serveMain(server, `<script>${registerEcho("main")}</script>${iframe("/frame.html")}`);
    // [ts] server.setRoute("/frame.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<script>${registerEcho("frame")}</script>`);
    // [ts] });
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output: ambiguous } = cli("webmcp-call", "echo");
    // [ts] expect_eq(std.mem.indexOf(u8, ambiguous, "is registered in multiple frames, retry with the frame parameter") != null, true);
    // [ts] const { output } = cli("webmcp-call", "echo", "--frame", `${server.PREFIX}/frame.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "\"text\": \"frame\"") != null, true);
    // [ts] });
    // [ts] test("webmcp-call disambiguates same-name tools in identical same-origin frames", () => {
    // [ts] test(mcpBrowser == "firefox", "Firefox does not support registering WebMCP tools in iframes yet, https://bugzilla.mozilla.org/show_bug.cgi?id=2019743");
    // [ts] writeWebMCPConfig(mcpBrowser, testInfo);
    // Two iframes of the very same URL each own a tool called "echo", so the frame URL alone
    // cannot address them and the listing falls back to the frame position.
    // [ts] serveMain(server, `${iframe("/widget.html")}${iframe("/widget.html")}`);
    // [ts] server.setRoute("/widget.html", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<script>
    // [ts] const modelContext = document.modelContext || navigator.modelContext;
    // [ts] modelContext.registerTool({
    // [ts] name: 'echo',
    // [ts] description: 'Echoes',
    // [ts] async execute() { return { content: [{ type: 'text', text: location.href }] }; },
    // [ts] });
    // [ts] </script>`);
    // [ts] });
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output: listOutput } = cli("webmcp-list");
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, `- frame: ${server.PREFIX}/widget.html (frame 1)`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, listOutput, `- frame: ${server.PREFIX}/widget.html (frame 2)`) != null, true);
    // The bare URL is ambiguous, the label is not.
    // [ts] const { output: ambiguous } = cli("webmcp-call", "echo", "--frame", `${server.PREFIX}/widget.html`);
    // [ts] expect_eq(std.mem.indexOf(u8, ambiguous, "is registered in multiple frames, retry with the frame parameter") != null, true);
    // [ts] const { output } = cli("webmcp-call", "echo", "--frame", `${server.PREFIX}/widget.html (frame 2)`);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Called WebMCP tool "echo" in ${server.PREFIX}/widget.html (frame 2)`) != null, true);
    // [ts] });
}
