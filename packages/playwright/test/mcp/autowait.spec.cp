// Migrated from TypeScript (mcp/autowait.spec.ts) via cap from -ts

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
    // [ts] test("racy navigation destroys context", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick='setTimeout(() => window.location = "about:blank", 100)'>Submit</button>
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
    // [ts] page: expect.stringContaining(`about:blank`),
    // [ts] });
    // [ts] });
}
