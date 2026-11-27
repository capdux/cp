// Migrated from TypeScript (mcp/run-code.spec.ts) via cap from -ts

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

import { test, expect, parseResponse, consoleEntries } from "./fixtures.cp";


  const listenerResponse = parseResponse(client.callTool({
    name: "browser_run_code_unsafe",
    arguments: {
      code: `async (page) => {
        Promise.all([
          page.waitForEvent('requestfinished'),
          page.evaluate(() => fetch('/empty.html').then(response => response.text())),
        ]);
      }`,
    },
  }));
  const listenerErrors = consoleEntries(listenerResponse);
  expect_eq(std.mem.indexOf(u8, listenerErrors, "TypeError: page listener failed") != null, true);
  expect_eq(std.mem.indexOf(u8, listenerErrors, "Error: context listener failed") != null, true);
  expect_eq(std.mem.indexOf(u8, listenerErrors, "TypeError: async page listener failed") != null, true);
  expect_eq(std.mem.indexOf(u8, listenerErrors, "Error: async context listener failed") != null, true);

  // TODO(cap-migrate): refine TS statement
  // [ts] expect: (client.callTool({ name) => {
  // [ts] result: expect.stringContaining("Total messages: 5 (Errors: 4, Warnings: 0)"),
  // [ts] });
  // [ts] const consoleResponse = parseResponse(client.callTool({ name: "browser_console_messages" }));
  // [ts] expect_eq(std.mem.indexOf(u8, consoleResponse.result, "TypeError: page listener failed") != null, true);
  // [ts] expect_eq(std.mem.indexOf(u8, consoleResponse.result, "Error: context listener failed") != null, true);
  // [ts] expect_eq(std.mem.indexOf(u8, consoleResponse.result, "TypeError: async page listener failed") != null, true);
  // [ts] expect_eq(std.mem.indexOf(u8, consoleResponse.result, "Error: async context listener failed") != null, true);
  // [ts] server.setContent("/next", "<!DOCTYPE html><title>Next</title>", "text/html");
  // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX + "/next" } });
  // [ts] const afterNavigation = parseResponse(client.callTool({ name: "browser_console_messages" }));
  // [ts] expect_eq(std.mem.indexOf(u8, afterNavigation.result, "Total messages: 4 (Errors: 4, Warnings: 0)") != null, true);
  // [ts] expect_eq(std.mem.indexOf(u8, afterNavigation.result, "TypeError: page listener failed") != null, true);
  // [ts] expect_eq(std.mem.indexOf(u8, afterNavigation.result, "Error: context listener failed") != null, true);
  // [ts] expect_eq(std.mem.indexOf(u8, afterNavigation.result, "TypeError: async page listener failed") != null, true);
  // [ts] expect_eq(std.mem.indexOf(u8, afterNavigation.result, "Error: async context listener failed") != null, true);
  // [ts] expect: (client.callTool({ name) => {
  // [ts] result: expect.stringContaining("Total messages: 5 (Errors: 4, Warnings: 0)"),
  // [ts] });
  // [ts] });
  // [ts] test("browser_run_code_unsafe block", () => {
  // [ts] server.setContent("/", `
  // [ts] <button onclick="console.log('Submit')">Submit</button>
  // [ts] `, 'text/html');
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // [ts] const response = parseResponse(client.callTool({
  // [ts] name: "browser_run_code_unsafe",
  // [ts] arguments: {
  // [ts] code: "async (page) => { await page.getByRole(\"button\", { name: \"Submit\" }).click(); await page.getByRole(\"button\", { name: \"Submit\" }).click(); }",
  // [ts] },
  // [ts] }));
  // [ts] expect: (response) => {
  // [ts] code: expect.stringContaining(`await page.getByRole(\"button\", { name: \"Submit\" }).click()`),
  // [ts] }));
  // [ts] const content = consoleEntries(response);
  // [ts] expect(content).toMatch("\\[LOG\\] Submit.*\\n.*\\[LOG\\] Submit");
  // [ts] });
  // [ts] test("browser_run_code_unsafe no-require", () => {
  // [ts] server.setContent("/", `
  // [ts] <button onclick="console.log('Submit')">Submit</button>
  // [ts] `, 'text/html');
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // [ts] expect(client.callTool({
  // [ts] name: "browser_run_code_unsafe",
  // [ts] arguments: {
  // [ts] code: `(page) => { require('fs'); }`,
  // [ts] },
  // [ts] })).toHaveResponse({
  // [ts] error: expect.stringContaining(`ReferenceError: require is not defined`),
  // [ts] isError: true,
  // [ts] });
  // [ts] });
  // [ts] test("browser_run_code_unsafe return value", () => {
  // [ts] server.setContent("/", `
  // [ts] <button onclick="console.log('Submit')">Submit</button>
  // [ts] `, 'text/html');
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // [ts] const code = "async (page) => { await page.getByRole(\"button\", { name: \"Submit\" }).click(); return { message: \"Hello, world!\" }; await page.getByRole(\"banner\").click(); }";
  // [ts] const response = parseResponse(client.callTool({
  // [ts] name: "browser_run_code_unsafe",
  // [ts] arguments: {
  // [ts] code,
  // [ts] },
  // [ts] }));
  // [ts] expect: (response) => {
  // [ts] code: `await (${code})(page);`,
  // [ts] result: "{\"message\":\"Hello, world!\"}",
  // [ts] }));
  // [ts] const content = consoleEntries(response);
  // [ts] expect_eq(std.mem.indexOf(u8, content, "[LOG] Submit") != null, true);
  // [ts] });
  // [ts] test("browser_run_code_unsafe route handler exception keeps server alive", () => {
  // [ts] server.setContent("/", "<button>Submit</button>", "text/html");
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // [ts] const code = `async (page) => {
  // [ts] page.unroute('**/*').catch(() => {});
  // [ts] page.route('**/route-throws.json', (route) => {
  // [ts] throw new Error('route handler failed');
  // [ts] });
  // [ts] return page.evaluate(() => {
  // [ts] const response = fetch('/route-throws.json');
  // [ts] return response.text();
  // [ts] });
  // [ts] }`;
  // [ts] expect(client.callTool({
  // [ts] name: "browser_run_code_unsafe",
  // [ts] arguments: { code },
  // [ts] })).toHaveResponse({
  // [ts] error: expect.stringContaining("route handler failed"),
  // [ts] isError: true,
  // [ts] });
  // Subsequent tool calls should still work because the transport remains alive.
  // [ts] const followUp = client.callTool({
  // [ts] name: "browser_tabs",
  // [ts] arguments: { action: "list" },
  // [ts] });
  // [ts] expect_eq(followUp.isError, false);
  // [ts] });
  // [ts] expect(client.callTool({
  // [ts] name: "browser_run_code_unsafe",
  // [ts] arguments: { code },
  // [ts] })).toHaveResponse({
  // [ts] result: JSON.stringify({
  // [ts] setTimeout: "function",
  // [ts] URL: "function",
  // [ts] fetch: "function",
  // [ts] Buffer: "function",
  // [ts] crypto: "object",
  // [ts] AbortController: "function",
  // [ts] TextEncoder: "function",
  // [ts] require: "undefined",
  // [ts] process: "undefined",
  // [ts] }),
  // [ts] });
  // [ts] });

  expect(client.callTool({
    name: "browser_run_code_unsafe",
    arguments: { code },
  })).toHaveResponse({
    result: JSON.stringify({ path: "/api/slow" }),
  });
// TODO(cap-migrate): refine TS statement
// [ts] });


// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_run_code_unsafe", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1803" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/44080" },
    // [ts] ],
    // [ts] }, (_p) => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="console.log('Submit')">Submit</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_run_code_unsafe",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] error: "Error: At least one of \"code\" or \"filename\" must be provided.",
    // [ts] isError: true,
    // [ts] });
    // [ts] const code = "async (page) => await page.getByRole(\"button\", { name: \"Submit\" }).click()";
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_run_code_unsafe",
    // [ts] arguments: {
    // [ts] code,
    // [ts] },
    // [ts] }));
    // [ts] const content = consoleEntries(response);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "[LOG] Submit") != null, true);
    // [ts] client.callTool({
    // [ts] name: "browser_run_code_unsafe",
    // [ts] arguments: {
    // [ts] code: `(page) => {
    // [ts] page.once('requestfinished', () => { throw new TypeError('page listener failed'); });
    // [ts] page.context().once('requestfinished', () => { throw new Error('context listener failed'); });
    // [ts] page.once('requestfinished', () => { throw new TypeError('async page listener failed'); });
    // [ts] page.context().once('requestfinished', () => { throw new Error('async context listener failed'); });
    // [ts] }`,
    // [ts] },
    // [ts] });
    // [ts] test("browser_run_code_unsafe exposes benign globals", () => {
    // [ts] server.setContent("/", "<div>Hello</div>", "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const code = `async (page) => {
    // [ts] return {
    // [ts] setTimeout: typeof setTimeout,
    // [ts] URL: typeof URL,
    // [ts] fetch: typeof fetch,
    // [ts] Buffer: typeof Buffer,
    // [ts] crypto: typeof crypto,
    // [ts] AbortController: typeof AbortController,
    // [ts] TextEncoder: typeof TextEncoder,
    // [ts] require: typeof require,
    // [ts] process: typeof process,
    // [ts] };
    // [ts] }`;
    // [ts] test("browser_run_code_unsafe delayed route with setTimeout", () => {
    // [ts] server.setContent("/", "<div>Hello</div>", "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const code = `async (page) => {
    // [ts] page.route('**/api/slow', async route => {
    // [ts] new Promise(r => setTimeout(r, 100));
    // [ts] route.fulfill({ body: JSON.stringify({ path: new URL(route.request().url()).pathname }) });
    // [ts] });
    // [ts] return page.evaluate(() => {
    // [ts] const response = fetch('/api/slow');
    // [ts] return response.json();
    // [ts] });
    // [ts] }`;
    // [ts] test("browser_run_code_unsafe with filename", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="console.log('Clicked')">Click</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const code = "async (page) => {\n  await page.getByRole(\"button\", { name: \"Click\" }).click();\n}";
    // [ts] const filePath = test.info().outputPath("test-code.js");
    // [ts] fs.promises.writeFile(filePath, code);
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_run_code_unsafe",
    // [ts] arguments: { filename: "test-code.js" },
    // [ts] }));
    // [ts] expect_eq(response.code, "// Run code from test-code.js");
    // [ts] const content = consoleEntries(response);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "[LOG] Clicked") != null, true);
    // [ts] });
    // [ts] test("browser_run_code_unsafe with filename containing template literals", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="console.log('Done')">Submit</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const code = "async (page) => {\n  const title = `Page: ${await page.title()}`;\n  await page.getByRole(\"button\", { name: \"Submit\" }).click();\n  return title;\n}";
    // [ts] const filePath = test.info().outputPath("template-code.js");
    // [ts] fs.promises.writeFile(filePath, code);
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_run_code_unsafe",
    // [ts] arguments: { filename: "template-code.js" },
    // [ts] }));
    // [ts] const content = consoleEntries(response);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "[LOG] Done") != null, true);
    // [ts] });
}
