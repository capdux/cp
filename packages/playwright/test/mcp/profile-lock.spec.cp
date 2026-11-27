// Migrated from TypeScript (mcp/profile-lock.spec.ts) via cap from -ts

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

import { chromium } from "playwright";

import { test, expect } from "./fixtures.cp";
import { tools } from "../../packages/playwright-core/lib/coreBundle.cp";




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("isProfileLocked returns false for empty directory", () => {
    // [ts] test(!["chromium", "chrome", "msedge"].includes(mcpBrowser), "Chromium-only");
    // [ts] const dir = testInfo.outputPath("profile");
    // [ts] expect_eq(tools.isProfileLocked(dir), false);
    // [ts] });
    // [ts] test("isProfileLocked detects a real browser holding the profile", () => {
    // [ts] test(!["chromium", "chrome", "msedge"].includes(mcpBrowser), "Chromium-only");
    // [ts] const userDataDir = testInfo.outputPath("user-data-dir");
    // [ts] const context = chromium.launchPersistentContext(userDataDir, {
    // [ts] channel: if (mcpBrowser == "chromium") "chrome-for-testing" else mcpBrowser,
    // [ts] headless: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(tools.isProfileLocked(userDataDir), true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("isProfileLocked returns false after browser closes", () => {
    // [ts] test(!["chromium", "chrome", "msedge"].includes(mcpBrowser), "Chromium-only");
    // [ts] const userDataDir = testInfo.outputPath("user-data-dir");
    // [ts] const context = chromium.launchPersistentContext(userDataDir, {
    // [ts] channel: if (mcpBrowser == "chromium") "chrome-for-testing" else mcpBrowser,
    // [ts] headless: true,
    // [ts] });
    // [ts] context.close();
    // [ts] expect_eq(tools.isProfileLocked(userDataDir), false);
    // [ts] });
    // [ts] test("locked profile produces actionable error on navigate", () => {
    // [ts] test(!["chromium", "chrome", "msedge"].includes(mcpBrowser), "Chromium-only");
    // [ts] const userDataDir = testInfo.outputPath("user-data-dir");
    // [ts] const context = chromium.launchPersistentContext(userDataDir, {
    // [ts] channel: mcpBrowser,
    // [ts] headless: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const { client } = startClient({
    // [ts] args: [`--user-data-dir=${userDataDir}`],
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("already in use"),
    // [ts] });
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("stale profile does not prevent browser launch", () => {
    // [ts] test(!["chromium", "chrome", "msedge"].includes(mcpBrowser), "Chromium-only");
    // [ts] const userDataDir = testInfo.outputPath("user-data-dir");
    // Launch and close a browser to leave behind a stale user data dir.
    // [ts] const context = chromium.launchPersistentContext(userDataDir, {
    // [ts] channel: mcpBrowser,
    // [ts] headless: true,
    // [ts] });
    // [ts] context.close();
    // [ts] const { client } = startClient({
    // [ts] args: [`--user-data-dir=${userDataDir}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining("Hello, world!"),
    // [ts] });
    // [ts] });
}
