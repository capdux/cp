// Migrated from TypeScript (mcp/click.spec.ts) via cap from -ts

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
    // [ts] test("browser_click", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <button>Submit</button>
    // [ts] <script>
    // [ts] const button = document.querySelector('button');
    // [ts] button.addEventListener('click', () => {
    // [ts] button.focus(); // without manual focus, webkit focuses body
    // [ts] });
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Submit button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('button', { name: 'Submit' }).click();`,
    // [ts] snapshot: expect.stringContaining(`button "Submit" [active] [ref=e2]`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_click (double)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <script>
    // [ts] function handle() {
    // [ts] document.querySelector('h1').textContent = 'Double clicked';
    // [ts] }
    // [ts] </script>
    // [ts] <h1 ondblclick="handle()">Click me</h1>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Click me",
    // [ts] target: "e2",
    // [ts] doubleClick: true,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('heading', { name: 'Click me' }).dblclick();`,
    // [ts] snapshot: expect.stringContaining(`heading "Double clicked" [level=1] [ref=e3]`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_click (right)", () => {
    // [ts] server.setContent("/", `
    // [ts] <button oncontextmenu="handle">Menu</button>
    // [ts] <script>
    // [ts] document.addEventListener('contextmenu', event => {
    // [ts] event.preventDefault();
    // [ts] document.querySelector('button').textContent = 'Right clicked';
    // [ts] });
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Menu",
    // [ts] target: "e2",
    // [ts] button: "right",
    // [ts] },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] code: [
    // [ts] `await page.getByRole('button', { name: 'Menu' }).click({`,
    // [ts] `  button: 'right'`,
    // [ts] `});`
    // [ts] ].join("\n"),
    // [ts] snapshot: expect.stringContaining(`button "Right clicked"`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_click (modifiers)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <button>Submit</button>
    // [ts] <div id="div"></div>
    // [ts] <script>
    // [ts] document.addEventListener('click', event => {
    // [ts] document.querySelector('div').textContent = 'ctrlKey:' + event.ctrlKey + ' metaKey:' + event.metaKey + ' shiftKey:' + event.shiftKey + ' altKey:' + event.altKey;
    // [ts] });
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] if (process.platform != "darwin") {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Submit button",
    // [ts] target: "e2",
    // [ts] modifiers: ["Control"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: [
    // [ts] `await page.getByRole('button', { name: 'Submit' }).click({`,
    // [ts] `  modifiers: ['ControlOrMeta']`,
    // [ts] `});`
    // [ts] ].join("\n"),
    // [ts] snapshot: expect.stringContaining(`generic [ref=e3]: ctrlKey:true metaKey:false shiftKey:false altKey:false`),
    // [ts] });
    // [ts] }
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Submit button",
    // [ts] target: "e2",
    // [ts] modifiers: ["Shift"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: [
    // [ts] `await page.getByRole('button', { name: 'Submit' }).click({`,
    // [ts] `  modifiers: ['Shift']`,
    // [ts] `});`
    // [ts] ].join("\n"),
    // [ts] snapshot: expect.stringContaining(`generic [ref=e3]: ctrlKey:false metaKey:false shiftKey:true altKey:false`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Submit button",
    // [ts] target: "e2",
    // [ts] modifiers: ["Shift", "Alt"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: [
    // [ts] `await page.getByRole('button', { name: 'Submit' }).click({`,
    // [ts] `  modifiers: ['Alt', 'Shift']`,
    // [ts] `});`
    // [ts] ].join("\n"),
    // [ts] snapshot: expect.stringContaining(`generic [ref=e3]: ctrlKey:false metaKey:false shiftKey:true altKey:true`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_click (test id attribute)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <button data-tid="submit">Submit</button>
    // [ts] `, 'text/html');
    // [ts] const { client } = startClient({
    // [ts] args: [
    // [ts] "--test-id-attribute", "data-tid",
    // [ts] ],
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Submit button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByTestId('submit').click();`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_click (locator target containing aria-ref-like substring)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Title</title>
    // [ts] <button id="z2l9e43l3">Submit</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Submit button",
    // [ts] target: "#z2l9e43l3",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.locator('#z2l9e43l3').click();`,
    // [ts] });
    // [ts] });
}
