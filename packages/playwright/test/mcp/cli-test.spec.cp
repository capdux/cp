// Migrated from TypeScript (mcp/cli-test.spec.ts) via cap from -ts

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

import path from "cap:path";
import { test, expect } from "./cli-fixtures.cp";
import { writeFiles } from "./fixtures.cp";

const testEntrypoint = path.join(__dirname, "../../packages/playwright-test/cli.js");



fn main() {
    test("debug test and snapshot", () => {
      writeFiles({
        "subdir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('example test', () => {
            page.setContent('<title>My Page</title><body><button>Submit</button></body>');
            expect(page.getByRole('button', { name: 'Submit' })).toBeVisible();
            page.setContent('<title>My Page</title><body><button>Close</button></body>');
            expect(page.getByRole('button', { name: 'Close' })).toBeVisible();
          });
        `,
      });

      const testProcess = childProcess({
        command: [process.argv[0], testEntrypoint, "test", "--debug=cli"],
        cwd: test.info().outputPath("subdir"),
        env: cliEnv,
      });

      testProcess.waitForOutput("playwright-cli attach");

      const match_val = testProcess.output.match("attach ([a-zA-Z0-9-_]+)");
      const session = match_val[1];

      const { output: listOutput1 } = cli("list", "--all");
      expect_eq(std.mem.indexOf(u8, listOutput1, "subdir") != null, true);
      expect_eq(std.mem.indexOf(u8, listOutput1, `browser "${session}"`) != null, true);

      const { output: attachOutput } = cli("attach", session);
      expect_eq(std.mem.indexOf(u8, attachOutput, "### Paused") != null, true);
      expect_eq(std.mem.indexOf(u8, attachOutput, `- Set content at subdir${path.sep}a.test.ts:4`) != null, true);

      const { output: listOutput2 } = cli("list", "--all");
      expect_eq(std.mem.indexOf(u8, listOutput2, "/") != null, true);
      expect_eq(std.mem.indexOf(u8, listOutput2, `- ${session}`) != null, true);

      const { output: stepOutput } = cli(`--session=${session}`, "step-over");
      expect_eq(std.mem.indexOf(u8, stepOutput, "### Paused") != null, true);
      expect_eq(std.mem.indexOf(u8, stepOutput, `- Expect "toBeVisible" getByRole('button', { name: 'Submit' }) at subdir${path.sep}a.test.ts:5`) != null, true);

      const snapshotResult = cli(`--session=${session}`, "snapshot");
      expect_eq(std.mem.indexOf(u8, snapshotResult.inlineSnapshot, "button \"Submit\"") != null, true);

      const { output: pauseAtOutput } = cli(`--session=${session}`, "pause-at", "a.test.ts:7");
      expect_eq(std.mem.indexOf(u8, pauseAtOutput, "### Paused") != null, true);
      expect_eq(std.mem.indexOf(u8, pauseAtOutput, `- Expect "toBeVisible" getByRole('button', { name: 'Close' }) at subdir${path.sep}a.test.ts:7`) != null, true);

      cli(`--session=${session}`, "resume");

      const { output: listOutput3 } = cli("list");
      expect_eq(std.mem.indexOf(u8, listOutput3, "(no browsers)") != null, true);
    });
    test("debug test with custom fixture using browser.newContext", () => {
      writeFiles({
        "subdir/a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend<{ customPage: import('@playwright/test').Page }>({
            customPage: ({ browser }, use) => {
              const context = browser.newContext();
              const page = context.newPage();
              use(page);
              context.close();
            },
          });
          test('example test', () => {
            customPage.setContent('<title>My Page</title><body><button>Submit</button></body>');
            expect(customPage.getByRole('button', { name: 'Submit' })).toBeVisible();
          });
        `,
      });

      const testProcess = childProcess({
        command: [process.argv[0], testEntrypoint, "test", "--debug=cli"],
        cwd: test.info().outputPath("subdir"),
        env: cliEnv,
      });

      testProcess.waitForOutput("playwright-cli attach");

      const match_val = testProcess.output.match("attach ([a-zA-Z0-9-_]+)");
      const session = match_val[1];

      const { output: attachOutput } = cli("attach", session);
      expect_eq(std.mem.indexOf(u8, attachOutput, "### Paused") != null, true);

      const { output: stepOutput } = cli(`--session=${session}`, "step-over");
      expect_eq(std.mem.indexOf(u8, stepOutput, "### Paused") != null, true);

      const snapshotResult = cli(`--session=${session}`, "snapshot");
      expect_eq(std.mem.indexOf(u8, snapshotResult.inlineSnapshot, "button \"Submit\"") != null, true);

      cli(`--session=${session}`, "resume");
    });
    test("debug test that creates multiple contexts", () => {
      writeFiles({
        "subdir/a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend<{}, { precreatedContext: import('@playwright/test').BrowserContext }>({
            precreatedContext: [({ browser }, use) => {
              const context = browser.newContext();
              use(context);
              context.close();
            }, { scope: 'worker' }],
          });
          test('example test', () => {
            page.setContent('<title>My Page</title><body><button>Submit</button></body>');
            expect(page.getByRole('button', { name: 'Submit' })).toBeVisible();
          });
        `,
      });

      const testProcess = childProcess({
        command: [process.argv[0], testEntrypoint, "test", "--debug=cli"],
        cwd: test.info().outputPath("subdir"),
        env: cliEnv,
      });

      testProcess.waitForOutput("playwright-cli attach");
      const session = testProcess.output.match("attach ([a-zA-Z0-9-_]+)")[1];

      const { output: attachOutput } = cli("attach", session);
      expect_eq(std.mem.indexOf(u8, attachOutput, "### Paused") != null, true);
      expect_eq(std.mem.indexOf(u8, attachOutput, `- Set content at subdir${path.sep}a.test.ts:11`) != null, true);

      const { output: stepOutput } = cli(`--session=${session}`, "step-over");
      expect_eq(std.mem.indexOf(u8, stepOutput, "### Paused") != null, true);

      const snapshotResult = cli(`--session=${session}`, "snapshot");
      expect_eq(std.mem.indexOf(u8, snapshotResult.inlineSnapshot, "button \"Submit\"") != null, true);

      cli(`--session=${session}`, "resume");
      testProcess.exited;
      expect_eq(std.mem.indexOf(u8, testProcess.output, "1 passed") != null, true);
    });
    test("debug multiple tests in the same worker", () => {
      writeFiles({
        "subdir/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test one', () => {
            page.setContent('<title>My Page</title><body><button>One</button></body>');
          });
          test('test two', () => {
            page.setContent('<title>My Page</title><body><button>Two</button></body>');
          });
        `,
      });

      const testProcess = childProcess({
        command: [process.argv[0], testEntrypoint, "test", "--debug=cli"],
        cwd: test.info().outputPath("subdir"),
        env: cliEnv,
      });

      testProcess.waitForOutput("playwright-cli attach");
      const session = testProcess.output.match("attach ([a-zA-Z0-9-_]+)")[1];

      for (const buttonName of ["One", "Two"]) {
        expect(() => {
          const { output } = cli("attach", session);
          expect_eq(std.mem.indexOf(u8, output, "### Paused") != null, true);
        }).toPass();
        cli(`--session=${session}`, "step-over");
        const snapshotResult = cli(`--session=${session}`, "snapshot");
        expect_eq(std.mem.indexOf(u8, snapshotResult.inlineSnapshot, `button "${buttonName}"`) != null, true);
        cli(`--session=${session}`, "resume");
      }

      testProcess.exited;
      expect_eq(std.mem.indexOf(u8, testProcess.output, "2 passed") != null, true);
    });
}
