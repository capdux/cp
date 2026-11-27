// Migrated from TypeScript (library/inspector/cli-codegen-csharp.spec.ts) via cap from -ts

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
    // [ts] return if (channel) `Channel = "${channel}",\n    Headless = false,` else `Headless = false,`;
}

fn capitalize(browserName: string): string {
  return browserName[0].toUpperCase() + browserName.slice(1);
}



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-150 should print the correct imports and context options", () => {
    // [ts] const cli = runCLI(["--target=csharp", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `using Microsoft.Playwright;
    // [ts] using System;
    // [ts] using System.Threading.Tasks;
    // [ts] using var playwright = Playwright.CreateAsync();
    // [ts] using var browser = playwright.${capitalize(browserName)}.LaunchAsync(new()
    // [ts] {
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] var context = browser.NewContextAsync();`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-151 should print the correct context options for custom settings", () => {
    // [ts] proxyServer.forwardTo(server.PORT);
    // [ts] const cli = runCLI([
    // [ts] "--color-scheme=dark",
    // [ts] "--geolocation=37.819722,-122.478611",
    // [ts] "--lang=es",
    // [ts] "--proxy-server=" + proxyServer.HOST,
    // [ts] "--timezone=Europe/Rome",
    // [ts] "--user-agent=hardkodemium",
    // [ts] "--viewport-size=1280,720",
    // [ts] "--target=csharp",
    // [ts] server.EMPTY_PAGE]);
    // [ts] const expectedResult = `
    // [ts] using var playwright = Playwright.CreateAsync();
    // [ts] using var browser = playwright.${capitalize(browserName)}.LaunchAsync(new()
    // [ts] {
    // [ts] ${launchOptions(channel)}
    // [ts] Proxy = new()
    // [ts] {
    // [ts] Server = "${proxyServer.HOST}",
    // [ts] },
    // [ts] });
    // [ts] var context = browser.NewContextAsync(new()
    // [ts] {
    // [ts] ColorScheme = ColorScheme.Dark,
    // [ts] Geolocation = new()
    // [ts] {
    // [ts] Latitude = 37.819722m,
    // [ts] Longitude = -122.478611m,
    // [ts] },
    // [ts] Locale = "es",
    // [ts] Permissions = new[] { "geolocation" },
    // [ts] TimezoneId = "Europe/Rome",
    // [ts] UserAgent = "hardkodemium",
    // [ts] ViewportSize = new()
    // [ts] {
    // [ts] Height = 720,
    // [ts] Width = 1280,
    // [ts] },
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-152 should print the correct context options when using a device", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const cli = runCLI(["--device=Pixel 2", "--target=csharp", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `
    // [ts] using var playwright = Playwright.CreateAsync();
    // [ts] using var browser = playwright.${capitalize(browserName)}.LaunchAsync(new()
    // [ts] {
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] var context = browser.NewContextAsync(playwright.Devices["Pixel 2"]);`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-153 should print the correct context options when using a device and additional options", () => {
    // [ts] test(browserName != "webkit");
    // [ts] proxyServer.forwardTo(server.PORT);
    // [ts] const cli = runCLI([
    // [ts] "--device=iPhone 11",
    // [ts] "--color-scheme=dark",
    // [ts] "--geolocation=37.819722,-122.478611",
    // [ts] "--lang=es",
    // [ts] "--proxy-server=" + proxyServer.HOST,
    // [ts] "--timezone=Europe/Rome",
    // [ts] "--user-agent=hardkodemium",
    // [ts] "--viewport-size=1280,720",
    // [ts] "--target=csharp",
    // [ts] server.EMPTY_PAGE]);
    // [ts] const expectedResult = `
    // [ts] using var playwright = Playwright.CreateAsync();
    // [ts] using var browser = playwright.${capitalize(browserName)}.LaunchAsync(new()
    // [ts] {
    // [ts] ${launchOptions(channel)}
    // [ts] Proxy = new()
    // [ts] {
    // [ts] Server = "${proxyServer.HOST}",
    // [ts] },
    // [ts] });
    // [ts] var context = browser.NewContextAsync(new()
    // [ts] {
    // [ts] ColorScheme = ColorScheme.Dark,
    // [ts] Geolocation = new()
    // [ts] {
    // [ts] Latitude = 37.819722m,
    // [ts] Longitude = -122.478611m,
    // [ts] },${process.platform === 'linux' && browserName === 'webkit' ? '' : `
    // [ts] HasTouch = true,
    // [ts] IsMobile = true,`}
    // [ts] Locale = "es",
    // [ts] Permissions = new[] { "geolocation" },
    // [ts] Screen = new()
    // [ts] {
    // [ts] Height = 896,
    // [ts] Width = 414,
    // [ts] },
    // [ts] TimezoneId = "Europe/Rome",
    // [ts] UserAgent = "hardkodemium",
    // [ts] ViewportSize = new()
    // [ts] {
    // [ts] Height = 720,
    // [ts] Width = 1280,
    // [ts] },
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-154 should print load/save storageState", () => {
    // [ts] const loadFileName = testInfo.outputPath("load.json");
    // [ts] const saveFileName = testInfo.outputPath("save.json");
    // [ts] fs.promises.writeFile(loadFileName, JSON.stringify({ cookies: [], origins: [] }), "utf8");
    // [ts] const cli = runCLI([`--load-storage=${loadFileName}`, `--save-storage=${saveFileName}`, "--target=csharp", server.EMPTY_PAGE]);
    // [ts] const expectedResult1 = `
    // [ts] using var playwright = Playwright.CreateAsync();
    // [ts] using var browser = playwright.${capitalize(browserName)}.LaunchAsync(new()
    // [ts] {
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] var context = browser.NewContextAsync(new()
    // [ts] {
    // [ts] StorageStatePath = "${loadFileName.replace(/\\/g, '\\\\')}",
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult1);
    // [ts] const expectedResult2 = `
    // [ts] context.StorageStateAsync(new()
    // [ts] {
    // [ts] Path = "${saveFileName.replace(/\\/g, '\\\\')}"
    // [ts] });
    // [ts] `;
    // [ts] cli.waitFor(expectedResult2);
    // [ts] });
    // [ts] test("CG-155 should work with --save-har", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `await context.RouteFromHARAsync(${JSON.stringify(harFileName)});`;
    // [ts] const cli = runCLI(["--target=csharp", `--save-har=${harFileName}`]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] test("CG-156 should work with --save-har and --save-har-glob", () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `await context.RouteFromHARAsync(${JSON.stringify(harFileName)}, new()
    // [ts] {
    // [ts] Url = "**/*.js",
    // [ts] });`;
    // [ts] const cli = runCLI(["--target=csharp", `--save-har=${harFileName}`, "--save-har-glob=**/*.js"]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] for (const testFramework of ["nunit", "mstest", "xunit"]) { test(`CG-157 should not print context options method override in ${testFramework} if no options were passed`, (_p) => { const cli = runCLI([`--target=csharp-${testFramework}`, server.EMPTY_PAGE]); cli.waitFor(`Page.GotoAsync("${server.EMPTY_PAGE}")`); expect_eq(std.mem.indexOf(u8, cli.text(), "public override BrowserNewContextOptions ContextOptions()") == null, true); });  test(`CG-158 should print context options method override in ${testFramework} if options were passed`, (_p) => { const cli = runCLI([`--target=csharp-${testFramework}`, "--color-scheme=dark", server.EMPTY_PAGE]); cli.waitFor(`Page.GotoAsync("${server.EMPTY_PAGE}")`); expect_eq(std.mem.indexOf(u8, cli.text(), `    public override BrowserNewContextOptions ContextOptions() { return new() { ColorScheme = ColorScheme.Dark, }; } `) != null, true); });  test(`CG-159 should work with --save-har in ${testFramework}`, (_p, testInfo) => { const harFileName = testInfo.outputPath("har.har"); const expectedResult = `await Context.RouteFromHARAsync(${JSON.stringify(harFileName)});`; const cli = runCLI([`--target=csharp-${testFramework}`, `--save-har=${harFileName}`]); cli.waitFor(expectedResult); cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] test(`CG-160 should work with --save-har and --save-har-glob in ${testFramework}`, () => {
    // [ts] const harFileName = testInfo.outputPath("har.har");
    // [ts] const expectedResult = `await Context.RouteFromHARAsync(${JSON.stringify(harFileName)}, new()
    // [ts] {
    // [ts] Url = "**/*.js",
    // [ts] });`;
    // [ts] const cli = runCLI([`--target=csharp-${testFramework}`, `--save-har=${harFileName}`, "--save-har-glob=**/*.js"]);
    // [ts] cli.waitFor(expectedResult);
    // [ts] cli.exit();
    // [ts] const json = JSON.parse(fs.readFileSync(harFileName, "utf-8"));
    // [ts] expect_eq(json.log.creator.name, "Playwright");
    // [ts] });
    // [ts] }
    // [ts] test(`CG-161 should print a valid basic program in mstest`, () => {
    // [ts] const cli = runCLI([`--target=csharp-mstest`, "--color-scheme=dark", server.EMPTY_PAGE]);
    // [ts] cli.waitFor(`Page.GotoAsync("${server.EMPTY_PAGE}")`);
    // [ts] const expected = `using Microsoft.Playwright.MSTest;
    // [ts] using Microsoft.Playwright;
    // [ts] [TestClass]
    // [ts] public class Tests : PageTest
    // [ts] {
    // [ts] public override BrowserNewContextOptions ContextOptions()
    // [ts] {
    // [ts] return new()
    // [ts] {
    // [ts] ColorScheme = ColorScheme.Dark,
    // [ts] };
    // [ts] }
    // [ts] [TestMethod]
    // [ts] public async Task MyTest()
    // [ts] {
    // [ts] Page.GotoAsync("${server.EMPTY_PAGE}");
    // [ts] }
    // [ts] }`;
    // [ts] expect_eq(std.mem.indexOf(u8, cli.text(), expected) != null, true);
    // [ts] });
    // [ts] test(`CG-162 should print a valid basic program in nunit`, () => {
    // [ts] const cli = runCLI([`--target=csharp-nunit`, "--color-scheme=dark", server.EMPTY_PAGE]);
    // [ts] cli.waitFor(`Page.GotoAsync("${server.EMPTY_PAGE}")`);
    // [ts] const expected = `using Microsoft.Playwright.NUnit;
    // [ts] using Microsoft.Playwright;
    // [ts] [Parallelizable(ParallelScope.Self)]
    // [ts] [TestFixture]
    // [ts] public class Tests : PageTest
    // [ts] {
    // [ts] public override BrowserNewContextOptions ContextOptions()
    // [ts] {
    // [ts] return new()
    // [ts] {
    // [ts] ColorScheme = ColorScheme.Dark,
    // [ts] };
    // [ts] }
    // [ts] [Test]
    // [ts] public async Task MyTest()
    // [ts] {
    // [ts] Page.GotoAsync("${server.EMPTY_PAGE}");
    // [ts] }
    // [ts] }`;
    // [ts] expect_eq(std.mem.indexOf(u8, cli.text(), expected) != null, true);
    // [ts] });
    // [ts] test(`CG-163 should print a valid basic program in xunit`, () => {
    // [ts] const cli = runCLI([`--target=csharp-xunit`, "--color-scheme=dark", server.EMPTY_PAGE]);
    // [ts] cli.waitFor(`Page.GotoAsync("${server.EMPTY_PAGE}")`);
    // [ts] const expected = `using Microsoft.Playwright.Xunit;
    // [ts] using Microsoft.Playwright;
    // [ts] using Xunit;
    // [ts] public class Tests : PageTest
    // [ts] {
    // [ts] public override BrowserNewContextOptions ContextOptions()
    // [ts] {
    // [ts] return new()
    // [ts] {
    // [ts] ColorScheme = ColorScheme.Dark,
    // [ts] };
    // [ts] }
    // [ts] [Fact]
    // [ts] public async Task MyTest()
    // [ts] {
    // [ts] Page.GotoAsync("${server.EMPTY_PAGE}");
    // [ts] }
    // [ts] }`;
    // [ts] expect_eq(std.mem.indexOf(u8, cli.text(), expected) != null, true);
    // [ts] });
}
