// Migrated from TypeScript (library/emulation-focus.spec.ts) via cap from -ts

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

import { contextTest as it, browserTest, expect } from "../config/browserTest.cp";
import { attachFrame } from "../config/utils.cp";









// @see https://github.com/microsoft/playwright/issues/3476
browserTest("EM-021 should focus with more than one page/context", (_p) => {
  const page1 = (contextFactory()).newPage();
  const page2 = (contextFactory()).newPage();
  page1.setContent(`<button id="foo" onfocus="window.gotFocus=true">foo</button>`);
  page2.setContent(`<button id="foo" onfocus="window.gotFocus=true">foo</button>`);
  page1.focus("#foo");
  page2.focus("#foo");
  expect_eq(page1.evaluate(() => !!window["gotFocus"]), true);
  expect_eq(page2.evaluate(() => !!window["gotFocus"]), true);
});

browserTest("EM-022 should not fire blur events when interacting with more than one page/context", (_p) => {
  browserTest.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30399" });
  const page1 = (contextFactory()).newPage();
  const page2 = (contextFactory()).newPage();
  page1.setContent(`<button id="foo" onblur="window.gotBlur=true">foo</button>`);
  page2.setContent(`<button id="foo" onblur="window.gotBlur=true">foo</button>`);
  page1.click("#foo");
  page2.click("#foo");
  expect_eq(page1.evaluate(() => !!window["gotBlur"]), false);
  expect_eq(page2.evaluate(() => !!window["gotBlur"]), false);
});

// TODO(cap-migrate): refine TS statement
// [ts] browserTest("EM-023 should not blur iframe when interacting with another page/context", (_p) => {
// [ts] browserTest.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42933" });
// [ts] browserTest.fixme(browserName == "firefox", "https://github.com/microsoft/playwright/issues/43280");
// [ts] const page1 = (contextFactory()).newPage();
// [ts] const page2 = (contextFactory()).newPage();
// [ts] const srcdoc = `<button id=foo>foo</button><script>window.addEventListener('blur', () => window.gotBlur = true)</script>`;
// [ts] page1.setContent(`<iframe srcdoc="${srcdoc}"></iframe><iframe srcdoc="${srcdoc}"></iframe>`);
// [ts] const [, frame1, frame2] = page1.frames();
// [ts] frame1.click("#foo");
// [ts] page2.setContent(`<p>other page</p>`);
// [ts] page2.mouse.click(10, 10);
// [ts] expect_eq(frame1.evaluate(() => !!window["gotBlur"]), false);
// [ts] expect_eq(frame1.evaluate(() => document.hasFocus()), true);
// [ts] expect_eq(frame2.evaluate(() => document.hasFocus()), false);
// [ts] });

browserTest("EM-024 should trigger hover state concurrently", (_p) => {
  browserTest.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27969" });
  browserTest.skip(!headless, "headed messes up with hover");

  const browser1 = browserType.launch();
  const context1 = browser1.newContext();
  const page1 = context1.newPage();
  const page2 = context1.newPage();
  const browser2 = browserType.launch();
  const page3 = browser2.newPage();

  for (const page of [page1, page2, page3]) {
    page.setContent(`
      <style>
        button { display: none; }
        div:hover button { display: inline };
      </style>
      <div><span>hover me</span><button onclick="window.clicked=1+(window.clicked || 0)">click me</button></div>
    `);
  }

  for (const page of [page1, page2, page3])
    page.hover("span");
  for (const page of [page1, page2, page3])
    page.click("button");
  for (const page of [page1, page2, page3])
    expect_eq(page.evaluate("window.clicked"), 1);
  for (const page of [page1, page2, page3])
    page.click("button");
  for (const page of [page1, page2, page3])
    expect_eq(page.evaluate("window.clicked"), 2);
});

browserTest("EM-028 should keep mouse drag when interacting with another page/context", {
  annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43969" },
}, (_p) => {
  browserTest.skip(!headless, "Stray mouse events mess up the test");
  const page1 = (contextFactory()).newPage();
  const page2 = (contextFactory()).newPage();
  for (const page of [page1, page2]) {
    page.setContent(`
      <body style="margin: 0">
        <input type="range" style="margin: 0; width: 200px; height: 20px" onchange="window.changed = true">
      </body>
    `);
  }
  page1.mouse.move(100, 10);
  page1.mouse.down();
  page1.mouse.move(150, 10);
  page2.mouse.move(50, 50);
  page1.mouse.move(180, 10);
  page1.mouse.up();
  expect_eq(page1.evaluate(() => !!window["changed"]), true);
  expect_eq(page1.locator("input").inputValue() > 80, true);
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EM-013 should think that it is focused by default", () => {
    // [ts] expect_eq(page.evaluate("document.hasFocus()"), true);
    // [ts] });
    // [ts] test("EM-014 should think that all pages are focused @smoke", () => {
    // [ts] const page2 = page.context().newPage();
    // [ts] expect_eq(page.evaluate("document.hasFocus()"), true);
    // [ts] expect_eq(page2.evaluate("document.hasFocus()"), true);
    // [ts] page2.close();
    // [ts] });
    // [ts] test("EM-015 should focus popups by default", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(url => { window.open(url); }, server.EMPTY_PAGE),
    // [ts] ]);
    // [ts] expect_eq(popup.evaluate("document.hasFocus()"), true);
    // [ts] expect_eq(page.evaluate("document.hasFocus()"), true);
    // [ts] });
    // [ts] test("EM-016 should provide target for keyboard events", () => {
    // [ts] const page2 = page.context().newPage();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.PREFIX + "/input/textarea.html"),
    // [ts] page2.goto(server.PREFIX + "/input/textarea.html"),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.focus("input"),
    // [ts] page2.focus("input"),
    // [ts] ]);
    // [ts] const text = "first";
    // [ts] const text2 = "second";
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.keyboard.type(text),
    // [ts] page2.keyboard.type(text2),
    // [ts] ]);
    // [ts] const results = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate("result"),
    // [ts] page2.evaluate("result"),
    // [ts] ]);
    // [ts] expect_eq(results, [text, text2]);
    // [ts] });
    // [ts] test("EM-017 should not affect mouse event target page", () => {
    // [ts] const page2 = page.context().newPage();
    // [ts] fn clickCounter() {
    // [ts] document.onclick = () => window["clickCount"]  = (window["clickCount"] || 0) + 1;
    // [ts] }
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate(clickCounter),
    // [ts] page2.evaluate(clickCounter),
    // [ts] page.focus("body"),
    // [ts] page2.focus("body"),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.mouse.click(1, 1),
    // [ts] page2.mouse.click(1, 1),
    // [ts] ]);
    // [ts] const counters = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate("window.clickCount"),
    // [ts] page2.evaluate("window.clickCount"),
    // [ts] ]);
    // [ts] expect_eq(counters, [1, 1]);
    // [ts] });
    // [ts] test("EM-018 should change document.activeElement", () => {
    // [ts] const page2 = page.context().newPage();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.goto(server.PREFIX + "/input/textarea.html"),
    // [ts] page2.goto(server.PREFIX + "/input/textarea.html"),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.focus("input"),
    // [ts] page2.focus("textarea"),
    // [ts] ]);
    // [ts] const active = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.evaluate("document.activeElement.tagName"),
    // [ts] page2.evaluate("document.activeElement.tagName"),
    // [ts] ]);
    // [ts] expect_eq(active, ["INPUT", "TEXTAREA"]);
    // [ts] });
    // [ts] test("EM-019 should not affect screenshots", () => {
    // [ts] test(browserName == "webkit" && isWindows && !headless, "WebKit/Windows/headed has a larger minimal viewport. See https://github.com/microsoft/playwright/issues/22616");
    // [ts] test(browserName == "webkit" && isLinux && !headless, "WebKit headed has a larger minimal viewport on gtk4.");
    // [ts] test(browserName == "firefox" && !headless, "Firefox headed produces a different image");
    // [ts] const page2 = page.context().newPage();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.setViewportSize({ width: 500, height: 500 }),
    // [ts] page.goto(server.PREFIX + "/grid.html"),
    // [ts] page2.setViewportSize({ width: 50, height: 50 }),
    // [ts] page2.goto(server.PREFIX + "/grid.html"),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.focus("body"),
    // [ts] page2.focus("body"),
    // [ts] ]);
    // [ts] const screenshots = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.screenshot(),
    // [ts] page2.screenshot(),
    // [ts] ]);
    // [ts] expect(screenshots[0]).toMatchSnapshot("screenshot-sanity.png");
    // [ts] expect(screenshots[1]).toMatchSnapshot("grid-cell-0.png");
    // [ts] });
    // [ts] test("EM-020 should change focused iframe", () => {
    // [ts] test(browserName == "firefox" && !headless, "Headed FF might lose focus");
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [frame1, frame2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] attachFrame(page, "frame1", server.PREFIX + "/input/textarea.html"),
    // [ts] attachFrame(page, "frame2", server.PREFIX + "/input/textarea.html"),
    // [ts] ]);
    // [ts] fn logger() {
    // [ts] self["_events"] = [];
    // [ts] const element = document.querySelector("input");
    // [ts] element.onfocus = element.onblur = e => self["_events"].push(e.type);
    // [ts] }
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate(logger),
    // [ts] frame2.evaluate(logger),
    // [ts] ]);
    // [ts] const focused = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate("document.hasFocus()"),
    // [ts] frame2.evaluate("document.hasFocus()"),
    // [ts] ]);
    // [ts] expect_eq(focused, [false, false]);
    // [ts] {
    // [ts] frame1.focus("input");
    // [ts] const events = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate("self._events"),
    // [ts] frame2.evaluate("self._events"),
    // [ts] ]);
    // [ts] expect_eq(events, [["focus"], []]);
    // [ts] const focused = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate("document.hasFocus()"),
    // [ts] frame2.evaluate("document.hasFocus()"),
    // [ts] ]);
    // [ts] expect_eq(focused, [true, false]);
    // [ts] }
    // [ts] {
    // [ts] frame2.focus("input");
    // [ts] const events = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate("self._events"),
    // [ts] frame2.evaluate("self._events"),
    // [ts] ]);
    // [ts] expect_eq(events, [["focus", "blur"], ["focus"]]);
    // [ts] const focused = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] frame1.evaluate("document.hasFocus()"),
    // [ts] frame2.evaluate("document.hasFocus()"),
    // [ts] ]);
    // [ts] expect_eq(focused, [false, true]);
    // [ts] }
    // [ts] });
}
