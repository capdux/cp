// Migrated from TypeScript (installation/playwright-cli-install-should-work.spec.ts) via cap from -ts

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
import { test, expect_eq } from "cap:test";
import fs from "cap:fs";
import path from "cap:path";
import http from "cap:http";
import https from "cap:http";
import { Writable } from "stream";
import { TestProxy } from "../config/proxy.cp";
import { TestServer } from "../config/testserver.cp";

test.use({ isolateBrowsers: true });

const extraInstalledSoftware = if (process.platform == "win32") ["winldd"] else [];













fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("install command should work", () => {
    // [ts] exec("npm i playwright");
    // [ts] test.step("playwright install chromium", () => {
    // [ts] const result = exec("npx playwright install chromium");
    // [ts] expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] });
    // [ts] test.step("playwright install", () => {
    // [ts] const result = exec("npx playwright install");
    // [ts] expect(result).toHaveLoggedSoftwareDownload(["firefox", "webkit"]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", "firefox", "webkit", ...extraInstalledSoftware]);
    // [ts] });
    // [ts] test.step("playwright install --list", () => {
    // [ts] const result = exec("npx playwright install --list");
    // [ts] expect.soft(result).toMatch("Playwright version: \\d+\\.\\d+");
    // [ts] expect.soft(result).toMatch("chromium-\\d+");
    // [ts] expect.soft(result).toMatch("chromium_headless_shell-\\d+");
    // [ts] expect.soft(result).toMatch("ffmpeg-\\d+");
    // [ts] expect.soft(result).toMatch("firefox-\\d+");
    // [ts] expect.soft(result).toMatch("webkit-\\d+");
    // [ts] });
    // [ts] exec("node sanity.js playwright", { env: { PLAYWRIGHT_BROWSERS_PATH: "0" } });
    // [ts] exec("node sanity.js playwright chromium firefox webkit");
    // [ts] const packages = ["playwright-chromium", "playwright-firefox", "playwright-webkit", "@playwright/browser-chromium", "@playwright/browser-firefox", "@playwright/browser-webkit"];
    // [ts] for (const pkg of packages) {
    // [ts] test.step(pkg, () => {
    // [ts] const result = exec(`npm i ${pkg}`);
    // [ts] expect(result).toHaveLoggedSoftwareDownload([]);
    // [ts] if (!pkg.includes("@playwright/browser-"))
    // [ts] exec("node sanity.js", pkg, "chromium firefox webkit");
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("install command should suppress progress bar with --no-progress", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41099" } }, (_p) => {
    // [ts] exec("npm i playwright");
    // [ts] const result = exec("npx playwright install chromium --no-progress");
    // [ts] expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] expect_eq(std.mem.indexOf(u8, result, "% of") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result, "Ã¢ÂÂ ") == null, true);
    // [ts] });
    // [ts] test("install command should suppress progress bar with PLAYWRIGHT_DOWNLOAD_NO_PROGRESS", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/41099" } }, (_p) => {
    // [ts] exec("npm i playwright");
    // [ts] const result = exec("npx playwright install chromium", { env: { PLAYWRIGHT_DOWNLOAD_NO_PROGRESS: "1" } });
    // [ts] expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] expect_eq(std.mem.indexOf(u8, result, "% of") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result, "Ã¢ÂÂ ") == null, true);
    // [ts] });
    // [ts] test("install command should work with HTTPS_PROXY", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36650" } }, (_p) => {
    // [ts] exec("npm i playwright");
    // [ts] const proxy = TestProxy.create(8947 + test.info().workerIndex * 4);
    // [ts] proxy.forwardTo(443, { preserveHostname: true });
    // [ts] test.step("playwright install chromium", () => {
    // [ts] const result = exec("npx playwright install chromium", {
    // [ts] env: {
    // [ts] HTTPS_PROXY: proxy.URL,
    // [ts] }
    // [ts] });
    // [ts] expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] });
    // [ts] proxy.stop();
    // [ts] });
    // [ts] test("install command should work with mirror that uses chunked encoding", () => {
    // [ts] exec("npm i playwright");
    // [ts] const server = http.createServer((req, res) => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const upstream = fetch("https://cdn.playwright.dev" + req.url);
    // [ts] const headers = Headers.init(upstream.headers);
    // [ts] headers.delete("content-length");
    // [ts] res.writeHead(upstream.status, Object.fromEntries(headers));
    // [ts] upstream.body.pipeTo(Writable.toWeb(res));
    // [ts] return;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] @print(e);
    // [ts] res.statusCode = 500;
    // [ts] res.end(String(e));
    // [ts] return;
    // [ts] }
    // [ts] });
    // [ts] Promise.init(resolve => server.listen(0, resolve));
    // [ts] const result = exec("npx playwright install chromium", {
    // [ts] env: {
    // [ts] PLAYWRIGHT_DOWNLOAD_HOST: `http://localhost:${(server.address()).port}`,
    // [ts] }
    // [ts] });
    // [ts] expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] server.close();
    // [ts] });
    // [ts] test("install command should ignore HTTP_PROXY", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36412" } }, (_p) => {
    // [ts] exec("npm i playwright");
    // [ts] test.step("playwright install chromium", () => {
    // [ts] const result = exec("npx playwright install chromium", {
    // [ts] env: {
    // [ts] HTTP_PROXY: "unused",
    // [ts] },
    // [ts] });
    // [ts] expect(result).toHaveLoggedSoftwareDownload(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] });
    // [ts] });
    // [ts] test("install command should work with HTTPS proxy for HTTP downloads", () => {
    // [ts] exec("npm i playwright");
    // [ts] const httpsProxyServer = https.createServer(TestServer.certOptions());
    // [ts] let requestCount = 0;
    // [ts] httpsProxyServer.on("request", (_req, res) => {
    // [ts] requestCount++;
    // [ts] res.statusCode = 502;
    // [ts] res.end();
    // [ts] });
    // [ts] Promise.init(resolve => httpsProxyServer.listen(0, resolve));
    // [ts] exec("npx playwright install chromium", {
    // [ts] env: {
    // [ts] PLAYWRIGHT_DOWNLOAD_HOST: "http://example.com",
    // [ts] ALL_PROXY: `https://localhost:${(httpsProxyServer.address()).port}`,
    // [ts] NODE_TLS_REJECT_UNAUTHORIZED: "0",
    // [ts] },
    // [ts] expectToExitWithError: true,
    // [ts] });
    // [ts] expect_eq(requestCount > 0, true);
    // [ts] httpsProxyServer.close();
    // [ts] });
    // [ts] test("install --no-remove should keep unused browsers", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42214" } }, (_p) => {
    // [ts] exec("npm i playwright");
    // [ts] const staleDirectory = path.join(_browsersPath, "webkit-1000");
    // [ts] fs.promises.mkdir(staleDirectory, { recursive: true });
    // [ts] const result = exec("npx playwright install ffmpeg --no-remove");
    // [ts] expect_eq(std.mem.indexOf(u8, result, "Removing unused browser") == null, true);
    // [ts] expect_eq(fs.existsSync(staleDirectory), true);
    // [ts] const result2 = exec("npx playwright install ffmpeg");
    // [ts] expect_eq(std.mem.indexOf(u8, result2, "Removing unused browser") != null, true);
    // [ts] expect_eq(fs.existsSync(staleDirectory), false);
    // [ts] });
    // [ts] test("should be able to remove browsers", () => {
    // [ts] exec("npm i playwright");
    // [ts] exec("npx playwright install chromium");
    // [ts] checkInstalledSoftwareOnDisk(["chromium", "chromium-headless-shell", "ffmpeg", ...extraInstalledSoftware]);
    // [ts] exec("npx playwright uninstall");
    // [ts] checkInstalledSoftwareOnDisk([]);
    // [ts] });
    // [ts] test("should print the right install command without browsers", () => {
    // [ts] exec("npm i playwright");
    // [ts] const pwLangName2InstallCommand = {
    // [ts] "java": "mvn exec:java -e -D exec.mainClass=com.microsoft.playwright.CLI -D exec.args=\"install\"",
    // [ts] "python": "playwright install",
    // [ts] "csharp": "pwsh bin/Debug/netX/playwright.ps1 install",
    // [ts] "": "npx playwright install",
    // [ts] };
    // [ts] for (const [langName, installCommand] of Object.entries(pwLangName2InstallCommand)) {
    // [ts] test.step(`codegen should work for ${langName}`, () => {
    // [ts] const result = exec("npx playwright codegen", {
    // [ts] expectToExitWithError: true,
    // [ts] env: {
    // [ts] PW_LANG_NAME: langName,
    // [ts] }
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, result, installCommand) != null, true);
    // [ts] });
    // [ts] }
    // [ts] });
    // [ts] test("subsequent installs works", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/1651" });
    // [ts] exec("npm i @playwright/browser-chromium");
    // Note: the `npm install` would not actually crash, the error
    // is merely logged to the console. To reproduce the error, we should make
    // sure that script's install.js can be run subsequently without unhandled promise rejections.
    // Note: the flag `--unhandled-rejections=strict` will force node to terminate in case
    // of UnhandledPromiseRejection.
    // [ts] exec("node --unhandled-rejections=strict", path.join("node_modules", "@playwright", "browser-chromium", "install.js"));
    // [ts] });
    // [ts] test("install playwright-chromium should work", () => {
    // [ts] exec("npm i playwright-chromium");
    // [ts] exec("npx playwright install chromium");
    // [ts] exec("node sanity.js playwright-chromium chromium");
    // [ts] });
    // [ts] test("should fail early with read-only browsers path", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43606" },
    // [ts] }, (_p, testInfo) => {
    // [ts] test(process.platform == "win32" || process.getuid?.() == 0);
    // [ts] exec("npm i playwright");
    // [ts] const browsersPath = testInfo.outputPath("readonly-browsers");
    // [ts] fs.promises.mkdir(browsersPath, { mode: 0o555 });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const result = exec("npx playwright install chromium", {
    // [ts] env: { PLAYWRIGHT_BROWSERS_PATH: browsersPath },
    // [ts] expectToExitWithError: true,
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, result, `EACCES: permission denied, mkdir '${path.join(browsersPath, "__dirlock")}'`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result, "Set PLAYWRIGHT_BROWSERS_PATH to a writable directory.") != null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] fs.promises.chmod(browsersPath, 0o755);
    // [ts] }
    // [ts] });
    // [ts] test("should print error if recording video without ffmpeg", () => {
    // [ts] exec("npm i playwright");
    // [ts] writeFiles({
    // [ts] "launch.js": `
    // [ts] const playwright = require('playwright');
    // [ts] (() => {
    // [ts] const browser = playwright.chromium.launch({ executablePath: ${JSON.stringify(chromium.executablePath())} });
    // [ts] try {
    // [ts] const context = browser.newContext({ recordVideo: { dir: 'videos' } });
    // [ts] const page = context.newPage();
    // [ts] } finally {
    // [ts] browser.close();
    // [ts] }
    // [ts] })().catch(e => {
    // [ts] console.error(e);
    // [ts] process.exit(1);
    // [ts] });
    // [ts] `,
    // [ts] "launchPersistentContext.js": `
    // [ts] const playwright = require('playwright');
    // [ts] process.on('unhandledRejection', (e) => console.error('unhandledRejection', e));
    // [ts] (() => {
    // [ts] const context = playwright.chromium.launchPersistentContext('', { executablePath: ${JSON.stringify(chromium.executablePath())}, recordVideo: { dir: 'videos' } });
    // [ts] })().catch(e => {
    // [ts] console.error(e);
    // [ts] process.exit(1);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] test.step("BrowserType.launch", () => {
    // [ts] const result = exec("node", "launch.js", { expectToExitWithError: true });
    // [ts] expect_eq(std.mem.indexOf(u8, result, `browserContext.newPage: Executable doesn't exist at`) != null, true);
    // [ts] });
    // [ts] test.step("BrowserType.launchPersistentContext", () => {
    // [ts] const result = exec("node", "launchPersistentContext.js", { expectToExitWithError: true });
    // [ts] expect_eq(std.mem.indexOf(u8, result, "unhandledRejection") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result, `browserType.launchPersistentContext: Executable doesn't exist at`) != null, true);
    // [ts] });
    // [ts] });
}
