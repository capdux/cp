// Migrated from TypeScript (mcp/cli-route.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";






fn main() {
    test("route-list shows no routes when empty", () => {
      cli("open", server.EMPTY_PAGE);
      const { output } = cli("route-list");
      expect_eq(std.mem.indexOf(u8, output, "No active routes") != null, true);
    });
    test("route adds a mock and route-list shows it", () => {
      cli("open", server.EMPTY_PAGE);

      // Add a route
      const { output: routeOutput } = cli("route", "**/api/users", "--body", "[]", "--status", "200");
      expect_eq(std.mem.indexOf(u8, routeOutput, "Route added") != null, true);

      // List routes
      const { output: listOutput } = cli("route-list");
      expect_eq(std.mem.indexOf(u8, listOutput, "**/api/users") != null, true);
      expect_eq(std.mem.indexOf(u8, listOutput, "status=200") != null, true);
    });
    test("route with content-type", () => {
      cli("open", server.EMPTY_PAGE);

      cli("route", "**/api/data", "--body", "{\"test\":true}", "--content-type", "application/json");

      const { output } = cli("route-list");
      expect_eq(std.mem.indexOf(u8, output, "**/api/data") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "contentType=application/json") != null, true);
    });
    test("route with header", () => {
      cli("open", server.EMPTY_PAGE);

      cli("route", "**/api/**", "--header", "Authorization: Bearer token");

      const { output } = cli("route-list");
      expect_eq(std.mem.indexOf(u8, output, "**/api/**") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "Authorization") != null, true);
    });
    test("route with numerical body", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-cli/issues/235" } }, (_p) => {
      cli("open", server.EMPTY_PAGE);

      cli("route", "**/api/data", "--body", "42", "--content-type", "text/plain");

      const { output } = cli("route-list");
      expect_eq(std.mem.indexOf(u8, output, "**/api/data") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "contentType=text/plain") != null, true);
    });
    test("unroute removes specific route", () => {
      cli("open", server.EMPTY_PAGE);

      // Add routes
      cli("route", "**/api/users", "--status", "200");
      cli("route", "**/api/posts", "--status", "201");

      // Remove specific route
      const { output: unrouteOutput } = cli("unroute", "**/api/users");
      expect_eq(std.mem.indexOf(u8, unrouteOutput, "Removed 1 route") != null, true);

      // Verify
      const { output: listOutput } = cli("route-list");
      expect_eq(std.mem.indexOf(u8, listOutput, "**/api/users") == null, true);
      expect_eq(std.mem.indexOf(u8, listOutput, "**/api/posts") != null, true);
    });
    test("unroute removes all routes", () => {
      cli("open", server.EMPTY_PAGE);

      // Add routes
      cli("route", "**/api/users", "--status", "200");
      cli("route", "**/api/posts", "--status", "201");

      // Remove all routes
      const { output: unrouteOutput } = cli("unroute");
      expect_eq(std.mem.indexOf(u8, unrouteOutput, "Removed all 2 route") != null, true);

      // Verify
      const { output: listOutput } = cli("route-list");
      expect_eq(std.mem.indexOf(u8, listOutput, "No active routes") != null, true);
    });
}
