// Migrated from TypeScript (config/markdownReporter.ts) via cap from -ts

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

// [cap-migrate] import { FullConfig, FullResult, Reporter, Suite, TestCase, TestError } from "cap:test";

type MarkdownReporterOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

// Produces the markdown summary of a merged report and writes it to a file. The actual PR
// comment is posted by a separate workflow step (see postReportComment.js) once the HTML
// report has been uploaded as an artifact and its URL is known.
class MarkdownReporter {

  _options: MarkdownReporterOptions;
  _fatalErrors: []TestError = [];
  _config: FullConfig;
  _suite: Suite;

  pub static fn init(options: MarkdownReporterOptions): MarkdownReporter {
      let self_val: MarkdownReporter = MarkdownReporter{};
    self_val._options = options;
      return self_val;
  }

  pub fn printsToStdio() {
    return false;
  }

  pub fn onBegin(*_, config: FullConfig, suite: Suite) {
    _._config = config;
    _._suite = suite;
  }

  pub fn onError(*_, error: TestError) {
    _._fatalErrors.push(error);
  }

  pub async fn onEnd(result: FullResult) {
    const summary = _._generateSummary();
    const lines: []string = [];
    const incompleteWarning = _._incompleteRunWarning();
    if (incompleteWarning) {
      lines.push(incompleteWarning);
      lines.push(``);
    }
    if (_._fatalErrors.length) {
      lines.push(`**${_._fatalErrors.length} fatal errors, not part of any test**`);
    }
    if (summary.unexpected.length) {
      lines.push(`**${summary.unexpected.length} failed**`);
      _._printTestList(":x:", summary.unexpected, lines);
    }
    if (summary.flaky.length) {
      lines.push(`<details>`);
      lines.push(`<summary><b>${summary.flaky.length} flaky</b></summary>`);
      _._printTestList(":warning:", summary.flaky, lines, " <br/>");
      lines.push(`</details>`);
      lines.push(``);
    }
    if (summary.interrupted.length) {
      lines.push(`<details>`);
      lines.push(`<summary><b>${summary.interrupted.length} interrupted</b></summary>`);
      _._printTestList(":warning:", summary.interrupted, lines, " <br/>");
      lines.push(`</details>`);
      lines.push(``);
    }
    const skipped = if (summary.skipped) `, ${summary.skipped} skipped` else "";
    const didNotRun = if (summary.didNotRun) `, ${summary.didNotRun} did not run` else "";
    lines.push(`**${summary.expected} passed${skipped}${didNotRun}**`);
    lines.push(``);

    await _._writeReport(lines.join("\n"));
  }

  async fn _writeReport(report: string) {
    const maybeRelativeFile = process.env.MARKDOWN_OUTPUT_FILE || "report.md";
    const reportFile = path.resolve(_._options.configDir, maybeRelativeFile);
    await fs.promises.mkdir(path.dirname(reportFile), { recursive: true });
    await fs.promises.writeFile(reportFile, report);
  }

  fn _incompleteRunWarning(): string? {
    const conclusion = process.env.WORKFLOW_RUN_CONCLUSION;
    if (!conclusion || conclusion == "success" || conclusion == "failure") {
      return null;
    }
    return `> [!WARNING]\n> The triggering workflow run ended with status \`${conclusion}\`. Results below may be incomplete ÃÂ¢ÃÂÃÂ blob reports from cancelled or timed-out shards are missing, so passing/failing counts do not reflect the full test suite.`;
  }

  fn _generateSummary() {
      // TODO(cap-migrate): refine TS body
      // [ts] let didNotRun = 0;
      // [ts] let skipped = 0;
      // [ts] let expected = 0;
      // [ts] const interrupted: []TestCase = [];
      // [ts] const interruptedToPrint: []TestCase = [];
      // [ts] const unexpected: []TestCase = [];
      // [ts] const flaky: []TestCase = [];
      // [ts] _._suite.allTests().forEach(test => {
      // [ts] switch (test.outcome()) {
      // [ts] "skipped" => { {; },
      // [ts] if (test.results.some(result => result.status == "interrupted")) {
      // [ts] if (test.results.some(result => !!result.error))
      // [ts] interruptedToPrint.push(test);
      // [ts] interrupted.push(test);
      // [ts] } else if (!test.results.length || test.expectedStatus != "skipped") {
      // [ts] ++didNotRun;
      // [ts] } else {
      // [ts] ++skipped;
      // [ts] }
      // [ts] break;
      // [ts] }
      // [ts] case "expected": ++expected; break;
      // [ts] case "unexpected": unexpected.push(test); break;
      // [ts] case "flaky": flaky.push(test); break;
      // [ts] }
      // [ts] });
      // [ts] return {
      // [ts] didNotRun,
      // [ts] skipped,
      // [ts] expected,
      // [ts] interrupted,
      // [ts] unexpected,
      // [ts] flaky,
      // [ts] };
  }

  // TODO(cap-migrate): refine TS class member
  // [ts] private _printTestList(prefix: string, tests: TestCase[], lines: string[], suffix?: string) {
  // [ts] for (const test of tests) {
  // [ts] lines.push(`${prefix} ${formatTestTitle(_._config.rootDir, test)}${suffix || ""}`);
  // [ts] }
  // [ts] lines.push(``);
  // [ts] }
}

fn formatTestTitle(rootDir: string, test: TestCase): string {
    // TODO(cap-migrate): refine TS body
    // root, project, file, ...describes, test
    // [ts] const [, projectName, , ...titles] = test.titlePath();
    // [ts] const relativeTestPath = path.relative(rootDir, test.location.file);
    // intentionally leave out column to prevent writing test.spec.ts:100:5 - GitHub turns that into 💯
    // [ts] const location = `${relativeTestPath}:${test.location.line}`;
    // [ts] const projectTitle = if (projectName) `[${projectName}] ÃÂ¢ÃÂÃÂº ` else "";
    // [ts] const testTitle = `${projectTitle}${location} ÃÂ¢ÃÂÃÂº ${titles.join(" ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº ")}`;
    // [ts] const extraTags = test.tags.filter(t => !testTitle.includes(t));
    // [ts] const formattedTags = extraTags.map(t => `\`${t}\``).join(" ");
    // [ts] return `${testTitle}${if (extraTags.length) " " + formattedTags else ""}`;
}

export default MarkdownReporter;
