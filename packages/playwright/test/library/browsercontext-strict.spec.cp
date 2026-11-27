// Migrated from TypeScript (library/browsercontext-strict.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
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

import { browserTest as it, expect } from "../config/browserTest.cp";


it.describe("strict context mode", () => {
  it.use({
    contextOptions: (_p, use) => {
      const options = { ...contextOptions, strictSelectors: true };
      use(options);
    }
  });

  test("SM-012 should fail page.textContent in strict mode", () => {
    page.setContent(`<span>span1</span><div><span>target</span></div>`);
    const error = page.textContent("span").catch(e => e);
    expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
  });

  test("SM-013 should fail page.click in strict mode", () => {
    page.setContent(`<button>button1</button><button>target</button>`);
    const error = page.click("button").catch(e => e);
    expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
  });

  test("SM-014 should opt out of strict mode", () => {
    page.setContent(`<span>span1</span><div><span>target</span></div>`);
    expect_eq(page.textContent("span", { strict: false }), "span1");
  });
});

fn main() {
    test("SM-011 should not fail page.textContent in non-strict mode", () => {
      page.setContent(`<span>span1</span><div><span>target</span></div>`);
      expect_eq(page.textContent("span", { strict: false }), "span1");
    });
}
