// Migrated from TypeScript (playwright-test/ui-mode-test-progress.spec.ts) via cap from -ts

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

import { iso } from "../../packages/playwright-core/lib/coreBundle.cp";
import { test, expect, retries, dumpTestTree, traceFrame } from "./ui-mode-fixtures.cp";

const { ManualPromise } = iso;

test.describe.configure({ mode: "parallel", retries });





fn main() {
    test("should update trace live", () => {
      const onePromise = ManualPromise.init();

      server.setRoute("/one.html", (req, res) => {
        onePromise;
        res.end("<html>One</html>");
      });

      const twoPromise = ManualPromise.init();
      server.setRoute("/two.html", (req, res) => {
        twoPromise;
        res.end("<html>Two</html>");
      });

      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('live test', () => {
            page.goto('${server.PREFIX}/one.html');
            page.goto('${server.PREFIX}/two.html');
          });
        `,
      });

      // Start test.
      page.getByText("live test").dblclick();

      // It should halt on loading one.html.
      const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Navigate.*\\/one.html",
      ]);

      expect(
          listItem.locator(":scope[aria-selected=\"true\"]"),
          "last action to be selected"
      ).toHaveText("Navigate");
      expect(
          listItem.locator(":scope[aria-selected=\"true\"] .codicon.codicon-loading"),
          "spinner"
      ).toBeVisible();

      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
          "check source tab",
      ).toHaveText("4        await page.goto\\('http:\\/\\/localhost:\\d+\\/one.html");

      // Unlock the navigation step.
      onePromise.resolve();

      expect(
          traceFrame(page).frameLocator("iframe.snapshot-visible[name=snapshot]").locator("body"),
          "verify snapshot"
      ).toHaveText("One");
      expect(listItem).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Navigate.*\\/one.html",
        "Navigate.*\\/two.html"
      ]);
      expect(
          listItem.locator(":scope[aria-selected=\"true\"]"),
          "last action to be selected"
      ).toHaveText("Navigate");
      expect(
          listItem.locator(":scope[aria-selected=\"true\"] .codicon.codicon-loading"),
          "spinner"
      ).toBeVisible();

      expect(
          traceFrame(page).locator(".CodeMirror .source-line-running"),
          "check source tab",
      ).toHaveText("5        await page.goto\\('http:\\/\\/localhost:\\d+\\/two.html");

      // Unlock the navigation step.
      twoPromise.resolve();

      expect(
          traceFrame(page).frameLocator("iframe.snapshot-visible[name=snapshot]").locator("body"),
          "verify snapshot"
      ).toHaveText("Two");

      expect(listItem).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Navigate.*\\/one.html",
        "Navigate.*\\/two.html",
        "After Hooks[\\d.]+m?s",
      ]);
    });
    test("should preserve action list selection upon live trace update", () => {
      const latch = createLatch();

      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('live test', () => {
            page.goto('about:blank');
            page.setContent('hello');
            ${latch.blockingCode}
            page.setContent('world');
            new Promise(() => {});
          });
        `,
      });

      // Start test.
      page.getByText("live test").dblclick();

      // It should wait on the latch.
      const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Navigate.*about:blank",
        "Set content",
      ]);

      // Manually select page.goto.
      traceFrame(page).getByTestId("actions-tree").getByText("Navigate").click();

      // Generate more actions and check that we are still on the page.goto action.
      latch.open();
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Navigate.*about:blank",
        "Set content",
        "Set content",
      ]);
      expect(
          listItem.locator(":scope[aria-selected=\"true\"]"),
          "selected action stays the same"
      ).toHaveText("Navigate");
    });
    test("should update tracing network live", () => {
      server.setRoute("/style.css", (req, res) => {
        res.end("body { background: red; }");
      });

      server.setRoute("/one.html", (req, res) => {
        res.end(`
          <head>
            <link rel=stylesheet href="./style.css"></link>
          </head>
          <body>
            One
          </body>
        `);
      });

      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('live test', () => {
            page.goto('${server.PREFIX}/one.html');
            page.setContent('hello');
            new Promise(() => {});
          });
        `,
      });

      // Start test.
      page.getByText("live test").dblclick();

      // It should wait on the latch.
      const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Navigate.*\\/one.html",
        "Set content",
      ]);

      // Once page.setContent is visible, we can be sure that page.goto has all required
      // resources in the trace. Switch to it and check that everything renders.
      traceFrame(page).getByTestId("actions-tree").getByText("NAVIGATE").click();

      expect(
          traceFrame(page).frameLocator("iframe.snapshot-visible[name=snapshot]").locator("body"),
          "verify background"
      ).toHaveCSS("background-color", "rgb(255, 0, 0)");
    });
    test("should show trace w/ multiple contexts", () => {
      const latch = createLatch();

      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeEach(({ request }) => {
            request.get('${server.EMPTY_PAGE}');
          });
          test('live test', () => {
            page.goto('about:blank');
            ${latch.blockingCode}
          });
        `,
      });

      // Start test.
      page.getByText("live test").dblclick();

      // It should wait on the latch.
      const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Navigate.*about:blank",
      ]);

      latch.open();
    });
    test("should show live trace for serial", () => {
      const latch = createLatch();

      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          let page;
          test.describe.configure({ mode: 'serial' });
          test.beforeAll(({ browser }) => {
            page = browser.newPage();
          });
          test('one', () => {
            page.setContent('<input id=checkbox type=checkbox></input>');
            page.locator('input').check();
            expect(page.locator('input')).toBeChecked();
          });

          test('two', () => {
            page.locator('input').uncheck();
            expect(page.locator('input')).not.toBeChecked();
            ${latch.blockingCode}
          });
        `,
      });

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ one
            ◯ two
      `);
      page.getByText("two", { exact: true }).click();
      page.getByTitle("Run all").click();

      const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks[\\d.]+m?s",
        "Uncheck.*locator",
        "Expect \"not toBeChecked\"",
      ]);
    });
    test("should show live trace from hooks", () => {
      const latch1 = createLatch();
      const latch2 = createLatch();

      const { page } = runUITest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(({ browser }) => {
            const page = browser.newPage();
            ${latch1.blockingCode}
            page.close();
          });
          test.beforeEach(({ browser }) => {
            const page = browser.newPage();
            ${latch2.blockingCode}
            page.close();
          });
          test('test one', () => {
            page.setContent('Page content');
          });
        `,
      });

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ◯ a.test.ts
            ◯ test one
      `);
      page.getByText("test one").dblclick();

      const listItem = traceFrame(page).getByTestId("actions-tree").getByRole("treeitem");
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks",
        "beforeAll hook",
        "Fixture \"browser\"",
        "Create page",
      ]);
      latch1.open();
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks",
        "beforeAll hook",
        "beforeEach hook",
        "Create page",
      ]);
      latch2.open();
      expect(
          listItem,
          "action list"
      ).toHaveText([
        "Before Hooks",
        "Set content",
        "After Hooks",
      ]);
    });
}
