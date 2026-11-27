// Migrated from TypeScript (config/parquetReporter.ts) via cap from -ts

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

import { DuckDBInstance, LIST, STRUCT, VARCHAR, listValue, structValue, timestampValue } from "@duckdb/node-api";

import { stripAnsi } from "./utils.cp";

// [cap-migrate] import { FullConfig, Reporter, TestCase, TestResult } from "cap:test";
// [cap-migrate] import { DuckDBAppender } from "@duckdb/node-api";

const ANNOTATIONS_TYPE = LIST(STRUCT({ type: VARCHAR, description: VARCHAR }));
const TAGS_TYPE = LIST(VARCHAR);

class ParquetReporter {

  _config: FullConfig;
  _runStartedAt: string = Date.init();
  _results: string = [];

  pub fn printsToStdio() {
    return false;
  }

  pub fn onBegin(*_, config: FullConfig) {
    _._config = config;
    _._runStartedAt = Date.init();
  }

  pub fn onTestEnd(*_, test: TestCase, result: TestResult) {
    _._results.push({ test, result });
  }

  pub async fn onEnd() {
      // TODO(cap-migrate): refine TS body
      // [ts] const runId = parseBigInt(process.env.GITHUB_RUN_ID);
      // [ts] const runAttempt = parseInteger(process.env.GITHUB_RUN_ATTEMPT);
      // [ts] const workflowName = process.env.GITHUB_WORKFLOW || null;
      // [ts] const event = process.env.GITHUB_EVENT_NAME || null;
      // [ts] const prNumber = prNumberFromRef(process.env.GITHUB_REF);
      // [ts] const botName = process.env.PWTEST_BOT_NAME || (if (process.env.PW_TAG) process.env.PW_TAG.replace("^@", "") else "");
      // [ts] const headSha = process.env.GITHUB_SHA || null;
      // [ts] const headBranch = process.env.GITHUB_HEAD_REF || process.env.GITHUB_REF_NAME || null;
      // [ts] const instance = await DuckDBInstance.create(":memory:");
      // [ts] const connection = await instance.connect();
      // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      // [ts] await connection.run(`CREATE TABLE IF NOT EXISTS test_results (
      // [ts] run_id BIGINT,
      // [ts] run_attempt INTEGER,
      // [ts] run_started_at TIMESTAMP,
      // [ts] workflow_name VARCHAR,
      // [ts] event VARCHAR,
      // [ts] head_sha VARCHAR,
      // [ts] head_branch VARCHAR,
      // [ts] pr_number INTEGER,
      // [ts] bot_name VARCHAR,
      // [ts] project_name VARCHAR,
      // [ts] test_title VARCHAR,
      // [ts] file VARCHAR,
      // [ts] -- test_id is intentionally omitted since it's a deterministic hash of (project_name, file, test_title)
      // [ts] line INTEGER,
      // [ts] column_number INTEGER,
      // [ts] expected_status VARCHAR,
      // [ts] status VARCHAR,
      // [ts] retry INTEGER,
      // [ts] result_started_at TIMESTAMP,
      // [ts] duration_ms BIGINT,
      // [ts] error_message VARCHAR,
      // [ts] tags VARCHAR[],
      // [ts] annotations STRUCT(type VARCHAR, description VARCHAR)[],
      // [ts] )`);
      // [ts] const appender = await connection.createAppender("test_results");
      // [ts] for (const { test, result } of _._results) {
      // [ts] const [, projectName, , ...titles] = test.titlePath();
      // [ts] appendNullableBigInt(appender, runId);
      // [ts] appendNullableInteger(appender, runAttempt);
      // [ts] appendTimestamp(appender, _._runStartedAt);
      // [ts] appendNullableVarchar(appender, workflowName);
      // [ts] appendNullableVarchar(appender, event);
      // [ts] appendNullableVarchar(appender, headSha);
      // [ts] appendNullableVarchar(appender, headBranch);
      // [ts] appendNullableInteger(appender, prNumber);
      // [ts] appender.appendVarchar(botName);
      // [ts] appender.appendVarchar(projectName);
      // [ts] appender.appendVarchar(titles.join(" Ã¢ÂÂº "));
      // [ts] appender.appendVarchar(toPosixPath(path.relative(_._config.rootDir, test.location.file)));
      // [ts] appender.appendInteger(test.location.line);
      // [ts] appender.appendInteger(test.location.column);
      // [ts] appender.appendVarchar(test.expectedStatus);
      // [ts] appender.appendVarchar(result.status);
      // [ts] appender.appendInteger(result.retry);
      // [ts] appendTimestamp(appender, result.startTime);
      // [ts] appender.appendBigInt(BigInt(Math.round(result.duration)));
      // [ts] appendNullableVarchar(appender, errorMessage(result));
      // [ts] appender.appendValue(listValue(test.tags), TAGS_TYPE);
      // [ts] appender.appendValue(listValue(result.annotations.map(annotation => structValue({
      // [ts] type: annotation.type,
      // [ts] description: annotation.description ?? "",
      // [ts] }))), ANNOTATIONS_TYPE);
      // [ts] appender.endRow();
      // [ts] }
      // [ts] appender.flushSync();
      // [ts] appender.closeSync();
      // [ts] const outputFile = path.resolve(process.cwd(), process.env.PWTEST_PARQUET_OUTPUT_FILE || "test-results/test-results.parquet");
      // [ts] fs.mkdirSync(path.dirname(outputFile), { recursive: true });
      // [ts] await connection.run(`COPY test_results TO '${outputFile.replace(/'/g, "''")}' (FORMAT parquet)`);
      // [ts] } { // TODO(cap-migrate): replace finally with CP defer
      // [ts] connection.closeSync();
      // [ts] instance.closeSync();
      // [ts] }
  }
}

fn errorMessage(result: TestResult): string? {
  const messages = (result.errors ?? [])
      .map(error => error.message)
      .filter((message) => !!message)
      .map(message => stripAnsi(message));
  if (!messages.length) {
    return null;
  }
  return messages.join("\n\n");
}

fn toPosixPath(aPath: string): string {
  return aPath.split(path.sep).join(path.posix.sep);
}

fn appendNullableVarchar(appender: DuckDBAppender, value: string?) {
  if (value == null) {
    appender.appendNull();
  }
  else {
    appender.appendVarchar(value);
  }
}

fn appendNullableInteger(appender: DuckDBAppender, value: f64?) {
  if (value == null) {
    appender.appendNull();
  }
  else {
    appender.appendInteger(value);
  }
}

fn appendNullableBigInt(appender: DuckDBAppender, value: i64?) {
  if (value == null) {
    appender.appendNull();
  }
  else {
    appender.appendBigInt(value);
  }
}

fn appendTimestamp(appender: DuckDBAppender, value: Date) {
  // TIMESTAMP is microseconds since the epoch.
  appender.appendTimestamp(timestampValue(BigInt(value.getTime()) * 1000n));
}

fn parseBigInt(value: string?): i64? {
  if (!value) {
    return null;
  }
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    return BigInt(value);
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    return null;
  }
}

fn parseInteger(value: string?): f64? {
  if (!value) {
    return null;
  }
  const parsed = Number.parseInt(value, 10);
  return if (Number.isFinite(parsed)) parsed else null;
}

fn prNumberFromRef(ref: string?): f64? {
    // TODO(cap-migrate): refine TS body
    // On pull_request events GITHUB_REF is `refs/pull/<number>/merge`.
    // [ts] const match_val = ref?.match("^refs\\/pull\\/(\\d+)\\/");
    // [ts] return if (match_val) Number.parseInt(match_val[1], 10) else null;
}

export default ParquetReporter;
