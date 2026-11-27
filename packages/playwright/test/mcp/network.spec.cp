// Migrated from TypeScript (mcp/network.spec.ts) via cap from -ts

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



  {
    const response = parseResponse(client.callTool({
      name: "browser_network_requests",
      arguments: { filter: "/api/", static: true },
    }));
    expect_eq(std.mem.indexOf(u8, response.result, `${server.PREFIX}/api/users`) != null, true);
    expect_eq(std.mem.indexOf(u8, response.result, `${server.PREFIX}/api/orders`) != null, true);
    expect_eq(std.mem.indexOf(u8, response.result, `${server.PREFIX}/static/image.png`) == null, true);
  }
// TODO(cap-migrate): refine TS statement
// [ts] });



  // Index assignment is stable across calls — the same request keeps the same number.
  const response = parseResponse(client.callTool({
    name: "browser_network_requests",
    arguments: { static: true },
  }));
  const lines = response.result.split("\n").filter(Boolean);
  expect(lines[0]).toMatch("^1\\. \\[GET\\] ");
  expect(lines).toHaveLength(3);
// TODO(cap-migrate): refine TS statement
// [ts] });








fn escapeRegExp(text: string): string {
  return text.replace("[.*+?^${}()|[\\]\\\\]", "\\$&");
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_network_requests", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/json')">Click me</button>
    // [ts] <img src="/image.png" />
    // [ts] `, 'text/html');
    // [ts] server.setContent("/json", JSON.stringify({ name: "John Doe" }), "application/json");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: {
    // [ts] element: "Click me button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] });
    // [ts] {
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, response.result, `[GET] ${`${server.PREFIX}/`} => [200] OK`) == null, true);
    // [ts] expect(response.result).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/json`)} => \[200\] OK$`, "m"));
    // [ts] expect(response.result).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/image.png`)} => \[404\]`, "m"));
    // [ts] expect_eq(std.mem.indexOf(u8, response.result, "Note: 1 static request not shown, run with \"static\" option to see it.") != null, true);
    // [ts] }
    // [ts] {
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: {
    // [ts] static: true,
    // [ts] },
    // [ts] }));
    // [ts] expect(response.result).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/`)} => \[200\] OK$`, "m"));
    // [ts] expect(response.result).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/json`)} => \[200\] OK$`, "m"));
    // [ts] expect(response.result).toMatch(RegExp.init(String.raw`^\d+\. \[GET\] ${escapeRegExp(`${server.PREFIX}/image.png`)} => \[404\]`, "m"));
    // [ts] expect_eq(std.mem.indexOf(u8, response.result, "not shown") == null, true);
    // [ts] }
    // [ts] });
    // [ts] test("browser_network_requests filter", () => {
    // [ts] server.setContent("/", "", "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `async () => {
    // [ts] Promise.all([fetch('/api/users'), fetch('/api/orders'), fetch('/static/image.png')]);
    // [ts] }`,
    // [ts] },
    // [ts] });
    // [ts] test("browser_network_requests rejects invalid regex filter", () => {
    // [ts] server.setContent("/", "", "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: { filter: "[invalid(" },
    // [ts] });
    // [ts] expect_eq(response.isError, true);
    // [ts] });
    // [ts] test("browser_network_requests numbers requests with stable indexes", () => {
    // [ts] server.setContent("/", "", "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: `async () => {
    // [ts] fetch('/api/users');
    // [ts] fetch('/api/orders');
    // [ts] }`,
    // [ts] },
    // [ts] });
    // [ts] test("browser_network_requests all option", () => {
    // [ts] server.setContent("/api/page1", "page1", "text/plain");
    // [ts] server.setContent("/api/page2", "page2", "text/plain");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.EMPTY_PAGE } });
    // [ts] client.callTool({ name: "browser_evaluate", arguments: { function: `async () => { await fetch('/api/page1'); }` } });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.EMPTY_PAGE } });
    // [ts] client.callTool({ name: "browser_evaluate", arguments: { function: `async () => { await fetch('/api/page2'); }` } });
    // [ts] const defaultResponse = parseResponse(client.callTool({ name: "browser_network_requests" }));
    // [ts] expect_eq(std.mem.indexOf(u8, defaultResponse.result, "/api/page1") == null, true);
    // [ts] const allResponse = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: { all: true },
    // [ts] }));
    // [ts] const page1Index = allResponse.result.match(RegExp.init(String.raw`^(\d+)\. \[GET\] ${escapeRegExp(`${server.PREFIX}/api/page1`)} => \[200\] OK$`, "m"))?.[1];
    // [ts] expect_eq(page1Index, true);
    // Requests keep their numbers across navigations.
    // [ts] const page2Line = allResponse.result.split("\n").find(line => line.includes("/api/page2"));
    // [ts] expect_eq(std.mem.indexOf(u8, defaultResponse.result, page2Line) != null, true);
    // [ts] const details = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index: Number(page1Index) },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, details.result, `#${page1Index} [GET] ${server.PREFIX}/api/page1`) != null, true);
    // [ts] });
    // [ts] test("browser_network_request shows full request and response details", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/api', { method: 'POST', headers: { 'X-Custom-Header': 'test-value' }, body: JSON.stringify({ key: 'value' }) })">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setRoute("/api", (_req, res) => {
    // [ts] res.setHeader("X-Custom-Response", "response-value");
    // [ts] res.setHeader("Content-Type", "application/json");
    // [ts] res.end(JSON.stringify({ name: "John Doe" }));
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Click me button", target: "e2" },
    // [ts] });
    // Find the index of the /api request from the listing.
    // [ts] const list = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] }));
    // [ts] const match_val = list.result.match("^(\\d+)\\. \\[POST\\] [^ ]+\\/api =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const index = Number(match_val[1]);
    // [ts] const detail = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, `#${index} [POST] ${server.PREFIX}/api`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "General") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "status:    [200] OK") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "mimeType:  application/json") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "Request headers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "x-custom-header: test-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "Response headers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "x-custom-response: response-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "Call browser_network_request with part=\"request-body\" to read the request body.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "Call browser_network_request with part=\"response-body\" to read the response body.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "Request body") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "Response body") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "{\"key\":\"value\"}") == null, true);
    // [ts] });
    // [ts] test("browser_network_request reports failed requests", () => {
    // [ts] server.setContent("/", `<img src="/missing.png" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const list = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: { static: true },
    // [ts] }));
    // [ts] const match_val = list.result.match("^(\\d+)\\. \\[GET\\] [^ ]+\\/missing\\.png =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const detail = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index: Number(match_val[1]) },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, `#${match_val[1]} [GET] ${server.PREFIX}/missing.png`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, "status:    [404]") != null, true);
    // [ts] });
    // [ts] test("browser_network_requests lists a failed request once", () => {
    // [ts] server.setContent("/", `<img src="http://does-not-exist.invalid/api/x" />`, "text/html");
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const list = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: { static: true },
    // [ts] }));
    // [ts] expect([...list.result.matchAll("\\/api\\/x =>")]).toHaveLength(1);
    // [ts] });
    // [ts] test("browser_network_request returns individual parts", () => {
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/api', { method: 'POST', headers: { 'X-Custom-Header': 'test-value' }, body: JSON.stringify({ key: 'value' }) })">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setRoute("/api", (_req, res) => {
    // [ts] res.setHeader("X-Custom-Response", "response-value");
    // [ts] res.setHeader("Content-Type", "application/json");
    // [ts] res.end(JSON.stringify({ name: "John Doe" }));
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Click me button", target: "e2" },
    // [ts] });
    // [ts] const list = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] }));
    // [ts] const match_val = list.result.match("^(\\d+)\\. \\[POST\\] [^ ]+\\/api =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const index = Number(match_val[1]);
    // [ts] const requestHeaders = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index, part: "request-headers" },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, requestHeaders.result, "x-custom-header: test-value") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, requestHeaders.result, "General") == null, true);
    // [ts] const requestBody = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index, part: "request-body" },
    // [ts] }));
    // [ts] expect_eq(requestBody.result, "{\"key\":\"value\"}");
    // [ts] const responseHeaders = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index, part: "response-headers" },
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, responseHeaders.result, "x-custom-response: response-value") != null, true);
    // [ts] const responseBody = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index, part: "response-body" },
    // [ts] }));
    // [ts] expect_eq(responseBody.result, "{\"name\":\"John Doe\"}");
    // [ts] });
    // [ts] test("browser_network_request response-body part saves binary to a file", () => {
    // [ts] const pngBytes = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/image.png')">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setRoute("/image.png", (_req, res) => {
    // [ts] res.setHeader("Content-Type", "image/png");
    // [ts] res.end(pngBytes);
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Click me button", target: "e2" },
    // [ts] });
    // [ts] const list = parseResponse(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] arguments: { static: true },
    // [ts] }));
    // [ts] const match_val = list.result.match("^(\\d+)\\. \\[GET\\] [^ ]+\\/image\\.png =>");
    // [ts] expect_eq(match_val != null, true);
    // [ts] const detail = parseResponse(client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index: Number(match_val[1]), part: "response-body" },
    // [ts] }));
    // [ts] const bodyPath = path.resolve(test.info().outputPath(), detail.result.trim());
    // [ts] expect_eq(fs.readFileSync(bodyPath), pngBytes);
    // [ts] });
    // [ts] test("browser_network_request rejects out-of-range index", () => {
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_network_request",
    // [ts] arguments: { index: 999 },
    // [ts] });
    // [ts] const parsed = parseResponse(response);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, "Request #999 not found") != null, true);
    // [ts] });
}
