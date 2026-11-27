// Migrated from TypeScript (page/expect-misc.spec.ts) via cap from -ts

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

import { stripAnsi } from "../config/utils.cp";
import { test, expect } from "./pageTest.cp";

// [cap-test] test.describe("toHaveCount", () => {






// [cap-test] });

// [cap-test] test.describe("toHaveJSProperty", () => {













// [cap-test] });

// [cap-test] test.describe("toHaveClass", () => {




// [cap-test] });

// [cap-test] test.describe("toContainClass", () => {




// [cap-test] });

// [cap-test] test.describe("toHaveTitle", () => {

// [cap-test] });

// [cap-test] test.describe("toHaveURL", () => {








// [cap-test] });

// [cap-test] test.describe("toHaveAttribute", () => {





// [cap-test] });

// [cap-test] test.describe("toHaveCSS", () => {


// [cap-test] });

// [cap-test] test.describe("toHaveId", () => {
// [cap-test] });

// [cap-test] test.describe("toBeInViewport", () => {



// [cap-test] });






fn main() {
    test("EX-181 toHaveCount pass", () => {
      page.setContent("<select><option>One</option></select>");
      const locator = page.locator("option");
      let done = false;
      const promise = expect(locator).toHaveCount(2).then(() => { done = true; });
      page.waitForTimeout(1000);
      expect_eq(done, false);
      page.setContent("<select><option>One</option><option>Two</option></select>");
      promise;
      expect_eq(done, true);
    });
    test("EX-182 pass zero", () => {
      page.setContent("<div></div>");
      const locator = page.locator("span");
      expect(locator).toHaveCount(0);
      expect(locator).not.toHaveCount(1);
    });
    test("EX-183 eventually pass zero", () => {
      page.setContent("<div><span>hello</span></div>");
      const locator = page.locator("span");
      setTimeout(() => page.evaluate(() => document.querySelector("div").textContent = "").catch(() => {}), 200);
      expect(locator).toHaveCount(0);
      expect(locator).not.toHaveCount(1);
    });
    test("EX-184 eventually pass non-zero", () => {
      page.setContent("<ul></ul>");
      setTimeout(() => {
        page.setContent("<ul><li>one</li><li>two</li></ul>");
      }, 500);
      const locator = page.locator("li");
      expect(locator).toHaveCount(2);
    });
    test("EX-185 eventually pass not non-zero", () => {
      page.setContent("<ul><li>one</li><li>two</li></ul>");
      setTimeout(() => {
        page.setContent("<ul></ul>");
      }, 500);
      const locator = page.locator("li");
      expect(locator).not.toHaveCount(2);
    });
    test("EX-186 fail zero", () => {
      page.setContent("<div><span></span></div>");
      const locator = page.locator("span");
      const error = expect(locator).toHaveCount(0, { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveCount(expected) failed

    Locator:  locator('span')
    Expected: 0
    Received: 1
    Timeout:  1000ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveCount" locator('span') with timeout 1000ms`) != null, true);
    });
    test("EX-187 fail zero 2", () => {
      page.setContent("<div><span></span></div>");
      const locator = page.locator("span");
      const error = expect(locator).not.toHaveCount(1, { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).not.toHaveCount(expected) failed

    Locator:  locator('span')
    Expected: not 1
    Received: 1
    Timeout:  1000ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "not toHaveCount" locator('span') with timeout 1000ms`) != null, true);
    });
    test("EX-188 pass", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = { a: 1, b: 'string', c: new Date(1627503992000) });
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo", { a: 1, b: "string", c: Date.init(1627503992000) });
    });
    test("EX-189 fail", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = { a: 1, b: 'string', c: new Date(1627503992000) });
      const locator = page.locator("div");
      const error = expect(locator).toHaveJSProperty("foo", { a: 1, b: "string", c: Date.init(1627503992001) }, { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `-   "c"`) != null, true);
    });
    test("EX-190 pass string", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = 'string');
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo", "string");
    });
    test("EX-191 fail string", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = 'string');
      const locator = page.locator("div");
      const error = expect(locator).toHaveJSProperty("foo", "error", { timeout: 200 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveJSProperty(expected) failed

    Locator:  locator('div')
    Expected: "error"
    Received: "string"
    Timeout:  200ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveJSProperty" locator('div') with timeout 200ms`) != null, true);
    });
    test("EX-192 pass number", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = 2021);
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo", 2021);
    });
    test("EX-193 fail number", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = 2021);
      const locator = page.locator("div");
      const error = expect(locator).toHaveJSProperty("foo", 1, { timeout: 200 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveJSProperty(expected) failed

    Locator:  locator('div')
    Expected: 1
    Received: 2021
    Timeout:  200ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveJSProperty" locator('div') with timeout 200ms`) != null, true);
    });
    test("EX-194 pass boolean", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = true);
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo", true);
    });
    test("EX-195 fail boolean", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = false);
      const locator = page.locator("div");
      const error = expect(locator).toHaveJSProperty("foo", true, { timeout: 200 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveJSProperty(expected) failed

    Locator:  locator('div')
    Expected: true
    Received: false
    Timeout:  200ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveJSProperty" locator('div') with timeout 200ms`) != null, true);
    });
    test("EX-196 pass boolean 2", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = false);
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo", false);
    });
    test("EX-197 fail boolean 2", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = false);
      const locator = page.locator("div");
      const error = expect(locator).toHaveJSProperty("foo", true, { timeout: 200 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveJSProperty(expected) failed

    Locator:  locator('div')
    Expected: true
    Received: false
    Timeout:  200ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveJSProperty" locator('div') with timeout 200ms`) != null, true);
    });
    test("EX-198 pass undefined", () => {
      page.setContent("<div></div>");
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo", null);
    });
    test("EX-199 pass null", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = null);
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo", null);
    });
    test("EX-200 pass nested", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = { nested: { a: 1, b: 'string', c: new Date(1627503992000) } });
      const locator = page.locator("div");
      expect(locator).toHaveJSProperty("foo.nested", { a: 1, b: "string", c: Date.init(1627503992000) });
      expect(locator).toHaveJSProperty("foo.nested.a", 1);
      expect(locator).toHaveJSProperty("foo.nested.b", "string");
      expect(locator).toHaveJSProperty("foo.nested.c", Date.init(1627503992000));
    });
    test("EX-201 fail nested", () => {
      page.setContent("<div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('div', e => (e as any).foo = { nested: { a: 1, b: 'string', c: new Date(1627503992000) } });
      const locator = page.locator("div");
      const error1 = expect(locator).toHaveJSProperty("foo.bar", { a: 1, b: "string", c: Date.init(1627503992001) }, { timeout: 1000 }).catch(e => e);
      expect.soft(stripAnsi(error1.message)).toContain(`Received: undefined`);
      const error2 = expect(locator).toHaveJSProperty("foo.nested.a", 2, { timeout: 1000 }).catch(e => e);
      expect.soft(stripAnsi(error2.message)).toContain(`Received: 1`);
    });
    test("EX-202 pass", () => {
      page.setContent("<div class=\"foo bar baz\"></div>");
      const locator = page.locator("div");
      expect(locator).toHaveClass("foo bar baz");
    });
    test("EX-203 pass with SVGs", () => {
      page.setContent(`<svg class="c1 c2" role="img" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"></svg>`);
      expect(page.locator("svg")).toHaveClass("c1");
    });
    test("EX-204 fail", () => {
      page.setContent("<div class=\"bar baz\"></div>");
      const locator = page.locator("div");
      const error = expect(locator).toHaveClass("foo bar baz", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveClass(expected) failed

    Locator:  locator('div')
    Expected: "foo bar baz"
    Received: "bar baz"
    Timeout:  1000ms

    Call log:
    `);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveClass" locator('div') with timeout 1000ms`) != null, true);
    });
    test("EX-205 pass with array", () => {
      page.setContent("<div class=\"foo\"></div><div class=\"bar\"></div><div class=\"baz\"></div>");
      const locator = page.locator("div");
      expect(locator).toHaveClass(["foo", "bar", "[a-z]az"]);
    });
    test("EX-206 fail with array", () => {
      page.setContent("<div class=\"foo\"></div><div class=\"bar\"></div><div class=\"bar\"></div>");
      const locator = page.locator("div");
      const error = expect(locator).toHaveClass(["foo", "bar", "[a-z]az"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(locator).toHaveClass(expected) failed`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `Timeout: 1000ms`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveClass" locator('div') with timeout 1000ms`) != null, true);
    });
    test("EX-207 pass", () => {
      page.setContent("<div class=\"foo bar baz\"></div>");
      const locator = page.locator("div");
      expect(locator).toContainClass("");
      expect(locator).toContainClass("bar");
      expect(locator).toContainClass("baz bar");
      expect(locator).toContainClass("  bar   foo ");
      expect(locator).not.toContainClass("  baz   not-matching "); // Strip whitespace and match individual classes
      expect(() => expect(locator).toContainClass("foo|bar")).toThrow("\"expected\\\" argument in toContainClass cannot be a RegExp value");
    });
    test("EX-208 pass with SVGs", () => {
      page.setContent(`<svg class="c1 c2" role="img" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"></svg>`);
      expect(page.locator("svg")).toContainClass("c1");
    });
    test("EX-209 fail", () => {
      page.setContent("<div class=\"bar baz\"></div>");
      const locator = page.locator("div");
      const error = expect(locator).toContainClass("does-not-exist", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toContainClass(expected) failed

    Locator:  locator('div')
    Expected: "does-not-exist"
    Received: "bar baz"
    Timeout:  1000ms

    Call log:
    `);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toContainClass" locator('div') with timeout 1000ms`) != null, true);
    });
    test("EX-210 pass with array", () => {
      page.setContent("<div class=\"foo\"></div><div class=\"hello bar\"></div><div class=\"baz\"></div>");
      const locator = page.locator("div");
      expect(locator).toContainClass(["foo", "hello", "baz"]);
      expect(() => expect(locator).toContainClass(["foo", "hello", "baz"])).toThrow("\"expected\" argument in toContainClass cannot contain RegExp values");
      expect(locator).not.toHaveClass(["not-there", "hello", "baz"]); // Class not there
      expect(locator).not.toHaveClass(["foo", "hello"]); // Length mismatch
    });
    test("EX-211 fail with array", () => {
      page.setContent("<div class=\"foo\"></div><div class=\"bar\"></div><div class=\"bar\"></div>");
      const locator = page.locator("div");
      const error = expect(locator).toContainClass(["foo", "bar", "baz"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(locator).toContainClass(expected) failed`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `Timeout: 1000ms`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toContainClass" locator('div') with timeout 1000ms`) != null, true);
    });
    test("EX-212 pass", () => {
      page.setContent("<title>  Hello     world</title>");
      expect(page).toHaveTitle("Hello  world");
    });
    test("EX-213 fail", () => {
      page.setContent("<title>Bye</title>");
      const error = expect(page).toHaveTitle("Hello", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(page).toHaveTitle(expected) failed

    Expected: "Hello"
    Received: "Bye"
    Timeout:  1000ms

    Call log:
    `);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveTitle" with timeout 1000ms`) != null, true);
    });
    test("EX-214 pass", () => {
      page.goto("data:text/html,<div>A</div>");
      expect(page).toHaveURL("data:text/html,<div>A</div>");
    });
    test("EX-215 fail string", () => {
      page.goto("data:text/html,<div>A</div>");
      const error = expect(page).toHaveURL("wrong", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(page).toHaveURL(expected) failed

    Expected: "wrong"
    Received: "data:text/html,<div>A</div>"
    Timeout:  1000ms

    Call log:
    `);
    });
    test("EX-216 fail with invalid argument", () => {
      page.goto("data:text/html,<div>A</div>");
      // @ts-expect-error
      const error = expect(page).toHaveURL({}).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(page).toHaveURL(expected) failed

    Error: expected value must be a string or regular expression
    Expected has type:  object
    Expected has value: {}
    `);
    });
    test("EX-217 fail with positive predicate", () => {
      page.goto("data:text/html,<div>A</div>");
      const error = expect(page).toHaveURL(_url => false).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(page).toHaveURL(expected) failed

    Expected: predicate to succeed
    Received: "data:text/html,<div>A</div>"
    Timeout:  10000ms
    `);
    });
    test("EX-218 fail with negative predicate", () => {
      page.goto("data:text/html,<div>A</div>");
      const error = expect(page).not.toHaveURL(_url => true).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(page).not.toHaveURL(expected) failed

    Expected: predicate to fail
    Received: "data:text/html,<div>A</div>"
    Timeout:  10000ms
    `);
    });
    test("EX-219 resolve predicate on initial call", () => {
      page.goto("data:text/html,<div>A</div>");
      expect(page).toHaveURL(url => url.href == "data:text/html,<div>A</div>", { timeout: 1000 });
    });
    test("EX-220 resolve predicate after retries", () => {
      page.goto("data:text/html,<div>A</div>");
      const expectPromise = expect(page).toHaveURL(url => url.href == "data:text/html,<div>B</div>", { timeout: 1000 });
      setTimeout(() => page.goto("data:text/html,<div>B</div>"), 500);
      expectPromise;
    });
    test("EX-221 support ignoreCase", () => {
      page.goto("data:text/html,<div>A</div>");
      expect(page).toHaveURL("DATA:teXT/HTml,<div>a</div>", { ignoreCase: true });
    });
    test("EX-222 support URLPattern", () => {
      test(globalThis.URLPattern == null, "URLPattern is not supported in this environment");
      page.goto("data:text/html,<div>A</div>");
      // @ts-ignore URLPattern is not in @types/node yet
      expect(page).toHaveURL(URLPattern.init({ protocol: "data" }));
      // @ts-ignore
      expect(page).not.toHaveURL(URLPattern.init({ protocol: "http" }));
    });
    test("EX-223 pass", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveAttribute("id", "node");
    });
    test("EX-224 should not match missing attribute", () => {
      page.setContent("<div checked id=node>Text content</div>");
      const locator = page.locator("#node");
      {
        const error = expect(locator).toHaveAttribute("disabled", "", { timeout: 1000 }).catch(e => e);
        expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveAttribute(expected) failed

    Locator:  locator('#node')
    Expected: ""
    Received: serializes to the same string
    Timeout:  1000ms

    Call log:
    `);
        expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveAttribute" locator('#node') with timeout 1000ms`) != null, true);
      }
      {
        const error = expect(locator).toHaveAttribute("disabled", ".*", { timeout: 1000 }).catch(e => e);
        expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveAttribute(expected) failed

    Locator: locator('#node')
    Expected pattern: /.*/
    Received string:  ""
    Timeout: 1000ms

    Call log:
    `);
        expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toHaveAttribute" locator('#node') with timeout 1000ms`) != null, true);
      }
      expect(locator).not.toHaveAttribute("disabled", "");
      expect(locator).not.toHaveAttribute("disabled", ".*");
    });
    test("EX-225 should match boolean attribute", () => {
      page.setContent("<div checked id=node>Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveAttribute("checked", "");
      expect(locator).toHaveAttribute("checked", ".*");
      {
        const error = expect(locator).not.toHaveAttribute("checked", "", { timeout: 1000 }).catch(e => e);
        expect(stripAnsi(error.message)).toContain(`expect(locator).not.toHaveAttribute(expected) failed

    Locator:  locator('#node')
    Expected: not ""
    Received: ""
    Timeout:  1000ms

    Call log:
    `);
        expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "not toHaveAttribute" locator('#node') with timeout 1000ms`) != null, true);
      }
      {
        const error = expect(locator).not.toHaveAttribute("checked", ".*", { timeout: 1000 }).catch(e => e);
        expect(stripAnsi(error.message)).toContain(`expect(locator).not.toHaveAttribute(expected) failed

    Locator: locator('#node')
    Expected pattern: not /.*/
    Received string: ""
    Timeout: 1000ms`);
        expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "not toHaveAttribute" locator('#node') with timeout 1000ms`) != null, true);
      }
    });
    test("EX-226 should match attribute without value", () => {
      page.setContent("<div checked id=node>Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveAttribute("id");
      expect(locator).toHaveAttribute("checked");
      expect(locator).not.toHaveAttribute("open");
    });
    test("EX-227 should support boolean attribute with options", () => {
      page.setContent("<div checked id=node>Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveAttribute("id", { timeout: 5000 });
      expect(locator).toHaveAttribute("checked", { timeout: 5000 });
      expect(locator).not.toHaveAttribute("open", { timeout: 5000 });
    });
    test("EX-228 support ignoreCase", () => {
      page.setContent("<div id=NoDe>Text content</div>");
      const locator = page.locator("#NoDe");
      expect(locator).toHaveAttribute("id", "node", { ignoreCase: true });
      expect(locator).not.toHaveAttribute("id", "node");
    });
    test("EX-229 pass", () => {
      page.setContent("<div id=node style=\"color: rgb(255, 0, 0)\">Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveCSS("color", "rgb(255, 0, 0)");
    });
    test("EX-230 should support pseudo element", () => {
      page.setContent(`
        <style>
          #node::before {
            color: rgb(255, 0, 0);
            content: "Text content";
          }
        </style>
        <div id=node></div>
      `);
      const locator = page.locator("#node");
      expect(locator).toHaveCSS("color", "rgb(255, 0, 0)", { pseudo: "before" });
    });
    test("EX-231 custom css properties", () => {
      page.setContent("<div id=node style=\"--custom-color-property:#FF00FF;\">Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveCSS("--custom-color-property", "#FF00FF");
    });
    test("EX-232 pass", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveId("node");
    });
    test("EX-233 should work", () => {
      page.setContent(`
        <div id=big style="height: 10000px;"></div>
        <div id=small>foo</div>
      `);
      expect(page.locator("#big")).toBeInViewport();
      expect(page.locator("#small")).not.toBeInViewport();
      page.locator("#small").scrollIntoViewIfNeeded();
      expect(page.locator("#small")).toBeInViewport();
      expect(page.locator("#small")).toBeInViewport({ ratio: 1 });
    });
    test("EX-234 should respect ratio option", () => {
      test.fixme(isAndroid, "ratio 0.24 is not in viewport for unknown reason");

      page.setContent(`
        <style>body, div, html { padding: 0; margin: 0; }</style>
        <div id=big style="height: 400vh;"></div>
      `);
      expect(page.locator("div")).toBeInViewport();
      expect(page.locator("div")).toBeInViewport({ ratio: 0.1 });
      expect(page.locator("div")).toBeInViewport({ ratio: 0.2 });

      expect(page.locator("div")).toBeInViewport({ ratio: 0.24 });
      // In this test, element's ratio is 0.25.
      expect(page.locator("div")).toBeInViewport({ ratio: 0.25 });
      expect(page.locator("div")).not.toBeInViewport({ ratio: 0.26 });

      expect(page.locator("div")).not.toBeInViewport({ ratio: 0.3 });
      expect(page.locator("div")).not.toBeInViewport({ ratio: 0.7 });
      expect(page.locator("div")).not.toBeInViewport({ ratio: 0.8 });
    });
    test("EX-235 should have good stack", () => {
      let error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect(page.locator("body")).not.toBeInViewport({ timeout: 3000 });
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
        error = e;
      }
      expect_eq(error, true);
      expect_eq("unexpected value \"viewport ratio \\d+".test(error.stack), true);
      const stackFrames = error.stack.split("\n").filter(line => line.trim().startsWith("at "));
      expect_eq(stackFrames.length, 1);
      expect_eq(std.mem.indexOf(u8, stackFrames[0], __filename) != null, true);
    });
    test("EX-236 should report intersection even if fully covered by other element", () => {
      page.setContent(`
        <h1>hello</h1>
        <div style="position: relative; height: 10000px; top: -5000px;></div>
      `);
      expect(page.locator("h1")).toBeInViewport();
    });
    test("EX-237 toHaveCount should not produce logs twice", () => {
      page.setContent("<select><option>One</option></select>");
      const error = expect(page.locator("option")).toHaveCount(2, { timeout: 2000 }).catch(e => e);
      const waitingForMessage = `waiting for locator('option')`;
      expect_eq(std.mem.indexOf(u8, error.message, waitingForMessage) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to 1 element`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `unexpected value "1"`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message.replace(waitingForMessage, "<redacted>"), waitingForMessage) == null, true);
    });
    test("EX-238 toHaveText should not produce logs twice", () => {
      page.setContent("<div>hello</div>");
      const error = expect(page.locator("div")).toHaveText("world", { timeout: 2000 }).catch(e => e);
      const waitingForMessage = `waiting for locator('div')`;
      expect_eq(std.mem.indexOf(u8, error.message, waitingForMessage) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to <div>hello</div>`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `unexpected value "hello"`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message.replace(waitingForMessage, "<redacted>"), waitingForMessage) == null, true);
    });
    test("EX-239 toHaveText that does not match should not produce logs twice", () => {
      page.setContent("<div>hello</div>");
      const error = expect(page.locator("span")).toHaveText("world", { timeout: 2000 }).catch(e => e);
      const waitingForMessage = `waiting for locator('span')`;
      expect_eq(std.mem.indexOf(u8, error.message, waitingForMessage) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, "locator resolved to") == null, true);
      expect_eq(std.mem.indexOf(u8, error.message.replace(waitingForMessage, "<redacted>"), waitingForMessage) == null, true);
    });
    test("EX-240 strict mode violation error format", () => {
      page.setContent("<div>a</div><div>b</div>");
      const error = expect(page.locator("div")).toHaveText("foo").catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator: locator('div')
    Expected: "foo"
    Error: strict mode violation: locator('div') resolved to 2 elements:
        1) <div>a</div> aka getByText('a')
        2) <div>b</div> aka getByText('b')

    Call log:
    `);
    });
    test("EX-241 invalid selector error format", () => {
      page.setContent("<div>a</div><div>b</div>");
      const error = expect(page.locator("##")).toBeVisible().catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toBeVisible() failed

    Locator: ##
    Expected: visible
    Error: Unexpected token "#" while parsing css selector "##". Did you mean to CSS.escape it?

    Call log:
    `);
    });
    test("EX-242 should report expect error details when page closes during expect", () => {
      page.setContent(`<div>hello</div>`);
      const promise = expect(page.locator("div")).toHaveText("world", { timeout: 10000 }).catch((e) => e);
      page.waitForTimeout(1000);
      page.close();
      const error = promise;
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "expect(locator).toHaveText(expected) failed") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Expected: \"world\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Received: \"hello\"") != null, true);
    });
    test("EX-243 should report page close reason when page closes during expect", () => {
      page.setContent(`<div>hello</div>`);
      const promise = expect(page.locator("non-existent")).toHaveText("world", { timeout: 10000 }).catch((e) => e);
      page.close({ reason: "Bye bye." });
      const error = promise;
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "expect(locator).toHaveText(expected) failed") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Bye bye.") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Internal server error") == null, true);
    });
}
