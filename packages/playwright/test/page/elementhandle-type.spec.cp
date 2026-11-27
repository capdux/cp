// Migrated from TypeScript (page/elementhandle-type.spec.ts) via cap from -ts

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
    test("EH-107 should work", () => {
      page.setContent(`<input type='text' />`);
      page.type("input", "hello");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('hello');
    });
    test("EH-108 should not select existing value", () => {
      page.setContent(`<input type='text' value='hello' />`);
      page.type("input", "world");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('worldhello');
    });
    test("EH-109 should reset selection when not focused", () => {
      page.setContent(`<input type='text' value='hello' /><div tabIndex=2>text</div>`);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
        // [ts-skip] input.selectionStart = 2;
        // [ts-skip] input.selectionEnd = 4;
        // [ts-skip] document.querySelector('div').focus();
      // [ts-skip] });
      page.type("input", "world");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('worldhello');
    });
    test("EH-110 should not modify selection when focused", () => {
      page.setContent(`<input type='text' value='hello' />`);
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('input', input => {
        // [ts-skip] input.focus();
        // [ts-skip] input.selectionStart = 2;
        // [ts-skip] input.selectionEnd = 4;
      // [ts-skip] });
      page.type("input", "world");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('heworldo');
    });
    test("EH-111 should work with number input", () => {
      it.fail(browserName == "webkit", "Started failing after https://github.com/WebKit/WebKit/commit/c92a2aea185d63b5e9998608a9c0321a461c496c");
      page.setContent(`<input type='number' value=2 />`);
      page.type("input", "13");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input', input => input.value)).toBe('132');
    });
}
