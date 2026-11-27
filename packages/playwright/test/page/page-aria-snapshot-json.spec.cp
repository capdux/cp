// Migrated from TypeScript (page/page-aria-snapshot-json.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";

type NodeJSON = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS function
// [ts] fn findNode(nodes: []string, predicate: node: NodeJSON = > boolean): NodeJSON? {
// [ts] for (const node of nodes) {
// [ts] if (@typeName(node) == "string") {
// [ts] continue;
// [ts] }
// [ts] if (predicate(node)) {
// [ts] return node;
// [ts] }
// [ts] const result = findNode(node.children || [], predicate);
// [ts] if (result) {
// [ts] return result;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("AR-096 should snapshot roles, names and text", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44124" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <h1>title</h1>
    // [ts] <ul aria-label="my list">
    // [ts] <li>one</li>
    // [ts] <li>two</li>
    // [ts] </ul>
    // [ts] `);
    // [ts] expect(page.ariaSnapshotJSON()).toEqual([
    // [ts] { role: "heading", name: "title", level: 1 },
    // [ts] {
    // [ts] role: "list",
    // [ts] name: "my list",
    // [ts] children: [
    // [ts] { role: "listitem", text: "one" },
    // [ts] { role: "listitem", text: "two" },
    // [ts] ],
    // [ts] },
    // [ts] ]);
    // [ts] server.setRoute("/snapshot.svg", (req, res) => {
    // [ts] res.setHeader("Content-Type", "image/svg+xml");
    // [ts] res.end("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"400\" height=\"200\"><text x=\"10\" y=\"30\">hello</text></svg>");
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/snapshot.svg");
    // [ts] expect_eq(page.evaluate(() => document.body) == null, true);
    // [ts] expect(page.ariaSnapshotJSON({ timeout: 2000 })).toEqual([
    // [ts] { role: "img", text: "hello" },
    // [ts] ]);
    // [ts] expect_eq(page.ariaSnapshot({ mode: "ai", timeout: 2000 }), page.locator(":root").ariaSnapshot({ mode: "ai" }));
    // [ts] });
    // [ts] test("AR-097 should snapshot flags as properties", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44204" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <input type="checkbox" title="Check" checked required>
    // [ts] <button disabled aria-required="true">Click</button>
    // [ts] <input type="email" aria-label="Email" required aria-required="false">
    // [ts] <textarea aria-label="Notes" required></textarea>
    // [ts] <select aria-label="Country" required></select>
    // [ts] <div role="textbox" aria-label="Custom" aria-required="TrUe"></div>
    // [ts] <input aria-label="Optional" aria-required="false">
    // [ts] `);
    // [ts] expect(page.ariaSnapshotJSON()).toEqual([
    // [ts] { role: "checkbox", name: "Check", checked: true, required: true },
    // [ts] { role: "button", name: "Click", disabled: true },
    // [ts] { role: "textbox", name: "Email", required: true },
    // [ts] { role: "textbox", name: "Notes", required: true },
    // [ts] { role: "combobox", name: "Country", required: true },
    // [ts] { role: "textbox", name: "Custom", required: true },
    // [ts] { role: "textbox", name: "Optional" },
    // [ts] ]);
    // [ts] expect(page.locator("body").ariaSnapshot()).toBe(`- checkbox "Check" [checked] [required]
    // [ts] - button "Click" [disabled]
    // [ts] - textbox "Email" [required]
    // [ts] - textbox "Notes" [required]
    // [ts] - combobox "Country" [required]
    // [ts] - textbox "Custom" [required]
    // [ts] - textbox "Optional"`);
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - textbox "Email" [required]
    // [ts] - textbox "Custom" [required=true]
    // [ts] - textbox "Optional" [required=false]
    // [ts] `);
    // [ts] expect(page).not.toMatchAriaSnapshot(`- textbox "Email" [required=false]`);
    // [ts] expect(page).not.toMatchAriaSnapshot(`- textbox "Optional" [required]`);
    // [ts] });
    // [ts] test("AR-098 should snapshot link url and textbox value", () => {
    // [ts] page.setContent(`
    // [ts] <a href="https://example.com/">Link</a>
    // [ts] <input title="Input" value="hello">
    // [ts] `);
    // [ts] expect(page.ariaSnapshotJSON()).toEqual([
    // [ts] { role: "link", name: "Link", url: "https://example.com/" },
    // [ts] { role: "textbox", name: "Input", text: "hello" },
    // [ts] ]);
    // [ts] });
    // [ts] test("AR-099 should snapshot text fragments in children", () => {
    // [ts] page.setContent(`<p>Hello <a href="/link">world</a> again</p>`);
    // [ts] expect(page.ariaSnapshotJSON()).toEqual([
    // [ts] {
    // [ts] role: "paragraph",
    // [ts] children: [
    // [ts] "Hello",
    // [ts] { role: "link", name: "world", url: "/link" },
    // [ts] "again",
    // [ts] ],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("AR-100 should snapshot top-level text fragments as text nodes", () => {
    // [ts] page.setContent(`Hello <button>One</button> again`);
    // [ts] expect(page.ariaSnapshotJSON()).toEqual([
    // [ts] { role: "text", text: "Hello" },
    // [ts] { role: "button", name: "One" },
    // [ts] { role: "text", text: "again" },
    // [ts] ]);
    // [ts] });
    // [ts] test("AR-101 should generate refs in ai mode", () => {
    // [ts] page.setContent(`
    // [ts] <button>One</button>
    // [ts] <button>Two</button>
    // [ts] `);
    // [ts] expect(page.ariaSnapshotJSON({ mode: "ai" })).toEqual([
    // [ts] {
    // [ts] role: "generic",
    // [ts] active: true,
    // [ts] ref: "e1",
    // [ts] children: [
    // [ts] { role: "button", name: "One", ref: "e2" },
    // [ts] { role: "button", name: "Two", ref: "e3" },
    // [ts] ],
    // [ts] },
    // [ts] ]);
    // [ts] expect(page.getByRef("e2")).toHaveText("One");
    // [ts] });
    // [ts] test("AR-102 should mark clickable elements with cursor in ai mode", () => {
    // [ts] page.setContent(`<button style="cursor: pointer">One</button>`);
    // [ts] const json = page.ariaSnapshotJSON({ mode: "ai" });
    // [ts] const button = findNode(json, node => node.role == "button");
    // [ts] expect_eq(button?.cursor, "pointer");
    // [ts] });
    // [ts] test("AR-103 should snapshot iframes in ai mode", () => {
    // [ts] page.setContent(`
    // [ts] <h1>Hello</h1>
    // [ts] <iframe srcdoc="<button>In frame</button>"></iframe>
    // [ts] `);
    // [ts] const json = page.ariaSnapshotJSON({ mode: "ai" });
    // [ts] const iframe = findNode(json, node => node.role == "iframe");
    // [ts] expect_eq(iframe?.ref, true);
    // [ts] const button = findNode(iframe.children, node => node.role == "button");
    // [ts] expect_eq(button?.name, "In frame");
    // [ts] expect(button?.ref).toMatch("^f\\d+e\\d+$");
    // [ts] });
    // [ts] test("AR-104 should limit depth", () => {
    // [ts] page.setContent(`<ul><li><button>One</button></li></ul>`);
    // [ts] expect(page.ariaSnapshotJSON({ depth: 1 })).toEqual([
    // [ts] {
    // [ts] role: "list",
    // [ts] children: [
    // [ts] { role: "listitem" },
    // [ts] ],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("AR-105 should include boxes when requested", () => {
    // [ts] page.setContent(`<button>One</button>`);
    // [ts] const json = page.ariaSnapshotJSON({ boxes: true });
    // [ts] const button = findNode(json, node => node.role == "button");
    // [ts] expect: (button?.box) => {
    // [ts] x: expect.any(Number),
    // [ts] y: expect.any(Number),
    // [ts] width: expect.any(Number),
    // [ts] height: expect.any(Number),
    // [ts] });
    // [ts] expect_eq(button.box.width > 0, true);
    // [ts] expect_eq(button.box.height > 0, true);
    // [ts] });
    // [ts] test("AR-106 should snapshot a locator", () => {
    // [ts] page.setContent(`
    // [ts] <h1>title</h1>
    // [ts] <ul>
    // [ts] <li>one</li>
    // [ts] <li>two</li>
    // [ts] </ul>
    // [ts] `);
    // [ts] expect(page.locator("ul").ariaSnapshotJSON()).toEqual([
    // [ts] {
    // [ts] role: "list",
    // [ts] children: [
    // [ts] { role: "listitem", text: "one" },
    // [ts] { role: "listitem", text: "two" },
    // [ts] ],
    // [ts] },
    // [ts] ]);
    // [ts] });
}
