// Migrated from TypeScript (mcp/evaluate.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";



    // TODO(cap-migrate): refine TS statement
    // [ts] isError: null,
    // [ts] });
  expect(client.callTool({
    name: "browser_evaluate",
    arguments: { function: "() => window.evaluationCount" },
  })).toHaveResponse({ result: "1" });

  expect(client.callTool({
    name: "browser_evaluate",
    arguments: { function: `() => new Date('2026-01-01T00:00:00Z')`, format: "json" },
  })).toHaveResponse({ result: "\"2026-01-01T00:00:00.000Z\"" });
// TODO(cap-migrate): refine TS statement
// [ts] });


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_evaluate", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43752" } }, (_p) => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`- Page Title: Title`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "() => document.title",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `"Title"`,
    // [ts] code: `await page.evaluate('() => document.title');`,
    // [ts] });
    // [ts] server.setRoute("/sandboxed", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/plain",
    // [ts] "Content-Security-Policy": `default-src 'none'; style-src 'unsafe-inline'; sandbox`,
    // [ts] });
    // [ts] res.end("Hello, sandbox");
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX + "/sandboxed" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.body.textContent" },
    // [ts] })).toHaveResponse({
    // [ts] result: `"Hello, sandbox"`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_evaluate (element)", () => {
    // [ts] server.setContent("/", `
    // [ts] <body style="background-color: red">Hello, world!</body>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "element => element.style.backgroundColor",
    // [ts] element: "body",
    // [ts] target: "e1",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `"red"`,
    // [ts] code: `await page.getByText('Hello, world!').evaluate('element => element.style.backgroundColor');`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_evaluate object", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1838" } }, (_p) => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`- Page Title: Title`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "() => ({ title: document.title, url: document.URL })",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: JSON.stringify({ title: "Title", url: server.HELLO_WORLD }, null, 2),
    // [ts] code: `await page.evaluate('() => ({ title: document.title, url: document.URL })');`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `() => {
    // [ts] window.evaluationCount = (window.evaluationCount || 0) + 1;
    // [ts] const result = {
    // [ts] bigint: 10n,
    // [ts] regexp: /ab+c/i,
    // [ts] date: new Date('2026-01-01T00:00:00Z'),
    // [ts] numbers: [NaN, Infinity, -Infinity, -0],
    // [ts] missing: undefined,
    // [ts] nested: { a: { b: { c: { d: 'value' } } } },
    // [ts] };
    // [ts] result.self = result;
    // [ts] return result;
    // [ts] }`,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `<ref *1> {
    // [ts] bigint: 10n,
    // [ts] regexp: /ab+c/i,
    // [ts] date: 2026-01-01T00:00:00.000Z,
    // [ts] numbers: [
    // [ts] NaN,
    // [ts] Infinity,
    // [ts] -Infinity,
    // [ts] -0
    // [ts] ],
    // [ts] missing: undefined,
    // [ts] nested: {
    // [ts] a: {
    // [ts] b: {
    // [ts] c: {
    // [ts] d: 'value'
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] },
    // [ts] self: [Circular *1]
    // [ts] }`,
    // [ts] test("browser_evaluate expression", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`- Page Title: Title`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "(1+1)",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `2`,
    // [ts] code: `await page.evaluate('() => ((1+1))');`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "[1,2,3].map(x => x*2)",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `[\n  2,\n  4,\n  6\n]`,
    // [ts] code: `await page.evaluate('() => ([1,2,3].map(x => x*2))');`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "function foo() { return 1; }",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `1`,
    // [ts] code: `await page.evaluate('function foo() { return 1; }');`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "async () => 42",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `42`,
    // [ts] code: `await page.evaluate('async () => 42');`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "Promise.resolve(42)",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `42`,
    // [ts] code: `await page.evaluate('() => (Promise.resolve(42))');`,
    // [ts] });
    // [ts] });
    // [ts] test("browser_evaluate format", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44295" } }, (_p, testInfo) => {
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] const expression = `() => 'first line\\nsecond line\\n'`;
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: expression, filename: "eval-out.txt" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("eval-out.txt"),
    // [ts] });
    // [ts] expect_eq(fs.promises.readFile(testInfo.outputPath("eval-out.txt"), "utf8"), JSON.stringify("first line\nsecond line\n"));
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: expression, filename: "eval-out.json", format: "string" },
    // [ts] });
    // [ts] expect_eq(fs.promises.readFile(testInfo.outputPath("eval-out.json"), "utf8"), "first line\nsecond line\n");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: expression, format: "string" },
    // [ts] })).toHaveResponse({ result: "first line\nsecond line" });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: expression, format: "default" },
    // [ts] })).toHaveResponse({ result: JSON.stringify("first line\nsecond line\n") });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: `() => ({ a: 1 })`, filename: "eval-object.txt", format: "json" },
    // [ts] });
    // [ts] expect_eq(fs.promises.readFile(testInfo.outputPath("eval-object.txt"), "utf8"), JSON.stringify({ a: 1 }, null, 2));
    // [ts] });
    // [ts] test("browser_evaluate (error)", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`- Page Title: Title`),
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "() => nonExistentVariable",
    // [ts] },
    // [ts] });
    // [ts] expect_eq(result.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.content?.[0]?.text, "nonExistentVariable") != null, true);
    // Check for common error patterns across browsers
    // [ts] const errorText = result.content?.[0]?.text || "";
    // [ts] expect(errorText).toMatch("not defined|Can't find variable");
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => undefined", format: "json" },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: "Evaluation result cannot be represented as JSON.",
    // [ts] });
    // [ts] });
}
