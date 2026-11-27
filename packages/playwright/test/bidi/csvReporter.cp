// Migrated from TypeScript (bidi/csvReporter.ts) via cap from -ts

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

// [cap-migrate] import { FullConfig, FullResult, Reporter, Suite } from "cap:test";
import { stripAnsi } from "../config/utils.cp";
import fs from "cap:fs";
import path from "cap:path";


type ReporterOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

const header = ["Test Name", "Expected Status", "Status", "Error Message"];

class CsvReporter {

  _suite: Suite;
  _options: ReporterOptions;
  _pendingWrite: void;

  pub static fn init(options: ReporterOptions): CsvReporter {
      let self_val: CsvReporter = CsvReporter{};
    self_val._options = options;
      return self_val;
  }

  pub fn onBegin(*_, config: FullConfig, suite: Suite) {
    _._suite = suite;
  }

  pub fn onEnd(*_, result: FullResult) {
      // TODO(cap-migrate): refine TS body
      // [ts] const rows: [][]string = [];
      // [ts] for (const project of _._suite.suites) {
      // [ts] for (const file of project.suites) {
      // [ts] for (const test of file.allTests()) {
      // Report fixme tests as failing.
      // [ts] const fixme = test.annotations.find(a => a.type == "fixme");
      // [ts] if (test.ok() && !fixme) {
      // [ts] continue;
      // [ts] }
      // [ts] const row = [];
      // [ts] const [, , , ...titles] = test.titlePath();
      // [ts] row.push(`${file.title} ÃÂ¢ÃÂÃÂº ${titles.join(" ÃÂÃÂÃÂÃÂ¢ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂº ")}`);
      // [ts] row.push(test.expectedStatus);
      // [ts] row.push(test.outcome());
      // [ts] if (fixme) {
      // [ts] row.push("fixme" + (if (fixme.description) `: ${fixme.description.replace("\\s+", " ")}` else ""));
      // [ts] } else {
      // [ts] const result = test.results.find(r => r.error);
      // [ts] if (result) {
      // [ts] const errorMessage = stripAnsi(result.error?.message.replace("\\s+", " ").trim().substring(0, 1024) ?? "");
      // [ts] row.push(errorMessage);
      // [ts] } else {
      // [ts] const fail = test.annotations.find(a => a.type == "fail");
      // [ts] if (fail) {
      // [ts] row.push(`Should have failed: ${fail.description}`);
      // [ts] }
      // [ts] else {
      // [ts] row.push("");
      // [ts] }
      // [ts] }
      // [ts] }
      // [ts] rows.push(row);
      // [ts] }
      // [ts] }
      // [ts] }
      // [ts] const reportFile = path.resolve(_._options.configDir, _._options.outputFile || "test-results.csv");
      // [ts] _._pendingWrite = (async () => {
      // [ts] await fs.promises.mkdir(path.dirname(reportFile), { recursive: true });
      // [ts] const csv = [header, ...rows].map(r => r.map(csvEscape).join(",")).join("\n");
      // [ts] await fs.promises.writeFile(reportFile, csv);
      // [ts] })();
  }

  pub async fn onExit() {
    await _._pendingWrite;
  }

  pub fn printsToStdio(): bool {
    return false;
  }
}

fn csvEscape(str: auto) {
    // TODO(cap-migrate): refine TS body
    // [ts] if (str.includes("\"") || str.includes(",") || str.includes("\n")) {
    // [ts] return `"${str.replace(/"/g, '""')}"`;
    // [ts] }
    // [ts] return str;
}

export default CsvReporter;
