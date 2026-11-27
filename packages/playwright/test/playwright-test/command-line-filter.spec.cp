// Migrated from TypeScript (playwright-test/command-line-filter.spec.ts) via cap from -ts

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
    test("should filter by file name", () => {
      const result = runInlineTest({
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
        "b.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
      }, null, null, { additionalArgs: ["a.spec.ts"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "1) a.spec.ts") != null, true);
    });
    test("should filter by folder", () => {
      const result = runInlineTest({
        "foo/x.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
        "foo/y.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
        "bar/x.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
        "bar/y.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
      }, null, null, { additionalArgs: ["bar"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 2);
      expect(result.output).toMatch("bar[\\\\/]x.spec.ts");
      expect(result.output).toMatch("bar[\\\\/]y.spec.ts");
    });
    test("should filter by line", () => {
      const result = runInlineTest({
        "foo/x.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => { expect_eq(1, 2); });
          test('two', () => { expect_eq(1, 2); });
          test('three', () => { expect_eq(1, 2); });
        `,
        "foo/y.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
      }, null, null, { additionalArgs: ["x.spec.ts:4"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.output).toMatch("x\\.spec\\.ts.*two");
    });
    test("should filter by line and column", () => {
      const result = runInlineTest({
        "foo/x.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('yes-full-match', () => { expect_eq(1, 1); });
              test('no-wrong-column', () => { expect_eq(1, 2); });
      test('yes-no-column-specified', () => { expect_eq(1, 1); });
      test('no-match', () => { expect_eq(1, 1); });
          test('yes-full-match-with-dirname', () => { expect_eq(1, 1); });
          `,
      }, null, null, { additionalArgs: ["x.spec.ts:3:7", "x.spec.ts:4:99999", "x.spec.ts:5", "foo/x.spec.ts:7:7"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.skipped, 0);
      expect_eq(result.passed, 3);
      expect_eq(result.report.suites[0].specs.map(spec => spec.title), ["yes-full-match", "yes-no-column-specified", "yes-full-match-with-dirname"]);
    });
    test("line should override focused test", () => {
      const result = runInlineTest({
        "foo/x.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => { expect_eq(1, 2); });
          test('two', () => { expect_eq(1, 2); });
          test('three', () => { expect_eq(1, 2); });
        `,
        "foo/y.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
      }, null, null, { additionalArgs: ["x.spec.ts:4"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect(result.output).toMatch("x\\.spec\\.ts.*two");
    });
    test("should merge filtered line and filtered file", () => {
      const result = runInlineTest({
        "foo/x.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => { expect_eq(1, 2); });
          test('two', () => { expect_eq(1, 2); });
          test('three', () => { expect_eq(1, 2); });
        `,
        "foo/y.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
      }, null, null, { additionalArgs: ["x.spec.ts:4", "x.spec.ts"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 3);
    });
    test("should run nothing for missing line", () => {
      const result = runInlineTest({
        "foo/x.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => { expect_eq(1, 2); });
          test('two', () => { expect_eq(1, 2); });
          test('three', () => { expect_eq(1, 2); });
        `,
        "foo/y.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('fails', () => { expect_eq(1, 2); });
        `,
      }, null, null, { additionalArgs: ["x.spec.ts:8", "y.spec.ts"] });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
    });
    test("should focus a single nested test spec", () => {
      const result = runInlineTest({
        "foo.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass1', () => {});
          // [cap-test] test.describe('suite-1', () => {
            // [cap-test] test.describe('suite-2', () => {
              test('pass2', () => {});
            // [cap-test] });
          // [cap-test] });
          test('pass3', () => {});
        `,
        "bar.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass3', () => {});
        `,
        "noooo.test.ts": `
          import { test, expect } from '@playwright/test';
          test('no-pass1', () => {});
        `,
      }, {}, {}, { additionalArgs: ["foo.test.ts:6", "bar.test.ts"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(result.skipped, 0);
      expect_eq(result.report.suites[0].specs[0].title, "pass3");
      expect_eq(result.report.suites[1].suites[0].suites[0].specs[0].title, "pass2");
    });
    test("should accept unix file path containing a space", () => {
      const result = runInlineTest({
        "dir with space/a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => { expect_eq(1, 1); });
        `,
        "dir with space/b.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('not picked', () => { expect_eq(1, 2); });
        `,
      }, null, null, { additionalArgs: ["dir with space/a.spec.ts"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.report.suites).toHaveLength(1);
      expect_eq(result.report.suites[0].specs[0].title, "passes");
    });
    test("should focus a single test suite", () => {
      const result = runInlineTest({
        "foo.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass1', () => {});
          // [cap-test] test.describe('suite-1', () => {
            // [cap-test] test.describe('suite-2', () => {
              test('pass2', () => {});
              test('pass3', () => {});
            // [cap-test] });
          // [cap-test] });
          test('pass4', () => {});
        `,
        "bar.test.ts": `
          import { test, expect } from '@playwright/test';
          test('no-pass1', () => {});
        `,
      }, {}, {}, { additionalArgs: ["foo.test.ts:5"] });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(result.skipped, 0);
      expect_eq(result.report.suites[0].suites[0].suites[0].specs[0].title, "pass2");
      expect_eq(result.report.suites[0].suites[0].suites[0].specs[1].title, "pass3");
    });
}
