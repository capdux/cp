// Migrated from TypeScript (mcp/trace-cli-fixtures.ts) via cap from -ts

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
import { execFileSync } from "cap:process";

import { test as baseTest, expect } from "./fixtures.cp";
import { chromium } from "playwright";

// TODO(cap-migrate): refine TS statement
// [ts] export { expect };  type TraceCliWorkerFixtures = { traceFile: string;
// [ts] traceCwd: string;
// [ts] };

type TraceCliFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = baseTest
// [ts] .extend({
// [ts] traceFile: [async (_p, use, workerInfo) => {
// [ts] const server = __servers.server;
// Record a trace with various actions for testing.
// [ts] const browser = await chromium.launch();
// [ts] const context = await browser.newContext({ viewport: { width: 800, height: 600 } });
// [ts] await context.tracing.start({ screenshots: true, snapshots: true });
// [ts] const page = await context.newPage();
// [ts] server.setContent("/", `
// [ts] <html>
// [ts] <head>
// [ts] <title>Test Page</title>
// [ts] <link rel="stylesheet" href="/style.css">
// [ts] </head>
// [ts] <body>
// [ts] <h1>Hello World</h1>
// [ts] <button id="btn">Click me</button>
// [ts] <input id="search" type="text" placeholder="Search..." />
// [ts] <a href="/page2">Go to page 2</a>
// [ts] <div id="styled">Styled text</div>
// [ts] </body>
// [ts] </html>
// [ts] `, 'text/html');
// [ts] server.setContent("/style.css", "#styled { color: rgb(255, 0, 0); }", "text/css");
// [ts] server.setContent("/page2", `
// [ts] <html>
// [ts] <head><title>Page 2</title></head>
// [ts] <body>
// [ts] <h1>Page 2</h1>
// [ts] <iframe src="/iframe" id="frame1"></iframe>
// [ts] </body>
// [ts] </html>
// [ts] `, 'text/html');
// [ts] server.setContent("/iframe", `
// [ts] <html>
// [ts] <head><title>Iframe</title></head>
// [ts] <body>
// [ts] <p>Iframe content</p>
// [ts] <iframe src="/iframe-inner" id="frame2"></iframe>
// [ts] </body>
// [ts] </html>
// [ts] `, 'text/html');
// [ts] server.setContent("/iframe-inner", `
// [ts] <html>
// [ts] <head><title>Inner iframe</title></head>
// [ts] <body><p>Innermost</p></body>
// [ts] </html>
// [ts] `, 'text/html');
// [ts] server.setContent("/feedback", JSON.stringify({ received: true }), "application/json");
// Navigate
// [ts] await page.goto(server.PREFIX);
// Click
// [ts] await page.locator("#btn").click();
// Fill
// [ts] await page.locator("#search").fill("test query");
// Console messages
// [ts] await page.evaluate(() => {
// [ts] @print("info message");
// [ts] @print("warning message");
// [ts] @print("error message");
// [ts] });
// Fetch
// [ts] await page.evaluate(() => fetch("/feedback", { method: "POST", body: "What a great product!" }).then(res => res.text()));
// Aborted fetch
// [ts] await page.route("**/blocked", route => route.abort());
// [ts] await page.evaluate(() => fetch("/blocked").catch(() => {}));
// Navigate to another page
// [ts] await page.locator("a").click();
// Click into innermost frame
// [ts] await page.frameLocator("#frame1").frameLocator("#frame2").locator("p").click();
// [ts] await page.close();
// [ts] const tmpDir = path.join(workerInfo.project.outputDir, "pw-trace-cli-" + workerInfo.workerIndex);
// [ts] const tracePath = path.join(tmpDir, "trace.zip");
// [ts] await context.tracing.stop({ path: tracePath });
// [ts] await browser.close();
// [ts] server.reset();
// [ts] await use(tracePath);
// [ts] await fs.promises.rm(tmpDir, { recursive: true, force: true });
// [ts] }, { scope: "worker" }],
// [ts] traceCwd: [async (_p, use, workerInfo) => {
// Create a working directory and open the trace once for all tests.
// [ts] const cwd = path.join(workerInfo.project.outputDir, "pw-trace-cwd-" + workerInfo.workerIndex);
// [ts] await fs.promises.mkdir(cwd, { recursive: true });
// [ts] const cliPath = path.resolve(__dirname, "../../packages/playwright-core/cli.js");
// [ts] execFileSync(process.execPath, [cliPath, "trace", "open", traceFile], { cwd });
// [ts] await use(cwd);
// [ts] await fs.promises.rm(cwd, { recursive: true, force: true });
// [ts] }, { scope: "worker" }],
// [ts] })
    // TODO(cap-migrate): refine TS statement
    // [ts] .extend({
    // [ts] runTraceCli: async (_p, use) => {
    // [ts] await use(async (args) => {
    // [ts] const cliPath = path.resolve(__dirname, "../../packages/playwright-core/cli.js");
    // [ts] const child = childProcess({
    // [ts] command: [process.execPath, cliPath, "trace", ...args],
    // [ts] cwd: traceCwd,
    // [ts] });
    // [ts] await child.exited;
    // [ts] return {
    // [ts] stdout: child.stdout.trim(),
    // [ts] stderr: child.stderr.trim(),
    // [ts] exitCode: await child.exitCode,
    // [ts] };
    // [ts] });
    // [ts] },
    // [ts] });
