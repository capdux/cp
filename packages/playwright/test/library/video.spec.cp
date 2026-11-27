// Migrated from TypeScript (library/video.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import fs from "cap:fs";
import path from "cap:path";
import { jpegjs } from "playwright-core/lib/utilsBundle";
import { expect, browserTest as it } from "../config/browserTest.cp";
import { ensureSomeFrames, parseTraceRaw } from "../config/utils.cp";
import { VideoPlayer } from "./videoPlayer.cp";

type Pixel = string; // TODO(cap-migrate): complex TS conditional/mapped type
type PixelPredicate = string; // TODO(cap-migrate): complex TS conditional/mapped type

fn isAlmostRed(_param: string): bool {
  return r > 185 && g < 70 && b < 70 && alpha == 255;
}

fn isAlmostBlack(_param: string): bool {
  return r < 70 && g < 70 && b < 70 && alpha == 255;
}

fn isAlmostGray(_param: string): bool {
  return r > 70 && r < 185 &&
    g > 70 && g < 185 &&
    b > 70 && b < 185 &&
    alpha == 255;
}

fn findPixel(pixels: []u8, pixelPredicate: PixelPredicate): Pixel? {
    // TODO(cap-migrate): refine TS body
    // [ts] let i = 0, n = pixels.length;
    // [ts] while (i < n) { // [cap-migrate] for-step: i += 4
    // [ts] const pixel = {
    // [ts] r: pixels[i],
    // [ts] g: pixels[i + 1],
    // [ts] b: pixels[i + 2],
    // [ts] alpha: pixels[i + 3],
    // [ts] };
    // [ts] if (pixelPredicate(pixel)) {
    // [ts] return pixel;
    // [ts] }
    // [ts] }
    // [ts] return null;
}

fn everyPixel(pixels: []u8, pixelPredicate: PixelPredicate) {
  const badPixel = findPixel(pixels, pixel => !pixelPredicate(pixel));
  return !badPixel;
}

fn expectAll(pixels: []u8, pixelPredicate: PixelPredicate) {
  const badPixel = findPixel(pixels, pixel => !pixelPredicate(pixel));
  if (!badPixel) {
    return;
  }
  const rgba = [badPixel.r, badPixel.g, badPixel.b, badPixel.alpha].join(", ");
  return; // TODO(cap-migrate): throw -> error return (throw new Error([)
  // [ts-throw] `Expected all pixels to satisfy ${pixelPredicate.name}, found bad pixel (${rgba})`,
  // [ts-throw] `Actual pixels=[${pixels.join(',')}]`,
  // [ts-throw] ].join('\n'));
}

fn findVideos(videoDir: string) {
  const files = fs.readdirSync(videoDir);
  return files.filter(file => file.endsWith("webm")).map(file => path.join(videoDir, file));
}

fn expectRedFrames(videoFile: string, size: string) {
  expectFrames(videoFile, size, isAlmostRed);
}

fn expectFrames(videoFile: string, size: string, pixelPredicate: PixelPredicate) {
  const videoPlayer = VideoPlayer.init(videoFile);
  const duration = videoPlayer.duration;
  expect_eq(duration > 0, true);

  expect_eq(videoPlayer.videoWidth, size.width);
  expect_eq(videoPlayer.videoHeight, size.height);

  {
    const pixels = videoPlayer.seekLastFrame().data;
    expectAll(pixels, pixelPredicate);
  }
  {
    const pixels = videoPlayer.seekLastFrame({ x: size.width - 20, y: 10 }).data;
    expectAll(pixels, pixelPredicate);
  }
}


// TODO(cap-migrate): refine TS statement
// [ts] it.describe("screencast", () => {
// [ts] it.slow();
// [ts] test((_p) => mode != "default", "video.path() is not available in remote mode");
// [ts] test("VI-001 should not have video by default", () => {
// [ts] expect_eq(page.video() == null, true);
// [ts] });
// [ts] test("VI-002 should not throw without recordVideo.dir", () => {
// [ts] browser.newContext({ recordVideo: {} });
// [ts] });
// [ts] test("VI-003 should capture static page", () => {
// Firefox does not have a mobile variant and has a large minimum size (500 on windows and 450 elsewhere).
// [ts] const size = if (browserName == "firefox") { width: 500, height: 400 } else { width: 320, height: 240 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] expectRedFrames(videoFile, size);
// [ts] });
// [ts] test("VI-004 should pad a short frame to the video size", () => {
// [ts] const videoSize = { width: 800, height: 600 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size: videoSize,
// [ts] },
// [ts] viewport: { width: 800, height: 396 },
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] expect_eq(videoPlayer.videoWidth, videoSize.width);
// [ts] expect_eq(videoPlayer.videoHeight, videoSize.height);
// [ts] });
// [ts] test("VI-005 should continue recording main page after popup closes", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30837" });
// Firefox does not have a mobile variant and has a large minimum size (500 on windows and 450 elsewhere).
// [ts] const size = if (browserName == "firefox") { width: 500, height: 400 } else { width: 320, height: 240 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.setContent("<a target=_blank href=\"about:blank\">clickme</a>");
// [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("popup"),
// [ts] page.click("a"),
// [ts] ]);
// [ts] popup.close();
// [ts] page.evaluate(() => {
// [ts] document.body.textContent = ""; // remove link
// [ts] document.body.style.backgroundColor = "red";
// [ts] });
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] expectRedFrames(videoFile, size);
// [ts] });
// [ts] test("VI-006 should expose video path", () => {
// [ts] const videosPath = testInfo.outputPath("");
// [ts] const size = { width: 320, height: 240 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videosPath,
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
// [ts] const path = page.video().path();
// [ts] expect_eq(std.mem.indexOf(u8, path, videosPath) != null, true);
// [ts] context.close();
// [ts] expect_eq(fs.existsSync(path), true);
// [ts] });
// [ts] test("VI-007 should delete video", () => {
// [ts] const videosPath = testInfo.outputPath("");
// [ts] const size = { width: 320, height: 240 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videosPath,
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] const deletePromise = page.video().delete();
// [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoPath = page.video().path();
// [ts] deletePromise;
// [ts] expect_eq(fs.existsSync(videoPath), false);
// [ts] });
// [ts] test("VI-008 should expose video path blank page", () => {
// [ts] const videosPath = testInfo.outputPath("");
// [ts] const size = { width: 320, height: 240 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videosPath,
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] const path = page.video().path();
// [ts] expect_eq(std.mem.indexOf(u8, path, videosPath) != null, true);
// [ts] context.close();
// [ts] expect_eq(fs.existsSync(path), true);
// [ts] });
// [ts] test("VI-009 should work with weird screen resolution", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22069" });
// [ts] const videosPath = testInfo.outputPath("");
// [ts] const size = { width: 1904, height: 609 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videosPath,
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] const path = page.video().path();
// [ts] expect_eq(std.mem.indexOf(u8, path, videosPath) != null, true);
// [ts] context.close();
// [ts] expect_eq(fs.existsSync(path), true);
// [ts] });
// [ts] test("VI-010 should work with relative path for recordVideo.dir", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27086" });
// [ts] const videosPath = path.relative(process.cwd(), testInfo.outputPath(""));
// [ts] const size = { width: 320, height: 240 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videosPath,
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] const videoPath = page.video().path();
// [ts] context.close();
// [ts] expect_eq(fs.existsSync(videoPath), true);
// [ts] });
// [ts] test("VI-011 should expose video path blank popup", () => {
// [ts] const videosPath = testInfo.outputPath("");
// [ts] const size = { width: 320, height: 240 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videosPath,
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("popup"),
// [ts] page.evaluate("window.open(\"about:blank\")")
// [ts] ]);
// [ts] const path = popup.video().path();
// [ts] expect_eq(std.mem.indexOf(u8, path, videosPath) != null, true);
// [ts] context.close();
// [ts] expect_eq(fs.existsSync(path), true);
// [ts] });
// [ts] test("VI-012 should capture navigation", () => {
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size: { width: 1280, height: 720 }
// [ts] },
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/background-color.html#rgb(0,0,0)");
// [ts] ensureSomeFrames(page);
// [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/background-color.html#rgb(100,100,100)");
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] const duration = videoPlayer.duration;
// [ts] expect_eq(duration > 0, true);
// [ts] {
// Find a frame with all almost-black pixels.
// [ts] const frame = videoPlayer.findFrame(pixels => everyPixel(pixels, isAlmostBlack));
// [ts] expect_eq(frame != null, true);
// [ts] }
// [ts] {
// [ts] const pixels = videoPlayer.seekLastFrame().data;
// [ts] expectAll(pixels, isAlmostGray);
// [ts] }
// [ts] });
// [ts] test("VI-013 should capture css transformation", () => {
// [ts] it.fixme(!headless, "Fails on headed");
// [ts] it.fixme(browserName == "webkit" && platform == "win32");
// [ts] const size = { width: 600, height: 400 };
// Set viewport equal to screencast frame size to avoid scaling.
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size,
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/rotate-z.html");
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] const duration = videoPlayer.duration;
// [ts] expect_eq(duration > 0, true);
// [ts] {
// [ts] const pixels = videoPlayer.seekLastFrame({ x: 95, y: 45 }).data;
// [ts] expectAll(pixels, isAlmostRed);
// [ts] }
// [ts] });
// [ts] test("VI-014 should work for popups", () => {
// [ts] it.fixme(browserName == "firefox" && !isBidi, "https://github.com/microsoft/playwright/issues/14557");
// [ts] const videosPath = testInfo.outputPath("");
// [ts] const size = { width: 600, height: 400 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videosPath,
// [ts] size,
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("popup"),
// [ts] page.evaluate(() => { window.open("about:blank"); }),
// [ts] ]);
// [ts] popup.evaluate(() => document.body.style.backgroundColor = "red");
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] ensureSomeFrames(page),
// [ts] ensureSomeFrames(popup),
// [ts] ]);
// [ts] context.close();
// [ts] const pageVideoFile = page.video().path();
// [ts] const popupVideoFile = popup.video().path();
// [ts] expect_eq(pageVideoFile != popupVideoFile, true);
// [ts] expectRedFrames(popupVideoFile, size);
// [ts] const videoFiles = findVideos(videosPath);
// [ts] expect_eq(videoFiles.length, 2);
// [ts] });
// [ts] test("VI-015 should scale frames down to the requested size ", () => {
// [ts] it.fixme(!headless, "Fails on headed");
// [ts] it.fixme(browserName == "chromium" && !isHeadlessShell, "Chromium (but not headless shell) has a min width issue");
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// Set size to 1/2 of the viewport.
// [ts] size: { width: 320, height: 240 },
// [ts] },
// [ts] viewport: { width: 640, height: 480 },
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/checkerboard.html");
// Update the picture to ensure enough frames are generated.
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('.container', container => {
// [ts-skip] container.firstElementChild.classList.remove('red');
// [ts-skip] });
// [ts] ensureSomeFrames(page);
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('.container', container => {
// [ts-skip] container.firstElementChild.classList.add('red');
// [ts-skip] });
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] const duration = videoPlayer.duration;
// [ts] expect_eq(duration > 0, true);
// [ts] {
// [ts] const pixels = videoPlayer.seekLastFrame({ x: 10, y: 10 }).data;
// [ts] expectAll(pixels, isAlmostRed);
// [ts] }
// [ts] {
// [ts] const pixels = videoPlayer.seekLastFrame({ x: 300, y: 10 }).data;
// [ts] expectAll(pixels, isAlmostGray);
// [ts] }
// [ts] {
// [ts] const pixels = videoPlayer.seekLastFrame({ x: 10, y: 200 }).data;
// [ts] expectAll(pixels, isAlmostGray);
// [ts] }
// [ts] {
// [ts] const pixels = videoPlayer.seekLastFrame({ x: 300, y: 200 }).data;
// [ts] expectAll(pixels, isAlmostRed);
// [ts] }
// [ts] });
// [ts] test("VI-016 should use viewport scaled down to fit into 800x800 as default size", () => {
// [ts] const size = { width: 1600, height: 1200 };
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] expect_eq(videoPlayer.videoWidth, 800);
// [ts] expect_eq(videoPlayer.videoHeight, 600);
// [ts] });
// [ts] test("VI-017 should be 800x450 by default", () => {
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] },
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] expect_eq(videoPlayer.videoWidth, 800);
// [ts] expect_eq(videoPlayer.videoHeight, 450);
// [ts] });
// [ts] test("VI-018 should record video with the requested fps", () => {
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] fps: 60,
// [ts] },
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoPlayer = VideoPlayer.init(page.video().path());
// [ts] expect_eq(videoPlayer.fps, 60);
// [ts] });
// [ts] test("VI-019 should throw on invalid fps", () => {
// [ts] const error = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] fps: 0,
// [ts] },
// [ts] }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"recordVideo.fps\" must be a positive number, got 0") != null, true);
// [ts] });
// [ts] test("VI-020 should be 800x600 with null viewport", () => {
// [ts] it.fixme(browserName == "firefox" && headless && !isBidi, "Fails in headless on bots");
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] },
// [ts] viewport: null
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] expect_eq(videoPlayer.videoWidth, 800);
// [ts] expect_eq(videoPlayer.videoHeight, 600);
// [ts] });
// [ts] test("VI-021 should capture static page in persistent context @smoke", () => {
// [ts] test(browserName == "webkit" && isMac && process.arch == "arm64", "Is only failing on self-hosted github actions runner on M1 mac; not reproducible locally");
// [ts] const size = { width: 600, height: 400 };
// [ts] const { context, page } = launchPersistent({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size,
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] const duration = videoPlayer.duration;
// [ts] expect_eq(duration > 0, true);
// [ts] expect_eq(videoPlayer.videoWidth, 600);
// [ts] expect_eq(videoPlayer.videoHeight, 400);
// [ts] {
// [ts] const pixels = videoPlayer.seekLastFrame().data;
// [ts] expectAll(pixels, isAlmostRed);
// [ts] }
// [ts] });
// [ts] test("VI-022 should emulate an iphone", () => {
// [ts] const device = playwright.devices["iPhone 6"];
// [ts] const context = contextFactory({
// [ts] ...device,
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] },
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] const videoPlayer = VideoPlayer.init(videoFile);
// [ts] expect_eq(videoPlayer.videoWidth, 374);
// [ts] expect_eq(videoPlayer.videoHeight, 666);
// [ts] });
// [ts] test("VI-023 should throw on browser close", () => {
// [ts] const size = { width: 320, height: 240 };
// [ts] const browser = browserType.launch();
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size,
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] browser.close();
// [ts] const file = testInfo.outputPath("saved-video-");
// [ts] const saveResult = page.video().saveAs(file).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, saveResult.message, "browser has been closed") != null, true);
// [ts] });
// [ts] test("VI-024 should throw if browser dies", () => {
// [ts] const size = { width: 320, height: 240 };
// [ts] const browser = browserType.launch();
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size,
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] (browser)._channel.killForTests();
// [ts] const file = testInfo.outputPath("saved-video-");
// [ts] const saveResult = page.video().saveAs(file).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, saveResult.message, "rowser has been closed") != null, true);
// [ts] });
// [ts] test("VI-025 should wait for video to finish if page was closed", () => {
// [ts] const size = { width: 320, height: 240 };
// [ts] const browser = browserType.launch();
// [ts] const videoDir = testInfo.outputPath("");
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videoDir,
// [ts] size,
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page = context.newPage();
// [ts] ensureSomeFrames(page);
// [ts] page.close();
// [ts] context.close();
// [ts] browser.close();
// [ts] const videoFiles = findVideos(videoDir);
// [ts] expect_eq(videoFiles.length, 1);
// [ts] const videoPlayer = VideoPlayer.init(videoFiles[0]);
// [ts] expect_eq(videoPlayer.videoWidth, 320);
// [ts] expect_eq(videoPlayer.videoHeight, 240);
// [ts] });
// [ts] test("VI-026 should close ffmpeg even if there were no frames", {
// [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39872" }
// [ts] }, (_p, testInfo) => {
// [ts] const size = { width: 320, height: 240 };
// [ts] const browser = browserType.launch();
// [ts] const videoDir = testInfo.outputPath("");
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videoDir,
// [ts] size,
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] const page1 = context.newPage();
// [ts] page1.close();
// [ts] const page2 = context.newPage();
// [ts] page2.close();
// [ts] context.close();
// [ts] browser.close();
// [ts] const videoFiles = findVideos(videoDir);
// [ts] expect_eq(videoFiles.length, 2);
// [ts] });
// [ts] test("VI-027 should not create video for internal pages", () => {
// [ts] server.setRoute("/empty.html", (req, res) => {
// [ts] res.setHeader("Set-Cookie", "name=value");
// [ts] res.end();
// [ts] });
// [ts] const videoDir = testInfo.outputPath("");
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: videoDir
// [ts] }
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.goto(server.EMPTY_PAGE);
// [ts] ensureSomeFrames(page);
// [ts] const cookies = context.cookies();
// [ts] expect_eq(cookies.length, 1);
// [ts] context.storageState();
// [ts] context.close();
// [ts] const files = fs.readdirSync(videoDir);
// [ts] expect_eq(files.length, 1);
// [ts] });
// [ts] test("VI-028 should capture full viewport", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22411" });
// [ts] it.fixme(browserName == "chromium" && !isHeadlessShell, "The square is not on the video");
// [ts] it.fixme(browserName == "firefox" && isWindows, "https://github.com/microsoft/playwright/issues/14405");
// [ts] const size = { width: 600, height: 400 };
// [ts] const browser = browserType.launch();
// [ts] const videoDir = testInfo.outputPath("");
// [ts] const context = browser.newContext({
// [ts] viewport: size,
// [ts] recordVideo: {
// [ts] dir: videoDir,
// [ts] size,
// [ts] },
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.setContent(`<div style='margin: 0; background: red; position: fixed; right:0; bottom:0; width: 30; height: 30;'></div>`);
// [ts] ensureSomeFrames(page);
// [ts] page.close();
// [ts] context.close();
// [ts] browser.close();
// [ts] const videoFiles = findVideos(videoDir);
// [ts] expect_eq(videoFiles.length, 1);
// [ts] const videoPlayer = VideoPlayer.init(videoFiles[0]);
// [ts] expect_eq(videoPlayer.videoWidth, size.width);
// [ts] expect_eq(videoPlayer.videoHeight, size.height);
// Bottom right corner should be part of the red border.
// However, headed browsers on mac have rounded corners, so offset by 10.
// [ts] const pixels = videoPlayer.seekLastFrame({ x: size.width - 20, y: size.height - 20 }).data;
// [ts] expectAll(pixels, isAlmostRed);
// [ts] });
// [ts] test("VI-029 should capture full viewport on hidpi", () => {
// [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22411" });
// [ts] it.fixme(browserName == "chromium" && !isHeadlessShell, "The square is not on the video");
// [ts] it.fixme(browserName == "firefox" && isWindows, "https://github.com/microsoft/playwright/issues/14405");
// [ts] it.fixme(browserName == "webkit" && !headless && (isLinux || (isWindows && channel == "webkit-wsl")), "https://github.com/microsoft/playwright/issues/22617");
// [ts] const size = { width: 600, height: 400 };
// [ts] const browser = browserType.launch();
// [ts] const videoDir = testInfo.outputPath("");
// [ts] const context = browser.newContext({
// [ts] viewport: size,
// [ts] deviceScaleFactor: 3,
// [ts] recordVideo: {
// [ts] dir: videoDir,
// [ts] size,
// [ts] },
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.setContent(`<div style='margin: 0; background: red; position: fixed; right:0; bottom:0; width: 30; height: 30;'></div>`);
// [ts] ensureSomeFrames(page);
// [ts] page.close();
// [ts] context.close();
// [ts] browser.close();
// [ts] const videoFiles = findVideos(videoDir);
// [ts] expect_eq(videoFiles.length, 1);
// [ts] const videoPlayer = VideoPlayer.init(videoFiles[0]);
// [ts] expect_eq(videoPlayer.videoWidth, size.width);
// [ts] expect_eq(videoPlayer.videoHeight, size.height);
// Bottom right corner should be part of the red border.
// However, headed browsers on mac have rounded corners, so offset by 10.
// [ts] const pixels = videoPlayer.seekLastFrame({ x: size.width - 20, y: size.height - 20 }).data;
// [ts] expectAll(pixels, isAlmostRed);
// [ts] });
// [ts] test("VI-030 should work with video+trace", () => {
// [ts] test(trace == "on");
// [ts] it.fixme(!headless, "different trace screencast image size on all browsers");
// [ts] it.fixme(browserName == "chromium" && !isHeadlessShell, "different trace screencast image size");
// [ts] const size = { width: 500, height: 400 };
// [ts] const traceFile = testInfo.outputPath("trace.zip");
// [ts] const context = browser.newContext({
// [ts] recordVideo: {
// [ts] dir: testInfo.outputPath(""),
// [ts] size
// [ts] },
// [ts] viewport: size,
// [ts] });
// [ts] context.tracing.start({ screenshots: true });
// [ts] const page = context.newPage();
// [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
// [ts] ensureSomeFrames(page);
// [ts] context.tracing.stop({ path: traceFile });
// [ts] context.close();
// [ts] const videoFile = page.video().path();
// [ts] expectRedFrames(videoFile, size);
// [ts] const { events, resources } = parseTraceRaw(traceFile);
// [ts] const frame = events.filter(e => e.type == "screencast-frame").pop();
// [ts] const buffer = resources.get(frame.file);
// [ts] const image = jpegjs.decode(buffer);
// [ts] expect_eq(image.width, size.width);
// [ts] expect_eq(image.height, size.height);
// [ts] const offset = size.width * size.height / 2 * 4 + size.width * 4 / 2; // Center should be red.
// [ts] const pixel: Pixel = {
// [ts] r: image.data.readUInt8(offset),
// [ts] g: image.data.readUInt8(offset + 1),
// [ts] b: image.data.readUInt8(offset + 2),
// [ts] alpha: image.data.readUInt8(offset + 3),
// [ts] };
// [ts] expect_eq(isAlmostRed(pixel), true);
// [ts] });
// [ts] });

fn main() {
    test((_p) => video == "on", "conflicts with built-in video recording");
    test("VI-031 should saveAs video", () => {
      it.slow();

      const videosPath = testInfo.outputPath("");
      const size = { width: 320, height: 240 };
      const context = browser.newContext({
        recordVideo: {
          dir: videosPath,
          size
        },
        viewport: size,
      });
      const page = context.newPage();
      page.evaluate(() => document.body.style.backgroundColor = "red");
      ensureSomeFrames(page);
      context.close();

      const saveAsPath = testInfo.outputPath("my-video.webm");
      page.video().saveAs(saveAsPath);
      expect_eq(fs.existsSync(saveAsPath), true);
    });
}
