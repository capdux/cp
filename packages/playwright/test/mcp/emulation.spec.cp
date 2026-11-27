// Migrated from TypeScript (mcp/emulation.spec.ts) via cap from -ts

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
    // [ts] test("browser_emulate_media", () => {
    // [ts] server.setContent("/", `
    // [ts] <div id="scheme"></div>
    // [ts] <div id="media"></div>
    // [ts] <script>
    // [ts] const colorScheme = matchMedia('(prefers-color-scheme: dark)');
    // [ts] const print = matchMedia('print');
    // [ts] const update = () => {
    // [ts] document.getElementById('scheme').textContent = \`Color scheme: \${colorScheme.matches ? 'dark' : 'light'}\`;
    // [ts] document.getElementById('media').textContent = \`Media type: \${print.matches ? 'print' : 'screen'}\`;
    // [ts] };
    // [ts] colorScheme.addEventListener('change', update);
    // [ts] print.addEventListener('change', update);
    // [ts] update();
    // [ts] </script>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_emulate_media",
    // [ts] arguments: { colorScheme: "dark" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.emulateMedia({ colorScheme: 'dark' });`,
    // [ts] });
    // [ts] expect.poll(() => client.callTool({ name: "browser_snapshot" })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`Color scheme: dark`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_emulate_media",
    // [ts] arguments: { media: "screen", reducedMotion: "reduce" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.emulateMedia({ media: 'screen', reducedMotion: 'reduce' });`,
    // [ts] });
    // [ts] expect.poll(() => client.callTool({ name: "browser_snapshot" })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`Color scheme: dark`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_emulate_media",
    // [ts] arguments: { media: "print" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.emulateMedia({ media: 'print' });`,
    // [ts] });
    // [ts] expect.poll(() => client.callTool({ name: "browser_snapshot" })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`Media type: print`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_emulate_media",
    // [ts] arguments: { colorScheme: null, contrast: null, forcedColors: null, media: null, reducedMotion: null },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.emulateMedia({ colorScheme: null, contrast: null, forcedColors: null, media: null, reducedMotion: null });`,
    // [ts] });
    // [ts] expect.poll(() => client.callTool({ name: "browser_snapshot" })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`Media type: screen`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_emulate_media requires a media feature", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_emulate_media",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("Specify at least one media feature to emulate."),
    // [ts] isError: true,
    // [ts] });
    // [ts] });
}
