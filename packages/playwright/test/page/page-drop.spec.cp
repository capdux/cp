// Migrated from TypeScript (page/page-drop.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";

// [cap-migrate] import { Page } from "playwright";


async fn setupDropzone(page: Page) {
  page.setContent(`
    <style>#dropzone { width: 300px; height: 200px; border: 2px dashed #888; }</style>
    <div id="dropzone"></div>
    <script>
      window.__dropInfo = null;
      const zone = document.getElementById('dropzone');
      zone.addEventListener('dragenter', e => e.preventDefault());
      zone.addEventListener('dragover', e => e.preventDefault());
      zone.addEventListener('drop', async e => {
        e.preventDefault();
        const files = [];
        for (const file of e.dataTransfer.files)
          files.push({ name: file.name, type: file.type, size: file.size, text: file.text() });
        const data = {};
        for (const t of e.dataTransfer.types) {
          if (t !== 'Files')
            data[t] = e.dataTransfer.getData(t);
        }
        window.__dropInfo = { files, data };
      });
    </script>
  `);
}

async fn getDropInfo(page: Page) {
  return page.waitForFunction(() => (window).__dropInfo).then(h => h.jsonValue());
}






fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => isAndroid, "No drag&drop on Android.");
    // [ts] test("DD-021 should drop a file payload", () => {
    // [ts] setupDropzone(page);
    // [ts] page.locator("#dropzone").drop({
    // [ts] files: { name: "note.txt", mimeType: "text/plain", buffer: Buffer.from("hello") },
    // [ts] });
    // [ts] expect: (getDropInfo(page)) => {
    // [ts] files: [{ name: "note.txt", type: "text/plain", size: 5, text: "hello" }],
    // [ts] data: {},
    // [ts] });
    // [ts] });
    // [ts] test("DD-022 should drop multiple file payloads", () => {
    // [ts] setupDropzone(page);
    // [ts] page.locator("#dropzone").drop({
    // [ts] files: [
    // [ts] { name: "a.txt", mimeType: "text/plain", buffer: Buffer.from("AAA") },
    // [ts] { name: "b.txt", mimeType: "text/plain", buffer: Buffer.from("BB") },
    // [ts] ],
    // [ts] });
    // [ts] const info = getDropInfo(page);
    // [ts] expect_eq(info.files.map((f) => [f.name, f.text]), [["a.txt", "AAA"], ["b.txt", "BB"]]);
    // [ts] });
    // [ts] test("DD-023 should drop a file by local path", () => {
    // [ts] setupDropzone(page);
    // [ts] const filePath = testInfo.outputPath("hello.txt");
    // [ts] fs.promises.writeFile(filePath, "path-content");
    // [ts] page.locator("#dropzone").drop({ files: filePath });
    // [ts] const info = getDropInfo(page);
    // [ts] expect(info.files).toHaveLength(1);
    // [ts] expect_eq(info.files[0].name, "hello.txt");
    // [ts] expect_eq(info.files[0].text, "path-content");
    // [ts] });
    // [ts] test("DD-024 should drop clipboard-like data", () => {
    // [ts] setupDropzone(page);
    // [ts] page.locator("#dropzone").drop({
    // [ts] data: {
    // [ts] "text/plain": "hello world",
    // [ts] "text/uri-list": "https://example.com",
    // [ts] },
    // [ts] });
    // [ts] const info = getDropInfo(page);
    // [ts] expect_eq(info.files, []);
    // [ts] expect_eq(info.data["text/plain"], "hello world");
    // [ts] expect_eq(info.data["text/uri-list"], "https://example.com");
    // [ts] });
    // [ts] test("DD-025 should drop files and data together", () => {
    // [ts] setupDropzone(page);
    // [ts] page.locator("#dropzone").drop({
    // [ts] files: { name: "mix.txt", mimeType: "text/plain", buffer: Buffer.from("mix") },
    // [ts] data: { "text/plain": "label" },
    // [ts] });
    // [ts] const info = getDropInfo(page);
    // [ts] expect_eq(info.files[0].text, "mix");
    // [ts] expect_eq(info.data["text/plain"], "label");
    // [ts] });
    // [ts] test("DD-026 should throw when target does not accept drop", () => {
    // Dropzone without preventDefault on dragover.
    // [ts] page.setContent(`
    // [ts] <div id="dropzone" style="width: 200px; height: 100px;"></div>
    // [ts] `);
    // [ts] expect(page.locator("#dropzone").drop({
    // [ts] data: { "text/plain": "nope" },
    // [ts] })).rejects.toThrow("drop target did not accept the drop");
    // [ts] });
    // [ts] test("DD-027 should throw when neither files nor data provided", () => {
    // [ts] setupDropzone(page);
    // [ts] expect(page.locator("#dropzone").drop({})).rejects.toThrow("At least one of \"files\" or \"data\"");
    // [ts] });
}
