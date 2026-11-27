// Migrated from TypeScript (mcp/wait.spec.ts) via cap from -ts

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






fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_wait_for(text)", () => {
    // [ts] server.setContent("/", `
    // [ts] <script>
    // [ts] function update() {
    // [ts] setTimeout(() => {
    // [ts] document.querySelector('div').textContent = 'Text to appear';
    // [ts] }, 1000);
    // [ts] }
    // [ts] </script>
    // [ts] <body>
    // [ts] <button onclick="update()">Click me</button>
    // [ts] <div>Text to disappear</div>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [ref=e3]: Text to disappear`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Click me",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { text: "Text to appear" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- generic [ref=e3]: Text to appear`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_wait_for(textGone)", () => {
    // [ts] server.setContent("/", `
    // [ts] <script>
    // [ts] function update() {
    // [ts] setTimeout(() => {
    // [ts] document.querySelector('div').textContent = 'Text to appear';
    // [ts] }, 1000);
    // [ts] }
    // [ts] </script>
    // [ts] <body>
    // [ts] <button onclick="update()">Click me</button>
    // [ts] <div>Text to disappear</div>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [ref=e3]: Text to disappear`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Click me",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { textGone: "Text to disappear" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- generic [ref=e3]: Text to appear`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_wait_for(time)", () => {
    // [ts] server.setContent("/", `<body><div>Hello World</div></body>`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { time: 1 },
    // [ts] })).toHaveResponse({
    // [ts] result: `Waited for 1 seconds`,
    // [ts] code: `await new Promise(f => setTimeout(f, 1 * 1000));`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_wait_for(time) reports capped time", () => {
    // [ts] test.slow();
    // [ts] server.setContent("/", `<body><div>Hello World</div></body>`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { time: 70 },
    // [ts] })).toHaveResponse({
    // [ts] result: `Waited for 30 seconds (requested 70, maximum is 30)`,
    // [ts] code: `await new Promise(f => setTimeout(f, 30 * 1000));`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_wait_for(time, text) uses time as a timeout", () => {
    // [ts] const { client } = startClient({ args: [`--timeout-action=500`] });
    // [ts] server.setContent("/", `
    // [ts] <body>
    // [ts] <div>Loading</div>
    // [ts] <script>
    // [ts] setTimeout(() => document.querySelector('div').textContent = 'Text to appear', 2000);
    // [ts] </script>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { text: "Text to appear", time: 20 },
    // [ts] })).toHaveResponse({
    // [ts] result: `Waited for Text to appear`,
    // [ts] code: `await page.getByText("Text to appear").first().waitFor({ state: 'visible' });`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_wait_for(time, text) fails when time passes", () => {
    // [ts] server.setContent("/", `<body><div>Hello World</div></body>`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { text: "This text will never appear", time: 1 },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining(`Timeout 1000ms exceeded.`),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_wait_for(text, textGone) waits for the first one", () => {
    // [ts] server.setContent("/", `
    // [ts] <body>
    // [ts] <div>Text to disappear</div>
    // [ts] <script>
    // [ts] setTimeout(() => document.querySelector('div').textContent = 'Something else', 1000);
    // [ts] </script>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_wait_for",
    // [ts] arguments: { text: "This text will never appear", textGone: "Text to disappear" },
    // [ts] })).toHaveResponse({
    // [ts] result: `Waited for Text to disappear`,
    // [ts] code: `await page.getByText("Text to disappear").first().waitFor({ state: 'hidden' });`,
    // [ts] });
    // [ts] });
}
