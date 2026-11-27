// Migrated from TypeScript (mcp/form.spec.ts) via cap from -ts

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
    // [ts] test("browser_fill_form (textbox)", () => {
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <body>
    // [ts] <form>
    // [ts] <label>
    // [ts] <input type="text" id="name" name="name" />
    // [ts] Name
    // [ts] </label>
    // [ts] <label>
    // [ts] <input type="email" id="email" name="email" />
    // [ts] Email
    // [ts] </label>
    // [ts] <label>
    // [ts] <input type="range" id="age" name="age" min="18" max="100" />
    // [ts] Age
    // [ts] </label>
    // [ts] <label>
    // [ts] <select id="country" name="country">
    // [ts] <option value="">Choose a country</option>
    // [ts] <option value="us">United States</option>
    // [ts] <option value="uk">United Kingdom</option>
    // [ts] </select>
    // [ts] Country
    // [ts] </label>
    // [ts] <label>
    // [ts] <input type="checkbox" name="subscribe" value="newsletter" />
    // [ts] Subscribe to newsletter
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
    // [ts] name: "Name textbox",
    // [ts] type: "textbox",
    // [ts] target: "e4",
    // [ts] value: "John Doe"
    // [ts] },
    // [ts] {
    // [ts] name: "Email textbox",
    // [ts] type: "textbox",
    // [ts] target: "e6",
    // [ts] value: "john.doe@example.com"
    // [ts] },
    // [ts] {
    // [ts] name: "Age textbox",
    // [ts] type: "slider",
    // [ts] target: "e8",
    // [ts] value: "25"
    // [ts] },
    // [ts] {
    // [ts] name: "Country select",
    // [ts] type: "combobox",
    // [ts] target: "e10",
    // [ts] value: "United States"
    // [ts] },
    // [ts] {
    // [ts] name: "Subscribe checkbox",
    // [ts] type: "checkbox",
    // [ts] target: "e12",
    // [ts] value: "true"
    // [ts] },
    // [ts] ]
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.getByRole('textbox', { name: 'Name' }).fill('John Doe');
    // [ts] page.getByRole('textbox', { name: 'Email' }).fill('john.doe@example.com');
    // [ts] page.getByRole('slider', { name: 'Age' }).fill('25');
    // [ts] page.getByLabel('Choose a country United').selectOption('United States');
    // [ts] page.getByRole('checkbox', { name: 'Subscribe to newsletter' }).check();`,
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {
    // [ts] },
    // [ts] });
    // [ts] expect.soft(response).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringMatching("textbox \"Name\".*John Doe"),
    // [ts] });
    // [ts] expect.soft(response).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringMatching("textbox \"Email\".*john.doe@example.com"),
    // [ts] });
    // [ts] expect.soft(response).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringMatching("slider \"Age\".*\"25\""),
    // [ts] });
    // [ts] expect.soft(response).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining("option \"United States\" [selected]"),
    // [ts] });
    // [ts] expect.soft(response).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining("checkbox \"Subscribe to newsletter\" [checked]"),
    // [ts] });
    // [ts] });
}
