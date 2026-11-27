// Migrated from TypeScript (src/screenshot.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";


import { getTestState, setupSeparateTestBrowserHooks, setupTestBrowserHooks } from "./mocha-utils.cp";
import { html } from "./utils.cp";

// [cap-test] describe("Screenshots", () => {
  setupTestBrowserHooks();
  const state = setupSeparateTestBrowserHooks({
    defaultViewport: null,
  });

  // [cap-test] describe("Page.screenshot", () => {


  // [cap-test] });

  // [cap-test] describe("ElementHandle.screenshot", () => {



  // [cap-test] });

  // [cap-test] describe("Cdp", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-sanity.png");
    // [ts] });
    // [ts] test("should clip rect", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] clip: {
    // [ts] x: 50,
    // [ts] y: 100,
    // [ts] width: 150,
    // [ts] height: 100,
    // [ts] },
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-clip-rect.png");
    // [ts] });
    // [ts] test("should get screenshot bigger than the viewport", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 50, height: 50});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] clip: {
    // [ts] x: 25,
    // [ts] y: 25,
    // [ts] width: 100,
    // [ts] height: 100,
    // [ts] },
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-offscreen-clip.png");
    // [ts] });
    // [ts] test("should clip clip bigger than the viewport without \"captureBeyondViewport\"", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 50, height: 50});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] captureBeyondViewport: false,
    // [ts] clip: {
    // [ts] x: 25,
    // [ts] y: 25,
    // [ts] width: 100,
    // [ts] height: 100,
    // [ts] },
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-offscreen-clip-2.png");
    // [ts] });
    // [ts] test("should run in parallel", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const promises = [];
    // [ts] let i = 0;
    // [ts] while (i < 3) { // [cap-migrate] for-step: ++i
    // [ts] promises.push(
    // [ts] page.screenshot({
    // [ts] clip: {
    // [ts] x: 50 * i,
    // [ts] y: 0,
    // [ts] width: 50,
    // [ts] height: 50,
    // [ts] },
    // [ts] }),
    // [ts] );
    // [ts] }
    // [ts] const screenshots = joinAll(promises); // migrated: Promise.all -> joinAll
    // [ts] expect(screenshots[1]).toBeGolden("grid-cell-1.png");
    // [ts] });
    // [ts] test("should take fullPage screenshots", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] fullPage: true,
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-grid-fullpage.png");
    // [ts] });
    // [ts] test("should take fullPage screenshots without captureBeyondViewport", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] fullPage: true,
    // [ts] captureBeyondViewport: false,
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-grid-fullpage-2.png");
    // [ts] expect(page.viewport()).toMatchObject({width: 500, height: 500});
    // [ts] });
    // [ts] test("should run in parallel in multiple pages", () => {
    // [ts] const {server, context} = getTestState();
    // [ts] const N = 2;
    // [ts] const pages = joinAll( // migrated: Promise.all -> joinAll
    // [ts] Array(N)
    // [ts] .fill(0)
    // [ts] .map(() => {
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] return page;
    // [ts] }),
    // [ts] );
    // [ts] const promises = [];
    // [ts] let i = 0;
    // [ts] while (i < N) { // [cap-migrate] for-step: ++i
    // [ts] promises.push(
    // [ts] pages[i].screenshot({
    // [ts] clip: {x: 50 * i, y: 0, width: 50, height: 50},
    // [ts] }),
    // [ts] );
    // [ts] }
    // [ts] const screenshots = joinAll(promises); // migrated: Promise.all -> joinAll
    // [ts] let i = 0;
    // [ts] while (i < N) { // [cap-migrate] for-step: ++i
    // [ts] expect(screenshots[i]).toBeGolden(`grid-cell-${i}.png`);
    // [ts] }
    // [ts] joinAll( // migrated: Promise.all -> joinAll
    // [ts] pages.map(page => {
    // [ts] return page.close();
    // [ts] }),
    // [ts] );
    // [ts] });
    // [ts] test("should work with odd clip size on Retina displays", () => {
    // [ts] const {page} = getTestState();
    // Make sure documentElement height is at least 11px.
    // [ts] page.setContent(
    // [ts] html`<div style="width: 11px; height: 11px;"></div>`,
    // [ts] );
    // [ts] const screenshot = page.screenshot({
    // [ts] clip: {
    // [ts] x: 0,
    // [ts] y: 0,
    // [ts] width: 11,
    // [ts] height: 11,
    // [ts] },
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-clip-odd-size.png");
    // [ts] });
    // [ts] test("should return base64", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] encoding: "base64",
    // [ts] });
    // [ts] expect(Buffer.from(screenshot, "base64")).toBeGolden(
    // [ts] "screenshot-sanity.png",
    // [ts] );
    // [ts] });
    // [ts] test("should take fullPage screenshots when defaultViewport is null", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] fullPage: true,
    // [ts] });
    // [ts] expect(screenshot).toBeInstanceOf(Uint8Array);
    // [ts] });
    // [ts] test("should restore to original viewport size after taking fullPage screenshots when defaultViewport is null", () => {
    // [ts] const {server, page} = state;
    // [ts] const originalSize = page.evaluate(() => {
    // [ts] return {width: window.innerWidth, height: window.innerHeight};
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/scrollbar.html");
    // [ts] page.screenshot({
    // [ts] fullPage: true,
    // [ts] captureBeyondViewport: false,
    // [ts] });
    // [ts] const size = page.evaluate(() => {
    // [ts] return {width: window.innerWidth, height: window.innerHeight};
    // [ts] });
    // [ts] expect_eq(page.viewport(), null);
    // [ts] expect_eq(size.width, originalSize.width);
    // [ts] expect_eq(size.height, originalSize.height);
    // [ts] });
    // [ts] test("should work", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] page.evaluate(() => {
    // [ts] return window.scrollBy(50, 100);
    // [ts] });
    // [ts] using elementHandle = (page.$(".box:nth-of-type(3)"));
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-element-bounding-box.png");
    // [ts] });
    // [ts] test("should work with a null viewport", () => {
    // [ts] const {server, page} = state;
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] page.evaluate(() => {
    // [ts] return window.scrollBy(50, 100);
    // [ts] });
    // [ts] using elementHandle = page.$(".box:nth-of-type(3)");
    // [ts] assert(elementHandle);
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect_eq(screenshot, true);
    // [ts] });
    // [ts] test("should take into account padding and border", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] something above
    // [ts] <style>
    // [ts] div {
    // [ts] border: 2px solid blue;
    // [ts] background: green;
    // [ts] width: 50px;
    // [ts] height: 50px;
    // [ts] }
    // [ts] </style>
    // [ts] <div></div>
    // [ts] `);
    // [ts] using elementHandle = (page.$("div"));
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-element-padding-border.png");
    // [ts] });
    // [ts] test("should capture full element when larger than viewport", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] something above
    // [ts] <style>
    // [ts] :root {
    // [ts] scrollbar-width: none;
    // [ts] }
    // [ts] div.to-screenshot {
    // [ts] border: 1px solid blue;
    // [ts] width: 600px;
    // [ts] height: 600px;
    // [ts] margin-left: 50px;
    // [ts] }
    // [ts] </style>
    // [ts] <div class="to-screenshot"></div>
    // [ts] `);
    // [ts] using elementHandle = (page.$("div.to-screenshot"));
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect(screenshot).toBeGolden(
    // [ts] "screenshot-element-larger-than-viewport.png",
    // [ts] );
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return {
    // [ts] w: window.innerWidth,
    // [ts] h: window.innerHeight,
    // [ts] };
    // [ts] }),
    // [ts] ).toEqual({w: 500, h: 500});
    // [ts] });
    // [ts] test("should scroll element into view", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] something above
    // [ts] <style>
    // [ts] div.above {
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
    // [ts] using elementHandle = (page.$("div.to-screenshot"));
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect(screenshot).toBeGolden(
    // [ts] "screenshot-element-scrolled-into-view.png",
    // [ts] );
    // [ts] });
    // [ts] test("should work with a rotated element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(
    // [ts] html`<!DOCTYPE html>
    // [ts] <div
    // [ts] style="position:absolute;
    // [ts] top: 100px;
    // [ts] left: 100px;
    // [ts] width: 100px;
    // [ts] height: 100px;
    // [ts] background: green;
    // [ts] transform: rotateZ(200deg);"
    // [ts] >
    // [ts] &nbsp;
    // [ts] </div>`,
    // [ts] );
    // [ts] using elementHandle = (page.$("div"));
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-element-rotate.png");
    // [ts] });
    // [ts] test("should fail to screenshot a detached element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<h1>remove this</h1>`);
    // [ts] using elementHandle = (page.$("h1"));
    // [ts] page.evaluate(element => {
    // [ts] return element.remove();
    // [ts] }, elementHandle);
    // [ts] const screenshotError = elementHandle.screenshot().catch(error => {
    // [ts] return error;
    // [ts] });
    // [ts] expect(screenshotError).toBeInstanceOf(Error);
    // [ts] expect(screenshotError.message).toMatch(
    // [ts] "Node is either not visible or not an HTMLElement|Node is detached from document",
    // [ts] );
    // [ts] });
    // [ts] test("should not hang with zero width/height element", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div style="width: 50px; height: 0"></div>`);
    // [ts] using div = (page.$("div"));
    // [ts] const error = div.screenshot().catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] expect_eq(error.message, "Node has 0 height.");
    // [ts] });
    // [ts] test("should work for an element with fractional dimensions", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] style="width:48.51px;height:19.8px;border:1px solid black;"
    // [ts] ></div>`,
    // [ts] );
    // [ts] using elementHandle = (page.$("div"));
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-element-fractional.png");
    // [ts] });
    // [ts] test("should work for an element with an offset", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<!DOCTYPE html>
    // [ts] <div
    // [ts] style="position:absolute; top: 10.3px; left: 20.4px;width:50.3px;height:20.2px;border:1px solid black;"
    // [ts] ></div>`,
    // [ts] );
    // [ts] using elementHandle = (page.$("div"));
    // [ts] const screenshot = elementHandle.screenshot();
    // [ts] expect(screenshot).toBeGolden("screenshot-element-fractional-offset.png");
    // [ts] });
    // [ts] test("should work with webp", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 100, height: 100});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] type: "webp",
    // [ts] });
    // [ts] expect(screenshot).toBeInstanceOf(Uint8Array);
    // [ts] });
    // [ts] test("should run in parallel in multiple pages", () => {
    // [ts] const {browser, server} = getTestState();
    // [ts] const context = browser.createBrowserContext();
    // [ts] const N = 2;
    // [ts] const pages = joinAll( // migrated: Promise.all -> joinAll
    // [ts] Array(N)
    // [ts] .fill(0)
    // [ts] .map(() => {
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] return page;
    // [ts] }),
    // [ts] );
    // [ts] const promises = [];
    // [ts] let i = 0;
    // [ts] while (i < N) { // [cap-migrate] for-step: ++i
    // [ts] promises.push(
    // [ts] pages[i].screenshot({
    // [ts] clip: {x: 50 * i, y: 0, width: 50, height: 50},
    // [ts] }),
    // [ts] );
    // [ts] }
    // [ts] const screenshots = joinAll(promises); // migrated: Promise.all -> joinAll
    // [ts] let i = 0;
    // [ts] while (i < N) { // [cap-migrate] for-step: ++i
    // [ts] expect(screenshots[i]).toBeGolden(`grid-cell-${i}.png`);
    // [ts] }
    // [ts] joinAll( // migrated: Promise.all -> joinAll
    // [ts] pages.map(page => {
    // [ts] return page.close();
    // [ts] }),
    // [ts] );
    // [ts] context.close();
    // [ts] });
    // [ts] test("should run in parallel with page.close()", () => {
    // [ts] const {browser} = getTestState();
    // [ts] using context = browser.createBrowserContext();
    // [ts] const page1 = context.newPage();
    // [ts] const page2 = context.newPage();
    // [ts] const screen1 = page1.screenshot();
    // [ts] const screen2 = page2.screenshot();
    // [ts] const close1 = screen1.then(() => {
    // [ts] return page1.close();
    // [ts] });
    // [ts] const close2 = screen2.then(() => {
    // [ts] return page2.close();
    // [ts] });
    // [ts] screen1;
    // [ts] const page3 = browser.newPage();
    // [ts] page3.screenshot();
    // [ts] const close3 = page3.close();
    // [ts] joinAll([close1, close2, close3]); // migrated: Promise.all -> joinAll
    // [ts] });
    // [ts] test("should use element clip", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(html`
    // [ts] something above
    // [ts] <style>
    // [ts] div {
    // [ts] border: 2px solid blue;
    // [ts] background: green;
    // [ts] width: 50px;
    // [ts] height: 50px;
    // [ts] }
    // [ts] </style>
    // [ts] <div></div>
    // [ts] `);
    // [ts] using elementHandle = (page.$("div"));
    // [ts] const screenshot = elementHandle.screenshot({
    // [ts] clip: {
    // [ts] x: 10,
    // [ts] y: 10,
    // [ts] width: 20,
    // [ts] height: 20,
    // [ts] },
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-element-clip.png");
    // [ts] });
    // [ts] test("should use scale for clip", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] clip: {
    // [ts] x: 50,
    // [ts] y: 100,
    // [ts] width: 150,
    // [ts] height: 100,
    // [ts] scale: 2,
    // [ts] },
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("screenshot-clip-rect-scale2.png");
    // [ts] });
    // [ts] test("should allow transparency", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 100, height: 100});
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const screenshot = page.screenshot({omitBackground: true});
    // [ts] expect(screenshot).toBeGolden("transparent.png");
    // [ts] });
    // [ts] test("should render white background on jpeg file", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 100, height: 100});
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const screenshot = page.screenshot({
    // [ts] omitBackground: true,
    // [ts] type: "jpeg",
    // [ts] });
    // [ts] expect(screenshot).toBeGolden("white.jpg");
    // [ts] });
    // [ts] test("should work in \"fromSurface: false\" mode", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.goto(server.PREFIX + "/grid.html");
    // [ts] const screenshot = page.screenshot({
    // [ts] fromSurface: false,
    // [ts] });
    // [ts] expect_eq(screenshot != null, true);
    // [ts] });
}
