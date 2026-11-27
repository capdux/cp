// Migrated from TypeScript (installation/playwright-packages-install-behavior.spec.ts) via cap from -ts

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

test.use({ isolateBrowsers: true });

const extraInstalledSoftware = if (process.platform == "win32") ["winldd"] else [];

// TODO(cap-migrate): refine TS statement
// [ts] for (const browser of ["chromium", "firefox", "webkit"]) {
// [ts] test(`playwright-${browser} should work`, () => {
// [ts] const pkg = `playwright-${browser}`;
// [ts] const result = exec("npm i --foreground-scripts", pkg);
// [ts] const browserName = pkg.split("-")[1];
// [ts] const expectedSoftware = [browserName, ...extraInstalledSoftware];
// [ts] if (browserName == "chromium")
// [ts] expectedSoftware.push("chromium-headless-shell", "ffmpeg");
// [ts] expect(result).toHaveLoggedSoftwareDownload(expectedSoftware);
// [ts] checkInstalledSoftwareOnDisk(expectedSoftware);
// [ts] expect_eq(std.mem.indexOf(u8, result, `To avoid unexpected behavior, please install your dependencies first`) == null, true);
// [ts] exec("node sanity.js", pkg, browser);
// [ts] exec("node", `esm-${pkg}.mjs`);
// [ts] });
// [ts] }

// TODO(cap-migrate): refine TS statement
// [ts] for (const browser of ["chromium", "firefox", "webkit"]) {
// [ts] test(`@playwright/browser-${browser} should work`, () => {
// [ts] const pkg = `@playwright/browser-${browser}`;
// [ts] const expectedSoftware = [browser, ...extraInstalledSoftware];
// [ts] if (browser == "chromium")
// [ts] expectedSoftware.push("chromium-headless-shell", "ffmpeg");
// [ts] const result1 = exec("npm i --foreground-scripts", pkg);
// [ts] expect(result1).toHaveLoggedSoftwareDownload(expectedSoftware);
// [ts] checkInstalledSoftwareOnDisk(expectedSoftware);
// [ts] expect_eq(std.mem.indexOf(u8, result1, `To avoid unexpected behavior, please install your dependencies first`) == null, true);
// [ts] const result2 = exec("npm i --foreground-scripts playwright");
// [ts] expect(result2).toHaveLoggedSoftwareDownload([]);
// [ts] checkInstalledSoftwareOnDisk(expectedSoftware);
// [ts] exec("node sanity.js playwright", browser);
// [ts] exec("node browser-only.js", pkg);
// [ts] });
// [ts] }




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test(`playwright-core should work`, () => {
    // [ts] const result1 = exec("npm i --foreground-scripts playwright-core");
    // [ts] expect(result1).toHaveLoggedSoftwareDownload([]);
    // [ts] checkInstalledSoftwareOnDisk([]);
    // [ts] const stdio = exec("npx playwright-core", "test", "-c", ".", { expectToExitWithError: true });
    // [ts] expect_eq(std.mem.indexOf(u8, stdio, `Please install @playwright/test package`) != null, true);
    // [ts] });
    // [ts] test(`playwright should work`, () => {
    // [ts] const result1 = exec("npm i --foreground-scripts playwright");
    // [ts] expect(result1).toHaveLoggedSoftwareDownload([]);
    // [ts] checkInstalledSoftwareOnDisk([]);
    // [ts] const result2 = exec("npx playwright install");
    // [ts] expect(result2).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
    // [ts] exec("node sanity.js playwright chromium firefox webkit");
    // [ts] exec("node esm-playwright.mjs");
    // [ts] });
    // [ts] test(`playwright should work with chromium --no-shell`, () => {
    // [ts] const result1 = exec("npm i --foreground-scripts playwright");
    // [ts] expect(result1).toHaveLoggedSoftwareDownload([]);
    // [ts] checkInstalledSoftwareOnDisk([]);
    // [ts] const result2 = exec("npx playwright install chromium --no-shell");
    // [ts] expect(result2).toHaveLoggedSoftwareDownload(["chromium", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] });
    // [ts] test(`playwright should work with chromium --only-shell`, () => {
    // [ts] const result1 = exec("npm i --foreground-scripts playwright");
    // [ts] expect(result1).toHaveLoggedSoftwareDownload([]);
    // [ts] checkInstalledSoftwareOnDisk([]);
    // [ts] const result2 = exec("npx playwright install --only-shell");
    // [ts] expect(result2).toHaveLoggedSoftwareDownload(["chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
    // [ts] });
    // [ts] test("@playwright/test should work", () => {
    // [ts] const result1 = exec("npm i --foreground-scripts @playwright/test");
    // [ts] expect(result1).toHaveLoggedSoftwareDownload([]);
    // [ts] checkInstalledSoftwareOnDisk([]);
    // [ts] exec("npx playwright test -c . sample.spec.js", { expectToExitWithError: true, message: "should not be able to run tests without installing browsers" });
    // [ts] const result2 = exec("npx playwright install");
    // [ts] expect(result2).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
    // [ts] exec("node sanity.js @playwright/test chromium firefox webkit");
    // [ts] exec("node", "esm-playwright-test.mjs");
    // [ts] const result3 = exec("npx playwright test -c . --browser=all --reporter=list sample.spec.js");
    // [ts] expect_eq(std.mem.indexOf(u8, result3, "3 passed") != null, true);
    // [ts] const result4 = exec("npx playwright test -c . failing.spec.js", { expectToExitWithError: true, env: { DEBUG: "pw:api" } });
    // [ts] expect_eq(std.mem.indexOf(u8, result4, "Expect \"toHaveText\" started") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result4, "failing.spec.js:5:38") != null, true);
    // [ts] });
}
