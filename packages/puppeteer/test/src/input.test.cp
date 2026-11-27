// Migrated from TypeScript (src/input.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import path from "cap:path";

import { test, expect_eq } from "cap:test";
import { TimeoutError } from "puppeteer";
// [cap-migrate] import {CdpPage} from "puppeteer-core/internal/cdp/Page.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html } from "./utils.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const FILE_TO_UPLOAD = path.join(
// [ts] import.meta.dirname,
// [ts] "/../assets/file-to-upload.txt",
// [ts] );

// [cap-test] describe("input tests", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("ElementHandle.uploadFile", () => {

  // [cap-test] });

  // [cap-test] describe("Page.waitForFileChooser", () => {




  // [cap-test] });

  // [cap-test] describe("FileChooser.accept", () => {
  // [cap-test] });

  // [cap-test] describe("FileChooser.cancel", () => {
  // [cap-test] });

  // [cap-test] describe("FileChooser.isMultiple", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should upload the file", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] using input = (page.$("input"));
    // [ts] input.evaluate(e => {
    // [ts] (globalThis)._inputEvents = [];
    // [ts] e.addEventListener("change", ev => {
    // [ts] return (globalThis)._inputEvents.push(ev.type);
    // [ts] });
    // [ts] e.addEventListener("input", ev => {
    // [ts] return (globalThis)._inputEvents.push(ev.type);
    // [ts] });
    // [ts] });
    // [ts] const file = path.relative(process.cwd(), FILE_TO_UPLOAD);
    // [ts] input.uploadFile(file);
    // [ts] expect(
    // [ts] input.evaluate(e => {
    // [ts] return e.files?.[0]?.name;
    // [ts] }),
    // [ts] ).toBe("file-to-upload.txt");
    // [ts] expect(
    // [ts] input.evaluate(e => {
    // [ts] return e.files?.[0]?.type;
    // [ts] }),
    // [ts] ).toBe("text/plain");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis)._inputEvents;
    // [ts] }),
    // [ts] ).toEqual(["input", "change"]);
    // [ts] });
    // [ts] test("should read the file", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] using input = (page.$("input"));
    // [ts] input.evaluate(e => {
    // [ts] (globalThis)._inputEvents = [];
    // [ts] e.addEventListener("change", ev => {
    // [ts] return (globalThis)._inputEvents.push(ev.type);
    // [ts] });
    // [ts] e.addEventListener("input", ev => {
    // [ts] return (globalThis)._inputEvents.push(ev.type);
    // [ts] });
    // [ts] });
    // [ts] const file = path.relative(process.cwd(), FILE_TO_UPLOAD);
    // [ts] input.uploadFile(file);
    // [ts] expect(
    // [ts] input.evaluate(e => {
    // [ts] const file = e.files?.[0];
    // [ts] if (!file) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('No file found');)
    // [ts] }
    // [ts] const reader = FileReader.init();
    // [ts] const promise = Promise.init(fulfill => {
    // [ts] reader.addEventListener("load", fulfill);
    // [ts] });
    // [ts] reader.readAsText(file);
    // [ts] return promise.then(() => {
    // [ts] return reader.result;
    // [ts] });
    // [ts] }),
    // [ts] ).toBe("contents of the file");
    // [ts] });
    // [ts] test("should work when file input is attached to DOM", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("should work when file input is not attached to DOM", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.evaluate(() => {
    // [ts] const el = document.createElement("input");
    // [ts] el.type = "file";
    // [ts] el.click();
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("should respect timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page.waitForFileChooser({timeout: 1}).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should respect default timeout when there is no custom timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setDefaultTimeout(1);
    // [ts] let error: Error;
    // [ts] page.waitForFileChooser().catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should prioritize exact timeout over default timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setDefaultTimeout(0);
    // [ts] let error: Error;
    // [ts] page.waitForFileChooser({timeout: 1}).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error).toBeInstanceOf(TimeoutError);
    // [ts] });
    // [ts] test("should work with no timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser({timeout: 0}),
    // [ts] page.evaluate(() => {
    // [ts] return setTimeout(() => {
    // [ts] const el = document.createElement("input");
    // [ts] el.type = "file";
    // [ts] el.click();
    // [ts] }, 50);
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(chooser, true);
    // [ts] });
    // [ts] test("should return the same file chooser when there are many watchdogs simultaneously", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [fileChooser1, fileChooser2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.waitForFileChooser(),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', input => {
    // [ts-skip] return input.click();
    // [ts-skip] }),
    // [ts] ]);
    // [ts] expect_eq(fileChooser1 == fileChooser2, true);
    // [ts] });
    // [ts] test("should reject immediately when the signal is already aborted", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.waitForFileChooser({signal: AbortSignal.abort(), timeout: 1000}),
    // [ts] ).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should be able to abort", () => {
    // [ts] const {page} = getTestState();
    // [ts] const abortController = AbortController.init();
    // [ts] const task = page.waitForFileChooser({signal: abortController.signal});
    // [ts] abortController.abort();
    // [ts] expect(task).rejects.toThrow("aborted");
    // [ts] });
    // [ts] test("should not throw unhandled rejection when file picker is opened without an input element", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(
    // [ts] html`<button
    // [ts] onclick="window.showOpenFilePicker?.().catch(() => {})"
    // [ts] ></button>`,
    // [ts] );
    // [ts] const unhandledRejections: []string = [];
    // [ts] const onUnhandledRejection = (reason) => {
    // [ts] unhandledRejections.push(reason);
    // [ts] };
    // [ts] process.on("unhandledRejection", onUnhandledRejection);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect(
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser({timeout: 200}),
    // [ts] page.click("button"),
    // [ts] ]),
    // [ts] ).rejects.toThrow(TimeoutError);
    // [ts] expect_eq(unhandledRejections, []);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] process.off("unhandledRejection", onUnhandledRejection);
    // [ts] }
    // [ts] });
    // [ts] test("should stop intercepting file choosers when no longer waiting", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const client = (page)._client();
    // [ts] let fileChooserOpenedEvents = 0;
    // [ts] const onFileChooserOpened = () => {
    // [ts] fileChooserOpenedEvents++;
    // [ts] };
    // [ts] client.on("Page.fileChooserOpened", onFileChooserOpened);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] chooser.accept([FILE_TO_UPLOAD]);
    // [ts] expect_eq(fileChooserOpenedEvents, 1);
    // Nobody is waiting, so this file chooser must not be intercepted.
    // [ts] page.click("input");
    // Events arrive in order, so an intercepted file chooser from the click
    // above would be reported before this one.
    // [ts] joinAll([page.waitForFileChooser(), page.click("input")]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(fileChooserOpenedEvents, 2);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] client.off("Page.fileChooserOpened", onFileChooserOpened);
    // [ts] }
    // [ts] });
    // [ts] test("should stop intercepting file choosers after a timeout", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const client = (page)._client();
    // [ts] let fileChooserOpenedEvents = 0;
    // [ts] const onFileChooserOpened = () => {
    // [ts] fileChooserOpenedEvents++;
    // [ts] };
    // [ts] client.on("Page.fileChooserOpened", onFileChooserOpened);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect(page.waitForFileChooser({timeout: 1})).rejects.toThrow(
    // [ts] TimeoutError,
    // [ts] );
    // Nobody is waiting, so this file chooser must not be intercepted.
    // [ts] page.click("input");
    // Events arrive in order, so an intercepted file chooser from the click
    // above would be reported before this one.
    // [ts] joinAll([page.waitForFileChooser(), page.click("input")]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(fileChooserOpenedEvents, 1);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] client.off("Page.fileChooserOpened", onFileChooserOpened);
    // [ts] }
    // [ts] });
    // [ts] test("should accept single file", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] type="file"
    // [ts] oninput="javascript:console.timeStamp()"
    // [ts] />`,
    // [ts] );
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] joinAll([chooser.accept([FILE_TO_UPLOAD])]); // migrated: Promise.all -> joinAll
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
    // [ts-skip] return input.files!.length;
    // [ts-skip] }),
    // [ts] ).toBe(1);
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
    // [ts-skip] return input.files![0]!.name;
    // [ts-skip] }),
    // [ts] ).toBe("file-to-upload.txt");
    // [ts] });
    // [ts] test("should be able to read selected file", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] void page.waitForFileChooser().then(chooser => {
    // [ts] return chooser.accept([FILE_TO_UPLOAD]);
    // [ts] });
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', async pick => {
    // [ts-skip] pick.click();
    // [ts-skip] await new Promise(x => {
    // [ts-skip] return (pick.oninput = x);
    // [ts-skip] });
    // [ts-skip] const reader = new FileReader();
    // [ts-skip] const promise = new Promise(fulfill => {
    // [ts-skip] return (reader.onload = fulfill);
    // [ts-skip] });
    // [ts-skip] reader.readAsText(pick.files![0]!);
    // [ts-skip] return await promise.then(() => {
    // [ts-skip] return reader.result;
    // [ts-skip] });
    // [ts-skip] }),
    // [ts] ).toBe("contents of the file");
    // [ts] });
    // [ts] test("should be able to reset selected files with empty file list", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] void page.waitForFileChooser().then(chooser => {
    // [ts] return chooser.accept([FILE_TO_UPLOAD]);
    // [ts] });
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', async pick => {
    // [ts-skip] pick.click();
    // [ts-skip] await new Promise(x => {
    // [ts-skip] return (pick.oninput = x);
    // [ts-skip] });
    // [ts-skip] return pick.files!.length;
    // [ts-skip] }),
    // [ts] ).toBe(1);
    // [ts] void page.waitForFileChooser().then(chooser => {
    // [ts] return chooser.accept([]);
    // [ts] });
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', async pick => {
    // [ts-skip] pick.click();
    // [ts-skip] await new Promise(x => {
    // [ts-skip] return (pick.oninput = x);
    // [ts-skip] });
    // [ts-skip] return pick.files!.length;
    // [ts-skip] }),
    // [ts] ).toBe(0);
    // [ts] });
    // [ts] test("should not accept multiple files for single-file input", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] let error: Error;
    // [ts] chooser
    // [ts] .accept([
    // [ts] path.relative(
    // [ts] process.cwd(),
    // [ts] import.meta.dirname + "/../assets/file-to-upload.txt",
    // [ts] ),
    // [ts] path.relative(
    // [ts] process.cwd(),
    // [ts] import.meta.dirname + "/../assets/pptr.png",
    // [ts] ),
    // [ts] ])
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] });
    // [ts] test("should succeed even for non-existent files", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] let error: Error;
    // [ts] chooser.accept(["file-does-not-exist.txt"]).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error == null, true);
    // [ts] });
    // [ts] test("should error on read of non-existent files", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] void page.waitForFileChooser().then(chooser => {
    // [ts] return chooser.accept(["file-does-not-exist.txt"]);
    // [ts] });
    // [ts] expect(
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', async pick => {
    // [ts-skip] pick.click();
    // [ts-skip] await new Promise(x => {
    // [ts-skip] return (pick.oninput = x);
    // [ts-skip] });
    // [ts-skip] const reader = new FileReader();
    // [ts-skip] const promise = new Promise(fulfill => {
    // [ts-skip] return (reader.onerror = fulfill);
    // [ts-skip] });
    // [ts-skip] reader.readAsText(pick.files![0]!);
    // [ts-skip] return await promise.then(() => {
    // [ts-skip] return false;
    // [ts-skip] });
    // [ts-skip] }),
    // [ts] ).toBeFalsy();
    // [ts] });
    // [ts] test("should fail when accepting file chooser twice", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [fileChooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', input => {
    // [ts-skip] return input.click();
    // [ts-skip] }),
    // [ts] ]);
    // [ts] fileChooser.accept([]);
    // [ts] let error: Error;
    // [ts] fileChooser.accept([]).catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toBe(
    // [ts] "Cannot accept FileChooser which is already handled!",
    // [ts] );
    // [ts] });
    // [ts] test("should cancel dialog", () => {
    // [ts] const {page} = getTestState();
    // Consider file chooser canceled if we can summon another one.
    // There's no reliable way in WebPlatform to see that FileChooser was
    // canceled.
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [fileChooser1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', input => {
    // [ts-skip] return input.click();
    // [ts-skip] }),
    // [ts] ]);
    // [ts] fileChooser1.cancel();
    // If this resolves, than we successfully canceled file chooser.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', input => {
    // [ts-skip] return input.click();
    // [ts-skip] }),
    // [ts] ]);
    // [ts] });
    // [ts] test("should fail when canceling file chooser twice", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [fileChooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('input', input => {
    // [ts-skip] return input.click();
    // [ts-skip] }),
    // [ts] ]);
    // [ts] fileChooser.cancel();
    // [ts] let error: Error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fileChooser.cancel();
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error_) {)
    // [ts] error = error_;
    // [ts] }
    // [ts] expect(error.message).toBe(
    // [ts] "Cannot cancel FileChooser which is already handled!",
    // [ts] );
    // [ts] });
    // [ts] test("should work for single file pick", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<input type="file" />`);
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser.isMultiple(), false);
    // [ts] });
    // [ts] test("should work for \"multiple\"", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] multiple
    // [ts] type="file"
    // [ts] />`,
    // [ts] );
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser.isMultiple(), true);
    // [ts] });
    // [ts] test("should work for \"webkitdirectory\"", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] multiple
    // [ts] webkitdirectory
    // [ts] type="file"
    // [ts] />`,
    // [ts] );
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForFileChooser(),
    // [ts] page.click("input"),
    // [ts] ]);
    // [ts] expect_eq(chooser.isMultiple(), true);
    // [ts] });
}
