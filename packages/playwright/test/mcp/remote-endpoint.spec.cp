// Migrated from TypeScript (mcp/remote-endpoint.spec.ts) via cap from -ts

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

import { tools } from "../../packages/playwright-core/lib/coreBundle.cp";

const { resolveCLIConfigForMCP, createBrowserWithInfo } = tools;




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mcpBrowser != "chromium", "Run only on the chromium project; the remote server connection is browser-agnostic.");
    // [ts] test("connect without headers fails on run-server endpoint", () => {
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] browser: {
    // [ts] remoteEndpoint: runServerEndpoint,
    // [ts] isolated: true,
    // [ts] },
    // [ts] },
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.EMPTY_PAGE },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] isError: true,
    // [ts] error: expect.stringContaining(`reading 'launch'`),
    // [ts] });
    // [ts] });
    // [ts] test("remoteEndpoint accepts ConnectOptions object with headers", () => {
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] browser: {
    // [ts] remoteEndpoint: {
    // [ts] endpoint: runServerEndpoint,
    // [ts] headers: { "x-playwright-browser": "chromium" },
    // [ts] },
    // [ts] isolated: true,
    // [ts] },
    // [ts] },
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] page: expect.stringContaining("Page Title: Title"),
    // [ts] });
    // [ts] });
    // [ts] test("browserInfo reports the browser running on the remote endpoint, not the configured one", () => {
    // Empty env to isolate the test from the host environment.
    // [ts] const config = resolveCLIConfigForMCP({ browser: "firefox", endpoint: wsEndpoint }, {});
    // [ts] const { browser, browserInfo } = createBrowserWithInfo(config, { clientName: "test-client", cwd: testInfo.outputPath() }, {});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(config.browser.browserName, "firefox");
    // [ts] expect_eq(browserInfo.browserName, "chromium");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] browser.close();
    // [ts] }
    // [ts] });
    // [ts] test("back-compat: remoteHeaders config still selects the browser on run-server endpoint", () => {
    // [ts] const { client } = startClient({
    // [ts] config: {
    // [ts] browser: {
    // [ts] remoteEndpoint: runServerEndpoint,
    // [ts] remoteHeaders: { "x-playwright-browser": "chromium" },
    // [ts] isolated: true,
    // [ts] },
    // [ts] },
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] page: expect.stringContaining("Page Title: Title"),
    // [ts] });
    // [ts] });
}
