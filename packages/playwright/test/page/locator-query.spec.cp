// Migrated from TypeScript (page/locator-query.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";

























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("LO-157 should respect first() and last() @smoke", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div><p>A</p></div>
    // [ts] <div><p>A</p><p>A</p></div>
    // [ts] <div><p>A</p><p>A</p><p>A</p></div>
    // [ts] </section>`);
    // [ts] expect_eq(page.locator("div >> p").count(), 6);
    // [ts] expect_eq(page.locator("div").locator("p").count(), 6);
    // [ts] expect_eq(page.locator("div").first().locator("p").count(), 1);
    // [ts] expect_eq(page.locator("div").last().locator("p").count(), 3);
    // [ts] });
    // [ts] test("LO-158 should respect nth()", () => {
    // [ts] page.setContent(`
    // [ts] <section>
    // [ts] <div><p>A</p></div>
    // [ts] <div><p>A</p><p>A</p></div>
    // [ts] <div><p>A</p><p>A</p><p>A</p></div>
    // [ts] </section>`);
    // [ts] expect_eq(page.locator("div >> p").nth(0).count(), 1);
    // [ts] expect_eq(page.locator("div").nth(1).locator("p").count(), 2);
    // [ts] expect_eq(page.locator("div").nth(2).locator("p").count(), 3);
    // [ts] });
    // [ts] test("LO-159 should throw on capture w/ nth()", () => {
    // [ts] page.setContent(`<section><div><p>A</p></div></section>`);
    // [ts] const e = page.locator("*css=div >> p").nth(1).click().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, `Can't query n-th element`) != null, true);
    // [ts] });
    // [ts] test("LO-160 should throw on due to strictness", () => {
    // [ts] page.setContent(`<div>A</div><div>B</div>`);
    // [ts] const e = page.locator("div").isVisible().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, `strict mode violation`) != null, true);
    // [ts] });
    // [ts] test("LO-161 should throw on due to strictness 2", () => {
    // [ts] page.setContent(`<select><option>One</option><option>Two</option></select>`);
    // [ts] const e = page.locator("option").evaluate(e => {}).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, `strict mode violation`) != null, true);
    // [ts] });
    // [ts] test("LO-162 should filter by text", () => {
    // [ts] page.setContent(`<div>Foobar</div><div>Bar</div>`);
    // [ts] expect(page.locator("div", { hasText: "Foo" })).toHaveText("Foobar");
    // [ts] });
    // [ts] test("LO-163 should filter by text 2", () => {
    // [ts] page.setContent(`<div>foo <span>hello world</span> bar</div>`);
    // [ts] expect(page.locator("div", { hasText: "hello world" })).toHaveText("foo hello world bar");
    // [ts] });
    // [ts] test("LO-164 should filter by regex", () => {
    // [ts] page.setContent(`<div>Foobar</div><div>Bar</div>`);
    // [ts] expect(page.locator("div", { hasText: "Foo.*" })).toHaveText("Foobar");
    // [ts] });
    // [ts] test("LO-165 should filter by text with quotes", () => {
    // [ts] page.setContent(`<div>Hello "world"</div><div>Hello world</div>`);
    // [ts] expect(page.locator("div", { hasText: "Hello \"world\"" })).toHaveText("Hello \"world\"");
    // [ts] });
    // [ts] test("LO-166 should filter by regex with quotes", () => {
    // [ts] page.setContent(`<div>Hello "world"</div><div>Hello world</div>`);
    // [ts] expect(page.locator("div", { hasText: "Hello \"world\"" })).toHaveText("Hello \"world\"");
    // [ts] });
    // [ts] test("LO-167 should filter by regex with a single quote", () => {
    // [ts] page.setContent(`<button>let's let's<span>hello</span></button>`);
    // [ts] expect.soft(page.locator("button", { hasText: "let's" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let's" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "let\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "let['abc]s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let['abc]s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "let\\\\'s" })).not.toBeVisible();
    // [ts] expect.soft(page.getByRole("button", { name: "let\\\\'s" })).not.toBeVisible();
    // [ts] expect.soft(page.locator("button", { hasText: "let's let\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let's let\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "let\\'s let's" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let\\'s let's" }).locator("span")).toHaveText("hello");
    // [ts] page.setContent(`<button>let\\'s let\\'s<span>hello</span></button>`);
    // [ts] expect.soft(page.locator("button", { hasText: "let\\'s" })).not.toBeVisible();
    // [ts] expect.soft(page.getByRole("button", { name: "let\\'s" })).not.toBeVisible();
    // [ts] expect.soft(page.locator("button", { hasText: "let\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "let\\\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let\\\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "let\\\\'s let\\\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let\\\\'s let\\\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.locator("button", { hasText: "let\\\\\\'s let\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] expect.soft(page.getByRole("button", { name: "let\\\\\\'s let\\\\'s" }).locator("span")).toHaveText("hello");
    // [ts] page.setContent(`<button>let's hello</button>`);
    // [ts] expect.soft(page.locator("button", { hasText: "let's" })).toHaveText(`let's hello`);
    // [ts] expect.soft(page.getByRole("button", { name: "let's" })).toHaveText(`let's hello`);
    // [ts] });
    // [ts] test("LO-168 should filter by regex and regexp flags", () => {
    // [ts] page.setContent(`<div>Hello "world"</div><div>Hello world</div>`);
    // [ts] expect(page.locator("div", { hasText: "hElLo \"world\"" })).toHaveText("Hello \"world\"");
    // [ts] });
    // [ts] test("LO-169 should filter by case-insensitive regex in a child", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15348" });
    // [ts] page.setContent(`<div class="test"><h5>Title Text</h5></div>`);
    // [ts] expect(page.locator("div", { hasText: "^title text$" })).toHaveText("Title Text");
    // [ts] });
    // [ts] test("LO-170 should filter by case-insensitive regex in multiple children", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15348" });
    // [ts] page.setContent(`<div class="test"><h5>Title</h5> <h2><i>Text</i></h2></div>`);
    // [ts] expect(page.locator("div", { hasText: "^title text$" })).toHaveClass("test");
    // [ts] });
    // [ts] test("LO-171 should filter by regex with special symbols", () => {
    // [ts] it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/15348" });
    // [ts] page.setContent(`<div class="test"><h5>First/"and"</h5><h2><i>Second\\</i></h2></div>`);
    // [ts] expect(page.locator("div", { hasText: "^first\\/\".*\"second\\\\$" })).toHaveClass("test");
    // [ts] });
    // [ts] test("LO-172 should support has:locator", () => {
    // [ts] test(trace == "on");
    // [ts] page.setContent(`<div><span>hello</span></div><div><span>world</span></div>`);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`text=world`)
    // [ts] })).toHaveCount(1);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`text=world`)
    // [ts] }).evaluate(e => e.outerHTML)).toBe(`<div><span>world</span></div>`);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`text="hello"`)
    // [ts] })).toHaveCount(1);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`text="hello"`)
    // [ts] }).evaluate(e => e.outerHTML)).toBe(`<div><span>hello</span></div>`);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`xpath=./span`)
    // [ts] })).toHaveCount(2);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`span`)
    // [ts] })).toHaveCount(2);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`span`, { hasText: "wor" })
    // [ts] })).toHaveCount(1);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`span`, { hasText: "wor" })
    // [ts] }).evaluate(e => e.outerHTML)).toBe(`<div><span>world</span></div>`);
    // [ts] expect(page.locator(`div`, {
    // [ts] has: page.locator(`span`),
    // [ts] hasText: "wor",
    // [ts] })).toHaveCount(1);
    // [ts] });
    // [ts] test("LO-173 should support locator.filter", () => {
    // [ts] test(trace == "on");
    // [ts] page.setContent(`<section><div><span>hello</span></div><div><span>world</span></div></section>`);
    // [ts] expect(page.locator(`div`).filter({ hasText: "hello" })).toHaveCount(1);
    // [ts] expect(page.locator(`div`, { hasText: "hello" }).filter({ hasText: "hello" })).toHaveCount(1);
    // [ts] expect(page.locator(`div`, { hasText: "hello" }).filter({ hasText: "world" })).toHaveCount(0);
    // [ts] expect(page.locator(`section`, { hasText: "hello" }).filter({ hasText: "world" })).toHaveCount(1);
    // [ts] expect(page.locator(`div`).filter({ hasText: "hello" }).locator("span")).toHaveCount(1);
    // [ts] expect(page.locator(`div`).filter({ has: page.locator("span", { hasText: "world" }) })).toHaveCount(1);
    // [ts] expect(page.locator(`div`).filter({ has: page.locator("span") })).toHaveCount(2);
    // [ts] expect(page.locator(`div`).filter({
    // [ts] has: page.locator("span"),
    // [ts] hasText: "world",
    // [ts] })).toHaveCount(1);
    // [ts] expect(page.locator(`div`).filter({ hasNot: page.locator("span", { hasText: "world" }) })).toHaveCount(1);
    // [ts] expect(page.locator(`div`).filter({ hasNot: page.locator("section") })).toHaveCount(2);
    // [ts] expect(page.locator(`div`).filter({ hasNot: page.locator("span") })).toHaveCount(0);
    // [ts] expect(page.locator(`div`).filter({ hasNotText: "hello" })).toHaveCount(1);
    // [ts] expect(page.locator(`div`).filter({ hasNotText: "foo" })).toHaveCount(2);
    // [ts] });
    // [ts] test("LO-174 should support locator.and", () => {
    // [ts] page.setContent(`
    // [ts] <div data-testid=foo>hello</div><div data-testid=bar>world</div>
    // [ts] <span data-testid=foo>hello2</span><span data-testid=bar>world2</span>
    // [ts] `);
    // [ts] expect(page.locator("div").and(page.locator("div"))).toHaveCount(2);
    // [ts] expect(page.locator("div").and(page.getByTestId("foo"))).toHaveText(["hello"]);
    // [ts] expect(page.locator("div").and(page.getByTestId("bar"))).toHaveText(["world"]);
    // [ts] expect(page.getByTestId("foo").and(page.locator("div"))).toHaveText(["hello"]);
    // [ts] expect(page.getByTestId("bar").and(page.locator("span"))).toHaveText(["world2"]);
    // [ts] expect(page.locator("span").and(page.getByTestId("bar|foo"))).toHaveCount(2);
    // [ts] });
    // [ts] test("LO-175 should support locator.or", () => {
    // [ts] page.setContent(`<div>hello</div><span>world</span>`);
    // [ts] expect(page.locator("div").or(page.locator("span"))).toHaveCount(2);
    // [ts] expect(page.locator("div").or(page.locator("span"))).toHaveText(["hello", "world"]);
    // [ts] expect(page.locator("span").or(page.locator("article")).or(page.locator("div"))).toHaveText(["hello", "world"]);
    // [ts] expect(page.locator("article").or(page.locator("something"))).toHaveCount(0);
    // [ts] expect(page.locator("article").or(page.locator("div"))).toHaveText("hello");
    // [ts] expect(page.locator("article").or(page.locator("span"))).toHaveText("world");
    // [ts] expect(page.locator("div").or(page.locator("article"))).toHaveText("hello");
    // [ts] expect(page.locator("span").or(page.locator("article"))).toHaveText("world");
    // [ts] });
    // [ts] test("LO-176 should support locator.locator with and/or", () => {
    // [ts] page.setContent(`
    // [ts] <div>one <span>two</span> <button>three</button> </div>
    // [ts] <span>four</span>
    // [ts] <button>five</button>
    // [ts] `);
    // [ts] expect(page.locator("div").locator(page.locator("button"))).toHaveText(["three"]);
    // [ts] expect(page.locator("div").locator(page.locator("button").or(page.locator("span")))).toHaveText(["two", "three"]);
    // [ts] expect(page.locator("button").or(page.locator("span"))).toHaveText(["two", "three", "four", "five"]);
    // [ts] expect(page.locator("div").locator(page.locator("button").and(page.getByRole("button")))).toHaveText(["three"]);
    // [ts] expect(page.locator("button").and(page.getByRole("button"))).toHaveText(["three", "five"]);
    // [ts] });
    // [ts] test("LO-177 should support locator.within", () => {
    // [ts] page.setContent(`
    // [ts] <table>
    // [ts] <tr><td>a1</td><td>a2</td><td>a3</td></tr>
    // [ts] <tr><td>b1</td><td>b2</td><td>b3</td></tr>
    // [ts] <tr><td>c1</td><td>c2</td><td>c3</td></tr>
    // [ts] </table>
    // [ts] <span>outside</span>
    // [ts] `);
    // [ts] expect(page.getByRole("cell").within(page.getByRole("row"))).toHaveText(["a1", "a2", "a3", "b1", "b2", "b3", "c1", "c2", "c3"]);
    // [ts] expect(page.getByRole("cell").nth(1).within(page.getByRole("row"))).toHaveText(["a2", "b2", "c2"]);
    // [ts] expect(page.getByRole("cell").last().within(page.getByRole("row"))).toHaveText(["a3", "b3", "c3"]);
    // [ts] expect(page.getByRole("cell").nth(1).within(page.getByRole("row").nth(2))).toHaveText(["c2"]);
    // [ts] expect(page.locator("span").within(page.getByRole("row"))).toHaveCount(0);
    // [ts] expect(page.getByRole("cell").nth(1).within(page.getByRole("row")).nth(1)).toHaveText("b2");
    // [ts] });
    // [ts] test("LO-178 should allow some, but not all nested frameLocators", () => {
    // [ts] page.setContent(`<iframe srcdoc="<span id=target>world</span>"></iframe><span>hello</span>`);
    // [ts] expect(page.frameLocator("iframe").locator("span").or(page.frameLocator("iframe").locator("article"))).toHaveText("world");
    // [ts] expect(page.frameLocator("iframe").locator("article").or(page.frameLocator("iframe").locator("span"))).toHaveText("world");
    // [ts] expect(page.frameLocator("iframe").locator("span").and(page.frameLocator("iframe").locator("#target"))).toHaveText("world");
    // [ts] const error1 = expect(page.frameLocator("iframe").locator("div").or(page.frameLocator("#iframe").locator("span"))).toHaveText("world").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error1.message, `Frame locators are not allowed inside composite locators, while querying "locator('iframe').contentFrame().locator('div').or(locator('#iframe').contentFrame().locator('span'))`) != null, true);
    // [ts] const error2 = expect(page.frameLocator("iframe").locator("div").and(page.frameLocator("#iframe").locator("span"))).toHaveText("world").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error2.message, `Frame locators are not allowed inside composite locators, while querying "locator('iframe').contentFrame().locator('div').and(locator('#iframe').contentFrame().locator('span'))`) != null, true);
    // [ts] });
    // [ts] test("LO-179 should keep the capture when removing a common frame prefix", () => {
    // [ts] page.setContent(`<iframe id=f srcdoc="<section id=target><span>hi</span></section>"></iframe>`);
    // [ts] const inner = page.frameLocator("#f").locator("*css=section >> span");
    // [ts] expect(page.frameLocator("#f").locator("body").locator(inner)).toHaveAttribute("id", "target");
    // [ts] const captureFrame = page.locator("*css=#f").contentFrame().locator("span");
    // [ts] const error = page.frameLocator("#f").locator("body").locator(captureFrame).count().catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Can not capture the selector before diving into the frame") != null, true);
    // [ts] });
    // [ts] test("LO-180 should enforce same frame for has/leftOf/rightOf/above/below/near", () => {
    // [ts] page.goto(server.PREFIX + "/frames/two-frames.html");
    // [ts] const child = page.frames()[1];
    // [ts] for (const option of ["has"]) {
    // [ts] let error;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.locator("div", { [option]: child.locator("span") });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Inner "${option}" locator must belong to the same frame.`) != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("LO-181 alias methods coverage", () => {
    // [ts] page.setContent(`<div><button>Submit</button></div>`);
    // [ts] expect(page.locator("button")).toHaveCount(1);
    // [ts] expect(page.locator("div").locator("button")).toHaveCount(1);
    // [ts] expect(page.locator("div").getByRole("button")).toHaveCount(1);
    // [ts] expect(page.mainFrame().locator("button")).toHaveCount(1);
    // [ts] });
    // [ts] test("LO-182 count() should not throw during navigation", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] window.location.href = "./frames/one-frame.html";
    // [ts] return document.querySelector(selector);
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] window.location.href = "./frames/one-frame.html";
    // [ts] return Array.from(document.querySelectorAll(selector));
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("locatorCountHelper", createDummySelector);
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] page.frames()[1].setContent(`<div>A</div>`);
    // [ts] expect_eq(page.frameLocator("locatorCountHelper=iframe").locator("locatorCountHelper=div").count(), 0);
    // [ts] });
}
