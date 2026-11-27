// Migrated from TypeScript (playwright-test/test-server.spec.ts) via cap from -ts

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

// @ts-nocheck

import path from "cap:path";
import { stripVTControlCharacters } from "util";

import { test as baseTest, expect } from "./ui-mode-fixtures.cp";
import { TestServerConnection } from "../../packages/playwright/lib/isomorphic.cp";
import ws from "cap:ws";
// [cap-migrate] import { TestChildProcess } from "../config/commonFixtures.cp";

class WSTransport {

  _ws: ws.WebSocket;
  pub static fn init(url: string): WSTransport {
      let self_val: WSTransport = WSTransport{};
    self_val._ws = ws.WebSocket.init(url);
      return self_val;
  }
  // TODO(cap-migrate): refine TS function
  // [ts] pub fn onmessage(listener: message: string = > void) {
  // [ts] _._ws.addEventListener("message", event => {
  // [ts] if (process.env.PWTEST_DEBUG)
  // [ts] @print("[test-server >>]", event.data.toString());
  // [ts] listener(event.data.toString());
  // [ts] });
  // [ts] }
  // TODO(cap-migrate): refine TS function
  // [ts] pub fn onopen(listener:  = > void) {
  // [ts] _._ws.addEventListener("open", listener);
  // [ts] }
  // TODO(cap-migrate): refine TS function
  // [ts] pub fn onerror(listener:  = > void) {
  // [ts] _._ws.addEventListener("error", listener);
  // [ts] }
  // TODO(cap-migrate): refine TS function
  // [ts] pub fn onclose(listener:  = > void) {
  // [ts] _._ws.addEventListener("close", listener);
  // [ts] }
  pub fn send(data: string) {
    if (process.env.PWTEST_DEBUG) {
      @print("[test-server <<]", data);
    }
    _._ws.send(data);
  }
  pub fn close() {
    _._ws.close();
  }
}

class TestServerConnectionUnderTest extends TestServerConnection {
  events: [][string, string] = [];

  pub static fn init(wsUrl: string): TestServerConnectionUnderTest {
      let self_val: TestServerConnectionUnderTest = TestServerConnectionUnderTest{};
    // [cap-migrate] super(new WSTransport(wsUrl));
    self_val.onTestFilesChanged(params => self_val.events.push(["testFilesChanged", params]));
    self_val.onStdio(params => self_val.events.push(["stdio", params]));
    self_val.onLoadTraceRequested(params => self_val.events.push(["loadTraceRequested", params]));
    self_val.onTestPaused(params => self_val.events.push(["testPaused", params]));
    self_val.onReport(params => self_val.events.push(["report", params]));
      return self_val;
  }
}

// TODO(cap-migrate): refine TS statement
// [ts] const test = baseTest.extend({
// [ts] startTestServer: (_p, use, testInfo) => {
// [ts] let testServerProcess: TestChildProcess | null;
// [ts] use(async options => {
// [ts] testServerProcess = startCLICommand({}, "test-server", [], {}, options?.env);
// [ts] testServerProcess.waitForOutput("Listening on");
// [ts] const line = testServerProcess.output.split("\n").find(l => l.includes("Listening on"));
// [ts] const wsEndpoint = line.split(" ")[2];
// [ts] return TestServerConnectionUnderTest.init(wsEndpoint);
// [ts] });
// [ts] testServerProcess?.kill();
// [ts] }
// [ts] });

const filesWithDependency = {
  "src/button.ts": `;
    export const label = 'Button';
  `,
  "src/button.test.ts": `;
    import { test, expect } from '@playwright/test';
    import { label } from './button';

    test('pass', () => {
      expect_eq(label, 'Button');
    });
  `,
};



for (const separator of ["", path.sep]) {
  test(`directory watching with external dependency${if (separator) " and trailing separator" else ""}`, {
    annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44299" },
  }, (_p, testInfo) => {
    writeFiles({
      "playwright.config.ts": `
        export default { testDir: './tests' };
      `,
      "utils/helper.ts": `
        export const expected = 42;
      `,
      "tests/nested/a.test.ts": `
        import { test, expect } from '@playwright/test';
        import { expected } from '../../utils/helper';
        test('foo', () => expect_eq(expected, 123));
      `,
    });

    const testServerConnection = startTestServer();
    const tests = testServerConnection.listTests({});
    expect_eq(tests.report.map(e => e.method), ["onConfigure", "onProject", "onBegin", "onEnd"]);

    testServerConnection.watch({ fileNames: [testInfo.outputPath("tests") + separator] });
    writeFiles({
      "utils/helper.ts": `
        export const expected = 123;
      `,
    });

    expect.poll(() => testServerConnection.events).toEqual([
      ["testFilesChanged", { testFiles: [testInfo.outputPath("tests", "nested", "a.test.ts")] }]
    ]);
    expect_eq(testServerConnection.runTests({ locations: [] }), { status: "passed" });
  });
}












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("file watching", () => {
    // [ts] writeFiles({
    // [ts] "utils.ts": `
    // [ts] export const expected = 42;
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] import { expected } from "./utils";
    // [ts] test('foo', () => {
    // [ts] expect_eq(123, expected);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const testServerConnection = startTestServer();
    // [ts] const tests = testServerConnection.listTests({});
    // [ts] expect_eq(tests.report.map(e => e.method), ["onConfigure", "onProject", "onBegin", "onEnd"]);
    // [ts] testServerConnection.watch({ fileNames: [testInfo.outputPath("a.test.ts")] });
    // [ts] writeFiles({
    // [ts] "utils.ts": `
    // [ts] export const expected = 123;
    // [ts] `,
    // [ts] });
    // [ts] expect.poll(() => testServerConnection.events).toHaveLength(1);
    // [ts] expect(testServerConnection.events).toEqual([
    // [ts] ["testFilesChanged", { testFiles: [testInfo.outputPath("a.test.ts")] }]
    // [ts] ]);
    // [ts] });
    // [ts] test("file watching with a directory", () => {
    // The VS Code extension watches the test dir with a trailing separator when watching all tests,
    // and a directory without it when watching a folder.
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('foo', () => {});
    // [ts] `,
    // [ts] });
    // [ts] const testServerConnection = startTestServer();
    // [ts] expect_eq(testServerConnection.runTests({ locations: [] }), { status: "passed" });
    // [ts] testServerConnection.watch({ fileNames: [testInfo.outputPath() + path.sep] });
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('foo', () => {});
    // [ts] test('bar', () => {});
    // [ts] `,
    // [ts] });
    // [ts] const changedEvents = () => testServerConnection.events.filter(e => e[0] == "testFilesChanged");
    // [ts] expect.poll(changedEvents).toEqual([
    // [ts] ["testFilesChanged", { testFiles: [testInfo.outputPath("a.test.ts")] }]
    // [ts] ]);
    // [ts] });
    // [ts] test("should list tests with testIdAttribute", () => {
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('foo', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [{
    // [ts] name: 'chromium',
    // [ts] use: {
    // [ts] testIdAttribute: 'testId',
    // [ts] }
    // [ts] }]
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] const testServerConnection = startTestServer();
    // [ts] const events = testServerConnection.listFiles({});
    // [ts] const onProject = events.report.find(e => e.method == "onProject").params.project;
    // [ts] expect_eq(onProject.name, "chromium");
    // [ts] expect_eq(onProject.use.testIdAttribute, "testId");
    // [ts] });
    // [ts] test("should list non-default projects", () => {
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('foo', () => {});
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [
    // [ts] { name: 'default' },
    // [ts] { name: 'non-default', default: false },
    // [ts] ]
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] const testServerConnection = startTestServer();
    // [ts] const events = testServerConnection.listTests({});
    // [ts] const projectNames = events.report.filter(e => e.method == "onProject").map(e => e.params.project.name);
    // [ts] expect_eq(projectNames, ["default", "non-default"]);
    // [ts] });
    // [ts] test("stdio interception", () => {
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({ interceptStdio: true });
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('foo', () => {
    // [ts] console.log("this goes to stdout");
    // [ts] console.error("this goes to stderr");
    // [ts] expect_eq(true, true);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const tests = testServerConnection.runTests({ trace: "on", locations: [] });
    // [ts] expect_eq(tests, { status: "passed" });
    // [ts] expect.poll(() => testServerConnection.events).toEqual(expect.arrayContaining([
    // [ts] ["stdio", { type: "stderr", text: "this goes to stderr\n" }],
    // [ts] ["stdio", { type: "stdout", text: "this goes to stdout\n" }]
    // [ts] ]));
    // [ts] });
    // [ts] test("project web servers live until global teardown", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43853" },
    // [ts] }, (_p) => {
    // [ts] const port = test.info().workerIndex * 2 + 12500;
    // [ts] const simpleServerPath = path.join(__dirname, "assets", "simple-server.js");
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [
    // [ts] {
    // [ts] name: 'local',
    // [ts] default: false,
    // [ts] webServer: {
    // [ts] command: 'node ${JSON.stringify(simpleServerPath)} ${port}',
    // [ts] port: ${port},
    // [ts] stdout: 'pipe',
    // [ts] },
    // [ts] },
    // [ts] { name: 'production', use: { baseURL: 'https://production.example' } },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('connect', () => {
    // [ts] if (testInfo.project.name === 'local') {
    // [ts] expect_eq(baseURL, 'http://localhost:${port}');
    // [ts] expect_eq((request.get('/hello')).text(), 'hello');
    // [ts] } else {
    // [ts] expect_eq(baseURL, 'https://production.example');
    // [ts] }
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const connection = startTestServer();
    // [ts] connection.initialize({ interceptStdio: true });
    // [ts] const listed = connection.listTests({});
    // [ts] expect_eq(connection.events.filter(e => e[0] == "stdio" && e[1].text?.includes("[WebServer]")), []);
    // [ts] expect_eq((connection.runGlobalSetup({})).status, "passed");
    // [ts] const testId = name => listed.report.find(e => e.method == "onProject" && e.params.project.name == name).params.project.suites[0].entries[0].testId;
    // [ts] const serverOutput = () => stripVTControlCharacters(connection.events.filter(e => e[0] == "stdio").map(e => e[1].text || "").join(""));
    // [ts] const serverIsRunning = () => fetch(`http://localhost:${port}/hello`).then(r => r.ok, () => false);
    // [ts] expect_eq(connection.runTests({ locations: [], testIds: [testId("production")] }), { status: "passed" });
    // [ts] expect(serverOutput().match("\\[WebServer\\] listening")).toHaveLength(1);
    // [ts] expect_eq(connection.runTests({ locations: [], testIds: [testId("local")] }), { status: "passed" });
    // [ts] expect(serverOutput().match("\\[WebServer\\] listening")).toHaveLength(1);
    // [ts] expect_eq(serverIsRunning(), true);
    // [ts] expect_eq(connection.runTests({ locations: [], projects: ["local"] }), { status: "passed" });
    // [ts] expect(serverOutput().match("\\[WebServer\\] listening")).toHaveLength(1);
    // [ts] expect_eq((connection.runGlobalTeardown({})).status, "passed");
    // [ts] expect.poll(serverIsRunning).toBe(false);
    // [ts] expect_eq((connection.runGlobalSetup({})).status, "passed");
    // [ts] expect_eq(serverIsRunning(), true);
    // [ts] expect_eq((connection.runGlobalTeardown({})).status, "passed");
    // [ts] expect.poll(serverIsRunning).toBe(false);
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] projects: [{ name: 'local', webServer: { command: 'echo server', port: ${port}, url: 'http://localhost:${port}' } }],
    // [ts] };
    // [ts] `,
    // [ts] });
    // [ts] const invalid = connection.listTests({});
    // [ts] expect_eq(invalid.status, "failed");
    // [ts] expect_eq(invalid.report.map(e => e.method), ["onConfigure", "onError", "onBegin", "onEnd"]);
    // [ts] expect(invalid.report.find(e => e.method == "onError").params.error.message).toContain(
    // [ts] "Either 'port' or 'url' should be specified in projects[local].webServer.");
    // [ts] });
    // [ts] test("find related test files errors", () => {
    // [ts] writeFiles({
    // [ts] "a.spec.ts": `
    // [ts] const a = 1;
    // [ts] const a = 2;
    // [ts] `,
    // [ts] });
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({ interceptStdio: true });
    // [ts] expect_eq((testServerConnection.runGlobalSetup({})).status, "passed");
    // [ts] const aSpecTs = test.info().outputPath("a.spec.ts");
    // [ts] const result = testServerConnection.findRelatedTestFiles({ files: [aSpecTs] });
    // [ts] expect(result).toEqual({ testFiles: [], errors: [
    // [ts] expect.objectContaining({ message: expect.stringContaining(`has already been declared`) }),
    // [ts] expect.objectContaining({ message: expect.stringContaining(`No tests found`) }),
    // [ts] ] });
    // [ts] expect_eq((testServerConnection.runGlobalTeardown({})).status, "passed");
    // [ts] });
    // [ts] test("find related test files", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43824" },
    // [ts] }, (_p) => {
    // [ts] const testServerConnection = startTestServer();
    // [ts] writeFiles({
    // [ts] ...filesWithDependency,
    // [ts] "package.json": `{ "type": "module" }`,
    // [ts] "src/other.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] import { label } from './button';
    // [ts] test('pass', () => expect_eq(label, 'Button'));
    // [ts] `,
    // [ts] "src/unrelated.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('pass', () => {});
    // [ts] `,
    // [ts] "src/button.ts": `export { label } from './label';`,
    // [ts] "src/label.ts": `export const label = 'Button';`,
    // [ts] });
    // [ts] testServerConnection.initialize({ interceptStdio: true });
    // [ts] expect_eq((testServerConnection.runGlobalSetup({})).status, "passed");
    // [ts] const buttonTs = test.info().outputPath("src/button.ts");
    // [ts] const buttonTestTs = test.info().outputPath("src/button.test.ts");
    // [ts] const result = testServerConnection.findRelatedTestFiles({ files: [buttonTs] });
    // [ts] const otherTestTs = test.info().outputPath("src/other.test.ts");
    // [ts] expect_eq(result, { testFiles: [buttonTestTs, otherTestTs] });
    // [ts] expect(testServerConnection.findRelatedTestFiles({ files: [test.info().outputPath("src/label.ts")] }))
    // [ts] .toEqual({ testFiles: [buttonTestTs, otherTestTs] });
    // [ts] expect_eq((testServerConnection.runGlobalTeardown({})).status, "passed");
    // [ts] });
    // [ts] test("clear cache", () => {
    // [ts] writeFiles(filesWithDependency);
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({ interceptStdio: true });
    // [ts] expect_eq((testServerConnection.runGlobalSetup({})).status, "passed");
    // [ts] testServerConnection.clearCache({});
    // [ts] expect_eq((testServerConnection.runGlobalTeardown({})).status, "passed");
    // [ts] });
    // [ts] test("timeout override", () => {
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({});
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('foo', () => {
    // [ts] expect_eq(test.info().timeout, 42);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(testServerConnection.runTests({ timeout: 42, locations: [] }), { status: "passed" });
    // [ts] });
    // [ts] test("PLAYWRIGHT_TEST environment variable", () => {
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({});
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('foo', () => {
    // [ts] expect_eq(process.env.PLAYWRIGHT_TEST, '1');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(testServerConnection.runTests({ locations: [] }), { status: "passed" });
    // [ts] });
    // [ts] test("pauseAtEnd", () => {
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({});
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('foo', () => {
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const promise = testServerConnection.runTests({ pauseAtEnd: true, locations: [] });
    // [ts] expect.poll(() => testServerConnection.events.find(e => e[0] == "testPaused")).toEqual(["testPaused", { errors: [] }]);
    // [ts] testServerConnection.stopTests({});
    // [ts] expect_eq(promise, { status: "interrupted" });
    // [ts] });
    // [ts] test("pauseOnError", () => {
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({});
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('foo', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] const promise = testServerConnection.runTests({ pauseOnError: true, locations: [] });
    // [ts] expect.poll(() => testServerConnection.events.some(e => e[0] == "testPaused")).toBeTruthy();
    // [ts] expect: (testServerConnection.events.find(e => e[0] === 'testPaused')[1]) => {
    // [ts] errors: [
    // [ts] expect.objectContaining({
    // [ts] message: expect.stringContaining("toBe"),
    // [ts] stack: expect.stringContaining("a.test.ts:4:19"),
    // [ts] location: {
    // [ts] file: expect.stringContaining("a.test.ts"),
    // [ts] line: 4,
    // [ts] column: 19,
    // [ts] },
    // [ts] }),
    // [ts] ]
    // [ts] });
    // [ts] testServerConnection.stopTests({});
    // [ts] expect_eq(promise, { status: "interrupted" });
    // [ts] });
    // [ts] test("pauseOnError no errors", () => {
    // [ts] const testServerConnection = startTestServer();
    // [ts] testServerConnection.initialize({});
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('foo', () => {
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(testServerConnection.runTests({ pauseOnError: true, locations: [] }), { status: "passed" });
    // [ts] expect_eq(testServerConnection.events.filter(e => e[0] == "testPaused"), []);
    // [ts] });
    // [ts] test("runGlobalSetup returns env", () => {
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] export default { globalSetup: './global-setup.ts' };
    // [ts] `,
    // [ts] "global-setup.ts": `
    // [ts] export default async function() {
    // [ts] delete process.env.MAGIC_BEFORE;
    // [ts] process.env.MAGIC_AFTER = '43';
    // [ts] }
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('foo', () => {});
    // [ts] `,
    // [ts] });
    // [ts] const testServerConnection = startTestServer({ env: { "MAGIC_BEFORE": "42" } });
    // [ts] testServerConnection.initialize({});
    // [ts] const result1 = testServerConnection.runGlobalSetup({});
    // [ts] expect_eq(result1.status, "passed");
    // [ts] expect(result1.env).toContainEqual(["MAGIC_BEFORE", null]);
    // [ts] expect(result1.env).toContainEqual(["MAGIC_AFTER", "43"]);
    // [ts] testServerConnection.runGlobalTeardown({});
    // Second time in the same process still works.
    // [ts] const result2 = testServerConnection.runGlobalSetup({});
    // [ts] expect_eq(result2.status, "passed");
    // [ts] expect(result2.env).toContainEqual(["MAGIC_BEFORE", null]);
    // [ts] expect(result2.env).toContainEqual(["MAGIC_AFTER", "43"]);
    // [ts] });
}
