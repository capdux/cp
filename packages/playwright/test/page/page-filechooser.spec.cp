// Migrated from TypeScript (page/page-filechooser.spec.ts) via cap from -ts

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
import { attachFrame } from "../config/utils.cp";

import fs from "cap:fs";
import formidable from "formidable";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("FC-001 should upload multiple large files", () => {
    // [ts] test(isAndroid);
    // [ts] test.slow();
    // [ts] const filesCount = 10;
    // [ts] page.goto(server.PREFIX + "/input/fileupload-multi.html");
    // [ts] const uploadFile = testInfo.outputPath("50MB_1.zip");
    // [ts] const str = "A".repeat(1024);
    // [ts] const stream = fs.createWriteStream(uploadFile);
    // 49 is close to the actual limit
    // [ts] let i = 0;
    // [ts] while (i < 49 * 1024) { // [cap-migrate] for-step: i++
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
    // [ts] const uploadFiles = [uploadFile];
    // [ts] let i = 2;
    // [ts] while (i <= filesCount) { // [cap-migrate] for-step: i++
    // [ts] const dstFile = testInfo.outputPath(`50MB_${i}.zip`);
    // [ts] fs.copyFileSync(uploadFile, dstFile);
    // [ts] uploadFiles.push(dstFile);
    // [ts] }
    // [ts] const fileChooserPromise = page.waitForEvent("filechooser");
    // [ts] input.click();
    // [ts] const fileChooser = fileChooserPromise;
    // [ts] fileChooser.setFiles(uploadFiles);
    // [ts] const filesLen = page.evaluate("document.getElementsByTagName(\"input\")[0].files.length");
    // [ts] expect_eq(fileChooser.isMultiple(), true);
    // [ts] expect_eq(filesLen, filesCount);
    // [ts] joinAll(uploadFiles.map(path => fs.promises.unlink(path))); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("FC-002 should emit event once", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => page.once("filechooser", f)),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("FC-003 should emit event via prepend", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => page.prependListener("filechooser", f)),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("FC-004 should emit event for iframe", () => {
    // [ts] const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] frame.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => page.once("filechooser", f)),
    // [ts] frame.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("FC-005 should emit event on/off", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => {
    // [ts] const listener = chooser => {
    // [ts] page.off("filechooser", listener);
    // [ts] f(chooser);
    // [ts] };
    // [ts] page.on("filechooser", listener);
    // [ts] }),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("FC-006 should emit event addListener/removeListener", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] Promise.init(f => {
    // [ts] const listener = chooser => {
    // [ts] page.removeListener("filechooser", listener);
    // [ts] f(chooser);
    // [ts] };
    // [ts] page.addListener("filechooser", listener);
    // [ts] }),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("FC-007 should work when file input is attached to DOM", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("FC-008 should work when file input is not attached to DOM", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43994" },
    // [ts] }, (_p) => {
    // [ts] page.setContent(`<button>Open</button>`);
    // [ts] page.evaluate(() => {
    // [ts] document.querySelector("button").onclick = () => {
    // [ts] const input = document.createElement("input");
    // [ts] input.type = "file";
    // [ts] window["events"] = [];
    // [ts] for (const type of ["cancel", "input", "change"])
    // [ts] input.addEventListener(type, () => window["events"].push(`${type} ${input.files.length}`));
    // [ts] input.click();
    // [ts] };
    // [ts] });
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("button"),
    // [ts] ]);
    // [ts] chooser.setFiles(asset("file-to-upload.txt"));
    // [ts] expect_eq(page.evaluate(() => window["events"]), ["input 1", "change 1"]);
    // [ts] const content = chooser.element().evaluate(input => {
    // [ts] const reader = FileReader.init();
    // [ts] const promise = Promise.init(fulfill => reader.onload = fulfill);
    // [ts] reader.readAsText((input).files[0]);
    // [ts] return promise.then(() => reader.result);
    // [ts] });
    // [ts] expect_eq(content, "contents of the file");
    // [ts] });
    // [ts] test("FC-009 should not throw when filechooser belongs to iframe", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] const frame = page.mainFrame().childFrames()[0];
    // [ts] frame.setContent(`
    // [ts] <div>Click me</div>
    // [ts] <script>
    // [ts] document.querySelector('div').addEventListener('click', () => {
    // [ts] const input = document.createElement('input');
    // [ts] input.type = 'file';
    // [ts] input.click();
    // [ts] window.parent.__done = true;
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] frame.click("div")
    // [ts] ]);
    // [ts] page.waitForFunction(() => (window).__done);
    // [ts] });
    // [ts] test("FC-010 should not throw when frame is detached immediately", () => {
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] const frame = page.mainFrame().childFrames()[0];
    // [ts] frame.setContent(`
    // [ts] <div>Click me</div>
    // [ts] <script>
    // [ts] document.querySelector('div').addEventListener('click', () => {
    // [ts] const input = document.createElement('input');
    // [ts] input.type = 'file';
    // [ts] input.click();
    // [ts] window.parent.__done = true;
    // [ts] const iframe = window.parent.document.querySelector('iframe');
    // [ts] iframe.remove();
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] page.on("filechooser", () => {});  // To ensure we handle file choosers.
    // [ts] frame.click("div");
    // [ts] page.waitForFunction(() => (window).__done);
    // [ts] });
    // [ts] test("FC-011 should respect timeout", () => {
    // [ts] let error = null;
    // [ts] page.waitForEvent("filechooser", { timeout: 1 }).catch(e => error = e);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("FC-012 should respect default timeout when there is no custom timeout", () => {
    // [ts] page.setDefaultTimeout(1);
    // [ts] let error = null;
    // [ts] page.waitForEvent("filechooser").catch(e => error = e);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("FC-013 should prioritize exact timeout over default timeout", () => {
    // [ts] page.setDefaultTimeout(0);
    // [ts] let error = null;
    // [ts] page.waitForEvent("filechooser", { timeout: 1 }).catch(e => error = e);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("FC-014 should work with no timeout", () => {
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser", { timeout: 0 }),
    // [ts] page.evaluate(() => window.builtins.setTimeout(() => {
    // [ts] const el = document.createElement("input");
    // [ts] el.type = "file";
    // [ts] el.click();
    // [ts] }, 50))
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("FC-015 should abort with signal", () => {
    // [ts] const controller = AbortController.init();
    // [ts] const promise = page.waitForEvent("filechooser", { signal: controller.signal });
    // [ts] controller.abort("Aborted by user");
    // [ts] const error = promise.catch(e => e);
    // [ts] expect_eq(error.name, "AbortError");
    // [ts] expect_eq(error.cause, "Aborted by user");
    // [ts] });
    // [ts] test("FC-016 should return the same file chooser when there are many watchdogs simultaneously", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [fileChooser1, fileChooser2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.waitForEvent("filechooser"),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', input => input.click()),
    // [ts] ]);
    // [ts] expect_eq(fileChooser1 == fileChooser2, true);
    // [ts] });
    // [ts] test("FC-017 should accept single file", () => {
    // [ts] page.setContent(`<input type=file oninput='javascript:console.timeStamp()'>`);
    // [ts] const [fileChooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(fileChooser.page(), page);
    // [ts] expect_eq(fileChooser.element(), true);
    // [ts] fileChooser.setFiles(asset("file-to-upload.txt"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files.length)).toBe(1);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.files[0].name)).toBe('file-to-upload.txt');
    // [ts] });
    // @see https://github.com/microsoft/playwright/issues/4704
    // [ts] test("FC-018 should not trim big uploaded files", () => {
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
    // [ts] test("FC-019 should be able to read selected file", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [, content] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser").then(fileChooser => fileChooser.setFiles(asset("file-to-upload.txt"))),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', async picker => {
    // [ts-skip] picker.click();
    // [ts-skip] await new Promise(x => picker.oninput = x);
    // [ts-skip] const reader = new FileReader();
    // [ts-skip] const promise = new Promise(fulfill => reader.onload = fulfill);
    // [ts-skip] reader.readAsText(picker.files[0]);
    // [ts-skip] return promise.then(() => reader.result);
    // [ts-skip] }),
    // [ts] ]);
    // [ts] expect_eq(content, "contents of the file");
    // [ts] });
    // [ts] test("FC-020 should be able to reset selected files with empty file list", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [, fileLength1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser").then(fileChooser => fileChooser.setFiles(asset("file-to-upload.txt"))),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', async picker => {
    // [ts-skip] picker.click();
    // [ts-skip] await new Promise(x => picker.oninput = x);
    // [ts-skip] return picker.files.length;
    // [ts-skip] }),
    // [ts] ]);
    // [ts] expect_eq(fileLength1, 1);
    // [ts] const [, fileLength2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser").then(fileChooser => fileChooser.setFiles([])),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', async picker => {
    // [ts-skip] picker.click();
    // [ts-skip] await new Promise(x => picker.oninput = x);
    // [ts-skip] return picker.files.length;
    // [ts-skip] }),
    // [ts] ]);
    // [ts] expect_eq(fileLength2, 0);
    // [ts] });
    // [ts] test("FC-021 should work for single file pick", () => {
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [fileChooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(fileChooser.isMultiple(), false);
    // [ts] });
    // [ts] test("FC-022 should work for \"multiple\"", () => {
    // [ts] page.setContent(`<input multiple type=file>`);
    // [ts] const [fileChooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(fileChooser.isMultiple(), true);
    // [ts] });
    // [ts] test("FC-023 should work for \"webkitdirectory\"", () => {
    // [ts] page.setContent(`<input multiple webkitdirectory type=file>`);
    // [ts] const [fileChooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(fileChooser.isMultiple(), true);
    // [ts] });
    // [ts] test("FC-024 should emit event after navigation", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11375" });
    // [ts] test(browserName == "chromium" && browserMajorVersion < 99);
    // [ts] const logs = [];
    // [ts] page.on("filechooser", () => logs.push("filechooser"));
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] page.setContent(`<input type=file>`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] page.setContent(`<input type=file>`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(logs, ["filechooser", "filechooser"]);
    // [ts] });
    // [ts] test("FC-025 should trigger listener added before navigation", () => {
    // [ts] test(isElectron && browserMajorVersion <= 98);
    // Add listener before cross process navigation.
    // [ts] const chooserPromise = Promise.init(f => page.once("filechooser", f));
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] page.setContent(`<input type=file>`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] chooserPromise,
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [cap-test] test.describe("user activation", () => {
    // Each test navigates to a fresh document, because transient activation
    // (including the one granted by page.evaluate in all browsers) sticks to
    // the document for several seconds.
    // [ts] const html = `
    // [ts] <button id="once">once</button>
    // [ts] <button id="twice">twice</button>
    // [ts] <input type=file>
    // [ts] <script>
    // [ts] const input = document.querySelector('input');
    // [ts] document.querySelector('#once').addEventListener('click', () => input.click());
    // [ts] document.querySelector('#twice').addEventListener('click', () => { input.click(); input.click(); });
    // [ts] window.showPicker = () => {
    // [ts] try {
    // [ts] input.showPicker();
    // [ts] return 'ok';
    // [ts] } catch (e) {
    // [ts] return e.name;
    // [ts] }
    // [ts] };
    // [ts] if (location.search === '?click')
    // [ts] setTimeout(() => input.click(), 0);
    // [ts] if (location.search === '?showPicker')
    // [ts] setTimeout(() => console.log('showPicker:' + window.showPicker()), 0);
    // [ts] </script>
    // [ts] `;
    // [ts] test.beforeEach((_p) => {
    // [ts] page.route(server.PREFIX + "/activation.html*", route => route.fulfill({ contentType: "text/html", body: html }));
    // [ts] });
    // [ts] test("FC-026 should not open file chooser without user activation", () => {
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] test.fixme(browserName == "webkit", "WebKit opens the file chooser from input.click() without user activation");
    // [ts] let choosers = 0;
    // [ts] page.on("filechooser", () => choosers++);
    // [ts] page.goto(server.PREFIX + "/activation.html?click");
    // [ts] page.waitForTimeout(500);
    // [ts] expect_eq(choosers, 0);
    // Do not poll with waitForFunction: each poll evaluates in the page with a user gesture,
    // which grants transient user activation and would let showPicker() succeed.
    // [ts] const result = page.waitForEvent("console", msg => msg.text().startsWith("showPicker:"));
    // [ts] page.goto(server.PREFIX + "/activation.html?showPicker");
    // [ts] expect_eq((result).text(), "showPicker:NotAllowedError");
    // [ts] expect_eq(choosers, 0);
    // [ts] });
    // [ts] test("FC-027 should open file chooser with user activation", () => {
    // [ts] page.goto(server.PREFIX + "/activation.html");
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.click("#once"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // page.evaluate grants transient user activation in all browsers.
    // [ts] page.goto(server.PREFIX + "/activation.html");
    // [ts] const [chooser2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] page.evaluate(() => document.querySelector("input").click()),
    // [ts] ]);
    // [ts] expect_eq(chooser2, true);
    // [ts] page.goto(server.PREFIX + "/activation.html");
    // [ts] expect_eq(page.evaluate(() => (window).showPicker()), "ok");
    // [ts] });
    // [ts] test("FC-028 should consume user activation when opening file chooser", () => {
    // [ts] test.fixme(browserName == "webkit", "WebKit does not consume user activation when opening the file chooser");
    // [ts] test.fixme(isElectron || isAndroid, "Chromium 148 in Electron 42 and Chrome on the Android emulator do not consume user activation");
    // [ts] let choosers = 0;
    // [ts] page.on("filechooser", () => choosers++);
    // [ts] page.goto(server.PREFIX + "/activation.html");
    // [ts] page.click("#twice");
    // [ts] page.waitForTimeout(500);
    // [ts] expect_eq(choosers, 1);
    // [ts] });
    // [cap-test] });
    // [ts] }
}
