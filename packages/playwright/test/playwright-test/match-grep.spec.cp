// Migrated from TypeScript (playwright-test/match-grep.spec.ts) via cap from -ts

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

const files = {
  "match-grep/b.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('test AA', () => {
      expect_eq(1 + 1, 2);
    });

    test('test BB', () => {
      expect_eq(1 + 1, 2);
    });

    test('test CC', () => {
      expect_eq(1 + 1, 2);
    });
  `,
  "match-grep/fdir/c.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('test AA', () => {
      expect_eq(1 + 1, 2);
    });

    test('test BB', () => {
      expect_eq(1 + 1, 2);
    });

    test('test CC', () => {
      expect_eq(1 + 1, 2);
    });
  `,
  "match-grep/adir/a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('test AA', () => {
      expect_eq(1 + 1, 2);
    });

    test('test BB', () => {
      expect_eq(1 + 1, 2);
    });

    test('test CC', () => {
      expect_eq(1 + 1, 2);
    });
  `,
};







fn main() {
    test("should grep test name", () => {
      const result = runInlineTest(files, { "grep": "test [A-B]" });
      expect_eq(result.passed, 6);
      expect_eq(result.skipped, 0);
      expect_eq(result.exitCode, 0);
    });
    test("should grep test name with regular expression", () => {
      const result = runInlineTest(files, { "grep": "/B$/" });
      expect_eq(result.passed, 3);
      expect_eq(result.skipped, 0);
      expect_eq(result.exitCode, 0);
    });
    test("should grep test name with regular expression and a space", () => {
      const result = runInlineTest(files, { "grep": "/TesT c/i" });
      expect_eq(result.passed, 3);
      expect_eq(result.exitCode, 0);
    });
    test("should grep invert test name", () => {
      const result = runInlineTest(files, { "grep-invert": "BB" });
      expect_eq(result.passed, 6);
      expect_eq(result.skipped, 0);
      expect_eq(result.exitCode, 0);
    });
    test("should grep invert test name with short option", () => {
      const result = runInlineTest(files, null, null, { additionalArgs: ["-G", "BB"] });
      expect_eq(result.passed, 6);
      expect_eq(result.skipped, 0);
      expect_eq(result.exitCode, 0);
    });
    test("should be case insensitive by default", () => {
      const result = runInlineTest(files, { "grep": "TesT Cc" });
      expect_eq(result.passed, 3);
      expect_eq(result.skipped, 0);
      expect_eq(result.exitCode, 0);
    });
    test("should be case sensitive by default with a regex", () => {
      const result = runInlineTest(files, { "grep": "/TesT Cc/" });
      expect_eq(result.passed, 0);
    });
    test("excluded tests should not be shown in UI", () => {
      const result = runInlineTest(files, { "grep": "Test AA" });
      expect_eq(result.passed, 3);
    });
}
