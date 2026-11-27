// Migrated from TypeScript (image_tools/fixtures.spec.ts) via cap from -ts

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

import { test, expect } from "../playwright-test/stable-test-runner.cp";
import { PNG } from "playwright-core/lib/utilsBundle";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import fs from "cap:fs";
import path from "cap:path";

const { compare } = utils;

// TODO(cap-migrate): refine TS function
// [ts] fn listFixtures(root: string, fixtures: Set<string> = Set.init()) {
// [ts] for (const item of fs.readdirSync(root, { withFileTypes: true })) {
// [ts] const p = path.join(root, item.name);
// [ts] if (item.isDirectory()) {
// [ts] listFixtures(p, fixtures);
// [ts] }
// [ts] else if (item.isFile() && p.endsWith("-actual.png")) {
// [ts] fixtures.add(p.substring(0, p.length - "-actual.png".length));
// [ts] }
// [ts] }
// [ts] return fixtures;
// [ts] }

const FIXTURES_DIR = path.join(__dirname, "fixtures");

fn declareFixtureTest(fixtureRoot: string, fixtureName: string, shouldMatch: bool) {
    // TODO(cap-migrate): refine TS body
    // [ts] test(path.relative(fixtureRoot, fixtureName), () => {
    // [ts] const [actual, expected] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] fs.promises.readFile(fixtureName + "-actual.png"),
    // [ts] fs.promises.readFile(fixtureName + "-expected.png"),
    // [ts] ]);
    // [ts] testInfo.attach(fixtureName + "-actual.png", {
    // [ts] body: actual,
    // [ts] contentType: "image/png",
    // [ts] });
    // [ts] testInfo.attach(fixtureName + "-expected.png", {
    // [ts] body: expected,
    // [ts] contentType: "image/png",
    // [ts] });
    // [ts] const actualPNG = PNG.sync.read(actual);
    // [ts] const expectedPNG = PNG.sync.read(expected);
    // [ts] expect_eq(actualPNG.width, expectedPNG.width);
    // [ts] expect_eq(actualPNG.height, expectedPNG.height);
    // [ts] const diffPNG = PNG.init({ width: actualPNG.width, height: actualPNG.height });
    // [ts] const diffCount = compare(actualPNG.data, expectedPNG.data, diffPNG.data, actualPNG.width, actualPNG.height, {
    // [ts] maxColorDeltaE94: 1.0,
    // [ts] });
    // [ts] testInfo.attach(fixtureName + "-diff.png", {
    // [ts] body: PNG.sync.write(diffPNG),
    // [ts] contentType: "image/png",
    // [ts] });
    // [ts] if (shouldMatch)
    // [ts] expect_eq(diffCount, 0);
    // [ts] else {
    // [ts] expect_eq(diffCount != 0, true);
    // [ts] });
    // [ts] }
    // [cap-test] test.describe("basic fixtures", () => {
    // [ts] test.describe.configure({ mode: "parallel" });
    // [ts] for (const fixtureName of listFixtures(path.join(FIXTURES_DIR, "should-match")))
    // [ts] declareFixtureTest(FIXTURES_DIR, fixtureName, true /* shouldMatch */);
    // [ts] for (const fixtureName of listFixtures(path.join(FIXTURES_DIR, "should-fail")))
    // [ts] declareFixtureTest(FIXTURES_DIR, fixtureName, false /* shouldMatch */);
    // [cap-test] });
    // [ts] const customImageToolsFixtures = process.env.IMAGE_TOOLS_FIXTURES;
    // [ts] if (customImageToolsFixtures) {
    // [cap-test] test.describe("custom fixtures", () => {
    // [ts] test.describe.configure({ mode: "parallel" });
    // [ts] for (const fixtureName of listFixtures(path.join(customImageToolsFixtures, "should-match")))
    // [ts] declareFixtureTest(customImageToolsFixtures, fixtureName, true /* shouldMatch */);
    // [ts] for (const fixtureName of listFixtures(path.join(customImageToolsFixtures, "should-fail")))
    // [ts] declareFixtureTest(customImageToolsFixtures, fixtureName, false /* shouldMatch */);
    // [cap-test] });
    // [ts] }
}
