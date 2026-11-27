// Migrated from TypeScript (mcp/core.spec.ts) via cap from -ts

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

import { execFileSync } from "cap:process";
import fs from "cap:fs";
import { pathToFileURL } from "cap:url";
import { test, expect } from "./fixtures.cp";










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_navigate", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.goto('${server.HELLO_WORLD}');`,
    // [ts] page: `- Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`,
    // [ts] snapshot: `- generic [active] [ref=e1]: Hello, world!`,
    // [ts] });
    // [ts] });
    // [ts] test("should git-ignore the default output directory", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44289" } }, (_p, testInfo) => {
    // [ts] const cwd = testInfo.outputPath();
    // [ts] execFileSync("git", ["init", "--quiet", cwd]);
    // [ts] server.setContent("/", `<body>Hello, world!<script>console.log('hello')</script></body>`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const files = fs.readdir(testInfo.outputPath(".playwright-mcp"));
    // [ts] expect_eq(files.some(file => file.startsWith("page-") && file.endsWith(".yml")), true);
    // [ts] expect_eq(files.some(file => file.startsWith("console-") && file.endsWith(".log")), true);
    // [ts] expect_eq(execFileSync("git", ["status", "--porcelain", "--untracked-files=all", "--", ".playwright-mcp"], { cwd, encoding: "utf-8" }), "");
    // [ts] });
    // [ts] test("browser_navigate with bare host", () => {
    // [ts] const urlWithoutScheme = server.HELLO_WORLD.replace("^https?:\\/\\/", "");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: urlWithoutScheme },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.goto('${server.HELLO_WORLD}');`,
    // [ts] page: `- Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`,
    // [ts] snapshot: `- generic [active] [ref=e1]: Hello, world!`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_navigate surfaces non-2xx HTTP status", () => {
    // [ts] server.setRoute("/locked", (req, res) => {
    // [ts] res.writeHead(402, { "Content-Type": "text/html" });
    // [ts] res.end("<title>Payment Required</title><body>Pay up</body>");
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/locked" },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`- HTTP status: 402 Payment Required`),
    // [ts] });
    // A redirect to a 2xx page must not carry a status line: the intermediate
    // 302 hop must not leak, and the final 2xx landing renders nothing.
    // [ts] server.setRoute("/redirect", (req, res) => {
    // [ts] res.writeHead(302, { location: server.HELLO_WORLD });
    // [ts] res.end();
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/redirect" },
    // [ts] })).not.toHaveResponse({
    // [ts] page: expect.stringContaining("HTTP status"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_navigate blocks file:// URLs by default", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "file:///etc/passwd" },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("Error: Access to \"file:\" protocol is blocked. Attempted URL: \"file:///etc/passwd\""),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_navigate allows about:, data: and javascript: protocols", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "about:blank" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.goto('about:blank');`,
    // [ts] page: `- Page URL: about:blank`,
    // [ts] snapshot: ``,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "data:text/html,<h1>Hello</h1>" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.goto('data:text/html,<h1>Hello</h1>');`,
    // [ts] page: expect.stringContaining(`- Page URL: data:text/html,<h1>Hello</h1>`),
    // [ts] snapshot: `- heading \"Hello\" [level=1] [ref=e2]`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_navigate can navigate to file:// URLs allowUnrestrictedFileAccess is true", () => {
    // [ts] const rootDir = testInfo.outputPath();
    // [ts] const fileOutsideRoot = testInfo.outputPath("test.txt");
    // [ts] fs.writeFile(fileOutsideRoot, "Test file content");
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] allowUnrestrictedFileAccess: true,
    // [ts] },
    // [ts] roots: [
    // [ts] {
    // [ts] name: "workspace",
    // [ts] uri: pathToFileURL(rootDir).href,
    // [ts] }
    // [ts] ],
    // [ts] });
    // [ts] const url = pathToFileURL(fileOutsideRoot).href;
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`- Page URL: ${url}`),
    // [ts] snapshot: `- generic [active] [ref=e1]: Test file content`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_navigate_back does not time out when load never fires", () => {
    // https://github.com/microsoft/playwright-mcp/issues/1635
    // Page A never fires the `load` event because the image request hangs forever.
    // Going back to it should still succeed because we wait for `commit`, not `load`.
    // [ts] server.setRoute("/hang", () => {});
    // [ts] server.setContent("/page-a", `<title>Page A</title><body>Page A<img src="/hang"></body>`, "text/html");
    // [ts] server.setContent("/page-b", `<title>Page B</title><body>Page B</body>`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: `${server.PREFIX}/page-a` },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: `${server.PREFIX}/page-b` },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate_back",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.goBack();`,
    // [ts] page: expect.stringContaining(`- Page URL: ${server.PREFIX}/page-a`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_select_option", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <select>
    // [ts] <option value="foo">Foo</option>
    // [ts] <option value="bar">Bar</option>
    // [ts] </select>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_select_option",
    // [ts] arguments: {
    // [ts] element: "Select",
    // [ts] target: "e2",
    // [ts] values: ["bar"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: `- combobox [ref=e2]:
    // [ts] - option "Foo"
    // [ts] - option "Bar" [selected]`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_select_option (multiple)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <select multiple>
    // [ts] <option value="foo">Foo</option>
    // [ts] <option value="bar">Bar</option>
    // [ts] <option value="baz">Baz</option>
    // [ts] </select>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_select_option",
    // [ts] arguments: {
    // [ts] element: "Select",
    // [ts] target: "e2",
    // [ts] values: ["bar", "baz"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('listbox').selectOption(['bar', 'baz']);`,
    // [ts] snapshot: `- listbox [ref=e2]:
    // [ts] - option "Foo" [ref=e3]
    // [ts] - option "Bar" [selected] [ref=e4]
    // [ts] - option "Baz" [selected] [ref=e5]`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_resize", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Resize Test</title>
    // [ts] <body>
    // [ts] <div id="size">Waiting for resize...</div>
    // [ts] <script>new ResizeObserver(() => { document.getElementById("size").textContent = \`Window size: \${window.innerWidth}x\${window.innerHeight}\`; }).observe(document.body);
    // [ts] </script>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_resize",
    // [ts] arguments: {
    // [ts] width: 390,
    // [ts] height: 780,
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] code: `await page.setViewportSize({ width: 390, height: 780 });`,
    // [ts] });
    // [ts] expect.poll(() => client.callTool({ name: "browser_snapshot" })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`Window size: 390x780`),
    // [ts] });
    // [ts] });
    // [ts] test("old locator error message", () => {
    // [ts] server.setContent("/", `
    // [ts] <button>Button 1</button>
    // [ts] <button>Button 2</button>
    // [ts] <script>
    // [ts] document.querySelector('button').addEventListener('click', () => {
    // [ts] document.querySelectorAll('button')[1].remove();
    // [ts] });
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`
    // [ts] - button "Button 1" [ref=e2]
    // [ts] - button "Button 2" [ref=e3]`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button 1",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button 2",
    // [ts] target: "e3",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Ref e3 not found in the current page snapshot. Try capturing new snapshot.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("visibility: hidden > visible should be shown", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/535" } }, (_p) => {
    // [ts] server.setContent("/", `
    // [ts] <div style="visibility: hidden;">
    // [ts] <div style="visibility: visible;">
    // [ts] <button>Button</button>
    // [ts] </div>
    // [ts] </div>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot"
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- button "Button"`),
    // [ts] });
    // [ts] });
    // [ts] test("snapshot depth", () => {
    // [ts] server.setContent("/", `
    // [ts] <ul>
    // [ts] <li>text</li>
    // [ts] <li>
    // [ts] <button>Button</button>
    // [ts] </li>
    // [ts] </ul>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {
    // [ts] depth: 1,
    // [ts] }
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: `- list [ref=e2]:
    // [ts] - listitem [ref=e3]: text
    // [ts] - listitem [ref=e4]`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {
    // [ts] depth: 2,
    // [ts] }
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: `- list [ref=e2]:
    // [ts] - listitem [ref=e3]: text
    // [ts] - listitem [ref=e4]:
    // [ts] - button "Button" [ref=e5]`,
    // [ts] });
    // [ts] });
    // [ts] test("snapshot with boxes", () => {
    // [ts] server.setContent("/", `
    // [ts] <style>body { margin: 0; }</style>
    // [ts] <button style="position:absolute;left:100px;top:50px;width:80px;height:40px;margin:0;padding:0;border:0;">click</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: { boxes: true },
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- button "click" [ref=e1] [box=100,50,80,40]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.not.stringMatching("\\[box="),
    // [ts] });
    // [ts] });
    // [ts] test("snapshot by ref", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/347" } }, (_p) => {
    // [ts] server.setContent("/", `
    // [ts] <ul>
    // [ts] <li>text</li>
    // [ts] <li>
    // [ts] <button>Button</button>
    // [ts] </li>
    // [ts] </ul>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {
    // [ts] target: "e2",
    // [ts] }
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: `- list [ref=e2]:
    // [ts] - listitem [ref=e3]: text
    // [ts] - listitem [ref=e4]:
    // [ts] - button "Button" [ref=e5]`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {
    // [ts] target: "e999",
    // [ts] }
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Ref e999 not found in the current page snapshot. Try capturing new snapshot.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
}
