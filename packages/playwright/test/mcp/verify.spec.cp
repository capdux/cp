// Migrated from TypeScript (mcp/verify.spec.ts) via cap from -ts

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

test.use({ mcpArgs: ["--caps=testing"] });

















fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_verify_element_visible", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <button>Submit</button>
    // [ts] <h1>Welcome</h1>
    // [ts] <div role="alert" aria-label="Success message"></div>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_element_visible",
    // [ts] arguments: {
    // [ts] role: "button",
    // [ts] accessibleName: "Submit",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByRole('button', { name: 'Submit' })).toBeVisible();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_element_visible",
    // [ts] arguments: {
    // [ts] role: "heading",
    // [ts] accessibleName: "Welcome",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByRole('heading', { name: 'Welcome' })).toBeVisible();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_element_visible",
    // [ts] arguments: {
    // [ts] role: "alert",
    // [ts] accessibleName: "Success message",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByRole('alert', { name: 'Success message' })).toBeVisible();`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_element_visible (not found)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <button>Submit</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_element_visible",
    // [ts] arguments: {
    // [ts] role: "button",
    // [ts] accessibleName: "Cancel",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: "Element with role \"button\" and accessible name \"Cancel\" not found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_element_visible (iframe)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <h1>Outer frame text</h1>
    // [ts] <iframe srcdoc="<html><body><h1>Inner iframe</h1></body></html>" width="400" height="200">
    // [ts] </iframe>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_element_visible",
    // [ts] arguments: {
    // [ts] role: "heading",
    // [ts] accessibleName: "Inner iframe",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.locator('iframe').contentFrame().getByRole('heading', { name: 'Inner iframe' })).toBeVisible();`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_text_visible (iframe)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <h1>Outer frame text</h1>
    // [ts] <iframe srcdoc="<html><body><h1>Inner iframe</h1></body></html>" width="400" height="200">
    // [ts] </iframe>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_text_visible",
    // [ts] arguments: {
    // [ts] text: "Inner iframe",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.locator('iframe').contentFrame().getByRole('heading', { name: 'Inner iframe' })).toBeVisible();`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_text_visible", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <p>Hello world</p>
    // [ts] <div>Welcome to our site</div>
    // [ts] <span>Status: Active</span>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_text_visible",
    // [ts] arguments: {
    // [ts] text: "Hello world",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByText('Hello world')).toBeVisible();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_text_visible",
    // [ts] arguments: {
    // [ts] text: "Welcome to our site",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByText('Welcome to our site')).toBeVisible();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_text_visible",
    // [ts] arguments: {
    // [ts] text: "Status: Active",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByText('Status: Active')).toBeVisible();`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_text_visible (not found)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <p>Hello world</p>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_text_visible",
    // [ts] arguments: {
    // [ts] text: "Goodbye world",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: "Text not found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_text_visible (with quotes)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <p>She said "Hello world"</p>
    // [ts] <div>It's a beautiful day</div>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_text_visible",
    // [ts] arguments: {
    // [ts] text: "She said \"Hello world\"",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByText('She said "Hello world"')).toBeVisible();`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_text_visible",
    // [ts] arguments: {
    // [ts] text: "It's a beautiful day",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: `await expect(page.getByText('It\\'s a beautiful day')).toBeVisible();`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_list_visible", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <ul>
    // [ts] <li>Apple</li>
    // [ts] <li>Banana</li>
    // [ts] <li>Cherry</li>
    // [ts] </ul>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_list_visible",
    // [ts] arguments: {
    // [ts] element: "Fruit list",
    // [ts] target: "e2",
    // [ts] items: ["Apple", "Banana", "Cherry"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] - list:
    // [ts] - listitem: "Apple"
    // [ts] - listitem: "Banana"
    // [ts] - listitem: "Cherry"
    // [ts] \`);`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_list_visible (partial items)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <ul>
    // [ts] <li>Apple</li>
    // [ts] <li>Banana</li>
    // [ts] <li>Cherry</li>
    // [ts] <li>Date</li>
    // [ts] </ul>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_list_visible",
    // [ts] arguments: {
    // [ts] element: "Fruit list",
    // [ts] target: "e2",
    // [ts] items: ["Apple", "Cherry"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] - list:
    // [ts] - listitem: "Apple"
    // [ts] - listitem: "Cherry"
    // [ts] \`);`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_list_visible (item not found)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <ul>
    // [ts] <li>Apple</li>
    // [ts] <li>Banana</li>
    // [ts] </ul>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_list_visible",
    // [ts] arguments: {
    // [ts] element: "Fruit list",
    // [ts] target: "e2",
    // [ts] items: ["Apple", "Cherry"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: "Item \"Cherry\" not found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (textbox)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <input type="text" aria-label="Name" value="John Doe" />
    // [ts] <input type="email" aria-label="Email" value="john@example.com" />
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "textbox",
    // [ts] element: "Name textbox",
    // [ts] target: "e3",
    // [ts] value: "John Doe",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.getByRole('textbox', { name: 'Name' })).toHaveValue('John Doe');`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "textbox",
    // [ts] element: "Email textbox",
    // [ts] target: "e4",
    // [ts] value: "john@example.com",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.getByRole('textbox', { name: 'Email' })).toHaveValue('john@example.com');`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (textbox wrong value)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <input type="text" name="name" value="John Doe" />
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "textbox",
    // [ts] element: "Name textbox",
    // [ts] target: "e3",
    // [ts] value: "Jane Smith",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: "Expected value \"Jane Smith\", but got \"John Doe\"",
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (checkbox checked)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <input type="checkbox" name="subscribe" checked />
    // [ts] <label for="subscribe">Subscribe to newsletter</label>
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "checkbox",
    // [ts] element: "Subscribe checkbox",
    // [ts] target: "e3",
    // [ts] value: "true",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.getByRole('checkbox')).toBeChecked();`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (checkbox unchecked)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <input type="checkbox" name="subscribe" />
    // [ts] <label for="subscribe">Subscribe to newsletter</label>
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "checkbox",
    // [ts] element: "Subscribe checkbox",
    // [ts] target: "e3",
    // [ts] value: "false",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.getByRole('checkbox')).not.toBeChecked();`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (checkbox wrong value)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <input type="checkbox" name="subscribe" checked />
    // [ts] <label for="subscribe">Subscribe to newsletter</label>
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "checkbox",
    // [ts] element: "Subscribe checkbox",
    // [ts] target: "e3",
    // [ts] value: "false",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: "Expected value \"false\", but got \"true\"",
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (radio checked)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <label for="red">Red</label>
    // [ts] <input id="red" type="radio" name="color" value="red" checked />
    // [ts] <label for="blue">Blue</label>
    // [ts] <input id="blue" type="radio" name="color" value="blue" />
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "radio",
    // [ts] element: "Color radio",
    // [ts] target: "e3",
    // [ts] value: "true",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.getByRole('radio', { name: 'Red' })).toBeChecked();`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (slider)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <input type="range" name="volume" min="0" max="100" value="75" />
    // [ts] <label>Volume</label>
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "slider",
    // [ts] element: "Volume slider",
    // [ts] target: "e3",
    // [ts] value: "75",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.getByRole('slider')).toHaveValue('75');`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_verify_value (combobox)", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test Page</title>
    // [ts] <form>
    // [ts] <select name="country">
    // [ts] <option>Choose a country</option>
    // [ts] <option selected>United States</option>
    // [ts] <option>United Kingdom</option>
    // [ts] </select>
    // [ts] </form>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_verify_value",
    // [ts] arguments: {
    // [ts] type: "combobox",
    // [ts] element: "Country select",
    // [ts] target: "e3",
    // [ts] value: "United States",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: "Done",
    // [ts] code: expect.stringContaining(`await expect(page.getByRole('combobox')).toHaveValue('United States');`),
    // [ts] });
    // [ts] });
}
