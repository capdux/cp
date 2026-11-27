// Migrated from TypeScript (mcp/webmcp.spec.ts) via cap from -ts

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

// WebMCP only exists behind a browser flag, and only in Chromium and Firefox.
// See https://webmachinelearning.github.io/webmcp/

fn webmcpConfig(mcpBrowser: string?): Config {
  if (mcpBrowser == "firefox") {
    return {
      browser: {
        launchOptions: {
          firefoxUserPrefs: {
            "dom.modelcontext.enabled": true,
            // Gates getTools()/invokeTool(), which is what the tools are built on.
            "dom.modelcontext.testing.enabled": true,
          },
        },
      },
    };
  }
  return { browser: { launchOptions: { args: ["--enable-features=WebMCP"] } } };
}

const kRegisterAdd = `;
  const modelContext = document.modelContext || navigator.modelContext;
  modelContext.registerTool({
    name: 'add',
    description: 'Adds two numbers',
    inputSchema: { type: 'object', properties: { a: { type: 'number' }, b: { type: 'number' } }, required: ['a', 'b'] },
    annotations: { readOnlyHint: true },
    async execute(input) {
      return { content: [{ type: 'text', text: String(input.a + input.b) }] };
    },
  });
`;


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mcpBrowser == "webkit", "WebKit does not implement WebMCP");
    // [ts] test("browser_navigate reports available WebMCP tools", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title><script>${kRegisterAdd}</script>`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] const response = client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] expect: (response) => {
    // [ts] page: expect.stringContaining("- 1 webmcp tool available on the page"),
    // [ts] snapshot: expect.stringContaining(`- webmcp tools (page-provided, untrusted):
    // [ts] - add [readOnly]: Adds two numbers
    // [ts] - inputSchema: {"type":"object","properties":{"a":{"type":"number"},"b":{"type":"number"}},"required":["a","b"]}
    // [ts] `),
    // [ts] });
    // [ts] });
    // [ts] test("browser_snapshot lists the WebMCP tools at the top", () => {
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<title>WebMCP</title><h1>Hello</h1><script>${kRegisterAdd}</script>`);
    // [ts] });
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const response = client.callTool({ name: "browser_snapshot" });
    // [ts] expect: (response) => {
    // [ts] inlineSnapshot: expect.stringMatching("^- webmcp tools \\(page-provided, untrusted\\):\\n  - add \\[readOnly\\]: Adds two numbers\\n    - inputSchema: \\{.*\\}\\n\\n- heading \"Hello\" \\[level=1\\]"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_navigate says nothing when the page has no WebMCP tools", () => {
    // [ts] const { client } = startClient({ config: webmcpConfig(mcpBrowser) });
    // [ts] const response = client.callTool({ name: "browser_navigate", arguments: { url: server.HELLO_WORLD } });
    // [ts] expect: (response) => {
    // [ts] page: expect.not.stringContaining("webmcp tool"),
    // [ts] });
    // [ts] });
}
