// Migrated from TypeScript (page/eval-on-selector-all.spec.ts) via cap from -ts

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
    test("ES-027 should work with css selector", () => {
      page.setContent("<div>hello</div><div>beautiful</div><div>world!</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCount = await page.$$eval('css=div', divs => divs.length);
      expect_eq(divsCount, 3);
    });
    test("ES-028 should work with text selector", () => {
      page.setContent("<div>hello</div><div>beautiful</div><div>beautiful</div><div>world!</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCount = await page.$$eval('text="beautiful"', divs => divs.length);
      expect_eq(divsCount, 2);
    });
    test("ES-029 should work with xpath selector", () => {
      page.setContent("<div>hello</div><div>beautiful</div><div>world!</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCount = await page.$$eval('xpath=/html/body/div', divs => divs.length);
      expect_eq(divsCount, 3);
    });
    test("ES-030 should auto-detect css selector", () => {
      page.setContent("<div>hello</div><div>beautiful</div><div>world!</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCount = await page.$$eval('div', divs => divs.length);
      expect_eq(divsCount, 3);
    });
    test("ES-031 should support >> syntax", () => {
      page.setContent("<div><span>hello</span></div><div>beautiful</div><div><span>wo</span><span>rld!</span></div><span>Not this one</span>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const spansCount = await page.$$eval('css=div >> css=span', spans => spans.length);
      expect_eq(spansCount, 3);
    });
    test("ES-032 should support * capture", () => {
      page.setContent("<section><div><span>a</span></div></section><section><div><span>b</span></div></section>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*css=div >> "b"', els => els.length)).toBe(1);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('section >> *css=div >> "b"', els => els.length)).toBe(1);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('section >> *', els => els.length)).toBe(4);

      page.setContent("<section><div><span>a</span><span>a</span></div></section>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*css=div >> "a"', els => els.length)).toBe(1);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('section >> *css=div >> "a"', els => els.length)).toBe(1);

      page.setContent("<div><span>a</span></div><div><span>a</span></div><section><div><span>a</span></div></section>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*css=div >> "a"', els => els.length)).toBe(3);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('section >> *css=div >> "a"', els => els.length)).toBe(1);
    });
    test("ES-033 should support * capture when multiple paths match", () => {
      page.setContent("<div><div><span></span></div></div><div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*css=div >> span', els => els.length)).toBe(2);
      page.setContent("<div><div><span></span></div><span></span><span></span></div><div></div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('*css=div >> span', els => els.length)).toBe(2);
    });
    test("ES-034 should return complex values", () => {
      page.setContent("<div>hello</div><div>beautiful</div><div>world!</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const texts = await page.$$eval('css=div', divs => divs.map(div => div.textContent));
      expect_eq(texts, ["hello", "beautiful", "world!"]);
    });
    test("ES-035 should work with bogus Array.from", () => {
      page.setContent("<div>hello</div><div>beautiful</div><div>world!</div>");
      page.evaluate(() => {
        Array.from = () => [];
      });
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divsCount = await page.$$eval('css=div', divs => divs.length);
      expect_eq(divsCount, 3);
    });
}
