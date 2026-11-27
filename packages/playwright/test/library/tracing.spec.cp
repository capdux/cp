// Migrated from TypeScript (library/tracing.spec.ts) via cap from -ts

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
import { PNG, jpegjs } from "playwright-core/lib/utilsBundle";
import path from "cap:path";
import { browserTest, contextTest as test, expect } from "../config/browserTest.cp";
import { parseTraceRaw } from "../config/utils.cp";
// [cap-migrate] import { StackFrame, ActionTraceEvent } from "../../packages/isomorphic/trace/trace.cp";
import { artifactsFolderName } from "../../packages/playwright/src/isomorphic/folders.cp";
import { rafraf } from "../page/pageTest.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => trace == "on");
    // [ts] test("TR-001 should collect trace with resources, but no js", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44074" }
    // [ts] }, (_p, testInfo) => {
    // [ts] const fetchedBodies: []string = [];
    // [ts] if (mode == "default") {
    // [ts] toImplInWorkerScope: (context) => { url() => {
    // [ts] const internalBody = response.internalBody.bind(response);
    // [ts] response.internalBody = () => {
    // [ts] fetchedBodies.push(response.url());
    // [ts] return internalBody();
    // [ts] };
    // [ts] });
    // [ts] }
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.goto(server.PREFIX + "/frames/frame.html");
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"Click\"");
    // [ts] page.mouse.move(20, 20);
    // [ts] page.mouse.dblclick(30, 30);
    // [ts] page.keyboard.insertText("abc");
    // [ts] page.goto(server.PREFIX + "/input/fileupload.html");
    // [ts] page.locator("input[type=\"file\"]").setInputFiles(asset("file-to-upload.txt"));
    // [ts] page.waitForTimeout(2000);  // Give it some time to produce screenshots.
    // [ts] page.close();
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events, actions } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(events[0].type, "context-options");
    // [ts] expect(actions).toEqual([
    // [ts] `Navigate ${server.HOST}/frames/frame.html`,
    // [ts] "Set content",
    // [ts] `Click locator('text="Click"')`,
    // [ts] "Mouse move",
    // [ts] "Double click",
    // [ts] "Insert \"abc\"",
    // [ts] `Navigate ${server.HOST}/input/fileupload.html`,
    // [ts] `Set input files locator('input[type="file"]')`,
    // [ts] "Wait for timeout",
    // [ts] "Close page",
    // [ts] ]);
    // [ts] expect_eq(events.some(e => e.type == "frame-snapshot"), true);
    // [ts] expect_eq(events.some(e => e.type == "screencast-frame"), true);
    // [ts] const style = events.find(e => e.type == "resource-snapshot" && e.snapshot.request.url.endsWith("style.css"));
    // [ts] expect_eq(style, true);
    // [ts] expect_eq(style.snapshot.response.content._file, true);
    // [ts] const script = events.find(e => e.type == "resource-snapshot" && e.snapshot.request.url.endsWith("script.js"));
    // [ts] expect_eq(script, true);
    // [ts] expect_eq(script.snapshot.response.content._file, null);
    // [ts] expect_eq(script.snapshot.response.bodySize > 0, true);
    // [ts] expect_eq(script.snapshot.response.headersSize > 0, true);
    // [ts] if (mode == "default") {
    // [ts] expect_eq(fetchedBodies.some(url => url.endsWith("style.css")), true);
    // [ts] expect_eq(fetchedBodies.some(url => url.endsWith("script.js")), false);
    // [ts] }
    // [ts] });
    // [ts] test("TR-002 should use the correct title for event driven callbacks", () => {
    // [ts] context.tracing.start();
    // route.* calls should not be included in the trace
    // [ts] page.route("**/empty.html", route => route.continue());
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // The default internal dialog handler should not provide an action.
    // [ts] page.evaluate(() => alert("yo"));
    // [ts] page.reload();
    // now we do it again with a dialog event listener attached which should produce an action.
    // [ts] page.on("dialog", dialog => {
    // [ts] void dialog.accept("answer!");
    // [ts] });
    // [ts] page.evaluate(() => alert("yo"));
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events, actions } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(events[0].type, "context-options");
    // [ts] expect(actions).toEqual([
    // [ts] "Route requests",
    // [ts] `Navigate ${server.HOST}/empty.html`,
    // [ts] "Continue request",
    // [ts] `Navigate ${server.HOST}/grid.html`,
    // [ts] "Evaluate",
    // [ts] "Reload",
    // [ts] "Evaluate",
    // [ts] "Accept dialog",
    // [ts] ]);
    // [ts] });
    // [ts] browserTest("TR-003 should not store secrets in context options", (_p, testInfo) => {
    // [ts] const context = browser.newContext({
    // [ts] baseURL: server.PREFIX,
    // [ts] viewport: { width: 500, height: 600 },
    // [ts] deviceScaleFactor: 2,
    // [ts] isMobile: false,
    // [ts] userAgent: "custom-ua",
    // [ts] httpCredentials: { username: "user", password: "secret-password" },
    // [ts] extraHTTPHeaders: { "x-secret": "secret-header" },
    // [ts] storageState: { cookies: [{ name: "session", value: "secret-cookie", domain: "localhost", path: "/", expires: -1, httpOnly: false, secure: false, sameSite: "Lax" }], origins: [] },
    // [ts] });
    // [ts] context.tracing.start();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] context.close();
    // [ts] const { events } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(events[0].type, "context-options");
    // [ts] expect: (events[0].options) => {
    // [ts] baseURL: server.PREFIX,
    // [ts] viewport: { width: 500, height: 600 },
    // [ts] deviceScaleFactor: 2,
    // [ts] isMobile: false,
    // [ts] userAgent: "custom-ua",
    // [ts] });
    // [ts] const traceText = fs.readFileSync(testInfo.outputPath("trace.zip")).toString("latin1");
    // [ts] expect_eq(std.mem.indexOf(u8, traceText, "secret-password") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, traceText, "secret-header") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, traceText, "secret-cookie") == null, true);
    // [ts] });
    // [ts] test("TR-004 should not collect snapshots by default", () => {
    // [ts] context.tracing.start();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"Click\"");
    // [ts] page.close();
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(events.some(e => e.type == "frame-snapshot"), false);
    // [ts] expect_eq(events.some(e => e.type == "resource-snapshot"), false);
    // [ts] });
    // [ts] test("TR-005 should not collect action screenshots and aria snapshots by default", () => {
    // [ts] context.tracing.start({ snapshots: true });
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(events.some(e => e.type == "screenshot"), false);
    // [ts] expect_eq(events.some(e => e.type == "aria-snapshot"), false);
    // [ts] });
    // [ts] test("TR-006 should collect action screenshots", () => {
    // [ts] context.tracing.start({ snapshots: { screen: true } });
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events, resources } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] const clickCallId = events.find(e => e.type == "before" && e.method == "click").callId;
    // [ts] const screenshots = events.filter(e => e.type == "screenshot" && e.callId == clickCallId);
    // [ts] expect_eq(screenshots.map(e => e.phase), ["before", "action", "after"]);
    // [ts] for (const screenshot of screenshots) {
    // [ts] expect_eq(screenshot.file, `screenshots/${clickCallId}-${screenshot.phase}.png`);
    // [ts] const buffer = resources.get(screenshot.file);
    // [ts] expect_eq(PNG.sync.read(buffer).width > 0, true);
    // [ts] }
    // [ts] });
    // [ts] test("TR-007 should collect aria snapshots", () => {
    // [ts] context.tracing.start({ snapshots: { aria: true } });
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events, resources } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] const clickCallId = events.find(e => e.type == "before" && e.method == "click").callId;
    // [ts] const ariaSnapshots = events.filter(e => e.type == "aria-snapshot" && e.callId == clickCallId);
    // [ts] expect_eq(ariaSnapshots.map(e => e.phase), ["before", "action", "after"]);
    // [ts] const hasButton = nodes => nodes.some(node => @typeName(node) == "object" && (node.role == "button" && node.name == "Click target" || hasButton(node.children ?? [])));
    // [ts] for (const ariaSnapshot of ariaSnapshots) {
    // [ts] expect_eq(ariaSnapshot.file, `aria/${clickCallId}-${ariaSnapshot.phase}.json`);
    // [ts] const snapshot = JSON.parse(resources.get(ariaSnapshot.file).toString());
    // [ts] expect_eq(hasButton(snapshot), true);
    // [ts] }
    // [ts] });
    // [ts] test("TR-008 can call tracing.group/groupEnd at any time and auto-close", () => {
    // [ts] context.tracing.group("ignored");
    // [ts] context.tracing.groupEnd();
    // [ts] context.tracing.group("ignored2");
    // [ts] context.tracing.start();
    // [ts] context.tracing.group("actual");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace.zip") });
    // [ts] context.tracing.group("ignored3");
    // [ts] context.tracing.groupEnd();
    // [ts] context.tracing.groupEnd();
    // [ts] context.tracing.groupEnd();
    // [ts] const { events } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] const groups = events.filter(e => e.method == "tracingGroup");
    // [ts] expect(groups).toHaveLength(1);
    // [ts] expect_eq(groups[0].title, "actual");
    // [ts] expect_eq(events.some(e => e.type == "after" && e.callId == groups[0].callId), true);
    // [ts] });
    // [ts] test("TR-009 should not include buffers in the trace", () => {
    // [ts] context.tracing.start({ snapshots: true });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] page.screenshot();
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events, actionObjects } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] const screenshotEvent = actionObjects.find(a => a.method == "screenshot");
    // [ts] const phases = events.filter(e => e.type == "frame-snapshot" && e.snapshot.callId == screenshotEvent.callId).map(e => e.snapshot.phase);
    // [ts] expect_eq(std.mem.indexOf(u8, phases, "before") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, phases, "after") != null, true);
    // [ts] expect: (screenshotEvent.result) => {
    // [ts] "binary": "<Buffer>",
    // [ts] });
    // [ts] });
    // [ts] test("TR-010 should exclude internal pages", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] context.tracing.start();
    // [ts] context.storageState();
    // [ts] page.close();
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const trace = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] const pageIds = Set.init();
    // [ts] trace.events.forEach(e => {
    // [ts] const pageId = e.pageId ?? e.params?.pageId;
    // [ts] if (pageId)
    // [ts] pageIds.add(pageId);
    // [ts] });
    // [ts] expect_eq(pageIds.size, 1);
    // [ts] });
    // [ts] test("TR-011 should record context API request trace independently", () => {
    // [ts] const browserTracePath = testInfo.outputPath("browser-trace.zip");
    // [ts] const apiTracePath = testInfo.outputPath("api-trace.zip");
    // [ts] const apiURL = server.PREFIX + "/simple.json";
    // [ts] expect_eq(context.request.tracing != context.tracing, true);
    // [ts] context.tracing.start({ snapshots: true });
    // [ts] context.request.tracing.start({ snapshots: true });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] page.request.post(apiURL, { data: { foo: "bar" } });
    // [ts] context.tracing.stop({ path: browserTracePath });
    // [ts] context.request.tracing.stop({ path: apiTracePath });
    // [ts] const browserTrace = parseTraceRaw(browserTracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, browserTrace.actions, `Navigate ${server.HOST}/one-style.html`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, browserTrace.actions, `POST ${server.HOST}/simple.json`) == null, true);
    // [ts] expect_eq(browserTrace.events.some(event => event.type == "resource-snapshot" && event.snapshot.request.url.endsWith("/simple.json")), false);
    // [ts] expect_eq(browserTrace.events.some(event => event.type == "resource-snapshot" && event.snapshot.request.url.endsWith("/one-style.html")), true);
    // [ts] const apiTrace = parseTraceRaw(apiTracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, apiTrace.actions, `POST ${server.HOST}/simple.json`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, apiTrace.actions, `Navigate ${server.HOST}/one-style.html`) == null, true);
    // [ts] const apiAction = apiTrace.actionObjects.find(action => action.class == "APIRequestContext" && action.method == "fetch");
    // [ts] expect_eq(relativeStack(apiAction, apiTrace.stacks), ["tracing.spec.ts"]);
    // [ts] expect_eq(apiTrace.events.filter(event => event.type == "resource-snapshot").map(event => event.snapshot.request.url), [apiURL]);
    // [ts] expect_eq(apiTrace.events.filter(event => event.type == "resource-snapshot").map(event => event.snapshot._apiRequestRef), [expect.stringMatching("^request-context@")]);
    // [ts] });
    // [ts] test("TR-012 should collect two traces", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"Click\"");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace1.zip") });
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.dblclick("\"Click\"");
    // [ts] page.close();
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace2.zip") });
    // [ts] {
    // [ts] const { events, actions } = parseTraceRaw(testInfo.outputPath("trace1.zip"));
    // [ts] expect_eq(events[0].type, "context-options");
    // [ts] expect(actions).toEqual([
    // [ts] `Navigate ${server.HOST}/empty.html`,
    // [ts] "Set content",
    // [ts] `Click locator('text="Click"')`,
    // [ts] ]);
    // [ts] }
    // [ts] {
    // [ts] const { events, actions } = parseTraceRaw(testInfo.outputPath("trace2.zip"));
    // [ts] expect_eq(events[0].type, "context-options");
    // [ts] expect(actions).toEqual([
    // [ts] `Double click locator('text="Click"')`,
    // [ts] "Close page",
    // [ts] ]);
    // [ts] }
    // [ts] });
    // [ts] test("TR-013 should respect tracesDir and name", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44086" }
    // [ts] }, (_p, testInfo) => {
    // [ts] const tracesDir = testInfo.outputPath("traces");
    // A relative path resolves against the current directory.
    // [ts] const browser = browserType.launch({ tracesDir: path.relative(process.cwd(), tracesDir) });
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] context.tracing.start({ name: "name1", snapshots: true });
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace1.zip") });
    // [ts] expect_eq(fs.existsSync(path.join(tracesDir, "name1.trace")), true);
    // [ts] expect_eq(fs.existsSync(path.join(tracesDir, "name1.network")), true);
    // [ts] context.tracing.startChunk({ name: "name2" });
    // [ts] page.goto(server.PREFIX + "/har.html");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace2.zip") });
    // [ts] expect_eq(fs.existsSync(path.join(tracesDir, "name2.trace")), true);
    // [ts] expect_eq(fs.existsSync(path.join(tracesDir, "name2.network")), true);
    // [ts] context.tracing.start({ name: "discarded", screenshots: true, snapshots: { dom: true, screen: true, aria: true } });
    // [ts] page.goto(server.PREFIX + "/har.html");
    // A stylesheet changed through CSSOM is saved as a snapshot resource.
    // [ts] page.evaluate(() => document.styleSheets[0].insertRule(".discarded-rule { color: red }"));
    // [ts] page.click("body");
    // [ts] const snapshotResources = () => fs.readdirSync(path.join(tracesDir, "resources")).filter(file => fs.readFileSync(path.join(tracesDir, "resources", file), "utf8").includes(".discarded-rule"));
    // [ts] for (const dir of ["screencast", "screenshots", "aria"])
    // [ts] expect.poll(() => fs.readdirSync(path.join(tracesDir, dir)).length).toBeGreaterThan(0);
    // [ts] expect.poll(snapshotResources).toHaveLength(1);
    // [ts] const networkResources = fs.readdirSync(path.join(tracesDir, "resources")).filter(file => !snapshotResources().includes(file));
    // [ts] context.tracing.stopChunk();
    // Discarded files are removed in the background.
    // [ts] expect.poll(() => fs.existsSync(path.join(tracesDir, "discarded.trace"))).toBe(false);
    // [ts] for (const dir of ["screencast", "screenshots", "aria"])
    // [ts] expect.poll(() => fs.readdirSync(path.join(tracesDir, dir))).toEqual([]);
    // [ts] expect.poll(() => fs.readdirSync(path.join(tracesDir, "resources"))).toEqual(networkResources);
    // [ts] expect_eq(fs.existsSync(path.join(tracesDir, "discarded.network")), true);
    // [ts] context.tracing.stop();
    // [ts] browser.close();
    // [ts] fn resourceNames(resources: Map<string, []u8>) {
    // [ts] return [...resources.keys()].map(file => {
    // [ts] return file.replace("^resources\\/.*\\.(html|css)$", "resources/XXX.$1");
    // [ts] }).sort();
    // [ts] }
    // [ts] {
    // [ts] const { resources, actions } = parseTraceRaw(testInfo.outputPath("trace1.zip"));
    // [ts] expect_eq(actions, [`Navigate ${server.HOST}/one-style.html`]);
    // [ts] expect(resourceNames(resources)).toEqual([
    // [ts] "resources/XXX.css",
    // [ts] "resources/XXX.html",
    // [ts] "trace.network",
    // [ts] "trace.stacks",
    // [ts] "trace.trace",
    // [ts] ]);
    // [ts] }
    // [ts] {
    // [ts] const { resources, actions } = parseTraceRaw(testInfo.outputPath("trace2.zip"));
    // [ts] expect_eq(actions, [`Navigate ${server.HOST}/har.html`]);
    // [ts] expect(resourceNames(resources)).toEqual([
    // [ts] "resources/XXX.css",
    // [ts] "resources/XXX.html",
    // [ts] "resources/XXX.html",
    // [ts] "trace.network",
    // [ts] "trace.stacks",
    // [ts] "trace.trace",
    // [ts] ]);
    // [ts] }
    // [ts] });
    // [ts] test("TR-014 should not include trace resources from the previous chunks", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true, sources: true });
    // [ts] context.tracing.startChunk();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] @keyframes move {
    // [ts] from { marign-left: 0; }
    // [ts] to   { margin-left: 1000px; }
    // [ts] }
    // [ts] button {
    // [ts] animation: 20s linear move;
    // [ts] animation-iteration-count: infinite;
    // [ts] }
    // [ts] </style>
    // [ts] <button>Click</button>
    // [ts] `);
    // [ts] page.click("\"Click\"", { force: true });
    // Give it enough time for both screenshots to get into the trace.
    // [ts] Promise.init(f => setTimeout(f, 3000));
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace1.zip") });
    // [ts] context.tracing.startChunk();
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace2.zip") });
    // [ts] let jpegs: []string = [];
    // [ts] {
    // [ts] const { resources } = parseTraceRaw(testInfo.outputPath("trace1.zip"));
    // [ts] const names = Array.from(resources.keys());
    // [ts] expect_eq(names.filter(n => n.endsWith(".html")).length, 1);
    // [ts] jpegs = names.filter(n => n.endsWith(".jpeg"));
    // 1 source file for the test.
    // [ts] expect_eq(names.filter(n => n.startsWith("src/")).length, 1);
    // [ts] }
    // [ts] {
    // [ts] const { resources } = parseTraceRaw(testInfo.outputPath("trace2.zip"));
    // [ts] const names = Array.from(resources.keys());
    // 1 network resource should be preserved.
    // [ts] expect_eq(names.filter(n => n.endsWith(".html")).length, 1);
    // screenshots from the previous chunk should not be preserved.
    // [ts] expect_eq(names.filter(n => jpegs.includes(n)).length, 0);
    // 0 source files for the second test.
    // [ts] expect_eq(names.filter(n => n.startsWith("src/")).length, 0);
    // [ts] }
    // [ts] });
    // [ts] test("TR-015 should overwrite existing file", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true, sources: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"Click\"");
    // [ts] const tracePath = testInfo.outputPath("trace1.zip");
    // [ts] context.tracing.stop({ path: tracePath });
    // [ts] {
    // [ts] const { resources } = parseTraceRaw(tracePath);
    // [ts] const names = Array.from(resources.keys());
    // [ts] expect_eq(names.filter(n => n.endsWith(".html")).length, 1);
    // [ts] }
    // [ts] context.tracing.start({ screenshots: true, snapshots: true, sources: true });
    // A relative path resolves against the current directory.
    // [ts] context.tracing.stop({ path: path.relative(process.cwd(), tracePath) });
    // [ts] {
    // [ts] const { resources } = parseTraceRaw(tracePath);
    // [ts] const names = Array.from(resources.keys());
    // [ts] expect_eq(names.filter(n => n.endsWith(".html")).length, 0);
    // [ts] }
    // [ts] });
    // [ts] test("TR-016 should collect sources", () => {
    // [ts] context.tracing.start({ sources: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"Click\"");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace1.zip") });
    // [ts] const { resources } = parseTraceRaw(testInfo.outputPath("trace1.zip"));
    // [ts] const sourceNames = Array.from(resources.keys()).filter(k => k.startsWith("src/"));
    // [ts] expect_eq(sourceNames.length, 1);
    // [ts] expect(sourceNames[0]).toMatch("^src\\/[0-9a-f]{40}\\.ts$");
    // [ts] const sourceFile = resources.get(sourceNames[0]);
    // [ts] const thisFile = fs.promises.readFile(__filename);
    // [ts] expect_eq(sourceFile, thisFile);
    // [ts] });
    // [ts] test("TR-017 should record network failures", () => {
    // [ts] context.tracing.start({ snapshots: true });
    // [ts] page.route("**/*", route => route.abort("connectionaborted"));
    // [ts] page.goto(server.EMPTY_PAGE).catch(e => {});
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace1.zip") });
    // [ts] const { events } = parseTraceRaw(testInfo.outputPath("trace1.zip"));
    // [ts] const requestEvent = events.find(e => e.type == "resource-snapshot" && !!e.snapshot.response._failureText);
    // [ts] expect_eq(requestEvent, true);
    // [ts] expect_eq(requestEvent.snapshot._monotonicTime > 0, true);
    // [ts] expect_eq(requestEvent.snapshot.time >= 0, true);
    // [ts] });
    // [ts] test("TR-018 should recover tracing after a failed stop", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42423" });
    // [ts] context.tracing.start();
    // Saving fails: a parent of the destination is a file, not a directory.
    // [ts] const blocker = testInfo.outputPath("blocker");
    // [ts] fs.promises.writeFile(blocker, "");
    // [ts] expect(context.tracing.stop({ path: path.join(blocker, "trace1.zip") })).rejects.toThrow("ENOTDIR|ENOENT|EEXIST");
    // The failed stop must not wedge tracing for the rest of the context lifetime.
    // [ts] context.tracing.start();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace2.zip") });
    // [ts] const { events, actions } = parseTraceRaw(testInfo.outputPath("trace2.zip"));
    // [ts] expect_eq(events[0].type, "context-options");
    // [ts] expect_eq(std.mem.indexOf(u8, actions, `Click locator('button')`) != null, true);
    // [ts] });
    // [ts] test("TR-019 should release the stack session when saving the trace fails", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42423" });
    // Override the test runner's tracesDir, so that the stack session owns a temporary directory.
    // [ts] const browser = browserType.launch({ tracesDir: null });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const context = browser.newContext();
    // [ts] context.tracing.start();
    // [ts] const stacksDir = path.dirname((context.tracing)._stacksId);
    // [ts] expect_eq(fs.existsSync(stacksDir), true);
    // [ts] const blocker = testInfo.outputPath("blocker");
    // [ts] fs.promises.writeFile(blocker, "");
    // [ts] expect(context.tracing.stop({ path: path.join(blocker, "trace.zip") })).rejects.toThrow("ENOTDIR|ENOENT|EEXIST");
    // [ts] expect_eq(fs.existsSync(stacksDir), false);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.close();
    // [ts] }
    // [ts] });
    // [ts] test("TR-020 should not crash when browser closes mid-trace", () => {
    // [ts] const browser = browserType.launch();
    // [ts] const page = browser.newPage();
    // [ts] page.context().tracing.start({ snapshots: true, screenshots: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] browser.close();
    // [ts] Promise.init(f => setTimeout(f, 1000));  // Give it some time to throw errors
    // [ts] });
    // [ts] test("TR-021 should survive browser.close with auto-created traces dir", () => {
    // [ts] const oldTracesDir = (browserType)._playwright._defaultTracesDir;
    // [ts] (browserType)._playwright._defaultTracesDir = null;
    // [ts] const browser = browserType.launch();
    // [ts] const page = browser.newPage();
    // [ts] page.context().tracing.start();
    // [ts] const done = { value: false };
    // [ts] async fn go() {
    // [ts] while (!done.value) {
    // Produce a lot of operations to make sure tracing operations are enqueued.
    // [ts] let i = 0;
    // [ts] while (i < 100) // [cap-migrate] for-step: i++
    // [ts] page.evaluate("1 + 1").catch(() => {});
    // [ts] Promise.init(f => setTimeout(f, 250));
    // [ts] }
    // [ts] }
    // [ts] void go();
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // Close the browser and give it some time to fail.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] browser.close(),
    // [ts] Promise.init(f => setTimeout(f, 500)),
    // [ts] ]);
    // [ts] done.value = true;
    // [ts] (browserType)._playwright._defaultTracesDir = oldTracesDir;
    // [ts] });
    // [ts] test("TR-022 should not stall on dialogs", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.on("dialog", async dialog => {
    // [ts] dialog.accept();
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] confirm("are you sure");
    // [ts] });
    // [ts] context.tracing.stop();
    // [ts] });
    // [ts] for (const params of [
    // [ts] {
    // [ts] id: "fit",
    // [ts] width: 500,
    // [ts] height: 500,
    // [ts] },
    // [ts] {
    // [ts] id: "crop",
    // [ts] width: 400, // Less than 450 to test firefox
    // [ts] height: 800,
    // [ts] },
    // [ts] {
    // [ts] id: "scale",
    // [ts] width: 1024,
    // [ts] height: 768,
    // [ts] }
    // [ts] ]) {
    // [ts] browserTest(`TR-023 should produce screencast frames ${params.id}`, (_p, testInfo) => {
    // [ts] browserTest.skip(video == "on", "Same screencast resolution conflicts");
    // [ts] browserTest.fixme(browserName == "chromium" && !isHeadlessShell, "Chromium (but not headless-shell) screencast has a min width issue");
    // [ts] browserTest.fixme(params.id == "fit" && browserName == "chromium" && platform == "darwin", "High DPI maxes image at 600x600");
    // [ts] browserTest.fixme(params.id == "fit" && browserName == "webkit" && platform == "linux", "Image size is flaky");
    // [ts] browserTest.fixme(browserName == "firefox" && !headless, "Image size is different");
    // [ts] const scale = Math.min(800 / params.width, 800 / params.height, 1);
    // [ts] const previewWidth = params.width * scale;
    // [ts] const previewHeight = params.height * scale;
    // [ts] const context = contextFactory({ viewport: { width: params.width, height: params.height } });
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] const page = context.newPage();
    // Make sure we have a chance to paint.
    // [ts] let i = 0;
    // [ts] while (i < 100) { // [cap-migrate] for-step: ++i
    // [ts] const percentage = (i + 1) + "%";
    // [ts] page.setContent(`<body style="box-sizing: border-box; width: 100%; height: 100%; margin:0; background: linear-gradient(to bottom, red ${percentage}, transparent ${percentage}); border: 50px solid blue"></body>`);
    // [ts] rafraf(page);
    // [ts] }
    // [ts] let i = 0;
    // [ts] while (i < 10) // [cap-migrate] for-step: ++i
    // [ts] rafraf(page);
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] const { events, resources } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] const frames = events.filter(e => e.type == "screencast-frame");
    // Check all frame sizes.
    // [ts] for (const frame of frames) {
    // [ts] expect.soft(frame.width).toBe(params.width);
    // [ts] expect.soft(frame.height).toBe(params.height);
    // [ts] const buffer = resources.get(frame.file);
    // [ts] const image = jpegjs.decode(buffer);
    // [ts] expect.soft(image.width).toBe(previewWidth);
    // [ts] expect.soft(image.height).toBe(previewHeight);
    // [ts] }
    // [ts] const frame = frames[frames.length - 1]; // pick last frame.
    // [ts] const buffer = resources.get(frame.file);
    // [ts] const image = jpegjs.decode(buffer);
    // [ts] expect_eq(image.data.byteLength, previewWidth * previewHeight * 4);
    // [ts] expectRed(image.data, previewWidth * previewHeight * 4 / 2 + previewWidth * 4 / 2); // center is red
    // [ts] expectBlue(image.data, previewWidth * 5 * 4 + previewWidth * 4 / 2); // top
    // [ts] expectBlue(image.data, previewWidth * (previewHeight - 5) * 4 + previewWidth * 4 / 2); // bottom
    // [ts] expectBlue(image.data, previewWidth * previewHeight * 4 / 2 + 5 * 4); // left
    // [ts] expectBlue(image.data, previewWidth * previewHeight * 4 / 2 + (previewWidth - 5) * 4); // right
    // [ts] });
    // [ts] }
    // [ts] test("TR-024 should include interrupted actions", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"ClickNoButton\"").catch(() =>  {});
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") });
    // [ts] context.close();
    // [ts] const { actions } = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(std.mem.indexOf(u8, actions, `Click locator('text="ClickNoButton"')`) != null, true);
    // [ts] });
    // [ts] test("TR-025 should throw when starting with different options", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] const error = context.tracing.start({ screenshots: false, snapshots: false }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Tracing has been already started") != null, true);
    // [ts] });
    // [ts] test("TR-026 should throw when stopping without start", () => {
    // [ts] const error = context.tracing.stop({ path: testInfo.outputPath("trace.zip") }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Must start tracing before stopping") != null, true);
    // [ts] });
    // [ts] test("TR-027 should not throw when stopping without start but not exporting", () => {
    // [ts] context.tracing.stop();
    // [ts] });
    // [ts] test("TR-028 start should return a disposable that discards the trace", () => {
    // [ts] const disposable = context.tracing.start();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] disposable.dispose();
    // [ts] const error = context.tracing.stop({ path: testInfo.outputPath("trace.zip") }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Must start tracing before stopping") != null, true);
    // [ts] });
    // [ts] test("TR-029 startChunk should return a disposable that discards the chunk", () => {
    // [ts] context.tracing.start();
    // [ts] const disposable = context.tracing.startChunk();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] disposable.dispose();
    // [ts] const error = context.tracing.stopChunk({ path: testInfo.outputPath("trace.zip") }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Must start tracing before stopping") != null, true);
    // [ts] });
    // [ts] test("TR-030 should work with multiple chunks", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.goto(server.PREFIX + "/frames/frame.html");
    // [ts] context.tracing.startChunk();
    // [ts] page.setContent("<button>Click</button>");
    // [ts] page.click("\"Click\"");
    // [ts] page.click("\"ClickNoButton\"", { timeout: 0 }).catch(() =>  {});
    // [ts] page.evaluate(() => {});
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace.zip") });
    // [ts] context.tracing.startChunk();
    // [ts] page.hover("\"Click\"");
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace2.zip") });
    // [ts] context.tracing.startChunk();
    // [ts] page.click("\"Click\"");
    // [ts] context.tracing.stopChunk();  // Should stop without a path.
    // [ts] const trace1 = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(trace1.events[0].type, "context-options");
    // [ts] expect(trace1.actions).toEqual([
    // [ts] "Set content",
    // [ts] `Click locator('text="Click"')`,
    // [ts] `Click locator('text="ClickNoButton"')`,
    // [ts] "Evaluate",
    // [ts] ]);
    // [ts] expect_eq(trace1.events.some(e => e.type == "frame-snapshot"), true);
    // [ts] expect_eq(trace1.events.some(e => e.type == "resource-snapshot" && e.snapshot.request.url.endsWith("style.css")), true);
    // [ts] const trace2 = parseTraceRaw(testInfo.outputPath("trace2.zip"));
    // [ts] expect_eq(trace2.events[0].type, "context-options");
    // [ts] expect(trace2.actions).toEqual([
    // [ts] `Hover locator('text="Click"')`,
    // [ts] ]);
    // [ts] expect_eq(trace2.events.some(e => e.type == "frame-snapshot"), true);
    // [ts] expect_eq(trace2.events.some(e => e.type == "resource-snapshot" && e.snapshot.request.url.endsWith("style.css")), true);
    // [ts] });
    // [ts] test("TR-031 should export trace concurrently to second navigation", () => {
    // [ts] let timeout = 0;
    // [ts] while (timeout < 200) { // [cap-migrate] for-step: timeout += 20
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // Navigate to the same page to produce the same trace resources
    // that might be concurrently exported.
    // [ts] const promise = page.goto(server.PREFIX + "/grid.html");
    // [ts] page.waitForTimeout(timeout);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] promise,
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace.zip") }),
    // [ts] ]);
    // [ts] }
    // [ts] });
    // [ts] test("TR-032 should not hang for clicks that open dialogs", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] const dialogPromise = page.waitForEvent("dialog");
    // [ts] page.setContent(`<div onclick='window.alert(123)'>Click me</div>`);
    // [ts] page.click("div", { timeout: 3500 }).catch(() => {});
    // [ts] const dialog = dialogPromise;
    // [ts] dialog.dismiss();
    // [ts] context.tracing.stop();
    // [ts] });
    // [ts] test("TR-033 should ignore iframes in head", () => {
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] document.head.appendChild(document.createElement("iframe"));
    // Add iframe in a shadow tree.
    // [ts] const div = document.createElement("div");
    // [ts] document.head.appendChild(div);
    // [ts] const shadow = div.attachShadow({ mode: "open" });
    // [ts] shadow.appendChild(document.createElement("iframe"));
    // [ts] });
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] page.click("button");
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace.zip") });
    // [ts] const trace = parseTraceRaw(testInfo.outputPath("trace.zip"));
    // [ts] expect_eq(trace.actions, [`Click locator('button')`]);
    // [ts] expect_eq(trace.events.find(e => e.type == "frame-snapshot"), true);
    // [ts] expect_eq(trace.events.find(e => e.type == "frame-snapshot" && JSON.stringify(e.snapshot.html).includes("IFRAME")), false);
    // [ts] });
    // [ts] test("TR-034 should hide internal stack frames", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] let evalPromise;
    // [ts] page.on("dialog", dialog => {
    // [ts] evalPromise = page.evaluate("2+2");
    // [ts] void dialog.dismiss();
    // [ts] });
    // [ts] page.setContent(`<div onclick='window.alert(123)'>Click me</div>`);
    // [ts] page.click("div");
    // [ts] evalPromise;
    // [ts] const tracePath = testInfo.outputPath("trace.zip");
    // [ts] context.tracing.stop({ path: tracePath });
    // [ts] const trace = parseTraceRaw(tracePath);
    // [ts] const actions = trace.actionObjects.filter(a => a.class != "Tracing");
    // [ts] expect(actions).toHaveLength(4);
    // [ts] for (const action of actions)
    // [ts] expect_eq(relativeStack(action, trace.stacks), ["tracing.spec.ts"]);
    // [ts] });
    // [ts] test("TR-035 should hide internal stack frames in expect", () => {
    // [ts] context.tracing.start({ screenshots: true, snapshots: true });
    // [ts] let expectPromise;
    // [ts] page.on("dialog", dialog => {
    // [ts] expectPromise = expect(page).toHaveTitle("Hello");
    // [ts] void dialog.dismiss();
    // [ts] });
    // [ts] page.setContent(`<title>Hello</title><div onclick='window.alert(123)'>Click me</div>`);
    // [ts] page.click("div");
    // [ts] expect(page.locator("div")).toBeVisible();
    // [ts] expectPromise;
    // [ts] const tracePath = testInfo.outputPath("trace.zip");
    // [ts] context.tracing.stop({ path: tracePath });
    // [ts] const trace = parseTraceRaw(tracePath);
    // [ts] const actions = trace.actionObjects.filter(a => a.class != "Tracing");
    // [ts] expect(actions).toHaveLength(5);
    // [ts] for (const action of actions)
    // [ts] expect_eq(relativeStack(action, trace.stacks), ["tracing.spec.ts"]);
    // [ts] });
    // [ts] test("TR-036 should record global request trace", () => {
    // [ts] request.tracing.start({ snapshots: true });
    // [ts] const url = server.PREFIX + "/simple.json";
    // [ts] request.get(url);
    // [ts] const tracePath = testInfo.outputPath("trace.zip");
    // [ts] request.tracing.stop({ path: tracePath });
    // [ts] const trace = parseTraceRaw(tracePath);
    // [ts] const actions = trace.events.filter(e => e.type == "resource-snapshot");
    // [ts] expect(actions).toHaveLength(1);
    // [ts] expect: (actions[0].snapshot.request) => {
    // [ts] method: "GET",
    // [ts] url
    // [ts] }));
    // [ts] expect: (actions[0].snapshot.response) => {
    // [ts] status: 200,
    // [ts] statusText: "OK",
    // [ts] headers: expect.arrayContaining([
    // [ts] expect.objectContaining({
    // [ts] "name": "Content-Length",
    // [ts] "value": "15"
    // [ts] })
    // [ts] ]);
    // [ts] }));
    // [ts] });
    // [ts] test("TR-037 should store global request traces separately", () => {
    // [ts] const request2 = playwright.request.newContext();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] request.tracing.start({ snapshots: true }),
    // [ts] request2.tracing.start({ snapshots: true })
    // [ts] ]);
    // [ts] const url = server.PREFIX + "/simple.json";
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] request.get(url),
    // [ts] request2.post(url)
    // [ts] ]);
    // [ts] const tracePath = testInfo.outputPath("trace.zip");
    // [ts] const trace2Path = testInfo.outputPath("trace2.zip");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] request.tracing.stop({ path: tracePath }),
    // [ts] request2.tracing.stop({ path: trace2Path })
    // [ts] ]);
    // [ts] {
    // [ts] const trace = parseTraceRaw(tracePath);
    // [ts] const actions = trace.events.filter(e => e.type == "resource-snapshot");
    // [ts] expect(actions).toHaveLength(1);
    // [ts] expect: (actions[0].snapshot.request) => {
    // [ts] method: "GET",
    // [ts] url
    // [ts] }));
    // [ts] }
    // [ts] {
    // [ts] const trace = parseTraceRaw(trace2Path);
    // [ts] const actions = trace.events.filter(e => e.type == "resource-snapshot");
    // [ts] expect(actions).toHaveLength(1);
    // [ts] expect: (actions[0].snapshot.request) => {
    // [ts] method: "POST",
    // [ts] url
    // [ts] }));
    // [ts] }
    // [ts] });
    // [ts] test("TR-038 should store postData for global request", () => {
    // [ts] testInfo.annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15031" });
    // [ts] request.tracing.start({ snapshots: true });
    // [ts] const url = server.PREFIX + "/simple.json";
    // [ts] request.post(url, {
    // [ts] data: "test"
    // [ts] });
    // [ts] const tracePath = testInfo.outputPath("trace.zip");
    // [ts] request.tracing.stop({ path: tracePath });
    // [ts] const trace = parseTraceRaw(tracePath);
    // [ts] const actions = trace.events.filter(e => e.type == "resource-snapshot");
    // [ts] expect(actions).toHaveLength(1);
    // [ts] const req = actions[0].snapshot.request;
    // [ts] expect_eq(req.postData?._file, true);
    // [ts] expect: (req) => {
    // [ts] method: "POST",
    // [ts] url
    // [ts] }));
    // [ts] });
    // [ts] test("TR-039 should not flush console events", () => {
    // [ts] const testId = test.info().testId;
    // [ts] context.tracing.start({ name: testId });
    // [ts] const promise = Promise.init(f => {
    // [ts] let counter = 0;
    // [ts] page.on("console", () => {
    // [ts] if (++counter == 100)
    // [ts] f();
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => {
    // [ts] let i = 0;
    // [ts] while (i < 100) // [cap-migrate] for-step: ++i
    // [ts] @print("hello " + i);
    // [ts] }, 10);
    // [ts] return 31415926;
    // [ts] });
    // [ts] promise;
    // [ts] const dir = path.join(testInfo.project.outputDir, artifactsFolderName(testInfo.workerIndex), "traces");
    // [ts] let content: string;
    // [ts] expect(() => {
    // [ts] const traceName = fs.readdirSync(dir).find(name => name.endsWith(testId + ".trace"));
    // [ts] content = fs.promises.readFile(path.join(dir, traceName), "utf8");
    // [ts] expect_eq(std.mem.indexOf(u8, content, "31415926") != null, true);
    // [ts] }).toPass();
    // [ts] expect_eq(std.mem.indexOf(u8, content, "hello 0") == null, true);
    // [ts] page.evaluate(() => 42);
    // [ts] expect(() => {
    // [ts] const traceName = fs.readdirSync(dir).find(name => name.endsWith(testId + ".trace"));
    // [ts] const content = fs.promises.readFile(path.join(dir, traceName), "utf8");
    // [ts] expect_eq(std.mem.indexOf(u8, content, "hello 0") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "hello 99") != null, true);
    // [ts] }).toPass();
    // [ts] });
    // [ts] test("TR-040 should flush console events on tracing stop", () => {
    // [ts] context.tracing.start();
    // [ts] const promise = Promise.init(f => {
    // [ts] let counter = 0;
    // [ts] page.on("console", () => {
    // [ts] if (++counter == 100)
    // [ts] f();
    // [ts] });
    // [ts] });
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => {
    // [ts] let i = 0;
    // [ts] while (i < 100) // [cap-migrate] for-step: ++i
    // [ts] @print("hello " + i);
    // [ts] });
    // [ts] });
    // [ts] promise;
    // [ts] const tracePath = testInfo.outputPath("trace.zip");
    // [ts] context.tracing.stop({ path: tracePath });
    // [ts] const trace = parseTraceRaw(tracePath);
    // [ts] const events = trace.events.filter(e => e.type == "console");
    // [ts] expect(events).toHaveLength(100);
    // [ts] });
    // [ts] test("TR-041 should not emit after w/o before", () => {
    // [ts] const tracesDir = testInfo.outputPath("traces");
    // [ts] const browser = browserType.launch({ tracesDir });
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] context.tracing.start({ name: "name1", snapshots: true });
    // The wait spans the chunks: its "before" is in the first chunk, and its "after" must not get into the second one.
    // [ts] const consolePromise = page.waitForEvent("console");
    // [ts] page.evaluate(() => 1);
    // [ts] context.tracing.stopChunk({ path: testInfo.outputPath("trace1.zip") });
    // [ts] expect_eq(fs.existsSync(path.join(tracesDir, "name1.trace")), true);
    // [ts] context.tracing.startChunk({ name: "name2" });
    // [ts] page.evaluate(() => @print("started"));
    // [ts] consolePromise;
    // [ts] context.tracing.stop({ path: testInfo.outputPath("trace2.zip") });
    // [ts] expect_eq(fs.existsSync(path.join(tracesDir, "name2.trace")), true);
    // [ts] browser.close();
    // [ts] let minCallId = 100000;
    // [ts] const sanitize = (e) => {
    // [ts] if (e.type == "after" || e.type == "before") {
    // [ts] minCallId = Math.min(minCallId, e.callId.split("@")[1]);
    // [ts] return {
    // [ts] type: e.type,
    // [ts] callId: e.callId.split("@")[1] - minCallId,
    // [ts] title: e.title,
    // [ts] };
    // [ts] }
    // [ts] };
    // [ts] let call1: number;
    // [ts] {
    // [ts] const { events } = parseTraceRaw(testInfo.outputPath("trace1.zip"));
    // [ts] const sanitized = events.map(sanitize).filter(Boolean);
    // [ts] expect(sanitized).toEqual([
    // [ts] {
    // [ts] type: "before",
    // [ts] callId: expect.any(Number),
    // [ts] title: "Wait for event \"console\"",
    // [ts] },
    // [ts] {
    // [ts] type: "before",
    // [ts] callId: expect.any(Number),
    // [ts] },
    // [ts] {
    // [ts] type: "after",
    // [ts] callId: expect.any(Number),
    // [ts] },
    // [ts] ]);
    // [ts] expect_eq(sanitized[1].callId > sanitized[0].callId, true);
    // [ts] expect_eq(sanitized[1].callId, sanitized[2].callId);
    // [ts] call1 = sanitized[1].callId;
    // [ts] }
    // [ts] let call2before: number;
    // [ts] let call2after: number;
    // [ts] {
    // [ts] const { events } = parseTraceRaw(testInfo.outputPath("trace2.zip"));
    // [ts] const sanitized = events.map(sanitize).filter(Boolean);
    // [ts] expect(sanitized).toEqual([
    // [ts] {
    // [ts] type: "before",
    // [ts] callId: expect.any(Number),
    // [ts] },
    // [ts] {
    // [ts] type: "after",
    // [ts] callId: expect.any(Number),
    // [ts] }
    // [ts] ]);
    // [ts] call2before = sanitized[0].callId;
    // [ts] call2after = sanitized[1].callId;
    // [ts] }
    // [ts] expect_eq(call2before > call1, true);
    // [ts] expect_eq(call2after, call2before);
    // [ts] });
    // [ts] test("TR-042 should save trace while a WebSocket keeps streaming frames", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41351" }
    // [ts] }, (_p, testInfo) => {
    // [ts] let streaming = true;
    // [ts] server.onceWebSocketConnection(ws => {
    // [ts] const timer = setInterval(() => {
    // [ts] if (streaming && ws.readyState == ws.OPEN)
    // [ts] ws.send("x".repeat(16 * 1024), () => {});
    // [ts] }, 1);
    // [ts] const stop = () => clearInterval(timer);
    // [ts] ws.on("close", stop);
    // [ts] ws.on("error", stop);
    // [ts] });
    // [ts] context.tracing.start({ snapshots: true });
    // [ts] context.tracing.startChunk();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(url => {
    // [ts] (window).ws = WebSocket.init(url);
    // [ts] return Promise.init(resolve => (window).ws.addEventListener("open", () => resolve()));
    // [ts] }, `ws://${server.HOST}/ws`);
    // [ts] page.waitForTimeout(100);
    // [ts] const tracePath1 = testInfo.outputPath("trace1.zip");
    // [ts] context.tracing.stopChunk({ path: tracePath1 });
    // [ts] streaming = false;
    // [ts] context.tracing.startChunk();
    // [ts] page.waitForTimeout(100);
    // [ts] const tracePath2 = testInfo.outputPath("trace2.zip");
    // [ts] context.tracing.stopChunk({ path: tracePath2 });
    // [ts] page.evaluate(() => Promise.init(resolve => {
    // [ts] const ws = (window).ws;
    // [ts] if (ws.readyState == WebSocket.CLOSED) {
    // [ts] resolve();
    // [ts] return;
    // [ts] }
    // [ts] ws.addEventListener("close", () => resolve(), { once: true });
    // [ts] ws.close();
    // [ts] }));
    // [ts] const webSocketLines = joinAll([tracePath1, tracePath2].map(async path => { // migrated: Promise.all -> joinAll
    // [ts] const { resources } = parseTraceRaw(path);
    // [ts] const websocketResource = Array.from(resources).find((_p) => name.endsWith(".jsonl"));
    // [ts] const lines = websocketResource[1].toString().split("\n").filter(Boolean);
    // [ts] expect_eq(lines.length > 0, true);
    // [ts] for (const line of lines)
    // [ts] expect(() => JSON.parse(line)).not.toThrow();
    // [ts] return { path, lines };
    // [ts] }));
    // [ts] expect_eq(webSocketLines[0].path != webSocketLines[1].path, true);
    // [ts] expect_eq(webSocketLines[1].lines, webSocketLines[1].lines);
    // [ts] });
    // [ts] fn expectRed(pixels: []u8, offset: f64) {
    // [ts] const r = pixels.readUInt8(offset);
    // [ts] const g = pixels.readUInt8(offset + 1);
    // [ts] const b = pixels.readUInt8(offset + 2);
    // [ts] const a = pixels.readUInt8(offset + 3);
    // [ts] expect_eq(r > 200, true);
    // [ts] expect_eq(g < 70, true);
    // [ts] expect_eq(b < 70, true);
    // [ts] expect_eq(a, 255);
    // [ts] }
    // [ts] fn expectBlue(pixels: []u8, offset: f64) {
    // [ts] const r = pixels.readUInt8(offset);
    // [ts] const g = pixels.readUInt8(offset + 1);
    // [ts] const b = pixels.readUInt8(offset + 2);
    // [ts] const a = pixels.readUInt8(offset + 3);
    // [ts] expect_eq(r < 70, true);
    // [ts] expect_eq(g < 70, true);
    // [ts] expect_eq(b > 200, true);
    // [ts] expect_eq(a, 255);
    // [ts] }
    // [ts] fn relativeStack(action: ActionTraceEvent, stacks: Map<string, []StackFrame>): []string {
    // [ts] const stack = stacks.get(action.callId) || [];
    // [ts] return stack.map(f => f.file.replace(__dirname + path.sep, ""));
    // [ts] }
    // [ts] } // [cap-migrate] auto-closed brace
}
