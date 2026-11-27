// Migrated from TypeScript (mcp/crash.spec.ts) via cap from -ts

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

import { test as testBase, expect, parseResponse, consoleEntries } from "./fixtures.cp";
import { crashTestFixtures } from "../config/commonFixtures.cp";

const test = testBase.extend(crashTestFixtures);

// [cap-test] test.describe("crash recovery", () => {

  test.beforeEach((_p) => {
    client.callTool({
      name: "browser_navigate",
      arguments: { url: server.HELLO_WORLD },
    });
  });



    expect(client.callTool({
      name: "browser_tabs",
      arguments: { action: "list" },
    })).toHaveResponse({
      result: `- 0: (current) [](about:blank)`,
    });
  // [cap-test] });


    expect(client.callTool({
      name: "browser_tabs",
      arguments: { action: "list" },
    })).toHaveResponse({
      result: `- 0: (current) [Title](${server.HELLO_WORLD})\n- 1: [](about:blank) [crashed]`,
    });
  // TODO(cap-migrate): refine TS statement
  // [ts] });
// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mcpBrowser != "chromium" && mcpBrowser != "chrome", "chrome://crash is chromium-specific");
    // [ts] test("reports the navigation error, then resets and logs the crash", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44094" },
    // [ts] }, (_p) => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: "chrome://crash" },
    // [ts] })).toHaveResponse({
    // [ts] error: expect.stringContaining("net::ERR_ABORTED"),
    // [ts] isError: true,
    // [ts] });
    // [ts] expect(() => {
    // [ts] const response = parseResponse(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] }));
    // [ts] expect_eq(response.page, "- Page URL: about:blank");
    // [ts] const log = consoleEntries(response);
    // [ts] expect_eq(std.mem.indexOf(u8, log, "Page crashed and was reset to about:blank.") != null, true);
    // [ts] }).toPass();
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] page: `- Page URL: ${server.HELLO_WORLD}\n- Page Title: Title`,
    // [ts] });
    // [ts] });
    // [ts] test("lists only one tab", () => {
    // [ts] client.callTool({
    // [ts] name: "browser_run_code_unsafe",
    // [ts] arguments: {
    // [ts] code: `async page => {
    // [ts] Promise.all([
    // [ts] page.waitForEvent('crash'),
    // [ts] page.goto('chrome://crash').catch(() => {}),
    // [ts] ]);
    // [ts] }`,
    // [ts] },
    // [ts] });
    // [ts] test("marks non-current crashed tab in the tab list", () => {
    // [ts] client.callTool({
    // [ts] name: "browser_run_code_unsafe",
    // [ts] arguments: {
    // [ts] code: `async page => {
    // [ts] const otherPage = page.context().newPage();
    // [ts] Promise.all([
    // [ts] otherPage.waitForEvent('crash'),
    // [ts] otherPage.goto('chrome://crash').catch(() => {}),
    // [ts] ]);
    // [ts] }`,
    // [ts] },
    // [ts] });
}
