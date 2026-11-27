// Migrated from TypeScript (playwright-test/playwright.config.spec.ts) via cap from -ts

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
    test("should fall back to launchOptions", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
              launchOptions: {
                headless: false,
                channel: 'chrome',
              }
            }
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect.soft(headless).toBe(false);
            expect.soft(channel).toBe('chrome');
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should override launchOptions", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
              headless: false,
              channel: 'chrome',
              launchOptions: {
                headless: true,
                channel: 'msedge',
              }
            }
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect.soft(headless).toBe(false);
            expect.soft(channel).toBe('chrome');
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should respect contextOptions", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
              contextOptions: {
                acceptDownloads: false,
                bypassCSP: true,
                colorScheme: 'dark',
                deviceScaleFactor: 2,
                extraHTTPHeaders: {'foo': 'bar'},
                hasTouch: true,
                ignoreHTTPSErrors: true,
                isMobile: true,
                javaScriptEnabled: true,
                locale: 'fr-FR',
                locateOnlyVisible: true,
                offline: true,
                permissions: ['geolocation'],
                timezoneId: 'TIMEZONE',
                userAgent: 'UA',
                viewport: null
              }
            }
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect.soft(acceptDownloads).toBe(false);
            expect.soft(bypassCSP).toBe(true);
            expect.soft(colorScheme).toBe('dark');
            expect.soft(deviceScaleFactor).toBe(2);
            expect.soft(extraHTTPHeaders).toEqual({'foo': 'bar'});
            expect.soft(hasTouch).toBe(true);
            expect.soft(ignoreHTTPSErrors).toBe(true);
            expect.soft(isMobile).toBe(true);
            expect.soft(javaScriptEnabled).toBe(true);
            expect.soft(locale).toBe('fr-FR');
            expect.soft(locateOnlyVisible).toBe(true);
            expect.soft(offline).toBe(true);
            expect.soft(permissions).toEqual(['geolocation']);
            expect.soft(timezoneId).toBe('TIMEZONE');
            expect.soft(userAgent).toBe('UA');
            expect.soft(viewport).toBe(null);
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should override contextOptions", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
            acceptDownloads: false,
            bypassCSP: true,
            colorScheme: 'dark',
            contrast: 'more',
            deviceScaleFactor: 2,
            extraHTTPHeaders: {'foo': 'bar'},
            forcedColors: 'active',
            hasTouch: true,
            ignoreHTTPSErrors: true,
            isMobile: true,
            javaScriptEnabled: true,
            locale: 'fr-FR',
            locateOnlyVisible: false,
            offline: true,
            permissions: ['geolocation'],
            reducedMotion: 'reduce',
            timezoneId: 'TIMEZONE',
            userAgent: 'UA',
            viewport: null,
            contextOptions: {
                acceptDownloads: true,
                bypassCSP: false,
                colorScheme: 'light',
                contrast: 'no-preference',
                deviceScaleFactor: 1,
                extraHTTPHeaders: {'foo': 'bar2'},
                forcedColors: 'none',
                hasTouch: false,
                ignoreHTTPSErrors: false,
                isMobile: false,
                javaScriptEnabled: false,
                locale: 'en-US',
                locateOnlyVisible: true,
                offline: false,
                permissions: [],
                reducedMotion: 'no-preference',
                timezoneId: 'TIMEZONE 2',
                userAgent: 'UA 2',
                viewport: { width: 500, height: 500 }
              }
            }
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect.soft(acceptDownloads).toBe(false);
            expect.soft(bypassCSP).toBe(true);
            expect.soft(colorScheme).toBe('dark');
            expect.soft(contrast).toBe('more');
            expect.soft(deviceScaleFactor).toBe(2);
            expect.soft(extraHTTPHeaders).toEqual({'foo': 'bar'});
            expect.soft(forcedColors).toBe('active');
            expect.soft(hasTouch).toBe(true);
            expect.soft(ignoreHTTPSErrors).toBe(true);
            expect.soft(isMobile).toBe(true);
            expect.soft(javaScriptEnabled).toBe(true);
            expect.soft(locale).toBe('fr-FR');
            expect.soft(locateOnlyVisible).toBe(false);
            expect.soft(offline).toBe(true);
            expect.soft(permissions).toEqual(['geolocation']);
            expect.soft(reducedMotion).toBe('reduce');
            expect.soft(timezoneId).toBe('TIMEZONE');
            expect.soft(userAgent).toBe('UA');
            expect.soft(viewport).toBe(null);
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should respect testIdAttribute and locateOnlyVisible", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
              testIdAttribute: 'data-pw',
              locateOnlyVisible: true,
            }
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            page.setContent('<button data-pw="myid" hidden>Hi</button><button data-pw="myid" onclick="this.textContent = \\'Clicked\\'">Hi</button>');
            expect(page.getByTestId('myid')).toHaveCount(1);
            page.getByTestId('myid').click();
            expect(page.getByTestId('myid')).toHaveText('Clicked');
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
    test("should respect testIdAttribute with multiple comma-separated names", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          export default {
            use: {
              testIdAttribute: 'data-pw,data-ti',
            }
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            page.setContent(\`
              <div data-pw="myid">A</div>
              <div data-ti="myid">B</div>
              <div data-testid="myid">C</div>
            \`);
            expect(page.getByTestId('myid')).toHaveCount(2);
            expect(page.getByTestId('myid')).toHaveText(['A', 'B']);
          });
        `,
      }, { workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
    });
}
