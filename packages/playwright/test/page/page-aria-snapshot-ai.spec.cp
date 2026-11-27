// Migrated from TypeScript (page/page-aria-snapshot-ai.spec.ts) via cap from -ts

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
import { unshift } from "../config/utils.cp";
// [cap-migrate] import { Page } from "playwright";

async fn snapshotForAI(page: Page, options: string? = null): string {
  return page.ariaSnapshot({ ...options, mode: "ai" });
}













































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("AR-048 should generate refs", () => {
    // [ts] page.setContent(`
    // [ts] <button>One</button>
    // [ts] <button>Two</button>
    // [ts] <button>Three</button>
    // [ts] `);
    // [ts] const snapshot1 = snapshotForAI(page);
    // [ts] expect(snapshot1).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - button "One" [ref=e2]
    // [ts] - button "Two" [ref=e3]
    // [ts] - button "Three" [ref=e4]
    // [ts] `);
    // [ts] expect(page.getByRef("e2")).toHaveText("One");
    // [ts] expect(page.getByRef("e3")).toHaveText("Two");
    // [ts] expect(page.getByRef("e4")).toHaveText("Three");
    // [ts] page.getByRef("e3").evaluate((e) => {
    // [ts] e.textContent = "Not Two";
    // [ts] });
    // [ts] const snapshot2 = snapshotForAI(page);
    // [ts] expect(snapshot2).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - button "One" [ref=e2]
    // [ts] - button "Not Two" [ref=e5]
    // [ts] - button "Three" [ref=e4]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-049 should list iframes", () => {
    // [ts] page.setContent(`
    // [ts] <h1>Hello</h1>
    // [ts] <iframe name="foo" src="data:text/html,<h1>World</h1>">
    // [ts] `);
    // [ts] const snapshot1 = snapshotForAI(page);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot1, "- iframe") != null, true);
    // [ts] const frameSnapshot = page.frameLocator(`iframe`).locator("body").ariaSnapshot();
    // [ts] expect_eq(frameSnapshot, "- heading \"World\" [level=1]");
    // [ts] });
    // [ts] test("AR-050 should snapshot a locator inside an iframe", () => {
    // [ts] page.setContent(`
    // [ts] <h1>Main Page</h1>
    // [ts] <iframe srcdoc="<ul><li>Item 1</li><li>Item 2</li></ul>"></iframe>
    // [ts] `);
    // [ts] const list = page.frames()[1].locator("ul");
    // [ts] const snapshot = list.ariaSnapshot({ mode: "ai" });
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - list [ref=f1e1]:
    // [ts] - listitem [ref=f1e2]: Item 1
    // [ts] - listitem [ref=f1e3]: Item 2
    // [ts] `);
    // [ts] });
    // [ts] test("AR-051 should limit depth across iframe boundary", () => {
    // [ts] page.setContent(`
    // [ts] <nav>
    // [ts] <iframe srcdoc="<ul><li><button>Deep</button></li></ul>"></iframe>
    // [ts] </nav>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page, { depth: 3 });
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - navigation [ref=e2]:
    // [ts] - iframe [ref=e3]:
    // [ts] - list [ref=f1e2]:
    // [ts] - listitem [ref=f1e3]
    // [ts] `);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "button") == null, true);
    // [ts] });
    // [ts] test("AR-052 should stitch all frame snapshots", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43628" },
    // [ts] }, (_p) => {
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - iframe [ref=e2]:
    // [ts] - generic [ref=f1e1]:
    // [ts] - iframe [ref=f1e2]:
    // [ts] - generic [ref=f3e1]: Hi, I'm frame
    // [ts] - iframe [ref=f1e3]:
    // [ts] - generic [ref=f4e1]: Hi, I'm frame
    // [ts] - iframe [ref=e3]:
    // [ts] - generic [ref=f2e1]: Hi, I'm frame
    // [ts] `);
    // [ts] const href = page.getByRef("e1").evaluate(e => e.ownerDocument.defaultView.location.href);
    // [ts] expect_eq(href, server.PREFIX + "/frames/nested-frames.html");
    // [ts] const href2 = page.getByRef("f1e2").evaluate(e => e.ownerDocument.defaultView.location.href);
    // [ts] expect_eq(href2, server.PREFIX + "/frames/two-frames.html");
    // [ts] const href3 = page.getByRef("f4e2").evaluate(e => e.ownerDocument.defaultView.location.href);
    // [ts] expect_eq(href3, server.PREFIX + "/frames/frame.html");
    // [ts] {
    // [ts] const resolved = page.getByRef("e1").normalize();
    // [ts] expect_eq(resolved.toString(), `locator('body')`);
    // [ts] }
    // [ts] {
    // [ts] const resolved = page.getByRef("f4e2").normalize();
    // [ts] expect_eq(resolved.toString(), `locator('iframe[name="2frames"]').contentFrame().locator('iframe[name="dos"]').contentFrame().getByText('Hi, I\\'m frame')`);
    // [ts] }
    // [ts] {
    // Should tolerate .describe().
    // [ts] const resolved = page.getByRef("f3e2").describe("foo bar").normalize();
    // [ts] expect_eq(resolved.toString(), `locator('iframe[name=\"2frames\"]').contentFrame().locator('iframe[name=\"uno\"]').contentFrame().getByText('Hi, I\\'m frame')`);
    // [ts] }
    // [ts] {
    // [ts] const error = page.getByRef("e1000").normalize().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `No element matching aria-ref=e1000`) != null, true);
    // [ts] }
    // [ts] page.locator("iframe").evaluateAll(iframes => {
    // [ts] for (const iframe of iframes)
    // [ts] iframe.textContent = "Your browser does not support frames";
    // [ts] });
    // [ts] expect_eq(snapshotForAI(page), snapshot);
    // [ts] });
    // [ts] test("AR-053 should re-number refs across navigations but not same-document navigations", () => {
    // [ts] server.setRoute("/one.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end("<button>One</button>");
    // [ts] });
    // [ts] server.setRoute("/two.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end("<button>Two</button>");
    // [ts] });
    // The first committed document keeps the base seq, so the main frame has no prefix.
    // [ts] page.goto(server.PREFIX + "/one.html");
    // [ts] const oneRef = (snapshotForAI(page)).match("button \"One\" \\[ref=(e\\d+)\\]")[1];
    // [ts] expect(page.getByRef(oneRef)).toHaveText("One");
    // Cross-document navigation re-numbers the main frame, so its refs gain a frame prefix.
    // [ts] page.goto(server.PREFIX + "/two.html");
    // [ts] const twoRef = (snapshotForAI(page)).match("button \"Two\" \\[ref=(f\\d+e\\d+)\\]")[1];
    // [ts] expect(page.getByRef(twoRef)).toHaveText("Two");
    // The stale ref from the previous document must not resolve against the new one.
    // [ts] const error = page.getByRef(oneRef).normalize().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `No element matching aria-ref=${oneRef}`) != null, true);
    // Same-document navigation keeps refs intact.
    // [ts] page.evaluate(() => history.pushState({}, "", "/pushed.html"));
    // [ts] expect_eq(std.mem.indexOf(u8, snapshotForAI(page), `button "Two" [ref=${twoRef}]`) != null, true);
    // [ts] expect(page.getByRef(twoRef)).toHaveText("Two");
    // [ts] });
    // [ts] test("AR-054 should persist iframe references", () => {
    // [ts] page.setContent(`
    // [ts] <ul>
    // [ts] <li><iframe srcdoc="<button>button1</button>"></iframe></li>
    // [ts] <li><iframe srcdoc="<button>button2</button>"></iframe></li>
    // [ts] </ul>
    // [ts] `);
    // [ts] expect(snapshotForAI(page)).toContainYaml(`
    // [ts] - list [ref=e2]:
    // [ts] - listitem [ref=e3]:
    // [ts] - iframe [ref=e4]:
    // [ts] - button "button1" [ref=f1e2]
    // [ts] - listitem [ref=e5]:
    // [ts] - iframe [ref=e6]:
    // [ts] - button "button2" [ref=f2e2]
    // [ts] `);
    // [ts] page.evaluate(() => document.querySelector("iframe").remove());
    // [ts] expect(snapshotForAI(page)).toContainYaml(`
    // [ts] - list [ref=e2]:
    // [ts] - listitem [ref=e3]
    // [ts] - listitem [ref=e5]:
    // [ts] - iframe [ref=e6]:
    // [ts] - button "button2" [ref=f2e2]
    // [ts] `);
    // [ts] expect(page.getByRef("f2e2")).toHaveText("button2");
    // [ts] page.evaluate(() => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.setAttribute("srcdoc", "<button>button1</button>");
    // [ts] document.querySelector("li").appendChild(frame);
    // [ts] });
    // [ts] expect(snapshotForAI(page)).toContainYaml(`
    // [ts] - list [ref=e2]:
    // [ts] - listitem [ref=e3]:
    // [ts] - iframe [ref=e7]:
    // [ts] - button "button1" [ref=f3e2]
    // [ts] - listitem [ref=e5]:
    // [ts] - iframe [ref=e6]:
    // [ts] - button "button2" [ref=f2e2]
    // [ts] `);
    // [ts] expect(page.getByRef("f3e2")).toHaveText("button1");
    // [ts] expect(page.getByRef("f2e2")).toHaveText("button2");
    // [ts] });
    // [ts] test("AR-055 should not generate refs for elements with pointer-events:none", () => {
    // [ts] page.setContent(`
    // [ts] <button style="pointer-events: none">no-ref</button>
    // [ts] <div style="pointer-events: none">
    // [ts] <button style="pointer-events: auto">with-ref</button>
    // [ts] </div>
    // [ts] <div style="pointer-events: none">
    // [ts] <div style="pointer-events: initial">
    // [ts] <button>with-ref</button>
    // [ts] </div>
    // [ts] </div>
    // [ts] <div style="pointer-events: none">
    // [ts] <div style="pointer-events: auto">
    // [ts] <button>with-ref</button>
    // [ts] </div>
    // [ts] </div>
    // [ts] <div style="pointer-events: auto">
    // [ts] <div style="pointer-events: none">
    // [ts] <button>no-ref</button>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - button "no-ref"
    // [ts] - button "with-ref" [ref=e2]
    // [ts] - button "with-ref" [ref=e4]
    // [ts] - button "with-ref" [ref=e6]
    // [ts] - generic [ref=e7]:
    // [ts] - generic:
    // [ts] - button "no-ref"
    // [ts] `);
    // [ts] });
    // [ts] test("AR-056 emit generic roles for nodes w/o roles", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] input {
    // [ts] width: 0;
    // [ts] height: 0;
    // [ts] opacity: 0;
    // [ts] }
    // [ts] </style>
    // [ts] <div>
    // [ts] <label>
    // [ts] <span>
    // [ts] <input type="radio" value="Apple" checked="">
    // [ts] </span>
    // [ts] <span>Apple</span>
    // [ts] </label>
    // [ts] <label>
    // [ts] <span>
    // [ts] <input type="radio" value="Pear">
    // [ts] </span>
    // [ts] <span>Pear</span>
    // [ts] </label>
    // [ts] <label>
    // [ts] <span>
    // [ts] <input type="radio" value="Orange">
    // [ts] </span>
    // [ts] <span>Orange</span>
    // [ts] </label>
    // [ts] </div>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [ref=e2]:
    // [ts] - generic [ref=e3]:
    // [ts] - generic [ref=e4]:
    // [ts] - radio "Apple" [checked]
    // [ts] - text: Apple
    // [ts] - generic [ref=e5]:
    // [ts] - generic [ref=e6]:
    // [ts] - radio "Pear"
    // [ts] - text: Pear
    // [ts] - generic [ref=e7]:
    // [ts] - generic [ref=e8]:
    // [ts] - radio "Orange"
    // [ts] - text: Orange
    // [ts] `);
    // [ts] });
    // [ts] test("AR-057 should collapse generic nodes", () => {
    // [ts] page.setContent(`
    // [ts] <div>
    // [ts] <div>
    // [ts] <div>
    // [ts] <button>Button</button>
    // [ts] </div>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - button \"Button\" [ref=e5]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-058 should include cursor pointer hint", () => {
    // [ts] page.setContent(`
    // [ts] <button style="cursor: pointer">Button</button>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - button \"Button\" [ref=e2] [cursor=pointer]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-059 should not nest cursor pointer hints", () => {
    // [ts] page.setContent(`
    // [ts] <a style="cursor: pointer" href="about:blank">
    // [ts] Link with a button
    // [ts] <button style="cursor: pointer">Button</button>
    // [ts] </a>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // The link's name is redundant - "Link with a button" prints as text and "Button" as the button -
    // so it is dropped even though the node is clickable.
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - link [ref=e2] [cursor=pointer]:
    // [ts] - /url: about:blank
    // [ts] - text: Link with a button
    // [ts] - button "Button" [ref=e3]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-060 should omit names that just repeat printed descendant nodes", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43809" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <h3><a style="cursor: pointer" href="/issues/1">Clipboard API</a></h3>
    // [ts] <dialog open aria-labelledby="title">
    // [ts] <h2 id="title">Delete item?</h2>
    // [ts] <button>OK</button>
    // [ts] </dialog>
    // [ts] <nav aria-labelledby="nav-title"><h2 id="nav-title">Account</h2></nav>
    // [ts] <table>
    // [ts] <caption>Monthly totals</caption>
    // [ts] <tr><th scope="col"><button>Price</button></th></tr>
    // [ts] </table>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - heading [level=3] [ref=e2]:
    // [ts] - link "Clipboard API" [ref=e3] [cursor=pointer]:
    // [ts] - /url: /issues/1
    // [ts] - dialog "Delete item?" [ref=e4]:
    // [ts] - heading "Delete item?" [level=2] [ref=e5]
    // [ts] - button "OK" [ref=e6]
    // [ts] - navigation "Account" [ref=e7]:
    // [ts] - heading "Account" [level=2] [ref=e8]
    // [ts] - table "Monthly totals" [ref=e9]:
    // [ts] - caption [ref=e10]: Monthly totals
    // [ts] - rowgroup [ref=e11]:
    // [ts] - row [ref=e12]:
    // [ts] - columnheader "Price" [ref=e13]:
    // [ts] - button "Price" [ref=e14]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-061 should omit redundant name when a contributing wrapper is collapsed", () => {
    // The flex span contributes to the heading's name, but is then removed from the tree as a
    // single-child generic wrapper. Its contribution is fully represented by the link, so the
    // heading's name is still redundant.
    // [ts] page.setContent(`
    // [ts] <h3><span style="display: flex"><a style="cursor: pointer" href="/issues/1">Clipboard API</a></span></h3>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - heading [level=3] [ref=e2]:
    // [ts] - link "Clipboard API" [ref=e4] [cursor=pointer]:
    // [ts] - /url: /issues/1
    // [ts] `);
    // [ts] });
    // [ts] test("AR-062 should omit redundant name when a contributor is a skipped leaf generic", () => {
    // The outer span is not collapsed (its child is an element, not text), so it becomes a leaf
    // generic node that contributes to both names. The link's rendered name covers it, which in turn
    // makes the heading's name redundant.
    // [ts] page.setContent(`
    // [ts] <h3><a style="cursor: pointer" href="/issues/1"><span><span>Clipboard API</span></span></a></h3>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - heading [level=3] [ref=e2]:
    // [ts] - link "Clipboard API" [ref=e3] [cursor=pointer]:
    // [ts] - /url: /issues/1
    // [ts] `);
    // [ts] });
    // [ts] test("AR-063 should keep the name when the contributing wrapper collapses into repeating text", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/41985" });
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/43315" });
    // The outer span is marked as represented while it still wraps the icon. Once the nameless
    // image is dropped, the span collapses into a lone text repeating the button's name and removes
    // itself, so the button must keep the name derived from it.
    // A wrapper that only adds whitespace to the name - a stray text node, or the spacing around an
    // inline-block icon - is not where the name comes from.
    // [ts] page.setContent(`
    // [ts] <button>
    // [ts] <span>
    // [ts] <span>
    // [ts] <svg focusable="false" tabindex="-1" aria-hidden="true" viewBox="0 0 448 512">
    // [ts] <path d="M416 208H272V64c0-17.67-14.33-32-32-32h-32c-17.67 0-32 14.33-32 32v144H32c-17.67 0-32 14.33-32 32v32c0 17.67 14.33 32 32 32h144v144c0 17.67 14.33 32 32 32h32c17.67 0 32-14.33 32-32V304h144c17.67 0 32-14.33 32-32v-32c0-17.67-14.33-32-32-32z"/>
    // [ts] </svg>
    // [ts] </span>
    // [ts] <span>Add New Item</span>
    // [ts] </span>
    // [ts] </button>
    // [ts] <button><span>\n  <svg aria-hidden="true"></svg></span>New survey</button>
    // [ts] <button><span><svg aria-hidden="true" style="width: 1em; height: 1em; display: inline-block"></svg></span>Edit survey</button>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - button "Add New Item" [ref=e2]
    // [ts] - button "New survey" [ref=e7]
    // [ts] - button "Edit survey" [ref=e10]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-064 should keep names not derived from printed nodes", () => {
    // [ts] page.setContent(`
    // [ts] <h3 aria-label="Clipboard API issue"><a style="cursor: pointer" href="/issues/1">Clipboard API</a></h3>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - heading "Clipboard API issue" [level=3] [ref=e2]:
    // [ts] - link "Clipboard API" [ref=e3] [cursor=pointer]:
    // [ts] - /url: /issues/1
    // [ts] `);
    // [ts] });
    // [ts] test("AR-065 should omit images without an accessible name", () => {
    // [ts] page.setContent(`
    // [ts] <img src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA=">
    // [ts] <img alt="A cat" src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA=">
    // [ts] <img style="cursor: pointer" src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA=">
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // A nameless image that cannot be clicked carries no information and is omitted.
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - img "A cat" [ref=e3]
    // [ts] - img [ref=e4] [cursor=pointer]
    // [ts] `);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "[ref=e2]") == null, true);
    // [ts] });
    // [ts] test("AR-066 should omit a nameless image nested inside a link", () => {
    // The decorative image has no name, so it is dropped even though it sits inside a clickable link.
    // [ts] page.setContent(`
    // [ts] <a style="cursor: pointer" href="/issue/1">Open issue <img src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA="></a>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - link "Open issue" [ref=e2] [cursor=pointer]:
    // [ts] - /url: /issue/1
    // [ts] `);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "img") == null, true);
    // [ts] });
    // [ts] test("AR-067 should keep icon-only clickable elements", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42013" },
    // [ts] }, (_p) => {
    // [ts] const icon = `<svg viewBox="0 0 1024 1024"><use xlink:href="#icon"></use></svg>`;
    // [ts] page.setContent(`
    // [ts] <div style="cursor: pointer" aria-haspopup="true"><i>${icon}</i></div>
    // [ts] <img style="cursor: pointer" src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA=">
    // [ts] <a style="cursor: pointer" href="/target">${icon}</a>
    // [ts] <div onclick="void 0">${icon}</div>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - generic [ref=e2] [cursor=pointer]
    // [ts] - img [ref=e5] [cursor=pointer]
    // [ts] - link [ref=e6] [cursor=pointer]:
    // [ts] - /url: /target
    // [ts] `);
    // [ts] });
    // [ts] test("AR-068 should omit leaf generic whose text is already in an ancestor name", () => {
    // The inner element is block so it survives as its own generic node (an inline single-text span
    // would be collapsed into the link instead). It inherits the link's pointer cursor.
    // [ts] page.setContent(`
    // [ts] <a style="cursor: pointer" href="/issues/15860"><div>[Feature] a dedicated clipboard API</div></a>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // The link keeps its name, and the inner leaf generic that the name was computed from is dropped
    // because its text is already shown by the name.
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - link "[Feature] a dedicated clipboard API" [ref=e2] [cursor=pointer]:
    // [ts] - /url: /issues/15860
    // [ts] `);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "[ref=e3]") == null, true);
    // [ts] });
    // [ts] test("AR-069 should omit name-repeating generic behind a wrapper", () => {
    // The leaf generic that repeats the link's name sits inside a nameless wrapper. Its text is
    // first inlined into the wrapper, which then faces the link and removes itself.
    // [ts] page.setContent(`
    // [ts] <a style="cursor: pointer" href="/labels"><span style="display: inline-block"><span style="display: inline-block"><span>P3-collecting-feedback</span></span></span></a>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - link "P3-collecting-feedback" [ref=e2] [cursor=pointer]:
    // [ts] - /url: /labels
    // [ts] `);
    // [ts] expect(snapshot.split("P3-collecting-feedback")).toHaveLength(2);
    // [ts] });
    // [ts] test("AR-070 should resolve refs of distilled-away nodes", () => {
    // [ts] page.setContent(`
    // [ts] <a style="cursor: pointer" href="/issues/15860"><div>[Feature] a dedicated clipboard API</div></a>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // The inner leaf generic is distilled away, but its ref still resolves to the element.
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "[ref=e3]") == null, true);
    // [ts] expect(page.getByRef("e3")).toHaveText("[Feature] a dedicated clipboard API");
    // [ts] });
    // [ts] test("AR-071 should not distill snapshots outside of ai mode", () => {
    // [ts] page.setContent(`
    // [ts] <h3><a href="/issues/1">Clipboard API</a></h3>
    // [ts] `);
    // The heading name would be dropped as redundant in ai mode; matching mode keeps it.
    // [ts] expect(page.locator("body")).toMatchAriaSnapshot(`
    // [ts] - heading "Clipboard API" [level=3]:
    // [ts] - link "Clipboard API":
    // [ts] - /url: /issues/1
    // [ts] `);
    // [ts] });
    // [ts] test("AR-072 should truncate data url in link", () => {
    // [ts] const base64 = Buffer.from("<p>hello</p>").toString("base64");
    // [ts] page.setContent(`<a href="data:text/html;base64,${base64}">a link</a>`);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "/url: data:text/html;base64,Ã¢ÂÂ¦") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, base64) == null, true);
    // [ts] });
    // [ts] test("AR-073 should gracefully fallback when child frame cant be captured", () => {
    // [ts] page.setContent(`
    // [ts] <p>Test</p>
    // [ts] <iframe src="${server.PREFIX}/redirectloop1.html#depth=100000"></iframe>
    // [ts] `, { waitUntil: 'domcontentloaded' });
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - paragraph [ref=e2]: Test
    // [ts] - iframe [ref=e3]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-074 should auto-wait for navigation", () => {
    // [ts] page.goto(server.PREFIX + "/frames/frame.html");
    // [ts] const [, snapshot] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(() => window.location.reload()),
    // [ts] snapshotForAI(page)
    // [ts] ]);
    // The snapshot races the reload, which may re-number the main frame, so accept any ref.
    // [ts] expect(snapshot).toMatch("- generic \\[active\\] \\[ref=(?:f\\d+)?e\\d+\\]: Hi, I'm frame");
    // [ts] });
    // [ts] test("AR-075 should auto-wait for blocking CSS", () => {
    // [ts] server.setRoute("/css", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/css");
    // [ts] setTimeout(() => res.end(`body { monospace }`), 1000);
    // [ts] });
    // [ts] page.setContent(`
    // [ts] <script src="${server.PREFIX}/css"></script>
    // [ts] <p>Hello World</p>
    // [ts] `, { waitUntil: 'commit' });
    // [ts] expect(snapshotForAI(page)).toContainYaml("Hello World");
    // [ts] });
    // [ts] test("AR-076 should show visible children of hidden elements", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36296" }  }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div style="visibility: hidden">
    // [ts] <div style="visibility: visible">
    // [ts] <button>Visible</button>
    // [ts] </div>
    // [ts] <div style="visibility: hidden">
    // [ts] <button style="visibility: visible">Visible</button>
    // [ts] </div>
    // [ts] <div>
    // [ts] <div style="visibility: visible">
    // [ts] <button style="visibility: hidden">Hidden</button>
    // [ts] </div>
    // [ts] <button>Hidden</button>
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] expect(snapshotForAI(page)).toEqual(unshift(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - button "Visible" [ref=e3]
    // [ts] - button "Visible" [ref=e4]
    // [ts] `));
    // [ts] });
    // [ts] test("AR-077 should include active element information", () => {
    // [ts] page.setContent(`
    // [ts] <button id="btn1">Button 1</button>
    // [ts] <button id="btn2" autofocus>Button 2</button>
    // [ts] <div>Not focusable</div>
    // [ts] `);
    // Wait for autofocus to take effect
    // [ts] page.waitForFunction(() => document.activeElement?.id == "btn2");
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [ref=e1]:
    // [ts] - button "Button 1" [ref=e2]
    // [ts] - button "Button 2" [active] [ref=e3]
    // [ts] - generic [ref=e4]: Not focusable
    // [ts] `);
    // [ts] });
    // [ts] test("AR-078 should update active element on focus", () => {
    // [ts] page.setContent(`
    // [ts] <input id="input1" placeholder="First input">
    // [ts] <input id="input2" placeholder="Second input">
    // [ts] `);
    // Initially there shouldn't be an active element on the inputs
    // [ts] const initialSnapshot = snapshotForAI(page);
    // [ts] expect(initialSnapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - textbox "First input" [ref=e2]
    // [ts] - textbox "Second input" [ref=e3]
    // [ts] `);
    // Focus the second input
    // [ts] page.locator("#input2").focus();
    // After focus, the second input should be active
    // [ts] const afterFocusSnapshot = snapshotForAI(page);
    // [ts] expect(afterFocusSnapshot).toContainYaml(`
    // [ts] - generic [ref=e1]:
    // [ts] - textbox "First input" [ref=e2]
    // [ts] - textbox "Second input" [active] [ref=e3]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-079 should mark iframe as active when it contains focused element", () => {
    // Create a simple HTML file for the iframe
    // [ts] page.setContent(`
    // [ts] <input id="regular-input" placeholder="Regular input">
    // [ts] <iframe src="data:text/html,<input id='iframe-input' placeholder='Input in iframe'>" tabindex="0"></iframe>
    // [ts] `);
    // Test 1: Focus the input inside the iframe
    // [ts] page.frameLocator("iframe").locator("#iframe-input").focus();
    // [ts] const inputInIframeFocusedSnapshot = snapshotForAI(page);
    // The iframe should be marked as active when it contains a focused element
    // [ts] expect(inputInIframeFocusedSnapshot).toContainYaml(`
    // [ts] - generic [ref=e1]:
    // [ts] - textbox "Regular input" [ref=e2]
    // [ts] - iframe [active] [ref=e3]:
    // [ts] - textbox "Input in iframe" [active] [ref=f1e2]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-080 return empty snapshot when iframe is not loaded", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/pull/36710" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div style="height: 5000px;">Test</div>
    // [ts] <iframe loading="lazy" src="${server.PREFIX}/frame.html"></iframe>
    // [ts] `);
    // Wait for the iframe to load
    // [ts] page.waitForSelector("iframe");
    // Get the snapshot of the page
    // [ts] const snapshot = snapshotForAI(page, { timeout: 3000 });
    // The iframe should be present but empty
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - generic [ref=e2]: Test
    // [ts] - iframe [ref=e3]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-081 should support many properties on iframes", () => {
    // [ts] page.setContent(`
    // [ts] <input id="regular-input" placeholder="Regular input">
    // [ts] <iframe style='cursor: pointer' src="data:text/html,<input id='iframe-input' placeholder='Input in iframe'/>" tabindex="0"></iframe>
    // [ts] `);
    // Test 1: Focus the input inside the iframe
    // [ts] page.frameLocator("iframe").locator("#iframe-input").focus();
    // [ts] const inputInIframeFocusedSnapshot = snapshotForAI(page);
    // [ts] expect(inputInIframeFocusedSnapshot).toContainYaml(`
    // [ts] - generic [ref=e1]:
    // [ts] - textbox "Regular input" [ref=e2]
    // [ts] - iframe [active] [ref=e3] [cursor=pointer]:
    // [ts] - textbox "Input in iframe" [active] [ref=f1e2]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-082 should snapshot frameset pages", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41784" } }, (_p) => {
    // [ts] server.setRoute("/frameset.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<frameset rows="50%,50%"><frameset cols="50%,50%"><frame src="/frame-one.html"><frame src="/frame-two.html"></frameset><frame src="/frame-three.html"></frameset>`);
    // [ts] });
    // [ts] for (const name of ["one", "two", "three"]) {
    // [ts] server.setRoute(`/frame-${name}.html`, (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<button>Button ${name}</button>`);
    // [ts] });
    // [ts] }
    // [ts] page.goto(server.PREFIX + "/frameset.html");
    // [ts] const snapshot = snapshotForAI(page, { timeout: 3000 });
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - generic [ref=e2]:
    // [ts] - iframe [ref=e3]:
    // [ts] - button "Button one" [ref=f1e2]
    // [ts] - iframe [ref=e4]:
    // [ts] - button "Button two" [ref=f2e2]
    // [ts] - iframe [ref=e5]:
    // [ts] - button "Button three" [ref=f3e2]
    // [ts] `);
    // [ts] expect(page.getByRef("f2e2")).toHaveText("Button two");
    // [ts] });
    // [ts] test("AR-083 should snapshot a locator inside a frameset frame", () => {
    // [ts] page.goto(server.PREFIX + "/frames/frameset.html");
    // [ts] const snapshot = page.frames()[1].locator("body").ariaSnapshot({ mode: "ai" });
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [ref=f1e1]: Hi, I'm frame
    // [ts] `);
    // [ts] expect(page.getByRef("f1e1")).toHaveText(`Hi, I'm frame`);
    // [ts] const resolved = page.getByRef("f1e1").normalize();
    // [ts] expect_eq(resolved.toString(), `locator('frame').first().contentFrame().locator('body')`);
    // [ts] });
    // [ts] test("AR-084 should stitch iframes inside a frameset frame", () => {
    // [ts] server.setRoute("/frameset-with-iframe.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<frameset><frame src="/frame-with-iframe.html"></frameset>`);
    // [ts] });
    // [ts] server.setRoute("/frame-with-iframe.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<button>In frame</button><iframe srcdoc="<button>In iframe</button>"></iframe>`);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/frameset-with-iframe.html");
    // [ts] expect(snapshotForAI(page)).toContainYaml(`
    // [ts] - iframe [ref=e2]:
    // [ts] - generic [ref=f1e1]:
    // [ts] - button "In frame" [ref=f1e2]
    // [ts] - iframe [ref=f1e3]:
    // [ts] - button "In iframe" [ref=f2e2]
    // [ts] `);
    // [ts] expect(page.getByRef("f1e2")).toHaveText("In frame");
    // [ts] expect(page.getByRef("f2e2")).toHaveText("In iframe");
    // [ts] const resolved = page.getByRef("f2e2").normalize();
    // [ts] expect_eq(resolved.toString(), `locator('frame').contentFrame().locator('iframe').contentFrame().getByRole('button', { name: 'In iframe' })`);
    // [ts] });
    // [ts] test("AR-085 should stitch nested frameset documents", () => {
    // [ts] server.setRoute("/outer-frameset.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<frameset><frame src="/inner-frameset.html"></frameset>`);
    // [ts] });
    // [ts] server.setRoute("/inner-frameset.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<frameset><frameset><frame src="/leaf.html"></frameset></frameset>`);
    // [ts] });
    // [ts] server.setRoute("/leaf.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end(`<button>Leaf button</button>`);
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/outer-frameset.html");
    // [ts] expect(snapshotForAI(page)).toContainYaml(`
    // [ts] - iframe [ref=e2]:
    // [ts] - iframe [ref=f1e3]:
    // [ts] - button "Leaf button" [ref=f2e2]
    // [ts] `);
    // [ts] expect(page.getByRef("f2e2")).toHaveText("Leaf button");
    // [ts] });
    // [ts] test("AR-086 should collapse inline generic nodes", () => {
    // [ts] page.setContent(`
    // [ts] <ul>
    // [ts] <li><b>3</b> <abbr>bds</abbr></li>
    // [ts] <li><b>2</b> <abbr>ba</abbr></li>
    // [ts] <li><b>1,200</b> <abbr>sqft</abbr></li>
    // [ts] </ul>
    // [ts] <ul>
    // [ts] <li><div>3</div></li>
    // [ts] <li><div>2</div></li>
    // [ts] <li><div>1,200</div></li>
    // [ts] </ul>`);
    // [ts] const snapshot1 = snapshotForAI(page);
    // [ts] expect(snapshot1).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - list [ref=e2]:
    // [ts] - listitem [ref=e3]: 3 bds
    // [ts] - listitem [ref=e4]: 2 ba
    // [ts] - listitem [ref=e5]: 1,200 sqft
    // [ts] - list [ref=e6]:
    // [ts] - listitem [ref=e7]:
    // [ts] - generic [ref=e8]: "3"
    // [ts] - listitem [ref=e9]:
    // [ts] - generic [ref=e10]: "2"
    // [ts] - listitem [ref=e11]:
    // [ts] - generic [ref=e12]: 1,200
    // [ts] `);
    // [ts] });
    // [ts] test("AR-087 should inline single leaf generic child into parent generic", () => {
    // The nameless images are distilled away, so each wrapper is left with a single leaf generic
    // child, whose text is inlined into the wrapper - recursively for the second one.
    // [ts] page.setContent(`
    // [ts] <div><img src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA="><div>Status: Open.</div></div>
    // [ts] <div><img src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA="><div><img src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA="><div>Nested twice.</div></div></div>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - generic [ref=e2]: "Status: Open."
    // [ts] - generic [ref=e5]: Nested twice.
    // [ts] `);
    // [ts] });
    // [ts] test("AR-088 should inline a deeply nested generic", () => {
    // Every wrapper contains a nameless image (distilled away) plus a single generic child, so the
    // text bubbles up the whole chain - all the way into the body.
    // [ts] const img = `<img src="data:image/gif;base64,R0lGODlhAQABAAAAACwAAAAAAQABAAA=">`;
    // [ts] page.setContent(`
    // [ts] <div>${img}<div>${img}<div>${img}<div>${img}<div>Deeply nested.</div></div></div></div></div>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic [active] [ref=e1]: Deeply nested.
    // [ts] `);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "img") == null, true);
    // [ts] });
    // [ts] test("AR-089 should not remove generic nodes with title", () => {
    // [ts] page.setContent(`<div title="Element title">Element content</div>`);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - generic "Element title" [ref=e2]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-090 should limit depth", () => {
    // [ts] page.setContent(`
    // [ts] <ul>
    // [ts] <li>item1</li>
    // [ts] <a href="about:blank" style="cursor:pointer">link</a>
    // [ts] <li>
    // [ts] <ul id=target>
    // [ts] <li>item2</li>
    // [ts] <li>
    // [ts] <ul>
    // [ts] <li>item3</li>
    // [ts] </ul>
    // [ts] </li>
    // [ts] </ul>
    // [ts] </li>
    // [ts] </ul>
    // [ts] `);
    // [ts] const snapshot1 = snapshotForAI(page, { depth: 1 });
    // [ts] expect(snapshot1).toContainYaml(`
    // [ts] - list [ref=e2]:
    // [ts] - listitem [ref=e3]: item1
    // [ts] - link "link" [ref=e4] [cursor=pointer]:
    // [ts] - /url: about:blank
    // [ts] - listitem [ref=e5]
    // [ts] `);
    // [ts] const snapshot2 = snapshotForAI(page, { depth: 3 });
    // [ts] expect(snapshot2).toContainYaml(`
    // [ts] - list [ref=e2]:
    // [ts] - listitem [ref=e3]: item1
    // [ts] - link "link" [ref=e4] [cursor=pointer]:
    // [ts] - /url: about:blank
    // [ts] - listitem [ref=e5]:
    // [ts] - list [ref=e6]:
    // [ts] - listitem [ref=e7]: item2
    // [ts] - listitem [ref=e8]
    // [ts] `);
    // [ts] const snapshot3 = snapshotForAI(page, { depth: 100 });
    // [ts] expect(snapshot3).toContainYaml(`
    // [ts] - list [ref=e2]:
    // [ts] - listitem [ref=e3]: item1
    // [ts] - link "link" [ref=e4] [cursor=pointer]:
    // [ts] - /url: about:blank
    // [ts] - listitem [ref=e5]:
    // [ts] - list [ref=e6]:
    // [ts] - listitem [ref=e7]: item2
    // [ts] - listitem [ref=e8]:
    // [ts] - list [ref=e9]:
    // [ts] - listitem [ref=e10]: item3
    // [ts] `);
    // [ts] const snapshot4 = page.locator("#target").ariaSnapshot({ mode: "ai", depth: 1 });
    // [ts] expect(snapshot4).toContainYaml(`
    // [ts] - list [ref=e6]:
    // [ts] - listitem [ref=e7]: item2
    // [ts] - listitem [ref=e8]
    // [ts] `);
    // [ts] });
    // [ts] test("AR-091 should annotate aria-hidden elements", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42223" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <h2>Visible heading</h2>
    // [ts] <div aria-hidden="true">
    // [ts] <h1>Hidden heading</h1>
    // [ts] <p>Hidden content</p>
    // [ts] </div>
    // [ts] <h2>After hidden</h2>
    // [ts] `);
    // [ts] expect(snapshotForAI(page)).toContainYaml(`
    // [ts] - generic [active] [ref=e1]:
    // [ts] - heading "Visible heading" [level=2] [ref=e2]
    // [ts] - generic [aria-hidden] [ref=e3]:
    // [ts] - heading [level=1] [ref=e4]: Hidden heading
    // [ts] - paragraph [ref=e5]: Hidden content
    // [ts] - heading "After hidden" [level=2] [ref=e6]
    // [ts] `);
    // Default snapshot excludes aria-hidden elements entirely.
    // [ts] const defaultSnapshot = page.locator("body").ariaSnapshot();
    // [ts] expect_eq(std.mem.indexOf(u8, defaultSnapshot, "Hidden content") == null, true);
    // [ts] });
    // [ts] test("AR-092 should only annotate the top element in a hidden subtree", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42223" } }, (_p) => {
    // [ts] page.setContent(`
    // [ts] <div aria-hidden="true">
    // [ts] <h1>Heading</h1>
    // [ts] <p>Paragraph</p>
    // [ts] </div>
    // [ts] `);
    // Only the element with aria-hidden="true" gets the annotation, not its descendants.
    // [ts] expect(snapshotForAI(page)).toContainYaml(`
    // [ts] - generic [aria-hidden] [ref=e2]:
    // [ts] - heading [level=1] [ref=e3]: Heading
    // [ts] - paragraph [ref=e4]: Paragraph
    // [ts] `);
    // [ts] });
    // [ts] test("AR-093 should not include hidden iframes", () => {
    // [ts] page.setContent(`
    // [ts] <iframe name="visible" srcdoc="<button>Visible</button>"></iframe>
    // [ts] <iframe name="hidden" style="visibility: hidden" srcdoc="<button>Hidden</button>"></iframe>
    // [ts] <iframe name="none" style="display: none" srcdoc="<button>None</button>"></iframe>
    // [ts] `);
    // [ts] const snapshot = snapshotForAI(page);
    // [ts] expect(snapshot).toContainYaml(`
    // [ts] - iframe [ref=e2]:
    // [ts] - button "Visible" [ref=f1e2]
    // [ts] `);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "Hidden") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, snapshot, "None") == null, true);
    // [ts] });
}
