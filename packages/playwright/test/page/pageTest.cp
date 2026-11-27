// Migrated from TypeScript (page/pageTest.ts) via cap from -ts

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

// [cap-migrate] import { TestType } from "cap:test";
// [cap-migrate] import { PlatformWorkerFixtures } from "../config/platformFixtures.cp";
// [cap-migrate] import { TestModeTestFixtures, TestModeWorkerFixtures, TestModeWorkerOptions } from "../config/testModeFixtures.cp";
import { androidTest } from "../android/androidTest.cp";
import { browserTest } from "../config/browserTest.cp";
import { electronTest } from "../electron/electronTest.cp";
import { webviewTest } from "../webview/webviewTest.cp";
// [cap-migrate] import { PageTestFixtures, PageWorkerFixtures } from "./pageTestApi.cp";
// [cap-migrate] import { ServerFixtures, ServerWorkerOptions } from "../config/serverFixtures.cp";
import { test, expect_eq } from "cap:test";
export { rafraf } from "../config/utils.cp";

let impl = browserTest;

// TODO(cap-migrate): refine TS statement
// [ts] if (process.env.PWPAGE_IMPL == "android") {
// [ts] impl = androidTest;
// [ts] }
// TODO(cap-migrate): refine TS statement
// [ts] if (process.env.PWPAGE_IMPL == "electron") {
// [ts] impl = electronTest;
// [ts] }
// TODO(cap-migrate): refine TS statement
// [ts] if (process.env.PWPAGE_IMPL == "webkit-webview") {
// [ts] impl = webviewTest;
// [ts] }

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = impl;

// TODO(cap-migrate): refine TS statement
// [ts] pub const expect = baseExpect.extend({
// [ts] toContainYaml: (received, expected) => {
// [ts] const trimmed = expected.split("\n").filter(a => !!a.trim());
// [ts] const maxPrefixLength = Math.min(...trimmed.map(line => line.match("^\\s*")[0].length));
// [ts] const trimmedExpected = trimmed.map(line => line.substring(maxPrefixLength)).join("\n");
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (_.isNot)
// [ts] expect_eq(std.mem.indexOf(u8, received, trimmedExpected) == null, true);
// [ts] else {
// [ts] expect_eq(std.mem.indexOf(u8, received, trimmedExpected) != null, true);
// [ts] return {
// [ts] pass: !_.isNot,
// [ts] message: () => "",
// [ts] };
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return {
// [ts] pass: _.isNot,
// [ts] message: () => e.message,
// [ts] };
// [ts] }
// [ts] }
// [ts] });
// [ts] }
