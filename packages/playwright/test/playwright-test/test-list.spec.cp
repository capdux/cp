// Migrated from TypeScript (playwright-test/test-list.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import path from "cap:path";
import fs from "cap:fs";
import { test, expect } from "./playwright-test-fixtures.cp";

const varietyWorkspace = {
  "playwright.config.ts": `;
    export default { projects: [{ name: 'p1' }, { name: 'p2' }], testDir: 'tests', };
  `,
  "dir/test.list": `;
      # this is a multiline comment

      # with an empty line in between
    [p1] > dir1/a.test.ts > test1
    [p2] › dir2${path.sep}b.spec.ts › suite › test2

    # more comments
          dir3/c.spec.ts > test2
  `,
  "dir/fileonly.list": `;
      # Run all tests in this file
      dir1/a.test.ts
  `,
  "dir/projectfile.list": `;
      [p1] › dir1/a.test.ts
  `,
  "dir/mixed.list": `;
      # All tests in this file
      dir1/a.test.ts
      # Specific test only
      dir3/c.spec.ts › test2
  `,
  "dir/suite.list": `;
      # Run all tests in the outer group
      dir4/d.spec.ts › outer
  `,
  "dir/nested-suite.list": `;
      # Run all tests in the nested group
      dir4/d.spec.ts › outer › inner
  `,
  "empty.list": `;
      # nothing to see here
  `,
  "ignore.list": `;
    [p1] > dir1/a.test.ts > test1
  `,
  "tests/dir1/a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('test1', () => { console.log('\\n%%a-test1-' + test.info().project.name); });
    test('test2', () => { console.log('\\n%%a-test2-' + test.info().project.name); });
  `,
  "tests/dir2/b.spec.ts": `;
    import { test, expect } from '@playwright/test';
    // [cap-test] test.describe('suite', () => {
      test('test1', () => { console.log('\\n%%b-test1-' + test.info().project.name); });
      // [cap-test] test.describe(() => {
        test('test2', () => { console.log('\\n%%b-test2-' + test.info().project.name); });
      // [cap-test] });
    // [cap-test] });
  `,
  "tests/dir3/c.spec.ts": `;
    import { test, expect } from '@playwright/test';
    test('test1', () => { console.log('\\n%%c-test1-' + test.info().project.name); });
    test('test2', () => { console.log('\\n%%c-test2-' + test.info().project.name); });
  `,
  "tests/dir4/d.spec.ts": `;
    import { test, expect } from '@playwright/test';
    // [cap-test] test.describe('outer', () => {
      test('test1', () => { console.log('\\n%%d-test1-' + test.info().project.name); });
      // [cap-test] test.describe('inner', () => {
        test('test2', () => { console.log('\\n%%d-test2-' + test.info().project.name); });
      // [cap-test] });
    // [cap-test] });
  `,
};










fn main() {
    test("--test-list should work", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "dir/test.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4);
      expect(result.outputLines).toEqual([
        "a-test1-p1",
        "c-test2-p1",
        "b-test2-p2",
        "c-test2-p2",
      ]);
    });
    test("--test-list-invert should work", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list-invert": "dir/test.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 12);
      expect(result.outputLines).toEqual([
        "a-test2-p1",
        "b-test1-p1",
        "b-test2-p1",
        "c-test1-p1",
        "d-test1-p1",
        "d-test2-p1",
        "a-test1-p2",
        "a-test2-p2",
        "b-test1-p2",
        "c-test1-p2",
        "d-test1-p2",
        "d-test2-p2",
      ]);
    });
    test("--test-list applies before --test-list-invert", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "dir/test.list", "test-list-invert": "ignore.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
      expect(result.outputLines).toEqual([
        "c-test2-p1",
        "b-test2-p2",
        "c-test2-p2",
      ]);
    });
    test("empty --test-list should work", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "empty.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect(result.outputLines).toEqual([
      ]);
    });
    test("--list output should work for --test-list", () => {
      const listResult = runInlineTest(varietyWorkspace, { "list": true });
      expect_eq(listResult.exitCode, 0);
      const lines = listResult.output.split("\n").filter(line => !line.includes("Listing tests") && !line.includes("Total:"));
      fs.promises.writeFile(test.info().outputPath("generated.list"), lines.join("\n"), "utf-8");

      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "generated.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 16);
      expect(result.outputLines).toEqual([
        "a-test1-p1",
        "a-test2-p1",
        "b-test1-p1",
        "b-test2-p1",
        "c-test1-p1",
        "c-test2-p1",
        "d-test1-p1",
        "d-test2-p1",
        "a-test1-p2",
        "a-test2-p2",
        "b-test1-p2",
        "b-test2-p2",
        "c-test1-p2",
        "c-test2-p2",
        "d-test1-p2",
        "d-test2-p2",
      ]);
    });
    test("--test-list with file-only entries should run all tests in file", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "dir/fileonly.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4); // 2 tests × 2 projects
      expect(result.outputLines).toEqual([
        "a-test1-p1",
        "a-test2-p1",
        "a-test1-p2",
        "a-test2-p2",
      ]);
    });
    test("--test-list with project-scoped file-only entry", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "dir/projectfile.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2); // 2 tests × 1 project
      expect(result.outputLines).toEqual([
        "a-test1-p1",
        "a-test2-p1",
      ]);
    });
    test("--test-list with mixed file-only and full test paths", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "dir/mixed.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 6); // 4 from a.test.ts + 2 from c.spec.ts test2
      expect(result.outputLines).toEqual([
        "a-test1-p1",
        "a-test2-p1",
        "c-test2-p1",
        "a-test1-p2",
        "a-test2-p2",
        "c-test2-p2",
      ]);
    });
    test("--test-list with group/suite entry should run all tests in that group", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "dir/suite.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 4); // 2 tests in outer × 2 projects
      expect(result.outputLines).toEqual([
        "d-test1-p1",
        "d-test2-p1",
        "d-test1-p2",
        "d-test2-p2",
      ]);
    });
    test("--test-list with nested group entry should run only tests in that nested group", () => {
      const result = runInlineTest(varietyWorkspace, { "workers": 1, "test-list": "dir/nested-suite.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2); // 1 test in inner × 2 projects
      expect(result.outputLines).toEqual([
        "d-test2-p1",
        "d-test2-p2",
      ]);
    });
    test("--test-list should not load files not in the list", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testDir: '.' };
        `,
        "test.list": `
          compiles.test.ts
        `,
        "compiles.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {});
        `,
        "fails.test.ts": `
          await;
        `,
      }, { "test-list": "test.list" });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
}
