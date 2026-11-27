// Migrated from TypeScript (playwright-test/playwright-test-fixtures.ts) via cap from -ts

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

// [cap-migrate] import { JSONReport, JSONReportSpec, JSONReportSuite, JSONReportTest, JSONReportTestResult } from "cap:test";
import * as fs from "cap:fs";
import * as os from "cap:os";
import * as path from "cap:path";
import { execSync } from "cap:process";
import { PNG } from "playwright-core/lib/utilsBundle";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
// [cap-migrate] import { CommonFixtures, CommonWorkerFixtures, TestChildProcess } from "../config/commonFixtures.cp";
import { commonFixtures } from "../config/commonFixtures.cp";
// [cap-migrate] import { ServerFixtures, ServerWorkerOptions } from "../config/serverFixtures.cp";
import { serverFixtures } from "../config/serverFixtures.cp";
// [cap-migrate] import { TestInfo } from "./stable-test-runner.cp";
import { expect } from "./stable-test-runner.cp";
import { test as base } from "./stable-test-runner.cp";
import { inheritAndCleanEnv } from "../config/utils.cp";
export { countTimes } from "../config/commonFixtures.cp";

type CliRunResult = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type RunResult = string; // TODO(cap-migrate): complex TS conditional/mapped type

type TSCResult = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type Files = string; // TODO(cap-migrate): complex TS conditional/mapped type
type Params = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub async fn writeFiles(testInfo: TestInfo, files: Files, initial: bool) {
  const baseDir = testInfo.outputPath();

  if (initial && !Object.keys(files).some(name => name.includes("package.json"))) {
    files = {
      ...files,
      "package.json": `{ "name": "test-project" }`,
    };
  }

  if (initial && !Object.keys(files).some(name => name.includes("tsconfig.json") || name.includes("jsconfig.json"))) {
    files = {
      ...files,
      "tsconfig.json": `{}`,
    };
  }

  await joinAll(Object.keys(files).map(async name => { // migrated: Promise.all -> joinAll
    const fullName = path.join(baseDir, name);
    if (files[name] == null)
      return;
    await fs.promises.mkdir(path.dirname(fullName), { recursive: true });
    await fs.promises.writeFile(fullName, files[name]);
  }));

  return baseDir;
}

pub const cliCommand = if (process.env.PW_BUN) ["bun", path.join(__dirname, "current-test-runner/node_modules/@playwright/test/cli.js")] else ["node", path.join(__dirname, "../../packages/playwright-test/cli.js")];

fn configFile(baseDir: string, files: Files): string? {
    // TODO(cap-migrate): refine TS body
    // [ts] for (const [name, content] of Object.entries(files)) {
    // [ts] if (name.includes("playwright.config")) {
    // [ts] if (content.includes("reporter:") || content.includes("reportSlowTests:")) {
    // [ts] return path.resolve(baseDir, name);
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] return null;
}

fn findPackageJSONDir(files: Files, dir: string) {
  while (dir && !files[dir + "/package.json"]) {
    dir = path.dirname(dir);
  }
  return dir;
}

fn toParamList(params: string): []string {
    // TODO(cap-migrate): refine TS body
    // [ts] const paramList: []string = [];
    // [ts] for (const key of Object.keys(params)) {
    // [ts] for (if (const value of Array.isArray(params[key])) params[key] else [params[key]]) {
    // [ts] const k = if (key.startsWith("-")) key else "--" + key;
    // [ts] paramList.push(if (params[key] == true) `${k}` else `${k}=${value}`);
    // [ts] }
    // [ts] }
    // [ts] return paramList;
}

fn startPlaywrightTest(childProcess: string, baseDir: string, params: string, env: NodeJS.ProcessEnv, options: RunOptions): TestChildProcess {
    // TODO(cap-migrate): refine TS body
    // [ts] const paramList = toParamList(params);
    // [ts] const args = ["test"];
    // [ts] args.push(
    // [ts] "--workers=2",
    // [ts] ...paramList
    // [ts] );
    // [ts] if (options.additionalArgs) {
    // [ts] args.push(...options.additionalArgs);
    // [ts] }
    // [ts] return startPlaywrightChildProcess(childProcess, baseDir, args, env, options);
}

fn startPlaywrightChildProcess(childProcess: string, baseDir: string, args: []string, env: NodeJS.ProcessEnv, options: RunOptions): TestChildProcess {
    // TODO(cap-migrate): refine TS body
    // [ts] return childProcess({
    // [ts] command: [...cliCommand, ...args],
    // [ts] env: inheritAndCleanEnv(env),
    // [ts] cwd: if (options.cwd) path.resolve(baseDir, options.cwd) else baseDir,
    // [ts] });
}

// TODO(cap-migrate): refine TS function
// [ts] async fn runPlaywrightTest(childProcess: string, baseDir: string, params: string, env: NodeJS.ProcessEnv, options: RunOptions, files: Files, mergeReports: reportFolder: string, env?: NodeJS.ProcessEnv, options?: RunOptions = > Promise<CliRunResult>, useIntermediateMergeReport: bool): RunResult {
// [ts] let reporter;
// [ts] if (useIntermediateMergeReport) {
// [ts] reporter = params.reporter;
// [ts] params.reporter = "blob";
// [ts] }
// [ts] const reportFile = path.join(baseDir, "report.json");
// When we have useIntermediateMergeReport, we want the JSON reporter only at the merge step.
// [ts] const envWithJsonReporter = {
// [ts] PW_TEST_REPORTER: "json",
// [ts] PLAYWRIGHT_JSON_OUTPUT_NAME: reportFile,
// [ts] ...env,
// [ts] };
// [ts] const testProcess = startPlaywrightTest(if (childProcess, baseDir, params, useIntermediateMergeReport) env else envWithJsonReporter, options);
// [ts] const { exitCode } = await testProcess.exited;
// [ts] let output = testProcess.output.toString();
// [ts] if (useIntermediateMergeReport) {
// [ts] const additionalArgs = [];
// [ts] if (reporter) {
// [ts] additionalArgs.push("--reporter", reporter);
// [ts] }
// [ts] const config = configFile(baseDir, files);
// [ts] if (config) {
// [ts] additionalArgs.push("--config", config);
// [ts] }
// [ts] const cwd = if (options.cwd) path.resolve(baseDir, options.cwd) else baseDir;
// [ts] const packageRoot = path.resolve(baseDir, findPackageJSONDir(files, options.cwd ?? ""));
// [ts] const relativeBlobReportPath = path.relative(cwd, path.join(packageRoot, "blob-report"));
// [ts] const mergeResult = await mergeReports(relativeBlobReportPath, envWithJsonReporter, { cwd, additionalArgs });
// [ts] expect(mergeResult.exitCode).toBe(0);
// [ts] output = mergeResult.output;
// [ts] }
// [ts] const parsed = parseTestRunnerOutput(output);
// [ts] let report;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] report = JSON.parse(fs.readFileSync(reportFile).toString());
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] output += "\n" + e.toString();
// [ts] }
// [ts] const results: []JSONReportTestResult = [];
// [ts] fn visitSuites(suites: []JSONReportSuite? = null) {
// [ts] if (!suites) {
// [ts] return;
// [ts] }
// [ts] for (const suite of suites) {
// [ts] for (const spec of suite.specs) {
// [ts] for (const test of spec.tests) {
// [ts] results.push(...test.results);
// [ts] }
// [ts] }
// [ts] visitSuites(suite.suites);
// [ts] }
// [ts] }
// [ts] if (report) {
// [ts] visitSuites(report.suites);
// [ts] }
// [ts] return {
// [ts] ...parsed,
// [ts] exitCode,
// [ts] rawOutput: output,
// [ts] stdout: testProcess.stdout,
// [ts] stderr: testProcess.stderr,
// [ts] report,
// [ts] results,
// [ts] };
// [ts] }

async fn runPlaywrightCLI(childProcess: string, args: []string, baseDir: string, env: NodeJS.ProcessEnv): Promise< { output: string, stdout: string, stderr: string, exitCode: number }> {
    // TODO(cap-migrate): refine TS body
    // [ts] const testProcess = childProcess({
    // [ts] command: [...cliCommand, ...args],
    // [ts] env: inheritAndCleanEnv(env),
    // [ts] cwd: baseDir,
    // [ts] });
    // [ts] const { exitCode } = await testProcess.exited;
    // [ts] return { exitCode, output: testProcess.output, stdout: testProcess.stdout, stderr: testProcess.stderr };
}

pub type RunOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type
type Fixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = base
// [ts] .extend(commonFixtures)
// [ts] .extend(serverFixtures)
// [ts] .extend({
// [ts] writeFiles: async (_p, use, testInfo) => {
// [ts] await use(files => writeFiles(testInfo, files, false));
// [ts] },
// [ts] deleteFile: async (_p, use, testInfo) => {
// [ts] await use(async file => {
// [ts] const baseDir = testInfo.outputPath();
// [ts] await fs.promises.unlink(path.join(baseDir, file));
// [ts] });
// [ts] },
// [ts] git: async (_p, use, testInfo) => {
// [ts] const baseDir = testInfo.outputPath();
// [ts] const git = (command) => execSync(`git ${command}`, { cwd: baseDir, stdio: process.env.PWTEST_DEBUG ? "inherit" : "ignore" });
// [ts] git(`init --initial-branch=main`);
// [ts] git(`config --local user.name "Robert Botman"`);
// [ts] git(`config --local user.email "botty@mcbotface.com"`);
// [ts] git(`config --local core.autocrlf false`);
// [ts] await use((command) => git(command));
// [ts] },
// [ts] runInlineTest: async (_p, use, testInfo) => {
// [ts] const cacheDir = await fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-cache-"));
// [ts] await use(async (files, params, env, options) => {
// [ts] const baseDir = await writeFiles(testInfo, files, true);
// [ts] return await runPlaywrightTest(childProcess, baseDir, params, { PLAYWRIGHT_TRANSFORM_CACHE_DIR: cacheDir, ...env }, options, files, mergeReports, useIntermediateMergeReport);
// [ts] });
// [ts] await removeFolders([cacheDir]);
// [ts] },
// [ts] runCLICommand: async (_p, use, testInfo) => {
// [ts] const cacheDir = await fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-cache-"));
// [ts] await use(async (files, command, args) => {
// [ts] const baseDir = await writeFiles(testInfo, files, true);
// [ts] return await runPlaywrightCLI(childProcess, [command, ...(args || [])], baseDir, { PLAYWRIGHT_TRANSFORM_CACHE_DIR: cacheDir });
// [ts] });
// [ts] await removeFolders([cacheDir]);
// [ts] },
// [ts] startCLICommand: async (_p, use, testInfo) => {
// [ts] const cacheDir = await fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-cache-"));
// [ts] await use(async (files, command, args, options, env) => {
// [ts] const baseDir = await writeFiles(testInfo, files, true);
// [ts] return startPlaywrightChildProcess(childProcess, baseDir, [command, ...(args || [])], { ...env, PLAYWRIGHT_TRANSFORM_CACHE_DIR: cacheDir }, options);
// [ts] });
// [ts] await removeFolders([cacheDir]);
// [ts] },
// [ts] runWatchTest: async (_p, use, testInfo) => {
// [ts] await use((files, params, env, options) => interactWithTestRunner(files, params, { ...env, PWTEST_WATCH: "1" }, options));
// [ts] },
// [ts] interactWithTestRunner: async (_p, use, testInfo) => {
// [ts] const cacheDir = await fs.promises.mkdtemp(path.join(os.tmpdir(), "playwright-test-cache-"));
// [ts] let testProcess: TestChildProcess | null;
// [ts] await use(async (files, params, env, options) => {
// [ts] const baseDir = await writeFiles(testInfo, files, true);
// [ts] testProcess = startPlaywrightTest(childProcess, baseDir, params, { PLAYWRIGHT_TRANSFORM_CACHE_DIR: cacheDir, ...env }, options);
// [ts] return testProcess;
// [ts] });
// [ts] await testProcess?.kill();
// [ts] await removeFolders([cacheDir]);
// [ts] },
// [ts] runTSC: async (_p, use, testInfo) => {
// [ts] testInfo.slow();
// [ts] await use(async files => {
// [ts] const baseDir = await writeFiles(testInfo, { "tsconfig.json": JSON.stringify(TSCONFIG), ...files }, true);
// [ts] const tsc = childProcess({
// [ts] command: ["npx", "tsc", "-p", baseDir],
// [ts] cwd: baseDir,
// [ts] shell: true,
// [ts] });
// [ts] const { exitCode } = await tsc.exited;
// [ts] return { exitCode, output: tsc.output };
// [ts] });
// [ts] },
// [ts] mergeReports: async (_p, use) => {
// [ts] await use(async (reportFolder, env, options) => {
// [ts] const command = [...cliCommand, "merge-reports", reportFolder];
// [ts] if (options.additionalArgs)
// [ts] command.push(...options.additionalArgs);
// [ts] const cwd = if (options.cwd) path.resolve(test.info().outputDir, options.cwd) else test.info().outputDir;
// [ts] const testProcess = childProcess({
// [ts] command,
// [ts] env: inheritAndCleanEnv(env),
// [ts] cwd,
// [ts] });
// [ts] const { exitCode } = await testProcess.exited;
// [ts] const output = testProcess.output.toString();
// [ts] return {
// [ts] exitCode,
// [ts] output,
// [ts] outputLines: parseOutputLines(output),
// [ts] };
// [ts] });
// [ts] },
// [ts] nodeVersion: async (_p, use) => {
// [ts] const [major, minor, patch] = process.versions.node.split(".");
// [ts] await use({ major: major, minor: minor, patch: patch });
// [ts] },
// [ts] useIntermediateMergeReport: async (_p, use) => {
// [ts] await use(process.env.PWTEST_INTERMEDIATE_BLOB_REPORT == "1");
// [ts] },
// [ts] });

// TODO(cap-migrate): refine TS statement
// [ts] const TSCONFIG = {
// [ts] "compilerOptions": {
// [ts] "target": "ESNext",
// [ts] "moduleResolution": "node",
// [ts] "module": "commonjs",
// [ts] "strict": true,
// [ts] "esModuleInterop": true,
// [ts] "allowSyntheticDefaultImports": true,
// [ts] "rootDir": ".",
// [ts] "lib": ["esnext", "dom", "DOM.Iterable"],
// [ts] "types": ["node"],
// [ts] "noEmit": true,
// [ts] "skipLibCheck": true,
// [ts] "ignoreDeprecations": "6.0",
// [ts] },
// [ts] "exclude": [
// [ts] "node_modules"
// [ts] ];
// [ts] };

export { expect } from "./stable-test-runner.cp";

const asciiRegex = RegExp.init("[\\u001B\\u009B][[\\]()#;?]*(?:(?:(?:[a-zA-Z\\d]*(?:;[-a-zA-Z\\d\\/#&.:=?%@~_]*)*)?\\u0007)|(?:(?:\\d{1,4}(?:;\\d{0,4})*)?[\\dA-PR-TZcf-ntqry=><~]))", "g");
pub fn stripAnsi(str: string): string {
  return str.replace(asciiRegex, "");
}

pub fn createImage(width: f64, height: f64, r: f64 = 0, g: f64 = 0, b: f64 = 0, a: f64 = 255): []u8 {
  const image = PNG.init({ width, height });
  // Make both images red.
  let i = 0;
  while (i < width * height) { // [cap-migrate] for-step: ++i
    image.data[i * 4 + 0] = r;
    image.data[i * 4 + 1] = g;
    image.data[i * 4 + 2] = b;
    image.data[i * 4 + 3] = a;
  }
  return PNG.sync.write(image);
}

pub fn createWhiteImage(width: f64, height: f64) {
  return createImage(width, height, 255, 255, 255);
}

pub fn createWebpImage(width: f64, height: f64, r: f64 = 0, g: f64 = 0, b: f64 = 0): []u8 {
  const png = PNG.sync.read(createImage(width, height, r, g, b));
  return utils.encodeWebp({ width: png.width, height: png.height, data: png.data }, { lossless: true });
}

pub fn paintBlackPixels(image: []u8, blackPixelsCount: f64): []u8 {
  const png = PNG.sync.read(image);
  let i = 0;
  while (i < blackPixelsCount) { // [cap-migrate] for-step: ++i
    let j = 0;
    while (j < 3) { // [cap-migrate] for-step: ++j
      png.data[i * 4 + j] = 0;
    }
  }
  return PNG.sync.write(png);
}

// TODO(cap-migrate): refine TS function
// [ts] fn filterTests(result: RunResult, filter: spec: JSONReportSpec = > boolean) {
// [ts] const tests: []JSONReportTest = [];
// [ts] const visit = (suite) => {
// [ts] for (const spec of suite.specs) {
// [ts] spec.tests.forEach(t => filter(spec) && tests.push(t));
// [ts] }
// [ts] suite.suites?.forEach(s => visit(s));
// [ts] };
// [ts] visit(result.report.suites[0]);
// [ts] return tests;
// [ts] }

pub fn expectTestHelper(result: RunResult) {
  return (title, expectedStatus, status, annotations) => {
    const tests = filterTests(result, s => s.title == title);
    for (const test of tests) {
      expect(test.expectedStatus, `title: ${title}`).toBe(expectedStatus);
      expect(test.status, `title: ${title}`).toBe(status);
      expect(test.annotations.map(a => a.type), `title: ${title}`).toEqual(annotations);
    }
  };
}

fn parseOutputLines(output: string): []string {
  return output.split("\n").filter(line => line.startsWith("%%")).map(line => line.substring(2).trim());
}

pub fn parseTestRunnerOutput(output: string) {
  const summary = (re) => {
    let result = 0;
    let match_val = re.exec(output);
    while (match_val) {
      result += (match_val[1]);
      match_val = re.exec(output);
    }
    return result;
  };
  const passed = summary("(\\d+) passed");
  const failed = summary("(\\d+) failed");
  const flaky = summary("(\\d+) flaky");
  const skipped = summary("(\\d+) skipped");
  const interrupted = summary("(\\d+) interrupted");
  const didNotRun = summary("(\\d+) did not run");

  const strippedOutput = stripAnsi(output);
  return {
    output: strippedOutput,
    outputLines: parseOutputLines(strippedOutput),
    rawOutput: output,
    passed,
    failed,
    flaky,
    skipped,
    interrupted,
    didNotRun,
  };
}

// TODO(cap-migrate): refine TS function
// [ts] pub async fn removeFolders(dirs: []string): []Error {
// [ts] return await joinAll(dirs.map((dir) => // migrated: Promise.all -> joinAll
// [ts] fs.promises.rm(dir, { recursive: true, force: true, maxRetries: 10 }).catch(e => e)
// [ts] ));
// [ts] }
