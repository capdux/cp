// Migrated from TypeScript (playwright-test/reporter-junit.spec.ts) via cap from -ts

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

import xml2js from "cap:xml";
import path from "cap:path";
import { test, expect } from "./playwright-test-fixtures.cp";
import fs from "cap:fs";

// TODO(cap-migrate): refine TS statement
// [ts] for (const useIntermediateMergeReport of [false, true]) { test.describe(`${if (useIntermediateMergeReport) "merged" else "created"}`, () => { test.use({ useIntermediateMergeReport });  test("should render expected", (_p) => { const result = runInlineTest({ "a.test.js": ` import { test, expect } from '@playwright/test'; test('one', async ({}) => { expect(1).toBe(1); }); `, "b.test.js": ` import { test, expect } from '@playwright/test'; test('two', async ({}) => { expect(1).toBe(1); }); `, }, { reporter: "junit" }); const xml = parseXML(result.output); expect_eq(xml["testsuites"]["$"]["tests"], "2"); expect_eq(xml["testsuites"]["$"]["failures"], "0"); expect_eq(xml["testsuites"]["testsuite"].length, 2); expect_eq(xml["testsuites"]["testsuite"][0]["$"]["name"], "a.test.js"); expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "1"); expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["skipped"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["name"], "b.test.js");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render unexpected", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["$"]["failures"], "1");
// [ts] expect_eq(xml["testsuites"]["$"]["errors"], "0");
// [ts] const failure = xml["testsuites"]["testsuite"][0]["testcase"][0]["failure"][0];
// [ts] expect_eq(std.mem.indexOf(u8, failure["$"]["message"], "expect(received).toBe(expected)") != null, true);
// [ts] expect_eq(failure["$"]["type"], "expect.toBe");
// [ts] expect_eq(std.mem.indexOf(u8, failure["_"], "expect(1).toBe(0)") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should render thrown error as <error> element", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] throw new Error('Boom');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["$"]["errors"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["errors"], "1");
// [ts] const error = xml["testsuites"]["testsuite"][0]["testcase"][0]["error"][0];
// [ts] expect_eq(error["$"]["message"], "Boom");
// [ts] expect_eq(error["$"]["type"], "Error");
// [ts] expect_eq(std.mem.indexOf(u8, error["_"], "Boom") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should render TypeError as <error> element with correct type", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] const obj = null;
// [ts] obj.foo();
// [ts] });
// [ts] `,
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["$"]["errors"], "1");
// [ts] expect_eq(xml["testsuites"]["$"]["failures"], "0");
// [ts] const error = xml["testsuites"]["testsuite"][0]["testcase"][0]["error"][0];
// [ts] expect_eq(error["$"]["type"], "TypeError");
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should render unexpected after retry", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] `,
// [ts] }, { retries: 3, reporter: "junit" });
// [ts] expect_eq(std.mem.indexOf(u8, result.output, `tests="1"`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, `failures="1"`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, `<failure`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "Retry #1") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "Retry #2") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "Retry #3") != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should render flaky", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(testInfo.retry, 3);
// [ts] });
// [ts] `,
// [ts] }, { retries: 3, reporter: "junit" });
// [ts] expect_eq(std.mem.indexOf(u8, result.output, "Retry #1") == null, true);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render stdout", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44292" } }, (_p) => {
// [ts] const result = runInlineTest({
// [ts] "a.test.ts": `
// [ts] import colors from 'colors/safe';
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] console.log(colors.yellow('Hello world'));
// [ts] console.log('Hello again');
// [ts] console.error('My error');
// [ts] console.error('\\0'); // null control character
// [ts] const bytes = Buffer.from('\\u{1f916}');
// [ts] for (const stream of [process.stdout, process.stderr]) {
// [ts] stream.write(bytes.subarray(0, 2));
// [ts] stream.write(bytes.subarray(2));
// [ts] }
// [ts] test.expect_eq("abc", 'abcd');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["system-out"].length, 1);
// [ts] expect_eq(std.mem.indexOf(u8, testcase["system-out"][0], "[33mHello world[39m\nHello again") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, testcase["system-out"][0], "\u{1f916}") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, testcase["system-out"][0], "u00") == null, true);
// [ts] expect_eq(std.mem.indexOf(u8, testcase["system-err"][0], "My error") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, testcase["system-err"][0], "\u{1f916}") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, testcase["system-err"][0], "\u0000") == null, true); // null control character
// [ts] expect_eq(std.mem.indexOf(u8, testcase["failure"][0]["_"], `> 14 |             test.expect("abc").toBe('abcd');`) != null, true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] test("should render stdout without ansi escapes", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [ ['junit', { stripANSIControlSequences: true }] ],
// [ts] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import colors from 'colors/safe';
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] console.log(colors.yellow('Hello world'));
// [ts] });
// [ts] `,
// [ts] }, { reporter: "" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["system-out"].length, 1);
// [ts] expect_eq(testcase["system-out"][0].trim(), "Hello world");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render, by default, character data as CDATA sections", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] reporter: [ ['junit'] ],
// [ts] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] process.stdout.write('Hello world &"\\'<>]]>');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["system-out"].length, 1);
// [ts] expect_eq(testcase["system-out"][0].trim(), "Hello world &\"'<>]]&gt;");
// [ts] expect_eq(std.mem.indexOf(u8, result.output, `<system-out>\n<![CDATA[Hello world &"\'<>]]&gt;]]>\n</system-out>`) != null, true);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render skipped", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] console.log('Hello world');
// [ts] });
// [ts] test('two', () => {
// [ts] test();
// [ts] console.log('Hello world');
// [ts] });
// [ts] `,
// [ts] }, { retries: 3, reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "2");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["skipped"], "1");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should report skipped due to sharding", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] });
// [ts] test('two', () => {
// [ts] test();
// [ts] });
// [ts] `,
// [ts] "b.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('three', () => {
// [ts] });
// [ts] test('four', () => {
// [ts] test();
// [ts] });
// [ts] test('five', () => {
// [ts] });
// [ts] `,
// [ts] }, { shard: "1/3", reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["testsuite"].length, 1);
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "2");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["skipped"], "1");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should not render projects if they dont exist", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"].length, 1);
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["name"], "a.test.js");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["skipped"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["testcase"][0]["$"]["name"], "one");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["testcase"][0]["$"]["classname"], "a.test.js");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render projects", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { projects: [ { name: 'project1' }, { name: 'project2' } ] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["$"]["tests"], "2");
// [ts] expect_eq(xml["testsuites"]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"].length, 2);
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["name"], "a.test.js");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["hostname"], "project1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["skipped"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["testcase"][0]["$"]["name"], "one");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["testcase"][0]["$"]["classname"], "a.test.js");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["name"], "a.test.js");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["hostname"], "project2");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["skipped"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["testcase"][0]["$"]["name"], "one");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["testcase"][0]["$"]["classname"], "a.test.js");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should includeProjectInTestName", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] projects: [ { name: 'project1' }, { name: 'project2' } ],
// [ts] reporter: [['junit', { includeProjectInTestName: true }]]
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] const xml = parseXML(result.output);
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["name"], "a.test.js");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["hostname"], "project1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["testcase"][0]["$"]["name"], "[project1] one");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["name"], "a.test.js");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["hostname"], "project2");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][1]["testcase"][0]["$"]["name"], "[project2] one");
// [ts] });
// [ts] test("should add file attribute relative to fileRoot", {
// [ts] annotation: [
// [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/44099" },
// [ts] { type: "issue", description: "https://github.com/microsoft/playwright/issues/44225" },
// [ts] ],
// [ts] }, (_p) => {
// [ts] const files = {
// [ts] "packages/foo/playwright.config.ts": `
// [ts] module.exports = {
// [ts] testDir: './src',
// [ts] reporter: [['junit', { addFileAttribute: true, fileRoot: process.env.TEST_FILE_ROOT }]]
// [ts] };
// [ts] `,
// [ts] "packages/foo/src/auth/a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] };
// [ts] const run = (env) => {
// [ts] const result = runInlineTest(files, { reporter: "", config: "./packages/foo/playwright.config.ts" }, env);
// [ts] expect_eq(result.exitCode, 0);
// [ts] return parseXML(result.output)["testsuites"]["testsuite"][0]["testcase"][0]["$"];
// [ts] };
// [ts] const byDefault = run({});
// [ts] expect_eq(byDefault["classname"], path.join("auth", "a.test.js"));
// [ts] expect_eq(byDefault["file"], "src/auth/a.test.js");
// The option is relative to the config directory.
// [ts] expect_eq((run({ TEST_FILE_ROOT: "../.." }))["file"], "packages/foo/src/auth/a.test.js");
// The environment variable wins and is relative to cwd.
// [ts] expect_eq((run({ TEST_FILE_ROOT: "../..", PLAYWRIGHT_JUNIT_FILE_ROOT: "packages" }))["file"], "foo/src/auth/a.test.js");
// [ts] expect_eq((run({ PLAYWRIGHT_JUNIT_ADD_FILE_ATTRIBUTE: "0" }))["file"] == null, true);
// [ts] });
// [ts] test("should render existing attachments, but not missing ones", () => {
// [ts] test(useIntermediateMergeReport, "Blob report hashes attachment paths");
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test.use({ screenshot: 'on' });
// [ts] test('one', () => {
// [ts] page.setContent('hello');
// [ts] const file = testInfo.outputPath('file.txt');
// [ts] require('fs').writeFileSync(file, 'my file', 'utf8');
// [ts] testInfo.attachments.push({ name: 'my-file', path: file, contentType: 'text/plain' });
// [ts] testInfo.attachments.push({ name: 'my-file-missing', path: file + '-missing', contentType: 'text/plain' });
// [ts] console.log('log here');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["system-out"].length, 1);
// [ts] expect(testcase["system-out"][0].trim()).toBe([
// [ts] `log here`,
// [ts] `\n[[ATTACHMENT|test-results${path.sep}a-one${path.sep}file.txt]]`,
// [ts] `\n[[ATTACHMENT|test-results${path.sep}a-one${path.sep}test-finished-1.png]]`,
// [ts] ].join("\n"));
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render attachment paths relative to report file when report file name is specified", () => {
// [ts] test(useIntermediateMergeReport, "Blob report hashes attachment paths");
// [ts] const result = runInlineTest({
// [ts] "project/playwright.config.ts": `
// [ts] export default { reporter: [['junit', { outputFile: '../my-report/junit/a.xml' }]] };
// [ts] `,
// [ts] "project/a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test.use({ screenshot: 'on' });
// [ts] test('one', () => {
// [ts] page.setContent('hello');
// [ts] const file = testInfo.outputPath('file.txt');
// [ts] require('fs').writeFileSync(file, 'my file', 'utf8');
// [ts] testInfo.attachments.push({ name: 'my-file', path: file, contentType: 'text/plain' });
// [ts] console.log('log here');
// [ts] });
// [ts] `,
// [ts] }, { reporter: "", config: "./project/playwright.config.ts" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath(path.join("my-report", "junit", "a.xml"))), true);
// [ts] const xml = parseXML(fs.readFileSync(testInfo.outputPath(path.join("my-report", "junit", "a.xml"))).toString());
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["system-out"].length, 1);
// [ts] expect(testcase["system-out"][0].trim()).toBe([
// [ts] `log here`,
// [ts] `\n[[ATTACHMENT|..${path.sep}..${path.sep}test-results${path.sep}a-one${path.sep}file.txt]]`,
// [ts] `\n[[ATTACHMENT|..${path.sep}..${path.sep}test-results${path.sep}a-one${path.sep}test-finished-1.png]]`,
// [ts] ].join("\n"));
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] fn parseXML(xml: string): string {
// [ts] let result: any;
// [ts] xml2js.parseString(xml, (err, r) => result = r);
// [ts] return result;
// [ts] }
// [ts] test("should render annotations to custom testcase properties", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `export default { reporter: 'junit' };`,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] testInfo.annotations.push({ type: 'test_description', description: 'sample description' });
// [ts] });
// [ts] `
// [ts] }, { reporter: "" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["properties"], true);
// [ts] expect_eq(testcase["properties"][0]["property"].length, 1);
// [ts] expect_eq(testcase["properties"][0]["property"][0]["$"]["name"], "test_description");
// [ts] expect_eq(testcase["properties"][0]["property"][0]["$"]["value"], "sample description");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render built-in annotations to testcase properties", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `export default { reporter: 'junit' };`,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] test.slow();
// [ts] });
// [ts] `
// [ts] }, { reporter: "" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["properties"], true);
// [ts] expect_eq(testcase["properties"][0]["property"].length, 1);
// [ts] expect_eq(testcase["properties"][0]["property"][0]["$"]["name"], "slow");
// [ts] expect_eq(testcase["properties"][0]["property"][0]["$"]["value"], "");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should render all annotations to testcase value based properties, if requested", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] const xrayOptions = {
// [ts] embedAnnotationsAsProperties: true
// [ts] }
// [ts] export default {
// [ts] reporter: [ ['junit', xrayOptions] ],
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] testInfo.annotations.push({ type: 'test_id', description: '1234' });
// [ts] testInfo.annotations.push({ type: 'test_key', description: 'CALC-2' });
// [ts] testInfo.annotations.push({ type: 'test_summary', description: 'sample summary' });
// [ts] testInfo.annotations.push({ type: 'requirements', description: 'CALC-5,CALC-6' });
// [ts] });
// [ts] `
// [ts] }, { reporter: "" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["properties"], true);
// [ts] expect_eq(testcase["properties"][0]["property"].length, 4);
// [ts] expect_eq(testcase["properties"][0]["property"][0]["$"]["name"], "test_id");
// [ts] expect_eq(testcase["properties"][0]["property"][0]["$"]["value"], "1234");
// [ts] expect_eq(testcase["properties"][0]["property"][1]["$"]["name"], "test_key");
// [ts] expect_eq(testcase["properties"][0]["property"][1]["$"]["value"], "CALC-2");
// [ts] expect_eq(testcase["properties"][0]["property"][2]["$"]["name"], "test_summary");
// [ts] expect_eq(testcase["properties"][0]["property"][2]["$"]["value"], "sample summary");
// [ts] expect_eq(testcase["properties"][0]["property"][3]["$"]["name"], "requirements");
// [ts] expect_eq(testcase["properties"][0]["property"][3]["$"]["value"], "CALC-5,CALC-6");
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should not embed attachments to a custom testcase property, if not explicitly requested", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] const file = testInfo.outputPath('evidence1.txt');
// [ts] require('fs').writeFileSync(file, 'hello', 'utf8');
// [ts] testInfo.attachments.push({ name: 'evidence1.txt', path: file, contentType: 'text/plain' });
// [ts] testInfo.attachments.push({ name: 'evidence2.txt', body: Buffer.from('world'), contentType: 'text/plain' });
// await testInfo.attach('evidence1.txt', { path: file, contentType: 'text/plain' });
// await testInfo.attach('evidence2.txt', { body: Buffer.from('world'), contentType: 'text/plain' });
// [ts] });
// [ts] `
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["properties"], false);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [cap-test] test.describe("report location", () => {
// [ts] test("with config should create report relative to config", () => {
// [ts] test(useIntermediateMergeReport);
// [ts] const result = runInlineTest({
// [ts] "nested/project/playwright.config.ts": `
// [ts] export default { reporter: [['junit', { outputFile: '../my-report/a.xml' }]] };
// [ts] `,
// [ts] "nested/project/a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 1);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "", config: "./nested/project/playwright.config.ts" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath(path.join("nested", "my-report", "a.xml"))), true);
// [ts] });
// [ts] test("with env var should create relative to cwd", () => {
// [ts] const result = runInlineTest({
// [ts] "foo/package.json": `{ "name": "foo" }`,
// [ts] "foo/bar/baz/tests/a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] const fs = require('fs');
// [ts] test('pass', () => {
// [ts] });
// [ts] `
// [ts] }, { "reporter": "junit,line" }, { "PLAYWRIGHT_JUNIT_OUTPUT_NAME": "../my-report.xml" }, {
// [ts] cwd: "foo/bar/baz/tests",
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "my-report.xml")), true);
// [ts] });
// [ts] test("support PLAYWRIGHT_JUNIT_OUTPUT_FILE", () => {
// [ts] const result = runInlineTest({
// [ts] "foo/package.json": `{ "name": "foo" }`,
// [ts] "foo/bar/baz/tests/a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] const fs = require('fs');
// [ts] test('pass', () => {
// [ts] });
// [ts] `
// [ts] }, { "reporter": "junit,line" }, { "PLAYWRIGHT_JUNIT_OUTPUT_FILE": "../my-report.xml" }, {
// [ts] cwd: "foo/bar/baz/tests",
// [ts] });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "my-report.xml")), true);
// [ts] });
// [ts] test("PLAYWRIGHT_JUNIT_OUTPUT_FILE should take precedence over config", () => {
// [ts] const result = runInlineTest({
// [ts] "package.json": `{ "name": "foo" }`,
// [ts] "bar/playwright.config.js": `
// [ts] module.exports = {
// [ts] reporter: [['junit', { outputFile: 'results.xml' }], ['line', {}]],
// [ts] projects: [ {} ]
// [ts] };
// [ts] `,
// [ts] "bar/baz/tests/a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] const fs = require('fs');
// [ts] test('pass', () => {
// [ts] });
// [ts] `
// [ts] }, { "config": "bar/playwright.config.js" }, { "PLAYWRIGHT_JUNIT_OUTPUT_FILE": "bar/my-report.xml" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("bar", "my-report.xml")), true);
// [ts] });
// [ts] test("support PLAYWRIGHT_JUNIT_OUTPUT_DIR and PLAYWRIGHT_JUNIT_OUTPUT_NAME", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.js": `
// [ts] module.exports = { projects: [ {} ] };
// [ts] `,
// [ts] "tests/a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] const fs = require('fs');
// [ts] test('pass', () => {
// [ts] });
// [ts] `
// [ts] }, { "reporter": "junit,line" }, { "PLAYWRIGHT_JUNIT_OUTPUT_DIR": "foo/bar", "PLAYWRIGHT_JUNIT_OUTPUT_NAME": "baz/my-report.xml" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "my-report.xml")), true);
// [ts] });
// [ts] test("PLAYWRIGHT_JUNIT_OUTPUT_NAME should take precedence over config", () => {
// [ts] const result = runInlineTest({
// [ts] "package.json": `{ "name": "foo" }`,
// [ts] "bar/playwright.config.js": `
// [ts] module.exports = {
// [ts] reporter: [['junit', {}], ['line', {}]],
// [ts] projects: [ {} ]
// [ts] };
// [ts] `,
// [ts] "bar/baz/tests/a.spec.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] const fs = require('fs');
// [ts] test('pass', () => {
// [ts] });
// [ts] `
// [ts] }, { "config": "bar/playwright.config.js" }, { "PLAYWRIGHT_JUNIT_OUTPUT_NAME": "foo/my-report.xml" });
// [ts] expect_eq(result.exitCode, 0);
// [ts] expect_eq(result.passed, 1);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("bar", "foo", "my-report.xml")), true);
// [ts] });
// [cap-test] });
// [ts] test("testsuites time is test run wall time", () => {
// [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30518" });
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] new Promise(f => setTimeout(f, 1000));
// [ts] });
// [ts] `
// [ts] }, { reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// [ts] const time = xml["testsuites"]["$"]["time"];
// [ts] expect_eq(time, result.report.stats.duration / 1000);
// [ts] expect_eq(time > 1, true);
// [ts] });
// [ts] test("should emit flakyFailure for flaky tests when includeRetries is enabled", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] retries: 2,
// [ts] reporter: [['junit', { includeRetries: true }]]
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(testInfo.retry, 2);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "" });
// [ts] const xml = parseXML(result.output);
// Single testcase for the test; flaky tests count as passed.
// [ts] expect_eq(xml["testsuites"]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["$"]["failures"], "0");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "0");
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["$"]["name"], "one");
// No <failure> element — test eventually passed.
// [ts] expect_eq(testcase["failure"], false);
// Two <flakyFailure> elements for the two failed attempts.
// [ts] expect_eq(testcase["flakyFailure"].length, 2);
// [ts] expect_eq(testcase["flakyFailure"][0]["stackTrace"], true);
// [ts] expect_eq(testcase["flakyFailure"][1]["stackTrace"], true);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should not include retries by default", () => {
// [ts] const result = runInlineTest({
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(testInfo.retry, 1);
// [ts] });
// [ts] `,
// [ts] }, { retries: 1, reporter: "junit" });
// [ts] const xml = parseXML(result.output);
// Default behavior: single testcase, no flakyFailure elements.
// [ts] expect_eq(xml["testsuites"]["$"]["tests"], "1");
// [ts] const testcases = xml["testsuites"]["testsuite"][0]["testcase"];
// [ts] expect_eq(testcases.length, 1);
// [ts] expect_eq(testcases[0]["$"]["name"], "one");
// [ts] expect_eq(testcases[0]["flakyFailure"], false);
// [ts] expect_eq(result.exitCode, 0);
// [ts] });
// [ts] test("should emit rerunFailure for permanent failures when includeRetries is enabled", () => {
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] retries: 1,
// [ts] reporter: [['junit', { includeRetries: true }]]
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('one', () => {
// [ts] expect_eq(1, 0);
// [ts] });
// [ts] `,
// [ts] }, { reporter: "" });
// [ts] const xml = parseXML(result.output);
// Single testcase; permanent failure counts as 1 test with 1 failure.
// [ts] expect_eq(xml["testsuites"]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["$"]["failures"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["tests"], "1");
// [ts] expect_eq(xml["testsuites"]["testsuite"][0]["$"]["failures"], "1");
// [ts] const testcase = xml["testsuites"]["testsuite"][0]["testcase"][0];
// [ts] expect_eq(testcase["$"]["name"], "one");
// <failure> for the first attempt.
// [ts] expect_eq(testcase["failure"], true);
// <rerunFailure> for the retry.
// [ts] expect_eq(testcase["rerunFailure"].length, 1);
// [ts] expect_eq(testcase["rerunFailure"][0]["stackTrace"], true);
// [ts] expect_eq(result.exitCode, 1);
// [ts] });
// [ts] });
// [ts] }
