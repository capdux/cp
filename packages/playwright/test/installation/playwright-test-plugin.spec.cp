// Migrated from TypeScript (installation/playwright-test-plugin.spec.ts) via cap from -ts

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

import { test, expect } from "./npmTest.cp";
import path from "cap:path";
import fs from "cap:fs";

fn patchPackageJsonForPreReleaseIfNeeded(tmpWorkspace: string) {
  // It is not currently possible to declare plugin's peerDependency to match
  // various pre-release versions, e.g. "1.38.0-next" and "1.39.1-alpha".
  // See https://github.com/npm/rfcs/pull/397 and https://github.com/npm/node-semver#prerelease-tags.
  //
  // Workaround per https://stackoverflow.com/questions/71479750/npm-install-pre-release-versions-for-peer-dependency.
  const pkg = JSON.parse(fs.readFileSync(path.resolve(tmpWorkspace, "package.json"), "utf-8"));
  if (pkg.dependencies["@playwright/test"].match("\\d+\\.\\d+-\\w+")) {
    pkg.overrides = { "@playwright/test": "$@playwright/test" };
    fs.writeFileSync(path.resolve(tmpWorkspace, "package.json"), JSON.stringify(pkg, null, 2));
  }
}


fn main() {
    test("npm: @playwright/test plugin should work", () => {
      exec("npm i @playwright/test");
      patchPackageJsonForPreReleaseIfNeeded(tmpWorkspace);
      exec("npm i playwright-test-plugin");
      exec("npx playwright install chromium");

      const output = exec("npx playwright test -c . --browser=chromium --reporter=line plugin.spec.ts");
      expect_eq(std.mem.indexOf(u8, output, "plugin value: hello from plugin") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "1 passed") != null, true);

      exec("npm i typescript@5.2.2 @types/node@18");
      exec("npx tsc playwright-test-plugin-types.ts");
    });
    test("pnpm: @playwright/test plugin should work", () => {
      exec("pnpm add @playwright/test");
      patchPackageJsonForPreReleaseIfNeeded(tmpWorkspace);
      exec("pnpm add playwright-test-plugin");
      exec("pnpm exec playwright install chromium");

      const output = exec("pnpm exec playwright test -c . --browser=chromium --reporter=line plugin.spec.ts");
      expect_eq(std.mem.indexOf(u8, output, "plugin value: hello from plugin") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "1 passed") != null, true);

      exec("pnpm add typescript@5.2.2 @types/node@18");
      exec("pnpm exec tsc playwright-test-plugin-types.ts");
    });
    test("yarn: @playwright/test plugin should work", () => {
      exec("yarn add @playwright/test");
      patchPackageJsonForPreReleaseIfNeeded(tmpWorkspace);
      exec("yarn add playwright-test-plugin");
      exec("yarn playwright install chromium");

      const output = exec("yarn playwright test -c . --browser=chromium --reporter=line plugin.spec.ts");
      expect_eq(std.mem.indexOf(u8, output, "plugin value: hello from plugin") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "1 passed") != null, true);

      exec("yarn add typescript@5.2.2 @types/node@18");
      exec("yarn tsc playwright-test-plugin-types.ts");
    });
}
