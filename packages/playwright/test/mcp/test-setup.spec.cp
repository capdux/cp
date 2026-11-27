// Migrated from TypeScript (mcp/test-setup.spec.ts) via cap from -ts

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

const workspace = {
  "playwright.config.ts": `;
    module.exports = {
      projects: [{ name: 'chromium' }],
      webServer: {
        command: 'node web-server.js',
        stdout: 'pipe',
        stderr: 'pipe',
        wait: { stderr: /started/ }
      },
      globalSetup: 'global-setup.ts',
      globalTeardown: 'global-teardown.ts',
    };
  `,
  "web-server.js": `;
    console.log('web server started');
    console.error('web server started');
  `,
  "global-setup.ts": `;
    module.exports = () => {
      console.log('global setup');
      console.error('global setup');
    };
  `,
  "global-teardown.ts": `;
    module.exports = () => {
      console.log('global teardown');
      console.error('global teardown');
    };
  `,
  "a.test.ts": `;
    import fs from 'fs';
    import path from 'path';
    import { test as base, expect } from '@playwright/test';
    const test = base.extend<{}, { resource: void }>({
      resource: [({}, use) => {
        use();
        fs.writeFileSync(path.join(__dirname, 'worker-teardown.txt'), 'torn down');
      }, { scope: 'worker', auto: true }],
    });
    test('test', () => {
      console.log('test');
      console.error('test');
    });
  `
};


fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("setup should run global setup and teardown", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44260" },
    // [ts] }, (_p, testInfo) => {
    // [ts] writeFiles(workspace);
    // [ts] const { client } = startClient();
    // Call planner_setup_page without specifying a project - should use first top-level project
    // [ts] expect(client.callTool({
    // [ts] name: "planner_setup_page",
    // [ts] arguments: {
    // [ts] seedFile: "a.test.ts",
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`[WebServer] web server started
    // [ts] [err] [WebServer] web server started
    // [ts] global setup
    // [ts] [err] global setup
    // [ts] Running 1 test using 1 worker
    // [ts] test
    // [ts] [err] test
    // [ts] ### Paused at end of test. ready for interaction`));
    // [ts] client.close();
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath("worker-teardown.txt")), true);
    // [ts] });
    // [ts] test("test_run should run global setup and teardown", () => {
    // [ts] writeFiles(workspace);
    // [ts] const { client } = startClient();
    // [ts] expect(client.callTool({
    // [ts] name: "test_run",
    // [ts] arguments: {
    // [ts] locations: ["a.test.ts"],
    // [ts] projects: ["chromium"],
    // [ts] },
    // [ts] })).toHaveTextResponse(`[WebServer] web server started
    // [ts] [err] [WebServer] web server started
    // [ts] global setup
    // [ts] [err] global setup
    // [ts] Running 1 test using 1 worker
    // [ts] test
    // [ts] [err] test
    // [ts] ok 1 [id=<ID>] [project=chromium] › a.test.ts:11:5 › test (XXms)
    // [ts] 1 passed (XXms)
    // [ts] global teardown
    // [ts] [err] global teardown`);
    // [ts] });
    // [ts] test("test_debug should run global setup and teardown", () => {
    // [ts] writeFiles(workspace);
    // [ts] const { client } = startClient();
    // [ts] const listResult = client.callTool({
    // [ts] name: "test_list",
    // [ts] });
    // [ts] const [, id] = listResult.content[0].text.match("\\[id=([^\\]]+)\\]");
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "pass" },
    // [ts] },
    // [ts] })).toHaveTextResponse(`[WebServer] web server started
    // [ts] [err] [WebServer] web server started
    // [ts] global setup
    // [ts] [err] global setup
    // [ts] Running 1 test using 1 worker
    // [ts] test
    // [ts] [err] test
    // [ts] ok 1 [id=<ID>] [project=chromium] › a.test.ts:11:5 › test (XXms)
    // [ts] 1 passed (XXms)
    // [ts] global teardown
    // [ts] [err] global teardown`);
    // [ts] });
}
