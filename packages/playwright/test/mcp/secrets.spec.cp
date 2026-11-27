// Migrated from TypeScript (mcp/secrets.spec.ts) via cap from -ts

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





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_type", () => {
    // [ts] const secretsFile = test.info().outputPath("secrets.env");
    // [ts] fs.promises.writeFile(secretsFile, "X-PASSWORD=password123");
    // [ts] const { client } = startClient({
    // [ts] args: ["--secrets", secretsFile],
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <input type='keypress' onkeypress="console.log('Key pressed:', event.key, ', Text:', event.target.value)"></input>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: {
    // [ts] element: "textbox",
    // [ts] target: "e2",
    // [ts] text: "X-PASSWORD",
    // [ts] submit: true,
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] code: `await page.getByRole('textbox').fill(process.env['X-PASSWORD']);
    // [ts] page.getByRole('textbox').press('Enter');`,
    // [ts] snapshot: expect.stringMatching("textbox (\\[active\\] )?\\[ref=e2\\]: <secret>X-PASSWORD<\\/secret>"),
    // [ts] });
    // [ts] }
    // [ts] expect(client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`[LOG] Key pressed: Enter , Text: <secret>X-PASSWORD</secret>`),
    // [ts] });
    // [ts] });
    // [ts] test("secrets are redacted in console log artifact", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41509" } }, (_p) => {
    // [ts] const secretsFile = test.info().outputPath("secrets.env");
    // [ts] fs.promises.writeFile(secretsFile, "X-PASSWORD=password123");
    // [ts] const { client } = startClient({
    // [ts] args: ["--secrets", secretsFile],
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <input type='keypress' onkeypress="console.log('Key pressed:', event.key, ', Text:', event.target.value)"></input>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: {
    // [ts] element: "textbox",
    // [ts] target: "e2",
    // [ts] text: "X-PASSWORD",
    // [ts] submit: true,
    // [ts] },
    // [ts] }));
    // [ts] const consoleLog = consoleEntries(response);
    // [ts] expect_eq(std.mem.indexOf(u8, consoleLog, "password123") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, consoleLog, "<secret>X-PASSWORD</secret>") != null, true);
    // [ts] });
    // [ts] test("browser_fill_form", () => {
    // [ts] const secretsFile = test.info().outputPath("secrets.env");
    // [ts] fs.promises.writeFile(secretsFile, "X-PASSWORD=password123");
    // [ts] const { client } = startClient({
    // [ts] args: ["--secrets", secretsFile],
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <body>
    // [ts] <form>
    // [ts] <label>
    // [ts] <input type="email" id="email" name="email" />
    // [ts] Email
    // [ts] </label>
    // [ts] <label>
    // [ts] <input type="password" id="name" name="password" />
    // [ts] Password
    // [ts] </label>
    // [ts] </form>
    // [ts] </body>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_fill_form",
    // [ts] arguments: {
    // [ts] fields: [
    // [ts] {
    // [ts] name: "Email textbox",
    // [ts] type: "textbox",
    // [ts] target: "e4",
    // [ts] value: "John Doe"
    // [ts] },
    // [ts] {
    // [ts] name: "Password textbox",
    // [ts] type: "textbox",
    // [ts] target: "e6",
    // [ts] value: "X-PASSWORD"
    // [ts] },
    // [ts] ]
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('textbox', { name: 'Email' }).fill('John Doe');
    // [ts] page.getByRole('textbox', { name: 'Password' }).fill(process.env['X-PASSWORD']);`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- textbox \"Password\" [active] [ref=e6]: <secret>X-PASSWORD</secret>`),
    // [ts] });
    // [ts] });
    // [ts] test("empty secret value is ignored", () => {
    // [ts] const secretsFile = test.info().outputPath("secrets.env");
    // [ts] fs.promises.writeFile(secretsFile, "EMPTY_SECRET=\nX-PASSWORD=password123");
    // [ts] const { client } = startClient({
    // [ts] args: ["--secrets", secretsFile],
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <body><p>hello password123</p></body>
    // [ts] </html>
    // [ts] `, 'text/html');
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // Response should not be mangled by empty secret replacement
    // [ts] expect: (response) => {
    // [ts] snapshot: expect.stringContaining(`<secret>X-PASSWORD</secret>`),
    // [ts] });
    // [ts] });
    // [ts] test("redacts url parameter values", { annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1791" },
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43799" },
    // [ts] ] }, (_p) => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--redact-url-params=code,access_token"],
    // [ts] });
    // [ts] server.setRedirect("/login", "/app?code=abc123&state=s1#access_token=jwt.payload.sig&token_type=bearer");
    // [ts] server.setContent("/app?code=abc123&state=s1", `<title>App</title><a href="/next?code=def456">Next</a>`, "text/html");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/login" },
    // [ts] })).toHaveResponse({
    // [ts] page: `- Page URL: ${server.PREFIX}/app?code=<redacted>&state=s1#access_token=<redacted>&token_type=bearer
    // [ts] - Page Title: App`,
    // [ts] snapshot: expect.stringContaining(`/url: /next?code=<redacted>`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "list" },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: (current) [App](${server.PREFIX}/app?code=<redacted>&state=s1#access_token=<redacted>&token_type=bearer)`,
    // [ts] });
    // [ts] const network = client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: { static: true },
    // [ts] });
    // [ts] expect: (network) => {
    // [ts] result: expect.stringContaining(`[GET] ${server.PREFIX}/app?code=<redacted>&state=s1 => [200] OK`),
    // [ts] });
    // [ts] const index = Number(parseResponse(network).result.match("^(\\d+)\\. \\[GET\\] .*\\/app\\?")[1]);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`#${index} [GET] ${server.PREFIX}/app?code=<redacted>&state=s1`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: { static: true, filename: "network.log" },
    // [ts] });
    // [ts] const networkLog = fs.promises.readFile(test.info().outputPath("network.log"), "utf-8");
    // [ts] expect_eq(std.mem.indexOf(u8, networkLog, `[GET] ${server.PREFIX}/app?code=<redacted>&state=s1 => [200] OK`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, networkLog, "abc123") == null, true);
    // [ts] });
}
