// Migrated from TypeScript (playwright-test/types.spec.ts) via cap from -ts

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

fn main() {
    test("should check types of fixtures", () => {
      const result = runTSC({
        "helper.ts": `
          import { test as base, expect, Page } from '@playwright/test';
          export type MyOptions = { foo: string, bar: number };
          export const test = base.extend<{ foo: string }, { bar: number }>({
            foo: 'foo',
            bar: [ 42, { scope: 'worker', timeout: 123 } ],
          });

          const good1 = test.extend<{}>({ foo: ({ bar }, run) => run('foo') });
          const good2 = test.extend<{}>({ bar: ({}, run) => run(42) });
          const good3 = test.extend<{}>({ bar: ({}, run) => run(42) });
          const good4 = test.extend<{}>({ bar: ({ bar }, run) => run(42) });
          const good5 = test.extend<{}>({ foo: ({ foo }, run) => run('foo') });
          const good6 = test.extend<{ baz: boolean }>({
            baz: false,
            foo: ({ baz }, run) => run('foo')
          });
          const good7 = test.extend<{ baz: boolean }>({
            baz: [ false, { auto: true, timeout: 0 } ],
          });
          const good8 = test.extend<{ foo: string }>({
            foo: [ ({}, use) => {
              use('foo');
            }, { scope: 'test' } ],
          });
          const good9 = test.extend<{}, {}>({
            bar: [ ({}, use) => {
              use(42);
            }, { scope: 'worker' } ],
          });

          // @ts-expect-error
          const fail1 = test.extend<{}>({ foo: 42 });
          // @ts-expect-error
          const fail2 = test.extend<{}>({ bar: ({ foo }, run) => run(42) });
          // @ts-expect-error
          const fail3 = test.extend<{}>({ baz: 42 });
          // @ts-expect-error
          const fail4 = test.extend<{}>({ foo: ({ foo }, run) => run(42) });
          // @ts-expect-error
          const fail5 = test.extend<{}>({ bar: ({}, run) => run('foo') });
          const fail6 = test.extend<{ baz: boolean }>({
            // @ts-expect-error
            baz: [ true, { scope: 'worker' } ],
          });
          const fail7 = test.extend<{}, { baz: boolean }>({
            // @ts-expect-error
            baz: [ true, { scope: 'test' } ],
          });
          const fail8 = test.extend<{}, { baz: boolean }>({
            // @ts-expect-error
            baz: true,
          });
          const fail9 = test.extend({
            foo: [ ({}, use) => {
              use('foo');
              // @ts-expect-error
            }, { scope: 'test', auto: true } ],
          });
          const fail10 = test.extend<{}, {}>({
            // @ts-expect-error
            bar: [ ({}, use) => {
              use(42);
            }, { scope: 'test' } ],
          });
          const fail11 = test.extend<{ yay: string }>({
            // @ts-expect-error
            yay: [ ({}, use) => {
              use('foo');
            }, { scope: 'test', timeout: 'str' } ],
          });

          type AssertNotAny<S> = {notRealProperty: number} extends S ? false : true;
          type AssertType<T, S> = S extends T ? AssertNotAny<S> : false;
          const funcTest = base.extend<{ foo: (x: number, y: string) => Promise<string> }>({
            foo: ({}, use) => {
              use((x, y) => {
                const assertionX: AssertType<number, typeof x> = true;
                const assertionY: AssertType<string, typeof y> = true;
                return y;
              });
            },
          });

          const chain1 = base.extend({
            page: ({ page }, use) => {
              use(page);
            },
          });
          const chain2 = chain1.extend<{ pageAsUser: Page }>({
            pageAsUser: ({ page }, use) => {
              // @ts-expect-error
              const x: number = page;
              // @ts-expect-error
              use(x);
            },
          });

          base.extend({
            page: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            },
          });

          base.extend<{ myFixture: (arg: number) => void }>({
            page: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            },
          });

          base.extend({
            // @ts-expect-error
            myFixture: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            }
          });

          base.extend<{ myFixture: (arg: number) => void }>({
            myFixture: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            }
          });

          base.extend({
            page: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            },
            // @ts-expect-error
            myFixture: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            }
          });

          base.extend<{ myFixture: (arg: number) => void }>({
            page: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            },
            myFixture: ({ page }) => {
              type IsPage = (typeof page) extends Page ? true : never;
              const isPage: IsPage = true;
            }
          });

          base.extend<{ myFixture: (arg: number) => void }>({
            // @ts-expect-error
            myFixture: (arg: number) => {},
          });

          base.extend<{ myFixture: (arg: number) => void }>({
            myFixture: (_, use) => {
              use((arg: number) => {});
              // @ts-expect-error
              use((arg: string) => {});
            }
          });
        `,
        "playwright.config.ts": `
          import { MyOptions } from './helper';
          import { Config } from '@playwright/test';
          const configs1: Config[] = [];
          configs1.push({ use: { foo: '42', bar: 42 } });
          configs1.push({ use: { foo: '42', bar: 42 }, timeout: 100 });

          const configs2: Config<MyOptions>[] = [];
          configs2.push({ use: { foo: '42', bar: 42 } });
          // @ts-expect-error
          configs2.push({ use: { bar: '42' } });
          // @ts-expect-error
          configs2.push(new Env2());
          // @ts-expect-error
          configs2.push({ use: { foo: 42, bar: 42 } });
          // @ts-expect-error
          configs2.push({ beforeAll: () => { return {}; } });
          // TODO: next line should not compile.
          configs2.push({ timeout: 100 });
          // @ts-expect-error
          configs2.push('alias');
          // TODO: next line should not compile.
          configs2.push({});
        `,
        "a.spec.ts": `
          import { test } from './helper';
          test.use({ foo: 'foo' });
          test.use({});

          // @ts-expect-error
          test.use({ foo: 42 });
          // @ts-expect-error
          test.use({ baz: 'baz' });

          test('my test', () => {
            bar += parseInt(foo);
          });
          test('my test', () => {
            bar += parseInt(foo);
          });
          test('my test', () => {});
          // @ts-expect-error
          test('my test', () => {
          });

          // @ts-expect-error
          test.beforeEach(({ a }) => {});
          test.beforeEach(({ foo, bar }) => {});
          test.beforeEach(() => {});

          // @ts-expect-error
          test.beforeAll(({ a }) => {});
          test.beforeAll(({ foo, bar }) => {});
          test.beforeAll(() => {});

          // @ts-expect-error
          test.afterEach(({ a }) => {});
          test.afterEach(({ foo, bar }) => {});
          test.afterEach(() => {});

          // @ts-expect-error
          test.afterAll(({ a }) => {});
          test.afterAll(({ foo, bar }) => {});
          test.afterAll(() => {});
        `,
        "playwright-props.config.ts": `
          import { PlaywrightTestConfig } from '@playwright/test';
          const config0: PlaywrightTestConfig = {
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
            },
          };

          const config1: PlaywrightTestConfig = {
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              // @ts-expect-error
              hasTouch: 'foo',
            },
          };

          const config2: PlaywrightTestConfig = {
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              // @ts-expect-error
              foo: true,
            },
          };

          const config3: PlaywrightTestConfig<{ foo: boolean }> = {
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              foo: true,
            },
          };

          const config4: PlaywrightTestConfig<{ foo: boolean }> = {
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              foo: true,
              // @ts-expect-error
              hasTouch: 'foo',
            },
          };
        `,

        "playwright-define.config.ts": `
          import { defineConfig } from '@playwright/test';
          const config0 = defineConfig({
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
            },
          });

          const config1 = defineConfig({
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              // @ts-expect-error
              hasTouch: 'foo',
            },
          });

          const config2 = defineConfig({
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              // @ts-expect-error
              foo: true,
            },
          });

          const config3 = defineConfig<{ foo: boolean }>({
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              foo: true,
            },
          });

          const config4 = defineConfig<{ foo: boolean }>({
            use: {
              ignoreHTTPSErrors: undefined,
              isMobile: true,
              javaScriptEnabled: false,
              foo: true,
              // @ts-expect-error
              hasTouch: 'foo',
            },
          });
        `,
        "playwright-define-merge.config.ts": `
          import { defineConfig } from '@playwright/test';
          const config0 = defineConfig({
            timeout: 1,
            // @ts-expect-error
            grep: 23,
          }, {
            timeout: 2,
          });
        `,
      });
      expect_eq(result.exitCode, 0);
    });
    test("config should allow void/empty options", () => {
      const result = runTSC({
        "playwright.config.ts": `
          import { Config } from '@playwright/test';
          const configs: Config[] = [];
          configs.push({});
          configs.push({ timeout: 100 });
          configs.push();
          configs.push({ use: { foo: 42 }});
        `,
        "a.spec.ts": `
          import { test, expect } from '@playwright/test';
          test('my test', () => {
          });
        `
      });
      expect_eq(result.exitCode, 0);
    });
}
