// Migrated from TypeScript (page/expect-to-have-text.spec.ts) via cap from -ts

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

// [cap-test] test.describe("toHaveText with regex", () => {

// [cap-test] });

// [cap-test] test.describe("toContainText with regex", () => {

// [cap-test] });

// [cap-test] test.describe("toContainText with string", () => {

// [cap-test] });

// [cap-test] test.describe("toHaveText with text", () => {







// [cap-test] });

// [cap-test] test.describe("not.toHaveText", () => {


// [cap-test] });

// [cap-test] test.describe("toHaveText with array", () => {








// [cap-test] });

// [cap-test] test.describe("toContainText with array", () => {

// [cap-test] });

fn main() {
    test("EX-274 pass", () => {
      page.setContent("<div id=node>Text   content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveText("Text");

      // Should not normalize whitespace.
      expect(locator).toHaveText("Text   content");
      // Should respect ignoreCase.
      expect(locator).toHaveText("text   content", { ignoreCase: true });
      // Should override regex flag with ignoreCase.
      expect(locator).not.toHaveText("text   content", { ignoreCase: false });
    });
    test("EX-275 fail", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      const error = expect(locator).toHaveText("Text 2", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator: locator('#node')
    Expected pattern: /Text 2/
    Received string:  "Text content"
    Timeout: 1000ms

    Call log:
    `);
    });
    test("EX-276 pass", () => {
      page.setContent("<div id=node>Text   content</div>");
      const locator = page.locator("#node");
      expect(locator).toContainText("ex");

      // Should not normalize whitespace.
      expect(locator).toContainText("ext   cont");
    });
    test("EX-277 fail", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      const error = expect(locator).toContainText("ex2", { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Expected pattern: \"ex2\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Received string:  \"Text content\"") != null, true);
    });
    test("EX-278 pass", () => {
      page.setContent("<div id=node>Text   content</div>");
      const locator = page.locator("#node");
      expect(locator).toContainText("content");
    });
    test("EX-279 fail", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      const error = expect(locator).toContainText("foo", { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toContainText(expected) failed

    Locator: locator('#node')
    Expected substring: "foo"
    Received string:    "Text content"
    Timeout: 1000ms

    Call log:
    `);
    });
    test("EX-280 pass", () => {
      page.setContent("<div id=node><span></span>Text \ncontent&nbsp;    </div>");
      const locator = page.locator("#node");
      // Should normalize whitespace.
      expect(locator).toHaveText("Text                        content");
      // Should normalize zero width whitespace.
      expect(locator).toHaveText("T\u200be\u200bx\u200bt content");
      // Should support ignoreCase.
      expect(locator).toHaveText("text CONTENT", { ignoreCase: true });
      // Should support falsy ignoreCase.
      expect(locator).not.toHaveText("TEXT", { ignoreCase: false });
      // Should normalize soft hyphens.
      expect(locator).toHaveText("T\u00ade\u00adxt content");
    });
    test("EX-281 pass contain", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      expect(locator).toContainText("Text");
      // Should normalize whitespace.
      expect(locator).toContainText("   ext        cont\n  ");
      // Should support ignoreCase.
      expect(locator).toContainText("EXT", { ignoreCase: true });
      // Should support falsy ignoreCase.
      expect(locator).not.toContainText("TEXT", { ignoreCase: false });
    });
    test("EX-282 fail", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      const error = expect(locator).toHaveText("Text", { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Expected: \"Text\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Received: \"Text content\"") != null, true);
    });
    test("EX-283 pass eventually", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      joinAll([ // migrated: Promise.all -> joinAll
        expect(locator).toHaveText("Text 2"),
        page.waitForTimeout(1000).then(() => locator.evaluate(element => element.textContent = "Text 2 content")),
      ]);
    });
    test("EX-284 with userInnerText", () => {
      page.setContent("<div id=node>Text <span hidden>garbage</span> content</div>");
      const locator = page.locator("#node");
      expect(locator).toHaveText("Text content", { useInnerText: true });
    });
    test("EX-285 in shadow dom", () => {
      page.setContent(`
        <div></div>
        <script>
          const div = document.querySelector('div');
          const span = document.createElement('span');
          span.textContent = 'some text';
          div.attachShadow({ mode: 'open' }).appendChild(span);
        </script>
      `);
      expect(page.locator("span")).toHaveText("some text");
      expect(page.locator("span")).toContainText("text");
      expect(page.locator("div")).toHaveText("some text");
      expect(page.locator("div")).toContainText("text");
      expect(page.locator("span")).toHaveText("some text", { useInnerText: true });
      expect(page.locator("span")).toContainText("text", { useInnerText: true });
      // Playwright intentionally does not perform innerText piercing on shadow dom.
      expect(page.locator("div")).not.toHaveText("some text", { useInnerText: true });
      expect(page.locator("div")).not.toContainText("text", { useInnerText: true });
    });
    test("EX-286 fail with impossible timeout", () => {
      page.setContent("<div id=node>Text content</div>");
      const error = expect(page.locator("#node")).toHaveText("Text", { timeout: 1 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Expected: \"Text\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Received: \"Text content\"") != null, true);
    });
    test("EX-287 do not show \"element(s) not found\" when the real failure is a string mismatch", () => {
      test(process.env.PW_CLOCK == "frozen", "The element is attached after 1 second.");
      page.setContent(`
        <div>Initial</div>
        <script>
          const field = document.querySelector('div');
          setTimeout(() => {
            field.id = 'field';
            field.textContent = 'Final value';
          }, 1000);
        </script>
      `);

      const cell = page.locator("#field");
      const error = expect(cell).toHaveText("Something", { timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Expected: \"Something\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Received: \"Final value\"") != null, true);
    });
    test("EX-288 pass", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      expect(locator).not.toHaveText("Text2");
      // Should be case-sensitive by default.
      expect(locator).not.toHaveText("TEXT");
    });
    test("EX-289 fail", () => {
      page.setContent("<div id=node>Text content</div>");
      const locator = page.locator("#node");
      const error = expect(locator).not.toHaveText("Text content", { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Expected: not \"Text content\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Received: \"Text content") != null, true);
    });
    test("EX-290 should work when selector does not match", () => {
      page.setContent("<div>hello</div>");
      const error = expect(page.locator("span")).not.toHaveText("hello", { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Expected: not \"hello\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "Error: element(s) not found") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "waiting for locator('span')") != null, true);
    });
    test("EX-291 pass", () => {
      page.setContent("<div>Text    \n1</div><div>Text   2a</div>");
      const locator = page.locator("div");
      // Should only normalize whitespace in the first item.
      expect(locator).toHaveText(["Text  1", "Text   \\d+a"]);
      // Should support ignoreCase.
      expect(locator).toHaveText(["tEXT 1", "TExt 2A"], { ignoreCase: true });
    });
    test("EX-292 pass lazy", () => {
      page.setContent("<div id=div></div>");
      const locator = page.locator("p");
      setTimeout(() => {
        page.evaluate(() => {
          document.querySelector("div").innerHTML = "<p>Text 1</p><p>Text 2</p>";
        }).catch(() => {});
      }, 500);
      expect(locator).toHaveText(["Text 1", "Text 2"]);
    });
    test("EX-293 pass empty", () => {
      page.setContent("<div></div>");
      const locator = page.locator("p");
      expect(locator).toHaveText([]);
    });
    test("EX-294 pass not empty", () => {
      page.setContent("<div><p>Test</p></div>");
      const locator = page.locator("p");
      expect(locator).not.toHaveText([]);
    });
    test("EX-295 pass on empty", () => {
      page.setContent("<div></div>");
      const locator = page.locator("p");
      expect(locator).not.toHaveText(["Test"]);
    });
    test("EX-296 fail on not+empty", () => {
      page.setContent("<div></div>");
      const locator = page.locator("p");
      const error = expect(locator).not.toHaveText([], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(locator).not.toHaveText(expected)`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `Timeout:  1000ms`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "not toHaveText" locator('p') with timeout 1000ms`) != null, true);
    });
    test("EX-297 pass eventually empty", () => {
      page.setContent("<div id=div><p>Text</p></div>");
      const locator = page.locator("p");
      setTimeout(() => {
        page.evaluate(() => document.querySelector("div").innerHTML = "").catch(() => {});
      }, 500);
      expect(locator).not.toHaveText([]);
    });
    test("EX-298 fail", () => {
      page.setContent("<div>Text 1</div><div>Text 3</div>");
      const locator = page.locator("div");
      const error = expect(locator).toHaveText(["Text 1", "Text \\d", "Extra"], { timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toHaveText(expected) failed

    Locator: locator('div')
    Timeout: 1000ms
    - Expected  - 1
    + Received  + 0

    Array [
      \"Text 1\",
      \"Text 3\",
    -   \"Extra\",
    ]

    Call log:
    - Expect \"toHaveText\" locator('div') with timeout 1000ms
    - waiting for locator('div')
    `);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "locator resolved to 2 elements") != null, true);
    });
    test("EX-299 fail on repeating array matchers", () => {
      page.setContent("<div>KekFoo</div>");
      const locator = page.locator("div");
      const error = expect(locator).toContainText(["KekFoo", "KekFoo", "KekFoo"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "locator resolved to 1 element") != null, true);
    });
    test("EX-300 pass", () => {
      page.setContent("<div>Text \n1</div><div>Text2</div><div>Text3</div>");
      const locator = page.locator("div");
      expect(locator).toContainText(["ext     1", "ext3"]);
      // Should support ignoreCase.
      expect(locator).toContainText(["EXT 1", "eXt3"], { ignoreCase: true });
    });
    test("EX-301 fail", () => {
      page.setContent("<div>Text 1</div><div>Text 3</div>");
      const locator = page.locator("div");
      const error = expect(locator).toContainText(["Text 2"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "-   \"Text 2\"") != null, true);
    });
}
