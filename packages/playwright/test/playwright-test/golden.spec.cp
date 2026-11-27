// Migrated from TypeScript (playwright-test/golden.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import colors from "colors/safe";
import * as fs from "cap:fs";
import * as path from "cap:path";
import { test, expect, createWebpImage, createWhiteImage, paintBlackPixels } from "./playwright-test-fixtures.cp";

const files = {
  "helper.ts": `;
    import { test as base } from '@playwright/test';
    export { expect } from '@playwright/test';
    export const test = base.extend({
      auto: [ ({}, run, testInfo) => {
        testInfo.snapshotSuffix = '';
        run();
      }, { auto: true } ]
    });
  `
};
















for (const updateSnapshots of ["all", "changed", "missing", "none", "default"]) {
  test(`should update snapshot with the update-snapshots=${updateSnapshots} (config)`, () => {
    const result = runInlineTest({
      "playwright.config.ts": `export default { updateSnapshots: '${updateSnapshots}' };`,
      ...files,
      "a.spec.js-snapshots/snapshot.txt": "Hello world",
      "a.spec.js": `
        const { test, expect } = require('./helper');
        test('is a test', () => {
          expect('Hello world updated').toMatchSnapshot('snapshot.txt');
        });
      `
    });

    const rebase = updateSnapshots == "all" || updateSnapshots == "changed";
    expect_eq(result.exitCode, if (rebase) 0 else 1);
    if (rebase) {
      const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
      if (updateSnapshots == "all")
        expect_eq(std.mem.indexOf(u8, result.output, `${snapshotOutputPath} is not the same, writing actual.`) != null, true);
      if (updateSnapshots == "changed")
        expect_eq(std.mem.indexOf(u8, result.output, `${snapshotOutputPath} does not match, writing actual.`) != null, true);
      const data = fs.readFileSync(snapshotOutputPath);
      expect_eq(data.toString(), "Hello world updated");
    } else {
      expect_eq(std.mem.indexOf(u8, result.output, `toMatchSnapshot`) != null, true);
    }
  });
}








































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should support golden", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should match text snapshot with trailing newline difference", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot1.txt": "Hello world\n",
    // [ts] "a.spec.js-snapshots/snapshot2.txt": "Hello world",
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('snapshot1.txt');
    // [ts] expect('Hello world\\n').toMatchSnapshot('snapshot2.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should work with non-txt extensions", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.csv": `1,2,3`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('1,2,4').toMatchSnapshot('snapshot.csv');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.green("-1,2,3")) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.red("+1,2,4")) != null, true);
    // [ts] });
    // [ts] test("should generate default name", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect.soft('foo').toMatchSnapshot();
    // [ts] expect.soft('bar').toMatchSnapshot();
    // [ts] expect.soft(page.screenshot({type: 'png'})).toMatchSnapshot();
    // [ts] expect.soft(page.screenshot({type: 'jpeg'})).toMatchSnapshot();
    // [ts] expect.soft(Buffer.from([1,2,3,4])).toMatchSnapshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-actual.txt")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-2-actual.txt")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-3-actual.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-4-actual.jpg")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-5-actual.dat")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.spec.js-snapshots", "is-a-test-1.txt")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.spec.js-snapshots", "is-a-test-2.txt")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.spec.js-snapshots", "is-a-test-3.png")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.spec.js-snapshots", "is-a-test-4.jpg")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.spec.js-snapshots", "is-a-test-5.dat")), true);
    // [ts] });
    // [ts] test("should generate separate actual results for repeating names", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29719" });
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/foo.txt": `b`,
    // [ts] "a.spec.js-snapshots/bar/baz.txt": `c`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('## ' + JSON.stringify(testInfo.attachments));
    // [ts] });
    // [ts] test('is a test', () => {
    // [ts] expect.soft('a').toMatchSnapshot('foo.txt');
    // [ts] expect.soft('a').toMatchSnapshot('foo.txt');
    // [ts] expect.soft('b').toMatchSnapshot(['bar', 'baz.txt']);
    // [ts] expect.soft('b').toMatchSnapshot(['bar', 'baz.txt']);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const outputText = result.output;
    // [ts] const attachments = outputText.split("\n").filter(l => l.startsWith("## ")).map(l => l.substring(3)).map(l => JSON.parse(l))[0];
    // [ts] for (const attachment of attachments) {
    // [ts] attachment.path = attachment.path.replace(testInfo.outputDir, "").substring(1).replace("\\\\", "/");
    // [ts] attachment.name = attachment.name.replace("\\\\", "/");
    // [ts] }
    // [ts] expect(attachments).toEqual([
    // [ts] {
    // [ts] "name": "foo-expected.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "a.spec.js-snapshots/foo.txt"
    // [ts] },
    // [ts] {
    // [ts] "name": "foo-actual.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "test-results/a-is-a-test/foo-actual.txt"
    // [ts] },
    // [ts] {
    // [ts] "name": "foo-1-expected.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "a.spec.js-snapshots/foo.txt"
    // [ts] },
    // [ts] {
    // [ts] "name": "foo-1-actual.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "test-results/a-is-a-test/foo-1-actual.txt"
    // [ts] },
    // [ts] {
    // [ts] "name": "bar/baz-expected.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "a.spec.js-snapshots/bar/baz.txt"
    // [ts] },
    // [ts] {
    // [ts] "name": "bar/baz-actual.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "test-results/a-is-a-test/bar/baz-actual.txt"
    // [ts] },
    // [ts] {
    // [ts] "name": "bar/baz-1-expected.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "a.spec.js-snapshots/bar/baz.txt"
    // [ts] },
    // [ts] {
    // [ts] "name": "bar/baz-1-actual.txt",
    // [ts] "contentType": "text/plain",
    // [ts] "path": "test-results/a-is-a-test/bar/baz-1-actual.txt"
    // [ts] }
    // [ts] ]);
    // [ts] });
    // [ts] test("should not attach a missing expected snapshot when update-snapshots is none", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('## ' + JSON.stringify(testInfo.attachments.map(a => a.name)));
    // [ts] });
    // [ts] test('is a test', () => {
    // [ts] expect.soft('a').toMatchSnapshot('foo.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "none" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const names = result.output.split("\n").filter(l => l.startsWith("## ")).map(l => JSON.parse(l.substring(3)))[0];
    // [ts] expect_eq(std.mem.indexOf(u8, names, "foo-actual.txt") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, names, "foo-expected.txt") == null, true);
    // [ts] });
    // [ts] test("should compile with different option combinations", () => {
    // [ts] const result = runTSC({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('is a test', () => {
    // [ts] expect('foo').toMatchSnapshot();
    // [ts] expect('foo').toMatchSnapshot({ threshold: 0.2 });
    // [ts] expect('foo').toMatchSnapshot({ maxDiffPixelRatio: 0.2 });
    // [ts] expect('foo').toMatchSnapshot({ maxDiffPixels: 0.2 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should fail on wrong golden", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": `Line1
    // [ts] Line2
    // [ts] Line3
    // [ts] Hello world line1
    // [ts] Line5
    // [ts] Line6
    // [ts] Line7`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] const data = [];
    // [ts] data.push('Line1');
    // [ts] data.push('Line22');
    // [ts] data.push('Line3');
    // [ts] data.push('Hi world line2');
    // [ts] data.push('Line5');
    // [ts] data.push('Line6');
    // [ts] data.push('Line7');
    // [ts] expect(data.join('\\n')).toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Line1") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.green("-Line2")) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, colors.red("+Line22")) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Line3") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Line5") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Line7") != null, true);
    // [ts] });
    // [ts] test("should write detailed failure result to an output folder", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world updated').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const outputText = result.output;
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "Error: expect(string).toMatchSnapshot(expected)") != null, true);
    // [ts] const expectedSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-expected.txt");
    // [ts] const actualSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-actual.txt");
    // [ts] expect(outputText).toMatch("Expected:.*a\\.spec\\.js-snapshots.snapshot\\.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, `Received: test-results${path.sep}a-is-a-test${path.sep}snapshot-actual.txt`) != null, true);
    // [ts] expect_eq(fs.existsSync(expectedSnapshotArtifactPath), true);
    // [ts] expect_eq(fs.existsSync(actualSnapshotArtifactPath), true);
    // [ts] });
    // [ts] test("doesn\'t create comparison artifacts in an output folder for passed negated snapshot matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world updated').not.toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const outputText = result.output;
    // [ts] const expectedSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-expected.txt");
    // [ts] const actualSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-actual.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, `Expected: ${expectedSnapshotArtifactPath}`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, `Received: ${actualSnapshotArtifactPath}`) == null, true);
    // [ts] expect_eq(fs.existsSync(expectedSnapshotArtifactPath), false);
    // [ts] expect_eq(fs.existsSync(actualSnapshotArtifactPath), false);
    // [ts] });
    // [ts] test("should fail on same snapshots with negate matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').not.toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Snapshot comparison failed:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Expected result should be different from the actual one.") != null, true);
    // [ts] });
    // [ts] test("should write missing expectations locally twice and continue", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('snapshot.txt');
    // [ts] expect('Hello world2').toMatchSnapshot('snapshot2.txt');
    // [ts] console.log('Here we are!');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshot1OutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: A snapshot doesn't exist at ${snapshot1OutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(fs.readFileSync(snapshot1OutputPath, "utf-8"), "Hello world");
    // [ts] const snapshot2OutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot2.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `Error: A snapshot doesn't exist at ${snapshot2OutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(fs.readFileSync(snapshot2OutputPath, "utf-8"), "Hello world2");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Here we are!") != null, true);
    // [ts] const stackLines = result.output.split("\n").filter(line => line.includes("    at ")).filter(line => !line.includes("a.spec.js"));
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "a.spec.js:4") != null, true);
    // [ts] expect_eq(stackLines.length, 0);
    // [ts] });
    // [ts] test("should not write missing expectations for negated matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').not.toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, matchers using ".not" won\'t write them automatically.`) != null, true);
    // [ts] expect_eq(fs.existsSync(snapshotOutputPath), false);
    // [ts] });
    // [ts] test("should update snapshot with the update-snapshots flag", () => {
    // [ts] const EXPECTED_SNAPSHOT = "Hello world";
    // [ts] const ACTUAL_SNAPSHOT = "Hello world updated";
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('${ACTUAL_SNAPSHOT}').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `${snapshotOutputPath} does not match, writing actual.`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), ACTUAL_SNAPSHOT);
    // [ts] });
    // [ts] test("should ignore text snapshot with the ignore-snapshots flag", () => {
    // [ts] const EXPECTED_SNAPSHOT = "Hello world";
    // [ts] const ACTUAL_SNAPSHOT = "Hello world updated";
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('${ACTUAL_SNAPSHOT}').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "ignore-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, ``) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), EXPECTED_SNAPSHOT);
    // [ts] });
    // [ts] test("shouldn't update snapshot with the update-snapshots flag for negated matcher", () => {
    // [ts] const EXPECTED_SNAPSHOT = "Hello world";
    // [ts] const ACTUAL_SNAPSHOT = "Hello world updated";
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('${ACTUAL_SNAPSHOT}').not.toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), EXPECTED_SNAPSHOT);
    // [ts] });
    // [ts] test("should silently write missing expectations locally with the update-snapshots flag", () => {
    // [ts] const ACTUAL_SNAPSHOT = "Hello world new";
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('${ACTUAL_SNAPSHOT}').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), ACTUAL_SNAPSHOT);
    // [ts] });
    // [ts] test("should pass when writing missing expectations with the update-snapshots=missing flag", () => {
    // [ts] const ACTUAL_SNAPSHOT = "Hello world new";
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('${ACTUAL_SNAPSHOT}').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "missing" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(fs.readFileSync(snapshotOutputPath, "utf-8"), ACTUAL_SNAPSHOT);
    // [ts] });
    // [ts] test("should fail when writing missing expectations with the update-snapshots=default flag", () => {
    // [ts] const ACTUAL_SNAPSHOT = "Hello world new";
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('${ACTUAL_SNAPSHOT}').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "default" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] expect_eq(fs.readFileSync(snapshotOutputPath, "utf-8"), ACTUAL_SNAPSHOT);
    // [ts] });
    // [ts] test("should still fail on a mismatching expectation with the update-snapshots=missing flag", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": "Hello world",
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world updated').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "missing" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(fs.readFileSync(snapshotOutputPath, "utf-8"), "Hello world");
    // [ts] });
    // [ts] test("should silently write missing expectations locally with the update-snapshots flag for negated matcher", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').not.toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, matchers using ".not" won\'t write them automatically.`) != null, true);
    // [ts] expect_eq(fs.existsSync(snapshotOutputPath), false);
    // [ts] });
    // [ts] test("should match multiple snapshots", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot1.txt": `Snapshot1`,
    // [ts] "a.spec.js-snapshots/snapshot2.txt": `Snapshot2`,
    // [ts] "a.spec.js-snapshots/snapshot3.txt": `Snapshot3`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Snapshot1').toMatchSnapshot('snapshot1.txt');
    // [ts] expect('Snapshot2').toMatchSnapshot('snapshot2.txt');
    // [ts] expect('Snapshot3').toMatchSnapshot('snapshot3.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should match snapshots from multiple projects", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] import * as path from 'path';
    // [ts] export default { projects: [
    // [ts] { testDir: path.join(__dirname, 'p1') },
    // [ts] { testDir: path.join(__dirname, 'p2') },
    // [ts] ]};
    // [ts] `,
    // [ts] "p1/a.spec.js": `
    // [ts] const { test, expect } = require('../helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Snapshot1').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `,
    // [ts] "p1/a.spec.js-snapshots/snapshot.txt": `Snapshot1`,
    // [ts] "p2/a.spec.js": `
    // [ts] const { test, expect } = require('../helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Snapshot2').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `,
    // [ts] "p2/a.spec.js-snapshots/snapshot.txt": `Snapshot2`,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use provided name", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/provided.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('provided.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use provided name via options", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/provided.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot({ name: 'provided.txt' });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should compare binary", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.dat": Buffer.from([1, 2, 3, 4]),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from([1,2,3,4])).toMatchSnapshot('snapshot.dat');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should respect maxDiffPixels option", () => {
    // [ts] const width = 20;
    // [ts] const height = 20;
    // [ts] const BAD_PIXELS = 120;
    // [ts] const image1 = createWhiteImage(width, height);
    // [ts] const image2 = paintBlackPixels(image1, BAD_PIXELS);
    // [ts] test.step("make sure default comparison fails", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png": image1,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${image2.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "120 pixels") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "ratio 0.30") != null, true);
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] });
    // [ts] expect((runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png": image1,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${image2.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png', {
    // [ts] maxDiffPixels: ${BAD_PIXELS}
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixels option is respected").toBe(0);
    // [ts] expect((runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [
    // [ts] { expect: { toMatchSnapshot: { maxDiffPixels: ${BAD_PIXELS} } } },
    // [ts] ]};
    // [ts] `,
    // [ts] "a.spec.js-snapshots/snapshot.png": image1,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${image2.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixels option in project config is respected").toBe(0);
    // [ts] });
    // [ts] test("should respect maxDiffPixelRatio option", () => {
    // [ts] const width = 20;
    // [ts] const height = 20;
    // [ts] const BAD_RATIO = 0.25;
    // [ts] const BAD_PIXELS = Math.floor(width * height * BAD_RATIO);
    // [ts] const image1 = createWhiteImage(width, height);
    // [ts] const image2 = paintBlackPixels(image1, BAD_PIXELS);
    // [ts] expect((runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png": image1,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${image2.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure default comparison fails").toBe(1);
    // [ts] expect((runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png": image1,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${image2.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png', {
    // [ts] maxDiffPixelRatio: ${BAD_RATIO}
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixelRatio option is respected").toBe(0);
    // [ts] expect((runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [
    // [ts] { expect: { toMatchSnapshot: { maxDiffPixelRatio: ${BAD_RATIO} } } },
    // [ts] ]};
    // [ts] `,
    // [ts] "a.spec.js-snapshots/snapshot.png": image1,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${image2.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] })).exitCode, "make sure maxDiffPixels option in project config is respected").toBe(0);
    // [ts] });
    // [ts] test("should compare PNG images", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png":
    // [ts] Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+P+/HgAFhAJ/wlseKgAAAABJRU5ErkJggg==", "base64"),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+P+/HgAFhAJ/wlseKgAAAABJRU5ErkJggg==', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should compare different PNG images", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png":
    // [ts] Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+P+/HgAFhAJ/wlseKgAAAABJRU5ErkJggg==", "base64"),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVQYV2NgYAAAAAMAAWgmWQ0AAAAASUVORK5CYII==', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const outputText = result.output;
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "Error: expect(Buffer).toMatchSnapshot(expected)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "1 pixels (ratio 1.00 of all image pixels) are different.") != null, true);
    // [ts] const expectedSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-expected.png");
    // [ts] const actualSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-actual.png");
    // [ts] const diffSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-diff.png");
    // [ts] expect(outputText).toMatch("Expected:.*a\\.spec\\.js-snapshots.snapshot\\.png");
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, `Received: test-results${path.sep}a-is-a-test${path.sep}snapshot-actual.png`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, `Diff:     test-results${path.sep}a-is-a-test${path.sep}snapshot-diff.png`) != null, true);
    // [ts] expect_eq(fs.existsSync(expectedSnapshotArtifactPath), true);
    // [ts] expect_eq(fs.existsSync(actualSnapshotArtifactPath), true);
    // [ts] expect_eq(fs.existsSync(diffSnapshotArtifactPath), true);
    // [ts] });
    // [ts] test("should compare WEBP images", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.webp": createWebpImage(1, 1, 255, 255, 255),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(page.screenshot({ type: 'webp', clip: { x: 0, y: 0, width: 1, height: 1 } })).toMatchSnapshot('snapshot.webp');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should compare different WEBP images", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.webp": createWebpImage(1, 1, 255, 0, 0),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from([${createWebpImage(1, 1, 0, 0, 255).join(',')}])).toMatchSnapshot('snapshot.webp');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const outputText = result.output;
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "Error: expect(Buffer).toMatchSnapshot(expected)") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "1 pixels (ratio 1.00 of all image pixels) are different.") != null, true);
    // [ts] const expectedSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-expected.webp");
    // [ts] const actualSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-actual.webp");
    // [ts] const diffSnapshotArtifactPath = testInfo.outputPath("test-results", "a-is-a-test", "snapshot-diff.webp");
    // [ts] expect_eq(fs.existsSync(expectedSnapshotArtifactPath), true);
    // [ts] expect_eq(fs.existsSync(actualSnapshotArtifactPath), true);
    // [ts] expect_eq(fs.existsSync(diffSnapshotArtifactPath), true);
    // [ts] });
    // [ts] test("should use webp extension for anonymous webp snapshots", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(page.screenshot({ type: 'webp' })).toMatchSnapshot();
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-is-a-test", "is-a-test-1-actual.webp")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("a.spec.js-snapshots", "is-a-test-1.webp")), true);
    // [ts] });
    // [ts] test("should correctly handle different JPEG image signatures", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('test1', () => {
    // [ts] expect(Buffer.from([0xff, 0xd8, 0xff, 0xe1, 0x00, 0xbc, 0x45, 0x78, 0x69, 0x66, 0x00, 0x00, 0x49])).toMatchSnapshot();
    // [ts] });
    // [ts] test('test2', () => {
    // [ts] expect(Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+P+/HgAFhAJ/wlseKgAAAABJRU5ErkJggg==', 'base64')).toMatchSnapshot();
    // [ts] });
    // [ts] test('test3', () => {
    // [ts] expect(Buffer.from('/9j/4AAQSkZJRgABJD+3EOqoh+DYbgljkJTDA0AfvKeYZU/uxcluvipXU7hAGOoguGFv/Tq3/azTyFRJjgsQRp4mu0elkP9IxBh6uj5gpJVpNk9XJdE+51Nk7kUSQSZtPiXYUR2zd7JxzAVMvGFsjQ==', 'base64')).toMatchSnapshot();
    // [ts] });
    // [ts] `,
    // [ts] }, { "update-snapshots": true });
    // [ts] const expectedTest1ArtifactPath = testInfo.outputPath("a.spec.js-snapshots", "test1-1.jpg");
    // [ts] const expectedTest2ArtifactPath = testInfo.outputPath("a.spec.js-snapshots", "test2-1.png");
    // [ts] const expectedTest3ArtifactPath = testInfo.outputPath("a.spec.js-snapshots", "test3-1.jpg");
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${expectedTest1ArtifactPath}, writing actual`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${expectedTest2ArtifactPath}, writing actual`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${expectedTest3ArtifactPath}, writing actual`) != null, true);
    // [ts] });
    // [ts] test("should respect threshold", () => {
    // [ts] const expected = fs.readFileSync(path.join(__dirname, "assets/screenshot-canvas-expected.png"));
    // [ts] const actual = fs.readFileSync(path.join(__dirname, "assets/screenshot-canvas-actual.png"));
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png": expected,
    // [ts] "a.spec.js-snapshots/snapshot2.png": expected,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png', { threshold: 0.3 });
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).not.toMatchSnapshot('snapshot.png', { threshold: 0.2 });
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot('snapshot2.png', { threshold: 0.3 });
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot({ name: 'snapshot2.png', threshold: 0.3 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should respect project threshold", () => {
    // [ts] const expected = fs.readFileSync(path.join(__dirname, "assets/screenshot-canvas-expected.png"));
    // [ts] const actual = fs.readFileSync(path.join(__dirname, "assets/screenshot-canvas-actual.png"));
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [
    // [ts] { expect: { toMatchSnapshot: { threshold: 0.2 } } },
    // [ts] ]};
    // [ts] `,
    // [ts] "a.spec.js-snapshots/snapshot.png": expected,
    // [ts] "a.spec.js-snapshots/snapshot2.png": expected,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png', { threshold: 0.3 });
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).not.toMatchSnapshot('snapshot.png');
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot('snapshot2.png', { threshold: 0.3 });
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot({ name: 'snapshot2.png', threshold: 0.3 });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should respect comparator name", () => {
    // [ts] const expected = fs.readFileSync(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-expected.png"));
    // [ts] const actual = fs.readFileSync(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-actual.png"));
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png": expected,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('should pass', () => {
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png', {
    // [ts] threshold: 0,
    // [ts] _comparator: 'ssim-cie94',
    // [ts] });
    // [ts] });
    // [ts] test('should fail', () => {
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png', {
    // [ts] threshold: 0,
    // [ts] _comparator: 'pixelmatch',
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].title, "should pass");
    // [ts] expect_eq(result.report.suites[0].specs[0].ok, true);
    // [ts] expect_eq(result.report.suites[0].specs[1].title, "should fail");
    // [ts] expect_eq(result.report.suites[0].specs[1].ok, false);
    // [ts] });
    // [ts] test("should respect comparator in config", () => {
    // [ts] const expected = fs.readFileSync(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-expected.png"));
    // [ts] const actual = fs.readFileSync(path.join(__dirname, "../image_tools/fixtures/should-match/tiny-antialiasing-sample/tiny-actual.png"));
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] snapshotPathTemplate: '__screenshots__/{testFilePath}/{arg}{ext}',
    // [ts] projects: [
    // [ts] {
    // [ts] name: 'should-pass',
    // [ts] expect: {
    // [ts] toMatchSnapshot: {
    // [ts] _comparator: 'ssim-cie94',
    // [ts] }
    // [ts] },
    // [ts] },
    // [ts] {
    // [ts] name: 'should-fail',
    // [ts] expect: {
    // [ts] toMatchSnapshot: {
    // [ts] _comparator: 'pixelmatch',
    // [ts] }
    // [ts] },
    // [ts] },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "__screenshots__/a.spec.js/snapshot.png": expected,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('test', () => {
    // [ts] expect(Buffer.from('${actual.toString('base64')}', 'base64')).toMatchSnapshot('snapshot.png', {
    // [ts] threshold: 0,
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].projectName, "should-pass");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[0].status, "expected");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[1].projectName, "should-fail");
    // [ts] expect_eq(result.report.suites[0].specs[0].tests[1].status, "unexpected");
    // [ts] });
    // [ts] test("should sanitize snapshot name when passed as string", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/-snapshot-.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');;
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('../../snapshot!.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should write missing expectations with sanitized snapshot name", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');;
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('../../snapshot!.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/-snapshot-.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), "Hello world");
    // [ts] });
    // [ts] test("should join array of snapshot path segments without sanitizing", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/test/path/snapshot.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot(['test', 'path', 'snapshot.txt']);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use snapshotDir as snapshot base directory", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] snapshotDir: 'snaps',
    // [ts] };
    // [ts] `,
    // [ts] "snaps/a.spec.js-snapshots/snapshot.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use snapshotDir with path segments as snapshot directory", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] snapshotDir: 'snaps',
    // [ts] };
    // [ts] `,
    // [ts] "snaps/tests/a.spec.js-snapshots/test/path/snapshot.txt": `Hello world`,
    // [ts] "tests/a.spec.js": `
    // [ts] const { test, expect } = require('../helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot(['test', 'path', 'snapshot.txt']);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use snapshotDir with nested test suite and path segments", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] snapshotDir: 'snaps',
    // [ts] };
    // [ts] `,
    // [ts] "snaps/path/to/tests/a.spec.js-snapshots/path/to/snapshot.txt": `Hello world`,
    // [ts] "path/to/tests/a.spec.js": `
    // [ts] const { test, expect } = require('../../../helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot(['path', 'to', 'snapshot.txt']);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should use project snapshotDir over base snapshotDir", () => {
    // [ts] const result = runInlineTest({
    // [ts] "helper.ts": `
    // [ts] import { test as base } from '@playwright/test';
    // [ts] export { expect } from '@playwright/test';
    // [ts] export const test = base.extend({
    // [ts] auto: [ ({}, run, testInfo) => {
    // [ts] testInfo.snapshotSuffix = 'suffix';
    // [ts] run();
    // [ts] }, { auto: true } ]
    // [ts] });
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [
    // [ts] {
    // [ts] name: 'foo',
    // [ts] snapshotDir: 'project_snaps',
    // [ts] },
    // [ts] ],
    // [ts] snapshotDir: 'snaps',
    // [ts] };
    // [ts] `,
    // [ts] "project_snaps/a.spec.js-snapshots/test/path/snapshot-foo-suffix.txt": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot(['test', 'path', 'snapshot.txt']);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should update snapshot with array of path segments", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot(['test', 'path', 'snapshot.txt']);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/test/path/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), "Hello world");
    // [ts] });
    // [ts] test("should attach expected/actual/diff with snapshot path", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/test/path/snapshot.png":
    // [ts] Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+P+/HgAFhAJ/wlseKgAAAABJRU5ErkJggg==", "base64"),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('## ' + JSON.stringify(testInfo.attachments));
    // [ts] });
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVQYV2NgYAAAAAMAAWgmWQ0AAAAASUVORK5CYII==', 'base64')).toMatchSnapshot(['test', 'path', 'snapshot.png']);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const outputText = result.output;
    // [ts] const attachments = outputText.split("\n").filter(l => l.startsWith("## ")).map(l => l.substring(3)).map(l => JSON.parse(l))[0];
    // [ts] for (const attachment of attachments) {
    // [ts] attachment.path = attachment.path.replace("\\\\", "/").replace(".*test-results\\/", "");
    // [ts] attachment.name = attachment.name.replace("\\\\", "/");
    // [ts] }
    // [ts] expect(attachments).toEqual([
    // [ts] {
    // [ts] name: "test/path/snapshot-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "golden-should-attach-expected-actual-diff-with-snapshot-path-playwright-test/a.spec.js-snapshots/test/path/snapshot.png"
    // [ts] },
    // [ts] {
    // [ts] name: "test/path/snapshot-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "a-is-a-test/test/path/snapshot-actual.png"
    // [ts] },
    // [ts] {
    // [ts] name: "test/path/snapshot-diff.png",
    // [ts] contentType: "image/png",
    // [ts] path: "a-is-a-test/test/path/snapshot-diff.png"
    // [ts] }
    // [ts] ]);
    // [ts] });
    // [ts] test("should attach expected/actual/diff", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png":
    // [ts] Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+P+/HgAFhAJ/wlseKgAAAABJRU5ErkJggg==", "base64"),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('## ' + JSON.stringify(testInfo.attachments));
    // [ts] });
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVQYV2NgYAAAAAMAAWgmWQ0AAAAASUVORK5CYII==', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const outputText = result.output;
    // [ts] const attachments = outputText.split("\n").filter(l => l.startsWith("## ")).map(l => l.substring(3)).map(l => JSON.parse(l))[0];
    // [ts] for (const attachment of attachments)
    // [ts] attachment.path = attachment.path.replace("\\\\", "/").replace(".*test-results\\/", "");
    // [ts] expect(attachments).toEqual([
    // [ts] {
    // [ts] name: "snapshot-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "golden-should-attach-expected-actual-diff-playwright-test/a.spec.js-snapshots/snapshot.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "a-is-a-test/snapshot-actual.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot-diff.png",
    // [ts] contentType: "image/png",
    // [ts] path: "a-is-a-test/snapshot-diff.png"
    // [ts] }
    // [ts] ]);
    // [ts] });
    // [ts] test("should attach expected/actual/diff for different sizes", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.png":
    // [ts] Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAYAAABytg0kAAAAEklEQVR42mP8z8AARAwMjDAGACwBA/9IB8FMAAAAAElFTkSuQmCC", "base64"),
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test.afterEach(({}, testInfo) => {
    // [ts] console.log('## ' + JSON.stringify(testInfo.attachments));
    // [ts] });
    // [ts] test('is a test', () => {
    // [ts] expect(Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVQYV2NgYAAAAAMAAWgmWQ0AAAAASUVORK5CYII==', 'base64')).toMatchSnapshot('snapshot.png');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const outputText = result.output;
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "Expected an image 2px by 2px, received 1px by 1px.") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, outputText, "4 pixels (ratio 1.00 of all image pixels) are different.") != null, true);
    // [ts] const attachments = outputText.split("\n").filter(l => l.startsWith("## ")).map(l => l.substring(3)).map(l => JSON.parse(l))[0];
    // [ts] for (const attachment of attachments)
    // [ts] attachment.path = attachment.path.replace(testInfo.outputDir, "").substring(1).replace("\\\\", "/");
    // [ts] expect(attachments).toEqual([
    // [ts] {
    // [ts] name: "snapshot-expected.png",
    // [ts] contentType: "image/png",
    // [ts] path: "a.spec.js-snapshots/snapshot.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot-actual.png",
    // [ts] contentType: "image/png",
    // [ts] path: "test-results/a-is-a-test/snapshot-actual.png"
    // [ts] },
    // [ts] {
    // [ts] name: "snapshot-diff.png",
    // [ts] contentType: "image/png",
    // [ts] path: "test-results/a-is-a-test/snapshot-diff.png"
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test("should fail with missing expectations and retries", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default { retries: 1 };
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), "Hello world");
    // [ts] });
    // [ts] test("should update expectations with retries", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default { retries: 1 };
    // [ts] `,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `A snapshot doesn't exist at ${snapshotOutputPath}, writing actual`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), "Hello world");
    // [ts] });
    // [ts] test("should allow comparing text with text without file extension", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot-no-extension": `Hello world`,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('Hello world').toMatchSnapshot('snapshot-no-extension');
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] });
    // [ts] test("should throw if a Promise was passed to toMatchSnapshot", () => {
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect(() => expect(new Promise(() => {})).toMatchSnapshot('foobar')).toThrow(/An unresolved Promise was passed to toMatchSnapshot\\(\\), make sure to resolve it by adding to it./);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] });
    // [ts] test("should respect update snapshot option from config", () => {
    // [ts] const EXPECTED_SNAPSHOT = "Hello world";
    // [ts] const ACTUAL_SNAPSHOT = "Hello world updated";
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "a.spec.js-snapshots/snapshot.txt": EXPECTED_SNAPSHOT,
    // [ts] "a.spec.js": `
    // [ts] const { test, expect } = require('./helper');
    // [ts] test('is a test', () => {
    // [ts] expect('${ACTUAL_SNAPSHOT}').toMatchSnapshot('snapshot.txt');
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const snapshotOutputPath = testInfo.outputPath("a.spec.js-snapshots/snapshot.txt");
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, `${snapshotOutputPath} does not match, writing actual.`) != null, true);
    // [ts] const data = fs.readFileSync(snapshotOutputPath);
    // [ts] expect_eq(data.toString(), ACTUAL_SNAPSHOT);
    // [ts] });
}
