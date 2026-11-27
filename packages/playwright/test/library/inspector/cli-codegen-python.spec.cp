// Migrated from TypeScript (library/inspector/cli-codegen-python.spec.ts) via cap from -ts

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

fn launchOptions(channel: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] return if (channel) `channel="${channel}", headless=False` else "headless=False";
}







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-120 should print the correct imports and context options", () => {
    // [ts] const cli = runCLI(["--target=python", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import re
    // [ts] from playwright.sync_api import Playwright, sync_playwright, expect
    // [ts] def run(playwright: Playwright) -> None:
    // [ts] browser = playwright.${browserName}.launch(${launchOptions(channel)})
    // [ts] context = browser.new_context()`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-121 should print the correct context options for custom settings", () => {
    // [ts] const cli = runCLI(["--color-scheme=light", "--target=python", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import re
    // [ts] from playwright.sync_api import Playwright, sync_playwright, expect
    // [ts] def run(playwright: Playwright) -> None:
    // [ts] browser = playwright.${browserName}.launch(${launchOptions(channel)})
    // [ts] context = browser.new_context(color_scheme="light")`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-122 should print the correct context options when using a device", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const cli = runCLI(["--device=Pixel 2", "--target=python", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import re
    // [ts] from playwright.sync_api import Playwright, sync_playwright, expect
    // [ts] def run(playwright: Playwright) -> None:
    // [ts] browser = playwright.chromium.launch(${launchOptions(channel)})
    // [ts] context = browser.new_context(**playwright.devices["Pixel 2"])`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-123 should print the correct context options when using a device and additional options", () => {
    // [ts] test(browserName != "webkit");
    // [ts] const cli = runCLI(["--color-scheme=light", "--device=iPhone 11", "--target=python", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import re
    // [ts] from playwright.sync_api import Playwright, sync_playwright, expect
    // [ts] def run(playwright: Playwright) -> None:
    // [ts] browser = playwright.webkit.launch(${launchOptions(channel)})
    // [ts] context = browser.new_context(**playwright.devices["iPhone 11"], color_scheme="light")`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-124 should save the codegen output to a file if specified", () => {
    // [ts] const cli = runCLI(["--target=python", server.EMPTY_PAGE]);
    // [ts] cli.waitFor(`import re
    // [ts] from playwright.sync_api import Playwright, sync_playwright, expect
    // [ts] def run(playwright: Playwright) -> None:
    // [ts] browser = playwright.${browserName}.launch(${launchOptions(channel)})
    // [ts] context = browser.new_context()
    // [ts] page = context.new_page()
    // [ts] page.goto("${server.EMPTY_PAGE}")
    // [ts] # ---------------------
    // [ts] context.close()
    // [ts] browser.close()
    // [ts] with sync_playwright() as playwright:
    // [ts] run(playwright)
    // [ts] `);
    // [ts] });
    // [ts] test("CG-125 should print load/save storage_state", () => {
    // [ts] const loadFileName = testInfo.outputPath("load.json");
    // [ts] const saveFileName = testInfo.outputPath("save.json");
    // [ts] fs.promises.writeFile(loadFileName, JSON.stringify({ cookies: [], origins: [] }), "utf8");
    // [ts] const cli = runCLI([`--load-storage=${loadFileName}`, `--save-storage=${saveFileName}`, "--target=python", server.EMPTY_PAGE]);
    // [ts] const expectedResult1 = `import re
    // [ts] from playwright.sync_api import Playwright, sync_playwright, expect
    // [ts] def run(playwright: Playwright) -> None:
    // [ts] browser = playwright.${browserName}.launch(${launchOptions(channel)})
    // [ts] context = browser.new_context(storage_state="${loadFileName.replace(/\\/g, '\\\\')}")`;
    // [ts] cli.waitFor(expectedResult1);
    // [ts] const expectedResult2 = `
    // [ts] # ---------------------
    // [ts] context.storage_state(path="${saveFileName.replace(/\\/g, '\\\\')}")
    // [ts] context.close()
    // [ts] browser.close()
    // [ts] with sync_playwright() as playwright:
    // [ts] run(playwright)
    // [ts] `;
    // [ts] cli.waitFor(expectedResult2);
    // [ts] });
    // [ts] test("CG-126 should work with --save-har", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `context.route_from_har(${JSON.stringify(harFileName)})`;
    // [ts] const cli = runCLI(["--target=python-async", `--save-har=${harFileName}`]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] test("CG-127 should work with --save-har and --save-har-glob", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `context.route_from_har(${JSON.stringify(harFileName)}, url="**/*.js")`;
    // [ts] const cli = runCLI(["--target=python-async", `--save-har=${harFileName}`, "--save-har-glob=**/*.js"]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
}
