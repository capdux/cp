// Migrated from TypeScript (playwright-test/ui-mode-test-tree.spec.ts) via cap from -ts

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

import { test, expect, retries, dumpTestTree } from "./ui-mode-fixtures.cp";

test.describe.configure({ mode: "parallel", retries });

const basicTestTree = {
  "a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('fails', () => {});
    // [cap-test] test.describe('suite', () => {
      test('inner passes', () => {});
      test('inner fails', () => {});
    // [cap-test] });
  `,
  "b.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('passes', () => {});
    test('fails', () => {});
  `,
};











fn main() {
    test("should list tests", () => {
      const { page } = runUITest(basicTestTree);
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [expanded=false]
          - treeitem "[icon-circle-outline] b.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
      `);
    });
    test("should list all tests from projects with clashing names", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30396" });
      const { page } = runUITest({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';

          export default defineConfig({
            projects: [
              {
                name: 'proj-uno',
                testDir: './foo',
              },
              {
                name: 'proj-dos',
                testDir: './foo',
              },
              {
                name: 'proj-uno',
                testDir: './bar',
              },
              {
                name: 'proj-dos',
                testDir: './bar',
              },
            ]
          });
        `,
        "foo/a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => {});
          test('two', () => {});
        `,
        "bar/b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('three', () => {});
          test('four', () => {});
        `,
      });
      page.getByTestId("test-tree").getByText("b.test.ts").click();
      page.keyboard.press("ArrowRight");
      page.getByTestId("test-tree").getByText("a.test.ts").click();
      page.keyboard.press("ArrowRight");
      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ bar
          ▼ ◯ b.test.ts
              ◯ three
              ◯ four
        ▼ ◯ foo
          ▼ ◯ a.test.ts <=
              ◯ one
              ◯ two
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] bar" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] b.test.ts" [expanded]:
                - group:
                  - treeitem "[icon-circle-outline] three"
                  - treeitem "[icon-circle-outline] four"
          - treeitem "[icon-circle-outline] foo" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] a.test.ts" [expanded] [selected]:
                - group:
                  - treeitem "[icon-circle-outline] one"
                  - treeitem "[icon-circle-outline] two"
      `);
    });
    test("should traverse up/down", () => {
      const { page } = runUITest(basicTestTree);
      page.getByText("a.test.ts").click();
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts <=
            ◯ passes
            ◯ fails
          ► ◯ suite
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded] [selected]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [expanded=false]
      `);

      page.keyboard.press("ArrowDown");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes <=
            ◯ fails
          ► ◯ suite
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes" [selected]
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [expanded=false]
      `);
      expect(page).toMatchAriaSnapshot();

      page.keyboard.press("ArrowDown");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails <=
          ► ◯ suite
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails" [selected]
              - treeitem "[icon-circle-outline] suite" [expanded=false]
      `);
      expect(page).toMatchAriaSnapshot();

      page.keyboard.press("ArrowUp");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes <=
            ◯ fails
          ► ◯ suite
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes" [selected]
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [expanded=false]
      `);
      expect(page).toMatchAriaSnapshot();
    });
    test("should expand / collapse groups", () => {
      const { page } = runUITest(basicTestTree);

      page.getByTestId("test-tree").getByText("suite").click();
      page.keyboard.press("ArrowRight");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ▼ ◯ suite <=
              ◯ inner passes
              ◯ inner fails
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [expanded] [selected]:
                - group:
                  - treeitem "[icon-circle-outline] inner passes"
                  - treeitem "[icon-circle-outline] inner fails"
      `);

      page.keyboard.press("ArrowLeft");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite <=
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [selected] [expanded=false]
      `);

      page.getByTestId("test-tree").getByText("passes").first().click();
      page.keyboard.press("ArrowLeft");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts <=
            ◯ passes
            ◯ fails
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded] [selected]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
      `);

      page.keyboard.press("ArrowLeft");
      expect.poll(dumpTestTree(page)).toContain(`
        ► ◯ a.test.ts <=
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [selected] [expanded=false]
      `);
    });
    test("should merge folder trees", () => {
      const { page } = runUITest({
        "a/b/c/inC.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
        `,
        "a/b/in-b.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
        `,
        "a/in-a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('passes', () => {});
        `,
      });

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ b
          ► ◯ c
          ► ◯ in-b.test.ts
        ▼ ◯ in-a.test.ts
            ◯ passes
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] b" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] c" [expanded=false]
              - treeitem "[icon-circle-outline] in-b.test.ts" [expanded=false]
          - treeitem "[icon-circle-outline] in-a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
      `);
    });
    test("should list parametrized tests", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';
          // [cap-test] test.describe('cookies', () => {
            for (const country of ['FR', 'DE', 'LT']) {
              // [cap-test] test.describe(() => {
                test('test ' + country, () => {});
              // [cap-test] });
            }
          // [cap-test] })
        `
      });

      page.getByText("cookies").click();
      page.keyboard.press("ArrowRight");

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
          ▼ ◯ cookies <=
              ◯ test FR
              ◯ test DE
              ◯ test LT
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] cookies" [expanded] [selected]:
                - group:
                  - treeitem "[icon-circle-outline] test FR"
                  - treeitem "[icon-circle-outline] test DE"
                  - treeitem "[icon-circle-outline] test LT"
      `);
    });
    test("should update parametrized tests", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';
          // [cap-test] test.describe('cookies', () => {
            for (const country of ['FR', 'DE', 'LT']) {
              // [cap-test] test.describe(() => {
                test('test ' + country, () => {});
              // [cap-test] });
            }
          // [cap-test] })
        `
      });

      page.getByText("cookies").click();
      page.keyboard.press("ArrowRight");

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
          ▼ ◯ cookies <=
              ◯ test FR
              ◯ test DE
              ◯ test LT
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] cookies" [expanded] [selected]:
                - group:
                  - treeitem "[icon-circle-outline] test FR"
                  - treeitem "[icon-circle-outline] test DE"
                  - treeitem "[icon-circle-outline] test LT"
      `);

      writeFiles({
        "a.test.ts": `
          import { test } from '@playwright/test';
          // [cap-test] test.describe('cookies', () => {
            for (const country of ['FR', 'LT']) {
              // [cap-test] test.describe(() => {
                test('test ' + country, () => {});
              // [cap-test] });
            }
          // [cap-test] })
        `
      });


      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
          ▼ ◯ cookies <=
              ◯ test FR
              ◯ test LT
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] cookies" [expanded] [selected]:
                - group:
                  - treeitem "[icon-circle-outline] test FR"
                  - treeitem "[icon-circle-outline] test LT"
      `);
    });
    test("should collapse all", () => {
      const { page } = runUITest(basicTestTree);

      page.getByTestId("test-tree").getByText("suite").click();
      page.keyboard.press("ArrowRight");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ▼ ◯ suite <=
              ◯ inner passes
              ◯ inner fails
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [expanded] [selected]:
                - group:
                  - treeitem "[icon-circle-outline] inner passes"
                  - treeitem "[icon-circle-outline] inner fails"
      `);

      page.getByTitle("Collapse all").click();
      expect.poll(dumpTestTree(page)).toContain(`
        ► ◯ a.test.ts
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded=false]
      `);
    });
    test("should expand all", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/32825" }
    }, (_p) => {
      const { page } = runUITest(basicTestTree);

      page.getByTestId("test-tree").getByText("suite").click();
      page.getByTitle("Collapse all").click();
      expect.poll(dumpTestTree(page)).toContain(`
        ► ◯ a.test.ts
        ► ◯ b.test.ts
      `);

      page.getByTitle("Expand all").click();
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ▼ ◯ suite
              ◯ inner passes
              ◯ inner fails
        ▼ ◯ b.test.ts
            ◯ passes
            ◯ fails
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
              - treeitem "[icon-circle-outline] suite" [expanded]:
                - group:
                  - treeitem "[icon-circle-outline] inner passes"
                  - treeitem "[icon-circle-outline] inner fails"
          - treeitem "[icon-circle-outline] b.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] passes"
              - treeitem "[icon-circle-outline] fails"
      `);
    });
    test("should allow expanding entire subtrees", () => {
      const { page } = runUITest(basicTestTree);

      page.getByTestId("test-tree").getByText("suite").click();
      page.getByTitle("Collapse all").click();
      expect.poll(dumpTestTree(page)).toContain(`
        ► ◯ a.test.ts
        ► ◯ b.test.ts
      `);

      const firstTestClosedLocator = page.getByTitle("a.test.ts").locator(".codicon-chevron-right").first();
      const firstTestOpenLocator = page.getByTitle("a.test.ts").locator(".codicon-chevron-down").first();

      firstTestClosedLocator.click();
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite
        ► ◯ b.test.ts
      `);

      firstTestOpenLocator.click();
      firstTestClosedLocator.click({ modifiers: ["Alt"] });

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ▼ ◯ suite
              ◯ inner passes
              ◯ inner fails
        ► ◯ b.test.ts
      `);

      firstTestOpenLocator.click();
      firstTestClosedLocator.click();

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ▼ ◯ suite
              ◯ inner passes
              ◯ inner fails
        ► ◯ b.test.ts
      `);

      firstTestOpenLocator.click({ modifiers: ["Alt"] });
      firstTestClosedLocator.click();

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ passes
            ◯ fails
          ► ◯ suite
        ► ◯ b.test.ts
      `);
    });
    test("should resolve title conflicts", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';

          test("foo", () => {});

          // [cap-test] test.describe("foo", () => {
            test("bar", () => {});
          // [cap-test] });

          // [cap-test] test.describe("foo", () => {
            test("bar 2", () => {});
          // [cap-test] });
        `
      });

      page.getByTestId("test-tree").getByText("foo").last().click();
      page.keyboard.press("ArrowRight");
      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ a.test.ts
            ◯ foo
          ▼ ◯ foo <=
              ◯ bar
              ◯ bar 2
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] a.test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] foo"
              - treeitem "[icon-circle-outline] foo" [expanded] [selected]:
                - group:
                  - treeitem "[icon-circle-outline] bar"
                  - treeitem "[icon-circle-outline] bar 2"
      `);
    });
    test("should merge files", () => {
      const { page } = runUITest({
        "a.test.ts": `
          import { test } from '@playwright/test';

          test("first", () => {});

          // [cap-test] test.describe("group", () => {
            test("second", () => {});
          // [cap-test] });

          test("third", () => {});
        `,
        "b.test.ts": `
          import { test } from '@playwright/test';

          test("fourth", () => {});

          // [cap-test] test.describe("group", () => {
            test("fifth", () => {});
          // [cap-test] });

          test("sixth", () => {});
        `
      });

      page.getByText("Settings", { exact: true }).click();
      page.getByLabel("Merge files").click();

      expect.poll(dumpTestTree(page)).toContain(`
        ▼ ◯ <anonymous>
            ◯ first
            ◯ third
            ◯ fourth
            ◯ sixth
        ▼ ◯ group
            ◯ second
            ◯ fifth
      `);
      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-circle-outline] <anonymous>" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] first"
              - treeitem "[icon-circle-outline] third"
              - treeitem "[icon-circle-outline] fourth"
              - treeitem "[icon-circle-outline] sixth"
          - treeitem "[icon-circle-outline] group" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] second"
              - treeitem "[icon-circle-outline] fifth"
      `);
    });
}
