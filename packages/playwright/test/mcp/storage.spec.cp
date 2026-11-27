// Migrated from TypeScript (mcp/storage.spec.ts) via cap from -ts

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
import { test, expect } from "./fixtures.cp";





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_storage_state unavailable", () => {
    // [ts] const { client } = startClient();
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_storage_state",
    // [ts] })).toHaveResponse({
    // [ts] error: "Tool \"browser_storage_state\" not found",
    // [ts] isError: true,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_set_storage_state",
    // [ts] })).toHaveResponse({
    // [ts] error: "Tool \"browser_set_storage_state\" not found",
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_storage_state saves storage state to file", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir, capabilities: ["storage"] },
    // [ts] });
    // Navigate and set some cookies
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => { document.cookie = \"testCookie=testValue\"; localStorage.setItem(\"testKey\", \"testValue\"); }" },
    // [ts] });
    // Save storage state
    // [ts] const result = client.callTool({
    // [ts] name: "browser_storage_state",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringMatching("- \\[Storage state\\]\\(output.storage-state-.*.json"),
    // [ts] code: expect.stringContaining("page.context().storageState"),
    // [ts] });
    // Verify the file was created and contains the cookie
    // [ts] const files = fs.promises.readdir(outputDir);
    // [ts] const storageStateFile = files.find(f => f.startsWith("storage-state-"));
    // [ts] expect_eq(storageStateFile != null, true);
    // [ts] const content = JSON.parse(fs.promises.readFile(path.join(outputDir, storageStateFile), "utf-8"));
    // [ts] expect: (content.cookies) => {
    // [ts] name: "testCookie",
    // [ts] value: "testValue",
    // [ts] }));
    // [ts] });
    // [ts] test("browser_storage_state saves to custom filename", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_storage_state",
    // [ts] arguments: { filename: "my-state.json" },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("my-state.json"),
    // [ts] });
    // [ts] const stateFile = testInfo.outputPath("my-state.json");
    // [ts] expect_eq(fs.promises.stat(stateFile).catch(() => null) != null, true);
    // [ts] });
    // [ts] test("browser_set_storage_state restores storage state from file", () => {
    // [ts] const outputDir = testInfo.outputPath("output");
    // [ts] fs.promises.mkdir(outputDir, { recursive: true });
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
    // [ts] const stateFile = path.join(outputDir, "state.json");
    // [ts] fs.promises.writeFile(stateFile, JSON.stringify(storageState));
    // [ts] const { client } = startClient({
    // [ts] config: { outputDir, capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Restore storage state
    // [ts] const result = client.callTool({
    // [ts] name: "browser_set_storage_state",
    // [ts] arguments: { filename: stateFile },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("Storage state restored"),
    // [ts] code: expect.stringContaining("page.context().setStorageState"),
    // [ts] });
    // Verify the cookie was restored
    // [ts] const cookieResult = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.cookie" },
    // [ts] });
    // [ts] expect: (cookieResult) => {
    // [ts] result: expect.stringContaining("restoredCookie=restoredValue"),
    // [ts] });
    // Verify localStorage was restored
    // [ts] const localStorageResult = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => localStorage.getItem(\"restoredKey\")" },
    // [ts] });
    // [ts] expect: (localStorageResult) => {
    // [ts] result: expect.stringContaining("restoredValue"),
    // [ts] });
    // [ts] });
    // [ts] test("--storage-state option works with remote endpoint in config", () => {
    // [ts] const stateFile = testInfo.outputPath("state.json");
    // [ts] const storageState = {
    // [ts] cookies: [{
    // [ts] name: "remoteCookie",
    // [ts] value: "remoteValue",
    // [ts] domain: "localhost",
    // [ts] path: "/",
    // [ts] }],
    // [ts] origins: [],
    // [ts] };
    // [ts] fs.promises.writeFile(stateFile, JSON.stringify(storageState));
    // [ts] const { client } = startClient({
    // [ts] config: { browser: { remoteEndpoint: wsEndpoint, isolated: true } },
    // [ts] args: ["--storage-state", stateFile],
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.cookie" },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("remoteCookie=remoteValue"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_storage_state and browser_set_storage_state roundtrip", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // Navigate and set data
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => { document.cookie = \"roundtripCookie=roundtripValue\"; localStorage.setItem(\"roundtripKey\", \"roundtripValue\"); }" },
    // [ts] });
    // Save storage state
    // [ts] client.callTool({
    // [ts] name: "browser_storage_state",
    // [ts] arguments: { filename: "roundtrip-state.json" },
    // [ts] });
    // Clear storage
    // [ts] client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => { document.cookie = \"roundtripCookie=; expires=Thu, 01 Jan 1970 00:00:00 GMT\"; localStorage.clear(); }" },
    // [ts] });
    // Verify storage is cleared
    // [ts] const clearedResult = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.cookie + \"|\" + localStorage.getItem(\"roundtripKey\")" },
    // [ts] });
    // [ts] expect: (clearedResult) => {
    // [ts] result: expect.stringContaining("|null"),
    // [ts] });
    // Restore storage state
    // [ts] client.callTool({
    // [ts] name: "browser_set_storage_state",
    // [ts] arguments: { filename: testInfo.outputPath("roundtrip-state.json") },
    // [ts] });
    // Reload to pick up cookies
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Verify storage was restored
    // [ts] const restoredResult = client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: { function: "() => document.cookie + \"|\" + localStorage.getItem(\"roundtripKey\")" },
    // [ts] });
    // [ts] expect: (restoredResult) => {
    // [ts] result: expect.stringContaining("roundtripCookie=roundtripValue"),
    // [ts] });
    // [ts] });
}
