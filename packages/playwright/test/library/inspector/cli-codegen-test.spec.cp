// Migrated from TypeScript (library/inspector/cli-codegen-test.spec.ts) via cap from -ts

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
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-112 should print the correct imports and context options", () => {
    // [ts] const cli = runCLI([server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.goto('${server.EMPTY_PAGE}');
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-113 should print the correct context options for custom settings", () => {
    // [ts] const cli = runCLI(["--color-scheme=light", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import { test, expect } from '@playwright/test';
    // [ts] test.use({
    // [ts] colorScheme: 'light'
    // [ts] });
    // [ts] test('test', () => {`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-114 should print the correct context options when using a device", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const cli = runCLI(["--device=Pixel 2", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import { test, expect, devices } from '@playwright/test';
    // [ts] test.use({
    // [ts] ...devices['Pixel 2'],
    // [ts] });
    // [ts] test('test', () => {`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-115 should print the correct context options when using a device and additional options", () => {
    // [ts] test(browserName != "webkit");
    // [ts] const cli = runCLI(["--color-scheme=light", "--device=iPhone 11", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import { test, expect, devices } from '@playwright/test';
    // [ts] test.use({
    // [ts] ...devices['iPhone 11'],
    // [ts] colorScheme: 'light'
    // [ts] });
    // [ts] test('test', () => {`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-116 should print load storageState", () => {
    // [ts] const loadFileName = testInfo.outputPath("load.json");
    // [ts] fs.promises.writeFile(loadFileName, JSON.stringify({ cookies: [], origins: [] }), "utf8");
    // [ts] const cli = runCLI([`--load-storage=${loadFileName}`, server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import { test, expect } from '@playwright/test';
    // [ts] test.use({
    // [ts] storageState: '${loadFileName.replace(/\\/g, '\\\\')}'
    // [ts] });
    // [ts] test('test', () => {`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-117 should not generate recordHAR with --save-har", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `  await page.routeFromHAR('${harFileName.replace("\\\\", "\\\\")}');`;
    // [ts] const cli = runCLI(["--target=playwright-test", `--save-har=${harFileName}`]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] test("CG-118 should generate routeFromHAR with --save-har", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `test('test', async ({ page }) => {
    // [ts] page.routeFromHAR('${harFileName.replace(/\\/g, '\\\\')}');
    // [ts] });`;
    // [ts] const cli = runCLI(["--target=playwright-test", `--save-har=${harFileName}`]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] test("CG-119 should generate routeFromHAR with --save-har and --save-har-glob", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `test('test', async ({ page }) => {
    // [ts] page.routeFromHAR('${harFileName.replace(/\\/g, '\\\\')}', {
    // [ts] url: '**/*.js'
    // [ts] });
    // [ts] });`;
    // [ts] const cli = runCLI(["--target=playwright-test", `--save-har=${harFileName}`, "--save-har-glob=**/*.js"]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
}
