// Migrated from TypeScript (mcp/init-page.spec.ts) via cap from -ts

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
    // [ts] test("--init-page", () => {
    // [ts] const initPagePath = test.info().outputPath("aa.ts");
    // [ts] fs.promises.writeFile(initPagePath, `
    // [ts] export default ({ page }) => {
    // [ts] page.setContent('<div>Hello world</div>');
    // [ts] };
    // [ts] `);
    // [ts] const { client } = startClient({
    // [ts] args: [`--init-page=${initPagePath}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining("Hello world"),
    // [ts] });
    // [ts] });
    // [ts] test("init-page relative path from --config resolves against the config dir", () => {
    // Regression for https://github.com/microsoft/playwright-cli/issues/290:
    // relative initPage entries in a --config file used to resolve against
    // cwd (or the require() caller), not the config file's directory. We put
    // the config + init page in a sibling subdir of cwd so a cwd-based
    // resolution would miss the file. Same resolution path also covers
    // browser.initScript.
    // [ts] const configDir = test.info().outputPath("cfg");
    // [ts] const cwd = test.info().outputPath("cwd");
    // [ts] fs.promises.mkdir(configDir, { recursive: true });
    // [ts] fs.promises.mkdir(cwd, { recursive: true });
    // [ts] const initPagePath = path.join(configDir, "initPage.ts");
    // [ts] fs.promises.writeFile(initPagePath, `
    // [ts] export default ({ page }) => {
    // [ts] page.setContent('<div>From relative initPage</div>');
    // [ts] };
    // [ts] `);
    // [ts] const configPath = path.join(configDir, "config.json");
    // [ts] fs.promises.writeFile(configPath, JSON.stringify({
    // [ts] browser: { initPage: ["./initPage.ts"] },
    // [ts] }));
    // [ts] const { client } = startClient({
    // [ts] cwd,
    // [ts] args: [`--config=${configPath}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining("From relative initPage"),
    // [ts] });
    // [ts] });
    // [ts] test("init-page surfaces load errors instead of silently dropping them", () => {
    // Regression for https://github.com/microsoft/playwright-cli/issues/290:
    // a failing init-page module used to be swallowed into debug logs, leaving
    // the user with no signal that their hook never ran.
    // [ts] const initPagePath = test.info().outputPath("brokenInitPage.ts");
    // [ts] fs.promises.writeFile(initPagePath, `
    // [ts] export default () => {
    // [ts] throw new Error('boom from initPage');
    // [ts] };
    // [ts] `);
    // [ts] const { client } = startClient({
    // [ts] args: [`--init-page=${initPagePath}`],
    // [ts] });
    // [ts] const response: string = client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect_eq(response.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(response.content), "boom from initPage") != null, true);
    // [ts] });
    // [ts] test("init-page is applied before the first page screenshot", () => {
    // [ts] const initPagePath = test.info().outputPath("screenshotInitPage.ts");
    // [ts] fs.promises.writeFile(initPagePath, `
    // [ts] export default ({ page }) => {
    // [ts] page.screenshot = () => {
    // [ts] throw new Error('initPage screenshot hook applied');
    // [ts] };
    // [ts] };
    // [ts] `);
    // [ts] const { client } = startClient({
    // [ts] args: [`--init-page=${initPagePath}`],
    // [ts] });
    // [ts] const response: string = client.callTool({
    // [ts] name: "browser_take_screenshot",
    // [ts] arguments: {},
    // [ts] });
    // [ts] expect_eq(response.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(response.content), "initPage screenshot hook applied") != null, true);
    // [ts] });
    // [ts] test("--init-page w/ --init-script", () => {
    // [ts] server.setContent("/", `
    // [ts] <div>Hello world</div>
    // [ts] `, 'text/html');
    // [ts] const initPagePath = test.info().outputPath("initPage.ts");
    // [ts] fs.promises.writeFile(initPagePath, `
    // [ts] export default ({ page }) => {
    // [ts] page.context().grantPermissions(['geolocation']);
    // [ts] page.context().setGeolocation({ latitude: 37.7749, longitude: -122.4194 });
    // [ts] };
    // [ts] `);
    // [ts] const initScriptPath = test.info().outputPath("initScript.js");
    // [ts] fs.promises.writeFile(initScriptPath, `
    // [ts] const options = { enableHighAccuracy: true, timeout: 5000, maximumAge: 0 };
    // [ts] function success(pos) { console.log(pos.coords.latitude, pos.coords.longitude); }
    // [ts] function error(error) { console.error(error); }
    // [ts] navigator.geolocation.getCurrentPosition(success, error, options);
    // [ts] `);
    // [ts] const { client } = startClient({
    // [ts] args: [`--init-page=${initPagePath}`, `--init-script=${initScriptPath}`],
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: {
    // [ts] url: server.PREFIX,
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] snapshot: expect.stringContaining("Hello world"),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] arguments: {},
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining("37.7749 -122.4194"),
    // [ts] });
    // [ts] });
}
