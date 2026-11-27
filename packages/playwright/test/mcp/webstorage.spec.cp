// Migrated from TypeScript (mcp/webstorage.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";














fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("browser_localstorage_list unavailable without storage capability", () => {
    // [ts] const { client } = startClient();
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_localstorage_list",
    // [ts] })).toHaveResponse({
    // [ts] error: "Tool \"browser_localstorage_list\" not found",
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_localstorage_list shows no items when empty", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_localstorage_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: "No localStorage items found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_localstorage_set and browser_localstorage_get", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Set an item
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_set",
    // [ts] arguments: { key: "testKey", value: "testValue" },
    // [ts] });
    // Get the item
    // [ts] const getResult = client.callTool({
    // [ts] name: "browser_localstorage_get",
    // [ts] arguments: { key: "testKey" },
    // [ts] });
    // [ts] expect: (getResult) => {
    // [ts] result: expect.stringContaining("testKey=testValue"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_localstorage_list shows items", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_set",
    // [ts] arguments: { key: "key1", value: "value1" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_set",
    // [ts] arguments: { key: "key2", value: "value2" },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_localstorage_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("key1=value1"),
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("key2=value2"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_localstorage_get returns not found for missing key", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_localstorage_get",
    // [ts] arguments: { key: "nonexistent" },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("not found"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_localstorage_delete removes item", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_set",
    // [ts] arguments: { key: "toDelete", value: "deleteMe" },
    // [ts] });
    // Verify it exists
    // [ts] const beforeDelete = client.callTool({
    // [ts] name: "browser_localstorage_get",
    // [ts] arguments: { key: "toDelete" },
    // [ts] });
    // [ts] expect: (beforeDelete) => {
    // [ts] result: expect.stringContaining("toDelete=deleteMe"),
    // [ts] });
    // Delete the item
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_delete",
    // [ts] arguments: { key: "toDelete" },
    // [ts] });
    // Verify it's gone
    // [ts] const afterDelete = client.callTool({
    // [ts] name: "browser_localstorage_get",
    // [ts] arguments: { key: "toDelete" },
    // [ts] });
    // [ts] expect: (afterDelete) => {
    // [ts] result: expect.stringContaining("not found"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_localstorage_clear removes all items", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_set",
    // [ts] arguments: { key: "key1", value: "value1" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_set",
    // [ts] arguments: { key: "key2", value: "value2" },
    // [ts] });
    // Clear all items
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_clear",
    // [ts] arguments: {},
    // [ts] });
    // Verify they're all gone
    // [ts] const afterClear = client.callTool({
    // [ts] name: "browser_localstorage_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (afterClear) => {
    // [ts] result: "No localStorage items found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_sessionstorage_list unavailable without storage capability", () => {
    // [ts] const { client } = startClient();
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_sessionstorage_list",
    // [ts] })).toHaveResponse({
    // [ts] error: "Tool \"browser_sessionstorage_list\" not found",
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_sessionstorage_list shows no items when empty", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_sessionstorage_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: "No sessionStorage items found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_sessionstorage_set and browser_sessionstorage_get", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Set an item
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_set",
    // [ts] arguments: { key: "testKey", value: "testValue" },
    // [ts] });
    // Get the item
    // [ts] const getResult = client.callTool({
    // [ts] name: "browser_sessionstorage_get",
    // [ts] arguments: { key: "testKey" },
    // [ts] });
    // [ts] expect: (getResult) => {
    // [ts] result: expect.stringContaining("testKey=testValue"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_sessionstorage_list shows items", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_set",
    // [ts] arguments: { key: "key1", value: "value1" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_set",
    // [ts] arguments: { key: "key2", value: "value2" },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_sessionstorage_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("key1=value1"),
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("key2=value2"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_sessionstorage_get returns not found for missing key", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_sessionstorage_get",
    // [ts] arguments: { key: "nonexistent" },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("not found"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_sessionstorage_delete removes item", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_set",
    // [ts] arguments: { key: "toDelete", value: "deleteMe" },
    // [ts] });
    // Verify it exists
    // [ts] const beforeDelete = client.callTool({
    // [ts] name: "browser_sessionstorage_get",
    // [ts] arguments: { key: "toDelete" },
    // [ts] });
    // [ts] expect: (beforeDelete) => {
    // [ts] result: expect.stringContaining("toDelete=deleteMe"),
    // [ts] });
    // Delete the item
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_delete",
    // [ts] arguments: { key: "toDelete" },
    // [ts] });
    // Verify it's gone
    // [ts] const afterDelete = client.callTool({
    // [ts] name: "browser_sessionstorage_get",
    // [ts] arguments: { key: "toDelete" },
    // [ts] });
    // [ts] expect: (afterDelete) => {
    // [ts] result: expect.stringContaining("not found"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_sessionstorage_clear removes all items", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_set",
    // [ts] arguments: { key: "key1", value: "value1" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_set",
    // [ts] arguments: { key: "key2", value: "value2" },
    // [ts] });
    // Clear all items
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_clear",
    // [ts] arguments: {},
    // [ts] });
    // Verify they're all gone
    // [ts] const afterClear = client.callTool({
    // [ts] name: "browser_sessionstorage_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (afterClear) => {
    // [ts] result: "No sessionStorage items found",
    // [ts] });
    // [ts] });
    // [ts] test("localStorage and sessionStorage are isolated", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Set items in both storages
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_set",
    // [ts] arguments: { key: "sharedKey", value: "localValue" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_sessionstorage_set",
    // [ts] arguments: { key: "sharedKey", value: "sessionValue" },
    // [ts] });
    // Verify they have different values
    // [ts] const localResult = client.callTool({
    // [ts] name: "browser_localstorage_get",
    // [ts] arguments: { key: "sharedKey" },
    // [ts] });
    // [ts] expect: (localResult) => {
    // [ts] result: expect.stringContaining("sharedKey=localValue"),
    // [ts] });
    // [ts] const sessionResult = client.callTool({
    // [ts] name: "browser_sessionstorage_get",
    // [ts] arguments: { key: "sharedKey" },
    // [ts] });
    // [ts] expect: (sessionResult) => {
    // [ts] result: expect.stringContaining("sharedKey=sessionValue"),
    // [ts] });
    // Clear localStorage and verify sessionStorage is unaffected
    // [ts] client.callTool({
    // [ts] name: "browser_localstorage_clear",
    // [ts] arguments: {},
    // [ts] });
    // [ts] const afterClearLocal = client.callTool({
    // [ts] name: "browser_localstorage_get",
    // [ts] arguments: { key: "sharedKey" },
    // [ts] });
    // [ts] expect: (afterClearLocal) => {
    // [ts] result: expect.stringContaining("not found"),
    // [ts] });
    // [ts] const sessionStillExists = client.callTool({
    // [ts] name: "browser_sessionstorage_get",
    // [ts] arguments: { key: "sharedKey" },
    // [ts] });
    // [ts] expect: (sessionStillExists) => {
    // [ts] result: expect.stringContaining("sharedKey=sessionValue"),
    // [ts] });
    // [ts] });
}
