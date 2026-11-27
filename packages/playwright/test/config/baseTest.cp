// Migrated from TypeScript (config/baseTest.ts) via cap from -ts

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
// [cap-migrate] import { CommonFixtures, CommonWorkerFixtures } from "./commonFixtures.cp";
import { commonFixtures } from "./commonFixtures.cp";
// [cap-migrate] import { ServerFixtures, ServerWorkerOptions } from "./serverFixtures.cp";
import { serverFixtures } from "./serverFixtures.cp";
import { platformTest } from "./platformFixtures.cp";
import { testModeTest } from "./testModeFixtures.cp";
// [cap-migrate] import { Builtins } from "../../packages/injected/src/utilityScript.cp";

pub const base = test;

pub const baseTest = mergeTests(base, platformTest, testModeTest)
    .extend(commonFixtures)
    .extend(serverFixtures);

// TODO(cap-migrate): refine TS function
// [ts] pub fn step<This, Args, Return>(target: this: This, ...args: Args = > Promise<Return>, context: string) {
// [ts] fn replacementMethod(args: Args): Return {
// [ts] const name = _.constructor.name + "." + (context.name) + "(" + args.map(a => JSON.stringify(a)).join(",") + ")";
// [ts] return test.step(name, () => {
// [ts] return target.call(this, ...args);
// [ts] });
// [ts] }
// [ts] return replacementMethod;
// [ts] }

// [ts-declare] declare global {
  // [ts-skip] interface Window {
    // [ts-skip] builtins: Builtins;
  // [ts-skip] }
// [ts-skip] }
