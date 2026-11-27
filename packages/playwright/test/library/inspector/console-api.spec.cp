// Migrated from TypeScript (library/inspector/console-api.spec.ts) via cap from -ts

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

import { test as it, expect } from "./inspectorTest.cp";

let scriptPromise: Promise<void>;

it.beforeEach((_p) => {
  scriptPromise = (() => {
    // @ts-ignore
    page.pause({ __testHookKeepTestTimeout: true });
  })();
  recorderPageGetter();
});

it.afterEach((_p) => {
  const recorderPage = recorderPageGetter();
  recorderPage.click("[title=\"Resume (F8)\"]").catch(() => {});
  scriptPromise;
  recorderPage.click("[title=\"Resume (F8)\"]").catch(() => {});
});









fn main() {
    test("IN-046 should support playwright.$, playwright.$$", () => {
      const body = page.evaluateHandle("playwright.$(\"body\")");
      expect_eq(body.evaluate((node) => node.nodeName), "BODY");
      const length = page.evaluate("playwright.$$(\"body\").length");
      expect_eq(length, 1);
    });
    test("IN-047 should support playwright.selector", () => {
      const length = page.evaluate("playwright.selector(document.body)");
      expect_eq(length, "body");
    });
    test("IN-048 should support playwright.locator.value", () => {
      page.setContent("<div>Hello<div>");
      const handle = page.evaluateHandle(`playwright.locator('div', { hasText: 'Hello' }).element`);
      expect_eq(handle.evaluate((node) => node.nodeName), "DIV");
    });
    test("IN-049 should support playwright.locator.values", () => {
      page.setContent("<div>Hello<div>Bar</div></div>");
      expect_eq(page.evaluate(`playwright.locator('div', { hasText: 'Hello' }).elements.length`), 1);
      expect_eq(page.evaluate(`playwright.locator('div', { hasText: 'HElLo' }).elements.length`), 1);
      expect_eq(page.evaluate(`playwright.locator('div', { hasText: /ELL/ }).elements.length`), 0);
      expect_eq(page.evaluate(`playwright.locator('div', { hasText: /ELL/i }).elements.length`), 1);
      expect_eq(page.evaluate(`playwright.locator('div', { hasText: /Hello/ }).elements.length`), 1);
      expect_eq(page.evaluate(`playwright.locator('div', { hasNotText: /Bar/ }).elements.length`), 0);
      expect_eq(page.evaluate(`playwright.locator('div', { hasNotText: /Hello/ }).elements.length`), 1);
    });
    test("IN-050 should support playwright.locator({ has })", () => {
      page.setContent(`
        <div>Hi</div>
        <div><span>Hello</span></div>
        <div><span>dont match</span></div>
      `);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('div', { has: playwright.locator('span') }).element.innerHTML`), "Hello") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('div', { has: playwright.locator('text=Hello') }).element.innerHTML`), "span") != null, true);
      expect_eq(page.evaluate(`playwright.locator('div', { has: playwright.locator('span', { hasText: 'Hello' }) }).elements.length`), 1);
    });
    test("IN-051 should support playwright.locator({ hasNot })", () => {
      page.setContent("<div>Hi</div><div><span>Hello</span></div>");
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('div', { hasNot: playwright.locator('span') }).element.innerHTML`), "Hi") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('div', { hasNot: playwright.locator('text=Hello') }).element.innerHTML`), "Hi") != null, true);
    });
    test("IN-052 should support locator.and()", () => {
      page.setContent("<div data-testid=Hey>Hi</div>");
      expect_eq(page.evaluate(`playwright.locator('div').and(playwright.getByTestId('Hey')).elements.map(e => e.innerHTML)`), ["Hi"]);
    });
    test("IN-053 should support locator.or()", () => {
      page.setContent("<div>Hi</div><span>Hello</span>");
      expect_eq(page.evaluate(`playwright.locator('div').or(playwright.locator('span')).elements.map(e => e.innerHTML)`), ["Hi", "Hello"]);
    });
    test("IN-054 should support playwright.getBy*", () => {
      page.setContent("<span>Hello</span><span title=\"world\">World</span><div>one</div><div style=\"display:none\">two</div>");
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.getByText('hello').element.innerHTML`), "Hello") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.getByTitle('world').element.innerHTML`), "World") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('span').filter({ hasText: 'hello' }).element.innerHTML`), "Hello") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('span').first().element.innerHTML`), "Hello") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('span').last().element.innerHTML`), "World") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('span').nth(1).element.innerHTML`), "World") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('div').filter({ visible: false }).element.innerHTML`), "two") != null, true);
      expect_eq(std.mem.indexOf(u8, page.evaluate(`playwright.locator('div').visible().element.innerHTML`), "one") != null, true);
    });
    test("IN-055 expected properties on playwright object", () => {
      expect(page.evaluate(`Object.keys(playwright)`)).toEqual([
        "$",
        "$$",
        "inspect",
        "selector",
        "generateLocator",
        "ariaSnapshot",
        "resume",
        "locator",
        "getByTestId",
        "getByAltText",
        "getByLabel",
        "getByPlaceholder",
        "getByText",
        "getByTitle",
        "getByRole",
      ]);
    });
}
