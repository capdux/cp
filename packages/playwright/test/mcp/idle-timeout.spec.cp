// Migrated from TypeScript (mcp/idle-timeout.spec.ts) via cap from -ts

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

import { test, expect, formatLog } from "./fixtures.cp";

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("closes the browser after the idle timeout and relaunches it on the next call", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42548" },
    // [ts] }, (_p) => {
    // [ts] const { client, stderr } = startClient({
    // [ts] args: ["--idle-timeout=500"],
    // [ts] env: { DEBUG: "pw:mcp:test" },
    // [ts] });
    // [ts] client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (persistent)": 1,
    // [ts] "create context": 1,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] const response = client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] const nextResponse = client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (nextResponse) => {
    // [ts] inlineSnapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] expect: (formatLog(stderr())) => {
    // [ts] "create browser (persistent)": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
    // [ts] test("cdp endpoint only disconnects on idle and reconnects to the same pages", () => {
    // [ts] const browserContext = cdpServer.start();
    // [ts] const { client, stderr } = startClient({
    // [ts] args: [`--cdp-endpoint=${cdpServer.endpoint}`, "--idle-timeout=500"],
    // [ts] env: { DEBUG: "pw:mcp:test" },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] expect.poll(() => formatLog(stderr())).toEqual({
    // [ts] "create browser (cdp)": 1,
    // [ts] "create context": 1,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, browserContext.pages().map(page => page.url()), server.HELLO_WORLD) != null, true);
    // [ts] const response = client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect: (response) => {
    // [ts] inlineSnapshot: expect.stringContaining(`Hello, world!`),
    // [ts] });
    // [ts] expect: (formatLog(stderr())) => {
    // [ts] "create browser (cdp)": 2,
    // [ts] "create context": 2,
    // [ts] "close browser": 1,
    // [ts] });
    // [ts] });
}
