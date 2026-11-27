// Migrated from TypeScript (mcp/webmcp-dynamic.spec.ts) via cap from -ts

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

// [cap-migrate] import { Config } from "../../packages/playwright-core/src/tools/mcp/config.d.cp";


fn webmcpConfig(mcpBrowser: string?): Config {
  if (mcpBrowser == "firefox") {
    return {
      browser: {
        launchOptions: {
          firefoxUserPrefs: {
            "dom.modelcontext.enabled": true,
            "dom.modelcontext.testing.enabled": true,
          },
        },
      },
    };
  }
  return { browser: { launchOptions: { args: ["--enable-features=WebMCP"] } } };
}

fn registerScript(tools: string) {
  return `<script>
    const modelContext = document.modelContext || navigator.modelContext;
    ${tools}
  </script>`;
}

const kAdd = `;
  modelContext.registerTool({
    name: 'add',
    description: 'Adds two numbers',
    inputSchema: { type: 'object', properties: { a: { type: 'number' }, b: { type: 'number' } }, required: ['a', 'b'] },
    annotations: { readOnlyHint: true },
    async execute(input) { return { content: [{ type: 'text', text: String(input.a + input.b) }] }; },
  });
`;


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mcpBrowser == "webkit", "WebKit does not implement WebMCP");
    // [ts] test("page tools are listed as MCP tools with disclaimers", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title>${registerScript(kAdd)}`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] expect_eq(std.mem.indexOf(u8, (client.listTools()).tools.map(tool => tool.name), "webmcp_add") == null, true);
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const tool = (client.listTools()).tools.find(t => t.name == "webmcp_add");
    // [ts] expect_eq(tool, true);
    // [ts] expect_eq(std.mem.indexOf(u8, tool.description, "[UNTRUSTED:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, tool.description, "[READ-ONLY]") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, tool.description, "Adds two numbers") != null, true);
    // [ts] expect_eq(tool.annotations?.readOnlyHint, true);
    // [ts] expect: (tool!.inputSchema) => {
    // [ts] type: "object",
    // [ts] properties: { a: { type: "number" }, b: { type: "number" } },
    // [ts] required: ["a", "b"],
    // [ts] });
    // [ts] });
    // [ts] test("a consequential tool is called out in the description", () => {
    // consequentialHint landed in Chromium 154, Chrome stable and Firefox drop it silently.
    // [ts] test(mcpBrowser != "chromium", "Only bundled Chromium reports the consequential annotation");
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title>${registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: "checkout",
    // [ts] description: "Places the order",
    // [ts] annotations: { consequentialHint: true },
    // [ts] async execute() { return { content: [{ type: "text", text: "ordered" }] }; },
    // [ts] });
    // [ts] `)}`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const tool = (client.listTools()).tools.find(t => t.name == "webmcp_checkout");
    // [ts] expect_eq(std.mem.indexOf(u8, tool.description, "[CONSEQUENTIAL:") != null, true);
    // [ts] expect_eq(tool.annotations?.readOnlyHint, false);
    // [ts] expect_eq(tool.annotations?.destructiveHint, true);
    // [ts] });
    // [ts] test("a page tool can be called directly", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title>${registerScript(kAdd)}`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect: (client.callTool({ name) => {
    // [ts] result: expect.stringContaining("\"text\": \"42\""),
    // [ts] });
    // [ts] });
    // [ts] test("a page tool that answers with isError fails the call", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title>${registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: "broken",
    // [ts] description: "Always fails",
    // [ts] async execute() { return { content: [{ type: "text", text: "nope" }], isError: true }; },
    // [ts] });
    // [ts] `)}`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const response = client.callTool({ name: "webmcp_broken", arguments: {} });
    // [ts] expect_eq(response.isError, true);
    // [ts] expect: (response) => {
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("\"isError\": true"),
    // [ts] });
    // [ts] });
    // [ts] test("calling a tool the page no longer registers reports it as not found", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title>${registerScript(kAdd)}`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.HELLO_WORLD } });
    // [ts] const response = client.callTool({ name: "webmcp_add", arguments: { a: 1, b: 1 } });
    // [ts] expect_eq(response.isError, true);
    // [ts] expect: (response) => {
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("Tool \"webmcp_add\" not found"),
    // [ts] });
    // [ts] });
    // [ts] test("only the current tab contributes tools, switching tabs swaps them", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>one</title>${registerScript(kAdd)}`);
    // [ts] });
    // [ts] server.setRoute("/second", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>two</title>${registerScript(`
    // [ts] modelContext.registerTool({
    // [ts] name: "subtract",
    // [ts] description: "Subtracts two numbers",
    // [ts] async execute() { return { content: [{ type: "text", text: "0" }] }; },
    // [ts] });
    // [ts] `)}`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const names = () => (client.listTools()).tools.map(tool => tool.name).filter(name => name.startsWith("webmcp_"));
    // [ts] expect_eq(names(), ["webmcp_add"]);
    // [ts] client.callTool({ name: "browser_tabs", arguments: { action: "new" } });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX + "/second" } });
    // [ts] expect_eq(names(), ["webmcp_subtract"]);
    // [ts] client.callTool({ name: "browser_tabs", arguments: { action: "select", index: 0 } });
    // [ts] expect_eq(names(), ["webmcp_add"]);
    // [ts] });
    // [ts] test("a tool registered in an iframe can be called after the page is reloaded", () => {
    // [ts] test(mcpBrowser == "firefox", "Firefox does not list tools registered in iframes");
    // [ts] server.setRoute("/frame", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>frame</title>${registerScript(kAdd)}`);
    // [ts] });
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>top</title><iframe src="/frame"></iframe>`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect: (client.callTool({ name) => {
    // [ts] result: expect.stringContaining("\"text\": \"3\""),
    // [ts] });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect: (client.callTool({ name) => {
    // [ts] result: expect.stringContaining("\"text\": \"5\""),
    // [ts] });
    // [ts] });
    // [ts] test("--no-webmcp opts out of page tools entirely", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title>${registerScript(kAdd)}`);
    // [ts] });
    // [ts] const { client } = startClient({
    // [ts] config: { ...webmcpConfig(mcpBrowser), webmcp: false },
    // [ts] });
    // [ts] const response = client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect: (response) => {
    // [ts] page: expect.not.stringContaining("webmcp tool"),
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, (client.listTools()).tools.map(tool => tool.name), "webmcp_add") == null, true);
    // [ts] });
    // [ts] } // [cap-migrate] auto-closed brace
    // [ts] } // [cap-migrate] auto-closed brace
    // [ts] } // [cap-migrate] auto-closed brace
}
