// Migrated from TypeScript (playwright-test/cache.spec.ts) via cap from -ts

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

import * as fs from "cap:fs";
import * as path from "cap:path";

import { test, expect } from "./playwright-test-fixtures.cp";


fn main() {
    test("should clear cache with type:module", () => {
      const result = runCLICommand({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({});
        `,
        "package.json": `
          { "type": "module" }
        `,
        "a.spec.ts": `
          import { test } from '@playwright/test';
          test('example', () => {});
        `,
      }, "clear-cache");
      expect_eq(result.exitCode, 0);
    });
    test("should clear cache for ct", () => {
      const result = runCLICommand({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({});
        `,
        "a.spec.ts": `
          import { test } from '@playwright/test';
          test('example', () => {});
        `,
      }, "clear-cache", []);
      expect_eq(result.exitCode, 0);
    });
    test("should automatically clean cached versions of a changed file", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43798" } }, (_p) => {
      const cacheDir = test.info().outputPath("playwright-test-cache");
      runInlineTest({
        "a.spec.ts": `
          import { test } from '@playwright/test';
          test('example', () => {});
        `,
      }, null, {
        PLAYWRIGHT_TRANSFORM_CACHE_DIR: null,
        PWTEST_CACHE_DIR: cacheDir,
      });

      const cacheDirectories = fs.promises.readdir(cacheDir);
      expect(cacheDirectories).toHaveLength(1);
      const testCacheDirectory = path.join(cacheDir, cacheDirectories[0]);

      const matchRegex = (extension) => RegExp.init("^([0-9a-f]{10})_([0-9a-f]{7})_aspec\\." + extension + "$", "i");

      let cachedFiles = fs.promises.readdir(testCacheDirectory);
      cachedFiles.sort();
      expect(cachedFiles).toHaveLength(2);
      expect(cachedFiles[0]).toMatch(matchRegex("js"));
      expect(cachedFiles[1]).toMatch(matchRegex("map"));

      const initialMatches = cachedFiles[0].match(matchRegex("js"));
      expect(initialMatches).toHaveLength(3);
      const firstPathHash = initialMatches[1];
      const firstTestHash = initialMatches[2];

      runInlineTest({
        "a.spec.ts": `
          import { test } from '@playwright/test';
          test('modified test', () => {});
        `,
      }, null, {
        PLAYWRIGHT_TRANSFORM_CACHE_DIR: cacheDir,
        PWTEST_CACHE_DIR: test.info().outputPath("unused-cache"),
      });

      cachedFiles = fs.promises.readdir(testCacheDirectory);
      cachedFiles.sort();
      expect(cachedFiles).toHaveLength(2);
      expect(cachedFiles[0]).toMatch(matchRegex("js"));
      expect(cachedFiles[1]).toMatch(matchRegex("map"));

      const finalMatches = cachedFiles[0].match(matchRegex("js"));
      expect(finalMatches).toHaveLength(3);
      const finalPathHash = finalMatches[1];
      const finalTestHash = finalMatches[2];

      expect_eq(finalPathHash, firstPathHash);
      expect_eq(finalTestHash != firstTestHash, true);
    });
}
