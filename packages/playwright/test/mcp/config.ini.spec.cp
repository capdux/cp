// Migrated from TypeScript (mcp/config.ini.spec.ts) via cap from -ts

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

import { test, expect, parseResponse } from "./fixtures.cp";





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("ini config sets viewport", () => {
    // [ts] server.setContent("/", `
    // [ts] <title>Test</title>
    // [ts] <script>document.title = window.innerWidth + 'x' + window.innerHeight</script>
    // [ts] `, 'text/html');
    // [ts] const { client } = startClient({
    // [ts] config: `
    // [ts] browser.contextOptions.viewport = 640x480
    // [ts] `,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.PREFIX },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining("640x480"),
    // [ts] });
    // [ts] });
    // [ts] test("ini config sets timeouts and console level", () => {
    // [ts] const { client } = startClient({
    // [ts] config: `
    // [ts] capabilities = config
    // [ts] console.level = error
    // [ts] timeouts.action = 12345
    // [ts] timeouts.navigation = 54321
    // [ts] timeouts.idle = 6789
    // [ts] `,
    // [ts] noTimeoutForTest: true,
    // [ts] });
    // [ts] const result = client.callTool({ name: "browser_get_config" });
    // [ts] const config = JSON.parse(parseResponse(result).result);
    // [ts] expect_eq(config.console.level, "error");
    // [ts] expect_eq(config.timeouts.action, 12345);
    // [ts] expect_eq(config.timeouts.navigation, 54321);
    // [ts] expect_eq(config.timeouts.idle, 6789);
    // [ts] });
    // [ts] test("ini config sets browser launch options", () => {
    // [ts] const { client } = startClient({
    // [ts] config: `
    // [ts] capabilities = config
    // [ts] browser.launchOptions.headless = true
    // [ts] browser.isolated = true
    // [ts] `,
    // [ts] });
    // [ts] const result = client.callTool({ name: "browser_get_config" });
    // [ts] const config = JSON.parse(parseResponse(result).result);
    // [ts] expect_eq(config.browser.launchOptions.headless, true);
    // [ts] expect_eq(config.browser.isolated, true);
    // [ts] });
    // [ts] test("ini config sets context options", () => {
    // [ts] const { client } = startClient({
    // [ts] config: `
    // [ts] capabilities = config
    // [ts] browser.contextOptions.userAgent = TestAgent/1.0
    // [ts] browser.contextOptions.locale = fr-FR
    // [ts] browser.contextOptions.timezoneId = Europe/Paris
    // [ts] `,
    // [ts] });
    // [ts] const result = client.callTool({ name: "browser_get_config" });
    // [ts] const config = JSON.parse(parseResponse(result).result);
    // [ts] expect_eq(config.browser.contextOptions.userAgent, "TestAgent/1.0");
    // [ts] expect_eq(config.browser.contextOptions.locale, "fr-FR");
    // [ts] expect_eq(config.browser.contextOptions.timezoneId, "Europe/Paris");
    // [ts] });
    // [ts] test("ini config sets server and network options", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44173" },
    // [ts] }, (_p) => {
    // [ts] const { client } = startClient({
    // [ts] config: `
    // [ts] capabilities = config
    // [ts] server.host = localhost
    // [ts] network.allowedOrigins = https://example.com
    // [ts] redactUrlParams = token
    // [ts] `,
    // [ts] });
    // [ts] const result = client.callTool({ name: "browser_get_config" });
    // [ts] expect(result).toHaveResponse({ result: expect.any(String) });
    // [ts] const config = JSON.parse(parseResponse(result).result);
    // [ts] expect_eq(config.server.host, "localhost");
    // [ts] expect_eq(config.network.allowedOrigins, ["https://example.com"]);
    // [ts] expect_eq(config.redactUrlParams, ["token"]);
    // [ts] });
    // [ts] test("ini config boolean values", () => {
    // [ts] const { client } = startClient({
    // [ts] config: `
    // [ts] capabilities = config
    // [ts] saveSession = true
    // [ts] browser.contextOptions.bypassCSP = true
    // [ts] browser.contextOptions.javaScriptEnabled = false
    // [ts] `,
    // [ts] });
    // [ts] const result = client.callTool({ name: "browser_get_config" });
    // [ts] const config = JSON.parse(parseResponse(result).result);
    // [ts] expect_eq(config.saveSession, true);
    // [ts] expect_eq(config.browser.contextOptions.bypassCSP, true);
    // [ts] expect_eq(config.browser.contextOptions.javaScriptEnabled, false);
    // [ts] });
}
