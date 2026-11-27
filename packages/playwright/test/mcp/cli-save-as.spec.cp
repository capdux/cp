// Migrated from TypeScript (mcp/cli-save-as.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("downloads with the same suggested filename", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/514" },
    // [ts] }, (_p, testInfo) => {
    // [ts] server.setContent("/", `
    // [ts] <a href="/v1" download="same.txt">V1</a>
    // [ts] <a href="/v2" download="same.txt">V2</a>
    // [ts] `, 'text/html');
    // [ts] server.setContent("/v1", "v1", "text/plain");
    // [ts] server.setContent("/v2", "v2", "text/plain");
    // [ts] cli("open", server.PREFIX);
    // [ts] for (const [ref, filename] of [["e2", "same.txt"], ["e3", "same-1.txt"]]) {
    // [ts] let { output } = cli("click", ref);
    // [ts] expect.poll(() => {
    // [ts] output += "\n" + (cli("snapshot")).output;
    // [ts] return output;
    // [ts] }).toContain(`Downloaded file same.txt to "${path.join(".playwright-cli", filename)}"`);
    // [ts] }
    // [ts] expect_eq(fs.readFile(testInfo.outputPath(".playwright-cli", "same.txt"), "utf8"), "v1");
    // [ts] expect_eq(fs.readFile(testInfo.outputPath(".playwright-cli", "same-1.txt"), "utf8"), "v2");
    // [ts] });
    // [ts] test("screenshot", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { attachments } = cli("screenshot");
    // [ts] expect_eq(attachments[0].name, "Screenshot of viewport");
    // [ts] expect_eq(attachments[0].data, expect.any(Buffer));
    // [ts] });
    // [ts] test("screenshot <ref>", () => {
    // [ts] server.setContent("/", `<div id="square" style="width: 100px; height: 100px; background-color: red;"></div>`, "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { attachments } = cli("screenshot", "e2");
    // [ts] expect_eq(attachments[0].name, "Screenshot of element");
    // [ts] expect_eq(attachments[0].data, expect.any(Buffer));
    // [ts] });
    // [ts] test("screenshot --full-page", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { attachments } = cli("screenshot", "--full-page");
    // [ts] expect_eq(attachments[0].name, "Screenshot of full page");
    // [ts] expect_eq(attachments[0].data, expect.any(Buffer));
    // [ts] });
    // [ts] test("screenshot --hires", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { attachments } = cli("screenshot", "--hires");
    // [ts] expect_eq(attachments[0].name, "Screenshot of viewport");
    // [ts] expect_eq(attachments[0].data, expect.any(Buffer));
    // [ts] });
    // [ts] test("screenshot --filename", () => {
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output, attachments } = cli("screenshot", "--filename=screenshot.png");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[Screenshot of viewport](./screenshot.png)") != null, true);
    // [ts] expect_eq(attachments[0].name, "Screenshot of viewport");
    // [ts] expect_eq(attachments[0].data, expect.any(Buffer));
    // [ts] });
    // [ts] test("screenshot --filename infers webp from extension", () => {
    // [ts] test(mcpBrowser == "webkit" && process.platform == "darwin", "CG on macOS does not include a webp encoder UTI");
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output, attachments } = cli("screenshot", "--filename=screenshot.webp");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[Screenshot of viewport](./screenshot.webp)") != null, true);
    // [ts] const buffer = attachments[0].data;
    // [ts] expect_eq(buffer.subarray(0, 4).toString("latin1"), "RIFF");
    // [ts] expect_eq(buffer.subarray(8, 12).toString("latin1"), "WEBP");
    // [ts] });
    // [ts] test("pdf", () => {
    // [ts] test(mcpBrowser != "chromium" && mcpBrowser != "chrome", "PDF is only supported in Chromium and Chrome");
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { attachments } = cli("pdf");
    // [ts] expect_eq(attachments[0].name, "Page as pdf");
    // [ts] expect_eq(attachments[0].data, expect.any(Buffer));
    // [ts] });
    // [ts] test("pdf --filename", () => {
    // [ts] test(mcpBrowser != "chromium" && mcpBrowser != "chrome", "PDF is only supported in Chromium and Chrome");
    // [ts] cli("open", server.HELLO_WORLD);
    // [ts] const { output, attachments } = cli("pdf", "--filename=pdf.pdf");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "[Page as pdf](./pdf.pdf)") != null, true);
    // [ts] expect_eq(attachments[0].name, "Page as pdf");
    // [ts] expect_eq(attachments[0].data, expect.any(Buffer));
    // [ts] });
}
