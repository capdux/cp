// Migrated from TypeScript (mcp/cookies.spec.ts) via cap from -ts

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
    // [ts] test("browser_cookie_list unavailable without storage capability", () => {
    // [ts] const { client } = startClient();
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] })).toHaveResponse({
    // [ts] error: "Tool \"browser_cookie_list\" not found",
    // [ts] isError: true,
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_list shows no cookies when empty", () => {
    // [ts] test(mcpBrowser == "msedge", "Edge is leaking some internal cookies");
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: "No cookies found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_set and browser_cookie_get", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // A blank page has no host to take the cookie domain from.
    // [ts] expect(client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "testCookie", value: "testValue" },
    // [ts] })).toHaveResponse({
    // [ts] isError: true,
    // [ts] error: expect.stringContaining(`Cannot infer the cookie domain from the page URL "about:blank", specify "domain".`),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Set a cookie
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "testCookie", value: "testValue" },
    // [ts] });
    // Get the cookie
    // [ts] const getResult = client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "testCookie" },
    // [ts] });
    // [ts] expect: (getResult) => {
    // [ts] result: expect.stringContaining("testCookie=testValue"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_list shows cookies", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "cookie1", value: "value1" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "cookie2", value: "value2" },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("cookie1=value1"),
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("cookie2=value2"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_list filters by domain", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "localCookie", value: "localValue" },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: { domain: "localhost" },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("localCookie=localValue"),
    // [ts] });
    // [ts] const noResult = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: { domain: "nonexistent.com" },
    // [ts] });
    // [ts] expect: (noResult) => {
    // [ts] result: "No cookies found",
    // [ts] });
    // The domain matches itself and its subdomains, not other domains that end with it.
    // [ts] for (const [name, domain] of [["apex", "example.com"], ["sub", "sub.example.com"], ["other", "my-example.com"]]) {
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name, value: "value", domain },
    // [ts] });
    // [ts] }
    // [ts] const exampleResult = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: { domain: "example.com" },
    // [ts] });
    // [ts] expect: (exampleResult) => {
    // [ts] result: expect.stringContaining("apex=value"),
    // [ts] });
    // [ts] expect: (exampleResult) => {
    // [ts] result: expect.stringContaining("sub=value"),
    // [ts] });
    // [ts] expect: (exampleResult) => {
    // [ts] result: expect.not.stringContaining("other=value"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_list filters by path", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // The path matches itself and its sub-paths, not other paths that start with it.
    // [ts] for (const [name, path] of [["exact", "/api"], ["sub", "/api/users"], ["other", "/apikeys"]]) {
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name, value: "value", path },
    // [ts] });
    // [ts] }
    // [ts] const apiResult = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: { path: "/api" },
    // [ts] });
    // [ts] expect: (apiResult) => {
    // [ts] result: expect.stringContaining("exact=value"),
    // [ts] });
    // [ts] expect: (apiResult) => {
    // [ts] result: expect.stringContaining("sub=value"),
    // [ts] });
    // [ts] expect: (apiResult) => {
    // [ts] result: expect.not.stringContaining("other=value"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_get returns not found for missing cookie", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const result = client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "nonexistent" },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("not found"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_set with all options", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] const futureTime = Math.floor(Date.now() / 1000) + 3600; // 1 hour from now
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: {
    // [ts] name: "fullCookie",
    // [ts] value: "fullValue",
    // [ts] path: "/test",
    // [ts] expires: futureTime,
    // [ts] httpOnly: true,
    // [ts] secure: false,
    // [ts] sameSite: "Lax",
    // [ts] },
    // [ts] });
    // Verify the cookie with detailed info
    // [ts] const getResult = client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "fullCookie" },
    // [ts] });
    // [ts] expect: (getResult) => {
    // [ts] result: expect.stringContaining("fullCookie=fullValue"),
    // [ts] });
    // [ts] expect: (getResult) => {
    // [ts] result: expect.stringContaining("httpOnly: true"),
    // [ts] });
    // [ts] expect: (getResult) => {
    // [ts] result: expect.stringContaining("sameSite: Lax"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_delete removes cookie", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "toDelete", value: "deleteMe" },
    // [ts] });
    // Verify it exists
    // [ts] const beforeDelete = client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "toDelete" },
    // [ts] });
    // [ts] expect: (beforeDelete) => {
    // [ts] result: expect.stringContaining("toDelete=deleteMe"),
    // [ts] });
    // Delete the cookie
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_delete",
    // [ts] arguments: { name: "toDelete" },
    // [ts] });
    // Verify it's gone
    // [ts] const afterDelete = client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "toDelete" },
    // [ts] });
    // [ts] expect: (afterDelete) => {
    // [ts] result: expect.stringContaining("not found"),
    // [ts] });
    // Domain and path narrow it down to one of the same-named cookies.
    // [ts] for (const domain of ["a.example", "b.example"]) {
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "session", value: domain, domain },
    // [ts] });
    // [ts] }
    // [ts] expect(client.callTool({
    // [ts] name: "browser_cookie_delete",
    // [ts] arguments: { name: "session", domain: "a.example", path: "/" },
    // [ts] })).toHaveResponse({
    // [ts] code: `await page.context().clearCookies({ name: 'session', domain: 'a.example', path: '/' });`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "session" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("session=b.example (domain: b.example"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_delete for nonexistent cookie does not error", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Set a cookie to verify delete of nonexistent doesn't affect existing cookies
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "existing", value: "value" },
    // [ts] });
    // Deleting nonexistent cookie should not throw
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_delete",
    // [ts] arguments: { name: "nonexistent" },
    // [ts] });
    // Existing cookie should still be there
    // [ts] const result = client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "existing" },
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] result: expect.stringContaining("existing=value"),
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_clear removes all cookies", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // Set multiple cookies
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "cookie1", value: "value1" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "cookie2", value: "value2" },
    // [ts] });
    // Verify they exist
    // [ts] const beforeClear = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (beforeClear) => {
    // [ts] result: expect.stringContaining("cookie1"),
    // [ts] });
    // Clear all cookies
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_clear",
    // [ts] arguments: {},
    // [ts] });
    // Verify they're all gone
    // [ts] const afterClear = client.callTool({
    // [ts] name: "browser_cookie_list",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (afterClear) => {
    // [ts] result: "No cookies found",
    // [ts] });
    // [ts] });
    // [ts] test("browser_cookie_set and browser_cookie_get use the current tab context", () => {
    // [ts] const { client } = startClient({
    // [ts] config: { capabilities: ["storage"] },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "user", value: "default" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "new", url: server.EMPTY_PAGE, isolatedContext: "alice" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_cookie_set",
    // [ts] arguments: { name: "user", value: "alice" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "user" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("user=alice"),
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_tabs",
    // [ts] arguments: { action: "select", index: 0 },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_cookie_get",
    // [ts] arguments: { name: "user" },
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("user=default"),
    // [ts] });
    // [ts] });
}
