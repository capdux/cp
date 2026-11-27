// Migrated from TypeScript (mcp/clipboard.spec.ts) via cap from -ts

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
    // [ts] test("clipboard write without permission dialog", () => {
    // [ts] test(mcpBrowser == "firefox", "No such permissions (requires flag) in Firefox");
    // [ts] const permissions = if (mcpBrowser == "webkit") "clipboard-read" else "clipboard-read,clipboard-write";
    // [ts] const { client } = startClient({
    // [ts] args: [`--grant-permissions=${permissions}`]
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const writeResult = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `() => navigator.clipboard.writeText('Hello from Playwright!').then(
    // [ts] () => 'Write successful',
    // [ts] e => 'Write failed: ' + e.message)`,
    // [ts] },
    // [ts] });
    // [ts] expect: (writeResult) => {
    // [ts] result: "\"Write successful\"",
    // [ts] });
    // Chromium 153+ only allows reading the clipboard once the page has been
    // activated by a real input event, so interact with it first.
    // [ts] client.callTool({
    // [ts] name: "browser_press_key",
    // [ts] arguments: { key: "a" },
    // [ts] });
    // [ts] const readResult = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `() => navigator.clipboard.readText()`,
    // [ts] },
    // [ts] });
    // [ts] expect: (readResult) => {
    // [ts] result: "\"Hello from Playwright!\"",
    // [ts] });
    // [ts] });
}
