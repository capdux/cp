// Migrated from TypeScript (playwright-test/reporter-perfetto.spec.ts) via cap from -ts

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

import * as crypto from "cap:crypto";
import * as fs from "cap:fs";
import * as path from "cap:path";
import * as zlib from "cap:zlib";
import { test, expect } from "./playwright-test-fixtures.cp";

type TraceEvent = string; // TODO(cap-migrate): complex TS conditional/mapped type

fn readTrace(baseDir: string, fileName: string = "test-results/perfetto.json") {
  const file = path.join(baseDir, fileName);
  const content = if (fileName.endsWith(".gz")) zlib.gunzipSync(fs.readFileSync(file)).toString("utf8") else fs.readFileSync(file, "utf8");
  return JSON.parse(content);
}

fn slices(events: []TraceEvent) {
  return events.filter(e => e.ph == "X");
}

fn threadNames(events: []TraceEvent) {
  return events.filter(e => e.ph == "M" && e.name == "thread_name").map(e => e.args.name);
}

fn findSlice(events: []TraceEvent, name: string) {
  return slices(events).find(e => e.name == name);
}

const testFiles = {
  "a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test.beforeAll(() => {});
    test.beforeEach(() => {});
    // [cap-test] test.describe('suite', () => {
      test('passing @smoke', { annotation: { type: 'issue', description: 'flaky' } }, ({}) => {
        console.log('hello from the test');
        test.step('outer', () => {
          test.step('inner', () => {
            expect_eq(1, 1);
          });
        });
      });
    // [cap-test] });
    test('failing', () => {
      expect_eq(1, 2);
    });
  `,
};






// TODO(cap-migrate): refine TS statement
// [ts] for (const fileName of ["trace.json", "trace.json.gz"]) {
// [ts] test(`should fail when ${fileName} output is a directory`, () => {
// [ts] const outputFile = testInfo.outputPath(fileName);
// [ts] fs.promises.mkdir(outputFile);
// [ts] const result = runInlineTest({
// [ts] "playwright.config.ts": `
// [ts] export default { reporter: [['perfetto', { outputFile: ${JSON.stringify(outputFile)} }]] };
// [ts] `,
// [ts] "a.test.ts": `
// [ts] import { test } from '@playwright/test';
// [ts] test('one', () => {
// [ts] test.info().annotations.push({ type: 'blob', description: ${JSON.stringify(crypto.randomBytes(256 * 1024).toString('base64'))} });
// [ts] });
// [ts] `,
// [ts] });
// [ts] expect_eq(result.exitCode != 0, true);
// [ts] });
// [ts] }




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should write a perfetto report", () => {
    // [ts] const result = runInlineTest(testFiles, { reporter: "perfetto" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const report = readTrace(testInfo.outputPath());
    // [ts] expect_eq(report.displayTimeUnit, "ms");
    // [ts] expect_eq(report.metadata.status, "failed");
    // [ts] const events = report.traceEvents;
    // [ts] expect_eq(events.filter(e => e.ph == "M" && e.name == "process_name")[0].args.name, "Playwright Test");
    // [ts] expect_eq(threadNames(events), ["Worker 0"]);
    // [ts] const passing = findSlice(events, "passing @smoke");
    // [ts] expect_eq(passing.cat, "test");
    // [ts] expect_eq(passing.cname, "good");
    // [ts] expect_eq(passing.dur > 0, true);
    // [ts] expect: (passing.args) => {
    // [ts] status: "passed",
    // [ts] expectedStatus: "passed",
    // [ts] title: "suite Ã¢ÂÂº passing @smoke",
    // [ts] workerIndex: 0,
    // [ts] parallelIndex: 0,
    // [ts] timeout: 30000,
    // [ts] tags: "@smoke",
    // [ts] annotations: ["issue: flaky"],
    // [ts] stdout: "hello from the test\n",
    // [ts] }));
    // [ts] expect_eq(std.mem.indexOf(u8, passing.args.location, "a.test.ts:") != null, true);
    // [ts] const failing = findSlice(events, "failing");
    // [ts] expect_eq(failing.cname, "bad");
    // [ts] expect_eq(failing.args.status, "failed");
    // [ts] expect_eq(std.mem.indexOf(u8, failing.args.errors[0], "expect(received).toBe(expected)") != null, true);
    // Hooks, fixtures and steps are all rendered as slices.
    // [ts] expect_eq(findSlice(events, "Before Hooks").cat, "hook");
    // [ts] expect_eq(findSlice(events, "beforeAll hook").cat, "hook");
    // [ts] expect_eq(findSlice(events, "beforeEach hook").cat, "hook");
    // [ts] expect_eq(findSlice(events, "After Hooks").cat, "hook");
    // [ts] expect_eq(findSlice(events, "Expect \"toBe\"").cat, "expect");
    // [ts] const outer = findSlice(events, "outer");
    // [ts] expect_eq(outer.cat, "test.step");
    // [ts] expect_eq(std.mem.indexOf(u8, outer.args.location, "a.test.ts:") != null, true);
    // [ts] });
    // [ts] test("should nest steps within the test slice", () => {
    // [ts] const result = runInlineTest(testFiles, { reporter: "perfetto" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const events = slices(readTrace(testInfo.outputPath()).traceEvents);
    // Complete events on the same thread must form a proper stack.
    // [ts] const byThread = Map.init();
    // [ts] for (const event of events) {
    // [ts] let list = byThread.get(event.tid);
    // [ts] if (!list) {
    // [ts] list = [];
    // [ts] byThread.set(event.tid, list);
    // [ts] }
    // [ts] list.push(event);
    // [ts] }
    // [ts] for (const list of byThread.values()) {
    // [ts] list.sort((a, b) => a.ts - b.ts || b.dur - a.dur);
    // [ts] const stack: []TraceEvent = [];
    // [ts] for (const event of list) {
    // [ts] while (stack.length && stack[stack.length - 1].ts + stack[stack.length - 1].dur <= event.ts)
    // [ts] stack.pop();
    // [ts] if (stack.length) {
    // [ts] const parent = stack[stack.length - 1];
    // [ts] expect_eq(event.ts + event.dur, `${event.name} inside ${parent.name}` <= parent.ts + parent.dur, true);
    // [ts] }
    // [ts] stack.push(event);
    // [ts] }
    // [ts] }
    // [ts] const outer = findSlice(events, "outer");
    // [ts] const inner = findSlice(events, "inner");
    // [ts] expect_eq(inner.ts >= outer.ts, true);
    // [ts] expect_eq(inner.ts + inner.dur <= outer.ts + outer.dur, true);
    // [ts] });
    // [ts] test("should use a lane per worker", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => { new Promise(f => setTimeout(f, 500)); });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('two', () => { new Promise(f => setTimeout(f, 500)); });
    // [ts] `,
    // [ts] }, { reporter: "perfetto", workers: 2 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const events = readTrace(testInfo.outputPath()).traceEvents;
    // [ts] expect_eq(threadNames(events), ["Worker 0", "Worker 1"]);
    // [ts] expect_eq(findSlice(events, "one").tid != findSlice(events, "two").tid, true);
    // [ts] });
    // [ts] test("should report attachment files", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import fs from 'fs';
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {
    // [ts] const file = test.info().outputPath('note.txt');
    // [ts] fs.writeFileSync(file, 'hello');
    // [ts] test.info().attach('inline', { body: 'body' });
    // [ts] test.info().attach('file', { path: file });
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "perfetto" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const one = findSlice(readTrace(testInfo.outputPath()).traceEvents, "one");
    // [ts] expect_eq(one.args.attachments, ["inline", expect.stringMatching("^test-results\\/a-one\\/attachments\\/file-.*\\.txt$")]);
    // [ts] });
    // [ts] test("should report step params", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {
    // [ts] page.goto('about:blank');
    // [ts] page.setContent('<button>Click me</button>');
    // [ts] page.getByRole('button').click();
    // [ts] expect(page.getByRole('button')).toBeVisible();
    // [ts] test.step('my step', () => {}, { params: { foo: 'bar', count: 7 } });
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "perfetto" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const events = slices(readTrace(testInfo.outputPath()).traceEvents);
    // [ts] expect_eq(findSlice(events, "Navigate about:blank").args.params, { url: "about:blank" });
    // [ts] expect_eq(findSlice(events, "Set content").args.params, null);
    // [ts] expect_eq(findSlice(events, `Click getByRole('button')`).args.params, { locator: `getByRole('button')` });
    // [ts] expect_eq(findSlice(events, `Expect "toBeVisible" getByRole('button')`).args.params, { locator: `getByRole('button')` });
    // [ts] expect_eq(findSlice(events, "my step").args.params, { foo: "bar", count: 7 });
    // [ts] });
    // [ts] test("should respect outputFile option", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: [['perfetto', { outputFile: 'reports/my-trace.json' }]] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(findSlice(readTrace(testInfo.outputPath(), "reports/my-trace.json").traceEvents, "one"), true);
    // [ts] });
    // [ts] test("should gzip the report when output file ends with .gz", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.ts": `
    // [ts] export default { reporter: [['perfetto', { outputFile: 'perfetto.json.gz' }]] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {});
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const gzipped = fs.readFileSync(testInfo.outputPath("perfetto.json.gz"));
    // [ts] expect_eq(gzipped.subarray(0, 2), Buffer.from([0x1f, 0x8b]));
    // [ts] expect_eq(findSlice(readTrace(testInfo.outputPath(), "perfetto.json.gz").traceEvents, "one"), true);
    // [ts] });
    // [ts] test("should respect PLAYWRIGHT_PERFETTO_OUTPUT_FILE", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {});
    // [ts] `,
    // [ts] }, { reporter: "perfetto" }, { PLAYWRIGHT_PERFETTO_OUTPUT_FILE: testInfo.outputPath("env-trace.json") });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(findSlice(readTrace(testInfo.outputPath(), "env-trace.json").traceEvents, "one"), true);
    // [ts] });
    // [ts] test("should report retries as separate slices", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('flaky', () => {
    // [ts] expect_eq(testInfo.retry, 1);
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "perfetto", retries: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const flaky = slices(readTrace(testInfo.outputPath()).traceEvents).filter(e => e.name == "flaky");
    // [ts] expect(flaky).toHaveLength(2);
    // [ts] expect_eq(flaky.map(e => e.args.status), ["failed", "passed"]);
    // [ts] expect_eq(flaky[1].args.retry, 1);
    // [ts] });
    // [ts] test("should report global errors as instant events", () => {
    // [ts] const result = runInlineTest({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('one', () => {});
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] throw new Error('Oh my!');
    // [ts] `,
    // [ts] }, { reporter: "perfetto" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const errors = readTrace(testInfo.outputPath()).traceEvents.filter(e => e.ph == "i");
    // [ts] expect(errors).toHaveLength(1);
    // [ts] expect_eq(std.mem.indexOf(u8, errors[0].args.error, "Oh my!") != null, true);
    // [ts] });
}
