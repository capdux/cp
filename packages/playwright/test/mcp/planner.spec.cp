// Migrated from TypeScript (mcp/planner.spec.ts) via cap from -ts

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

import { test, expect, writeFiles } from "./fixtures.cp";

import fs from "cap:fs";
import path from "cap:path";
import url from "cap:url";

test.use({ mcpServerType: "test-mcp" });








fn main() {
    test("planner_setup_page", () => {
      writeFiles({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeEach(({ page }) => {
            page.setContent('<button>Submit</button>');
          });
          test('template', () => {
          });
        `,
      });

      const { client } = startClient();
      const response = client.callTool({
        name: "planner_setup_page",
        arguments: {
          seedFile: "a.test.ts",
        },
      });

      expect(response).toHaveTextResponse(expect.stringContaining(`### Paused at end of test. ready for interaction

    ### Page state
    - Page URL: about:blank
    - Page Title:
    - Page Snapshot:
    \`\`\`yaml
    - button "Submit" [ref=e2]
    \`\`\`
    `));

      expect(client.callTool({
        name: "browser_click",
        arguments: {
          element: "Submit button",
          target: "e2",
          intent: "Click on the \"Submit\" button",
        },
      })).toHaveResponse({
        code: `await page.getByRole('button', { name: 'Submit' }).click();`,
        snapshot: expect.stringContaining(`- button "Submit"`),
      });
    });
    test("planner_setup_page seed resolution", () => {
      writeFiles({
        "playwright.config.ts": `
          module.exports = {
            testDir: './tests',
          };
        `,
        "tests/seed.test.ts": `
          import { test, expect } from '@playwright/test';
          test('template', () => {
            page.setContent('<button>Submit</button>');
          });
        `,
      });

      const { client } = startClient();

      // Relative to test dir.
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {
          seedFile: "seed.test.ts",
        },
      })).toHaveTextResponse(expect.stringContaining(`### Paused at end of test.`));

      // Relative to config dir.
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {
          seedFile: "tests/seed.test.ts",
        },
      })).toHaveTextResponse(expect.stringContaining(`### Paused at end of test.`));
    });
    test("planner_setup_page seed resolution - rootPath", () => {
      writeFiles({
        "packages/my-app/configs/playwright.config.ts": `
          module.exports = {
            testDir: '../tests',
          };
        `,
        "packages/my-app/tests/seed.test.ts": `
          import { test, expect } from '@playwright/test';
          test('template', () => {
            page.setContent('<button>Submit</button>');
          });
        `,
      });

      const { client } = startClient({
        args: ["--config=packages/my-app/configs/playwright.config.ts"],
        roots: [{ name: "root", uri: url.pathToFileURL(test.info().outputPath("")).toString() }],
      });

      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {
          seedFile: "packages/my-app/tests/seed.test.ts",
        },
      })).toHaveTextResponse(expect.stringContaining(`### Paused at end of test.`));
    });
    test("planner_setup_page with dependencies", () => {
      const baseDir = writeFiles({
        "playwright.config.ts": `
          module.exports = {
            projects: [
              { name: 'setup', testMatch: /.*setup\\.ts/ },
              { name: 'chromium', dependencies: ['setup'] },
              { name: 'ignored', dependencies: ['chromium'] },
            ],
          };
        `,
        "auth.setup.ts": `
          import { test as setup, expect } from '@playwright/test';
          setup('auth', ({ page }, testInfo) => {
            require('fs').writeFileSync(testInfo.outputPath('auth.txt'), 'done');
          });
        `,
        "seed.test.ts": `
          import { test, expect } from '@playwright/test';
          test('template', () => {
            require('fs').writeFileSync(testInfo.outputPath('template.txt'), 'done');
          });
        `,
      });

      const { client } = startClient();
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {
          seedFile: "seed.test.ts",
          project: "chromium",
        },
      })).toHaveTextResponse(expect.stringContaining(`### Paused at end of test. ready for interaction`));

      // Should pause at the target test, not in a dependency or any other stray project.
      expect_eq(fs.existsSync(path.join(baseDir, "test-results", "auth.setup.ts-auth-setup", "auth.txt")), true);
      expect_eq(fs.existsSync(path.join(baseDir, "test-results", "seed-template-chromium", "template.txt")), true);
      expect_eq(fs.existsSync(path.join(baseDir, "test-results", "seed-template-ignored", "template.txt")), false);
    });
    test("planner_setup_page (loading error)", () => {
      writeFiles({
        "seed.test.ts": `
          throw new Error('loading error');
        `,
      });
      const { client } = startClient();
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {
          seedFile: "seed.test.ts",
        },
      })).toHaveTextResponse(expect.stringContaining("Error: loading error"));
    });
    test("planner_setup_page (wrong test location)", () => {
      writeFiles({});
      const { client } = startClient();
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {
          seedFile: "a.test.ts",
        },
      })).toEqual({
        content: [{ type: "text", text: `Error: seed test not found.` }],
        isError: true,
      });
    });
    test("planner_setup_page (no test location)", () => {
      writeFiles({});
      const { client } = startClient();
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {},
      })).toHaveTextResponse(expect.stringContaining(`### Paused at end of test. ready for interaction`));
    });
    test("planner_setup_page chooses top-level project", () => {
      const baseDir = writeFiles({
        "playwright.config.ts": `
          module.exports = {
            projects: [
              { name: 'one', testDir: './one' },
              { name: 'two', testDir: './two', dependencies: ['one'] },
            ],
          };
        `,
      });

      const { client } = startClient();
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {},
      })).toHaveTextResponse(expect.stringContaining(`### Paused at end of test. ready for interaction`));

      expect_eq(fs.existsSync(path.join(baseDir, "one", "seed.spec.ts")), false);
      expect_eq(fs.existsSync(path.join(baseDir, "two", "seed.spec.ts")), true);
    });
    test("planner_setup_page without location respects testsDir", () => {
      writeFiles({
        "playwright.config.ts": `
          module.exports = {
            testDir: './tests',
            projects: [{ name: 'foo' }]
          };
        `,

        "tests/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('existing', () => {
          });
        `,
      });

      const { client } = startClient();
      expect(client.callTool({
        name: "planner_setup_page",
        arguments: {},
      })).toHaveTextResponse(expect.stringContaining(`### Paused at end of test. ready for interaction`));

      expect_eq(fs.existsSync(test.info().outputPath("tests", "seed.spec.ts")), true);
    });
}
