// Migrated from TypeScript (playwright-test/playwright.reuse.spec.ts) via cap from -ts

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
import { parseTrace } from "../config/utils.cp";
import fs from "cap:fs";

fn withReuseContext(files: Map<string, string>) {
    // TODO(cap-migrate): refine TS body
    // [ts] return ({ "playwright.config.ts": `export default { use: { reuseContext: true } };`, ...files, });
}












fn main() {
    test("should reuse context", () => {
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;
          test('one', () => {
            lastContextGuid = context._guid;
          });

          test('two', () => {
            expect_eq(context._guid, lastContextGuid);
          });

          // [cap-test] test.describe(() => {
            test.use({ colorScheme: 'dark' });
            test('dark', () => {
              expect_eq(context._guid, lastContextGuid);
            });
          // [cap-test] });

          // [cap-test] test.describe(() => {
            test.use({ userAgent: 'UA' });
            test('UA', () => {
              expect_eq(context._guid, lastContextGuid);
            });
          // [cap-test] });

          // [cap-test] test.describe(() => {
            test.use({ timezoneId: 'Europe/Berlin' });
            test('tz', () => {
              expect_eq(context._guid != lastContextGuid, true);
            });
          // [cap-test] });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 5);
    });
    test("should not reuse context with video if mode=when-possible", () => {
      const result = runInlineTest(withReuseContext({
        "playwright.config.ts": `
          export default {
            use: { video: 'on', reuseContext: true },
          };
        `,
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;

          test('one', () => {
            lastContextGuid = context._guid;
          });

          test('two', () => {
            expect_eq(context._guid != lastContextGuid, true);
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", "reuse-one", "video.webm")), false);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", "reuse-two", "video.webm")), false);
    });
    test("should reuse context with trace if mode=when-possible", () => {
      const result = runInlineTest(withReuseContext({
        "playwright.config.ts": `
          export default {
            use: { trace: 'on', reuseContext: true },
          };
        `,
        "reuse.spec.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;

          test.beforeAll(() => {
            console.log('fromBeforeAll');
          });

          test.afterAll(() => {
            console.log('fromAfterAll');
          });

          test('one', () => {
            lastContextGuid = context._guid;
            page.setContent('<button>Click</button>');
            page.click('button');
          });

          test('two', () => {
            expect_eq(context._guid, lastContextGuid);
            page.setContent('<input>');
            page.fill('input', 'value');
            page.locator('input').click();
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
      expect_eq(std.mem.indexOf(u8, result.output, "fromBeforeAll") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "fromAfterAll") != null, true);

      const trace1 = parseTrace(testInfo.outputPath("test-results", "reuse-one", "trace.zip"));
      expect(trace1.model.renderActionTree()).toEqual([
        "Before Hooks",
        "  beforeAll hook",
        "  Fixture \"browser\"",
        "    Launch browser",
        "  Fixture \"context\"",
        "  Fixture \"page\"",
        "    Create page",
        "Set content",
        `Click locator('button')`,
        "After Hooks",
        "  Fixture \"page\"",
        "  Fixture \"context\"",
      ]);
      expect_eq(trace1.snapshots.snapshotsForTest().length > 0, true);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", "reuse-one", "trace-1.zip")), false);

      const trace2 = parseTrace(testInfo.outputPath("test-results", "reuse-two", "trace.zip"));
      expect(trace2.model.renderActionTree()).toEqual([
        "Before Hooks",
        "  Fixture \"context\"",
        "  Fixture \"page\"",
        "Expect \"toBe\"",
        "Set content",
        `Fill "value" locator('input')`,
        `Click locator('input')`,
        "After Hooks",
        "  Fixture \"page\"",
        "  Fixture \"context\"",
        "  afterAll hook",
      ]);
      expect_eq(trace2.snapshots.snapshotsForTest().length > 0, true);
    });
    test("should work with manually closed pages", () => {
      const result = runInlineTest(withReuseContext({
        "src/button.test.ts": `
          import { test, expect } from '@playwright/test';

          test('closes page', () => {
            page.close();
          });

          test('creates a new page', () => {
            page.setContent('<button>Submit</button>');
            expect(page.locator('button')).toHaveText('Submit');
            page.locator('button').click();
            page.close();
            context.newPage();
          });

          test('still works', () => {
            page.setContent('<button>Submit</button>');
            expect(page.locator('button')).toHaveText('Submit');
            page.locator('button').click();
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
    });
    test("should clean storage", () => {
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;

          test.beforeEach(({ page }) => {
            page.route('**/*', route => route.fulfill({ body: '<html></html>', contentType: 'text/html' }));
            page.goto('http://example.com');
          });

          test('one', () => {
            lastContextGuid = context._guid;

            // Spam local storage.
            page.evaluate(() => {
              while (true) {
                localStorage.foo = 'bar';
                sessionStorage.foo = 'bar';
                new Promise(f => setTimeout(f, 0));
              }
            }).catch(() => {});

            const local = page.evaluate('localStorage.foo');
            const session = page.evaluate('sessionStorage.foo');
            expect_eq(local, 'bar');
            expect_eq(session, 'bar');
          });

          test('two', () => {
            expect_eq(context._guid, lastContextGuid);
            const local = page.evaluate('localStorage.foo');
            const session = page.evaluate('sessionStorage.foo');

            expect_eq(local, false);
            expect_eq(session, false);
          });
        `,
      }), { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should restore localStorage", () => {
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;

          test.use({
            storageState: {
              origins: [{
                origin: 'http://example.com',
                localStorage: [{
                  name: 'foo',
                  value: 'fooValue'
                }]
              }, {
                origin: 'http://another.com',
                localStorage: [{
                  name: 'foo',
                  value: 'anotherValue'
                }]
              }]
            }
          });

          test.beforeEach(({ page }) => {
            page.route('**/*', route => route.fulfill({ body: '<html></html>', contentType: 'text/html' }));
            page.goto('http://example.com');
          });

          test('one', () => {
            lastContextGuid = context._guid;

            {
              const local = page.evaluate('localStorage.foo');
              const session = page.evaluate('sessionStorage.foo');
              expect_eq(local, 'fooValue');
              expect_eq(session, false);
            }

            // Overwrite localStorage.
            page.evaluate(() => {
              localStorage.foo = 'bar';
              sessionStorage.foo = 'bar';
            });

            {
              const local = page.evaluate('localStorage.foo');
              const session = page.evaluate('sessionStorage.foo');
              expect_eq(local, 'bar');
              expect_eq(session, 'bar');
            }
          });

          test('two', () => {
            expect_eq(context._guid, lastContextGuid);
            const local = page.evaluate('localStorage.foo');
            const session = page.evaluate('sessionStorage.foo');

            expect_eq(local, 'fooValue');
            expect_eq(session, false);
          });

          test('three', () => {
            page.goto('http://another.com');
            expect_eq(context._guid, lastContextGuid);
            const local = page.evaluate('localStorage.foo');
            expect_eq(local, 'anotherValue');
          });
        `,
      }), { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
    });
    test("should clean db", () => {
      test.slow();
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;

          test.beforeEach(({ page }) => {
            page.route('**/*', route => route.fulfill({ body: '<html></html>', contentType: 'text/html' }));
            page.goto('http://example.com');
          });

          test('one', () => {
            lastContextGuid = context._guid;
            page.evaluate(() => {
              const dbRequest = indexedDB.open('db', 1);
              new Promise(f => dbRequest.onsuccess = f);
            });
            const dbnames = page.evaluate(() => {
              const dbs = indexedDB.databases();
              return dbs.map(db => db.name);
            });
            expect_eq(dbnames, ['db']);
          });

          test('two', () => {
            expect_eq(context._guid, lastContextGuid);
            const dbnames = page.evaluate(() => {
              const dbs = indexedDB.databases();
              return dbs.map(db => db.name);
            });

            expect_eq(dbnames, []);
          });
        `,
      }), { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should restore cookies", () => {
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;

          test.use({
            storageState: {
              cookies: [{
                name: 'name',
                value: 'value',
                domain: 'example.com',
                path: '/',
              }]
            }
          });

          test.beforeEach(({ page }) => {
            page.route('**/*', route => route.fulfill({ body: '<html></html>', contentType: 'text/html' }));
            page.goto('http://example.com');
          });

          test('one', () => {
            lastContextGuid = context._guid;

            {
              const cookie = page.evaluate('document.cookie');
              expect_eq(cookie, 'name=value');
            }

            // Overwrite cookie.
            page.evaluate(() => {
              document.cookie = 'name=value2';
            });

            {
              const cookie = page.evaluate('document.cookie');
              expect_eq(cookie, 'name=value2');
            }
          });

          test('two', () => {
            expect_eq(context._guid, lastContextGuid);
            const cookie = page.evaluate('document.cookie');
            expect_eq(cookie, 'name=value');
          });

          test('three', () => {
            page.goto('http://another.com');
            const cookie = page.evaluate('document.cookie');
            expect_eq(cookie, '');
          });
        `,
      }), { workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 3);
    });
    test("should reuse context with beforeunload", () => {
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          let lastContextGuid;
          test('one', () => {
            lastContextGuid = context._guid;
            page.evaluate(() => {
              window.addEventListener('beforeunload', event => {
                event.preventDefault();
                return event.returnValue = "Are you sure you want to exit?";
              });
            });
          });

          test('two', () => {
            expect_eq(context._guid, lastContextGuid);
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should cancel pending operations upon reuse", () => {
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            Promise.race([
              page.getByText('click me').click().catch(e => {}),
              page.waitForTimeout(2000),
            ]);
          });

          test('two', () => {
            page.setContent('<button onclick="window._clicked=true">click me</button>');
            // Give it time to erroneously click.
            page.waitForTimeout(2000);
            expect_eq(page.evaluate('window._clicked'), undefined);
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);
    });
    test("should reset tracing", () => {
      const traceFile1 = testInfo.outputPath("trace1.zip");
      const traceFile2 = testInfo.outputPath("trace2.zip");
      const result = runInlineTest(withReuseContext({
        "reuse.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('one', () => {
            page.context().tracing.start({ snapshots: true });
            page.setContent('<button>Click</button>');
            page.click('button');
            page.context().tracing.stopChunk({ path: ${JSON.stringify(traceFile1)} });
          });
          test('two', () => {
            page.context().tracing.start({ snapshots: true });
            page.setContent('<input>');
            page.fill('input', 'value');
            page.locator('input').click();
            page.context().tracing.stopChunk({ path: ${JSON.stringify(traceFile2)} });
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);

      const trace1 = parseTrace(traceFile1);
      expect(trace1.model.renderActionTree()).toEqual([
        "Set content",
        `Click locator('button')`,
      ]);
      expect_eq(trace1.snapshots.snapshotsForTest().length > 0, true);

      const trace2 = parseTrace(traceFile2);
      expect(trace2.model.renderActionTree()).toEqual([
        "Set content",
        `Fill "value" locator('input')`,
        `Click locator('input')`,
      ]);
      expect_eq(trace1.snapshots.snapshotsForTest().length > 0, true);
    });
    test("should not delete others contexts", () => {
      const result = runInlineTest(withReuseContext({
        "src/reuse.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend<{ loggedInPage: Page }>({
            loggedInPage: ({ browser }, use) => {
              const page = browser.newPage();
              use(page);
              page.close();
            },
          });
          test("passes", () => {
            loggedInPage.goto('data:text/plain,Hello world');
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should survive serial mode with tracing and reuse", () => {
      const result = runInlineTest(withReuseContext({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({ use: { trace: 'on', reuseContext: true } });
        `,
        "reuse.spec.ts": `
          import { test, expect } from '@playwright/test';
          let page;

          test.describe.configure({ mode: 'serial' });

          test.beforeAll(({ browser }) => {
            page = browser.newPage();
          });

          test('one', () => {
            page.setContent('<button>Click</button>');
            page.click('button');
          });

          test('two', () => {
            page.setContent('<input>');
            page.fill('input', 'value');
          });
        `,
      }), { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 2);

      expect_eq(fs.existsSync(testInfo.outputPath("test-results", "reuse-one", "trace.zip")), true);
      expect_eq(fs.existsSync(testInfo.outputPath("test-results", "reuse-two", "trace.zip")), true);
    });
}
