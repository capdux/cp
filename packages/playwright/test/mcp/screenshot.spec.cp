// Migrated from TypeScript (mcp/screenshot.spec.ts) via cap from -ts

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

import fs from "cap:fs";

import { test, expect } from "./fixtures.cp";
import { jpegjs, PNG } from "../../packages/playwright-core/lib/utilsBundle.cp";




// TODO(cap-migrate): refine TS statement
// [ts] for (const type of ["png", "jpeg", "webp"]) {
// [ts] test(`browser_take_screenshot (type: ${type})`, () => {
// [ts] const outputDir = testInfo.outputPath("output");
// [ts] const { client } = startClient({
// [ts] config: { outputDir },
// [ts] });
// [ts] expect(client.callTool({
// [ts] name: "browser_navigate",
// [ts] arguments: { url: server.PREFIX },
// [ts] })).toHaveResponse({
// [ts] code: expect.stringContaining(`page.goto('http://localhost`),
// [ts] });
// [ts] expect(client.callTool({
// [ts] name: "browser_take_screenshot",
// [ts] arguments: { type },
// [ts] })).toEqual({
// [ts] content: [
// [ts] {
// [ts] text: expect.stringMatching(
// [ts] RegExp.init(`page-\\d{4}-\\d{2}-\\d{2}T\\d{2}-\\d{2}-\\d{2}\\-\\d{3}Z\\.${type}`)
// [ts] ),
// [ts] type: "text",
// [ts] },
// [ts] {
// [ts] data: expect.any(String),
// [ts] mimeType: `image/${type}`,
// [ts] type: "image",
// [ts] },
// [ts] ],
// [ts] });
// [ts] const files = [...fs.readdirSync(outputDir)].filter(f => f.endsWith(`.${type}`));
// [ts] expect_eq(fs.existsSync(outputDir), true);
// [ts] expect(files).toHaveLength(1);
// [ts] expect(files[0]).toMatch(
// [ts] RegExp.init(`^page-\\d{4}-\\d{2}-\\d{2}T\\d{2}-\\d{2}-\\d{2}-\\d{3}Z\\.${type}$`)
// [ts] );
// [ts] });
// [ts] }











fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_take_screenshot (viewport)", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`await page.screenshot`),
    // [ts] attachments: [{
    // [ts] data: expect.any(String),
    // [ts] mimeType: "image/png",
    // [ts] type: "image",
    // [ts] }],
    // [ts] });
    // [ts] });
    // [ts] test("browser_take_screenshot (element)", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`[ref=e1]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: {
    // [ts] element: "hello button",
    // [ts] target: "e1",
    // [ts] },
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringContaining(`page.getByText('Hello, world!').screenshot`),
    // [ts] type: "text",
    // [ts] },
    // [ts] {
    // [ts] data: expect.any(String),
    // [ts] mimeType: "image/png",
    // [ts] type: "image",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("--output-dir should work", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] });
    // [ts] expect_eq(fs.existsSync(outputDir), true);
    // [ts] const files = [...fs.readdirSync(outputDir)].filter(f => f.endsWith(".png"));
    // [ts] expect(files).toHaveLength(1);
    // [ts] expect(files[0]).toMatch("^page-\\d{4}-\\d{2}-\\d{2}T\\d{2}-\\d{2}-\\d{2}-\\d{3}Z\\.png$");
    // [ts] });
    // [ts] test("browser_take_screenshot (default type should be png)", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.goto('${server.PREFIX}');`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringMatching(
    // [ts] RegExp.init(`page-\\d{4}-\\d{2}-\\d{2}T\\d{2}-\\d{2}-\\d{2}\\-\\d{3}Z\\.png`)
    // [ts] ),
    // [ts] type: "text",
    // [ts] },
    // [ts] {
    // [ts] data: expect.any(String),
    // [ts] mimeType: "image/png",
    // [ts] type: "image",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] const files = [...fs.readdirSync(outputDir)].filter(f => f.endsWith(".png"));
    // [ts] expect_eq(fs.existsSync(outputDir), true);
    // [ts] expect(files).toHaveLength(1);
    // [ts] expect(files[0]).toMatch("^page-\\d{4}-\\d{2}-\\d{2}T\\d{2}-\\d{2}-\\d{2}-\\d{3}Z\\.png$");
    // [ts] });
    // [ts] test("browser_take_screenshot (filename is empty string)", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: {
    // [ts] filename: "",
    // [ts] },
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringMatching(
    // [ts] RegExp.init(`page-\\d{4}-\\d{2}-\\d{2}T\\d{2}-\\d{2}-\\d{2}\\-\\d{3}Z\\.png`)
    // [ts] ),
    // [ts] type: "text",
    // [ts] },
    // [ts] {
    // [ts] data: expect.any(String),
    // [ts] mimeType: "image/png",
    // [ts] type: "image",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] const files = [...fs.readdirSync(outputDir)].filter(f => f.endsWith(".png"));
    // [ts] expect_eq(fs.existsSync(outputDir), true);
    // [ts] expect(files).toHaveLength(1);
    // [ts] expect(files[0]).toMatch(
    // [ts] RegExp.init(`^page-\\d{4}-\\d{2}-\\d{2}T\\d{2}-\\d{2}-\\d{2}-\\d{3}Z\\.png$`)
    // [ts] );
    // [ts] });
    // [ts] test("browser_take_screenshot (filename: \"output.png\")", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: {
    // [ts] filename: "output.png",
    // [ts] },
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringContaining(`output.png`),
    // [ts] type: "text",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] const files = [...fs.readdirSync(testInfo.outputPath())].filter(f => f.endsWith(".png"));
    // [ts] expect(files).toHaveLength(1);
    // [ts] expect(files[0]).toMatch("^output\\.png$");
    // [ts] });
    // [ts] test("browser_take_screenshot (filename: \"sub/dir/output.png\")", () => {
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: {
    // [ts] filename: "sub/dir/output.png",
    // [ts] },
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringContaining(`output.png`),
    // [ts] type: "text",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("sub", "dir", "output.png")), true);
    // [ts] });
    // [ts] test("browser_take_screenshot (imageResponses=omit)", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] outputDir,
    // [ts] imageResponses: "omit",
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringContaining(`await page.screenshot`),
    // [ts] type: "text",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("browser_take_screenshot (imageResponses=only)", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] args: ["--image-responses=only"],
    // [ts] config: { outputDir },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] data: expect.any(String),
    // [ts] mimeType: "image/png",
    // [ts] type: "image",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] expect(fs.readdirSync(outputDir).filter(f => f.endsWith(".png"))).toHaveLength(1);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { filename: "screenshot.png" },
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringContaining("[Screenshot of viewport](./screenshot.png)"),
    // [ts] type: "text",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("browser_take_screenshot (fullPage: true)", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] code: expect.stringContaining(`page.goto('http://localhost`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { fullPage: true },
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringContaining("fullPage: true"),
    // [ts] type: "text",
    // [ts] },
    // [ts] {
    // [ts] data: expect.any(String),
    // [ts] mimeType: "image/png",
    // [ts] type: "image",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("browser_take_screenshot preserves image dimensions", () => {
    // [ts] test(!["chrome", "msedge", "chromium"].includes(mcpBrowser ?? ""), "Non-chrome has unusual full page size");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // [ts] const expectations = [
    // [ts] { title: "2000x500", pageWidth: 2000, pageHeight: 500, expectedWidth: 2000, expectedHeight: 720 },
    // [ts] { title: "2000x2000", pageWidth: 2000, pageHeight: 2000, expectedWidth: 2000, expectedHeight: 2000 },
    // [ts] { title: "1280x800", pageWidth: 1280, pageHeight: 800, expectedWidth: 1280, expectedHeight: 800 },
    // [ts] ];
    // [ts] for (const expectation of expectations) {
    // [ts] test.step(expectation.title, () => {
    // [ts] server.setContent("/", `<body style="width: ${expectation.pageWidth}px; height: ${expectation.pageHeight}px; background: red; margin: 0;"></body>`, "text/html");
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.PREFIX } });
    // [ts] const pngResult = client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { fullPage: true },
    // [ts] });
    // [ts] const png = PNG.sync.read(Buffer.from(pngResult.content?.[1]?.data, "base64"));
    // [ts] expect_eq(png.width, expectation.expectedWidth);
    // [ts] expect_eq(png.height, expectation.expectedHeight);
    // [ts] const jpegResult = client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { fullPage: true, type: "jpeg" },
    // [ts] });
    // [ts] const jpeg = jpegjs.decode(Buffer.from(jpegResult.content?.[1]?.data, "base64"));
    // [ts] expect_eq(jpeg.width, expectation.expectedWidth);
    // [ts] expect_eq(jpeg.height, expectation.expectedHeight);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("browser_take_screenshot (scale: device)", () => {
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] outputDir: testInfo.outputPath("output"),
    // [ts] browser: {
    // [ts] contextOptions: {
    // [ts] viewport: { width: 400, height: 300 },
    // [ts] deviceScaleFactor: 2,
    // [ts] },
    // [ts] },
    // [ts] },
    // [ts] });
    // [ts] client.callTool({ name: "browser_navigate", arguments: { url: server.HELLO_WORLD } });
    // [ts] const cssResult = client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { scale: "css" },
    // [ts] });
    // [ts] const css = PNG.sync.read(Buffer.from(cssResult.content?.[1]?.data, "base64"));
    // [ts] expect_eq(css.width, 400);
    // [ts] expect_eq(css.height, 300);
    // [ts] const deviceResult = client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: { scale: "device" },
    // [ts] });
    // [ts] expect: (deviceResult) => {
    // [ts] code: expect.stringContaining(`scale: 'device'`),
    // [ts] });
    // [ts] const device = PNG.sync.read(Buffer.from(deviceResult.content?.[1]?.data, "base64"));
    // [ts] expect_eq(device.width, 800);
    // [ts] expect_eq(device.height, 600);
    // [ts] });
    // [ts] test("browser_take_screenshot (fullPage with element should error)", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`[ref=e1]`),
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: {
    // [ts] fullPage: true,
    // [ts] element: "hello button",
    // [ts] target: "e1",
    // [ts] },
    // [ts] });
    // [ts] expect_eq(result.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.content?.[0]?.text, "fullPage cannot be used with element screenshots") != null, true);
    // [ts] });
    // [ts] test("browser_take_screenshot (viewport without snapshot)", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir: testInfo.outputPath("output") },
    // [ts] });
    // Ensure we have a tab but don't navigate anywhere (no snapshot captured)
    // [ts] expect(client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: {
    // [ts] action: "list",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `- 0: (current) [](about:blank)`,
    // [ts] });
    // This should work without requiring a snapshot since it's a viewport screenshot
    // [ts] expect(client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] {
    // [ts] text: expect.stringContaining(`page.screenshot`),
    // [ts] type: "text",
    // [ts] },
    // [ts] {
    // [ts] data: expect.any(String),
    // [ts] mimeType: "image/png",
    // [ts] type: "image",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
}
