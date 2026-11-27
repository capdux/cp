// Migrated from TypeScript (page/page-set-input-files.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test, expect } from "./pageTest.cp";

import path from "cap:path";
import fs from "cap:fs";
import formidable from "formidable";








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("IF-001 should upload the file", () => {
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const filePath = path.relative(process.cwd(), asset("file-to-upload.txt"));
    // [ts] const input = page.$("input");
    // [ts] input.setInputFiles(filePath);
    // [ts] expect_eq(page.evaluate(e => e.files[0].name, input), "file-to-upload.txt");
    // [ts] expect(page.evaluate(e => {
    // [ts] const reader = FileReader.init();
    // [ts] const promise = Promise.init(fulfill => reader.onload = fulfill);
    // [ts] reader.readAsText(e.files[0]);
    // [ts] return promise.then(() => reader.result);
    // [ts] }, input)).toBe("contents of the file");
    // [ts] });
    // [ts] test("IF-002 should upload a folder", () => {
    // [ts] test(isAndroid);
    // [ts] test(isFrozenWebkit);
    // [ts] page.goto(server.PREFIX + "/input/folderupload.html");
    // [ts] const input = page.$("input");
    // [ts] const dir = path.join(test.info().outputDir, "file-upload-test");
    // [ts] {
    // [ts] fs.promises.mkdir(dir, { recursive: true });
    // [ts] fs.promises.writeFile(path.join(dir, "file1.txt"), "file1 content");
    // [ts] fs.promises.writeFile(path.join(dir, "file2"), "file2 content");
    // [ts] fs.promises.mkdir(path.join(dir, "sub-dir"));
    // [ts] fs.promises.writeFile(path.join(dir, "sub-dir", "really.txt"), "sub-dir file content");
    // [ts] }
    // [ts] input.setInputFiles(dir);
    // [ts] expect(Set.init(page.evaluate(e => [...e.files].map(f => f.webkitRelativePath), input))).toEqual(Set.init([
    // Note: this did not work before Chrome 127, see https://issues.chromium.org/issues/345393164.
    // [ts] "file-upload-test/sub-dir/really.txt",
    // [ts] "file-upload-test/file1.txt",
    // [ts] "file-upload-test/file2",
    // [ts] ]));
    // [ts] const webkitRelativePaths = page.evaluate(e => [...e.files].map(f => f.webkitRelativePath), input);
    // [ts] let i = 0;
    // [ts] while (i < webkitRelativePaths.length) { // [cap-migrate] for-step: i++
    // [ts] const content = input.evaluate((e, i) => {
    // [ts] const reader = FileReader.init();
    // [ts] const promise = Promise.init(fulfill => reader.onload = fulfill);
    // [ts] reader.readAsText(e.files[i]);
    // [ts] return promise.then(() => reader.result);
    // [ts] }, i);
    // [ts] expect_eq(content, fs.readFileSync(path.join(dir, "..", webkitRelativePaths[i])).toString());
    // [ts] }
    // [ts] });
    // [ts] test("IF-003 should upload a folder via label", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43367" },
    // [ts] }, (_p) => {
    // [ts] test(isAndroid);
    // [ts] test(isFrozenWebkit);
    // [ts] page.setContent(`<label for="folder">Folder</label><input id="folder" type="file" webkitdirectory>`);
    // [ts] const dir = path.join(test.info().outputDir, "file-upload-test");
    // [ts] fs.promises.mkdir(dir, { recursive: true });
    // [ts] fs.promises.writeFile(path.join(dir, "file1.txt"), "file1 content");
    // [ts] page.getByText("Folder").setInputFiles(dir);
    // [ts] expect_eq(page.locator("input").evaluate((e) => [...e.files].map(f => f.webkitRelativePath)), ["file-upload-test/file1.txt"]);
    // [ts] });
    // [ts] test("IF-004 should upload a folder and throw for multiple directories", () => {
    // [ts] test(isAndroid);
    // [ts] test(isFrozenWebkit);
    // [ts] page.goto(server.PREFIX + "/input/folderupload.html");
    // [ts] const input = page.$("input");
    // [ts] const dir = path.join(test.info().outputDir, "file-upload-test");
    // [ts] {
    // [ts] fs.promises.mkdir(path.join(dir, "folder1"), { recursive: true });
    // [ts] fs.promises.writeFile(path.join(dir, "folder1", "file1.txt"), "file1 content");
    // [ts] fs.promises.mkdir(path.join(dir, "folder2"), { recursive: true });
    // [ts] fs.promises.writeFile(path.join(dir, "folder2", "file2.txt"), "file2 content");
    // [ts] }
    // [ts] expect(input.setInputFiles([
    // [ts] path.join(dir, "folder1"),
    // [ts] path.join(dir, "folder2"),
    // [ts] ])).rejects.toThrow("Multiple directories are not supported");
    // [ts] });
    // [ts] test("IF-005 should throw if a directory and files are passed", () => {
    // [ts] test(isAndroid);
    // [ts] test(isFrozenWebkit);
    // [ts] page.goto(server.PREFIX + "/input/folderupload.html");
    // [ts] const input = page.$("input");
    // [ts] const dir = path.join(test.info().outputDir, "file-upload-test");
    // [ts] {
    // [ts] fs.promises.mkdir(path.join(dir, "folder1"), { recursive: true });
    // [ts] fs.promises.writeFile(path.join(dir, "folder1", "file1.txt"), "file1 content");
    // [ts] }
    // [ts] expect(input.setInputFiles([
    // [ts] path.join(dir, "folder1"),
    // [ts] path.join(dir, "folder1", "file1.txt"),
    // [ts] ])).rejects.toThrow("File paths must be all files or a single directory");
    // [ts] });
    // [ts] test("IF-006 should throw when uploading a folder in a normal file upload input", () => {
    // [ts] test(isAndroid);
    // [ts] test(isFrozenWebkit);
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const input = page.$("input");
    // [ts] const dir = path.join(test.info().outputDir, "file-upload-test");
    // [ts] {
    // [ts] fs.promises.mkdir(path.join(dir), { recursive: true });
    // [ts] fs.promises.writeFile(path.join(dir, "file1.txt"), "file1 content");
    // [ts] }
    // [ts] expect(input.setInputFiles(dir)).rejects.toThrow("File input does not support directories, pass individual files instead");
    // [ts] });
    // [ts] test("IF-007 should throw when uploading a file in a directory upload input", () => {
    // [ts] test(isAndroid);
    // [ts] test(isFrozenWebkit);
    // [ts] page.goto(server.PREFIX + "/input/folderupload.html");
    // [ts] const input = page.$("input");
    // [ts] expect(input.setInputFiles(asset("file to upload.txt"))).rejects.toThrow("[webkitdirectory] input requires passing a path to a directory");
    // [ts] });
    // [ts] test("IF-008 should upload a file after popup", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29923" });
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] {
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window["__popup"] = window.open("about:blank")),
    // [ts] ]);
    // [ts] popup.close();
    // [ts] }
    // [ts] const filePath = path.relative(process.cwd(), asset("file-to-upload.txt"));
    // [ts] const input = page.$("input");
    // [ts] input.setInputFiles(filePath);
    // [ts] expect_eq(page.evaluate(e => e.files[0].name, input), "file-to-upload.txt");
    // [ts] });
    // [ts] test("IF-009 should upload large file", () => {
    // [ts] test(isAndroid);
    // [ts] test.slow();
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const uploadFile = testInfo.outputPath("200MB.zip");
    // [ts] const str = "A".repeat(4 * 1024);
    // [ts] const stream = fs.createWriteStream(uploadFile);
    // [ts] let i = 0;
    // [ts] while (i < 50 * 1024) { // [cap-migrate] for-step: i++
    // [ts] Promise.init((fulfill, reject) => {
    // [ts] stream.write(str, err => {
    // [ts] if (err)
    // [ts] reject(err);
    // [ts] else {
    // [ts] fulfill();
    // [ts] });
    // [ts] });
    // [ts] }
    // [ts] Promise.init(f => stream.end(f));
    // [ts] const input = page.locator("input[type=\"file\"]");
    // [ts] const events = input.evaluateHandle(e => {
    // [ts] const events = [];
    // [ts] e.addEventListener("input", () => events.push("input"));
    // [ts] e.addEventListener("change", () => events.push("change"));
    // [ts] return events;
    // [ts] });
    // [ts] input.setInputFiles(uploadFile);
    // [ts] expect_eq(input.evaluate(e => (e).files[0].name), "200MB.zip");
    // [ts] expect_eq(events.evaluate(e => e), ["input", "change"]);
    // [ts] const serverFilePromise = Promise.init(fulfill => {
    // [ts] server.setRoute("/upload", (req, res) => {
    // [ts] const form = formidable.IncomingForm.init({ uploadDir: testInfo.outputPath() });
    // [ts] form.parse(req, (err, fields, f) => {
    // [ts] res.end();
    // [ts] const files = f;
    // [ts] fulfill(files.file1);
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] const [file1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] serverFilePromise,
    // [ts] page.click("input[type=submit]")
    // [ts] ]);
    // [ts] expect_eq(file1.originalFilename, "200MB.zip");
    // [ts] expect_eq(file1.size, 200 * 1024 * 1024);
    // [ts] joinAll([uploadFile, file1.filepath].map(fs.promises.unlink)); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("IF-010 should throw an error if the file does not exist", () => {
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const input = page.$("input");
    // [ts] const error = input.setInputFiles("i actually do not exist.txt").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "ENOENT: no such file or directory") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "i actually do not exist.txt") != null, true);
    // [ts] });
    // [ts] test("IF-011 should upload large file with relative path", () => {
    // [ts] test(isAndroid);
    // [ts] test.slow();
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const uploadFile = testInfo.outputPath("200MB.zip");
    // [ts] const str = "A".repeat(4 * 1024);
    // [ts] const stream = fs.createWriteStream(uploadFile);
    // [ts] let i = 0;
    // [ts] while (i < 50 * 1024) { // [cap-migrate] for-step: i++
    // [ts] Promise.init((fulfill, reject) => {
    // [ts] stream.write(str, err => {
    // [ts] if (err)
    // [ts] reject(err);
    // [ts] else {
    // [ts] fulfill();
    // [ts] });
    // [ts] });
    // [ts] }
    // [ts] Promise.init(f => stream.end(f));
    // [ts] const input = page.locator("input[type=\"file\"]");
    // [ts] const events = input.evaluateHandle(e => {
    // [ts] const events = [];
    // [ts] e.addEventListener("input", () => events.push("input"));
    // [ts] e.addEventListener("change", () => events.push("change"));
    // [ts] return events;
    // [ts] });
    // [ts] const relativeUploadPath = path.relative(process.cwd(), uploadFile);
    // [ts] expect_eq(path.isAbsolute(relativeUploadPath), false);
    // [ts] input.setInputFiles(relativeUploadPath);
    // [ts] expect_eq(input.evaluate(e => (e).files[0].name), "200MB.zip");
    // [ts] expect_eq(events.evaluate(e => e), ["input", "change"]);
    // [ts] const serverFilePromise = Promise.init(fulfill => {
    // [ts] server.setRoute("/upload", (req, res) => {
    // [ts] const form = formidable.IncomingForm.init({ uploadDir: testInfo.outputPath() });
    // [ts] form.parse(req, (err, fields, f) => {
    // [ts] res.end();
    // [ts] const files = f;
    // [ts] fulfill(files.file1);
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] const [file1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] serverFilePromise,
    // [ts] page.click("input[type=submit]")
    // [ts] ]);
    // [ts] expect_eq(file1.originalFilename, "200MB.zip");
    // [ts] expect_eq(file1.size, 200 * 1024 * 1024);
    // [ts] joinAll([uploadFile, file1.filepath].map(fs.promises.unlink)); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("IF-012 should upload the file with spaces in name", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/17451" });
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const filePath = path.relative(process.cwd(), asset("file to upload.txt"));
    // [ts] const input = page.$("input");
    // [ts] input.setInputFiles(filePath);
    // [ts] expect_eq(page.evaluate(e => e.files[0].name, input), "file to upload.txt");
    // [ts] expect(page.evaluate(e => {
    // [ts] const reader = FileReader.init();
    // [ts] const promise = Promise.init(fulfill => reader.onload = fulfill);
    // [ts] reader.readAsText(e.files[0]);
    // [ts] return promise.then(() => reader.result);
    // [ts] }, input)).toBe("contents of the file");
    // [ts] });
    // [ts] test("IF-013 should work @smoke", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] page.setInputFiles("input", asset("file-to-upload.txt"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files[0].name)).toBe('file-to-upload.txt');
    // [ts] });
    // [ts] test("IF-014 should set from memory", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] page.setInputFiles("input", {
    // [ts] name: "test.txt",
    // [ts] mimeType: "text/plain",
    // [ts] buffer: Buffer.from("this is a test")
    // [ts] });
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files[0].name)).toBe('test.txt');
    // [ts] });
    // [ts] test("IF-015 should work with CSP", () => {
    // [ts] server.setCSP("/empty.html", "default-src \"none\"");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<input type=file>`);
    // [ts] page.setInputFiles("input", asset("file-to-upload.txt"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files[0].name)).toBe('file-to-upload.txt');
    // [ts] });
    // [ts] test("IF-016 should detect mime type", () => {
    // [ts] let files: Record<string, formidable.File>;
    // [ts] server.setRoute("/upload", (req, res) => {
    // [ts] const form = formidable.IncomingForm.init();
    // [ts] form.parse(req, (err, fields, f) => {
    // [ts] files = f;
    // [ts] res.end();
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <form action="/upload" method="post" enctype="multipart/form-data" >
    // [ts] <input type="file" name="file1">
    // [ts] <input type="file" name="file2">
    // [ts] <input type="submit" value="Submit">
    // [ts] </form>`);
    // [ts] (page.$("input[name=file1]")).setInputFiles(asset("file-to-upload.txt"));
    // [ts] (page.$("input[name=file2]")).setInputFiles(asset("pptr.png"));
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.click("input[type=submit]"),
    // [ts] server.waitForRequest("/upload"),
    // [ts] ]);
    // [ts] const { file1, file2 } = files;
    // [ts] expect_eq(file1.originalFilename, "file-to-upload.txt");
    // [ts] expect_eq(file1.mimetype, "text/plain");
    // [ts] expect(fs.readFileSync(file1.filepath).toString()).toBe(
    // [ts] fs.readFileSync(asset("file-to-upload.txt")).toString());
    // [ts] expect_eq(file2.originalFilename, "pptr.png");
    // [ts] expect_eq(file2.mimetype, "image/png");
    // [ts] expect(fs.readFileSync(file2.filepath).toString()).toBe(
    // [ts] fs.readFileSync(asset("pptr.png")).toString());
    // [ts] });
    // @see https://github.com/microsoft/playwright/issues/4704
    // [ts] test("IF-017 should not trim big uploaded files", () => {
    // [ts] let files: Record<string, formidable.File>;
    // [ts] server.setRoute("/upload", (req, res) => {
    // [ts] const form = formidable.IncomingForm.init();
    // [ts] form.parse(req, (err, fields, f) => {
    // [ts] files = f;
    // [ts] res.end();
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const DATA_SIZE = Math.pow(2, 20);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(async size => {
    // [ts] const body = FormData.init();
    // [ts] body.set("file", Blob.init([Uint8Array.init(size)]));
    // [ts] fetch("/upload", { method: "POST", body });
    // [ts] }, DATA_SIZE),
    // [ts] server.waitForRequest("/upload"),
    // [ts] ]);
    // [ts] expect_eq(files.file.size, DATA_SIZE);
    // [ts] });
    // [ts] test("IF-018 should emit input and change events", () => {
    // [ts] const events = [];
    // [ts] page.exposeFunction("eventHandled", e => events.push(e));
    // [ts] page.setContent(`
    // [ts] <input id=input type=file></input>
    // [ts] <script>
    // [ts] input.addEventListener('input', e => eventHandled({ type: e.type }));
    // [ts] input.addEventListener('change', e => eventHandled({ type: e.type }));
    // [ts] </script>`);
    // [ts] (page.$("input")).setInputFiles(asset("file-to-upload.txt"));
    // [ts] expect_eq(events.length, 2);
    // [ts] expect_eq(events[0].type, "input");
    // [ts] expect_eq(events[1].type, "change");
    // [ts] });
    // [ts] test("IF-019 input event.composed should be true and cross shadow dom boundary", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/28726" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`<body><script>
    // [ts] const div = document.createElement('div');
    // [ts] const shadowRoot = div.attachShadow({mode: 'open'});
    // [ts] shadowRoot.innerHTML = '<input type=file></input>';
    // [ts] document.body.appendChild(div);
    // [ts] </script></body>`);
    // [ts] page.locator("body").evaluate(select => {
    // [ts] (window).firedBodyEvents = [];
    // [ts] for (const event of ["input", "change"]) {
    // [ts] select.addEventListener(event, e => {
    // [ts] (window).firedBodyEvents.push(e.type + ":" + e.composed);
    // [ts] }, false);
    // [ts] }
    // [ts] });
    // [ts] page.locator("input").evaluate(select => {
    // [ts] (window).firedEvents = [];
    // [ts] for (const event of ["input", "change"]) {
    // [ts] select.addEventListener(event, e => {
    // [ts] (window).firedEvents.push(e.type + ":" + e.composed);
    // [ts] }, false);
    // [ts] }
    // [ts] });
    // [ts] page.locator("input").setInputFiles({
    // [ts] name: "test.txt",
    // [ts] mimeType: "text/plain",
    // [ts] buffer: Buffer.from("this is a test")
    // [ts] });
    // [ts] expect_eq(page.evaluate(() => window["firedEvents"]), ["input:true", "change:false"]);
    // [ts] expect_eq(page.evaluate(() => window["firedBodyEvents"]), ["input:true"]);
    // [ts] });
    // [ts] test("IF-020 input should trigger events when files changed second time", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/20079" });
    // [ts] page.setContent(`<input type=file multiple=true/>`);
    // [ts] const input = page.locator("input");
    // [ts] const events = input.evaluateHandle(e => {
    // [ts] const events = [];
    // [ts] e.addEventListener("input", () => events.push("input"));
    // [ts] e.addEventListener("change", () => events.push("change"));
    // [ts] return events;
    // [ts] });
    // [ts] input.setInputFiles(asset("file-to-upload.txt"));
    // [ts] expect_eq(input.evaluate(e => (e).files[0].name), "file-to-upload.txt");
    // [ts] expect_eq(events.evaluate(e => e), ["input", "change"]);
    // [ts] events.evaluate(e => e.length = 0);
    // [ts] input.setInputFiles(asset("pptr.png"));
    // [ts] expect_eq(input.evaluate(e => (e).files[0].name), "pptr.png");
    // [ts] expect_eq(events.evaluate(e => e), ["input", "change"]);
    // [ts] });
    // [ts] test("IF-021 should preserve lastModified timestamp", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27452" });
    // [ts] page.setContent(`<input type=file multiple=true/>`);
    // [ts] const input = page.locator("input");
    // [ts] const files = ["file-to-upload.txt", "file-to-upload-2.txt"];
    // [ts] input.setInputFiles(files.map(f => asset(f)));
    // [ts] expect_eq(input.evaluate(e => [...(e).files].map(f => f.name)), files);
    // [ts] const timestamps = input.evaluate(e => [...(e).files].map(f => f.lastModified));
    // [ts] const expectedTimestamps = files.map(file => Math.round(fs.statSync(asset(file)).mtimeMs));
    // On Linux browser sometimes reduces the timestamp by 1ms: 1696272058110.0715  -> 1696272058109 or even
    // rounds it to seconds in WebKit: 1696272058110 -> 1696272058000.
    // [ts] let i = 0;
    // [ts] while (i < timestamps.length) // [cap-migrate] for-step: i++
    // [ts] expect_eq(Math.abs(timestamps[i] - expectedTimestamps[i]), `expected: ${expectedTimestamps}; actual: ${timestamps}` <= 1000, true);
    // [ts] });
    // [ts] test("IF-022 should store an uploaded file in IndexedDB", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/42795" },
    // [ts] { type: "issue", description: "https://bugs.webkit.org/show_bug.cgi?id=156347" },
    // [ts] ],
    // [ts] }, (_p) => {
    // [ts] test.fixme(browserName == "webkit", "WebKit does not support storing files in IndexedDB in private browsing.");
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] const input = page.locator("input[type=file]");
    // [ts] input.setInputFiles(asset("file-to-upload.txt"));
    // [ts] const storedFile = input.evaluate((input) => {
    // [ts] const db = Promise.init((resolve, reject) => {
    // [ts] const request = indexedDB.open("file-upload");
    // [ts] request.onupgradeneeded = () => request.result.createObjectStore("files");
    // [ts] request.onsuccess = () => resolve(request.result);
    // [ts] request.onerror = () => reject(request.error);
    // [ts] });
    // [ts] const file = Promise.init((resolve, reject) => {
    // [ts] const transaction = db.transaction("files", "readwrite");
    // [ts] const store = transaction.objectStore("files");
    // [ts] store.put(input.files[0], "file");
    // [ts] const request = store.get("file");
    // [ts] transaction.oncomplete = () => resolve(request.result);
    // [ts] transaction.onabort = () => reject(transaction.error);
    // [ts] });
    // [ts] db.close();
    // [ts] return { name: file.name, text: file.text() };
    // [ts] });
    // [ts] expect_eq(storedFile, { name: "file-to-upload.txt", text: "contents of the file" });
    // [ts] });
    // [ts] }
    // [ts] }
}
