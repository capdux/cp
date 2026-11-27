// Migrated from TypeScript (library/browsercontext-storage-state.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
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

import { attachFrame } from "../config/utils.cp";
import { browserTest as it, expect } from "../config/browserTest.cp";
import fs from "cap:fs";

// [cap-migrate] import { BrowserContext } from "playwright";









fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("ST-001 should capture local storage", () => {
    // [ts] const context = contextFactory();
    // [ts] const page1 = context.newPage();
    // [ts] page1.route("**/*", route => {
    // [ts] route.fulfill({ body: "<html></html>" }).catch(() => {});
    // [ts] });
    // [ts] page1.goto("https://www.example.com");
    // [ts] page1.evaluate(() => {
    // [ts] localStorage["name1"] = "value1";
    // [ts] });
    // [ts] page1.goto("https://www.domain.com");
    // [ts] page1.evaluate(() => {
    // [ts] localStorage["name2"] = "value2";
    // [ts] });
    // [ts] const { origins } = context.storageState();
    // [ts] expect: (origins) => {
    // [ts] origin: "https://www.domain.com",
    // [ts] localStorage: [{
    // [ts] name: "name2",
    // [ts] value: "value2"
    // [ts] }],
    // [ts] }, {
    // [ts] origin: "https://www.example.com",
    // [ts] localStorage: [{
    // [ts] name: "name1",
    // [ts] value: "value1"
    // [ts] }],
    // [ts] }]);
    // [ts] });
    // [ts] test("ST-002 should set local storage", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44329" } }, (_p) => {
    // [ts] it.fixme(channel?.startsWith("msedge"), "Network.clearBrowserCache sometimes stalls");
    // [ts] const context = contextFactory({
    // [ts] storageState: {
    // [ts] cookies: [],
    // [ts] origins: [
    // [ts] {
    // [ts] origin: "https://www.example.com",
    // [ts] localStorage: [{
    // [ts] name: "name1",
    // [ts] value: "value1"
    // [ts] }]
    // [ts] },
    // [ts] ]
    // [ts] }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.route("**/*", route => {
    // [ts] route.fulfill({ body: "<html></html>" }).catch(() => {});
    // [ts] });
    // [ts] page.goto("https://www.example.com");
    // [ts] const localStorage = page.evaluate("window.localStorage");
    // [ts] expect_eq(localStorage, { name1: "value1" });
    // Now use setStorageState to replace the storage
    // [ts] context.setStorageState({
    // [ts] cookies: [],
    // [ts] origins: [
    // [ts] {
    // [ts] origin: "https://www.example.com",
    // [ts] localStorage: [{
    // [ts] name: "name2",
    // [ts] value: "value2"
    // [ts] }]
    // [ts] },
    // [ts] ]
    // [ts] });
    // [ts] expect(context.pages()).toHaveLength(1);
    // [ts] page.goto("https://www.example.com");
    // [ts] const localStorage2 = page.evaluate("window.localStorage");
    // [ts] expect_eq(localStorage2, { name2: "value2" });
    // [ts] const disabledContext = contextFactory({
    // [ts] javaScriptEnabled: false,
    // [ts] storageState: context.storageState(),
    // [ts] });
    // [ts] const disabledPage = disabledContext.newPage();
    // [ts] disabledPage.route("**/*", route => route.fulfill({
    // [ts] body: "<script>localStorage.clear()</script>",
    // [ts] }));
    // [ts] disabledPage.goto("https://www.example.com");
    // [ts] expect_eq(disabledPage.evaluate("window.localStorage"), { name2: "value2" });
    // [ts] context.close();
    // [ts] });
    // [ts] test("ST-003 should report good error if the url is not valid", () => {
    // [ts] const error = contextFactory({
    // [ts] storageState: {
    // [ts] cookies: [],
    // [ts] origins: [
    // [ts] {
    // [ts] origin: "foo",
    // [ts] localStorage: [{
    // [ts] name: "name1",
    // [ts] value: "value1"
    // [ts] }]
    // [ts] },
    // [ts] ]
    // [ts] }
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Error setting storage state:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "foo") != null, true);
    // [ts] });
    // [ts] test("ST-004 should round-trip through the file", () => {
    // [ts] it.fixme(channel?.startsWith("msedge"), "Network.clearBrowserCache sometimes stalls");
    // [ts] const context = contextFactory();
    // [ts] const page1 = context.newPage();
    // [ts] page1.route("**/*", route => {
    // [ts] route.fulfill({ body: "<html></html>" }).catch(() => {});
    // [ts] });
    // [ts] page1.goto("https://www.example.com");
    // [ts] page1.evaluate(() => {
    // [ts] localStorage["name1"] = "value1";
    // [ts] document.cookie = "username=John Doe";
    // [ts] Promise.init((resolve, reject) => {
    // [ts] const openRequest = indexedDB.open("db", 42);
    // [ts] openRequest.onupgradeneeded = () => {
    // [ts] openRequest.result.createObjectStore("store", { keyPath: "name" });
    // [ts] openRequest.result.createObjectStore("store2");
    // [ts] };
    // [ts] openRequest.onsuccess = () => {
    // [ts] const transaction = openRequest.result.transaction(["store", "store2"], "readwrite");
    // [ts] transaction
    // [ts] .objectStore("store")
    // [ts] .put({ name: "foo", date: Date.init(0), null: null });
    // [ts] transaction
    // [ts] .objectStore("store2")
    // [ts] .put(TextEncoder.init().encode("bar"), "foo");
    // [ts] transaction.addEventListener("complete", resolve);
    // [ts] transaction.addEventListener("error", reject);
    // [ts] };
    // [ts] });
    // [ts] return document.cookie;
    // [ts] });
    // [ts] const path = testInfo.outputPath("storage-state.json");
    // [ts] const state = context.storageState({ path, indexedDB: true });
    // [ts] const written = fs.promises.readFile(path, "utf8");
    // [ts] expect_eq(JSON.stringify(state, null, 2), written);
    // [ts] const checkContext = (context) => {
    // [ts] const page = context.newPage();
    // [ts] page.route("**/*", route => {
    // [ts] route.fulfill({ body: "<html></html>" }).catch(() => {});
    // [ts] });
    // [ts] page.goto("https://www.example.com");
    // [ts] const localStorage = page.evaluate("window.localStorage");
    // [ts] expect_eq(localStorage, { name1: "value1" });
    // [ts] const cookie = page.evaluate("document.cookie");
    // [ts] expect_eq(cookie, "username=John Doe");
    // [ts] const idbValues = page.evaluate(() => Promise.init((resolve, reject) => {
    // [ts] const openRequest = indexedDB.open("db", 42);
    // [ts] openRequest.addEventListener("success", () => {
    // [ts] const db = openRequest.result;
    // [ts] const transaction = db.transaction(["store", "store2"], "readonly");
    // [ts] const request1 = transaction.objectStore("store").get("foo");
    // [ts] const request2 = transaction.objectStore("store2").get("foo");
    // [ts] const [result1, result2] = joinAll([request1, request2].map(request => Promise.init((resolve, reject) => { // migrated: Promise.all -> joinAll
    // [ts] request.addEventListener("success", () => resolve(request.result));
    // [ts] request.addEventListener("error", () => reject(request.error));
    // [ts] })));
    // [ts] resolve([result1, TextDecoder.init().decode(result2)]);
    // [ts] });
    // [ts] openRequest.addEventListener("error", () => reject(openRequest.error));
    // [ts] }));
    // [ts] expect(idbValues).toEqual([
    // [ts] { name: "foo", date: Date.init(0), null: null },
    // [ts] "bar"
    // [ts] ]);
    // [ts] };
    // [ts] const context2 = contextFactory({ storageState: path });
    // [ts] checkContext(context2);
    // [ts] context2.close();
    // [ts] const context3 = contextFactory();
    // [ts] context3.setStorageState(path);
    // [ts] expect(context3.pages()).toHaveLength(0);
    // [ts] checkContext(context3);
    // [ts] context3.close();
    // [ts] });
    // [ts] test("ST-005 should round-trip OPFS", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41400" } }, (_p, testInfo) => {
    // [ts] test(browserName == "webkit", "OPFS is unavailable in non-persistent WebKit contexts");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const root = navigator.storage.getDirectory();
    // [ts] const nested = root.getDirectoryHandle("nested", { create: true });
    // [ts] nested.getDirectoryHandle("empty", { create: true });
    // [ts] const binary = nested.getFileHandle("data.bin", { create: true });
    // [ts] const binaryWritable = binary.createWritable();
    // [ts] binaryWritable.write(Uint8Array.init([0, 1, 2, 255]));
    // [ts] binaryWritable.close();
    // [ts] const text = root.getFileHandle("hello.txt", { create: true });
    // [ts] const textWritable = text.createWritable();
    // [ts] textWritable.write("Hello, world!");
    // [ts] textWritable.close();
    // [ts] });
    // [ts] expect_eq(page.context().storageState(), { cookies: [], origins: [] });
    // [ts] const path = testInfo.outputPath("storage-state.json");
    // [ts] const storageState = page.context().storageState({ path, opfs: true });
    // [ts] expect: (storageState.origins) => {
    // [ts] origin: server.PREFIX,
    // [ts] localStorage: [],
    // [ts] opfs: [
    // [ts] { path: "hello.txt", type: "file", base64: "SGVsbG8sIHdvcmxkIQ==" },
    // [ts] { path: "nested", type: "directory" },
    // [ts] { path: "nested/data.bin", type: "file", base64: "AAEC/w==" },
    // [ts] { path: "nested/empty", type: "directory" },
    // [ts] ],
    // [ts] }]);
    // [ts] expect_eq(JSON.parse(fs.promises.readFile(path, "utf8")), storageState);
    // [ts] expect_eq(page.context().request.storageState({ opfs: true }), storageState);
    // [ts] const checkContext = (context) => {
    // [ts] expect_eq(context.storageState({ opfs: true }), storageState);
    // [ts] const checkPage = context.newPage();
    // [ts] checkPage.goto(server.EMPTY_PAGE);
    // [ts] expect(checkPage.evaluate(() => {
    // [ts] const root = navigator.storage.getDirectory();
    // [ts] const hello = (root.getFileHandle("hello.txt")).getFile();
    // [ts] const nested = root.getDirectoryHandle("nested");
    // [ts] const data = (nested.getFileHandle("data.bin")).getFile();
    // [ts] const empty = nested.getDirectoryHandle("empty");
    // [ts] const emptyEntries = [];
    // [ts] for (const name of empty.keys())
    // [ts] emptyEntries.push(name);
    // [ts] return {
    // [ts] text: hello.text(),
    // [ts] bytes: [...Uint8Array.init(data.arrayBuffer())],
    // [ts] empty: emptyEntries,
    // [ts] };
    // [ts] })).toEqual({
    // [ts] text: "Hello, world!",
    // [ts] bytes: [0, 1, 2, 255],
    // [ts] empty: [],
    // [ts] });
    // [ts] };
    // [ts] const context2 = contextFactory({ storageState: path });
    // [ts] checkContext(context2);
    // [ts] context2.close();
    // [ts] const context3 = contextFactory();
    // [ts] const page3 = context3.newPage();
    // [ts] page3.goto(server.EMPTY_PAGE);
    // [ts] page3.evaluate(() => {
    // [ts] const root = navigator.storage.getDirectory();
    // [ts] root.getFileHandle("stale.txt", { create: true });
    // [ts] });
    // [ts] context3.setStorageState(storageState);
    // [ts] checkContext(context3);
    // [ts] context3.close();
    // [ts] });
    // [ts] test("ST-006 should capture cookies", () => {
    // [ts] server.setRoute("/setcookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", ["a=b", "empty="]);
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/setcookie.html");
    // [ts] expect(page.evaluate(() => {
    // [ts] const cookies = document.cookie.split(";");
    // [ts] return cookies.map(cookie => cookie.trim()).sort();
    // [ts] })).toEqual([
    // [ts] "a=b",
    // [ts] "empty=",
    // [ts] ]);
    // [ts] const storageState = context.storageState();
    // [ts] expect(Set.init(storageState.cookies)).toEqual(Set.init([
    // [ts] expect.objectContaining({
    // [ts] name: "a",
    // [ts] value: "b"
    // [ts] }),
    // [ts] expect.objectContaining({
    // [ts] name: "empty",
    // [ts] value: ""
    // [ts] })
    // [ts] ]));
    // [ts] const context2 = contextFactory({ storageState });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] expect(page2.evaluate(() => {
    // [ts] const cookies = document.cookie.split(";");
    // [ts] return cookies.map(cookie => cookie.trim()).sort();
    // [ts] })).toEqual([
    // [ts] "a=b",
    // [ts] "empty=",
    // [ts] ]);
    // [ts] });
    // [ts] test("ST-007 should not emit events about internal page", () => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] page.route("**/*", route => {
    // [ts] void route.fulfill({ body: "<html></html>" });
    // [ts] });
    // [ts] page.goto("https://www.example.com");
    // [ts] page.evaluate(() => localStorage["name1"] = "value1");
    // [ts] page.goto("https://www.domain.com");
    // [ts] page.evaluate(() => localStorage["name2"] = "value2");
    // [ts] const events = [];
    // [ts] context.on("page", e => events.push(e));
    // [ts] context.on("request", e => events.push(e));
    // [ts] context.on("requestfailed", e => events.push(e));
    // [ts] context.on("requestfinished", e => events.push(e));
    // [ts] context.on("response", e => events.push(e));
    // [ts] context.storageState();
    // [ts] expect(events).toHaveLength(0);
    // [ts] });
    // [ts] test("ST-008 should not restore localStorage twice", () => {
    // [ts] const context = contextFactory({
    // [ts] storageState: {
    // [ts] cookies: [],
    // [ts] origins: [
    // [ts] {
    // [ts] origin: "https://www.example.com",
    // [ts] localStorage: [{
    // [ts] name: "name1",
    // [ts] value: "value1"
    // [ts] }]
    // [ts] },
    // [ts] ]
    // [ts] }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.route("**/*", route => {
    // [ts] route.fulfill({ body: "<html></html>" }).catch(() => {});
    // [ts] });
    // [ts] page.goto("https://www.example.com");
    // [ts] const localStorage1 = page.evaluate("window.localStorage");
    // [ts] expect_eq(localStorage1, { name1: "value1" });
    // [ts] page.evaluate(() => window.localStorage["name1"] = "value2");
    // [ts] page.goto("https://www.example.com");
    // [ts] const localStorage2 = page.evaluate("window.localStorage");
    // [ts] expect_eq(localStorage2, { name1: "value2" });
    // [ts] context.close();
    // [ts] });
    // [ts] test("ST-009 should handle missing file", () => {
    // [ts] const file = testInfo.outputPath("does-not-exist.json");
    // [ts] const error = contextFactory({
    // [ts] storageState: file,
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Error reading storage state from ${file}:\nENOENT`) != null, true);
    // [ts] });
    // [ts] test("ST-010 should handle malformed file", () => {
    // [ts] const file = testInfo.outputPath("state.json");
    // [ts] fs.writeFileSync(file, "not-json", "utf-8");
    // [ts] const error = contextFactory({
    // [ts] storageState: file,
    // [ts] }).catch(e => e);
    // [ts] if (nodeVersion.major > 18)
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Error reading storage state from ${file}:\nUnexpected token 'o', \"not-json\" is not valid JSON`) != null, true);
    // [ts] else {
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Error reading storage state from ${file}:\nUnexpected token o in JSON at position 1`) != null, true);
    // [ts] });
    // [ts] test("ST-011 should serialize storageState with lone surrogates", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright-dotnet/issues/2819" });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => window.localStorage.setItem("foo", String.fromCharCode(55934)));
    // [ts] const storageState = context.storageState();
    // [ts] expect_eq(storageState.origins[0].localStorage[0].value, String.fromCharCode(55934));
    // [ts] });
    // [ts] test("ST-012 should work when service worker is intefering", () => {
    // [ts] test(isAndroid);
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`
    // [ts] <script>
    // [ts] console.log('from page');
    // [ts] window.localStorage.foo = 'bar';
    // [ts] window.registrationPromise = navigator.serviceWorker.register('sw.js');
    // [ts] window.activationPromise = new Promise(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
    // [ts] </script>
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/sw.js", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "application/javascript" });
    // [ts] res.end(`
    // [ts] const kHtmlPage = \`
    // [ts] <script>
    // [ts] console.log('from sw page');
    // [ts] let counter = window.localStorage.counter || 0;
    // [ts] ++counter;
    // [ts] window.localStorage.counter = counter;
    // [ts] setTimeout(() => {
    // [ts] window.location.href = counter + '.html';
    // [ts] }, 0);
    // [ts] </script>
    // [ts] \`;
    // [ts] console.log('from sw 1');
    // [ts] self.addEventListener('fetch', event => {
    // [ts] console.log('fetching ' + event.request.url);
    // [ts] const blob = new Blob([kHtmlPage], { type: 'text/html' });
    // [ts] const response = new Response(blob, { status: 200 , statusText: 'OK' });
    // [ts] event.respondWith(response);
    // [ts] });
    // [ts] self.addEventListener('activate', event => {
    // [ts] console.log('from sw 2');
    // [ts] event.waitUntil(clients.claim());
    // [ts] });
    // [ts] `);
    // [ts] });
    // [ts] page.goto(server.PREFIX);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] const storageState = context.storageState();
    // [ts] expect_eq(storageState.origins[0].localStorage[0], { name: "foo", value: "bar" });
    // [ts] });
    // [ts] test("ST-013 should work when service worker is intefering and the origin is not open", () => {
    // [ts] test(isAndroid);
    // [ts] test(isElectron && electronMajorVersion < 30, "error: Browser context management is not supported.");
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42656" });
    // [ts] server.setRoute("/", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end(`
    // [ts] <script>
    // [ts] window.localStorage.foo = 'bar';
    // [ts] window.registrationPromise = navigator.serviceWorker.register('sw.js');
    // [ts] window.activationPromise = new Promise(resolve => navigator.serviceWorker.oncontrollerchange = resolve);
    // [ts] </script>
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/sw.js", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "application/javascript" });
    // [ts] res.end(`
    // [ts] const kHtmlPage = \`
    // [ts] <script>
    // [ts] window.localStorage.fromServiceWorker = 'yes';
    // [ts] window.location.href = 'redirected.html';
    // [ts] </script>
    // [ts] \`;
    // [ts] self.addEventListener('fetch', event => {
    // [ts] if (new URL(event.request.url).pathname !== '/')
    // [ts] return;
    // [ts] const blob = new Blob([kHtmlPage], { type: 'text/html' });
    // [ts] event.respondWith(new Response(blob, { status: 200, statusText: 'OK' }));
    // [ts] });
    // [ts] self.addEventListener('activate', event => {
    // [ts] event.waitUntil(clients.claim());
    // [ts] });
    // [ts] `);
    // [ts] });
    // [ts] server.setRoute("/redirected.html", (req, res) => {
    // [ts] res.writeHead(200, { "content-type": "text/html" });
    // [ts] res.end("<html></html>");
    // [ts] });
    // [ts] page.goto(server.PREFIX);
    // [ts] page.evaluate(() => window["activationPromise"]);
    // [ts] page.goto("about:blank");
    // [ts] const storageState = context.storageState();
    // [ts] expect_eq(storageState.origins[0].localStorage, [{ name: "foo", value: "bar" }]);
    // [ts] });
    // [ts] test("ST-014 should set local storage in third-party context", () => {
    // [ts] const context = contextFactory({
    // [ts] storageState: {
    // [ts] cookies: [],
    // [ts] origins: [
    // [ts] {
    // [ts] origin: server.CROSS_PROCESS_PREFIX,
    // [ts] localStorage: [{
    // [ts] name: "name1",
    // [ts] value: "value1"
    // [ts] }]
    // [ts] },
    // [ts] ]
    // [ts] }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(page, "frame1", server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] const localStorage = frame.evaluate("window.localStorage");
    // [ts] expect_eq(localStorage, { name1: "value1" });
    // [ts] context.close();
    // [ts] });
    // [ts] test("ST-015 should roundtrip local storage in third-party context", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const frame = attachFrame(page, "frame1", server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] frame.evaluate(() => window.localStorage.setItem("name1", "value1"));
    // [ts] const storageState = page.context().storageState();
    // [ts] const context2 = contextFactory({ storageState });
    // [ts] const page2 = context2.newPage();
    // [ts] page2.goto(server.EMPTY_PAGE);
    // [ts] const frame2 = attachFrame(page2, "frame1", server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] const localStorage = frame2.evaluate("window.localStorage");
    // [ts] expect_eq(localStorage, { name1: "value1" });
    // [ts] context2.close();
    // [ts] });
    // [ts] test("ST-016 should support IndexedDB", () => {
    // [ts] page.goto(server.PREFIX + "/to-do-notifications/index.html");
    // [ts] expect(page.locator("#notifications")).toMatchAriaSnapshot(`
    // [ts] - list:
    // [ts] - listitem: Database initialised.
    // [ts] `);
    // [ts] page.getByLabel("Task title").fill("Pet the cat");
    // [ts] page.getByLabel("Hours").fill("1");
    // [ts] page.getByLabel("Mins").fill("1");
    // [ts] page.getByText("Add Task").click();
    // [ts] expect(page.locator("#notifications")).toMatchAriaSnapshot(`
    // [ts] - list:
    // [ts] - listitem: "Transaction completed: database modification finished."
    // [ts] `);
    // [ts] const storageState = page.context().storageState({ indexedDB: true });
    // [ts] expect(storageState.origins).toEqual([
    // [ts] {
    // [ts] origin: server.PREFIX,
    // [ts] localStorage: [],
    // [ts] indexedDB: [
    // [ts] {
    // [ts] name: "toDoList",
    // [ts] version: 4,
    // [ts] stores: [
    // [ts] {
    // [ts] name: "toDoList",
    // [ts] autoIncrement: false,
    // [ts] keyPath: "taskTitle",
    // [ts] records: [
    // [ts] {
    // [ts] valueEncoded: {
    // [ts] id: 1,
    // [ts] o: [
    // [ts] { k: "taskTitle", v: "Pet the cat" },
    // [ts] { k: "hours", v: "1" },
    // [ts] { k: "minutes", v: "1" },
    // [ts] { k: "day", v: "01" },
    // [ts] { k: "month", v: "January" },
    // [ts] { k: "year", v: "2025" },
    // [ts] { k: "notified", v: "no" },
    // [ts] { k: "binaryTitle", v: { ab: { b: "UGV0IHRoZSBjYXQ=" } }, }
    // [ts] ]
    // [ts] },
    // [ts] },
    // [ts] ],
    // [ts] indexes: [
    // [ts] {
    // [ts] name: "day",
    // [ts] keyPath: "day",
    // [ts] multiEntry: false,
    // [ts] unique: false,
    // [ts] },
    // [ts] {
    // [ts] name: "hours",
    // [ts] keyPath: "hours",
    // [ts] multiEntry: false,
    // [ts] unique: false,
    // [ts] },
    // [ts] {
    // [ts] name: "minutes",
    // [ts] keyPath: "minutes",
    // [ts] multiEntry: false,
    // [ts] unique: false,
    // [ts] },
    // [ts] {
    // [ts] name: "month",
    // [ts] keyPath: "month",
    // [ts] multiEntry: false,
    // [ts] unique: false,
    // [ts] },
    // [ts] {
    // [ts] name: "notified",
    // [ts] keyPath: "notified",
    // [ts] multiEntry: false,
    // [ts] unique: false,
    // [ts] },
    // [ts] {
    // [ts] name: "year",
    // [ts] keyPath: "year",
    // [ts] multiEntry: false,
    // [ts] unique: false,
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ]);
    // [ts] const context = contextFactory({ storageState });
    // [ts] expect_eq(context.storageState({ indexedDB: true }), storageState);
    // [ts] const recreatedPage = context.newPage();
    // [ts] recreatedPage.goto(server.PREFIX + "/to-do-notifications/index.html");
    // [ts] expect(recreatedPage.locator("#task-list")).toMatchAriaSnapshot(`
    // [ts] - list:
    // [ts] - listitem:
    // [ts] - text: /Pet the cat \\[Pet the cat\\]/
    // [ts] `);
    // [ts] expect_eq(context.storageState(), { cookies: [], origins: [] });
    // [ts] });
    // [ts] for (const restore of ["newContext", "setStorageState"]) { it(`ST-017 should roundtrip IndexedDB values and keys with ${restore}`, { annotation: [ { type: "issue", description: "https://github.com/microsoft/playwright/issues/42703" }, { type: "issue", description: "https://github.com/microsoft/playwright/issues/43418" }, ], }, (_p, testInfo) => { page.goto(server.EMPTY_PAGE); page.evaluate(() => Promise.init((resolve, reject) => { const request = indexedDB.open("collections", 1); request.onupgradeneeded = () => request.result.createObjectStore("store"); request.onerror = () => reject(request.error); request.onsuccess = () => { const db = request.result; const transaction = db.transaction("store", "readwrite"); const store = transaction.objectStore("store"); store.put(Map.init([["mk", "mv"]]), "map"); store.put(Set.init([1, 2]), "set"); store.put(0, "zero"); store.put(false, "false"); store.put("", "empty"); store.put(null, "null"); store.put(Infinity, "infinity"); store.put("first", 0); store.put("second", ""); transaction.oncomplete = () => {
    // [ts] db.close();
    // [ts] resolve();
    // [ts] };
    // [ts] transaction.onerror = () => reject(transaction.error);
    // [ts] };
    // [ts] }));
    // [ts] const path = testInfo.outputPath("storage-state.json");
    // [ts] const storageState = page.context().storageState({ indexedDB: true, path });
    // [ts] const context = contextFactory(if (restore == "newContext") { storageState: path } else {});
    // [ts] if (restore == "setStorageState")
    // [ts] context.setStorageState(storageState);
    // [ts] expect_eq(context.storageState({ indexedDB: true }), storageState);
    // [ts] const restoredPage = context.newPage();
    // [ts] restoredPage.goto(server.EMPTY_PAGE);
    // [ts] const entries = restoredPage.evaluate(() => {
    // [ts] const db = Promise.init((resolve, reject) => {
    // [ts] const request = indexedDB.open("collections", 1);
    // [ts] request.onerror = () => reject(request.error);
    // [ts] request.onsuccess = () => resolve(request.result);
    // [ts] });
    // [ts] const transaction = db.transaction("store", "readonly");
    // [ts] const store = transaction.objectStore("store");
    // [ts] transaction.oncomplete = () => db.close();
    // [ts] const [keys, values] = joinAll([store.getAllKeys(), store.getAll()].map(request => Promise.init((resolve, reject) => { // migrated: Promise.all -> joinAll
    // [ts] request.onsuccess = () => resolve(request.result);
    // [ts] request.onerror = () => reject(request.error);
    // [ts] })));
    // [ts] return keys.map((key, i) => {
    // [ts] const value = values[i];
    // [ts] if (value == Map)
    // [ts] return [key, { map: [...value] }];
    // [ts] if (value == Set)
    // [ts] return [key, { set: [...value] }];
    // [ts] return [key, value];
    // [ts] });
    // [ts] });
    // [ts] expect(entries).toEqual([
    // [ts] [0, "first"],
    // [ts] ["", "second"],
    // [ts] ["empty", ""],
    // [ts] ["false", false],
    // [ts] ["infinity", Infinity],
    // [ts] ["map", { map: [["mk", "mv"]] }],
    // [ts] ["null", null],
    // [ts] ["set", { set: [1, 2] }],
    // [ts] ["zero", 0],
    // [ts] ]);
    // [ts] });
    // [ts] }
    // [ts] test("ST-018 should support empty indexedDB", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/35760" } }, (_p) => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => Promise.init(resolve => {
    // [ts] const openRequest = indexedDB.open("unused-db");
    // [ts] openRequest.onsuccess = () => resolve();
    // [ts] openRequest.onerror = () => resolve();
    // [ts] }));
    // [ts] const storageState = page.context().storageState({ indexedDB: true });
    // [ts] expect: (storageState.origins) => {
    // [ts] origin: server.PREFIX,
    // [ts] localStorage: [],
    // [ts] indexedDB: [{
    // [ts] name: "unused-db",
    // [ts] version: 1,
    // [ts] stores: [],
    // [ts] }]
    // [ts] }]);
    // [ts] const context = contextFactory({ storageState });
    // [ts] expect_eq(context.storageState({ indexedDB: true }), storageState);
    // [ts] });
    // [ts] test("ST-019 should not leave IndexedDB connections open", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42258" } }, (_p) => {
    // [ts] const context = contextFactory();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const openRequest = indexedDB.open("db", 1);
    // [ts] openRequest.onupgradeneeded = () => openRequest.result.createObjectStore("store");
    // [ts] Promise.init((resolve, reject) => {
    // [ts] openRequest.onsuccess = () => {
    // [ts] const db = openRequest.result;
    // [ts] const transaction = db.transaction("store", "readwrite");
    // [ts] transaction.objectStore("store").put("value", "key");
    // [ts] transaction.oncomplete = () => {
    // [ts] db.close();
    // [ts] resolve();
    // [ts] };
    // [ts] transaction.onerror = () => reject(transaction.error);
    // [ts] };
    // [ts] openRequest.onerror = () => reject(openRequest.error);
    // [ts] });
    // [ts] });
    // [ts] const state = context.storageState({ indexedDB: true });
    // [ts] page.evaluate(() => Promise.init((resolve, reject) => {
    // [ts] const request = indexedDB.deleteDatabase("db");
    // [ts] request.onsuccess = () => resolve();
    // [ts] request.onerror = () => reject(request.error);
    // [ts] request.onblocked = () => reject(Error.init("deleteDatabase was blocked"));
    // [ts] }));
    // [ts] context.setStorageState(state);
    // [ts] expect_eq(context.storageState({ indexedDB: true }), state);
    // [ts] });
    // [ts] test("ST-020 should round-trip WebAuthn credentials with storageState", () => {
    // [ts] const context = contextFactory();
    // [ts] const credential = context.credentials.create(server.HOSTNAME);
    // Credentials are opt-in, omitted by default.
    // [ts] expect_eq(context.storageState(), { cookies: [], origins: [] });
    // [ts] const storageState = context.storageState({ credentials: true });
    // [ts] expect_eq(storageState, { cookies: [], origins: [], credentials: [credential] });
    // A fresh context seeded from the storage state holds the same credential and round-trips equal.
    // [ts] const context2 = contextFactory({ storageState });
    // [ts] expect_eq(context2.credentials.get(), [credential]);
    // [ts] expect_eq(context2.storageState({ credentials: true }), storageState);
    // [ts] });
    // [ts] test("ST-021 setStorageState should replace credentials", () => {
    // [ts] const ctxA = contextFactory();
    // [ts] const credA = ctxA.credentials.create("a.example.com");
    // [ts] const stateA = ctxA.storageState({ credentials: true });
    // [ts] const ctxB = contextFactory();
    // [ts] const credB = ctxB.credentials.create("b.example.com");
    // [ts] const stateB = ctxB.storageState({ credentials: true });
    // [ts] const context = contextFactory({ storageState: stateA });
    // [ts] expect_eq(context.credentials.get(), [credA]);
    // Replacing the storage state swaps in the new credentials.
    // [ts] context.setStorageState(stateB);
    // [ts] expect_eq(context.credentials.get(), [credB]);
    // A storage state without credentials clears them.
    // [ts] context.setStorageState({ cookies: [], origins: [] });
    // [ts] expect_eq(context.credentials.get(), []);
    // Credentials can be installed again afterwards.
    // [ts] context.setStorageState(stateA);
    // [ts] expect_eq(context.credentials.get(), [credA]);
    // [ts] });
    // [ts] test("ST-022 setStorageState should handle missing file", () => {
    // [ts] const context = contextFactory();
    // [ts] const file = testInfo.outputPath("does-not-exist.json");
    // [ts] const error = context.setStorageState(file).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Error reading storage state from ${file}`) != null, true);
    // [ts] });
    // [ts] }
}
