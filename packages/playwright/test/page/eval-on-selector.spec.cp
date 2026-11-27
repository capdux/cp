// Migrated from TypeScript (page/eval-on-selector.spec.ts) via cap from -ts

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
    // [ts] test("ES-001 should work with css selector", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('css=section', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-002 should work with id selector", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('id=testAttribute', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-003 should work with data-test selector", () => {
    // [ts] page.setContent("<section data-test=foo id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('data-test=foo', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-004 should work with data-testid selector", () => {
    // [ts] page.setContent("<section data-testid=foo id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('data-testid=foo', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-005 should work with data-test-id selector", () => {
    // [ts] page.setContent("<section data-test-id=foo id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('data-test-id=foo', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-006 should work with text selector in quotes", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('text="43543"', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-007 should work with xpath selector", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('xpath=/html/body/section', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-008 should work with text selector", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('text=43543', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-009 should auto-detect css selector", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('section', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-010 should auto-detect css selector with attributes", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('section[id="testAttribute"]', e => e.id);
    // [ts] expect_eq(idAttribute, "testAttribute");
    // [ts] });
    // [ts] test("ES-011 should auto-detect nested selectors", () => {
    // [ts] page.setContent("<div foo=bar><section>43543<span>Hello<div id=target></div></span></section></div>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('div[foo=bar] > section >> "Hello" >> div', e => e.id);
    // [ts] expect_eq(idAttribute, "target");
    // [ts] });
    // [ts] test("ES-012 should accept arguments", () => {
    // [ts] page.setContent("<section>hello</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const text = await page.$eval('section', (e, suffix) => e.textContent + suffix, ' world!');
    // [ts] expect_eq(text, "hello world!");
    // [ts] });
    // [ts] test("ES-013 should accept ElementHandles as arguments", () => {
    // [ts] page.setContent("<section>hello</section><div> world</div>");
    // [ts] const divHandle = page.$("div");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const text = await page.$eval('section', (e, div) => e.textContent + div.textContent, divHandle);
    // [ts] expect_eq(text, "hello world");
    // [ts] });
    // [ts] test("ES-014 should throw error if no element is found", () => {
    // [ts] let error = null;
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('section', e => e.id).catch(e => error = e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Failed to find element matching selector \"section\"") != null, true);
    // [ts] });
    // [ts] test("ES-015 should support >> syntax", () => {
    // [ts] page.setContent("<section><div>hello</div></section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const text = await page.$eval('css=section >> css=div', (e, suffix) => e.textContent + suffix, ' world!');
    // [ts] expect_eq(text, "hello world!");
    // [ts] });
    // [ts] test("ES-016 should support >> syntax with different engines", () => {
    // [ts] page.setContent("<section><div><span>hello</span></div></section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const text = await page.$eval('xpath=/html/body/section >> css=div >> text="hello"', (e, suffix) => e.textContent + suffix, ' world!');
    // [ts] expect_eq(text, "hello world!");
    // [ts] });
    // [ts] test("ES-017 should support spaces with >> syntax", () => {
    // [ts] page.goto(server.PREFIX + "/deep-shadow.html");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const text = await page.$eval(' css = div >>css=div>>css   = span  ', e => e.textContent);
    // [ts] expect_eq(text, "Hello from root2");
    // [ts] });
    // [ts] test("ES-018 should not stop at first failure with >> syntax", () => {
    // [ts] page.setContent("<div><span>Next</span><button>Previous</button><button>Next</button></div>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const html = await page.$eval('button >> "Next"', e => e.outerHTML);
    // [ts] expect_eq(html, "<button>Next</button>");
    // [ts] });
    // [ts] test("ES-019 should support * capture", () => {
    // [ts] page.setContent("<section><div><span>a</span></div></section><section><div><span>b</span></div></section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('*css=div >> "b"', e => e.outerHTML)).toBe('<div><span>b</span></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('section >> *css=div >> "b"', e => e.outerHTML)).toBe('<div><span>b</span></div>');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('css=div >> *text="b"', e => e.outerHTML)).toBe('<span>b</span>');
    // [ts] expect_eq(page.$("*"), true);
    // [ts] });
    // [ts] test("ES-020 should throw on multiple * captures", () => {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const error = await page.$eval('*css=div >> *css=span', e => e.outerHTML).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Only one of the selectors can capture using * modifier") != null, true);
    // [ts] });
    // [ts] test("ES-021 should throw on malformed * capture", () => {
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const error = await page.$eval('*=div', e => e.outerHTML).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unknown engine \"\" while parsing selector *=div") != null, true);
    // [ts] });
    // [ts] test("ES-022 should work with spaces in css attributes", () => {
    // [ts] page.setContent("<div><input placeholder=\"Select date\"></div>");
    // [ts] expect_eq(page.waitForSelector(`[placeholder="Select date"]`), true);
    // [ts] expect_eq(page.waitForSelector(`[placeholder='Select date']`), true);
    // [ts] expect_eq(page.waitForSelector(`input[placeholder="Select date"]`), true);
    // [ts] expect_eq(page.waitForSelector(`input[placeholder='Select date']`), true);
    // [ts] expect_eq(page.$(`[placeholder="Select date"]`), true);
    // [ts] expect_eq(page.$(`[placeholder='Select date']`), true);
    // [ts] expect_eq(page.$(`input[placeholder="Select date"]`), true);
    // [ts] expect_eq(page.$(`input[placeholder='Select date']`), true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`[placeholder="Select date"]`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`[placeholder='Select date']`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`input[placeholder="Select date"]`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`input[placeholder='Select date']`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=[placeholder="Select date"]`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=[placeholder='Select date']`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=input[placeholder="Select date"]`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`css=input[placeholder='Select date']`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> [placeholder="Select date"]`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval(`div >> [placeholder='Select date']`, e => e.outerHTML)).toBe('<input placeholder="Select date">');
    // [ts] });
    // [ts] test("ES-023 should work with quotes in css attributes", () => {
    // [ts] page.setContent("<div><input placeholder=\"Select&quot;date\"></div>");
    // [ts] expect_eq(page.$(`[placeholder="Select\\"date"]`), true);
    // [ts] expect_eq(page.$(`[placeholder='Select"date']`), true);
    // [ts] page.setContent("<div><input placeholder=\"Select &quot; date\"></div>");
    // [ts] expect_eq(page.$(`[placeholder="Select \\" date"]`), true);
    // [ts] expect_eq(page.$(`[placeholder='Select " date']`), true);
    // [ts] page.setContent("<div><input placeholder=\"Select&apos;date\"></div>");
    // [ts] expect_eq(page.$(`[placeholder="Select'date"]`), true);
    // [ts] expect_eq(page.$(`[placeholder='Select\\'date']`), true);
    // [ts] page.setContent("<div><input placeholder=\"Select &apos; date\"></div>");
    // [ts] expect_eq(page.$(`[placeholder="Select ' date"]`), true);
    // [ts] expect_eq(page.$(`[placeholder='Select \\' date']`), true);
    // [ts] });
    // [ts] test("ES-024 should work with spaces in css attributes when missing", () => {
    // [ts] const inputPromise = page.waitForSelector(`[placeholder="Select date"]`);
    // [ts] expect_eq(page.$(`[placeholder="Select date"]`), null);
    // [ts] page.setContent("<div><input placeholder=\"Select date\"></div>");
    // [ts] inputPromise;
    // [ts] });
    // [ts] test("ES-025 should work with quotes in css attributes when missing", () => {
    // [ts] const inputPromise = page.waitForSelector(`[placeholder="Select\\"date"]`);
    // [ts] expect_eq(page.$(`[placeholder="Select\\"date"]`), null);
    // [ts] page.setContent("<div><input placeholder=\"Select&quot;date\"></div>");
    // [ts] inputPromise;
    // [ts] });
    // [ts] test("ES-026 should return complex values", () => {
    // [ts] page.setContent("<section id=\"testAttribute\">43543</section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const idAttribute = await page.$eval('css=section', e => [{ id: e.id }]);
    // [ts] expect_eq(idAttribute, [{ id: "testAttribute" }]);
    // [ts] });
}
