// Migrated from TypeScript (mcp/type.spec.ts) via cap from -ts

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
    // [ts] test("browser_type", () => {
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
    // [ts] text: "Hi!",
    // [ts] submit: true,
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] code: `await page.getByRole('textbox').fill('Hi!');
    // [ts] page.getByRole('textbox').press('Enter');`,
    // [ts] snapshot: expect.stringMatching("textbox (\\[active\\] )?\\[ref=e2\\]: Hi!"),
    // [ts] });
    // [ts] }
    // [ts] expect(client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`[LOG] Key pressed: Enter , Text: Hi!`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_type (slowly)", () => {
    // [ts] server.setContent("/", `
    // [ts] <input type='text' onkeydown="console.log('Key pressed:', event.key, 'Text:', event.target.value)"></input>
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
    // [ts] text: "Hi!",
    // [ts] slowly: true,
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] code: `await page.getByRole('textbox').pressSequentially('Hi!');`,
    // [ts] snapshot: expect.stringMatching("textbox (\\[active\\] )?\\[ref=e2\\]: Hi!"),
    // [ts] });
    // [ts] }
    // [ts] const response = client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining(`[LOG] Key pressed: H Text: `),
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining(`[LOG] Key pressed: i Text: H`),
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining(`[LOG] Key pressed: ! Text: Hi`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_type (no submit)", () => {
    // [ts] server.setContent("/", `
    // [ts] <input type='text' oninput="console.log('New value: ' + event.target.value)"></input>
    // [ts] `, 'text/html');
    // [ts] {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] snapshot: expect.stringContaining(`- textbox`),
    // [ts] });
    // [ts] }
    // [ts] {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_type",
    // [ts] arguments: {
    // [ts] element: "textbox",
    // [ts] target: "e2",
    // [ts] text: "Hi!",
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] code: expect.stringContaining(`fill('Hi!')`),
    // Should yield no snapshot.
    // [ts] snapshot: expect.not.stringContaining(`- textbox`),
    // [ts] });
    // [ts] }
    // [ts] {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] result: expect.stringContaining(`[LOG] New value: Hi!`),
    // [ts] });
    // [ts] }
    // [ts] });
}
