// Migrated from TypeScript (installation/playwright-test-package-managers.spec.ts) via cap from -ts

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





fn main() {
    test("npm: @playwright/test should work", () => {
      exec("npm i @playwright/test");
      exec("npx playwright install");
      exec("npx playwright test -c . --browser=all --reporter=list,json sample.spec.js", { env: {  PLAYWRIGHT_JSON_OUTPUT_NAME: "report.json" } });
      exec("node read-json-report.js", path.join(tmpWorkspace, "report.json"));
      exec("node sanity.js @playwright/test chromium firefox webkit");
      exec("node", "esm-playwright-test.mjs");
    });
    test("npm: playwright + @playwright/test should work", () => {
      exec("npm i playwright");
      exec("npm i @playwright/test");
      exec("npx playwright install");
      exec("npx playwright test -c . --browser=all --reporter=list,json sample.spec.js", { env: {  PLAYWRIGHT_JSON_OUTPUT_NAME: "report.json" } });
      exec("node read-json-report.js", path.join(tmpWorkspace, "report.json"));
      exec("node sanity.js @playwright/test chromium firefox webkit");
      exec("node", "esm-playwright-test.mjs");
    });
    test("npm: @playwright/test + playwright-core should work", () => {
      exec("npm i @playwright/test");
      exec("npm i playwright-core");
      exec("npx playwright install");
      exec("npx playwright test -c . --browser=all --reporter=list,json sample.spec.js", { env: {  PLAYWRIGHT_JSON_OUTPUT_NAME: "report.json" } });
      exec("node read-json-report.js", path.join(tmpWorkspace, "report.json"));
      exec("node sanity.js @playwright/test chromium firefox webkit");
      exec("node", "esm-playwright-test.mjs");
    });
    test("npm: @playwright/test should install playwright-core bin", () => {
      exec("npm i @playwright/test");
      const result = exec("npx playwright-core --version");
      expect_eq(std.mem.indexOf(u8, result, "Version 1.") != null, true);
    });
    test("yarn: @playwright/test should work", () => {
      exec("yarn add @playwright/test");
      exec("yarn playwright install");
      exec("yarn playwright test -c . --browser=all --reporter=list,json sample.spec.js", { env: {  PLAYWRIGHT_JSON_OUTPUT_NAME: "report.json" } });
      exec("node read-json-report.js", path.join(tmpWorkspace, "report.json"));
      exec("node sanity.js @playwright/test chromium firefox webkit");
      exec("node", "esm-playwright-test.mjs");
    });
    test("pnpm: @playwright/test should work", () => {
      exec("pnpm add @playwright/test");
      exec("pnpm exec playwright install");
      exec("pnpm exec playwright test -c . --browser=all --reporter=list,json sample.spec.js", { env: {  PLAYWRIGHT_JSON_OUTPUT_NAME: "report.json" } });
      exec("node read-json-report.js", path.join(tmpWorkspace, "report.json"));
      exec("node sanity.js @playwright/test chromium firefox webkit");
      exec("node", "esm-playwright-test.mjs");
    });
}
