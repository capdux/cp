// Migrated from TypeScript (page/elementhandle-screenshot.spec.ts) via cap from -ts

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

import { test as it, expect, rafraf } from "./pageTest.cp";
import { verifyViewport } from "../config/utils.cp";
import { PNG } from "../../packages/playwright-core/lib/utilsBundle.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import path from "cap:path";
import fs from "cap:fs";

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("element screenshot", () => {
// [ts] test((_p) => browserName == "firefox" && !headless);
// [ts] test((_p) => isAndroid, "Different dpr. Remove after using 1x scale for screenshots.");
// [ts] test("EH-074 should work", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => window.scrollBy(50, 100));
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-bounding-box.png");
// [ts] });
// [ts] test("EH-075 should work with webp", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] const webpImage = utils.decodeWebp(elementHandle.screenshot({ type: "webp" }));
// The default webp screenshot is lossless, so it must decode to the exact same pixels as png.
// [ts] const pngImage = PNG.sync.read(elementHandle.screenshot({ type: "png" }));
// [ts] expect_eq(webpImage.width, pngImage.width);
// [ts] expect_eq(webpImage.height, pngImage.height);
// [ts] expect_eq(webpImage.data.equals(pngImage.data), true);
// [ts] });
// [ts] test("EH-076 should work when main world busts JSON.stringify", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => {
// [ts] window.scrollBy(50, 100);
// [ts] JSON.stringify = () => null;
// [ts] });
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-bounding-box.png");
// [ts] });
// [ts] test("EH-077 should take into account padding and border", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.setContent(`
// [ts] <div style="height: 14px">oooo</div>
// [ts] <style>div {
// [ts] border: 2px solid blue;
// [ts] background: green;
// [ts] width: 50px;
// [ts] height: 50px;
// [ts] }
// [ts] </style>
// [ts] <div id="d"></div>
// [ts] `);
// [ts] const elementHandle = page.$("div#d");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-padding-border.png");
// [ts] });
// [ts] test("EH-078 should capture full element when larger than viewport in parallel", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.setContent(`
// [ts] <div style="height: 14px">oooo</div>
// [ts] <style>
// [ts] div.to-screenshot {
// [ts] border: 1px solid blue;
// [ts] width: 600px;
// [ts] height: 600px;
// [ts] margin-left: 50px;
// [ts] }
// [ts] ::-webkit-scrollbar{
// [ts] display: none;
// [ts] }
// [ts] </style>
// [ts] <div class="to-screenshot"></div>
// [ts] <div class="to-screenshot"></div>
// [ts] <div class="to-screenshot"></div>
// [ts] `);
// [ts] const elementHandles = page.$$("div.to-screenshot");
// [ts] const promises = elementHandles.map(handle => handle.screenshot());
// [ts] const screenshots = joinAll(promises); // migrated: Promise.all -> joinAll
// [ts] expect(screenshots[2]).toMatchSnapshot("screenshot-element-larger-than-viewport.png");
// [ts] verifyViewport(page, 500, 500);
// [ts] });
// [ts] test("EH-079 should capture full element when larger than viewport", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.setContent(`
// [ts] <div style="height: 14px">oooo</div>
// [ts] <style>
// [ts] div.to-screenshot {
// [ts] border: 1px solid blue;
// [ts] width: 600px;
// [ts] height: 600px;
// [ts] margin-left: 50px;
// [ts] }
// [ts] ::-webkit-scrollbar{
// [ts] display: none;
// [ts] }
// [ts] </style>
// [ts] <div class="to-screenshot"></div>
// [ts] <div class="to-screenshot"></div>
// [ts] <div class="to-screenshot"></div>
// [ts] `);
// [ts] const elementHandle = page.$("div.to-screenshot");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-larger-than-viewport.png");
// [ts] verifyViewport(page, 500, 500);
// [ts] });
// [ts] test("EH-080 should scroll element into view", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.setContent(`
// [ts] <div style="height: 14px">oooo</div>
// [ts] <style>div.above {
// [ts] border: 2px solid blue;
// [ts] background: red;
// [ts] height: 1500px;
// [ts] }
// [ts] div.to-screenshot {
// [ts] border: 2px solid blue;
// [ts] background: green;
// [ts] width: 50px;
// [ts] height: 50px;
// [ts] }
// [ts] </style>
// [ts] <div class="above"></div>
// [ts] <div class="to-screenshot"></div>
// [ts] `);
// [ts] const elementHandle = page.$("div.to-screenshot");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-scrolled-into-view.png");
// [ts] });
// [ts] test("EH-081 should scroll 15000px into view", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.setContent(`
// [ts] <div style="height: 14px">oooo</div>
// [ts] <style>div.above {
// [ts] border: 2px solid blue;
// [ts] background: red;
// [ts] height: 15000px;
// [ts] }
// [ts] div.to-screenshot {
// [ts] border: 2px solid blue;
// [ts] background: green;
// [ts] width: 50px;
// [ts] height: 50px;
// [ts] }
// [ts] </style>
// [ts] <div class="above"></div>
// [ts] <div class="to-screenshot"></div>
// [ts] `);
// [ts] const elementHandle = page.$("div.to-screenshot");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-scrolled-into-view.png");
// [ts] });
// [ts] test("EH-082 should work with a rotated element", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.setContent(`<div style="position:absolute;
// [ts] top: 100px;
// [ts] left: 100px;
// [ts] width: 100px;
// [ts] height: 100px;
// [ts] background: green;
// [ts] transform: rotateZ(200deg);">&nbsp;</div>`);
// [ts] const elementHandle = page.$("div");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-rotate.png");
// [ts] });
// [ts] test("EH-083 should fail to screenshot a detached element", () => {
// [ts] page.setContent("<h1>remove this</h1>");
// [ts] const elementHandle = page.$("h1");
// [ts] page.evaluate(element => element.remove(), elementHandle);
// [ts] const screenshotError = elementHandle.screenshot().catch(error => error);
// [ts] expect_eq(std.mem.indexOf(u8, screenshotError.message, "Element is not attached to the DOM") != null, true);
// [ts] });
// [ts] test("EH-084 should timeout waiting for visible", () => {
// [ts] page.setContent("<div style=\"width: 50px; height: 0\"></div>");
// [ts] const div = page.$("div");
// [ts] const error = div.screenshot({ timeout: 3000 }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "element is not visible") != null, true);
// [ts] });
// [ts] test("EH-085 should wait for visible", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => window.scrollBy(50, 100));
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] elementHandle.evaluate(e => e.style.visibility = "hidden");
// [ts] let done = false;
// [ts] const promise = elementHandle.screenshot().then(buffer => {
// [ts] done = true;
// [ts] return buffer;
// [ts] });
// [ts] rafraf(page, 10);
// [ts] expect_eq(done, false);
// [ts] elementHandle.evaluate(e => e.style.visibility = "visible");
// [ts] const screenshot = promise;
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-bounding-box.png");
// [ts] });
// [ts] test("EH-086 should work for an element with fractional dimensions", () => {
// [ts] page.setContent("<div style=\"width:48.51px;height:19.8px;border:1px solid black;\"></div>");
// [ts] const elementHandle = page.$("div");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-fractional.png");
// [ts] });
// [ts] test("EH-087 should work for an element with an offset", () => {
// [ts] page.setContent("<div style=\"position:absolute; top: 10.3px; left: 20.4px;width:50.3px;height:20.2px;border:1px solid black;\"></div>");
// [ts] const elementHandle = page.$("div");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-fractional-offset.png");
// [ts] });
// [ts] test("EH-088 should wait for element to stop moving", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] elementHandle.evaluate(e => e.classList.add("animation"));
// [ts] rafraf(page);
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-bounding-box.png");
// [ts] });
// [ts] test("EH-089 should take screenshot of disabled button", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.setContent(`<button disabled>Click me</button>`);
// [ts] const button = page.$("button");
// [ts] const screenshot = button.screenshot();
// [ts] expect(screenshot).toBeInstanceOf(Buffer);
// [ts] });
// [ts] test("EH-090 path option should create subdirectories", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => window.scrollBy(50, 100));
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] const outputPath = testInfo.outputPath(path.join("these", "are", "directories", "screenshot.png"));
// [ts] elementHandle.screenshot({ path: outputPath });
// [ts] expect(fs.promises.readFile(outputPath)).toMatchSnapshot("screenshot-element-bounding-box.png");
// [ts] });
// [ts] test("EH-091 should prefer type over extension", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => window.scrollBy(50, 100));
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] const outputPath = testInfo.outputPath("file.png");
// [ts] const buffer = elementHandle.screenshot({ path: outputPath, type: "jpeg" });
// [ts] expect_eq([buffer[0], buffer[1], buffer[2]], [0xFF, 0xD8, 0xFF]);
// [ts] });
// [ts] test("EH-092 should not issue resize event", () => {
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] let resizeTriggered = false;
// [ts] page.exposeFunction("resize", () => {
// [ts] resizeTriggered = true;
// [ts] });
// [ts] page.evaluate(() => {
// [ts] window.addEventListener("resize", () => (window).resize());
// [ts] });
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] elementHandle.screenshot();
// [ts] expect_eq(resizeTriggered, false);
// [ts] });
// [ts] });
