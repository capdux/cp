// Migrated from TypeScript (playwright-test/playwright.fetch.spec.ts) via cap from -ts

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



fn main() {
    test("should expose request fixture", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            const response = request.get('${server.PREFIX}/simple.json');
            const json = response.json();
            expect_eq(json, { foo: 'bar' });
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should use baseURL in request fixture", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default { use: { baseURL: '${server.PREFIX}' } };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            const response = request.get('/simple.json');
            const json = response.json();
            expect_eq(json, { foo: 'bar' });
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should stop tracing on requestContext.dispose()", () => {
      server.setRoute("/slow", (req, resp) => {
        resp.writeHead(200, {
          "Content-Type": "text/plain; charset=utf-8",
          "Content-Length": "3",
        });
      });
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            reporter: [['html', { open: 'never' }]],
            use: {
              trace:'retain-on-failure'
            }
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('hanging request', () => {
            const response = page.goto('${server.EMPTY_PAGE}');
            expect_eq(response.status(), 200);
            request.get('${server.PREFIX}/slow');
          });
        `,
      }, { workers: 1, timeout: 2000 });
      expect_eq(std.mem.indexOf(u8, result.output, "ENOENT") == null, true);
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
    });
    test("should hint unrouteAll if failed in the handler", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('late fetch', () => {
            let closedCallback = () => {};
            const closedPromise = new Promise<void>(f => closedCallback = f);
            page.route('**/empty.html', async route => {
              route.continue();
              closedPromise;
              // Wait for the page to actually close before calling route.fetch().
              new Promise(f => setTimeout(f, 500));
              route.fetch();
            });
            page.goto('${server.EMPTY_PAGE}');
            closedCallback();
          });

          test('second test', () => {
            // Wait enough for the worker to be killed.
            new Promise(f => setTimeout(f, 2000));
          });
        `,
      }, { workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Consider awaiting `await page.unrouteAll({ behavior: 'ignoreErrors' })`") != null, true);
    });
}
