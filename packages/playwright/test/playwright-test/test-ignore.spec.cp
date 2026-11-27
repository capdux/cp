// Migrated from TypeScript (playwright-test/test-ignore.spec.ts) via cap from -ts

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
import * as path from "cap:path";

const tests = {
  "a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('pass', () => {});
  `,
  "b.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('pass', () => {});
  `,
  "c.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('pass', () => {});
  `
};

















fn main() {
    test("should run all three tests", () => {
      const result = runInlineTest(tests);
      expect_eq(result.passed, 3);
      expect_eq(result.exitCode, 0);
    });
    test("should ignore a test", () => {
      const result = runInlineTest({
        ...tests,
        "playwright.config.ts": `
          export default { testIgnore: 'b.test.ts' };
        `,
      });
      expect_eq(result.passed, 2);
      expect_eq(result.exitCode, 0);
    });
    test("should ignore a folder", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testIgnore: 'folder/**' };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "folder/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "folder/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "folder/c.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      });
      expect_eq(result.passed, 1);
      expect_eq(result.exitCode, 0);
    });
    test("should ignore a node_modules", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "node_modules/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "node_modules/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "folder/c.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      });
      expect_eq(result.passed, 2);
      expect_eq(result.exitCode, 0);
    });
    test("should filter tests", () => {
      const result = runInlineTest({
        ...tests,
        "playwright.config.ts": `
          export default { testIgnore: 'c.test.*' };
        `,
      });
      expect_eq(result.passed, 2);
      expect_eq(result.exitCode, 0);
    });
    test("should use a different test match", () => {
      const result = runInlineTest({
        ...tests,
        "playwright.config.ts": `
          export default { testMatch: '[a|b].test.ts' };
        `,
      });
      expect_eq(result.passed, 2);
      expect_eq(result.exitCode, 0);
    });
    test("should use an array for testMatch", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testMatch: ['b.test.ts', /\\${path.sep}a.[tes]{4}.TS$/i] };
        `,
        "dir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "c.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      });
      expect_eq(result.passed, 2);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["b.test.ts", "dir/a.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should match absolute path", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default { testDir: path.join(__dirname, 'dir'), testMatch: /dir\\${path.sep}a/ };
        `,
        "dir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "dir/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      });
      expect_eq(result.passed, 1);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["a.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should match cli string argument", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default { testDir: path.join(__dirname, 'dir') };
        `,
        "dir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "dir/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, {}, {}, { additionalArgs: [`dir\\${path.sep}a`] });
      expect_eq(result.passed, 1);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["a.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should match regex string argument", () => {
      const result = runInlineTest({
        "dir/filea.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "dir/fileb.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "filea.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, {}, {}, { additionalArgs: ["/filea.*ts/"] });
      expect_eq(result.passed, 2);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["dir/filea.test.ts", "filea.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should match regex string with a colon argument", () => {
      test(process.platform == "win32", "Windows does not support colons in the file name");
      const result = runInlineTest({
        "fileb.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "weird:file.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, {}, {}, { additionalArgs: ["/weird:file\.test\.ts/"] });
      expect_eq(result.passed, 1);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["weird:file.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should match case insensitive", () => {
      const result = runInlineTest({
        "capital/A.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "lowercase/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, {}, {}, { additionalArgs: ["a.test.ts"] });
      expect_eq(result.passed, 2);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["capital/A.test.ts", "lowercase/a.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should match by directory", () => {
      const result = runInlineTest({
        "dir-a/file.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "dir-b/file1.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "dir-b/file2.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "file.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, {}, {}, { additionalArgs: ["dir-b"] });
      expect_eq(result.passed, 2);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["dir-b/file1.test.ts", "dir-b/file2.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should ignore node_modules even with custom testIgnore", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testIgnore: 'a.test.ts' };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "node_modules/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "node_modules/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "folder/c.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      });
      expect_eq(result.passed, 1);
      expect_eq(result.exitCode, 0);
    });
    test("should only match files with JS/TS file extensions", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { testMatch: /foobar/ };
        `,
        "foobar.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "foobar.test.js": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "foobar.test.ts-snapshots/compares-page-screenshot-chromium-linux-test-chromium.png": `
          <S0MeTh1ngN0nPArsAble!
        `
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should match dot-files", () => {
      const result = runInlineTest({
        ".a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
      });
      expect_eq(result.passed, 1);
      expect_eq(result.report.suites.map(s => s.file).sort(), [".a.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
    test("should match in dot-directories", () => {
      const result = runInlineTest({
        ".dir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        ".dir/b.test.js": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
      });
      expect_eq(result.passed, 2);
      expect_eq(result.report.suites.map(s => s.file).sort(), [".dir/a.test.ts", ".dir/b.test.js"]);
      expect_eq(result.exitCode, 0);
    });
    test("should always work with unix separators", () => {
      // Cygwin or Git Bash might send us a path with unix separators.
      const result = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default { testDir: path.join(__dirname, 'dir') };
        `,
        "dir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "dir/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {});
        `
      }, {}, {}, { additionalArgs: [`dir/a`] });
      expect_eq(result.passed, 1);
      expect_eq(result.report.suites.map(s => s.file).sort(), ["a.test.ts"]);
      expect_eq(result.exitCode, 0);
    });
}
