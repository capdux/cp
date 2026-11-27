// Migrated from TypeScript (mcp/codegen.spec.ts) via cap from -ts

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
import { test, expect } from "./fixtures.cp";

// [cap-migrate] import { Client } from "@modelcontextprotocol/sdk/client/index.js";

async fn navigateToForm(client: Client, server: string) {
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
    // [ts] test("codegen defaults to PW_LANG_NAME", () => {
    // [ts] const { client } = startClient({ env: { PW_LANG_NAME: "python" } });
    // [ts] expect: (navigateToForm(client, server)) => {
    // [ts] code: `page.goto("${server.PREFIX}")`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] })).toHaveResponse({
    // [ts] code: `page.get_by_role("button", name="Submit").click()`,
    // [ts] });
    // [ts] });
    // [ts] test("codegen python", () => {
    // [ts] const { client } = startClient({ args: ["--codegen=python"] });
    // [ts] expect: (navigateToForm(client, server)) => {
    // [ts] code: `page.goto("${server.PREFIX}")`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] })).toHaveResponse({
    // [ts] code: `page.get_by_role("button", name="Submit").click()`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2", modifiers: ["Control"] },
    // [ts] })).toHaveResponse({
    // [ts] code: `page.get_by_role("button", name="Submit").click(modifiers=["ControlOrMeta"])`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: { element: "textbox", target: "e3", text: `it's a secret`, submit: true },
    // [ts] })).toHaveResponse({
    // [ts] code: `page.get_by_role("textbox").fill("it's a secret")\npage.get_by_role("textbox").press("Enter")`,
    // [ts] });
    // Page-level keyboard input has no action equivalent and stays as JavaScript.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_press_key",
    // [ts] arguments: { key: "Escape" },
    // [ts] })).toHaveResponse({
    // [ts] code: `// Press Escape\nawait page.keyboard.press('Escape');`,
    // [ts] });
    // [ts] });
    // [ts] test("codegen java", () => {
    // [ts] const { client } = startClient({ args: ["--codegen=java"] });
    // [ts] expect: (navigateToForm(client, server)) => {
    // [ts] code: `page.navigate("${server.PREFIX}");`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] })).toHaveResponse({
    // [ts] code: `page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")).click();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: { element: "textbox", target: "e3", text: "hello", submit: false },
    // [ts] })).toHaveResponse({
    // [ts] code: `page.getByRole(AriaRole.TEXTBOX).fill("hello");`,
    // [ts] });
    // [ts] });
    // [ts] test("codegen csharp", () => {
    // [ts] const { client } = startClient({ args: ["--codegen=csharp"] });
    // [ts] expect: (navigateToForm(client, server)) => {
    // [ts] code: `await page.GotoAsync("${server.PREFIX}");`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.GetByRole(AriaRole.Button, new() { Name = "Submit" }).ClickAsync();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: { element: "textbox", target: "e3", text: "hello", submit: false },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.GetByRole(AriaRole.Textbox).FillAsync("hello");`,
    // [ts] });
    // [ts] });
    // [ts] test("codegen verify tools", () => {
    // [ts] const { client } = startClient({ args: ["--codegen=python", "--caps=testing"] });
    // [ts] navigateToForm(client, server);
    // [ts] client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: { element: "textbox", target: "e3", text: "hello", submit: false },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: { type: "textbox", element: "textbox", target: "e3", value: "hello" },
    // [ts] })).toHaveResponse({
    // [ts] code: `expect(page.get_by_role("textbox")).to_have_value("hello")`,
    // [ts] });
    // [ts] });
    // [ts] test("codegen renders secrets as environment lookups", () => {
    // [ts] const secretsFile = test.info().outputPath("secrets.env");
    // [ts] fs.promises.writeFile(secretsFile, "X-PASSWORD=password123");
    // [ts] for (const [language, code] of [
    // [ts] ["typescript", `await page.getByRole('textbox').fill(process.env['X-PASSWORD']);`],
    // [ts] ["python", `page.get_by_role("textbox").fill(os.environ["X-PASSWORD"])`],
    // [ts] ["java", `page.getByRole(AriaRole.TEXTBOX).fill(System.getenv("X-PASSWORD"));`],
    // [ts] ["csharp", `await page.GetByRole(AriaRole.Textbox).FillAsync(Environment.GetEnvironmentVariable("X-PASSWORD"));`],
    // [ts] ]) { const { client } = startClient({ args: [`--codegen=${language}`, "--secrets", secretsFile] }); navigateToForm(client, server); expect(client.callTool({ name: "browser_type", arguments: { element: "textbox", target: "e3", text: "X-PASSWORD", submit: false }, })).toHaveResponse({ code }); client.close(); }
    // [ts] });
    // [ts] test("codegen falls back to JavaScript for scripted lines", () => {
    // [ts] const { client } = startClient({ args: ["--codegen=python"] });
    // [ts] navigateToForm(client, server);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.title" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.evaluate('() => document.title');`,
    // [ts] });
    // [ts] });
}
