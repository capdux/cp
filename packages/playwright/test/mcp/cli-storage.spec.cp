// Migrated from TypeScript (mcp/cli-storage.spec.ts) via cap from -ts

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
import path from "cap:path";
import { test, expect } from "./cli-fixtures.cp";



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("state-save saves storage state to file", () => {
    // [ts] const config = { capabilities: ["storage"] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));
    // [ts] cli("open", server.EMPTY_PAGE);
    // Set some cookies and localStorage
    // [ts] cli("eval", "() => { document.cookie = \"testCookie=testValue\"; localStorage.setItem(\"testKey\", \"testValue\"); }");
    // Save storage state
    // [ts] const { output } = cli("state-save");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "storage-state-") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, ".json") != null, true);
    // Verify the file was created and contains the cookie
    // [ts] const outputDir = testInfo.outputPath(".playwright-cli");
    // [ts] const files = fs.promises.readdir(outputDir);
    // [ts] const storageStateFile = files.find(f => f.startsWith("storage-state-"));
    // [ts] expect_eq(storageStateFile != null, true);
    // [ts] const content = JSON.parse(fs.promises.readFile(path.join(outputDir, storageStateFile), "utf-8"));
    // [ts] expect: (content.cookies) => {
    // [ts] name: "testCookie",
    // [ts] value: "testValue",
    // [ts] }));
    // [ts] });
    // [ts] test("state-save saves to custom filename", () => {
    // [ts] const config = { capabilities: ["storage"] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));
    // [ts] cli("open", server.EMPTY_PAGE);
    // [ts] const { output } = cli("state-save", "my-state.json");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "my-state.json") != null, true);
    // [ts] const stateFile = testInfo.outputPath("my-state.json");
    // [ts] expect_eq(fs.promises.stat(stateFile).catch(() => null) != null, true);
    // [ts] });
    // [ts] test("state-load restores storage state from file", () => {
    // [ts] const config = { capabilities: ["storage"] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));
    // Create a storage state file
    // [ts] const storageState = {
    // [ts] cookies: [{
    // [ts] name: "restoredCookie",
    // [ts] value: "restoredValue",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] }],
    // [ts] origins: [{
    // [ts] origin: server.PREFIX,
    // [ts] localStorage: [{
    // [ts] name: "restoredKey",
    // [ts] value: "restoredValue",
    // [ts] }],
    // [ts] }],
    // [ts] };
    // [ts] const stateFile = testInfo.outputPath("state.json");
    // [ts] fs.promises.writeFile(stateFile, JSON.stringify(storageState));
    // [ts] cli("open", server.EMPTY_PAGE);
    // Restore storage state
    // [ts] const { output } = cli("state-load", "state.json");
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Storage state restored") != null, true);
    // Verify the cookie was restored
    // [ts] const { output: cookieResult } = cli("eval", "() => document.cookie");
    // [ts] expect_eq(std.mem.indexOf(u8, cookieResult, "restoredCookie=restoredValue") != null, true);
    // Verify localStorage was restored
    // [ts] const { output: localStorageResult } = cli("eval", "() => localStorage.getItem(\"restoredKey\")");
    // [ts] expect_eq(std.mem.indexOf(u8, localStorageResult, "restoredValue") != null, true);
    // [ts] });
    // [ts] test("state-save and state-load roundtrip", () => {
    // [ts] const config = { capabilities: ["storage"] };
    // [ts] fs.promises.writeFile(testInfo.outputPath(".playwright", "cli.config.json"), JSON.stringify(config, null, 2));
    // [ts] cli("open", server.EMPTY_PAGE);
    // Set data
    // [ts] cli("eval", "() => { document.cookie = \"roundtripCookie=roundtripValue\"; localStorage.setItem(\"roundtripKey\", \"roundtripValue\"); }");
    // Save storage state
    // [ts] cli("state-save", "roundtrip-state.json");
    // Clear storage
    // [ts] cli("eval", "() => { document.cookie = \"roundtripCookie=; expires=Thu, 01 Jan 1970 00:00:00 GMT\"; localStorage.clear(); }");
    // Verify storage is cleared
    // [ts] const { output: clearedResult } = cli("eval", "() => document.cookie + \"|\" + localStorage.getItem(\"roundtripKey\")");
    // [ts] expect_eq(std.mem.indexOf(u8, clearedResult, "|null") != null, true);
    // Restore storage state
    // [ts] cli("state-load", testInfo.outputPath("roundtrip-state.json"));
    // Reload to pick up cookies
    // [ts] cli("goto", server.EMPTY_PAGE);
    // Verify storage was restored
    // [ts] const { output: restoredResult } = cli("eval", "() => document.cookie + \"|\" + localStorage.getItem(\"roundtripKey\")");
    // [ts] expect_eq(std.mem.indexOf(u8, restoredResult, "roundtripCookie=roundtripValue") != null, true);
    // [ts] });
}
