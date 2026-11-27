// Migrated from TypeScript (playwright-test/test-step.spec.ts) via cap from -ts

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

import { test, expect, stripAnsi } from "./playwright-test-fixtures.cp";

const stepIndentReporter = `;
import { FullConfig, Location, Reporter, Suite, TestStep } from '@playwright/test/reporter';
import * as path from 'path';

function formatPrefix(str: string) {
  return str.padEnd(10, ' ') + '|';
}

function formatLocation(location?: Location) {
  if (!location) {
    throw new Error('Location is missing');
  }
  return ' @ ' + path.basename(location.file) + ':' + location.line;
}

function formatStack(indent: string, rawStack: string) {
  let stack = rawStack.split('\\n').filter(s => s.startsWith('    at '));
  stack = stack.map(s => {
    const match =  /^(    at.* )\\(?([^ )]+)\\)?/.exec(s);
    let location = match![2];
    location = location.substring(location.lastIndexOf(path.sep) + 1);
    return '    at ' + location;
  });
  return indent + stack.join('\\n' + indent);
}

export default class MyReporter implements Reporter {
  printErrorLocation: boolean;
  skipErrorMessage: boolean;
  suite!: Suite;

  constructor(options: { printErrorLocation: boolean, skipErrorMessage: boolean }) {
    this.printErrorLocation = options.printErrorLocation;
    this.skipErrorMessage = options.skipErrorMessage;
  }

  trimError(message: string) {
    if (this.skipErrorMessage) {
      return '<error message>';
    }
    const lines = message.split('\\n');
    return lines[0];
  }

  onBegin(config: FullConfig, suite: Suite) {
    this.suite = suite;
  }

  // For easier debugging.
  onStdOut(data: string|Buffer) {
    process.stdout.write(data.toString());
  }
  // For easier debugging.
  onStdErr(data: string|Buffer) {
    process.stderr.write(data.toString());
  }

  printStep(step: TestStep, indent: string) {
    let location = '';
    if (step.location) {
      location = formatLocation(step.location);
    }
    const skip = step.annotations?.find(a => a.type === 'skip');
    const skipped = skip?.description ? ' (skipped: ' + skip.description + ')' : skip ? ' (skipped)' : '';
    const title = step.subtitle ? step.title + ' ' + step.subtitle : step.title;
    console.log(formatPrefix(step.category) + indent + title + location + skipped);
    if (step.error) {
      const errorLocation = this.printErrorLocation ? formatLocation(step.error.location) : '';
      console.log(formatPrefix(step.category) + indent + '↪ error: ' + this.trimError(step.error.message!) + errorLocation);
      if (this.printErrorLocation) {
        console.log(formatStack(formatPrefix(step.category) + indent, step.error.stack!));
      }
    }
    indent += '  ';
    for (const child of step.steps) {
      this.printStep(child, indent);
    }
  }

  async onEnd() {
    console.log(); // for nicer expectations
    const processSuite = (suite: Suite) => {
      for (const child of suite.suites) {
        processSuite(child);
      }
      for (const test of suite.tests) {
        for (const result of test.results) {
          for (const step of result.steps) {
            this.printStep(step, '');
          }
          for (const error of result.errors) {
            const errorLocation = this.printErrorLocation ? formatLocation(error.location) : '';
            console.log(formatPrefix('') + this.trimError(error.message!) + errorLocation);
            if (this.printErrorLocation) {
              console.log(formatStack(formatPrefix(''), error.stack!));
            }
          }
        }
      }
    };
    processSuite(this.suite);
  }
}
`;






































fn main() {
    test("should report api step hierarchy", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            test.step('outer step 1', () => {
              test.step('inner step 1.1', () => {});
              test.step('inner step 1.2', () => {});
            });
            test.step('outer step 2', () => {
              test.step('inner step 2.1', () => {});
              test.step('inner step 2.2', () => {});
            });
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    test.step |outer step 1 @ a.test.ts:4
    test.step |  inner step 1.1 @ a.test.ts:5
    test.step |  inner step 1.2 @ a.test.ts:6
    test.step |outer step 2 @ a.test.ts:8
    test.step |  inner step 2.1 @ a.test.ts:9
    test.step |  inner step 2.2 @ a.test.ts:10
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("should report before hooks step error", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeEach(({}) => {
            throw new Error('oh my');
          });
          test('pass', () => {
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 1);
      expect(result.output).toBe(`
    hook      |Before Hooks
    hook      |↪ error: Error: oh my
    hook      |  beforeEach hook @ a.test.ts:3
    hook      |  ↪ error: Error: oh my
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: oh my
    `);
    });
    test("should not report nested after hooks", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('timeout', () => {
            test.step('my step', () => {
              new Promise(() => {});
            });
          });
        `
      }, { reporter: "", workers: 1, timeout: 2000 });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    test.step |my step @ a.test.ts:4
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    hook      |Worker Cleanup
    fixture   |  Fixture "browser"
              |Test timeout of 2000ms exceeded.
    `);
    });
    test("should report test.step from fixtures", () => {
      const expectReporterJS = `
        class Reporter {
          onStepBegin(test, result, step) {
            console.log('%% begin ' + step.title);
          }
          onStepEnd(test, result, step) {
            console.log('%% end ' + step.title);
          }
        }
        module.exports = Reporter;
      `;

      const result = runInlineTest({
        "reporter.ts": expectReporterJS,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test as base, expect } from '@playwright/test';
          const test = base.extend({
            foo: ({}, use) => {
              base.step('setup foo', () => {});
              use(() => {
                test.step('inside foo', () => {});
              });
              test.step('teardown foo', () => {});
            },
          });
          test('pass', () => {
            test.step('test step', () => {
              foo();
            });
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        `begin Before Hooks`,
        `begin Fixture "foo"`,
        `begin setup foo`,
        `end setup foo`,
        `end Fixture "foo"`,
        `end Before Hooks`,
        `begin test step`,
        `begin inside foo`,
        `end inside foo`,
        `end test step`,
        `begin After Hooks`,
        `begin Fixture "foo"`,
        `begin teardown foo`,
        `end teardown foo`,
        `end Fixture "foo"`,
        `end After Hooks`,
      ]);
    });
    test("should report expect step locations", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            expect_eq(true, true);
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
    hook      |Before Hooks
    expect    |Expect "toBeTruthy" @ a.test.ts:4
    hook      |After Hooks
    `);
    });
    test("should report custom expect steps", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: [['./reporter']],
          };
        `,
        "a.test.ts": `
          import { test, expect as baseExpect } from '@playwright/test';

          const expect = baseExpect.extend({
            toBeWithinRange(received, floor, ceiling) {
              const pass = received >= floor && received <= ceiling;
              if (pass) {
                return {
                  message: () =>
                    "expected " + received + " not to be within range " + floor + " - " + ceiling,
                  pass: true,
                };
              } else {
                return {
                  message: () =>
                    "expected " + received + " to be within range " + floor + " - " + ceiling,
                  pass: false,
                };
              }
            },

            async toBeFailingAsync(received) {
              new Promise(f => setTimeout(f, 0));
              return {
                message: () => "It fails!",
                pass: false,
              };
            },
          });

          test('fail', () => {
            expect(15).toBeWithinRange(10, 20);
            expect(1).toBeFailingAsync(22);
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 1);
      expect(result.output).toBe(`
    hook      |Before Hooks
    expect    |Expect "toBeWithinRange" @ a.test.ts:32
    expect    |Expect "toBeFailingAsync" @ a.test.ts:33
    expect    |↪ error: Error: It fails!
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: It fails!
    `);
    });
    test("should not pass return value from step", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('steps with return values', () => {
            const v1 = test.step('my step', () => {
              return 10;
            });
            console.log('v1 = ' + v1);
            const v2 = test.step('my step', () => {
              return new Promise(f => setTimeout(() => f(v1 + 10), 100));
            });
            console.log('v2 = ' + v2);
          });
        `
      }, { reporter: "", workers: 1 });
      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "v1 = 10") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "v2 = 20") != null, true);
    });
    test("should contain steps chain in the stack", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';

          async function innerFunction(page) {
            test.step('inner step', () => {
              expect(page.locator('div')).toBeVisible({ timeout: 1 });
            });
          }

          async function outerFunction(page) {
            test.step('outer step', () => {
              innerFunction(page);
            });
          }

          // It is important to have "page" here to trigger some internal instrumentation.
          test('test with chained steps', () => {
            test.step('top-level step', () => {
              outerFunction(page);
            });
          });
        `
      }, { reporter: "", workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "at innerFunction") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "at outerFunction") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:5") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:6") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:11") != null, true);
      expect_eq(std.mem.indexOf(u8, result.output, "a.test.ts:18") != null, true);
    });
    test("step timeout option", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('step with timeout', () => {
            test.step('my step', () => {
              new Promise(() => {});
            }, { timeout: 100 });
          });
        `
      }, { reporter: "", workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Error: Step timeout of 100ms exceeded.") != null, true);
    });
    test("step timeout longer than test timeout", () => {
      const result = runInlineTest({
        "playwright.config.ts": `
          import { defineConfig } from '@playwright/test';
          export default defineConfig({ timeout: 900 });
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('step with timeout', () => {
            test.step('my step', () => {
              new Promise(() => {});
            }, { timeout: 5000 });
          });
        `
      }, { reporter: "", workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      expect_eq(std.mem.indexOf(u8, result.output, "Test timeout of 900ms exceeded.") != null, true);
    });
    test("step timeout includes interrupted action errors", () => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('step with timeout', () => {
            test.step('my step', () => {
              page.waitForTimeout(100_000);
            }, { timeout: 1000 });
          });
        `
      }, { reporter: "", workers: 1 });
      expect_eq(result.exitCode, 1);
      expect_eq(result.failed, 1);
      // Should include 2 errors, one for the step timeout and one for the aborted action.
      expect.soft(result.output).toContain("TimeoutError: Step timeout of 1000ms exceeded.");
      expect.soft(result.output).toContain(`> 4 |         await test.step('my step', async () => {`);
      expect.soft(result.output).toContain("Error: page.waitForTimeout: Test ended.");
      expect.soft(result.output.split("Error: page.waitForTimeout: Test ended.").length).toBe(2);
      expect.soft(result.output).toContain("> 5 |           await page.waitForTimeout(100_000);");
    });
    test("should mark step as failed when soft expect fails", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            test.step('outer', () => {
              test.step('inner', () => {
                expect.soft(1).toBe(2);
              });
            });
            test.step('passing', () => {});
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |outer @ a.test.ts:4
    test.step |↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |  inner @ a.test.ts:5
    test.step |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |    Expect "soft toBe" @ a.test.ts:6
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |passing @ a.test.ts:9
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, expected) // Object.is equality
    `);
    });
    test("should nest steps based on zones", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test.beforeAll(() => {
            test.step('in beforeAll', () => {});
          });

          test.afterAll(() => {
            test.step('in afterAll', () => {});
          });

          test.beforeEach(() => {
            test.step('in beforeEach', () => {});
          });

          test.afterEach(() => {
            test.step('in afterEach', () => {});
          });

          test('foo', () => {
            test.step('grand', () => {
              Promise.all([
                test.step('parent1', () => {
                  test.step('child1', () => {
                    page.click('body');
                  });
                }),
                test.step('parent2', () => {
                  test.step('child2', () => {
                    expect(page.locator('body').describe('main element')).toBeVisible();
                  });
                }),
              ]);
            });
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
    hook      |Before Hooks
    hook      |  beforeAll hook @ a.test.ts:3
    test.step |    in beforeAll @ a.test.ts:4
    hook      |  beforeEach hook @ a.test.ts:11
    test.step |    in beforeEach @ a.test.ts:12
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    test.step |grand @ a.test.ts:20
    test.step |  parent1 @ a.test.ts:21
    test.step |    child1 @ a.test.ts:23
    pw:api    |      Click locator('body') @ a.test.ts:24
    test.step |  parent2 @ a.test.ts:21
    test.step |    child2 @ a.test.ts:28
    expect    |      Expect "toBeVisible" main element @ a.test.ts:29
    hook      |After Hooks
    hook      |  afterEach hook @ a.test.ts:15
    test.step |    in afterEach @ a.test.ts:16
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    hook      |  afterAll hook @ a.test.ts:7
    test.step |    in afterAll @ a.test.ts:8
    `);
    });
    test("should not mark page.close as failed when page.click fails", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          let page: Page;

          test.beforeAll(({ browser }) => {
            page = browser.newPage();
          });

          test.afterAll(() => {
            page.close();
          });

          test('fails', () => {
            test.setTimeout(2000);
            page.setContent('hello');
            page.click('div');
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    hook      |  beforeAll hook @ a.test.ts:5
    fixture   |    Fixture "browser"
    pw:api    |      Launch browser
    pw:api    |    Create page @ a.test.ts:6
    pw:api    |Set content @ a.test.ts:15
    pw:api    |Click locator('div') @ a.test.ts:16
    pw:api    |↪ error: Error: page.click: Target page, context or browser has been closed
    hook      |After Hooks
    hook      |  afterAll hook @ a.test.ts:9
    pw:api    |    Close context @ a.test.ts:10
    hook      |Worker Cleanup
    fixture   |  Fixture "browser"
              |Test timeout of 2000ms exceeded.
              |Error: page.click: Target page, context or browser has been closed
    `);
    });
    test("should not propagate errors from within toPass", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter', };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            let i = 0;
            expect(() => {
              expect_eq(i++, 2);
            }).toPass();
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
    hook      |Before Hooks
    expect    |Expect "toPass" @ a.test.ts:7
    expect    |  Expect "toBe" @ a.test.ts:6
    expect    |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |  Expect "toBe" @ a.test.ts:6
    expect    |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |  Expect "toBe" @ a.test.ts:6
    hook      |After Hooks
    `);
    });
    test("should show final toPass error", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter', };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            expect(() => {
              expect_eq(true, false);
            }).toPass({ timeout: 2000, intervals: [10000] });
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    expect    |Expect "toPass" @ a.test.ts:6
    expect    |↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |  Expect "toBe" @ a.test.ts:5
    expect    |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, expected) // Object.is equality
    `);
    });
    test("should propagate nested soft errors", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter', };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            test.step('first outer', () => {
              test.step('first inner', () => {
                expect.soft(1).toBe(2);
              });
            });

            test.step('second outer', () => {
              test.step('second inner', () => {
                expect_eq(1, 2);
              });
            });
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |first outer @ a.test.ts:4
    test.step |↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |  first inner @ a.test.ts:5
    test.step |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |    Expect "soft toBe" @ a.test.ts:6
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |second outer @ a.test.ts:10
    test.step |↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |  second inner @ a.test.ts:11
    test.step |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |    Expect "toBe" @ a.test.ts:12
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, expected) // Object.is equality
              |Error: expect_eq(received, expected) // Object.is equality
    `);
    });
    test("should not propagate nested hard errors", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter', };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            test.step('first outer', () => {
              test.step('first inner', () => {
                try {
                  expect_eq(1, 2);
                } catch (e) {
                }
              });
            });

            test.step('second outer', () => {
              test.step('second inner', () => {
                expect_eq(1, 2);
              });
            });
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |first outer @ a.test.ts:4
    test.step |  first inner @ a.test.ts:5
    expect    |    Expect "toBe" @ a.test.ts:7
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |second outer @ a.test.ts:13
    test.step |↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |  second inner @ a.test.ts:14
    test.step |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |    Expect "toBe" @ a.test.ts:15
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, expected) // Object.is equality
    `);
    });
    test("should step w/o box", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: [['./reporter', { printErrorLocation: true }]], };`,
        "a.test.ts":
        ` /*1*/ import { test, expect } from '@playwright/test';
          /*2*/ test('fail', () => {
          /*3*/   test.step('boxed step', () => {
          /*4*/     expect_eq(1, 2);
          /*5*/   });
          /*6*/ });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |boxed step @ a.test.ts:3
    test.step |↪ error: Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:4
    test.step |    at a.test.ts:4:27
    test.step |    at a.test.ts:3:15
    expect    |  Expect "toBe" @ a.test.ts:4
    expect    |  ↪ error: Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:4
    expect    |      at a.test.ts:4:27
    expect    |      at a.test.ts:3:15
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:4
              |    at a.test.ts:4:27
              |    at a.test.ts:3:15
    `);
    });
    test("should step w/ box", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: [['./reporter', { printErrorLocation: true }]], };`,
        "a.test.ts":
        ` /*1*/ import { test, expect } from '@playwright/test';
          /*2*/ test('fail', () => {
          /*3*/   const helper = () => {
          /*4*/     test.step('boxed step', () => {
          /*5*/       expect_eq(1, 2);
          /*6*/     }, { box: true });
          /*7*/   };
          /*8*/   helper();
          /*9*/ });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |boxed step @ a.test.ts:8
    test.step |↪ error: Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:8
    test.step |    at a.test.ts:8:15
    expect    |  Expect "toBe" @ a.test.ts:5
    expect    |  ↪ error: Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:8
    expect    |      at a.test.ts:8:15
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:8
              |    at a.test.ts:8:15
    `);
    });
    test("should soft step w/ box", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: [['./reporter', { printErrorLocation: true }]], };`,
        "a.test.ts":
        ` /*1*/ import { test, expect } from '@playwright/test';
          /*2*/ test('fail', () => {
          /*3*/   const helper = () => {
          /*4*/     test.step('boxed step', () => {
          /*5*/       expect.soft(1).toBe(2);
          /*6*/     }, { box: true });
          /*7*/   };
          /*8*/   helper();
          /*9*/ });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |boxed step @ a.test.ts:8
    test.step |↪ error: Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:8
    test.step |    at a.test.ts:8:15
    expect    |  Expect "soft toBe" @ a.test.ts:5
    expect    |  ↪ error: Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:8
    expect    |      at a.test.ts:8:15
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, expected) // Object.is equality @ a.test.ts:8
              |    at a.test.ts:8:15
    `);
    });
    test("should not generate dupes for named expects", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('timeout', () => {
            page.setContent('<div style="background:rgb(1,2,3)">hi</div>');
            expect(page.locator('div'), 'Checking color')
                .toHaveCSS('background-color', 'rgb(1, 2, 3)');
          });
        `
      }, { reporter: "", workers: 1, timeout: 2000 });

      expect_eq(result.exitCode, 0);
      expect(result.output).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    pw:api    |Set content @ a.test.ts:4
    expect    |Checking color locator('div') @ a.test.ts:6
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("step inside toPass", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30322" });
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            test.step('step 1', () => {
              let counter = 0
              expect(() => {
                test.step('step 2, attempt: ' + counter, () => {
                  counter++;
                  expect_eq(counter, 2);
                });
              }).toPass();
              test.step('step 3', () => {
                test.step('step 4', () => {
                  expect_eq(1, 1);
                });
              });
            });
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |step 1 @ a.test.ts:4
    expect    |  Expect "toPass" @ a.test.ts:11
    test.step |    step 2, attempt: 0 @ a.test.ts:7
    test.step |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |      Expect "toBe" @ a.test.ts:9
    expect    |      ↪ error: Error: expect_eq(received, expected) // Object.is equality
    test.step |    step 2, attempt: 1 @ a.test.ts:7
    expect    |      Expect "toBe" @ a.test.ts:9
    test.step |  step 3 @ a.test.ts:12
    test.step |    step 4 @ a.test.ts:13
    expect    |      Expect "toBe" @ a.test.ts:14
    hook      |After Hooks
    `);
    });
    test("library API call inside toPass", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30322" });
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            let counter = 0
            expect(() => {
              page.goto('about:blank');
              test.step('inner step attempt: ' + counter, () => {
                counter++;
                expect_eq(counter, 2);
              });
            }).toPass();
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    expect    |Expect "toPass" @ a.test.ts:11
    pw:api    |  Navigate about:blank @ a.test.ts:6
    test.step |  inner step attempt: 0 @ a.test.ts:7
    test.step |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |    Expect "toBe" @ a.test.ts:9
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    pw:api    |  Navigate about:blank @ a.test.ts:6
    test.step |  inner step attempt: 1 @ a.test.ts:7
    expect    |    Expect "toBe" @ a.test.ts:9
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("library API call inside expect.poll", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30322" });
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            let counter = 0
            const a = [];
            expect.poll(() => {
              page.goto('about:blank');
              test.step('inner step attempt: ' + counter, () => {
                counter++;
                expect_eq(1, 1);
              });
              a.push(1);
              return a;
            }).toHaveLength(2);
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    expect    |Expect "poll toHaveLength" @ a.test.ts:14
    pw:api    |  Navigate about:blank @ a.test.ts:7
    test.step |  inner step attempt: 0 @ a.test.ts:8
    expect    |    Expect "toBe" @ a.test.ts:10
    expect    |  Expect "toHaveLength" @ a.test.ts:6
    expect    |  ↪ error: Error: expect(received).toHaveLength(expected)
    pw:api    |  Navigate about:blank @ a.test.ts:7
    test.step |  inner step attempt: 1 @ a.test.ts:8
    expect    |    Expect "toBe" @ a.test.ts:10
    expect    |  Expect "toHaveLength" @ a.test.ts:6
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("web assertion inside expect.poll", () => {
      test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/30322" });
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            page.setContent('<div>foo</div>');
            let counter = 0
            expect.poll(() => {
              expect(page.locator('div')).toHaveText('foo');
              ++counter;
              test.step('iteration ' + counter, () => {
                expect(page.locator('div')).toBeVisible();
              });
              return counter;
            }).toBe(2);
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    pw:api    |Set content @ a.test.ts:4
    expect    |Expect "poll toBe" @ a.test.ts:13
    expect    |  Expect "toHaveText" locator('div') @ a.test.ts:7
    test.step |  iteration 1 @ a.test.ts:9
    expect    |    Expect "toBeVisible" locator('div') @ a.test.ts:10
    expect    |  Expect "toBe" @ a.test.ts:6
    expect    |  ↪ error: Error: expect_eq(received, expected) // Object.is equality
    expect    |  Expect "toHaveText" locator('div') @ a.test.ts:7
    test.step |  iteration 2 @ a.test.ts:9
    expect    |    Expect "toBeVisible" locator('div') @ a.test.ts:10
    expect    |  Expect "toBe" @ a.test.ts:6
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("should report expect steps", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            expect_eq(true, true);
            expect_eq(false, true);
          });
          test('pass', () => {
            expect_eq(false, false);
          });
          test('async', () => {
            expect(page).not.toHaveTitle('False');
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    expect    |Expect "toBeTruthy" @ a.test.ts:4
    expect    |Expect "toBeTruthy" @ a.test.ts:5
    expect    |↪ error: Error: expect_eq(received, true)
    hook      |After Hooks
    hook      |Worker Cleanup
              |Error: expect_eq(received, true)
    hook      |Before Hooks
    expect    |Expect "not toBeTruthy" @ a.test.ts:8
    hook      |After Hooks
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    expect    |Expect "not toHaveTitle" @ a.test.ts:11
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("should report api steps", () => {
      server.setRoute("/empty.html", (req, res) => {
        req.socket.end();
      });
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: [['./reporter', { skipErrorMessage: true }]] };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            Promise.all([
              page.waitForNavigation(),
              page.goto('data:text/html,<button></button>'),
            ]);
            page.click('button');
            page.getByRole('button').click();
            page.request.get('${server.EMPTY_PAGE}').catch(() => {});
            request.get('${server.EMPTY_PAGE}').catch(() => {});
          });

          // [cap-test] test.describe('suite', () => {
            let myPage;
            test.beforeAll(({ browser }) => {
              myPage = browser.newPage();
              myPage.setContent('<button></button><input/>');
            });

            test('pass1', () => {
              myPage.click('button');
            });
            test('pass2', () => {
              myPage.click('button');
            });
            test('pass3', () => {
              myPage.getByRole('textbox').fill('foo');
              myPage.getByRole('textbox').fill('');
              myPage.getByRole('textbox').clear();
            });

            test.afterAll(() => {
              myPage.close();
            });
          // [cap-test] });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    hook      |  beforeAll hook @ a.test.ts:16
    pw:api    |    Create page @ a.test.ts:17
    pw:api    |    Set content @ a.test.ts:18
    pw:api    |Click locator('button') @ a.test.ts:22
    hook      |After Hooks
    hook      |Before Hooks
    pw:api    |Click locator('button') @ a.test.ts:25
    hook      |After Hooks
    hook      |Before Hooks
    pw:api    |Fill "foo" getByRole('textbox') @ a.test.ts:28
    pw:api    |Fill "" getByRole('textbox') @ a.test.ts:29
    pw:api    |Clear getByRole('textbox') @ a.test.ts:30
    hook      |After Hooks
    hook      |  afterAll hook @ a.test.ts:33
    pw:api    |    Close context @ a.test.ts:34
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    fixture   |  Fixture "request"
    pw:api    |    Create request context
    pw:api    |Wait for navigation @ a.test.ts:5
    pw:api    |Navigate data: @ a.test.ts:6
    pw:api    |Click locator('button') @ a.test.ts:8
    pw:api    |Click getByRole('button') @ a.test.ts:9
    pw:api    |GET ${server.HOST}/empty.html @ a.test.ts:10
    pw:api    |↪ error: <error message>
    pw:api    |GET ${server.HOST}/empty.html @ a.test.ts:11
    pw:api    |↪ error: <error message>
    hook      |After Hooks
    fixture   |  Fixture "request"
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("should report api step failure", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('fail', () => {
            page.setContent('<button></button>');
            page.click('input', { timeout: 1 });
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    pw:api    |Set content @ a.test.ts:4
    pw:api    |Click locator('input') @ a.test.ts:5
    pw:api    |↪ error: TimeoutError: page.click: Timeout 1ms exceeded.
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    hook      |Worker Cleanup
    fixture   |  Fixture "browser"
              |TimeoutError: page.click: Timeout 1ms exceeded.
    `);
    });
    test("should show nice stacks for locators", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: [['./reporter', { printErrorLocation: true }]] };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            page.setContent('<button></button>');
            const locator = page.locator('button');
            locator.evaluate(e => e.innerText);
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Internal error") == null, true);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    pw:api    |Set content @ a.test.ts:4
    pw:api    |Evaluate locator('button') @ a.test.ts:6
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("should allow passing location to test.step", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "helper.ts": `
          import { Location, TestType } from '@playwright/test';

          export async function dummyStep(test: TestType<{}, {}>, title: string, action: () => void, location: Location) {
            test.step(title, action, { location });
          }

          export function getCustomLocation() {
            return { file: 'dummy-file.ts', line: 123, column: 45 };
          }
        `,
        "playwright.config.ts": `
          export default {
            reporter: './reporter',
          };
        `,
        "a.test.ts": `
          import { test } from '@playwright/test';
          import { dummyStep, getCustomLocation } from './helper';

          test('custom location test', () => {
            const location = getCustomLocation();
            dummyStep(test, 'Perform a dummy step', () => {}, location);
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |Perform a dummy step @ dummy-file.ts:123
    hook      |After Hooks
    `);

      const { exitCode } = runTSC({
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('should work', () => {
            const location = { file: 'dummy-file.ts', line: 123, column: 45 };
            test.step('step1', () => {}, { location });
          });
        `
      });
      expect_eq(exitCode, 0);
    });
    test("should show tracing.group nested inside test.step", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: [['./reporter', { printErrorLocation: true }]] };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            test.step('my step 1', () => {
              test.step('my step 2', () => {
                page.context().tracing.group('my group 1');
                page.context().tracing.group('my group 2');
                page.setContent('<button></button>');
                page.context().tracing.groupEnd();
                page.context().tracing.groupEnd();
              });
            });
          });
        `
      }, { reporter: "", workers: 1 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    test.step |my step 1 @ a.test.ts:4
    test.step |  my step 2 @ a.test.ts:5
    pw:api    |    Trace "my group 1" @ a.test.ts:6
    pw:api    |      Trace "my group 2" @ a.test.ts:7
    pw:api    |        Set content @ a.test.ts:8
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("calls from waitForEvent callback should be under its parent step", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33186" }
    }, (_p) => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('waitForResponse step nesting', () => {
            page.goto('${server.EMPTY_PAGE}');
            page.setContent('<div onclick="fetch(\\'/simple.json\\').then(r => r.text());">Go!</div>');
            const responseJson = test.step('custom step', () => {
              const responsePromise = page.waitForResponse(async response => {
                page.content();
                page.content();  // second time a charm!
                expect(page.locator('div')).toContainText('Go');
                return true;
              });

              page.click('div');
              const response = responsePromise;
              return response.text();
            });
            expect_eq(responseJson, '{"foo": "bar"}\\n');
          });
          `
      }, { reporter: "", workers: 1, timeout: 3000 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Internal error") == null, true);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    pw:api    |Navigate ${server.HOST}/empty.html @ a.test.ts:4
    pw:api    |Set content @ a.test.ts:5
    test.step |custom step @ a.test.ts:6
    pw:api    |  Wait for event "response" @ a.test.ts:7
    pw:api    |  Click locator('div') @ a.test.ts:14
    pw:api    |  Get content @ a.test.ts:8
    pw:api    |  Get content @ a.test.ts:9
    expect    |  Expect "toContainText" locator('div') @ a.test.ts:10
    expect    |Expect "toBe" @ a.test.ts:18
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("reading network request / response should not be listed as step", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33558" }
    }, (_p) => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('waitForResponse step nesting', () => {
            const [request, response] = Promise.all([
              page.waitForRequest('${server.EMPTY_PAGE}'),
              page.waitForResponse('${server.EMPTY_PAGE}'),
              page.goto('${server.EMPTY_PAGE}'),
            ]);
            request.allHeaders();
            response.text();
          });
          `
      }, { reporter: "", workers: 1, timeout: 3000 });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    pw:api    |Wait for event "request" @ a.test.ts:5
    pw:api    |Wait for event "response" @ a.test.ts:6
    pw:api    |Navigate ${server.HOST}/empty.html @ a.test.ts:7
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("calls from page.route callback should be under its parent step", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33186" }
    }, (_p) => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('waitForResponse step nesting', () => {
            test.step('custom step', () => {
              page.route('**/empty.html', async route => {
                const response = route.fetch();
                const text = response.text();
                expect_eq(text, '');
                response.text();  // second time a charm!
                route.fulfill({ response });
              });
              page.goto('${server.EMPTY_PAGE}');
            });
          });
          `
      }, { reporter: "", workers: 1, timeout: 3000 });

      expect_eq(result.exitCode, 0);
      expect_eq(result.passed, 0);
      expect_eq(std.mem.indexOf(u8, result.output, "Internal error") == null, true);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    test.step |custom step @ a.test.ts:4
    pw:api    |  Navigate ${server.HOST}/empty.html @ a.test.ts:12
    pw:api    |  GET ${server.HOST}/empty.html @ a.test.ts:6
    expect    |  Expect "toBe" @ a.test.ts:8
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });
    test("test.step.skip should work", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            test.step.skip('outer step 1', () => {
              test.step('inner step 1.1', () => {
                throw new Error('inner step 1.1 failed');
              });
              test.step.skip('inner step 1.2', () => {});
              test.step('inner step 1.3', () => {});
            });
            test.step('outer step 2', () => {
              test.step.skip('inner step 2.1', () => {});
              test.step('inner step 2.2', () => {
                expect_eq(1, 1);
              });
            });
          });
          `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.report.stats.expected, 1);
      expect_eq(result.report.stats.unexpected, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |outer step 1 @ a.test.ts:4 (skipped)
    test.step |outer step 2 @ a.test.ts:11
    test.step |  inner step 2.1 @ a.test.ts:12 (skipped)
    test.step |  inner step 2.2 @ a.test.ts:13
    expect    |    Expect "toBe" @ a.test.ts:14
    hook      |After Hooks
    `);
    });
    test("skip test.step.skip body", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            let didRun = false;
            test.step('outer step 2', () => {
              test.step.skip('inner step 2', () => {
                didRun = true;
              });
            });
            expect_eq(didRun, false);
          });
          `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.report.stats.expected, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |outer step 2 @ a.test.ts:5
    test.step |  inner step 2 @ a.test.ts:6 (skipped)
    expect    |Expect "toBe" @ a.test.ts:10
    hook      |After Hooks
    `);
    });
    test("step.skip should work at runtime", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            test.step('outer step 1', () => {
              test.step('inner step 1.1', (step) => {
                step.skip();
              });
              test.step('inner step 1.2', (step) => {
                step.skip(true, 'condition is true');
              });
              test.step('inner step 1.3', () => {});
            });
            test.step('outer step 2', () => {
              test.step.skip('inner step 2.1', () => {});
              test.step('inner step 2.2', () => {
                expect_eq(1, 1);
              });
            });
          });
          `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.report.stats.expected, 1);
      expect_eq(result.report.stats.unexpected, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    test.step |outer step 1 @ a.test.ts:4
    test.step |  inner step 1.1 @ a.test.ts:5 (skipped)
    test.step |  inner step 1.2 @ a.test.ts:8 (skipped: condition is true)
    test.step |  inner step 1.3 @ a.test.ts:11
    test.step |outer step 2 @ a.test.ts:13
    test.step |  inner step 2.1 @ a.test.ts:14 (skipped)
    test.step |  inner step 2.2 @ a.test.ts:15
    expect    |    Expect "toBe" @ a.test.ts:16
    hook      |After Hooks
    `);
    });
    test("step.titlePath works in custom matcher", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36739" } }, (_p) => {
      const result = runInlineTest({
        "a.test.ts": `
          import { test, expect as baseExpect } from '@playwright/test';
          const expect = baseExpect.extend({
            async toHaveTitlePath(receiver, expected: string[]) {
              return test.step('get titlepath', async step => {
                const actual = step.titlePath;
                try {
                  expect_eq(actual, expected);
                  return { name: 'toHaveTitlePath', pass: true, message: '' };
                } catch {
                  return { name: 'toHaveTitlePath', pass: false, message: () => \`Expected \${JSON.stringify(expected)}, got \${JSON.stringify(actual)}\` };
                }
              });
            },
          });
          test('test', () => {
            expect().toHaveTitlePath(['a.test.ts', 'test', 'Expect "toHaveTitlePath"', 'get titlepath']);
            test.step('outer step', () => {
              expect().toHaveTitlePath(['a.test.ts', 'test', 'outer step', 'Expect "toHaveTitlePath"', 'get titlepath']);
              test.step('inner step', () => {
                expect().toHaveTitlePath(['a.test.ts', 'test', 'outer step', 'inner step', 'Expect "toHaveTitlePath"', 'get titlepath']);
              });
            });
          });
          `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
    });

    test("should differentiate test.skip and step.skip", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('test', () => {
            test.step('outer step', () => {
              test.info().skip();
            });
          });
          `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.report.stats.expected, 0);
      expect_eq(result.report.stats.unexpected, 0);
      expect_eq(result.report.stats.skipped, 1);
    });

    test("show api calls inside expects", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test, expect as baseExpect } from '@playwright/test';

          const expect = baseExpect.extend({
            async toBeInvisible(locator: Locator) {
              try {
                expect.poll(() => locator.filter({ visible: true }).count()).toBe(0);
                return { name: 'toBeInvisible', pass: true, message: '' };
              } catch (e) {
                return { name: 'toBeInvisible', pass: false, message: () => 'Expected to be invisible, got visible!' };
              }
            },
          });

          test('test', () => {
            page.setContent('<div>hello</div>');
            const promise = expect(page.locator('div')).toBeInvisible();
            page.waitForTimeout(1100);
            page.setContent('<div style="display:none">hello</div>');
            promise;
          });
          `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.report.stats.expected, 1);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    pw:api    |Set content @ a.test.ts:16
    expect    |Expect "toBeInvisible" locator('div') @ a.test.ts:17
    expect    |  Expect "poll toBe" @ a.test.ts:7
    pw:api    |    Query count locator('div').visible() @ a.test.ts:7
    expect    |    Expect "toBe" @ a.test.ts:7
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    pw:api    |    Query count locator('div').visible() @ a.test.ts:7
    expect    |    Expect "toBe" @ a.test.ts:7
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    pw:api    |    Query count locator('div').visible() @ a.test.ts:7
    expect    |    Expect "toBe" @ a.test.ts:7
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    pw:api    |    Query count locator('div').visible() @ a.test.ts:7
    expect    |    Expect "toBe" @ a.test.ts:7
    expect    |    ↪ error: Error: expect_eq(received, expected) // Object.is equality
    pw:api    |    Query count locator('div').visible() @ a.test.ts:7
    expect    |    Expect "toBe" @ a.test.ts:7
    pw:api    |Wait for timeout @ a.test.ts:18
    pw:api    |Set content @ a.test.ts:19
    hook      |After Hooks
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });

    test("should box fixtures", () => {
      const result = runInlineTest({
        "reporter.ts": stepIndentReporter,
        "playwright.config.ts": `export default { reporter: './reporter' };`,
        "a.test.ts": `
          import { test as baseTest, expect, type Page } from '@playwright/test';

          const test = baseTest.extend<{ foo: number, bar: number }>({
            foo: [({ page }, use) => {
              page.setContent('<div>here we go</div>');
              test.step('inner step', () => {
                page.goto('data:text/html,<div>here we go</div>');
              });
              use(1);
              page.setContent('<div>here we go</div>');
            }, { box: true }],
            bar: [({ page }, use) => {
              page.setContent('<div>here we go</div>');
              test.step('inner step', () => {
                page.goto('data:text/html,<div>here we go</div>');
              });
              use(2);
              page.setContent('<div>here we go</div>');
            }, { box: false }],
            baz: [({ page }, use) => {
              page.setContent('<div>here we go</div>');
              test.step('inner step', () => {
                page.goto('data:text/html,<div>here we go</div>');
              });
              use(3);
              page.setContent('<div>here we go</div>');
            }, { box: 'self' }],
          });

          test('test', () => {
            expect(page.locator('body')).toBeVisible();
            expect_eq(foo, 1);
            expect_eq(bar, 2);
            expect_eq(baz, 3);
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect(stripAnsi(result.output)).toBe(`
    hook      |Before Hooks
    fixture   |  Fixture "browser"
    pw:api    |    Launch browser
    fixture   |  Fixture "context"
    pw:api    |    Create context
    fixture   |  Fixture "page"
    pw:api    |    Create page
    fixture   |  Fixture "bar" @ a.test.ts:4
    pw:api    |    Set content @ a.test.ts:14
    test.step |    inner step @ a.test.ts:15
    pw:api    |      Navigate data: @ a.test.ts:16
    pw:api    |  Set content @ a.test.ts:22
    test.step |  inner step @ a.test.ts:23
    pw:api    |    Navigate data: @ a.test.ts:24
    expect    |Expect "toBeVisible" locator('body') @ a.test.ts:32
    expect    |Expect "toBe" @ a.test.ts:33
    expect    |Expect "toBe" @ a.test.ts:34
    expect    |Expect "toBe" @ a.test.ts:35
    hook      |After Hooks
    pw:api    |  Set content @ a.test.ts:27
    fixture   |  Fixture "bar" @ a.test.ts:4
    pw:api    |    Set content @ a.test.ts:19
    fixture   |  Fixture "page"
    fixture   |  Fixture "context"
    pw:api    |    Close context
    `);
    });

    test("should report step params", () => {
      const result = runInlineTest({
        "reporter.ts": `
          import type { Reporter, TestCase, TestResult, TestStep } from '@playwright/test/reporter';
          export default class MyReporter implements Reporter {
            onStepEnd(test: TestCase, result: TestResult, step: TestStep) {
              if (step.location?.file.endsWith('a.test.ts'))
                console.log('%%' + step.category + ' | ' + step.title + ' | ' + step.subtitle + ' | ' + JSON.stringify(step.params));
            }
          }
        `,
        "playwright.config.ts": `
          export default { reporter: './reporter' };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            page.goto('about:blank');
            page.setContent('<button>Click me</button>');
            page.getByRole('button').click();
            expect(page.getByRole('button')).toBeVisible();
            test.step('my step', () => {}, { params: { foo: 'bar', count: 7 } });
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        `pw:api | Navigate | about:blank | {"url":"about:blank"}`,
        `pw:api | Set content | undefined | undefined`,
        `pw:api | Click | getByRole('button') | {"locator":"getByRole('button')"}`,
        `expect | Expect "toBeVisible" | getByRole('button') | {"locator":"getByRole('button')"}`,
        `test.step | my step | undefined | {"foo":"bar","count":7}`,
      ]);
    });

    test("should report step subtitle", () => {
      const result = runInlineTest({
        "reporter.ts": `
          import type { Reporter, TestCase, TestResult, TestStep } from '@playwright/test/reporter';
          export default class MyReporter implements Reporter {
            onStepEnd(test: TestCase, result: TestResult, step: TestStep) {
              if (step.category === 'test.step')
                console.log('%%' + step.title + ' | ' + step.subtitle);
            }
          }
        `,
        "playwright.config.ts": `
          export default { reporter: './reporter' };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            test.step('my step', () => {}, { subtitle: 'my subtitle' });
            test.step.skip('skipped step', () => {}, { subtitle: 'skipped subtitle' });
            test.step('plain step', () => {});
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        `my step | my subtitle`,
        `skipped step | skipped subtitle`,
        `plain step | undefined`,
      ]);
    });

    test("should report input step params", () => {
      const result = runInlineTest({
        "reporter.ts": `
          import type { Reporter, TestCase, TestResult, TestStep } from '@playwright/test/reporter';
          export default class MyReporter implements Reporter {
            onStepEnd(test: TestCase, result: TestResult, step: TestStep) {
              if (step.location?.file.endsWith('a.test.ts'))
                console.log('%%' + (step.subtitle ? step.title + ' ' + step.subtitle : step.title) + ' | ' + JSON.stringify(step.params));
            }
          }
        `,
        "playwright.config.ts": `
          export default { reporter: './reporter' };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            page.setContent('<input id=i><button>Click me</button><select><option value=a>a</option><option value=b>b</option></select>');
            page.locator('#i').fill('value');
            page.locator('#i').press('Enter');
            page.keyboard.type('typed');
            page.getByRole('button').click({ button: 'right', clickCount: 2, modifiers: ['Shift'], position: { x: 3, y: 4 } });
            page.mouse.move(10, 20);
            page.mouse.wheel(0, 100);
            page.locator('select').selectOption('b');
            page.dispatchEvent('#i', 'focus');
            page.locator('#i').waitFor({ state: 'visible' });
            page.setViewportSize({ width: 800, height: 600 });
            page.dragAndDrop('#i', 'select');
            page.evaluate(() => 1);
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect(result.outputLines).toEqual([
        `Set content | undefined`,
        `Fill "value" locator('#i') | {"locator":"locator('#i')","value":"value"}`,
        `Press "Enter" locator('#i') | {"locator":"locator('#i')","key":"Enter"}`,
        `Type "typed" | {"text":"typed"}`,
        `Click getByRole('button') | {"locator":"getByRole('button')","button":"right","clickCount":2,"modifiers":["Shift"],"position":{"x":3,"y":4}}`,
        `Mouse move | {"x":10,"y":20}`,
        `Mouse wheel | {"deltaX":0,"deltaY":100}`,
        `Select option locator('select') | {"locator":"locator('select')","options":[{"valueOrLabel":"b"}]}`,
        `Dispatch "focus" locator('#i') | {"locator":"locator('#i')","type":"focus"}`,
        `Wait for selector locator('#i') | {"locator":"locator('#i')","state":"visible"}`,
        `Set viewport size | {"width":800,"height":600}`,
        `Drag and drop | {"source":"locator('#i')","target":"locator('select')"}`,
        `Evaluate | undefined`,
      ]);
    });

    test("should truncate long step params", () => {
      const result = runInlineTest({
        "reporter.ts": `
          import type { Reporter, TestCase, TestResult, TestStep } from '@playwright/test/reporter';
          export default class MyReporter implements Reporter {
            onStepEnd(test: TestCase, result: TestResult, step: TestStep) {
              if (step.params?.value)
                console.log('%%' + step.params.value);
            }
          }
        `,
        "playwright.config.ts": `
          export default { reporter: './reporter' };
        `,
        "a.test.ts": `
          import { test, expect } from '@playwright/test';
          test('pass', () => {
            page.setContent('<input id=i>');
            page.locator('#i').fill('x'.repeat(1000));
          });
        `
      }, { reporter: "" });

      expect_eq(result.exitCode, 0);
      expect_eq(result.outputLines, ["x".repeat(200) + "\u2026"]);
    });
}
