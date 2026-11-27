// Migrated from TypeScript (library/inspector/cli-codegen-java.spec.ts) via cap from -ts

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
    // [ts] return if (channel) `.setChannel("${channel}")\n        .setHeadless(false)` else ".setHeadless(false)";
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-141 should print the correct imports and context options", () => {
    // [ts] const cli = runCLI(["--target=java", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import com.microsoft.playwright.*;
    // [ts] import com.microsoft.playwright.options.*;
    // [ts] import static com.microsoft.playwright.assertions.PlaywrightAssertions.assertThat;
    // [ts] import java.util.*;
    // [ts] public class Example {
    // [ts] public static void main(String[] args) {
    // [ts] try (Playwright playwright = Playwright.create()) {
    // [ts] Browser browser = playwright.${browserName}().launch(new BrowserType.LaunchOptions()
    // [ts] ${launchOptions(channel)});
    // [ts] BrowserContext context = browser.newContext();`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-142 should print the correct context options for custom settings", () => {
    // [ts] const cli = runCLI(["--color-scheme=light", "--target=java", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `BrowserContext context = browser.newContext(new Browser.NewContextOptions()
    // [ts] .setColorScheme(ColorScheme.LIGHT));`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-143 should print the correct context options when using a device", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const cli = runCLI(["--device=Pixel 2", "--target=java", server.EMPTY_PAGE]);
    // [ts] cli.waitFor(`.setViewportSize(411, 731));`);
    // [ts] const expectedResult = `BrowserContext context = browser.newContext(new Browser.NewContextOptions()
    // [ts] .setDeviceScaleFactor(2.625)
    // [ts] .setHasTouch(true)
    // [ts] .setIsMobile(true)
    // [ts] .setUserAgent("Mozilla/5.0 (Linux; Android 8.0; Pixel 2 Build/OPD3.170816.012) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/XXXX Mobile Safari/537.36")
    // [ts] .setViewportSize(411, 731));`;
    // [ts] const text = cli.text();
    // [ts] expect_eq(std.mem.indexOf(u8, text.replace("(.*Chrome\\/)(.*?)( .*)", "$1XXXX$3"), expectedResult) != null, true);
    // [ts] });
    // [ts] test("CG-144 should print the correct context options when using a device and additional options", () => {
    // [ts] test(browserName != "webkit");
    // [ts] const cli = runCLI(["--color-scheme=light", "--device=iPhone 11", "--target=java", server.EMPTY_PAGE]);
    // [ts] cli.waitFor(`.setViewportSize(414, 715));`);
    // [ts] const expectedResult = `BrowserContext context = browser.newContext(new Browser.NewContextOptions()
    // [ts] .setColorScheme(ColorScheme.LIGHT)
    // [ts] .setDeviceScaleFactor(2)
    // [ts] .setHasTouch(true)
    // [ts] .setIsMobile(true)
    // [ts] .setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 12_2 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/XXXX Mobile/15E148 Safari/604.1")
    // [ts] .setViewportSize(414, 715));`;
    // [ts] const text = cli.text();
    // [ts] expect_eq(std.mem.indexOf(u8, text.replace("(.*Version\\/)(.*?)( .*)", "$1XXXX$3"), expectedResult) != null, true);
    // [ts] });
    // [ts] test("CG-145 should print load/save storage_state", () => {
    // [ts] const loadFileName = testInfo.outputPath("load.json");
    // [ts] const saveFileName = testInfo.outputPath("save.json");
    // [ts] fs.promises.writeFile(loadFileName, JSON.stringify({ cookies: [], origins: [] }), "utf8");
    // [ts] const cli = runCLI([`--load-storage=${loadFileName}`, `--save-storage=${saveFileName}`, "--target=java", server.EMPTY_PAGE]);
    // [ts] const expectedResult1 = `BrowserContext context = browser.newContext(new Browser.NewContextOptions()
    // [ts] .setStorageStatePath(Paths.get(${JSON.stringify(loadFileName)})));`;
    // [ts] cli.waitFor(expectedResult1);
    // [ts] const expectedResult2 = `
    // [ts] context.storageState(new BrowserContext.StorageStateOptions().setPath("${saveFileName.replace(/\\/g, '\\\\')}"))`;
    // [ts] cli.waitFor(expectedResult2);
    // [ts] });
    // [ts] test("CG-146 should work with --save-har and --save-har-glob as java-library", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `context.routeFromHAR(Paths.get(${JSON.stringify(harFileName)}), new BrowserContext.RouteFromHAROptions()
    // [ts] .setUrl("**/*.js"));`;
    // [ts] const cli = runCLI(["--target=java", `--save-har=${harFileName}`, "--save-har-glob=**/*.js"]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] test("CG-147 should work with --save-har and --save-har-glob as java-junit", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `page.routeFromHAR(Paths.get(${JSON.stringify(harFileName)}), new Page.RouteFromHAROptions()
    // [ts] .setUrl("**/*.js"));`;
    // [ts] const cli = runCLI(["--target=java-junit", `--save-har=${harFileName}`, "--save-har-glob=**/*.js"]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] test("CG-148 should print the correct imports in junit", () => {
    // [ts] const cli = runCLI(["--target=java-junit", server.EMPTY_PAGE]);
    // [ts] const expectedImportResult = `import com.microsoft.playwright.junit.UsePlaywright;
    // [ts] import com.microsoft.playwright.Page;
    // [ts] import com.microsoft.playwright.options.*;
    // [ts] import org.junit.jupiter.api.*;
    // [ts] import static com.microsoft.playwright.assertions.PlaywrightAssertions.*;`;
    // [ts] cli.waitFor(expectedImportResult);
    // [ts] });
    // [ts] test("CG-149 should print a valid basic program in junit", () => {
    // [ts] const cli = runCLI(["--target=java-junit", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `import com.microsoft.playwright.junit.UsePlaywright;
    // [ts] import com.microsoft.playwright.Page;
    // [ts] import com.microsoft.playwright.options.*;
    // [ts] import org.junit.jupiter.api.*;
    // [ts] import static com.microsoft.playwright.assertions.PlaywrightAssertions.*;
    // [ts] @UsePlaywright
    // [ts] public class TestExample {
    // [ts] @Test
    // [ts] void test(Page page) {
    // [ts] page.navigate("${server.EMPTY_PAGE}");
    // [ts] }
    // [ts] }`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
}
