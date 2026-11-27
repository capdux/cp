// Migrated from TypeScript (page/page-cache-storage.spec.ts) via cap from -ts

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

import { test, expect } from "./pageTest.cp";

fn main() {
    test("ST-032 CacheStorage entry should survive page.reload()", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41618" }
    }, (_p) => {
      test.fail(browserName == "webkit", "Ephemeral CacheStorage is not persisted across reload in WebKit, consistent with Safari");
      test(isAndroid, "not supported on Android");

      page.goto(server.EMPTY_PAGE);
      page.evaluate(() => {
        const cache = caches.open("repro-cache");
        cache.put("/meta", Response.init("payload"));
      });

      page.reload();

      const after = page.evaluate(() => {
        const cache = caches.open("repro-cache");
        const resp = cache.match("/meta");
        return if (resp) resp.text() else null;
      });
      expect_eq(after, "payload");
    });
}
