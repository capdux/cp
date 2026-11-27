// Migrated from TypeScript (library/inspector/cli-codegen-javascript.spec.ts) via cap from -ts

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
import { test } from "./inspectorTest.cp";

fn launchOptions(channel: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] return if (channel) `channel: '${channel}',\n    headless: false` else "headless: false";
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-105 should print the correct imports and context options", () => {
    // [ts] const cli = runCLI(["--target=javascript", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `const { ${browserName} } = require('playwright');
    // [ts] (() => {
    // [ts] const browser = ${browserName}.launch({
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] const context = browser.newContext();`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-106 should print the correct context options for custom settings", () => {
    // [ts] const cli = runCLI(["--color-scheme=light", "--target=javascript", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `const { ${browserName} } = require('playwright');
    // [ts] (() => {
    // [ts] const browser = ${browserName}.launch({
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] const context = browser.newContext({
    // [ts] colorScheme: 'light'
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-107 should work with --http-credentials", () => {
    // [ts] server.setAuth("/empty.html", "user", "pass");
    // [ts] const cli = runCLI(["--http-credentials=user:pass", "--target=javascript", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `const { ${browserName} } = require('playwright');
    // [ts] (() => {
    // [ts] const browser = ${browserName}.launch({
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] const context = browser.newContext({
    // [ts] httpCredentials: {
    // [ts] password: 'pass',
    // [ts] username: 'user'
    // [ts] }
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] page.goto('${server.EMPTY_PAGE}');`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-108 should print the correct context options when using a device", () => {
    // [ts] test(browserName != "chromium");
    // [ts] const cli = runCLI(["--device=Pixel 2", "--target=javascript", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `const { chromium, devices } = require('playwright');
    // [ts] (() => {
    // [ts] const browser = chromium.launch({
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] const context = browser.newContext({
    // [ts] ...devices['Pixel 2'],
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-109 should print the correct context options when using a device and additional options", () => {
    // [ts] test(browserName != "webkit");
    // [ts] const cli = runCLI(["--color-scheme=light", "--device=iPhone 11", "--target=javascript", server.EMPTY_PAGE]);
    // [ts] const expectedResult = `const { webkit, devices } = require('playwright');
    // [ts] (() => {
    // [ts] const browser = webkit.launch({
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] const context = browser.newContext({
    // [ts] ...devices['iPhone 11'],
    // [ts] colorScheme: 'light'
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult);
    // [ts] });
    // [ts] test("CG-110 should save the codegen output to a file if specified", () => {
    // [ts] const cli = runCLI(["--target=javascript", server.EMPTY_PAGE]);
    // [ts] cli.waitFor(`const { ${browserName} } = require('playwright');
    // [ts] (() => {
    // [ts] const browser = ${browserName}.launch({
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto('${server.EMPTY_PAGE}');
    // ---------------------
    // [ts] context.close();
    // [ts] browser.close();
    // [ts] })();`);
    // [ts] });
    // [ts] test("CG-111 should print load/save storageState", () => {
    // [ts] const loadFileName = testInfo.outputPath("load.json");
    // [ts] const saveFileName = testInfo.outputPath("save.json");
    // [ts] fs.promises.writeFile(loadFileName, JSON.stringify({ cookies: [], origins: [] }), "utf8");
    // [ts] const cli = runCLI([`--load-storage=${loadFileName}`, `--save-storage=${saveFileName}`, "--target=javascript", server.EMPTY_PAGE]);
    // [ts] const expectedResult1 = `const { ${browserName} } = require('playwright');
    // [ts] (() => {
    // [ts] const browser = ${browserName}.launch({
    // [ts] ${launchOptions(channel)}
    // [ts] });
    // [ts] const context = browser.newContext({
    // [ts] storageState: '${loadFileName.replace(/\\/g, '\\\\')}'
    // [ts] });`;
    // [ts] cli.waitFor(expectedResult1);
    // [ts] const expectedResult2 = `
    // ---------------------
    // [ts] context.storageState({ path: '${saveFileName.replace(/\\/g, '\\\\')}' });
    // [ts] context.close();
    // [ts] browser.close();
    // [ts] })();`;
    // [ts] cli.waitFor(expectedResult2);
    // [ts] });
}
