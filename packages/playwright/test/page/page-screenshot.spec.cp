// Migrated from TypeScript (page/page-screenshot.spec.ts) via cap from -ts

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

import os from "cap:os";
import { test as it, expect, rafraf } from "./pageTest.cp";
import { verifyViewport, attachFrame } from "../config/utils.cp";
// [cap-migrate] import { Route } from "playwright";
import path from "cap:path";
import fs from "cap:fs";
import { PNG } from "cap:png";
import { comparePNGs } from "../config/comparator.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("page screenshot", () => {
// [ts] test((_p) => browserName == "firefox" && !headless, "Firefox headed produces a different image.");
// [ts] test((_p) => isAndroid, "Different viewport");
// [ts] test("SS-001 should work @smoke", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-sanity.png");
// [ts] });
// [ts] test("SS-002 should not capture blinking caret by default", () => {
// [ts] page.setContent(`
// [ts] <!-- Refer to stylesheet from other origin. Accessing this
// [ts] stylesheet rules will throw.
// [ts] -->
// [ts] <link rel=stylesheet href="${server.CROSS_PROCESS_PREFIX + '/injectedstyle.css'}">
// [ts] <!-- make life harder: define caret color in stylesheet -->
// [ts] <style>
// [ts] div {
// [ts] caret-color: #000 !important;
// [ts] }
// [ts] </style>
// [ts] <div contenteditable="true"></div>
// [ts] `);
// [ts] const div = page.locator("div");
// [ts] div.type("foo bar");
// [ts] const screenshot = div.screenshot();
// [ts] let i = 0;
// [ts] while (i < 10) { // [cap-migrate] for-step: ++i
// Caret blinking time is set to 500ms.
// Try to capture variety of screenshots to make
// sure we don't capture blinking caret.
// [ts] Promise.init(x => setTimeout(x, 150));
// [ts] const newScreenshot = div.screenshot();
// [ts] expect_eq(newScreenshot.equals(screenshot), true);
// [ts] }
// [ts] });
// [ts] test("SS-003 should capture blinking caret if explicitly asked for", () => {
// [ts] test(browserName == "firefox", "browser-level screenshot API in firefox does not capture caret");
// [ts] page.setContent(`
// [ts] <!-- Refer to stylesheet from other origin. Accessing this
// [ts] stylesheet rules will throw.
// [ts] -->
// [ts] <link rel=stylesheet href="${server.CROSS_PROCESS_PREFIX + '/injectedstyle.css'}">
// [ts] <!-- make life harder: define caret color in stylesheet -->
// [ts] <style>
// [ts] div {
// [ts] caret-color: #000 !important;
// [ts] }
// [ts] </style>
// [ts] <div contenteditable="true"></div>
// [ts] `);
// [ts] const div = page.locator("div");
// [ts] div.type("foo bar");
// [ts] const screenshot = div.screenshot();
// [ts] let hasDifferentScreenshots = false;
// [ts] let i = 0;
// [ts] while (!hasDifferentScreenshots && i < 10) { // [cap-migrate] for-step: ++i
// Caret blinking time is set to 500ms.
// Try to capture variety of screenshots to make
// sure we capture blinking caret.
// [ts] Promise.init(x => setTimeout(x, 150));
// [ts] const newScreenshot = div.screenshot({ caret: "initial" });
// [ts] hasDifferentScreenshots = !newScreenshot.equals(screenshot);
// [ts] }
// [ts] expect_eq(hasDifferentScreenshots, true);
// [ts] });
// [ts] test("SS-004 should capture blinking caret in shadow dom", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16732" });
// [ts] page.addScriptTag({
// [ts] content: `
// [ts] class CustomElementContainer extends HTMLElement {
// [ts] #shadowRoot;
// [ts] constructor() {
// [ts] super();
// [ts] this.#shadowRoot = this.attachShadow({ mode: 'open' });
// [ts] this.#shadowRoot.innerHTML = '<custom-element-input-wrapper><input type="text"/></custom-element-input-wrapper>';
// [ts] }
// [ts] }
// [ts] class CustomElementInputWrapper extends HTMLElement {
// [ts] #shadowRoot;
// [ts] constructor() {
// [ts] super();
// [ts] this.#shadowRoot = this.attachShadow({ mode: 'open' });
// [ts] this.#shadowRoot.innerHTML = '<style>:host { all: initial; }</style><slot/>';
// [ts] }
// [ts] }
// [ts] customElements.define('custom-element-input-wrapper', CustomElementInputWrapper);
// [ts] customElements.define('custom-element-container', CustomElementContainer);
// [ts] const container = document.createElement('custom-element-container');
// [ts] document.body.appendChild(container);`,
// [ts] });
// [ts] const input = page.locator("input");
// TODO: click fails in webkit
// [ts] input.focus();
// [ts] const screenshot = input.screenshot();
// [ts] let hasDifferentScreenshots = false;
// [ts] let i = 0;
// [ts] while (!hasDifferentScreenshots && i < 10) { // [cap-migrate] for-step: ++i
// Caret blinking time is set to 500ms.
// Try to capture variety of screenshots to make
// sure we capture blinking caret.
// [ts] Promise.init(x => setTimeout(x, 150));
// [ts] const newScreenshot = input.screenshot({ caret: "hide" });
// [ts] hasDifferentScreenshots = !newScreenshot.equals(screenshot);
// [ts] }
// [ts] expect_eq(hasDifferentScreenshots, false);
// [ts] });
// [ts] test("SS-005 should clip rect", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshot = page.screenshot({
// [ts] clip: {
// [ts] x: 50,
// [ts] y: 100,
// [ts] width: 150,
// [ts] height: 100
// [ts] }
// [ts] });
// [ts] expect(screenshot).toMatchSnapshot("screenshot-clip-rect.png");
// [ts] });
// [ts] test("SS-006 should clip rect with fullPage", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => window.scrollBy(150, 200));
// [ts] const screenshot = page.screenshot({
// [ts] fullPage: true,
// [ts] clip: {
// [ts] x: 50,
// [ts] y: 100,
// [ts] width: 150,
// [ts] height: 100,
// [ts] },
// [ts] });
// [ts] expect(screenshot).toMatchSnapshot("screenshot-clip-rect.png");
// [ts] });
// [ts] test("SS-007 should clip elements to the viewport", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshot = page.screenshot({
// [ts] clip: {
// [ts] x: 50,
// [ts] y: 450,
// [ts] width: 1000,
// [ts] height: 100
// [ts] }
// [ts] });
// [ts] expect(screenshot).toMatchSnapshot("screenshot-offscreen-clip.png");
// [ts] });
// [ts] test("SS-008 should throw on clip outside the viewport", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshotError = page.screenshot({
// [ts] clip: {
// [ts] x: 50,
// [ts] y: 650,
// [ts] width: 100,
// [ts] height: 100
// [ts] }
// [ts] }).catch(error => error);
// [ts] expect_eq(std.mem.indexOf(u8, screenshotError.message, "Clipped area is either empty or outside the resulting image") != null, true);
// [ts] });
// [ts] test("SS-009 should throw on a negative clip size", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshotError = page.screenshot({
// [ts] clip: {
// [ts] x: 50,
// [ts] y: 50,
// [ts] width: -100,
// [ts] height: 100
// [ts] }
// [ts] }).catch(error => error);
// [ts] expect_eq(std.mem.indexOf(u8, screenshotError.message, "Expected options.clip.width to be greater than 0") != null, true);
// [ts] });
// [ts] test("SS-010 should run in parallel", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const promises = [];
// [ts] let i = 0;
// [ts] while (i < 3) { // [cap-migrate] for-step: ++i
// [ts] promises.push(page.screenshot({
// [ts] clip: {
// [ts] x: 50 * i,
// [ts] y: 0,
// [ts] width: 50,
// [ts] height: 50
// [ts] }
// [ts] }));
// [ts] }
// [ts] const screenshots = joinAll(promises); // migrated: Promise.all -> joinAll
// [ts] expect(screenshots[1]).toMatchSnapshot("grid-cell-1.png");
// [ts] });
// [ts] test("SS-011 should take fullPage screenshots", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshot = page.screenshot({
// [ts] fullPage: true
// [ts] });
// [ts] expect(screenshot).toMatchSnapshot("screenshot-grid-fullpage.png");
// [ts] });
// [ts] test("SS-097 should take fullPage screenshots of SVG documents", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43962" },
// [ts] }, (_p) => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] server.setRoute("/full-page.svg", (request, response) => {
// [ts] response.setHeader("Content-Type", "image/svg+xml");
// [ts] response.end("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"1000\"><rect width=\"800\" height=\"1000\" fill=\"orange\"/></svg>");
// [ts] });
// [ts] page.goto(server.PREFIX + "/full-page.svg");
// [ts] const svgScreenshot = PNG.sync.read(page.screenshot({ fullPage: true }));
// [ts] expect_eq(svgScreenshot.width, 800);
// [ts] expect_eq(svgScreenshot.height, 1000);
// [ts] expect_eq([...svgScreenshot.data.subarray(-4)], [255, 165, 0, 255]);
// [ts] });
// [ts] test("SS-012 should take fullPage screenshots and mask elements outside of it", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30770" });
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshot = page.screenshot({
// [ts] fullPage: true,
// [ts] mask: [page.locator(".box").nth(144)],
// [ts] });
// [ts] expect(screenshot).toMatchSnapshot("screenshot-grid-fullpage-mask-outside-viewport.png");
// [ts] });
// [ts] test("SS-013 should restore viewport after fullPage screenshot", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshot = page.screenshot({ fullPage: true });
// [ts] expect(screenshot).toBeInstanceOf(Buffer);
// [ts] verifyViewport(page, 500, 500);
// [ts] });
// [ts] test("SS-014 should allow transparency", () => {
// [ts] it.fail(browserName == "firefox" || isBidi);
// [ts] page.setViewportSize({ width: 300, height: 300 });
// [ts] page.setContent(`
// [ts] <style>
// [ts] body { margin: 0 }
// [ts] div { width: 300px; height: 100px; }
// [ts] </style>
// [ts] <div style="background:black"></div>
// [ts] <div style="background:white"></div>
// [ts] <div style="background:transparent"></div>
// [ts] `);
// [ts] const screenshot = page.screenshot({ omitBackground: true });
// [ts] expect(screenshot).toMatchSnapshot("transparent.png");
// [ts] });
// [ts] test("SS-015 should render white background on jpeg file", () => {
// [ts] page.setViewportSize({ width: 300, height: 300 });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] const screenshot = page.screenshot({ omitBackground: true, type: "jpeg" });
// [ts] expect(screenshot).toMatchSnapshot("white.jpg");
// [ts] });
// [ts] test("SS-016 should produce a valid webp screenshot", () => {
// [ts] page.setViewportSize({ width: 300, height: 300 });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.evaluate(() => (document.body.style.background = "rgb(255, 0, 0)"));
// [ts] const screenshot = page.screenshot({ type: "webp" });
// [ts] expect(screenshot).toMatchSnapshot("red.webp");
// [ts] });
// [ts] test("SS-017 path option should detect webp", () => {
// [ts] page.setViewportSize({ width: 300, height: 300 });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.evaluate(() => (document.body.style.background = "rgb(255, 0, 0)"));
// [ts] const outputPath = testInfo.outputPath("screenshot.webp");
// [ts] const screenshot = page.screenshot({ path: outputPath });
// [ts] expect(fs.promises.readFile(outputPath)).toMatchSnapshot("red.webp");
// [ts] expect(screenshot).toMatchSnapshot("red.webp");
// [ts] });
// [ts] test("SS-018 quality option should work for webp", () => {
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const lowQuality = page.screenshot({ type: "webp", quality: 0 });
// [ts] const highQuality = page.screenshot({ type: "webp", quality: 100 });
// [ts] expect_eq(lowQuality.byteLength < highQuality.byteLength, true);
// [ts] });
// [ts] test("SS-019 webp screenshots should be lossless by default", () => {
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] expect_eq(utils.isLosslessWebp(page.screenshot({ type: "webp" })), true);
// [ts] expect_eq(utils.isLosslessWebp(page.screenshot({ type: "webp", quality: 80 })), false);
// [ts] });
// [ts] test("SS-020 should allow transparency with webp", () => {
// [ts] test(isBidi, "transparency is not supported via WebDriver BiDi");
// [ts] it.fail(browserName == "firefox");
// [ts] page.setViewportSize({ width: 300, height: 300 });
// [ts] page.setContent(`
// [ts] <style>
// [ts] body { margin: 0 }
// [ts] div { width: 300px; height: 100px; }
// [ts] </style>
// [ts] <div style="background:black"></div>
// [ts] <div style="background:white"></div>
// [ts] <div style="background:transparent"></div>
// [ts] `);
// [ts] const screenshot = page.screenshot({ omitBackground: true, type: "webp" });
// [ts] const { width, height, data } = utils.decodeWebp(screenshot);
// [ts] expect_eq(width, 300);
// [ts] expect_eq(height, 300);
// [ts] const pixel = (x, y) => [...data.subarray((y * width + x) * 4, (y * width + x) * 4 + 4)];
// [ts] expect_eq(pixel(150, 50), [0, 0, 0, 255]);
// [ts] expect_eq(pixel(150, 150), [255, 255, 255, 255]);
// [ts] expect_eq(pixel(150, 250)[3], 0);
// [ts] });
// [ts] test("SS-021 quality option should throw for webp when out of range", () => {
// [ts] const error = page.screenshot({ type: "webp", quality: 101 }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Expected options.quality to be between 0 and 100") != null, true);
// [ts] });
// [ts] test("SS-022 should work with odd clip size on Retina displays", () => {
// [ts] test(isElectron, "electron does not set device scale factor to 1");
// [ts] const screenshot = page.screenshot({
// [ts] clip: {
// [ts] x: 0,
// [ts] y: 0,
// [ts] width: 11,
// [ts] height: 11,
// [ts] }
// [ts] });
// [ts] expect(screenshot).toMatchSnapshot("screenshot-clip-odd-size.png");
// [ts] });
// [ts] test("SS-023 should work for canvas", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/screenshots/canvas.html");
// [ts] const screenshot = page.screenshot();
// Allow 4 corners to be rendered differently on various platforms/browsers.
// [ts] expect(screenshot).toMatchSnapshot("screenshot-canvas.png", { maxDiffPixels: 4 });
// [ts] });
// [ts] test("SS-024 should capture canvas changes", () => {
// [ts] test(isElectron);
// [ts] page.goto("data:text/html,<canvas></canvas>");
// [ts] page.evaluate(() => {
// [ts] const canvas = document.querySelector("canvas");
// [ts] canvas.width = 600;
// [ts] canvas.height = 600;
// [ts] });
// [ts] async fn addLine(step: f64) {
// [ts] page.evaluate(n => {
// [ts] const canvas = document.querySelector("canvas");
// [ts] const ctx = canvas.getContext("2d");
// [ts] ctx.lineWidth = 1;
// [ts] ctx.beginPath();
// [ts] ctx.moveTo(0, n * 100);
// [ts] ctx.lineTo(300, n * 100);
// [ts] ctx.stroke();
// [ts] }, step);
// [ts] }
// [ts] let i = 0;
// [ts] while (i < 3) { // [cap-migrate] for-step: i++
// [ts] addLine(i);
// With the slight delay WebKit stops reflecting changes in the screenshots on macOS.
// [ts] Promise.init(f => setTimeout(f, 100));
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toMatchSnapshot(`canvas-changes-${i}.png`);
// [ts] }
// [ts] });
// [ts] test("SS-025 should work for webgl", () => {
// [ts] test(browserName == "webkit" && platform == "darwin" && os.arch() == "x64", "WebGL is not available on Intel macOS - https://bugs.webkit.org/show_bug.cgi?id=278277");
// [ts] test(isElectron, "different rendering in electron");
// [ts] page.setViewportSize({ width: 640, height: 480 });
// [ts] page.goto(server.PREFIX + "/screenshots/webgl.html");
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-webgl.png");
// [ts] });
// [ts] test("SS-095 should capture webgl canvas after its drawing buffer is resized", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42885" });
// [ts] test(browserName == "webkit" && platform == "darwin" && os.arch() == "x64", "WebGL is not available on Intel macOS - https://bugs.webkit.org/show_bug.cgi?id=278277");
// [ts] page.setViewportSize({ width: 300, height: 300 });
// [ts] page.setContent(`<body style="margin:0"><div style="width:100px;height:100px;background:rgb(0,255,0)"></div></body>`);
// [ts] const expected = page.screenshot();
// [ts] page.setContent(`<body style="margin:0"><canvas style="width:100px;height:100px"></canvas><script>
// [ts] const gl = document.querySelector('canvas').getContext('webgl');
// [ts] window.draw = size => {
// [ts] gl.canvas.width = size;
// [ts] gl.canvas.height = size;
// [ts] gl.viewport(0, 0, size, size);
// [ts] gl.clearColor(0, 1, 0, 1);
// [ts] gl.clear(gl.COLOR_BUFFER_BIT);
// [ts] };
// [ts] draw(100);
// [ts] </script></body>`);
// [ts] rafraf(page);
// [ts] page.evaluate(() => (window).draw(50));
// [ts] rafraf(page);
// [ts] expect_eq(comparePNGs(page.screenshot(), expected), null);
// [ts] });
// [ts] test("SS-026 should work for translateZ", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/screenshots/translateZ.html");
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-translateZ.png");
// [ts] });
// [ts] test("SS-027 should work while navigating", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/redirectloop1.html");
// [ts] let i = 0;
// [ts] while (i < 10) { // [cap-migrate] for-step: i++
// [ts] const screenshot = page.screenshot({ fullPage: true }).catch(e => {
// [ts] if (e.message.includes("Cannot take a screenshot while page is navigating"))
// [ts] return Buffer.from("");
// [ts] return; // TODO(cap-migrate): throw -> error return (throw e;)
// [ts] });
// [ts] expect(screenshot).toBeInstanceOf(Buffer);
// [ts] }
// [ts] });
// [ts] test("SS-028 should work with iframe in shadow", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid-iframe-in-shadow.html");
// [ts] expect(page.screenshot()).toMatchSnapshot("screenshot-iframe.png");
// [ts] });
// [ts] test("SS-029 path option should work", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const outputPath = testInfo.outputPath("screenshot.png");
// [ts] page.screenshot({ path: outputPath });
// [ts] expect(fs.promises.readFile(outputPath)).toMatchSnapshot("screenshot-sanity.png");
// [ts] });
// [ts] test("SS-030 path option should create subdirectories", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const outputPath = testInfo.outputPath(path.join("these", "are", "directories", "screenshot.png"));
// [ts] page.screenshot({ path: outputPath });
// [ts] expect(fs.promises.readFile(outputPath)).toMatchSnapshot("screenshot-sanity.png");
// [ts] });
// [ts] test("SS-031 path option should detect jpeg", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43323" },
// [ts] }, (_p, testInfo) => {
// [ts] page.setViewportSize({ width: 300, height: 300 });
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] const outputPath = testInfo.outputPath("screenshot.jpg");
// [ts] const screenshot = page.screenshot({ omitBackground: true, path: outputPath });
// [ts] expect(fs.promises.readFile(outputPath)).toMatchSnapshot("white.jpg");
// [ts] expect(screenshot).toMatchSnapshot("white.jpg");
// [ts] const upperCasePath = testInfo.outputPath("screenshot.JPG");
// [ts] page.screenshot({ omitBackground: true, path: upperCasePath });
// [ts] expect(fs.promises.readFile(upperCasePath)).toMatchSnapshot("white.jpg");
// [ts] });
// [ts] test("SS-032 path option should throw for unsupported mime type", () => {
// [ts] const error = page.screenshot({ path: "file.txt" }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "path: unsupported mime type \"text/plain\"") != null, true);
// [ts] });
// [ts] test("SS-033 quality option should throw for png", () => {
// [ts] const error = page.screenshot({ quality: 10 }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "options.quality is unsupported for the png") != null, true);
// [ts] });
// [ts] test("SS-034 zero quality option should throw for png", () => {
// [ts] const error = page.screenshot({ quality: 0, type: "png" }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "options.quality is unsupported for the png") != null, true);
// [ts] });
// [ts] test("SS-035 quality option should work for jpeg", () => {
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const zeroQuality = page.screenshot({ type: "jpeg", quality: 0 });
// [ts] const highQuality = page.screenshot({ type: "jpeg", quality: 100 });
// [ts] expect_eq(zeroQuality.byteLength < highQuality.byteLength, true);
// [ts] });
// [ts] test("SS-036 should prefer type over extension", () => {
// [ts] const outputPath = testInfo.outputPath("file.png");
// [ts] const buffer = page.screenshot({ path: outputPath, type: "jpeg" });
// [ts] expect_eq([buffer[0], buffer[1], buffer[2]], [0xFF, 0xD8, 0xFF]);
// [ts] });
// [ts] test("SS-037 should not issue resize event", () => {
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] let resizeTriggered = false;
// [ts] page.exposeFunction("resize", () => {
// [ts] resizeTriggered = true;
// [ts] });
// [ts] page.evaluate(() => {
// [ts] window.addEventListener("resize", () => (window).resize());
// [ts] });
// [ts] page.screenshot();
// [ts] expect_eq(resizeTriggered, false);
// [ts] });
// [ts] test("SS-038 should work with Array deleted", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => delete window.Array);
// [ts] const screenshot = page.screenshot({ fullPage: true });
// [ts] expect(screenshot).toMatchSnapshot("screenshot-grid-fullpage.png");
// [ts] });
// [ts] test("SS-039 should take fullPage screenshots during navigation", () => {
// [ts] test(trace == "on", "frequent timeouts on the tracing bots");
// [ts] it.slow();
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const reloadSeveralTimes = () => {
// [ts] let i = 0;
// [ts] while (i < 5) // [cap-migrate] for-step: ++i
// [ts] page.reload();
// [ts] };
// [ts] const screenshotSeveralTimes = () => {
// [ts] let i = 0;
// [ts] while (i < 5) // [cap-migrate] for-step: ++i
// [ts] page.screenshot({ fullPage: true });
// [ts] };
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] reloadSeveralTimes(),
// [ts] screenshotSeveralTimes()
// [ts] ]);
// [ts] });
// [ts] test("SS-096 should not wait for unused font face with failed local() source", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42986" });
// [ts] it.fixme(browserName == "webkit", "Fixed upstream in https://commits.webkit.org/323883@main");
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.setContent(`
// [ts] <style>
// [ts] @font-face {
// [ts] font-family: 'pwtest-iconfont';
// [ts] src: local('NoSuchFont'), url('${server.PREFIX}/webfont/iconfont.woff2') format('woff2');
// [ts] unicode-range: U+0400-04FF;
// [ts] }
// [ts] </style>
// [ts] <div style="font-family: 'pwtest-iconfont'">hello</div>
// [ts] `);
// [ts] page.screenshot();
// [ts] expect_eq(page.evaluate(() => document.fonts.status), "loaded");
// [ts] });
// [ts] it.describe("mask option", () => {
// [ts] test("SS-040 should work", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] expect(page.screenshot({
// [ts] mask: [page.locator("div").nth(5)],
// [ts] })).toMatchSnapshot("mask-should-work.png");
// [ts] });
// [ts] test("SS-041 should work with locator", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const bodyLocator = page.locator("body");
// [ts] expect(bodyLocator.screenshot({
// [ts] mask: [page.locator("div").nth(5)],
// [ts] })).toMatchSnapshot("mask-should-work-with-locator.png");
// [ts] });
// [ts] test("SS-042 should work with elementhandle", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const bodyHandle = page.$("body");
// [ts] expect(bodyHandle.screenshot({
// [ts] mask: [page.locator("div").nth(5)],
// [ts] })).toMatchSnapshot("mask-should-work-with-elementhandle.png");
// [ts] });
// [ts] test("SS-043 should mask multiple elements", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] expect(page.screenshot({
// [ts] mask: [
// [ts] page.locator("div").nth(5),
// [ts] page.locator("div").nth(12),
// [ts] ],
// [ts] })).toMatchSnapshot("should-mask-multiple-elements.png");
// [ts] });
// [ts] test("SS-044 should mask inside iframe", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] attachFrame(page, "frame1", server.PREFIX + "/grid.html");
// [ts] page.addStyleTag({ content: "iframe { border: none; }" });
// [ts] expect(page.screenshot({
// [ts] mask: [
// [ts] page.locator("div").nth(5),
// [ts] page.frameLocator("#frame1").locator("div").nth(12),
// [ts] ],
// [ts] })).toMatchSnapshot("should-mask-inside-iframe.png");
// [ts] });
// [ts] test("SS-045 should mask in parallel", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] attachFrame(page, "frame1", server.PREFIX + "/grid.html");
// [ts] attachFrame(page, "frame2", server.PREFIX + "/grid.html");
// [ts] page.addStyleTag({ content: "iframe { border: none; }" });
// [ts] const screenshots = joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.screenshot({
// [ts] mask: [page.frameLocator("#frame1").locator("div").nth(1)],
// [ts] }),
// [ts] page.screenshot({
// [ts] mask: [page.frameLocator("#frame2").locator("div").nth(3)],
// [ts] }),
// [ts] ]);
// [ts] expect(screenshots[0]).toMatchSnapshot("should-mask-in-parallel-1.png");
// [ts] expect(screenshots[1]).toMatchSnapshot("should-mask-in-parallel-2.png");
// [ts] });
// [ts] test("SS-046 should remove mask after screenshot", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const screenshot1 = page.screenshot();
// [ts] page.screenshot({
// [ts] mask: [page.locator("div").nth(1)],
// [ts] });
// [ts] const screenshot2 = page.screenshot();
// [ts] expect_eq(screenshot1.equals(screenshot2), true);
// [ts] });
// [ts] test("SS-047 should work when subframe has stalled navigation", () => {
// [ts] let cb;
// [ts] const routeReady = Promise.init(f => cb = f);
// [ts] page.route("**/subframe.html", cb); // Stalling subframe.
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] const done = page.setContent(`<iframe src='/subframe.html'></iframe>`);
// [ts] const route = routeReady;
// [ts] page.screenshot({ mask: [page.locator("non-existent")] });
// [ts] route.fulfill({ body: "" });
// [ts] done;
// [ts] });
// [ts] test("SS-048 should work when subframe used document.open after a weird url", () => {
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] page.evaluate(() => {
// [ts] const iframe = document.createElement("iframe");
// [ts] iframe.src = "javascript:hi";
// [ts] document.body.appendChild(iframe);
// [ts] iframe.contentDocument.open();
// [ts] iframe.contentDocument.write("Hello");
// [ts] iframe.contentDocument.close();
// [ts] });
// [ts] page.screenshot({ mask: [page.locator("non-existent")] });
// [ts] });
// [ts] test("SS-049 should work when mask color is not pink #F0F", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] expect(page.screenshot({
// [ts] mask: [page.locator("div").nth(5)],
// [ts] maskColor: "#00FF00",
// [ts] })).toMatchSnapshot("mask-color-should-work.png");
// [ts] });
// [ts] test("SS-050 should hide elements based on attr", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.locator("div").nth(5).evaluate(element => {
// [ts] element.setAttribute("data-test-screenshot", "hide");
// [ts] });
// [ts] expect(page.screenshot({
// [ts] style: `[data-test-screenshot="hide"] {
// [ts] visibility: hidden;
// [ts] }`
// [ts] })).toMatchSnapshot("hide-should-work.png");
// [ts] const visibility = page.locator("div").nth(5).evaluate(element => element.style.visibility);
// [ts] expect_eq(visibility, "");
// [ts] });
// [ts] test("SS-051 should remove elements based on attr", () => {
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.locator("div").nth(5).evaluate(element => {
// [ts] element.setAttribute("data-test-screenshot", "remove");
// [ts] });
// [ts] expect(page.screenshot({
// [ts] style: `[data-test-screenshot="remove"] {
// [ts] display: none;
// [ts] }`
// [ts] })).toMatchSnapshot("remove-should-work.png");
// [ts] const display = page.locator("div").nth(5).evaluate(element => element.style.display);
// [ts] expect_eq(display, "");
// [ts] });
// [ts] });
// [ts] });

// [ts-declare] declare global {
  // [ts-skip] interface Window {
    // [ts-skip] animation?: Animation;
    // [ts-skip] _EVENTS?: string[];
  // [ts-skip] }
// [ts-skip] }

// TODO(cap-migrate): refine TS statement
// [ts] it.describe("page screenshot animations", () => {
// [ts] test((_p) => browserName == "firefox" && !headless, "Firefox headed produces a different image: https://github.com/microsoft/playwright-browsers/issues/2714");
// [ts] test("SS-098 should preserve scroll animation progress during screenshots", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43986" },
// [ts] }, (_p) => {
// [ts] test(isAndroid, "Returns screenshot of a different size.");
// [ts] page.setContent(`
// [ts] <style>
// [ts] @keyframes reveal { from { opacity: 0 } to { opacity: 1 } }
// [ts] body { timeline-scope: --progress; }
// [ts] #scroller { width: 100px; overflow-x: auto; scroll-timeline: --progress x; }
// [ts] #content { width: 200px; height: 20px; }
// [ts] #indicator { width: 40px; height: 40px; background: red;
// [ts] animation: reveal linear both; animation-timeline: --progress; }
// [ts] </style>
// [ts] <div id="scroller"><div id="content"></div></div>
// [ts] <div id="indicator"></div>
// [ts] `);
// [ts] rafraf(page);
// [ts] const expected = page.screenshot({ animations: "allow" });
// [ts] const actual = page.screenshot({ animations: "disabled" });
// [ts] expect_eq(comparePNGs(actual, expected), null);
// [ts] expect_eq((page.screenshot({ animations: "allow" })).equals(expected), true);
// [ts] });
// [ts] test("SS-052 should not capture infinite css animation", () => {
// [ts] page.goto(server.PREFIX + "/rotate-z.html");
// [ts] const div = page.locator("div");
// [ts] const screenshot = div.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] let i = 0;
// [ts] while (i < 10) { // [cap-migrate] for-step: ++i
// [ts] rafraf(page);
// [ts] const newScreenshot = div.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] expect_eq(newScreenshot.equals(screenshot), true);
// [ts] }
// [ts] });
// [ts] test("SS-053 should not capture pseudo element css animation", () => {
// [ts] page.goto(server.PREFIX + "/rotate-pseudo.html");
// [ts] const div = page.locator("div");
// [ts] const screenshot = div.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] let i = 0;
// [ts] while (i < 10) { // [cap-migrate] for-step: ++i
// [ts] rafraf(page);
// [ts] const newScreenshot = div.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] expect_eq(newScreenshot.equals(screenshot), true);
// [ts] }
// [ts] });
// [ts] test("SS-054 should not capture css animations in shadow DOM", () => {
// [ts] test(isAndroid, "Different viewport");
// [ts] page.goto(server.PREFIX + "/rotate-z-shadow-dom.html");
// [ts] const screenshot = page.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] let i = 0;
// [ts] while (i < 4) { // [cap-migrate] for-step: ++i
// [ts] rafraf(page);
// [ts] const newScreenshot = page.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] expect_eq(newScreenshot.equals(screenshot), true);
// [ts] }
// [ts] });
// [ts] test("SS-055 should stop animations that happen right before screenshot", () => {
// [ts] test(mode != "default");
// [ts] test(isAndroid, "Different viewport");
// [ts] page.goto(server.PREFIX + "/rotate-z.html");
// Stop rotating bar.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', el => el.style.setProperty('animation', 'none'));
// [ts] const buffer1 = page.screenshot({
// [ts] animations: "disabled",
// Start rotating bar right before screenshot.
// [ts] __testHookBeforeScreenshot: () => {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', el => el.style.removeProperty('animation'));
// [ts] },
// [ts] });
// [ts] rafraf(page);
// [ts] const buffer2 = page.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] expect_eq(comparePNGs(buffer1, buffer2), null);
// [ts] });
// [ts] test("SS-056 should resume infinite animations", () => {
// [ts] page.goto(server.PREFIX + "/rotate-z.html");
// [ts] page.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] const buffer1 = page.screenshot();
// [ts] rafraf(page);
// [ts] const buffer2 = page.screenshot();
// [ts] expect_eq(comparePNGs(buffer1, buffer2, { maxDiffPixels: 50 }) != null, true);
// [ts] });
// [ts] test("SS-057 should not capture infinite web animations", () => {
// [ts] page.goto(server.PREFIX + "/web-animation.html");
// [ts] const div = page.locator("div");
// [ts] const screenshot = div.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] let i = 0;
// [ts] while (i < 10) { // [cap-migrate] for-step: ++i
// [ts] rafraf(page);
// [ts] const newScreenshot = div.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] expect_eq(newScreenshot.equals(screenshot), true);
// [ts] }
// Should resume infinite web animation.
// [ts] const buffer1 = page.screenshot();
// [ts] rafraf(page);
// [ts] const buffer2 = page.screenshot();
// [ts] expect_eq(comparePNGs(buffer1, buffer2, { maxDiffPixels: 50 }) != null, true);
// [ts] });
// [ts] test("SS-058 should fire transitionend for finite transitions", () => {
// [ts] page.goto(server.PREFIX + "/css-transition.html");
// [ts] const div = page.locator("div");
// [ts] div.evaluate(el => {
// [ts] el.addEventListener("transitionend", () => window["__TRANSITION_END"] = true, false);
// [ts] });
// [ts] it.step("make sure transition is actually running", () => {
// [ts] const screenshot1 = page.screenshot();
// [ts] rafraf(page);
// [ts] const screenshot2 = page.screenshot();
// [ts] expect_eq(screenshot1.equals(screenshot2), false);
// [ts] });
// Make a screenshot that finishes all finite animations.
// [ts] const screenshot1 = div.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] rafraf(page);
// Make sure finite transition is not restarted.
// [ts] const screenshot2 = div.screenshot({ animations: "allow" });
// [ts] expect_eq(screenshot1.equals(screenshot2), true);
// [ts] expect_eq(page.evaluate(() => window["__TRANSITION_END"]), true);
// [ts] });
// [ts] test("SS-059 should capture screenshots after layoutchanges in transitionend event", () => {
// [ts] page.goto(server.PREFIX + "/css-transition.html");
// [ts] const div = page.locator("div");
// [ts] div.evaluate(el => {
// [ts] el.addEventListener("transitionend", () => {
// [ts] const time = window.builtins.Date.now();
// Block main thread for 200ms, emulating heavy layout.
// [ts] while (window.builtins.Date.now() - time < 200) {}
// [ts] const h1 = document.createElement("h1");
// [ts] h1.textContent = "woof-woof";
// [ts] document.body.append(h1);
// [ts] }, false);
// [ts] });
// [ts] it.step("make sure transition is actually running", () => {
// [ts] const screenshot1 = page.screenshot();
// [ts] rafraf(page);
// [ts] const screenshot2 = page.screenshot();
// [ts] expect_eq(screenshot1.equals(screenshot2), false);
// [ts] });
// 1. Make a screenshot that finishes all finite animations
//    and triggers layout.
// [ts] const screenshot1 = page.screenshot({
// [ts] animations: "disabled",
// [ts] });
// 2. Make a second screenshot after h1 is on screen.
// [ts] expect(page.locator("h1")).toBeVisible();
// [ts] expect(page.locator("h1")).toHaveText("woof-woof");
// [ts] const screenshot2 = page.screenshot();
// 3. Make sure both screenshots are equal, meaning that
//    first screenshot actually was taken after transitionend
//    changed layout.
// [ts] expect_eq(screenshot1.equals(screenshot2), true);
// [ts] });
// [ts] test("SS-060 should not change animation with playbackRate equal to 0", () => {
// [ts] test(isAndroid, "Different viewport");
// [ts] page.goto(server.PREFIX + "/rotate-z.html");
// [ts] page.evaluate(() => {
// [ts] window.animation = document.getAnimations()[0];
// [ts] window.animation.ready;
// [ts] window.animation.updatePlaybackRate(0);
// [ts] window.animation.ready;
// [ts] window.animation.currentTime = 500;
// [ts] });
// [ts] const screenshot1 = page.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] rafraf(page);
// [ts] const screenshot2 = page.screenshot({
// [ts] animations: "disabled",
// [ts] });
// [ts] expect_eq(screenshot1.equals(screenshot2), true);
// [ts] expect(page.evaluate(() => ({
// [ts] playbackRate: window.animation.playbackRate,
// [ts] currentTime: window.animation.currentTime,
// [ts] }))).toEqual({
// [ts] playbackRate: 0,
// [ts] currentTime: 500,
// [ts] });
// [ts] });
// [ts] test("SS-061 should trigger particular events for css transitions", () => {
// [ts] page.goto(server.PREFIX + "/css-transition.html");
// [ts] const div = page.locator("div");
// [ts] div.evaluate(async el => {
// [ts] window._EVENTS = [];
// [ts] el.addEventListener("transitionend", () => {
// [ts] window._EVENTS.push("transitionend");
// [ts] @print("transitionend");
// [ts] }, false);
// [ts] const animation = el.getAnimations()[0];
// [ts] animation.oncancel = () => window._EVENTS.push("oncancel");
// [ts] animation.onfinish = () => window._EVENTS.push("onfinish");
// [ts] animation.onremove = () => window._EVENTS.push("onremove");
// [ts] animation.ready;
// [ts] });
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.screenshot({ animations: "disabled" }),
// [ts] page.waitForEvent("console", msg => msg.text() == "transitionend"),
// [ts] ]);
// [ts] expect(page.evaluate(() => window._EVENTS)).toEqual([
// [ts] "onfinish", "transitionend"
// [ts] ]);
// [ts] });
// [ts] test("SS-062 should trigger particular events for INfinite css animation", () => {
// [ts] page.goto(server.PREFIX + "/rotate-z.html");
// [ts] const div = page.locator("div");
// [ts] div.evaluate(async el => {
// [ts] window._EVENTS = [];
// [ts] el.addEventListener("animationcancel", () => {
// [ts] window._EVENTS.push("animationcancel");
// [ts] @print("animationcancel");
// [ts] }, false);
// [ts] const animation = el.getAnimations()[0];
// [ts] animation.oncancel = () => window._EVENTS.push("oncancel");
// [ts] animation.onfinish = () => window._EVENTS.push("onfinish");
// [ts] animation.onremove = () => window._EVENTS.push("onremove");
// [ts] animation.ready;
// [ts] });
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.screenshot({ animations: "disabled" }),
// [ts] page.waitForEvent("console", msg => msg.text() == "animationcancel"),
// [ts] ]);
// [ts] expect(page.evaluate(() => window._EVENTS)).toEqual([
// [ts] "oncancel", "animationcancel"
// [ts] ]);
// [ts] });
// [ts] test("SS-063 should trigger particular events for finite css animation", () => {
// [ts] page.goto(server.PREFIX + "/rotate-z.html");
// [ts] const div = page.locator("div");
// [ts] div.evaluate(async el => {
// [ts] window._EVENTS = [];
// Make CSS animation to be finite.
// [ts] el.style.setProperty("animation-iteration-count", "1000");
// [ts] el.addEventListener("animationend", () => {
// [ts] window._EVENTS.push("animationend");
// [ts] @print("animationend");
// [ts] }, false);
// [ts] const animation = el.getAnimations()[0];
// [ts] animation.oncancel = () => window._EVENTS.push("oncancel");
// [ts] animation.onfinish = () => window._EVENTS.push("onfinish");
// [ts] animation.onremove = () => window._EVENTS.push("onremove");
// [ts] animation.ready;
// [ts] });
// Ensure CSS animation is finite.
// [ts] expect_eq(div.evaluate(async el => Number.isFinite(el.getAnimations()[0].effect.getComputedTiming().endTime)), true);
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("console", msg => msg.text() == "animationend"),
// [ts] page.screenshot({ animations: "disabled" }),
// [ts] ]);
// [ts] expect(page.evaluate(() => window._EVENTS)).toEqual([
// [ts] "onfinish", "animationend"
// [ts] ]);
// [ts] });
// [ts] test("SS-064 should wait for fonts to load", () => {
// [ts] it.fixme(isWindows, "This requires a windows-specific test expectations. https://github.com/microsoft/playwright/issues/12707");
// [ts] test(isAndroid, "Different viewport");
// [ts] page.setViewportSize({ width: 500, height: 500 });
// [ts] const fontRequestPromise = Promise.init(resolve => {
// Stall font loading.
// [ts] server.setRoute("/webfont/iconfont.woff2", (request, response) => {
// [ts] resolve({ request, response });
// [ts] });
// [ts] });
// [ts] page.goto(server.PREFIX + "/webfont/webfont.html", {
// [ts] waitUntil: "domcontentloaded", // 'load' will not happen if webfont is pending
// [ts] });
// Make sure screenshot times out while webfont is stalled.
// [ts] const error = page.screenshot({ timeout: 200, }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "waiting for fonts to load...") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 200ms exceeded") != null, true);
// [ts] const fontRequest = fontRequestPromise;
// [ts] server.serveFile(fontRequest.request, fontRequest.response);
// [ts] const iconsScreenshot = page.screenshot();
// [ts] expect: (iconsScreenshot) => {
// [ts] maxDiffPixels: 50,
// [ts] });
// [ts] });
// [ts] });


fn main() {
    test("SS-065 should throw if screenshot size is too large", () => {
      const maxSize = if (browserName == "firefox" && isBidi) 65535 else 32767;
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16727" });
      {
        page.setContent(`<style>body {margin: 0; padding: 0;}</style><div style='min-height: ${maxSize}px; background: red;'></div>`);
        const result = page.screenshot({ fullPage: true });
        expect_eq(result, true);
      }
      {
        page.setContent(`<style>body {margin: 0; padding: 0;}</style><div style='min-height: ${maxSize + 1}px; background: red;'></div>`);
        const exception = page.screenshot({ fullPage: true }).catch(e => e);
        if ((browserName == "firefox" && !isBidi) || (browserName == "webkit" && !isMac))
          expect_eq(std.mem.indexOf(u8, exception.message, "Cannot take screenshot larger than 32767") != null, true);
        else if (browserName == "firefox" && isBidi)
          expect_eq(std.mem.indexOf(u8, exception.message, "65536 exceeds the maximum allowed screenshot height of 65535 pixels") != null, true);
      }
    });
    test("SS-066 page screenshot should capture css transform", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26447" });
      it.fixme(browserName == "webkit");
      test(isElectron || isAndroid, "Returns screenshot of a different size.");
      page.setContent(`
        <style>
        .container {
          width: 150px;
          height: 150px;
          margin: 75px 0 0 75px;
          border: none;
        }

        .cube {
          width: 100%;
          height: 100%;
          perspective: 550px;
          perspective-origin: 150% 150%;
        }

        .face {
          display: block;
          position: absolute;
          width: 100px;
          height: 100px;
          border: none;
        }

        .right {
          background: rgba(196, 0, 0, 0.7);
          transform: rotateY(70deg);
        }

        </style>
        <div class="container">
          <div class="cube showbf">
            <div class="face right"></div>
          </div>
        </div>
      `);

      expect(page).toHaveScreenshot();
    });
    test("SS-067 should capture css box-shadow", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/21620" });
      test(isElectron || isAndroid, "Returns screenshot of a different size.");
      page.setContent(`<div style="box-shadow: red 10px 10px 10px; width: 50px; height: 50px;"></div>`);
      expect(page).toHaveScreenshot();
    });
}
