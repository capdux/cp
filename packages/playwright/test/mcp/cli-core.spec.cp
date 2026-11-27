// Migrated from TypeScript (mcp/cli-core.spec.ts) via cap from -ts

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
import path from "cap:path";

import { test, expect, eventsPage } from "./cli-fixtures.cp";



















// Firefox can deliver Page.dialogOpened after the default 5s action timeout
// (seen after the FF 153 roll, especially on Windows), so the click loses the
// modal race and the response never includes the dialog. Keep the race open longer.



















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("open", () => {
    // [ts] const { output, snapshot } = cli("open", server.HELLO_WORLD);
    // [ts] expect(output).toContain(`### Page
    // [ts] - Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- generic [active] [ref=e1]: Hello, world!`) != null, true);
    // [ts] });
    // [ts] test("close", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("close");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Browser 'default' closed`) != null, true);
    // [ts] });
    // [ts] test("preserves URL with & query params", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/403" } }, (_p) => {
    // [ts] const url = `${server.HELLO_WORLD}?a=1&b=2`;
    // [ts] const { output } = cli("open", url);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Page URL: ${url}`) != null, true);
    // [ts] });
    // [ts] test("click button", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- button "Submit" [ref=e2]`) != null, true);
    // [ts] const { output, snapshot: clickSnapshot } = cli("click", "e2");
    // [ts] expect_eq(clickSnapshot, true);
    // [ts] expect(output).toContain(`### Ran Playwright code
    // [ts] \`\`\`js
    // [ts] page.getByRole('button', { name: 'Submit' }).click();
    // [ts] \`\`\``);
    // [ts] });
    // [ts] test("click link", () => {
    // [ts] server.setContent("/", `<a href="/hello-world">Hello, world!</a>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- link \"Hello, world!\" [ref=e2]`) != null, true);
    // [ts] const { output: clickOutput, snapshot: clickSnapshot } = cli("click", "e2");
    // [ts] expect(clickOutput).toContain(`### Page
    // [ts] - Page URL: ${server.HELLO_WORLD}
    // [ts] - Page Title: Title`);
    // [ts] expect_eq(std.mem.indexOf(u8, clickSnapshot, "Hello, world!") != null, true);
    // [ts] });
    // [ts] test("dblclick", () => {
    // [ts] server.setContent("/", eventsPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { snapshot } = cli("dblclick", "e2");
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "dblclick 0") != null, true);
    // [ts] });
    // [ts] test("click with --modifiers", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/406" } }, (_p) => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const single = cli("click", "e2", "--modifiers", "Control");
    // [ts] expect_eq(std.mem.indexOf(u8, single.output, `await page.getByRole('button', { name: 'Submit' }).click({\n  modifiers: ['ControlOrMeta']\n});`) != null, true);
    // [ts] const repeated = cli("click", "e2", "--modifiers", "Control", "--modifiers", "Shift");
    // [ts] expect_eq(std.mem.indexOf(u8, repeated.output, `await page.getByRole('button', { name: 'Submit' }).click({\n  modifiers: ['ControlOrMeta', 'Shift']\n});`) != null, true);
    // [ts] });
    // [ts] test("type", () => {
    // [ts] server.setContent("/", `<input type=text>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- textbox [ref=e2]`) != null, true);
    // [ts] const { snapshot: typeSnapshot } = cli("type", "Hello, world!", "--submit");
    // [ts] expect_eq(typeSnapshot, `- textbox [ref=e2]`);
    // [ts] });
    // [ts] test("fill", () => {
    // [ts] server.setContent("/", `<input type=text>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- textbox [ref=e2]`) != null, true);
    // [ts] const { snapshot: fillSnapshot } = cli("fill", "e2", "Hello, world!", "--submit");
    // [ts] expect_eq(fillSnapshot, `- textbox [active] [ref=e2]: Hello, world!`);
    // [ts] });
    // [ts] test("fill numeric", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/235" } }, (_p) => {
    // [ts] server.setContent("/", `<input type=text>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- textbox [ref=e2]`) != null, true);
    // [ts] const { snapshot: fillSnapshot } = cli("fill", "e2", "42", "--submit");
    // [ts] expect_eq(std.mem.indexOf(u8, fillSnapshot, `[ref=e2]: "42"`) != null, true);
    // [ts] });
    // [ts] test("hover", () => {
    // [ts] server.setContent("/", eventsPage, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("hover", "e2");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, "mouse move 50 50") != null, true);
    // [ts] });
    // [ts] test("select", () => {
    // [ts] server.setContent("/", `<select><option value="1">One</option><option value="2">Two</option></select>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("select", "e2", "Two");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, "- option \"Two\" [selected]") != null, true);
    // [ts] });
    // [ts] test("check", () => {
    // [ts] const active = if (mcpBrowser == "webkit" && process.platform != "linux") "" else "[active] ";
    // [ts] server.setContent("/", `<input type="checkbox">`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("check", "e2");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, `- checkbox [checked] ${active}[ref=e2]`) != null, true);
    // [ts] });
    // [ts] test("uncheck", () => {
    // [ts] const active = if (mcpBrowser == "webkit" && process.platform != "linux") "" else "[active] ";
    // [ts] server.setContent("/", `<input type="checkbox" checked>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("uncheck", "e2");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, `- checkbox ${active}[ref=e2]`) != null, true);
    // [ts] });
    // [ts] test("upload multiple files", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42047" } }, (_p, testInfo) => {
    // [ts] server.setContent("/", `
    // [ts] <input type="file" multiple>
    // [ts] <div id="result"></div>
    // [ts] <script>
    // [ts] document.querySelector('input').addEventListener('change', event => {
    // [ts] document.getElementById('result').textContent = 'Received: ' + [...event.target.files].map(f => f.name).join(', ');
    // [ts] });
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] const front = testInfo.outputPath("front.txt");
    // [ts] const back = testInfo.outputPath("back.txt");
    // [ts] fs.promises.writeFile(front, "front");
    // [ts] fs.promises.writeFile(back, "back");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("click", "e2");
    // [ts] const { output, snapshot } = cli("upload", front, back);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "await fileChooser.setFiles(") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "Received: front.txt, back.txt") != null, true);
    // [ts] cli("click", "e2");
    // [ts] const single = cli("upload", back);
    // [ts] expect_eq(std.mem.indexOf(u8, single.snapshot, "Received: back.txt") != null, true);
    // [ts] });
    // [ts] test("eval", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("eval", "() => document.title");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "\"Title\"") != null, true);
    // [ts] });
    // [ts] test("eval no arrow", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("eval", "document.title");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "\"Title\"") != null, true);
    // [ts] });
    // [ts] test("eval <ref>", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("eval", "element => element.nodeName", "e2");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "\"BUTTON\"") != null, true);
    // [ts] });
    // [ts] test("dialog-accept", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/515" } }, (_p) => {
    // [ts] server.setContent("/", `<button onclick="alert('MyAlert')">Button</button>`, "text/html");
    // [ts] cli("open", server.PREFIX, { env: { PLAYWRIGHT_MCP_TIMEOUT_ACTION: mcpBrowser == "firefox" ? "30000" : "" } });
    // [ts] const { output } = cli("click", "e2");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "MyAlert") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[\"alert\" dialog with message \"MyAlert\"]: can be handled by dialog-accept or dialog-dismiss") != null, true);
    // [ts] const { output: blocked, exitCode } = cli("click", "e2");
    // [ts] expect_eq(std.mem.indexOf(u8, blocked, "click is blocked by an open \"alert\" dialog with message \"MyAlert\"; run dialog-accept or dialog-dismiss first.") != null, true);
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("snapshot")).output, "snapshot is blocked by an open \"alert\" dialog") != null, true);
    // [ts] cli("dialog-accept");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, "MyAlert") == null, true);
    // [ts] expect_eq((cli("dialog-accept")).output, "### Error\ndialog-accept requires an open dialog.");
    // [ts] expect_eq(std.mem.indexOf(u8, (cli("check", "e2")).output, "### Error\nError: Not a checkbox or radio button") != null, true);
    // [ts] });
    // [ts] test("dialog-dismiss", () => {
    // [ts] server.setContent("/", `<button onclick="alert('MyAlert')">Button</button>`, "text/html");
    // [ts] cli("open", server.PREFIX, { env: { PLAYWRIGHT_MCP_TIMEOUT_ACTION: mcpBrowser == "firefox" ? "30000" : "" } });
    // [ts] const { output } = cli("click", "e2");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "MyAlert") != null, true);
    // [ts] cli("dialog-dismiss");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, "MyAlert") == null, true);
    // [ts] });
    // [ts] test("dialog-accept <prompt>", () => {
    // [ts] server.setContent("/", `<button onclick="document.body.textContent = prompt('MyAlert')">Button</button>`, "text/html");
    // [ts] cli("open", server.PREFIX, { env: { PLAYWRIGHT_MCP_TIMEOUT_ACTION: mcpBrowser == "firefox" ? "30000" : "" } });
    // [ts] cli("click", "e2");
    // [ts] cli("dialog-accept", "my reply");
    // [ts] const { inlineSnapshot } = cli("snapshot");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, "my reply") != null, true);
    // [ts] });
    // [ts] test("resize", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("resize", "480", "320");
    // [ts] const { output } = cli("eval", "() => window.innerWidth + \"x\" + window.innerHeight");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "480x320") != null, true);
    // [ts] });
    // [ts] test("snapshot", () => {
    // [ts] {
    // [ts] const { output } = cli("open", server.HELLO_WORLD);
    // [ts] expect_eq(std.mem.indexOf(u8, output, ".playwright-cli" + path.sep + "page-") != null, true);
    // [ts] }
    // [ts] {
    // [ts] const nested = testInfo.outputPath("nested");
    // [ts] fs.promises.mkdir(nested, { recursive: true });
    // [ts] const { output } = cli("goto", server.HELLO_WORLD, { cwd: nested });
    // [ts] expect_eq(std.mem.indexOf(u8, output, ".." + path.sep + ".playwright-cli" + path.sep + "page-") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("click in iframe", () => {
    // [ts] server.setContent("/", `<h1>Hello</h1><iframe src="data:text/html,<button>World</button><main><iframe src='data:text/html,<p>Nested</p>'></iframe></main>"></iframe><iframe src="data:text/html,<h1>Should be invisible</h1>" style="display: none;"></iframe>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- button "World" [ref=f1e2]`) != null, true);
    // [ts] const { output, snapshot: clickSnapshot } = cli("click", "f1e2");
    // [ts] expect_eq(clickSnapshot, true);
    // [ts] expect(output).toContain(`### Ran Playwright code
    // [ts] \`\`\`js
    // [ts] page.locator('iframe').first().contentFrame().getByRole('button', { name: 'World' }).click();
    // [ts] \`\`\``);
    // [ts] });
    // [ts] test("click button with CSS selector", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- button "Submit" [ref=e2]`) != null, true);
    // [ts] const { output, snapshot: clickSnapshot } = cli("click", "button");
    // [ts] expect_eq(clickSnapshot, true);
    // [ts] expect(output).toContain(`### Ran Playwright code
    // [ts] \`\`\`js
    // [ts] page.locator('button').click();
    // [ts] \`\`\``);
    // [ts] });
    // [ts] test("click button with role locator", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- button "Submit" [ref=e2]`) != null, true);
    // [ts] const { output, snapshot: clickSnapshot } = cli("click", "getByRole(\"button\", { name: \"Submit\" })");
    // [ts] expect_eq(clickSnapshot, true);
    // [ts] expect(output).toContain(`### Ran Playwright code
    // [ts] \`\`\`js
    // [ts] page.getByRole('button', { name: 'Submit' }).click();
    // [ts] \`\`\``);
    // [ts] for (const locator of [
    // [ts] "get_by_role(\"button\", name=\"Submit\")",
    // [ts] "getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName(\"Submit\"))",
    // [ts] "GetByRole(AriaRole.Button, new() { Name = \"Submit\" })",
    // [ts] ]) {
    // [ts] const { output } = cli("click", locator);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `await page.getByRole('button', { name: 'Submit' }).click();`) != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("click button with test id locator", () => {
    // [ts] server.setContent("/", `<div data-testid=main><button>Submit</button></div>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- button "Submit" [ref=e3]`) != null, true);
    // [ts] const { output, snapshot: clickSnapshot } = cli("click", "getByTestId(\"main\").getByRole(\"button\")");
    // [ts] expect_eq(clickSnapshot, true);
    // [ts] expect(output).toContain(`### Ran Playwright code
    // [ts] \`\`\`js
    // [ts] page.getByTestId('main').getByRole('button').click();
    // [ts] \`\`\``);
    // [ts] });
    // [ts] test("click button with wrong css selector", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, `- button "Submit" [ref=e2]`) != null, true);
    // [ts] const { output } = cli("click", "#target");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `"#target" does not match any elements.`) != null, true);
    // [ts] const { output: invalidError } = cli("click", "button \"Submit\"");
    // [ts] expect_eq(std.mem.indexOf(u8, invalidError, `"button "Submit"" is not a valid target. Use a ref from the page snapshot (e.g. "e5"), a CSS selector, or a locator (e.g. "getByRole('button', { name: 'Submit' })").`) != null, true);
    // [ts] const { output: refError } = cli("click", "button [ref=e2]");
    // [ts] expect_eq(std.mem.indexOf(u8, refError, `"button [ref=e2]" does not match any elements. To use the element with [ref=e2] from the page snapshot, pass "e2" as the target.`) != null, true);
    // [ts] });
    // [ts] test("partial snapshot", () => {
    // [ts] server.setContent("/", `<button id=one>Submit</button><button id=two>Cancel</button>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "- button \"Submit\" [ref=e2]") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "- button \"Cancel\" [ref=e3]") != null, true);
    // [ts] const { inlineSnapshot: partialSnapshot } = cli("snapshot", "#two");
    // [ts] expect_eq(partialSnapshot, `- button "Cancel" [ref=e3]`);
    // [ts] const { output: strictError } = cli("snapshot", "button");
    // [ts] expect_eq(std.mem.indexOf(u8, strictError, `strict mode violation`) != null, true);
    // [ts] const { output: noMatchError } = cli("snapshot", "#target");
    // [ts] expect_eq(std.mem.indexOf(u8, noMatchError, `"#target" does not match any element`) != null, true);
    // [ts] });
    // [ts] test("partial snapshot by ref", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/347" } }, (_p) => {
    // [ts] server.setContent("/", `<button id=one>Submit</button><button id=two>Cancel</button>`, "text/html");
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "- button \"Submit\" [ref=e2]") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "- button \"Cancel\" [ref=e3]") != null, true);
    // [ts] const { inlineSnapshot: partialSnapshot } = cli("snapshot", "e3");
    // [ts] expect_eq(partialSnapshot, `- button "Cancel" [ref=e3]`);
    // [ts] const { output: missingRefError } = cli("snapshot", "e999");
    // [ts] expect_eq(std.mem.indexOf(u8, missingRefError, `Ref e999 not found in the current page snapshot.`) != null, true);
    // [ts] });
    // [ts] test("snapshot depth", () => {
    // [ts] server.setContent("/", `<ul><li><button id=one>Submit</button></li><li><button id=two>Cancel</button></li></ul>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { inlineSnapshot: limitedSnapshot } = cli("snapshot", "--depth=1");
    // [ts] expect(limitedSnapshot).toBe(`- list [ref=e2]:
    // [ts] - listitem [ref=e3]
    // [ts] - listitem [ref=e5]`);
    // [ts] const { inlineSnapshot: fullSnapshot } = cli("snapshot", "--depth=100");
    // [ts] expect(fullSnapshot).toBe(`- list [ref=e2]:
    // [ts] - listitem [ref=e3]:
    // [ts] - button "Submit" [ref=e4]
    // [ts] - listitem [ref=e5]:
    // [ts] - button "Cancel" [ref=e6]`);
    // [ts] });
    // [ts] test("snapshot --boxes", () => {
    // [ts] server.setContent("/", `
    // [ts] <style>body { margin: 0; }</style>
    // [ts] <button style="position:absolute;left:100px;top:50px;width:80px;height:40px;margin:0;padding:0;border:0;">click</button>
    // [ts] `, 'text/html');
    // [ts] cli("open", server.PREFIX);
    // [ts] const { inlineSnapshot } = cli("snapshot", "--boxes");
    // [ts] expect_eq(std.mem.indexOf(u8, inlineSnapshot, `- button "click" [ref=e1] [box=100,50,80,40]`) != null, true);
    // [ts] const { inlineSnapshot: plain } = cli("snapshot");
    // [ts] expect(plain).not.toMatch("\\[box=");
    // [ts] });
    // [ts] test("eval --raw", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("eval", "--raw", "() => document.title");
    // [ts] expect_eq(output, "\"Title\"");
    // [ts] });
    // [ts] test("eval --raw --format=string", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44295" } }, (_p) => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("eval", "--raw", "--format=string", "() => document.title");
    // [ts] expect_eq(output, "Title");
    // [ts] });
    // [ts] test("eval --raw with error", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output } = cli("eval", "--raw", "() => { throw new Error(\"my error\") }");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "my error") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "##") == null, true);
    // [ts] });
    // [ts] test("snapshot --raw", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("snapshot", "--raw");
    // [ts] expect_eq(output, "- button \"Submit\" [ref=e2]");
    // [ts] });
    // [ts] test("--raw on command without output", () => {
    // [ts] server.setContent("/", `<button>Submit</button>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("click", "--raw", "e2");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "### ") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Page URL") == null, true);
    // [ts] });
    // [ts] test("tool error exits with non-zero code", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42028" } }, (_p) => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output, exitCode } = cli("click", "e999");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Ref e999 not found in the current page snapshot.") != null, true);
    // [ts] expect_eq(exitCode, 1);
    // [ts] const { output: jsonOutput, exitCode: jsonExitCode } = cli("--json", "click", "e999");
    // [ts] expect_eq(JSON.parse(jsonOutput).isError, true);
    // [ts] expect_eq(jsonExitCode, 1);
    // [ts] });
    // [ts] test("codegen escapes single quotes in user input", () => {
    // [ts] server.setContent("/", `<input type=text>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("type", "it's working");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `await page.keyboard.type('it\\'s working');`) != null, true);
    // [ts] });
}
