// Migrated from TypeScript (library/screencast.spec.ts) via cap from -ts

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
import { expect, browserTest as test } from "../config/browserTest.cp";
import { ensureSomeFrames } from "../config/utils.cp";
import { kTargetClosedErrorMessage } from "../config/errors.cp";
import { VideoPlayer } from "./videoPlayer.cp";

test.slow();
















type Pixel = string; // TODO(cap-migrate): complex TS conditional/mapped type
type PixelPredicate = string; // TODO(cap-migrate): complex TS conditional/mapped type

fn isAlmostWhite(_param: string): bool {
  return r > 185 && g > 185 && b > 185 && alpha == 255;
}

fn isAlmostRed(_param: string): bool {
  return r > 185 && g < 70 && b < 70 && alpha == 255;
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

fn jpegDimensions(buffer: []u8) {
    // TODO(cap-migrate): refine TS body
    // [ts] width: number, height: number
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default", "screencast is not available in remote mode");
    // [ts] test((_p) => video == "on", "conflicts with built-in video recording");
    // [ts] test("SC-001 screencast.start delivers frames via onFrame callback", () => {
    // [ts] test(trace == "on", "trace=on has different screencast image configuration");
    // [ts] test.fixme(browserName == "firefox" && isMac && !headless, "wrong frame size in headed Firefox on Mac");
    // [ts] const context = browser.newContext({ viewport: { width: 1000, height: 400 } });
    // [ts] const page = context.newPage();
    // [ts] const frames: [][]u8 = [];
    // [ts] const size = { width: 500, height: 400 };
    // [ts] page.screencast.start({ onFrame: (_p) => frames.push(data), size });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] page.screencast.stop();
    // [ts] expect_eq(frames.length > 0, true);
    // [ts] for (const frame of frames) {
    // Each frame must be a valid JPEG (starts with FF D8)
    // [ts] expect_eq(frame[0], 0xff);
    // [ts] expect_eq(frame[1], 0xd8);
    // [ts] }
    // Chromium scales the first frame against the window size it had before the viewport
    // was applied, so only the last frame is reliably scaled down to fit the maximum size.
    // [ts] const { width, height } = jpegDimensions(frames[frames.length - 1]);
    // [ts] expect_eq(width, 500);
    // [ts] expect_eq(height, 200);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-002 applies backpressure while async onFrame callback is pending", () => {
    // [ts] test(trace == "on", "trace recording acknowledges screencast frames independently");
    // [ts] const context = browser.newContext({ viewport: { width: 500, height: 400 } });
    // [ts] const page = context.newPage();
    // [ts] let releaseCallback: () => void;
    // [ts] const callbackDone = Promise.init(f => releaseCallback = f);
    // [ts] let firstFrame: () => void;
    // [ts] const firstFrameReceived = Promise.init(f => firstFrame = f);
    // [ts] let frameCount = 0;
    // [ts] let lastFrameTimestamp = 0;
    // [ts] page.screencast.start({
    // [ts] onFrame: () => {
    // [ts] ++frameCount;
    // [ts] lastFrameTimestamp = Date.now();
    // [ts] firstFrame();
    // [ts] callbackDone;
    // [ts] },
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => {
    // [ts] const animate = () => {
    // [ts] document.body.style.backgroundColor = if (document.body.style.backgroundColor == "red") "blue" else "red";
    // [ts] requestAnimationFrame(animate);
    // [ts] };
    // [ts] requestAnimationFrame(animate);
    // [ts] });
    // [ts] firstFrameReceived;
    // [ts] expect.poll(() => Date.now() - lastFrameTimestamp, { timeout: 30000 }).toBeGreaterThan(1000);
    // [ts] const framesWhileBlocked = frameCount;
    // [ts] ensureSomeFrames(page);
    // [ts] expect_eq(frameCount, framesWhileBlocked);
    // [ts] releaseCallback();
    // [ts] expect.poll(() => {
    // [ts] page.evaluate(() => {
    // [ts] document.body.style.backgroundColor = if (document.body.style.backgroundColor == "red") "blue" else "red";
    // [ts] });
    // [ts] ensureSomeFrames(page);
    // [ts] return frameCount;
    // [ts] }, { timeout: 30000 }).toBeGreaterThan(framesWhileBlocked);
    // [ts] page.screencast.stop();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-003 onFrame receives viewport size", () => {
    // [ts] test(trace == "on", "trace=on has different screencast image configuration");
    // [ts] test.fixme(browserName == "firefox" && isMac && !headless, "wrong frame size in headed Firefox on Mac");
    // [ts] const context = browser.newContext({ viewport: { width: 1000, height: 400 } });
    // [ts] const page = context.newPage();
    // [ts] const frames = [];
    // [ts] page.screencast.start({
    // [ts] onFrame: (_p) => frames.push({ timestamp, viewportWidth, viewportHeight }),
    // [ts] size: { width: 500, height: 400 },
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] ensureSomeFrames(page);
    // [ts] page.screencast.stop();
    // [ts] expect_eq(frames.length > 0, true);
    // [ts] for (const frame of frames) {
    // [ts] expect_eq(frame.viewportWidth, 1000);
    // [ts] expect_eq(frame.viewportHeight, 400);
    // [ts] expect_eq(@typeName(frame.timestamp), "number");
    // [ts] }
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-004 start throws if screencast is already started", () => {
    // [ts] const context = browser.newContext({ viewport: { width: 500, height: 400 } });
    // [ts] const page = context.newPage();
    // [ts] page.screencast.start({ onFrame: () => {} });
    // [ts] expect(page.screencast.start({ onFrame: () => {} })).rejects.toThrow("Screencast is already started");
    // [ts] page.screencast.stop();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-005 start allows restart with different options after stop", () => {
    // [ts] test(trace == "on", "trace=on enables screencast with different options");
    // [ts] const context = browser.newContext({ viewport: { width: 500, height: 400 } });
    // [ts] const page = context.newPage();
    // [ts] page.screencast.start({ onFrame: () => {}, size: { width: 500, height: 400 } });
    // [ts] page.screencast.stop();
    // Different options should succeed once the previous screencast is stopped.
    // [ts] page.screencast.start({ onFrame: () => {}, size: { width: 320, height: 240 } });
    // [ts] page.screencast.stop();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-006 start returns a disposable that stops screencast", () => {
    // [ts] test(trace == "on", "trace=on has different screencast image configuration");
    // [ts] const context = browser.newContext({ viewport: { width: 500, height: 400 } });
    // [ts] const page = context.newPage();
    // [ts] const frames: [][]u8 = [];
    // [ts] page.screencast.start({ onFrame: (_p) => frames.push(data), size: { width: 500, height: 400 } });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] page.screencast.stop();
    // [ts] const frameCountAfterDispose = frames.length;
    // [ts] expect_eq(frameCountAfterDispose > 0, true);
    // No more frames should arrive after dispose.
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "blue");
    // [ts] ensureSomeFrames(page);
    // [ts] expect_eq(frames.length, frameCountAfterDispose);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-007 start/stop twice without path creates two files in artifactsDir", () => {
    // [ts] test.slow();
    // [ts] const artifactsDir = testInfo.outputPath("artifacts");
    // [ts] const browser = browserType.launch({ artifactsDir });
    // [ts] const size = { width: 800, height: 800 };
    // [ts] const context = browser.newContext({ viewport: size });
    // [ts] const page = context.newPage();
    // [ts] page.screencast.start({ path: testInfo.outputPath("video1.webm"), size });
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] page.screencast.stop();
    // [ts] page.screencast.start({ path: testInfo.outputPath("video2.webm"), size });
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "blue");
    // [ts] ensureSomeFrames(page);
    // [ts] page.screencast.stop();
    // [ts] const videoFiles = fs.readdirSync(artifactsDir).filter(f => f.endsWith(".webm"));
    // [ts] expect(videoFiles).toHaveLength(2);
    // [ts] context.close();
    // [ts] browser.close();
    // [ts] });
    // [ts] test("SC-008 start should work when recordVideo is set", () => {
    // [ts] test.slow();
    // [ts] const autoDir = testInfo.outputPath("auto");
    // [ts] const manualDir = testInfo.outputPath("manual");
    // [ts] const context = browser.newContext({
    // [ts] recordVideo: {
    // [ts] dir: autoDir,
    // [ts] },
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.screencast.start({ path: path.join(manualDir, "video.webm") });
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "blue");
    // [ts] ensureSomeFrames(page);
    // [ts] page.screencast.stop();
    // [ts] const videoFiles1 = fs.readdirSync(manualDir).filter(f => f.endsWith(".webm"));
    // [ts] expect(videoFiles1).toHaveLength(1);
    // [ts] context.close();
    // [ts] const videoFiles2 = fs.readdirSync(autoDir).filter(f => f.endsWith(".webm"));
    // [ts] expect(videoFiles2).toHaveLength(1);
    // [ts] });
    // [ts] test("SC-009 start should record video with the requested fps", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const videoPath = testInfo.outputPath("video.webm");
    // [ts] page.screencast.start({ path: videoPath, fps: 60 });
    // [ts] ensureSomeFrames(page);
    // [ts] page.screencast.stop();
    // [ts] expect_eq(VideoPlayer.init(videoPath).fps, 60);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-010 start should throw on invalid fps", () => {
    // [ts] const error = page.screencast.start({ path: testInfo.outputPath("video.webm"), fps: -1 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"fps\" must be a positive number, got -1") != null, true);
    // [ts] });
    // [ts] test("SC-011 start should fail when another recording is in progress", () => {
    // [ts] test(trace == "on", "trace=on has different screencast image configuration");
    // [ts] page.screencast.start({ path: testInfo.outputPath("video.webm") });
    // [ts] const error = page.screencast.start({ path: testInfo.outputPath("video2.webm") }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Screencast is already started") != null, true);
    // [ts] });
    // [ts] test("SC-012 stop should not fail when no recording is in progress", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // stop() is a no-op when screencast is not started.
    // [ts] page.screencast.stop();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-013 start should finish when page is closed", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] const videoPath = testInfo.outputPath("video.webm");
    // [ts] page.screencast.start({ path: videoPath, size: { width: 800, height: 800 } });
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] page.close();
    // [ts] const error = page.screencast.stop().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, kTargetClosedErrorMessage) != null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-014 empty video", () => {
    // [ts] const size = { width: 800, height: 800 };
    // [ts] const context = browser.newContext({ viewport: size });
    // [ts] const page = context.newPage();
    // [ts] const videoPath = testInfo.outputPath("empty-video.webm");
    // [ts] page.screencast.start({ path: videoPath, size });
    // [ts] page.screencast.stop();
    // [ts] context.close();
    // [ts] expectFrames(videoPath, size, isAlmostWhite);
    // [ts] });
    // [ts] test("SC-015 start dispose stops recording", () => {
    // [ts] test.slow();
    // [ts] const size = { width: 800, height: 800 };
    // [ts] const context = browser.newContext({ viewport: size });
    // [ts] const page = context.newPage();
    // [ts] const videoPath = testInfo.outputPath("dispose-video.webm");
    // [ts] const disposable = page.screencast.start({ path: videoPath, size });
    // [ts] page.evaluate(() => document.body.style.backgroundColor = "red");
    // [ts] ensureSomeFrames(page);
    // [ts] disposable.dispose();
    // [ts] expectRedFrames(videoPath, size);
    // [ts] context.close();
    // [ts] });
}
