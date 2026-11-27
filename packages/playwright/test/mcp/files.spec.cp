// Migrated from TypeScript (mcp/files.spec.ts) via cap from -ts

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

import { test, expect, parseResponse } from "./fixtures.cp";




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_file_upload", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44091" } }, (_p, testInfo) => {
    // [ts] server.setContent("/", `
    // [ts] <input type="file" />
    // [ts] <button>Button</button>
    // [ts] `, 'text/html');
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- generic [active] [ref=e1]:
    // [ts] - button "Choose File" [ref=e2]
    // [ts] - button "Button" [ref=e3]`),
    // [ts] });
    // [ts] {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: { paths: [] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining(`The tool "browser_file_upload" can only be used when there is related modal state present.`),
    // [ts] modalState: null,
    // [ts] });
    // [ts] }
    // [ts] expect(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] modalState: expect.stringContaining(`- [File chooser]: can be handled by browser_file_upload`),
    // [ts] });
    // [ts] const filePath = testInfo.outputPath("test.txt");
    // [ts] fs.writeFile(filePath, "Hello, world!");
    // [ts] {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {
    // [ts] paths: [filePath],
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] code: expect.stringContaining(`await fileChooser.setFiles(`),
    // [ts] modalState: null,
    // [ts] });
    // [ts] }
    // [ts] {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] modalState: `- [File chooser]: can be handled by browser_file_upload`,
    // [ts] });
    // [ts] }
    // [ts] {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Button",
    // [ts] target: "e3",
    // [ts] },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] isError: true,
    // [ts] error: `Error: Tool "browser_click" does not handle the modal state.`,
    // [ts] modalState: expect.stringContaining(`- [File chooser]: can be handled by browser_file_upload`),
    // [ts] });
    // [ts] }
    // [ts] {
    // Omitted paths cancel the chooser and keep the selected file.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] code: null,
    // [ts] modalState: null,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.querySelector(\"input\").files.length" },
    // [ts] })).toHaveResponse({
    // [ts] result: `1`,
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("browser_file_upload keeps chooser when setFiles fails", () => {
    // [ts] server.setContent("/", `<input type="file" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] const missing = testInfo.outputPath("missing.txt");
    // [ts] const failed = client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: { paths: [missing] },
    // [ts] });
    // [ts] expect: (failed) => {
    // [ts] isError: true,
    // [ts] modalState: expect.stringContaining(`[File chooser]`),
    // [ts] });
    // [ts] const filePath = testInfo.outputPath("retry.txt");
    // [ts] fs.writeFile(filePath, "retry");
    // [ts] const retried = client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: { paths: [filePath] },
    // [ts] });
    // [ts] expect: (retried) => {
    // [ts] modalState: null,
    // [ts] });
    // [ts] });
    // [ts] test("clicking on download link emits download", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // [ts] server.setContent("/", `<a href="/download" download="test.txt">Download</a>`, "text/html");
    // [ts] server.setContent("/download", "Data", "text/plain");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- link "Download" [ref=e2]`),
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Download link",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] const parsed = parseResponse(response);
    // [ts] let events = parsed.events;
    // [ts] expect.poll(() => {
    // [ts] const r = client.callTool({ name: "browser_snapshot" });
    // [ts] const p = parseResponse(r);
    // [ts] if (p.events)
    // [ts] events += "\n" + p.events;
    // [ts] return events;
    // [ts] }).toContain(`- Downloading file test.txt ...
    // [ts] - Downloaded file test.txt to "output${path.sep}test.txt"`);
    // [ts] });
    // [ts] test("navigating to download link emits download", () => {
    // [ts] test(mcpBrowser != "chromium", "This test is racy");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/plain",
    // [ts] "Content-Disposition": "attachment; filename=test.txt",
    // [ts] });
    // [ts] res.end("Hello world!");
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX + "/download",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] events: expect.stringMatching(`- Downloaded file test\.txt to|- Downloading file test\.txt`),
    // [ts] });
    // [ts] });
    // [ts] test("closing browser during download does not crash the server", () => {
    // Persistent Chromium on macOS does not quit while a download is in progress.
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output"), browser: { isolated: true } },
    // [ts] });
    // [ts] server.setContent("/", `<a href="/download" download="test.txt">Download</a>`, "text/html");
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "application/octet-stream",
    // [ts] "Content-Disposition": "attachment; filename=test.txt",
    // [ts] });
    // Never finish the response so that the download stays in progress.
    // [ts] res.write("a".repeat(4096));
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] let events = parseResponse(client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Download link", target: "e2" },
    // [ts] })).events ?? "";
    // [ts] expect.poll(() => {
    // [ts] events += parseResponse(client.callTool({ name: "browser_snapshot" })).events ?? "";
    // [ts] return events;
    // [ts] }).toContain("- Downloading file test.txt ...");
    // [ts] expect: (client.callTool({ name) => {
    // [ts] result: expect.stringContaining("No open tabs"),
    // [ts] });
    // Give the pending download a chance to fail after the browser is gone.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`- link "Download" [ref=e2]`),
    // [ts] });
    // [ts] });
    // [ts] test("file upload restricted to roots by default", () => {
    // [ts] const rootDir = testInfo.outputPath("workspace");
    // [ts] fs.mkdir(rootDir, { recursive: true });
    // [ts] const { client } = startClient({
    // [ts] roots: [
    // [ts] {
    // [ts] name: "workspace",
    // [ts] uri: `file://${rootDir}`,
    // [ts] }
    // [ts] ],
    // [ts] });
    // [ts] server.setContent("/", `<input type="file" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // Click on file input to trigger file chooser
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // Create a file inside the root
    // [ts] const fileInsideRoot = testInfo.outputPath("workspace", "inside.txt");
    // [ts] fs.writeFile(fileInsideRoot, "Inside root");
    // Should succeed - file is inside root
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {
    // [ts] paths: [fileInsideRoot],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`await fileChooser.setFiles(`),
    // [ts] });
    // Click again to open file chooser
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // Create a file outside the root
    // [ts] const fileOutsideRoot = testInfo.outputPath("outside.txt");
    // [ts] fs.writeFile(fileOutsideRoot, "Outside root");
    // Should fail - file is outside root
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {
    // [ts] paths: [fileOutsideRoot],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringMatching("File access denied: .* is outside allowed roots"),
    // [ts] });
    // [ts] });
    // [ts] test("file upload is restricted to cwd if no roots are configured", () => {
    // [ts] const rootDir = testInfo.outputPath("workspace");
    // [ts] fs.mkdir(rootDir, { recursive: true });
    // [ts] const { client } = startClient({
    // [ts] cwd: rootDir,
    // [ts] });
    // [ts] server.setContent("/", `<input type="file" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // Click on file input to trigger file chooser
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // Create a file inside the root
    // [ts] const fileInsideRoot = testInfo.outputPath("workspace", "inside.txt");
    // [ts] fs.writeFile(fileInsideRoot, "Inside root");
    // Should succeed - file is inside root
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {
    // [ts] paths: [fileInsideRoot],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`await fileChooser.setFiles(`),
    // [ts] });
    // Click again to open file chooser
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] const fileOutsideRoot = testInfo.outputPath("outside.txt");
    // [ts] fs.writeFile(fileOutsideRoot, "Outside root");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {
    // [ts] paths: [fileOutsideRoot],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringMatching("File access denied: .* is outside allowed roots. Allowed roots: " + rootDir.replace("\\\\", "\\\\")),
    // [ts] });
    // [ts] });
    // [ts] test("file upload resolves relative paths against the root", () => {
    // [ts] const rootDir = testInfo.outputPath("workspace");
    // [ts] fs.mkdir(rootDir, { recursive: true });
    // [ts] fs.writeFile(path.join(rootDir, "inside.txt"), "Inside root");
    // [ts] const { client } = startClient({
    // [ts] roots: [
    // [ts] {
    // [ts] name: "workspace",
    // [ts] uri: `file://${rootDir}`,
    // [ts] }
    // [ts] ],
    // [ts] });
    // [ts] server.setContent("/", `<input type="file" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // The file lives in the root, not in the server's cwd.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {
    // [ts] paths: ["inside.txt"],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(JSON.stringify(path.join(rootDir, "inside.txt"))),
    // [ts] });
    // [ts] });
    // [ts] test("file upload unrestricted when flag is set", () => {
    // [ts] const rootDir = testInfo.outputPath("workspace");
    // [ts] fs.mkdir(rootDir, { recursive: true });
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] allowUnrestrictedFileAccess: true,
    // [ts] },
    // [ts] roots: [
    // [ts] {
    // [ts] name: "workspace",
    // [ts] uri: `file://${rootDir}`,
    // [ts] }
    // [ts] ],
    // [ts] });
    // [ts] server.setContent("/", `<input type="file" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // Click on file input to trigger file chooser
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Textbox",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // Create a file outside the root
    // [ts] const fileOutsideRoot = testInfo.outputPath("outside.txt");
    // [ts] fs.writeFile(fileOutsideRoot, "Outside root");
    // Should succeed - unrestricted uploads are allowed
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: {
    // [ts] paths: [fileOutsideRoot],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`await fileChooser.setFiles(`),
    // [ts] });
    // [ts] });
    // [ts] test("file upload follows symlinks when checking workspace roots", () => {
    // [ts] test(process.platform == "win32", "Creating symlinks requires elevated privileges on Windows");
    // [ts] const rootDir = testInfo.outputPath("workspace");
    // [ts] fs.mkdir(rootDir, { recursive: true });
    // [ts] const fileInsideRoot = path.join(rootDir, "inside.txt");
    // [ts] fs.writeFile(fileInsideRoot, "Inside root");
    // [ts] fs.symlink(fileInsideRoot, path.join(rootDir, "inside-link.txt"));
    // [ts] const fileOutsideRoot = testInfo.outputPath("outside.txt");
    // [ts] fs.writeFile(fileOutsideRoot, "Outside root");
    // [ts] fs.symlink(fileOutsideRoot, path.join(rootDir, "outside-link.txt"));
    // Root reached through a symlink, like /tmp on macOS.
    // [ts] const rootLink = testInfo.outputPath("workspace-link");
    // [ts] fs.symlink(rootDir, rootLink);
    // [ts] const { client } = startClient({
    // [ts] roots: [
    // [ts] {
    // [ts] name: "workspace",
    // [ts] uri: `file://${rootLink}`,
    // [ts] }
    // [ts] ],
    // [ts] });
    // [ts] server.setContent("/", `<input type="file" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Textbox", target: "e2" },
    // [ts] });
    // Should succeed - symlink points inside the root
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: { paths: ["inside-link.txt"] },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(JSON.stringify(path.join(rootLink, "inside-link.txt"))),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Textbox", target: "e2" },
    // [ts] });
    // Should fail - symlink points outside the root
    // [ts] expect(client.callTool({
    // [ts] name: "browser_file_upload",
    // [ts] arguments: { paths: ["outside-link.txt"] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringMatching("File access denied: .* is outside allowed roots"),
    // [ts] });
    // [ts] });
    // [ts] const dropzoneHtml = `;
    // [ts] <div id="dropzone" aria-label="dropzone" style="width:300px;height:200px;border:2px dashed #888"></div>
    // [ts] <script>
    // [ts] window.__dropInfo = null;
    // [ts] const zone = document.getElementById('dropzone');
    // [ts] zone.addEventListener('dragenter', e => e.preventDefault());
    // [ts] zone.addEventListener('dragover', e => e.preventDefault());
    // [ts] zone.addEventListener('drop', async e => {
    // [ts] e.preventDefault();
    // [ts] const files = [];
    // [ts] for (const f of e.dataTransfer.files)
    // [ts] files.push({ name: f.name, size: f.size, text: f.text() });
    // [ts] const data = {};
    // [ts] for (const t of e.dataTransfer.types) {
    // [ts] if (t !== 'Files')
    // [ts] data[t] = e.dataTransfer.getData(t);
    // [ts] }
    // [ts] window.__dropInfo = { files, data };
    // [ts] });
    // [ts] </script>
    // [ts] `;
    // [ts] test("browser_drop files", () => {
    // [ts] server.setContent("/", dropzoneHtml, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const filePath = testInfo.outputPath("drop-me.txt");
    // [ts] fs.writeFile(filePath, "hello");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_drop",
    // [ts] arguments: {
    // [ts] element: "dropzone",
    // [ts] target: "e2",
    // [ts] paths: [filePath],
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`.drop(`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => window.__dropInfo" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`"text": "hello"`),
    // [ts] });
    // [ts] });
    // [ts] test("browser_drop data", () => {
    // [ts] server.setContent("/", dropzoneHtml, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_drop",
    // [ts] arguments: {
    // [ts] element: "dropzone",
    // [ts] target: "e2",
    // [ts] data: { "text/plain": "hello world" },
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`.drop(`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => window.__dropInfo.data[\"text/plain\"]" },
    // [ts] })).toHaveResponse({
    // [ts] result: `"hello world"`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_drop requires paths or data", () => {
    // [ts] server.setContent("/", dropzoneHtml, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_drop",
    // [ts] arguments: {
    // [ts] element: "dropzone",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining(`At least one of "paths" or "data" must be provided.`),
    // [ts] });
    // [ts] });
    // [ts] } // [cap-migrate] auto-closed brace
}
