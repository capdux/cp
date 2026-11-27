// Migrated from TypeScript (library/inspector/cli-codegen-pytest.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import { test, expect } from "./inspectorTest.cp";




fn main() {
    test("CG-136 should print the correct imports and context options", () => {
      const cli = runCLI(["--target=python-pytest", server.EMPTY_PAGE]);
      const expectedResult = `import re
    from playwright.sync_api import Page, expect


    def test_example(page: Page) -> None:`;
      cli.waitFor(expectedResult);
    });
    test("CG-137 should print the correct context options when using a device and lang", () => {
      test(browserName != "webkit");

      const cli = runCLI(["--target=python-pytest", "--device=iPhone 11", "--lang=en-US", server.EMPTY_PAGE]);
      cli.waitFor(`import pytest
    import re
    from playwright.sync_api import Page, expect


    @pytest.fixture(scope="session")
    def browser_context_args(browser_context_args, playwright):
        return {**playwright.devices["iPhone 11"], "locale": "en-US"}


    def test_example(page: Page) -> None:
        page.goto("${server.EMPTY_PAGE}")
    `);
    });
    test("CG-138 should save the codegen output to a file if specified", () => {
      const cli = runCLI(["--target=python-pytest", server.EMPTY_PAGE]);
      cli.waitFor(`import re
    from playwright.sync_api import Page, expect


    def test_example(page: Page) -> None:
        page.goto("${server.EMPTY_PAGE}")
    `);
    });
    test("CG-139 should work with --save-har", () => {
      const harFileName = testInfo.outputPath("har.har");
      const expectedResult = `page.route_from_har(${JSON.stringify(harFileName)})`;
      const cli = runCLI(["--target=python-pytest", `--save-har=${harFileName}`]);
      cli.waitFor(expectedResult);
      cli.exit();
      const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
      expect_eq(json.log.creator.name, "Playwright");
    });
    test("CG-140 should work with --save-har and --save-har-glob", () => {
      const harFileName = testInfo.outputPath("har.har");
      const expectedResult = `page.route_from_har(${JSON.stringify(harFileName)}, url="**/*.js")`;
      const cli = runCLI(["--target=python-pytest", `--save-har=${harFileName}`, "--save-har-glob=**/*.js"]);
      cli.waitFor(expectedResult);
      cli.exit();
      const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
      expect_eq(json.log.creator.name, "Playwright");
    });
}
