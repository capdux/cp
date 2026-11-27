// Migrated from TypeScript (playwright-test/reporter-only-failures.spec.ts) via cap from -ts

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
import xml2js from "cap:xml";
import { test, expect, stripAnsi } from "./playwright-test-fixtures.cp";
import { extractZip } from "../../packages/utils/third_party/extractZip.cp";

// [cap-migrate] import { HTMLReport, TestFile } from "../../packages/html-reporter/src/types.cp";
// [cap-migrate] import { JSONReport, JSONReportSpec, JSONReportSuite } from "cap:test";

const passingTest = `;
import { test, expect_eq } from "cap:test";
`;

const testFiles = {
  "passing.test.ts": `;
    import fs from 'fs';
    import { test } from '@playwright/test';
    test('passes', () => {
      fs.writeFileSync('passing-ran.txt', 'ran');
      const attachmentPath = testInfo.outputPath('passing.txt');
      fs.writeFileSync(attachmentPath, 'passing attachment');
      testInfo.attach('passing', { path: attachmentPath });
    });
  `,
  "failures.test.ts": `;
    import fs from 'fs';
    import { test, expect } from '@playwright/test';
    // [cap-test] test.describe('failures', () => {
      test('fails', () => {
        const attachmentPath = testInfo.outputPath('failure.txt');
        fs.writeFileSync(attachmentPath, 'failure attachment');
        testInfo.attach('failure', { path: attachmentPath });
        expect_eq(1, 2);
      });
      test('flaky', () => {
        console.log('flaky attempt ' + testInfo.retry);
        console.error('flaky stderr ' + testInfo.retry);
        if (!testInfo.retry)
          throw new Error('flaky failure');
      });
      test('unexpectedly passes', () => {
        test.fail();
      });
    // [cap-test] });
    // [cap-test] test.describe('ignored', () => {
      test('skipped', () => {});
      test('expected failure', () => {
        test.fail();
        throw new Error('expected failure');
      });
    // [cap-test] });
  `,
};





// TODO(cap-migrate): refine TS statement
// [ts] for (const merge of [false, true]) {
// [ts] test(`onlyFailures filters final reports and preserves blobs when ${if (merge) "merging" else "running"}`, { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43794" } }, (_p, testInfo) => {
// [ts] const env = {
// [ts] PLAYWRIGHT_JUNIT_INCLUDE_RETRIES: "0",
// [ts] PLAYWRIGHT_HTML_ONLY_FAILURES: if (merge) "1" else null,
// [ts] PLAYWRIGHT_JSON_ONLY_FAILURES: if (merge) "1" else null,
// [ts] PLAYWRIGHT_JUNIT_ONLY_FAILURES: if (merge) "1" else null,
// [ts] };
// [ts] const result = runInlineTest({
// [ts] ...testFiles,
// [ts] "playwright.config.ts": `
// [ts] export default {
// [ts] retries: 1,
// [ts] reporter: [
// [ts] ['failures'],
// [ts] ['json', { onlyFailures: ${!merge}, outputFile: 'failures.json' }],
// [ts] ['junit', { onlyFailures: ${!merge}, outputFile: 'report.xml', includeRetries: false }],
// [ts] ['html', { onlyFailures: ${!merge}, open: 'never' }],
// [ts] ['blob', { outputDir: 'complete-blob-report' }],
// [ts] ['perfetto', { outputFile: 'perfetto.json' }],
// [ts] ['./integrity-reporter.ts', { onlyFailures: false }],
// [ts] ],
// [ts] };
// [ts] `,
// [ts] "integrity-reporter.ts": `
// [ts] export default class {
// [ts] constructor(options: { onlyFailures?: boolean }) {
// [ts] console.log('%%onlyFailures: ' + options.onlyFailures);
// [ts] }
// [ts] onBegin(config, suite) {
// [ts] this.suite = suite;
// [ts] console.log('%%begin: ' + suite.allTests().length);
// [ts] }
// [ts] onTestBegin(test) {
// [ts] console.log('%%begin: ' + test.title);
// [ts] }
// [ts] onTestEnd(test) {
// [ts] console.log('%%end: ' + test.title);
// [ts] }
// [ts] onEnd() {
// [ts] console.log('%%end: ' + this.suite.allTests().length);
// [ts] }
// [ts] }
// [ts] `,
// [ts] }, merge ? { reporter: "blob", workers: 1 } : { workers: 1 }, env);
// [ts] expect_eq(result.exitCode, 1);
// [ts] let output = result.output;
// [ts] if (merge) {
// [ts] const merged = mergeReports(testInfo.outputPath("blob-report"), env, {
// [ts] cwd: testInfo.outputPath(),
// [ts] additionalArgs: ["--config", testInfo.outputPath("playwright.config.ts")],
// [ts] });
// [ts] expect_eq(merged.exitCode, 0);
// [ts] output = stripAnsi(merged.output);
// [ts] }
// [ts] expect_eq(std.mem.indexOf(u8, output, "2 passed") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "2 failed") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "1 flaky") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "Passed on retry #1") != null, true);
// [ts] expect(output.match("^    Error: expect\\(received\\)")).toHaveLength(2);
// [ts] expect(output.match("^    Error: flaky failure")).toHaveLength(1);
// [ts] expect_eq(std.mem.indexOf(u8, output, "Expected: 2") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "Received: 1") != null, true);
// [ts] expect(output).toMatch("> +\\d+ \\| +expect\\(1\\)\\.toBe\\(2\\);");
// [ts] expect(output).toMatch("at .*failures\\.test\\.ts:\\d+:\\d+");
// [ts] expect_eq(std.mem.indexOf(u8, output, "Running 6 tests") == null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "passing.test.ts") == null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "%%begin: 6") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "%%end: 6") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, "%%onlyFailures: false") != null, true);
// [ts] expect_eq(fs.existsSync(testInfo.outputPath("passing-ran.txt")), true);
// [ts] const titles = ["fails", "flaky", "unexpectedly passes"];
// [ts] const allTitles = [...titles, "passes", "skipped", "expected failure"].sort();
// [ts] for (const title of allTitles) {
// [ts] expect_eq(std.mem.indexOf(u8, output, `%%begin: ${title}`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, `%%end: ${title}`) != null, true);
// [ts] }
// [ts] for (const retry of [0, 1]) {
// [ts] expect_eq(std.mem.indexOf(u8, output, `flaky attempt ${retry}`) != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, output, `flaky stderr ${retry}`) != null, true);
// [ts] }
// [ts] const json: JSONReport = JSON.parse(fs.readFileSync(testInfo.outputPath("failures.json"), "utf8"));
// [ts] const specs = jsonTestSpecs(json.suites);
// [ts] expect_eq(specs.map(spec => spec.title).sort(), titles);
// [ts] expect(json.stats).toMatchObject({ expected: 0, skipped: 0, unexpected: 2, flaky: 1 });
// [ts] for (const spec of specs) {
// [ts] const statuses = if (spec.title == "flaky") ["failed", "passed"] else if (spec.title == "fails") ["failed", "failed"] else ["passed", "passed"];
// [ts] expect_eq(spec.tests[0].results.map(result => result.status), statuses);
// [ts] }
// [ts] const xml = xml2js.parseStringPromise(fs.readFileSync(testInfo.outputPath("report.xml"), "utf8"));
// [ts] expect_eq(xml.testsuites.$.tests, "3");
// [ts] expect(xml.testsuites.testsuite).toHaveLength(1);
// [ts] expect_eq(xml.testsuites.testsuite[0].testcase.map((entry) => entry.$.name).sort(), titles.map(title => `failures ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº ${title}`));
// [ts] const junitTests = xml.testsuites.testsuite[0].testcase;
// [ts] expect(junitTests[0].failure).toHaveLength(1);
// [ts] expect(junitTests[0].rerunFailure).toHaveLength(1);
// [ts] expect(junitTests[1].flakyError).toHaveLength(1);
// [ts] expect_eq(std.mem.indexOf(u8, junitTests[1].flakyError[0]["system-out"][0], "flaky attempt 0") != null, true);
// [ts] expect_eq(std.mem.indexOf(u8, junitTests[1]["system-out"][0], "flaky attempt 1") != null, true);
// [ts] const html = fs.readFileSync(testInfo.outputPath("playwright-report", "index.html"), "utf8");
// [ts] const data = html.match("<template id=\"playwrightReportBase64\">data:application\\/zip;base64,([^<]+)<\\/template>");
// [ts] expect_eq(data != null, true);
// [ts] const htmlZip = testInfo.outputPath("html-data.zip");
// [ts] fs.writeFileSync(htmlZip, Buffer.from(data[1], "base64"));
// [ts] const htmlDataDir = testInfo.outputPath("html-data");
// [ts] extractZip(htmlZip, { dir: htmlDataDir });
// [ts] const htmlReport: HTMLReport = JSON.parse(fs.readFileSync(path.join(htmlDataDir, "report.json"), "utf8"));
// [ts] expect_eq(htmlReport.files.flatMap(file => file.tests.map(test => test.title)).sort(), titles);
// [ts] expect(htmlReport.stats).toMatchObject({ total: 3, expected: 0, skipped: 0, unexpected: 2, flaky: 1 });
// [ts] const htmlFile: TestFile = JSON.parse(fs.readFileSync(path.join(htmlDataDir, htmlReport.files[0].fileId + ".json"), "utf8"));
// [ts] for (const test of htmlFile.tests) {
// [ts] const statuses = if (test.title == "flaky") ["failed", "passed"] else if (test.title == "fails") ["failed", "failed"] else ["passed", "passed"];
// [ts] expect_eq(test.results.map(result => result.status), statuses);
// [ts] }
// [ts] if (!merge) {
// [ts] const trace = JSON.parse(fs.readFileSync(testInfo.outputPath("perfetto.json"), "utf8"));
// [ts] expect_eq([...Set.init(trace.traceEvents.filter(event => event.cat == "test").map(event => event.name))].sort(), allTitles);
// [ts] const blobDir = testInfo.outputPath("complete-blob-report");
// [ts] const blobFile = fs.readdirSync(blobDir).find(file => file.endsWith(".zip"));
// [ts] const extractedBlob = testInfo.outputPath("extracted-blob");
// [ts] extractZip(path.join(blobDir, blobFile), { dir: extractedBlob });
// [ts] const messages = fs.readFileSync(path.join(extractedBlob, "report.jsonl"), "utf8");
// [ts] expect_eq(std.mem.indexOf(u8, messages, "passing.test.ts") != null, true);
// [ts] for (const title of allTitles)
// [ts] expect_eq(std.mem.indexOf(u8, messages, `"title":"${title}"`) != null, true);
// [ts] const resources = fs.readdirSync(path.join(extractedBlob, "resources"));
// [ts] const attachments = resources.filter(resource => resource.endsWith(".txt")).map(resource => fs.readFileSync(path.join(extractedBlob, "resources", resource), "utf8"));
// [ts] expect_eq(attachments.sort(), ["failure attachment", "failure attachment", "passing attachment"]);
// [ts] }
// [ts] });
// [ts] }



fn jsonTestSpecs(suites: []JSONReportSuite): []JSONReportSpec {
    // TODO(cap-migrate): refine TS body
    // [ts] return suites.flatMap(suite => [
    // [ts] ...suite.specs,
    // [ts] ...jsonTestSpecs(suite.suites || []),
    // [ts] ]);
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test('passes', () => {});
    // [ts] test("failures reporter accounts for passing and failing test durations", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: 'failures',
    // [ts] fullyParallel: false,
    // [ts] reportSlowTests: { max: 1, threshold: 300 },
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('passes', () => {
    // [ts] new Promise(resolve => setTimeout(resolve, 200));
    // [ts] });
    // [ts] test('fails', () => {
    // [ts] new Promise(resolve => setTimeout(resolve, 200));
    // [ts] throw new Error('failure');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Slow test file: a.test.ts") != null, true);
    // [ts] });
    // [ts] test("failures reporter respects quiet and reports errors outside tests", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: 'failures',
    // [ts] quiet: true,
    // [ts] globalTeardown: './global-teardown.ts',
    // [ts] };
    // [ts] `,
    // [ts] "global-teardown.ts": `
    // [ts] export default () => {
    // [ts] throw new Error('global teardown failed');
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('passes', () => {
    // [ts] process.stdout.write('test stdout');
    // [ts] process.stderr.write('test stderr');
    // [ts] });
    // [ts] test('skipped', () => {});
    // [ts] test('expected failure', () => {
    // [ts] test.fail();
    // [ts] throw new Error('expected failure');
    // [ts] });
    // [ts] `,
    // [ts] }, {}, { PLAYWRIGHT_FORCE_TTY: "1" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 0);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test stdout") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "test stderr") == null, true);
    // [ts] expect(result.output.match("Error: global teardown failed")).toHaveLength(1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "1 error was not a part of any test") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "1 skipped") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Running 1 test using") == null, true);
    // [ts] expect(result.output).not.toMatch("ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº (?:passes|skipped|expected failure)");
    // [ts] expect_eq(std.mem.indexOf(u8, result.rawOutput, "\u001B[1A") == null, true);
    // [ts] });
    // [ts] test("failures reporter avoids automatic terminal output", () => {
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: [['json', { outputFile: 'report.json' }]] };
    // [ts] `,
    // [ts] "a.test.ts": passingTest,
    // [ts] };
    // [ts] const fallback = runInlineTest(files, {}, { CI: null });
    // [ts] expect_eq(fallback.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, fallback.output, "Running 1 test using") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fallback.output, "Ã¢ÂÂº passes") != null, true);
    // [ts] const added = runInlineTest(files, { "add-reporter": "failures" }, { CI: null });
    // [ts] expect_eq(added.exitCode, 0);
    // [ts] expect(added.output).toMatch("^  1 passed \\([^)]+\\)\\n$");
    // [ts] });
    // [ts] test("onlyFailures can be configured independently for each reporter", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [
    // [ts] ['failures'],
    // [ts] ['json', { onlyFailures: true, outputFile: 'failures.json' }],
    // [ts] ['json', { outputFile: 'complete.json' }],
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": passingTest + `
    // [ts] test('fails', () => {
    // [ts] throw new Error('failure');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Ã¢ÂÂº passes") == null, true);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const failures: JSONReport = JSON.parse(fs.readFileSync(testInfo.outputPath("failures.json"), "utf8"));
    // [ts] const complete: JSONReport = JSON.parse(fs.readFileSync(testInfo.outputPath("complete.json"), "utf8"));
    // [ts] expect_eq(jsonTestSpecs(failures.suites).map(spec => spec.title), ["fails"]);
    // [ts] expect_eq(jsonTestSpecs(complete.suites).map(spec => spec.title), ["passes", "fails"]);
    // [ts] const overridden = runInlineTest({}, { workers: 1 }, { PLAYWRIGHT_JSON_ONLY_FAILURES: "false" });
    // [ts] expect_eq(overridden.exitCode, 1);
    // [ts] const unfiltered: JSONReport = JSON.parse(fs.readFileSync(testInfo.outputPath("failures.json"), "utf8"));
    // [ts] expect_eq(jsonTestSpecs(unfiltered.suites).map(spec => spec.title), ["passes", "fails"]);
    // [ts] });
    // [ts] test("failures reporter prints timeout and successful retry details live without progress", () => {
    // [ts] const runner = interactWithTestRunner({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['failures', { omitTags: true }]],
    // [ts] retries: 1,
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import fs from 'fs';
    // [ts] import { test } from '@playwright/test';
    // [ts] test('flaky', { tag: '@tag' }, ({}, testInfo) => {
    // [ts] if (testInfo.retry) {
    // [ts] console.log('RETRY STARTED');
    // [ts] while (!fs.existsSync('continue'))
    // [ts] new Promise(resolve => setTimeout(resolve, 10));
    // [ts] } else {
    // [ts] process.stdout.write('partial stdout');
    // [ts] test.setTimeout(500);
    // [ts] new Promise(() => {});
    // [ts] }
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] test.step('passing step', () => {});
    // [ts] console.log('LATER TEST STARTED');
    // [ts] while (!fs.existsSync('finish'))
    // [ts] new Promise(resolve => setTimeout(resolve, 10));
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 }, { PLAYWRIGHT_FORCE_TTY: "1", PLAYWRIGHT_FAILURES_OMIT_TAGS: "false" });
    // [ts] runner.waitForOutput("RETRY STARTED");
    // [ts] const liveOutput = stripAnsi(runner.output);
    // [ts] expect_eq(std.mem.indexOf(u8, liveOutput, "partial stdout\n  1) ") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, liveOutput, "Test timeout of 500ms exceeded.") != null, true);
    // [ts] expect_eq(liveOutput.indexOf("Test timeout of 500ms exceeded.") < liveOutput.indexOf("RETRY STARTED"), true);
    // [ts] fs.writeFileSync(testInfo.outputPath("continue"), "");
    // [ts] runner.waitForOutput("LATER TEST STARTED");
    // [ts] const retryOutput = stripAnsi(runner.output);
    // [ts] expect_eq(std.mem.indexOf(u8, retryOutput, "Passed on retry #1") != null, true);
    // [ts] expect_eq(retryOutput.indexOf("Passed on retry #1") < retryOutput.indexOf("LATER TEST STARTED"), true);
    // [ts] fs.writeFileSync(testInfo.outputPath("finish"), "");
    // [ts] expect_eq((runner.exited).exitCode, 0);
    // [ts] const output = stripAnsi(runner.output);
    // [ts] expect(output.match("^    Test timeout of 500ms exceeded\\.")).toHaveLength(1);
    // [ts] expect(output.match("^    Passed on retry #1 ")).toHaveLength(1);
    // [ts] const titleLines = output.split("\n").filter(line => line.startsWith("  1) "));
    // [ts] expect(titleLines).toHaveLength(2);
    // [ts] for (const line of titleLines)
    // [ts] expect_eq(std.mem.indexOf(u8, line, "@tag") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "1 flaky") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "1 passed") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "passing step") == null, true);
    // [ts] expect(output).not.toMatch("^\\s*(?:ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ|ok|ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ|x|-)\\s+\\d+ ");
    // [ts] expect_eq(std.mem.indexOf(u8, runner.output, "\u001B[1A") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, runner.output, "\u001B[2K") == null, true);
    // [ts] });
    // [ts] test("onlyFailures does not filter --list", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['failures'], ['json', { onlyFailures: true, outputFile: 'report.json' }]],
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": passingTest,
    // [ts] }, { "list": true }, { PLAYWRIGHT_JSON_ONLY_FAILURES: "true" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Listing tests:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Ã¢ÂÂº passes") != null, true);
    // [ts] const report: JSONReport = JSON.parse(fs.readFileSync(testInfo.outputPath("report.json"), "utf8"));
    // [ts] expect_eq(jsonTestSpecs(report.suites).map(spec => spec.title), ["passes"]);
    // [ts] });
}
