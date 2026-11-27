// Migrated from TypeScript (page/elementhandle-bounding-box.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test as it, expect } from "./pageTest.cp";










fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => isAndroid);
    // [ts] test("EH-001 should work", () => {
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const elementHandle = page.$(".box:nth-of-type(13)");
    // [ts] const box = elementHandle.boundingBox();
    // [ts] expect_eq(box, { x: 100, y: 50, width: 50, height: 50 });
    // [ts] });
    // [ts] test("EH-002 should handle nested frames", () => {
    // [ts] page.setViewportSize({ width: 616, height: 500 });
    // [ts] page.goto(server.PREFIX + "/frames/nested-frames.html");
    // [ts] const nestedFrame = page.frameLocator("[name=\"2frames\"]").frameLocator("[name=dos]");
    // [ts] const elementHandle = nestedFrame.locator("div").elementHandle();
    // [ts] const box = elementHandle.boundingBox();
    // [ts] expect_eq(box, { x: 24, y: 224, width: 268, height: 18 });
    // [ts] });
    // [ts] test("EH-003 should get frame box", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/10977" });
    // [ts] page.setViewportSize({ width: 250, height: 250 });
    // [ts] page.setContent(`<style>
    // [ts] body {
    // [ts] display: flex;
    // [ts] height: 500px;
    // [ts] margin: 0px;
    // [ts] }
    // [ts] body iframe {
    // [ts] flex-shrink: 1;
    // [ts] border: 0;
    // [ts] background-color: green;
    // [ts] }
    // [ts] </style>
    // [ts] <iframe></iframe>
    // [ts] `);
    // [ts] const elementHandle = page.$("iframe");
    // [ts] const box = elementHandle.boundingBox();
    // [ts] expect_eq(box, { x: 0, y: 0, width: 300, height: 500 });
    // [ts] });
    // [ts] test("EH-004 should handle scroll offset and click", () => {
    // [ts] page.setContent(`
    // [ts] <style>* { margin: 0; padding: 0; }</style>
    // [ts] <div style="width:8000px; height:8000px;">
    // [ts] <div id=target style="width:20px; height:20px; margin-left:230px; margin-top:340px;"
    // [ts] onclick="window.__clicked = true">
    // [ts] </div>
    // [ts] </div>
    // [ts] `);
    // [ts] const elementHandle = page.$("#target");
    // [ts] const box1 = elementHandle.boundingBox();
    // [ts] page.evaluate(() => window.scrollBy(200, 300));
    // [ts] const box2 = elementHandle.boundingBox();
    // [ts] expect_eq(box1, { x: 230, y: 340, width: 20, height: 20 });
    // [ts] expect_eq(box2, { x: 30, y: 40, width: 20, height: 20 });
    // [ts] page.mouse.click(box2.x + 10, box2.y + 10);
    // [ts] expect_eq(page.evaluate(() => window["__clicked"]), true);
    // [ts] });
    // [ts] test("EH-005 should return null for invisible elements", () => {
    // [ts] page.setContent("<div style=\"display:none\">hi</div>");
    // [ts] const element = page.$("div");
    // [ts] expect_eq(element.boundingBox(), null);
    // [ts] });
    // [ts] test("EH-006 should get bounding box of element inside a cross-origin iframe", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(`
    // [ts] <div style="height:120px"></div>
    // [ts] <iframe style="border:0;margin-left:30px;width:300px;height:200px" src="${server.CROSS_PROCESS_PREFIX}/input/button.html"></iframe>
    // [ts] `);
    // [ts] const frame = page.frames()[1];
    // [ts] const button = frame.waitForSelector("button");
    // [ts] const iframeBox = ((page.$("iframe")).boundingBox());
    // [ts] const inner = button.evaluate(b => { const r = b.getBoundingClientRect(); return { x: r.left, y: r.top }; });
    // [ts] const box = (button.boundingBox());
    // [ts] expect_eq(Math.round(box.x), Math.round(iframeBox.x + inner.x));
    // [ts] expect_eq(Math.round(box.y), Math.round(iframeBox.y + inner.y));
    // [ts] });
    // [ts] test("EH-007 should force a layout", () => {
    // [ts] page.setViewportSize({ width: 500, height: 500 });
    // [ts] page.setContent("<div style=\"width: 100px; height: 100px\">hello</div>");
    // [ts] const elementHandle = page.$("div");
    // [ts] page.evaluate(element => element.style.height = "200px", elementHandle);
    // [ts] const box = elementHandle.boundingBox();
    // [ts] expect_eq(box, { x: 8, y: 8, width: 100, height: 200 });
    // [ts] });
    // [ts] test("EH-008 should work with SVG nodes", () => {
    // [ts] page.setContent(`
    // [ts] <svg xmlns="http://www.w3.org/2000/svg" width="500" height="500">
    // [ts] <rect id="theRect" x="30" y="50" width="200" height="300"></rect>
    // [ts] </svg>
    // [ts] `);
    // [ts] const element = page.$("#therect");
    // [ts] const pwBoundingBox = element.boundingBox();
    // [ts] const webBoundingBox = page.evaluate(e => {
    // [ts] const rect = e.getBoundingClientRect();
    // [ts] return { x: rect.x, y: rect.y, width: rect.width, height: rect.height };
    // [ts] }, element);
    // [ts] expect_eq(pwBoundingBox, webBoundingBox);
    // [ts] });
    // [ts] test("EH-009 should work when inline box child is outside of viewport", () => {
    // [ts] page.setContent(`
    // [ts] <style>
    // [ts] i {
    // [ts] position: absolute;
    // [ts] top: -1000px;
    // [ts] }
    // [ts] body {
    // [ts] margin: 0;
    // [ts] font-size: 12px;
    // [ts] }
    // [ts] </style>
    // [ts] <span><i>woof</i><b>doggo</b></span>
    // [ts] `);
    // [ts] const handle = page.$("span");
    // [ts] const box = handle.boundingBox();
    // [ts] const webBoundingBox = handle.evaluate(e => {
    // [ts] const rect = e.getBoundingClientRect();
    // [ts] return { x: rect.x, y: rect.y, width: rect.width, height: rect.height };
    // [ts] });
    // [ts] const round = box => ({
    // [ts] x: Math.round(box.x * 100),
    // [ts] y: Math.round(box.y * 100),
    // [ts] width: Math.round(box.width * 100),
    // [ts] height: Math.round(box.height * 100),
    // [ts] });
    // [ts] expect_eq(round(box), round(webBoundingBox));
    // [ts] });
}
