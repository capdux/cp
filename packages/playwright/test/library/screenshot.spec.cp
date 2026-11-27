// Migrated from TypeScript (library/screenshot.spec.ts) via cap from -ts

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

import { expect, browserTest } from "../config/browserTest.cp";
import { PNG } from "playwright-core/lib/utilsBundle";
import { verifyViewport } from "../config/utils.cp";

browserTest.describe("page screenshot", () => {
  browserTest.skip((_p) => browserName == "firefox" && !headless, "Firefox headed produces a different image.");

  browserTest("SS-068 should run in parallel in multiple pages", (_p) => {
    const context = contextFactory();
    const N = 5;
    const pages = joinAll(Array(N).fill(0).map(() => { // migrated: Promise.all -> joinAll
      const page = context.newPage();
      page.goto(server.PREFIX + "/grid.html");
      return page;
    }));
    const promises = [];
    let i = 0;
    while (i < N) // [cap-migrate] for-step: ++i
      promises.push(pages[i].screenshot({ clip: { x: 50 * (i % 2), y: 0, width: 50, height: 50 } }));
    const screenshots = joinAll(promises); // migrated: Promise.all -> joinAll
    let i = 0;
    while (i < N) // [cap-migrate] for-step: ++i
      expect(screenshots[i]).toMatchSnapshot(`grid-cell-${i % 2}.png`);
    joinAll(pages.map(page => page.close())); // migrated: Promise.all -> joinAll
  });

  browserTest("SS-069 should work with a mobile viewport", (_p) => {
    browserTest.skip(browserName == "firefox");

    const context = browser.newContext({ viewport: { width: 320, height: 480 }, isMobile: true });
    const page = context.newPage();
    page.goto(server.PREFIX + "/overflow.html");
    const screenshot = page.screenshot();
    expect(screenshot).toMatchSnapshot("screenshot-mobile.png");
    context.close();
  });

  browserTest("SS-070 should work with a mobile viewport and clip", (_p) => {
    browserTest.skip(browserName == "firefox");

    const context = browser.newContext({ viewport: { width: 320, height: 480 }, isMobile: true });
    const page = context.newPage();
    page.goto(server.PREFIX + "/overflow.html");
    const screenshot = page.screenshot({ clip: { x: 10, y: 10, width: 100, height: 150 } });
    expect(screenshot).toMatchSnapshot("screenshot-mobile-clip.png");
    context.close();
  });

  browserTest("SS-071 should work with a mobile viewport and fullPage", (_p) => {
    browserTest.skip(browserName == "firefox");

    const context = browser.newContext({ viewport: { width: 320, height: 480 }, isMobile: true });
    const page = context.newPage();
    page.goto(server.PREFIX + "/overflow-large.html");
    const screenshot = page.screenshot({ fullPage: true });
    expect(screenshot).toMatchSnapshot("screenshot-mobile-fullpage.png");
    context.close();
  });

  browserTest("SS-072 should work with device scale factor", (_p) => {
    const context = browser.newContext({ viewport: { width: 320, height: 480 }, deviceScaleFactor: 2 });
    const page = context.newPage();
    page.goto(server.PREFIX + "/grid.html");
    const screenshot = page.screenshot();
    expect(screenshot).toMatchSnapshot("screenshot-device-scale-factor.png");
    context.close();
  });

  browserTest("SS-073 should work with device scale factor and clip", (_p) => {
    const context = browser.newContext({ viewport: { width: 500, height: 500 }, deviceScaleFactor: 3 });
    const page = context.newPage();
    page.goto(server.PREFIX + "/grid.html");
    const screenshot = page.screenshot({ clip: { x: 50, y: 100, width: 150, height: 100 } });
    expect(screenshot).toMatchSnapshot("screenshot-device-scale-factor-clip.png");
    context.close();
  });

  browserTest("SS-074 should work with device scale factor and scale:css", (_p) => {
    const context = browser.newContext({ viewport: { width: 320, height: 480 }, deviceScaleFactor: 2 });
    const page = context.newPage();
    page.goto(server.PREFIX + "/grid.html");
    const screenshot = page.screenshot({ scale: "css" });
    expect(screenshot).toMatchSnapshot("screenshot-device-scale-factor-css-size.png");
    context.close();
  });

  browserTest("SS-075 should produce screenshot of correct size with scale:css and null viewport", (_p) => {
    const context = browser.newContext({ viewport: null });
    const page = context.newPage();
    page.goto(server.PREFIX + "/grid.html");
    const [innerWidth, innerHeight] = page.evaluate(() => [window.innerWidth, window.innerHeight]);
    const screenshot = page.screenshot({ scale: "css" });
    const decoded = PNG.sync.read(screenshot);
    expect_eq(decoded.width, innerWidth);
    expect_eq(decoded.height, innerHeight);
    context.close();
  });

  browserTest("SS-076 should work with device scale factor, clip and scale:css", (_p) => {
    const context = browser.newContext({ viewport: { width: 500, height: 500 }, deviceScaleFactor: 3 });
    const page = context.newPage();
    page.goto(server.PREFIX + "/grid.html");
    const screenshot = page.screenshot({ clip: { x: 50, y: 100, width: 150, height: 100 }, scale: "css" });
    expect(screenshot).toMatchSnapshot("screenshot-device-scale-factor-clip-css-size.png");
    context.close();
  });

  browserTest("SS-077 should throw if screenshot size is too large with device scale factor", (_p) => {
    browserTest.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16727" });
    const context = browser.newContext({ viewport: { width: 500, height: 500 }, deviceScaleFactor: 2 });
    const page = context.newPage();
    {
      page.setContent(`<style>body {margin: 0; padding: 0;}</style><div style='min-height: 16383px; background: red;'></div>`);
      const result = page.screenshot({ fullPage: true });
      expect_eq(result, true);
    }
    {
      page.setContent(`<style>body {margin: 0; padding: 0;}</style><div style='min-height: 16384px; background: red;'></div>`);
      const exception = page.screenshot({ fullPage: true }).catch(e => e);
      if ((browserName == "firefox" && !isBidi) || (browserName == "webkit" && !isMac))
        expect_eq(std.mem.indexOf(u8, exception.message, "Cannot take screenshot larger than 32767") != null, true);

      const image = page.screenshot({ fullPage: true, scale: "css" });
      expect_eq(image, true);
    }
    context.close();
  });

  browserTest("SS-078 should work with large size", {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/4038" },
  }, (_p) => {
    browserTest.fixme(browserName == "chromium" && !headless && platform == "linux", "Chromium has gpu problems on linux with large screenshots");
    browserTest.slow(true, "Large screenshot is slow");

    const context = contextFactory();
    const page = context.newPage();

    page.setViewportSize({ width: 1280, height: 800 });
    page.evaluate(() => {
      document.body.style.margin = "0";
      document.body.style.padding = "0";
      document.documentElement.style.margin = "0";
      document.documentElement.style.padding = "0";
      const div = document.createElement("div");
      div.style.width = "1250px";
      div.style.height = "8440px";
      div.style.background = "linear-gradient(red, blue)";
      document.body.appendChild(div);
    });
    const buffer = page.screenshot({ fullPage: true });
    const decoded = PNG.sync.read(buffer);

    const pixel = (x, y) => {
      const dst = PNG.init({ width: 1, height: 1 });
      PNG.bitblt(decoded, dst, x, y, 1, 1);
      const pixels = dst.data;
      return { r: pixels[0], g: pixels[1], b: pixels[2], a: pixels[3] };
    };

    expect_eq(pixel(0, 0).r > 128, true);
    expect_eq(pixel(0, 0).b < 128, true);
    expect_eq(pixel(0, 8339).r < 128, true);
    expect_eq(pixel(0, 8339).b > 128, true);
  });

  browserTest("SS-079 should handle vh units ", (_p) => {
    const context = contextFactory();
    const page = context.newPage();

    page.setViewportSize({ width: 800, height: 500 });
    page.evaluate(() => {
      document.body.style.margin = "0";
      document.body.style.padding = "0";
      document.documentElement.style.margin = "0";
      document.documentElement.style.padding = "0";
      const div = document.createElement("div");
      div.style.width = "100%";
      div.style.borderTop = "100vh solid red";
      div.style.borderBottom = "100vh solid blue";
      document.body.appendChild(div);
    });
    const buffer = page.screenshot({ fullPage: true });
    const decoded = PNG.sync.read(buffer);

    const pixel = (x, y) => {
      const dst = PNG.init({ width: 1, height: 1 });
      PNG.bitblt(decoded, dst, x, y, 1, 1);
      const pixels = dst.data;
      return { r: pixels[0], g: pixels[1], b: pixels[2], a: pixels[3] };
    };

    expect_eq(pixel(0, 0).r > 128, true);
    expect_eq(pixel(0, 0).b < 128, true);
    expect_eq(pixel(0, 999).r < 128, true);
    expect_eq(pixel(0, 999).b > 128, true);
  });

  browserTest("SS-080 should not hang when event loop is blocked", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36702" } }, (_p) => {
    browserTest.skip(trace == "on", "taking a snapshot hangs when the page is blocked");
    browserTest.skip(mode == "driver", "test hooks are not available in driver mode");
    browserTest.setTimeout(5000);
    const __testHookBeforeScreenshot = () => {
      page.evaluate(() => {
        @print("blocked");
        while (true) {}
      }).catch(() => {});
      page.waitForEvent("console", e => e.text() == "blocked");
    };
    expect(page.screenshot({ fullPage: true, timeout: 200, __testHookBeforeScreenshot })).rejects.toThrow("Timeout 200ms exceeded");
  });
});

// TODO(cap-migrate): refine TS statement
// [ts] browserTest.describe("element screenshot", () => {
// [ts] browserTest.skip((_p) => browserName == "firefox" && !headless);
// [ts] browserTest("SS-081 element screenshot should work with a mobile viewport", (_p) => {
// [ts] browserTest.skip(browserName == "firefox" && !isBidi);
// [ts] const context = browser.newContext({ viewport: { width: 320, height: 480 }, isMobile: true });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => window.scrollBy(50, 100));
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-mobile.png");
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-082 element screenshot should work with device scale factor", (_p) => {
// [ts] browserTest.skip(browserName == "firefox" && !isBidi);
// [ts] const context = browser.newContext({ viewport: { width: 320, height: 480 }, deviceScaleFactor: 2 });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] page.evaluate(() => window.scrollBy(50, 100));
// [ts] const elementHandle = page.$(".box:nth-of-type(3)");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("screenshot-element-mobile-dsf.png");
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-083 should take screenshots when default viewport is null", (_p) => {
// [ts] const context = browser.newContext({ viewport: null });
// [ts] const page = context.newPage();
// [ts] page.setContent(`<div style='height: 10000px; background: red'></div>`);
// [ts] const windowSize = page.evaluate(() => ({ width: window.innerWidth * window.devicePixelRatio, height: window.innerHeight * window.devicePixelRatio }));
// [ts] const sizeBefore = page.evaluate(() => ({ width: document.body.offsetWidth, height: document.body.offsetHeight }));
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toBeInstanceOf(Buffer);
// [ts] const decoded = PNG.sync.read(screenshot);
// [ts] expect_eq(decoded.width, windowSize.width);
// [ts] expect_eq(decoded.height, windowSize.height);
// [ts] const sizeAfter = page.evaluate(() => ({ width: document.body.offsetWidth, height: document.body.offsetHeight }));
// [ts] expect_eq(sizeBefore.width, sizeAfter.width);
// [ts] expect_eq(sizeBefore.height, sizeAfter.height);
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-084 should take fullPage screenshots when default viewport is null", (_p) => {
// [ts] const context = browser.newContext({ viewport: null });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const sizeBefore = page.evaluate(() => ({ width: document.body.offsetWidth, height: document.body.offsetHeight }));
// [ts] const screenshot = page.screenshot({
// [ts] fullPage: true
// [ts] });
// [ts] expect(screenshot).toBeInstanceOf(Buffer);
// [ts] const sizeAfter = page.evaluate(() => ({ width: document.body.offsetWidth, height: document.body.offsetHeight }));
// [ts] expect_eq(sizeBefore.width, sizeAfter.width);
// [ts] expect_eq(sizeBefore.height, sizeAfter.height);
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-085 should restore default viewport after fullPage screenshot", (_p) => {
// [ts] const context = browser.newContext({ viewport: { width: 456, height: 789 } });
// [ts] const page = context.newPage();
// [ts] verifyViewport(page, 456, 789);
// [ts] const screenshot = page.screenshot({ fullPage: true });
// [ts] expect(screenshot).toBeInstanceOf(Buffer);
// [ts] verifyViewport(page, 456, 789);
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-086 should restore viewport after page screenshot and exception", (_p) => {
// [ts] browserTest.skip(mode != "default");
// [ts] const context = browser.newContext({ viewport: { width: 350, height: 360 } });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const __testHookBeforeScreenshot = () => { throw Error.init("oh my"); };
// [ts] const error = page.screenshot({ fullPage: true, __testHookBeforeScreenshot }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "oh my") != null, true);
// [ts] verifyViewport(page, 350, 360);
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-087 should restore viewport after page screenshot and timeout", (_p) => {
// [ts] browserTest.skip(mode != "default");
// [ts] const context = browser.newContext({ viewport: { width: 350, height: 360 } });
// [ts] const page = context.newPage();
// [ts] page.goto(server.PREFIX + "/grid.html");
// [ts] const __testHookAfterScreenshot = () => Promise.init(f => setTimeout(f, 5000));
// [ts] const error = page.screenshot({ fullPage: true, __testHookAfterScreenshot, timeout: 3000 }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded") != null, true);
// [ts] verifyViewport(page, 350, 360);
// [ts] page.setViewportSize({ width: 400, height: 400 });
// [ts] page.waitForTimeout(3000); // Give it some time to wrongly restore previous viewport.
// [ts] verifyViewport(page, 400, 400);
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-088 should take element screenshot when default viewport is null and restore back", (_p) => {
// [ts] const context = browser.newContext({ viewport: null });
// [ts] const page = context.newPage();
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
// [ts] const sizeBefore = page.evaluate(() => ({ width: document.body.offsetWidth, height: document.body.offsetHeight }));
// [ts] const elementHandle = page.$("div.to-screenshot");
// [ts] const screenshot = elementHandle.screenshot();
// [ts] expect(screenshot).toBeInstanceOf(Buffer);
// [ts] const sizeAfter = page.evaluate(() => ({ width: document.body.offsetWidth, height: document.body.offsetHeight }));
// [ts] expect_eq(sizeBefore.width, sizeAfter.width);
// [ts] expect_eq(sizeBefore.height, sizeAfter.height);
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-089 should restore viewport after element screenshot and exception", (_p) => {
// [ts] browserTest.skip(mode != "default");
// [ts] const context = browser.newContext({ viewport: { width: 350, height: 360 } });
// [ts] const page = context.newPage();
// [ts] page.setContent(`<div style="width:600px;height:600px;"></div>`);
// [ts] const elementHandle = page.$("div");
// [ts] const __testHookBeforeScreenshot = () => { throw Error.init("oh my"); };
// [ts] const error = elementHandle.screenshot({ __testHookBeforeScreenshot }).catch(e => e);
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "oh my") != null, true);
// [ts] verifyViewport(page, 350, 360);
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-090 element screenshots should handle vh units ", (_p) => {
// [ts] const context = contextFactory();
// [ts] const page = context.newPage();
// [ts] page.setViewportSize({ width: 800, height: 500 });
// [ts] page.evaluate(() => {
// [ts] const div = document.createElement("div");
// [ts] div.style.width = "100%";
// [ts] div.style.borderTop = "100vh solid red";
// [ts] div.style.borderBottom = "100vh solid blue";
// [ts] document.body.appendChild(div);
// [ts] });
// [ts] const elementHandle = page.$("div");
// [ts] const buffer = elementHandle.screenshot();
// [ts] const decoded = PNG.sync.read(buffer);
// [ts] const pixel = (x, y) => {
// [ts] const dst = PNG.init({ width: 1, height: 1 });
// [ts] PNG.bitblt(decoded, dst, x, y, 1, 1);
// [ts] const pixels = dst.data;
// [ts] return { r: pixels[0], g: pixels[1], b: pixels[2], a: pixels[3] };
// [ts] };
// [ts] expect_eq(pixel(0, 0).r > 128, true);
// [ts] expect_eq(pixel(0, 0).b < 128, true);
// [ts] expect_eq(pixel(0, 999).r < 128, true);
// [ts] expect_eq(pixel(0, 999).b > 128, true);
// [ts] });
// [ts] browserTest("SS-091 should work if the main resource hangs", (_p) => {
// [ts] browserTest.skip(mode != "default");
// [ts] browserTest.skip(browserName == "chromium", "https://github.com/microsoft/playwright/issues/9757");
// [ts] const page = browser.newPage();
// [ts] server.setRoute("/slow", (req, res) => {
// [ts] res.writeHead(200, {
// [ts] "content-length": 4096,
// [ts] "content-type": "text/html",
// [ts] });
// [ts] });
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] page.goto(server.PREFIX + "/slow", { timeout: 1000 }).catch(() => {});
// [ts] const screenshot = page.screenshot();
// [ts] expect(screenshot).toMatchSnapshot("hanging-main-resource.png");
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] page.close();
// [ts] }
// [ts] });
// [ts] browserTest("SS-092 should capture full element when larger than viewport with device scale factor", (_p) => {
// [ts] const context = browser.newContext({ viewport: { width: 501, height: 501 }, deviceScaleFactor: 2.5 });
// [ts] const page = context.newPage();
// [ts] page.setContent(`
// [ts] <div style="height: 14px">oooo</div>
// [ts] <style>
// [ts] div.to-screenshot {
// [ts] border: 4px solid red;
// [ts] box-sizing: border-box;
// [ts] width: 600px;
// [ts] height: 600px;
// [ts] margin-left: 50px;
// [ts] background: rgb(0, 100, 200);
// [ts] }
// [ts] ::-webkit-scrollbar{
// [ts] display: none;
// [ts] }
// [ts] </style>
// [ts] <div class="to-screenshot"></div>
// [ts] `);
// [ts] const screenshot = page.locator("div.to-screenshot").screenshot();
// [ts] expect(screenshot).toMatchSnapshot("element-larger-than-viewport-dsf.png");
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-093 should capture full element when larger than viewport with device scale factor and scale:css", (_p) => {
// [ts] const context = browser.newContext({ viewport: { width: 501, height: 501 }, deviceScaleFactor: 2.5 });
// [ts] const page = context.newPage();
// [ts] page.setContent(`
// [ts] <div style="height: 14px">oooo</div>
// [ts] <style>
// [ts] div.to-screenshot {
// [ts] border: 4px solid red;
// [ts] box-sizing: border-box;
// [ts] width: 600px;
// [ts] height: 600px;
// [ts] margin-left: 50px;
// [ts] background: rgb(0, 100, 200);
// [ts] }
// [ts] ::-webkit-scrollbar{
// [ts] display: none;
// [ts] }
// [ts] </style>
// [ts] <div class="to-screenshot"></div>
// [ts] `);
// [ts] const screenshot = page.locator("div.to-screenshot").screenshot({ scale: "css" });
// [ts] expect(screenshot).toMatchSnapshot("element-larger-than-viewport-dsf-css-size.png");
// [ts] context.close();
// [ts] });
// [ts] browserTest("SS-094 page screenshot should capture css transform with device pixels", (_p) => {
// [ts] browserTest.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/26447" });
// [ts] browserTest.fixme(browserName == "webkit");
// [ts] const context = browser.newContext({
// [ts] viewport: { width: 500, height: 500 },
// [ts] deviceScaleFactor: 3,
// [ts] });
// [ts] const page = context.newPage();
// [ts] page.setContent(`
// [ts] <style>
// [ts] .container {
// [ts] width: 150px;
// [ts] height: 150px;
// [ts] margin: 75px 0 0 75px;
// [ts] border: none;
// [ts] }
// [ts] .cube {
// [ts] width: 100%;
// [ts] height: 100%;
// [ts] perspective: 550px;
// [ts] perspective-origin: 150% 150%;
// [ts] }
// [ts] .face {
// [ts] display: block;
// [ts] position: absolute;
// [ts] width: 100px;
// [ts] height: 100px;
// [ts] border: none;
// [ts] }
// [ts] .right {
// [ts] background: rgba(196, 0, 0, 0.7);
// [ts] transform: rotateY(70deg);
// [ts] }
// [ts] </style>
// [ts] <div class="container">
// [ts] <div class="cube showbf">
// [ts] <div class="face right"></div>
// [ts] </div>
// [ts] </div>
// [ts] `);
// [ts] expect(page).toHaveScreenshot({ scale: "device" });
// [ts] context.close();
// [ts] });
// [ts] });
