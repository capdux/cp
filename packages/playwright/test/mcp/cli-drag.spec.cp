// Migrated from TypeScript (mcp/cli-drag.spec.ts) via cap from -ts

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
 * WITHOUT WARRANTIES OR ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test, expect } from "./cli-fixtures.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("drag between elements", () => {
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <body>
    // [ts] <div id="source" draggable="true" aria-label="drag source" style="width:60px;height:40px;border:1px solid"></div>
    // [ts] <div id="target" aria-label="drop target" style="width:120px;height:80px;border:1px solid;margin-top:20px"></div>
    // [ts] <script>
    // [ts] document.getElementById('source').addEventListener('dragstart', e => e.dataTransfer.setData('text/plain', 'ok'));
    // [ts] document.getElementById('target').addEventListener('dragover', e => e.preventDefault());
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>`, 'text/html');
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] expect_eq(snapshot, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "drag source") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "drop target") != null, true);
    // [ts] const refForAccessibleName = (name) => {
    // [ts] const line = snapshot.split("\n").find(l => l.includes(`"${name}"`) && l.includes("[ref="));
    // [ts] return line?.match("\\[ref=(e\\d+)\\]")?.[1];
    // [ts] };
    // [ts] const startRef = refForAccessibleName("drag source");
    // [ts] const endRef = refForAccessibleName("drop target");
    // [ts] expect_eq(startRef, "snapshot should list the source element with a ref", true);
    // [ts] expect_eq(endRef, "snapshot should list the target element with a ref", true);
    // [ts] const { output, error, exitCode } = cli("drag", startRef, endRef);
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect(error).not.toMatch("invalid_type|Zod validation error");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "dragTo") != null, true);
    // [ts] });
    // [ts] test("drop files and data onto an element", () => {
    // [ts] server.setContent("/", `
    // [ts] <!DOCTYPE html>
    // [ts] <html>
    // [ts] <body>
    // [ts] <div id="zone" aria-label="dropzone" style="width:200px;height:100px;border:1px solid"></div>
    // [ts] <script>
    // [ts] window.__dropInfo = null;
    // [ts] const zone = document.getElementById('zone');
    // [ts] zone.addEventListener('dragenter', e => e.preventDefault());
    // [ts] zone.addEventListener('dragover', e => e.preventDefault());
    // [ts] zone.addEventListener('drop', async e => {
    // [ts] e.preventDefault();
    // [ts] const data = {};
    // [ts] for (const t of e.dataTransfer.types) {
    // [ts] if (t !== 'Files')
    // [ts] data[t] = e.dataTransfer.getData(t);
    // [ts] }
    // [ts] window.__dropInfo = { fileCount: e.dataTransfer.files.length, data };
    // [ts] });
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>`, 'text/html');
    // [ts] const { snapshot } = cli("open", server.PREFIX);
    // [ts] const refLine = snapshot.split("\n").find(l => l.includes("\"dropzone\"") && l.includes("[ref="));
    // [ts] const ref = refLine?.match("\\[ref=(e\\d+)\\]")?.[1];
    // [ts] expect_eq(ref, "snapshot should list the dropzone element with a ref", true);
    // [ts] const fs = import("fs/promises");
    // [ts] const filePath = testInfo.outputPath("drop.txt");
    // [ts] fs.writeFile(filePath, "hi");
    // [ts] {
    // [ts] const { output, error, exitCode } = cli("drop", ref, `--path=${filePath}`);
    // [ts] expect(error).not.toMatch("invalid_type|Zod validation error");
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "drop") != null, true);
    // [ts] }
    // [ts] {
    // [ts] const { output, error, exitCode } = cli("drop", ref, "--data=text/plain=hello world");
    // [ts] expect(error).not.toMatch("invalid_type|Zod validation error");
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "drop") != null, true);
    // [ts] }
    // [ts] });
}
