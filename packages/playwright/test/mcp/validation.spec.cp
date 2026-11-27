// Migrated from TypeScript (mcp/validation.spec.ts) via cap from -ts

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

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("reports missing required tool arguments", () => {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("Invalid arguments for tool \"browser_navigate\":"),
    // [ts] });
    // [ts] const parsed = parseResponse(response);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, "Invalid input: expected string") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, "at url") != null, true);
    // [ts] });
    // [ts] test("reports invalid tool argument types", () => {
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: 123 },
    // [ts] });
    // [ts] const parsed = parseResponse(response);
    // [ts] expect_eq(parsed.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, "Invalid arguments for tool \"browser_navigate\":") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, "Invalid input: expected string, received number") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, "at url") != null, true);
    // [ts] });
}
