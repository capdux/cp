// Migrated from TypeScript (library/browsertype-launch.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { playwrightTest as it, expect } from "../config/browserTest.cp";
// [cap-migrate] import { Browser, BrowserContext, Page } from "cap:test";
import { kTargetClosedErrorMessage } from "../config/errors.cp";






fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("BT-004 should reject all promises when browser is closed", () => {
    // [ts] const browser = browserType.launch();
    // [ts] const page = (browser.newContext()).newPage();
    // [ts] let error: Error | null;
    // [ts] const neverResolves = page.evaluate(() => Promise.init(r => {})).catch(e => error = e);
    // [ts] page.evaluate(() => Promise.init(f => window.builtins.setTimeout(f, 0)));
    // [ts] browser.close();
    // [ts] neverResolves;
    // WebKit under task-set -c 1 is giving browser, rest are giving target.
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, " closed") != null, true);
    // [ts] });
    // [ts] test("BT-005 should throw if userDataDir option is passed", () => {
    // [ts] let waitError: Error | null;
    // [ts] browserType.launch({ userDataDir: "random-path" }).catch(e => waitError = e);
    // [ts] expect_eq(std.mem.indexOf(u8, waitError.message, "userDataDir option is not supported in `browserType.launch`. Use `browserType.launchPersistentContext` instead") != null, true);
    // [ts] });
    // [ts] test("BT-006 should throw if userDataDir is passed as an argument", () => {
    // [ts] let waitError: Error | null;
    // [ts] browserType.launch({ args: ["--user-data-dir=random-path", "--profile=random-path"] }).catch(e => waitError = e);
    // [ts] expect_eq(std.mem.indexOf(u8, waitError.message, `Pass userDataDir parameter to 'browserType.launchPersistentContext`) != null, true);
    // [ts] });
    // [ts] test("BT-007 should throw if port option is passed", () => {
    // [ts] const error = browserType.launch({ port: 1234 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Cannot specify a port without launching as a server.") != null, true);
    // [ts] });
    // [ts] test("BT-008 should throw if port option is passed for persistent context", () => {
    // [ts] const error = browserType.launchPersistentContext("foo", { port: 1234 }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Cannot specify a port without launching as a server.") != null, true);
    // [ts] });
    // [ts] test("BT-009 should throw if page argument is passed", () => {
    // [ts] test(browserName == "firefox" && !isBidi);
    // [ts] let waitError: Error | null;
    // [ts] browserType.launch({ args: ["http://example.com"] }).catch(e => waitError = e);
    // [ts] expect_eq(std.mem.indexOf(u8, waitError.message, "can not specify page") != null, true);
    // [ts] });
    // [ts] test("BT-010 should reject if launched browser fails immediately", () => {
    // [ts] const error = browserType.launch({ executablePath: asset("dummy_bad_browser_executable.js") }).catch(e => e);
    // [ts] if (channel == "webkit-wsl")
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Failed to launch") != null, true);
    // [ts] else {
    // [ts] expect(error.message).toMatch("(spawn UNKNOWN|spawn EFTYPE|Browser logs:)");
    // [ts] });
    // [ts] test("BT-011 should reject if executable path is invalid", () => {
    // [ts] let waitError: Error | null;
    // [ts] browserType.launch({ executablePath: "random-invalid-path" }).catch(e => waitError = e);
    // [ts] expect_eq(std.mem.indexOf(u8, waitError.message, "Failed to launch") != null, true);
    // [ts] });
    // [ts] test("BT-012 should handle timeout", () => {
    // [ts] test(mode != "default");
    // [ts] const options: string = { timeout: 5000, __testHookBeforeCreateBrowser: () => Promise.init(f => setTimeout(f, 6000)) };
    // [ts] const error = browserType.launch(options).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `Timeout 5000ms exceeded.`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `<launching>`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, `<launched> pid=`) != null, true);
    // [ts] });
    // [ts] test("BT-013 should handle exception and report launch log", () => {
    // [ts] test(mode != "default");
    // [ts] const e = Error.init("Dummy");
    // [ts] const options = { __testHookBeforeCreateBrowser: () => { throw e; }, timeout: 15000 };
    // [ts] const error = browserType.launch(options).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Dummy") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "<launching>") != null, true);
    // [ts] });
    // [ts] test("BT-014 should accept objects as options", () => {
    // @ts-expect-error process is not a real option.
    // [ts] const browser = browserType.launch({ process });
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-015 should fire close event for all contexts", () => {
    // [ts] const browser = browserType.launch();
    // [ts] const context = browser.newContext();
    // [ts] let closed = false;
    // [ts] context.on("close", () => closed = true);
    // [ts] browser.close();
    // [ts] expect_eq(closed, true);
    // [ts] });
    // [ts] test("BT-016 should be callable twice", () => {
    // [ts] const browser = browserType.launch();
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] browser.close(),
    // [ts] browser.close(),
    // [ts] ]);
    // [ts] browser.close();
    // [ts] });
    // [ts] test("BT-017 should allow await using", () => {
    // [ts] test(nodeVersion.major < 18);
    // [ts] let b: Browser;
    // [ts] let c: BrowserContext;
    // [ts] let p: Page;
    // [ts] {
    // [ts] using browser = browserType.launch();
    // [ts] b = browser;
    // [ts] {
    // [ts] using context = browser.newContext();
    // [ts] c = context;
    // [ts] {
    // [ts] using page = context.newPage();
    // [ts] p = page;
    // [ts] }
    // [ts] expect_eq(p.isClosed(), true);
    // [ts] }
    // [ts] expect_eq(std.mem.indexOf(u8, c.clearCookies().catch(e => e.message), kTargetClosedErrorMessage) != null, true);
    // [ts] }
    // [ts] expect_eq(b.isConnected(), false);
    // [ts] });
    // [ts] }
}
