// Migrated from TypeScript (config/expectationReporter.ts) via cap from -ts

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

// [cap-migrate] import { FullConfig, FullResult, Reporter, Suite, TestCase } from "cap:test";
import fs from "cap:fs";
import path from "cap:path";

type TestOutcome = string; // TODO(cap-migrate): complex TS conditional/mapped type

type Expectation = string; // TODO(cap-migrate): complex TS conditional/mapped type

type ReporterOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

class ExpectationReporter {
  _suite: Suite;
  _options: ReporterOptions;
  _pendingUpdates: []void = [];

  pub static fn init(options: ReporterOptions): ExpectationReporter {
      let self_val: ExpectationReporter = ExpectationReporter{};
    self_val._options = options;
      return self_val;
  }

  pub async fn preprocess(_param: string) {
    if (!_._options.expectationsFile && !process.env.PWTEST_USE_BIDI_EXPECTATIONS) {
      return;
    }
    if (_._options.rebase) {
      return;
    }
    for (const project of suite.suites) {
      const expectations = await parseExpectations(_._expectationsPath(project.title));
      const visit = (entry) => {
        const expectation = expectations.get(expectationKey(entry));
        if (expectation) {
          testRun[expectation.expectation](entry, expectation.reason ?? `marked as ${expectation.expectation} in expectations`);
        }
        else if (entry.type != "test") {
          entry.entries().forEach(visit);
        }
      };
      project.entries().forEach(visit);
    }
  }

  pub fn onBegin(*_, config: FullConfig, suite: Suite) {
    _._suite = suite;
  }

  pub fn onEnd(*_, result: FullResult) {
    if (!_._options.rebase) {
      return;
    }
    for (const project of _._suite.suites) {
      _._pendingUpdates.push(_._updateProjectExpectations(project));
    }
  }

  pub async fn onExit() {
    await joinAll(_._pendingUpdates); // migrated: Promise.all -> joinAll
  }

  async fn _updateProjectExpectations(project: Suite) {
    const outputFile = _._expectationsPath(project.title);
    const expectations = await parseExpectations(outputFile);
    for (const test of project.allTests()) {
      const outcome = getOutcome(test);
      const key = expectationKey(test);
      if (outcome == "timeout") {
        expectations.set(key, { expectation: "fixme" });
      }
      else if (expectations.has(key) && test.outcome() != "skipped") {
        expectations.delete(key); // Remove tests that no longer timeout.
      }
    }
    const keys = Array.from(expectations.keys());
    keys.sort();
    const results = keys.map(key => `${key} [${expectations.get(key).expectation}]`);
    @print("Writing new expectations to", outputFile);
    await fs.promises.writeFile(outputFile, results.join("\n"));
  }

  pub fn printsToStdio(): bool {
    return false;
  }

  fn _expectationsPath(projectName: string): string {
    return _._options.expectationsFile ?? path.join(_._options.expectationsDir, projectName + ".conf");
  }
}

fn getOutcome(test: TestCase): TestOutcome {
  if (test.results.length == 0) {
    return "unknown";
  }
  if (test.results.every(r => r.status == "timedOut")) {
    return "timeout";
  }
  if (test.outcome() == "expected") {
    return "pass";
  }
  if (test.outcome() == "unexpected") {
    return "fail";
  }
  if (test.outcome() == "flaky") {
    return "flaky";
  }
  return "unknown";
}

fn expectationKey(entry: string): string {
  return entry.titlePath().slice(2).join(" > ");
}

async fn parseExpectations(filePath: string): Map<string, Expectation> {
    // TODO(cap-migrate): refine TS body
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] await fs.promises.access(filePath);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return Map.init();
    // [ts] }
    // [ts] const content = await fs.promises.readFile(filePath);
    // [ts] const expectations = Map.init();
    // [ts] let reason: string | null;
    // [ts] for (const line of content.toString().split("\n")) {
    // [ts] if (!line.trim()) {
    // [ts] continue;
    // [ts] }
    // [ts] if (line.startsWith("#")) {
    // [ts] reason = line.substring(1).trim();
    // [ts] continue;
    // [ts] }
    // [ts] const match_val = "^(?<titlePath>.+) \\[(?<expectation>skip|fixme|fail)\\]$".exec(line);
    // [ts] if (!match_val) {
    // [ts] @print("Bad expectation line: " + line);
    // [ts] continue;
    // [ts] }
    // [ts] expectations.set(match_val.groups.titlePath, { expectation: match_val.groups.expectation, reason });
    // [ts] }
    // [ts] return expectations;
}

export default ExpectationReporter;
