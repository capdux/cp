// Migrated from TypeScript (src/click.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import { KnownDevices } from "puppeteer";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, html, waitEvent } from "./utils.cp";

// [cap-test] describe("Page.click", () => {
  setupTestBrowserHooks();

  // @see https://github.com/puppeteer/puppeteer/issues/4281





  // @see https://github.com/puppeteer/puppeteer/issues/161
  // @see https://github.com/puppeteer/puppeteer/issues/206
  // @see https://github.com/puppeteer/puppeteer/issues/4110
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should click the button", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should click svg", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <svg
    // [ts] height="100"
    // [ts] width="100"
    // [ts] >
    // [ts] <circle
    // [ts] onclick="javascript:window.__CLICKED=42"
    // [ts] cx="50"
    // [ts] cy="50"
    // [ts] r="40"
    // [ts] stroke="black"
    // [ts] stroke-width="3"
    // [ts] fill="red"
    // [ts] />
    // [ts] </svg>
    // [ts] `);
    // [ts] page.click("circle");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__CLICKED;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should click the button if window.Node is removed", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // @ts-expect-error Expected.
    // [ts] return delete window.Node;
    // [ts] });
    // [ts] page.click("button");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should click on a span with an inline element inside", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <style>
    // [ts] span::before {
    // [ts] content: 'q';
    // [ts] }
    // [ts] </style>
    // [ts] <span onclick="javascript:window.CLICKED=42"></span>
    // [ts] `);
    // [ts] page.click("span");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).CLICKED;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should not throw UnhandledPromiseRejection when page closes", () => {
    // [ts] const {page} = getTestState();
    // [ts] const newPage = page.browser().newPage();
    // [ts] joinAll([newPage.close(), newPage.mouse.click(1, 2)]).catch( // migrated: Promise.all -> joinAll
    // [ts] () => {},
    // [ts] );
    // [ts] });
    // [ts] test("should click the button after navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should click with disabled javascript", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setJavaScriptEnabled(false);
    // [ts] page.goto(server.PREFIX + "/wrappedlink.html");
    // [ts] joinAll([page.click("a"), page.waitForNavigation()]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(page.url(), server.PREFIX + "/wrappedlink.html#clicked");
    // [ts] });
    // [ts] test("should scroll and click with disabled javascript", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setJavaScriptEnabled(false);
    // [ts] page.goto(server.PREFIX + "/wrappedlink.html");
    // [ts] using body = page.waitForSelector("body");
    // [ts] body.evaluate(el => {
    // [ts] el.style.paddingTop = "3000px";
    // [ts] });
    // [ts] joinAll([page.click("a"), page.waitForNavigation()]); // migrated: Promise.all -> joinAll
    // [ts] expect_eq(page.url(), server.PREFIX + "/wrappedlink.html#clicked");
    // [ts] });
    // [ts] test("should click when one of inline box children is outside of viewport", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <style>
    // [ts] i {
    // [ts] position: absolute;
    // [ts] top: -1000px;
    // [ts] }
    // [ts] </style>
    // [ts] <span onclick="javascript:window.CLICKED = 42;"
    // [ts] ><i>woof</i><b>doggo</b></span
    // [ts] >
    // [ts] `);
    // [ts] page.click("span");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).CLICKED;
    // [ts] }),
    // [ts] ).toBe(42);
    // [ts] });
    // [ts] test("should select the text by triple clicking", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.focus("textarea");
    // [ts] const text =
    // [ts] "This is the text that we are going to try to select. Let's see how it goes.";
    // [ts] page.keyboard.type(text);
    // [ts] page.evaluate(() => {
    // [ts] (window).clicks = [];
    // [ts] window.addEventListener("click", event => {
    // [ts] return (window).clicks.push(event.detail);
    // [ts] });
    // [ts] });
    // [ts] page.click("textarea", {count: 3});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (window).clicks;
    // [ts] }),
    // [ts] ).toMatchObject({0: 1, 1: 2, 2: 3});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] const textarea = document.querySelector("textarea");
    // [ts] return textarea.value.substring(
    // [ts] textarea.selectionStart,
    // [ts] textarea.selectionEnd,
    // [ts] );
    // [ts] }),
    // [ts] ).toBe(text);
    // [ts] });
    // [ts] test("should click offscreen buttons", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/offscreenbuttons.html");
    // [ts] const messages: []string = [];
    // [ts] page.on("console", msg => {
    // [ts] if (msg.type() == "log") {
    // [ts] return messages.push(msg.text());
    // [ts] }
    // [ts] return;
    // [ts] });
    // [ts] let i = 0;
    // [ts] while (i < 11) { // [cap-migrate] for-step: ++i
    // We might've scrolled to click a button - reset to (0, 0).
    // [ts] page.evaluate(() => {
    // [ts] return window.scrollTo(0, 0);
    // [ts] });
    // The console event can arrive after the click resolves.
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "console", msg => {
    // [ts] return msg.text() == `button #${i} clicked`;
    // [ts] }),
    // [ts] page.click(`#btn${i}`),
    // [ts] ]);
    // [ts] }
    // [ts] expect(messages).toEqual([
    // [ts] "button #0 clicked",
    // [ts] "button #1 clicked",
    // [ts] "button #2 clicked",
    // [ts] "button #3 clicked",
    // [ts] "button #4 clicked",
    // [ts] "button #5 clicked",
    // [ts] "button #6 clicked",
    // [ts] "button #7 clicked",
    // [ts] "button #8 clicked",
    // [ts] "button #9 clicked",
    // [ts] "button #10 clicked",
    // [ts] ]);
    // [ts] });
    // [ts] test("should click half-offscreen elements", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<!DOCTYPE html>
    // [ts] <style>
    // [ts] body {
    // [ts] overflow: hidden;
    // [ts] }
    // [ts] #target {
    // [ts] width: 200px;
    // [ts] height: 200px;
    // [ts] background: red;
    // [ts] position: fixed;
    // [ts] left: -150px;
    // [ts] top: -150px;
    // [ts] }
    // [ts] </style>
    // [ts] <div
    // [ts] id="target"
    // [ts] onclick="window.CLICKED=true;"
    // [ts] ></div>`,
    // [ts] );
    // [ts] page.click("#target");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).CLICKED;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] using element = page.locator("#target").waitHandle();
    // [ts] expect: (element.boundingBox()) => {
    // [ts] height: 200,
    // [ts] width: 200,
    // [ts] x: -150,
    // [ts] y: -150,
    // [ts] });
    // [ts] expect: (element.clickablePoint()) => {
    // [ts] x: 25,
    // [ts] y: 25,
    // [ts] });
    // [ts] });
    // [ts] test("should click wrapped links", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/wrappedlink.html");
    // [ts] page.click("a");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).__clicked;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] });
    // [ts] test("should click on checkbox input and toggle", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/checkbox.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.check;
    // [ts] }),
    // [ts] ).toBe(null);
    // [ts] page.click("input#agree");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.check;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.events;
    // [ts] }),
    // [ts] ).toEqual([
    // [ts] "mouseover",
    // [ts] "mouseenter",
    // [ts] "mousemove",
    // [ts] "mousedown",
    // [ts] "mouseup",
    // [ts] "click",
    // [ts] "input",
    // [ts] "change",
    // [ts] ]);
    // [ts] page.click("input#agree");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.check;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should click on checkbox label and toggle", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/checkbox.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.check;
    // [ts] }),
    // [ts] ).toBe(null);
    // [ts] page.click("label[for=\"agree\"]");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.check;
    // [ts] }),
    // [ts] ).toBe(true);
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.events;
    // [ts] }),
    // [ts] ).toEqual(["click", "input", "change"]);
    // [ts] page.click("label[for=\"agree\"]");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result.check;
    // [ts] }),
    // [ts] ).toBe(false);
    // [ts] });
    // [ts] test("should fail to click a missing button", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] let error: Error;
    // [ts] page.click("button.does-not-exist").catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).toBe(
    // [ts] "No element found for selector: button.does-not-exist",
    // [ts] );
    // [ts] });
    // [ts] test("should not hang with touch-enabled viewports", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setViewport(KnownDevices["iPhone 6"].viewport);
    // [ts] page.mouse.down();
    // [ts] page.mouse.move(100, 10);
    // [ts] page.mouse.up();
    // [ts] });
    // [ts] test("should scroll and click the button", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.click("#button-5");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("#button-5").textContent;
    // [ts] }),
    // [ts] ).toBe("clicked");
    // [ts] page.click("#button-80");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("#button-80").textContent;
    // [ts] }),
    // [ts] ).toBe("clicked");
    // [ts] });
    // [ts] test("should double click the button", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).double = false;
    // [ts] const button = document.querySelector("button");
    // [ts] button.addEventListener("dblclick", () => {
    // [ts] (globalThis).double = true;
    // [ts] });
    // [ts] });
    // [ts] using button = (page.$("button"));
    // [ts] button.click({count: 2});
    // [ts] expect_eq(page.evaluate("double"), true);
    // [ts] expect_eq(page.evaluate("result"), "Clicked");
    // [ts] });
    // [ts] test("should double multiple times", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] (globalThis).count = 0;
    // [ts] const button = document.querySelector("button");
    // [ts] button.addEventListener("dblclick", () => {
    // [ts] (globalThis).count++;
    // [ts] });
    // [ts] });
    // [ts] using button = (page.$("button"));
    // [ts] button.click({count: 2});
    // [ts] button.click({count: 2});
    // [ts] expect_eq(page.evaluate("count"), 2);
    // [ts] });
    // [ts] test("should click a partially obscured button", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.evaluate(() => {
    // [ts] const button = document.querySelector("button");
    // [ts] button.textContent = "Some really long text that will go offscreen";
    // [ts] button.style.position = "absolute";
    // [ts] button.style.left = "368px";
    // [ts] });
    // [ts] page.click("button");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should click a rotated button", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/rotatedButton.html");
    // [ts] page.click("button");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should fire contextmenu event on right click", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.click("#button-8", {button: "right"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("#button-8").textContent;
    // [ts] }),
    // [ts] ).toBe("context menu");
    // [ts] });
    // [ts] test("should fire aux event on middle click", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.click("#button-8", {button: "middle"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("#button-8").textContent;
    // [ts] }),
    // [ts] ).toBe("aux click");
    // [ts] });
    // [ts] test("should fire back click", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.click("#button-8", {button: "back"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("#button-8").textContent;
    // [ts] }),
    // [ts] ).toBe("back click");
    // [ts] });
    // [ts] test("should fire forward click", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/input/scrollable.html");
    // [ts] page.click("#button-8", {button: "forward"});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return document.querySelector("#button-8").textContent;
    // [ts] }),
    // [ts] ).toBe("forward click");
    // [ts] });
    // [ts] test("should click links which cause navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setContent(html`<a href="${server.EMPTY_PAGE}">empty.html</a>`);
    // This await should not hang.
    // [ts] page.click("a");
    // [ts] });
    // [ts] test("should click the button inside an iframe", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setContent(
    // [ts] html`<div style="width:100px;height:100px">spacer</div>`,
    // [ts] );
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "button-test",
    // [ts] server.PREFIX + "/input/button.html",
    // [ts] );
    // [ts] const frame = page.frames()[1];
    // [ts] using button = frame.$("button");
    // [ts] button.click();
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should click the button with fixed position inside an iframe", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.setViewport({width: 500, height: 500});
    // [ts] page.setContent(
    // [ts] html`<div style="width:100px;height:2000px">spacer</div>`,
    // [ts] );
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "button-test",
    // [ts] server.CROSS_PROCESS_PREFIX + "/input/button.html",
    // [ts] );
    // [ts] const frame = page.frames()[1];
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await frame!.$eval('button', button => {
    // [ts-skip] return button.style.setProperty('position', 'fixed');
    // [ts-skip] });
    // [ts] frame.click("button");
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
    // [ts] test("should click the button with deviceScaleFactor set", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setViewport({width: 400, height: 400, deviceScaleFactor: 5});
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window.devicePixelRatio;
    // [ts] }),
    // [ts] ).toBe(5);
    // [ts] page.setContent(
    // [ts] html`<div style="width:100px;height:100px">spacer</div>`,
    // [ts] );
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "button-test",
    // [ts] server.PREFIX + "/input/button.html",
    // [ts] );
    // [ts] const frame = page.frames()[1];
    // [ts] using button = frame.$("button");
    // [ts] button.click();
    // [ts] expect(
    // [ts] frame.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe("Clicked");
    // [ts] });
}
