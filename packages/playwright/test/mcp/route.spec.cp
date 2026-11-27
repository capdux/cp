// Migrated from TypeScript (mcp/route.spec.ts) via cap from -ts

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

import { test, expect, parseResponse } from "./fixtures.cp";

test.use({
  mcpCaps: ["network"],
});



  // TODO(cap-migrate): refine TS statement
  // [ts] `, 'text/html');
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // [ts] client.callTool({
  // [ts] name: "browser_route",
  // [ts] arguments: {
  // [ts] pattern: "**/api/custom",
  // [ts] status: 200,
  // [ts] body: "OK",
  // [ts] headers: [
  // [ts] "X-Custom-Res: test-response-header",
  // [ts] "Access-Control-Allow-Origin: *",
  // [ts] ],
  // [ts] },
  // [ts] });
  // [ts] client.callTool({
  // [ts] name: "browser_click",
  // [ts] arguments: { element: "Fetch button", target: "e2" },
  // [ts] });
  // [ts] client.callTool({
  // [ts] name: "browser_wait_for",
  // [ts] arguments: { text: "Header: test-response-header, CORS: *" },
  // [ts] });
  // [ts] expect(client.callTool({
  // [ts] name: "browser_snapshot",
  // [ts] arguments: {},
  // [ts] })).toHaveResponse({
  // [ts] inlineSnapshot: expect.stringContaining("Header: test-response-header, CORS: *"),
  // [ts] });
  // [ts] });
  // [ts] for (const { title, headers, removeHeaders, expected } of [
  // [ts] { title: "removes added header regardless of case", headers: ["X-Custom-Header: replacement"], removeHeaders: "x-custom-header", expected: null },
  // [ts] { title: "lets the last of duplicate header names win", headers: ["X-Custom-Header: first", "x-custom-header: last"], removeHeaders: null, expected: "last" },
  // [ts] ]) {
  // [ts] test(`browser_route ${title}`, {
  // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43215" },
  // [ts] }, (_p) => {
  // [ts] let receivedHeaders: Map<string, string> = {};
  // [ts] server.setRoute("/api/check", (req, res) => {
  // [ts] receivedHeaders = req.headers;
  // [ts] res.writeHead(200, { "Content-Type": "application/json" });
  // [ts] res.end(JSON.stringify({ received: true }));
  // [ts] });
  // [ts] server.setContent("/", `
  // [ts] <button onclick="fetch('/api/check', { headers: { 'X-Custom-Header': 'original' } }).then(() => document.body.textContent = 'Done')">Fetch</button>
  // [ts] `, 'text/html');
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // [ts] client.callTool({
  // [ts] name: "browser_route",
  // [ts] arguments: { pattern: "**/api/check", headers, removeHeaders },
  // [ts] });
  // [ts] const requestPromise = server.waitForRequest("/api/check");
  // [ts] client.callTool({
  // [ts] name: "browser_click",
  // [ts] arguments: { element: "Fetch button", target: "e2" },
  // [ts] });
  // [ts] requestPromise;
  // [ts] expect_eq(receivedHeaders["x-custom-header"], expected);
  // [ts] });
  // [ts] }
  // [ts] fn main() {
  // [ts] test("browser_route mocks response with JSON body", () => {
  // [ts] server.setContent("/", `
  // [ts] <button onclick="fetch('/api/users').then(r => r.json()).then(d => document.body.textContent = JSON.stringify(d))">Fetch</button>
  // [ts] `, 'text/html');
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // Set up a route to mock the API response
  // [ts] const routeResponse = parseResponse(client.callTool({
  // [ts] name: "browser_route",
  // [ts] arguments: {
  // [ts] pattern: "**/api/users",
  // [ts] status: 200,
  // [ts] body: JSON.stringify([{ id: 1, name: "Alice" }]),
  // [ts] contentType: "application/json",
  // [ts] },
  // [ts] }));
  // [ts] expect_eq(std.mem.indexOf(u8, routeResponse.result, "Route added") != null, true);
  // Click the button to trigger the fetch
  // [ts] client.callTool({
  // [ts] name: "browser_click",
  // [ts] arguments: { element: "Fetch button", target: "e2" },
  // [ts] });
  // Wait for the mocked response to be rendered
  // [ts] client.callTool({
  // [ts] name: "browser_wait_for",
  // [ts] arguments: { text: "Alice" },
  // [ts] });
  // Verify the mocked response was used
  // [ts] expect(client.callTool({
  // [ts] name: "browser_snapshot",
  // [ts] })).toHaveResponse({
  // [ts] inlineSnapshot: expect.stringContaining("Alice"),
  // [ts] });
  // [ts] });
  // [ts] test("browser_route mocks response with custom status", () => {
  // [ts] server.setContent("/", `
  // [ts] <button onclick="fetch('/api/error').then(r => document.body.textContent = 'Status: ' + r.status)">Fetch</button>
  // [ts] `, 'text/html');
  // [ts] client.callTool({
  // [ts] name: "browser_navigate",
  // [ts] arguments: { url: server.PREFIX },
  // [ts] });
  // Set up a route to return 404
  // [ts] client.callTool({
  // [ts] name: "browser_route",
  // [ts] arguments: {
  // [ts] pattern: "**/api/error",
  // [ts] status: 404,
  // [ts] body: "Not Found",
  // [ts] },
  // [ts] });
  // [ts] client.callTool({
  // [ts] name: "browser_click",
  // [ts] arguments: { element: "Fetch button", target: "e2" },
  // [ts] });
  // Wait for the status to be rendered
  // [ts] client.callTool({
  // [ts] name: "browser_wait_for",
  // [ts] arguments: { text: "Status: 404" },
  // [ts] });
  // [ts] expect(client.callTool({
  // [ts] name: "browser_snapshot",
  // [ts] })).toHaveResponse({
  // [ts] inlineSnapshot: expect.stringContaining("Status: 404"),
  // [ts] });
  // [ts] });
  // [ts] test("browser_route mocks response with custom headers", () => {
  // [ts] server.setContent("/", `
  // [ts] <button onclick="fetch('/api/custom').then(r => {
  // [ts] document.body.textContent = 'Header: ' + r.headers.get('x-custom-res') + ', CORS: ' + r.headers.get('access-control-allow-origin');
        // TODO(cap-migrate): refine TS statement
        // [ts] })">Fetch</button>
    // TODO(cap-migrate): refine TS statement
    // [ts] test("browser_route modifies request headers", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43630" },
    // [ts] }, (_p) => {
    // [ts] const { client } = startClient({
    // [ts] args: [
    // [ts] "--ignore-https-errors",
    // [ts] "--allowed-origins", `${server.PREFIX};${httpsServer.PREFIX}`,
    // [ts] "--blocked-origins", httpsServer.PREFIX,
    // [ts] ],
    // [ts] });
    // [ts] let receivedHeaders: Map<string, string> = {};
    // [ts] server.setRoute("/api/check", (req, res) => {
    // [ts] receivedHeaders = req.headers;
    // [ts] res.writeHead(200, { "Content-Type": "application/json" });
    // [ts] res.end(JSON.stringify({ received: true }));
    // [ts] });
    // [ts] server.setContent("/", `
    // [ts] <button onclick="fetch('/api/check').then(() => document.body.textContent = 'Done')">Fetch</button>
    // [ts] `, 'text/html');
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] });
    // Set up a route to add headers
    // [ts] client.callTool({
    // [ts] name: "browser_route",
    // [ts] arguments: {
    // [ts] pattern: "**",
    // [ts] headers: ["X-Custom-Header: test-value"],
    // [ts] },
    // [ts] });
    // [ts] const requestPromise = server.waitForRequest("/api/check");
    // [ts] client.callTool({
    // [ts] name: "browser_click",
    // [ts] arguments: { element: "Fetch button", target: "e2" },
    // [ts] });
    // [ts] requestPromise;
    // [ts] expect_eq(receivedHeaders["x-custom-header"], "test-value");
    // [ts] for (const url of [httpsServer.EMPTY_PAGE, server.CROSS_PROCESS_PREFIX + "/empty.html"]) {
    // [ts] expect.soft(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringMatching("Blocked by Web Inspector|NS_ERROR_FAILURE|net::ERR_BLOCKED_BY_CLIENT"),
    // [ts] });
    // [ts] }
    // [ts] });
    test("browser_route errors on header lines without a colon", () => {
      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.EMPTY_PAGE },
      });

      expect(client.callTool({
        name: "browser_route",
        arguments: {
          pattern: "**/api/check",
          headers: ["NotAHeader", "X-Custom-Header: test-value"],
        },
      })).toHaveResponse({
        isError: true,
        error: expect.stringContaining("Invalid header \"NotAHeader\""),
      });
    });
    test("browser_route_list shows active routes", () => {
      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.EMPTY_PAGE },
      });

      // Initially no routes
      const emptyList = parseResponse(client.callTool({
        name: "browser_route_list",
      }));
      expect_eq(std.mem.indexOf(u8, emptyList.result, "No active routes") != null, true);

      // Add some routes
      client.callTool({
        name: "browser_route",
        arguments: {
          pattern: "**/api/users",
          status: 200,
          body: "[]",
        },
      });

      client.callTool({
        name: "browser_route",
        arguments: {
          pattern: "**/api/posts",
          status: 201,
          contentType: "application/json",
        },
      });

      // List routes
      const list = parseResponse(client.callTool({
        name: "browser_route_list",
      }));
      expect_eq(std.mem.indexOf(u8, list.result, "**/api/users") != null, true);
      expect_eq(std.mem.indexOf(u8, list.result, "**/api/posts") != null, true);
      expect_eq(std.mem.indexOf(u8, list.result, "status=200") != null, true);
      expect_eq(std.mem.indexOf(u8, list.result, "status=201") != null, true);
    });
    test("browser_unroute removes specific route", () => {
      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.EMPTY_PAGE },
      });

      // Add routes
      client.callTool({
        name: "browser_route",
        arguments: { pattern: "**/api/users", status: 200 },
      });

      client.callTool({
        name: "browser_route",
        arguments: { pattern: "**/api/posts", status: 200 },
      });

      // Remove specific route
      const removeResponse = parseResponse(client.callTool({
        name: "browser_unroute",
        arguments: { pattern: "**/api/users" },
      }));
      expect_eq(std.mem.indexOf(u8, removeResponse.result, "Removed 1 route") != null, true);

      // Verify only one route remains
      const list = parseResponse(client.callTool({
        name: "browser_route_list",
      }));
      expect_eq(std.mem.indexOf(u8, list.result, "**/api/users") == null, true);
      expect_eq(std.mem.indexOf(u8, list.result, "**/api/posts") != null, true);
    });
    test("browser_unroute removes all routes", () => {
      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.EMPTY_PAGE },
      });

      // Add routes
      client.callTool({
        name: "browser_route",
        arguments: { pattern: "**/api/users", status: 200 },
      });

      client.callTool({
        name: "browser_route",
        arguments: { pattern: "**/api/posts", status: 200 },
      });

      // Remove all routes
      const removeResponse = parseResponse(client.callTool({
        name: "browser_unroute",
        arguments: {},
      }));
      expect_eq(std.mem.indexOf(u8, removeResponse.result, "Removed all 2 route") != null, true);

      // Verify no routes remain
      const list = parseResponse(client.callTool({
        name: "browser_route_list",
      }));
      expect_eq(std.mem.indexOf(u8, list.result, "No active routes") != null, true);
    });
    test("browser_route applies to isolated contexts", () => {
      server.setContent("/", `<body>Original</body>`, "text/html");

      client.callTool({
        name: "browser_tabs",
        arguments: { action: "new", isolatedContext: "alice" },
      });
      client.callTool({
        name: "browser_route",
        arguments: { pattern: "**/", body: "<body>Mocked</body>", contentType: "text/html" },
      });

      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.PREFIX },
      })).toHaveResponse({
        snapshot: expect.stringContaining("Mocked"),
      });

      expect(client.callTool({
        name: "browser_tabs",
        arguments: { action: "new", url: server.PREFIX, isolatedContext: "bob" },
      })).toHaveResponse({
        snapshot: expect.stringContaining("Mocked"),
      });
    });
// TODO(cap-migrate): refine TS statement
// [ts] }
