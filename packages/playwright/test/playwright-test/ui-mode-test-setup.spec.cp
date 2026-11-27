// Migrated from TypeScript (playwright-test/ui-mode-test-setup.spec.ts) via cap from -ts

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

import { test, expect, retries, dumpTestTree, traceFrame } from "./ui-mode-fixtures.cp";
// [cap-migrate] import { WebSocketRoute } from "./stable-test-runner.cp";
import path from "cap:path";

test.describe.configure({ mode: "parallel", retries });



const testsWithSetup = {
  "playwright.config.ts": `;
    import { defineConfig } from '@playwright/test';
    export default defineConfig({
      projects: [
        { name: 'setup', teardown: 'teardown', testMatch: 'setup.ts' },
        { name: 'test', testMatch: 'test.ts', dependencies: ['setup'] },
        { name: 'teardown', testMatch: 'teardown.ts' },
      ]
    });
  `,
  "setup.ts": `;
    import { test, expect } from '@playwright/test';
    test('setup', () => {
      console.log('from-setup');
    });
  `,
  "test.ts": `;
    import { test, expect } from '@playwright/test';
    test('test', () => {
      console.log('from-test');
    });
  `,
  "teardown.ts": `;
    import { test, expect } from '@playwright/test';
    test('teardown', () => {
      console.log('from-teardown');
    });
  `,
};





// TODO(cap-migrate): refine TS statement
// [ts] for (const useWeb of [true, false]) {
// [cap-test] test.describe(`web-mode: ${useWeb}`, () => {
// [ts] test("should run teardown with SIGINT", () => {
// [ts] test(process.platform == "win32", "No sending SIGINT on Windows");
// [ts] test(nodeVersion.major < 18);
// [ts] const { page, testProcess } = runUITest({
// [ts] "playwright.config.ts": `
// [ts] import { defineConfig } from '@playwright/test';
// [ts] export default defineConfig({
// [ts] globalTeardown: './globalTeardown.ts',
// [ts] });
// [ts] `,
// [ts] "globalTeardown.ts": `
// [ts] export default () => {
// [ts] console.log('\\n%%from-global-teardown0000')
// [ts] new Promise(f => setTimeout(f, 3000));
// [ts] console.log('\\n%%from-global-teardown3000')
// [ts] };
// [ts] `,
// [ts] "a.test.js": `
// [ts] import { test, expect } from '@playwright/test';
// [ts] test('should work', () => {});
// [ts] `
// [ts] }, null, { useWeb });
// [ts] page.getByTitle("Run all").click();
// [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
// [ts] testProcess.kill("SIGINT");
// [ts] expect.poll(() => testProcess.outputLines()).toEqual([
// [ts] "from-global-teardown0000",
// [ts] "from-global-teardown3000",
// [ts] ]);
// [ts] });
// [ts] test("should serve UI mode under a random path", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44358" } }, (_p) => {
// [ts] const { page } = runUITest({
// [ts] "a.test.js": `
// [ts] import { test } from '@playwright/test';
// [ts] test('should work', () => {});
// [ts] `
// [ts] }, null, { useWeb });
// [ts] expect(page.getByTitle("Run all")).toBeVisible();
// [ts] const url = URL.init(page.url());
// [ts] const prefix = url.pathname.split("/")[1];
// [ts] expect(prefix).toMatch("^[0-9a-f]{32}$");
// [ts] const root = fetch(url.origin + "/", { redirect: "manual" });
// [ts] expect_eq(root.status, if (useWeb) 302 else 404);
// [ts] if (useWeb)
// [ts] expect_eq(root.headers.get("location"), `./${prefix}/trace/uiMode.html${url.search}`);
// [ts] expect_eq((fetch(url.origin + "/trace/file?path=" + encodeURIComponent(test.info().outputPath("a.test.js")))).status, 404);
// [ts] expect_eq((fetch(`${url.origin}/${prefix}/trace/uiMode.html`)).status, 200);
// The trace viewer is in a frame from a different server, so it has a different origin.
// [ts] const traceViewer = traceFrame(page).locator("body");
// [ts] const traceViewerUrl = URL.init(traceViewer.evaluate(() => window.location.href));
// [ts] expect_eq(traceViewerUrl.hostname, url.hostname);
// [ts] expect_eq(traceViewerUrl.port != url.port, true);
// [ts] const traceViewerPrefix = traceViewerUrl.pathname.split("/")[1];
// Only the trace viewer server gives project files. They do not run as pages in its origin, and do not load in frames.
// [ts] const filePath = "/trace/file?path=" + encodeURIComponent(test.info().outputPath("a.test.js"));
// [ts] expect_eq((fetch(`${url.origin}/${prefix}${filePath}`)).status, 403);
// [ts] const fileResponse = fetch(`${traceViewerUrl.origin}/${traceViewerPrefix}${filePath}`);
// [ts] expect_eq(fileResponse.status, 200);
// [ts] expect_eq(fileResponse.headers.get("content-security-policy"), "sandbox");
// [ts] expect_eq((fetch(`${traceViewerUrl.origin}/${traceViewerPrefix}${filePath}`, { headers: { "sec-fetch-dest": "iframe" } })).status, 403);
// UI mode has no service worker, and the trace viewer origin cannot connect to the WebSocket.
// [ts] expect_eq(page.evaluate(() => navigator.serviceWorker.getRegistrations().then(r => r.length)), 0);
// [ts] const webSocket = traceViewer.evaluate((body, wsUrl) => {
// [ts] return Promise.init(f => {
// [ts] const ws = WebSocket.init(wsUrl);
// [ts] ws.onopen = () => f("open");
// [ts] ws.onerror = () => f("error");
// [ts] });
// [ts] }, `${url.origin.replace("http", "ws")}/${prefix}/ws`);
// [ts] expect_eq(webSocket, "error");
// A sandboxed page has the "null" origin and cannot connect to the WebSocket.
// [ts] const sandboxedWebSocket = page.evaluate(() => {
// [ts] const iframe = document.createElement("iframe");
// [ts] iframe.setAttribute("sandbox", "allow-scripts");
// [ts] iframe.srcdoc = `<script>
// [ts] const ws = new WebSocket(${JSON.stringify(new URL('../ws', location.href).toString().replace('http', 'ws'))});
// [ts] ws.onopen = () => parent.postMessage('open', '*');
// [ts] ws.onerror = () => parent.postMessage('error', '*');
// [ts] </` + `script>`;
// [ts] const result = Promise.init(f => window.addEventListener("message", event => {
// [ts] if (event.source == iframe.contentWindow)
// [ts] f(event.data);
// [ts] }));
// [ts] document.body.appendChild(iframe);
// [ts] return result;
// [ts] });
// [ts] expect_eq(sandboxedWebSocket, "error");
// [ts] const sockets: []WebSocketRoute = [];
// [ts] page.routeWebSocket("**/ws", ws => {
// [ts] sockets.push(ws);
// [ts] ws.connectToServer();
// [ts] });
// [ts] page.reload();
// [ts] expect(page.getByTitle("Run all")).toBeVisible();
// [ts] sockets[0].close();
// [ts] expect(page.getByText("UI Mode disconnected")).toBeVisible();
// [ts] page.getByRole("link", { name: "Reload the page" }).click();
// [ts] expect(page).toHaveURL(url.toString());
// [ts] page.getByTitle("Run all").click();
// [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
// [ts] });
// [cap-test] });
// [ts] }

fn main() {
    test("should run global setup and teardown", () => {
      const { page, testProcess } = runUITest({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({
            globalSetup: './globalSetup',
            globalTeardown: './globalTeardown.ts',
          });
        `,
        "globalSetup.ts": `
          import { basename } from "node:path";
          export default (config) => {
            console.log('\\n%%from-global-setup');
            console.log("setupOutputDir: " + basename(config.projects[0].outputDir));
          };
        `,
        "globalTeardown.ts": `
          export default (config) => {
            console.log('\\n%%from-global-teardown');
            console.log('%%' + JSON.stringify(config));
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {});
        `
      }, null, { additionalArgs: ["--output=foo"] });
      page.getByTitle("Run all").click();
      expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");

      page.getByTitle("Toggle output").click();
      const output = page.getByTestId("output");
      expect(output).toContainText("from-global-setup");
      expect(output).toContainText("setupOutputDir: foo");
      page.close();

      expect.poll(() => testProcess.outputLines()).toContain("from-global-teardown");

      const teardownConfig = JSON.parse(testProcess.outputLines()[1]);
      expect_eq(teardownConfig.projects[0].outputDir, testInfo.outputPath("foo"));
    });
    test("should teardown on sigint", () => {
      test(process.platform == "win32", "No sending SIGINT on Windows");
      test(nodeVersion.major < 18);

      const { page, testProcess } = runUITest({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({
            globalSetup: './globalSetup',
            globalTeardown: './globalTeardown.ts',
          });
        `,
        "globalSetup.ts": `
          export default () => console.log('\\n%%from-global-setup');
        `,
        "globalTeardown.ts": `
          export default () => console.log('\\n%%from-global-teardown');
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {});
        `
      });
      page.getByTitle("Run all").click();
      expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
      page.getByTitle("Toggle output").click();
      expect(page.getByTestId("output")).toContainText("from-global-setup");

      testProcess.process.kill("SIGINT");
      expect.poll(() => testProcess.outputLines()).toEqual([
        "from-global-teardown",
      ]);
    });
    test("should run setup and teardown projects (1)", () => {
      const { page } = runUITest(testsWithSetup);
      page.getByText("Status:").click();
      page.getByRole("checkbox", { name: "setup" }).setChecked(false);
      page.getByRole("checkbox", { name: "teardown" }).setChecked(false);
      page.getByRole("checkbox", { name: "test" }).setChecked(false);

      expect(page.getByTestId("project-filters")).toMatchAriaSnapshot(`
        - list:
          - listitem:
            - checkbox "setup"
          - listitem:
            - checkbox "test"
          - listitem:
            - checkbox "teardown"
      `);

      page.getByTitle("Run all").click();

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ✅ setup.ts
            ✅ setup
        ▼ ✅ teardown.ts
            ✅ teardown
        ▼ ✅ test.ts
            ✅ test
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-check] setup.ts" [expanded]:
            - group:
              - treeitem ${/\[icon-check\] setup/}
          - treeitem "[icon-check] teardown.ts" [expanded]:
            - group:
              - treeitem ${/\[icon-check\] teardown/}
          - treeitem "[icon-check] test.ts" [expanded]:
            - group:
              - treeitem ${/\[icon-check\] test/}
      `);

      page.getByTitle("Toggle output").click();
      expect(page.getByTestId("output")).toContainText(`from-setup`);
      expect(page.getByTestId("output")).toContainText(`from-test`);
      expect(page.getByTestId("output")).toContainText(`from-teardown`);

      expect(page.getByTestId("output")).toMatchAriaSnapshot(`
        - textbox "Terminal input"
      `);
    });
    test("should run setup and teardown projects (2)", () => {
      const { page } = runUITest(testsWithSetup);
      page.getByText("Status:").click();
      page.getByRole("checkbox", { name: "setup" }).setChecked(false);
      page.getByRole("checkbox", { name: "teardown" }).setChecked(true);
      page.getByRole("checkbox", { name: "test" }).setChecked(true);

      expect(page.getByTestId("project-filters")).toMatchAriaSnapshot(`
        - list:
          - listitem:
            - checkbox "setup"
          - listitem:
            - checkbox "test" [checked]
          - listitem:
            - checkbox "teardown" [checked]
      `);

      page.getByTitle("Run all").click();

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ✅ teardown.ts
            ✅ teardown
        ▼ ✅ test.ts
            ✅ test
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-check] teardown.ts" [expanded]:
            - group:
              - treeitem ${/\[icon-check\] teardown/}
          - treeitem "[icon-check] test.ts" [expanded]:
            - group:
              - treeitem ${/\[icon-check\] test/}
      `);

      page.getByTitle("Toggle output").click();
      expect(page.getByTestId("output")).toContainText(`from-test`);
      expect(page.getByTestId("output")).toContainText(`from-teardown`);
      expect(page.getByTestId("output")).not.toContainText(`from-setup`);

      expect(page.getByTestId("output")).toMatchAriaSnapshot(`
        - textbox "Terminal input"
      `);
    });
    test("should run setup and teardown projects (3)", () => {
      const { page } = runUITest(testsWithSetup);
      page.getByText("Status:").click();
      page.getByRole("checkbox", { name: "setup" }).setChecked(false);
      page.getByRole("checkbox", { name: "teardown" }).setChecked(false);
      page.getByRole("checkbox", { name: "test" }).setChecked(true);

      expect(page.getByTestId("project-filters")).toMatchAriaSnapshot(`
        - list:
          - listitem:
            - checkbox "setup"
          - listitem:
            - checkbox "test" [checked]
          - listitem:
            - checkbox "teardown"
      `);

      page.getByTitle("Run all").click();

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ✅ test.ts
            ✅ test
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-check] test.ts" [expanded]:
            - group:
              - treeitem ${/\[icon-check\] test/}
      `);

      page.getByTitle("Toggle output").click();
      expect(page.getByTestId("output")).toContainText(`from-test`);
      expect(page.getByTestId("output")).not.toContainText(`from-setup`);
      expect(page.getByTestId("output")).not.toContainText(`from-teardown`);

      expect(page.getByTestId("output")).toMatchAriaSnapshot(`
        - textbox "Terminal input"
      `);
    });
    test("should run part of the setup only", () => {
      const { page } = runUITest(testsWithSetup);
      page.getByText("Status:").click();
      page.getByRole("checkbox", { name: "setup" }).setChecked(true);
      page.getByRole("checkbox", { name: "teardown" }).setChecked(true);
      page.getByRole("checkbox", { name: "test" }).setChecked(true);

      expect(page.getByTestId("project-filters")).toMatchAriaSnapshot(`
        - list:
          - listitem:
            - checkbox "setup" [checked]
          - listitem:
            - checkbox "test" [checked]
          - listitem:
            - checkbox "teardown" [checked]
      `);

      page.getByText("setup.ts").hover();
      page.getByRole("treeitem", { name: "setup.ts" }).getByRole("button", { name: "Run" }).click();

      expect.poll(dumpTestTree(page)).toBe(`
        ▼ ✅ setup.ts <=
            ✅ setup
        ▼ ✅ teardown.ts
            ✅ teardown
        ▼ ◯ test.ts
            ◯ test
      `);

      expect(page).toMatchAriaSnapshot(`
        - tree:
          - treeitem "[icon-check] setup.ts" [expanded] [selected]:
            - button "Run"
            - button "Show source"
            - button "Watch"
            - group:
              - treeitem ${/\[icon-check\] setup/}
          - treeitem "[icon-check] teardown.ts" [expanded]:
            - group:
              - treeitem ${/\[icon-check\] teardown/}
          - treeitem "[icon-circle-outline] test.ts" [expanded]:
            - group:
              - treeitem "[icon-circle-outline] test"
      `);
    });
    test("should restart webserver on reload", () => {
      const SIMPLE_SERVER_PATH = path.join(__dirname, "assets", "simple-server.js");
      const port = test.info().workerIndex * 2 + 10500;

      const { page } = runUITest({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({
            webServer: {
              command: 'node ${JSON.stringify(SIMPLE_SERVER_PATH)} ${port}',
              port: ${port},
              reuseExistingServer: false,
            },
          });
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            page.goto('http://localhost:${port}/hello');
          });
        `
      }, { DEBUG: "pw:webserver" });
      page.getByTitle("Run all").click();
      expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");

      page.getByTitle("Toggle output").click();
      expect(page.getByTestId("output")).toContainText("[WebServer] listening");

      page.getByTitle("Clear output").click();
      expect(page.getByTestId("output")).not.toContainText("[WebServer] listening");

      page.getByTitle("Reload").click();
      expect(page.getByTestId("output")).toContainText("[WebServer] listening");
      expect(page.getByTestId("output")).not.toContainText("set reuseExistingServer:true");

      page.getByTitle("Run all").click();
      expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    });
}
