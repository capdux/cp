// Migrated from TypeScript (playwright-test/reporter-preprocess.spec.ts) via cap from -ts

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












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("preprocess sees the filtered corpus, can skip tests, and records the caller location", () => {
    // preprocess runs between project setup and onBegin and sees the .only-narrowed corpus.
    // [ts] const only = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] console.log('%% plan: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] for (const t of suite.allTests())
    // [ts] if (t.title.includes('skip-me')) testRun.skip(t, 'planned skip');
    // [ts] }
    // [ts] onBegin(config, suite) {
    // [ts] console.log('%% onBegin: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] }
    // [ts] onTestEnd(test, result) {
    // [ts] const a = test.annotations.find(a => a.type === 'skip');
    // [ts] const loc = a && a.location ? require('path').basename(a.location.file) + ':' + a.location.line : 'none';
    // [ts] console.log('%% end ' + test.title + ' status=' + result.status + ' expected=' + test.expectedStatus + ' ann=' + test.annotations.map(a => a.type + ':' + (a.description || '')).join(',') + ' loc=' + loc);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts' };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('ignored-by-only', () => {});
    // [ts] test('run-me', () => {});
    // [ts] test('skip-me', () => { throw new Error('should not run'); });
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(only.exitCode, 0);
    // [ts] expect(only.outputLines).toEqual([
    // [ts] "plan: run-me,skip-me",
    // [ts] "onBegin: run-me,skip-me",
    // [ts] "end run-me status=passed expected=passed ann= loc=none",
    // The skip annotation location points at the reporter's `testRun.skip(...)` call (line 6 of reporter.ts).
    // [ts] "end skip-me status=skipped expected=skipped ann=skip:planned skip loc=reporter.ts:6",
    // [ts] ]);
    // preprocess respects --grep.
    // [ts] const grep = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite }) {
    // [ts] console.log('%% plan: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts' };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('foo-one', () => {});
    // [ts] test('bar-two', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1, grep: "foo" });
    // [ts] expect_eq(grep.exitCode, 0);
    // [ts] expect_eq(grep.outputLines, ["plan: foo-one"]);
    // preprocess respects --project.
    // [ts] const project = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite }) {
    // [ts] console.log('%% plan projects: ' + suite.suites.map(s => s.title).join(','));
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: './reporter.ts',
    // [ts] projects: [{ name: 'one' }, { name: 'two' }],
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('t', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1, project: "one" });
    // [ts] expect_eq(project.exitCode, 0);
    // [ts] expect_eq(project.outputLines, ["plan projects: one"]);
    // [ts] });
    // [ts] test("TestCase.exclude and Suite.exclude remove entries from the run and report", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] for (const t of suite.allTests())
    // [ts] if (t.title === 'excluded-test') testRun.exclude(t);
    // [ts] const visit = (s) => {
    // [ts] if (s.title === 'excluded-suite') testRun.exclude(s);
    // [ts] else for (const child of s.suites || []) visit(child);
    // [ts] };
    // [ts] visit(suite);
    // [ts] }
    // [ts] onBegin(config, suite) {
    // [ts] console.log('%% begin: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] }
    // [ts] onTestEnd(test, result) {
    // [ts] console.log('%% ran ' + test.title);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts' };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('kept', () => {});
    // [ts] test('excluded-test', () => { throw new Error('should not run'); });
    // [cap-test] test.describe('excluded-suite', () => {
    // [ts] test('doomed', () => { throw new Error('should not run'); });
    // [cap-test] });
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "begin: kept",
    // [ts] "ran kept",
    // [ts] ]);
    // [ts] });
    // [ts] test("Suite.skip cascades to all descendants", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] const visit = (s) => {
    // [ts] if (s.title === 'doomed') testRun.skip(s, 'whole group');
    // [ts] for (const child of s.suites || []) visit(child);
    // [ts] };
    // [ts] visit(suite);
    // [ts] }
    // [ts] onTestEnd(test, result) {
    // [ts] console.log('%% ' + test.title + ':' + result.status + ':' + test.expectedStatus);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts' };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [cap-test] test.describe('doomed', () => {
    // [ts] test('one', () => { throw new Error('nope'); });
    // [ts] test('two', () => { throw new Error('nope'); });
    // [cap-test] });
    // [ts] test('keep', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines.sort()).toEqual([
    // [ts] "keep:passed:passed",
    // [ts] "one:skipped:skipped",
    // [ts] "two:skipped:skipped",
    // [ts] ]);
    // [ts] });
    // [ts] test("TestRun methods throw outside preprocess, and the root suite cannot be excluded", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] this.testRun = testRun;
    // Excluding the root suite is banned even during preprocess.
    // [ts] try {
    // [ts] testRun.exclude(suite);
    // [ts] console.log('%% root-exclude: no-throw');
    // [ts] } catch (e) {
    // [ts] console.log('%% root-exclude: ' + e.message);
    // [ts] }
    // [ts] }
    // [ts] onBegin(config, suite) {
    // [ts] const testCase = suite.allTests()[0];
    // [ts] const fileSuite = testCase.parent;
    // [ts] for (const [label, obj] of [['TestCase', testCase], ['Suite', fileSuite]]) {
    // [ts] for (const method of ['skip', 'fixme', 'fail', 'exclude', ...(label === 'Suite' ? ['mode'] : [])]) {
    // [ts] try {
    // [ts] this.testRun[method](obj);
    // [ts] console.log('%% ' + label + '.' + method + ': no-throw');
    // [ts] } catch (e) {
    // [ts] console.log('%% ' + label + '.' + method + ': ' + e.message);
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts' };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('t', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "root-exclude: TestRun.exclude() cannot be called on the root suite.",
    // [ts] "TestCase.skip: TestRun.skip() can only be called from Reporter.preprocess().",
    // [ts] "TestCase.fixme: TestRun.fixme() can only be called from Reporter.preprocess().",
    // [ts] "TestCase.fail: TestRun.fail() can only be called from Reporter.preprocess().",
    // [ts] "TestCase.exclude: TestRun.exclude() can only be called from Reporter.preprocess().",
    // [ts] "Suite.skip: TestRun.skip() can only be called from Reporter.preprocess().",
    // [ts] "Suite.fixme: TestRun.fixme() can only be called from Reporter.preprocess().",
    // [ts] "Suite.fail: TestRun.fail() can only be called from Reporter.preprocess().",
    // [ts] "Suite.exclude: TestRun.exclude() can only be called from Reporter.preprocess().",
    // [ts] "Suite.mode: TestRun.mode() can only be called from Reporter.preprocess().",
    // [ts] ]);
    // [ts] });
    // [ts] test("preprocess throwing aborts the run before onBegin", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite }) {
    // [ts] throw new Error('plan-aborted');
    // [ts] }
    // [ts] onBegin(config, suite) {
    // [ts] console.log('%% onBegin: ' + suite.allTests().length);
    // [ts] }
    // [ts] onError(err) {
    // [ts] console.log('%% error: ' + err.message);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts' };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('one', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode != 0, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines, "error: Error: plan-aborted") != null, true);
    // Synthetic empty-suite onBegin is OK; the real onBegin (size 1) must NOT happen.
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines, "onBegin: 1") == null, true);
    // [ts] });
    // [ts] test("multiple reporters: preprocess called in order, annotations accumulate, exclude prunes for next reporter", () => {
    // [ts] const result = runInlineTest({
    // [ts] "first.ts": `
    // [ts] class R {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] console.log('%% first plan sees: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] for (const t of suite.allTests()) {
    // [ts] if (t.title === 'gone') testRun.exclude(t);
    // [ts] else testRun.fail(t, 'first reason');
    // [ts] }
    // [ts] }
    // [ts] onTestEnd(test, result) {
    // [ts] console.log('%% first onTestEnd: ' + test.expectedStatus + ' ann=' + test.annotations.map(a => a.type).join(','));
    // [ts] }
    // [ts] }
    // [ts] export default R;
    // [ts] `,
    // [ts] "second.ts": `
    // [ts] class R {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] console.log('%% second plan sees: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] testRun.skip(suite.allTests()[0], 'second reason');
    // [ts] }
    // [ts] }
    // [ts] export default R;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: [['./first.ts'], ['./second.ts']] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('kept', () => {});
    // [ts] test('gone', () => { throw new Error('should not run'); });
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // skip beats fail in expectedStatus, both annotations accumulate.
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "first plan sees: kept,gone",
    // [ts] "second plan sees: kept",
    // [ts] "first onTestEnd: skipped ann=fail,skip",
    // [ts] ]);
    // [ts] });
    // [ts] test("multiple reporters: a later reporter observes an earlier reporter Suite.skip on the tests", () => {
    // [ts] const result = runInlineTest({
    // [ts] "first.ts": `
    // [ts] class R {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] testRun.skip(suite.allTests()[0].parent, 'first reason');
    // [ts] }
    // [ts] }
    // [ts] export default R;
    // [ts] `,
    // [ts] "second.ts": `
    // [ts] class R {
    // [ts] async preprocess({ config, suite }) {
    // [ts] const skipped = suite.allTests().filter(t => t.expectedStatus === 'skipped').map(t => t.title);
    // [ts] console.log('%% second sees skipped: ' + skipped.join(','));
    // [ts] }
    // [ts] }
    // [ts] export default R;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: [['./first.ts'], ['./second.ts']] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [cap-test] test.describe('group', () => {
    // [ts] test('one', () => {});
    // [ts] test('two', () => {});
    // [cap-test] });
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // Suite.skip from the first reporter is applied before the second reporter runs.
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines, "second sees skipped: one,two") != null, true);
    // [ts] });
    // [ts] test("skipSharding disables the built-in shard filter; preprocess sees the full corpus", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class R {
    // [ts] async preprocess({ config, suite, testRun }) {
    // preprocess observes the full, un-sharded corpus regardless of --shard.
    // [ts] console.log('%% plan: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] testRun.skipSharding();
    // [ts] let i = 0;
    // [ts] for (const t of suite.allTests()) {
    // [ts] if (i++ % 2 === 1) testRun.exclude(t);
    // [ts] }
    // [ts] }
    // [ts] onBegin(config, suite) {
    // [ts] console.log('%% begin: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] }
    // [ts] }
    // [ts] export default R;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts', shard: { current: 1, total: 2 } };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] for (let i = 0; i < 4; i++)
    // [ts] test('t' + i, () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // preprocess sees all four tests even though --shard=1/2 was configured.
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines, "plan: t0,t1,t2,t3") != null, true);
    // The reporter's own exclusions define the shard; the built-in shard filter did NOT run
    // (it would have produced a different split), so t0,t2 remain.
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines, "begin: t0,t2") != null, true);
    // [ts] });
    // [ts] test("custom sharding may produce an empty shard", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class R {
    // [ts] async preprocess({ suite, testRun }) {
    // [ts] testRun.skipSharding();
    // [ts] for (const t of suite.allTests())
    // [ts] testRun.exclude(t);
    // [ts] }
    // [ts] }
    // [ts] export default R;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: './reporter.ts', shard: { current: 2, total: 2 } };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('t0', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "No tests found") == null, true);
    // [ts] });
    // [ts] test("multiple reporters declaring custom sharding throws", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter-a.ts": `
    // [ts] class A {
    // [ts] preprocess({ testRun }) { testRun.skipSharding(); }
    // [ts] onError(err) { console.log('%% error: ' + err.message); }
    // [ts] }
    // [ts] export default A;
    // [ts] `,
    // [ts] "reporter-b.ts": `
    // [ts] class B { preprocess({ testRun }) { testRun.skipSharding(); } }
    // [ts] export default B;
    // [ts] `,
    // [ts] "playwright.config.ts": `export default { reporter: [['./reporter-a.ts'], ['./reporter-b.ts']] };`,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('t', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode != 0, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.outputLines.join("\n"), `Multiple reporters called 'skipSharding'`) != null, true);
    // [ts] });
    // [ts] test("plan.suite exposes setup/teardown dependency projects but they are read-only", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] this.testRun = testRun;
    // [ts] console.log('%% plan projects: ' + suite.suites.map(s => s.title).join(','));
    // [ts] console.log('%% plan tests: ' + suite.allTests().map(t => t.title).join(','));
    // [ts] this.preprocessedTests = new Set(suite.allTests());
    // [ts] const setupTest = suite.allTests().find(t => t.title === 'setup-test');
    // [ts] for (const method of ['skip', 'fixme', 'fail', 'exclude']) {
    // [ts] try {
    // [ts] testRun[method](setupTest);
    // [ts] console.log('%% dep-' + method + ': no-throw');
    // [ts] } catch (e) {
    // [ts] console.log('%% dep-' + method + ': ' + e.message);
    // [ts] }
    // [ts] }
    // [ts] const setupProject = suite.suites.find(s => s.title === 'setup');
    // [ts] console.log('%% setup mode: ' + testRun.mode(setupProject));
    // [ts] console.log('%% teardown mode: ' + testRun.mode(suite.suites.find(s => s.title === 'teardown')));
    // [ts] try {
    // [ts] testRun.exclude(setupProject);
    // [ts] console.log('%% dep-suite-exclude: no-throw');
    // [ts] } catch (e) {
    // [ts] console.log('%% dep-suite-exclude: ' + e.message);
    // [ts] }
    // [ts] }
    // [ts] onBegin(config, suite) {
    // [ts] console.log('%% same test objects: ' + suite.allTests().every(test => this.preprocessedTests.has(test)));
    // [ts] const setupTest = suite.allTests().find(t => t.title === 'setup-test');
    // [ts] try {
    // [ts] this.testRun.skip(setupTest);
    // [ts] console.log('%% dep-after-preprocess: no-throw');
    // [ts] } catch (e) {
    // [ts] console.log('%% dep-after-preprocess: ' + e.message);
    // [ts] }
    // [ts] }
    // [ts] onTestEnd(test, result) {
    // [ts] console.log('%% ran ' + test.parent.project().name + '/' + test.title);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: './reporter.ts',
    // [ts] projects: [
    // [ts] { name: 'setup', testMatch: /a\\.setup\\.ts/, teardown: 'teardown' },
    // [ts] { name: 'teardown', testMatch: /a\\.teardown\\.ts/ },
    // [ts] { name: 'main', testMatch: /a\\.test\\.ts/, dependencies: ['setup'] },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "a.setup.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('setup-test', () => {});
    // [ts] `,
    // [ts] "a.teardown.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('teardown-test', () => {});
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('main-test', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 }, null, { additionalArgs: ["--project=main"] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "plan projects: teardown,setup,main",
    // [ts] "plan tests: teardown-test,setup-test,main-test",
    // [ts] "dep-skip: TestRun.skip() cannot be called on a setup or teardown project test; these always run in full.",
    // [ts] "dep-fixme: TestRun.fixme() cannot be called on a setup or teardown project test; these always run in full.",
    // [ts] "dep-fail: TestRun.fail() cannot be called on a setup or teardown project test; these always run in full.",
    // [ts] "dep-exclude: TestRun.exclude() cannot be called on a setup or teardown project test; these always run in full.",
    // [ts] "setup mode: default",
    // [ts] "teardown mode: default",
    // [ts] "dep-suite-exclude: TestRun.exclude() cannot be called on a setup or teardown project; these always run in full.",
    // [ts] "same test objects: true",
    // [ts] "dep-after-preprocess: TestRun.skip() can only be called from Reporter.preprocess().",
    // [ts] "ran setup/setup-test",
    // [ts] "ran main/main-test",
    // [ts] "ran teardown/teardown-test",
    // [ts] ]);
    // [ts] });
    // [ts] test("plan.suite temporarily exposes dependencies without changing final project selection", () => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ config, suite, testRun }) {
    // [ts] console.log('%% plan projects: ' + suite.suites.map(suite => suite.title).join(','));
    // [ts] testRun.exclude(suite.suites.find(suite => suite.title === 'main'));
    // [ts] }
    // [ts] onTestEnd(test, result) {
    // [ts] console.log('%% ran ' + test.parent.project().name + '/' + test.title);
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: './reporter.ts',
    // [ts] projects: [
    // [ts] { name: 'setup', testMatch: /setup\\.spec\\.ts/ },
    // [ts] { name: 'main', testMatch: /main\\.spec\\.ts/, dependencies: ['setup'] },
    // [ts] { name: 'keep', testMatch: /keep\\.spec\\.ts/ },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "setup.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('setup-test', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "main.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('main-test', () => {});
    // [ts] `,
    // [ts] "keep.spec.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('keep-test', () => {});
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 }, null, { additionalArgs: ["setup.spec.ts", "main.spec.ts", "keep.spec.ts"] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "plan projects: setup,main,keep",
    // [ts] "ran keep/keep-test",
    // [ts] ]);
    // [ts] });
    // [ts] test("suite modes and serial annotations are available for custom sharding", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43602" } }, (_p) => {
    // [ts] const result = runInlineTest({
    // [ts] "reporter.ts": `
    // [ts] class Reporter {
    // [ts] async preprocess({ suite, testRun }) {
    // [ts] console.log('%% root:' + testRun.mode(suite));
    // [ts] for (const project of suite.suites) {
    // [ts] console.log('%% project ' + project.title + ':' + testRun.mode(project));
    // [ts] console.log('%% file ' + project.title + ':' + testRun.mode(project.suites[0]));
    // [ts] }
    // [ts] for (const t of suite.allTests())
    // [ts] console.log('%% ' + t.parent.project().name + '/' + t.title + ':' + t.annotations.filter(a => a.type === 'serial').length + ':' + testRun.mode(t.parent));
    // [ts] }
    // [ts] }
    // [ts] export default Reporter;
    // [ts] `,
    // [ts] "playwright.config.ts": `
    // [ts] export default {
    // [ts] reporter: './reporter.ts',
    // [ts] fullyParallel: true,
    // [ts] projects: [{ name: 'parallel' }, { name: 'default', fullyParallel: false }],
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('plain', () => {});
    // [ts] test.describe.serial('s', () => {
    // [ts] test('serial', () => {});
    // [ts] });
    // [cap-test] test.describe('c', () => {
    // [ts] test.describe.configure({ mode: 'serial' });
    // [ts] test('configure', () => {});
    // [cap-test] });
    // [ts] test.describe.serial('outer', () => {
    // [cap-test] test.describe('inherited serial', () => {
    // [ts] test('inherited-serial', () => {});
    // [cap-test] });
    // [ts] test.describe.serial('inner', () => {
    // [ts] test('nested', () => {});
    // [ts] });
    // [ts] });
    // [ts] test.describe.parallel('parallel', () => {
    // [cap-test] test.describe('inherited parallel', () => {
    // [ts] test('inherited-parallel', () => {});
    // [cap-test] });
    // [cap-test] test.describe('default', () => {
    // [ts] test.describe.configure({ mode: 'default' });
    // [ts] test('default-one', () => {});
    // [cap-test] test.describe('inherited default', () => {
    // [ts] test('default-two', () => {});
    // [cap-test] });
    // [cap-test] });
    // [ts] });
    // [ts] `,
    // [ts] }, { reporter: "", workers: 1 });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect(result.outputLines).toEqual([
    // [ts] "root:default",
    // [ts] "project parallel:parallel",
    // [ts] "file parallel:parallel",
    // [ts] "project default:default",
    // [ts] "file default:default",
    // [ts] ...["parallel", "default"].flatMap(project => [
    // [ts] project + "/plain:0:" + project,
    // [ts] project + "/serial:1:serial",
    // [ts] project + "/configure:1:serial",
    // [ts] project + "/inherited-serial:1:serial",
    // [ts] project + "/nested:2:serial",
    // [ts] project + "/inherited-parallel:0:parallel",
    // [ts] project + "/default-one:0:default",
    // [ts] project + "/default-two:0:default",
    // [ts] ]),
    // [ts] ]);
    // [ts] });
}
