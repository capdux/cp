// Migrated from TypeScript (playwright-test/playwright.trace.spec.ts) via cap from -ts

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

import { test, expect } from "./playwright-test-fixtures.cp";
import { parseTrace, parseTraceRaw } from "../config/utils.cp";

import fs from "cap:fs";
import path from "cap:path";

test.describe.configure({ mode: "parallel" });













for (const mode of ["off", "retain-on-failure", "on-first-retry", "on-all-retries", "retain-on-first-failure", "retain-on-failure-and-retries"]) {
  test(`trace:${mode} should not create trace zip artifact if page test passed`, () => {
    const result = runInlineTest({
      "a.spec.ts": `
        import { test as base, expect } from '@playwright/test';
        import fs from 'fs';
        let artifactsDir;
        const test = base.extend({
          workerAuto: [({}, use) => {
            use();
            const entries =  fs.readdirSync(artifactsDir);
            expect_eq(entries.filter(e => e.endsWith('.zip')), []);
          }, { scope: 'worker', auto: true }],
        });
        test('passing test', () => {
          artifactsDir = test.info()._tracing.artifactsDir();
          page.goto('about:blank');
        });
      `,
    }, { trace: "retain-on-failure" });
    expect_eq(result.exitCode, 0);
    expect_eq(result.passed, 1);
  });

  test(`trace:${mode} should not create trace zip artifact if APIRequestContext test passed`, () => {
    const result = runInlineTest({
      "a.spec.ts": `
        import { test as base, expect } from '@playwright/test';
        import fs from 'fs';
        let artifactsDir;
        const test = base.extend({
          workerAuto: [({}, use) => {
            use();
            const entries =  fs.readdirSync(artifactsDir);
            expect_eq(entries.filter(e => e.endsWith('.zip')), []);
          }, { scope: 'worker', auto: true }],
        });
        test('passing test', () => {
          artifactsDir = test.info()._tracing.artifactsDir();
          expect(request.get('${server.EMPTY_PAGE}')).toBeOK();
        });
      `,
    }, { trace: "retain-on-failure" });
    expect_eq(result.exitCode, 0);
    expect_eq(result.passed, 1);
  });
}

































fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should stop tracing with trace: on-first-retry, when not retrying", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on-first-retry' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [cap-test] test.describe('shared', () => {
    // [ts] let page;
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] page = browser.newPage();
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] page.close();
    // [ts] });
    // [ts] test('flaky', () => {
    // [ts] expect_eq(testInfo.retry, 1);
    // [ts] });
    // [ts] test('no tracing', () => {
    // [ts] const e = page.context().tracing.stop({ path: 'ignored' }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, e.message, 'Must start tracing before stopping') != null, true);
    // [ts] });
    // [cap-test] });
    // [ts] `,
    // [ts] }, { workers: 1, retries: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.flaky, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-shared-flaky-retry1", "trace.zip")), true);
    // [ts] });
    // [ts] test("should record api trace", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44027" },
    // [ts] }, (_p, testInfo) => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on', viewport: { width: 900, height: 600 } } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.goto('about:blank');
    // [ts] request.get('${server.EMPTY_PAGE}');
    // [ts] });
    // [ts] test('api pass', () => {
    // [ts] const request = playwright.request.newContext();
    // [ts] request.get('${server.EMPTY_PAGE}');
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] page.goto('about:blank');
    // [ts] request.get('${server.EMPTY_PAGE}');
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 2);
    // [ts] expect_eq(result.failed, 1);
    // One trace file for request context and one for each APIRequestContext
    // [ts] const trace1 = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect_eq(trace1.model.browserName, "chromium");
    // [ts] expect_eq(trace1.model.options.viewport, { width: 900, height: 600 });
    // [ts] expect(trace1.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"request\"",
    // [ts] "    Create request context",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "Navigate about:blank",
    // [ts] `GET ${server.HOST}/empty.html`,
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] "  Fixture \"request\"",
    // [ts] ]);
    // [ts] const trace2 = parseTrace(testInfo.outputPath("test-results", "a-api-pass", "trace.zip"));
    // [ts] expect_eq(trace2.model.browserName, "");
    // [ts] expect_eq(trace2.model.options, {});
    // [ts] expect(trace2.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "Create request context",
    // [ts] `GET ${server.HOST}/empty.html`,
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] const trace3 = parseTrace(testInfo.outputPath("test-results", "a-fail", "trace.zip"));
    // [ts] expect(trace3.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"request\"",
    // [ts] "    Create request context",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "Navigate about:blank",
    // [ts] `GET ${server.HOST}/empty.html`,
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] "  Fixture \"request\"",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"browser\"",
    // [ts] ]);
    // [ts] });
    // [ts] test("should not throw with trace: on-first-retry and two retries in the same worker", () => {
    // [ts] const files = {};
    // [ts] let i = 0;
    // [ts] while (i < 6) { // [cap-migrate] for-step: i++
    // [ts] files[`a${i}.spec.ts`] = `
    // [ts] import { test, expect } from './helper';
    // [ts] test('flaky', () => {
    // [ts] new Promise(f => setTimeout(f, 200 + Math.round(Math.random() * 1000)));
    // [ts] expect_eq(testInfo.retry, 1);
    // [ts] });
    // [ts] test('passing', () => {
    // [ts] new Promise(f => setTimeout(f, 200 + Math.round(Math.random() * 1000)));
    // [ts] });
    // [ts] `;
    // [ts] }
    // [ts] const result = runInlineTest({
    // [ts] ...files,
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on-first-retry' } };
    // [ts] `,
    // [ts] "helper.ts": `
    // [ts] import { test as base } from '@playwright/test';
    // [ts] export * from '@playwright/test';
    // [ts] export const test = base.extend({
    // [ts] myContext: [({ browser }, use) => {
    // [ts] const c = browser.newContext();
    // [ts] use(c);
    // [ts] c.close();
    // [ts] }, { scope: 'worker' }]
    // [ts] })
    // [ts] `,
    // [ts] }, { workers: 3, retries: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 6);
    // [ts] expect_eq(result.flaky, 6);
    // [ts] });
    // [ts] test("should not mixup network files between contexts", () => {
    // NOTE: this test reproduces the issue 10% of the time. Running with --repeat-each=20 helps.
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22089" });
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] let page1, page2;
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] page1 = browser.newPage();
    // [ts] page1.goto("${server.EMPTY_PAGE}");
    // [ts] page1.request.get("${server.PREFIX}/simple.json?context=1");
    // [ts] page2 = browser.newPage();
    // [ts] page2.goto("${server.EMPTY_PAGE}");
    // [ts] page2.request.get("${server.PREFIX}/simple.json?context=2");
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] page1.close();
    // [ts] page2.close();
    // [ts] });
    // [ts] test('example', () => {
    // [ts] page.goto("${server.EMPTY_PAGE}");
    // [ts] page.request.get("${server.PREFIX}/simple.json?context=3");
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, timeout: 15000 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tracePath = testInfo.outputPath("test-results", "a-example", "trace.zip");
    // [ts] const { resources } = parseTraceRaw(tracePath);
    // [ts] const traceEntries = [...resources].filter((_p) => name.endsWith(".trace")).map((_p) => ({
    // [ts] prefix: name.slice(0, -".trace".length),
    // [ts] contextOptions: JSON.parse(content.toString().split("\n")[0]),
    // [ts] })).filter(entry => entry.contextOptions.origin == "library");
    // Each of the 3 browser contexts and 3 api request contexts produces
    // a trace chunk per test phase it was recording during.
    // [ts] const browserTraces = traceEntries.filter(entry => entry.contextOptions.browserName);
    // [ts] const apiTraces = traceEntries.filter(entry => !entry.contextOptions.browserName);
    // [ts] expect_eq(browserTraces.length >= 3, true);
    // [ts] expect_eq(apiTraces.length >= 3, true);
    // [ts] for (const entry of browserTraces) {
    // [ts] const network = resources.get(entry.prefix + ".network").toString();
    // [ts] expect_eq(std.mem.indexOf(u8, network, "?context=") == null, true);
    // [ts] }
    // [ts] const apiURLs = [
    // [ts] server.PREFIX + "/simple.json?context=1",
    // [ts] server.PREFIX + "/simple.json?context=2",
    // [ts] server.PREFIX + "/simple.json?context=3",
    // [ts] ];
    // Api request context network files are chunk-specific, so each of the requests
    // must show up in exactly one network file.
    // [ts] const apiURLsByTrace = apiTraces.map(entry => {
    // [ts] const network = resources.get(entry.prefix + ".network").toString();
    // [ts] return apiURLs.filter(url => network.includes(url));
    // [ts] });
    // [ts] expect_eq(apiURLsByTrace.every(urls => urls.length <= 1), true);
    // [ts] expect_eq(apiURLsByTrace.flat().sort(), apiURLs);
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(trace.model.resources.filter(resource => resource._apiRequestRef).map(resource => resource.request.url).sort(), apiURLs);
    // [ts] });
    // [ts] test("should save sources when requested", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] use: {
    // [ts] trace: 'on',
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.evaluate(2 + 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const { resources } = parseTraceRaw(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect([...resources.keys()].filter(name => name.startsWith("src/"))).toHaveLength(1);
    // [ts] });
    // [ts] test("should expand snapshots object in trace option", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] use: {
    // [ts] trace: { mode: 'on', snapshots: { dom: true, aria: true, screen: true } },
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<button>Click me</button>');
    // [ts] page.locator('button').click();
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const { model } = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] const click = model.actions.find(a => a.method == "click");
    // [ts] expect_eq(model.hasDomSnapshotForCall(click.callId, "after"), true);
    // [ts] expect_eq(model.screenshotForCall(click.callId, "after"), true);
    // [ts] expect_eq(model.ariaSnapshotForCall(click.callId, "after"), true);
    // [ts] });
    // [ts] test("should keep trace config options when forcing mode with --trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] use: {
    // [ts] trace: { mode: 'on-first-retry', snapshots: { dom: true, aria: true, screen: true } },
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<button>Click me</button>');
    // [ts] page.locator('button').click();
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1, trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // The 'on' mode is forced, while the snapshots configuration is preserved.
    // [ts] const { model } = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] const click = model.actions.find(a => a.method == "click");
    // [ts] expect_eq(model.hasDomSnapshotForCall(click.callId, "after"), true);
    // [ts] expect_eq(model.screenshotForCall(click.callId, "after"), true);
    // [ts] expect_eq(model.ariaSnapshotForCall(click.callId, "after"), true);
    // [ts] });
    // [ts] test("should not save sources when not requested", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] use: {
    // [ts] trace: {
    // [ts] mode: 'on',
    // [ts] sources: false,
    // [ts] }
    // [ts] }
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.evaluate(2 + 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const { resources } = parseTraceRaw(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect([...resources.keys()].filter(name => name.startsWith("src/"))).toHaveLength(0);
    // [ts] });
    // [ts] test("should work in serial mode", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'retain-on-failure' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.describe.serial('serial', () => {
    // [ts] let page;
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] page = browser.newPage();
    // [ts] });
    // [ts] test.afterAll(() => {
    // [ts] page.close();
    // [ts] });
    // [ts] test('passes', () => {
    // [ts] });
    // [ts] test('fails', () => {
    // [ts] throw new Error('oh my');
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-serial-passes", "trace.zip")), false);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-serial-fails", "trace.zip")), true);
    // [ts] });
    // [ts] test("should not override trace file in afterAll", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'retain-on-failure' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] page.goto('about:blank');
    // [ts] throw 'oh no!';
    // [ts] });
    // Another test in the same file to affect after hooks order.
    // [ts] test('test 2', () => {
    // [ts] });
    // [ts] test.afterAll(({ request }) => {
    // [ts] request.get('${server.EMPTY_PAGE}');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const trace1 = parseTrace(testInfo.outputPath("test-results", "a-test-1", "trace.zip"));
    // [ts] expect(trace1.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "Navigate about:blank",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] "  afterAll hook",
    // [ts] "    Fixture \"request\"",
    // [ts] "      Create request context",
    // [ts] `    GET ${server.HOST}/empty.html`,
    // [ts] "    Fixture \"request\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"browser\"",
    // [ts] ]);
    // [ts] expect_eq(trace1.model.errors.map(e => e.message), [`'oh no!'`]);
    // [ts] const error = parseTrace(testInfo.outputPath("test-results", "a-test-2", "trace.zip")).catch(e => e);
    // [ts] expect_eq(error, true);
    // [ts] });
    // [ts] test("should retain traces for interrupted tests", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'retain-on-failure' }, maxFailures: 1 };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] page.waitForTimeout(2000);
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 2', () => {
    // [ts] page.goto('about:blank');
    // [ts] page.waitForTimeout(5000);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 2 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(result.interrupted, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-test-1", "trace.zip")), true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "b-test-2", "trace.zip")), true);
    // [ts] });
    // [ts] test("should respect --trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] page.goto('about:blank');
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-test-1", "trace.zip")), true);
    // [ts] });
    // [ts] test(`trace:retain-on-failure should create trace if context is closed before failure in the test`, () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'retain-on-failure' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passing test', () => {
    // [ts] page.goto('about:blank');
    // [ts] context.close();
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-failure" });
    // [ts] const tracePath = test.info().outputPath("test-results", "a-passing-test", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), "Navigate about:blank") != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] });
    // [ts] test(`trace:retain-on-failure should create trace if context is closed before failure in afterEach`, () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'retain-on-failure' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passing test', () => {
    // [ts] });
    // [ts] test.afterEach(({ page, context }) => {
    // [ts] page.goto('about:blank');
    // [ts] context.close();
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-failure" });
    // [ts] const tracePath = test.info().outputPath("test-results", "a-passing-test", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), "    Navigate about:blank") != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] });
    // [ts] test(`trace:retain-on-failure should create trace if request context is disposed before failure`, () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'retain-on-failure' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passing test', () => {
    // [ts] expect(request.get('${server.EMPTY_PAGE}')).toBeOK();
    // [ts] request.dispose();
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-failure" });
    // [ts] const tracePath = test.info().outputPath("test-results", "a-passing-test", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), `GET ${server.HOST}/empty.html`) != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] });
    // [ts] test("should include attachments by default", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] testInfo.attach('foo', { body: 'bar' });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tracePath = testInfo.outputPath("test-results", "a-pass", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "Attach \"foo\"",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] expect: (trace.model.actions[1].attachments) => {
    // [ts] name: "foo",
    // [ts] contentType: "text/plain",
    // [ts] file: expect.any(String),
    // [ts] }]);
    // [ts] const { resources } = parseTraceRaw(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, [...resources.keys()], trace.model.actions[1].attachments[0].file) != null, true);
    // [ts] });
    // [ts] test("should opt out of attachments", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: { mode: 'on', attachments: false } } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] testInfo.attach('foo', { body: 'bar' });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tracePath = testInfo.outputPath("test-results", "a-pass", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] `Attach "foo"`,
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] expect_eq(trace.model.actions[1].attachments, null);
    // [ts] const { resources } = parseTraceRaw(tracePath);
    // [ts] expect([...resources.keys()].filter(f => f.startsWith("attachments/") || f.startsWith("resources/"))).toHaveLength(0);
    // [ts] });
    // [ts] test("should record with custom page fixture", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] myPage: ({ browser }, use) => {
    // [ts] use(browser.newPage());
    // [ts] },
    // [ts] });
    // [ts] test.use({ trace: 'on' });
    // [ts] test('fails', () => {
    // [ts] myPage.setContent('hello');
    // [ts] throw new Error('failure!');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "failure!") != null, true);
    // [ts] const trace = parseTraceRaw(testInfo.outputPath("test-results", "a-fails", "trace.zip"));
    // [ts] expect: (trace.events) => {
    // [ts] type: "frame-snapshot",
    // [ts] }));
    // [ts] });
    // [ts] test("should expand expect.toPass", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: { mode: 'on' } } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] let i = 0;
    // [ts] expect(() => {
    // [ts] page.goto('data:text/html,Hello world');
    // [ts] expect_eq(i++, 2);
    // [ts] }).toPass();
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "Expect \"toPass\"",
    // [ts] "  Navigate data:",
    // [ts] "  Expect \"toBe\"",
    // [ts] "  Navigate data:",
    // [ts] "  Expect \"toBe\"",
    // [ts] "  Navigate data:",
    // [ts] "  Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] ]);
    // [ts] });
    // [ts] test("should show non-expect error in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: { mode: 'on' } } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] expect_eq(1, 1);
    // [ts] undefinedVariable1 = 'this throws an exception';
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-fail", "trace.zip"));
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"browser\"",
    // [ts] ]);
    // [ts] expect_eq(trace.model.errors.map(e => e.message), ["ReferenceError: undefinedVariable1 is not defined"]);
    // [ts] });
    // [ts] test("should show error from beforeAll in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: { mode: 'on' } } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.beforeAll(() => {
    // [ts] throw new Error('Oh my!');
    // [ts] })
    // [ts] test('fail', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-fail", "trace.zip"));
    // [ts] expect_eq(trace.model.errors.map(e => e.message), ["Error: Oh my!"]);
    // [ts] });
    // [ts] test("should throw when trace fixture is a function", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.use({
    // [ts] trace: ({}, use) => {
    // [ts] use('on');
    // [ts] },
    // [ts] });
    // [ts] test('skipped', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: \"trace\" option cannot be a function") != null, true);
    // [ts] });
    // [ts] test("should not throw when attachment is missing", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {
    // [ts] test.info().attachments.push({ name: 'screenshot', path: 'nothing-to-see-here', contentType: 'image/png' });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-passes", "trace.zip"));
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), "Attach \"screenshot\"") != null, true);
    // [ts] });
    // [ts] test("should not throw when screenshot on failure fails", () => {
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment; filename=file.txt");
    // [ts] res.end(`Hello world`);
    // [ts] });
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on', screenshot: 'on' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('has download page', () => {
    // [ts] page.goto("${server.EMPTY_PAGE}");
    // [ts] page.setContent('<a href="/download" target="blank">open me!</a>');
    // [ts] const downloadPromise = page.waitForEvent('download');
    // [ts] page.click('a');
    // [ts] const download = downloadPromise;
    // [ts] expect_eq(download.suggestedFilename(), 'file.txt');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-has-download-page", "trace.zip"));
    // [ts] const attachedScreenshots = trace.model.actions.filter(a => a.attachments).flatMap(a => a.attachments);
    // One screenshot for the page, no screenshot for the download page since it should have failed.
    // [ts] expect_eq(attachedScreenshots.length, 1);
    // [ts] });
    // [ts] test("should use custom expect message in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: { mode: 'on' } } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] expect(
    // [ts] page.getByRole('button', { name: 'Find a hotel' }),
    // [ts] 'expect to have text: find a hotel'
    // [ts] ).toHaveCount(0);
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-fail", "trace.zip"));
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] `expect to have text: find a hotel getByRole('button', { name: 'Find a hotel' })`,
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] ]);
    // [ts] });
    // [ts] test("should not throw when merging traces multiple times", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/27286" });
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { BrowserContext, expect, Page, test as baseTest } from '@playwright/test';
    // [ts] let pg: Page;
    // [ts] let ctx: BrowserContext;
    // [ts] const test = baseTest.extend({
    // [ts] page: ({}, use) => {
    // [ts] use(pg);
    // [ts] },
    // [ts] context: ({}, use) => {
    // [ts] use(ctx);
    // [ts] },
    // [ts] });
    // [ts] test.beforeAll(({ browser }) => {
    // [ts] ctx = browser.newContext();
    // [ts] pg = ctx.newPage();
    // [ts] });
    // [ts] test.beforeAll(({ page }) => {
    // [ts] page.goto(${JSON.stringify(server.PREFIX + '/frames/nested-frames.html')});
    // [ts] });
    // [ts] test.afterAll(({ context }) => {
    // [ts] context.close();
    // [ts] });
    // [ts] test('foo', () => {
    // [ts] expect(page.frameLocator('iframe').nth(1).locator('div')).toHaveText("Hi, I'm frame");
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("test-results", "a-foo", "trace.zip")), true);
    // [ts] });
    // [ts] test("should record nested steps, even after timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] use: { trace: { mode: 'on' } },
    // [ts] timeout: 5000,
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fooPage: ({ page }, use) => {
    // [ts] expect_eq(1, 'fooPage setup', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] page.setContent('hello');
    // [ts] test.step('step in fooPage setup', () => {
    // [ts] page.setContent('bar');
    // [ts] });
    // [ts] use(page);
    // [ts] expect_eq(1, 'fooPage teardown', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] page.setContent('hi');
    // [ts] test.step('step in fooPage teardown', () => {
    // [ts] page.setContent('bar');
    // [ts] });
    // [ts] },
    // [ts] barPage: ({ browser }, use) => {
    // [ts] expect_eq(1, 'barPage setup', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] const page = browser.newPage();
    // [ts] test.step('step in barPage setup', () => {
    // [ts] page.setContent('bar');
    // [ts] });
    // [ts] use(page);
    // [ts] expect_eq(1, 'barPage teardown', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] test.step('step in barPage teardown', () => {
    // [ts] page.close();
    // [ts] });
    // [ts] },
    // [ts] });
    // [ts] test.beforeAll(({ barPage }) => {
    // [ts] expect_eq(1, 'beforeAll start', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] barPage.setContent('hello');
    // [ts] test.step('step in beforeAll', () => {
    // [ts] barPage.setContent('bar');
    // [ts] });
    // [ts] });
    // [ts] test.beforeEach(({ fooPage }) => {
    // [ts] expect_eq(1, 'beforeEach start', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] fooPage.setContent('hello');
    // [ts] test.step('step in beforeEach', () => {
    // [ts] fooPage.setContent('hi');
    // Next line times out. We make sure that after hooks steps
    // form the expected step tree even when some previous steps have not finished.
    // [ts] new Promise(() => {});
    // [ts] });
    // [ts] });
    // [ts] test('example', () => {
    // [ts] });
    // [ts] test.afterEach(({ fooPage }) => {
    // [ts] expect_eq(1, 'afterEach start', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] fooPage.setContent('hello');
    // [ts] test.step('step in afterEach', () => {
    // [ts] fooPage.setContent('bar');
    // [ts] });
    // [ts] });
    // [ts] test.afterAll(({ barPage }) => {
    // [ts] expect_eq(1, 'afterAll start', 1);
    // [ts] new Promise(f => setTimeout(f, 1));  // To avoid same-wall-time sorting issues.
    // [ts] barPage.setContent('hello');
    // [ts] test.step('step in afterAll', () => {
    // [ts] barPage.setContent('bar');
    // [ts] });
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-example", "trace.zip"));
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  beforeAll hook",
    // [ts] "    Fixture \"browser\"",
    // [ts] "      Launch browser",
    // [ts] "    Fixture \"barPage\"",
    // [ts] "      barPage setup",
    // [ts] "      Create page",
    // [ts] "      step in barPage setup",
    // [ts] "        Set content",
    // [ts] "    beforeAll start",
    // [ts] "    Set content",
    // [ts] "    step in beforeAll",
    // [ts] "      Set content",
    // [ts] "    Fixture \"barPage\"",
    // [ts] "      barPage teardown",
    // [ts] "      step in barPage teardown",
    // [ts] "        Close context",
    // [ts] "  beforeEach hook",
    // [ts] "    Fixture \"context\"",
    // [ts] "      Create context",
    // [ts] "    Fixture \"page\"",
    // [ts] "      Create page",
    // [ts] "    Fixture \"fooPage\"",
    // [ts] "      fooPage setup",
    // [ts] "      Set content",
    // [ts] "      step in fooPage setup",
    // [ts] "        Set content",
    // [ts] "    beforeEach start",
    // [ts] "    Set content",
    // [ts] "    step in beforeEach",
    // [ts] "      Set content",
    // [ts] "After Hooks",
    // [ts] "  afterEach hook",
    // [ts] "    afterEach start",
    // [ts] "    Set content",
    // [ts] "    step in afterEach",
    // [ts] "      Set content",
    // [ts] "  Fixture \"fooPage\"",
    // [ts] "    fooPage teardown",
    // [ts] "    Set content",
    // [ts] "    step in fooPage teardown",
    // [ts] "      Set content",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] "  afterAll hook",
    // [ts] "    Fixture \"barPage\"",
    // [ts] "      barPage setup",
    // [ts] "      Create page",
    // [ts] "      step in barPage setup",
    // [ts] "        Set content",
    // [ts] "    afterAll start",
    // [ts] "    Set content",
    // [ts] "    step in afterAll",
    // [ts] "      Set content",
    // [ts] "    Fixture \"barPage\"",
    // [ts] "      barPage teardown",
    // [ts] "      step in barPage teardown",
    // [ts] "        Close context",
    // [ts] "Attach \"error-context\"",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"browser\"",
    // [ts] ]);
    // [ts] });
    // [ts] test("should not produce an action entry for calling a binding", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { use: { trace: 'on' } };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {
    // [ts] let wasCalled = false;
    // [ts] page.exposeBinding('customBinding', () => {
    // [ts] wasCalled = true;
    // [ts] return 'foo';
    // [ts] });
    // [ts] const output = page.evaluate(() => window['customBinding']());
    // [ts] expect_eq(wasCalled, true);
    // [ts] expect_eq(output, 'foo');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-passes", "trace.zip"));
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "Evaluate",
    // [ts] "Expect \"toBe\"",
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] ]);
    // [ts] });
    // [ts] test("should attribute worker fixture teardown to the right test", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] use: { trace: { mode: 'on' } },
    // [ts] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] foo: [({}, use) => {
    // [ts] expect_eq(1, 'step in foo setup', 1);
    // [ts] use('foo');
    // [ts] expect_eq(1, 'step in foo teardown', 1);
    // [ts] }, { scope: 'worker' }],
    // [ts] });
    // [ts] test('one', () => {
    // [ts] });
    // [ts] test('two', () => {
    // [ts] throw new Error('failure');
    // [ts] });
    // [ts] `,
    // [ts] }, { workers: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const trace1 = parseTrace(testInfo.outputPath("test-results", "a-one", "trace.zip"));
    // [ts] expect(trace1.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"foo\"",
    // [ts] "    step in foo setup",
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] const trace2 = parseTrace(testInfo.outputPath("test-results", "a-two", "trace.zip"));
    // [ts] expect(trace2.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "After Hooks",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"foo\"",
    // [ts] "    step in foo teardown",
    // [ts] ]);
    // [ts] });
    // [ts] test("trace:retain-on-first-failure should create trace but only on first failure", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.goto('about:blank');
    // [ts] expect_eq(true, false);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-first-failure", retries: 1 });
    // [ts] const retryTracePath = test.info().outputPath("test-results", "a-fail-retry1", "trace.zip");
    // [ts] const retryTraceExists = fs.existsSync(retryTracePath);
    // [ts] expect_eq(retryTraceExists, false);
    // [ts] const tracePath = test.info().outputPath("test-results", "a-fail", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), "Navigate about:blank") != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] });
    // [ts] test("trace:retain-on-first-failure should create trace if context is closed before failure in the test", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.goto('about:blank');
    // [ts] context.close();
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-first-failure" });
    // [ts] const tracePath = test.info().outputPath("test-results", "a-fail", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), "Navigate about:blank") != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] });
    // [ts] test("trace:retain-on-first-failure should create trace if context is closed before failure in afterEach", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] });
    // [ts] test.afterEach(({ page, context }) => {
    // [ts] page.goto('about:blank');
    // [ts] context.close();
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-first-failure" });
    // [ts] const tracePath = test.info().outputPath("test-results", "a-fail", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), "    Navigate about:blank") != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] });
    // [ts] test("trace:retain-on-first-failure should create trace if request context is disposed before failure", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] expect(request.get('${server.EMPTY_PAGE}')).toBeOK();
    // [ts] request.dispose();
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-first-failure" });
    // [ts] const tracePath = test.info().outputPath("test-results", "a-fail", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect_eq(std.mem.indexOf(u8, trace.model.renderActionTree(), `GET ${server.HOST}/empty.html`) != null, true);
    // [ts] expect_eq(result.failed, 1);
    // [ts] });
    // [ts] test("trace:retain-on-failure-and-retries should keep all traces when test is flaky", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('flaky', () => {
    // [ts] page.goto('about:blank');
    // [ts] expect_eq(test.info().retry, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-failure-and-retries", retries: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.flaky, 1);
    // [ts] const firstRunTracePath = testInfo.outputPath("test-results", "a-flaky", "trace.zip");
    // [ts] expect_eq(fs.existsSync(firstRunTracePath), true);
    // [ts] const retryTracePath = testInfo.outputPath("test-results", "a-flaky-retry1", "trace.zip");
    // [ts] expect_eq(fs.existsSync(retryTracePath), true);
    // [ts] });
    // [ts] test("trace:retain-on-failure-and-retries should keep all traces when test fails on retries", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.goto('about:blank');
    // [ts] expect_eq(true, false);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "retain-on-failure-and-retries", retries: 1 });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const firstRunTracePath = testInfo.outputPath("test-results", "a-fail", "trace.zip");
    // [ts] expect_eq(fs.existsSync(firstRunTracePath), true);
    // [ts] const retryTracePath = testInfo.outputPath("test-results", "a-fail-retry1", "trace.zip");
    // [ts] expect_eq(fs.existsSync(retryTracePath), true);
    // [ts] });
    // [ts] test("should not corrupt actions when no library trace is present", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] foo: ({}, use) => {
    // [ts] expect_eq(1, 1);
    // [ts] use();
    // [ts] expect_eq(2, 2);
    // [ts] },
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const tracePath = test.info().outputPath("test-results", "a-fail", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"foo\"",
    // [ts] "    Expect \"toBe\"",
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"foo\"",
    // [ts] "    Expect \"toBe\"",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] ]);
    // [ts] });
    // [ts] test("should record trace for manually created context in a failed test", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/31541" });
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] const page = browser.newPage();
    // [ts] page.setContent('<script>console.log("from the page");</script>');
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const tracePath = test.info().outputPath("test-results", "a-fail", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "Create page",
    // [ts] "Set content",
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"browser\"",
    // [ts] ]);
    // Check console events to make sure that library trace is recorded.
    // [ts] expect(trace.model.events).toContainEqual(expect.objectContaining({ type: "console", text: "from the page" }));
    // [ts] });
    // [ts] test("should not nest top level expect into unfinished api calls ", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/31959" }
    // [ts] }, (_p) => {
    // [ts] server.setRoute("/index", (req, res) => {
    // [ts] res.writeHead(200, { "Content-Type": "text/html" });
    // [ts] res.end(`<script>fetch('/api')</script><div>Hello!</div>`);
    // [ts] });
    // [ts] server.setRoute("/hang", () => {});
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.route('**/api', async route => {
    // [ts] const response = route.fetch({ url: '${server.PREFIX}/hang' });
    // [ts] route.fulfill({ response });
    // [ts] });
    // [ts] page.goto('${server.PREFIX}/index');
    // [ts] expect(page.getByText('Hello!')).toBeVisible();
    // [ts] page.unrouteAll({ behavior: 'ignoreErrors' });
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.failed, 0);
    // [ts] const tracePath = test.info().outputPath("test-results", "a-pass", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] `Navigate ${server.HOST}/index`,
    // [ts] `GET ${server.HOST}/hang`,
    // [ts] `Expect "toBeVisible" getByText('Hello!')`,
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] ]);
    // [ts] });
    // [ts] test("should record trace after fixture teardown timeout", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/30718" },
    // [ts] }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend({
    // [ts] fixture: ({}, use) => {
    // [ts] use('foo');
    // [ts] new Promise(() => {});
    // [ts] },
    // [ts] });
    // Note: it is important that "fixture" is last, so that it runs the teardown first.
    // [ts] test('fails', () => {
    // [ts] page.evaluate(() => console.log('from the page'));
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on", timeout: "3000" }, { DEBUG: "pw:test" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] const tracePath = test.info().outputPath("test-results", "a-fails", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "  Fixture \"fixture\"",
    // [ts] "Evaluate",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"fixture\"",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"browser\"",
    // [ts] ]);
    // Check console events to make sure that library trace is recorded.
    // [ts] expect(trace.model.events).toContainEqual(expect.objectContaining({ type: "console", text: "from the page" }));
    // [ts] });
    // [ts] test("should record trace snapshot for more obscure commands", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test 1', () => {
    // [ts] const page = browser.newPage();
    // [ts] page.setContent('<div>Content</div>');
    // [ts] expect_eq(page.locator('div').count(), 1);
    // [ts] page.locator('div').boundingBox();
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const tracePath = test.info().outputPath("test-results", "a-test-1", "trace.zip");
    // [ts] const trace = parseTrace(tracePath);
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"browser\"",
    // [ts] "    Launch browser",
    // [ts] "Create page",
    // [ts] "Set content",
    // [ts] `Query count locator('div')`,
    // [ts] "Expect \"toBe\"",
    // [ts] `Bounding box locator('div')`,
    // [ts] "After Hooks",
    // [ts] ]);
    // [ts] const countAction = trace.model.actions.find(a => a.method == "queryCount");
    // [ts] expect_eq(trace.snapshots.snapshotForCall(countAction.callId, "before"), true);
    // [ts] expect_eq(trace.snapshots.snapshotForCall(countAction.callId, "after"), true);
    // [ts] const boundingBoxAction = trace.model.actions.find(a => a.title == "Bounding box");
    // [ts] expect_eq(trace.snapshots.snapshotForCall(boundingBoxAction.callId, "before"), true);
    // [ts] expect_eq(trace.snapshots.snapshotForCall(boundingBoxAction.callId, "after"), true);
    // [ts] });
    // [ts] test("should record default test timeout in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect_eq(trace.model.testTimeout, 30_000);
    // [ts] });
    // [ts] test("should record custom test timeout in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] test.setTimeout(120_000);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect_eq(trace.model.testTimeout, 120_000);
    // [ts] });
    // [ts] test("should record test annotations in trace", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42035" });
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', {
    // [ts] annotation: { type: 'note1', description: 'static annotation' },
    // [ts] }, ({}) => {
    // [ts] test.info().annotations.push({ type: 'note2', description: 'dynamic annotation' });
    // [ts] test.info().annotations.push({ type: 'note3' });
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect(trace.model.annotations).toEqual([
    // [ts] { type: "note1", description: "static annotation" },
    // [ts] { type: "note2", description: "dynamic annotation" },
    // [ts] { type: "note3" },
    // [ts] ]);
    // [ts] });
    // [ts] test("should record step params in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(1, 1);
    // [ts] test.step('my step', () => {}, { params: { foo: 'bar' } });
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] const actionByTitle = (title) => trace.model.actions.find(a => a.title == title);
    // [ts] expect_eq(actionByTitle("my step").params, { foo: "bar" });
    // [ts] expect_eq(actionByTitle("Expect \"toBe\"").params, { expected: "1" });
    // [ts] });
    // [ts] test("should record step stack in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] async function helper(page) {
    // [ts] test.step('my step', () => {
    // [ts] page.setContent('<title>hello</title>');
    // [ts] expect(page).toHaveTitle('hello');
    // [ts] });
    // [ts] }
    // [ts] test('pass', () => {
    // [ts] helper(page);
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] const stacks = trace.model.actions.filter(a => a.stack?.length).map(a => a.stack.map(f => `${path.basename(f.file)}:${f.line}:${f.column}`));
    // [ts] expect(stacks).toEqual([
    // test.step
    // [ts] ["a.spec.ts:4:9", "a.spec.ts:10:9"],
    // page.setContent
    // [ts] ["a.spec.ts:5:22", "a.spec.ts:4:9", "a.spec.ts:10:9"],
    // expect
    // [ts] ["a.spec.ts:6:30", "a.spec.ts:4:9", "a.spec.ts:10:9"],
    // [ts] ]);
    // [ts] });
    // [ts] test("should record step subtitle in trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] test.step('my step', () => {}, { subtitle: 'my subtitle' });
    // [ts] });
    // [ts] `,
    // [ts] }, { trace: "on" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const trace = parseTrace(testInfo.outputPath("test-results", "a-pass", "trace.zip"));
    // [ts] expect_eq(trace.model.actions.find(a => a.title == "my step").subtitle, "my subtitle");
    // [ts] });
}
