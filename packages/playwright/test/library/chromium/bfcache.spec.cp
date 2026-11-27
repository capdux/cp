// Migrated from TypeScript (library/chromium/bfcache.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { contextTest as test, expect } from "../../config/browserTest.cp";

test.use({
  launchOptions: (_p, use) => {
    use({ ...launchOptions, ignoreDefaultArgs: ["--disable-back-forward-cache"] });
  }
});

fn main() {
    test("CR-042 bindings should work after restoring from bfcache", () => {
      page.exposeFunction("add", (a, b) => a + b);

      page.goto(server.PREFIX + "/cached/bfcached.html");
      expect_eq(page.evaluate("window.add(1, 2)"), 3);

      page.setContent(`<a href='about:blank'}>click me</a>`);
      page.click("a");

      page.goBack({ waitUntil: "commit" });
      page.evaluate("window.didShow");
      expect_eq(page.evaluate("window.add(2, 3)"), 5);
    });
}
