// Migrated from TypeScript (mcp/devtools.spec.ts) via cap from -ts

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

test.use({ mcpCaps: ["devtools"] });




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_highlight", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] const { client } = startClient({ args: [`--endpoint=default`] });
    // [ts] client.callTool({ name: "browser_snapshot" });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_highlight",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] })).toHaveResponse({
    // [ts] result: `Highlighted Submit button`,
    // [ts] });
    // [ts] const highlight = page.locator("x-pw-highlight");
    // [ts] const tooltip = page.locator("x-pw-tooltip-line");
    // [ts] expect(highlight).toBeVisible();
    // [ts] expect(tooltip).toHaveText(`locator('aria-ref=e2')`);
    // [ts] expect_eq(highlight.boundingBox(), page.getByRole("button", { name: "Submit" }).boundingBox());
    // [ts] });
    // [ts] test("browser_highlight with style", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] const { client } = startClient({ args: [`--endpoint=default`] });
    // [ts] client.callTool({ name: "browser_snapshot" });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_highlight",
    // [ts] arguments: {
    // [ts] element: "Submit button",
    // [ts] target: "e2",
    // [ts] style: "outline: 3px solid rgb(255, 0, 0); background-color: rgba(0, 255, 0, 0.25)",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `Highlighted Submit button`,
    // [ts] });
    // [ts] const highlight = page.locator("x-pw-highlight");
    // [ts] expect(highlight).toBeVisible();
    // [ts] expect(highlight.evaluate((el) => ({
    // [ts] outlineWidth: el.style.outlineWidth,
    // [ts] outlineStyle: el.style.outlineStyle,
    // [ts] outlineColor: el.style.outlineColor,
    // [ts] backgroundColor: el.style.backgroundColor,
    // [ts] }))).toEqual({
    // [ts] outlineWidth: "3px",
    // [ts] outlineStyle: "solid",
    // [ts] outlineColor: "rgb(255, 0, 0)",
    // [ts] backgroundColor: "rgba(0, 255, 0, 0.25)",
    // [ts] });
    // [ts] });
    // [ts] test("browser_hide_highlight", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] const { client } = startClient({ args: [`--endpoint=default`] });
    // [ts] client.callTool({ name: "browser_snapshot" });
    // [ts] client.callTool({
    // [ts] name: "browser_highlight",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] });
    // [ts] expect(page.locator("x-pw-highlight")).toBeVisible();
    // [ts] expect(client.callTool({
    // [ts] name: "browser_hide_highlight",
    // [ts] arguments: { element: "Submit button", target: "e2" },
    // [ts] })).toHaveResponse({
    // [ts] result: `Hid highlight for Submit button`,
    // [ts] });
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] });
    // [ts] test("browser_resume completes when context closes", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button>`);
    // [ts] const context = boundBrowser.contexts()[0];
    // Pause the debugger before the next action.
    // [ts] context.debugger.requestPause();
    // [ts] const clickPromise = page.click("button");
    // [ts] Promise.init(resolve => context.debugger.once("pausedstatechanged", resolve));
    // [ts] expect_eq(context.debugger.pausedDetails(), true);
    // browser_resume without step or location resumes execution and
    // waits for either the next pause or context close. Closing the
    // context after resume simulates a test finishing with no further
    // breakpoints. Before the fix, this would hang indefinitely.
    // [ts] const { client } = startClient({ args: [`--endpoint=default`] });
    // [ts] const resumePromise = client.callTool({ name: "browser_resume" });
    // [ts] clickPromise;
    // [ts] context.close();
    // [ts] resumePromise;
    // [ts] });
    // [ts] test("browser_hide_highlight all", () => {
    // [ts] const page = boundBrowser.newPage();
    // [ts] page.setContent(`<button>Submit</button><a href="#">Go</a>`);
    // [ts] const { client } = startClient({ args: [`--endpoint=default`] });
    // [ts] client.callTool({ name: "browser_snapshot" });
    // [ts] client.callTool({ name: "browser_highlight", arguments: { element: "Submit button", target: "e2" } });
    // [ts] client.callTool({ name: "browser_highlight", arguments: { element: "Go link", target: "e3" } });
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(2);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_hide_highlight",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] result: "Hid page highlight",
    // [ts] });
    // [ts] expect(page.locator("x-pw-highlight")).toHaveCount(0);
    // [ts] });
}
