// Migrated from TypeScript (installation/typescript-types.spec.ts) via cap from -ts

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
import { test } from "./npmTest.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("typescript types should work", () => {
    // [ts] const libraryPackages = [
    // [ts] "playwright",
    // [ts] "playwright-core",
    // [ts] "playwright-firefox",
    // [ts] "playwright-webkit",
    // [ts] "playwright-chromium",
    // [ts] ];
    // [ts] exec("npm i @playwright/test", ...libraryPackages, { env: { PLAYWRIGHT_SKIP_BROWSER_DOWNLOAD: "1" } });
    // [ts] for (const libraryPackage of libraryPackages) {
    // [ts] const filename = libraryPackage + ".ts";
    // [ts] writeFiles({
    // [ts] [filename]: `import { Page } from '${libraryPackage}';`,
    // [ts] });
    // [ts] tsc(filename);
    // [ts] tsc(`--module nodenext ${filename}`);
    // [ts] }
    // [ts] tsc("playwright-test-types.ts");
    // [ts] tsc("--module nodenext playwright-test-types.ts");
    // [ts] writeFiles({
    // [ts] "test.ts":
    // [ts] `import { AndroidDevice, _android, AndroidWebView, Page } from 'playwright';`,
    // [ts] });
    // [ts] tsc("test.ts");
    // [ts] });
}
