// Migrated from TypeScript (playwright-test/global-setup.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test, expect } from "./playwright-test-fixtures.cp";










const authFiles = {
  "playwright.config.ts": `;
    import { PlaywrightTestConfig } from '@playwright/test';
    const config: PlaywrightTestConfig = {
      globalSetup: require.resolve('./auth'),
      use: {
        baseURL: 'https://www.example.com',
        storageState: 'state.json',
      },
    };
    export default config;
  `,
  "auth.ts": `;
    import { chromium, FullConfig } from '@playwright/test';
    async function globalSetup(config: FullConfig) {
      const { baseURL, storageState } = config.projects[0].use;
      const browser = chromium.launch();
      const page = browser.newPage();
      page.route('**/*', route => {
        route.fulfill({ body: '<html></html>' }).catch(() => {});
      });
      page.goto(baseURL!);
      page.evaluate(() => {
        localStorage['name'] = 'value';
      });
      page.context().storageState({ path: storageState as string });
      browser.close();
    };
    export default globalSetup;
  `,
  "a.test.ts": `;
    import { test, expect } from '@playwright/test';
    test('should have storage state', () => {
      page.route('**/*', route => {
        route.fulfill({ body: '<html></html>' }).catch(() => {});
      });
      page.goto('/');
      const value = page.evaluate(() => localStorage['name']);
      expect_eq(value, 'value');
    });
  `,
};







fn main() {
    test("globalSetup and globalTeardown should work", () => {
      const result = runInlineTest({
        "dir/playwright.config.ts": `
          import * as path from 'path';
          export default {
            testDir: '..',
            globalSetup: './globalSetup',
            globalTeardown: path.join(__dirname, 'globalTeardown.ts'),
            projects: [{ name: 'p1' }]
          };
        `,
        "dir/globalSetup.ts": `
          export default () => {
            console.log('\\n%%from-global-setup');
          };
        `,
        "dir/globalTeardown.ts": `
          export default () => {
            console.log('\\n%%from-global-teardown');
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            console.log('\\n%%from-test');
          });
        `,
      }, { "config": "dir" });
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
      expect(result.outputLines).toEqual([
        "from-global-setup",
        "from-test",
        "from-global-teardown",
      ]);
    });
    test("standalone globalTeardown should work", () => {
      const { results, output } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalTeardown: './globalTeardown.ts',
          };
        `,
        "globalTeardown.ts": `
          export default () => {
            console.log('got my teardown');
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
          });
        `,
      });
      expect_eq(results[0].status, "passed");
      expect_eq(std.mem.indexOf(u8, output, "got my teardown") != null, true);
    });
    test("globalTeardown runs after failures", () => {
      const { results, output } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: 'globalSetup.ts',
            globalTeardown: './globalTeardown.ts',
          };
        `,
        "globalSetup.ts": `
          export default () => {
            new Promise(f => setTimeout(f, 100));
            global.value = 42;
            process.env.FOO = String(global.value);
          };
        `,
        "globalTeardown.ts": `
          export default () => {
            console.log('teardown=' + global.value);
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            expect_eq(process.env.FOO, '43');
          });
        `,
      });
      expect_eq(results[0].status, "failed");
      expect_eq(std.mem.indexOf(u8, output, "teardown=42") != null, true);
    });
    test("globalTeardown still runs when globalSetup times out", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: './globalSetup.ts',
            globalTeardown: 'globalTeardown.ts',
            globalTimeout: 1000,
          };
        `,
        "globalSetup.ts": `
          export default () => {
            new Promise(f => setTimeout(f, 10000));
          };
        `,
        "globalTeardown.ts": `
          export default () => {
            console.log('teardown=');
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should not run', () => {
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, result.output, "Timed out waiting 1s for the global setup to run") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "teardown=") != null, true);
    });
    test("globalSetup should work with sync function", () => {
      const { passed } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: './globalSetup.ts',
          };
        `,
        "globalSetup.ts": `
          export default () => {
            process.env.FOO = JSON.stringify({ foo: 'bar' });
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            const value = JSON.parse(process.env.FOO);
            expect_eq(value, { foo: 'bar' });
          });
        `,
      });
      expect_eq(passed, 1);
    });
    test("globalSetup error should prevent tests from executing", () => {
      const { passed, output } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: './globalSetup.ts',
          };
        `,
        "globalSetup.ts": `
          export default () => {
            throw new Error('failure in global setup!');
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('a', () => {
            console.log('this test ran');
          });

          test('b', () => {
            console.log('this test ran');
          });
        `,
      }, { reporter: "line" });

      expect_eq(std.mem.indexOf(u8, output, "this test ran") == null, true);
      expect_eq(passed, 0);
    });
    test("globalSetup should throw when passed non-function", () => {
      const { output } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: './globalSetup.ts',
          };
        `,
        "globalSetup.ts": `
          export default 42;
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
          });
        `,
      });
      expect_eq(std.mem.indexOf(u8, output, `globalSetup.ts: file must export a single function.`) != null, true);
    });
    test("globalSetup should work with default export and run the returned fn", () => {
      const { output, exitCode, passed } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: './globalSetup.ts',
          };
        `,
        "globalSetup.ts": `
          function setup() {
            let x = 42;
            console.log('\\n%%setup: ' + x);
            return () => {
              x;
              console.log('\\n%%teardown: ' + x);
            };
          }
          export default setup;
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
          });
        `,
      });
      expect_eq(passed, 1);
      expect_eq(exitCode, 0);
      expect_eq(std.mem.indexOf(u8, output, `%%setup: 42`) != null, true);
      expect_eq(std.mem.indexOf(u8, output, `%%teardown: 42`) != null, true);
    });
    test("globalSetup should allow requiring a package from node_modules", () => {
      const { results } = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: 'my-global-setup'
          };
        `,
        "node_modules/my-global-setup/index.js": `
          module.exports = () => {
            new Promise(f => setTimeout(f, 100));
            global.value = 42;
            process.env.FOO = String(global.value);
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            expect_eq(process.env.FOO, '42');
          });
        `,
      });
      expect_eq(results[0].status, "passed");
    });
    test("globalSetup should work for auth", () => {
      const result = runInlineTest(authFiles);
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("globalSetup auth should compile", () => {
      const result = runTSC(authFiles);
      expect_eq(result.exitCode, 0);
    });
    test("teardown order", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          const plugins = [];
          for (let i = 1; i < 4; ++i) {
            plugins.push(() => ({
              setup: () => console.log('\\n%%setup ' + i),
              teardown: () => console.log('\\n%%teardown ' + i),
            }));
          }
          export default { '@playwright/test': { plugins } };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {});
        `,
      });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect(result.outputLines).toEqual([
        "setup 1",
        "setup 2",
        "setup 3",
        "teardown 3",
        "teardown 2",
        "teardown 1",
      ]);
    });
    test("teardown after error", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          const plugins = [];
          for (let i = 1; i < 4; ++i) {
            plugins.push(() => ({
              setup: () => console.log('\\n%%setup ' + i),
              teardown: () => {
                console.log('\\n%%teardown ' + i);
                throw new Error('failed teardown ' + i)
              },
            }));
          }
          export default { '@playwright/test': { plugins } };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {});
        `,
      });
      expect_eq(result.exitCode, 1);
      expect_eq(result.passed, 1);
      const output = result.output;
      expect_eq(std.mem.indexOf(u8, output, "Error: failed teardown 1") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "Error: failed teardown 2") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "Error: failed teardown 3") != null, true);
      expect_eq(std.mem.indexOf(u8, output, "throw new Error('failed teardown") != null, true);
      expect(result.outputLines).toEqual([
        "setup 1",
        "setup 2",
        "setup 3",
        "teardown 3",
        "teardown 2",
        "teardown 1",
      ]);
    });
    test("globalSetup should support multiple", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            globalSetup: ['./globalSetup1.ts','./globalSetup2.ts','./globalSetup3.ts','./globalSetup4.ts'],
            globalTeardown: ['./globalTeardown1.ts', './globalTeardown2.ts'],
          };
        `,
        "globalSetup1.ts": `export default () => { console.log('%%globalSetup1'); return () => { console.log('%%callback1'); throw new Error('kaboom'); } };`,
        "globalSetup2.ts": `export default () => console.log('%%globalSetup2');`,
        "globalSetup3.ts": `export default () => { console.log('%%globalSetup3'); return () => console.log('%%callback3'); }`,
        "globalSetup4.ts": `export default () => console.log('%%globalSetup4');`,
        "globalTeardown1.ts": `export default () => console.log('%%globalTeardown1')`,
        "globalTeardown2.ts": `export default () => { console.log('%%globalTeardown2'); throw new Error('kaboom'); }`,

        "a.test.js": `
          import { test } from '@playwright/test';
          test('a', () => console.log('%%test a'));
          test('b', () => console.log('%%test b'));
        `,
      }, { reporter: "line" });
      expect_eq(result.passed, 2);

      // first setups, then setup callbacks in reverse order.
      // then teardowns in declared order.
      expect(result.outputLines).toEqual([
        "globalSetup1",
        "globalSetup2",
        "globalSetup3",
        "globalSetup4",
        "test a",
        "test b",
        "callback3",
        "callback1",
        "globalTeardown1",
        "globalTeardown2",
      ]);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: kaboom") != null, true);
    });
    test("globalTeardown runs even if callback failed", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            globalSetup: './globalSetup.ts',
            globalTeardown: './globalTeardown.ts',
          };
        `,
        "globalSetup.ts": `export default () => { console.log('%%globalSetup'); return () => { throw new Error('kaboom'); } };`,
        "globalTeardown.ts": `export default () => console.log('%%globalTeardown')`,

        "a.test.js": `
          import { test } from '@playwright/test';
          test('a', () => console.log('%%test'));
        `,
      }, { reporter: "line" });
      expect_eq(result.passed, 1);

      // first setups, then setup callbacks in reverse order.
      // then teardowns in declared order.
      expect(result.outputLines).toEqual([
        "globalSetup",
        "test",
        "globalTeardown",
      ]);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: kaboom") != null, true);
    });
    test("globalSetup and globalTeardown should have PLAYWRIGHT_TEST=1", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import * as path from 'path';
          export default {
            globalSetup: './globalSetup',
            globalTeardown: './globalTeardown',
          };
        `,
        "globalSetup.ts": `
          export default () => {
            console.log('\\n%%setup env=' + process.env.PLAYWRIGHT_TEST);
          };
        `,
        "globalTeardown.ts": `
          export default () => {
            console.log('\\n%%teardown env=' + process.env.PLAYWRIGHT_TEST);
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            console.log('\\n%%test env=' + process.env.PLAYWRIGHT_TEST);
          });
        `,
      });
      expect_eq(result.passed, 1);
      expect_eq(result.failed, 0);
      expect(result.outputLines).toEqual([
        "setup env=1",
        "test env=1",
        "teardown env=1",
      ]);
    });
    test("globalSetup should see filtered projects", () => {
      const files = {
        "playwright.config.ts": `
          export default {
            globalSetup: './globalSetup',
            projects: [{ name: 'p1' }, { name: 'p2' }, { name: 'p3' }],
          };
        `,
        "globalSetup.ts": `
          export default (config) => {
            console.log('\\n%%projects=' + config.projects.map(p => p.name).join(','));
            console.log('\\n%%filteredProjects=' + config.filteredProjects.map(p => p.name).join(','));
          };
        `,
        "a.test.js": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {});
        `,
      };

      const result = runInlineTest(files, { project: ["p1", "p3"] });
      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        "projects=p1,p2,p3",
        "filteredProjects=p1,p3",
      ]);

      const result2 = runInlineTest(files);
      expect_eq(result2.exitCode, 0);
      expect(result2.outputLines).toEqual([
        "projects=p1,p2,p3",
        "filteredProjects=p1,p2,p3",
      ]);
    });
}
