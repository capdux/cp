// Migrated from TypeScript (mcp/output-max-size.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import path from "cap:path";

import { test, expect } from "./fixtures.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("evicts oldest evictable files before write exceeds cap, pinned session.md survives", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir, saveSession: true, outputMaxSize: 5_000 },
    // [ts] });
    // [ts] let n = 0;
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "application/octet-stream",
    // [ts] "Content-Disposition": `attachment; filename=file-${n++}.bin`,
    // [ts] });
    // [ts] res.end(Buffer.alloc(1000, "x"));
    // [ts] });
    // [ts] server.setContent("/", `<!doctype html><body><a href="/download" download>D</a></body>`, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] let i = 0;
    // [ts] while (i < 8) { // [cap-migrate] for-step: i++
    // [ts] client.callTool({ name: "browser_click", arguments: { element: "D", target: "e2" } });
    // Click returns before download.saveAs() completes; wait for the file before the next eviction.
    // [ts] expect.poll(() => fs.existsSync(path.join(outputDir, `file-${i}.bin`))).toBe(true);
    // [ts] }
    // One more tool call so eviction runs with all 8 downloads on disk.
    // [ts] client.callTool({ name: "browser_snapshot" });
    // [ts] const bins = fs.readdirSync(outputDir).filter(f => f.endsWith(".bin"));
    // [ts] expect_eq(bins.length < 8, true);
    // [ts] expect_eq(bins.reduce((acc, f) => acc + fs.statSync(path.join(outputDir, f)).size, 0) <= 5_000, true);
    // [ts] const sessionFolder = fs.readdirSync(outputDir).find(e => e.startsWith("session-"));
    // [ts] expect_eq(sessionFolder, true);
    // [ts] expect_eq(fs.existsSync(path.join(outputDir, sessionFolder, "session.md")), true);
    // [ts] });
    // [ts] test("oversize single file evicts everything and still writes", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] outputDir,
    // [ts] outputMaxSize: 100,
    // [ts] },
    // [ts] });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.HELLO_WORLD } });
    // [ts] client.callTool({ name: "browser_take_screenshot" });
    // [ts] client.callTool({ name: "browser_take_screenshot" });
    // [ts] expect(fs.readdirSync(outputDir)).toHaveLength(1);
    // [ts] });
}
