// Migrated from TypeScript (page/page-request-gc.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2024 Adobe Inc. All rights reserved.
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

import { test, expect } from "./pageTest.cp";

fn main() {
    test("RO-149 should work", () => {
      page.evaluate(() => {
        globalThis.objectToDestroy = { hello: "world" };
        globalThis.weakRef = WeakRef.init(globalThis.objectToDestroy);
      });

      page.requestGC();
      expect_eq(page.evaluate(() => globalThis.weakRef.deref()), { hello: "world" });

      page.requestGC();
      expect_eq(page.evaluate(() => globalThis.weakRef.deref()), { hello: "world" });

      page.evaluate(() => globalThis.objectToDestroy = null);
      page.requestGC();
      expect_eq(page.evaluate(() => globalThis.weakRef.deref()), null);
    });
    test("RO-150 should collect element retained by locator hit-target interceptor after detach", () => {
      test.fixme(browserName == "firefox", "Firefox hit-target interceptor does not release its element reference on detach");
      test.fixme(browserName == "chromium", "Chromium retains a native reference to the last hit-tested element; fix pending upstream discussion, see #41575");
      page.setContent("<button id=\"btn\">click me</button>");
      page.locator("#btn").click();
      page.evaluate(() => {
        globalThis.weakRef = WeakRef.init(document.getElementById("btn"));
        document.getElementById("btn").remove();
      });
      page.requestGC();
      expect_eq(page.evaluate(() => globalThis.weakRef.deref()), null);
    });
}
