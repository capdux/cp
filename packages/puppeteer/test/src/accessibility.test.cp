// Migrated from TypeScript (src/accessibility.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

// [cap-migrate] import {SerializedAXNode} from "puppeteer-core/internal/cdp/Accessibility.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, html, htmlRaw } from "./utils.cp";

// [cap-test] describe("Accessibility", () => {
  setupTestBrowserHooks();




  // [cap-test] describe("iframes", () => {





  // [cap-test] });

  // [cap-test] describe("filtering children of leaf nodes", () => {

    // Firefox does not support contenteditable="plaintext-only".
    // [cap-test] describe("plaintext contenteditable", () => {
    // [cap-test] });

    // [cap-test] describe("root option", () => {

  fn findFocusedNode(node: SerializedAXNode?): SerializedAXNode? {
    if (node?.focused) {
      return node;
    }
    for (const child of node?.children || []) {
      const focusedChild = findFocusedNode(child);
      if (focusedChild) {
        return focusedChild;
      }
    }
    return null;
  }

// [cap-test] });
// TODO(cap-migrate): refine TS statement
// [ts] interface TestAXNode {
// [ts] role: string;
// [ts] name: string;
// [ts] children?: TestAXNode[];
// [ts] }

fn buildTestAxTree(children: []TestAXNode) {
  return {
    role: "RootWebArea",
    name: "HTML Elements Showcase",
    children: [
      {
        role: "banner",
        name: "",
        children: [
          {
            role: "heading",
            name: "HTML Elements Showcase",
            level: 1,
          },
          {
            role: "navigation",
            name: "",
            children: [
              {
                role: "link",
                name: "Forms",
                children: [
                  {
                    role: "StaticText",
                    name: "Forms",
                  },
                ],
              },
              {
                role: "link",
                name: "Media",
                children: [
                  {
                    role: "StaticText",
                    name: "Media",
                  },
                ],
              },
              {
                role: "link",
                name: "Interactive",
                children: [
                  {
                    role: "StaticText",
                    name: "Interactive",
                  },
                ],
              },
            ],
          },
        ],
      },
      {
        role: "main",
        name: "",
        children,
      },
      {
        role: "complementary",
        name: "",
        children: [
          {
            role: "StaticText",
            name: "complementary",
          },
        ],
      },
      {
        role: "contentinfo",
        name: "",
        children: [
          {
            role: "StaticText",
            name: "contentinfo",
          },
        ],
      },
    ],
  };
}
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace
// TODO(cap-migrate): refine TS statement
// [ts] } // [cap-migrate] auto-closed brace

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] htmlRaw`
    // [ts] <!DOCTYPE html>
    // [ts] <html lang="en">
    // [ts] <head>
    // [ts] <title>Accessibility Test</title>
    // [ts] </head>
    // [ts] <body>
    // [ts] <div>Hello World</div>
    // [ts] <h1>Inputs</h1>
    // [ts] <input placeholder="Empty input" autofocus />
    // [ts] <input placeholder="readonly input" readonly />
    // [ts] <input placeholder="disabled input" disabled />
    // [ts] <input aria-label="Input with whitespace" value="  " />
    // [ts] <input value="value only" />
    // [ts] <input aria-placeholder="placeholder" value="and a value" />
    // [ts] <div aria-hidden="true" id="desc">This is a description!</div>
    // [ts] <input
    // [ts] aria-placeholder="placeholder"
    // [ts] value="and a value"
    // [ts] aria-describedby="desc"
    // [ts] />
    // [ts] <select>
    // [ts] <option>First Option</option>
    // [ts] <option>Second Option</option>
    // [ts] </select>
    // [ts] <a href="https://example.com">example</a>
    // [ts] </body>`,
    // [ts] );
    // [ts] page.focus("[placeholder=\"Empty input\"]");
    // [ts] expect: (page.accessibility.snapshot()) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "Accessibility Test",
    // [ts] children: [
    // [ts] {role: "StaticText", name: "Hello World"},
    // [ts] {role: "heading", name: "Inputs", level: 1},
    // [ts] {role: "textbox", name: "Empty input", focused: true},
    // [ts] {role: "textbox", name: "readonly input", readonly: true},
    // [ts] {role: "textbox", name: "disabled input", disabled: true},
    // [ts] {role: "textbox", name: "Input with whitespace", value: "  "},
    // [ts] {role: "textbox", name: "", value: "value only"},
    // [ts] {role: "textbox", name: "placeholder", value: "and a value"},
    // [ts] {
    // [ts] role: "textbox",
    // [ts] name: "placeholder",
    // [ts] value: "and a value",
    // [ts] description: "This is a description!",
    // [ts] },
    // [ts] {
    // [ts] role: "combobox",
    // [ts] name: "",
    // [ts] value: "First Option",
    // [ts] haspopup: "menu",
    // [ts] expanded: false,
    // [ts] children: [
    // [ts] {role: "option", name: "First Option", selected: true},
    // [ts] {role: "option", name: "Second Option"},
    // [ts] ],
    // [ts] },
    // [ts] {
    // [ts] name: "example",
    // [ts] role: "link",
    // [ts] url: "https://example.com/",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("should work for showcase", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/a11y/landmarks.html");
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect(snapshot).toMatchObject(
    // [ts] buildTestAxTree([
    // State before 149.0.7819.0. TODO: remove after roll
    // [ts] {
    // [ts] role: "form",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "search",
    // [ts] name: "",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ]),
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // State after 149.0.7819.0
    // [ts] expect(snapshot).toMatchObject(
    // [ts] buildTestAxTree([
    // [ts] {
    // [ts] role: "search",
    // [ts] name: "",
    // [ts] },
    // [ts] ]),
    // [ts] );
    // [ts] }
    // [ts] });
    // [ts] test("should report uninteresting nodes", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<textarea>hi</textarea>`);
    // [ts] page.focus("textarea");
    // [ts] expect(
    // [ts] findFocusedNode(
    // [ts] page.accessibility.snapshot({interestingOnly: false}),
    // [ts] ),
    // [ts] ).toMatchObject({
    // [ts] role: "textbox",
    // [ts] name: "",
    // [ts] value: "hi",
    // [ts] focused: true,
    // [ts] multiline: true,
    // [ts] children: [
    // [ts] {
    // [ts] role: "generic",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "StaticText",
    // [ts] name: "hi",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("get snapshots while the tree is re-calculated", () => {
    // see https://github.com/puppeteer/puppeteer/issues/9404
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] htmlRaw`<!DOCTYPE html>
    // [ts] <html lang="en">
    // [ts] <head>
    // [ts] <meta charset="UTF-8" />
    // [ts] <meta http-equiv="X-UA-Compatible" content="IE=edge" />
    // [ts] <meta
    // [ts] name="viewport"
    // [ts] content="width=device-width, initial-scale=1.0"
    // [ts] />
    // [ts] <title>Accessible name + aria-expanded puppeteer bug</title>
    // [ts] <style>
    // [ts] [aria-expanded='false'] + * {
    // [ts] display: none;
    // [ts] }
    // [ts] </style>
    // [ts] </head>
    // [ts] <body>
    // [ts] <button hidden>Show</button>
    // [ts] <p>Some content</p>
    // [ts] <script>
    // [ts] const button = document.querySelector('button');
    // [ts] button.removeAttribute('hidden');
    // [ts] button.setAttribute('aria-expanded', 'false');
    // [ts] button.addEventListener('click', function () {
    // [ts] button.setAttribute(
    // [ts] 'aria-expanded',
    // [ts] button.getAttribute('aria-expanded') !== 'true',
    // [ts] );
    // [ts] if (button.getAttribute('aria-expanded') == 'true') {
    // [ts] button.textContent = 'Hide';
    // [ts] } else {
    // [ts] button.textContent = 'Show';
    // [ts] }
    // [ts] });
    // [ts] </script>
    // [ts] </body>
    // [ts] </html>`,
    // [ts] );
    // [ts] async fn getAccessibleName(page: string, element: string) {
    // [ts] return (page.accessibility.snapshot({root: element})).name;
    // [ts] }
    // [ts] using button = page.$("button");
    // [ts] expect_eq(getAccessibleName(page, button), "Show");
    // [ts] button?.click();
    // [ts] page.waitForSelector("aria/Hide");
    // [ts] });
    // [ts] test("roledescription", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] tabindex="-1"
    // [ts] aria-roledescription="foo"
    // [ts] >Hi</div
    // [ts] >`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // See https://chromium-review.googlesource.com/c/chromium/src/+/3088862
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] assert(snapshot.children[0]);
    // [ts] expect_eq(snapshot.children[0].roledescription == null, true);
    // [ts] });
    // [ts] test("orientation", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<a
    // [ts] href=""
    // [ts] role="slider"
    // [ts] aria-orientation="vertical"
    // [ts] >11</a
    // [ts] >`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] assert(snapshot.children[0]);
    // [ts] expect_eq(snapshot.children[0].orientation, "vertical");
    // [ts] });
    // [ts] test("autocomplete", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] type="number"
    // [ts] aria-autocomplete="list"
    // [ts] />`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] assert(snapshot.children[0]);
    // [ts] expect_eq(snapshot.children[0].autocomplete, "list");
    // [ts] });
    // [ts] test("multiselectable", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="grid"
    // [ts] tabindex="-1"
    // [ts] aria-multiselectable="true"
    // [ts] >
    // [ts] hey
    // [ts] </div>`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] assert(snapshot.children[0]);
    // [ts] expect_eq(snapshot.children[0].multiselectable, true);
    // [ts] });
    // [ts] test("should not include iframe data if not requested", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] frame1.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.innerText = "value1";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] const snapshot = page.accessibility.snapshot({
    // [ts] interestingOnly: true,
    // [ts] });
    // [ts] expect: (snapshot) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] });
    // [ts] });
    // [ts] test("same-origin iframe (interesting only)", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] frame1.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.innerText = "value1";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] const snapshot = page.accessibility.snapshot({
    // [ts] interestingOnly: true,
    // [ts] includeIframes: true,
    // [ts] });
    // [ts] expect: (snapshot) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "Iframe",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "value1",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("cross-origin iframe (interesting only)", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const frame1 = page.frames()[1];
    // [ts] frame1.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.innerText = "value1";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] const snapshot = page.accessibility.snapshot({
    // [ts] interestingOnly: true,
    // [ts] includeIframes: true,
    // [ts] });
    // [ts] expect: (snapshot) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "Iframe",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "value1",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("same-origin iframe (all nodes)", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame1 = page.frames()[1];
    // [ts] frame1.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.innerText = "value1";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] const snapshot = page.accessibility.snapshot({
    // [ts] interestingOnly: false,
    // [ts] includeIframes: true,
    // [ts] });
    // [ts] expect: (snapshot) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "none",
    // [ts] children: [
    // [ts] {
    // [ts] role: "generic",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "Iframe",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "none",
    // [ts] children: [
    // [ts] {
    // [ts] role: "generic",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "value1",
    // [ts] children: [
    // [ts] {
    // [ts] role: "StaticText",
    // [ts] name: "value1",
    // [ts] children: [
    // [ts] {
    // [ts] role: "InlineTextBox",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("same-origin iframe with root option", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setContent(
    // [ts] htmlRaw`
    // [ts] <section aria-label="Container">
    // [ts] <iframe src="${server.EMPTY_PAGE}"></iframe>
    // [ts] </section>
    // [ts] `,
    // [ts] );
    // [ts] const frame1 = page.frames()[1];
    // [ts] frame1.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.innerText = "value1";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] using container = (page.$("section"));
    // [ts] const containerSnapshot = page.accessibility.snapshot({
    // [ts] root: container,
    // [ts] interestingOnly: true,
    // [ts] includeIframes: true,
    // [ts] });
    // [ts] expect: (containerSnapshot) => {
    // [ts] role: "region",
    // [ts] name: "Container",
    // [ts] children: [
    // [ts] {
    // [ts] role: "Iframe",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "value1",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] using iframeEl = (page.$("iframe"));
    // [ts] const iframeSnapshot = page.accessibility.snapshot({
    // [ts] root: iframeEl,
    // [ts] interestingOnly: true,
    // [ts] includeIframes: true,
    // [ts] });
    // [ts] expect: (iframeSnapshot) => {
    // [ts] role: "Iframe",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "value1",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("cross-origin iframe with root option", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.setContent(
    // [ts] htmlRaw`
    // [ts] <section aria-label="Container">
    // [ts] <iframe src="${server.CROSS_PROCESS_PREFIX}/empty.html"></iframe>
    // [ts] </section>
    // [ts] `,
    // [ts] );
    // [ts] const frame1 = page.frames()[1];
    // [ts] frame1.evaluate(() => {
    // [ts] const button = document.createElement("button");
    // [ts] button.innerText = "value1";
    // [ts] document.body.appendChild(button);
    // [ts] });
    // [ts] using container = (page.$("section"));
    // [ts] const snapshot = page.accessibility.snapshot({
    // [ts] root: container,
    // [ts] interestingOnly: true,
    // [ts] includeIframes: true,
    // [ts] });
    // [ts] expect: (snapshot) => {
    // [ts] role: "region",
    // [ts] name: "Container",
    // [ts] children: [
    // [ts] {
    // [ts] role: "Iframe",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "RootWebArea",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "value1",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("keyshortcuts", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="grid"
    // [ts] tabindex="-1"
    // [ts] aria-keyshortcuts="foo"
    // [ts] >hey</div
    // [ts] >`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] assert(snapshot.children[0]);
    // [ts] expect_eq(snapshot.children[0].keyshortcuts, "foo");
    // [ts] });
    // [ts] test("should not report text nodes inside controls", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html` <div role="tablist">
    // [ts] <div
    // [ts] role="tab"
    // [ts] aria-selected="true"
    // [ts] ><b>Tab1</b></div
    // [ts] >
    // [ts] <div role="tab">Tab2</div>
    // [ts] </div>`,
    // [ts] );
    // [ts] expect: (page.accessibility.snapshot()) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "My test page",
    // [ts] children: [
    // [ts] {
    // [ts] role: "tab",
    // [ts] name: "Tab1",
    // [ts] selected: true,
    // [ts] },
    // [ts] {
    // [ts] role: "tab",
    // [ts] name: "Tab2",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("rich text editable fields should have children", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html` <div contenteditable="true">
    // [ts] Edit this image:
    // [ts] <img
    // [ts] src="fakeimage.png"
    // [ts] alt="my fake image"
    // [ts] />
    // [ts] </div>`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] expect: (snapshot.children[0]) => {
    // [ts] role: "generic",
    // [ts] name: "",
    // [ts] value: "Edit this image: ",
    // [ts] children: [
    // [ts] {
    // [ts] role: "StaticText",
    // [ts] name: "Edit this image: ",
    // [ts] },
    // [ts] {
    // [ts] role: "image",
    // [ts] name: "my fake image",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("rich text editable fields with role should have children", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html` <div
    // [ts] contenteditable="true"
    // [ts] role="textbox"
    // [ts] >
    // [ts] Edit this image:
    // [ts] <img
    // [ts] src="fakeimage.png"
    // [ts] alt="my fake image"
    // [ts] />
    // [ts] </div>`,
    // [ts] );
    // Image node should not be exposed in contenteditable elements. See https://crbug.com/1324392.
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] expect: (snapshot.children[0]) => {
    // [ts] role: "textbox",
    // [ts] name: "",
    // [ts] value: "Edit this image: ",
    // [ts] multiline: true,
    // [ts] children: [
    // [ts] {
    // [ts] role: "StaticText",
    // [ts] name: "Edit this image: ",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("plain text field with role should not have children", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] contenteditable="plaintext-only"
    // [ts] role="textbox"
    // [ts] >Edit this image:<img
    // [ts] src="fakeimage.png"
    // [ts] alt="my fake image"
    // [ts] /></div>`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] expect: (snapshot.children[0]) => {
    // [ts] role: "textbox",
    // [ts] name: "",
    // [ts] value: "Edit this image:",
    // [ts] multiline: true,
    // [ts] });
    // [ts] });
    // [ts] test("non editable textbox with role and tabIndex and label should not have children", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="textbox"
    // [ts] tabindex="0"
    // [ts] aria-checked="true"
    // [ts] aria-label="my favorite textbox"
    // [ts] >
    // [ts] this is the inner content
    // [ts] <img
    // [ts] alt="yo"
    // [ts] src="fakeimg.png"
    // [ts] />
    // [ts] </div>`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] expect: (snapshot.children[0]) => {
    // [ts] role: "textbox",
    // [ts] name: "my favorite textbox",
    // [ts] value: "this is the inner content ",
    // [ts] });
    // [ts] });
    // [ts] test("checkbox with and tabIndex and label should not have children", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html` <div
    // [ts] role="checkbox"
    // [ts] tabindex="0"
    // [ts] aria-checked="true"
    // [ts] aria-label="my favorite checkbox"
    // [ts] >
    // [ts] this is the inner content
    // [ts] <img
    // [ts] alt="yo"
    // [ts] src="fakeimg.png"
    // [ts] />
    // [ts] </div>`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] expect: (snapshot.children[0]) => {
    // [ts] role: "checkbox",
    // [ts] name: "my favorite checkbox",
    // [ts] checked: true,
    // [ts] });
    // [ts] });
    // [ts] test("checkbox without label should not have children", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div
    // [ts] role="checkbox"
    // [ts] aria-checked="true"
    // [ts] >
    // [ts] this is the inner content
    // [ts] <img
    // [ts] alt="yo"
    // [ts] src="fakeimg.png"
    // [ts] />
    // [ts] </div>`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] assert(snapshot);
    // [ts] assert(snapshot.children);
    // [ts] expect: (snapshot.children[0]) => {
    // [ts] role: "checkbox",
    // [ts] name: "this is the inner content yo",
    // [ts] checked: true,
    // [ts] });
    // [ts] });
    // [ts] test("should work a button", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<button>My Button</button>`);
    // [ts] using button = (page.$("button"));
    // [ts] expect(page.accessibility.snapshot({root: button})).toMatchObject(
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "My Button",
    // [ts] },
    // [ts] );
    // [ts] });
    // [ts] test("should work an input", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<input
    // [ts] title="My Input"
    // [ts] value="My Value"
    // [ts] />`,
    // [ts] );
    // [ts] using input = (page.$("input"));
    // [ts] expect: (page.accessibility.snapshot({root) => {
    // [ts] role: "textbox",
    // [ts] name: "My Input",
    // [ts] value: "My Value",
    // [ts] });
    // [ts] });
    // [ts] test("should work a menu", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <div
    // [ts] role="menu"
    // [ts] title="My Menu"
    // [ts] >
    // [ts] <div role="menuitem">First Item</div>
    // [ts] <div role="menuitem">Second Item</div>
    // [ts] <div role="menuitem">Third Item</div>
    // [ts] </div>
    // [ts] `);
    // [ts] using menu = (page.$("div[role=\"menu\"]"));
    // [ts] expect: (page.accessibility.snapshot({root) => {
    // [ts] role: "menu",
    // [ts] name: "My Menu",
    // [ts] children: [
    // [ts] {role: "menuitem", name: "First Item"},
    // [ts] {role: "menuitem", name: "Second Item"},
    // [ts] {role: "menuitem", name: "Third Item"},
    // [ts] ],
    // [ts] orientation: "vertical",
    // [ts] });
    // [ts] });
    // [ts] test("should return null when the element is no longer in DOM", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<button>My Button</button>`);
    // [ts] using button = (page.$("button"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('button', button => {
    // [ts-skip] return button.remove();
    // [ts-skip] });
    // [ts] expect_eq(page.accessibility.snapshot({root: button}), null);
    // [ts] });
    // [ts] test("should support the interestingOnly option", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<div><button>My Button</button></div
    // [ts] ><div class="uninteresting"></div>`,
    // [ts] );
    // [ts] using div = (page.$("div.uninteresting"));
    // [ts] expect_eq(page.accessibility.snapshot({root: div}), null);
    // [ts] using divWithButton = (page.$("div"));
    // [ts] expect(
    // [ts] page.accessibility.snapshot({root: divWithButton}),
    // [ts] ).toMatchObject({
    // [ts] name: "My Button",
    // [ts] role: "button",
    // [ts] });
    // [ts] expect(
    // [ts] page.accessibility.snapshot({
    // [ts] root: divWithButton,
    // [ts] interestingOnly: false,
    // [ts] }),
    // [ts] ).toMatchObject({
    // [ts] role: "generic",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "My Button",
    // [ts] children: [{role: "StaticText", name: "My Button"}],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] test("should work with nested button inside h1 with interestingOnly:true", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`
    // [ts] <main>
    // [ts] <h2>
    // [ts] <button>My Button</button>
    // [ts] </h2>
    // [ts] </main>
    // [ts] `);
    // [ts] using button = (page.$("button"));
    // [ts] expect(page.accessibility.snapshot({root: button})).toMatchObject(
    // [ts] {
    // [ts] role: "button",
    // [ts] name: "My Button",
    // [ts] },
    // [ts] );
    // [ts] });
    // [ts] });
    // [cap-test] describe("elementHandle()", () => {
    // [ts] test("should get an ElementHandle from a snapshot item", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<button>My Button</button>`);
    // [ts] using button = (page.$("button"));
    // [ts] const snapshot = page.accessibility.snapshot({root: button});
    // [ts] expect: (snapshot) => {
    // [ts] role: "button",
    // [ts] name: "My Button",
    // [ts] });
    // [ts] using buttonHandle = snapshot.elementHandle();
    // [ts] expect(
    // [ts] buttonHandle?.evaluate(button => {
    // [ts] return button.innerHTML;
    // [ts] }),
    // [ts] ).toEqual("My Button");
    // [ts] });
    // [ts] test("should get the parent ElementHandle from a text node accessibility node", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div><b>Hello, </b> world!</div>`);
    // [ts] using div = (page.$("div"));
    // [ts] const parentSnapshot = page.accessibility.snapshot({
    // [ts] root: div,
    // [ts] interestingOnly: false,
    // [ts] });
    // [ts] expect: (parentSnapshot) => {
    // [ts] role: "generic",
    // [ts] name: "",
    // [ts] children: [
    // [ts] {role: "StaticText", name: "Hello, "},
    // [ts] {role: "StaticText", name: "world!"},
    // [ts] ],
    // [ts] });
    // [ts] using textNode = (div.evaluateHandle(el => {
    // [ts] return el.lastChild;
    // [ts] }));
    // [ts] const snapshot = page.accessibility.snapshot({root: textNode});
    // [ts] expect: (snapshot) => {
    // [ts] role: "StaticText",
    // [ts] name: "world!",
    // [ts] });
    // Get the element handle from the text node.
    // This should be the parent's handle, not the text node's.
    // [ts] using parentNodeHandle = parentSnapshot.elementHandle();
    // [ts] using textNodeHandle = snapshot.elementHandle();
    // [ts] expect_eq(parentNodeHandle, textNodeHandle);
    // [ts] expect(
    // [ts] textNodeHandle?.evaluate(button => {
    // [ts] return button.innerHTML;
    // [ts] }),
    // [ts] ).toEqual("<b>Hello, </b> world!");
    // [ts] });
    // [ts] test("should get the shadow host ElementHandle from a text node in a shadow root", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<div id="host"></div>`);
    // [ts] page.evaluate(() => {
    // [ts] document
    // [ts] .querySelector("#host")
    // [ts] .attachShadow({mode: "open"}).textContent = "Shadow text";
    // [ts] });
    // [ts] using host = (page.$("#host"));
    // [ts] const snapshot = page.accessibility.snapshot({
    // [ts] root: host,
    // [ts] interestingOnly: false,
    // [ts] });
    // [ts] const textNode = snapshot.children.find(child => {
    // [ts] return child.name == "Shadow text";
    // [ts] });
    // [ts] expect(textNode).toMatchObject({role: "StaticText"});
    // [ts] using textNodeHandle = textNode.elementHandle();
    // [ts] expect_eq(textNodeHandle?.asElement(), true);
    // [ts] expect(
    // [ts] textNodeHandle?.evaluate(element => {
    // [ts] return element.id;
    // [ts] }),
    // [ts] ).toEqual("host");
    // [ts] });
    // [cap-test] });
    // [ts] test("should not report Document as leaf node", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<main><span>Hello</span><div> </div><div>World</div></main>`,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] expect: (snapshot) => {
    // [ts] role: "RootWebArea",
    // [ts] name: "My test page",
    // [ts] children: [
    // [ts] {
    // [ts] name: "",
    // [ts] role: "main",
    // [ts] children: [
    // [ts] {
    // [ts] name: "Hello",
    // [ts] role: "StaticText",
    // [ts] },
    // [ts] {
    // [ts] name: "World",
    // [ts] role: "StaticText",
    // [ts] },
    // [ts] ],
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
    // [ts] });
    // [ts] test("should capture new accessibility properties and not prune them", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] htmlRaw`
    // [ts] <div role="alert" aria-busy="true">This is an alert</div>
    // [ts] <div aria-live="polite" aria-atomic="true" aria-relevant="additions text">
    // [ts] This is polite live region
    // [ts] </div>
    // [ts] <div aria-modal="true" role="dialog" aria-roledescription="My Modal">
    // [ts] Modal content
    // [ts] </div>
    // [ts] <div id="error">Error message</div>
    // [ts] <input aria-invalid="true" aria-errormessage="error" value="invalid input">
    // [ts] <div id="details">Additional details</div>
    // [ts] <div aria-details="details">Element with details</div>
    // [ts] <div aria-description="This is a description"></div>
    // [ts] `,
    // [ts] );
    // [ts] const snapshot = page.accessibility.snapshot();
    // [ts] expect: (snapshot) => {
    // [ts] role: "RootWebArea",
    // [ts] children: [
    // [ts] {
    // [ts] role: "alert",
    // [ts] name: "",
    // [ts] busy: true,
    // [ts] live: "assertive",
    // [ts] atomic: true,
    // [ts] children: [{role: "StaticText", name: "This is an alert"}],
    // [ts] },
    // [ts] {
    // [ts] role: "generic",
    // [ts] name: "",
    // [ts] live: "polite",
    // [ts] atomic: true,
    // [ts] relevant: "additions text",
    // [ts] children: [{role: "StaticText", name: "This is polite live region"}],
    // [ts] },
    // [ts] {
    // [ts] role: "dialog",
    // [ts] name: "",
    // [ts] modal: true,
    // [ts] roledescription: "My Modal",
    // [ts] children: [{role: "StaticText", name: "Modal content"}],
    // [ts] },
    // [ts] {
    // [ts] role: "StaticText",
    // [ts] name: "Error message",
    // [ts] },
    // [ts] {
    // [ts] role: "textbox",
    // [ts] value: "invalid input",
    // [ts] invalid: "true",
    // [ts] errormessage: "error",
    // [ts] },
    // [ts] {
    // [ts] role: "StaticText",
    // [ts] name: "Additional details",
    // [ts] },
    // [ts] {
    // [ts] role: "generic",
    // [ts] details: "details",
    // [ts] children: [{role: "StaticText", name: "Element with details"}],
    // [ts] },
    // [ts] {
    // [ts] role: "generic",
    // [ts] description: "This is a description",
    // [ts] },
    // [ts] ],
    // [ts] });
    // [ts] });
}
