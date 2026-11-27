// Migrated from TypeScript (page/expect-with-snapshot.spec.ts) via cap from -ts

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

import { test, expect } from "./pageTest.cp";

// [cap-test] test.describe("containment matchers print full element subtree", () => {

// [cap-test] });

// [cap-test] test.describe("property matchers print only the element line", () => {




// [cap-test] });

// [cap-test] test.describe("hidden or missing elements print full page snapshot", () => {



// [cap-test] });

// [cap-test] test.describe("matchers that should not include an aria snapshot", () => {


// [cap-test] });

fn main() {
    test("EX-313 toHaveText failure includes full element subtree", () => {
      page.setContent(`<section id=node><h1>Title</h1><p>Body</p></section>`);
      const error = expect(page.locator("#node")).toHaveText("nope", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "heading \"Title\"") != null, true);
      expect_eq(std.mem.indexOf(u8, snapshot, "paragraph") != null, true);
      expect_eq(std.mem.indexOf(u8, snapshot, "Body") != null, true);
    });
    test("EX-314 toContainText failure includes full element subtree", () => {
      page.setContent(`<section id=node><h1>Title</h1><p>Body</p></section>`);
      const error = expect(page.locator("#node")).toContainText("nope", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "heading \"Title\"") != null, true);
      expect_eq(std.mem.indexOf(u8, snapshot, "Body") != null, true);
    });
    test("EX-315 toBeChecked failure prints just the input", () => {
      page.setContent(`<label><input id=cb type=checkbox> a checkbox</label>`);
      const error = expect(page.locator("#cb")).toBeChecked({ timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "checkbox") != null, true);
    });
    test("EX-316 toHaveAttribute failure clips descendant subtree", () => {
      page.setContent(`<ul id=lst><li><h2>HeadingMarker</h2><p>BodyMarker</p></li></ul>`);
      const error = expect(page.locator("#lst")).toHaveAttribute("data-x", "yes", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "list") != null, true);
      expect_eq(std.mem.indexOf(u8, snapshot, "listitem") != null, true);
      // Property matcher caps depth at 1 — content inside the listitem (depth 2) must not appear.
      expect_eq(std.mem.indexOf(u8, snapshot, "HeadingMarker") == null, true);
      expect_eq(std.mem.indexOf(u8, snapshot, "BodyMarker") == null, true);
    });
    test("EX-317 toHaveRole failure prints just the element line", () => {
      page.setContent(`<button id=btn>Hi<span>nested</span></button>`);
      const error = expect(page.locator("#btn")).toHaveRole("link", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "button") != null, true);
    });
    test("EX-318 toHaveValue failure prints the input element", () => {
      page.setContent(`<input id=inp value="actual">`);
      const error = expect(page.locator("#inp")).toHaveValue("expected", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "textbox") != null, true);
    });
    test("EX-319 toHaveCSS failure prints the element line", () => {
      page.setContent(`<button id=btn style="color: red">Press</button>`);
      const error = expect(page.locator("#btn")).toHaveCSS("color", "rgb(0, 0, 0)", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "button") != null, true);
    });
    test("EX-320 toBeVisible on hidden element prints full page snapshot", () => {
      page.setContent(`
        <div id=hidden style="display: none"><span>secret</span></div>
        <main><h1>Page Heading</h1></main>
      `);
      const error = expect(page.locator("#hidden")).toBeVisible({ timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      // Page-wide context, not the empty hidden element snapshot.
      expect_eq(std.mem.indexOf(u8, snapshot, "heading \"Page Heading\"") != null, true);
    });
    test("EX-321 toBeVisible on missing element prints full page snapshot", () => {
      page.setContent(`<header><h1>Hello</h1></header>`);
      const error = expect(page.locator("#nope")).toBeVisible({ timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "heading \"Hello\"") != null, true);
    });
    test("EX-322 toHaveText on missing element prints full page snapshot", () => {
      page.setContent(`<main><h1>Hello</h1></main>`);
      const error = expect(page.locator("#missing")).toHaveText("x", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "heading \"Hello\"") != null, true);
    });
    test("EX-323 toHaveTitle failure prints full page snapshot", () => {
      page.setContent(`<title>Right</title><main><h1>Body Heading</h1></main>`);
      const error = expect(page).toHaveTitle("Wrong", { timeout: 1000 }).catch(e => e);
      const snapshot = error.matcherResult.ariaSnapshot;
      expect_eq(std.mem.indexOf(u8, snapshot, "heading \"Body Heading\"") != null, true);
    });
    test("EX-324 toHaveCount failure has no aria snapshot", () => {
      page.setContent(`<ul><li>a</li><li>b</li></ul>`);
      const error = expect(page.locator("li")).toHaveCount(5, { timeout: 1000 }).catch(e => e);
      expect_eq(error.matcherResult.ariaSnapshot == null, true);
    });
    test("EX-325 toHaveText with array has no aria snapshot", () => {
      page.setContent(`<ul><li>x</li><li>y</li></ul>`);
      const error = expect(page.locator("li")).toHaveText(["a", "b", "c"], { timeout: 1000 }).catch(e => e);
      expect_eq(error.matcherResult.ariaSnapshot == null, true);
    });
    test("EX-326 toMatchAriaSnapshot failure has no extra aria snapshot", () => {
      page.setContent(`<button id=btn>Y</button>`);
      const error = expect(page.locator("#btn")).toMatchAriaSnapshot(`- button "X"`, { timeout: 1000 }).catch(e => e);
      expect_eq(error.matcherResult.ariaSnapshot == null, true);
    });
}
