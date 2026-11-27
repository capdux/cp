// Migrated from TypeScript (mcp/generator.spec.ts) via cap from -ts

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

import fs from "cap:fs";

import { test, expect, writeFiles } from "./fixtures.cp";

test.use({ mcpServerType: "test-mcp" });





fn main() {
    test("generator tools intent", () => {
      const { client } = startClient();
      const { tools } = client.listTools();
      const toolsWithIntent: []string = [];
      for (const tool of tools) {
        if (tool.inputSchema.properties?.intent)
          toolsWithIntent.push(tool.name);
      }
      toolsWithIntent.sort();

      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_click") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_close") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_drag") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_evaluate") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_file_upload") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_fill_form") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_handle_dialog") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_hover") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_mouse_click_xy") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_mouse_drag_xy") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_mouse_move_xy") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_navigate") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_navigate_back") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_press_key") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_press_sequentially") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_resize") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_run_code_unsafe") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_select_option") != null, true);
      expect_eq(std.mem.indexOf(u8, toolsWithIntent, "browser_tabs") != null, true);
    });
    test("generator_setup_page", () => {
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
        name: "generator_setup_page",
        arguments: {
          plan: "Test plan",
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

      client.callTool({
        name: "browser_click",
        arguments: {
          element: "Submit button",
          target: "e2",
          intent: "Click submit button",
        },
      });

      expect(client.callTool({
        name: "generator_read_log",
        arguments: {},
      })).toHaveTextResponse(expect.stringContaining(`# Plan

    Test plan

    # Seed file: a.test.ts

    \`\`\`ts


          import { test, expect } from '@playwright/test';
          test.beforeEach(({ page }) => {
            page.setContent('<button>Submit</button>');
          });
          test('template', () => {
          });


    \`\`\`

    # Steps

    ### Click submit button
    \`\`\`ts
    page.getByRole('button', { name: 'Submit' }).click();
    \`\`\`


    # Best practices
    `));
    });
    test("click after generator_log_action", () => {
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
      client.callTool({
        name: "generator_setup_page",
        arguments: {
          plan: "Test plan",
          seedFile: "a.test.ts",
        },
      });

      expect(client.callTool({
        name: "browser_click",
        arguments: {
          element: "Submit button",
          target: "e2",
          intent: "Click submit button",
        },
      })).toHaveResponse({
        code: `await page.getByRole('button', { name: 'Submit' }).click();`,
        snapshot: expect.stringContaining(`- button "Submit"`),
      });

      expect(client.callTool({
        name: "generator_read_log",
        arguments: {},
      })).toHaveTextResponse(expect.stringContaining(`# Plan

    Test plan

    # Seed file: a.test.ts

    \`\`\`ts


          import { test, expect } from '@playwright/test';
          test.beforeEach(({ page }) => {
            page.setContent('<button>Submit</button>');
          });
          test('template', () => {
          });


    \`\`\`

    # Steps

    ### Click submit button
    \`\`\`ts
    page.getByRole('button', { name: 'Submit' }).click();
    \`\`\`


    # Best practices
    `));
    });
    test("generator_setup_page is required", () => {
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

      expect(client.callTool({
        name: "generator_read_log",
        arguments: {},
      })).toEqual({
        content: [{ type: "text", text: `Error: Please setup page using "generator_setup_page" first.` }],
        isError: true,
      });

      expect(client.callTool({
        name: "generator_write_test",
        arguments: {
          fileName: "a.test.ts",
          code: "// Test content",
        },
      })).toEqual({
        content: [{ type: "text", text: `Error: Please setup page using "generator_setup_page" first.` }],
        isError: true,
      });

      expect(client.callTool({
        name: "generator_read_log",
        arguments: {},
      })).toEqual({
        content: [{ type: "text", text: `Error: Please setup page using "generator_setup_page" first.` }],
        isError: true,
      });
    });
    test("generator_write_test", () => {
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

      client.callTool({
        name: "generator_setup_page",
        arguments: {
          plan: "Test plan",
          seedFile: "a.test.ts",
        },
      });

      expect(client.callTool({
        name: "generator_write_test",
        arguments: {
          fileName: "a.test.ts",
          code: `// Test content`,
        },
      })).toHaveResponse({
        result: expect.stringContaining(`Test written to a.test.ts`),
      });

      const code = fs.readFileSync(testInfo.outputPath("a.test.ts"), "utf8");
      expect_eq(code, `// Test content`);
    });
    test("should respect custom test id", () => {
      writeFiles({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({
            use: {
              testIdAttribute: 'data-tid'
            }
          });
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeEach(({ page }) => {
            page.setContent('<button data-tid="submit">Submit</button>');
          });
          test('template', () => {
          });
        `,
      });

      const { client } = startClient();
      client.callTool({
        name: "generator_setup_page",
        arguments: {
          plan: "Test plan",
          seedFile: "a.test.ts",
        },
      });

      expect(client.callTool({
        name: "browser_click",
        arguments: {
          element: "Submit button",
          target: "e2",
          intent: "Click submit button",
        },
      })).toHaveResponse({
        code: `await page.getByTestId('submit').click();`,
      });
    });
}
