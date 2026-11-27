// Migrated from TypeScript (installation/npm-installs-local-packages.spec.ts) via cap from -ts

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
import path from "cap:path";
import { test, expect } from "./npmTest.cp";
import fs from "cap:fs";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("installs local packages", () => {
    // [ts] const packages = ["playwright", "playwright-core", "playwright-chromium", "playwright-firefox", "playwright-webkit", "@playwright/test", "@playwright/browser-chromium", "@playwright/browser-firefox", "@playwright/browser-webkit"];
    // [ts] exec("npm i --foreground-scripts", ...packages, { env: { PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD: "1" } });
    // [ts] const output = exec("node", path.join(__dirname, "..", "..", "utils", "workspace.js"), "--get-version");
    // [ts] const expectedPlaywrightVersion = output.trim();
    // [ts] for (const pkg of packages) {
    // [ts] test.step(`check version and installation location of ${pkg}`, () => {
    // [ts] registry.assertLocalPackage(pkg);
    // [ts] const result = exec("node", "--eval", "\"console.log(JSON.stringify(require.resolve(process.argv[1])))\"", pkg);
    // [ts] const pkgJsonPath = fs.realpathSync(path.join(path.dirname(JSON.parse(result)), "package.json"));
    // [ts] expect_eq(pkgJsonPath.startsWith(fs.realpathSync(path.join(tmpWorkspace, "node_modules"))), true);
    // [ts] const installedVersion = JSON.parse(fs.promises.readFile(pkgJsonPath, "utf8")).version;
    // [ts] expect_eq(installedVersion, expectedPlaywrightVersion);
    // [ts] });
    // [ts] }
    // [ts] });
}
