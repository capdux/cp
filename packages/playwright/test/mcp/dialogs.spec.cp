// Migrated from TypeScript (mcp/dialogs.spec.ts) via cap from -ts

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
    // [ts] test("alert dialog", () => {
    // [ts] server.setContent("/", `<title>Title</title><button onclick="alert('Alert')">Button</button>`, "text/html");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('button', { name: 'Button' }).click();`,
    // [ts] modalState: `- ["alert" dialog with message "Alert"]: can be handled by browser_handle_dialog`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: null,
    // [ts] modalState: `- ["alert" dialog with message "Alert"]: can be handled by browser_handle_dialog`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: {
    // [ts] accept: true,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: null,
    // [ts] page: expect.stringContaining(`- Page Title: Title`),
    // [ts] });
    // [ts] });
    // [ts] test("two alert dialogs", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <body>
    // [ts] <button onclick="alert('Alert 1');alert('Alert 2');">Button</button>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('button', { name: 'Button' }).click();`,
    // [ts] modalState: expect.stringContaining(`- ["alert" dialog with message "Alert 1"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: {
    // [ts] accept: true,
    // [ts] },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] modalState: expect.stringContaining(`- ["alert" dialog with message "Alert 2"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] const result2 = client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: {
    // [ts] accept: true,
    // [ts] },
    // [ts] });
    // [ts] expect: (result2) => {
    // [ts] modalState: expect.stringContaining(`- ["alert" dialog with message "Alert 2"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] });
    // [ts] test("confirm dialog (true)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <body>
    // [ts] <button onclick="document.body.textContent = confirm('Confirm')">Button</button>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: expect.stringContaining(`- ["confirm" dialog with message "Confirm"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: {
    // [ts] accept: true,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: null,
    // [ts] });
    // [ts] });
    // [ts] test("confirm dialog (false)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <body>
    // [ts] <button onclick="document.body.textContent = confirm('Confirm')">Button</button>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: expect.stringContaining(`- ["confirm" dialog with message "Confirm"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: {
    // [ts] accept: false,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: null,
    // [ts] });
    // [ts] });
    // [ts] test("prompt dialog", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <body>
    // [ts] <button onclick="document.body.textContent = prompt('Prompt')">Button</button>
    // [ts] </body>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: expect.stringContaining(`- ["prompt" dialog with message "Prompt"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: {
    // [ts] accept: true,
    // [ts] promptText: "Answer",
    // [ts] },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] modalState: null,
    // [ts] });
    // [ts] });
    // [ts] test("dialog closed out of band", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41837" },
    // [ts] }, (_p) => {
    // [ts] server.setContent("/", `<title>Title</title><button onclick="alert('Alert')">Button</button>`, "text/html");
    // [ts] const browserContext = cdpServer.start();
    // [ts] const [page] = browserContext.pages();
    // Subscribe to the dialog event to prevent this connection from auto-dismissing dialogs.
    // [ts] page.on("dialog", () => {});
    // Establish the CDP session up front: creating one while a dialog is blocking the page hangs.
    // [ts] const cdpSession = browserContext.newCDPSession(page);
    // [ts] cdpSession.send("Page.enable");
    // [ts] const { client } = startClient({ args: [`--cdp-endpoint=${cdpServer.endpoint}`] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: `- ["alert" dialog with message "Alert"]: can be handled by browser_handle_dialog`,
    // [ts] });
    // Close the dialog through CDP as a side-channel, similar to the user closing it in the headed browser.
    // [ts] const closedPromise = page.waitForEvent("dialogclosed");
    // [ts] cdpSession.send("Page.handleJavaScriptDialog", { accept: true });
    // [ts] closedPromise;
    // [ts] expect.poll(() => client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] modalState: null,
    // [ts] inlineSnapshot: expect.stringContaining(`- button "Button"`),
    // [ts] });
    // [ts] });
    // [ts] test("alert dialog w/ race", () => {
    // [ts] server.setContent("/", `<title>Title</title><button onclick="setTimeout(() => alert('Alert'), 100)">Button</button>`, "text/html");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- button "Button" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('button', { name: 'Button' }).click();`,
    // [ts] modalState: expect.stringContaining(`- ["alert" dialog with message "Alert"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: {
    // [ts] accept: true,
    // [ts] },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] modalState: null,
    // [ts] page: expect.stringContaining(`- Page URL: ${server.PREFIX}/
    // [ts] - Page Title: Title`),
    // [ts] });
    // [ts] });
    // [ts] test("alert dialog during navigation", () => {
    // [ts] server.setContent("/", `<title>Title</title><script>alert('Alert')</script><button>Button</button>`, "text/html");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] modalState: expect.stringContaining(`- ["alert" dialog with message "Alert"]: can be handled by browser_handle_dialog`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: { accept: true },
    // [ts] })).toHaveResponse({
    // [ts] modalState: null,
    // [ts] page: expect.stringContaining(`- Page URL: ${server.PREFIX}/
    // [ts] - Page Title: Title`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- button "Button"`),
    // [ts] });
    // [ts] });
    // [ts] test("dialog during navigate back", () => {
    // [ts] server.setContent("/page1", `<title>Page 1</title><button>Button 1</button>`, "text/html");
    // [ts] server.setContent("/page2", `<title>Page 2</title><button id="btn">Click me</button><script>
    // [ts] window.addEventListener('beforeunload', (e) => {
    // [ts] e.preventDefault();
    // [ts] e.returnValue = '';
    // [ts] });
    // [ts] </script>`, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/page1" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/page2" },
    // [ts] });
    // User interaction is required for beforeunload to trigger
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Click me", target: "e2" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate_back",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] modalState: expect.stringMatching("- \\[\"beforeunload\" dialog.*\\]: can be handled by browser_handle_dialog"),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_handle_dialog",
    // [ts] arguments: { accept: true },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- button "Button 1"`),
    // [ts] });
    // [ts] });
}
