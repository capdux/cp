// Migrated from TypeScript (mcp/pdf.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("save as pdf unavailable", () => {
    // [ts] const { client } = startClient();
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_pdf_save",
    // [ts] })).toHaveResponse({
    // [ts] error: "Tool \"browser_pdf_save\" not found",
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("save as pdf", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output"), capabilities: ["pdf"] },
    // [ts] });
    // [ts] test(!!mcpBrowser && !["chromium", "chrome", "msedge"].includes(mcpBrowser), "Save as PDF is only supported in Chromium.");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [active] [ref=e1]: Hello, world!`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_pdf_save",
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`await page.pdf(`),
    // [ts] result: expect.stringMatching("\\[Page as pdf\\]\\(.*page-[^:]+.pdf\\)"),
    // [ts] });
    // [ts] });
    // [ts] test("save as pdf (filename: output.pdf)", () => {
    // [ts] test(!!mcpBrowser && !["chromium", "chrome", "msedge"].includes(mcpBrowser), "Save as PDF is only supported in Chromium.");
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["pdf"] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [active] [ref=e1]: Hello, world!`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_pdf_save",
    // [ts] arguments: {
    // [ts] filename: "output.pdf",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`output.pdf`),
    // [ts] code: expect.stringContaining(`await page.pdf(`),
    // [ts] });
    // [ts] const files = [...fs.readdirSync(testInfo.outputPath())];
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath()), true);
    // [ts] const pdfFiles = files.filter(f => f.endsWith(".pdf"));
    // [ts] expect(pdfFiles).toHaveLength(1);
    // [ts] expect(pdfFiles[0]).toMatch("^output.pdf$");
    // [ts] });
}
