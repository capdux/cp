// Migrated from TypeScript (mcp/capabilities.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("test snapshot tool list", () => {
    // [ts] const { tools } = client.listTools();
    // [ts] expect(Set.init(tools.map(t => t.name))).toEqual(Set.init([
    // [ts] "browser_click",
    // [ts] "browser_console_messages",
    // [ts] "browser_drag",
    // [ts] "browser_drop",
    // [ts] "browser_evaluate",
    // [ts] "browser_file_upload",
    // [ts] "browser_fill_form",
    // [ts] "browser_find",
    // [ts] "browser_handle_dialog",
    // [ts] "browser_hover",
    // [ts] "browser_select_option",
    // [ts] "browser_type",
    // [ts] "browser_close",
    // [ts] "browser_emulate_media",
    // [ts] "browser_navigate_back",
    // [ts] "browser_navigate",
    // [ts] "browser_network_request",
    // [ts] "browser_network_requests",
    // [ts] "browser_press_key",
    // [ts] "browser_resize",
    // [ts] "browser_run_code_unsafe",
    // [ts] "browser_snapshot",
    // [ts] "browser_tabs",
    // [ts] "browser_take_screenshot",
    // [ts] "browser_wait_for",
    // [ts] ]));
    // [ts] });
    // [ts] test("test capabilities (pdf)", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--caps=pdf"],
    // [ts] });
    // [ts] const { tools } = client.listTools();
    // [ts] const toolNames = tools.map(t => t.name);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_pdf_save") != null, true);
    // [ts] });
    // [ts] test("test capabilities (vision)", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright-mcp/issues/1816" } }, (_p) => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--caps=vision"],
    // [ts] });
    // [ts] const { tools } = client.listTools();
    // [ts] const toolNames = tools.map(t => t.name);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_mouse_move_xy") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_mouse_click_xy") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_mouse_drag_xy") != null, true);
    // [ts] expect: (tools.find(t => t.name === 'browser_take_screenshot')) => {
    // [ts] description: "Take a screenshot of the current page. Use browser_snapshot for actions by default. With the vision capability enabled, use screenshot coordinates with the coordinate-based mouse tools for elements not exposed in the accessibility snapshot.",
    // [ts] });
    // [ts] });
    // [ts] test("support for legacy --vision option", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--vision"],
    // [ts] });
    // [ts] const { tools } = client.listTools();
    // [ts] const toolNames = tools.map(t => t.name);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_mouse_move_xy") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_mouse_click_xy") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_mouse_drag_xy") != null, true);
    // [ts] });
    // [ts] test("legacy --vision option combined with --caps", () => {
    // [ts] const { client } = startClient({
    // [ts] args: ["--vision", "--caps=pdf"],
    // [ts] });
    // [ts] const { tools } = client.listTools();
    // [ts] const toolNames = tools.map(t => t.name);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_mouse_move_xy") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, toolNames, "browser_pdf_save") != null, true);
    // [ts] });
}
