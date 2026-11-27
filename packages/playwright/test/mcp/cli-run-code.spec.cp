// Migrated from TypeScript (mcp/cli-run-code.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";




  const { output } = cli("eval", "async () => { const r = await fetch(\"/api/slow\"); return r.text(); }");
  expect_eq(std.mem.indexOf(u8, output, "loaded") != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });

  expect_eq(output, "### Error\nError: route handler failed");

  server.setContent("/next", "<title>Next</title>", "text/html");
  const { output: gotoOutput } = cli("goto", server.PREFIX + "/next");
  expect_eq(std.mem.indexOf(u8, gotoOutput, "Page Title: Next") != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("run-code inline", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="document.title = 'clicked'">Submit</button>
    // [ts] `, 'text/html');
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("run-code", "async (page) => { await page.getByRole(\"button\", { name: \"Submit\" }).click(); return await page.title(); }");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "clicked") != null, true);
    // [ts] });
    // [ts] test("run-code from file", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="document.title = 'from-file'">Submit</button>
    // [ts] `, 'text/html');
    // [ts] cli("open", server.PREFIX);
    // [ts] const code = "async (page) => { await page.getByRole(\"button\", { name: \"Submit\" }).click(); return await page.title(); }";
    // [ts] fs.promises.writeFile(test.info().outputPath("script.js"), code);
    // [ts] const { output } = cli("run-code", "--filename=script.js");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "from-file") != null, true);
    // [ts] });
    // [ts] test("run-code from file with template literals", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="document.title = 'templated'">Submit</button>
    // [ts] `, 'text/html');
    // [ts] cli("open", server.PREFIX);
    // [ts] const code = [
    // [ts] "async (page) => {",
    // [ts] "  await page.getByRole(\"button\", { name: \"Submit\" }).click();",
    // [ts] "  const title = await page.title();",
    // [ts] "  return `result: ${title}`;",
    // [ts] "}",
    // [ts] ].join("\n");
    // [ts] fs.promises.writeFile(test.info().outputPath("template-script.js"), code);
    // [ts] const { output } = cli("run-code", "--filename=template-script.js");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "result: templated") != null, true);
    // [ts] });
    // [ts] test("run-code delayed route with setTimeout", () => {
    // [ts] server.setContent("/", "<div>Hello</div>", "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] cli("run-code", `async page => {
    // [ts] page.route('**/api/slow', async route => {
    // [ts] new Promise(r => setTimeout(r, 100));
    // [ts] route.fulfill({ body: 'loaded' });
    // [ts] });
    // [ts] }`);
    // [ts] test("run-code reports route handler exception", () => {
    // [ts] server.setContent("/", "<div>Hello</div>", "text/html");
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("run-code", `async page => {
    // [ts] page.route('**/route-throws', async route => {
    // [ts] throw new Error('route handler failed');
    // [ts] });
    // [ts] page.goto('${server.PREFIX}/route-throws', { timeout: 5000 });
    // [ts] }`);
}
