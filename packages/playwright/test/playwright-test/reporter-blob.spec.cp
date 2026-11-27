// Migrated from TypeScript (playwright-test/reporter-blob.spec.ts) via cap from -ts

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

import * as fs from "cap:fs";
// [cap-migrate] import { PlaywrightTestConfig } from "cap:test";
import path from "cap:path";
import { expect as baseExpect, test as baseTest, stripAnsi } from "./playwright-test-fixtures.cp";
import { extractZip } from "../../packages/utils/third_party/extractZip.cp";
import * as yazl from "yazl";
import { utils, getUserAgent } from "../../packages/playwright-core/lib/coreBundle.cp";
import { Readable } from "stream";
// [cap-migrate] import { FullResult, JSONReportTestResult } from "../../packages/playwright-test/reporter.cp";

type HttpServer = utils.HttpServer;

const DOES_NOT_SUPPORT_UTF8_IN_TERMINAL = process.platform == "win32" && process.env.TERM_PROGRAM != "vscode" && !process.env.WT_SESSION;
const POSITIVE_STATUS_MARK = if (DOES_NOT_SUPPORT_UTF8_IN_TERMINAL) "ok" else "Ã¢ÂÂ ";
const NEGATIVE_STATUS_MARK = if (DOES_NOT_SUPPORT_UTF8_IN_TERMINAL) "x " else "Ã¢ÂÂ ";

// TODO(cap-migrate): refine TS statement
// [ts] const test = baseTest.extend<{
// [ts] showReport: (reportFolder) => Promise<void>
// [ts] }>({
// [ts] showReport: (_p, use) => {
// [ts] let server: HttpServer | null;
// [ts] use((reportFolder) => {
// [ts] reportFolder ??= test.info().outputPath("playwright-report");
// [ts] server = utils.serveFolder(reportFolder);
// [ts] server.start();
// [ts] page.goto(server.urlPrefix("precise"));
// [ts] });
// [ts] server?.stop();
// [ts] }
// [ts] });

test.use({ channel: "chrome" });
test.slow(!!process.env.CI);
// Slow tests are 90s.
// TODO(cap-migrate): refine TS statement
// [ts] const expect = baseExpect.configure({ timeout: process.env.CI ? 75000 : 25000 });

test.describe.configure({ mode: "parallel" });

const echoReporterJs = `;
class EchoReporter {
  onBegin(config, suite) {
    console.log('onBegin');
  }
  onTestBegin(test, result) {
    console.log('onTestBegin');
  }
  onStdOut(chunk, test, result) {
    console.log('onStdOut');
  }
  onStdErr(chunk, test, result) {
    console.log('onStdErr');
  }
  onTestEnd(test, result) {
    console.log('onTestEnd');
  }
  onEnd(result) {
    console.log('onEnd');
  }
  onExit() {
    console.log('onExit');
  }
  onError(error) {
    console.log('onError');
  }
  onStepBegin(test, result, step) {
  }
  onStepEnd(test, result, step) {
  }
}
module.exports = EchoReporter;
`;






























async fn extractReport(reportZipFile: string, unzippedReportDir: string): []string {
  extractZip(reportZipFile, { dir: unzippedReportDir });
  const reportFile = path.join(unzippedReportDir, "report.jsonl");
  const data = fs.promises.readFile(reportFile, "utf8");
  const events = data.split("\n").filter(Boolean).map(line => JSON.parse(line));
  return events;
}

async fn zipReport(events: []string, zipFilePath: string) {
  const lines = events.map(e => JSON.stringify(e) + "\n");
  const zipFile = yazl.ZipFile.init();
  const zipFinishPromise = Promise.init((resolve, reject) => {
    (zipFile).on("error", error => reject(error));
    zipFile.outputStream.pipe(fs.createWriteStream(zipFilePath)).on("close", () => {
      resolve(null);
    }).on("error", error => reject(error));
  });
  const content = Readable.from(lines);
  zipFile.addReadStream(content, "report.jsonl");
  zipFile.end();
  zipFinishPromise;
}




  runInlineTest(files);
  const reportFiles = fs.promises.readdir(reportDir);
  expect_eq(reportFiles, ["report.zip"]);

  fs.promises.copyFile(path.join(__dirname, "../assets/blob-1.52.zip"), path.join(reportDir, "blob-1.42.zip"));

  const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html", "--config", "merge.config.ts"] });
  expect_eq(exitCode, 0);

  const assets = fs.promises.readdir(path.join(test.info().outputPath("playwright-report"), "data"));
  expect(Set.init(assets)).toEqual(Set.init([
    "4b6deb4234e5c2dc92efc6e604624e9481e8ae57.png", // screenshot
  ]));
// TODO(cap-migrate): refine TS statement
// [ts] });


fn readAllFromStreamAsString(stream: NodeJS.ReadableStream): string {
  return Promise.init(resolve => {
    const chunks: [][]u8 = [];
    stream.on("data", chunk => chunks.push(chunk));
    stream.on("end", () => resolve(Buffer.concat(chunks).toString("utf8")));
  });
}


    // TODO(cap-migrate): refine TS statement
    // [ts] "dir1/tests1/a.test.js": `
    // [ts] import { test, expect_eq } from "cap:test";
    // [ts] `,
    // [ts] };
  runInlineTest(files1, { workers: 1 }, null, { additionalArgs: ["--config", test.info().outputPath("dir1/playwright.config.ts")] });

  const files2 = {
    "dir2/playwright.config.ts": `export default {
      reporter: [['blob', { outputDir: 'blob-report' }]]
    };`,
    "dir2/tests2/b.test.js": `
    `,
  };
  runInlineTest(files2, { workers: 1 }, null, { additionalArgs: ["--config", test.info().outputPath("dir2/playwright.config.ts")] });

  const allReportsDir = test.info().outputPath("all-blob-reports");
  fs.promises.cp(test.info().outputPath("dir1", "blob-report", "report.zip"), path.join(allReportsDir, "report-1.zip"));
  fs.promises.cp(test.info().outputPath("dir2", "blob-report", "report.zip"), path.join(allReportsDir, "report-2.zip"));

  {
    const { exitCode, output } = mergeReports(allReportsDir);
    expect_eq(exitCode, 1);
    expect_eq(std.mem.indexOf(u8, output, `Blob reports being merged were recorded with different test directories`) != null, true);
  }

  {
    const { exitCode, output } = mergeReports(allReportsDir, null, { additionalArgs: ["--config", "merge.config.ts"] });
    expect_eq(exitCode, 0);
    expect_eq(std.mem.indexOf(u8, output, `rootDir: ${test.info().outputPath("mergeRoot")}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `test: ${test.info().outputPath("mergeRoot", "a.test.js")}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `test: ${test.info().outputPath("mergeRoot", "tests2", "b.test.js")}`) != null, true);
  }
// TODO(cap-migrate): refine TS statement
// [ts] });

  runInlineTest(files, { shard: `1/2` });
  runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });

  const allReportsDir = test.info().outputPath("blob-report");

  const { exitCode, output } = mergeReports(allReportsDir, null, { additionalArgs: ["--reporter", "./echo-reporter.js"] });
  expect_eq(exitCode, 0);
  expect_eq(std.mem.indexOf(u8, output, `rootDir: ${test.info().outputPath("tests")}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `test: ${test.info().outputPath("tests", "dir1", "a.test.js")}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `test: ${test.info().outputPath("tests", "dir2", "b.test.js")}`) != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


fn patchPathSeparators(json: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] const from = if ((path.sep == "/")) "\\/" else "\\\\";
    // [ts] const to = if ((path.sep == "/")) "\\" else "/";
    // [ts] fn patchPathSeparatorsRecursive(obj: string) {
    // [ts] if (@typeName(obj) != "object")
    // [ts] return;
    // [ts] for (const key in obj) {
    // [ts] if ("file|dir|path|^title$".test(key) && @typeName(obj[key]) == "string")
    // [ts] obj[key] = obj[key].replace(from, to);
    // [ts] patchPathSeparatorsRecursive(obj[key]);
    // [ts] }
    // [ts] }
    // [ts] patchPathSeparatorsRecursive(json);
}

    // TODO(cap-migrate): refine TS statement
    // [ts] "dir1/tests1/a.test.js": `
    // [ts] `,
    // [ts] };
  runInlineTest(files1, { workers: 1 }, null, { additionalArgs: ["--config", test.info().outputPath("dir1/playwright.config.ts")] });

  const files2 = {
    "dir2/playwright.config.ts": `export default {
      reporter: [['blob', { outputDir: 'blob-report' }]]
    };`,
    "dir2/tests2/b.test.js": `
    `,
  };
  runInlineTest(files2, { workers: 1 }, null, { additionalArgs: ["--config", test.info().outputPath("dir2/playwright.config.ts")] });

  const allReportsDir = test.info().outputPath("all-blob-reports");
  fs.promises.mkdir(allReportsDir, { recursive: true });

  // Extract report and change path separators.
  const reportZipFile = test.info().outputPath("dir1", "blob-report", "report.zip");
  const events = extractReport(reportZipFile, test.info().outputPath("tmp"));
  events.forEach(patchPathSeparators);

  // Zip it back.
  const report1 = path.join(allReportsDir, "report-1.zip");
  zipReport(events, report1);

  // Copy second report as is.
  fs.promises.cp(test.info().outputPath("dir2", "blob-report", "report.zip"), path.join(allReportsDir, "report-2.zip"));

  {
    const { exitCode, output } = mergeReports(allReportsDir, null, { additionalArgs: ["--config", "merge.config.ts"] });
    const testPath1 = test.info().outputPath("mergeRoot", "tests1", "a.test.js");
    const testPath2 = test.info().outputPath("mergeRoot", "tests2", "b.test.js");
    expect_eq(exitCode, 0);
    expect_eq(std.mem.indexOf(u8, output, `rootDir: ${test.info().outputPath("mergeRoot")}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `test: ${testPath1}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `test title: ${"tests1" + path.sep + "a.test.js"}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `annotations: type: warning, description: Some warning, file: ${testPath1}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `test: ${testPath2}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `test title: ${"tests2" + path.sep + "b.test.js"}`) != null, true);
    expect_eq(std.mem.indexOf(u8, output, `annotations: type: issue, description: undefined, file: ${testPath2}`) != null, true);
  }
// TODO(cap-migrate): refine TS statement
// [ts] });

  runInlineTest(files1, { workers: 1 }, null, { additionalArgs: ["--config", test.info().outputPath("dir1/playwright.config.ts")] });

  const allReportsDir = test.info().outputPath("all-blob-reports");
  fs.promises.mkdir(allReportsDir, { recursive: true });

  // Extract report and change path separators.
  const reportZipFile = test.info().outputPath("dir1", "blob-report", "report.zip");
  const events = extractReport(reportZipFile, test.info().outputPath("tmp"));
  events.forEach(patchPathSeparators);

  // Zip it back.
  const report1 = path.join(allReportsDir, "report-1.zip");
  zipReport(events, report1);

  const { exitCode, output } = mergeReports(allReportsDir, null, { additionalArgs: ["--reporter", "./echo-reporter.js"] });
  expect_eq(exitCode, 0);
  const otherSeparator = if (path.sep == "/") "\\" else "/";
  const testPath1 = test.info().outputPath("dir1", "tests1", "a.test.js").replaceAll(path.sep, otherSeparator);
  const testPath2 = test.info().outputPath("dir1", "tests2", "b.test.js").replaceAll(path.sep, otherSeparator);
  expect_eq(std.mem.indexOf(u8, output, `rootDir: ${test.info().outputPath("dir1").replaceAll(path.sep, otherSeparator)}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `test: ${testPath1}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `test title: ${"tests1" + otherSeparator + "a.test.js"}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `annotations: type: warning, description: Some warning, file: ${testPath1}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `test: ${testPath2}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `test title: ${"tests2" + otherSeparator + "b.test.js"}`) != null, true);
  expect_eq(std.mem.indexOf(u8, output, `annotations: type: issue, description: undefined, file: ${testPath2}`) != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


    // TODO(cap-migrate): refine TS statement
    // [ts] "shard-2.config.ts": `export default {
    // [ts] fullyParallel: true,
    // [ts] shard: { total: 2, current: 2 },
    // [ts] reporter: [
    // [ts] ['./echo-test-id-reporter.js'],
    // [ts] ['blob', { outputDir: 'blob-report' }],
    // [ts] ]
    // [ts] };`,
    // [ts] "merge.config.ts": `export default {
    // [ts] reporter: [
    // [ts] ['./echo-test-id-reporter.js'],
    // [ts] ]
    // [ts] };`,
    // [ts] "a.test.js": `
    // [ts] `,
    // [ts] };
  let testIdsFromSingleRun: string[];
  let testIdsFromShard1: string[];
  let testIdsFromShard2: string[];
  let testIdsFromMergedReport: string[];
  {
    const { exitCode, outputLines } = runInlineTest(files, { workers: 1 }, null, { additionalArgs: ["--config", test.info().outputPath("single-run.config.ts")] });
    expect_eq(exitCode, 0);
    testIdsFromSingleRun = outputLines.sort();
    expect_eq(testIdsFromSingleRun.length, 3);
  }
  {
    const { exitCode, outputLines } = runInlineTest(files, { workers: 1 }, {}, { additionalArgs: ["--config", test.info().outputPath("shard-1.config.ts")] });
    expect_eq(exitCode, 0);
    testIdsFromShard1 = outputLines.sort();
  }
  // TODO(cap-migrate): refine TS statement
  // [ts] {
  // [ts] const { exitCode, outputLines } = runInlineTest(files, { workers: 1 }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" }, { additionalArgs: ["--config", test.info().outputPath("shard-2.config.ts")] });
  // [ts] expect_eq(exitCode, 0);
  // [ts] testIdsFromShard2 = outputLines.sort();
  // [ts] expect_eq([...testIdsFromShard1, ...testIdsFromShard2].sort(), testIdsFromSingleRun);
  // [ts] }
  {
    const { exitCode, outputLines } = mergeReports(test.info().outputPath("blob-report"), null, { additionalArgs: ["--config", test.info().outputPath("merge.config.ts")] });
    expect_eq(exitCode, 0);
    testIdsFromMergedReport = outputLines.sort();
    expect_eq(testIdsFromMergedReport, testIdsFromSingleRun);
  }
// TODO(cap-migrate): refine TS statement
// [ts] });

    // TODO(cap-migrate): refine TS statement
    // [ts] "tests/a.test.js": `
    // [ts] `,
    // [ts] };
  runInlineTest(files1);

  const { exitCode, output } = mergeReports(test.info().outputPath("blob-report"), null, { additionalArgs: ["--config", "merge.config.ts"] });
  expect_eq(exitCode, 0);
  expect_eq(std.mem.indexOf(u8, output, `test project: my-project`) != null, true);
// TODO(cap-migrate): refine TS statement
// [ts] });


  const blobDir = test.info().outputPath("blob-report");
  fs.promises.mkdir(blobDir, { recursive: true });
  fs.promises.copyFile(path.join(__dirname, "../assets/blob-1.42.zip"), path.join(blobDir, "blob-1.42.zip"));

  const { exitCode, output } = mergeReports(blobDir, null, { additionalArgs: ["--config", "merge.config.ts"] });
  expect_eq(exitCode, 0);
  expect(output).toContain(` > chromium > example.spec.ts > test 0
 > chromium > example.spec.ts > describe 1 > describe 2 > test 3
 > chromium > example.spec.ts > describe 1 > describe 2 > test 4
 > chromium > example.spec.ts > describe 1 > describe 2 > test 2
 > chromium > example.spec.ts > describe 1 > test 1
 > chromium > example.spec.ts > describe 1 > test 5
 > chromium > example.spec.ts > test 6
 > firefox > example.spec.ts > describe 1 > describe 2 > test 2
 > firefox > example.spec.ts > describe 1 > describe 2 > test 3
 > firefox > example.spec.ts > test 0
 > firefox > example.spec.ts > describe 1 > describe 2 > test 4
 > firefox > example.spec.ts > describe 1 > test 1
 > firefox > example.spec.ts > test 6
 > firefox > example.spec.ts > describe 1 > test 5
 > webkit > example.spec.ts > describe 1 > describe 2 > test 4
 > webkit > example.spec.ts > test 0
 > webkit > example.spec.ts > describe 1 > test 1
 > webkit > example.spec.ts > describe 1 > describe 2 > test 2
 > webkit > example.spec.ts > describe 1 > describe 2 > test 3
 > webkit > example.spec.ts > test 6
 > webkit > example.spec.ts > describe 1 > test 5`);
// TODO(cap-migrate): refine TS statement
// [ts] });





fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should call methods in right order", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "echo-reporter.js": echoReporterJs,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('flaky 1', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `,
    // [ts] "c.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 3', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('flaky 2', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 3', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/3` });
    // [ts] runInlineTest(files, { shard: `3/3` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, [expect.stringMatching("report-.*.zip"), expect.stringMatching("report-.*.zip")]);
    // [ts] const { exitCode, output } = mergeReports(reportDir, {}, { additionalArgs: ["--reporter", test.info().outputPath("echo-reporter.js")] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const lines = output.split("\n").filter(l => l.trim().length);
    // [ts] expect_eq(lines[0], "onBegin");
    // [ts] expect_eq(std.mem.indexOf(u8, lines, "onTestBegin") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, lines, "onEnd") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, lines, "onExit") != null, true);
    // [ts] expect_eq(lines.indexOf("onBegin") < lines.indexOf("onTestBegin"), true);
    // [ts] expect_eq(lines[lines.length - 2], "onEnd");
    // [ts] expect_eq(lines[lines.length - 1], "onExit");
    // [ts] expect_eq(lines.filter(l => l == "onBegin").length, 1);
    // [ts] expect_eq(lines.filter(l => l == "onEnd").length, 1);
    // [ts] expect_eq(lines.filter(l => l == "onExit").length, 1);
    // [ts] });
    // [ts] test("should fail merge and report error when reporter throws in onEnd", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "throwing-reporter.js": `
    // [ts] class ThrowingReporter {
    // [ts] onEnd() {
    // [ts] throw new Error('Error in merge onEnd!');
    // [ts] }
    // [ts] }
    // [ts] module.exports = ThrowingReporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // Merge through the dot reporter so the reporter error is surfaced, and the
    // throwing reporter to fail the merge command.
    // [ts] const { exitCode, output } = mergeReports(reportDir, {}, { additionalArgs: ["--reporter", "dot," + test.info().outputPath("throwing-reporter.js")] });
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Error in merge onEnd!") != null, true);
    // [ts] });
    // [ts] test("should merge into html with dependencies", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]],
    // [ts] projects: [
    // [ts] { name: 'test', dependencies: ['setup'] },
    // [ts] { name: 'setup', testMatch: /.*setup.js/ },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "setup.js": `
    // [ts] import { test as setup } from '@playwright/test';
    // [ts] setup('login once', ({}) => {
    // [ts] setup.step('login step', () => {});
    // [ts] });
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('flaky 1', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `,
    // [ts] "c.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 3', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('flaky 2', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 3', () => {});
    // [ts] `
    // [ts] };
    // [ts] const totalShards = 3;
    // [ts] let i = 0;
    // [ts] while (i < totalShards) // [cap-migrate] for-step: i++
    // [ts] runInlineTest(files, { shard: `${i + 1}/${totalShards}` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, [expect.stringMatching("report-.*.zip"), expect.stringMatching("report-.*.zip"), expect.stringMatching("report-.*.zip")]);
    // [ts] const { exitCode, output } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "To open last HTML report run:") == null, true);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("10");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("6");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("3");
    // [ts] expect(page.locator(".test-file-test .test-file-title")).toHaveText([
    // [ts] "failing 1", "flaky 1", "math 1",
    // [ts] "failing 2", "math 2",
    // [ts] "flaky 2", "math 3",
    // [ts] "login once", "login once", "login once",
    // [ts] ]);
    // [ts] let i = 0;
    // [ts] while (i < 3) { // [cap-migrate] for-step: i++
    // [ts] page.getByText("login once").nth(i).click();
    // [ts] expect(page.getByText("login step")).toBeVisible();
    // [ts] page.goBack();
    // [ts] }
    // [ts] });
    // [ts] test("should merge blob into blob", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report-orig");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('flaky 1', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] {
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, null, { additionalArgs: ["--reporter", "blob"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] }
    // [ts] {
    // [ts] const compinedBlobReportDir = test.info().outputPath("blob-report");
    // [ts] const { exitCode } = mergeReports(compinedBlobReportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html,json"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(fs.existsSync(test.info().outputPath("report.json")), true);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("5");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("1");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("2");
    // [ts] }
    // [ts] });
    // [ts] test("should produce consistent step ids", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31023" },
    // [ts] }, (_p) => {
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [
    // [ts] ['blob', { outputFile: 'blob-report/report-1.zip' }],
    // [ts] ['blob', { outputFile: 'blob-report/report-2.zip' }]
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html,json"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("2");
    // [ts] });
    // [ts] test("be able to merge incomplete shards", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => { });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `,
    // [ts] "c.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 3', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('flaky 2', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 3', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/3` });
    // [ts] runInlineTest(files, { shard: `3/3` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, [expect.stringMatching("report-.*.zip"), expect.stringMatching("report-.*.zip")]);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("4");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("1");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("1");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("2");
    // [ts] });
    // [ts] test("total time is from test run not from merge", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('slow 1', () => {
    // [ts] new Promise(f => setTimeout(f, 2000));
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('slow 1', () => {
    // [ts] new Promise(f => setTimeout(f, 1000));
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const { exitCode, output } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "To open last HTML report run:") == null, true);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("2");
    // [ts] const durationText = page.getByTestId("overall-duration").textContent();
    // "Total time: 2.1s"
    // [ts] const time = "Total time: (\\d+)(\\.\\d+)?s".exec(durationText);
    // [ts] expect_eq(time, true);
    // [ts] expect_eq(parseInt(time[1], 10) >= 2, true);
    // [ts] });
    // [ts] test("merge into list report by default", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('flaky 1', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `,
    // [ts] "c.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 3', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('flaky 2', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 3', () => {});
    // [ts] `
    // [ts] };
    // [ts] const totalShards = 3;
    // [ts] let i = 0;
    // [ts] while (i < totalShards) // [cap-migrate] for-step: i++
    // [ts] runInlineTest(files, { shard: `${i + 1}/${totalShards}` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip", "report-3.zip"]);
    // [ts] const { exitCode, output } = mergeReports(reportDir, { PW_TEST_DEBUG_REPORTERS: "1", PLAYWRIGHT_LIST_PRINT_STEPS: "1", PLAYWRIGHT_FORCE_TTY: "80" }, { additionalArgs: ["--reporter", "list"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const text = stripAnsi(output);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "Running 10 tests using 3 workers") != null, true);
    // [ts] const lines = text.split("\n").filter(l => l.match("^#.* :")).map(l => l.replace("[.\\d]+m?s", "Xms"));
    // [ts] expect(lines).toEqual([
    // [ts] `#0 :       1 a.test.js:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº math 1`,
    // [ts] `#0 :   ${POSITIVE_STATUS_MARK}  1 a.test.js:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº math 1 (Xms)`,
    // [ts] `#1 :       2 a.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 1`,
    // [ts] `#1 :   ${NEGATIVE_STATUS_MARK}  2 a.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 1 (Xms)`,
    // [ts] `#2 :       3 a.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 1 (retry #1)`,
    // [ts] `#2 :   ${NEGATIVE_STATUS_MARK}  3 a.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 1 (retry #1) (Xms)`,
    // [ts] `#3 :       4 a.test.js:9:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 1`,
    // [ts] `#3 :   ${NEGATIVE_STATUS_MARK}  4 a.test.js:9:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 1 (Xms)`,
    // [ts] `#4 :       5 a.test.js:9:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 1 (retry #1)`,
    // [ts] `#4 :   ${POSITIVE_STATUS_MARK}  5 a.test.js:9:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 1 (retry #1) (Xms)`,
    // [ts] `#5 :       6 a.test.js:12:12 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº skipped 1`,
    // [ts] `#5 :   -   6 a.test.js:12:12 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº skipped 1`,
    // [ts] `#6 :       7 b.test.js:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº math 2`,
    // [ts] `#6 :   ${POSITIVE_STATUS_MARK}  7 b.test.js:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº math 2 (Xms)`,
    // [ts] `#7 :       8 b.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 2`,
    // [ts] `#7 :   ${NEGATIVE_STATUS_MARK}  8 b.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 2 (Xms)`,
    // [ts] `#8 :       9 b.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 2 (retry #1)`,
    // [ts] `#8 :   ${NEGATIVE_STATUS_MARK}  9 b.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº failing 2 (retry #1) (Xms)`,
    // [ts] `#9 :      10 b.test.js:9:12 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº skipped 2`,
    // [ts] `#9 :   -  10 b.test.js:9:12 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº skipped 2`,
    // [ts] `#10 :      11 c.test.js:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº math 3`,
    // [ts] `#10 :   ${POSITIVE_STATUS_MARK} 11 c.test.js:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº math 3 (Xms)`,
    // [ts] `#11 :      12 c.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 2`,
    // [ts] `#11 :   ${NEGATIVE_STATUS_MARK} 12 c.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 2 (Xms)`,
    // [ts] `#12 :      13 c.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 2 (retry #1)`,
    // [ts] `#12 :   ${POSITIVE_STATUS_MARK} 13 c.test.js:6:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº flaky 2 (retry #1) (Xms)`,
    // [ts] `#13 :      14 c.test.js:9:12 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº skipped 3`,
    // [ts] `#13 :   -  14 c.test.js:9:12 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº skipped 3`,
    // [ts] ]);
    // [ts] });
    // [ts] test("should print progress", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('flaky 1', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `,
    // [ts] "c.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 3', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('flaky 2', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 3', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip"]);
    // [ts] const { exitCode, output } = mergeReports(reportDir, { PLAYWRIGHT_HTML_OPEN: "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const lines = output.split("\n");
    // [ts] expect(lines).toContainEqual(expect.stringMatching("extracting: blob-report[\\/\\\\]report-.*zip$"));
    // [ts] expect(lines).toContainEqual(expect.stringMatching("merging events$"));
    // [ts] expect(lines).toContainEqual(expect.stringMatching("building final report"));
    // [ts] expect(lines).toContainEqual(expect.stringMatching("finished building report"));
    // [ts] });
    // [ts] test("preserve attachments", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import fs from 'fs';
    // [ts] test('first', () => {
    // [ts] const attachmentPath = test.info().outputPath('foo.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'hello!');
    // [ts] test.info().attach('file-attachment', {path: attachmentPath});
    // [ts] console.log('console info');
    // [ts] console.error('console error');
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] test.info().attach('text-attachment', { body: 'hi!' });
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => { });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] page.getByText("first").click();
    // [ts] const downloadPromise = page.waitForEvent("download");
    // Check file attachment.
    // [ts] page.getByRole("link", { name: "file-attachment" }).click();
    // [ts] const download = downloadPromise;
    // Check file attachment content.
    // [ts] expect_eq(readAllFromStreamAsString(download.createReadStream()), "hello!");
    // [ts] page.goBack();
    // [ts] page.getByText("failing 1").click();
    // [ts] expect(page.getByText("'text-attachment', { body: 'hi!'")).toBeVisible();
    // [ts] });
    // [ts] test("generate html with attachment urls", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] use: {
    // [ts] trace: 'on'
    // [ts] },
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import fs from 'fs';
    // [ts] test('first', () => {
    // [ts] const attachmentPath = test.info().outputPath('foo.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'hello!');
    // [ts] test.info().attach('file-attachment', { path: attachmentPath });
    // [ts] console.log('console info');
    // [ts] console.error('console error');
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] test.info().attach('text-attachment', { body: 'hi!' });
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => { });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const htmlReportDir = test.info().outputPath("playwright-report");
    // [ts] for (const entry of fs.promises.readdir(htmlReportDir))
    // [ts] fs.promises.cp(path.join(htmlReportDir, entry), path.join(reportDir, entry), { recursive: true });
    // [ts] const oldServeFile = server.serveFile;
    // [ts] server.serveFile = (req, res) => {
    // [ts] const pathName = URL.init(req.url, "http://localhost").pathname;
    // [ts] const filePath = path.join(reportDir, pathName.substring(1));
    // [ts] return oldServeFile.call(server, req, res, filePath);
    // [ts] };
    // [ts] page.goto(`${server.PREFIX}/index.html`);
    // [ts] page.getByText("first").click();
    // [ts] const downloadPromise = page.waitForEvent("download");
    // Check file attachment.
    // [ts] page.getByRole("link", { name: "file-attachment" }).click();
    // [ts] const download = downloadPromise;
    // Check file attachment content.
    // [ts] expect_eq(readAllFromStreamAsString(download.createReadStream()), "hello!");
    // [ts] page.goBack();
    // Check inline attachment.
    // [ts] page.getByText("failing 1").click();
    // [ts] expect(page.getByText("'text-attachment', { body: 'hi!'")).toBeVisible();
    // [ts] page.goBack();
    // Check that trace loads.
    // [ts] page.locator(".test-file-test").filter({ hasText: "failing 1" }).getByRole("link", { name: "View Trace" }).click();
    // [ts] expect(page).toHaveTitle("Playwright Trace Viewer");
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - tree:
    // [ts] - treeitem /Expect "toBe" \\d+[hmsp]+/ [selected]
    // [ts] `);
    // [ts] });
    // [ts] test("resource names should not clash between runs", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import fs from 'fs';
    // [ts] import path from 'path';
    // [ts] test('first', () => {
    // [ts] const attachmentPath = path.join(test.info().config.rootDir, 'foo.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'hello!');
    // [ts] test.info().attachments.push({ name: 'file-attachment', path: attachmentPath, contentType: 'text/plain' });
    // [ts] });
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import fs from 'fs';
    // [ts] import path from 'path';
    // [ts] test('failing 2', () => {
    // [ts] const attachmentPath = path.join(test.info().config.rootDir, 'foo.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'bye!');
    // [ts] test.info().attachments.push({ name: 'file-attachment', path: attachmentPath, contentType: 'text/plain' });
    // [ts] });
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] const fileAttachment = page.getByRole("link", { name: "file-attachment" });
    // Check first attachment content.
    // [ts] {
    // [ts] page.getByText("first").click();
    // [ts] expect(fileAttachment).toBeVisible();
    // [ts] const downloadPromise = page.waitForEvent("download");
    // [ts] fileAttachment.click();
    // [ts] const download = downloadPromise;
    // [ts] expect_eq(readAllFromStreamAsString(download.createReadStream()), "hello!");
    // [ts] page.goBack();
    // [ts] }
    // Check second attachment content.
    // [ts] {
    // [ts] page.getByText("failing 2").click();
    // [ts] expect(fileAttachment).toBeVisible();
    // [ts] const downloadPromise = page.waitForEvent("download");
    // [ts] fileAttachment.click();
    // [ts] const download = downloadPromise;
    // [ts] expect_eq(readAllFromStreamAsString(download.createReadStream()), "bye!");
    // [ts] page.goBack();
    // [ts] }
    // [ts] });
    // [ts] test("multiple output reports", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import fs from 'fs';
    // [ts] test('first', () => {
    // [ts] const attachmentPath = test.info().outputPath('foo.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'hello!');
    // [ts] test.info().attach('file-attachment', {path: attachmentPath});
    // [ts] console.log('console info');
    // [ts] console.error('console error');
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] test.info().attach('text-attachment', { body: 'hi!' });
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => { });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip"]);
    // [ts] const { exitCode, output } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html,line"] });
    // [ts] expect_eq(exitCode, 0);
    // Check that line reporter was called.
    // [ts] const text = stripAnsi(output);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "Running 3 tests using 1 worker") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "[1/3] a.test.js:5:7 Ã¢ÂÂº first") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "a.test.js:13:7 ÃÂ¢ÃÂÃÂº failing 1 (retry #1)") != null, true);
    // Check html report presence.
    // [ts] showReport();
    // [ts] expect(page.getByText("first")).toBeVisible();
    // [ts] });
    // [ts] test("multiple output reports based on config", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "merged/playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob', { outputDir: 'merged-blob' }], ['html', { outputFolder: 'html', open: 'never' }], ['line']]
    // [ts] };
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import fs from 'fs';
    // [ts] test('first', () => {
    // [ts] const attachmentPath = test.info().outputPath('foo.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'hello!');
    // [ts] test.info().attach('file-attachment', {path: attachmentPath});
    // [ts] console.log('console info');
    // [ts] console.error('console error');
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] test.info().attach('text-attachment', { body: 'hi!' });
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => { });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip"]);
    // [ts] const { exitCode, output } = mergeReports(reportDir, null, {
    // [ts] cwd: test.info().outputPath("merged"),
    // [ts] additionalArgs: ["--config", "playwright.config.ts"],
    // [ts] });
    // [ts] expect_eq(exitCode, 0);
    // Check that line reporter was called.
    // [ts] const text = stripAnsi(output);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "Running 6 tests using 2 workers") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "[1/6] a.test.js:5:7 Ã¢ÂÂº first") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "a.test.js:13:7 ÃÂ¢ÃÂÃÂº failing 1 (retry #1)") != null, true);
    // Check html report presence.
    // [ts] expect_eq((fs.promises.stat(test.info().outputPath("merged/html/index.html"))).isFile, true);
    // Check report presence.
    // [ts] const mergedBlobReportFiles = fs.promises.readdir(test.info().outputPath("merged/merged-blob"));
    // [ts] expect_eq(mergedBlobReportFiles, ["report.zip"]);
    // [ts] });
    // [ts] test("onError in the report", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] tag: process.env.GLOBAL_TAG,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend<{}, { errorInTearDown: string }>({
    // [ts] errorInTearDown: [({ }, use) => {
    // [ts] use('');
    // [ts] throw new Error('Error in teardown');
    // [ts] }, { scope: 'worker' }],
    // [ts] });
    // [ts] test('test', () => {
    // [ts] });
    // [ts] test('pass', () => {
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => { });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `,
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 3', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('flaky 2', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 3', () => {});
    // [ts] `
    // [ts] };
    // [ts] const result = runInlineTest(files, { shard: `1/3` }, { GLOBAL_TAG: "@macos-node16-ttest" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("0");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("0");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("1");
    // [ts] expect(page.getByTestId("report-errors")).toContainText("(@macos-node16-ttest) Error: Error in teardown");
    // [ts] });
    // [ts] test("preserve config fields", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44264" } }, (_p) => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const config: PlaywrightTestConfig = {
    // Runner options:
    // [ts] globalTimeout: 202300,
    // [ts] failOnFlakyTests: true,
    // [ts] maxFailures: 3,
    // [ts] metadata: {
    // [ts] "a": "b",
    // [ts] "b": 100,
    // [ts] },
    // [ts] workers: 1,
    // [ts] projects: [
    // [ts] { name: "limited", workers: 10 },
    // [ts] { name: "unlimited" },
    // [ts] ],
    // [ts] retries: 1,
    // [ts] reporter: [["blob", { outputDir: `${reportDir.replace("\\\\", "/")}` }]],
    // Reporter options:
    // [ts] reportSlowTests: {
    // [ts] max: 7,
    // [ts] threshold: 15_000,
    // [ts] },
    // [ts] quiet: true
    // [ts] };
    // [ts] const files = {
    // [ts] "echo-reporter.js": `
    // [ts] const fs = require('fs');
    // [ts] class EchoReporter {
    // [ts] onBegin(config, suite) {
    // [ts] fs.writeFileSync('config.json', JSON.stringify(config));
    // [ts] }
    // [ts] }
    // [ts] module.exports = EchoReporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default ${JSON.stringify(config, null, 2)};
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "c.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 3', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/3`, workers: 1 });
    // [ts] runInlineTest(files, { shard: `3/3`, workers: 1 }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const mergeConfig = {
    // [ts] reportSlowTests: {
    // [ts] max: 2,
    // [ts] threshold: 1_000,
    // [ts] },
    // [ts] quiet: false
    // [ts] };
    // [ts] fs.promises.writeFile(test.info().outputPath("merge.config.ts"), `module.exports = ${JSON.stringify(mergeConfig, null, 2)};`);
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-3.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, {}, { additionalArgs: ["--reporter", test.info().outputPath("echo-reporter.js"), "-c", test.info().outputPath("merge.config.ts")] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const json = JSON.parse(fs.readFileSync(test.info().outputPath("config.json")).toString());
    // Test shard parameters.
    // [ts] expect_eq(json.rootDir, test.info().outputDir);
    // [ts] expect_eq(json.globalTimeout, config.globalTimeout);
    // [ts] expect_eq(json.maxFailures, config.maxFailures);
    // [ts] expect_eq(json.failOnFlakyTests, config.failOnFlakyTests);
    // [ts] expect_eq(json.metadata, expect.objectContaining(config.metadata));
    // [ts] expect_eq(json.workers, 2);
    // [ts] expect(json.projects.map(project => [project.name, project.workers])).toEqual([
    // [ts] ["limited", 10],
    // [ts] ["unlimited", null],
    // [ts] ]);
    // [ts] expect_eq(json.version, true);
    // [ts] expect_eq(json.version != test.info().config.version, true);
    // Reporter config parameters.
    // [ts] expect_eq(json.reportSlowTests, mergeConfig.reportSlowTests);
    // [ts] expect_eq(json.configFile, test.info().outputPath("merge.config.ts"));
    // [ts] expect_eq(json.quiet, mergeConfig.quiet);
    // [ts] });
    // [ts] test("preserve stdout and stderr", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "echo-reporter.js": `
    // [ts] const fs = require('fs');
    // [ts] class EchoReporter {
    // [ts] log = [];
    // [ts] onStdOut(chunk, test, result) {
    // [ts] this.log.push('onStdOut: ' + chunk);
    // [ts] this.log.push('result.stdout: ' + result.stdout);
    // [ts] }
    // [ts] onStdErr(chunk, test, result) {
    // [ts] this.log.push('onStdErr: ' + chunk);
    // [ts] this.log.push('result.stderr: ' + result.stderr);
    // [ts] }
    // [ts] onTestEnd(test, result) {
    // [ts] this.log.push('onTestEnd');
    // [ts] this.log.push('result.stdout: ' + result.stdout);
    // [ts] this.log.push('result.stderr: ' + result.stderr);
    // [ts] }
    // [ts] onEnd() {
    // [ts] fs.writeFileSync('log.txt', this.log.join('\\n'));
    // [ts] }
    // [ts] }
    // [ts] module.exports = EchoReporter;
    // [ts] `,
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] reporter: [['blob']]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('a test', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] console.log('stdout text');
    // [ts] console.error('stderr text');
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const { exitCode } = mergeReports(reportDir, {}, { additionalArgs: ["--reporter", test.info().outputPath("echo-reporter.js")] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const log = stripAnsi(fs.readFileSync(test.info().outputPath("log.txt")).toString());
    // [ts] expect(log).toBe(`onStdOut: stdout text
    // [ts] result.stdout: stdout text
    // [ts] onStdErr: stderr text
    // [ts] result.stderr: stderr text
    // [ts] onTestEnd
    // [ts] result.stdout: stdout text
    // [ts] result.stderr: stderr text
    // [ts] `);
    // [ts] });
    // [ts] test("encode inline attachments", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "echo-reporter.js": `
    // [ts] const fs = require('fs');
    // [ts] class EchoReporter {
    // [ts] onTestEnd(test, result) {
    // [ts] const attachmentBodies = result.attachments.map(a => a.body?.toString('base64'));
    // [ts] result.attachments.forEach(a => console.log(a.body, 'isBuffer', Buffer.isBuffer(a.body)));
    // [ts] fs.writeFileSync('log.txt', attachmentBodies.join(','));
    // [ts] }
    // [ts] }
    // [ts] module.exports = EchoReporter;
    // [ts] `,
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] reporter: [['blob']]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('a test', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] test.info().attachments.push({
    // [ts] name: 'example.txt',
    // [ts] contentType: 'text/plain',
    // [ts] body: Buffer.from('foo'),
    // [ts] });
    // [ts] test.info().attachments.push({
    // [ts] name: 'example.json',
    // [ts] contentType: 'application/json',
    // [ts] body: Buffer.from(JSON.stringify({ foo: 1 })),
    // [ts] });
    // [ts] test.info().attachments.push({
    // [ts] name: 'example-utf16.txt',
    // [ts] contentType: 'text/plain, charset=utf16le',
    // [ts] body: Buffer.from('utf16 encoded', 'utf16le'),
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const { exitCode } = mergeReports(reportDir, {}, { additionalArgs: ["--reporter", test.info().outputPath("echo-reporter.js")] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const log = fs.readFileSync(test.info().outputPath("log.txt")).toString();
    // [ts] expect_eq(log, `Zm9v,eyJmb28iOjF9,dQB0AGYAMQA2ACAAZQBuAGMAbwBkAGUAZAA=`);
    // [ts] });
    // [ts] test("preserve steps in html report", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']]
    // [ts] };
    // [ts] `,
    // [ts] "tests/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeAll(() => {
    // [ts] expect_eq(1, 1);
    // [ts] })
    // [ts] test('test 1', () => {
    // [ts] test.step('my step', () => {
    // [ts] expect_eq(2, 2);
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report.zip"]);
    // Run merger in a different directory to make sure relative paths will not be resolved
    // relative to the current directory.
    // [ts] const mergeCwd = test.info().outputPath("foo");
    // [ts] fs.promises.mkdir(mergeCwd, { recursive: true });
    // [ts] const { exitCode, output } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"], cwd: mergeCwd });
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "To open last HTML report run:") == null, true);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("1");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("1");
    // [ts] page.getByRole("link", { name: "test 1" }).click();
    // [ts] page.getByText("Before Hooks").click();
    // [ts] page.getByText("beforeAll hook").click();
    // [ts] expect(page.getByText("Expect \"toBe\"")).toBeVisible();
    // Collapse hooks.
    // [ts] page.getByText("Before Hooks").click();
    // [ts] expect(page.getByText("Expect \"toBe\"")).not.toBeVisible();
    // Check that 'my step' location is relative.
    // [ts] expect(page.getByText("ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ tests/a.test.js:7")).toBeVisible();
    // [ts] page.getByText("my step").click();
    // [ts] expect(page.getByText("Expect \"toBe\"")).toBeVisible();
    // [ts] });
    // [ts] test("preserve step params", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "params-reporter.js": `
    // [ts] class ParamsReporter {
    // [ts] onStepEnd(test, result, step) {
    // [ts] if (step.category === 'test.step' || step.title.startsWith('Navigate'))
    // [ts] console.log('%%' + (step.subtitle ? step.title + ' ' + step.subtitle : step.title) + ' | ' + JSON.stringify(step.params));
    // [ts] }
    // [ts] }
    // [ts] module.exports = ParamsReporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] page.goto('about:blank');
    // [ts] test.step('my step', () => {}, { subtitle: 'my subtitle', params: { foo: 'bar', count: 7 } });
    // [ts] });
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const { exitCode, outputLines } = mergeReports(reportDir, null, { additionalArgs: ["--reporter", "./params-reporter.js"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] expect(outputLines).toEqual([
    // [ts] `Navigate about:blank | {"url":"about:blank"}`,
    // [ts] `my step my subtitle | {"foo":"bar","count":7}`,
    // [ts] ]);
    // [ts] });
    // [ts] test("support fileName option", () => {
    // [ts] const files = (fileSuffix) => ({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob', { fileName: 'report-${fileSuffix}.zip' }]],
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1 @smoke', () => {});
    // [ts] `,
    // [ts] });
    // [ts] runInlineTest(files("one"));
    // [ts] runInlineTest(files("two"), null, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report-one.zip", "report-two.zip"]);
    // [ts] });
    // [ts] test("support PLAYWRIGHT_BLOB_OUTPUT_DIR env variable", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30091" });
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']],
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1 @smoke', () => {});
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files, null, { PLAYWRIGHT_BLOB_OUTPUT_DIR: "my/dir" });
    // [ts] const reportDir = test.info().outputPath("my", "dir");
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report.zip"]);
    // [ts] });
    // [ts] test("support PLAYWRIGHT_BLOB_OUTPUT_NAME env variable", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30091" });
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']],
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1 @smoke', () => {});
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files, null, { PLAYWRIGHT_BLOB_OUTPUT_NAME: "report-one.zip" });
    // [ts] runInlineTest(files, null, { PLAYWRIGHT_BLOB_OUTPUT_NAME: "report-two.zip", PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report-one.zip", "report-two.zip"]);
    // [ts] });
    // [ts] test("support outputFile option", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30091" });
    // [ts] const files = (fileSuffix) => ({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob', { outputDir: 'should-be-ignored', outputFile: 'my-reports/report-${fileSuffix}.zip' }]],
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1 @smoke', () => {});
    // [ts] `,
    // [ts] });
    // [ts] runInlineTest(files("one"));
    // [ts] runInlineTest(files("two"));
    // [ts] const reportDir = test.info().outputPath("my-reports");
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report-one.zip", "report-two.zip"]);
    // [ts] });
    // [ts] test("support PLAYWRIGHT_BLOB_OUTPUT_FILE environment variable", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30091" });
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: 'blob',
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1 @smoke', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1 @smoke', () => {});
    // [ts] `,
    // [ts] };
    // [ts] const defaultDir = test.info().outputPath("blob-report");
    // [ts] fs.mkdirSync(defaultDir, { recursive: true });
    // [ts] const file = path.join(defaultDir, "some.file");
    // [ts] fs.writeFileSync(file, "content");
    // [ts] runInlineTest(files, { shard: `1/2` }, { PLAYWRIGHT_BLOB_OUTPUT_FILE: "subdir/report-one.zip" });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PLAYWRIGHT_BLOB_OUTPUT_FILE: test.info().outputPath("subdir/report-two.zip") });
    // [ts] const reportDir = test.info().outputPath("subdir");
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report-one.zip", "report-two.zip"]);
    // [ts] expect_eq(fs.existsSync(file), "Default directory should not be cleaned up if output file is specified.", true);
    // [ts] });
    // [ts] test("keep projects with same name different global tag separate", () => {
    // [ts] const files = (reportName) => ({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']],
    // [ts] tag: process.env.GLOBAL_TAG,
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => { expect_eq('${reportName}', 'second'); });
    // [ts] `,
    // [ts] });
    // [ts] runInlineTest(files("first"), null, { GLOBAL_TAG: "@first" });
    // [ts] runInlineTest(files("second"), null, { GLOBAL_TAG: "@second", PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report-1b98925.zip", "report-562ed66.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("1");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("1");
    // [ts] page.getByText("test 1.*first").getByRole("link", { name: "test" }).click();
    // [ts] expect(page.getByText("Errors")).toBeVisible();
    // [ts] page.goBack();
    // [ts] page.getByText("test 1.*second").getByRole("link", { name: "test" }).click();
    // [ts] expect(page.getByText("Errors")).not.toBeVisible();
    // [ts] });
    // [ts] test("no reports error", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] fs.mkdirSync(reportDir, { recursive: true });
    // [ts] const { exitCode, output } = mergeReports(reportDir);
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `No report files found in`) != null, true);
    // [ts] });
    // [ts] test("blob-report should be next to package.json", () => {
    // [ts] const result = runInlineTest({
    // [ts] "foo/package.json": `{ "name": "foo" }`,
    // unused config along "search path"
    // [ts] "foo/bar/playwright.config.js": `
    // [ts] module.exports = { projects: [ {} ] };
    // [ts] `,
    // [ts] "foo/bar/baz/tests/a.spec.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] const fs = require('fs');
    // [ts] test('pass', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "blob" }, {}, { cwd: "foo/bar/baz/tests" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("blob-report")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "blob-report")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "blob-report")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("foo", "bar", "baz", "tests", "blob-report")), false);
    // [ts] });
    // [ts] test("blob report should include version", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']]
    // [ts] };
    // [ts] `,
    // [ts] "tests/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] `,
    // [ts] };
    // CI/1 is a part of user agent string, make sure it matches in the nested test runner.
    // [ts] runInlineTest(files, null, { CI: process.env.CI });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles, ["report.zip"]);
    // [ts] const events = extractReport(test.info().outputPath("blob-report", "report.zip"), test.info().outputPath("tmp"));
    // [ts] const metadataEvent = events.find(e => e.method == "onBlobReportMetadata");
    // [ts] expect_eq(metadataEvent.params.version, 2);
    // [ts] expect_eq(metadataEvent.params.userAgent.split(" node/")[0], getUserAgent().split(" node/")[0]);
    // [ts] });
    // [ts] test("merge-reports should throw if report version is from the future", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']]
    // [ts] };
    // [ts] `,
    // [ts] "tests/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] `,
    // [ts] "tests/b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip"]);
    // Extract report and modify version.
    // [ts] const reportZipFile = test.info().outputPath("blob-report", reportFiles[1]);
    // [ts] const events = extractReport(reportZipFile, test.info().outputPath("tmp"));
    // [ts] const metadataEvent = events.find(e => e.method == "onBlobReportMetadata");
    // [ts] expect_eq(metadataEvent.params.version, true);
    // [ts] ++metadataEvent.params.version;
    // Zip it back.
    // [ts] fs.promises.rm(reportZipFile, { force: true });
    // [ts] zipReport(events, reportZipFile);
    // [ts] const { exitCode, output } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Blob report report-2.zip was created with a newer version of Playwright.`) != null, true);
    // [ts] });
    // [ts] test("merge-reports should merge old attachments (pre-1.53)", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob']]
    // [ts] };
    // [ts] `,
    // [ts] "tests/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] `,
    // [ts] "merge.config.ts": `export default {
    // [ts] testDir: 'mergeRoot',
    // [ts] };`,
    // [ts] };
    // [ts] test("should merge blob reports with same name", () => {
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: 'blob'
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 1', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('flaky 1', () => {
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] test('skipped 1', () => {});
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('failing 2', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] test('skipped 2', () => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const reportZip = test.info().outputPath("blob-report", "report.zip");
    // [ts] const allReportsDir = test.info().outputPath("all-blob-reports");
    // [ts] fs.promises.cp(reportZip, path.join(allReportsDir, "report-1.zip"));
    // [ts] fs.promises.cp(reportZip, path.join(allReportsDir, "report-2.zip"));
    // [ts] const { exitCode } = mergeReports(allReportsDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] expect(page.locator(".subnav-item:has-text(\"All\") .counter")).toHaveText("10");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Passed\") .counter")).toHaveText("4");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Failed\") .counter")).toHaveText("4");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Flaky\") .counter")).toHaveText("2");
    // [ts] expect(page.locator(".subnav-item:has-text(\"Skipped\") .counter")).toHaveText("4");
    // [ts] });
    // [ts] test("reporter list in the custom config", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27281" });
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: 'blob' };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('first', () => {
    // [ts] });
    // [ts] `,
    // [ts] "merged/testReporter.js": `
    // [ts] class TestReporter {
    // [ts] onBegin(fullConfig, suite) {
    // [ts] console.log('reporter', fullConfig.reporter);
    // [ts] }
    // [ts] }
    // [ts] module.exports = TestReporter;
    // [ts] `,
    // [ts] "merged/playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['./testReporter.js', { myOpt: 1 }]]
    // [ts] };
    // [ts] `,
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const { exitCode, output } = mergeReports(reportDir, null, { additionalArgs: ["--config", test.info().outputPath("merged/playwright.config.ts")] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const text = stripAnsi(output);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "testReporter.js") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, "myOpt: 1") != null, true);
    // [ts] });
    // [ts] test("merge reports with different rootDirs", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27877" });
    // [ts] const files1 = {
    // [ts] "echo-reporter.js": `
    // [ts] export default class EchoReporter {
    // [ts] onBegin(config, suite) {
    // [ts] console.log('rootDir:', config.rootDir);
    // [ts] }
    // [ts] onTestBegin(test) {
    // [ts] console.log('test:', test.location.file);
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "merge.config.ts": `export default {
    // [ts] testDir: 'mergeRoot',
    // [ts] reporter: './echo-reporter.js'
    // [ts] };`,
    // [ts] "dir1/playwright.config.ts": `export default {
    // [ts] testDir: 'tests1',
    // [ts] reporter: [['blob', { outputDir: 'blob-report' }]]
    // [ts] };`,
    // [ts] test('math 1', () => { });
    // [ts] test('math 2', () => { });
    // [ts] test("merge reports same rootDirs", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27877" });
    // [ts] const files = {
    // [ts] "echo-reporter.js": `
    // [ts] export default class EchoReporter {
    // [ts] onBegin(config, suite) {
    // [ts] console.log('rootDir:', config.rootDir);
    // [ts] }
    // [ts] onTestBegin(test) {
    // [ts] console.log('test:', test.location.file);
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "playwright.config.ts": `export default {
    // [ts] testDir: 'tests',
    // [ts] reporter: [['blob', { outputDir: 'blob-report' }]]
    // [ts] };`,
    // [ts] "tests/dir1/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => { });
    // [ts] `,
    // [ts] "tests/dir2/b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => { });
    // [ts] `,
    // [ts] };
    // [ts] test("merge reports with different rootDirs and path separators", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27877" });
    // [ts] const files1 = {
    // [ts] "echo-reporter.js": `
    // [ts] export default class EchoReporter {
    // [ts] onBegin(config, suite) {
    // [ts] console.log('rootDir:', config.rootDir);
    // [ts] }
    // [ts] onTestBegin(test) {
    // [ts] console.log('test:', test.location.file);
    // [ts] console.log('test title:', test.titlePath()[2]);
    // [ts] }
    // [ts] onTestEnd(test) {
    // [ts] console.log('annotations:', test.annotations.map(a => 'type: ' + a.type + ', description: ' + a.description + ', file: ' + a.location.file).join(','));
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "merge.config.ts": `export default {
    // [ts] testDir: 'mergeRoot',
    // [ts] reporter: './echo-reporter.js'
    // [ts] };`,
    // [ts] "dir1/playwright.config.ts": `export default {
    // [ts] reporter: [['blob', { outputDir: 'blob-report' }]]
    // [ts] };`,
    // [ts] test('math 1', { annotation: { type: 'warning', description: 'Some warning' } }, ({}) => { });
    // [ts] test('math 2', { annotation: { type: 'issue' } }, ({}) => { });
    // [ts] test("merge reports without --config preserves path separators", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27877" });
    // [ts] const files1 = {
    // [ts] "echo-reporter.js": `
    // [ts] export default class EchoReporter {
    // [ts] onBegin(config, suite) {
    // [ts] console.log('rootDir:', config.rootDir);
    // [ts] }
    // [ts] onTestBegin(test) {
    // [ts] console.log('test:', test.location.file);
    // [ts] console.log('test title:', test.titlePath()[2]);
    // [ts] }
    // [ts] onTestEnd(test) {
    // [ts] console.log('annotations:', test.annotations.map(a => 'type: ' + a.type + ', description: ' + a.description + ', file: ' + a.location.file).join(','));
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "dir1/playwright.config.ts": `export default {
    // [ts] reporter: [['blob', { outputDir: 'blob-report' }]]
    // [ts] };`,
    // [ts] "dir1/tests1/a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', { annotation: { type: 'warning', description: 'Some warning' } }, ({}) => { });
    // [ts] `,
    // [ts] "dir1/tests2/b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', { annotation: { type: 'issue' } }, ({}) => { });
    // [ts] `,
    // [ts] };
    // [ts] test("merge reports should preserve attachments", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report-orig");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] retries: 1,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import * as fs from 'fs';
    // [ts] test('attachment A', () => {
    // [ts] const attachmentPath = test.info().outputPath('foo.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'hello!');
    // [ts] test.info().attach('file-attachment1', { path: attachmentPath });
    // [ts] test.info().attachments.push({ name: 'file-attachment2', path: attachmentPath, contentType: 'text/html' });
    // [ts] test.info().attach('file-attachment3', { path: attachmentPath });
    // [ts] test.info().attach('file-attachment4', { path: attachmentPath });
    // [ts] test.info().attachments.push({ name: 'file-attachment5', path: attachmentPath, contentType: 'text/html' });
    // [ts] test.info().attachments.push({ name: 'file-attachment6', path: attachmentPath, contentType: 'text/html' });
    // [ts] test.info().attach('file-attachment7', { path: attachmentPath });
    // [ts] });
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import * as fs from 'fs';
    // [ts] test('attachment B', () => {
    // [ts] const attachmentPath = test.info().outputPath('bar.txt');
    // [ts] fs.writeFileSync(attachmentPath, 'goodbye!');
    // [ts] test.info().attach('file-attachment8', { path: attachmentPath });
    // [ts] test.info().attachments.push({ name: 'file-attachment9', path: attachmentPath, contentType: 'application/json' });
    // [ts] });
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] runInlineTest(files, { shard: `2/2` }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] reportFiles.sort();
    // [ts] expect_eq(reportFiles, ["report-1.zip", "report-2.zip"]);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "blob,html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const reportZipFile = test.info().outputPath("blob-report", "report.zip");
    // [ts] const events = extractReport(reportZipFile, test.info().outputPath("tmp"));
    // [ts] type Attachment = Omit<JSONReportTestResult["attachments"][number], "path"> & {
    // [ts] path: any
    // [ts] };
    // [ts] const attachment1: Attachment = { name: "file-attachment1", path: expect.stringContaining(""), contentType: "text/plain" };
    // [ts] const attachment2: Attachment = { name: "file-attachment2", path: expect.stringContaining(""), contentType: "text/html" };
    // [ts] const attachment3: Attachment = { name: "file-attachment3", path: expect.stringContaining(""), contentType: "text/plain" };
    // [ts] const attachment4: Attachment = { name: "file-attachment4", path: expect.stringContaining(""), contentType: "text/plain" };
    // [ts] const attachment5: Attachment = { name: "file-attachment5", path: expect.stringContaining(""), contentType: "text/html" };
    // [ts] const attachment6: Attachment = { name: "file-attachment6", path: expect.stringContaining(""), contentType: "text/html" };
    // [ts] const attachment7: Attachment = { name: "file-attachment7", path: expect.stringContaining(""), contentType: "text/plain" };
    // [ts] const attachment8: Attachment = { name: "file-attachment8", path: expect.stringContaining(""), contentType: "text/plain" };
    // [ts] const attachment9: Attachment = { name: "file-attachment9", path: expect.stringContaining(""), contentType: "application/json" };
    // [ts] const aAttachments = [attachment1, attachment2, attachment3, attachment4, attachment5, attachment6, attachment7];
    // [ts] const bAttachments = [attachment8, attachment9];
    // [ts] const allStepAttachments = events.flatMap(e => e.method == "onStepEnd" ? e?.params?.step?.attachments ?? [] : []);
    // [ts] expect_eq(allStepAttachments, [0, 2, 3, 6, 0]);
    // [ts] const allTestAttachments = events.flatMap(e => e.method == "onAttach" ? e?.params?.attachments ?? [] : []);
    // [ts] expect_eq(allTestAttachments, [...aAttachments, ...bAttachments]);
    // [ts] showReport();
    // [ts] {
    // [ts] page.getByRole("link", { name: "Attachment A" }).click();
    // [ts] for (const attachment of aAttachments)
    // [ts] expect(page.getByRole("link", { name: attachment.name })).toBeVisible();
    // [ts] page.goBack();
    // [ts] }
    // [ts] {
    // [ts] page.getByRole("link", { name: "Attachment B" }).click();
    // [ts] for (const attachment of bAttachments)
    // [ts] expect(page.getByRole("link", { name: attachment.name })).toBeVisible();
    // [ts] page.goBack();
    // [ts] }
    // [ts] });
    // [ts] test("merge reports must not change test ids when there is no need to", () => {
    // [ts] const files = {
    // [ts] "echo-test-id-reporter.js": `
    // [ts] export default class EchoTestIdReporter {
    // [ts] onTestBegin(test) {
    // [ts] console.log('%%' + test.id);
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "single-run.config.ts": `export default {
    // [ts] reporter: [
    // [ts] ['./echo-test-id-reporter.js'],
    // [ts] ]
    // [ts] };`,
    // [ts] "shard-1.config.ts": `export default {
    // [ts] fullyParallel: true,
    // [ts] shard: { total: 2, current: 1 },
    // [ts] reporter: [
    // [ts] ['./echo-test-id-reporter.js'],
    // [ts] ['blob', { outputDir: 'blob-report' }],
    // [ts] ]
    // [ts] };`,
    // [ts] test('test 1', () => { });
    // [ts] test('test 2', () => { });
    // [ts] test('test 3', () => { });
    // [ts] test("TestSuite.project() should return owning project", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29173" });
    // [ts] const files1 = {
    // [ts] "echo-reporter.js": `
    // [ts] export default class EchoReporter {
    // [ts] onTestBegin(test) {
    // [ts] console.log('test project:', test.parent?.project()?.name);
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "merge.config.ts": `export default {
    // [ts] testDir: 'mergeRoot',
    // [ts] reporter: './echo-reporter.js'
    // [ts] };`,
    // [ts] "playwright.config.ts": `export default {
    // [ts] testDir: 'tests',
    // [ts] reporter: [['blob', { outputDir: 'blob-report' }]],
    // [ts] projects: [{name: 'my-project'}]
    // [ts] };`,
    // [ts] test('math 1', () => { });
    // [ts] test("open blob-1.42", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29984" });
    // [ts] runInlineTest({
    // [ts] "echo-reporter.js": `
    // [ts] export default class EchoReporter {
    // [ts] lines = [];
    // [ts] onTestBegin(test) {
    // [ts] this.lines.push(test.titlePath().join(' > '));
    // [ts] }
    // [ts] onEnd() {
    // [ts] console.log(this.lines.join('\\n'));
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "merge.config.ts": `export default {
    // [ts] testDir: 'mergeRoot',
    // [ts] reporter: './echo-reporter.js'
    // [ts] };`,
    // [ts] });
    // [ts] test("preserve static annotations when tests did not run", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30260" });
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: 'blob',
    // [ts] projects: [
    // [ts] { name: 'setup', testMatch: 'setup.js' },
    // [ts] { name: 'tests', testMatch: /.*test.js/, dependencies: ['setup'] },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "setup.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('setup', { annotation: [{ type: "sample", description: "uno" }] }, ({}) => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('first', { annotation: [{ type: "sample", description: "uno" }] }, ({}) => {});
    // [ts] test('second', { annotation: [{ type: "sample", description: "dos" }] }, ({}) => {});
    // [ts] `
    // [ts] };
    // [ts] runInlineTest(files);
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // Show skipped tests.
    // [ts] page.getByText("Skipped").click();
    // Check first annotation.
    // [ts] {
    // [ts] page.getByRole("link", { name: "first" }).click();
    // [ts] expect(page.getByText("sample: uno")).toBeVisible();
    // [ts] page.goBack();
    // [ts] }
    // Check second annotation.
    // [ts] {
    // [ts] page.getByRole("link", { name: "second" }).click();
    // [ts] expect(page.getByText("sample: dos")).toBeVisible();
    // [ts] page.goBack();
    // [ts] }
    // [ts] });
    // [ts] test("project filter in report name", () => {
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: 'blob',
    // [ts] projects: [
    // [ts] { name: 'foo' },
    // [ts] { name: 'b%/\\ar' },
    // [ts] { name: 'baz' },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1 @smoke', () => {});
    // [ts] `,
    // [ts] };
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] {
    // [ts] const result = runInlineTest(files, { shard: `2/2`, project: "foo" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report-foo-2.zip"]);
    // [ts] }
    // [ts] {
    // [ts] const result = runInlineTest(files, { shard: `1/2`, project: ["foo", "b*r"], grep: "smoke" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles.sort(), ["report-foo-b-r-c29b5fa-1.zip"]);
    // [ts] }
    // [ts] {
    // [ts] const result = runInlineTest({ ...files, "test-list.txt": `foo` }, { "test-list": "test-list.txt" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] const result2 = runInlineTest({ ...files, "test-list.txt": `bar` }, { "test-list": "test-list.txt" });
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] const reportFiles2 = fs.promises.readdir(reportDir);
    // [ts] expect_eq(reportFiles2.sort() != reportFiles.sort(), true);
    // [ts] }
    // [ts] });
    // [ts] test("should report duration across all shards", () => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] const config: PlaywrightTestConfig = {
    // [ts] reporter: [["blob", { outputDir: `${reportDir.replace("\\\\", "/")}` }]],
    // [ts] };
    // [ts] class CustomReporter {
    // [ts] onEnd: (result) => {
    // [ts] @print("%%" + JSON.stringify(result));
    // [ts] }
    // [ts] }
    // [ts] const files = {
    // [ts] "playwright.config.ts": `
    // [ts] export default ${JSON.stringify(config, null, 2)};
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('math 2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "reporter.js": `module.exports = ${CustomReporter.toString()};`,
    // [ts] };
    // [ts] runInlineTest(files, { shard: `1/2`, workers: 1 });
    // [ts] Promise.init(f => setTimeout(f, 1500)); // Ensure different start times.
    // [ts] runInlineTest(files, { shard: `2/2`, workers: 1 }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const { exitCode, outputLines } = mergeReports(reportDir, {}, { additionalArgs: ["--reporter", test.info().outputPath("reporter.js"), "-c", test.info().outputPath("playwright.config.ts")] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const { duration } = JSON.parse(outputLines[0]);
    // [ts] expect_eq(duration >= 1500, true);
    // [ts] });
    // [ts] test("shard chart", () => {
    // [ts] const reportDir = test.info().outputPath("blob-reports");
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] fullyParallel: true,
    // [ts] tag: process.env.BOT_TAG,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import timers from 'timers/promises';
    // [ts] test('one', () => {
    // [ts] timers.setTimeout(100);
    // [ts] });
    // [ts] test('two', () => {
    // [ts] timers.setTimeout(200);
    // [ts] });
    // [ts] test('three', () => {
    // [ts] timers.setTimeout(300);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] runInlineTest({}, { shard: "1/3" }, { PWTEST_BLOB_DO_NOT_REMOVE: "1", BOT_TAG: "@linux" });
    // [ts] runInlineTest({}, { shard: "2/3" }, { PWTEST_BLOB_DO_NOT_REMOVE: "1", BOT_TAG: "@linux" });
    // [ts] runInlineTest({}, { shard: "3/3" }, { PWTEST_BLOB_DO_NOT_REMOVE: "1", BOT_TAG: "@linux" });
    // [ts] runInlineTest({}, { shard: "1/2" }, { PWTEST_BLOB_DO_NOT_REMOVE: "1", BOT_TAG: "@mac" });
    // [ts] runInlineTest({}, { shard: "2/2" }, { PWTEST_BLOB_DO_NOT_REMOVE: "1", BOT_TAG: "@mac" });
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] page.getByRole("link", { name: "Speedboard" }).click();
    // [ts] expect(page).toMatchAriaSnapshot(`
    // [ts] - button "Timeline"
    // [ts] - region:
    // [ts] - img:
    // [ts] - listitem /@linux/
    // [ts] - listitem /@linux/
    // [ts] - listitem /@linux/
    // [ts] - listitem /@mac/
    // [ts] - listitem /@mac/
    // [ts] `);
    // [ts] });
    // [ts] test("should populate projects in config when merging reports", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43853" }
    // [ts] }, (_p) => {
    // [ts] const reportDir = test.info().outputPath("blob-report");
    // [ts] server.setRoute("/ready", (req, res) => res.end());
    // [ts] class CustomReporter {
    // [ts] onBegin: (config, suite) => {
    // [ts] const projectNames = config.projects.map(p => p.name);
    // [ts] @print("%%" + JSON.stringify(projectNames));
    // [ts] @print("%%" + JSON.stringify(config.projects.map(p => p.webServer)));
    // [ts] @print("%%" + JSON.stringify(suite.suites.map(s => s.project().webServer)));
    // [ts] @print("%%" + JSON.stringify(config.webServer));
    // [ts] }
    // [ts] }
    // [ts] const files = {
    // [ts] "reporter.js": `module.exports = ${CustomReporter.toString()};`,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]],
    // [ts] webServer: {
    // [ts] command: 'echo private-top-level-command',
    // [ts] env: { TOKEN: 'private-top-level-token' },
    // [ts] cwd: __dirname,
    // [ts] url: '${server.PREFIX}/ready',
    // [ts] reuseExistingServer: true,
    // [ts] },
    // [ts] projects: [
    // [ts] { name: 'setup' },
    // [ts] {
    // [ts] name: 'p1',
    // [ts] dependencies: ['setup'],
    // [ts] webServer: {
    // [ts] command: 'echo private-server-command',
    // [ts] env: { TOKEN: 'private-server-token' },
    // [ts] cwd: __dirname,
    // [ts] url: '${server.PREFIX}/ready',
    // [ts] reuseExistingServer: true,
    // [ts] },
    // [ts] },
    // [ts] { name: 'p2', dependencies: ['setup'] },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('test 1', () => {});
    // [ts] `,
    // [ts] };
    // [ts] expect_eq((runInlineTest(files, { shard: `1/2`, workers: 1 })).exitCode, 0);
    // [ts] expect_eq((runInlineTest(files, { shard: `2/2`, workers: 1 }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" })).exitCode, 0);
    // [ts] const reportFiles = fs.promises.readdir(reportDir);
    // [ts] expect(reportFiles).toHaveLength(2);
    // [ts] for (const [index, reportFile] of reportFiles.entries()) {
    // [ts] const events = extractReport(path.join(reportDir, reportFile), test.info().outputPath(`extracted-${index}`));
    // [ts] const projects = events.filter(e => e.method == "onProject").map(e => e.params.project);
    // [ts] expect_eq(projects.length > 0, true);
    // [ts] for (const project of projects)
    // [ts] expect(project).not.toHaveProperty("webServer");
    // [ts] expect(events.find(e => e.method == "onConfigure").params.config).not.toHaveProperty("webServer");
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(events), "private-server-token") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(events), "private-server-command") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(events), "private-top-level-token") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(events), "private-top-level-command") == null, true);
    // [ts] }
    // [ts] const { exitCode, outputLines } = mergeReports(reportDir, {}, { additionalArgs: ["--reporter", test.info().outputPath("reporter.js")] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] const projectNames = JSON.parse(outputLines[0]);
    // [ts] expect_eq(projectNames, ["setup", "p1", "setup", "p2"]);
    // [ts] expect_eq(JSON.parse(outputLines[1]), [[], [], [], []]);
    // [ts] expect_eq(JSON.parse(outputLines[2]), [[], [], [], []]);
    // [ts] expect_eq(JSON.parse(outputLines[3]), null);
    // [ts] });
    // [ts] test("workerIndex is rebased", () => {
    // [ts] const reportDir = test.info().outputPath("blob-reports");
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] fullyParallel: true,
    // [ts] reporter: [['blob', { outputDir: '${reportDir.replace(/\\/g, '/')}' }]]
    // [ts] };
    // [ts] `,
    // [ts] "a.test.js": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test-one', () => {});
    // [ts] test('test-two', () => {});
    // [ts] `,
    // [ts] });
    // [ts] runInlineTest({}, { shard: "1/2", workers: "1" }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] runInlineTest({}, { shard: "2/2", workers: "1" }, { PWTEST_BLOB_DO_NOT_REMOVE: "1" });
    // [ts] const { exitCode } = mergeReports(reportDir, { "PLAYWRIGHT_HTML_OPEN": "never" }, { additionalArgs: ["--reporter", "html"] });
    // [ts] expect_eq(exitCode, 0);
    // [ts] showReport();
    // [ts] page.getByRole("link", { name: "test-two", exact: true }).click();
    // [ts] page.getByRole("button", { name: "Executed in Worker #1" }).click();
    // [ts] expect(page.getByTestId("worker-test-list")).toMatchAriaSnapshot(`
    // [ts] - 'button "Executed in Worker #1" [expanded]'
    // [ts] - region:
    // [ts] - list:
    // [ts] - listitem:
    // [ts] - link "test-two"
    // [ts] - link "a.test.js:4"
    // [ts] `);
    // [ts] });
}
