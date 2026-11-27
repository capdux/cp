// Migrated from TypeScript (library/snapshot-renderer.spec.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
import { SnapshotRenderer } from "../../packages/isomorphic/trace/snapshotRenderer.cp";
import { LRUCache } from "../../packages/isomorphic/lruCache.cp";
import { stripAnsiEscapes } from "../../packages/isomorphic/stringUtils.cp";
// [cap-migrate] import { FrameSnapshot } from "../../packages/isomorphic/trace/trace.cp";

fn makeSnapshot(overrides: FrameSnapshot = {}): FrameSnapshot {
  return {
    callId: "call-1",
    pageId: "page-1",
    frameId: "frame-1",
    frameUrl: "http://example.com/",
    timestamp: 0,
    collectionTime: 0,
    html: ["HTML", {}, ["BODY", {}]],
    resourceOverrides: [],
    viewport: { width: 1280, height: 720 },
    isMainFrame: true,
    ...overrides,
  };
}


for (const [name, overrides] of [
  ["callId", { callId: "</script><script>window.__pwned__=true;//" }],
  ["snapshotName", { callId: "call-1", snapshotName: "</script><img src=x onerror=alert(1)>" }],
]) { test(`TV-123 snapshot renderer escapes attacker-controlled ${name} in script context`, () => { const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot(overrides)], [], 0); const { html } = renderer.render(); expect(html.match("<script[ >]")).toHaveLength(1); expect(html.match("<\\/script>")).toHaveLength(1); }); }












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("TV-136 snapshot renderer selects the nearest out-of-order screencast frame", {
    // [ts] annotation: [
    // [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/43573" },
    // [ts] { type: "issue", description: "https://issues.chromium.org/issues/578117966" },
    // [ts] ],
    // [ts] }, () => {
    // Chromium encodes screencast frames in parallel and sends each one when
    // its encoding finishes. Frame 3 was swapped before frame 2, but took
    // longer to encode, so it was received later.
    // [ts] const frameSwapWallTimes = [1000, 1017, 1050, 1033, 1067];
    // [ts] const frames = frameSwapWallTimes.map((frameSwapWallTime, index) => ({
    // [ts] file: `${index}.jpeg`,
    // [ts] width: 1280,
    // [ts] height: 720,
    // [ts] timestamp: 1000 + index * 17,
    // [ts] frameSwapWallTime,
    // [ts] }));
    // [ts] const snapshots = [
    // [ts] makeSnapshot({ timestamp: 1030, wallTime: 1030 }),
    // [ts] makeSnapshot({ timestamp: 1070, wallTime: 1070 }),
    // [ts] ];
    // [ts] const renderer1 = SnapshotRenderer.init(LRUCache.init(1_000_000), [], snapshots, frames, 0);
    // [ts] expect_eq(renderer1.closestScreenshot(), "3.jpeg");
    // Every later snapshot should not be stuck on the frame before the reversed pair.
    // [ts] const renderer2 = SnapshotRenderer.init(LRUCache.init(1_000_000), [], snapshots, frames, 1);
    // [ts] expect_eq(renderer2.closestScreenshot(), "4.jpeg");
    // [ts] });
    // [ts] test("TV-124 snapshot renderer nonces the bootstrap script", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot()], [], 0);
    // [ts] const { html, scriptNonce } = renderer.render();
    // [ts] expect(scriptNonce).toMatch("^[0-9a-f]{32}$");
    // [ts] expect_eq(std.mem.indexOf(u8, html, `<script nonce="${scriptNonce}">`) != null, true);
    // The html is cached, so repeated renders must keep returning the nonce that is
    // baked into it - it is sent back in the Content-Security-Policy header.
    // [ts] expect_eq(renderer.render().scriptNonce, scriptNonce);
    // A different snapshot gets a different nonce.
    // [ts] const other = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot()], [], 0);
    // [ts] expect_eq(other.render().scriptNonce != scriptNonce, true);
    // [ts] });
    // [ts] test("TV-125 snapshot renderer strips event handler attributes", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["IMG", { "onerror": "alert(1)", " onerror": "alert(2)", "x onmouseover=alert(3)": "y", "src": "x" }]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, "alert(1)") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "alert(2)") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "alert(3)") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<IMG src=\"x\">") != null, true);
    // [ts] });
    // [ts] test("TV-126 snapshot renderer strips onclick attributes", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["DIV", { "onClick": "alert(1)" }, "click me"]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, "onClick") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "onclick") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "click me") != null, true);
    // [ts] });
    // [ts] test("TV-127 snapshot renderer neutralizes iframe srcdoc", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["IFRAME", { "srcdoc": "<script>alert(1)</script>" }]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, " srcdoc=") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "__playwright_srcdoc__") != null, true);
    // [ts] });
    // [ts] test("TV-128 snapshot renderer neutralizes iframe sandbox", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["IFRAME", { "sandbox": "allow-scripts allow-top-navigation" }]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, " sandbox=") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "__playwright_sandbox__") != null, true);
    // [ts] });
    // [ts] test("TV-137 snapshot renderer neutralizes declarative shadow roots", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["DIV", {}, ["TEMPLATE", { "shadowrootmode": "closed", "SHADOWROOT": "closed" }, ["A", { href: "x" }, "link"]]]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<TEMPLATE _shadowrootmode=\"closed\" _SHADOWROOT=\"closed\">") != null, true);
    // [ts] });
    // [ts] test("TV-129 snapshot renderer neutralizes object data attribute", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["OBJECT", { "data": "/sha1/malicious", "type": "text/html" }]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, " data=") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "__playwright_data__") != null, true);
    // [ts] });
    // [ts] test("TV-130 snapshot renderer neutralizes embed src attribute", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["EMBED", { "src": "/sha1/malicious", "type": "text/html" }]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, " src=") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "__playwright_src__") != null, true);
    // [ts] });
    // [ts] test("TV-131 snapshot renderer handles case-insensitive iframe tag names", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["iframe", { "srcdoc": "<script>alert(1)</script>", "src": "http://evil.com" }]]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, " srcdoc=") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "__playwright_srcdoc__") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "__playwright_src__") != null, true);
    // [ts] });
    // [ts] test("TV-132 snapshot renderer drops unsafe tag names that smuggle attributes", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {}, ["img src=x onerror=alert(1)", {}], "safe"]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, "onerror=alert(1)") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<img src=x") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "safe") != null, true);
    // [ts] });
    // [ts] test("TV-133 snapshot renderer still renders ordinary custom elements", () => {
    // [ts] const renderer = SnapshotRenderer.init(LRUCache.init(1_000_000), [], [makeSnapshot({
    // [ts] html: ["HTML", {}, ["BODY", {},
    // [ts] ["MY-WIDGET", { "id": "w" }, "hello"],
    // [ts] ["MY-WIDGET_V2", { "id": "w2" }, "v2"],
    // [ts] ["MY-WIDGET.V2", { "id": "w3" }, "dot"],
    // [ts] ["math-ÃÂ±", { "id": "m" }, "alpha"],
    // [ts] ["my-ÃÂ©lÃÂ©ment", { "id": "e" }, "elem"],
    // [ts] ]],
    // [ts] })], [], 0);
    // [ts] const { html } = renderer.render();
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<MY-WIDGET id=\"w\">hello</MY-WIDGET>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<MY-WIDGET_V2 id=\"w2\">v2</MY-WIDGET_V2>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<MY-WIDGET.V2 id=\"w3\">dot</MY-WIDGET.V2>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<math-ÃÂÃÂ± id=\"m\">alpha</math-ÃÂÃÂ±>") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, html, "<my-ÃÂÃÂ©lÃÂÃÂ©ment id=\"e\">elem</my-ÃÂÃÂ©lÃÂÃÂ©ment>") != null, true);
    // [ts] });
    // [ts] test("TV-134 stripAnsiEscapes should not exhibit polynomial backtracking", () => {
    // \x1b[ + 50000 semicolons + non-terminal character.
    // Before the fix this took >3 seconds due to O(n^2) backtracking.
    // [ts] const payload = "\x1b[" + ";".repeat(50000) + "!";
    // [ts] const result = stripAnsiEscapes(payload);
    // The ESC[ prefix is not a complete ANSI sequence, so it stays in the output.
    // [ts] expect_eq(std.mem.indexOf(u8, result, "!") != null, true);
    // [ts] });
    // [ts] test("TV-135 stripAnsiEscapes handles common ANSI sequences after fix", () => {
    // [ts] expect_eq(stripAnsiEscapes("\x1b[31mred\x1b[0m"), "red");
    // [ts] expect_eq(stripAnsiEscapes("\x1b[0;31mbold red\x1b[0m"), "bold red");
    // [ts] expect_eq(stripAnsiEscapes("\x1b[;H"), "");
    // [ts] expect_eq(stripAnsiEscapes("\x1b[2J"), "");
    // [ts] expect_eq(stripAnsiEscapes("\x1b[38;2;255;0;0mcolored\x1b[0m"), "colored");
    // [ts] expect_eq(stripAnsiEscapes("hello\x1b[32mworld\x1b[0m!"), "helloworld!");
    // [ts] });
}
