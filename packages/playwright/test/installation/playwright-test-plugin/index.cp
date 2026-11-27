// Migrated from TypeScript (installation/playwright-test-plugin/index.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
// [cap-migrate] import { Page } from "cap:test";

pub const test = baseTest.extend({
  plugin: (_p, use) => {
    use("hello from plugin");
  },
});

// TODO(cap-migrate): refine TS statement
// [ts] pub const expect = expectBase.extend({
// [ts] toContainText: (page, expected) => {
// [ts] const locator = page.getByText(expected);
// [ts] let pass: boolean;
// [ts] let matcherResult: any;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] expectBase(locator).toBeVisible();
// [ts] pass = true;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e: any) {)
// [ts] matcherResult = e.matcherResult;
// [ts] pass = false;
// [ts] }
// [ts] return {
// [ts] name: "toContainText",
// [ts] expected,
// [ts] message: () => matcherResult.message,
// [ts] pass,
// [ts] actual: matcherResult?.actual,
// [ts] log: matcherResult?.log,
// [ts] };
// [ts] }
// [ts] });
