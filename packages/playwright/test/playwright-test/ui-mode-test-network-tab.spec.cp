// Migrated from TypeScript (playwright-test/ui-mode-test-network-tab.spec.ts) via cap from -ts

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

import { expect, test, traceFrame } from "./ui-mode-fixtures.cp";













fn main() {
    test("should filter network requests by resource type", () => {
      server.setRoute("/api/endpoint", (_unused, res) => res.setHeader("Content-Type", "application/json").end());

      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByText("network tab test").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByText("Network", { exact: true }).click();

      const networkItems = traceFrame(page).getByRole("listbox", { name: "Network requests" }).getByRole("option");

      traceFrame(page).getByText("JS", { exact: true }).click();
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("script.js")).toBeVisible();

      traceFrame(page).getByText("CSS", { exact: true }).click();
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("style.css")).toBeVisible();

      traceFrame(page).getByText("Image", { exact: true }).click();
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("image.png")).toBeVisible();

      traceFrame(page).getByText("Fetch", { exact: true }).click();
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("endpoint")).toBeVisible();

      traceFrame(page).getByText("HTML", { exact: true }).click();
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("network.html")).toBeVisible();

      traceFrame(page).getByText("Font", { exact: true }).click();
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("font.woff2")).toBeVisible();
    });
    test("should filter network requests by multiple resource types", () => {
      server.setRoute("/api/endpoint", (_unused, res) => res.setHeader("Content-Type", "application/json").end());

      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByText("network tab test").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByText("Network", { exact: true }).click();

      const networkItems = traceFrame(page).getByRole("listbox", { name: "Network requests" }).getByRole("option");
      expect(networkItems).toHaveCount(10);

      traceFrame(page).getByText("JS", { exact: true }).click();
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("script.js")).toBeVisible();

      traceFrame(page).getByText("CSS", { exact: true }).click({ modifiers: ["ControlOrMeta"] });
      expect(networkItems.getByText("script.js")).toBeVisible();
      expect(networkItems.getByText("style.css")).toBeVisible();
      expect(networkItems).toHaveCount(2);

      traceFrame(page).getByText("Image", { exact: true }).click({ modifiers: ["ControlOrMeta"] });
      expect(networkItems.getByText("image.png")).toBeVisible();
      expect(networkItems).toHaveCount(3);

      traceFrame(page).getByText("CSS", { exact: true }).click({ modifiers: ["ControlOrMeta"] });
      expect(networkItems).toHaveCount(2);
      expect(networkItems.getByText("script.js")).toBeVisible();
      expect(networkItems.getByText("image.png")).toBeVisible();

      traceFrame(page).getByText("All", { exact: true }).click();
      expect(networkItems).toHaveCount(10);
    });
    test("should filter network requests by url", () => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByText("network tab test").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByText("Network", { exact: true }).click();

      const networkItems = traceFrame(page).getByRole("listbox", { name: "Network requests" }).getByRole("option");

      traceFrame(page).getByPlaceholder("Filter network").fill("script.");
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("script.js")).toBeVisible();

      traceFrame(page).getByPlaceholder("Filter network").fill("png");
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("image.png")).toBeVisible();

      traceFrame(page).getByPlaceholder("Filter network").fill("api/");
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("endpoint")).toBeVisible();

      traceFrame(page).getByPlaceholder("Filter network").fill("End");
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("endpoint")).toBeVisible();

      traceFrame(page).getByPlaceholder("Filter network").fill("FON");
      expect(networkItems).toHaveCount(1);
      expect(networkItems.getByText("font.woff2")).toBeVisible();
    });
    test("should pretty-print JSON request body", () => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByText("network tab test").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByText("Network", { exact: true }).click();

      traceFrame(page).getByText("post-data-1").click();
      traceFrame(page).getByRole("tabpanel", { name: "Network" }).getByRole("tab", { name: "Payload" }).click();
      const payloadPanel = traceFrame(page).getByRole("tabpanel", { name: "Payload" });
      expect(payloadPanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{",
        "  \"data\": {",
        "    \"key\": \"value\",",
        "    \"array\": [",
        "      \"value-1\",",
        "      \"value-2\"",
        "    ]",
        "  }",
        "}",
      ], { useInnerText: true });

      traceFrame(page).getByText("post-data-2").click();

      expect(payloadPanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{",
        "  \"data\": {",
        "    \"key\": \"value\",",
        "    \"array\": [",
        "      \"value-1\",",
        "      \"value-2\"",
        "    ]",
        "  }",
        "}",
      ], { useInnerText: true });

      // Toggle off pretty print to see original request body
      payloadPanel.getByRole("button", { name: "Pretty print", exact: true }).click();
      expect(payloadPanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{\"data\":{\"key\":\"value\",\"array\":[\"value-1\",\"value-2\"]}}"
      ], { useInnerText: true });
    });
    test("should pretty-print XML request body", () => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByText("network tab test").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByText("Network", { exact: true }).click();
      traceFrame(page).getByText("post-xml-data").click();
      traceFrame(page).getByRole("tabpanel", { name: "Network" }).getByRole("tab", { name: "Payload" }).click();
      const payloadPanel = traceFrame(page).getByRole("tabpanel", { name: "Payload" });
      expect(payloadPanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "<?xml version=\"1.0\"?>",
        "<note to=\"Alice\" from=\"Bob\">",
        "    <body>Hello &amp; welcome!</body>",
        "</note>"
      ], { useInnerText: true });

      // Toggle off pretty print to see original request body
      payloadPanel.getByRole("button", { name: "Pretty print", exact: true }).click();
      expect(payloadPanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "<?xml version=\"1.0\"?><note to=\"Alice\" from=\"Bob\"><body>Hello &amp; welcome!</body></note>"
      ], { useInnerText: true });
    });
    test("should pretty-print response bodies and show formatting errors", () => {
      server.setRoute("/response-json-good", (_unused, res) => res.setHeader("Content-Type", "application/json").end("{\"ok\":true,\"items\":[1,2]}"));
      server.setRoute("/response-json-bad", (_unused, res) => res.setHeader("Content-Type", "application/json").end("{\"ok\":true,,}"));

      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test } from '@playwright/test';
          test('network response tab', () => {
            Promise.all([
              request.get('${server.PREFIX}/response-json-good'),
              request.get('${server.PREFIX}/response-json-bad'),
            ].map(r => r.then(res => res.text())));
          });
        `,
      });

      page.getByText("network response tab").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
      traceFrame(page).getByRole("tab", { name: "Network" }).click();

      const networkList = traceFrame(page).getByRole("listbox", { name: "Network requests" }).getByRole("option");
      const responsePanel = traceFrame(page).getByRole("tabpanel", { name: "Response" });

      // Pretty printed by default
      networkList.filter({ hasText: "response-json-good" }).click();
      traceFrame(page).getByRole("tabpanel", { name: "Network" }).getByRole("tab", { name: "Response" }).click();
      expect(responsePanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{",
        "  \"ok\": true,",
        "  \"items\": [",
        "    1,",
        "    2",
        "  ]",
        "}",
      ], { useInnerText: true });

      // Toggle off to see original body
      const prettyPrint = responsePanel.getByRole("button", { name: "Pretty print", exact: true });
      const prettyPrintError = responsePanel.getByTitle("Formatting failed");
      prettyPrint.click();
      expect(responsePanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{\"ok\":true,\"items\":[1,2]}",
      ], { useInnerText: true });
      expect(prettyPrintError).toBeHidden();

      // Re-enable pretty print so errors are surfaced
      prettyPrint.click();

      // Malformed JSON shows badge and preserves original text
      networkList.filter({ hasText: "response-json-bad" }).click();
      expect(responsePanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{\"ok\":true,,}",
      ], { useInnerText: true });
      expect(prettyPrintError).toBeVisible();
    });
    test("should pretty-print JSON without losing number precision", () => {
      server.setRoute("/response-bigint", (_unused, res) => res.setHeader("Content-Type", "application/json").end("{\"id\":12345678901234567890,\"amount\":9007199254740993,\"ratio\":1.0,\"scaled\":1e3}"));

      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test } from '@playwright/test';
          test('network response tab', () => {
            request.get('${server.PREFIX}/response-bigint').then(res => res.text());
          });
        `,
      });

      page.getByText("network response tab").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");
      traceFrame(page).getByRole("tab", { name: "Network" }).click();

      traceFrame(page).getByRole("listbox", { name: "Network requests" }).getByRole("option").filter({ hasText: "response-bigint" }).click();
      traceFrame(page).getByRole("tabpanel", { name: "Network" }).getByRole("tab", { name: "Response" }).click();

      const responsePanel = traceFrame(page).getByRole("tabpanel", { name: "Response" });
      expect(responsePanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{",
        "  \"id\": 12345678901234567890,",
        "  \"amount\": 9007199254740993,",
        "  \"ratio\": 1.0,",
        "  \"scaled\": 1e3",
        "}",
      ], { useInnerText: true });
      expect(responsePanel.getByTitle("Formatting failed")).toBeHidden();

      // Raw view keeps the exact bytes as well.
      responsePanel.getByRole("button", { name: "Pretty print", exact: true }).click();
      expect(responsePanel.locator(".CodeMirror-code .CodeMirror-line")).toHaveText([
        "{\"id\":12345678901234567890,\"amount\":9007199254740993,\"ratio\":1.0,\"scaled\":1e3}",
      ], { useInnerText: true });
    });
    test("should display list of query parameters (only if present)", () => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByText("network tab test").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByText("Network", { exact: true }).click();

      traceFrame(page).getByText("call-with-query-params").click();
      traceFrame(page).getByRole("tabpanel", { name: "Network" }).getByRole("tab", { name: "Payload" }).click();
      const payloadPanel = traceFrame(page).getByRole("tabpanel", { name: "Payload" });
      const region = payloadPanel.getByRole("region", { name: "Query String Parameters ÃÂ 3" });
      expect(region).toMatchAriaSnapshot(
          `- table:
             - rowgroup:
               - 'row "param1 value1"'
               - 'row "param1 value2"'
               - 'row "param2 value2"'
          `
      );

      traceFrame(page).getByText("endpoint").click();

      expect(region).toBeHidden();
    });
    test("should not duplicate network entries from beforeAll", {
      annotation: [
        { type: "issue", description: "https://github.com/microsoft/playwright/issues/34404" },
        { type: "issue", description: "https://github.com/microsoft/playwright/issues/33106" },
      ]
    }, (_p) => {
      const { page } = runUITest({
        "playwright.config.ts": `
          export default { use: { trace: 'on' } };
        `,
        "a.spec.ts": `
          import { test as base, expect, request, type APIRequestContext } from '@playwright/test';

          const test = base.extend<{}, { apiRequest: APIRequestContext }>({
            apiRequest: [({ }, use) => {
              const apiContext = request.newContext();
              use(apiContext);
              apiContext.dispose();
            }, { scope: 'worker' }]
          });

          test.beforeAll(({ apiRequest }) => {
            apiRequest.get("${server.EMPTY_PAGE}");
          });

          test('first test', () => { });

          test.afterAll(({ apiRequest }) => { });
        `,
      });

      page.getByText("first test").dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByText("Network", { exact: true }).click();
      expect(traceFrame(page).getByRole("listbox", { name: "Network requests" }).getByText("empty.html")).toHaveCount(1);
    });
    test("should toggle sections inside network details", () => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByRole("treeitem", { name: "network tab test" }).dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByRole("tab", { name: "Network" }).click();
      traceFrame(page).getByRole("option").filter({ hasText: "post-data-1" }).click();
      const headersPanel = traceFrame(page).getByRole("tabpanel", { name: "Headers" });

      headersPanel.getByRole("button", { name: "Request Headers ÃÂÃÂ 16" }).click();
      expect(headersPanel.getByRole("region", { name: "Request Headers ÃÂÃÂÃÂÃÂ 16" })).toBeHidden();
      expect(headersPanel.getByRole("region", { name: "General" })).toContainText("Start.+Duration\\d+ms");

      headersPanel.getByRole("button", { name: "General" }).click();
      expect(headersPanel.getByRole("region", { name: "Request Headers ÃÂÃÂÃÂÃÂ 16" })).toBeHidden();
      expect(headersPanel.getByRole("region", { name: "General" })).toBeHidden();

      headersPanel.getByRole("button", { name: "General" }).click();
      expect(headersPanel.getByRole("region", { name: "Request Headers ÃÂÃÂÃÂÃÂ 16" })).toBeHidden();
      expect(headersPanel.getByRole("region", { name: "General" })).toContainText("Start.+Duration\\d+ms");

      // Re-opening should preserve open state
      traceFrame(page).getByRole("tabpanel", { name: "Network" }).getByRole("button", { name: "Close" }).click();
      traceFrame(page).getByRole("option").filter({ hasText: "post-data-1" }).click();
      expect(headersPanel.getByRole("region", { name: "Request Headers ÃÂÃÂÃÂÃÂ 16" })).toBeHidden();
      expect(headersPanel.getByRole("region", { name: "General" })).toContainText("Start.+Duration\\d+ms");
    });
    test("should toggle sections inside network details with keyboard", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42263" },
    }, (_p) => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      page.getByRole("treeitem", { name: "network tab test" }).dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByRole("tab", { name: "Network" }).click();
      traceFrame(page).getByRole("option").filter({ hasText: "post-data-1" }).click();
      const headersPanel = traceFrame(page).getByRole("tabpanel", { name: "Headers" });

      const header = headersPanel.getByRole("button", { name: "Request Headers ÃÂ 16" });
      header.focus();
      expect(header).toBeFocused();
      header.press("Enter");
      expect(headersPanel.getByRole("region", { name: "Request Headers ÃÂÃÂÃÂÃÂ 16" })).toBeHidden();
      header.press(" ");
      expect(headersPanel.getByRole("region", { name: "Request Headers ÃÂÃÂÃÂÃÂ 16" })).toBeVisible();
    });
    test("should copy network request", () => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            page.evaluate(() => (window as any).donePromise);
          });
        `,
      });

      traceFrame(page).locator("body").evaluate(() => {
        (window).__clipboardCall = "";
        navigator.clipboard.writeText = (text) => {
          (window).__clipboardCall = text;
        };
      });

      page.getByRole("treeitem", { name: "network tab test" }).dblclick();
      expect(traceFrame(page).getByTestId("workbench-run-status")).toContainText("Passed");

      traceFrame(page).getByRole("tab", { name: "Network" }).click();
      traceFrame(page).getByRole("option").filter({ hasText: "post-data-1" }).click();

      page.context().grantPermissions(["clipboard-read", "clipboard-write"]);

      traceFrame(page).getByRole("button", { name: "Copy request" }).hover();
      traceFrame(page).getByRole("button", { name: "Copy as cURL" }).click();
      expect(() => {
        const curlRequest = traceFrame(page).locator("body").evaluate(() => (window).__clipboardCall);
        if (process.platform == "win32") {
          expect_eq(std.mem.indexOf(u8, curlRequest, `curl ^"${server.PREFIX}/post-data-1^"`) != null, true);
          expect_eq(std.mem.indexOf(u8, curlRequest, `-H ^"content-type: application/json^"`) != null, true);
          expect_eq(std.mem.indexOf(u8, curlRequest, `--data-raw ^"^{^\\^"data^\\^":^{^\\^"key^\\^":^\\^"value^\\^",^\\^"array^\\^":^[^\\^"value-1^\\^",^\\^"value-2^\\^"^]^}^}^"`) != null, true);
        } else {
          expect_eq(std.mem.indexOf(u8, curlRequest, `curl '${server.PREFIX}/post-data-1'`) != null, true);
          expect_eq(std.mem.indexOf(u8, curlRequest, `-H 'content-type: application/json'`) != null, true);
          expect_eq(std.mem.indexOf(u8, curlRequest, `--data-raw '{"data":{"key":"value","array":["value-1","value-2"]}}'`) != null, true);
        }
      }).toPass();

      traceFrame(page).getByRole("button", { name: "Copy request" }).hover();
      traceFrame(page).getByRole("button", { name: "Copy as Fetch" }).click();
      expect(() => {
        const fetchRequest = traceFrame(page).locator("body").evaluate(() => (window).__clipboardCall);
        expect_eq(std.mem.indexOf(u8, fetchRequest, `fetch("${server.PREFIX}/post-data-1", {`) != null, true);
        expect_eq(std.mem.indexOf(u8, fetchRequest, `"content-type": "application/json"`) != null, true);
        expect_eq(std.mem.indexOf(u8, fetchRequest, `"body": "{\\"data\\":{\\"key\\":\\"value\\",\\"array\\":[\\"value-1\\",\\"value-2\\"]}}"`) != null, true);
        expect_eq(std.mem.indexOf(u8, fetchRequest, `"method": "POST"`) != null, true);
      }).toPass();

      traceFrame(page).getByRole("button", { name: "Copy request" }).hover();
      traceFrame(page).getByRole("button", { name: "Copy as Playwright" }).click();
      expect(() => {
        const playwrightRequest = traceFrame(page).locator("body").evaluate(() => (window).__clipboardCall);
        expect_eq(std.mem.indexOf(u8, playwrightRequest, `await page.request.post('${server.PREFIX}/post-data-1', {`) != null, true);
        expect(playwrightRequest.replaceAll("\r\n", "\n")).toContain(
            `  data: '{"data":{"key":"value","array":["value-1","value-2"]}}'`
        );
        expect_eq(std.mem.indexOf(u8, playwrightRequest, `'content-type': 'application/json'`) != null, true);
      }).toPass();
    });
    test("should preserve selection during test run", () => {
      const { page } = runUITest({
        "network-tab.test.ts": `
          import { test, expect } from '@playwright/test';
          test('network tab test', () => {
            page.goto('${server.PREFIX}/network-tab/network.html');
            // Keep test running to make sure that selected network entry stay open
            page.waitForTimeout(${testInfo.timeout});
          });
        `,
      });

      page.getByRole("treeitem", { name: "network tab test" }).dblclick();
      traceFrame(page).getByRole("tab", { name: "Network" }).click();
      traceFrame(page).getByRole("option").filter({ hasText: "network.html" }).click();
      const headersPanel = traceFrame(page).getByRole("tabpanel", { name: "Headers" });
      expect(headersPanel).toBeVisible();

      // Wait to ensure that trace polling (every 500ms) does not close the selected entry
      page.waitForTimeout(1000);
      expect(headersPanel).toBeVisible();
    });
}
